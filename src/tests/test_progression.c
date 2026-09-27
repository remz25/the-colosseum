/* On-target tests: roguelike progression (Phase 10, src/core/reward.c): the 3-win reward,
 * Recover, promotion (level kept, level 10+). Map tests: ColTest_MapRun restores gColRun and the
 * two units. The first living blue unit (sA) is put in roster slot 0 as the only member. */
#include "coliseum.h"
#include "bmunit.h"
#include "bmitem.h"
#include "bmitemuse.h"
#include "bmbattle.h"

#define CHECK(cond) do { if (!(cond)) return __LINE__; } while (0)

#define CLASS_MERCENARY 0x0F
#define CLASS_HERO      0x11
#define ITEM_IRON_SWORD 0x01

/* sA alone in the roster, a level-`level` Mercenary with an Iron Sword, at full HP. */
static int Solo(struct Unit *sA, int level)
{
    int pool = Col_PoolIndexOfUnit(sA), i;

    if (pool < 0)
        return 0;
    Col_RunNew(7);
    gColRun.roster[0] = (u8)pool;
    gColRun.rosterCount = 1;
    gColRun.recruitedMask = 1u << pool;
    sA->pClassData = GetClassData(CLASS_MERCENARY);
    sA->level = (u8)level;
    sA->exp = 40;
    sA->state &= ~US_DEAD;
    sA->curHP = sA->maxHP;
    for (i = 0; i < UNIT_ITEM_COUNT; i++)
        sA->items[i] = 0;
    sA->items[0] = MakeNewItem(ITEM_IRON_SWORD);
    return 1;
}

static int Offered(int kind)
{
    int i;
    for (i = 0; i < 3; i++)
        if (gColRun.rewardKinds[i] == kind)
            return 1;
    return 0;
}

/* The offer: only valid kinds, all different, rolled once; Gold 100-500 (spec 52). */
int Test_RewardRoll(struct Unit *sA, struct Unit *sT)
{
    int k, i, j, minGold = 999, maxGold = 0;

    CHECK(Solo(sA, 5));
    CHECK(Col_RewardValid(COL_REWARD_GOLD) && Col_RewardValid(COL_REWARD_RECRUIT));
    CHECK(!Col_RewardValid(COL_REWARD_HEAL));                   /* everyone at full HP */
    CHECK(!Col_RewardValid(COL_REWARD_PROMOTION));              /* level 5 */
    for (k = 0; k < 60; k++) {
        gColRun.rewardDue = 1;
        for (i = 0; i < 3; i++)
            gColRun.rewardKinds[i] = 0;
        Col_RollReward();
        CHECK(Col_RewardCount() == 3);                          /* recruit, skill, gold valid */
        for (i = 0; i < 3; i++) {
            CHECK(gColRun.rewardKinds[i] != COL_REWARD_HEAL && gColRun.rewardKinds[i] != COL_REWARD_PROMOTION);
            for (j = 0; j < i; j++)
                CHECK(gColRun.rewardKinds[i] != gColRun.rewardKinds[j]);
        }
        if (Offered(COL_REWARD_SKILL))
            CHECK(gColRun.rewardSkill && Col_CanLearnRewardSkill(sA));
        CHECK(gColRun.rewardGold >= 100 && gColRun.rewardGold <= 500);
        if (gColRun.rewardGold < minGold) minGold = gColRun.rewardGold;
        if (gColRun.rewardGold > maxGold) maxGold = gColRun.rewardGold;
    }
    CHECK(maxGold - minGold > 150);                             /* really random */
    {                                                           /* rolled once: no reroll */
        u8 before[3] = { gColRun.rewardKinds[0], gColRun.rewardKinds[1], gColRun.rewardKinds[2] };
        Col_RollReward();
        CHECK(before[0] == gColRun.rewardKinds[0] && before[1] == gColRun.rewardKinds[1] &&
              before[2] == gColRun.rewardKinds[2]);
    }
    /* hurt + level 10: Heal and Promotion become valid */
    sA->curHP = 1;
    sA->level = 10;
    CHECK(Col_RewardValid(COL_REWARD_HEAL) && Col_RewardValid(COL_REWARD_PROMOTION));
    /* nobody left to recruit */
    gColRun.recruitedMask = COL_POOL_ALL;
    CHECK(!Col_RewardValid(COL_REWARD_RECRUIT));
    return 0;
}

