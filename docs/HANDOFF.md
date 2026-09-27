# COLOSSEUM: Handoff (where we stopped, what comes next)

Last session: 2026-09-26 (Phase 9, relics). Last commit: see `git log -1`. Everything below is
committed. The developer play-tested Phases 1-8 (New Game -> team -> Prepare -> battles ->
level-up choice -> shop -> next battle). **Phase 9 (relics) was checked in game by Claude
(scripted, screenshots) but not yet play-tested by the developer.**

## Where we are

Phases 1-9 of 17 are done (spec §86). Details per phase: `TODO.md`; decisions: `GAME_DESIGN.md`;
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
| 9 Relics | done | 15 prototype relics, generic modifier engine, Prepare > Relics, shop relics (`docs/RELICS.md`) |
| 10-17 | not started | See "Next" |

Tests: `py -3 scripts/build.py --test && py -3 tests/run_tests.py` -> 8 run-state + 52 map tests +
save/brightness checks, all passing.

## How the game flows right now

New Game -> (chapter fades in) "Your team" (3 random characters, Begin) -> [every 3 wins:
recruitment offer] -> "Prepare" menu (Fight! / Shop / Deploy (>3 alive) / Transfer / Fuse / Relics) ->
3v3 battle on the arena map -> win: gold, HP kept, new shop stock, FE8 save screen -> next battle.
Level-ups show the stat-choice menu. Losing (all deployed dead, or turn 20 passes) ends the run and
invalidates its save. Elite/Boss encounters are counted by the run state but still play as normal
fights (their content is Phases 11-12).

## Developer decisions to remember (all in GAME_DESIGN.md)

- Class skills count toward the 4 skill slots; "no stat caps" = ceiling 127; Str/Mag split on.
- All deployed units dead = the run is over (even with reserves alive).
- Weapons AND staves have unlimited uses; consumables are still used up.
- New separate project in `C:\Users\RdotS\Downloads\Colosseum`, built on the FE8 Skill System.

## Latest (2026-09-27)

After the developer's play-test: the first Elite is gentler, runs start with 2 Common + 1 Rare
relics (listed on the team screen), and player EXP is x1.5 (CHANGELOG.md). All tests pass.
Still open from 2026-09-26: view Selene's recoloured dance screenshot; screenshot the promoted
palettes not yet seen (Morrow's Berserker, Silas's Mage Knight, Idris's Sage, Hale's Ranger,
Dagny's and Oriane's Great Knight).

Arena system milestone 1 (docs/ARENAS.md): 8 arenas, weather, hot rock, sacred seal,
hazard-aware AI, Arena debug menu (test/debug builds). Next after that: Phase 10.

Phase 10 done (2026-09-27): 3-win reward menu, promotion (keeps level, Lv 10+), Recover,
post-battle Prepare menu, autosave. Next: developer play-test, then Phase 11 (Elite AI).

## IN PROGRESS: release package v0.1.0 (stopped 2026-09-28, resume here)

