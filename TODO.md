# COLISEUM: TODO

Phases from the spec (§86). A feature is done only when it meets the Definition of Done (§91):
built, working in game, tested, documented.

## Phase 1: Repository analysis — DONE (2026-09-25)
- [x] Inspect the Skill System buildfile, systems, tools, limits -> `ARCHITECTURE.md`
- [x] Plan presented and approved (decisions in `GAME_DESIGN.md`)

## Phase 2: Technical foundation — IN PROGRESS
- [x] Own repository in `Downloads\Coliseum`, upstream remote kept as `upstream`
- [x] Checked build script `scripts/build.py`; unmodified base boots to its test map in mGBA
- [x] Documentation set (this file, GAME_DESIGN, ARCHITECTURE, CHANGELOG, TEST_STATUS, BALANCE_NOTES)
- [x] Config: Str/Mag split on; debug build variant (`--debug` -> `Coliseum_debug.gba`)
- [x] C build for `src/` (Arm GNU Toolchain + lyn, FE-CLib headers); objects may not use RAM variables
- [x] Test framework: **on-target unit tests** (`--test` build + `tests/run_tests.py` calls each test
      through mGBA's debugger; no host compiler needed). Proven to report failures with line numbers.
- [x] COLISEUM RAM block `0x0203F600-0x0203FDFF` (untouched after boot: checked by run_tests.py)
- [x] Run-state structure (`src/include/coliseum.h`): floor, encounter schedule, wins, 3-win reward,
      gold, Recover charges, roster/deployed/dead/recruited, seed, history
- [x] Run state saved in the game save (and suspend chunk declared): SRAM round trip and
      WriteGameSave/ReadGameSave integration pass
- [ ] Suspend round trip test (chunk is declared; not yet exercised by a test)
- [ ] RAM block untouched during a full battle (only checked after boot so far)
- [ ] Legacy save module that survives a new run, with tests
- [ ] Debug menu: first COLISEUM commands (give gold, set floor/fight, reset run)

## Phase 3-17
Not started. See `docs/COLISEUM_SPEC.md` §86 for the list.
