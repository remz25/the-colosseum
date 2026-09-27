# COLOSSEUM
## Claude Code Master Development Prompt
### Version 1.0

> Saved verbatim from the developer's master specification (2026-09-25) (the name "Coliseum" was later changed to "Colosseum" throughout, at the developer's request). This is the design
> source of truth. Clarifications and decisions made since are recorded in `GAME_DESIGN.md`.

---

# 1. ROLE

You are Claude Code acting as the lead technical developer for an original Fire Emblem-inspired roguelike Colosseum game built on the **Fire Emblem 8 / Sacred Stones GBA buildfile ecosystem**.

You are responsible for:

- Game systems
- FE8/buildfile integration
- Tactical combat
- Unit systems
- Enemy AI
- Skills
- Classes
- Promotion
- Weapons
- Relics
- Shops
- Roguelike progression
- Elite encounters
- Boss encounters
- Legacy system
- Save system
- UI
- Debugging tools
- Automated testing
- Documentation
- Build pipeline
- Balancing support

You should work autonomously on technical implementation whenever the design specification provides enough information.

You MUST NOT make major gameplay decisions that contradict this document.

If a missing decision would materially change gameplay, stop and ask the developer.

For minor implementation decisions, choose the simplest robust solution, document the decision, and continue.

---

# 2. CORE GAME VISION

The game is a Fire Emblem-style tactical roguelike set inside a mysterious supernatural Colosseum.

The primary gameplay inspiration is:

1. Fire Emblem
2. Roguelike games
3. Slay the Spire-style progression
4. Traditional tactical RPGs

Fire Emblem is the dominant influence.

The player controls a commander attempting to survive multiple Colosseum floors.

Each run creates a unique team and history.

The defining systems are:

- 3v3 tactical battles
- Maximum 5-unit roster
- Permanent death
- Random recruitment
- Skill builds
- FE-style level progression
- Promotion
- Relics
- Weapon upgrading
- Elite encounters
- Bosses
- Legacy characters and Legacy weapons
- Persistent progression between runs

---

# 3. TECHNICAL FOUNDATION

Use:

- Fire Emblem 8 / Sacred Stones
- Modern FE8 buildfile ecosystem
- FERepo resources and systems wherever appropriate
- Git-based source control
- Automated ROM building

Do not reinvent existing FERepo systems when a reliable system already exists.

Extend existing systems rather than creating incompatible duplicate frameworks.

The project must remain compatible with the GBA's actual technical constraints.

---

# 4. DEVELOPMENT PHILOSOPHY

Build incrementally.

Never attempt to implement the entire game at once.

Use:

DESIGN → SMALLEST WORKING IMPLEMENTATION → TEST → BUILD ROM → VERIFY → EXPAND → TEST AGAIN

Every major system must have tests.

Never move to a dependent system while the underlying system is fundamentally broken.

---

# 5. CORE RUN STRUCTURE

A run consists of multiple Colosseum Floors.

Each floor contains approximately:

- Normal battles
- Elite battles
- One boss battle

A typical floor is:

Battle 1 → Battle 2 → Battle 3 → Elite → Battle 4 → Battle 5 → Battle 6 → Elite → Battle 7 → Battle 8 → Battle 9 → Boss / Fight 10 → Next Colosseum Floor

The boss replaces the Elite that would otherwise occur around Fight 10.

The run continues through multiple floors.

The ultimate goal is to eventually reach and defeat the final boss.

---

# 6. NO OVERWORLD MAP

There is no traditional overworld or branching Slay-the-Spire map.

After completing a battle, the player receives direct options.

The game immediately loads the selected next situation.

The standard post-battle choices are:

- Next Fight
- Shop
- Recover

Elite encounters occur automatically after the appropriate number of victories.

Bosses occur at the end of the floor.

---

# 7. STARTING A RUN

At the beginning of a run:

- All 15 playable characters are eligible.
- The game randomly selects 3 characters.
- The player is shown the three characters.
- The player accepts the starting team.
- The run begins.

There is no starting reroll unless explicitly added later.

The starting roster therefore contains: 3 / 5 slots occupied.

The player enters the first battle with those 3 characters.

---

# 8. ROSTER SYSTEM

Maximum living roster size: **5 units**

Maximum deployed team: **3 units**

The remaining units are reserves.

Example: Roster: 1. Knight 2. Mage 3. Archer 4. Cleric 5. Wyvern Rider. Battle team: 3 selected. Reserve: 2.

