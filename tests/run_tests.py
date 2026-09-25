#!/usr/bin/env python3
"""Run COLISEUM's on-target unit tests (src/tests/*.c) in mGBA.

    py -3 scripts/build.py --test && py -3 tests/run_tests.py

Starts a separate mGBA with its debugger on a temporary copy of Coliseum_test.gba, boots
the game into its main loop, then calls ColTest_Count() and ColTest_Run(i) for every test
through the debugger. A test returns 0 on success or the source line of its failed check.
Exit code 0 only if every test passed."""
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
WRITE_GAME_SAVE = 0x080A5011     # vanilla WriteGameSave(slot), hooked by the Expanded Modular Save
READ_GAME_SAVE = 0x080A5129      # vanilla ReadGameSave(slot)
COL_RAM = 0x0203F600             # gColRun (src/include/coliseum.h)
SLOT3_RUN_CHUNK = 0x0E000000 + 0x55D4 + 0x11F0   # SRAM: save slot 3 + COLISEUM game-save chunk


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
    return problems


def main() -> int:
    if not ROM.is_file() or not SYM.is_file():
        print("Build the test ROM first: py -3 scripts/build.py --test")
        return 2
    syms = read_sym(SYM)
    for name in ("ColTest_Count", "ColTest_Run"):
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
        count = g.call(syms["ColTest_Count"])
        print(f"Running {count} on-target test(s)")
        for i in range(count):
            r = g.call(syms["ColTest_Run"], i)
            if r == 0:
                print(f"  [PASS] test {i}")
            else:
                failures += 1
                where = "invalid test index" if r == 0xFFFFFFFF else f"check at src/tests line {r}"
                print(f"  [FAIL] test {i}: {where}")
        problems = save_integration(g, syms)
        if problems:
            failures += 1
            print("  [FAIL] save integration: " + "; ".join(problems))
        else:
            print("  [PASS] save integration: WriteGameSave/ReadGameSave keep the run state")
    finally:
        proc.terminate()
        time.sleep(0.5)
        shutil.rmtree(tmp, ignore_errors=True)
    print("ALL PASSED" if failures == 0 else f"{failures} FAILED")
    return 0 if failures == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
