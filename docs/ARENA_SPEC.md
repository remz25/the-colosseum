# THE COLISEUM — ARENA & ENVIRONMENT SYSTEM
## Multiple Arenas, Weather, Terrain & Damage Tiles

_Saved verbatim from the developer's prompt (2026-09-27). Decisions and interpretations since:
`GAME_DESIGN.md`; implementation: `docs/ARENAS.md`._

You are continuing development of my Fire Emblem-inspired roguelike Coliseum game built on the FE8 / Sacred Stones GBA buildfile ecosystem.

I want to implement a system that allows the Coliseum to use **many different battle arenas**, environmental conditions, weather effects, terrain effects, and damaging tiles.

The purpose is to prevent the game from feeling like the player is fighting in the same arena repeatedly.

The arena system should make battles visually different and, where appropriate, tactically different.

IMPORTANT:

Do not turn every arena into an overly complicated gimmick.

Some arenas should simply provide a different battlefield and visual atmosphere.

Others can introduce meaningful environmental mechanics.

The system must remain compatible with the game's:

- 3v3 battles
- 20-turn battle limit
- Permanent death
- FE-style combat
- Terrain
- Skills
- Relics
- Enemy AI
- Elite battles
- Boss battles
- Multiple Coliseum floors

---

# 1. CORE ARENA SYSTEM

Create an Arena system that allows the game to select from multiple battlefield environments.

Conceptually:

```text
Arena
 ├── Name
 ├── Floor
 ├── Visual Theme
 ├── Map
 ├── Terrain Set
 ├── Weather
 ├── Environmental Effects
 ├── Hazard Tiles
 ├── Special Rules
 ├── Music
 └── Boss Variants
```

Use the existing FE8/buildfile architecture wherever possible.

Do not create a completely separate map engine if the existing FE8 map system can be extended.

---

# 2. ARENA TYPES

Create an initial collection of different arena concepts.

The first implementation should contain enough variety to prevent repetition.

Initial arena themes:

### 1. Grand Coliseum

Traditional stone arena.

Terrain:

- Plains
- Stone floor
- Walls
- Pillars

Weather:

None / Clear

Special effects:

None.

This is the baseline arena.

---

### 2. Forest Arena

A battlefield surrounded by dense forest.

Terrain:

- Forest
- Grass
- Small clearings
- Trees
- Hills

Potential effects:

Forest provides traditional Avoid/defensive benefits.

No special weather required.

---

### 3. Frozen Arena

A frozen battlefield.

Terrain:

- Snow
- Ice
- Frozen water
- Mountains

Weather:

Snow

Potential effects:

Snow can reduce movement for certain units.

Ice can have different movement behaviour from normal terrain.

Keep this compatible with existing FE movement rules.

---

### 4. Volcanic Arena

A volcanic battlefield.

Terrain:

- Stone
- Lava
- Ash
- Rocky ground

Weather:

Ash / volcanic activity

Hazards:

**Lava Tiles**

Units standing on active lava tiles take damage at the appropriate point in the turn.

For example:

**5 HP damage at the beginning of the unit's turn.**

Do not allow the damage to kill a unit unless explicitly designed to do so.

If necessary, leave the unit at 1 HP.

---

### 5. Ruined Cathedral

A ruined religious battlefield.

Terrain:

- Stone
- Ruins
- Pillars
- Sacred tiles
- Dark tiles

Weather:

Darkness / storm

Potential environmental effects:

Certain sacred tiles could provide defensive or healing bonuses.

Certain corrupted tiles could increase damage taken.

Keep effects simple for the first implementation.

---

### 6. Swamp Arena

A dangerous swamp.

Terrain:

- Swamp
- Water
- Mud
- Grass
- Small islands

Potential effects:

Swamp reduces movement.

Some tiles may inflict a small damage-over-time effect.

Certain units may ignore or reduce swamp penalties depending on class.

---

### 7. Royal Arena

A massive royal battlefield.

Terrain:

- Stone
- Bridges
- Walls
- Courtyard
- Defensive positions

No major environmental hazard.

The difficulty comes from:

- Chokepoints
- Long sight lines
- Defensive terrain
- Enemy formations

This arena should be particularly suitable for elite formations.

