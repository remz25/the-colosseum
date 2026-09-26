/* Battle setup and results (Phase 3): the reusable battle chapter calls these from its events.
 *   Col_PrepareBattle   (beginning scene, ASMC): start a run if none is active, place the deployed
 *                       roster units, keep reserves off the map, restore HP from the last battle,
 *                       and create the enemies for this encounter.
 *   Col_OnBattleWon     (ending scene, ASMC): record the victory, deaths and HP in the run state.
 * HP persists between battles (spec 20: no healing outside battle except Recover), so FE8's
 * between-chapter healing is undone from the run state. */
#include "coliseum.h"
#include "bmunit.h"
#include "bmmap.h"
#include "rng.h"
#include "bmudisp.h"

#define FACTION_BLUE_ALLEGIANCE  0
#define FACTION_RED_ALLEGIANCE   2
#define GENERIC_ENEMY_CHAR       0x80      /* vanilla generic "Soldier" character */

/* Arena spawn tiles (battle map: vanilla chapter 67's hall, see BattleChapter.event) */
static const u8 kPlayerSpawn[COL_MAX_DEPLOY][2] = { { 0, 4 }, { 0, 5 }, { 1, 3 } };
static const u8 kEnemySpawn[3][2] = { { 14, 4 }, { 14, 5 }, { 13, 3 } };

/* Enemy faction for Phase 3 (mercenaries): class and weapon. Factions per floor come later. */
static const u8 kEnemyKinds[][2] = {
    { 0x3F, 0x1F },   /* Fighter    Iron Axe   */
    { 0x4E, 0x14 },   /* Soldier    Iron Lance */
    { 0x0F, 0x01 },   /* Mercenary  Iron Sword */
    { 0x19, 0x2D },   /* Archer     Iron Bow   */
    { 0x25, 0x38 },   /* Mage       Fire       */
};

static void ClearDef(struct UnitDefinition *d)
{
    u8 *p = (u8 *)d;
    unsigned i;
    for (i = 0; i < sizeof(*d); i++)
        p[i] = 0;
}

struct Unit *Col_RosterUnit(int slot)
{
    int pool;
    if (slot < 0 || slot >= COL_MAX_ROSTER)
        return NULL;
    pool = gColRun.roster[slot];
    if (pool >= COL_POOL_SIZE)
        return NULL;
    return GetUnitFromCharIdAndFaction(gColPool[pool].charId, FACTION_BLUE);
}

/* A pool character as a new blue unit: level 5 with the character's level-5 bases (spec 21;
 * CharacterTable.csv holds them at base level 5, so no autolevel), EXP 0, starting items. */
struct Unit *Col_LoadPoolUnit(int pool)
{
    const struct ColPoolEntry *e;
    struct UnitDefinition def;
    struct Unit *unit;
    int i;

    if (pool < 0 || pool >= COL_POOL_SIZE)
        return NULL;
    e = &gColPool[pool];
    ClearDef(&def);
    def.charIndex = e->charId;
    def.classIndex = e->classId;
    def.autolevel = 0;
    def.allegiance = FACTION_BLUE_ALLEGIANCE;
    def.level = COL_START_LEVEL;
    for (i = 0; i < 4; i++)
        def.items[i] = e->items[i];
    unit = LoadUnit(&def);
    if (unit)
        unit->exp = 0;              /* vanilla disables EXP only at level 20; our cap is 30 */
    return unit;
}

/* Create the unit for roster slot `slot` if it doesn't exist yet. */
static struct Unit *EnsureRosterUnit(int slot)
{
    struct Unit *unit = Col_RosterUnit(slot);

