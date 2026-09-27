/* On-target unit tests (test build only: py -3 scripts/build.py --test).
 * Each test returns 0 on success or the line number of the first failed check.
 * tests/run_tests.py calls ColTest_Count() and ColTest_Run(i) through the emulator's
 * debugger and reports the results. */
#include "colosseum.h"
#include "bmsave.h"

#define CHECK(cond) do { if (!(cond)) return __LINE__; } while (0)

static int Test_NewRun(void)
{
    Col_RunNew(1234);
    CHECK(Col_RunIsValid());
    CHECK(gColRun.active == 1);
    CHECK(gColRun.floor == 1);
    CHECK(gColRun.recoverCharges == COL_RECOVER_PER_FLOOR);
    CHECK(gColRun.gold == 0);
    CHECK(gColRun.roster[0] == 0xFF && gColRun.roster[4] == 0xFF);
    CHECK(gColRun.deployed[2] == 0xFF);
    CHECK(Col_NextEncounter() == COL_ENC_NORMAL);
    return 0;
}

/* spec 5: B1 B2 B3 E B4 B5 B6 E B7 B8 B9 Boss, then the next floor; spec 51: reward every 3 wins */
static int Test_FloorSequence(void)
{
    static const u8 expected[] = {
        COL_ENC_NORMAL, COL_ENC_NORMAL, COL_ENC_NORMAL, COL_ENC_ELITE,
        COL_ENC_NORMAL, COL_ENC_NORMAL, COL_ENC_NORMAL, COL_ENC_ELITE,
        COL_ENC_NORMAL, COL_ENC_NORMAL, COL_ENC_NORMAL, COL_ENC_BOSS,
    };
    int floorNo, i, rewards = 0;

    Col_RunNew(1);
    for (floorNo = 1; floorNo <= 2; floorNo++) {
        CHECK(gColRun.floor == floorNo);
        for (i = 0; i < (int)sizeof(expected); i++) {
            int enc = Col_NextEncounter();
            CHECK(enc == expected[i]);
            Col_OnVictory(enc);
            if (gColRun.rewardDue) {
                rewards++;
                CHECK(enc == COL_ENC_NORMAL);
                Col_TakeReward();
            }
        }
    }
    CHECK(rewards == 6);                 /* 3 per floor */
    CHECK(gColRun.floor == 3);
    CHECK(gColRun.battlesWon == 24);
    return 0;
}

/* spec 54: 3 charges per floor, costs gold, reset after the boss */
static int Test_Recover(void)
{
    Col_RunNew(1);
    CHECK(!Col_CanRecover(100));         /* no gold */
    Col_AddGold(250);
    CHECK(Col_UseRecover(100));
    CHECK(Col_UseRecover(100));
    CHECK(!Col_UseRecover(100));         /* 50 gold left */
    Col_AddGold(500);
    CHECK(Col_UseRecover(100));
    CHECK(gColRun.recoverCharges == 0);
    CHECK(!Col_UseRecover(100));         /* no charges */
    CHECK(gColRun.gold == 450);
    gColRun.normalWins = COL_NORMAL_FIGHTS_FLOOR;
    Col_OnVictory(COL_ENC_BOSS);
    CHECK(gColRun.recoverCharges == COL_RECOVER_PER_FLOOR);
    return 0;
}

static int Test_Gold(void)
{
    Col_RunNew(1);
    CHECK(Col_AddGold(-5) == 0);
    CHECK(Col_AddGold(COL_GOLD_MAX - 10) == COL_GOLD_MAX - 10);
    CHECK(Col_AddGold(100) == 10);       /* capped */
    CHECK(gColRun.gold == COL_GOLD_MAX);
    CHECK(!Col_SpendGold(COL_GOLD_MAX + 1));
    CHECK(Col_SpendGold(COL_GOLD_MAX));
    CHECK(gColRun.gold == 0);
    return 0;
}

static int Test_InvalidLoadClears(void)
{
    Col_RunNew(99);
    gColRun.magic = 0xFFFFFFFF;          /* as a save from before COLOSSEUM data existed */
    CHECK(!Col_RunIsValid());
    Col_RunClear();
    CHECK(Col_RunIsValid() && !gColRun.active);
    return 0;
}

