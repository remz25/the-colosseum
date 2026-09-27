# COLOSSEUM: Changelog

## 2026-09-28 (release preparation fixes)
- B now works as Back in every menu with a Back row (relic menus, transfer, fuse, replace,
  "who gets it?"); before, only the Back row closed them.
- Fix: reopening an R help box for the same row showed a garbled first line. The text cache that
  is cleared before each help box is at 0x0202B6AC (0x0202A6AC is the decoded text itself).
- `scripts/ups.py`: UPS patch create/apply (CRC-checked); `scripts/build.py --output PATH`
  builds the player ROM elsewhere (e.g. while Colosseum.gba is open in mGBA).
## 2026-09-27 (in-game name: The Colosseum)
- Player-visible text renamed (developer): chapter title card "The Colosseum", arena "Grand
  Colosseum", Elite weapon descriptions "A Colosseum champion's ...". Code, files and docs keep
  the COLOSSEUM project name for now.

## 2026-09-27 (R-button help in every COLOSSEUM menu)
- Pressing R on a menu row shows FE8's help box for it without choosing it; moving the cursor
  updates it; R or B closes it (`src/core/help.c`). Skills: their description. Relics: name,
  rarity and every effect. Weapons and items: FE8's stat / description box. Characters: their
  personal skill (team, recruit) or personal + slot skills (roster lists). Reward rows: what the
  reward does. Menus: team, recruit, replace, deploy, 3-win reward, "who gets it?", skill learn
  menu (new skill and each slot), shop, transfer items, fusions, relic unit list (both relics),
  relic slots, relic choices.
- Built text goes through text ID 0x0FF0 (`ColText_Help`), which points at a RAM buffer
  (COL_RAM_BASE + 0x400) via the anti-Huffman patch.
- Prepare menu one tile wider; the Recover row reads "Recover 3/3 300G".

## 2026-09-27 (Phase 10: roguelike progression)
- 3-win reward menu (spec 51-53): 3 different valid kinds from Recruit / Skill / Promotion /
  Heal / Gold, rolled once and saved (run state 0xBC-0xC1, formerly reserved); rows show the
  contents ("Skill: Vantage", "Gold: 332G"). Recruit (Decline returns to the choice), Skill and
  Promotion ask who gets it (Back returns), Heal and Gold apply at once (`src/core/reward.c`,
  `src/roster/roster_ui.c`). Replaces the interim "every reward is a recruit offer".
- Promotion reward: the chosen unit (level 10+, unpromoted) gets a promotion item it can use
  (Master Seal, else Ocean Seal / Lunar or Solar Brace); FE8's own promotion screen with the
  branching class choice runs when it is used.
- Promotion keeps level and EXP: the level/EXP reset in ApplyUnitDefaultPromotion and
  ApplyUnitPromotion is patched out (0x0802BD20, 0x0802BE70; `src/units/Units.event`).
- Recover (spec 54) in the Prepare menu: "Recover 3/3 300G" (300 + 100 per floor above 1),
  full heal for every living roster member, one of 3 charges per floor (reset after the boss).
- The Prepare menu is the post-battle menu (spec 6): Next fight / Shop / Recover / Deploy /
  Transfer / Fuse / Relics; it opens with the next arena on screen.
- Autosave after every win (spec 76): FE8's post-chapter save menu is replaced by a save to the
  run's slot (game control 0x08591924, `src/battle/SkipWorldMap.event`).
- Generic notice helper (`Col_ShowNotice`).
- Tests: Test_RewardRoll, Test_RewardTake, Test_Promotion (level and EXP kept in both FE8
  promotion functions), Test_Recover.

## 2026-09-27 (arenas, weather, hazard and sacred tiles: milestone 1, docs/ARENAS.md)
- 8 arenas cut from vanilla FE8 maps (`src/arenas/`, `scripts/arenas.py`): Grand Colosseum,
  Forest, Desert, Volcanic, Ruined Cathedral, Royal, Abyss, Misty Ruins; floor pools; the next
  arena and weather are rolled after each win and saved in the run state (bytes 0x69-0x6A,
  formerly padding: v6 saves stay valid).
- The battle chapter's data comes from the arena: `GetROMChapterStruct` replaced; RAM copy at
  COL_RAM_BASE + 0x300.
- Weather with FE8's own animations, fog of war and rain/snow movement; Hit penalties (rain -5,
  sandstorm -10, ashfall -5). Arena notice before each battle.
- Hazard tiles (hot rock, 5 HP at the phase start, never below 1 HP, fliers immune) through the
  replaced poison step; sacred tiles (seal, 10% heal) through the HP restoration loop.
- Enemy AI: hazard-aware attack positions and move end tiles.
- Arena debug menu in the Prepare menu (debug/test builds only; the build refuses it in the
  player ROM).
- **Upstream Skill System files changed**: `PreBattleCalcLoop.event` (+`Col_WeatherPreBattle`),
  `HPRestorationCalcLoop.event` (+`Col_SacredTileHeal`).
- Tests: 6 on-target arena/weather tests; `tests/check_arenas.py` static map checks (run by
  run_tests.py); the runner's RAM checks allow the arena chapter copy.

## 2026-09-27 (balance after the developer's play-test)
- The run's first Elite (floor 1, after 3 wins) is gentler: the Champion is 2 levels lower with
  +6 HP / +1 stats (instead of +12 / +2); Elite Squads are 1 level lower (`Col_IsFirstElite`,
  `src/core/encounters.c`). Later Elites are unchanged.
