/* Enemies for each battle: normal 3v3 (spec 46), Elite battles (spec 47-50), enemy drops.
 *
 * Elite battles (every 3 normal wins) come in two formats (spec 47):
 *   Champion    1 very strong enemy: promoted class, Elite level (+3), +12 HP and +2 to every
 *               other stat, its own skills, an Elite weapon it drops when defeated.
 *   Elite Squad 3 enemies built to work together (e.g. a Guardian that shields its allies'
 *               Def, a Mender that heals, a Reaper that finishes): promoted classes 2 levels
 *               below the normal level, role skills; iron weapons on floor 1, steel from floor 2
 *               (a floor-1 squad with steel weapons nearly one-shot Lv 6 units in testing).
 *   The run's first Elite is gentler: Champion -2 levels, +6 HP / +1 stats; squads -1 level.
 * The setup is rolled once when the Elite is next (Col_CurrentElite) so the announcement before
 * the battle and the enemies agree; it is saved in the run state. Skills come from each
 * role's generic character (Tables: PersonalSkillEditor / CharacterLevelUpSkillEditor, lists in
 * src/skills/Skills.event); their names ("Champion", "Guardian", ...) from CharacterTable.csv.
 * Advanced Elite AI (spec 49) is a later step: they use FE8's AI (charge, heal with staves).
 *
 * Drops (developer, 2026-09-26): some enemies carry gold, a skill or a relic, given when they
 * die. The first battle of every run always has one enemy carrying a relic; every Elite battle
 * has one too. Other enemies: 25% chance (Elite squads 50%): gold 50%, skill 25%, relic 25%.
 * A skill drop is chosen when claimed, among the skills the killer can learn, and offered to the
 * killer (learn / replace / decline). The killer is recorded in the battle proc loop
 * (Col_DropKillProc). Claiming: drops_ui.c. */
#include "colosseum.h"
#include "bmunit.h"
#include "bmitem.h"
#include "bmbattle.h"
#include "rng.h"

#define FACTION_RED_ALLEGIANCE   2
#define GENERIC_ENEMY_CHAR       0x80      /* vanilla generic "Soldier" character */


/* Normal enemies (mercenaries): class and weapon. Factions per floor come later. */
static const u8 kEnemyKinds[][2] = {
    { 0x3F, 0x1F },   /* Fighter    Iron Axe   */
    { 0x4E, 0x14 },   /* Soldier    Iron Lance */
    { 0x0F, 0x01 },   /* Mercenary  Iron Sword */
    { 0x19, 0x2D },   /* Archer     Iron Bow   */
    { 0x25, 0x38 },   /* Mage       Fire       */
};

/* Elite role characters (generic IDs; name and skills in the tables) */
#define CHAMP_BLADE  0x81
#define CHAMP_AXE    0x82
#define CHAMP_LANCE  0x83
#define CHAMP_BOW    0x84
#define GUARDIAN     0x85
#define MENDER       0x86
#define REAPER       0x87
#define STRIKER      0x88
/* classes */
#define PALADIN 0x07
#define GENERAL 0x0B
#define SWORDMASTER 0x15
#define ASSASSIN 0x17
#define SNIPER 0x1B
#define WYVERN_LORD 0x21
#define SAGE 0x27
#define BISHOP 0x2B
#define GREAT_KNIGHT 0x35
#define WARRIOR 0x40
#define BERSERKER 0x43
/* items */
#define STEEL_SWORD 0x03
#define KILLING_EDGE 0x0D
#define STEEL_LANCE 0x16
#define STEEL_AXE 0x20
#define HAND_AXE 0x28
#define STEEL_BOW 0x2E
#define ELFIRE 0x3A
#define LIGHTNING 0x3F
#define HEAL 0x4B
#define MEND 0x4C
#define KEEN_EDGE 0xC0
#define TITAN_AXE 0xC1
#define GALE_LANCE 0xC2
#define HAWK_BOW 0xC3

const struct ColEliteSetup gColEliteSetups[] = {
    { "Champion: Blademaster", 1, {0}, { { CHAMP_BLADE, SWORDMASTER, { KEEN_EDGE } } } },
    { "Champion: Warlord",     1, {0}, { { CHAMP_AXE, WARRIOR, { TITAN_AXE } } } },
    { "Champion: Lancer",      1, {0}, { { CHAMP_LANCE, GREAT_KNIGHT, { GALE_LANCE } } } },
    { "Champion: Deadeye",     1, {0}, { { CHAMP_BOW, SNIPER, { HAWK_BOW } } } },
    { "Elite Squad: Vanguard", 3, {0}, { { GUARDIAN, PALADIN, { STEEL_LANCE, STEEL_SWORD } },
                                         { MENDER, BISHOP, { LIGHTNING, MEND } },
                                         { REAPER, SWORDMASTER, { KILLING_EDGE } } } },
    { "Elite Squad: Siege",    3, {0}, { { GUARDIAN, GENERAL, { STEEL_LANCE } },
                                         { STRIKER, SAGE, { ELFIRE, HEAL } },
                                         { STRIKER, SNIPER, { STEEL_BOW } } } },
    { "Elite Squad: Raiders",  3, {0}, { { STRIKER, WYVERN_LORD, { STEEL_LANCE } },
                                         { REAPER, ASSASSIN, { STEEL_SWORD } },
                                         { STRIKER, BERSERKER, { STEEL_AXE, HAND_AXE } } } },
};

