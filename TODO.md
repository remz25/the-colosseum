# COLISEUM: TODO

Phases from the spec (§86). A feature is done only when it meets the Definition of Done (§91):
built, working in game, tested, documented.

## Phase 1: Repository analysis — DONE (2026-09-25)
- [x] Inspect the Skill System buildfile, systems, tools, limits -> `ARCHITECTURE.md`
- [x] Plan presented and approved (decisions in `GAME_DESIGN.md`)

## Phase 2: Technical foundation — DONE (2026-09-25)
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
- [x] Suspend round trip (WriteSuspendSave/ReadSuspendSave keep the run state)
- Moved on: RAM-block check through a full battle -> Phase 3 (needs a battle); Legacy save block ->
  Phase 13; debug commands are added with each system they control (spec 81).

## Phase 3: Tactical combat — IN PROGRESS
- [ ] Battle chapter: one reusable chapter; player units from the run's deployed roster, enemies from
      the run state (3v3); objective: defeat all enemies (no retreat)
- [ ] 20-turn limit: warnings on turns 15, 18, 19; defeat after turn 20
- [ ] Victory -> run state (Col_OnVictory) -> next battle (post-battle menu comes in Phase 10)
- [ ] FE combat verified by tests: damage, hit, crit, doubling (AS 4), weapon triangle, terrain, death
- [ ] Enemy AI foundation
- [ ] RAM block untouched through a full battle

## Phase 4-17
Not started. See `docs/COLISEUM_SPEC.md` §86 for the list.
