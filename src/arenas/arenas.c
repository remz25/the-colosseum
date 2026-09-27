/* Arenas (docs/ARENAS.md, docs/ARENA_SPEC.md): every battle is fought in an arena rolled from
 * the floor's pool, with a weather and, on some arenas, hazard or sacred tiles.
 *
 * Maps: each arena's map is a region cut from a vanilla FE8 chapter (arena_maps.txt,
 * scripts/arenas.py); it keeps that chapter's tileset, palette, tile config and animations.
 * The battle chapter (slot 0) stays the same chapter; GetROMChapterStruct (0x08034618, replaced
 * in Arenas.event) returns for it a copy of its data with the arena's map, tileset, fog and
 * weather. The copy lives at COL_ARENA_CHAPTER and is rebuilt from the run state (arena,
 * weather) on every call, so it never needs saving: a game save or suspend brings back the same
 * arena through the run state.
 *
 * Arena and weather are rolled when a battle is won (Col_OnBattleWon), for the next battle, and
 * saved with the run. A new run starts in the Grand Coliseum.
 *
 * Weather uses FE8's own systems: rain and snow switch every class to FE8's rain/snow movement
 * costs, fog is FE8's fog of war (vision 3), each has FE8's weather animation. Rain, sandstorm
 * and ashfall also lower Hit (pre-battle loop, both sides alike); the notice before the battle
 * and the forecast show it.
 *
 * Tiles: hazard tiles hurt a unit standing on them at the start of its own phase (the poison
 * step of FE8's phase start, which shows the damage), never below 1 HP; flying units are not
 * hurt by burning or poisoned ground. Sacred tiles heal 10% of max HP at the start of the
 * unit's phase (the Skill System's HP restoration loop, like FE8's forts).
 *
 * AI (ARENA_SPEC 15): when an enemy picks the tile to attack from, hazard tiles it would be hurt
 * on score lower (10 per HP of damage) and sacred tiles higher (+10), so it attacks from a safe
 * tile when it can and still attacks from a hazard when that is the only way. When it only
 * moves (towards a target, a heal point, an escape), it never ends that move on a hazard that
 * would hurt it (FE8's danger check); it may still walk across one. Fliers ignore burning ground. */
#include "coliseum.h"
#include "bmunit.h"
#include "bmmap.h"
#include "bmbattle.h"
#include "chapterdata.h"
#include "bmsave.h"
#include "uiselecttarget.h"
#include "rng.h"
#include "variables.h"

enum { GRAND, FOREST, DESERT, VOLCANIC, CATHEDRAL, ROYAL, ABYSS, RUINS };

#define T(x, y, kind) { x, y, COL_TILE_##kind, 0 }
#define END { 0, 0, COL_TILE_NONE, 0 }

/* Tiles follow what the map shows, so the player can read them: every walkable tile of glowing
 * orange rock burns (chapter 0x12's hot-rock metatiles 0x08F 0x0AB 0x0E6 0x10F 0x127 0x128 0x129
 * 0x191 0x2D0 0x2D1 0x2D3 0x2F0 0x2F1; the half-lit edge tiles do not), and the whole circular seal of the
 * cathedral is sacred. */
static const struct ColArenaTile kVolcanicTiles[] = {
    T(11, 2, BURNING), T(0, 3, BURNING), T(11, 3, BURNING), T(12, 3, BURNING), T(13, 3, BURNING),
    T(0, 4, BURNING), T(1, 4, BURNING), T(11, 4, BURNING), T(12, 4, BURNING), T(13, 4, BURNING),
    T(14, 4, BURNING), T(0, 5, BURNING), T(1, 5, BURNING), T(11, 5, BURNING), T(12, 5, BURNING),
    T(0, 6, BURNING), T(1, 6, BURNING), T(10, 6, BURNING), T(11, 6, BURNING), T(12, 6, BURNING),
    T(1, 7, BURNING), T(2, 7, BURNING), T(10, 7, BURNING), T(11, 7, BURNING), T(2, 8, BURNING), END,
};
static const struct ColArenaTile kCathedralTiles[] = {
                      T(6, 2, SACRED), T(7, 2, SACRED), T(8, 2, SACRED),
    T(5, 3, SACRED), T(6, 3, SACRED), T(7, 3, SACRED), T(8, 3, SACRED), T(9, 3, SACRED),
    T(5, 4, SACRED), T(6, 4, SACRED), T(7, 4, SACRED), T(8, 4, SACRED), T(9, 4, SACRED),
    T(5, 5, SACRED), T(6, 5, SACRED), T(7, 5, SACRED), T(8, 5, SACRED), T(9, 5, SACRED),
                      T(6, 6, SACRED), T(7, 6, SACRED), T(8, 6, SACRED), END,
};