int Col_EliteSetupCount(void)
{
    return sizeof(gColEliteSetups) / sizeof(gColEliteSetups[0]);
}

const struct ColEliteSetup *Col_CurrentElite(void)
{
    if (Col_NextEncounter() != COL_ENC_ELITE)
        return NULL;
    if (!gColRun.elite || gColRun.elite > Col_EliteSetupCount())
        gColRun.elite = (u8)(1 + NextRN_N(Col_EliteSetupCount()));   /* 4 Champions, 3 Squads */
    return &gColEliteSetups[gColRun.elite - 1];
}

static void ClearDef(struct UnitDefinition *d)
{
    u8 *p = (u8 *)d;
    unsigned i;
    for (i = 0; i < sizeof(*d); i++)
        p[i] = 0;
}

static int EnemyLevel(int encounter)
{
    int level = COL_START_LEVEL - 1 + (gColRun.floor - 1) * 5 + gColRun.normalWins / 2;
    if (encounter == COL_ENC_ELITE)
        level += 3;
    else if (encounter == COL_ENC_BOSS)
        level += 6;
    return level < 1 ? 1 : level > 30 ? 30 : level;
}

static struct Unit *Spawn(int charId, int classId, const u8 *items, int nItems, int level, int slot)
{
    struct UnitDefinition def;
    struct Unit *u;
    int i;

    ClearDef(&def);
    def.charIndex = (u8)charId;
    def.classIndex = (u8)classId;
    def.autolevel = 1;
    def.allegiance = FACTION_RED_ALLEGIANCE;
    def.level = (u8)level;
    def.xPosition = Col_CurrentArena()->enemy[slot][0];      /* the arena's spawn tiles */
    def.yPosition = Col_CurrentArena()->enemy[slot][1];
    for (i = 0; i < nItems && i < 4; i++)
        def.items[i] = items[i];
    u = LoadUnit(&def);                         /* AI bytes 0 = charge (vanilla default) */
    for (i = 0; u && i < nItems; i++) {         /* able to use every weapon / staff it carries */
        int item = items[i], need = GetItemRequiredExp(item);
        if (item && (GetItemAttributes(item) & (IA_WEAPON | IA_STAFF)) && u->ranks[GetItemType(item)] < need)
            u->ranks[GetItemType(item)] = (u8)need;
    }
    return u;
}

/* Floor 1 squads carry iron instead of steel (Phase 16 lever). */
static int SquadItem(int item)
{
    if (gColRun.floor > 1)
        return item;
    switch (item) {
    case STEEL_SWORD: return 0x01;
    case STEEL_LANCE: return 0x14;
    case STEEL_AXE:   return 0x1F;
    case STEEL_BOW:   return 0x2D;
    case ELFIRE:      return 0x38;              /* Fire */
    }
    return item;
}

/* The run's first Elite (floor 1, after 3 wins) is gentler (developer, 2026-09-27: too hard):
 * Champion 2 levels lower with half the boost, squads 1 level lower. Phase 16 levers. */
int Col_IsFirstElite(void)
{
    return gColRun.floor == 1 && gColRun.normalWins <= COL_WINS_PER_REWARD;
}

static void Boost(struct Unit *u)               /* a Champion fights 3 alone */
{
    int hp = 12, stat = 2;
    if (Col_IsFirstElite()) {
        hp = 6;
        stat = 1;
    }
    u->maxHP += hp;
    u->curHP = u->maxHP;
    u->pow += stat; u->skl += stat; u->spd += stat; u->def += stat; u->res += stat; u->lck += stat;
    ((s8 *)u)[0x3A] += stat;                    /* Mag (Str/Mag split) */
}