---

### 8. Abyss Arena

A supernatural battlefield.

Terrain:

- Broken stone
- Void
- Floating platforms
- Dangerous gaps

Potential effects:

Some tiles are unstable.

Certain tiles may disappear or become dangerous after several turns.

Use this arena primarily for later floors.

---

### 9. Desert Arena

A large desert battlefield.

Terrain:

- Sand
- Dunes
- Rocky ground
- Oasis

Weather:

Sandstorm

Potential effects:

Sand reduces movement.

Sandstorm can reduce Hit or visibility depending on implementation feasibility.

Do not make the effect excessively punishing.

---

### 10. Graveyard Arena

A battlefield surrounded by graves and ruined structures.

Terrain:

- Grass
- Tombstones
- Ruins
- Dark ground

Weather:

Fog

Potential effects:

Fog can reduce attack accuracy or alter enemy visibility.

If true visibility mechanics are technically difficult, simulate the effect through a controlled Hit penalty instead.

---

# 3. WEATHER SYSTEM

Create a reusable weather system.

Weather should be independent from the arena where possible.

Conceptually:

```text
Clear
Rain
Heavy Rain
Snow
Blizzard
Fog
Sandstorm
Storm
Ashfall
Darkness
```

Do NOT implement every weather type immediately.

The initial prototype should use:

- Clear
- Rain
- Snow
- Fog
- Sandstorm
- Ashfall

---

# 4. WEATHER RULES

Weather should normally provide a **small tactical effect**, not completely determine the battle.

Examples:

### Clear

No modifier.

---

### Rain

Potential effects:

- Small Hit penalty
- Fire magic interaction
- Movement effects on certain terrain

Keep the effect simple.

---

### Snow

Potential effects:

- Movement penalties
- Ice terrain becomes more important

---

### Fog

Potential effects:

- Reduced Hit
- Reduced effective attack range where practical

Do not introduce a complicated visibility system unless the FE8 engine supports it safely.

---

### Sandstorm

Potential effects:

- Reduced Hit
- Movement penalty on sand

---

### Ashfall

Potential effects:

- Small Hit penalty
- Certain magic interactions

---

# 5. WEATHER MUST BE READABLE

The player must always know the current weather.

Display something like:

```text
WEATHER: SNOW
```

or use an existing FE8-compatible visual/status system.

The player should not need to guess why their Hit values changed.

---

# 6. DAMAGE TILES

Create a reusable hazard tile system.

Possible hazard types:

### Lava

Damage:

**5 HP per turn**

---

### Poison Swamp

Damage:

**3 HP per turn**

---

### Burning Ground

Damage:

**5 HP per turn**

---

### Abyss

Damage:

**10 HP per turn**

---

### Frozen Hazard

Small damage or movement penalty.

---

### Cursed Ground

Potentially:

- Damage
- Reduced healing
- Reduced stats

Keep the initial implementation simple.

---

# 7. HAZARD TIMING

Hazard damage should occur at a predictable time.

Preferred:

**Beginning of the affected unit's turn.**

Example:

Player moves onto Lava.

Enemy phase begins.

The unit remains on Lava.

At the beginning of its next turn:

> Lava deals 5 damage.

This prevents the player from being surprised by immediate damage after moving.

Document the exact timing.

---

# 8. HAZARD DAMAGE RULES

Hazard damage should:

- Be clearly visible
- Have a consistent timing
- Use a clear animation/effect if practical
- Display the damage
- Be included in combat/debug logs where appropriate

Example:

```text
LAVA
-5 HP
```

Avoid unnecessary complexity.

---

# 9. CAN HAZARDS KILL?

For the prototype, use this rule:

Environmental hazards **cannot reduce a unit below 1 HP** unless explicitly marked as lethal.

This gives the player an opportunity to move the unit away.

Future hazards may be allowed to be lethal.

---

# 10. TERRAIN + WEATHER INTERACTIONS

Allow certain combinations.

Examples:

### Rain + Forest

No major additional effect.

### Rain + Mud

Movement penalty becomes stronger.

### Snow + Ice

Ice terrain becomes especially dangerous.

### Sandstorm + Desert

Reduced Hit.

### Ashfall + Lava

