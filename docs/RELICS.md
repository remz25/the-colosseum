# COLOSSEUM: Relic system (Phase 9, prototype pool v1)

Spec §33-36 plus the developer's "Relic System Implementation Prompt, Prototype Relic Pool v1"
(2026-09-26). Future relic ideas are kept separately in [`RELICS_FUTURE.md`](RELICS_FUTURE.md).

## Rules

- Every character can wear **2 relics** (spec §33, core rule §88).
- Relics belong to the **run**, not to a character. Outside battle (Prepare > Relics) the player can
  equip, unequip and transfer them. Unequipped relics go to the **relic bag** (16 places), and
  nothing is lost when a relic is unequipped.
- **Transfer:** when choosing a relic for a slot, the list shows the bag *and* relics worn by other
  roster members (with the wearer's name). Taking one moves it. If the slot already held a
  relic, the two swap.
- **Duplicates** are allowed (spec §33): Iron Heart + Iron Heart gives +10 Def.
- When a character **leaves the run** (death or being replaced), their relics go back to the bag.
  A relic is only lost if the bag is full at that moment.
- A **new run** starts with no relics.
- Only player characters wear relics. Enemies never have any (Elite/Boss relics would be a later
  decision).
- Before equipping (or buying), the **info screen** shows the name, rarity and every effect.
  Positive effects are green, drawbacks gold, and conditions appear as a header line
  ("Below 50% HP:"). Flat and percentage values are both written out ("+5 Def", "+20% Str").
- **Getting relics (prototype):** the shop's Relic category (one per stock, sometimes a second as
  an extra). Relics are rarity-weighted, and bought relics go into the bag. Elite/Boss relic
  rewards and relic reward choices come with Phases 10-12.

## Rarities

Common, Uncommon, Rare, Epic, Legendary, Mythic (spec §34). All six are supported by the data
format, the text and the shop. The prototype pool has 6 Common, 5 Uncommon, 3 Rare and 1 Epic.

| Rarity | Shop weight | Shop base price |
|---|---|---|
| Common | 45 | 500 |
| Uncommon | 30 | 900 |
| Rare | 17 | 1400 |
| Epic | 8 | 2200 |
| Legendary | 4 (no relics yet) | 3200 |
| Mythic | 1 (no relics yet) | 4500 |

Rarities with no relics are skipped when rolling. Weights and prices are Phase 16 balance levers
(`src/relics/relics.c` kRarityWeight, `src/shop/shop.c` kRelicPrice). Shop prices also rise
+10% per purchase, like everything else in the shop.

## The prototype pool (`src/relics/relics_data.c`)

| ID | Relic | Rarity | Effects |
|---|---|---|---|
| 1 | Iron Heart | Common | +5 Def, -3 Spd |
| 2 | Warrior's Band | Common | +5 Str, -3 Res |
| 3 | Swift Feather | Common | +5 Spd, -3 Def |
| 4 | Scholar's Lens | Common | +5 Mag, -3 Str |
| 5 | Eagle Eye | Common | +10 Hit, -5 Avoid |
| 6 | Sturdy Boots | Common | +1 Mov, -5 Def |
| 7 | Bloodied Band | Uncommon | below 50% HP: +5 Str, +5 Crit |
| 8 | Guardian's Crest | Uncommon | adjacent allies +3 Def (not the wearer), -2 Spd |
| 9 | Mage's Ring | Uncommon | +15% magic damage, -5 Def |
| 10 | Golden Thread | Uncommon | +25% battle gold, -5 Lck |
| 11 | Berserker's Fang | Uncommon | +5 Str, +5 Crit, -5 Def |
| 12 | Blood Pact | Rare | +20% Str, +10% Spd, lose 2 HP per attack (never below 1 HP) |
| 13 | Wind Soul | Rare | +20% Spd, -15% Def |
| 14 | Arcane Blood | Rare | +20% Mag, -15% Def, +10% magic damage |
| 15 | Fortress Heart | Epic | -20% damage taken, -5 Spd, -1 Mov |

IDs are saved in the run state. Never reorder or remove entries; always append new relics at
the end.

## How effects work

A relic is a name, a rarity and up to 5 **modifiers**. Each modifier is a *kind*, an *amount* and
an optional *condition* (`colosseum.h`: `enum ColRelicModKind`, `enum ColRelicCond`). No relic is
hard-coded anywhere in the battle code: each hook asks "what is the total of kind X on this
unit?" (`Col_RelicModTotal`).

| Group | Kinds | Applied in |
|---|---|---|
| Stat modifiers | Str Mag Skl Spd Lck Def Res Mov (flat); Str..Res % | The Skill System's stat getters (`EngineHacks/Necessary/StatGetters/*.event`, `Col_Relic*Getter`). The same numbers are used by the stat screen, the forecast, the AI, movement ranges and combat. |
| Battle rates | Hit, Avoid, Crit | Pre-battle calc loop (`Col_RelicPreBattle`) |
| Damage modifiers | magic damage %, damage dealt %, damage taken % | Battle proc loop, once per strike (`Col_RelicDamageProc`) |
| Team effects | adjacent allies +n Def | Pre-battle calc loop (`Col_RelicPreBattle`) |
| Costs | HP per attack | Battle proc loop, just before the Skill System's `Proc_Finish` (`Col_RelicHpCostProc`) |
| Reward modifiers | battle gold % | `Col_OnBattleWon` (`src/core/battle.c`) through `Col_RelicApplyGold` |
| Conditions | always, below 50% HP | Checked on every read (`CondMet`) |