/*                         weather weights: Clear Rain Snow Fog Sand Ash */
const struct ColArenaDef gColArenas[] = {
    [GRAND]     = { "Grand Coliseum", NULL, NULL, 0x43, 0xDB, 0, { 1, 0, 0, 0, 0, 0 },
                    { { 0, 4 }, { 0, 5 }, { 1, 3 } }, { { 14, 4 }, { 14, 5 }, { 13, 3 } } },
    [FOREST]    = { "Forest Arena", NULL, NULL, 0x14, 0x04, 0, { 6, 4, 0, 0, 0, 0 },
                    { { 1, 4 }, { 0, 5 }, { 3, 4 } }, { { 14, 1 }, { 13, 2 }, { 12, 1 } } },
    [DESERT]    = { "Desert Arena", NULL, NULL, 0x0F, 0x08, 0, { 5, 0, 0, 0, 5, 0 },
                    { { 0, 2 }, { 0, 3 }, { 0, 4 } }, { { 14, 2 }, { 14, 3 }, { 14, 4 } } },
    [VOLCANIC]  = { "Volcanic Arena", "Hot rock: -5 HP each turn", kVolcanicTiles, 0x12, 0x0B, 0,
                    { 0, 0, 0, 0, 0, 1 },
                    { { 0, 1 }, { 1, 1 }, { 2, 1 } }, { { 9, 8 }, { 10, 8 }, { 11, 8 } } },
    [CATHEDRAL] = { "Ruined Cathedral", "Seal: heals 10% each turn", kCathedralTiles, 0x15, 0x15,
                    COL_ARENA_ELITE, { 5, 5, 0, 0, 0, 0 },
                    { { 0, 3 }, { 0, 4 }, { 1, 4 } }, { { 14, 3 }, { 14, 4 }, { 13, 4 } } },
    [ROYAL]     = { "Royal Arena", NULL, NULL, 0x13, 0x25, COL_ARENA_ELITE, { 1, 0, 0, 0, 0, 0 },
                    { { 0, 5 }, { 0, 6 }, { 1, 5 } }, { { 14, 5 }, { 13, 5 }, { 14, 4 } } },
    [ABYSS]     = { "Abyss Arena", NULL, NULL, 0x15, 0x39, 0,
                    { 0, 0, 0, 1, 0, 0 },
                    { { 0, 1 }, { 0, 2 }, { 1, 1 } }, { { 14, 1 }, { 14, 2 }, { 13, 1 } } },
    [RUINS]     = { "Misty Ruins", NULL, NULL, 0x2E, 0x4D, 0, { 3, 0, 0, 7, 0, 0 },
                    { { 0, 0 }, { 0, 1 }, { 1, 1 } }, { { 14, 7 }, { 14, 8 }, { 14, 9 } } },
};

/* Floor pools (spec 12 of ARENA_SPEC, adjusted to the arenas that exist); floor 7 and up use the
 * last. 0xFF ends a pool. */
#define POOL_FLOORS 7
static const u8 kFloorPools[POOL_FLOORS][5] = {
    { GRAND, FOREST, ROYAL, 0xFF },
    { FOREST, DESERT, GRAND, 0xFF },
    { CATHEDRAL, RUINS, ROYAL, 0xFF },
    { FOREST, DESERT, RUINS, 0xFF },
    { ROYAL, GRAND, DESERT, CATHEDRAL, 0xFF },
    { VOLCANIC, ABYSS, CATHEDRAL, 0xFF },
    { ABYSS, VOLCANIC, ROYAL, 0xFF },
};

/* FE8 weather and vision per COLISEUM weather; Hit penalty (Phase 16 levers) */
static const u8 kFe8Weather[COL_WX_COUNT] = {
    WEATHER_FINE, WEATHER_RAIN, WEATHER_SNOW, WEATHER_FINE, WEATHER_SANDSTORM, WEATHER_FLAMES,
};
static const u8 kVision[COL_WX_COUNT] = { 0, 0, 0, 3, 0, 0 };
static const u8 kHitPenalty[COL_WX_COUNT] = { 0, 5, 0, 0, 10, 5 };
static const char *const kWeatherNames[COL_WX_COUNT] = {
    "Clear", "Rain", "Snow", "Fog", "Sandstorm", "Ashfall",
};
static const u8 kHazardDamage[COL_TILE_KIND_COUNT] = { 0, 5, 3, 10, 0 };
#define SACRED_HEAL_PERCENT 10

