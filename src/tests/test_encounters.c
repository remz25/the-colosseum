/* On-target tests: Elite battles (spec 47-50) and enemy drops (encounters.c). Map tests
 * (ColTest_MapRun restores gColRun and the two units; units created here are removed). */
#include "coliseum.h"
#include "bmunit.h"
#include "bmitem.h"
#include "bmmap.h"
#include "bmbattle.h"

#define CHECK(cond) do { if (!(cond)) return __LINE__; } while (0)

static int IsRed(struct Unit *u) { return u && u->pCharacterData && UNIT_FACTION(u) == FACTION_RED; }

/* Every setup is sound: 1 (Champion) or 3 members, Elite role characters, weapons. */
int Test_EliteSetups(struct Unit *sA, struct Unit *sT)
{
    int s, m, champions = 0, squads = 0;
    const struct ColEliteSetup *e;

    for (s = 0; s < Col_EliteSetupCount(); s++) {
        e = &gColEliteSetups[s];
        CHECK(e->name[0] && (e->count == 1 || e->count == 3));
        if (e->count == 1) champions++; else squads++;
        for (m = 0; m < e->count; m++) {
            CHECK(e->members[m].charId >= 0x81 && e->members[m].charId <= 0x88);
            CHECK(GetItemAttributes(e->members[m].items[0]) & IA_WEAPON);
            CHECK(GetClassData(e->members[m].classId)->attributes & CA_PROMOTED);
        }
    }
    CHECK(champions == 4 && squads == 3);

    /* no Elite when a normal battle is next; rolled once and kept when one is */
    Col_RunNew(1);
    CHECK(Col_CurrentElite() == NULL && gColRun.elite == 0);
    gColRun.eliteDue = 1;
    e = Col_CurrentElite();
    CHECK(e && gColRun.elite >= 1 && gColRun.elite <= Col_EliteSetupCount());
    for (s = 0; s < 10; s++)
        CHECK(Col_CurrentElite() == e);
    return 0;
}

/* Spawns the enemies of `encounter` (Elite setup `elite`, 0 = random), collects the new red
 * units in out[], and returns how many. */
static int SpawnTest(int encounter, int elite, struct Unit **out)
{
    u8 before[0x40] = { 0 };
    int i, n = 0;

    for (i = 1; i < 0x40; i++)
        before[i] = IsRed(GetUnit(FACTION_RED + i));
    gColRun.elite = (u8)elite;
    Col_CreateEnemies(encounter);
    for (i = 1; i < 0x40 && n < 3; i++)
        if (!before[i] && IsRed(GetUnit(FACTION_RED + i)))
            out[n++] = GetUnit(FACTION_RED + i);
    return n;
}

static void Despawn(struct Unit **units, int n)
{
    int i;
    for (i = 0; i < n; i++)
        ClearUnit(units[i]);
    RefreshEntityBmMaps();
}

static int DropsOn(struct Unit **units, int n, int kind)
{
    int i, d, c = 0;
    for (d = 0; d < COL_MAX_DROPS; d++)
        for (i = 0; i < n; i++)
            if (gColRun.drops[d].kind == kind && gColRun.drops[d].unit == (u8)units[i]->index)
                c++;
    return c;
}

/* A Champion: one boosted promoted enemy with its Elite weapon (usable, dropped) and a relic. */
int Test_EliteChampion(struct Unit *sA, struct Unit *sT)
{
    struct Unit *u[3];
    int n, result = 0;
    const struct ClassData *cls;

    Col_RunNew(2);
    gColRun.eliteDue = 1;
    gColRun.battlesWon = 3;
    n = SpawnTest(COL_ENC_ELITE, 1, u);                     /* setup 1: Champion: Blademaster */
    if (n != 1) { result = __LINE__; goto out; }
    cls = u[0]->pClassData;
    if (u[0]->pCharacterData->number != 0x81 || cls->number != 0x15) { result = __LINE__; goto out; }
    if (ITEM_INDEX(u[0]->items[0]) != 0xC0 || !CanUnitUseWeapon(u[0], u[0]->items[0])) { result = __LINE__; goto out; }
    if (!(u[0]->state & US_DROP_ITEM)) { result = __LINE__; goto out; }
    if (u[0]->maxHP < 30 || u[0]->curHP != u[0]->maxHP) { result = __LINE__; goto out; }   /* +12 HP */
    if (DropsOn(u, n, COL_DROP_RELIC) != 1) { result = __LINE__; goto out; }             /* guaranteed */
    if (!Col_RelicDef(gColRun.drops[0].value) && !Col_RelicDef(gColRun.drops[1].value)) { result = __LINE__; goto out; }
out:
    Despawn(u, n);
    return result;
}

