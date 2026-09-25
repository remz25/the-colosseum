/* Roster (spec 7-9, 12, 45, 77): starting a run, recruitment and replacement, deployment, and
 * the end of a run.
 *
 *   Start (spec 7): 3 random pool characters, all deployed.
 *   Recruitment (spec 9): up to 3 random characters never recruited in this run (alive, dead or
 *     replaced); pick one or decline. A full roster (5) means replacing someone, who leaves the
 *     run for good. Recruits join at the roster's average level with a fixed build (spec 45):
 *     their level-5 bases plus their average growth for the extra levels.
 *   Deployment (spec 8): 3 of the living roster (all of them with fewer than 3: 2v3, spec 12).
 *   End (spec 12 + developer decision): losing a battle (every deployed unit dead or the turn
 *     limit) ends the run: it is cleared and its save slot and suspend are invalidated, so a lost
 *     run cannot be continued. New Game always starts a fresh run. */
#include "coliseum.h"
#include "bmunit.h"
#include "bmsave.h"
#include "rng.h"
#include "event.h"
#include "variables.h"

#define SAVE_ID_SUSPEND 3

extern const u8 MagCharTable[];                 /* MagCharEditor.csv: {base, growth} per character */

/* ---- start ---- */

void Col_StartRun(void)
{
    int i, picked = 0;

    for (i = FACTION_BLUE + 1; i < FACTION_BLUE + 0x40; i++) {   /* nothing from an old run */
        struct Unit *u = GetUnit(i);
        if (u && u->pCharacterData)
            ClearUnit(u);
    }
    Col_RunNew((u32)NextRN());
    Col_ClearRunSkills();                                        /* skills are per run */
    while (picked < 3) {
        int pool = NextRN_N(COL_POOL_SIZE);
        if (gColRun.recruitedMask & (1 << pool))
            continue;
        gColRun.recruitedMask |= (u16)(1 << pool);
        gColRun.roster[picked] = (u8)pool;
        gColRun.deployed[picked] = (u8)picked;
        gColRun.hp[picked] = 0;                  /* 0 = full (not yet in a battle) */
        gColRun.choiceLevel[picked] = 0;         /* set when the unit is created */
        picked++;
    }
    gColRun.rosterCount = 3;
}

/* ---- recruitment ---- */

int Col_FreeRosterSlot(void)
{
    int slot;
    for (slot = 0; slot < COL_MAX_ROSTER; slot++)
        if (gColRun.roster[slot] >= COL_POOL_SIZE)
            return slot;
    return -1;
}

/* Up to 3 distinct candidates; returns how many (0 when everyone was already recruited). */
int Col_RollRecruits(u8 out[COL_RECRUIT_CHOICES])
{
    u8 eligible[COL_POOL_SIZE];
    int n = 0, count = 0, pool;

    for (pool = 0; pool < COL_POOL_SIZE; pool++)
        if (!(gColRun.recruitedMask & (1 << pool)))
            eligible[n++] = (u8)pool;
    while (count < COL_RECRUIT_CHOICES && n > 0) {
        int k = NextRN_N(n);
        out[count++] = eligible[k];
        eligible[k] = eligible[--n];
    }
    return count;
}

/* The living roster's average level (at least 5). */
int Col_RecruitLevel(void)
{
    int slot, sum = 0, n = 0;
    for (slot = 0; slot < COL_MAX_ROSTER; slot++) {
        struct Unit *u = Col_RosterUnit(slot);
        if (u && !(u->state & US_DEAD)) {
            sum += u->level;
            n++;
        }
    }
    if (!n || sum / n < COL_START_LEVEL)
        return COL_START_LEVEL;
    return sum / n > COL_MAX_LEVEL ? COL_MAX_LEVEL : sum / n;
}

static int AverageGain(int growth, int levels)
{
    return (growth * levels + 50) / 100;
}

/* A level-5 unit raised to `level` by its average growth: the same build every time (spec 45). */
void Col_ScaleUnitToLevel(struct Unit *unit, int level)
{
    const struct CharacterData *c = unit->pCharacterData;
    int d;

    if (level > COL_MAX_LEVEL)
        level = COL_MAX_LEVEL;
    d = level - unit->level;
    if (d <= 0)
        return;
    Col_AddStat(unit, COL_STAT_HP, AverageGain(c->growthHP, d));
    Col_AddStat(unit, COL_STAT_STR, AverageGain(c->growthPow, d));
    Col_AddStat(unit, COL_STAT_MAG, AverageGain(MagCharTable[c->number * 2 + 1], d));
    Col_AddStat(unit, COL_STAT_SKL, AverageGain(c->growthSkl, d));
    Col_AddStat(unit, COL_STAT_SPD, AverageGain(c->growthSpd, d));
    Col_AddStat(unit, COL_STAT_LCK, AverageGain(c->growthLck, d));
    Col_AddStat(unit, COL_STAT_DEF, AverageGain(c->growthDef, d));
    Col_AddStat(unit, COL_STAT_RES, AverageGain(c->growthRes, d));
    unit->curHP = unit->maxHP;
    unit->level = (s8)level;
    unit->exp = level >= COL_MAX_LEVEL ? UNIT_EXP_DISABLED : 0;
}