Greater visual intensity and potentially increased hazard pressure.

Do not create dozens of interactions.

Only implement interactions that are easy to understand and technically reliable.

---

# 11. ARENA-SPECIFIC SPECIAL RULES

Some arenas may have unique rules.

Examples:

### Frozen Arena

Every few turns, a small number of water tiles become frozen.

### Volcanic Arena

Some lava tiles become active/inactive.

### Abyss Arena

Certain unstable tiles change after several turns.

### Graveyard

Certain cursed tiles provide enemies with small bonuses.

### Forest

Dense forest can provide additional Avoid.

These mechanics are optional.

Do not implement every example automatically.

Choose the most technically reliable systems for the prototype.

---

# 12. ARENA VARIETY

The game should avoid loading the exact same battlefield repeatedly.

When selecting a battle:

Use the current floor's available arena pool.

For example:

### Floor 1

Grand Coliseum
Forest Arena
Royal Arena

### Floor 2

Frozen Arena
Forest Arena
Mountain Arena

### Floor 3

Ruined Cathedral
Graveyard
Dark Arena

### Floor 4

Forest Arena
Beast Domain
Swamp Arena

### Floor 5

Royal Arena
Grand Coliseum
Mountain Arena

### Floor 6

Volcanic Arena
Abyss Arena
Ruined Arena

### Floor 7

Final Coliseum
Abyss Arena
Special Boss Arenas

The exact pools can be adjusted after technical analysis.

---

# 13. BOSS ARENAS

Boss battles should sometimes have their own arena.

For example:

### Frost Queen

Frozen throne room.

### Ancient Beast

Ancient forest.

### Coliseum King

Royal arena.

### Abyssal Champion

Broken floating arena.

### Final Boss

Unique final arena.

This makes boss battles visually and mechanically memorable.

---

# 14. ELITE ARENAS

Elite battles may use special arenas.

Examples:

- Smaller battlefield
- More chokepoints
- Special terrain
- Hazard tiles
- Strong defensive positions

However, Elite battles must remain readable.

The player should understand why the battlefield is dangerous before committing to combat.

---

# 15. ENEMY AI AWARENESS

This is VERY IMPORTANT.

Enemy AI must understand environmental hazards.

An enemy should NOT blindly move onto Lava if doing so provides no tactical benefit.

AI should understand:

- Hazard tiles
- Terrain bonuses
- Movement penalties
- Weather effects
- Defensive terrain
- Dangerous positions
- Safe positions

Example:

If a player is standing on a Lava tile and an enemy can attack them from a safe tile, the enemy should prefer the safe position where appropriate.

Likewise, an enemy with immunity/resistance to a hazard can intentionally exploit that terrain.

---

# 16. UNIT / CLASS INTERACTIONS

Eventually allow certain classes or skills to interact with terrain.

Examples:

### Flying Units

Ignore some terrain movement penalties.

### Armoured Units

May suffer greater movement penalties in snow/swamp.

### Beast Units

Could gain bonuses in forests.

### Fire-based abilities

Could interact with certain terrain.

Do NOT implement all of these initially.

Build the architecture so they can be added later.

---

# 17. RELIC INTERACTIONS

The arena system should eventually support relic interactions.

Examples:

**Frostwalker Relic**

> Ignore snow movement penalties.

**Lava Heart**

> Immune to lava damage.

**Stormcaller**

> Gain +10% damage during storms.

**Swamp Walker**

> Ignore swamp movement penalties.

These are future relics.

Do not implement them unless requested.

However, the environment system should not make future relic interactions impossible.

---

# 18. SKILL INTERACTIONS

Future skills may interact with terrain and weather.

Examples:

- Weather resistance
- Terrain mastery
- Fire resistance
- Movement bonuses
- Hazard immunity
- Bonus damage on certain terrain

The system should therefore expose environmental state to the skill system where practical.

---

# 19. VISUAL PRESENTATION

Where possible, environments should have:

- Unique palettes
- Weather animations
- Terrain animations
- Environmental effects
- Appropriate music
- Boss music
- Hazard animations

However:

**Gameplay comes first.**

Do not sacrifice GBA stability or memory for visual effects.