### Percentage rules (one system for every relic)

- **Additive within a kind.** All percentages of the same kind on one unit are added together:
  Blood Pact (+10% Spd) + Wind Soul (+20% Spd) = +30% Spd.
- **Applied after flat changes.** A stat percentage is applied once, at the end of the stat's
  getter chain, to the current value. That value includes the unit's stat, weapon and skill
  bonuses, debuffs and the relics' own flat changes. For example, Swift Feather + Wind Soul on
  Spd 10 gives (10 + 5) × 1.2 = 18.
- **Rounded to the nearest whole number**, halves away from zero: 9 × 1.1 = 9.9 → 10;
  7 × 0.85 = 5.95 → 6; 5 × 1.1 = 5.5 → 6. The formula is `Col_RelicPercent`.
- Stat percentages only apply to positive stats. Stats still never go below 0 (the Skill
  System's `prMinZero` runs after the relics).
- **Damage:** first the attacker's "damage dealt" percentages (all damage, plus magic damage when
  the weapon is magic), then the defender's "damage taken" percentage. The two are applied one
  after the other, each rounded. This happens after critical hits and before skills that set
  damage outright (Bane, Lethality), and before Pavise/Aegis-style reductions.
- **Magic** means the weapon has the item's "magic" attribute. That is the same rule the Str/Mag
  split uses to choose Mag over Str, so tomes count and physical weapons never do.

### Notes per effect

- **Bloodied Band:** "below 50% HP" means current HP × 2 < max HP (exactly half does not count).
  It is checked whenever a stat is read (stat screen, forecast) and at the start of every combat,
  so it switches on and off as HP changes during the fight. Within one exchange, the numbers are
  fixed at its start, as with every FE8 battle stat (the engine's Wrath works the same way).
- **Guardian's Crest:** allies standing directly next to the wearer (up, down, left, right) get
  +3 Def in their combats, against physical attacks only (Def is not used against magic). The
  wearer never gets its own bonus. It updates with positions because it is checked when each
  combat starts. It shows in the battle forecast, not on the stat screen (it depends on
  position). Two crests next to the same ally stack.
- **Mage's Ring / Arcane Blood:** the damage bonus only applies to magic attacks. Arcane Blood's
  +20% Mag and its +10% magic damage are separate effects.
- **Golden Thread:** applied in exactly one place: the gold paid for a battle victory
  (`Col_OnBattleWon`: `Col_AddGold(Col_RelicApplyGold(Col_BattleGold(encounter)))`). It counts
  every Golden Thread worn by a living roster member, reserves included. Two give +50%. Shop
  refunds, 3-win gold rewards (Phase 10) or any later currency never pass through it.
- **Blood Pact:** each attack the wearer makes costs 2 HP, including counterattacks, misses and
  follow-ups. It uses the Skill System's attacker-HP-change field (the same mechanism as Gaiden
  magic's HP cost), so the battle animation shows it. `Proc_Finish` keeps the attacker at 1 HP
  or more, so **Blood Pact never kills its wearer** (verified in a real battle).
- **Fortress Heart:** only damage the wearer receives from attacks. Healing, allies and other HP
  changes are not affected.

## Adding a relic

1. Append a line to `gColRelics` in `src/relics/relics_data.c`, using existing kinds:
   `{ "Name", RARE, {0}, { M(STR_PCT, 15), MC(CRIT, 10, BELOW_HALF_HP) } }`.
2. That's all for existing kinds: the shop, menus, texts and hooks pick it up. `Test_RelicPool`
   checks the rarity counts, so update it.

A **new kind of effect** (for example, damage dealt below 50% HP, or "choose 2 rewards"):

1. Add a value to `enum ColRelicModKind` (before `COL_RM_KIND_COUNT`).
2. Apply it in the one hook where it belongs (or a new hook for a new mechanic: Mythic
   rule-changing effects will get their own hook points, found the same way as these).
3. Add its text in `Col_RelicModText`, and a test in `src/tests/test_relics.c`.

A **new condition**: add it to `enum ColRelicCond`, handle it in `CondMet` (`relics.c`) and in
the info screen's header text (`relic_ui.c`).

## Storage and saves

Run state v5 (`colosseum.h`): `relics[15][2]` (per pool character, offset 0x54) and
`relicBag[16]` (offset 0x72). Both are saved with the game save and the suspend (the run-state
chunk). v4 saves (before relics) are upgraded on load, because those bytes were reserved and
always zero.

## Tests

14 on-target tests in `src/tests/test_relics.c` (pool, texts, equip/transfer/bag, leaving the
run, save + v4 upgrade, flat stats, percentages, battle rates, Bloodied Band, Guardian's Crest,
damage relics, Blood Pact, gold, shop). What was checked in game: `TEST_STATUS.md`.
