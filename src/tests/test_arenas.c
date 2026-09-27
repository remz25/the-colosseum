/* On-target tests: arenas (src/arenas/arenas.c, docs/ARENAS.md). Map tests (ColTest_MapRun
 * restores gColRun and the two units). The static map checks (spawn tiles walkable, not on
 * hazards, a foot path between the sides) run offline in tests/check_arenas.py. */
#include "colosseum.h"
#include "bmunit.h"
#include "bmmap.h"
#include "chapterdata.h"
#include "uiselecttarget.h"
#include "bmtarget.h"
#include "variables.h"

#define CHECK(cond) do { if (!(cond)) return __LINE__; } while (0)

#define VOLCANIC 3
#define CATHEDRAL 4
#define CLASS_PEGASUS_KNIGHT 0x49

/* The battle chapter's data follows the arena and weather; other chapters are untouched. */
int Test_ArenaChapterData(struct Unit *sA, struct Unit *sT)
{
    const struct ROMChapterData *c;
    int a;

    Col_RunNew(1);
    for (a = 0; a < Col_ArenaCount(); a++) {
        const struct ColArenaDef *def = &gColArenas[a];
        const u8 *map = gChapterDataAssetTable[def->mapAsset];

        Col_SetArena(a, COL_WX_CLEAR);
        c = GetROMChapterStruct(COL_BATTLE_CHAPTER);
        CHECK(c == (const struct ROMChapterData *)COL_ARENA_CHAPTER);
        CHECK(c->map.mainLayerId == def->mapAsset && c->map.changeLayerId == 0);
        CHECK(c->map.obj1Id == gChapterDataTable[def->chapter].map.obj1Id);
        CHECK(c->map.paletteId == gChapterDataTable[def->chapter].map.paletteId);
        CHECK(c->map.tileConfigId == gChapterDataTable[def->chapter].map.tileConfigId);
        CHECK(c->initialWeather == WEATHER_FINE && c->initialFogLevel == 0);
        CHECK(c->mapEventDataId == gChapterDataTable[COL_BATTLE_CHAPTER].mapEventDataId);   /* same events */
        /* the map: LZ77, at least 15x10, spawns inside */
        CHECK(map && map[0] == 0x10 && (map[1] | map[2] << 8) >= 2 + 15 * 10 * 2);
        CHECK(def->name && def->name[0]);
    }
    Col_SetArena(VOLCANIC, COL_WX_ASHFALL);
    c = GetROMChapterStruct(COL_BATTLE_CHAPTER);
    CHECK(c->initialWeather == WEATHER_FLAMES && c->initialFogLevel == 0);
    Col_SetArena(1, COL_WX_FOG);
    CHECK(GetROMChapterStruct(COL_BATTLE_CHAPTER)->initialFogLevel == 3);
    Col_SetArena(1, COL_WX_RAIN);
    CHECK(GetROMChapterStruct(COL_BATTLE_CHAPTER)->initialWeather == WEATHER_RAIN);
    Col_SetArena(1, COL_WX_SNOW);
    CHECK(GetROMChapterStruct(COL_BATTLE_CHAPTER)->initialWeather == WEATHER_SNOW);
    Col_SetArena(1, COL_WX_SANDSTORM);
    CHECK(GetROMChapterStruct(COL_BATTLE_CHAPTER)->initialWeather == WEATHER_SANDSTORM);
    CHECK(GetROMChapterStruct(5) == &gChapterDataTable[5]);                  /* other chapters */
    gColRun.active = 0;                                                      /* no run: Grand */
    CHECK(Col_CurrentArena() == &gColArenas[0] && Col_CurrentWeather() == COL_WX_CLEAR);
    CHECK(GetROMChapterStruct(COL_BATTLE_CHAPTER)->map.mainLayerId == gColArenas[0].mapAsset);
    return 0;
}

/* Arenas come from the floor's pool, never the same twice in a row; weathers the arena allows. */
int Test_ArenaRoll(struct Unit *sA, struct Unit *sT)
{
    int floor, k, i;

    Col_RunNew(2);
    CHECK(gColRun.arena == 0 && gColRun.weather == COL_WX_CLEAR);           /* a run opens in the Grand Colosseum */
    for (floor = 1; floor <= 9; floor++) {
        u8 pool[5];
        int n = Col_FloorPool(floor, pool);
        CHECK(n >= 3);
        gColRun.floor = (u8)floor;
        for (k = 0; k < 40; k++) {
            int before = gColRun.arena, inPool = 0;
            Col_RollArena();
            for (i = 0; i < n; i++)
                inPool |= pool[i] == gColRun.arena;
            if (!inPool)
                return __LINE__ + floor * 1000;
            CHECK(gColRun.arena != before);                                 /* every pool has 3+ */
            CHECK(gColArenas[gColRun.arena].weather[gColRun.weather] > 0);
        }
    }
    return 0;
}

static int TargetFor(struct Unit *u)            /* damage/heal listed for u, or -999 */
{
    int i;
    for (i = 0; i < GetSelectTargetCount(); i++)
        if ((u8)GetTarget(i)->uid == (u8)u->index)             /* both s8: red units are 0x81+ */
            return GetTarget(i)->extra;
    return -999;
}

/* Hazard tiles hurt at the start of the unit's phase (the poison step), never below 1 HP;
 * poison still adds; fliers are not hurt by burning ground. */