void Col_CreateEnemies(int encounter)
{
    struct Unit *enemies[3] = { 0 };
    const struct ColEliteSetup *elite = encounter == COL_ENC_ELITE ? Col_CurrentElite() : NULL;
    int count = 0, i;

    if (elite) {
        int level = elite->count == 1 ? EnemyLevel(COL_ENC_ELITE) : EnemyLevel(COL_ENC_NORMAL) - 2;
        if (Col_IsFirstElite())
            level -= elite->count == 1 ? 2 : 1;
        if (level < 1)
            level = 1;
        for (i = 0; i < elite->count; i++) {
            const struct ColEliteMember *m = &elite->members[i];
            u8 items[2];
            struct Unit *u;
            items[0] = (u8)(elite->count == 1 ? m->items[0] : SquadItem(m->items[0]));
            items[1] = (u8)(elite->count == 1 ? m->items[1] : SquadItem(m->items[1]));
            u = Spawn(m->charId, m->classId, items, items[1] ? 2 : 1, level, i);
            if (!u)
                continue;
            if (elite->count == 1) {
                Boost(u);
                u->state |= US_DROP_ITEM;       /* drops its Elite weapon (spec 50) */
            }
            enemies[count++] = u;
        }
    } else {
        for (i = 0; i < 3; i++) {               /* spec 46: 3v3 */
            const u8 *kind = kEnemyKinds[NextRN_N(sizeof(kEnemyKinds) / sizeof(kEnemyKinds[0]))];
            struct Unit *u = Spawn(GENERIC_ENEMY_CHAR, kind[0], &kind[1], 1, EnemyLevel(encounter), i);
            if (u)
                enemies[count++] = u;
        }
    }
    Col_RollDrops(encounter, enemies, count);
}

/* ---- drops ---- */

static void SetDrop(struct ColDrop *d, struct Unit *u, int kind, int encounter)
{
    d->unit = (u8)u->index;
    d->kind = (u8)kind;
    d->killer = 0;
    d->value = 0;
    if (kind == COL_DROP_RELIC)
        d->value = (u8)Col_RelicRoll();
    else if (kind == COL_DROP_GOLD) {
        int gold = 80 + 40 * gColRun.floor + NextRN_N(61);           /* 120-180 on floor 1 */
        if (encounter != COL_ENC_NORMAL)
            gold *= 2;
        d->value = (u8)(gold / 10);
    }
}

void Col_RollDrops(int encounter, struct Unit **enemies, int count)
{
    int i, guaranteed = -1, chance = encounter == COL_ENC_NORMAL ? 25 : 50;

    for (i = 0; i < COL_MAX_DROPS; i++) {
        gColRun.drops[i].unit = 0;
        gColRun.drops[i].kind = COL_DROP_NONE;
        gColRun.drops[i].value = 0;
        gColRun.drops[i].killer = 0;
    }
    if (count <= 0)
        return;
    if (gColRun.battlesWon == 0 || encounter == COL_ENC_ELITE)
        guaranteed = NextRN_N(count);           /* the run's first battle, and every Elite */
    for (i = 0; i < count && i < COL_MAX_DROPS; i++) {
        int r;
        if (i == guaranteed) {
            SetDrop(&gColRun.drops[i], enemies[i], COL_DROP_RELIC, encounter);
            continue;
        }
        if (NextRN_N(100) >= chance)
            continue;
        r = NextRN_N(100);
        SetDrop(&gColRun.drops[i], enemies[i], r < 50 ? COL_DROP_GOLD : r < 75 ? COL_DROP_SKILL : COL_DROP_RELIC,
                encounter);
    }
}

static int IsDead(int index)
{
    struct Unit *u = GetUnit(index);
    return !u || !u->pCharacterData || (u->state & US_DEAD) || u->curHP <= 0;
}

int Col_PendingDrop(void)
{
    int i;
    if (!Col_RunIsValid() || !gColRun.active)
        return -1;
    for (i = 0; i < COL_MAX_DROPS; i++)
        if (gColRun.drops[i].kind != COL_DROP_NONE && gColRun.drops[i].unit && IsDead(gColRun.drops[i].unit))
            return i;
    return -1;
}

/* Battle proc loop (before Proc_Finish), once per strike of a real battle: remember who deals a
 * killing blow to an enemy carrying a drop (the skill drop goes to them). */
struct ColProcStats { u16 config; u16 range; s16 damage; };
#define HIT_ATTR_MISS 2

void Col_DropKillProc(struct BattleUnit *att, struct BattleUnit *def, u8 *hit, struct ColProcStats *stats)
{
    int i;

    if (!(gBattleStats.config & BATTLE_CONFIG_REAL) || (*(u32 *)hit & HIT_ATTR_MISS))
        return;
    if (def->unit.curHP - stats->damage > 0)
        return;
    for (i = 0; i < COL_MAX_DROPS; i++)
        if (gColRun.drops[i].kind != COL_DROP_NONE && gColRun.drops[i].unit == (u8)def->unit.index)
            gColRun.drops[i].killer = (u8)att->unit.index;
}
