/* Gold and the shop (spec 37-39).
 *
 *   Gold: every victory pays Col_BattleGold (normal 200 + 75 per floor above 1 + 0-100 random;
 *     Elite x2, Boss x4 - Phase 16 levers). The run's gold is mirrored into FE8's party gold
 *     so the vanilla screens show it.
 *   Stock: rolled once per battle (after each victory, and for the first battle), never
 *     rerolled within a battle's preparation. Fully random (developer, 2026-09-26): each of the
 *     8 entries picks a category by weight - Weapon, Skill, Relic, Consumable, Recruit (at most
 *     one, while anyone can still be recruited), Healing (at most one: 30/50/100%), Promotion
 *     (at most one) - so the mix and the order change every round. No duplicate weapon, skill
 *     or relic. Relics are rarity-weighted (Col_RelicRoll).
 *   Prices (spec 39): each purchase makes every later purchase 10% dearer, compounding over the
 *     run: price = base x 1.1^purchases, at most 5x base and 9999 gold. */
#include "coliseum.h"
#include "bmunit.h"
#include "bmitem.h"
#include "variables.h"
#include "rng.h"

#define PRICE_MAX           9999
#define PRICE_MAX_FACTOR    5

extern const struct ColSkillInfo gColSkillCatalog[];

/* ---- gold ---- */

int Col_BattleGold(int encounter)
{
    int gold = 200 + 75 * (gColRun.floor > 1 ? gColRun.floor - 1 : 0) + NextRN_N(101);
    if (encounter == COL_ENC_ELITE)
        gold *= 2;
    else if (encounter == COL_ENC_BOSS)
        gold *= 4;
    return gold;
}

void Col_SyncPartyGold(void)
{
    gPlaySt.partyGoldAmount = gColRun.gold;
}

/* ---- prices ---- */

int Col_ShopPrice(int basePrice)
{
    int price = basePrice, n;
    int cap = basePrice * PRICE_MAX_FACTOR;

    for (n = 0; n < gColRun.shopPurchases && price < cap; n++)
        price = (price * 11 + 5) / 10;          /* +10%, rounded */
    if (price > cap)
        price = cap;
    if (price > PRICE_MAX)
        price = PRICE_MAX;
    return price;
}

static int RankPrice(int item)
{
    int exp = GetItemRequiredExp(item);         /* E 1, D 31, C 71, B 121, A 181, S 251 */
    if (exp >= 181) return 2400;
    if (exp >= 121) return 1600;
    if (exp >= 71)  return 1100;
    if (exp >= 31)  return 700;
    return 400;
}

static const u16 kSkillPrice[COL_RARITY_COUNT + 1] = { 0, 400, 700, 1100, 1700, 2800 };
/* Common .. Mythic (Phase 16 levers) */
static const u16 kRelicPrice[COL_RELIC_RARITY_COUNT + 1] = { 0, 500, 900, 1400, 2200, 3200, 4500 };

/* ---- pools ---- */

struct ColShopItem { u8 item; u8 minFloor; };

static const struct ColShopItem kWeapons[] = {
    { 0x01, 1 }, { 0x14, 1 }, { 0x1F, 1 }, { 0x2D, 1 }, { 0x38, 1 }, { 0x3F, 1 }, { 0x45, 1 },
    { 0x03, 1 }, { 0x16, 1 }, { 0x20, 1 }, { 0x2E, 1 }, { 0x39, 1 }, { 0x40, 1 },
    { 0x0D, 2 }, { 0x1A, 2 }, { 0x24, 2 }, { 0x31, 2 }, { 0x3A, 2 }, { 0x41, 2 }, { 0x47, 2 },
    { 0x04, 3 }, { 0x17, 3 }, { 0x21, 3 }, { 0x2F, 3 }, { 0x3B, 3 }, { 0x42, 3 },
    { 0x0B, 4 }, { 0x19, 4 }, { 0x23, 4 }, { 0x32, 4 },
    { 0, 0 },
};
/* Hero Crest, Knight Crest, Orion's Bolt, Elysian Whip, Guiding Ring, Master Seal */
static const u8 kPromotions[] = { 0x64, 0x65, 0x66, 0x67, 0x68, 0x88, 0 };
static const u16 kPromotionPrice = 2500;
/* Vulnerary, Elixir, Pure Water; stat boosters: Angelic Robe, Energy Ring (Mag), Secret Book,
 * Speedwings, Goddess Icon, Dragonshield, Talisman */
static const u8 kConsumables[] = { 0x6C, 0x6C, 0x6D, 0x6E, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F, 0x60, 0x61, 0 };
static const u16 kConsumablePrice[] = { 150, 150, 600, 300, 1800, 1800, 1800, 1800, 1500, 1800, 1800 };
/* team healing offers: percent and base price */
static const u8 kHealPercent[] = { 30, 50, 100 };
static const u16 kHealPrice[] = { 300, 500, 900 };
#define RECRUIT_PRICE 1500

static int Count8(const u8 *list)
{
    int n = 0;
    while (list[n])
        n++;
    return n;
}

static void Add(int category, int value, int price)
{
    struct ColShopEntry *e;
    if (gColRun.shopCount >= COL_SHOP_SIZE)
        return;
    e = &gColRun.shop[gColRun.shopCount++];
    e->category = (u8)category;
    e->value = (u8)value;
    e->basePrice = (u16)price;
}

static int HasEntry(int category, int value)
{
    int i;
    for (i = 0; i < gColRun.shopCount; i++)
        if (gColRun.shop[i].category == category && gColRun.shop[i].value == value)
            return 1;
    return 0;
}

