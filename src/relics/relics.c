/* Relics (spec 33-36, Phase 9). Design and rules: docs/RELICS.md.
 *
 * Storage (run state, saved): gColRun.relics[pool][2] holds the relics each pool character
 * wears, gColRun.relicBag the unequipped ones. Relics belong to the run, not to a unit: they
 * move between characters and the bag outside battle, and go back to the bag when their wearer
 * leaves the run (death or replacement). Only player units (pool characters) wear relics.
 *
 * Effects are generic modifiers (coliseum.h, table in relics_data.c), applied by five hooks:
 *   stat getters      Col_Relic*Getter        flat stat changes, then percentages (Skill System
 *                                             MSG lists, EngineHacks/Necessary/StatGetters)
 *   pre-battle loop   Col_RelicPreBattle      Hit / Avoid / Crit, adjacent-ally Def auras
 *   battle proc loop  Col_RelicDamageProc     damage dealt / magic damage / damage taken
 *                     Col_RelicHpCostProc     HP lost per attack (never below 1 HP)
 *   victory           Col_RelicApplyGold      battle gold (battle.c Col_OnBattleWon)
 *
 * Percentages: all percentages of one kind worn by a unit are added together (two +10% = +20%)
 * and applied once, after every flat change, to the current value; the result is rounded to
 * the nearest whole number (halves away from zero). Stat percentages only apply to positive
 * stats; stats still never go below 0 (the Skill System's prMinZero runs after the relics).
 * Damage: dealt percentages (all + magic) apply first, then the target's taken percentage.
 * Conditions (e.g. below 50% HP) are checked every time a stat is read: on the stat screen,
 * the forecast, and at the start of every combat, so they switch on and off as HP changes. */
#include "coliseum.h"
#include "bmunit.h"
#include "bmitem.h"
#include "bmmap.h"
#include "bmbattle.h"
#include "bmphase.h"
#include "rng.h"

const struct ColRelicDef *Col_RelicDef(int relic)
{
    if (relic <= 0 || relic > Col_RelicCount())
        return NULL;
    return &gColRelics[relic];
}

const char *Col_RelicRarityName(int rarity)
{
    static const char *const kNames[] = { "", "Common", "Uncommon", "Rare", "Epic", "Legendary", "Mythic" };
    return rarity >= 1 && rarity <= COL_RELIC_RARITY_COUNT ? kNames[rarity] : "";
}

int Col_RelicPercent(int value, int percent)
{
    int r = value * percent;
    return value + (r >= 0 ? r + 50 : r - 50) / 100;
}

/* ---- who wears what ---- */

int Col_PoolIndexOfUnit(struct Unit *unit)
{
    int i, charId;

    if (!unit || !unit->pCharacterData || UNIT_FACTION(unit) != FACTION_BLUE || !Col_RunIsValid())
        return -1;
    charId = unit->pCharacterData->number;
    for (i = 0; i < COL_POOL_SIZE; i++)
        if (gColPool[i].charId == charId)
            return i;
    return -1;
}

int Col_UnitRelic(struct Unit *unit, int slot)
{
    int pool = Col_PoolIndexOfUnit(unit);
    if (pool < 0 || slot < 0 || slot >= COL_RELIC_SLOTS)
        return 0;
    return gColRun.relics[pool][slot];
}

static int CondMet(struct Unit *unit, int cond)
{
    switch (cond) {
    case COL_RC_ALWAYS:
        return 1;
    case COL_RC_BELOW_HALF_HP:                  /* in combat, unit is the battle copy (current HP) */
        return unit->curHP * 2 < unit->maxHP;
    }
    return 0;
}

static int PoolModTotal(int pool, struct Unit *unit, int kind)
{
    int slot, m, total = 0;

    for (slot = 0; slot < COL_RELIC_SLOTS; slot++) {
        const struct ColRelicDef *def = Col_RelicDef(gColRun.relics[pool][slot]);
        if (!def)
            continue;
        for (m = 0; m < COL_RELIC_MODS && def->mods[m].kind != COL_RM_END; m++)
            if (def->mods[m].kind == kind && (!unit || CondMet(unit, def->mods[m].cond)))
                total += def->mods[m].amount;
    }
    return total;
}

int Col_RelicModTotal(struct Unit *unit, int kind)
{
    int pool = Col_PoolIndexOfUnit(unit);
    return pool < 0 ? 0 : PoolModTotal(pool, unit, kind);
}

/* ---- stat getters (MSG modifier lists: value in r0, unit in r1, new value returned) ---- */

