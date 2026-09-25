/* Skills (spec 27-32): 1 personal skill + 3 slots, rarity, prerequisites, no duplicates.
 *
 *   Personal skill: PersonalSkillTable (fixed, never replaced).
 *   Slots: the 3 first learned-skill bytes of the character's BWL entry (Skills.event limits the
 *     Skill System's adder to 3). Class skills are learned into a slot at level 1 (skill_lists).
 *   Catalog (Skills.event): every skill a player can be offered, with rarity and prerequisites;
 *     enemy-only skills are marked and never offered to players.
 *   A new run clears every pool character's slots (Col_StartRun). */
#include "coliseum.h"
#include "bmunit.h"
#include "functions.h"
#include "rng.h"

#define BWL_TABLE       0x0203E884      /* Skill System: BWL entry = 16 bytes per character */
#define BWL_CHAR_MAX    0x45            /* characters above this have no BWL entry */

extern const u8 PersonalSkillTable[];
extern const u16 SkillDescTable[];
extern const struct ColSkillInfo gColSkillCatalog[];
extern void InitSkillBuffers(void);             /* Skill System: forget cached skill lists */

static const u8 kRarityWeight[COL_RARITY_COUNT + 1] = { 0, 40, 30, 18, 9, 3 };

static u8 *Slots(struct Unit *unit)
{
    int pid = unit->pCharacterData->number;
    if (pid == 0 || pid > BWL_CHAR_MAX)
        return NULL;
    return (u8 *)(BWL_TABLE + pid * 0x10 + 1);
}

static int ValidSkill(int skill)
{
    return skill != 0 && skill != 0xFF;
}

int Col_PersonalSkill(struct Unit *unit)
{
    return PersonalSkillTable[unit->pCharacterData->number];
}

int Col_SlotSkill(struct Unit *unit, int slot)
{
    u8 *s = Slots(unit);
    if (!s || slot < 0 || slot >= COL_SKILL_SLOTS || !ValidSkill(s[slot]))
        return 0;
    return s[slot];
}

int Col_UnitHasSkill(struct Unit *unit, int skill)
{
    int i;
    if (!ValidSkill(skill))
        return 0;
    if (Col_PersonalSkill(unit) == skill)
        return 1;
    for (i = 0; i < COL_SKILL_SLOTS; i++)
        if (Col_SlotSkill(unit, i) == skill)
            return 1;
    return 0;
}

int Col_FreeSkillSlot(struct Unit *unit)
{
    int i;
    if (!Slots(unit))
        return -1;
    for (i = 0; i < COL_SKILL_SLOTS; i++)
        if (!Col_SlotSkill(unit, i))
            return i;
    return -1;
}

const struct ColSkillInfo *Col_SkillInfo(int skill)
{
    const struct ColSkillInfo *e;
    for (e = gColSkillCatalog; e->skill; e++)
        if (e->skill == skill)
            return e;
    return NULL;
}

/* spec 30-31: offered to players, not already known, prerequisites met */
int Col_CanLearnSkill(struct Unit *unit, int skill)
{
    const struct ColSkillInfo *e = Col_SkillInfo(skill);
    u32 attr;
    int t;

    if (!e || (e->flags & COL_SKF_ENEMY_ONLY) || !Slots(unit) || Col_UnitHasSkill(unit, skill))
        return 0;
    attr = unit->pCharacterData->attributes | unit->pClassData->attributes;
    if ((e->flags & COL_SKF_PROMOTED) && !(attr & CA_PROMOTED))
        return 0;
    if ((e->flags & COL_SKF_MOUNTED) && !(attr & (CA_MOUNTED | CA_FLYER)))
        return 0;
    if (e->weapons) {
        int ok = 0;
        for (t = 0; t < 8; t++)
            if ((e->weapons & (1 << t)) && unit->ranks[t])
                ok = 1;
        if (!ok)
            return 0;
    }
    if (e->stat && Col_GetStat(unit, e->stat - 1) < e->statMin)
        return 0;
    return 1;
}

/* Learn into slot `slot` (replacing what is there), or the first free slot when slot < 0.
 * 1 if learned; 0 if not allowed or no free slot (then the player picks one to replace). */
int Col_LearnSkill(struct Unit *unit, int skill, int slot)
{
    u8 *s = Slots(unit);

    if (!s || !Col_CanLearnSkill(unit, skill))
        return 0;
    if (slot < 0)
        slot = Col_FreeSkillSlot(unit);
    if (slot < 0 || slot >= COL_SKILL_SLOTS)
        return 0;
    s[slot] = (u8)skill;
    InitSkillBuffers();                         /* the Skill System caches each unit's skills */
    return 1;
}

/* A new run: nobody keeps skills learned in an earlier run. */
void Col_ClearRunSkills(void)
{
    int pool, i;
    for (pool = 0; pool < COL_POOL_SIZE; pool++) {
        int pid = gColPool[pool].charId;
        if (pid && pid <= BWL_CHAR_MAX)
            for (i = 0; i < 4; i++)
                ((u8 *)(BWL_TABLE + pid * 0x10 + 1))[i] = 0;
    }
}

static int RollRarity(void)
{
    int total = 0, r, k;
    for (k = 1; k <= COL_RARITY_COUNT; k++)
        total += kRarityWeight[k];
    r = NextRN_N(total);
    for (k = 1; k <= COL_RARITY_COUNT; k++) {
        if (r < kRarityWeight[k])
            return k;
        r -= kRarityWeight[k];
    }
    return 1;
}

/* Up to n distinct skills the unit could learn, rarity-weighted (C 40 / U 30 / R 18 / E 9 /
 * L 3, a Phase 16 lever). Returns how many were found. */
int Col_RollSkillOffers(struct Unit *unit, u8 *out, int n)
{
    int found = 0, tries;

    for (tries = 0; tries < 200 && found < n; tries++) {
        int rarity = RollRarity(), count = 0, pick, k;
        const struct ColSkillInfo *e;

        for (e = gColSkillCatalog; e->skill; e++)
            if (e->rarity == rarity && Col_CanLearnSkill(unit, e->skill))
                count++;
        if (!count)
            continue;
        pick = NextRN_N(count);
        for (e = gColSkillCatalog; e->skill; e++)
            if (e->rarity == rarity && Col_CanLearnSkill(unit, e->skill) && pick-- == 0)
                break;
        for (k = 0; k < found; k++)
            if (out[k] == e->skill)
                break;
        if (k == found)
            out[found++] = e->skill;
    }
    return found;
}

/* The skill's name: its description text up to the ':' (as the Skill System's menus do). */
const char *Col_SkillName(int skill)
{
    char *desc, *it;
    if (!ValidSkill(skill) || !SkillDescTable[skill])
        return "";
    desc = GetStringFromIndex(SkillDescTable[skill]);
    for (it = desc; *it; ++it)
        if (*it == ':') {
            *it = 0;
            break;
        }
    return desc;
}
