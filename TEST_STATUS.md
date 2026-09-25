# COLISEUM: Test status

Honest status of every test area in the spec (§82-84). "Verified in game" means seen working in
mGBA, not only compiled.

| Area | Status | How | Last checked |
|---|---|---|---|
| ROM builds | PASS | `py -3 scripts/build.py`: tables, text, maps, assemble, header/size checks | 2026-09-25 |
| Base boots | PASS (in game) | Unmodified Skill System test map reached in mGBA via the automated driver | 2026-09-25 |
| On-target unit tests | PASS (7 run-state + 6 combat) | `py -3 scripts/build.py --test && py -3 tests/run_tests.py` (boots New Game into the arena first) | 2026-09-25 |
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
| Units, Skills, Relics, Weapons, Shop, Legacy | NOT STARTED | | |
| Integration run (§83) | NOT STARTED | | |
