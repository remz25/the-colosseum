# COLISEUM: Test status

Honest status of every test area in the spec (§82-84). "Verified in game" means seen working in
mGBA, not only compiled.

| Area | Status | How | Last checked |
|---|---|---|---|
| ROM builds | PASS | `py -3 scripts/build.py`: tables, text, maps, assemble, header/size checks | 2026-09-25 |
| Base boots | PASS (in game) | Unmodified Skill System test map reached in mGBA via the automated driver | 2026-09-25 |
| On-target unit tests | PASS (8 run-state + 38 map) | `py -3 scripts/build.py --test && py -3 tests/run_tests.py` (boots New Game into the arena first) | 2026-09-25 |
| Run state: new run, floor sequence (B1-3, E, B4-6, E, B7-9, Boss, next floor), 3-win rewards, Recover charges and reset, gold limits | PASS | unit tests 0-4 | 2026-09-25 |
| Save: run state through SRAM (chunk functions) | PASS | unit test 5 | 2026-09-25 |
| Save: WriteGameSave/ReadGameSave carry the run state | PASS | run_tests.py integration | 2026-09-25 |
| Save: suspend (WriteSuspendSave/ReadSuspendSave keep the run state) | PASS | run_tests.py integration | 2026-09-25 |
| RAM block unused by the game | PARTIAL | all zero after boot; only the 64-byte run state used after battle load + combat tests; a whole played battle not yet automated | 2026-09-25 |
| Team screen visible (screen faded in) after New Game | PASS | run_tests.py brightness check (fails without the fix) | 2026-09-25 |
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
| Shop prices: +10% compounding per purchase, 5x and 9999 caps | PASS | map test 33 | 2026-09-25 |
| Shop stock: 8 entries, every relevant category, no duplicate weapon/skill, floor 1 no B+ weapons, floor 4 better weapons, no Recruit when nobody is left | PASS | map test 34 (20 rolls) | 2026-09-25 |
| Buying: only when affordable, once per entry, prices rise after, party gold mirrored | PASS | map test 35 | 2026-09-25 |
| Team healing +50% (max HP cap), kept in the run state | PASS | map test 36 | 2026-09-25 |
| Battle gold ranges by floor and encounter | PASS | map test 37 | 2026-09-25 |
| Shop in game: skill bought (learn menu), heal bought, prices +10%/+21%, Sold rows, bow grayed for a team without bow users | PASS (in game) | scripted, screenshots | 2026-09-25 |
| Relic pool: 15 relics, rarity counts 6/5/3/1, every effect has a text, Mythic supported, rarity-weighted rolls (no Legendary/Mythic while none exist) | PASS | map test 38 (Test_RelicPool) | 2026-09-26 |
| Relic effect texts ("+5 Def", "-15% Def", "Lose 2 HP per attack", good/drawback flag) | PASS | map test 39 | 2026-09-26 |
| Equip from bag (old one back to the bag), transfer, swap, unequip, full bag refuses, duplicates stack, enemies never wear relics | PASS | map test 40 | 2026-09-26 |
| Wearer dies / is replaced -> relics back to the bag; new run clears relics | PASS | map test 41 | 2026-09-26 |
| Relics through the save chunk (SRAM); v4 save upgraded to v5 with its data kept | PASS | map test 42 | 2026-09-26 |
| Flat relic stats through the game's getters (Str Mag Spd Def Res Lck Mov; 0 floor; own Guardian's Crest gives no Def) | PASS | map test 43 | 2026-09-26 |
| Percent stats: additive, after flat, rounded to nearest (Wind Soul, Blood Pact, combinations) | PASS | map test 44 | 2026-09-26 |
| Hit / Avoid / Crit / Str relics in battle calculations, attacking and defending | PASS | map test 45 | 2026-09-26 |
| Bloodied Band: only below 50% HP (not at exactly 50%), switches off when healed | PASS | map test 46 | 2026-09-26 |
| Guardian's Crest: +3 Def for an adjacent ally vs physical, not when apart, not for the wearer, not vs magic | PASS | map test 47 (real map-unit grid) | 2026-09-26 |
| Mage's Ring / Arcane Blood only on magic attacks (+15%, +10%, stacked +25%); Fortress Heart -20% on received counters only | PASS | map test 48 | 2026-09-26 |
| Blood Pact: -2 HP per attack (counters too), never below 1 HP | PASS | map test 49 | 2026-09-26 |
| Golden Thread: +25% each (roster only), base battle gold unchanged | PASS | map test 50 | 2026-09-26 |
| Shop stocks a valid relic every time, priced by rarity | PASS | map test 51 (+ Test_ShopStock covers the category) | 2026-09-26 |
| Relic menus in game: Prepare > Relics, unit list, slots, bag list with pages ("More..."), info screen (rarity, green/gold effects, "Below 50% HP:" header), Equip, transfer from another unit, Unequip | PASS (in game) | player ROM, scripted with screenshots | 2026-09-26 |
| Shop relic in game: Relic row, info screen with Buy, bought into the bag, gold paid, "Sold", prices +10% | PASS (in game) | player ROM, screenshots | 2026-09-26 |
| Relic stats in game: stat screen shows "Spd 12 -3 / Def 4 +5" (Iron Heart); getters give Blood Pact +10% Spd (9 -> 10) | PASS (in game) | player ROM, screenshot + getter calls | 2026-09-26 |
| Blood Pact in a real battle with animation: 19 HP -> 4 (2 attacks x 2 + an 11-damage counter), HP drain shown, kept on the map | PASS (in game) | player ROM, real attack, screenshots | 2026-09-26 |
| Golden Thread at a real victory; Guardian's Crest / Fortress Heart / Mage's Ring in a played battle; Sturdy Boots on the movement range; relics after a real save + reload from the title | NOT VERIFIED in game | covered by the on-target tests above, not played through | |
| Shop stock fully random: every category over 40 rolls, layout varies (30+ of 39), at most one Recruit/Heal/Promotion, Heal 30/50/100 | PASS | map test 34 (rewritten) | 2026-09-26 |
| Elite setups: 4 Champions + 3 Squads, promoted classes, role characters, rolled once and kept | PASS | map test 52 | 2026-09-26 |
| Champion: Blademaster spawns alone, Keen Edge usable and dropped (US_DROP_ITEM), boosted HP, a relic drop | PASS | map test 53 | 2026-09-26 |
| Elite Squad (Vanguard): 3 roles, every weapon/staff usable, Mender can use Mend, a relic drop | PASS | map test 54 | 2026-09-26 |
| Normal battle: 3 generic enemies; the run's first battle has a relic drop | PASS | map test 55 | 2026-09-26 |
| Drop odds: first battle always a relic; ~25% per normal enemy, gold > skill, relics appear; every Elite a relic | PASS | map test 56 | 2026-09-26 |
| Killing blow recorded in real battles only (not forecasts); drop claimable only once its enemy is dead | PASS | map test 57 | 2026-09-26 |
| In game: "ELITE BATTLE! / Elite Squad: Vanguard" announcement, random shop, squad on the map ("Guardian", "Reaper" names), relic drop notice after a kill (relic in the bag), skill drop notice ("Skill: Pursuit / Lute can learn it") then the learn menu, stat choice after the notice | PASS (in game) | test ROM, scripted with screenshots | 2026-09-26 |
| Champion fight played through; Mender actually healing; Champion weapon drop in game; gold drop notice; drop claimed from an enemy-phase kill | NOT VERIFIED in game | covered by the on-target tests where possible | |
| All deployed units dead -> run over: enemy-phase deaths (3 units, and 3 + a reserve), last unit killed by a counter on the player phase | PASS (in game) | test ROM, scripted; each reached FE8's GAME OVER and cleared the run | 2026-09-26 |
| Turn-start safety net (no blue unit fighting on the map -> run lost + GAME OVER; no false trigger on turn 1) | PASS (in game) + map test 58 | event run directly with every deployed unit dead | 2026-09-26 |
| Developer's report "battle went on to turn 20 after everyone died" (reserves in the roster) | FIXED (in game) | cause: FE8 clears the reserves' not-deployed flag after the beginning event, so its unit count included hidden reserves; CountAvailableBlueUnits replaced. Reproduced with a reserve recruited before the battle, then fixed: GAME OVER right after the enemy phase | 2026-09-26 |
| Original characters: name, class, portrait, personal skill, female flag (Selene only), loadable at level 5 | PASS | map test 59 + pool tests | 2026-09-26 |
| Portrait table entries for the 5 sheets hold the converted data (build step) | PASS | scripts/portraits.py verify_rom | 2026-09-26 |
| v4 and v5 saves upgrade to v6 with every field kept (byte-built images) | PASS | map test 42 | 2026-09-26 |
| In game: Morrow / Hale / Selene deployed - unit window portrait, stat screens (portrait, name, class, skill icons), map sprites; Selene's Dance refreshed Morrow (with animation, EXP); Selene's new death quote with her portrait | PASS (in game) | player ROM, scripted with screenshots | 2026-09-26 |
| Silas and Idris seen in game; Morrow's Pickup; Hale's Deadeye sleep | NOT VERIFIED in game | data checked by tests | |
| First Elite gentler (levels, boost), starting relics (2 Common + 1 Rare, distinct Commons), EXP x1.5 for player units through the real EXP loop | PASS | map tests 60-62; full suite 8 run-state + 63 map tests all pass | 2026-09-27 |
| "Your team" screen shows the 3 starting relics (Rare in gold), faded in, cursor on Begin | PASS (in game) | player ROM, scripted screenshot | 2026-09-27 |
| First Elite difficulty and EXP pace feel right | NOT VERIFIED | needs the developer's play-test | |
| Arenas: chapter data per arena/weather, pool rolls, hazard damage (cap at 1 HP, poison adds, fliers immune), sacred heal via the Skill System loop, AI attack score and move filter, weather Hit both sides | PASS | map tests 63-68 | 2026-09-27 |
| Arenas: every map 15x10, spawns walkable for 6 classes in every rolled weather, not on hazards, foot + armour paths between the sides | PASS | tests/check_arenas.py (run by run_tests.py) | 2026-09-27 |
| In game: all 8 arenas load with their tileset, weather animation (rain, sandstorm, embers), fog vision 3, spawns; arena notice after a real win (next arena rolled); hot rock 4 -> 1 HP and 15 -> 10 at the phase start; seal 3 -> 4 HP; enemies stop short of hot rock; Arena debug menu restarts in Volcanic/Ashfall | PASS (in game) | player and test ROM, scripted with screenshots | 2026-09-27 |
| Resuming a suspended battle in a rolled arena | NOT VERIFIED in game | run state round trip is tested | |
| Legacy | NOT STARTED | | |
| Integration run (§83) | NOT STARTED | | |
