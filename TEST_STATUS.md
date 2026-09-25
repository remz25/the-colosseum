# COLISEUM: Test status

Honest status of every test area in the spec (§82-84). "Verified in game" means seen working in
mGBA, not only compiled.

| Area | Status | How | Last checked |
|---|---|---|---|
| ROM builds | PASS | `py -3 scripts/build.py`: tables, text, maps, assemble, header/size checks | 2026-09-25 |
| Base boots | PASS (in game) | Unmodified Skill System test map reached in mGBA via the automated driver | 2026-09-25 |
| On-target unit tests | PASS (8 run-state + 33 map) | `py -3 scripts/build.py --test && py -3 tests/run_tests.py` (boots New Game into the arena first) | 2026-09-25 |
| Run state: new run, floor sequence (B1-3, E, B4-6, E, B7-9, Boss, next floor), 3-win rewards, Recover charges and reset, gold limits | PASS | unit tests 0-4 | 2026-09-25 |
| Save: run state through SRAM (chunk functions) | PASS | unit test 5 | 2026-09-25 |
| Save: WriteGameSave/ReadGameSave carry the run state | PASS | run_tests.py integration | 2026-09-25 |
| Save: suspend (WriteSuspendSave/ReadSuspendSave keep the run state) | PASS | run_tests.py integration | 2026-09-25 |
| RAM block unused by the game | PARTIAL | all zero after boot; only the 64-byte run state used after battle load + combat tests; a whole played battle not yet automated | 2026-09-25 |
| Battle chapter: New Game -> arena, 3v3 placement | PASS (in game + runner) | mGBA driver screenshots; run_tests.py battle stage | 2026-09-25 |
| Win -> run state (1 win, HP kept) -> save menu -> next fight | PASS (in game) | enemies removed by memory poke, unit waits, next fight loads with new enemies | 2026-09-25 |
| 20-turn limit: popups 15/18/19, game over after 20 | PASS (in game) | turn counter poked, turns ended, screenshots ("FINAL TURN.", "Time is up.", GAME OVER) | 2026-09-25 |
| Combat formulas: attack, defense, AS, hit, avoid, crit, effective rates, damage | PASS | combat test 0 | 2026-09-25 |
| Weapon triangle (+-15 hit, +-1 dmg), terrain (forest), doubling (AS 4 yes / 3 no, both sides), weight vs Con, Str/Mag split, zero damage, kill ends battle, crit x3 (real RNG battles) | PASS | combat tests 1-5 | 2026-09-25 |
| Enemy AI | PARTIAL | vanilla charge AI seen attacking in game; no automated AI tests yet | 2026-09-25 |
| Test runner detects failures | PASS | deliberate failure reported with its line, exit code 1 | 2026-09-25 |
| Units: pool (15 distinct chars, distinct personal skills, base level 5); all 15 load at Lv 5 / EXP 0 / level-5 bases / usable weapon | PASS | map tests 6-7 | 2026-09-25 |
| Level cap 30 (EXP past 20, disabled at 30) | PASS | map test 8 | 2026-09-25 |
| No stat caps (gains past class caps and Luck 30; 127 ceiling) | PASS | map test 9 (fails when the cap hooks are removed) | 2026-09-25 |
| Stats above 31 through suspend and game save | PASS | run_tests.py integration | 2026-09-25 |
| Level-up choices: 3 options, all stats possible, duplicates, +1 applied, one per level, dead excluded | PASS | map tests 10-11 | 2026-09-25 |
| Level-up choice menu in game (player phase) | PASS (in game) | Lute Lv5->6: growth screen, menu "+1 Res / +1 Mag / +1 Skl", Res 7->8 | 2026-09-25 |
| Level-up choice after an enemy-phase level-up | NOT VERIFIED | same function via the turn event; not played through | |
| Recruitment: 3 distinct never-recruited candidates, fewer when few are left, none when all used | PASS | map test 12 | 2026-09-25 |
| Recruit into an empty slot: hidden reserve at roster level, never twice | PASS | map test 13 | 2026-09-25 |
| Full roster: replace; the replaced unit leaves (not dead, never again); deployment refilled | PASS | map test 14 | 2026-09-25 |
| Recruit build: level-5 bases + average growth; level 30 cap | PASS | map test 15 | 2026-09-25 |
| Deployment: exactly 3 of 5, refusals keep the old deployment, empty slots refused, 2v3 | PASS | map test 16 | 2026-09-25 |
| Run lost: run cleared, game save invalidated | PASS | map test 17 | 2026-09-25 |
| New Game clears the run (InitPlayConfig hook), play state reset as vanilla | PASS | map test 18 (fails without the hook) | 2026-09-25 |
| Team screen, recruit menu, deployment menu in game | PASS (in game) | scripted playthrough with screenshots: team shown, reward -> recruit -> 4 in roster -> deploy toggles -> battle with the chosen 3 | 2026-09-25 |
| Game over from real deaths ends the run and invalidates the save | PASS (in game) | enemies kill all 3 deployed units (real battles): FE8 GAME OVER, run inactive, save slot 2 invalid | 2026-09-25 |
| No durability: all weapons and staves indestructible, uses unchanged after a real battle; Vulnerary still consumed | PASS | map test 19 | 2026-09-25 |
| Class skill learned into slot 1, not implicit (gone when the slot is replaced) | PASS | map test 20 | 2026-09-25 |
| 3 slots: 4th needs replacement; Skill System adder stops at 3 (fails with the patch reverted); personal untouched | PASS | map test 21 | 2026-09-25 |
| No duplicate skills (slot or personal) | PASS | map test 22 | 2026-09-25 |
| Prerequisites: bow, mounted, promoted, stat minimum, enemy-only | PASS | map test 23 | 2026-09-25 |
| Catalog: valid IDs, 5 rarities present, no duplicates, names | PASS | map test 24 | 2026-09-25 |
| Offers: 3 distinct learnable, rarity-weighted (C > R > L > 0 over 400 rolls) | PASS | map test 25 | 2026-09-25 |
| New run clears skills | PASS | map test 26 | 2026-09-25 |
| Enemy skills: floor-gated by level, class skill via class list | PASS | map test 27 | 2026-09-25 |
| "Learn a skill" menu in game: 3 slots with icons, replace slot 2 (Vantage -> Luna) | PASS (in game) | scripted, screenshots; side windows closed first (icon VRAM) | 2026-09-25 |
| Fusion recipes: 20+, same type, never lower rank, order-independent, none for non-weapons | PASS | map test 28 | 2026-09-25 |
| Fuse: both consumed, result in first slot, no auto-fusion, unique Killing Edge + Keen Edge | PASS | map test 29 | 2026-09-25 |
| Transfer: weapon type compatibility (any rank), staves, items to anyone, full inventory, not to self | PASS | map test 30 | 2026-09-25 |
| Proficiency x3 (every weapon/staff giving weapon EXP gives >= 3, multiple of 3) | PASS | map test 31 | 2026-09-25 |
| Elite variants, unique fusion, boss weapons: weapons, unbreakable, named, boss ones unsellable and strongest | PASS | map test 32 | 2026-09-25 |
| `%` / modulo correct (signed and unsigned) | PASS | run-state test 7 | 2026-09-25 |
| Prepare / Transfer / Fuse menus in game | PASS (in game) | scripted: Prepare, Transfer (Vulnerary to unit 2), Fuse (2x Iron Sword -> Steel Sword); fixed text-tile exhaustion across chained menus (ResetTextFont before each menu) | 2026-09-25 |
| Relics, Shop, Legacy | NOT STARTED | | |
| Integration run (§83) | NOT STARTED | | |