The player can change which 3 units are deployed between battles.

---

# 9. RECRUITMENT

Recruitment is a major roguelike progression system.

When Recruitment is offered:

- Generate 3 random eligible characters.
- The player chooses 1.

If there is an empty roster slot: the new character occupies that slot.

If the roster is already full: the player chooses one existing character to replace. The replaced character is permanently removed from that run.

Recruitment is optional. The player may decline.

A character cannot be recruited twice within the same run.

Promoted characters may occasionally appear as rare recruits.

New recruits scale appropriately to the current progression of the run.

New recruit level, stats, skills and equipment should be appropriate to the current floor.

---

# 10. CHARACTER POOL

Initial playable roster: **15 characters**

All 15 are available from the beginning.

Recruitment uses the 15-character pool.

Characters should have distinct identities.

Characters sharing a class should still feel different through:

- Personal skills
- Personal growth rates
- Starting equipment
- Character-specific design
- Potential promotion choices
- Dialogue

---

# 11. CHARACTER PERSONALITY

Characters should have light personality and dialogue.

Story is not the primary focus.

Use:

- Short introductions
- Recruitment dialogue
- Character dialogue
- Boss dialogue
- Colosseum atmosphere
- Short narrative moments

Do NOT implement a full support system.

No traditional FE Support system is required.

---

# 12. DEATH

Death is permanent within the current run.

If a unit dies:

- They are removed from the living roster.
- They cannot be revived.
- They cannot be recruited again during that run.
- Their death affects the player's future roster options.

If the player loses all 5 roster slots: **RUN OVER**

If the player has only 1 or 2 living units: the player must continue fighting with those remaining units.

Example: 2 surviving units. Player: 2, Enemy: 3. Battle continues as 2v3. This is intentional.

---

# 13. BATTLE OBJECTIVE

Default battle objective: **Defeat all enemies.**

No retreat.

Once combat begins, the player is committed.

There is no escape option.

---

# 14. TURN LIMIT

Every battle has a maximum of: **20 turns**

The player must defeat all enemies before the turn limit expires.

If the battle has not been completed by the end of Turn 20: **DEFEAT**

Turn warnings should appear:

- Turn 15: "5 turns remaining."
- Turn 18: "2 turns remaining."
- Turn 19: "FINAL TURN."

---

# 15. COMBAT

Combat should closely follow traditional Fire Emblem GBA mechanics.

Use FE-style: HP, Strength, Magic, Skill, Speed, Luck, Defense, Resistance.

Derived statistics should follow the FE8 framework where appropriate.

Use: Attack, Hit, Critical, Avoid, Dodge, Attack Speed, Weapon Might, Weapon Accuracy.

Use traditional Fire Emblem doubling. If the attacker has sufficient Attack Speed advantage, they may attack twice.

Use traditional Fire Emblem critical hit principles.

Magic uses: Magic → Resistance

Physical attacks use: Strength → Defense

---

# 16. WEAPON TRIANGLE

Use the traditional weapon triangle: Sword > Axe > Lance > Sword

Magic and ranged weapons operate according to appropriate FE8 mechanics.

Do not unnecessarily reinvent established FE mechanics.

---

# 17. MOVEMENT

Use traditional Fire Emblem grid-based movement.

Classes have movement values.

Movement is affected by: Terrain, Unit type, Class, Terrain restrictions.

---

# 18. TERRAIN

Terrain must matter.

Use appropriate FE8-style terrain effects including: Defense, Avoid, Movement cost, Restrictions, Healing where appropriate.

Terrain should be part of tactical decision-making.

---

# 19. HEALING

Use traditional Fire Emblem healing.

Staff users can heal allies.

Healing is balanced naturally because a dedicated healer generally sacrifices an attacking action.

There is no mana system. There is no MP system.

Staffs should use traditional FE-style behaviour.

Include appropriate staff types such as: Heal, Mend, Physic, Fortify, Warp, Rescue, Restore.

Use FE8 mechanics where practical.

There is no weapon/staff durability requirement for this project.

---

# 20. ITEMS / CONSUMABLES

Use traditional FE-style consumables where appropriate: Vulnerary, Elixir, Pure Water, Antitoxin, Other suitable FE8 consumables.

Consumables can be used during battle.

Using an item consumes the unit's action.

There is no healing outside battle except through explicitly defined systems such as: Staffs during battle, Recover, Other explicitly designed effects.

