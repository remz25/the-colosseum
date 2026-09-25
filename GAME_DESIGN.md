# COLISEUM: Game Design

The master specification is [`docs/COLISEUM_SPEC.md`](docs/COLISEUM_SPEC.md) (saved verbatim).
It is the source of truth. This file records **clarifications and decisions made since**, and
any interpretation of the spec that affects gameplay. Core rules (spec §88) never change here
without the developer's approval.

## Decisions log

| Date | Decision | By | Notes |
|---|---|---|---|
| 2026-09-25 | COLISEUM is a **new, separate project** (The Severed Star is untouched, `C:\Dev\TheSeveredStar`). | Developer | |
| 2026-09-25 | Foundation: the **community FE8 Skill System** buildfile (FireEmblemUniverse/SkillSystem_FE8, upstream `65b959d`). | Developer | Reuse its skills, save expansion, stat screen, debug tools (spec §3). |
| 2026-09-25 | Project folder: `C:\Users\RdotS\Downloads\Coliseum`. | Developer | |
| 2026-09-25 | **Class skills count toward the 4 skill slots.** A unit has 1 personal skill + 3 slots shared by class skills and learned skills (spec §27, Phase 6). | Developer | |
| 2026-09-25 | **"No stat caps" = a ceiling of 127 per stat** (displayed up to 99). The GBA unit struct stores stats as signed bytes; a higher ceiling needs a large engine rewrite. Saves must store full bytes (vanilla packs stats into 5 bits, max 31). | Developer | |
| 2026-09-25 | **Str/Mag split on** (Skill System's `USE_STRMAG_SPLIT`): Magic is a separate stat (spec §15). | Developer | |
| 2026-09-25 | **All deployed units dead = the run ends**, even with living reserves (spec §12 / Phase 5). | Developer | Current behavior (FE8 game over). |
| 2026-09-25 | The 15 characters are FE8 characters (portraits and animations exist): Joshua, Marisa, Gerik, Garcia, Gilliam, Franz, Neimi, Colm, Vanessa, Cormag, Lute, Knoll, Natasha, Artur, L'Arachel. | Claude (implementation) | Names/identities can change in Phase 14. Ross dropped (trainee class promotes at 10). |
| 2026-09-25 | Level-up choice menu: title + 3 options (+1 stat, new value); B cannot skip; shown after the battle's level-up screen. | Claude (implementation) | Spec §23. |
| 2026-09-25 | Losing a battle ends the run for good: the run's save slot and the suspend are invalidated (spec §76-77: one active run, no reloading a lost run). New Game always starts a fresh run. | Claude (implementation) | Follows the developer's "all deployed dead = run over". |
| 2026-09-25 | Recruits join at the living roster's average level (min 5, max 30) with level-5 bases + average growth for the extra levels (spec §9 "appropriate to progression", §45 fixed build). | Claude (implementation) | Balance lever for Phase 16. |
| 2026-09-25 | Until the 3-win reward menu (Phase 10), every 3-win reward is a recruitment offer at the start of the next battle. | Claude (implementation) | Interim. |
| 2026-09-25 | Debug mode: the Skill System's `__DEBUG__` build flag; debug builds are separate ROMs and never the player build (spec §81). | Claude (implementation) | See `ARCHITECTURE.md`. |

## Open questions (not decided yet)

- Final boss concept (spec §60: Claude may propose one; to be presented before Phase 12).
- Staves: unlimited uses too, or keep their uses? (spec §40 removes *weapon* durability; staves
  currently keep uses.)
- FE8's vanilla death quotes still play for the pool characters (Phase 14: replace).
- 5 bosses, floor themes and factions: names and designs (Phase 12 / 14); character dialogue (Phase 14).
- Exact Recover cost, gold rewards, shop prices (Phase 16 balance).