Use FE8-compatible assets and existing FERepo resources where practical.

---

# 20. GBA PERFORMANCE

This is a GBA project.

Be extremely conscious of:

- ROM size
- RAM
- CPU usage
- Map complexity
- Animation complexity
- Number of active effects
- Weather animations
- Dynamic tile changes

Do not implement a modern particle/weather engine.

Use lightweight GBA-compatible effects.

If an effect is technically unsafe, implement a simpler equivalent.

---

# 21. SAVE/LOAD

Environmental state should be handled correctly.

If saving during a battle is supported, ensure the game preserves:

- Arena
- Weather
- Current turn
- Active hazards
- Changed terrain
- Unit positions
- Unit HP
- Environmental states

If FE8's existing save system cannot safely preserve a particular dynamic effect, document the limitation and use the safest alternative.

---

# 22. DEBUG TOOLS

Add developer tools where practical:

```text
Set Arena
Set Weather
Clear Weather
Spawn Hazard
Remove Hazard
Activate Hazard
Deactivate Hazard
Advance Weather
Deal Environmental Damage
```

Examples:

```text
SET WEATHER: SNOW
SET ARENA: VOLCANIC
SPAWN HAZARD: LAVA
```

---

# 23. TESTING

Test every arena.

Test:

- Loading
- Terrain
- Weather
- Movement
- Combat
- Enemy AI
- Hazard damage
- Turn transitions
- Save/load
- Elite battles
- Boss battles

Test combinations:

- Rain + Forest
- Snow + Ice
- Sandstorm + Desert
- Ashfall + Volcano
- Fog + Graveyard

Test edge cases:

- Unit enters hazard with 1 HP
- Unit starts turn on hazard
- Unit dies elsewhere and hazard is still active
- Hazard changes during enemy phase
- Multiple hazards
- Two units occupying/using adjacent hazardous terrain
- Save/load while hazard is active
- Boss enters hazard
- Enemy AI evaluates hazard
- Flying unit interacts with hazard
- Armoured unit interacts with difficult terrain

---

# 24. DOCUMENTATION

Update:

`GAME_DESIGN.md`

`ARCHITECTURE.md`

`TODO.md`

`CHANGELOG.md`

`TEST_STATUS.md`

`BALANCE_NOTES.md`

Create:

`docs/ARENAS.md`

Document:

- Arena list
- Arena themes
- Terrain
- Weather
- Hazards
- Damage timing
- AI interaction
- Future interactions
- Technical limitations

---

# 25. IMPLEMENTATION ORDER

### Stage 1

Inspect existing FE8 map and terrain systems.

### Stage 2

Create Arena data structure.

### Stage 3

Create multiple arena maps.

### Stage 4

Implement basic arena selection.

### Stage 5

Implement weather framework.

### Stage 6

Implement basic hazards.

### Stage 7

Connect hazards to turns.

### Stage 8

Connect environmental information to AI.

### Stage 9

Connect arenas to floor progression.

### Stage 10

Add boss/elite arena variants.

### Stage 11

Implement debug tools.

### Stage 12

Full QA and balancing.

---

# 26. IMPORTANT DESIGN PRINCIPLE

The arena should **enhance the battle rather than become the battle.**

A player should be able to win because of:

- Good unit composition
- Positioning
- Weapon choice
- Skills
- Relics
- Terrain
- Tactical decisions

—not because they happened to get a favourable weather effect.

Environmental effects should generally be:

**Small → understandable → predictable → tactically meaningful.**

---

# 27. DEFINITION OF DONE

The Arena System is complete when:

- Multiple distinct arenas exist.
- Floors can use different arena pools.
- Battles no longer repeatedly use one battlefield.
- Terrain works.
- Weather framework works.
- Initial weather types work.
- Hazard tiles work.
- Damage timing is consistent.
- Hazards are readable.
- Enemy AI understands hazards.
- Bosses can use unique arenas.
- Elite fights can use special arenas.
- Save/load works.
- Debug tools work.
- Tests pass.
- ROM builds successfully.
- The game has been tested in an emulator.
- Documentation is updated.

Do not consider the system complete just because different maps load.

The arenas must actually create meaningful tactical variety while remaining stable on GBA hardware.
