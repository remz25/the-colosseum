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
| 2026-09-25 | **Staves have unlimited uses too** (like weapons, spec §40). Consumables are still used up. | Developer | |
| 2026-09-25 | Class skills are real, replaceable slot skills (learned at level 1); e.g. Franz, Vanessa, Cormag, L'Arachel start with Canto and Colm with Cunning in slot 1. | Claude (implementation) | Follows "class skills count toward the 4". |
| 2026-09-25 | Skill catalog: 48 player skills over 5 rarities with prerequisites; Killing Machine, Nihil, Great Shield enemy-only. Offer weights C 40 / U 30 / R 18 / E 9 / L 3. | Claude (implementation) | Balance levers (Phase 16). |
| 2026-09-25 | Normal enemies: class skills, plus Duelist's Blow from level 10, Vantage from 16, Luna from 22 (floors 2-4). | Claude (implementation) | Balance lever. |
| 2026-09-25 | Proficiency: weapon EXP x3 (FE8 rank thresholds unchanged). | Claude (implementation) | Spec §41; balance lever. |
| 2026-09-25 | Transfer: to units using that weapon type at any rank (they can grow into it); non-weapons to anyone. Fusion: two weapons in one unit's inventory, result in the first slot. | Claude (implementation) | Spec §40, §42. |
| 2026-09-25 | A "Prepare" menu (Fight!/Deploy/Transfer/Fuse) opens before each battle until the post-battle menu (Phase 10). Deployment is optional from there. | Claude (implementation) | Interim placement. |
| 2026-09-25 | Gold per victory 200 + 75 per floor above 1 + 0-100 (Elite x2, Boss x4). Shop base prices: recruit 1500, skills 400/700/1100/1700/2800 by rarity, weapons 400-2400 by rank, promotion 2500, heal 500, consumables 150-1800. | Claude (implementation) | Balance levers (Phase 16). |
| 2026-09-25 | Price scaling: every purchase raises all later prices 10% (compounding, rounded each step), capped at 5x base and 9999. The shop opens from the Prepare menu until the post-battle menu (Phase 10). | Claude (implementation) | Spec §39. |
| 2026-09-25 | Debug mode: the Skill System's `__DEBUG__` build flag; debug builds are separate ROMs and never the player build (spec §81). | Claude (implementation) | See `ARCHITECTURE.md`. |
| 2026-09-26 | **Relic prototype pool v1**: exactly 15 relics (6 Common, 5 Uncommon, 3 Rare, 1 Epic) with the effects in docs/RELICS.md; rarities up to Mythic supported; future concepts kept in docs/RELICS_FUTURE.md. | Developer | "Relic System Implementation Prompt". |
| 2026-09-26 | Relics belong to the run: unequipped relics go to a relic bag (16 places); choosing a relic for a slot can take one from another unit (transfer; a relic already in the slot swaps back). Duplicates allowed (spec §33). | Claude (implementation) | |
| 2026-09-26 | When a wearer dies or is replaced, its relics return to the bag (lost only if the bag is full). Enemies never wear relics (for now). | Claude (implementation) | Developer may prefer relics lost on death: one line in run_state.c. |
| 2026-09-26 | One percentage rule for all relics: percentages of a kind add up, apply once after all flat changes, round to nearest (halves away from zero); damage: dealt % then taken %, after crits. | Claude (implementation) | Asked for by the prompt (§8, §13). |
| 2026-09-26 | Bloodied Band's "below 50% HP" (HP x 2 < max) is checked on every stat read and at the start of every combat (like FE's Wrath), not mid-exchange. | Claude (implementation) | |
| 2026-09-26 | Guardian's Crest: +3 Def only against physical attacks (Def isn't used against magic); two crests stack; shown in the forecast, not the stat screen. | Claude (implementation) | |
| 2026-09-26 | Golden Thread counts for every living roster member wearing it (reserves too) and stacks (+25% each); it only changes battle victory gold. | Claude (implementation) | |
| 2026-09-26 | Blood Pact's 2 HP cost applies to every attack the wearer makes: counters, follow-ups and misses too; never below 1 HP. | Claude (implementation) | |
| 2026-09-26 | Shop relic weights C 45 / U 30 / R 17 / E 8 / L 4 / M 1 (empty rarities skipped); base prices 500 / 900 / 1400 / 2200 / 3200 / 4500. One relic per stock plus possible extras. | Claude (implementation) | Balance levers (Phase 16). |
| 2026-09-26 | **Shop fully random every round**: all 8 entries pick a random category (weights: Weapon 22, Consumable 20, Skill 18, Relic 16, Recruit 10, Promotion 8, Heal 6); at most one Recruit / Heal / Promotion; Heal is 30/50/100% (300/500/900 G). Still rolled once per battle. | Developer | Replaces "one of each category". Weights are Phase 16 levers. |
| 2026-09-26 | **Enemy drops**: enemies can carry gold, a skill or a relic, given when they die. **The first battle of every run always has one enemy carrying a relic.** | Developer | |
| 2026-09-26 | Drop odds: normal enemies 25% (Elite squads 50%); gold 50% / skill 25% / relic 25%; gold 120-180 on floor 1 (+40 per floor, x2 in Elites); every Elite also guarantees one relic. A skill drop is a random skill the killer can learn, offered to the killer (learn / replace / decline). A relic with a full bag, or a skill nobody can learn, becomes 300 gold. Drops are hidden until claimed. | Claude (implementation) | Balance levers. |
| 2026-09-26 | **Elite battles made real (Phase 11, first part)**: "ELITE BATTLE!" announcement before the Prepare menu; 4 Champions (1 boosted promoted enemy, own skills, Elite weapon it drops) and 3 Elite Squads (Guardian / Mender / Reaper / Striker roles with synergy skills); 2x gold + a guaranteed relic. | Developer (scope), Claude (content) | Advanced Elite AI (spec 49) still to come: they use FE8's AI. |
| 2026-09-26 | Elite balance: Champion at Elite level (+3) with +12 HP and +2 other stats; squads 2 levels below the normal level with iron weapons on floor 1 (steel from floor 2). | Claude (implementation) | A floor-1 squad with steel weapons nearly one-shot Lv 6 units in testing. |
| 2026-09-26 | **The character pool grows from 15 to 20** (changes spec §10): 5 new original characters with FE-Repo community portraits (free to use and edit, credited). Claude proposes the designs; the developer approves them before anything is built. Proposal: Morrow (Pirate), Silas (Mage), Hale (Archer), Selene (Dancer), Idris (Priest). | Developer | Approved 2026-09-26 as proposed (Selene kept). Built. |
| 2026-09-26 | The 5 original characters take the slots of FE8 characters outside the pool (Ross -> Morrow, Ewan -> Silas, Kyle -> Hale, Tethys -> Selene, Moulder -> Idris): their names, descriptions, portraits, bases, growths, magic, personal skills and death quotes are replaced. Battle animations and map sprites are the class ones; their colours still come from the old characters' palettes. | Claude (implementation) | Palettes: follow-up. |
| 2026-09-26 | Pickup and Stunning Smile were disabled in the Skill System (no free skill ID): they take the IDs of the unused joke skills Thighdeology (187) and Thotslayer (188). | Claude (implementation) | |
| 2026-09-26 | **Second batch approved** (pool 20 -> 25): Dagny (Knight F, Barricade), Oriane (Cavalier F, Charge), Ysolde (Manakete F, Tantivy; never promotes), Celestine (Eirika's lord class, Rapier, Charisma), Aurel (Ephraim's lord class, Inspiration). Slots: Amelia, Tana, Myrrh, Syrene, Forde. The lords are ordinary fighters: losing one doesn't end the run. | Developer | Built. |
| 2026-09-26 | **Battle sprites take the portraits' colours** (developer request): per-character battle palettes for the 10 original characters in their base classes and promotions (scripts/charpal.py). | Developer (request), Claude (colours) | Colours chosen from the portraits; adjustable in src/graphics/battle_palettes.txt. |

## Open questions (not decided yet)

- Final boss concept (spec §60: Claude may propose one; to be presented before Phase 12).
- FE8's vanilla death quotes still play for the pool characters (Phase 14: replace).
- 5 bosses, floor themes and factions: names and designs (Phase 12 / 14); character dialogue (Phase 14).
- Exact Recover cost, gold rewards, shop prices (Phase 16 balance).
