# COLISEUM: Handoff (where we stopped, what comes next)

Last session: 2026-09-25/26. Last commit: see `git log -1`. Everything below is committed.
The developer play-tested the latest build (New Game -> team -> Prepare -> battles -> level-up
choice -> shop -> next battle) and confirmed it works.

## Where we are

Phases 1-8 of 17 are done (spec §86). Details per phase: `TODO.md`; decisions: `GAME_DESIGN.md`;
test evidence: `TEST_STATUS.md`; changes: `CHANGELOG.md`; engine notes: `ARCHITECTURE.md`.

| Phase | State | In one line |
|---|---|---|
| 1 Analysis | done | Architecture and reuse plan (`ARCHITECTURE.md`) |
| 2 Foundation | done | `scripts/build.py`, C pipeline (gcc + lyn), run state + save chunks, on-target tests |
| 3 Combat | done | Reusable battle chapter (slot 0), 3v3, 20-turn limit, combat tests |
| 4 Units | done | 15 characters at Lv 5, cap 30, no stat caps, pick-1-of-3 stat on level-up |
| 5 Roster | done | Team screen, recruitment/replacement, deployment, permadeath, run ends on a loss |
| 6 Skills | done | 1 personal + 3 slots (class skills in slots), catalog with rarity/prerequisites |
| 7 Weapons | done | No durability (weapons AND staves), fusion, transfer, 3x proficiency, variants |
| 8 Shop | done | Battle gold, 8-entry shop (+10% compounding prices), Prepare menu |
| 9-17 | not started | See "Next" |

Tests: `py -3 scripts/build.py --test && py -3 tests/run_tests.py` -> 8 run-state + 38 map tests +
save/brightness checks, all passing.

## How the game flows right now

New Game -> (chapter fades in) "Your team" (3 random characters, Begin) -> [every 3 wins:
recruitment offer] -> "Prepare" menu (Fight! / Shop / Deploy (>3 alive) / Transfer / Fuse) ->
3v3 battle on the arena map -> win: gold, HP kept, new shop stock, FE8 save screen -> next battle.
Level-ups show the stat-choice menu. Losing (all deployed dead, or turn 20 passes) ends the run and
invalidates its save. Elite/Boss encounters are counted by the run state but still play as normal
fights (their content is Phases 11-12).

## Developer decisions to remember (all in GAME_DESIGN.md)

- Class skills count toward the 4 skill slots; "no stat caps" = ceiling 127; Str/Mag split on.
- All deployed units dead = the run is over (even with reserves alive).
- Weapons AND staves have unlimited uses; consumables are still used up.
- New separate project in `C:\Users\RdotS\Downloads\Coliseum`, built on the FE8 Skill System.

## Next (in order)

1. **Phase 9 - Relics** (spec §33-36): 2 relic slots per unit, rarity, positive/negative/conditional
   effects, transfer, duplicates, build-changing effects, reward-modifying relics. Then add the
   Relic category to the shop (Col_ShopGenerate in `src/shop/shop.c` skips it for now). Relics
   need run-state space (per roster slot) - the run state has room (0x54-0xFF free).
2. **Phase 10 - Roguelike progression**: post-battle menu (Next Fight / Shop / Recover) replacing
   FE8's save screen between fights and taking over the Prepare menu; the real 3-win reward choice
   (Recruit / Skill / Promotion / Heal / Gold 100-500) - today every 3-win reward is a recruitment
   offer (interim); Recover (3 per floor, gold cost, full heal); promotion design (branching paths;
   FE8 resets level to 1 on promotion - decide how that fits "level 5-30").
3. Phase 11 Elites (Champion / Elite squads, synergy, Elite AI, the Elite weapons already made:
   Keen Edge, Titan Axe, Gale Lance, Hawk Bow). Phase 12 Bosses (5 bosses, Phase 2 at 50% HP, boss
   weapons already made: Tyrant Blade, Warlord Pike, Ruin Cleaver, Storm Longbow, Abyss Tome).
4. Phases 13-17: Legacy/Hall of Champions, story, presentation, balance, full QA.

## Small open items

- FE8's vanilla death quotes still play for pool characters (replace in Phase 14).
- FE8's title menu has 3 save slots and hides "New Game" when all 3 are used; the spec wants one
  active run (§77). Make New Game always available (Phase 15).
- Enemy-phase level-up -> stat choice at the next player phase: same code as the tested path, but
  never played through (TEST_STATUS.md says NOT VERIFIED).
- Balance numbers are first guesses (gold, prices, recruit level, skill weights, enemy skills):
  Phase 16 measures them (`BALANCE_NOTES.md`).
- The developer's old save is `Coliseum.sav.bak-2026-09-25` (git-ignored) if ever needed.

## Gotchas learned the hard way (details in ARCHITECTURE.md)

- The Skill System's battle calc loop zeroes r11 -> all COLISEUM C is built with `-ffixed-r11`.
- FE-CLib mapped `%` (modulo) to FE8 routines with another calling convention -> fixed with
  `src/core/divmod.c` + a stripped reference; lyn runs with `-nohook`.
- Skill System battle hits are at 0x0203AAC0 (8 bytes each), not vanilla gBattleHitArray.
- The Skill System caches unit skills: call `InitSkillBuffers()` after changing skills.
- Call `ResetTextFont()` before every COLISEUM menu, or text tiles run out after a few menus.
- A chapter starts faded to black; menus in the beginning event need `FADU` first. The scripted
  screenshots read VRAM and ignore fades - check brightness via gLCDControlBuffer (the runner does).
- Close mGBA before building the player ROM (`Coliseum.gba` is locked while it is open); the test
  ROM (`Coliseum_test.gba`) is a debug build with a boot menu.
- Bash heredocs choke on apostrophes in this environment: write multi-line patch scripts with the
  Write tool and run them with `py -3 <file>`.

## Starting next session

1. Read `CLAUDE.md`, then this file, then `TODO.md`.
2. `py -3 scripts/build.py --test && py -3 tests/run_tests.py` to confirm everything still passes.
3. Start Phase 9 (Relics) unless the developer asks for something else.