int Col_ArenaCount(void)
{
    return sizeof(gColArenas) / sizeof(gColArenas[0]);
}

static int RunActive(void)
{
    return Col_RunIsValid() && gColRun.active;
}

const struct ColArenaDef *Col_CurrentArena(void)
{
    if (!RunActive() || gColRun.arena >= Col_ArenaCount())
        return &gColArenas[GRAND];
    return &gColArenas[gColRun.arena];
}

int Col_CurrentWeather(void)
{
    if (!RunActive() || gColRun.weather >= COL_WX_COUNT)
        return COL_WX_CLEAR;
    return gColRun.weather;
}

const char *Col_WeatherName(int weather)
{
    return weather >= 0 && weather < COL_WX_COUNT ? kWeatherNames[weather] : kWeatherNames[0];
}

int Col_WeatherHitPenalty(int weather)
{
    return weather >= 0 && weather < COL_WX_COUNT ? kHitPenalty[weather] : 0;
}

int Col_FloorPool(int floor, u8 *out)
{
    const u8 *pool = kFloorPools[floor < 1 ? 0 : floor > POOL_FLOORS ? POOL_FLOORS - 1 : floor - 1];
    int n = 0;

    while (n < 5 && pool[n] != 0xFF) {
        out[n] = pool[n];
        n++;
    }
    return n;
}

void Col_SetArena(int arena, int weather)
{
    gColRun.arena = (u8)(arena >= 0 && arena < Col_ArenaCount() ? arena : GRAND);
    gColRun.weather = (u8)(weather >= 0 && weather < COL_WX_COUNT ? weather : COL_WX_CLEAR);
}

static int RollWeather(const struct ColArenaDef *a)
{
    int w, total = 0, pick;

    for (w = 0; w < COL_WX_COUNT; w++)
        total += a->weather[w];
    if (!total)
        return COL_WX_CLEAR;
    pick = NextRN_N(total);
    for (w = 0; w < COL_WX_COUNT; w++) {
        if (pick < a->weather[w])
            return w;
        pick -= a->weather[w];
    }
    return COL_WX_CLEAR;
}

/* The next battle's arena: from the floor's pool, not the same as this one when the pool allows;
 * Elite battles count arenas made for them twice. */
void Col_RollArena(void)
{
    u8 pool[5], weights[5];
    int n = Col_FloorPool(gColRun.floor, pool), i, total = 0, pick, elite = Col_NextEncounter() == COL_ENC_ELITE;

    for (i = 0; i < n; i++) {
        weights[i] = (u8)((n > 1 && pool[i] == gColRun.arena) ? 0 :
                          (elite && (gColArenas[pool[i]].flags & COL_ARENA_ELITE)) ? 2 : 1);
        total += weights[i];
    }
    pick = NextRN_N(total);
    for (i = 0; i < n - 1; i++) {
        if (pick < weights[i])
            break;
        pick -= weights[i];
    }
    Col_SetArena(pool[i], RollWeather(&gColArenas[pool[i]]));
}

/* ---- chapter data for the battle chapter ---- */

#define gColArenaChapter (*(struct ROMChapterData *)COL_ARENA_CHAPTER)
_Static_assert(sizeof(struct ROMChapterData) <= 0x100, "arena chapter copy must fit 0x300-0x3FF");

/* Replaces GetROMChapterStruct (0x08034618): chapter 0x7F is the link arena's extra map (as in
 * vanilla); the battle chapter gets the current arena's data; every other chapter its own. */
const struct ROMChapterData *Col_GetROMChapterStruct(unsigned chIndex)
{
    const struct ColArenaDef *a;
    const u8 *src;
    u8 *dst;
    unsigned i;
    int weather;

    if (chIndex == 0x7F)
        return gExtraMapInfo->chapter_info;
    if (chIndex != COL_BATTLE_CHAPTER)
        return &gChapterDataTable[chIndex];
    a = Col_CurrentArena();
    weather = Col_CurrentWeather();
    src = (const u8 *)&gChapterDataTable[COL_BATTLE_CHAPTER];
    dst = (u8 *)&gColArenaChapter;
    for (i = 0; i < sizeof(struct ROMChapterData); i++)
        dst[i] = src[i];
    gColArenaChapter.map = gChapterDataTable[a->chapter].map;       /* tileset, palette, config, anims */
    gColArenaChapter.map.mainLayerId = a->mapAsset;
    gColArenaChapter.map.changeLayerId = 0;
    gColArenaChapter.initialFogLevel = kVision[weather];
    gColArenaChapter.initialWeather = kFe8Weather[weather];
    return &gColArenaChapter;
}