static int StatHook(int value, struct Unit *unit, int flatKind, int pctKind)
{
    int pool = Col_PoolIndexOfUnit(unit), pct;

    if (pool < 0)
        return value;
    value += PoolModTotal(pool, unit, flatKind);
    pct = pctKind ? PoolModTotal(pool, unit, pctKind) : 0;
    if (pct && value > 0)
        value = Col_RelicPercent(value, pct);
    return value;
}

int Col_RelicPowGetter(int v, struct Unit *u) { return StatHook(v, u, COL_RM_STR, COL_RM_STR_PCT); }
int Col_RelicMagGetter(int v, struct Unit *u) { return StatHook(v, u, COL_RM_MAG, COL_RM_MAG_PCT); }
int Col_RelicSklGetter(int v, struct Unit *u) { return StatHook(v, u, COL_RM_SKL, COL_RM_SKL_PCT); }
int Col_RelicSpdGetter(int v, struct Unit *u) { return StatHook(v, u, COL_RM_SPD, COL_RM_SPD_PCT); }
int Col_RelicLckGetter(int v, struct Unit *u) { return StatHook(v, u, COL_RM_LCK, COL_RM_LCK_PCT); }
int Col_RelicDefGetter(int v, struct Unit *u) { return StatHook(v, u, COL_RM_DEF, COL_RM_DEF_PCT); }
int Col_RelicResGetter(int v, struct Unit *u) { return StatHook(v, u, COL_RM_RES, COL_RM_RES_PCT); }
int Col_RelicMovGetter(int v, struct Unit *u) { return StatHook(v, u, COL_RM_MOV, 0); }

/* ---- combat ---- */

static int IsMagicAttack(struct BattleUnit *bu)
{
    /* the Str/Mag split's rule: weapons with the "magic" attribute use Mag against Res */
    return bu->weapon && (GetItemAttributes(bu->weapon) & IA_MAGIC);
}

/* Def from allies next to `bu` wearing an adjacent-ally relic (never the wearer itself). */
static int AdjacentAllyDef(struct BattleUnit *bu)
{
    static const s8 kDx[] = { 0, 0, -1, 1 }, kDy[] = { -1, 1, 0, 0 };
    int i, total = 0;

    if (!gBmMapUnit)
        return 0;
    for (i = 0; i < 4; i++) {
        int x = bu->unit.xPos + kDx[i], y = bu->unit.yPos + kDy[i], id;
        struct Unit *ally;
        if (x < 0 || y < 0 || x >= gBmMapSize.x || y >= gBmMapSize.y)
            continue;
        id = gBmMapUnit[y][x];
        if (!id || id == (u8)bu->unit.index || !AreUnitsAllied(id, (u8)bu->unit.index))
            continue;
        ally = GetUnit(id);
        if (ally && ally->pCharacterData && !(ally->state & (US_DEAD | US_NOT_DEPLOYED)))
            total += Col_RelicModTotal(ally, COL_RM_ADJ_ALLY_DEF);
    }
    return total;
}

/* Pre-battle calc loop (PreBattleCalcLoop.event): runs for (actor, target) and (target, actor);
 * each call changes only `a`, whose battle stats are complete by now. */
void Col_RelicPreBattle(struct BattleUnit *a, struct BattleUnit *b)
{
    int def;

    if (Col_PoolIndexOfUnit(&a->unit) >= 0) {
        a->battleHitRate += Col_RelicModTotal(&a->unit, COL_RM_HIT);
        a->battleAvoidRate += Col_RelicModTotal(&a->unit, COL_RM_AVOID);
        a->battleCritRate += Col_RelicModTotal(&a->unit, COL_RM_CRIT);
        if (a->battleHitRate < 0) a->battleHitRate = 0;
        if (a->battleAvoidRate < 0) a->battleAvoidRate = 0;
        if (a->battleCritRate < 0) a->battleCritRate = 0;
    }
    def = AdjacentAllyDef(a);                   /* Defense: only against physical attacks */
    if (def && !IsMagicAttack(b))
        a->battleDefense += def;
}

/* Battle proc loop (BattleProcCalcLoop.event), once per strike: `att` strikes `def`.
 * `hit` is the Skill System's 8-byte hit entry, `stats` its battle data (damage at +4). */
struct ColProcStats { u16 config; u16 range; s16 damage; };