int Test_HazardDamage(struct Unit *sA, struct Unit *sT)
{
    Col_RunNew(3);
    Col_SetArena(VOLCANIC, COL_WX_ASHFALL);
    CHECK(Col_ArenaTileAt(0, 4) == COL_TILE_BURNING && Col_ArenaTileAt(2, 1) == COL_TILE_NONE);
    sA->xPos = 0; sA->yPos = 4;
    sA->maxHP = 30; sA->curHP = 20;
    sA->statusIndex = 0;
    MakePoisonDamageTargetList(FACTION_BLUE);
    CHECK(TargetFor(sA) == 5);
    sA->curHP = 3;
    MakePoisonDamageTargetList(FACTION_BLUE);
    CHECK(TargetFor(sA) == 2);                                              /* left at 1 HP */
    sA->curHP = 1;
    MakePoisonDamageTargetList(FACTION_BLUE);
    CHECK(TargetFor(sA) == -999);                                           /* nothing to take */
    sA->curHP = 20;
    sA->statusIndex = UNIT_STATUS_POISON;
    sA->statusDuration = 3;
    MakePoisonDamageTargetList(FACTION_BLUE);
    CHECK(TargetFor(sA) >= 6 && TargetFor(sA) <= 8);                        /* poison 1-3 + 5 */
    sA->statusIndex = 0;
    sA->xPos = 2; sA->yPos = 1;                                             /* cool rock */
    MakePoisonDamageTargetList(FACTION_BLUE);
    CHECK(TargetFor(sA) == -999);
    sA->xPos = 0; sA->yPos = 4;
    sA->pClassData = GetClassData(CLASS_PEGASUS_KNIGHT);                    /* flying over it */
    CHECK(Col_HazardDamageFor(sA) == 0);
    MakePoisonDamageTargetList(FACTION_BLUE);
    CHECK(TargetFor(sA) == -999);
    sT->xPos = 12; sT->yPos = 4;                                            /* enemies too, on their phase */
    sT->curHP = 10;
    sT->statusIndex = 0;
    MakePoisonDamageTargetList(FACTION_RED);
    CHECK(TargetFor(sT) == 5);
    Col_SetArena(0, COL_WX_CLEAR);                                          /* no hazards elsewhere */
    MakePoisonDamageTargetList(FACTION_RED);
    CHECK(TargetFor(sT) == -999);
    return 0;
}

/* Sacred tiles: +10% heal at the start of the phase, through the Skill System's heal loop. */
int Test_SacredTiles(struct Unit *sA, struct Unit *sT)
{
    Col_RunNew(4);
    Col_SetArena(CATHEDRAL, COL_WX_CLEAR);
    sA->xPos = 7; sA->yPos = 4;
    CHECK(Col_SacredTileHeal(sA, 0) == 10);
    sA->maxHP = 40; sA->curHP = 10;
    sA->statusIndex = 0;
    MakeTerrainHealTargetList(FACTION_BLUE);
    CHECK(TargetFor(sA) == 4);                                              /* 10% of 40 */
    sA->xPos = 0; sA->yPos = 3;                                             /* off the seal */
    CHECK(Col_SacredTileHeal(sA, 0) == 0);
    return 0;
}

/* Enemy AI: a tile to attack from scores 10 lower per HP of hazard; fliers don't mind. */
int Test_AiHazardScore(struct Unit *sA, struct Unit *sT)
{
    struct Unit *saved = gActiveUnit;
    int hot, cool;

    Col_RunNew(5);
    Col_SetArena(VOLCANIC, COL_WX_CLEAR);
    gActiveUnit = sT;
    hot = Col_AiTerrainScore(0, 4);
    Col_SetArena(0, COL_WX_CLEAR);                                          /* same map tile, no hazard */
    cool = Col_AiTerrainScore(0, 4);
    CHECK(hot == cool - 50);
    Col_SetArena(VOLCANIC, COL_WX_CLEAR);
    sT->pClassData = GetClassData(CLASS_PEGASUS_KNIGHT);                    /* a flier: no penalty */
    hot = Col_AiTerrainScore(0, 4);
    Col_SetArena(0, COL_WX_CLEAR);
    CHECK(hot == Col_AiTerrainScore(0, 4));
    Col_SetArena(CATHEDRAL, COL_WX_CLEAR);
    gActiveUnit = sA;
    hot = Col_AiTerrainScore(7, 4);
    Col_SetArena(0, COL_WX_CLEAR);
    CHECK(hot == Col_AiTerrainScore(7, 4) + 10);
    /* moving (not attacking): never ends on a hazard that would hurt the unit */
    Col_SetArena(VOLCANIC, COL_WX_CLEAR);
    sT->pClassData = GetClassData(0x0F);                                    /* on foot again (Mercenary) */
    gActiveUnit = sT;
    CHECK(gBmMapOther != NULL);
    {
        u8 other = gBmMapOther[4][0], other2 = gBmMapOther[1][2];
        gBmMapOther[4][0] = 0;
        gBmMapOther[1][2] = 0;
        hot = Col_AiCheckDangerAt(0, 4, 0xFF);
        cool = Col_AiCheckDangerAt(2, 1, 0xFF);
        gBmMapOther[4][0] = other;
        gBmMapOther[1][2] = other2;
    }
    CHECK(hot == 0 && cool == 1);
    gActiveUnit = saved;
    return 0;
}
