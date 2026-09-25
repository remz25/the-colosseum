/* On-target unit tests for Phase 4 (spec 10, 21-24): the pool, level 5 start, level cap 30,
 * no stat caps, level-up stat choices. Map tests (they create units): run by
 * tests/run_tests.py on the battle map through ColTest_MapRun (test_combat.c). */
#include "coliseum.h"
#include "bmunit.h"
#include "bmbattle.h"
#include "bmitem.h"

#define CHECK(cond) do { if (!(cond)) return __LINE__; } while (0)

extern const u8 PersonalSkillTable[];          /* Skills/PersonalSkillEditor.csv, by character */

/* 15 distinct characters, each with its own personal skill, based at level 5 (spec 10, 21). */
int Test_Pool(struct Unit *a, struct Unit *t)
{
    int i, j;
    for (i = 0; i < COL_POOL_SIZE; i++) {
        const struct CharacterData *c = GetCharacterData(gColPool[i].charId);
        CHECK(c && c->number == gColPool[i].charId);
        CHECK(c->baseLevel == COL_START_LEVEL);
        CHECK(PersonalSkillTable[c->number] != 0);
        CHECK(gColPool[i].items[0] != 0);
        for (j = 0; j < i; j++) {
            CHECK(gColPool[j].charId != gColPool[i].charId);
            CHECK(PersonalSkillTable[gColPool[j].charId] != PersonalSkillTable[c->number]);
        }
    }
    return 0;
}

/* Every pool character loads at level 5 with EXP 0, its level-5 bases, and a usable weapon. */
int Test_LoadPoolUnits(struct Unit *a, struct Unit *t)
{
    int i, result = 0;
    for (i = 0; i < COL_POOL_SIZE && !result; i++) {
        struct Unit *u = Col_LoadPoolUnit(i);
        if (!u)
            return __LINE__;
        if (u->level != COL_START_LEVEL || u->exp != 0)
            result = __LINE__;
        else if (u->maxHP != u->pCharacterData->baseHP + u->pClassData->baseHP
                 || u->pow != u->pCharacterData->basePow + u->pClassData->basePow
                 || u->spd != u->pCharacterData->baseSpd + u->pClassData->baseSpd)
            result = __LINE__;
        else if (u->curHP != u->maxHP)
            result = __LINE__;
        else {
            int k, usable = 0;
            for (k = 0; k < UNIT_ITEM_COUNT; k++)
                if (u->items[k] && (CanUnitUseWeapon(u, u->items[k]) || CanUnitUseStaff(u, u->items[k])))
                    usable = 1;
            if (!usable)
                result = __LINE__;
        }
        ClearUnit(u);
    }
    return result;
}

/* Level cap 30 (spec 21): EXP keeps working past vanilla's 20 and stops at 30. */
int Test_LevelCap(struct Unit *a, struct Unit *t)
{
    struct BattleUnit bu;
    struct Unit *u = Col_LoadPoolUnit(0);
    int result = 0;

    if (!u)
        return __LINE__;
    u->level = 20;
    u->exp = 99;
    InitBattleUnit(&bu, u);
    bu.unit.exp = 100;
    CheckBattleUnitLevelUp(&bu);
    if (bu.unit.level != 21 || bu.unit.exp == UNIT_EXP_DISABLED)
        result = __LINE__;

    u->level = 29;
    InitBattleUnit(&bu, u);
    bu.unit.exp = 100;
    CheckBattleUnitLevelUp(&bu);
    if (!result && (bu.unit.level != COL_MAX_LEVEL || bu.unit.exp != UNIT_EXP_DISABLED))
        result = __LINE__;
    ClearUnit(u);
    return result;
}

/* No stat caps (spec 21): gains beyond the old class caps and Luck 30 are kept; only the s8
 * ceiling (127) limits them. */
int Test_NoStatCaps(struct Unit *a, struct Unit *t)
{
    struct BattleUnit bu;
    struct Unit *u = Col_LoadPoolUnit(0);
    int result = 0;

    if (!u)
        return __LINE__;
    u->pow = 60;
    u->lck = 40;
    u->spd = 127;
    u->maxHP = 90;
    bu.changePow = 2;
    bu.changeLck = 1;
    bu.changeSpd = 1;
    bu.changeHP = 3;
    bu.changeSkl = bu.changeDef = bu.changeRes = 0;
    CheckBattleUnitStatCaps(u, &bu);
    if (bu.changePow != 2 || bu.changeLck != 1 || bu.changeSpd != 0 || bu.changeHP != 3)
        result = __LINE__;

    UnitCheckStatCaps(u);
    if (!result && (u->pow != 60 || u->lck != 40 || u->maxHP != 90))
        result = __LINE__;
    ClearUnit(u);
    return result;
}

