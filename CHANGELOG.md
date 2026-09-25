# COLISEUM: Changelog

## 2026-09-25 (Phase 7)
- Weapons (`src/weapons/`): fusion recipes and Fuse; Transfer to compatible units; weapon EXP x3.
- 10 new items (0xC0-0xC9): 4 Elite variants, Lethal Edge, 5 boss weapons (names in coliseum.txt).
- "Prepare" menu before each battle: Fight! / Deploy / Transfer / Fuse; roster units now exist
  (hidden) before it opens.
- Fixed `%` (modulo) for all COLISEUM C: correct __aeabi_idivmod/uidivmod (src/core/divmod.c),
  linked against a reference copy without FE8's mismatched ones; lyn runs with -nohook.
- Tests: 5 weapon map tests + modulo test; runner waits for placed units.

## 2026-09-25 (Phase 6)
- Skills (`src/skills/`): 1 personal + 3 slots; class skills moved into slots (level-1 class list
  entries, ClassSkillEditor.csv cleared); catalog with rarity and prerequisites; offers; the
  "learn a skill" menu with replacement; skills cleared per run.
- Normal enemies: floor-gated skills (generic character list); enemy-only skills marked.
- Tests: 8 skill map tests.

## 2026-09-25 (Phase 5)
- Roster (`src/roster/`): "Your team" screen at a new run; recruitment (3 candidates or decline,
  replace when full); "Choose 3 fighters" deployment with more than 3 alive; 2v3 continuation.
- Recruits: roster-average level, fixed build (bases + average growth).
- End of a run: New Game clears the run (InitPlayConfig hook); a game over ends the run and
  invalidates its save and the suspend (CallGameOverEvent hook; time limit too).
- Interim: the 3-win reward is a recruitment offer until Phase 10.
- Tests: 7 roster map tests; the runner now clicks through the team screen.
- No weapon durability (spec 40, requested now): all 110 non-staff weapons are Indestructible.
- Staves unlimited too (developer decision).

## 2026-09-25 (Phase 4)
- 15-character pool finalized (Marisa replaces Ross); all start at level 5 with level-5 bases and
  personal growths (tables); casters use the Str/Mag split; distinct personal skills.
- Level cap 30 for every class; no stat caps (class caps 127; `src/units/stats.c` replaces
  FE8's two cap functions).
- Level-up stat choice (`src/units/stat_choice.c`): pick 1 of 3 random +1 stats after the growth
  rolls; run state v3 tracks choices per roster slot.
- Tests: 6 unit map tests; the runner checks stats above 31 through suspend and game save.

## 2026-09-25 (Phase 3)
- Battle chapter (`src/battle/BattleChapter.event`, `src/core/battle.c`): chapter slot 0 as the
  arena; New Game starts a run with 3 random pool characters (placeholder pool of 15 vanilla
  characters, `src/core/pool.c`); 3 enemies scaled by floor/wins/encounter; rout objective.
- New Game skips the Magvel intro and world map (`src/battle/SkipWorldMap.event`).
- 20-turn limit: warnings on turns 15/18/19, game over after turn 20.
- Victory records the win, deaths and HP, then the next fight starts (after FE8's save menu).
- Combat tests (`src/tests/test_combat.c`) run on the battle map; `tests/run_tests.py` now boots
  New Game into the arena first.
- Found and worked around: the Skill System's battle calc loop zeroes r11 -> `-ffixed-r11` for all
  COLISEUM C. The Skill System's battle hit buffer lives at `0x0203AAC0` (8 bytes per hit).

## 2026-09-25 (later)
- Str/Mag split on. `build.py --debug` (debug menu, separate ROM) and `--test` (on-target tests).
- C pipeline: `src/**/*.c` -> arm-none-eabi-gcc -> lyn -> `build/Coliseum.lyn.event`, installed by
  `src/Coliseum.event` (one include line added to `ROMBuildfile.event`).
- Run state (`src/core/run_state.c`): encounter schedule (3 normal wins -> reward + Elite; 9 ->
  Boss), gold, Recover charges; saved in the game save and suspend (two chunks added to upstream
  `ExModularSave.event`: game `$11F0`, suspend `$290E`, 0x40 bytes each).
- `tests/run_tests.py` + `tests/emu/gdb.py`: on-target unit tests and save integration in mGBA.

## 2026-09-25
- Project created from the FE8 Skill System (upstream `65b959d`), own git history on `main`.
- `scripts/build.py`: checked build (clean-ROM CRC, each tool from its folder, assembler must say
  "No errors", output header/size validated). `MAKE_HACK_full.cmd` fixed to call `ColorzCore.exe`.
- Master spec saved as `docs/COLISEUM_SPEC.md`; Phase 1 analysis in `ARCHITECTURE.md`; decisions
  in `GAME_DESIGN.md`.
