/* On-target tests for Phase 5 (spec 7-9, 12, 45, 77): recruitment, replacement, recruit builds,
 * deployment (incl. 2v3), the end of a run and New Game. Map tests (they create units): run by
 * tests/run_tests.py through ColTest_MapRun (test_combat.c), which restores gColRun.
 * Test rosters use pool characters that are not on the map, so no real unit is touched. */
#include "coliseum.h"
#include "bmunit.h"
#include "bmsave.h"
#include "variables.h"

#define CHECK(cond) do { if (!(cond)) return __LINE__; } while (0)

extern const u8 MagCharTable[];

/* n pool indices whose characters are not on the map */
static int OffMapPools(u8 *out, int n)
{
    int pool, k = 0;
    for (pool = 0; pool < COL_POOL_SIZE && k < n; pool++)
        if (!GetUnitFromCharIdAndFaction(gColPool[pool].charId, FACTION_BLUE))
            out[k++] = (u8)pool;
    return k == n;
}

static void Roster(const u8 *pools, int n)
{
    int i;
    Col_RunNew(7);
    for (i = 0; i < n; i++) {
        gColRun.roster[i] = pools[i];
        gColRun.recruitedMask |= 1u << pools[i];
        gColRun.choiceLevel[i] = COL_START_LEVEL;
    }
    gColRun.rosterCount = (u8)n;
    Col_AutoDeploy();
}

/* spec 9: 3 distinct candidates never recruited in this run; fewer when few are left. */
int Test_RollRecruits(struct Unit *a, struct Unit *t)
{
    u8 out[COL_RECRUIT_CHOICES];
    int i, k, n;

    Col_RunNew(1);
    gColRun.recruitedMask = COL_POOL_ALL & ~((1u << 4) | (1u << 9));   /* only pools 4 and 9 left */
    n = Col_RollRecruits(out);
    CHECK(n == 2 && out[0] != out[1]);
    CHECK((out[0] == 4 || out[0] == 9) && (out[1] == 4 || out[1] == 9));
    gColRun.recruitedMask = COL_POOL_ALL;
    CHECK(Col_RollRecruits(out) == 0);

    gColRun.recruitedMask = (1 << 0) | (1 << 1) | (1 << 2);
    for (i = 0; i < 50; i++) {
        CHECK(Col_RollRecruits(out) == COL_RECRUIT_CHOICES);
        for (k = 0; k < COL_RECRUIT_CHOICES; k++) {
            CHECK(out[k] < COL_POOL_SIZE && !(gColRun.recruitedMask & (1 << out[k])));
            CHECK(out[k] != out[(k + 1) % COL_RECRUIT_CHOICES]);
        }
    }
    return 0;
}

/* Recruit into an empty slot: joins as a hidden reserve at the roster's level; never twice. */
int Test_RecruitEmptySlot(struct Unit *a, struct Unit *t)
{
    u8 p[4];
    struct Unit *u;
    int result = 0;

    CHECK(OffMapPools(p, 4));
    Roster(p, 3);
    u = Col_Recruit(p[3], -1);
    CHECK(u != NULL);
    if (gColRun.roster[3] != p[3] || gColRun.rosterCount != 4 || !(gColRun.recruitedMask & (1 << p[3])))
        result = __LINE__;
    else if (u->level != COL_START_LEVEL || gColRun.choiceLevel[3] != COL_START_LEVEL)
        result = __LINE__;
    else if (!(u->state & US_NOT_DEPLOYED) || (Col_DeploymentMask() & (1 << 3)))
        result = __LINE__;                               /* 3 already deployed: a reserve */
    else if (Col_Recruit(p[3], -1) != NULL)
        result = __LINE__;                               /* never twice */
    ClearUnit(u);
    return result;
}

/* Full roster: recruiting needs a replacement; the replaced one leaves for good (not dead). */
int Test_RecruitReplace(struct Unit *a, struct Unit *t)
{
    u8 p[6];
    struct Unit *units[COL_MAX_ROSTER + 1];
    int i, d, result = 0;

    CHECK(OffMapPools(p, 6));
    Roster(p, 0);
    for (i = 0; i < 5; i++)
        units[i] = Col_Recruit(p[i], -1);
    for (i = 0; i < 5; i++)
        CHECK(units[i] != NULL);
    CHECK(gColRun.rosterCount == 5 && Col_FreeRosterSlot() == -1);
    CHECK(Col_Recruit(p[5], -1) == NULL);                 /* full: must replace */

    units[5] = Col_Recruit(p[5], 1);                      /* replace slot 1 (deployed) */
    if (!units[5] || gColRun.roster[1] != p[5] || gColRun.rosterCount != 5)
        result = __LINE__;
    else if (Col_RosterUnit(1) != units[5])
        result = __LINE__;
    else if (!(gColRun.recruitedMask & (1 << p[1])) || (gColRun.deadMask & (1 << p[1])))
        result = __LINE__;                                /* gone, not dead, never again */
    else if (Col_Recruit(p[1], -1) != NULL)
        result = __LINE__;
    else {
        int n = 0;
        for (d = 0; d < COL_MAX_DEPLOY; d++)
            if (gColRun.deployed[d] < COL_MAX_ROSTER && gColRun.roster[gColRun.deployed[d]] < COL_POOL_SIZE)
                n++;
        if (n != 3)
            result = __LINE__;                            /* a reserve took the deployment */
    }
    for (i = 0; i < 6; i++)
        if (i != 1 && units[i])
            ClearUnit(units[i]);
    return result;
}

