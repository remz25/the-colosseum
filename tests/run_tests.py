#!/usr/bin/env python3
"""Run COLISEUM's on-target unit tests (src/tests/*.c) in mGBA.

    py -3 scripts/build.py --test && py -3 tests/run_tests.py

Starts a separate mGBA with its debugger on a temporary copy of Coliseum_test.gba (fresh save
file), boots the game, starts a New Game into the COLISEUM battle chapter, then calls the
on-target tests through the debugger: ColTest_Run(i) (run state) and ColTest_MapRun(i)
(combat, needs the battle map). A test returns 0 on success or the source line of its failed
check. Exit code 0 only if every test passed."""
import shutil
import sys
import tempfile
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent / "emu"))
from gdb import Gdb, read_sym, start_mgba   # noqa: E402

ROOT = Path(__file__).resolve().parent.parent
ROM = ROOT / "Coliseum_test.gba"
SYM = ROOT / "Coliseum_test.sym"
BOOT_FRAMES = 120
# Title -> New Game -> the battle chapter on a fresh save: START past the intro and title, then
# A through the menus (the boot menu, New Game, ...) until the beginning event starts the run.
TITLE_KEYS = [(180, "START", 5), (120, "START", 5)]
MENU_A_PRESSES = 20
WRITE_GAME_SAVE = 0x080A5011     # vanilla WriteGameSave(slot), hooked by the Expanded Modular Save
READ_GAME_SAVE = 0x080A5129      # vanilla ReadGameSave(slot)
COL_RAM = 0x0203F600             # gColRun (src/include/coliseum.h)
SLOT3_RUN_CHUNK = 0x0E000000 + 0x55D4 + 0x11F0   # SRAM: save slot 3 + COLISEUM game-save chunk
WRITE_SUSPEND = 0x080A5A49       # vanilla WriteSuspendSave(id), hooked by the Expanded Modular Save
READ_SUSPEND = 0x080A5C15        # vanilla ReadSuspendSave(id)
SAVE_ID_SUSPEND = 3
SUSPEND_RUN_CHUNK = 0x0E000000 + 0x00D4 + 0x290E  # SRAM: suspend block + COLISEUM suspend chunk


def save_integration(g, syms) -> list[str]:
    """The game's own save and load routines carry the run state (chunk in ExModularSave.event)."""
    problems = []
    g.call(syms["Col_RunNew"], 0xC0FFEE)
    g.call(syms["Col_AddGold"], 1234)
    g.call(WRITE_GAME_SAVE, 2)
    chunk = g.read(SLOT3_RUN_CHUNK, 0x40)
    if chunk[0:4] != b"COLR":
        problems.append(f"save slot 3 has no run state (magic {chunk[0:4].hex()})")
    elif int.from_bytes(chunk[0x1C:0x20], "little") != 1234:
        problems.append("saved gold is wrong")
    g.call(syms["Col_RunClear"])
    g.call(READ_GAME_SAVE, 2)
    ram = g.read(COL_RAM, 0x40)
    if ram[0:4] != b"COLR" or ram[6] != 1 or int.from_bytes(ram[0x1C:0x20], "little") != 1234             or int.from_bytes(ram[0x20:0x24], "little") != 0xC0FFEE:
        problems.append(f"run state not restored by ReadGameSave: {ram[:0x24].hex()}")

    # suspend: a different gold value so the two paths can't be confused
    g.call(syms["Col_RunNew"], 0x5EED)
    g.call(syms["Col_AddGold"], 777)
    g.call(WRITE_SUSPEND, SAVE_ID_SUSPEND)
    chunk = g.read(SUSPEND_RUN_CHUNK, 0x40)
    if chunk[0:4] != b"COLR" or int.from_bytes(chunk[0x1C:0x20], "little") != 777:
        problems.append(f"suspend has no/wrong run state: {chunk[:0x20].hex()}")
    g.call(syms["Col_RunClear"])
    g.call(READ_SUSPEND, SAVE_ID_SUSPEND)
    ram = g.read(COL_RAM, 0x40)
    if ram[0:4] != b"COLR" or int.from_bytes(ram[0x1C:0x20], "little") != 777             or int.from_bytes(ram[0x20:0x24], "little") != 0x5EED:
        problems.append(f"run state not restored by ReadSuspendSave: {ram[:0x24].hex()}")
    return problems


BLUE_UNIT_1 = 0x0202BE4C        # first blue unit (struct Unit, 0x48 bytes)


def high_stat_saves(g) -> list[str]:
    """Spec 21 (no stat caps): stats far above vanilla's 5-bit save fields (max 31) survive the
    suspend and the game save. Needs a unit on the map; restores it afterwards."""
    problems = []
    saved = g.read(BLUE_UNIT_1, 0x48)
    stats = bytes([80, 80, 60, 45, 50, 40, 35, 45])        # maxHP curHP Str Skl Spd Def Res Lck
    g.write(BLUE_UNIT_1 + 8, bytes([25]))
    g.write(BLUE_UNIT_1 + 0x12, stats)
    g.write(BLUE_UNIT_1 + 0x3A, bytes([70]))                # Mag (Str/Mag split)
    for name, write, read, arg in [("suspend", WRITE_SUSPEND, READ_SUSPEND, SAVE_ID_SUSPEND),
                                   ("game save", WRITE_GAME_SAVE, READ_GAME_SAVE, 2)]:
        g.call(write, arg)
        g.write(BLUE_UNIT_1 + 8, bytes([1]))
        g.write(BLUE_UNIT_1 + 0x12, bytes(8))
        g.write(BLUE_UNIT_1 + 0x3A, bytes([0]))
        g.call(read, arg)
        u = g.read(BLUE_UNIT_1, 0x48)
        if u[8] != 25 or u[0x12:0x1A] != stats or u[0x3A] != 70:
            problems.append(f"{name}: level {u[8]}, stats {list(u[0x12:0x1A])}, mag {u[0x3A]}")
    g.write(BLUE_UNIT_1, saved)
    return problems