---

# 21. LEVELS

Starting level: **5**

Maximum level: **30**

Leveling happens automatically.

No stat caps. Do not impose traditional FE stat caps.

---

# 22. EXPERIENCE

Use traditional Fire Emblem-style EXP.

EXP should consider: Enemy level, Player level, Enemy strength, Enemy type, Boss status, Other FE8-compatible factors.

Bosses should provide substantial EXP.

Higher-level enemies should generally provide appropriate EXP.

---

# 23. LEVEL-UP SYSTEM

Level-ups combine:

1. Personal growth rolls
2. Player choice

Personal growth rates determine random stat gains. Example: a character may naturally gain +1 HP, +1 Strength, +1 Speed.

The game then generates **3 random stat choices**. Example: +1 HP, +1 Defense, +1 Speed.

The player chooses one.

Final result: Natural growth: +1 HP, +1 Strength, +1 Speed. Player choice: +1 Defense. Total: +1 HP, +1 Strength, +1 Speed, +1 Defense.

The player may choose a stat that already increased. Example: Natural: +1 Strength. Choices: +1 Strength, +1 Speed, +1 Defense. Choosing Strength produces: +2 Strength.

The three choices may contain duplicate stats.

No stat caps.

---

# 24. PERSONAL GROWTH RATES

Every playable character should have personal growth rates.

Characters should therefore naturally develop differently.

Example: Character A: High Strength / Defense. Character B: High Speed / Skill. Character C: High Magic / Resistance.

This should combine with the player's level-up choices to produce different builds.

---

# 25. CLASSES

Use an FE8-inspired class structure.

Initial classes can include:

Physical: Myrmidon, Mercenary, Fighter, Knight, Cavalier, Archer, Thief, Pegasus Knight, Wyvern Rider

Magic: Mage, Shaman, Cleric

Advanced classes can include: Swordmaster, Hero, Warrior, General, Paladin, Great Knight, Sniper, Assassin, Wyvern Lord, Falcon Knight, Sage, Druid, Bishop

Exact roster can be expanded during implementation.

---

# 26. PROMOTION

Promotion is permanent within a run.

Promotion changes: Class, Stats, Weapons, Movement, Abilities, Skills.

Promotion can provide new skills.

Promotion is primarily obtained through: 3-win reward, Boss-related promotion items, Shop.

Promotion should use branching class paths where appropriate. Example: Cavalier → Paladin OR → Great Knight.

The player chooses the promotion path.

---

# 27. SKILL SYSTEM

Every playable unit has: **1 Personal Skill** plus **3 additional skill slots**

Maximum: **4 total equipped skills**

The Personal Skill:

- Cannot be removed
- Cannot be replaced
- Cannot be changed

The remaining 3 slots are flexible.

---

# 28. SKILL RARITY

Skill rarity: 1. Common 2. Uncommon 3. Rare 4. Epic 5. Legendary

---

# 29. SKILL SOURCES

Skills can be obtained through: Level/progression systems where appropriate, 3-win rewards, Shops, Bosses, Elite encounters, Promotion, Relics.

There are no random-event nodes.

If a unit has all 4 skill slots occupied and receives a new skill: the player must choose one non-personal skill to replace.

The Personal Skill can never be removed.

---

# 30. SKILL PREREQUISITES

Skills may have: Class requirements, Promotion requirements, Weapon requirements, Stat requirements, Other logical prerequisites.

Use these to preserve balance and create meaningful builds.

---

# 31. SKILL DUPLICATION

A unit cannot equip duplicate copies of the exact same skill.

Different bonuses may stack unless explicitly marked as non-stacking.

---

# 32. ENEMY SKILLS

Enemies use the skill system.

Normal enemies can have skills where appropriate.

Elite enemies should have more sophisticated builds.

Bosses should have unique abilities.

Elite and boss enemies may have skills unavailable to normal playable units.

---

# 33. RELICS

Relics are one of the major roguelike systems.

Each character can equip: **2 Relics**

Relics are character-specific equipment.

Relics can be transferred between characters outside battle.

Relics can be duplicated. A character may theoretically equip Relic A + Relic A if the game permits the duplicate.

Relics work for all classes unless specifically restricted.

---

# 34. RELIC RARITY

Use: Common, Uncommon, Rare, Epic, Legendary, Mythic

---

# 35. RELIC DESIGN

