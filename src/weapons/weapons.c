/* Weapons (spec 40-44): no durability (ItemTable.csv: Indestructible), fast proficiency (weapon
 * EXP x3 in ItemTable.csv), transfer between compatible characters, fusion, variants.
 *
 *   Fusion (spec 42): two weapons in one unit's inventory -> a better one, only when the player
 *     chooses (Fuse menu); both are consumed; the result takes the first one's slot.
 *     Recipes: two of the same weapon step up its line (Iron -> Steel -> Silver, ...), and special
 *     pairs such as Killing Edge + Keen Edge (an Elite variant, spec 43) -> Lethal Edge.
 *   Transfer (spec 40): an item moves to another roster member who uses its weapon type (other
 *     items go to anyone) and has room (5 items). */
#include "colosseum.h"
#include "bmunit.h"
#include "bmitem.h"

struct ColFusion { u8 a, b, result; };

/* item IDs: vanilla FE8 + COLOSSEUM's (0xC0-0xC9, ItemTable.csv) */
static const struct ColFusion kFusions[] = {
    { 0x01, 0x01, 0x03 }, { 0x03, 0x03, 0x04 },                        /* Iron/Steel/Silver Sword */
    { 0x05, 0x05, 0x06 }, { 0x06, 0x06, 0x07 },                        /* Iron/Steel/Silver Blade */
    { 0x14, 0x14, 0x16 }, { 0x16, 0x16, 0x17 },                        /* Iron/Steel/Silver Lance */
    { 0x1F, 0x1F, 0x20 }, { 0x20, 0x20, 0x21 },                        /* Iron/Steel/Silver Axe */
    { 0x2D, 0x2D, 0x2E }, { 0x2E, 0x2E, 0x2F },                        /* Iron/Steel/Silver Bow */
    { 0x38, 0x38, 0x39 }, { 0x39, 0x39, 0x3A }, { 0x3A, 0x3A, 0x3B }, { 0x3B, 0x3B, 0x3C },
                                                  /* Fire, Thunder, Elfire, Bolting, Fimbulvetr */
    { 0x3F, 0x3F, 0x40 }, { 0x40, 0x40, 0x41 }, { 0x41, 0x41, 0x42 }, { 0x42, 0x42, 0x43 },
                                                  /* Lightning, Shine, Divine, Purge, Aura */
    { 0x45, 0x45, 0x47 },                                              /* Flux -> Nosferatu */
    { 0x0D, 0xC0, 0xC4 },                          /* Killing Edge + Keen Edge -> Lethal Edge */
    { 0, 0, 0 },
};

/* The fusion result of two items (either order), or 0. */
int Col_FusionResult(int itemA, int itemB)
{
    const struct ColFusion *f;
    int a = itemA & 0xFF, b = itemB & 0xFF;

    for (f = kFusions; f->a; f++)
        if ((f->a == a && f->b == b) || (f->a == b && f->b == a))
            return f->result;
    return 0;
}

/* Fuse inventory slots s1 and s2 of `unit`: the result replaces s1, s2 is removed. */
int Col_Fuse(struct Unit *unit, int s1, int s2)
{
    int result;

    if (s1 == s2 || s1 < 0 || s2 < 0 || s1 >= UNIT_ITEM_COUNT || s2 >= UNIT_ITEM_COUNT)
        return 0;
    if (!unit->items[s1] || !unit->items[s2])
        return 0;
    result = Col_FusionResult(unit->items[s1], unit->items[s2]);
    if (!result)
        return 0;
    unit->items[s1] = (u16)MakeNewItem(result);
    UnitRemoveItem(unit, s2);                   /* shifts later items up */
    return 1;
}

/* First fusible pair in the unit's inventory at or after pair index `from` (pairs are ordered
 * (0,1), (0,2) ... (3,4)); returns the pair index or -1, with the slots in *s1, *s2. */
int Col_FindFusion(struct Unit *unit, int from, int *s1, int *s2)
{
    int i, j, n = 0;
    for (i = 0; i < UNIT_ITEM_COUNT; i++)
        for (j = i + 1; j < UNIT_ITEM_COUNT; j++, n++)
            if (n >= from && unit->items[i] && unit->items[j]
                && Col_FusionResult(unit->items[i], unit->items[j])) {
                *s1 = i;
                *s2 = j;
                return n;
            }
    return -1;
}

/* Can `to` receive `item`? Compatible characters (spec 40): weapons and staves go to units that
 * use that weapon type (any rank: they may grow into it); other items go to anyone. */
int Col_CanReceiveItem(struct Unit *to, int item)
{
    int attr = GetItemAttributes(item);

    if (!to || GetUnitItemCount(to) >= UNIT_ITEM_COUNT)
        return 0;
    if (attr & (IA_WEAPON | IA_STAFF))
        return to->ranks[GetItemType(item)] > 0;
    return 1;
}

int Col_TransferItem(struct Unit *from, int slot, struct Unit *to)
{
    int item;

    if (!from || !to || from == to || slot < 0 || slot >= UNIT_ITEM_COUNT || !from->items[slot])
        return 0;
    item = from->items[slot];
    if (!Col_CanReceiveItem(to, item))
        return 0;
    UnitAddItem(to, item);
    UnitRemoveItem(from, slot);
    return 1;
}
