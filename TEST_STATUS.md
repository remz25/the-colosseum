# COLISEUM: Test status

Honest status of every test area in the spec (§82-84). "Verified in game" means seen working in
mGBA, not only compiled.

| Area | Status | How | Last checked |
|---|---|---|---|
| ROM builds | PASS | `py -3 scripts/build.py`: tables, text, maps, assemble, header/size checks | 2026-09-25 |
| Base boots | PASS (in game) | Unmodified Skill System test map reached in mGBA via the automated driver | 2026-09-25 |
| On-target unit tests | PASS (6/6) | `py -3 scripts/build.py --test && py -3 tests/run_tests.py` | 2026-09-25 |
| Run state: new run, floor sequence (B1-3, E, B4-6, E, B7-9, Boss, next floor), 3-win rewards, Recover charges and reset, gold limits | PASS | unit tests 0-4 | 2026-09-25 |
| Save: run state through SRAM (chunk functions) | PASS | unit test 5 | 2026-09-25 |
| Save: WriteGameSave/ReadGameSave carry the run state | PASS | run_tests.py integration | 2026-09-25 |
| Save: suspend | NOT TESTED | chunk declared only | |
| RAM block unused by the game | PARTIAL | all zero after boot; not yet checked through a battle | 2026-09-25 |
| Test runner detects failures | PASS | deliberate failure reported with its line, exit code 1 | 2026-09-25 |
| Combat, Units, Skills, Relics, Weapons, Shop, Legacy | NOT STARTED | | |
| Integration run (§83) | NOT STARTED | | |