/* spec 45: a recruit's build is fixed: level-5 bases + average growth for the extra levels. */
int Test_RecruitBuild(struct Unit *a, struct Unit *t)
{
    struct Unit *u = Col_LoadPoolUnit(3);
    const struct CharacterData *c;
    int hp, pow, spd, mag, result = 0;

    CHECK(u != NULL);
    c = u->pCharacterData;
    hp = u->maxHP; pow = u->pow; spd = u->spd; mag = u->_u3A;
    Col_ScaleUnitToLevel(u, 15);
    if (u->level != 15 || u->exp != 0 || u->curHP != u->maxHP)
        result = __LINE__;
    else if (u->maxHP != hp + (c->growthHP * 10 + 50) / 100 || u->pow != pow + (c->growthPow * 10 + 50) / 100
             || u->spd != spd + (c->growthSpd * 10 + 50) / 100
             || u->_u3A != mag + (MagCharTable[c->number * 2 + 1] * 10 + 50) / 100)
        result = __LINE__;
    Col_ScaleUnitToLevel(u, 40);                          /* capped at 30, EXP off */
    if (!result && (u->level != COL_MAX_LEVEL || u->exp != UNIT_EXP_DISABLED))
        result = __LINE__;
    ClearUnit(u);
    return result;
}

/* spec 8, 12: exactly 3 of the living (all of them when fewer than 3: 2v3). */
int Test_Deployment(struct Unit *a, struct Unit *t)
{
    u8 p[5];

    CHECK(OffMapPools(p, 5));
    Roster(p, 5);
    CHECK(Col_DeployTarget() == 3);
    CHECK(Col_SetDeployment(0x1C));                       /* slots 2, 3, 4 */
    CHECK(Col_DeploymentMask() == 0x1C);
    CHECK(!Col_SetDeployment(0x03));                      /* 2 of 5: no */
    CHECK(!Col_SetDeployment(0x0F));                      /* 4: no */
    CHECK(Col_DeploymentMask() == 0x1C);                  /* unchanged after a refusal */

    gColRun.roster[4] = 0xFF;
    gColRun.rosterCount = 4;
    CHECK(!Col_SetDeployment(0x13));                      /* slot 4 is empty */

    Roster(p, 2);                                         /* 2 alive: 2v3 */
    CHECK(Col_DeployTarget() == 2);
    CHECK(Col_DeploymentMask() == 0x03);
    CHECK(Col_SetDeployment(0x03) && !Col_SetDeployment(0x01));
    return 0;
}

/* A lost run ends: cleared, and its game save and suspend are invalidated (spec 12, 77). */
int Test_RunLost(struct Unit *a, struct Unit *t)
{
    int slot = gPlaySt.gameSaveSlot, result = 0;

    gPlaySt.gameSaveSlot = 2;
    WriteGameSave(2);
    if (!IsSaveValid(2))
        result = __LINE__;
    else {
        gColRun.active = 1;
        Col_RunLost();
        if (IsSaveValid(2) || gColRun.active || !Col_RunIsValid())
            result = __LINE__;
    }
    gPlaySt.gameSaveSlot = (u8)slot;
    return result;
}

/* New Game (InitPlayConfig, hooked) never carries a run over; the play state is reset as vanilla. */
int Test_NewGameClearsRun(struct Unit *a, struct Unit *t)
{
    u8 saved[sizeof(struct PlaySt)];
    u8 *ps = (u8 *)&gPlaySt;
    unsigned i;
    int result = 0;

    for (i = 0; i < sizeof(saved); i++)
        saved[i] = ps[i];
    gColRun.active = 1;
    gPlaySt.chapterTurnNumber = 9;
    InitPlayConfig(1, 0);
    if (gColRun.active || !Col_RunIsValid())
        result = __LINE__;
    else if (gPlaySt.chapterTurnNumber != 0 || !(gPlaySt.chapterStateBits & PLAY_FLAG_HARD)
             || gPlaySt.config.textSpeed != 1)
        result = __LINE__;
    for (i = 0; i < sizeof(saved); i++)
        ps[i] = saved[i];
    return result;
}
