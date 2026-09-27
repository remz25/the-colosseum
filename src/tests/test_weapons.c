/* On-target tests for Phase 7 (spec 40-44): fusion recipes and fusing, transfer between
 * compatible characters, fast proficiency, the Elite variant and boss weapons.
 * Map tests (ColTest_MapRun restores the two units they change). */
#include "colosseum.h"
#include "bmunit.h"
#include "bmitem.h"

#define CHECK(cond) do { if (!(cond)) return __LINE__; } while (0)

#define IRON_SWORD   0x01
#define STEEL_SWORD  0x03
#define SILVER_SWORD 0x04
#define KILLING_EDGE 0x0D
#define IRON_LANCE   0x14
#define IRON_AXE     0x1F
#define HEAL         0x4B
#define VULNERARY    0x6C
#define KEEN_EDGE    0xC0
#define LETHAL_EDGE  0xC4
#define FIRST_NEW    0xC0
#define LAST_NEW     0xC9
#define FIRST_BOSS   0xC5

static void Inventory(struct Unit *u, int a, int b, int c, int d, int e)
{
    u->items[0] = a ? (u16)MakeNewItem(a) : 0;
    u->items[1] = b ? (u16)MakeNewItem(b) : 0;
    u->items[2] = c ? (u16)MakeNewItem(c) : 0;
    u->items[3] = d ? (u16)MakeNewItem(d) : 0;
    u->items[4] = e ? (u16)MakeNewItem(e) : 0;
}

/* Every recipe gives a weapon of the same type, never weaker in rank, either order. */
int Test_FusionRecipes(struct Unit *x, struct Unit *y)
{
    int a, b, n = 0;

    for (a = 1; a < 0xCA; a++)
        for (b = a; b < 0xCA; b++) {
            int r = Col_FusionResult(a, b);
            if (!r)
                continue;
            n++;
            CHECK(r != a && r == Col_FusionResult(b, a));
            CHECK(GetItemAttributes(r) & IA_WEAPON);
            CHECK(GetItemType(r) == GetItemType(a));
            CHECK(GetItemRequiredExp(r) >= GetItemRequiredExp(a));
        }
    CHECK(n >= 20);
    CHECK(Col_FusionResult(IRON_SWORD, IRON_SWORD) == STEEL_SWORD);
    CHECK(Col_FusionResult(STEEL_SWORD, STEEL_SWORD) == SILVER_SWORD);
    CHECK(Col_FusionResult(KEEN_EDGE, KILLING_EDGE) == LETHAL_EDGE);
    CHECK(Col_FusionResult(IRON_SWORD, IRON_LANCE) == 0);
    CHECK(Col_FusionResult(VULNERARY, VULNERARY) == 0);
    return 0;
}

/* Fusing consumes both; the result takes the first slot; nothing happens without a recipe. */
int Test_Fuse(struct Unit *a, struct Unit *t)
{
    int s1, s2;

    Inventory(a, IRON_SWORD, VULNERARY, IRON_SWORD, 0, 0);
    CHECK(Col_FindFusion(a, 0, &s1, &s2) >= 0 && s1 == 0 && s2 == 2);
    CHECK(!Col_Fuse(a, 0, 1));                               /* sword + vulnerary: no */
    CHECK((a->items[0] & 0xFF) == IRON_SWORD && GetUnitItemCount(a) == 3);
    CHECK(Col_Fuse(a, 0, 2));
    CHECK((a->items[0] & 0xFF) == STEEL_SWORD && (a->items[1] & 0xFF) == VULNERARY);
    CHECK(GetUnitItemCount(a) == 2);
    CHECK(Col_FindFusion(a, 0, &s1, &s2) < 0);               /* never automatic */

    Inventory(a, KILLING_EDGE, KEEN_EDGE, 0, 0, 0);
    CHECK(Col_Fuse(a, 1, 0) && (a->items[1 - 1] & 0xFF) != 0);
    CHECK((a->items[0] & 0xFF) == LETHAL_EDGE && GetUnitItemCount(a) == 1);
    return 0;
}

/* Transfer: weapons to units using that weapon type (any rank), others to anyone, room needed. */
int Test_Transfer(struct Unit *a, struct Unit *t)
{
    int i;

    for (i = 0; i < 8; i++)
        a->ranks[i] = t->ranks[i] = 0;
    a->ranks[0] = 1;                                          /* a: swords E */
    t->ranks[0] = 1;                                          /* t: swords E (Silver needs A) */
    Inventory(a, SILVER_SWORD, IRON_AXE, VULNERARY, HEAL, 0);
    Inventory(t, IRON_SWORD, 0, 0, 0, 0);

    CHECK(Col_CanReceiveItem(t, a->items[0]));                /* grows into it */
    CHECK(!Col_CanReceiveItem(t, a->items[1]));               /* no axes */
    CHECK(!Col_CanReceiveItem(t, a->items[3]));               /* no staves */
    CHECK(Col_CanReceiveItem(t, a->items[2]));                /* items: anyone */
    CHECK(!Col_TransferItem(a, 1, t));
    CHECK(Col_TransferItem(a, 0, t));
    CHECK((t->items[1] & 0xFF) == SILVER_SWORD && (a->items[0] & 0xFF) == IRON_AXE);
    CHECK(GetUnitItemCount(a) == 3 && GetUnitItemCount(t) == 2);
    CHECK(!Col_TransferItem(a, 0, a));                        /* not to oneself */

    Inventory(t, IRON_SWORD, IRON_SWORD, IRON_SWORD, IRON_SWORD, IRON_SWORD);
    CHECK(!Col_CanReceiveItem(t, MakeNewItem(VULNERARY)));    /* full */
    return 0;
}

/* spec 41: weapon EXP tripled: every weapon and staff that gives weapon EXP gives at least 3
 * per use (monster weapons and a few boss-only ones give none, as in FE8). */
int Test_FastProficiency(struct Unit *x, struct Unit *y)
{
    int id, n = 0;
    for (id = 1; id < 0xCA; id++) {
        int attr = GetItemAttributes(id), wexp = GetItemAwardedExp(id);
        if (!(attr & (IA_WEAPON | IA_STAFF)) || !wexp)
            continue;
        CHECK(wexp >= 3 && wexp % 3 == 0);
        n++;
    }
    CHECK(n >= 100);
    CHECK(GetItemAwardedExp(0x01) == 3);                     /* Iron Sword: was 1 */
    return 0;
}

/* spec 43-44: the Elite variants, the unique fusion and the boss weapons exist and behave. */
int Test_SpecialWeapons(struct Unit *x, struct Unit *y)
{
    int id;
    for (id = FIRST_NEW; id <= LAST_NEW; id++) {
        int attr = GetItemAttributes(id);
        CHECK(attr & IA_WEAPON);
        CHECK(attr & IA_UNBREAKABLE);
        CHECK(GetItemName(id)[0] != 0);
        CHECK(GetItemMight(id) > 0 && GetItemHit(id) > 0);
        if (id >= FIRST_BOSS)
            CHECK(attr & IA_UNSELLABLE);
    }
    CHECK(GetItemCrit(KEEN_EDGE) > GetItemCrit(KILLING_EDGE));
    CHECK(GetItemCrit(LETHAL_EDGE) > GetItemCrit(KEEN_EDGE));
    for (id = FIRST_BOSS; id <= LAST_NEW; id++)
        CHECK(GetItemMight(id) > GetItemMight(SILVER_SWORD));
    return 0;
}
