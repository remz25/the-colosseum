/* On-target tests for Phase 8 (spec 37-39): battle gold, shop stock, price scaling, buying,
 * healing. Map tests (ColTest_MapRun restores gColRun and the two units). */
#include "coliseum.h"
#include "bmunit.h"
#include "bmitem.h"
#include "variables.h"

#define CHECK(cond) do { if (!(cond)) return __LINE__; } while (0)

extern const struct ColSkillInfo gColSkillCatalog[];

/* spec 39: +10% per purchase, compounding, at most 5x base and 9999. */
int Test_ShopPrices(struct Unit *a, struct Unit *t)
{
    gColRun.shopPurchases = 0;
    CHECK(Col_ShopPrice(1000) == 1000);
    gColRun.shopPurchases = 1;
    CHECK(Col_ShopPrice(1000) == 1100);
    gColRun.shopPurchases = 2;
    CHECK(Col_ShopPrice(1000) == 1210);
    gColRun.shopPurchases = 5;
    CHECK(Col_ShopPrice(1000) == 1610);                     /* rounded each step: 1100 1210 1331 1464 1610 */
    gColRun.shopPurchases = 40;
    CHECK(Col_ShopPrice(1000) == 5000);                     /* capped at 5x */
    CHECK(Col_ShopPrice(2800) == 9999);                     /* capped at 9999 */
    gColRun.shopPurchases = 3;
    CHECK(Col_ShopPrice(150) > Col_ShopPrice(149));
    return 0;
}

/* spec 38: 8 entries, every relevant category, no duplicate weapon/skill, floor-appropriate. */
int Test_ShopStock(struct Unit *a, struct Unit *t)
{
    int round, i, j;

    for (round = 0; round < 20; round++) {
        int seen = 0;
        Col_RunNew(round);
        gColRun.floor = 1;
        Col_ShopGenerate();
        CHECK(gColRun.shopCount == COL_SHOP_SIZE && gColRun.shopSold == 0);
        for (i = 0; i < gColRun.shopCount; i++) {
            const struct ColShopEntry *e = &gColRun.shop[i];
            seen |= 1 << e->category;
            CHECK(e->basePrice > 0);
            switch (e->category) {
            case COL_SHOP_RECRUIT:
                CHECK(e->value < COL_POOL_SIZE && !(gColRun.recruitedMask & (1 << e->value)));
                break;
            case COL_SHOP_SKILL:
                CHECK(Col_SkillInfo(e->value) && !(Col_SkillInfo(e->value)->flags & COL_SKF_ENEMY_ONLY));
                break;
            case COL_SHOP_WEAPON:
                CHECK(GetItemAttributes(e->value) & IA_WEAPON);
                CHECK(GetItemRequiredExp(e->value) < 121);   /* floor 1: no B/A/S */
                break;
            case COL_SHOP_HEAL:
                CHECK(e->value == 50);
                break;
            case COL_SHOP_PROMOTION:
            case COL_SHOP_CONSUMABLE:
                CHECK(GetItemName(e->value)[0]);
                break;
            default:
                return __LINE__;
            }
            for (j = 0; j < i; j++)
                if (e->category == COL_SHOP_WEAPON || e->category == COL_SHOP_SKILL)
                    CHECK(!(gColRun.shop[j].category == e->category && gColRun.shop[j].value == e->value));
        }
        CHECK(seen == ((1 << COL_SHOP_RECRUIT) | (1 << COL_SHOP_SKILL) | (1 << COL_SHOP_WEAPON)
                       | (1 << COL_SHOP_HEAL) | (1 << COL_SHOP_PROMOTION) | (1 << COL_SHOP_CONSUMABLE)));
    }
    /* nobody left to recruit: no Recruit entry, still a full stock */
    gColRun.recruitedMask = 0x7FFF;
    Col_ShopGenerate();
    CHECK(gColRun.shopCount == COL_SHOP_SIZE);
    for (i = 0; i < gColRun.shopCount; i++)
        CHECK(gColRun.shop[i].category != COL_SHOP_RECRUIT);
    /* later floors sell better weapons */
    {
        int best = 0, r;
        gColRun.floor = 4;
        for (r = 0; r < 30; r++) {
            Col_ShopGenerate();
            for (i = 0; i < gColRun.shopCount; i++)
                if (gColRun.shop[i].category == COL_SHOP_WEAPON && GetItemRequiredExp(gColRun.shop[i].value) > best)
                    best = GetItemRequiredExp(gColRun.shop[i].value);
        }
        CHECK(best >= 121);
    }
    return 0;
}

/* Paying: only when affordable, once per entry; later prices rise; party gold follows. */
int Test_ShopBuy(struct Unit *a, struct Unit *t)
{
    u32 partyGold = gPlaySt.partyGoldAmount;
    int price, result = 0;

    Col_RunNew(3);
    Col_ShopGenerate();
    price = Col_ShopEntryPrice(0);
    if (Col_ShopCanAfford(0) || Col_ShopPay(0))
        result = __LINE__;                                   /* no gold */
    else {
        Col_AddGold(price + 5000);
        if (!Col_ShopPay(0) || gColRun.gold != 5000u)
            result = __LINE__;
        else if (!(gColRun.shopSold & 1) || gColRun.shopPurchases != 1 || Col_ShopPay(0))
            result = __LINE__;
        else if (Col_ShopEntryPrice(1) != Col_ShopPrice(gColRun.shop[1].basePrice)
                 || Col_ShopEntryPrice(1) <= gColRun.shop[1].basePrice)
            result = __LINE__;                               /* +10% after a purchase */
        else if (gPlaySt.partyGoldAmount != gColRun.gold)
            result = __LINE__;
    }
    gPlaySt.partyGoldAmount = partyGold;
    return result;
}

/* Healing: +50% of max HP to each living roster unit, kept in the run state for the battle. */
int Test_HealTeam(struct Unit *a, struct Unit *t)
{
    int slot, mine = -1, s;

    for (s = 0; s < COL_MAX_ROSTER; s++)
        if (Col_RosterUnit(s) == a)
            mine = s;
    CHECK(mine >= 0);
    for (s = 0; s < COL_MAX_ROSTER; s++)
        if (s != mine)
            gColRun.roster[s] = 0xFF;                        /* only the actor */
    slot = mine;
    a->maxHP = 40;
    a->curHP = 10;
    Col_HealTeam(50);
    CHECK(a->curHP == 30 && gColRun.hp[slot] == 30);
    Col_HealTeam(50);
    CHECK(a->curHP == 40 && gColRun.hp[slot] == 40);         /* not above max */
    return 0;
}

/* spec 37: battles award gold, more on later floors, Elites x2, Bosses x4. */
int Test_BattleGold(struct Unit *a, struct Unit *t)
{
    int i;
    for (i = 0; i < 50; i++) {
        int g;
        gColRun.floor = 1;
        g = Col_BattleGold(COL_ENC_NORMAL);
        CHECK(g >= 200 && g <= 300);
        g = Col_BattleGold(COL_ENC_ELITE);
        CHECK(g >= 400 && g <= 600);
        g = Col_BattleGold(COL_ENC_BOSS);
        CHECK(g >= 800 && g <= 1200);
        gColRun.floor = 3;
        g = Col_BattleGold(COL_ENC_NORMAL);
        CHECK(g >= 350 && g <= 450);
    }
    return 0;
}