Relics can provide: Positive effects, Negative effects, Conditional effects, Build-defining effects, Team synergy, Skill modifications, Stat changes, Gold effects, Healing effects, Promotion effects, Reward modifications.

Cursed/negative relic effects must be visible to the player before equipping.

---

# 36. REWARD MODIFICATION RELICS

Relics may modify game rules. Example: a relic may allow "Choose 2 rewards instead of 1."

Such effects must be clearly communicated.

---

# 37. GOLD

Gold is the primary currency.

Gold can purchase: Recruit, Skill, Weapon, Weapon upgrade, Relic, Healing/Recover, Promotion, Stat-related options where implemented, Consumables.

Normal battles award Gold.

Gold rewards should be balanced according to progression.

---

# 38. SHOP

The Shop is always available after a battle.

Shop categories: 1. Recruit 2. Skill 3. Weapon 4. Relic 5. Healing/Recover 6. Promotion 7. Consumable

The shop inventory is random. However, it must contain at least one item from each relevant category.

No shop reroll system.

---

# 39. SHOP PRICE SCALING

Shop prices increase by approximately **10%** after each purchase.

The exact implementation should be documented and tested.

Prices should scale throughout the run while respecting reasonable minimum/maximum boundaries.

---

# 40. WEAPONS

Weapon durability is removed.

Weapons can be freely used without breaking.

Weapons can be transferred between compatible characters.

---

# 41. WEAPON PROFICIENCY

Use weapon proficiency. Examples: Sword, Lance, Axe, Bow, Anima, Light, Dark, Staff.

Proficiency should increase quickly enough that players are not forced into long grinding sessions before accessing better weapons.

The system should preserve FE-style weapon requirements without making progression tedious.

---

# 42. WEAPON FUSION

Weapons can be upgraded. Example: Iron Sword + Iron Sword = Steel Sword.

The player chooses when to combine weapons.

Fusion consumes both source weapons.

Do not automatically fuse weapons. This prevents accidental loss of useful duplicate equipment.

---

# 43. WEAPON VARIANTS

Elite and Boss encounters can provide special weapon variants.

Example: Killing Edge + special Elite version could produce a unique weapon with additional properties.

Boss weapons should be significantly more distinctive.

---

# 44. LEGACY WEAPONS

Legacy weapons are persistent rewards created by exceptional characters.

The weapon's name should include the character's name. Examples: Kael's Edge, Mira's Radiance, Alden's Axe.

Legacy weapons can be extremely powerful.

Their strength is determined by the character's performance.

---

# 45. RECRUITMENT BUILDS

A character's initial build should be fixed when they are recruited.

The character's identity should remain recognizable.

Randomness should primarily affect progression during the run.

Do not randomise the character into a completely different starting identity.

---

# 46. NORMAL BATTLES

Normal battles are generally: **3v3**

They should be relatively straightforward compared with Elite encounters.

Enemies still use appropriate tactical AI.

Normal enemies should scale according to Colosseum floor and run progression.

---

# 47. ELITE BATTLES

Elite battles occur automatically after every 3 normal victories.

Elite battles have two main formats.

### Champion

1 extremely powerful enemy vs 3 player units.

The Champion may have: Multiple skills, Unique weapon, Special abilities, Advanced AI, Unique rewards.

### Elite Squad

3 enemy units vs 3 player units.

The enemies are specifically designed to work together.

Example: Paladin: Defensive support. Bishop: Healing. Swordmaster: Finisher.

---

# 48. ELITE SYNERGY

Elite teams should be intentionally designed around synergy.

Enemy teams can: Protect each other, Heal each other, Set up kills, Exploit wounded targets, Control terrain, Bait player units, Coordinate skill combinations.

The player should be able to inspect enemy information before committing to attacks.

Do not explicitly explain every synergy to the player.

The player should discover the tactical relationships through gameplay.

---

# 49. ELITE AI

Elite enemies use advanced AI.

They should understand: Enemy threat, Kill opportunities, Survival, Terrain, Skills, Weapon matchups, Healing, Positioning, Team synergy, Objectives, Future tactical consequences.

---

# 50. ELITE REWARDS

Elite victories provide substantially better rewards.

Possible rewards include: Large Gold, Recruitment opportunity, Skill, Promotion, Weapon, Relic, Other rare rewards.

Elite enemies may drop their unique weapons.

Elite enemies may become recruitable.