/* An Elite Squad: 3 role enemies, all able to use their weapons/staves, one guaranteed relic. */
int Test_EliteSquad(struct Unit *sA, struct Unit *sT)
{
    struct Unit *u[3];
    int n, i, result = 0;

    Col_RunNew(3);
    gColRun.eliteDue = 1;
    gColRun.battlesWon = 3;
    n = SpawnTest(COL_ENC_ELITE, 5, u);                     /* setup 5: Elite Squad: Vanguard */
    if (n != 3) { result = __LINE__; goto out; }
    for (i = 0; i < n; i++) {
        if (!CanUnitUseWeapon(u[i], u[i]->items[0])) { result = __LINE__; goto out; }
        if (u[i]->state & US_DROP_ITEM) { result = __LINE__; goto out; }                 /* only Champions */
    }
    if (u[1]->pCharacterData->number != 0x86 || !CanUnitUseStaff(u[1], u[1]->items[1])) { result = __LINE__; goto out; }
    if (DropsOn(u, n, COL_DROP_RELIC) < 1) { result = __LINE__; goto out; }
out:
    Despawn(u, n);
    return result;
}

/* Normal battles: 3 generic enemies; the run's first battle always has a relic drop. */
int Test_NormalEnemiesAndFirstRelic(struct Unit *sA, struct Unit *sT)
{
    struct Unit *u[3];
    int n, i, result = 0;

    Col_RunNew(4);
    n = SpawnTest(COL_ENC_NORMAL, 0, u);
    if (n != 3) { result = __LINE__; goto out; }
    for (i = 0; i < n; i++)
        if (u[i]->pCharacterData->number != 0x80) { result = __LINE__; goto out; }
    if (DropsOn(u, n, COL_DROP_RELIC) < 1) { result = __LINE__; goto out; }             /* first battle */
out:
    Despawn(u, n);
    return result;
}

/* Drop odds: normal 25% per enemy (gold / skill / relic), Elite squads 50% + a relic. */
int Test_DropOdds(struct Unit *sA, struct Unit *sT)
{
    struct Unit *one[1] = { sT };
    int r, drops = 0, kinds[4] = { 0 };

    Col_RunNew(5);
    gColRun.battlesWon = 0;                                 /* first battle: always a relic */
    for (r = 0; r < 20; r++) {
        Col_RollDrops(COL_ENC_NORMAL, one, 1);
        CHECK(gColRun.drops[0].kind == COL_DROP_RELIC && gColRun.drops[0].unit == (u8)sT->index);
        CHECK(Col_RelicDef(gColRun.drops[0].value));
    }
    gColRun.battlesWon = 4;
    for (r = 0; r < 400; r++) {
        Col_RollDrops(COL_ENC_NORMAL, one, 1);
        if (gColRun.drops[0].kind) {
            drops++;
            kinds[gColRun.drops[0].kind]++;
            if (gColRun.drops[0].kind == COL_DROP_GOLD)
                CHECK(gColRun.drops[0].value * 10 >= 120 && gColRun.drops[0].value * 10 <= 180);
        }
    }
    CHECK(drops > 60 && drops < 140);                        /* ~100 of 400 */
    CHECK(kinds[COL_DROP_GOLD] > kinds[COL_DROP_SKILL] && kinds[COL_DROP_SKILL] > 5 && kinds[COL_DROP_RELIC] > 5);
    for (r = 0; r < 20; r++) {
        Col_RollDrops(COL_ENC_ELITE, one, 1);
        CHECK(gColRun.drops[0].kind == COL_DROP_RELIC);     /* every Elite */
    }
    Col_RollDrops(COL_ENC_NORMAL, one, 0);
    CHECK(gColRun.drops[0].kind == COL_DROP_NONE);
    return 0;
}

/* The killing blow of a real battle is recorded; forecasts don't count; a drop is claimable
 * only once its enemy is dead. */
int Test_DropKiller(struct Unit *sA, struct Unit *sT)
{
    struct Unit *one[1] = { sT };
    int i;

    Col_RunNew(6);
    gColRun.battlesWon = 0;
    Col_RollDrops(COL_ENC_NORMAL, one, 1);                  /* sT carries a relic */
    CHECK(Col_PendingDrop() < 0);                           /* alive */

    /* sA (a Mercenary with an Iron Sword) certain to kill sT with its first strike */
    sA->pClassData = GetClassData(0x0F);
    for (i = 0; i < UNIT_ITEM_COUNT; i++)
        sA->items[i] = 0;
    sA->items[0] = MakeNewItem(0x01);
    sA->ranks[0] = 1;
    sA->pow = 40; sA->skl = 60; sA->spd = 20;
    sT->curHP = 1; sT->maxHP = 20; sT->def = 0; sT->spd = 0; sT->lck = 0;
    sA->xPos = 6; sA->yPos = 4;
    sT->xPos = 7; sT->yPos = 4;

    BattleGenerateSimulation(sA, sT, sA->xPos, sA->yPos, 0);
    CHECK(gColRun.drops[0].killer == 0);                    /* a forecast is not a kill */
    BattleGenerateReal(sA, sT);
    CHECK(gBattleTarget.unit.curHP == 0);
    CHECK(gColRun.drops[0].killer == (u8)sA->index);

    sT->curHP = 0;                                          /* the enemy is dead: claimable */
    CHECK(Col_PendingDrop() == 0);
    gColRun.drops[0].kind = COL_DROP_NONE;                  /* claimed */
    CHECK(Col_PendingDrop() < 0);
    return 0;
}