- Every run starts with 2 different Common relics and 1 Rare in the relic bag
  (`Col_GiveStartingRelics`); the "Your team" screen lists them (the Rare one in gold).
- Player units gain 1.5x battle EXP, rounded up, still at most 100 per battle (`Col_ExpBoost`,
  `src/units/stats.c`, `COL_EXP_PERCENT`). Staff EXP is unchanged.
- **Upstream Skill System file changed**: `EXPCalcLoop.event` (+`Col_ExpBoost` after the EXP
  skills, one line marked COLOSSEUM).
- Tests: Test_FirstEliteGentler, Test_StartingRelics, Test_ExpBoost (a real kill through the EXP
  loop); the Champion and Squad tests now spawn a floor's second Elite (full strength).

## 2026-09-26 (second batch of original characters; battle palettes)
- Dagny, Oriane, Ysolde, Celestine, Aurel (pool 25) in the slots of Amelia, Tana, Myrrh, Syrene
  and Forde: FE-Repo portraits (credited), stats, skills, death quotes; Syrene's and Tana's
  pegasus Triangle Attack flag removed.
- Per-character battle palettes (`scripts/charpal.py`, manifest `src/graphics/battle_palettes.txt`):
  the 10 original characters' battle sprites use their portraits' colours in their base classes
  and promotions. FE8's per-character class/palette rows (0x95E0A4 / 0x95EEA4) are rewritten; the
  new palettes live in palette-list entries of FE8 characters who never appear (repointed). The
  build verifies every row and palette in the output ROM. `py -3 scripts/charpal.py CHAR CLASS...`
  prints a base palette.

## 2026-09-26 (5 original characters: the pool grows to 20)
- Morrow (Pirate), Silas (Mage), Hale (Archer), Selene (Dancer), Idris (Priest): FE-Repo
  portraits, names, descriptions, level-5 bases, growths, magic, personal skills, death quotes;
  pool entries 15-19 (`src/core/pool.c`). Credits in CREDITS.md.
- Portrait pipeline ported from The Severed Star: `scripts/portraits.py` + `scripts/mugtool.py`,
  manifest `src/graphics/portraits/Portraits.txt`; the build converts the sheets with
  PortraitFormatter, writes the portrait table and verifies it in the output ROM.
- Run state v6: pool of up to 32 (32-bit dead/recruited masks, relic slots for 32 characters);
  v4/v5 saves are upgraded on load (`Col_UpgradeRunState`, tested byte by byte).
- **Upstream Skill System file changed**: `skill_definitions.event` - Pickup and Stunning Smile
  enabled with the IDs of Thighdeology (187) and Thotslayer (188), which are now disabled.
- Tests: Test_OriginalCharacters; the pool load test accepts a dancer (no weapon); the save test
  now upgrades real v4/v5 images.

## 2026-09-26 (fix: the run didn't end when every deployed unit died)
- Cause: FE8 clears every blue unit's not-deployed flag after the battle's beginning event, so
  the hidden reserves counted as available units and FE8's game over (no units left) never came;
  the battle went on to the turn limit. `CountAvailableBlueUnits` (0x08018FF0) is replaced by
  `Col_CountAvailableBlueUnits` (hidden units don't count unless carried by an ally).
- A second guard: the player-phase turn event ends the run when no deployed unit is alive
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
- **Upstream Skill System files changed** (one line each, marked "COLOSSEUM relics"):
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
- 10 new items (0xC0-0xC9): 4 Elite variants, Lethal Edge, 5 boss weapons (names in colosseum.txt).
- "Prepare" menu before each battle: Fight! / Deploy / Transfer / Fuse; roster units now exist
  (hidden) before it opens.
- Fixed `%` (modulo) for all COLOSSEUM C: correct __aeabi_idivmod/uidivmod (src/core/divmod.c),
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
  COLOSSEUM C. The Skill System's battle hit buffer lives at `0x0203AAC0` (8 bytes per hit).

## 2026-09-25 (later)
- Str/Mag split on. `build.py --debug` (debug menu, separate ROM) and `--test` (on-target tests).
- C pipeline: `src/**/*.c` -> arm-none-eabi-gcc -> lyn -> `build/Colosseum.lyn.event`, installed by
  `src/Colosseum.event` (one include line added to `ROMBuildfile.event`).
- Run state (`src/core/run_state.c`): encounter schedule (3 normal wins -> reward + Elite; 9 ->
  Boss), gold, Recover charges; saved in the game save and suspend (two chunks added to upstream
  `ExModularSave.event`: game `$11F0`, suspend `$290E`, 0x40 bytes each).
- `tests/run_tests.py` + `tests/emu/gdb.py`: on-target unit tests and save integration in mGBA.

## 2026-09-25
- Project created from the FE8 Skill System (upstream `65b959d`), own git history on `main`.
- `scripts/build.py`: checked build (clean-ROM CRC, each tool from its folder, assembler must say
  "No errors", output header/size validated). `MAKE_HACK_full.cmd` fixed to call `ColorzCore.exe`.
- Master spec saved as `docs/COLOSSEUM_SPEC.md`; Phase 1 analysis in `ARCHITECTURE.md`; decisions
  in `GAME_DESIGN.md`.