After Elite rewards, the normal post-battle options return: Shop, Recover, Next Fight.

---

# 51. THREE-WIN REWARD

After every 3 normal victories, the player receives one major reward choice.

Options: 1. Recruit 2. Skill 3. Promotion 4. Heal 5. Gold

The choices are randomized.

The player chooses exactly one.

A relic may later modify this rule and allow additional selections.

---

# 52. THREE-WIN GOLD

The Gold reward option must always provide an amount between **100 and 500 Gold**.

The exact amount is randomized within that range.

---

# 53. PROMOTION REWARD

If Promotion is offered and an eligible character exists: the player chooses which eligible character to promote.

If no character is eligible: Promotion should be replaced by another valid reward rather than becoming useless.

---

# 54. RECOVER

Recover is available as a post-battle option.

Recover:

- Costs Gold
- Does not count as a fight
- Fully restores all living units
- Does not resurrect dead units

Each Colosseum floor provides: **3 Recover uses**

After defeating the floor boss: Recover charges reset to **3**

The exact Gold cost should be tuned during balancing.

---

# 55. BOSS BATTLES

A Boss occurs on Fight 10 of each Colosseum floor.

Bosses are unique characters.

Initial game: **5 major bosses**

Bosses should have: Unique identity, Unique weapon/abilities, Unique AI, Unique dialogue, Unique rewards, Second phase.

---

# 56. BOSS PHASE TWO

At **50% HP** the boss enters Phase 2.

Phase 2 does not completely replace the boss. Instead:

- Existing identity remains
- Stronger abilities activate
- Behaviour becomes more dangerous
- New skills/effects may activate

---

# 57. BOSS TELEGRAPHING

Before the boss, the game should provide hints about the area, enemy faction, weapons or abilities associated with the upcoming boss.

The player should have some opportunity to understand the type of challenge they are approaching.

The exact boss identity can be revealed appropriately.

---

# 58. BOSS POOLS

Bosses should not necessarily appear in the exact same order every run.

Example: Floor 1: 2 possible bosses. Floor 2: 3 possible bosses. Later floors: Additional possibilities.

This improves replayability.

---

# 59. BOSS REWARDS

Every boss should provide: Guaranteed unique Boss reward, Gold, Additional rewards.

Boss rewards may include: Unique weapon, Promotion item, Relic, Skill, Recruit, Other rare rewards.

Boss weapons can become available in future runs.

---

# 60. FINAL BOSS

The game should have an ultimate final boss beyond the initial five-boss pool.

Claude may design an original final boss concept that fits the supernatural Colosseum premise.

The final boss should be mechanically and narratively significant.

Do not simply make it a stronger normal boss.

---

# 61. COLOSSEUM THEMES

Each Colosseum floor can have a distinct theme. Examples: Grand Arena, Frozen Arena, Ruined Cathedral, Dragon Domain, Abyss.

Themes can influence: Terrain, Enemy factions, Music, Weapons, Bosses, Dialogue, Visual presentation.

---

# 62. ENEMY FACTIONS

Enemies should be organised into thematic factions such as: Kingdom soldiers, Mercenaries, Bandits, Assassins, Cultists, Monsters, Royal Guard, Other appropriate factions.

Enemy compositions should reflect faction identity.

---

# 63. ADVANCED AI

The AI is a major feature.

The AI should use layered tactical reasoning.

### Layer 1 — Individual Tactical Evaluation

Evaluate: Can I kill? Can I survive? What threatens me? What attacks are available? What terrain is advantageous?

### Layer 2 — Team Coordination

Evaluate: Who should heal? Who should protect? Who should attack? Who can finish a wounded target? Who can create a setup?

### Layer 3 — Skill Awareness

AI understands relevant skills. It should recognise threats such as: Vantage, Wrath, Counter, Critical bonuses, Defensive abilities, Healing abilities, Movement abilities.

### Layer 4 — Objective Awareness

AI understands: Protect boss, Attack priority targets, Control terrain, Preserve important units, Reach objectives.

### Layer 5 — Forward Planning

AI should evaluate likely consequences of actions. Example: "If I move here, the player can kill me next turn." "If I attack this unit, my ally can finish it." "If I protect the Bishop, the team remains sustainable."

---

# 64. AI HARDWARE CONSTRAINT

The AI should be as sophisticated as realistically possible within GBA CPU, memory and processing constraints.

Do not attempt an AI system that cannot practically run on the target hardware.