void Col_RelicDamageProc(struct BattleUnit *att, struct BattleUnit *def, u8 *hit, struct ColProcStats *stats)
{
    int dmg = stats->damage, dealt, taken;

    if (dmg <= 0)
        return;
    dealt = Col_RelicModTotal(&att->unit, COL_RM_DMG_DEALT_PCT);
    if (IsMagicAttack(att))
        dealt += Col_RelicModTotal(&att->unit, COL_RM_MAGIC_DMG_PCT);
    taken = Col_RelicModTotal(&def->unit, COL_RM_DMG_TAKEN_PCT);
    if (dealt)
        dmg = Col_RelicPercent(dmg, dealt);
    if (taken)
        dmg = Col_RelicPercent(dmg, taken);
    stats->damage = (s16)(dmg < 0 ? 0 : dmg);
}

#define HIT_ATTR_HPSTEAL   0x100            /* the attacker's HP changes (shown by the animation) */
#define HIT_ATTACKER_HP    5                /* s8: the attacker's HP change this strike */

/* Runs just before the Skill System's Proc_Finish, which applies the attacker's HP change and
 * keeps the attacker at 1 HP or more, so a relic HP cost can never kill its wearer. */
void Col_RelicHpCostProc(struct BattleUnit *att, struct BattleUnit *def, u8 *hit, struct ColProcStats *stats)
{
    int cost = Col_RelicModTotal(&att->unit, COL_RM_HP_PER_ATTACK);

    if (cost <= 0)
        return;
    *(u32 *)hit |= HIT_ATTR_HPSTEAL;
    hit[HIT_ATTACKER_HP] = (u8)((s8)hit[HIT_ATTACKER_HP] - cost);
}

/* ---- gold (spec 36: reward modifiers) ---- */

/* Battle gold after a victory, with the gold relics worn by living roster members (reserves
 * included). Only Col_OnBattleWon calls this: shop refunds or other gold sources never do. */
int Col_RelicApplyGold(int gold)
{
    int slot, pct = 0;

    for (slot = 0; slot < COL_MAX_ROSTER; slot++) {
        int pool = gColRun.roster[slot];
        if (pool < COL_POOL_SIZE)
            pct += PoolModTotal(pool, Col_RosterUnit(slot), COL_RM_GOLD_PCT);
    }
    return pct ? Col_RelicPercent(gold, pct) : gold;
}

/* ---- bag, equip, transfer ---- */

int Col_RelicBagCount(void)
{
    int i, n = 0;
    for (i = 0; i < COL_RELIC_BAG; i++)
        if (gColRun.relicBag[i])
            n++;
    return n;
}

int Col_RelicBagAdd(int relic)
{
    int i;
    if (!Col_RelicDef(relic))
        return 0;
    for (i = 0; i < COL_RELIC_BAG; i++)
        if (!gColRun.relicBag[i]) {
            gColRun.relicBag[i] = (u8)relic;
            return 1;
        }
    return 0;
}

static int ValidSlot(int pool, int slot)
{
    return pool >= 0 && pool < COL_POOL_SIZE && slot >= 0 && slot < COL_RELIC_SLOTS;
}

/* Wear bag relic `bagIndex` in `slot`; a relic already there takes its place in the bag. */
int Col_RelicEquipFromBag(int pool, int slot, int bagIndex)
{
    u8 old;

    if (!ValidSlot(pool, slot) || bagIndex < 0 || bagIndex >= COL_RELIC_BAG || !gColRun.relicBag[bagIndex])
        return 0;
    old = gColRun.relics[pool][slot];
    gColRun.relics[pool][slot] = gColRun.relicBag[bagIndex];
    gColRun.relicBag[bagIndex] = old;
    return 1;
}

/* Transfer (spec 33): a worn relic to another character's slot; if that slot holds a relic,
 * the two swap. */
int Col_RelicMove(int fromPool, int fromSlot, int toPool, int toSlot)
{
    u8 t;

    if (!ValidSlot(fromPool, fromSlot) || !ValidSlot(toPool, toSlot) || !gColRun.relics[fromPool][fromSlot]
        || (fromPool == toPool && fromSlot == toSlot))
        return 0;
    t = gColRun.relics[toPool][toSlot];
    gColRun.relics[toPool][toSlot] = gColRun.relics[fromPool][fromSlot];
    gColRun.relics[fromPool][fromSlot] = t;
    return 1;
}

int Col_RelicUnequip(int pool, int slot)
{
    if (!ValidSlot(pool, slot) || !gColRun.relics[pool][slot] || !Col_RelicBagAdd(gColRun.relics[pool][slot]))
        return 0;
    gColRun.relics[pool][slot] = 0;
    return 1;
}

