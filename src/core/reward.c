/* Roguelike progression (Phase 10, spec 26, 51-54, 76):
 *
 * 3-win reward (spec 51-53; developer: 3 kinds offered, pick 1): when a reward is due, 3
 * different kinds are rolled from Recruit / Skill / Promotion / Heal / Gold, only kinds that can
 * be used (someone left to recruit, a skill someone can learn, someone who can promote, someone
 * hurt; Gold always), so fewer than 3 are offered only when fewer are valid. The offer is rolled
 * once and saved in the run state (no reroll by resetting). Contents are rolled with it: the
 * Skill is a named skill, Gold is 100-500 (spec 52).
 *   Recruit: the recruitment offer (roster_ui.c). Skill: whoever can learn it learns it (learn /
 *   replace menu). Heal: every living roster member to full HP. Gold: added at once.
 *   Promotion: the chosen unit receives a promotion item it can use (Master Seal, or its own
 *   seal / brace); using it from Items runs FE8's promotion with its branching class choice.
 *
 * Promotion (developer): level 10+ (FE8's rule for promotion items), and the unit keeps its level
 * and EXP (Units.event removes FE8's reset to level 1).
 *
 * Recover (spec 54): 3 charges per floor (reset after the boss, run_state.c), costs gold, fully
 * heals every living roster member, not a fight. Cost 300 + 100 per floor above 1 (Phase 16).
 *
 * Autosave (spec 76): FE8's save menu between battles is replaced by a save to the run's slot. */
#include "colosseum.h"
#include "bmunit.h"
#include "bmitem.h"
#include "bmitemuse.h"
#include "bmsave.h"
#include "rng.h"

static const u8 kPromotionItems[] = { COL_MASTER_SEAL, 0x97, 0x98, 0x99, 0 };  /* Master/Ocean Seal, Lunar/Solar Brace */

int Col_RecruitsLeft(void)
{
    int pool, n = 0;
    for (pool = 0; pool < COL_POOL_SIZE; pool++)
        if (!(gColRun.recruitedMask & (1u << pool)))
            n++;
    return n;
}

static struct Unit *LivingRosterUnit(int slot)
{
    struct Unit *u;

    if (gColRun.roster[slot] >= COL_POOL_SIZE)
        return NULL;
    u = Col_RosterUnit(slot);
    return u && !(u->state & US_DEAD) && u->curHP > 0 ? u : NULL;
}

static int PromotionItemFor(struct Unit *unit)
{
    int i;

    if (!unit || unit->level < COL_PROMOTION_LEVEL || (UNIT_CATTRIBUTES(unit) & CA_PROMOTED) ||
        !unit->pClassData->promotion)
        return 0;
    for (i = 0; kPromotionItems[i]; i++)
        if (CanUnitUseItem(unit, MakeNewItem(kPromotionItems[i])))
            return kPromotionItems[i];
    return 0;
}

int Col_CanPromote(struct Unit *unit)
{
    return PromotionItemFor(unit) && GetUnitItemCount(unit) < UNIT_ITEM_COUNT;
}

int Col_CanLearnRewardSkill(struct Unit *unit)
{
    return unit && gColRun.rewardSkill && Col_CanLearnSkill(unit, gColRun.rewardSkill);
}

int Col_TeamHurt(void)
{
    int slot;
    for (slot = 0; slot < COL_MAX_ROSTER; slot++) {
        struct Unit *u = LivingRosterUnit(slot);
        if (u && u->curHP < u->maxHP)
            return 1;
    }
    return 0;
}

/* A skill some living roster member can learn: the first found, trying members in random order. */
static int RollRewardSkill(void)
{
    int start = NextRN_N(COL_MAX_ROSTER), k;

    for (k = 0; k < COL_MAX_ROSTER; k++) {
        struct Unit *u = LivingRosterUnit((start + k) % COL_MAX_ROSTER);
        u8 skill;
        if (u && Col_RollSkillOffers(u, &skill, 1))
            return skill;
    }
    return 0;
}

int Col_RewardValid(int kind)
{
    int slot;

    switch (kind) {
    case COL_REWARD_RECRUIT:
        return Col_RecruitsLeft() > 0;
    case COL_REWARD_SKILL:
        return 1;                               /* checked when the skill is rolled */
    case COL_REWARD_PROMOTION:
        for (slot = 0; slot < COL_MAX_ROSTER; slot++)
            if (Col_CanPromote(LivingRosterUnit(slot)))
                return 1;
        return 0;
    case COL_REWARD_HEAL:
        return Col_TeamHurt();
    case COL_REWARD_GOLD:
        return 1;
    }
    return 0;
}

void Col_RollReward(void)
{
    u8 kinds[COL_REWARD_KIND_COUNT];
    int n = 0, k, out = 0;

    if (!gColRun.rewardDue || gColRun.rewardKinds[0])
        return;
    gColRun.rewardSkill = (u8)RollRewardSkill();
    for (k = COL_REWARD_RECRUIT; k < COL_REWARD_KIND_COUNT; k++)
        if (Col_RewardValid(k) && (k != COL_REWARD_SKILL || gColRun.rewardSkill))
            kinds[n++] = (u8)k;
    while (out < 3 && n > 0) {                  /* 3 different kinds, random order */
        int pick = NextRN_N(n);
        gColRun.rewardKinds[out++] = kinds[pick];
        kinds[pick] = kinds[--n];
    }
    for (; out < 3; out++)
        gColRun.rewardKinds[out] = COL_REWARD_NONE;
    gColRun.rewardGold = (u16)(100 + NextRN_N(401));   /* spec 52: 100-500 */
}

int Col_RewardCount(void)
{
    int i, n = 0;
    for (i = 0; i < 3; i++)
        if (gColRun.rewardKinds[i] != COL_REWARD_NONE)
            n++;
    return n;
}

void Col_TakeGoldReward(void)
{
    Col_AddGold(gColRun.rewardGold);
    Col_SyncPartyGold();
    Col_TakeReward();
}

void Col_TakeHealReward(void)
{
    Col_HealTeam(100);
    Col_TakeReward();
}

int Col_GivePromotionSeal(struct Unit *unit)
{
    int item = PromotionItemFor(unit);

    if (!item || GetUnitItemCount(unit) >= UNIT_ITEM_COUNT || !UnitAddItem(unit, MakeNewItem(item)))
        return 0;
    Col_TakeReward();
    return 1;
}

/* ---- Recover ---- */

int Col_RecoverCost(void)
{
    return 300 + 100 * (gColRun.floor > 1 ? gColRun.floor - 1 : 0);
}

int Col_CanUseRecover(void)
{
    return Col_CanRecover((u32)Col_RecoverCost()) && Col_TeamHurt();
}

int Col_Recover(void)
{
    if (!Col_TeamHurt() || !Col_UseRecover((u32)Col_RecoverCost()))
        return 0;
    Col_HealTeam(100);
    Col_SyncPartyGold();
    return 1;
}

/* ---- autosave ---- */

/* Game control (SkipWorldMap.event), after the chapter switch where FE8 would open its save
 * menu: the active run is saved to its slot. */
void Col_AutoSave(void)
{
    if (Col_RunIsValid() && gColRun.active)
        WriteGameSave(gPlaySt.gameSaveSlot);
}