Prefer efficient tactical heuristics, scoring systems, threat evaluation and limited lookahead over computationally impossible systems.

Elite and Boss AI can receive more sophisticated processing than ordinary enemies where practical.

---

# 65. PLAYER INFORMATION

The player must be able to inspect enemy: HP, Stats, Class, Weapon, Skills, Relevant abilities.

Enemy attack ranges should be visible.

Enemy equipment should be visible.

The player must have enough information to make informed tactical decisions.

---

# 66. LEGACY SYSTEM

Legacy is one of the defining systems of the game.

A character who performs exceptionally during a run can leave behind a permanent Legacy.

The Legacy may be: **A powerful weapon** and/or **A permanent unlock** and **A Hall of Champions record**

---

# 67. LEGACY PERFORMANCE

Track: Battles entered, Battles survived, Victories, Enemies killed, Elite kills, Boss kills, Damage dealt, Damage taken, Healing performed, Final level, Promotions, Cause of death, Run progress.

Keep the Legacy calculation relatively simple.

Do not create an excessively complicated formula.

Performance should determine Legacy quality.

---

# 68. LEGACY TIERS

Use: 1. No Legacy / Forgotten 2. Notable 3. Heroic 4. Legendary 5. Mythic

The better the record, the stronger the Legacy.

---

# 69. FORGOTTEN CHARACTERS

Characters who die early or fail to achieve the required performance leave **No permanent Legacy.**

They are effectively forgotten after the run.

They should not receive a permanent Hall of Fame entry.

Their current run death may still appear in the run history.

---

# 70. LEGACY DEATH

A character can earn a Legacy even if they die.

The quality depends partly on: How far they reached, How many battles they survived, Kills, Elite kills, Boss kills, Damage/healing performance, Circumstances of death.

A character dying during a late-floor Boss fight may leave an exceptional Legacy.

A character dying very early with little contribution may leave nothing.

---

# 71. LEGACY ANNOUNCEMENT

When a character qualifies, display a major announcement. Example: **✦ MIRA HAS CREATED A LEGACY ✦**

Then reveal the Legacy item.

This should be a memorable moment.

---

# 72. LEGACY WEAPON GENERATION

Legacy weapons should be generated based on the character's achievements.

Example: Mira: 42 kills, 3 Elite kills, 1 Boss kill, Survived multiple floors. Could create: **Mira's Radiance** with powerful bonuses.

Legacy weapon naming should use the character's name.

---

# 73. LEGACY POOL

All unlocked Legacy items remain available in future runs.

The player does not need to manually select a subset.

The complete unlocked Legacy pool can appear in future runs.

Legacy items enter the normal shop pool randomly.

---

# 74. HALL OF CHAMPIONS

Characters who create a Legacy receive a permanent record.

Store: Character name, Class, Final level, Battles, Victories, Kills, Elite kills, Boss kills, Damage dealt, Damage taken, Healing, Promotion, Cause of death/survival, Legacy tier, Legacy weapon.

The player must be able to view this information.

---

# 75. LEGACY PERSISTENCE

Legacy information must survive between runs.

Use persistent save data.

The system must be designed so new Legacy types can be added later.

---

# 76. SAVE SYSTEM

There is **1 active run**.

Use automatic saving.

Save after every battle and important progression point.

Save should preserve: Current floor, Current fight, Roster, Dead characters, Levels, Stats, EXP, Skills, Weapons, Relics, Gold, Recover charges, Boss progress, Legacy unlocks, Run history.

---

# 77. RUN CONTINUITY

If the game is closed during a run, the player resumes the active run.

Do not create multiple active run slots in V1.

---

# 78. VISUAL STYLE

Initial presentation: **GBA+**

Use FE8/GBA as the visual foundation.

The initial prototype can use FERepo assets.

The underlying systems must remain separated from presentation wherever practical so custom art can replace placeholder content later.

---

# 79. FUTURE ART

Once the game systems are proven, replace or supplement placeholder assets with original content.

Potential future improvements: Custom portraits, Custom character sprites, Custom battle animations, Custom tilesets, Custom UI, Custom weapons, Custom effects.

The project should eventually move toward an original visual identity while retaining the GBA tactical presentation.

---

# 80. MUSIC

Use Fire Emblem-style music during initial development.

Battle music can dynamically intensify based on battle conditions.

Possible states: Normal combat, Dangerous combat, Boss, Boss Phase 2, Victory.

---