/* Level-up choices (spec 23): 3 random stats, duplicates allowed, +1 applied, one per level. */
int Test_StatChoices(struct Unit *a, struct Unit *t)
{
    u8 opts[COL_STAT_CHOICES];
    u16 seen = 0;
    int i, dup = 0, hp, cur, mag;

    for (i = 0; i < 300; i++) {
        Col_RollStatChoices(opts);
        CHECK(opts[0] < COL_STAT_COUNT && opts[1] < COL_STAT_COUNT && opts[2] < COL_STAT_COUNT);
        seen |= (1 << opts[0]) | (1 << opts[1]) | (1 << opts[2]);
        if (opts[0] == opts[1] || opts[1] == opts[2] || opts[0] == opts[2])
            dup = 1;
    }
    CHECK(seen == (1 << COL_STAT_COUNT) - 1);                /* every stat can be offered */
    CHECK(dup);                                               /* duplicates happen */

    /* +1 to the chosen stat; HP raises max and current; nothing past 127 */
    hp = a->maxHP;
    cur = a->curHP;
    CHECK(Col_AddStat(a, COL_STAT_HP, 1) == 1);
    CHECK(a->maxHP == hp + 1 && a->curHP == cur + 1);
    mag = a->_u3A;
    CHECK(Col_AddStat(a, COL_STAT_MAG, 1) == 1 && a->_u3A == mag + 1);
    a->def = 127;
    CHECK(Col_AddStat(a, COL_STAT_DEF, 1) == 0 && a->def == 127);
    return 0;
}

/* Owed choices: one per level above choiceLevel, for living roster units only. */
int Test_PendingChoices(struct Unit *a, struct Unit *t)
{
    int slot, s, str;
    struct Unit *u;

    for (slot = -1, s = 0; s < COL_MAX_ROSTER; s++)     /* the actor: restored by the runner */
        if (Col_RosterUnit(s) == a)
            slot = s;
    CHECK(slot >= 0);
    u = a;
    for (s = 0; s < COL_MAX_ROSTER; s++)                     /* nothing owed */
        if (Col_RosterUnit(s))
            gColRun.choiceLevel[s] = (u8)Col_RosterUnit(s)->level;
    CHECK(Col_FindPendingChoice() == -1);

    u->level += 2;                                            /* two level-ups */
    CHECK(Col_FindPendingChoice() == slot);
    str = u->pow;
    Col_TakeStatChoice(slot, COL_STAT_STR);
    CHECK(u->pow == str + 1);
    CHECK(Col_FindPendingChoice() == slot);
    Col_TakeStatChoice(slot, COL_STAT_STR);
    CHECK(Col_FindPendingChoice() == -1);
    CHECK(u->pow == str + 2);

    u->level++;
    u->state |= US_DEAD;                                      /* the dead get no choices */
    CHECK(Col_FindPendingChoice() == -1);
    return 0;
}

/* spec 40: no weapon durability; staves are unlimited too (developer, 2026-09-25). Every weapon
 * and staff is indestructible; using one leaves it unchanged, including a real battle;
 * consumables still get used up. */
int Test_NoDurability(struct Unit *a, struct Unit *t)
{
    int id, item;

    for (id = 1; id < 0xC0; id++) {
        int attr = GetItemAttributes(id);
        if (attr & (IA_WEAPON | IA_STAFF))
            CHECK(attr & IA_UNBREAKABLE);
    }
    item = MakeNewItem(0x01);                              /* Iron Sword */
    CHECK(GetItemAfterUse(item) == item);
    item = MakeNewItem(0x4B);                              /* Heal staff */
    CHECK(GetItemAfterUse(item) == item);
    item = MakeNewItem(0x6C);                              /* Vulnerary: still consumed */
    CHECK(GetItemAfterUse(item) != item);

    a->items[0] = MakeNewItem(0x01);
    t->items[0] = MakeNewItem(0x01);
    a->xPos = 6; a->yPos = 4; t->xPos = 7; t->yPos = 4;
    a->ranks[0] = t->ranks[0] = 1;
    BattleGenerateReal(a, t);
    BattleApplyUnitUpdates();
    CHECK(a->items[0] == MakeNewItem(0x01));
    return 0;
}