/* Taking rewards: Gold adds the rolled amount; Heal heals fully; both end the reward. */
int Test_RewardTake(struct Unit *sA, struct Unit *sT)
{
    CHECK(Solo(sA, 5));
    gColRun.rewardDue = 1;
    Col_RollReward();
    {
        int gold = gColRun.rewardGold;
        gColRun.gold = 50;
        Col_TakeGoldReward();
        CHECK(gColRun.gold == (u32)(50 + gold));
        CHECK(gColRun.rewardDue == 0 && gColRun.rewardKinds[0] == 0 && gColRun.rewardGold == 0);
    }
    sA->curHP = 1;
    gColRun.rewardDue = 1;
    Col_RollReward();
    Col_TakeHealReward();
    CHECK(sA->curHP == sA->maxHP && gColRun.rewardDue == 0);
    return 0;
}

/* Promotion: level 10+, unpromoted; the seal goes into the inventory and FE8 accepts it; the
 * promoted unit keeps its level and EXP (both FE8 promotion functions). */
int Test_Promotion(struct Unit *sA, struct Unit *sT)
{
    int i;

    CHECK(Solo(sA, 9));
    CHECK(!Col_CanPromote(sA));
    sA->level = 10;
    CHECK(Col_CanPromote(sA));
    gColRun.rewardDue = 1;
    CHECK(Col_GivePromotionSeal(sA));
    CHECK(gColRun.rewardDue == 0);
    for (i = 0; i < UNIT_ITEM_COUNT; i++)
        if (ITEM_INDEX(sA->items[i]) == COL_MASTER_SEAL)
            break;
    CHECK(i < UNIT_ITEM_COUNT && CanUnitUseItem(sA, sA->items[i]));
    for (i = 1; i < UNIT_ITEM_COUNT; i++)                       /* full inventory: can't take one */
        sA->items[i] = MakeNewItem(ITEM_IRON_SWORD);
    CHECK(!Col_CanPromote(sA));

    sA->level = 14;
    sA->exp = 37;
    ApplyUnitPromotion(sA, CLASS_HERO);
    CHECK(sA->pClassData->number == CLASS_HERO && sA->level == 14 && sA->exp == 37);
    CHECK(!Col_CanPromote(sA));                                 /* promoted */
    sA->pClassData = GetClassData(CLASS_MERCENARY);
    sA->level = 12;
    sA->exp = 5;
    ApplyUnitDefaultPromotion(sA);
    CHECK((UNIT_CATTRIBUTES(sA) & CA_PROMOTED) && sA->level == 12 && sA->exp == 5);
    return 0;
}

/* Recover (spec 54): costs gold, uses a charge, heals fully; not without need, gold or charges;
 * 3 charges again after the boss. */
int Test_Recover(struct Unit *sA, struct Unit *sT)
{
    int cost;

    CHECK(Solo(sA, 5));
    cost = Col_RecoverCost();
    CHECK(cost == 300);
    gColRun.floor = 3;
    CHECK(Col_RecoverCost() == 500);
    gColRun.floor = 1;
    gColRun.gold = 1000;
    CHECK(!Col_CanUseRecover());                                /* nobody hurt */
    sA->curHP = 2;
    CHECK(Col_CanUseRecover() && Col_Recover());
    CHECK(sA->curHP == sA->maxHP && gColRun.gold == 700 && gColRun.recoverCharges == 2);
    sA->curHP = 2;
    gColRun.gold = 299;
    CHECK(!Col_CanUseRecover() && !Col_Recover() && sA->curHP == 2);  /* not enough gold */
    gColRun.gold = 5000;
    CHECK(Col_Recover());
    sA->curHP = 2;
    CHECK(Col_Recover() && gColRun.recoverCharges == 0);
    sA->curHP = 2;
    CHECK(!Col_CanUseRecover() && !Col_Recover());              /* no charges left */
    gColRun.normalWins = COL_NORMAL_FIGHTS_FLOOR;               /* the boss is next */
    Col_OnVictory(COL_ENC_BOSS);
    CHECK(gColRun.recoverCharges == COL_RECOVER_PER_FLOOR && gColRun.floor == 2);
    return 0;
}
