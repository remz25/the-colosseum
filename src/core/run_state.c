/* COLISEUM run state: floor progression, encounters, gold, Recover charges (spec 5, 51, 54, 76).
 * Pure logic on gColRun (no UI), so it can be unit-tested on the target (src/tests). */
#include "coliseum.h"
#include "agb_sram.h"

_Static_assert(sizeof(struct ColRunState) == COL_RUN_SIZE, "ColRunState must stay COL_RUN_SIZE bytes");
_Static_assert(COL_RUN_SIZE <= COL_RAM_SIZE, "run state must fit the COLISEUM RAM block");

void Col_RunClear(void)
{
    u8 *p = (u8 *)&gColRun;
    unsigned i;
    for (i = 0; i < sizeof(struct ColRunState); i++)
        p[i] = 0;
    for (i = 0; i < COL_MAX_ROSTER; i++)
        gColRun.roster[i] = 0xFF;
    for (i = 0; i < COL_MAX_DEPLOY; i++)
        gColRun.deployed[i] = 0xFF;
    gColRun.magic = COL_RUN_MAGIC;
    gColRun.version = COL_RUN_VERSION;
}

/* A new run: floor 1, no wins, full Recover charges, no gold (spec 7, 54). The roster is
 * chosen afterwards (spec 7: 3 random characters). */
void Col_RunNew(u32 seed)
{
    Col_RunClear();
    gColRun.active = 1;
    gColRun.floor = 1;
    gColRun.recoverCharges = COL_RECOVER_PER_FLOOR;
    gColRun.seed = seed;
}

int Col_RunIsValid(void)
{
    return gColRun.magic == COL_RUN_MAGIC && gColRun.version == COL_RUN_VERSION;
}

/* The encounter the player faces next (spec 5): battles 1-3, Elite, 4-6, Elite, 7-9, Boss.
 * After the 9th normal win the boss replaces that floor's third Elite. */
int Col_NextEncounter(void)
{
    if (gColRun.normalWins >= COL_NORMAL_FIGHTS_FLOOR)
        return COL_ENC_BOSS;
    if (gColRun.eliteDue)
        return COL_ENC_ELITE;
    return COL_ENC_NORMAL;
}

void Col_OnVictory(int encounter)
{
    gColRun.battlesWon++;
    switch (encounter) {
    case COL_ENC_NORMAL:
        gColRun.normalWins++;
        if (++gColRun.winsToReward >= COL_WINS_PER_REWARD) {   /* spec 51 */
            gColRun.winsToReward = 0;
            gColRun.rewardDue = 1;
            if (gColRun.normalWins < COL_NORMAL_FIGHTS_FLOOR)   /* spec 47; the boss replaces the 3rd */
                gColRun.eliteDue = 1;
        }
        break;
    case COL_ENC_ELITE:
        gColRun.eliteDue = 0;
        break;
    case COL_ENC_BOSS:                                         /* next floor (spec 5, 54) */
        gColRun.floor++;
        gColRun.normalWins = 0;
        gColRun.winsToReward = 0;
        gColRun.eliteDue = 0;
        gColRun.recoverCharges = COL_RECOVER_PER_FLOOR;
        break;
    }
}

void Col_TakeReward(void)
{
    gColRun.rewardDue = 0;
}

int Col_AddGold(int amount)
{
    u32 g = gColRun.gold;
    if (amount <= 0)
        return 0;
    if (g + (u32)amount > COL_GOLD_MAX)
        amount = (int)(COL_GOLD_MAX - g);
    gColRun.gold = g + (u32)amount;
    return amount;
}

int Col_SpendGold(u32 amount)
{
    if (gColRun.gold < amount)
        return 0;
    gColRun.gold -= amount;
    return 1;
}

int Col_CanRecover(u32 cost)
{
    return gColRun.recoverCharges > 0 && gColRun.gold >= cost;
}

int Col_UseRecover(u32 cost)
{
    if (!Col_CanRecover(cost))
        return 0;
    gColRun.gold -= cost;
    gColRun.recoverCharges--;
    return 1;
}

/* ---- roster ---- */

/* A roster member died (spec 12): marked dead for the run (never recruitable again), the slot
 * and any deployment of it are freed, and a living reserve takes the deployment. */
void Col_RosterRemoveDead(int slot)
{
    int d, pool;

    if (slot < 0 || slot >= COL_MAX_ROSTER || gColRun.roster[slot] >= COL_POOL_SIZE)
        return;
    pool = gColRun.roster[slot];
    gColRun.deadMask |= (u16)(1 << pool);
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

static int IsDeployed(int slot)
{
    int d;
    for (d = 0; d < COL_MAX_DEPLOY; d++)
        if (gColRun.deployed[d] == slot)
            return 1;
    return 0;
}

/* Until the deployment screen exists (Phase 5), empty deployment slots take living reserves in
 * roster order. With fewer than 3 living units the battle is 2v3 or 1v3 (spec 12). */
void Col_AutoDeploy(void)
{
    int d, slot;

    for (d = 0; d < COL_MAX_DEPLOY; d++) {
        if (gColRun.deployed[d] != 0xFF)
            continue;
        for (slot = 0; slot < COL_MAX_ROSTER; slot++)
            if (gColRun.roster[slot] < COL_POOL_SIZE && !IsDeployed(slot)) {
                gColRun.deployed[d] = (u8)slot;
                break;
            }
    }
}

/* ---- save chunks ---- */

void Col_SaveRunChunk(void *sram, unsigned size)
{
    if (size < sizeof(struct ColRunState))
        return;
    WriteAndVerifySramFast(&gColRun, sram, sizeof(struct ColRunState));
}

/* Loading a save without COLISEUM data (or an older layout) leaves no active run. */
void Col_LoadRunChunk(void *sram, unsigned size)
{
    if (size < sizeof(struct ColRunState))
        return;
    ReadSramFast(sram, &gColRun, sizeof(struct ColRunState));
    if (!Col_RunIsValid())
        Col_RunClear();
}