/* Leaves the run for good (replaced, spec 9): not dead, but never recruitable again. */
void Col_RemoveFromRoster(int slot)
{
    struct Unit *u = Col_RosterUnit(slot);
    int d;

    if (slot < 0 || slot >= COL_MAX_ROSTER || gColRun.roster[slot] >= COL_POOL_SIZE)
        return;
    if (u)
        ClearUnit(u);
    gColRun.roster[slot] = 0xFF;
    gColRun.hp[slot] = 0;
    gColRun.choiceLevel[slot] = 0;
    if (gColRun.rosterCount)
        gColRun.rosterCount--;
    for (d = 0; d < COL_MAX_DEPLOY; d++)
        if (gColRun.deployed[d] == slot)
            gColRun.deployed[d] = 0xFF;
    Col_AutoDeploy();
}

/* Recruit pool character `pool` into an empty slot, or replacing `replaceSlot` (-1: empty slot).
 * Returns the new unit (a hidden reserve until a battle deploys it), or NULL if not possible. */
struct Unit *Col_Recruit(int pool, int replaceSlot)
{
    struct Unit *unit;
    int slot, level;

    if (pool < 0 || pool >= COL_POOL_SIZE || (gColRun.recruitedMask & (1 << pool)))
        return NULL;
    level = Col_RecruitLevel();
    if (replaceSlot >= 0) {
        if (replaceSlot >= COL_MAX_ROSTER || gColRun.roster[replaceSlot] >= COL_POOL_SIZE)
            return NULL;
        Col_RemoveFromRoster(replaceSlot);
        slot = replaceSlot;
    } else {
        slot = Col_FreeRosterSlot();
        if (slot < 0)
            return NULL;
    }
    unit = Col_LoadPoolUnit(pool);
    if (!unit)
        return NULL;
    Col_ScaleUnitToLevel(unit, level);
    unit->state |= US_HIDDEN | US_NOT_DEPLOYED;
    gColRun.roster[slot] = (u8)pool;
    gColRun.recruitedMask |= (u16)(1 << pool);
    gColRun.rosterCount++;
    gColRun.hp[slot] = 0;
    gColRun.choiceLevel[slot] = (u8)unit->level;
    Col_AutoDeploy();
    return unit;
}

/* ---- deployment ---- */

int Col_DeployTarget(void)
{
    return gColRun.rosterCount < COL_MAX_DEPLOY ? gColRun.rosterCount : COL_MAX_DEPLOY;
}

u8 Col_DeploymentMask(void)
{
    int d;
    u8 mask = 0;
    for (d = 0; d < COL_MAX_DEPLOY; d++)
        if (gColRun.deployed[d] < COL_MAX_ROSTER)
            mask |= (u8)(1 << gColRun.deployed[d]);
    return mask;
}

/* Deploy exactly the living roster slots in `mask` (bit = slot). 1 if valid and applied. */
int Col_SetDeployment(u8 mask)
{
    int slot, d = 0, n = 0;

    for (slot = 0; slot < COL_MAX_ROSTER; slot++)
        if (mask & (1 << slot)) {
            if (gColRun.roster[slot] >= COL_POOL_SIZE)
                return 0;
            n++;
        }
    if (mask >> COL_MAX_ROSTER || n != Col_DeployTarget())
        return 0;
    for (slot = 0; slot < COL_MAX_ROSTER; slot++)
        if (mask & (1 << slot))
            gColRun.deployed[d++] = (u8)slot;
    while (d < COL_MAX_DEPLOY)
        gColRun.deployed[d++] = 0xFF;
    return 1;
}

/* ---- end of a run ---- */

void Col_RunLost(void)
{
    if (gColRun.active) {
        InvalidateGameSave(gPlaySt.gameSaveSlot);
        InvalidateSuspendSave(SAVE_ID_SUSPEND);
    }
    Col_RunClear();
}

/* Replaces vanilla InitPlayConfig (0x08030CF4; only New Game and the unused trial maps call it):
 * the same play state reset, plus no run carried over into the new game. */
void Col_InitPlayConfig(int isDifficult, s8 controller)
{
    u16 *p = (u16 *)&gPlaySt;
    unsigned i;

    Col_RunClear();
    for (i = 0; i < sizeof(gPlaySt) / 2; i++)
        p[i] = 0;
    gPlaySt.chapterIndex = 0;
    if (isDifficult)
        gPlaySt.chapterStateBits |= PLAY_FLAG_HARD;
    gPlaySt.config.controller = controller;
    gPlaySt.config.textSpeed = 1;
}

/* Replaces vanilla CallGameOverEvent (0x0800D390): the run is lost, then FE8's game over. */
void Col_OnGameOver(void)
{
    Col_RunLost();
    EventEngine_Create((const u16 *)0x08592104, 2);   /* EventScr_GameOver, EV_EXEC_GAMEPLAY */
}
