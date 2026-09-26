# COLISEUM: Future relic concepts (not implemented)

The prototype pool (15 relics, [`RELICS.md`](RELICS.md)) is deliberately small. This file keeps
the relic ideas for later so they aren't lost. **Nothing here is in the game yet.**

> **To do:** the developer mentioned more relic concepts discussed outside this repository.
> Paste them here verbatim when available. This list only holds what the spec and the prototype
> prompt say.

## Design direction (from the prototype prompt)

Rarity should not simply mean "bigger numbers". It should grow with:

- complexity
- build-defining potential
- unusual mechanics
- conditional effects
- rule-changing effects

| Rarity | Intended feel |
|---|---|
| Common | Straightforward flat stat trade-offs |
| Uncommon | First conditions, auras and small percentages |
| Rare | Percentage-based scaling appears more often |
| Epic | Strong, build-shaping effects with real drawbacks |
| Legendary | (none yet) build-defining |
| Mythic | (none yet) special combat behaviour; can change normal combat rules |

## Effect categories still to use (spec §35-36)

Relics can provide: positive effects, negative effects, conditional effects, build-defining
effects, **team synergy**, **skill modifications**, stat changes, gold effects, **healing
effects**, **promotion effects**, **reward modifications**. Cursed/negative effects must always
be visible before equipping (already true for every relic).

- **Reward modification** (spec §36): for example "Choose 2 rewards instead of 1" at the 3-win
  reward. Needs Phase 10's reward menu. The effect must be clearly communicated.
- **Skill modifications:** for example raising a skill's activation rate, or granting a skill while
  worn.
- **Healing effects:** for example healing at the start of each turn, or stronger healing received
  from staves.
- **Promotion effects:** for example cheaper promotions, or promotion without an item. Needs
  Phase 10's promotion design.
- **Team synergy:** more auras like Guardian's Crest (Hit, Avoid, damage), and effects for
  wearing relics of the same family.

## Percentage modifiers already supported by the engine

These percentage kinds already work for any future relic without code changes: +/-n% Str, Mag,
Skl, Spd, Lck, Def, Res; +/-n% damage dealt; +/-n% magic damage; +/-n% damage taken; +/-n%
battle gold. The prompt's examples (+10% / +15% Str, -20% Spd, +25% magic damage, -15% Def,
+30% damage dealt, -20% damage taken, +25% gold) can each be written as one line in
`relics_data.c`.

## Mythic relics: what the engine will need

Mythic relics with "special combat behaviours" (rule changes) will need new modifier kinds and,
for some, new hook points. Examples of the kind of hooks available in the Skill System:

- pre-battle loop: combat stats before a fight
- battle proc loop: every strike (damage, extra effects, HP changes)
- post-battle loop: after a combat
- the doubling check (`CanUnitDoubleCalcLoop`): for example "always double" or "never be doubled"
- the weapon-triangle loop (`WTACalcLoop`): for example "reverse the triangle"
- the range loop (`RangeCalcLoop`): for example "+1 range"
- the start-of-turn loop (`TurnLoop`): for example "heal 10% each turn"

The data format already reserves the Mythic rarity (weight 1, price 4500 in the shop), and
supports up to 5 modifiers with conditions per relic.
