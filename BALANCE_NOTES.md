# COLISEUM: Balance notes

Balance is done with repeatable test runs and recorded results, not guesswork (spec Phase 16).
Each entry: what was measured, how (seed / test script), the result, and the change made.

Fixed by the spec (not balance levers): 3v3, 5-unit roster, level 5-30, 20-turn limit,
3-win reward gold 100-500, 3 Recover charges per floor, shop prices +~10% per purchase.

Open levers (to tune in Phase 16): gold per battle, Recover cost, shop base prices and
price bounds, EXP curve, enemy scaling per floor, relic/skill rarity weights, Legacy tier thresholds.

_No measurements yet._

## Starting characters (Phase 4, 2026-09-25) — derived, not yet measured

Rule: each character's FE8 bases moved to level 5 by its average growth
(`base + round(growth x (5 - FE8 base level) / 100)`, min 0). Casters (Lute, Knoll, Natasha,
Artur, L'Arachel) had Magic in FE8's Atk column: Str set to 0 / 10% growth, Magic from
MagCharEditor.csv. Personal values below; the class bases are added in game.

| Char (FE8 Lv) | HP | Str | Mag | Skl | Spd | Def | Res | Lck | Growths HP/Str/Mag/Skl/Spd/Def/Res/Lck |
|---|---|---|---|---|---|---|---|---|---|
| Joshua (5) | 8 | 4 | 0 | 4 | 5 | 3 | 2 | 7 | 80/35/20/55/55/20/20/30 |
| Marisa (5) | 8 | 4 | 1 | 3 | 3 | 3 | 2 | 9 | 75/30/10/55/60/15/25/50 |
| Gerik (10) | 10 | 8 | 0 | 3 | 3 | 4 | 3 | 6 | 90/45/10/40/30/35/25/30 |
| Garcia (4) | 9 | 4 | 0 | 5 | 3 | 3 | 1 | 3 | 80/65/5/40/20/25/15/40 |
| Gilliam (4) | 9 | 4 | 0 | 4 | 3 | 1 | 3 | 3 | 90/45/5/35/30/55/20/30 |
| Franz (1) | 3 | 4 | 1 | 5 | 4 | 1 | 2 | 4 | 80/40/10/40/50/25/20/40 |
| Neimi (1) | 2 | 3 | 3 | 4 | 4 | 1 | 3 | 6 | 55/45/15/50/60/15/35/50 |
| Colm (2) | 4 | 2 | 1 | 4 | 3 | 2 | 2 | 9 | 75/40/10/40/65/25/20/45 |
| Vanessa (1) | 5 | 2 | 3 | 4 | 8 | 4 | 4 | 6 | 50/35/25/55/60/20/30/50 |
| Cormag (9) | 7 | 5 | 0 | 4 | 3 | 3 | 1 | 3 | 85/55/5/40/45/25/15/35 |
| Lute (1) | 3 | 0 | 8 | 4 | 7 | 2 | 3 | 10 | 45/10/65/30/45/15/40/45 |
| Knoll (9) | 2 | 0 | 8 | 6 | 5 | 0 | 4 | 0 | 70/10/50/40/35/10/45/20 |
| Natasha (1) | 4 | 0 | 3 | 3 | 8 | 3 | 2 | 8 | 50/10/60/25/40/15/55/60 |
| Artur (2) | 3 | 0 | 7 | 7 | 7 | 1 | 3 | 3 | 55/10/50/50/40/15/55/25 |
| L'Arachel (3) | 4 | 0 | 6 | 6 | 8 | 3 | 4 | 13 | 45/10/50/45/45/15/50/65 |

## Developer play-test (2026-09-27)

The developer found the first Elite a little too hard. Changes: the first Elite is gentler
(Champion -2 levels, +6 HP / +1 stats; squads -1 level), the run starts with 2 Common + 1 Rare
relic, and player EXP is x1.5. For a floor-1 Champion after 3 wins, that makes it Lv 6 instead
of Lv 8. Nothing has been measured yet: re-check after the next play-test.

Skill catalog (src/skills/Skills.event, Phase 6): rarities and prerequisites are first guesses;
offer weights C 40 / U 30 / R 18 / E 9 / L 3; enemy floor skills at levels 10/16/22.

To check in Phase 16: Gilliam's low personal Def (the Knight class base carries it), Knoll's
0 Lck/Def, the casters' Str 0.
