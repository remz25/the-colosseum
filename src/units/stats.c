/* Stats without caps (spec 21) and level-up stat choices (spec 23).
 *
 * No stat caps: class caps are 127 in the tables (ClassTable.csv, MagClassEditor.csv) and the
 * two vanilla cap functions are replaced (Units.event) by versions that only keep stats inside
 * the s8 range (COL_STAT_CEILING) - vanilla also caps Luck at a hardcoded 30.
 *
 * Level-up choice: after the growth rolls, the player picks one of 3 random +1 stat bonuses
 * (duplicates allowed). gColRun.choiceLevel[slot] records the level up to which a roster unit's
 * choices were made; stat_choice.c shows the menu for every level still owed. */
#include "coliseum.h"
#include "bmunit.h"
#include "bmbattle.h"
#include "rng.h"

/* ---- caps ---- */

static void ClampGain(s8 *change, int current)
{
    if (current + *change > COL_STAT_CEILING)
        *change = (s8)(COL_STAT_CEILING - current);
}

/* Replaces vanilla CheckBattleUnitStatCaps (0x0802BF24): level-up gains may not leave the s8
 * range. Con and Mag (the Str/Mag split keeps Mag's gain in changeCon) are untouched, as in
 * vanilla; Mag's cap is MagClassTable's (127). */
void Col_CheckBattleUnitStatCaps(struct Unit *unit, struct BattleUnit *bu)
{
    ClampGain(&bu->changeHP, unit->maxHP);
    ClampGain(&bu->changePow, unit->pow);
    ClampGain(&bu->changeSkl, unit->skl);
    ClampGain(&bu->changeSpd, unit->spd);
    ClampGain(&bu->changeDef, unit->def);
    ClampGain(&bu->changeRes, unit->res);
    ClampGain(&bu->changeLck, unit->lck);
}

/* Replaces vanilla UnitCheckStatCaps (0x080181C8): no stat caps; Con and Mov bonuses keep their
 * vanilla limits (they are not level-up stats). */
void Col_UnitCheckStatCaps(struct Unit *unit)
{
    if (unit->conBonus > (UNIT_CON_MAX(unit) - UNIT_CON_BASE(unit)))
        unit->conBonus = (UNIT_CON_MAX(unit) - UNIT_CON_BASE(unit));
    if (unit->movBonus > (UNIT_MOV_MAX(unit) - UNIT_MOV_BASE(unit)))
        unit->movBonus = (UNIT_MOV_MAX(unit) - UNIT_MOV_BASE(unit));
}

/* ---- stats ---- */

static s8 *StatField(struct Unit *unit, int stat)
{
    switch (stat) {
    case COL_STAT_HP:  return &unit->maxHP;
    case COL_STAT_STR: return &unit->pow;
    case COL_STAT_MAG: return (s8 *)&unit->_u3A;     /* Str/Mag split: magic byte */
    case COL_STAT_SKL: return &unit->skl;
    case COL_STAT_SPD: return &unit->spd;
    case COL_STAT_LCK: return &unit->lck;
    case COL_STAT_DEF: return &unit->def;
    case COL_STAT_RES: return &unit->res;
    }
    return NULL;
}

int Col_GetStat(struct Unit *unit, int stat)
{
    s8 *f = StatField(unit, stat);
    return f ? *f : 0;
}

int Col_AddStat(struct Unit *unit, int stat, int n)
{
    s8 *f = StatField(unit, stat);
    int v;

    if (!f || n <= 0)
        return 0;
    v = *f + n;
    if (v > COL_STAT_CEILING)
        v = COL_STAT_CEILING;
    n = v - *f;
    *f = (s8)v;
    if (stat == COL_STAT_HP)
        unit->curHP = (s8)(unit->curHP + n > COL_STAT_CEILING ? COL_STAT_CEILING : unit->curHP + n);
    return n;
}

const char *Col_StatName(int stat)
{
    static const char *const kNames[COL_STAT_COUNT] = {
        "HP", "Str", "Mag", "Skl", "Spd", "Lck", "Def", "Res",
    };
    return (stat >= 0 && stat < COL_STAT_COUNT) ? kNames[stat] : "";
}

void Col_RollStatChoices(u8 out[COL_STAT_CHOICES])
{
    int i;
    for (i = 0; i < COL_STAT_CHOICES; i++)
        out[i] = (u8)NextRN_N(COL_STAT_COUNT);
}

/* ---- owed choices ---- */

int Col_FindPendingChoice(void)
{
    int slot;
    for (slot = 0; slot < COL_MAX_ROSTER; slot++) {
        struct Unit *unit = Col_RosterUnit(slot);
        if (!unit || (unit->state & US_DEAD) || !gColRun.choiceLevel[slot])
            continue;
        if (unit->level > gColRun.choiceLevel[slot])
            return slot;
    }
    return -1;
}

void Col_TakeStatChoice(int slot, int stat)
{
    struct Unit *unit = Col_RosterUnit(slot);
    if (!unit)
        return;
    Col_AddStat(unit, stat, 1);
    gColRun.choiceLevel[slot]++;
}