static void AddWeapon(void)
{
    int n = 0, pick, tries;
    const struct ColShopItem *w;

    for (w = kWeapons; w->item; w++)
        if (w->minFloor <= gColRun.floor)
            n++;
    for (tries = 0; tries < 10; tries++) {
        pick = NextRN_N(n);
        for (w = kWeapons; w->item; w++)
            if (w->minFloor <= gColRun.floor && pick-- == 0)
                break;
        if (!HasEntry(COL_SHOP_WEAPON, w->item)) {
            Add(COL_SHOP_WEAPON, w->item, RankPrice(w->item));
            return;
        }
    }
}

static void AddConsumable(void)
{
    int k = NextRN_N(Count8(kConsumables));
    Add(COL_SHOP_CONSUMABLE, kConsumables[k], kConsumablePrice[k]);
}

static void AddSkill(void)
{
    const struct ColSkillInfo *e;
    int n = 0, pick, tries;

    for (e = gColSkillCatalog; e->skill; e++)
        if (!(e->flags & COL_SKF_ENEMY_ONLY))
            n++;
    for (tries = 0; tries < 10 && n; tries++) {
        pick = NextRN_N(n);
        for (e = gColSkillCatalog; e->skill; e++)
            if (!(e->flags & COL_SKF_ENEMY_ONLY) && pick-- == 0)
                break;
        if (!HasEntry(COL_SHOP_SKILL, e->skill)) {
            Add(COL_SHOP_SKILL, e->skill, kSkillPrice[e->rarity]);
            return;
        }
    }
}

static void AddRelic(void)
{
    int tries;
    for (tries = 0; tries < 10; tries++) {
        int relic = Col_RelicRoll();
        if (relic && !HasEntry(COL_SHOP_RELIC, relic)) {
            Add(COL_SHOP_RELIC, relic, kRelicPrice[Col_RelicDef(relic)->rarity]);
            return;
        }
    }
}

static int HasCategory(int category)
{
    int i;
    for (i = 0; i < gColRun.shopCount; i++)
        if (gColRun.shop[i].category == category)
            return 1;
    return 0;
}

/* Category weights for each stock entry (Phase 16 levers). */
static const u8 kCategoryWeight[] = {
    [COL_SHOP_RECRUIT] = 10, [COL_SHOP_SKILL] = 18, [COL_SHOP_WEAPON] = 22, [COL_SHOP_RELIC] = 16,
    [COL_SHOP_HEAL] = 6, [COL_SHOP_PROMOTION] = 8, [COL_SHOP_CONSUMABLE] = 20,
};

/* A new stock (spec 38): 8 entries, each a random category. */
void Col_ShopGenerate(void)
{
    u8 recruits[COL_RECRUIT_CHOICES];
    int canRecruit = Col_RollRecruits(recruits), guard;

    gColRun.shopCount = 0;
    gColRun.shopSold = 0;
    for (guard = 0; gColRun.shopCount < COL_SHOP_SIZE && guard < 64; guard++) {
        int total = 0, pick, c, before = gColRun.shopCount;

        for (c = COL_SHOP_RECRUIT; c <= COL_SHOP_CONSUMABLE; c++)
            total += kCategoryWeight[c];
        pick = NextRN_N(total);
        for (c = COL_SHOP_RECRUIT; c < COL_SHOP_CONSUMABLE && pick >= kCategoryWeight[c]; c++)
            pick -= kCategoryWeight[c];
        switch (c) {
        case COL_SHOP_RECRUIT:                  /* at most one; a different candidate each round */
            if (canRecruit && !HasCategory(c))
                Add(c, recruits[NextRN_N(canRecruit)], RECRUIT_PRICE);
            break;
        case COL_SHOP_SKILL:      AddSkill(); break;
        case COL_SHOP_WEAPON:     AddWeapon(); break;
        case COL_SHOP_RELIC:      AddRelic(); break;
        case COL_SHOP_HEAL:
            if (!HasCategory(c)) {
                int k = NextRN_N(sizeof(kHealPercent));
                Add(c, kHealPercent[k], kHealPrice[k]);
            }
            break;
        case COL_SHOP_PROMOTION:
            if (!HasCategory(c))
                Add(c, kPromotions[NextRN_N(Count8(kPromotions))], kPromotionPrice);
            break;
        default:                  AddConsumable(); break;
        }
        if (gColRun.shopCount == before && guard >= 48)
            AddConsumable();                    /* never short of 8 entries */
    }
}

/* ---- buying ---- */

int Col_ShopEntryPrice(int index)
{
    return Col_ShopPrice(gColRun.shop[index].basePrice);
}

int Col_ShopCanAfford(int index)
{
    return index >= 0 && index < gColRun.shopCount && !(gColRun.shopSold & (1 << index))
        && gColRun.gold >= (u32)Col_ShopEntryPrice(index);
}

/* Pays for entry `index` (the caller has delivered it): 1 if paid. */
int Col_ShopPay(int index)
{
    if (!Col_ShopCanAfford(index))
        return 0;
    Col_SpendGold((u32)Col_ShopEntryPrice(index));
    gColRun.shopSold |= (u8)(1 << index);
    gColRun.shopPurchases++;
    Col_SyncPartyGold();
    return 1;
}

/* Healing (spec 38 category 5): every living roster unit regains `percent`% of max HP. Kept in
 * the run state too, since the next battle restores HP from it. */
void Col_HealTeam(int percent)
{
    int slot;
    for (slot = 0; slot < COL_MAX_ROSTER; slot++) {
        struct Unit *u = Col_RosterUnit(slot);
        int hp;
        if (!u || (u->state & US_DEAD))
            continue;
        hp = u->curHP + (u->maxHP * percent + 99) / 100;
        if (hp > u->maxHP)
            hp = u->maxHP;
        u->curHP = (s8)hp;
        gColRun.hp[slot] = (u8)hp;
    }
}