def any_unit_placed(g) -> bool:
    """A blue unit exists and is on the map (not hidden / undeployed): the battle has begun."""
    for i in range(5):
        u = g.read(BLUE_UNIT_1 + 0x48 * i, 0x10)
        if u[0:4] != bytes(4) and not (int.from_bytes(u[0x0C:0x10], "little") & 0x9):
            return True
    return False


def enter_battle(g, syms) -> str | None:
    """New Game into the battle chapter; returns a problem description or None."""
    for wait, key, hold in TITLE_KEYS:
        g.frames(wait)
        g.frames(hold, key)
    for _ in range(MENU_A_PRESSES):
        g.frames(85)
        ram = g.read(COL_RAM, 0x40)
        if ram[0:4] == b"COLR" and ram[6] == 1 and ram[0x0D] == 3:   # active, 3 in the roster
            # "Your team" (cursor on Begin), then Prepare (cursor on Fight!): A until units are placed
            for _ in range(10):
                g.frames(5, "A")
                g.frames(60)
                if any_unit_placed(g):
                    g.frames(120)                                # beginning event finishes
                    return None
            return "the units never appeared after the team screen"
        g.frames(5, "A")
    return f"the battle chapter did not start a run (run state {g.read(COL_RAM, 0x10).hex()})"


def main() -> int:
    if not ROM.is_file() or not SYM.is_file():
        print("Build the test ROM first: py -3 scripts/build.py --test")
        return 2
    syms = read_sym(SYM)
    for name in ("ColTest_Count", "ColTest_Run", "ColTest_MapCount", "ColTest_MapRun"):
        if name not in syms:
            print(f"{name} not in {SYM.name}: was the test build made with --test?")
            return 2

    tmp = Path(tempfile.mkdtemp(prefix="coliseum_test_"))
    rom = tmp / "t.gba"
    shutil.copyfile(ROM, rom)
    proc = start_mgba(str(rom))
    failures = 0
    try:
        g = Gdb()
        g.frames(BOOT_FRAMES)
        block = g.read(COL_RAM, 0x800)
        if any(block):
            failures += 1
            print("  [FAIL] RAM block 0x0203F600-0x0203FDFF was written by the game before COLISEUM code ran")
        else:
            print("  [PASS] RAM block untouched after boot (0x0203F600-0x0203FDFF all zero)")

        problem = enter_battle(g, syms)
        if problem:
            print(f"  [FAIL] {problem}")
            return 1
        print("  [PASS] New Game starts a run and loads the battle chapter")
        block = g.read(COL_RAM, 0x800)
        if any(block[0x100:0x200]) or any(block[0x280:]):
            failures += 1
            print("  [FAIL] RAM block outside the run state and UI scratch was written during the battle")
        else:
            print("  [PASS] RAM block outside the run state (0x000-0x0FF) and UI scratch (0x200-0x27F) untouched in battle")

        count = g.call(syms["ColTest_MapCount"])
        print(f"Running {count} map test(s) (combat, units) on the battle map")
        for i in range(count):
            r = g.call(syms["ColTest_MapRun"], i)
            if r == 0:
                print(f"  [PASS] map test {i}")
            else:
                failures += 1
                where = {0xFFFFFFFF: "invalid test index", 0xFFFFFFFE: "no units on the map"}.get(
                    r, f"check at src/tests/test_combat.c / test_units.c / test_roster.c / test_skills.c / test_weapons.c / test_shop.c line {r}")
                print(f"  [FAIL] map test {i}: {where}")

        problems = high_stat_saves(g)
        if problems:
            failures += 1
            print("  [FAIL] high stats through saves: " + "; ".join(problems))
        else:
            print("  [PASS] level 25 / stats up to 80 survive suspend and game save")

        count = g.call(syms["ColTest_Count"])
        print(f"Running {count} run-state test(s)")
        for i in range(count):
            r = g.call(syms["ColTest_Run"], i)
            if r == 0:
                print(f"  [PASS] test {i}")
            else:
                failures += 1
                where = "invalid test index" if r == 0xFFFFFFFF else f"check at src/tests/test_run_state.c line {r}"
                print(f"  [FAIL] test {i}: {where}")
        problems = save_integration(g, syms)
        if problems:
            failures += 1
            print("  [FAIL] save integration: " + "; ".join(problems))
        else:
            print("  [PASS] save integration: game save and suspend both keep the run state")
    finally:
        proc.terminate()
        time.sleep(0.5)
        shutil.rmtree(tmp, ignore_errors=True)
    print("ALL PASSED" if failures == 0 else f"{failures} FAILED")
    return 0 if failures == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