    if (unit)
        return unit;
    unit = Col_LoadPoolUnit(gColRun.roster[slot]);
    if (unit)
        gColRun.choiceLevel[slot] = (u8)unit->level;    /* no stat choice owed for level 5 */
    return unit;
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

static void CreateEnemies(int encounter)
{
    int count = 3, i;                 /* spec 46: 3v3. Champions (1 unit) come with Phase 11 */

    for (i = 0; i < count; i++) {
        const u8 *kind = kEnemyKinds[NextRN_N(sizeof(kEnemyKinds) / sizeof(kEnemyKinds[0]))];
        struct UnitDefinition def;

        ClearDef(&def);
        def.charIndex = GENERIC_ENEMY_CHAR;
        def.classIndex = kind[0];
        def.autolevel = 1;
        def.allegiance = FACTION_RED_ALLEGIANCE;
        def.level = (u8)EnemyLevel(encounter);
        def.xPosition = kEnemySpawn[i][0];
        def.yPosition = kEnemySpawn[i][1];
        def.items[0] = kind[1];
        LoadUnit(&def);               /* AI bytes 0 = charge (vanilla default) */
    }
}

/* Every living roster member exists as a (hidden) unit: menus before the battle need them. */
void Col_EnsureRosterUnits(void)
{
    int slot;
    for (slot = 0; slot < COL_MAX_ROSTER; slot++) {
        struct Unit *unit;
        if (gColRun.roster[slot] >= COL_POOL_SIZE || Col_RosterUnit(slot))
            continue;
        unit = EnsureRosterUnit(slot);
        if (unit)
            unit->state |= US_HIDDEN | US_NOT_DEPLOYED;    /* placed by Col_PrepareBattle */
    }
}

void Col_PrepareBattle(void)
{
    int slot, placed = 0;

    if (!Col_RunIsValid() || !gColRun.active)
        Col_StartRun();

    for (slot = 0; slot < COL_MAX_ROSTER; slot++) {
        struct Unit *unit;
        int d, deployed = 0;

        if (gColRun.roster[slot] >= COL_POOL_SIZE)
            continue;
        unit = EnsureRosterUnit(slot);
        if (!unit || (unit->state & US_DEAD))
            continue;
        for (d = 0; d < COL_MAX_DEPLOY; d++)
            if (gColRun.deployed[d] == slot)
                deployed = 1;
        if (deployed && placed < COL_MAX_DEPLOY) {
            unit->state &= ~(US_HIDDEN | US_NOT_DEPLOYED | US_UNSELECTABLE);
            unit->xPos = kPlayerSpawn[placed][0];
            unit->yPos = kPlayerSpawn[placed][1];
            placed++;
        } else {
            unit->state |= US_HIDDEN | US_NOT_DEPLOYED;     /* reserve (spec 8) */
        }
        if (gColRun.hp[slot])                               /* HP from the last battle */
            unit->curHP = gColRun.hp[slot] > unit->maxHP ? unit->maxHP : gColRun.hp[slot];
    }

    CreateEnemies(Col_NextEncounter());
    RefreshEntityBmMaps();
    RefreshUnitSprites();
}

/* After a victory: deaths (spec 12) and HP, then the run's progression. */
void Col_OnBattleWon(void)
{
    int slot;

    for (slot = 0; slot < COL_MAX_ROSTER; slot++) {
        struct Unit *unit;
        int pool = gColRun.roster[slot];

        if (pool >= COL_POOL_SIZE)
            continue;
        unit = Col_RosterUnit(slot);
        if (!unit || (unit->state & US_DEAD) || unit->curHP <= 0) {
            Col_RosterRemoveDead(slot);
            continue;
        }
        gColRun.hp[slot] = (u8)unit->curHP;
    }
    {
        int encounter = Col_NextEncounter();
        /* spec 37: battles award gold; gold relics (spec 36) change only this amount */
        Col_AddGold(Col_RelicApplyGold(Col_BattleGold(encounter)));
        Col_SyncPartyGold();
        Col_OnVictory(encounter);
    }
    Col_ShopGenerate();                                  /* spec 38: new stock, no reroll */
}

/* Replaces the intro monologue and world map in the game control proc (SkipWorldMap.event). */
void Col_NoStory(void)
{
}