/* The save chunk functions through real SRAM: save slot 3's COLOSSEUM chunk (COLOSSEUM uses one
 * run slot, spec 77). */
static int Test_SramRoundTrip(void)
{
    u8 *sram = (u8 *)GetSaveWriteAddr(2) + 0x11F0;   /* game save chunk offset (ExModularSave.event) */

    Col_RunNew(0xABCD1234);
    Col_AddGold(4321);
    gColRun.floor = 3;
    gColRun.roster[0] = 7;
    Col_SaveRunChunk(sram, COL_RUN_SIZE);
    Col_RunClear();
    CHECK(gColRun.gold == 0 && gColRun.floor == 0);
    Col_LoadRunChunk(sram, COL_RUN_SIZE);
    CHECK(Col_RunIsValid());
    CHECK(gColRun.active == 1 && gColRun.gold == 4321 && gColRun.floor == 3);
    CHECK(gColRun.roster[0] == 7 && gColRun.seed == 0xABCD1234);
    return 0;
}

/* spec 12: deaths are permanent; a reserve steps in; 2v3 when only two remain */
static int Test_Deaths(void)
{
    int d, deployedCount;

    Col_RunNew(1);
    gColRun.roster[0] = 3; gColRun.roster[1] = 7; gColRun.roster[2] = 11; gColRun.roster[3] = 14;
    gColRun.rosterCount = 4;
    gColRun.deployed[0] = 0; gColRun.deployed[1] = 1; gColRun.deployed[2] = 2;

    Col_RosterRemoveDead(1);                         /* pool 7 dies; reserve (slot 3) deploys */
    CHECK(gColRun.deadMask == (1 << 7));
    CHECK(gColRun.roster[1] == 0xFF && gColRun.rosterCount == 3);
    CHECK(gColRun.deployed[1] == 3);

    Col_RosterRemoveDead(0);
    Col_RosterRemoveDead(0);                         /* already gone: no change */
    CHECK(gColRun.rosterCount == 2);
    for (deployedCount = 0, d = 0; d < COL_MAX_DEPLOY; d++)
        if (gColRun.deployed[d] != 0xFF)
            deployedCount++;
    CHECK(deployedCount == 2);                       /* 2v3 */
    CHECK(gColRun.deadMask == ((1 << 7) | (1 << 3)));
    return 0;
}

/* `%` works (divmod.c): FE-CLib mapped GCC's modulo helpers onto FE8 routines with another
 * calling convention. volatile keeps the compiler from folding the constants. */
static int Test_Modulo(void)
{
    volatile int a = 7, b = 3, c = -7, big = 1000000007, p = 97;
    volatile unsigned u = 10, v = 4;
    CHECK(a % b == 1);
    CHECK(c % b == -1);
    CHECK(a / b == 2);
    CHECK(u % v == 2u);
    CHECK(big % p == 1000000007 - (1000000007 / 97) * 97);
    CHECK(3 % b == 0);
    return 0;
}

typedef int (*ColTestFn)(void);
static const ColTestFn kTests[] = {
    Test_NewRun, Test_FloorSequence, Test_Recover, Test_Gold, Test_InvalidLoadClears,
    Test_SramRoundTrip, Test_Deaths, Test_Modulo,
};

int ColTest_Count(void)
{
    return sizeof(kTests) / sizeof(kTests[0]);
}

static void CopyBytes(void *dst, const void *src, unsigned n)
{
    u8 *d = dst;
    const u8 *s = src;
    while (n--)
        *d++ = *s++;
}

/* Runs test i with the real run state saved (on the stack) and restored around it. */
int ColTest_Run(int i)
{
    struct ColRunState saved;
    int result;

    CopyBytes(&saved, &gColRun, sizeof(saved));
    result = (i >= 0 && i < ColTest_Count()) ? kTests[i]() : -1;
    CopyBytes(&gColRun, &saved, sizeof(saved));
    return result;
}