/* The character left the run (died or was replaced): its relics stay in the run, in the bag.
 * (Lost only if the bag is full.) */
void Col_RelicsReturnToBag(int pool)
{
    int slot;
    if (pool < 0 || pool >= COL_POOL_SIZE)
        return;
    for (slot = 0; slot < COL_RELIC_SLOTS; slot++) {
        if (gColRun.relics[pool][slot])
            Col_RelicBagAdd(gColRun.relics[pool][slot]);
        gColRun.relics[pool][slot] = 0;
    }
}

/* ---- offers ---- */

/* Rarity weights for random relics (shop). Rarities without relics are skipped. Phase 16 lever. */
static const u8 kRarityWeight[COL_RELIC_RARITY_COUNT + 1] = { 0, 45, 30, 17, 8, 4, 1 };

int Col_RelicRoll(void)
{
    int counts[COL_RELIC_RARITY_COUNT + 1] = { 0 };
    int r, id, total = 0, pick;

    for (id = 1; id <= Col_RelicCount(); id++)
        counts[gColRelics[id].rarity]++;
    for (r = 1; r <= COL_RELIC_RARITY_COUNT; r++)
        if (counts[r])
            total += kRarityWeight[r];
    if (!total)
        return 0;
    pick = NextRN_N(total);
    for (r = 1; r <= COL_RELIC_RARITY_COUNT; r++) {
        if (!counts[r])
            continue;
        if (pick < kRarityWeight[r])
            break;
        pick -= kRarityWeight[r];
    }
    pick = NextRN_N(counts[r]);
    for (id = 1; id <= Col_RelicCount(); id++)
        if (gColRelics[id].rarity == r && pick-- == 0)
            return id;
    return 0;
}

/* ---- text ---- */

static char *AppendStr(char *out, const char *s)
{
    while (*s)
        *out++ = *s++;
    return out;
}

static char *AppendNumber(char *out, int n)
{
    char digits[8];
    int k = 0;

    do {
        digits[k++] = (char)('0' + n % 10);
        n /= 10;
    } while (n);
    while (k)
        *out++ = digits[--k];
    return out;
}

static char *AppendSigned(char *out, int n)
{
    *out++ = n < 0 ? '-' : '+';
    return AppendNumber(out, n < 0 ? -n : n);
}

/* One effect line, e.g. "+5 Def", "+20% Str", "-20% damage taken". Returns 1 if the effect is
 * good for the wearer (shown in green), 0 for a drawback. */
int Col_RelicModText(const struct ColRelicMod *mod, char *out)
{
    static const char *const kStat[] = { "Str", "Mag", "Skl", "Spd", "Lck", "Def", "Res", "Mov" };
    int k = mod->kind, n = mod->amount, good = n > 0;

    if (k >= COL_RM_STR && k <= COL_RM_MOV) {
        out = AppendSigned(out, n);
        out = AppendStr(AppendStr(out, " "), kStat[k - COL_RM_STR]);
    } else if (k >= COL_RM_STR_PCT && k <= COL_RM_RES_PCT) {
        out = AppendStr(AppendSigned(out, n), "% ");
        out = AppendStr(out, kStat[k - COL_RM_STR_PCT]);
    } else switch (k) {
    case COL_RM_HIT:    out = AppendStr(AppendSigned(out, n), " Hit"); break;
    case COL_RM_AVOID:  out = AppendStr(AppendSigned(out, n), " Avoid"); break;
    case COL_RM_CRIT:   out = AppendStr(AppendSigned(out, n), " Crit"); break;
    case COL_RM_MAGIC_DMG_PCT: out = AppendStr(AppendSigned(out, n), "% magic damage"); break;
    case COL_RM_DMG_DEALT_PCT: out = AppendStr(AppendSigned(out, n), "% damage dealt"); break;
    case COL_RM_DMG_TAKEN_PCT: out = AppendStr(AppendSigned(out, n), "% damage taken"); good = n < 0; break;
    case COL_RM_GOLD_PCT: out = AppendStr(AppendSigned(out, n), "% battle gold"); break;
    case COL_RM_ADJ_ALLY_DEF:
        out = AppendStr(out, "Adjacent allies ");
        out = AppendStr(AppendSigned(out, n), " Def");
        break;
    case COL_RM_HP_PER_ATTACK:
        out = AppendStr(out, "Lose ");
        out = AppendNumber(out, n);
        out = AppendStr(out, " HP per attack");
        good = 0;
        break;
    }
    *out = 0;
    return good;
}
