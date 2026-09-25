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

## Phase 3: Tactical combat — DONE (2026-09-25)
- [x] Battle chapter: one reusable chapter (slot 0); deployed roster placed, reserves hidden, 3
      enemies from the run state (level by floor/wins, +3 Elite, +6 Boss); objective: rout
- [x] New Game skips the intro and world map straight into the arena
- [x] 20-turn limit: popups on turns 15, 18, 19; "Time is up." + game over after turn 20 (in game)
- [x] Victory -> Col_OnBattleWon (HP kept, deaths removed) -> save menu -> next fight (in game)
- [x] FE combat verified by tests: damage, hit, avoid, crit (x3), doubling (AS 4 vs 3), weapon
      triangle, terrain, weight, Str/Mag, zero damage, death (6 combat tests on the battle map)
- [x] Enemy AI foundation: vanilla AI (ai bytes 0 = charge and attack), seen in game; the scoring AI
      comes with Elites/Bosses (spec: Phase 11/12)
- [x] RAM block: nothing beyond the 64-byte run state written after boot, battle load and combat
      tests (automated). A whole played battle is not yet checked automatically.
- Moved on: all-deployed-dead with living reserves -> answered: the run ends (GAME_DESIGN.md);
  post-battle menu instead of the save menu -> Phase 10; enemy factions per floor -> Phase 11.

## Phase 4: Units — DONE (2026-09-25)
- [x] 15 characters (spec 10): FE8 characters, one per listed class + a second Myrmidon (Marisa
      replaces Ross: Journeyman trainees promote at 10); distinct personal skills
- [x] Level-5 bases and personal growths incl. Magic (CharacterTable.csv, MagCharEditor.csv);
      casters' Str/Mag split; derivation in BALANCE_NOTES.md
- [x] Level 5 start (no autolevel), level cap 30 for every class (ClassLevelCapTable.csv)
- [x] No stat caps: class caps 127, vanilla cap functions replaced (Luck no longer capped at 30);
      stats above 31 survive suspend and game save (runner check)
- [x] EXP: FE8's formula (level, enemy level/class power, kills, bosses) kept; EXP continues past 20
- [x] Level-up = growth rolls + choose 1 of 3 random +1 stats (duplicates allowed); menu after
      player-phase battles, at the start of each player phase and at battle end (in game: player
      phase checked; the enemy-phase path uses the same function but was not played through)
- Moved on: EXP amounts -> Phase 16 balance; names/dialogue -> Phase 14; skill pass -> Phase 6.

## Phase 5: Roster — DONE (2026-09-25)
- [x] New run (spec 7): "Your team" screen shows the 3 random characters; Begin accepts (no reroll)
- [x] Roster 5 / deploy 3 / 2 reserves (spec 8): "Choose 3 fighters" before a battle when more than
      3 are alive; reserves stay off the map
- [x] Recruitment (spec 9): 3 random never-recruited characters or Decline; full roster -> "Replace
      whom?" (Back returns); the replaced unit leaves the run for good; never recruited twice
- [x] Recruits join at the roster's average level with a fixed build: level-5 bases + average growth
      (spec 45)
- [x] Permanent death (spec 12): dead units leave the roster, never recruitable again
- [x] 2v3 / 1v3 continuation (fewer than 3 alive: all deploy)
- [x] Run termination: a lost battle (all deployed dead, or the turn limit) ends the run: run cleared,
      its game save and the suspend invalidated; New Game always starts a fresh run
- Interim until Phase 10 (reward menu): the 3-win reward is always a recruitment offer, shown at
  the start of the next battle.
- Later: promoted recruits (rare, spec 9) -> Phase 10 (promotion); recruit skills/equipment by
  floor -> Phases 6-7; deployment from the post-battle menu -> Phase 10.

## Phase 7 (early, on request 2026-09-25)
- [x] No weapon durability (spec 40): every weapon and staff is Indestructible (ItemTable.csv);
      consumables keep their uses

## Phase 6-17
Not started. See `docs/COLISEUM_SPEC.md` §86 for the list.
