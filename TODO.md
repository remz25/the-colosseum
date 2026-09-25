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
- [ ] Config: Str/Mag split on; debug build variant (`__DEBUG__`) as a separate ROM
- [ ] C build for `src/` (Arm GNU Toolchain + lyn) and a native C unit-test runner
- [ ] Emulator test driver in the repo (`tests/emu/`: scripted input, screenshots, RAM checks)
- [ ] Free RAM map (EWRAM/IWRAM areas safe for COLISEUM data)
- [ ] Run-state data structure + IDs (floor, fight, wins, gold, Recover charges, roster, dead list)
- [ ] Save module for the run state (save, load, suspend) with tests
- [ ] Legacy save module that survives a new run, with tests
- [ ] Debug menu: first COLISEUM commands (give gold, set floor/fight, reset run)

## Phase 3-17
Not started. See `docs/COLISEUM_SPEC.md` §86 for the list.
