# COLOSSEUM: TODO

Phases from the spec (§86). A feature is done only when it meets the Definition of Done (§91):
built, working in game, tested, documented.

## Phase 1: Repository analysis — DONE (2026-09-25)
- [x] Inspect the Skill System buildfile, systems, tools, limits -> `ARCHITECTURE.md`
- [x] Plan presented and approved (decisions in `GAME_DESIGN.md`)

## Phase 2: Technical foundation — DONE (2026-09-25)
- [x] Own repository in `Downloads\Colosseum`, upstream remote kept as `upstream`
- [x] Checked build script `scripts/build.py`; unmodified base boots to its test map in mGBA
- [x] Documentation set (this file, GAME_DESIGN, ARCHITECTURE, CHANGELOG, TEST_STATUS, BALANCE_NOTES)
- [x] Config: Str/Mag split on; debug build variant (`--debug` -> `Colosseum_debug.gba`)
- [x] C build for `src/` (Arm GNU Toolchain + lyn, FE-CLib headers); objects may not use RAM variables
- [x] Test framework: **on-target unit tests** (`--test` build + `tests/run_tests.py` calls each test
      through mGBA's debugger; no host compiler needed). Proven to report failures with line numbers.
- [x] COLOSSEUM RAM block `0x0203F600-0x0203FDFF` (untouched after boot: checked by run_tests.py)
- [x] Run-state structure (`src/include/colosseum.h`): floor, encounter schedule, wins, 3-win reward,
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

## Phase 6: Skills — DONE (2026-09-25)
- [x] 1 personal (fixed) + 3 slots; the Skill System's adder limited to 3 (Skills.event)
- [x] Class skills count toward the 4: no longer implicit; learned into a slot at level 1 from the
      class list (26 lists), replaceable like any slot skill
- [x] Rarity (Common..Legendary) and prerequisites (weapon type, mounted, promoted, stat minimum)
      in a catalog of 48 player skills + 3 enemy-only skills (Skills.event)
- [x] No duplicates (personal or slot); rarity-weighted offers (C 40 / U 30 / R 18 / E 9 / L 3)
- [x] Replacement: "learn a skill" menu (3 slots with icons; pick a slot to replace, or Don't learn);
      the personal skill is never listed
- [x] Skills are per run (cleared at a new run)
- [x] Enemy skills: class skills + floor-gated skills for normal enemies (levels 10/16/22);
      enemy-only skills in the catalog for Elites/Bosses
- Later: skill sources wired in their phases: 3-win reward and promotion (Phase 10), shop (Phase 8),
  relics (Phase 9), Elite skill sets (Phase 11), boss skills and Phase 2 (Phase 12).

## Phase 7: Weapons — DONE (2026-09-25)
- [x] No durability (spec 40): every weapon and staff is Indestructible; consumables keep uses
- [x] Weapon types and FE8 ranks kept; proficiency 3x faster (weapon EXP x3, spec 41)
- [x] Inventory: 5 items per unit (FE8); items persist between battles
- [x] Transfer (spec 40): Prepare > Transfer: giver, item, receiver; receivers must use that
      weapon type (any rank), other items go to anyone; room needed
- [x] Fusion (spec 42): Prepare > Fuse, only when chosen; both consumed; lines Iron > Steel > Silver
      (swords, blades, lances, axes, bows), Fire > ... > Fimbulvetr, Lightning > ... > Aura,
      Flux > Nosferatu, and Killing Edge + Keen Edge > Lethal Edge
- [x] Variants (spec 43): Elite weapons Keen Edge, Titan Axe, Gale Lance, Hawk Bow; unique Lethal
      Edge; boss weapons (spec 44/55) Tyrant Blade, Warlord Pike, Ruin Cleaver, Storm Longbow,
      Abyss Tome (unsellable) - handed out in Phases 11/12
- [x] "Prepare" menu before each battle: Fight! / Deploy (more than 3 alive) / Transfer / Fuse
- [x] Fixed: `%` in COLOSSEUM C was wrong (FE-CLib mapped GCC's modulo helpers onto FE8 routines
      with another convention) - src/core/divmod.c + stripped reference; lyn -nohook
- Later: Legacy weapons (Phase 13); weapon drops from Elites/Bosses (Phases 11/12); shop (Phase 8).

## Phase 8: Shop & Economy — DONE (2026-09-25)
- [x] Gold (spec 37): every victory pays 200 + 75/floor + 0-100 (Elite x2, Boss x4); shown in the
      Prepare and Shop menus and mirrored into FE8's party gold
- [x] Shop (spec 38) from the Prepare menu: 8 entries rolled once per battle (no reroll); at least
      one of each relevant category: Recruit (while anyone is left), Skill, Weapon (by floor),
      Healing (team +50% HP), Promotion (crests, Master Seal), Consumable (Vulnerary, Elixir,
      Pure Water, stat boosters)
- [x] Buying: skill -> who learns it -> learn/replace menu; weapons/items -> who receives it
      (weapons: units using that type); recruit joins (or replaces when full); heal applies.
      Gold is only taken when the purchase went through; entries nobody can take are grayed
- [x] Price scaling (spec 39): +10% per purchase, compounding over the run, rounded each step;
      at most 5x base and 9999 gold
- [x] Run state grown to 256 bytes (v4; save chunks 0x100); UI scratch moved to 0x200
- Later: Relic category (Phase 9), Recover (Phase 10, post-battle menu), gold/price balance (Phase 16).

## Phase 9: Relics (prototype pool v1) — DONE (2026-09-26), see docs/RELICS.md
- [x] 2 relics per character (spec 33); relics belong to the run: equip, unequip, transfer (swap)
      outside battle; relic bag (16) for unequipped relics; duplicates allowed
- [x] Rarities Common..Mythic supported (spec 34); prototype pool: 6 Common, 5 Uncommon, 3 Rare,
      1 Epic, exactly as the developer's prompt (15 relics)
- [x] Generic modifier engine: flat / percent stats (stat getters), Hit/Avoid/Crit and adjacent-ally
      Def (pre-battle loop), damage dealt / magic / taken % and HP per attack (battle proc loop),
      battle gold % (victory); conditions (below 50% HP)
- [x] One percentage rule: additive per kind, after flat changes, rounded to nearest
- [x] Info screen before equipping/buying: name, rarity, every effect (good green, drawback gold,
      conditions as headers); Prepare > Relics menus; shop Relic category (rarity-weighted)
- [x] Wearer leaves the run -> relics back to the bag; new run clears relics
- [x] Saved in the run state (v5); v4 saves upgraded on load
- [x] 14 on-target tests; in game: menus, equip/transfer/unequip, shop purchase, stat screen,
      Blood Pact in a real battle (TEST_STATUS.md)
- Future relic concepts: docs/RELICS_FUTURE.md (developer to paste the full concept list).
- Later: relic rewards from Elites/Bosses and reward choices (Phases 10-12); relic list on the
  stat screen (optional); relic weights/prices (Phase 16).

## Phase 11: Elites — PARTLY DONE (2026-09-26)
- [x] Elite battles are distinct: announcement before the battle; Champion (1 strong enemy, own
      skills, Elite weapon dropped) or Elite Squad (3 roles built for synergy) (spec 47-48)
- [x] Elite rewards: 2x gold, a guaranteed relic drop, the Champion's weapon (spec 50, part)
- [x] Enemy drops (developer request): gold / skill / relic; first battle of each run: a relic
- [ ] Advanced Elite AI (spec 49): Elites still use FE8's AI (charge; healers heal)
- [ ] More Elite rewards (recruitable Elites, reward choice) with the Phase 10 reward menu
- [ ] Elite setups per floor/faction; balance (Phase 16)

## Original characters (developer request, 2026-09-26)
- [x] Batch 1 (approved): Morrow, Silas, Hale, Selene, Idris - pool 15 -> 20; portrait pipeline;
      run state v6 (room for 32)
- [x] Batch 2 (approved): Dagny, Oriane, Ysolde, Celestine, Aurel - pool 25
- [x] Battle palettes matching the portraits (base classes and promotions)

## Shop (developer request, 2026-09-26)
- [x] Fully random stock every round (categories and order), Heal 30/50/100%

## Balance after the play-test (developer request, 2026-09-27)
- [x] First Elite gentler; 2 Common + 1 Rare starting relics (team screen); EXP x1.5
- [ ] Developer play-test of the new values

## Arena system (developer request, 2026-09-27), see docs/ARENAS.md
- [x] Milestone 1: 8 arenas from FE8 tilesets, floor pools, weather (FE8 animations, fog of war,
      Hit penalties), hot-rock hazards, sacred seal, arena notice, hazard-aware AI, debug menu,
      tests (on-target + static map checks), seen in game
- [ ] Developer play-test of the arenas
- [ ] Milestone 2: Frozen / Swamp / Graveyard with FE-Repo tilesets and their special rules
- [ ] Boss arenas (with Phase 12); fire animation for burning ground; Abyss hazards

## Phase 10: Roguelike progression — DONE (2026-09-27)
- [x] 3-win cycle and reward choice: 3 of Recruit / Skill / Promotion / Heal / Gold, invalid
      kinds replaced, rolled once, Gold 100-500 (spec 51-53)
- [x] Promotion: level 10+, keeps level and EXP, branching path via FE8's promotion screen
- [x] Recover: 3 charges per floor, gold cost, full heal, reset after the boss (spec 54)
- [x] Post-battle menu: Next fight / Shop / Recover (+ team tools); autosave after every win
- [ ] Developer play-test (reward menu, Recover, promotion keeping the level)
- Later: relics that change rewards (spec 36, "choose 2"), promoted recruits (spec 9, rare),
  boss promotion items (Phase 12).

## Release v0.1.0 (developer request, 2026-09-28)
- [x] UPS patch tool (`scripts/ups.py`), verified round trip; `build.py --output`
- [x] Play-test on the patched ROM: team, shop, relics, attack, win, autosave, 3-win reward,
      Elite in a second arena, Resume Chapter, several fights in a row (tests/release/)
- [x] Screenshots (release/screenshots, 2x); docs in release/ (README, patching guide, notes,
      changelog, known issues, credits, FEUniverse post)
- [x] `scripts/make_release.py`: build -> patch -> verify -> zip (no ROMs) -> extract + re-verify
- [ ] Developer: name for CREDITS.txt, read the docs, post on FEUniverse

## Phase 12-17
Not started. See `docs/COLOSSEUM_SPEC.md` §86 for the list.