The developer pasted a 27-section release prompt ("THE COLOSSEUM - RELEASE PATCH & SHOWCASE
PACKAGE"): UPS patch, verified patch test, README, patching guide, credits, changelog, known
issues, FEUniverse post, 6-10 screenshots, clean ZIP without ROMs, honest wording, final report.
If more detail is needed, ask the developer to paste it again. Done so far:
- Clean build verified (build/ deleted, rebuilt; identical ROM). `scripts/build.py --output PATH`
  builds the player ROM elsewhere while Colosseum.gba is open in mGBA.
- `scripts/ups.py create|apply`: UPS patches in byuu's format (varints, XOR hunks, 3 CRC32s);
  checked: clean FE8 + patch = byte-identical ROM (SHA-1), wrong base / damaged patch rejected,
  500 random round trips. Patch size ~500 KB.
- Base ROM: Fire Emblem - The Sacred Stones (USA, Australia), 16,777,216 bytes,
  SHA-1 c25b145e37456171ada4b0d440bf88a19f4d509f, MD5 005531fef9efbb642095fb8f64645236,
  CRC32 A47246AE.
- Play-tested on the PATCHED ROM (clean FE8 + UPS) in mGBA, scripted with screenshots: boots to
  the (vanilla FE8) title, New Game, team screen with starting relics, Prepare, shop + R help,
  equipping a relic (stat screen shows Wind Soul's Spd +1 / Def -1), a real player attack by
  button presses (Silas's Fire 17 -> 11, counter 19 -> 17), battle animation, the victory event,
  autosave (run copy with 1 win in SRAM), 3-win reward menu + help.
- Fixed while testing: B works as Back in menus with a Back row; R help reopening showed a
  garbled line (wrong cache address).
- Licenses: only LICENSE (Skill System, CC0) in the repo; FE-Repo portraits are F2E with credit
  (CREDITS.md). No custom music or animations. Build tools are not shipped.

Still to do (in order):
1. Finish the play-test on the patched ROM: Elite battle, a second arena (e.g. Volcanic),
   save -> quit -> Continue (the run resumes). The scripted drivers are in tests/release/
   (play.py / t2.py / t3.py; they expect patched.gba and built.sym next to them - copy them into
   a scratch folder with those files first). Driver gotchas:
   delete stale .sav files next to the ROM copy; wait until enemies are placed before input;
   after the shop the Prepare cursor returns to the top.
2. Screenshots (native 240x160, 2x integer scale): title, run start (team), 3v3 battle, stat
   screen, relics, Elite, [no bosses yet -> 3-win reward], arena (Volcanic/Desert), shop,
   [no Legacy -> battle animation].
3. Docs in release/: README.txt, PATCHING_GUIDE.txt, CREDITS.txt, CHANGELOG.txt,
   KNOWN_ISSUES.txt, FEUNIVERSE_POST.txt, RELEASE_NOTES.txt. Describe ONLY what is built:
   no real bosses yet (fight 10 is a stronger normal fight, floors still advance), no Legacy, no
   custom title screen, arenas = the 8 of docs/ARENAS.md, Elite AI is FE8's.
   Status: "early playable prototype / development build", version v0.1.0 everywhere.
4. scripts/make_release.py: build -> UPS -> apply to the clean ROM and compare -> assemble
   release/ -> The_Colosseum_v0.1.0.zip (no ROMs) -> extract and check the contents.
5. Final clean-room test on the patched ROM, commit, report to the developer.

## Next (in order)

0. **Developer play-test of Elites, drops and the random shop** (2026-09-26 additions, checked by
   Claude in game with the test ROM). Still open for Phase 11: advanced Elite AI (spec 49).
1. **Developer play-test of the relics** (Prepare > Relics; shop relics; Blood Pact, Guardian's
   Crest, Golden Thread in real fights). The developer should paste the full list of relic concepts
   discussed elsewhere into `docs/RELICS_FUTURE.md`. The relic prompt arrived cut off after its
   section 9: ask whether sections 10+ had more requirements.
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
- The developer's old save is `Colosseum.sav.bak-2026-09-25` (git-ignored) if ever needed.

## Gotchas learned the hard way (details in ARCHITECTURE.md)

- The Skill System's battle calc loop zeroes r11 -> all COLOSSEUM C is built with `-ffixed-r11`.
- FE-CLib mapped `%` (modulo) to FE8 routines with another calling convention -> fixed with
  `src/core/divmod.c` + a stripped reference; lyn runs with `-nohook`.
- Skill System battle hits are at 0x0203AAC0 (8 bytes each), not vanilla gBattleHitArray.
- The Skill System caches unit skills: call `InitSkillBuffers()` after changing skills.
- Call `ResetTextFont()` before every COLOSSEUM menu, or text tiles run out after a few menus.
- A chapter starts faded to black; menus in the beginning event need `FADU` first. The scripted
  screenshots read VRAM and ignore fades - check brightness via gLCDControlBuffer (the runner does).
- Close mGBA before building the player ROM (`Colosseum.gba` is locked while it is open); the test
  ROM (`Colosseum_test.gba`) is a debug build with a boot menu.
- Bash heredocs choke on apostrophes in this environment: write multi-line patch scripts with the
  Write tool and run them with `py -3 <file>`.

## Starting next session

1. Read `CLAUDE.md`, then this file, then `TODO.md`.
2. `py -3 scripts/build.py --test && py -3 tests/run_tests.py` to confirm everything still passes.
3. Start Phase 9 (Relics) unless the developer asks for something else.
