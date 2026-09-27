# COLISEUM: Arenas, weather, hazard and sacred tiles

The developer's prompt is saved verbatim in [`ARENA_SPEC.md`](ARENA_SPEC.md). This file describes
what is built (milestone 1, 2026-09-27), how it works, and what is left.

Decisions (GAME_DESIGN.md, 2026-09-27): milestone 1 uses FE8's own tilesets only (Frozen, Swamp
and a real Graveyard need community tilesets: milestone 2); fog is FE8's real fog of war.

## Arenas

Every arena is a 15x10 region cut from a vanilla FE8 chapter map. It keeps that chapter's
tileset, palette, tile config and tile animations (`src/arenas/arena_maps.txt`,
`scripts/arenas.py`); gameplay data is in `src/arenas/arenas.c`.

| # | Arena | Cut from | Weather (weights) | Tiles | Floors |
|---|---|---|---|---|---|
| 0 | Grand Colosseum | ch 0x43 hall (whole map) | Clear | - | 1, 2, 5 |
| 1 | Forest Arena | ch 0x14 (2,1): woods, thickets, a mountain | Clear 6 / Rain 4 | - | 1, 2, 4 |
| 2 | Desert Arena | ch 0x0F (4,1): oasis, temple ruin, cliffs | Clear 5 / Sandstorm 5 | - | 2, 4, 5 |
| 3 | Volcanic Arena | ch 0x12 (7,4): lava pool ringed by rock | Ashfall | 25 hot-rock tiles (burning) | 6, 7+ |
| 4 | Ruined Cathedral | ch 0x15 (4,14): Black Temple seal | Clear 5 / Rain 5 | 21 seal tiles (sacred) | 3, 5, 6 |
| 5 | Royal Arena | ch 0x13 (7,5): dark throne room, pillars | Clear | - | 1, 3, 5, 7+ |
| 6 | Abyss Arena | ch 0x15 (1,6): walkways over the void | Fog | - | 6, 7+ |
| 7 | Misty Ruins | ch 0x2E (3,6): Lagdou-style ruins | Clear 3 / Fog 7 | - | 3, 4 |

- A run starts in the Grand Colosseum. After every win, the next battle's arena is rolled from the
  floor's pool (floor 7 and up use the last pool), never the same arena twice in a row; Elite
  battles count arenas flagged for them (Royal, Cathedral) twice. Arena and weather are saved in
  the run state (`arena`, `weather`, bytes 0x69-0x6A; zero = Grand Colosseum, clear).
- Misty Ruins stands in for the Graveyard (FE8 has no tombstone art).
- Each arena has its own 3 player and 3 enemy spawn tiles.

## Weather

| Weather | FE8 effect | COLISEUM rule |
|---|---|---|
| Clear | - | - |
| Rain | rain animation; FE8's rain movement costs | -5 Hit (both sides) |
| Snow | snow animation; FE8's snow movement costs | - (no arena rolls it yet) |
| Fog | fog of war, vision 3 | - |
| Sandstorm | sandstorm animation | -10 Hit |
| Ashfall | FE8's "flames" (embers) animation | -5 Hit |

The Hit penalty is in the pre-battle loop, so the forecast shows it. Readability (spec 5): a
notice before every battle names the arena, the weather and its effect, and the tiles
(not shown for the Grand Colosseum in clear weather).

## Tiles

Tiles follow what the map shows: every walkable tile of glowing orange rock burns; the whole
circular seal of the cathedral is sacred.

- **Hazard (burning, 5 HP)**: at the start of the phase of the unit standing on it (so a unit
  that moves onto one is not hurt until its next turn begins). Applied in FE8's poison step
  (`MakePoisonDamageTargetList`, replaced), which shows the damage with the poison-cloud
  animation and the HP bar. Never below 1 HP. Poison still adds its own 1-3. Flying units are
  not hurt by burning or poisoned ground. Poison (3) and void (10) kinds exist in the code; no
  arena uses them yet.
- **Sacred (heal 10%)**: at the start of the unit's phase, through the Skill System's HP
  restoration loop (like a fort). Enemies on the seal heal too.

## Enemy AI

- When choosing the tile to attack from, FE8's terrain score is replaced
  (`AiGetTerrainCombatPositionScoreComponent`): minus 10 per HP of hazard the unit would take,
  plus 10 on sacred tiles. It attacks from a safe tile when one exists, from a hazard otherwise.
- When only moving (towards a target, a heal point, an escape), FE8's end-tile filter
  (`AiCheckDangerAt`) also refuses hazards that would hurt the unit. It may walk across one.
  Fliers are not hurt, so they don't mind.
- Seen in game: without the filter two enemies walked onto hot rock; with it they stopped short.

## How it works (technical)

- One battle chapter (slot 0) for every arena. `GetROMChapterStruct` (0x08034618) is replaced:
  for chapter 0 it returns a RAM copy (`COL_RAM_BASE + 0x300`, 0x94 bytes) of chapter 0's data
  with the arena's map ids, fog and weather. The copy is rebuilt from the run state on every call,
  so a game save or suspend brings back the same arena through the run state. Every other
  chapter (and the link arena's 0x7F) behaves as in vanilla.
- The cut maps are stored in chapter asset table slots of vanilla story maps (arena_maps.txt);
  the build checks the pointers.
- Hooks: 0x08034618 GetROMChapterStruct, 0x080259EC MakePoisonDamageTargetList, 0x0803E23C AI
  terrain score, 0x0803E448 AiCheckDangerAt (`src/arenas/Arenas.event`); one line each in the
  upstream PreBattleCalcLoop (weather Hit) and HPRestorationCalcLoop (sacred tiles).

## Debug (spec 81, ARENA_SPEC 22)

Debug and test builds: Prepare > "Arena debug": Arena (A: next), Weather (A: next), Clear
weather, Fight here (the battle restarts in that arena and weather through the beginning event),
Back. The player ROM has it switched off (`ColDebugMenu`; the build refuses otherwise). Hazard
tiles are part of each arena's design and cannot be placed one by one.

## Tests

- On-target (`src/tests/test_arenas.c`, `Test_WeatherHit`): chapter data per arena and weather,
  other chapters untouched, rolls stay in the floor pool and never repeat, hazard damage (5,
  capped at 1 HP, poison adds, fliers immune, enemies on their phase), sacred heal through the
  real heal loop, AI scores and move filter, weather Hit on both sides.
- Static (`tests/check_arenas.py`, run by run_tests.py): every map at least 15x10, spawns
  distinct, walkable for Mercenary / Knight / Cavalier / Archer / Mage / Fighter in every
  weather the arena can roll, not on hazards; tiles walkable; a foot and an armour path from
  the player side to every enemy spawn.

## Left for later

- Milestone 2: Frozen Arena, Swamp Arena and a real Graveyard with FE-Repo tilesets; their rules
  (freezing water, swamp poison, cursed tiles).
- Abyss hazards (unstable tiles) need tile art that shows them.
- A fire animation for burning ground instead of the poison cloud.
- Boss arenas (Phase 12): bosses don't exist yet; the arena table is ready for a boss field.
- Resuming a suspended battle in a rolled arena: follows from the run state; not yet seen in game.
- Balance (Phase 16): weather weights, Hit penalties, hazard damage, heal %, pools.