# 81. DEBUG MODE

Create a developer/debug mode from the beginning.

It should support functions such as: Give Gold, Give EXP, Level Unit, Promote Unit, Recruit Unit, Kill Unit, Heal Team, Give Weapon, Give Skill, Give Relic, Spawn Enemy, Start Elite, Start Boss, Win Battle, Skip Battle, Create Legacy, Reset Run, Modify Recover charges.

Debug tools must never be accessible during normal player gameplay.

---

# 82. TESTING

Every major system must be tested.

Required testing areas:

- **Combat:** Damage, Hit, Crit, Doubling, Weapon triangle, Terrain, Movement, Death, Turn limit
- **Units:** Growth rates, Level ups, Stat choices, EXP, Promotion, Class changes
- **Skills:** Acquisition, Replacement, Prerequisites, Personal skills, Skill limits, Duplicate prevention
- **Relics:** Equip, Transfer, Duplicate relics, Effects, Negative effects, Rarity
- **Weapons:** Equipment, Proficiency, Fusion, Variants, Boss weapons, Legacy weapons
- **Progression:** Gold, Shop, Price scaling, Recruitment, 3-win reward
- **Combat Progression:** Elite, Boss, Phase 2, Multiple floors, Recover charges
- **Legacy:** Statistics, Eligibility, Tier calculation, Death, Weapon generation, Persistence, Hall of Champions
- **Save:** Save, Load, Continue, Persistent Legacy data

---

# 83. INTEGRATION TESTING

Test complete sequences. Example:

Start run → receive 3 characters → complete battle → receive EXP → level up → receive stat choice → shop → recruit → replace unit → battle → death → continue 2v3 → 3-win reward → Elite → Elite reward → boss → Legacy → next floor → save → reload

The complete sequence must work.

---

# 84. BUILD TESTING

Claude must regularly verify: ROM builds, Tables compile, Event scripts compile, Assets exist, References are valid, No broken pointers, No missing data, No duplicate IDs, No invalid class/skill references.

---

# 85. DOCUMENTATION

Maintain: `GAME_DESIGN.md`, `ARCHITECTURE.md`, `TODO.md`, `CHANGELOG.md`, `TEST_STATUS.md`, `BALANCE_NOTES.md`

Update these as the project develops.

---

# 86. DEVELOPMENT PHASES

Implement in this order.

- **Phase 1 — Repository Analysis.** Before changing anything: Inspect FE8 repository, Understand build system, Identify existing systems, Identify reusable FERepo components, Identify limitations, Document architecture. Do not modify the project until this analysis is complete.
- **Phase 2 — Technical Foundation.** Project configuration, Data structures, IDs, Save framework, Debug framework, Testing framework.
- **Phase 3 — Tactical Combat.** 3v3, Grid, Movement, Terrain, FE calculations, Weapon triangle, Doubling, Crit, Death, 20-turn limit, Enemy AI foundation.
- **Phase 4 — Units.** 15 characters, Classes, Stats, Growth rates, EXP, Level 5 start, Level 30 maximum, No stat caps, Random stat choices, Personal skills.
- **Phase 5 — Roster.** 5-unit maximum, 3 deployment, 2 reserves, Recruitment, Replacement, Permanent death, 2v3 continuation, Run termination.
- **Phase 6 — Skills.** 4 total slots, Personal skill, Skill rarity, Skill prerequisites, Skill replacement, Class skills, Elite skills, Boss skills.
- **Phase 7 — Weapons.** Weapon types, Proficiency, No durability, Inventory, Transfer, Fusion, Weapon variants, Boss weapons.
- **Phase 8 — Shop & Economy.** Gold, Shop, 7 categories, Random inventory, Guaranteed category representation, 10% purchase inflation, Recruitment, Healing, Promotion, Consumables.
- **Phase 9 — Relics.** 2 relic slots, Rarity, Positive effects, Negative effects, Transfer, Duplicate relics, Build-changing effects, Reward modifiers.
- **Phase 10 — Roguelike Progression.** 3-win cycle, Random rewards, Recruit, Skill, Promotion, Heal, Gold, Recover, Recover charges.
- **Phase 11 — Elite Encounters.** Champion fights, Elite squads, Unique skills, Synergy, Elite AI, Elite rewards, Elite recruitment.
- **Phase 12 — Bosses.** 5 initial bosses, Boss pools, Boss dialogue, Boss weapons, Phase 2, Boss AI, Boss rewards, Multiple Colosseum floors.
- **Phase 13 — Legacy.** Statistics, Legacy eligibility, Legacy tiers, Legacy generation, Legacy weapons, Death Legacy, Hall of Champions, Persistent Legacy pool.
- **Phase 14 — Story.** Colosseum premise, Commander, Short introductions, Character dialogue, Boss dialogue, Floor themes, Final narrative.
- **Phase 15 — Presentation.** GBA+ presentation, UI, Menus, Battle UI, Roster UI, Shop UI, Relic UI, Legacy UI, Hall of Champions, Music, Effects.
- **Phase 16 — Balance.** Economy, EXP, Level progression, Skills, Relics, Weapon fusion, Elite difficulty, Boss difficulty, AI, 20-turn limit, Recover economy, Legacy strength. Do not balance by guesswork alone. Use repeatable test runs and documented results.
- **Phase 17 — Full QA.** Perform full run simulations. Test: Successful runs, Failed runs, 1-unit survival, 2-unit survival, Full roster death, Replacements, Promotion, Skill replacement, Elite recruitment, Boss Phase 2, Legacy generation, Save/load, Multiple floors, Final boss.

