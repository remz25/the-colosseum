"""Win fights (the battle's own victory event) until a Desert (2) or Volcanic (3) arena comes up."""
from play import *
from gdb import read_sym
syms = read_sym(os.path.join(HERE, "built.sym"))
proc, g = boot()
step = [0]
def s(name):
    shot(g, f"t6_{step[0]:02d}_{name}", 1); step[0] += 1
try:
    g.frames(rt.BOOT_FRAMES)
    print("enter:", rt.enter_battle(g, None))
    for fight in range(14):
        run = g.read(COL, 0x70)
        print("fight", fight, "floor", run[7], "wins", run[8], "battlesWon", int.from_bytes(run[0x24:0x28], "little"),
              "elite", run[0x68], "arena", run[0x69], "weather", run[0x6A], "gold", int.from_bytes(run[0x1C:0x20], "little"))
        if run[0x69] in (2, 3):
            g.frames(200)
            s(f"arena{run[0x69]}_map")
            g.call(0x08015BBC, 7, 4); g.frames(60)
            s(f"arena{run[0x69]}_mid")
            break
        won = int.from_bytes(run[0x24:0x28], "little")
        g.call(0x0800D07C, syms["ColBattle_Ending"], 1)
        for k in range(40):
            g.frames(60)
            if int.from_bytes(g.read(COL + 0x24, 4), "little") > won and k > 8:
                break
        for k in range(20):
            press(g, "A", 150)
            if k == 0:
                s(f"fight{fight}_menu")
            red = g.read(RED, 0x10)
            if red[0:4] != bytes(4) and rt.any_unit_placed(g):
                break
        g.frames(300)
finally:
    proc.kill()
