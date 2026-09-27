from play import *
from gdb import read_sym
syms = read_sym(os.path.join(HERE, "built.sym"))
proc, g = boot()
step = [0]
def s(name):
    shot(g, f"t3_{step[0]:02d}_{name}", 1); step[0] += 1
def sram():
    return b"".join(g.read(0x0E000000 + k, 0x80) for k in range(0, 0x8000, 0x80))
try:
    g.frames(rt.BOOT_FRAMES)
    print("enter:", rt.enter_battle(g, None))
    run = g.read(COL, 0x70)
    print("run active", run[6], "arena", run[0x69], "wins", run[8])
    g.write(COL + 0x09, bytes([2]))            # winsToReward = 2: this win is the 3rd
    g.call(0x0800D07C, syms["ColBattle_Ending"], 1)   # the battle's own victory event
    for k in range(40):
        g.frames(40)
        run = g.read(COL, 0x70)
        if k % 2 == 0:
            s(f"after_win_{k}")
        if run[0x0B] and b"COLR" in g.read(0x0E004000, 0x400):
            pass
        if run[0x0B] and k > 8:                # reward due and the next chapter is up
            break
    g.frames(200)
    s("reward_menu")
    press(g, "R", 150)
    s("reward_help")
    press(g, "B", 80)                          # close the help
    kinds = list(g.read(COL + 0xBC, 3)); print("reward kinds", kinds, "gold", int.from_bytes(g.read(COL + 0xC0, 2), "little"))
    target = kinds.index(5) if 5 in kinds else 0    # take Gold if offered
    for _ in range(target):
        press(g, "DOWN", 30)
    gold_before = int.from_bytes(g.read(COL + 0x1C, 4), "little")
    press(g, "A", 150)
    print("gold", gold_before, "->", int.from_bytes(g.read(COL + 0x1C, 4), "little"))
    for k in range(12):                         # skill/recruit follow-ups, Elite and arena notices
        s(f"flow_{k}")
        if g.read(COL + 0x0B, 1)[0] == 0 and k > 1:
            pass
        press(g, "A", 150)
        red = g.read(RED, 0x10)
        if red[0:4] != bytes(4) and rt.any_unit_placed(g):
            break
    g.frames(300)
    s("elite_map")
    run = g.read(COL, 0x70)
    print("after reward: rewardDue", run[0x0B], "gold", int.from_bytes(run[0x1C:0x20], "little"), "elite", run[0x68], "arena", run[0x69], "weather", run[0x6A])
    print("units", units(g))
    data = sram()
    i = data.find(b"COLR"); found = []
    while i >= 0:
        found.append((hex(i), data[i + 0x24])); i = data.find(b"COLR", i + 1)
    print("run copies in SRAM (offset, battlesWon):", found)
    g.frames(600)                               # let mGBA write the .sav
finally:
    proc.kill()
import shutil
shutil.copyfile(os.path.join(HERE, "run.sav"), os.path.join(HERE, "keep.sav"))
print("sav size", os.path.getsize(os.path.join(HERE, "keep.sav")))