---

# 87. FUTURE FEATURES — DO NOT IMPLEMENT YET

Keep the architecture extensible for: Ascension system, More Colosseums, More characters, More bosses, More classes, More relics, More Legacy systems, More difficulty, Daily challenges, Other future modes.

Do NOT implement Ascension in V1.

Do NOT implement an injury system.

Do NOT implement random-event nodes.

Do NOT implement a traditional overworld map.

Do NOT implement Support conversations.

---

# 88. DESIGN IMMUTABILITY

The following are core design decisions and must not be changed without developer approval:

3 deployed units; 5 maximum roster; Permanent death; 2v3 continuation; 20-turn battle limit; Level 5 starting point; Level 30 maximum; No stat caps; Personal growth rates; Player stat choices; 4 skills total; Personal skill cannot be removed; 2 relics per character; No weapon durability; 3-win reward; Elite every 3 wins; Boss around Fight 10; Multiple Colosseum floors; 3 Recover uses per floor; Recover resets after boss; No retreat; No random events; No Support system; Legacy persistence; FE8/buildfile foundation.

---

# 89. AUTONOMY RULE

Claude may independently determine: File names, Internal class structures, Function names, Data formats, Internal algorithms, Memory-efficient implementations, Optimisations, Debugging methods, Test implementation, Code organisation, provided these do not alter gameplay.

Claude must ask before changing: Core rules, Progression, Number of units, Major reward structures, Permanent death, Legacy rules, Combat rules, Run structure, Major player-facing mechanics.

---

# 90. FIRST ACTION

DO NOT immediately start writing the game. First:

1. Inspect the entire FE8/buildfile repository.
2. Identify existing systems.
3. Identify FERepo dependencies.
4. Identify existing skill/class/item/event systems.
5. Identify what can be reused.
6. Identify what must be extended.
7. Identify GBA memory/engine limitations.
8. Create an implementation architecture.
9. Create a dependency graph.
10. Create the Phase 1 implementation plan.
11. Identify technical risks.
12. Present the plan to the developer.

Only after this analysis should implementation begin.

---

# 91. DEFINITION OF DONE

A feature is not considered complete merely because code exists. A feature is complete when:

- Code exists
- It compiles
- ROM builds
- Feature works in-game
- Relevant tests pass
- Edge cases are handled
- Save/load works if relevant
- Debug tools exist where appropriate
- Documentation is updated
- No existing systems are broken

Never mark a feature complete simply because it has been coded.

---

# 92. FINAL DEVELOPMENT PRINCIPLE

Build a game that feels like: **Fire Emblem + Roguelike + Character-building + Permanent consequences.**

The most important thing is not the number of systems.

The most important thing is that the player becomes attached to their five characters.

A character should be able to: Start as a basic unit → level → develop differently → learn skills → equip relics → promote → defeat Elites → defeat bosses → become part of the player's story → potentially die → potentially leave behind a Legacy → have that Legacy appear in a future run.

The game should make the player think: > "This is MY team."

And when a character dies: > "I can't get them back."

And when a Legacy appears in a future run: > "That's the weapon my old character left behind."

That is the emotional and mechanical identity of COLOSSEUM.

---

# END OF MASTER SPECIFICATION
