# COLISEUM: Changelog

## 2026-09-26 (run-over safety net)
- The developer saw a battle continue to the turn limit after every unit died. Not reproduced;
  the player-phase turn event now also ends the run when no deployed unit is alive
  (`Col_CheckDeployedAlive`, `BattleChapter.event`). Map test 58.

## 2026-09-26 (Elite battles, enemy drops, random shop)
- Elite battles (`src/core/encounters.c`): an "ELITE BATTLE!" announcement (`src/core/notice_ui.c`)
  before the Prepare menu; 4 Champion setups (boosted, own skills, Elite weapon dropped on
  defeat) and 3 Elite Squads (Guardian / Mender / Reaper / Striker roles). Role characters
  0x81-0x88 renamed and given skills (CharacterTable.csv, PersonalSkillEditor.csv,
  CharacterLevelUpSkillEditor.csv, lists in Skills.event).
- Enemy drops: gold / skill / relic, claimed with a notice after the kill (player phase), at the
  next player phase (enemy-phase kills) or at the battle's end; skill drops open the learn menu
  for the killer. The first battle of every run always has a relic drop; every Elite too.
- Shop: all 8 entries random every round (categories, order); Heal 30/50/100%.
- Enemy creation moved from battle.c to encounters.c. Run state: `elite` and `drops[4]` in
  reserved space (still v5). UI scratch grown to 0x200-0x2FF (notices at +0x80).
- Fixed: comparisons of FE-CLib's signed `unit->index` with saved unsigned indices (enemy
  indices 0x80+ never matched); found by the new drop tests.
- Tests: 6 map tests (Elite setups, Champion, Squad, first-battle relic, drop odds, killer
  tracking); Test_ShopStock rewritten for random stock.
- **Upstream Skill System file changed**: `BattleProcCalcLoop.event` (+`Col_DropKillProc` before
  Proc_Finish).

## 2026-09-26 (Phase 9: relics, prototype pool v1)
- Relic system (`src/relics/`, docs/RELICS.md): 15 prototype relics (6 Common, 5 Uncommon,
  3 Rare, 1 Epic), rarities up to Mythic supported, a generic modifier engine (flat/percent stats,
  battle rates, damage %, HP cost, gold %, adjacent-ally aura, conditions).
- Prepare > Relics: equip / unequip / transfer with an info screen (rarity, effects, drawbacks).
- Shop: Relic category (rarity-weighted, priced by rarity); bought relics go into the relic bag.
- Battle gold passes through the gold relics (`Col_OnBattleWon`).
- Relics return to the bag when their wearer dies or is replaced.
- Run state v5 (relics + bag); v4 saves are upgraded on load.
- Tests: 14 relic map tests; Test_ShopStock expects the Relic category.
- **Upstream Skill System files changed** (one line each, marked "COLISEUM relics"):
  `EngineHacks/Necessary/StatGetters/{Power,Magic,Skill,Speed,Luck,Defense,Resistance,Movement}.event`
  (relic getter before prMinZero / the Freeze and Guard-AI nullifiers),
  `EngineHacks/Necessary/CalcLoops/PreBattleCalcLoop/PreBattleCalcLoop.event` (Col_RelicPreBattle),
  `EngineHacks/Necessary/CalcLoops/BattleProcCalcLoop/BattleProcCalcLoop.event`
  (Col_RelicDamageProc before Bane/Lethality, Col_RelicHpCostProc before Proc_Finish).
- docs/RELICS_FUTURE.md keeps the relic ideas for later.

## 2026-09-25 (fix)
- Fixed: after New Game the screen stayed black. The battle's beginning event opened the
  team/Prepare menus before FE8 faded the chapter in (FE8 fades in after that event), so the menus
  were invisible. The event now fades in first (`FADU 16`).
- The test runner now checks the real screen brightness when the team screen is up (the VRAM
  screenshots used by the scripted checks ignore fades, which is how this was missed).

## 2026-09-25 (Phase 8)
- Gold from every victory; party gold mirrors the run's gold.
- Shop (`src/shop/shop.c`, menus in `src/weapons/prepare_ui.c`): 8-entry stock per battle, all
  relevant categories, +10% compounding prices with caps, buy flows for recruit / skill / weapon /
  heal / promotion / consumable.
- Run state v4: 256 bytes (shop stock), save chunks 0x100; UI scratch at 0x200.
- Tests: 5 shop map tests.

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
