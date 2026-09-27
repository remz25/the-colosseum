/* The relic pool (Phase 9 prototype, 15 relics; docs/RELICS.md).
 *
 * A relic is a name, a rarity and up to COL_RELIC_MODS modifiers. Every effect is one of the
 * generic modifier kinds in colosseum.h (flat or percent stats, battle rates, damage percentages,
 * gold, adjacent-ally auras, HP costs), each with an optional condition. Adding a relic that
 * only uses existing kinds is one line here; a new kind of effect is a new enum value plus the
 * one hook that applies it (relics.c). IDs are indices into this table and are saved in the run
 * state, so never reorder or remove entries: append new relics at the end. */
#include "colosseum.h"

#define M(kind, amount)        { COL_RM_##kind, amount, COL_RC_ALWAYS, 0 }
#define MC(kind, amount, cond) { COL_RM_##kind, amount, COL_RC_##cond, 0 }

enum { COMMON = 1, UNCOMMON, RARE, EPIC, LEGENDARY, MYTHIC };

const struct ColRelicDef gColRelics[] = {
    /* 0 */ { "", 0, {0}, { {0} } },

    /* ---- Common: plain stat trade-offs ---- */
    /* 1 */ { "Iron Heart",      COMMON,   {0}, { M(DEF, 5), M(SPD, -3) } },
    /* 2 */ { "Warrior's Band",  COMMON,   {0}, { M(STR, 5), M(RES, -3) } },
    /* 3 */ { "Swift Feather",   COMMON,   {0}, { M(SPD, 5), M(DEF, -3) } },
    /* 4 */ { "Scholar's Lens",  COMMON,   {0}, { M(MAG, 5), M(STR, -3) } },
    /* 5 */ { "Eagle Eye",       COMMON,   {0}, { M(HIT, 10), M(AVOID, -5) } },
    /* 6 */ { "Sturdy Boots",    COMMON,   {0}, { M(MOV, 1), M(DEF, -5) } },

    /* ---- Uncommon: conditions, auras, first percentages ---- */
    /* 7 */ { "Bloodied Band",   UNCOMMON, {0}, { MC(STR, 5, BELOW_HALF_HP), MC(CRIT, 5, BELOW_HALF_HP) } },
    /* 8 */ { "Guardian's Crest", UNCOMMON, {0}, { M(ADJ_ALLY_DEF, 3), M(SPD, -2) } },
    /* 9 */ { "Mage's Ring",     UNCOMMON, {0}, { M(MAGIC_DMG_PCT, 15), M(DEF, -5) } },
    /* 10 */ { "Golden Thread",  UNCOMMON, {0}, { M(GOLD_PCT, 25), M(LCK, -5) } },
    /* 11 */ { "Berserker's Fang", UNCOMMON, {0}, { M(STR, 5), M(CRIT, 5), M(DEF, -5) } },

    /* ---- Rare: percentage scaling ---- */
    /* 12 */ { "Blood Pact",     RARE,     {0}, { M(STR_PCT, 20), M(SPD_PCT, 10), M(HP_PER_ATTACK, 2) } },
    /* 13 */ { "Wind Soul",      RARE,     {0}, { M(SPD_PCT, 20), M(DEF_PCT, -15) } },
    /* 14 */ { "Arcane Blood",   RARE,     {0}, { M(MAG_PCT, 20), M(DEF_PCT, -15), M(MAGIC_DMG_PCT, 10) } },

    /* ---- Epic ---- */
    /* 15 */ { "Fortress Heart", EPIC,     {0}, { M(DMG_TAKEN_PCT, -20), M(SPD, -5), M(MOV, -1) } },
};

int Col_RelicCount(void)
{
    return sizeof(gColRelics) / sizeof(gColRelics[0]) - 1;
}
