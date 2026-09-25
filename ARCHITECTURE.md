# COLISEUM: Architecture

Phase 1 analysis (2026-09-25) of the FE8 Skill System buildfile and the plan for COLISEUM.

## Build

- `py -3 scripts/build.py` (full) / `--quick` (assemble only) -> `Coliseum.gba` + `.sym`.
  Base ROM `FE8_clean.gba` (FE8U, CRC32 `A47246AE`, git-ignored). Takes about 25 s.
- Steps: `Tables/` CSVs -> c2ea; `Text/` -> text-process-classic + ParseFileUTF8; `Maps/` (Tiled)
  -> tmx2ea; `ROMBuildfile.event` -> ColorzCore. Every tool is named with its `.exe` (the repo also
  ships extensionless Linux binaries that `cmd.exe` picks by mistake; this silently broke
  `MAKE_HACK_full.cmd`, now patched).
- Free space starts at `0x8FE4000` (ROM expanded past 16 MiB); vanilla-area free space in BL range
  at `0x81C1EC0` (`CustomDefinitions.event`).

## What the Skill System provides (reused)

| Area | Where | Use in COLISEUM |
|---|---|---|
| Skills (~250) | `EngineHacks/SkillSystem/` (`skill_definitions.event`, `Skills/`, `skill_lists.event`) | Skill effects, personal/class skills, learned skills (per character), scrolls, skill popups, debug skill editor. |
| Personal / class skills | `Tables/NightmareModules/Skills/*.csv` | Personal skill per character; class skills. |
| Str/Mag split | `EngineHacks/ExternalHacks/StrMagSplit` (`USE_STRMAG_SPLIT`) | Magic stat. |
| Expanded Modular Save | `EngineHacks/Necessary/ExpandedModularSave` | Custom save modules for run state and Legacy. |
| MSG (stat getters) | `EngineHacks/Necessary/MSG` | Relic and skill stat modifiers hook here. |
| Modular Stat Screen | `EngineHacks/Necessary/ModularStatScreen` | Skills, relics, enemy info (spec §65). |
| Unit menu rework | `EngineHacks/Necessary/UnitMenu` | New commands. |
| Danger Zone, HP bars, battle stats (anims off) | `EngineHacks/QualityOfLife` | Enemy ranges/info (spec §65). |
| Modular EXP | `EngineHacks/ExternalHacks/ModularEXP` | EXP by class/boss (spec §22). |
| Item Effect Revamp | `EngineHacks/Necessary/ItemEffectRevamp` | Consumables, promotion items. |
| Debug startup menu | `__DEBUG__` in `CustomDefinitions.event` | Debug mode (spec §81), extended with COLISEUM commands. |

## What COLISEUM builds (new code, `src/`, C via the Arm GNU Toolchain + lyn)

| System | Approach |
|---|---|
| Run loop (spec §5-6) | One reusable battle chapter; the run state picks map, enemy set and floor. After a battle, a custom menu (Next Fight / Shop / Recover) instead of chapters and a world map. Elite after every 3 wins, Boss at fight 10. |
| Run state | One RAM block + an Expanded Modular Save module (saved with the game save and suspend). |
| Legacy / Hall of Champions | A separate save module that survives new runs (the space of the unused save slots 2-3). |
| Roster (5 / deploy 3) | Run state holds the 5 roster characters; the battle chapter deploys 3. |
| Level-ups | Vanilla growth rolls + a choose-1-of-3 stat screen; level cap 30; stat ceiling 127. |
| Relics | 2 slots per unit in run state; effects through MSG stat getters and skill-system hooks. |
| Shop, rewards, recruitment, fusion | Custom menus (procs), data tables for prices/pools. |
| Legacy weapon names | Item name hook returning a RAM string for Legacy item IDs. |
| Advanced AI | C scoring AI hooked into FE8's AI decision step; budgeted per enemy (Elite/Boss deeper). |
| 20-turn limit | Turn events (warnings at 15, 18, 19; defeat after 20). |
| Boss phase 2 | Battle hook when HP crosses 50%. |

## GBA constraints

- CPU 16.78 MHz: AI search must be budgeted (heuristic scoring + limited look-ahead).
- Stats are `s8` in the unit struct -> ceiling 127.
- SRAM 32 KiB total (Expanded Modular Save layout: meta `0x00-0xD4`, suspend, 3 game saves,
  link arena, `0x7400` block). COLISEUM needs: 1 run save + suspend + Legacy block.
- EWRAM 256 KiB / IWRAM 32 KiB: new RAM must be placed in documented free areas (to be mapped in
  Phase 2 before any allocation).

## Dependency graph

```
Build + test harness + debug menu (Ph2)
        |
Run-state data + save modules (Ph2) ---------------+
        |                                           |
Battle chapter + 20-turn limit + 3v3 (Ph3)          |
        |                                           |
Units: 15 chars, Lv5-30, stat choice (Ph4)          |
        |                                           |
Roster / recruit / permadeath (Ph5) -- Skills (Ph6)-+
        |                                |          |
Weapons + fusion (Ph7) -- Shop/economy (Ph8) -- Relics (Ph9)
        |                                           |
Run loop: 3-win, Recover (Ph10) -> Elites (Ph11) -> Bosses (Ph12)
        |                                           |
Advanced AI (starts Ph3, deepens Ph11/12)           |
        |                                           |
Legacy + Hall of Champions (Ph13) ------------------+
        |
Story (Ph14) -> Presentation (Ph15) -> Balance (Ph16) -> QA (Ph17)
```

## Technical risks (highest first)

1. Advanced AI cost on the GBA CPU.
2. Stat ceiling and save packing (full-byte stats in saves).
3. SRAM layout for run + suspend + Legacy.
4. Runtime item names for Legacy weapons.
5. UI volume (shop, recruit, rewards, relics, Hall of Champions).
6. End-to-end integration testing of the run loop (emulator driver + native C tests).
