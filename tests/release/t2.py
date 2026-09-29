from play import *
proc, g = boot()
step = [0]
def s(name, scale=1):
    shot(g, f"t2_{step[0]:02d}_{name}", scale); step[0] += 1
try:
    g.frames(rt.BOOT_FRAMES)
    for i, (wait, key, hold) in enumerate(rt.TITLE_KEYS):
        g.frames(wait)
        if i == 1:
            s("title")
        g.frames(hold, key)
    for _ in range(rt.MENU_A_PRESSES):
        g.frames(85)
        ram = g.read(COL, 0x40)
        if ram[0:4] == b"COLR" and ram[6] == 1 and ram[0x0D] == 3:
            break
        g.frames(5, "A")
    g.frames(120)
    s("team")
    press(g, "A", 220)                # Begin -> Prepare
    s("prepare")
    press(g, "DOWN", 40)
    press(g, "A", 150)                # Shop
    s("shop")
    press(g, "DOWN", 40)
    press(g, "R", 150)
    s("shop_help")
    press(g, "R", 40)                 # close help
    press(g, "B", 150)                # leave shop
    s("prepare2")
    for _ in range(5):
        press(g, "DOWN", 30)          # Recover, Transfer, Fuse, Relics
    s("prepare_relics")
    press(g, "A", 150)                # relic unit list
    s("relic_units")
    press(g, "A", 150)                # first unit -> slots
    s("relic_slots")
    press(g, "A", 150)                # slot 1 -> choices
    s("relic_choices")
    press(g, "DOWN", 40)
    press(g, "DOWN", 40)              # the 3rd bag relic (the Rare)
    press(g, "R", 150)
    s("relic_choice_help")
    press(g, "R", 40)
    press(g, "A", 150)                # info screen
    s("relic_info")
    press(g, "A", 150)                # Equip
    s("relic_after")
    print("relics worn:", g.read(COL + 0x7C, 64).hex())
    press(g, "B", 120)                # back to the unit list
    press(g, "B", 120)                # back to Prepare
    s("prepare3")
    # the Prepare cursor returns to the top (Next fight) after a submenu
    s("prepare_next")
    press(g, "A", 30)
    for k in range(60):
        g.frames(30)
        red = g.read(RED, 0x10)
        if red[0:4] != bytes(4) and rt.any_unit_placed(g):
            break
    g.frames(300)
    s("battle_start")
    print("units", units(g))
    # the stat screen: R on the first unit (cursor starts on it)
    press(g, "R", 200)
    s("stat_screen")
    press(g, "B", 150)
    # combat: our unit next to an enemy, then Attack by hand
    g.write(BLUE + 0x10, bytes([7, 4]))
    g.write(RED + 0x10, bytes([8, 4]))
    g.call(0x0801A1F4); g.call(0x080271A0)
    g.call(0x08015BBC, 7, 4)
    g.frames(30)
    s("adjacent")
    press(g, "A", 60)                 # select the unit
    s("selected")
    press(g, "A", 60)                 # stay on this tile
    s("action_menu")
    press(g, "A", 60)                 # Attack
    s("weapon_menu")
    press(g, "A", 60)                 # the weapon
    s("target")
    press(g, "A", 60)                 # the target
    s("forecast")
    press(g, "A", 30)                 # confirm -> battle
    hp0 = units(g)
    for k in range(10):
        g.frames(25)
        s(f"combat{k}")
    g.frames(400)
    print("after combat", units(g), "before", hp0)
finally:
    proc.kill()
