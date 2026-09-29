"""Elite battle on the patched ROM: resume keep.sav, attack an Elite by hand, win, next fight."""
from play import *
from gdb import read_sym
syms = read_sym(os.path.join(HERE, "built.sym"))
proc, g = boot(save="keep.sav")
step = [0]
def s(name):
    shot(g, f"t5_{step[0]:02d}_{name}", 1); step[0] += 1
try:
    g.frames(rt.BOOT_FRAMES)
    for wait, key, hold in rt.TITLE_KEYS:
        g.frames(wait); g.frames(hold, key)
    g.frames(120)
    for k in range(8):
        press(g, "A", 120)
        if g.read(COL, 4) == b"COLR" and rt.any_unit_placed(g):
            break
    for k in range(20):
        g.frames(60)
        if rt.any_unit_placed(g) and g.read(RED, 4) != bytes(4):
            break
    g.frames(300)
    s("elite_start")
    # enemy info: R on an Elite (stat screen shows the Elite's skills/weapon)
    g.write(BLUE + 0x10, bytes([9, 6]))
    g.write(RED + 0x10, bytes([10, 6]))
    g.call(0x0801A1F4); g.call(0x080271A0)
    g.call(0x08015BBC, 10, 6)
    g.frames(60)
    s("on_elite")
    press(g, "R", 200)
    s("elite_stats")
    press(g, "B", 150)
    g.call(0x08015BBC, 9, 6)
    g.frames(60)
    press(g, "A", 60)                 # select Silas
    press(g, "A", 60)                 # stay
    s("action_menu")
    press(g, "A", 60)                 # Attack
    press(g, "A", 60)                 # weapon
    press(g, "A", 60)                 # target
    s("forecast")
    before = units(g)
    press(g, "A", 30)                 # fight
    for k in range(6):
        g.frames(40)
        s(f"combat{k}")
    g.frames(500)
    print("before", before)
    print("after ", units(g))
    # win the Elite battle through the battle's own victory event
    g.call(0x0800D07C, syms["ColBattle_Ending"], 1)
    for k in range(30):
        g.frames(60)
        if k % 3 == 0:
            s(f"after_win_{k}")
        run = g.read(COL, 0x70)
        if run[8] == 2 and k > 8:
            break
    for k in range(12):               # notices (drop, arena) until the next battle is placed
        press(g, "A", 150)
        s(f"flow_{k}")
        red = g.read(RED, 0x10)
        if red[0:4] != bytes(4) and rt.any_unit_placed(g):
            break
    g.frames(300)
    s("next_battle")
    run = g.read(COL, 0x70)
    print("wins", run[8], "gold", int.from_bytes(run[0x1C:0x20], "little"), "elite", run[0x68],
          "eliteDue", run[0x0A], "arena", run[0x69], "weather", run[0x6A])
    print("units", units(g))
    print("inventory Silas", g.read(BLUE + 0x1E, 10).hex())
finally:
    proc.kill()