/* ---- tiles ---- */

int Col_ArenaTileAt(int x, int y)
{
    const struct ColArenaTile *t = Col_CurrentArena()->tiles;

    for (; t && t->kind != COL_TILE_NONE; t++)
        if (t->x == x && t->y == y)
            return t->kind;
    return COL_TILE_NONE;
}

int Col_HazardDamage(int kind)
{
    return kind > 0 && kind < COL_TILE_KIND_COUNT ? kHazardDamage[kind] : 0;
}

int Col_HazardDamageAt(struct Unit *unit, int x, int y)
{
    int kind = Col_ArenaTileAt(x, y);

    if ((kind == COL_TILE_BURNING || kind == COL_TILE_POISON) && (UNIT_CATTRIBUTES(unit) & CA_FLYER))
        return 0;                               /* flying over the ground */
    return Col_HazardDamage(kind);
}

int Col_HazardDamageFor(struct Unit *unit)
{
    return Col_HazardDamageAt(unit, unit->xPos, unit->yPos);
}

/* Replaces MakePoisonDamageTargetList (0x080259EC), the poison step at the start of a phase:
 * vanilla poison (1-3, can kill) plus the hazard under the unit (never below 1 HP). */
void Col_MakePoisonDamageTargetList(int faction)
{
    int i;

    InitTargets(0, 0);
    for (i = faction + 1; i < faction + 0x40; i++) {
        struct Unit *unit = GetUnit(i);
        int damage = 0, hazard;

        if (!UNIT_IS_VALID(unit) || (unit->state & (US_DEAD | US_NOT_DEPLOYED | US_RESCUED | US_BIT16)))
            continue;
        if (unit->statusIndex == UNIT_STATUS_POISON)
            damage = NextRN_N(3) + 1;
        hazard = Col_HazardDamageFor(unit);
        if (hazard) {
            damage += hazard;
            if (damage >= unit->curHP)
                damage = unit->curHP - 1;       /* hazards never kill (ARENA_SPEC 9) */
        }
        if (damage > 0)
            AddTarget(unit->xPos, unit->yPos, unit->index, damage);
    }
}

/* HP restoration loop (HPRestorationCalcLoop.event): heal % at the start of the unit's phase. */
int Col_SacredTileHeal(struct Unit *unit, int percent)
{
    if (Col_ArenaTileAt(unit->xPos, unit->yPos) == COL_TILE_SACRED)
        percent += SACRED_HEAL_PERCENT;
    return percent;
}

/* Pre-battle loop (PreBattleCalcLoop.event): weather lowers everyone's Hit alike. */
void Col_WeatherPreBattle(struct BattleUnit *a, struct BattleUnit *b)
{
    int penalty = Col_WeatherHitPenalty(Col_CurrentWeather());

    if (!penalty)
        return;
    a->battleHitRate -= penalty;
    if (a->battleHitRate < 0)
        a->battleHitRate = 0;
}

/* Replaces AiGetTerrainCombatPositionScoreComponent (0x0803E23C): FE8's terrain score for a tile
 * to attack from (Avoid + Def + Res bonuses), minus the hazard the active unit would take there,
 * plus a little for sacred tiles. */
int Col_AiTerrainScore(int x, int y)
{
    int terrain = gBmMapTerrain[y][x];
    int score = gActiveUnit->pClassData->pTerrainAvoidLookup[terrain]
              + gActiveUnit->pClassData->pTerrainDefenseLookup[terrain]
              + gActiveUnit->pClassData->pTerrainResistanceLookup[terrain];

    score -= 10 * Col_HazardDamageAt(gActiveUnit, x, y);
    if (Col_ArenaTileAt(x, y) == COL_TILE_SACRED)
        score += 10;
    return score;
}

/* Replaces AiCheckDangerAt (0x0803E448), FE8's filter for the tiles an AI move may end on
 * (AiTryMoveTowards and its variants): vanilla's danger threshold, and no hazard that would hurt
 * the moving unit. */
s8 Col_AiCheckDangerAt(int x, int y, u8 threshold)
{
    if (gBmMapOther[y][x] > threshold)
        return 0;
    if (Col_HazardDamageAt(gActiveUnit, x, y) > 0)
        return 0;
    return 1;
}
