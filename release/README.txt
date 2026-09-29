THE COLOSSEUM  v0.1.0
A Fire Emblem: The Sacred Stones roguelike (ROM hack, UPS patch)
Status: early playable prototype / development build

-----------------------------------------------------------------
WHAT IT IS
-----------------------------------------------------------------
The Colosseum turns FE8 into a run-based arena roguelike. You start
with 3 random characters and fight short 3-vs-3 battles, one after
another. Between fights you shop, equip relics, fuse weapons and pick
rewards. Characters who fall are gone for the rest of the run, and a
lost battle ends the run.

It is built on the community FE8 Skill System (Event Assembler
buildfile). You need your own copy of the game; this package holds
only a patch (see PATCHING_GUIDE.txt).

-----------------------------------------------------------------
WHAT IS IN v0.1.0
-----------------------------------------------------------------
Runs and battles
- 3-vs-3 battles with a 20-turn limit (warnings on turns 15, 18, 19).
- A floor is 9 normal fights with an Elite battle after every 3 wins.
- HP carries over between battles. Recover (3 charges per floor,
  costs gold) heals the whole team.
- Permadeath. The run ends when every deployed unit has fallen, or
  when turn 20 ends.
- Autosave after every win. "Resume Chapter" on the title screen
  continues a suspended battle.

Characters
- A pool of 25: 15 FE8 characters and 10 original characters with
  FE-Repo community portraits and their own battle palettes.
- Everyone starts at level 5. The level cap is 30, and there are no
  stat caps (stats go up to 99 on screen).
- On every level-up you pick 1 of 3 stat increases.
- Player units gain 1.5x battle EXP.
- Promotion at level 10+ keeps the unit's level. FE8's branching
  class choice is kept.

Skills
- 1 personal skill + 3 slots (class skills use slots too).
- 48 player skills in 5 rarities, some with prerequisites.

Weapons
- Weapons and staves never break. Consumables are still used up.
- Weapon EXP is gained 3x faster (FE8 rank thresholds).
- Transfer weapons between characters who can use them.
- Fusion: two weapons in one inventory become a better one
  (Iron > Steel > Silver lines, tome lines, a few special pairs).
- Elite weapons dropped by Champions.

Economy and rewards
- Gold from every win (more from Elites).
- An 8-entry shop that is fully random every round: weapons,
  consumables, skills, relics, recruits, promotion items, healing.
  Every purchase raises later prices by 10% for the rest of the run.
- Every 3 wins: choose 1 of 3 rewards (Recruit / Skill / Promotion /
  Heal / Gold).
- Enemies can drop gold, skills or relics. The first battle of every
  run always has a relic carrier.

Relics
- 15 relics (Common to Epic) with stat, damage, gold and other
  effects. A run starts with 2 Common + 1 Rare relic.

Arenas
- 8 arenas: Grand Colosseum, Forest, Desert, Volcanic, Ruined
  Cathedral, Royal, Abyss, Misty Ruins.
- Weather (rain, sandstorm, ashfall, fog of war), hot-rock hazard
  tiles and sacred healing tiles. The enemy AI avoids hazards.

Interface
- Press R on any Colosseum menu row for help (skills, relics, items,
  characters, rewards).

-----------------------------------------------------------------
NOT IN v0.1.0 (planned)
-----------------------------------------------------------------
- Bosses. The 10th fight of a floor is a stronger normal fight, and
  floors still advance. There is no final boss yet, so a run goes on
  until it is lost.
- Elite-specific AI (Elites use FE8's normal AI).
- Legacy / Hall of Champions, run records, story and dialogue.
- A custom title screen, custom music or custom animations.
- The planned Shard and extended Fusion system.
Full list: KNOWN_ISSUES.txt.

-----------------------------------------------------------------
FILES
-----------------------------------------------------------------
The_Colosseum_v0.1.0.ups  the patch
README.txt                this file
PATCHING_GUIDE.txt        how to apply the patch
RELEASE_NOTES.txt         what this release is
CHANGELOG.txt             development history
KNOWN_ISSUES.txt          known problems and missing features
CREDITS.txt               credits and licences
screenshots/              in-game screenshots (2x scale)

No ROM is included, and none will be provided.
