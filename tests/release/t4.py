"""Save -> quit -> Continue on the patched ROM: boot with keep.sav (autosave after 1 win, Elite next)."""
from play import *


proc, g = boot(save="keep.sav")
step = [0]
def s(name):
    shot(g, f"t4_{step[0]:02d}_{name}", 1); step[0] += 1
try:
    g.frames(rt.BOOT_FRAMES)
    for wait, key, hold in rt.TITLE_KEYS:
        g.frames(wait); g.frames(hold, key)
    g.frames(120)
    s("title_menu")
    for k in range(8):
        press(g, "A", 120)
        s(f"menu_{k}")
        run = g.read(COL, 0x70)
        if run[0:4] == b"COLR" and run[6] == 1 and rt.any_unit_placed(g):
            break
    for k in range(20):
        g.frames(60)
        if rt.any_unit_placed(g) and g.read(RED, 4) != bytes(4):
            break
    g.frames(300)
    s("resumed")
    run = g.read(COL, 0x70)
    print("magic", run[0:4], "active", run[6], "wins", run[8], "gold", int.from_bytes(run[0x1C:0x20], "little"),
          "elite", run[0x68], "arena", run[0x69], "weather", run[0x6A])
    print("units", units(g))
finally:
    proc.kill()
