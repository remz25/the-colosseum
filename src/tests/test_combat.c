/* On-target combat tests (Phase 3, spec 90): FE8's battle formulas as the Skill System builds
 * them, run through the game's own BattleGenerateSimulation / BattleGenerateReal on the battle
 * map. tests/run_tests.py runs these after booting into the COLISEUM battle chapter.
 *
 * Two map units (the first living blue and red) are rebuilt as skill-less test units (generic
 * character 0x80, classes without class skills), placed side by side away from everyone else
 * (no auras), and restored afterwards together with the two terrain tiles used.
 *
 * Formulas checked (vanilla FE8, Str/Mag split on):
 *   attack  = might + triangle dmg (+-1) + Str (Mag for magic weapons)
 *   defense = Def + terrain def (Res + terrain res vs magic)
 *   AS      = Spd - max(0, weight - Con)      hit   = Skl*2 + weapon hit + Lck/2 + triangle (+-15)
 *   avoid   = AS*2 + terrain avoid + Lck      crit  = Skl/2 + weapon crit    dodge = Lck
 *   battle hit = clamp(hit - avoid), battle crit = clamp(crit - dodge); doubles at AS +4;
 *   damage = attack - defense (min 0), x3 on a crit; a kill ends the battle. */
#include "coliseum.h"
#include "bmunit.h"
#include "bmitem.h"
#include "bmmap.h"
#include "bmbattle.h"

#define CHECK(cond) do { if (!(cond)) return __LINE__; } while (0)

#define TEST_CHAR          0x80    /* generic Grado Soldier: no personal skill */
#define CLASS_MERCENARY    0x0F    /* no class skills (skill_lists.event, ClassSkillEditor.csv) */
#define CLASS_MAGE         0x25
#define CLASS_FIGHTER      0x3F
#define CLASS_SOLDIER      0x4E
#define ITEM_IRON_SWORD    0x01
#define ITEM_IRON_LANCE    0x14
#define ITEM_IRON_AXE      0x1F
#define ITEM_FIRE          0x38
#define UNIT_MAGIC_OFFSET  0x3A    /* Str/Mag split: magic byte in struct Unit (MSG Strmag.event) */
#define TEST_CON           20

/* The Skill System moves the battle hit buffer (EngineHacks/SkillSystem/Internals/
 * repointbuffer.event): 8 bytes per hit, the first 4 being FE8's struct BattleHit. */
#define SS_BATTLE_HITS     0x0203AAC0
#define SS_BATTLE_HIT_SIZE 8
#define SS_BATTLE_HIT_MAX  31
#define BattleHitAt(i)     ((const struct BattleHit *)(SS_BATTLE_HITS + (i) * SS_BATTLE_HIT_SIZE))

#define AX 6                       /* test tiles: the middle of the arena */
#define AY 4
#define TX 7
#define TY 4

struct TestStats { s8 hp, pow, skl, spd, def, res, lck, mag; };

static struct Unit *FirstLiving(int faction)
{
    int i;
    for (i = faction + 1; i < faction + 0x40; i++) {
        struct Unit *u = GetUnit(i);
        if (u && u->pCharacterData && !(u->state & (US_DEAD | US_NOT_DEPLOYED)))
            return u;
    }
    return NULL;
}

static void Prep(struct Unit *u, int classId, const struct TestStats *s, int item, int x, int y, int terrain)
{
    int i;

    u->pCharacterData = GetCharacterData(TEST_CHAR);
    u->pClassData = GetClassData(classId);
    u->level = 10;
    u->exp = 0;
    u->maxHP = s->hp;
    u->curHP = s->hp;
    u->pow = s->pow;
    u->skl = s->skl;
    u->spd = s->spd;
    u->def = s->def;
    u->res = s->res;
    u->lck = s->lck;
    ((u8 *)u)[UNIT_MAGIC_OFFSET] = (u8)s->mag;
    u->conBonus = TEST_CON - UNIT_CON_BASE(u);
    u->statusIndex = 0;
    u->statusDuration = 0;
    for (i = 0; i < UNIT_ITEM_COUNT; i++)
        u->items[i] = 0;
    u->items[0] = MakeNewItem(item);
    for (i = 0; i < 8; i++)
        u->ranks[i] = 0;
    u->ranks[GetItemType(item)] = GetItemRequiredExp(item) ? GetItemRequiredExp(item) : 1;
    for (i = 0; i < UNIT_SUPPORT_MAX_COUNT; i++)
        u->supports[i] = 0;
    u->xPos = x;
    u->yPos = y;
    gBmMapTerrain[y][x] = (u8)terrain;
}

/* actor sA at (AX, AY) attacks target sT with its first item */
#define Simulate() BattleGenerateSimulation(sA, sT, AX, AY, 0)

static int Clamp100(int v)
{
    return v < 0 ? 0 : v > 100 ? 100 : v;
}

/* Hits in the generated battle: 1 = actor strikes, 2 = target strikes. */
static int CountHits(int retaliation)
{
    int i, n = 0;
    for (i = 0; i < SS_BATTLE_HIT_MAX; i++) {
        const struct BattleHit *h = BattleHitAt(i);
        if (h->info & BATTLE_HIT_INFO_END)
            break;
        if (!!(h->info & BATTLE_HIT_INFO_RETALIATION) == retaliation)
            n++;
    }
    return n;
}

static const struct TestStats kPlain = { 30, 8, 10, 7, 5, 2, 6, 0 };

/* Neutral sword vs sword on plains: every derived stat from the formulas. */
static int Test_Formulas(struct Unit *sA, struct Unit *sT)
{
    int mt = GetItemMight(ITEM_IRON_SWORD), hit = GetItemHit(ITEM_IRON_SWORD);
    int crit = GetItemCrit(ITEM_IRON_SWORD);
    const s8 *tdef = GetClassData(CLASS_MERCENARY)->pTerrainDefenseLookup;
    const s8 *tavo = GetClassData(CLASS_MERCENARY)->pTerrainAvoidLookup;

    Prep(sA, CLASS_MERCENARY, &kPlain, ITEM_IRON_SWORD, AX, AY, TERRAIN_PLAINS);
    Prep(sT, CLASS_MERCENARY, &kPlain, ITEM_IRON_SWORD, TX, TY, TERRAIN_PLAINS);
    Simulate();

    CHECK(gBattleActor.weapon && gBattleTarget.weapon);
    CHECK(gBattleActor.wTriangleHitBonus == 0 && gBattleActor.wTriangleDmgBonus == 0);
    CHECK(gBattleActor.battleAttack == kPlain.pow + mt);
    CHECK(gBattleTarget.battleDefense == kPlain.def + tdef[TERRAIN_PLAINS]);
    CHECK(gBattleActor.battleSpeed == kPlain.spd);                         /* Con 20: no penalty */
    CHECK(gBattleActor.battleHitRate == kPlain.skl * 2 + hit + kPlain.lck / 2);
    CHECK(gBattleTarget.battleAvoidRate == kPlain.spd * 2 + tavo[TERRAIN_PLAINS] + kPlain.lck);
    CHECK(gBattleActor.battleEffectiveHitRate
          == Clamp100(gBattleActor.battleHitRate - gBattleTarget.battleAvoidRate));
    CHECK(gBattleActor.battleCritRate == kPlain.skl / 2 + crit);
    CHECK(gBattleActor.battleEffectiveCritRate
          == Clamp100(gBattleActor.battleCritRate - kPlain.lck));
    CHECK(CountHits(0) == 1 && CountHits(1) == 1);                        /* no doubling */
    CHECK(BattleHitAt(0)->hpChange == (kPlain.pow + mt) - kPlain.def);  /* damage */
    return 0;
}

/* Sword beats axe: +15 hit / +1 damage for the sword, -15 / -1 for the axe. */
static int Test_Triangle(struct Unit *sA, struct Unit *sT)
{
    int swordMt = GetItemMight(ITEM_IRON_SWORD), axeMt = GetItemMight(ITEM_IRON_AXE);

    Prep(sA, CLASS_MERCENARY, &kPlain, ITEM_IRON_SWORD, AX, AY, TERRAIN_PLAINS);
    Prep(sT, CLASS_FIGHTER, &kPlain, ITEM_IRON_AXE, TX, TY, TERRAIN_PLAINS);
    Simulate();

    CHECK(gBattleActor.wTriangleHitBonus == 15 && gBattleActor.wTriangleDmgBonus == 1);
    CHECK(gBattleTarget.wTriangleHitBonus == -15 && gBattleTarget.wTriangleDmgBonus == -1);
    CHECK(gBattleActor.battleAttack == kPlain.pow + swordMt + 1);
    CHECK(gBattleTarget.battleAttack == kPlain.pow + axeMt - 1);
    CHECK(gBattleActor.battleHitRate == kPlain.skl * 2 + GetItemHit(ITEM_IRON_SWORD) + kPlain.lck / 2 + 15);
    CHECK(gBattleTarget.battleHitRate == kPlain.skl * 2 + GetItemHit(ITEM_IRON_AXE) + kPlain.lck / 2 - 15);

    /* lance beats sword */
    Prep(sT, CLASS_SOLDIER, &kPlain, ITEM_IRON_LANCE, TX, TY, TERRAIN_PLAINS);
    Simulate();
    CHECK(gBattleActor.wTriangleHitBonus == -15 && gBattleTarget.wTriangleHitBonus == 15);
    return 0;
}

/* Forest: the defender's class terrain bonuses apply to defense and avoid. */
static int Test_Terrain(struct Unit *sA, struct Unit *sT)
{
    const s8 *tdef = GetClassData(CLASS_MERCENARY)->pTerrainDefenseLookup;
    const s8 *tavo = GetClassData(CLASS_MERCENARY)->pTerrainAvoidLookup;

    CHECK(tdef[TERRAIN_FOREST] > 0 && tavo[TERRAIN_FOREST] > 0);          /* test is meaningful */
    Prep(sA, CLASS_MERCENARY, &kPlain, ITEM_IRON_SWORD, AX, AY, TERRAIN_PLAINS);
    Prep(sT, CLASS_MERCENARY, &kPlain, ITEM_IRON_SWORD, TX, TY, TERRAIN_FOREST);
    Simulate();

    CHECK(gBattleTarget.terrainDefense == tdef[TERRAIN_FOREST]);
    CHECK(gBattleTarget.battleDefense == kPlain.def + tdef[TERRAIN_FOREST]);
    CHECK(gBattleTarget.battleAvoidRate == kPlain.spd * 2 + tavo[TERRAIN_FOREST] + kPlain.lck);
    return 0;
}

/* Doubling at AS difference 4, not 3; weapon weight over Con slows. */
static int Test_Doubling(struct Unit *sA, struct Unit *sT)
{
    struct TestStats fast = kPlain, heavy = kPlain;
    int wt = GetItemWeight(ITEM_IRON_AXE);

    fast.spd = kPlain.spd + 4;
    Prep(sA, CLASS_MERCENARY, &fast, ITEM_IRON_SWORD, AX, AY, TERRAIN_PLAINS);
    Prep(sT, CLASS_MERCENARY, &kPlain, ITEM_IRON_SWORD, TX, TY, TERRAIN_PLAINS);
    Simulate();
    CHECK(CountHits(0) == 2 && CountHits(1) == 1);

    fast.spd = kPlain.spd + 3;
    Prep(sA, CLASS_MERCENARY, &fast, ITEM_IRON_SWORD, AX, AY, TERRAIN_PLAINS);
    Simulate();
    CHECK(CountHits(0) == 1 && CountHits(1) == 1);

    /* the target doubles back */
    Prep(sA, CLASS_MERCENARY, &kPlain, ITEM_IRON_SWORD, AX, AY, TERRAIN_PLAINS);
    fast.spd = kPlain.spd + 4;
    Prep(sT, CLASS_MERCENARY, &fast, ITEM_IRON_SWORD, TX, TY, TERRAIN_PLAINS);
    Simulate();
    CHECK(CountHits(0) == 1 && CountHits(1) == 2);

    /* weight: Con 20 - 5 = 15 against the axe's weight */
    heavy.spd = 12;
    Prep(sA, CLASS_FIGHTER, &heavy, ITEM_IRON_AXE, AX, AY, TERRAIN_PLAINS);
    sA->conBonus = 5 - UNIT_CON_BASE(sA);
    Simulate();
    CHECK(wt > 5);
    CHECK(gBattleActor.battleSpeed == (heavy.spd - (wt - 5) > 0 ? heavy.spd - (wt - 5) : 0));
    return 0;
}

/* Str/Mag split: magic uses Mag against Res; Str doesn't matter. */
static int Test_Magic(struct Unit *sA, struct Unit *sT)
{
    struct TestStats mage = kPlain;
    mage.pow = 0;
    mage.mag = 9;

    Prep(sA, CLASS_MAGE, &mage, ITEM_FIRE, AX, AY, TERRAIN_PLAINS);
    Prep(sT, CLASS_MERCENARY, &kPlain, ITEM_IRON_SWORD, TX, TY, TERRAIN_PLAINS);
    Simulate();

    CHECK(gBattleActor.weapon != 0);
    CHECK(gBattleActor.battleAttack == mage.mag + GetItemMight(ITEM_FIRE));
    CHECK(gBattleTarget.battleDefense == kPlain.res + gBattleTarget.terrainResistance);
    CHECK(BattleHitAt(0)->hpChange == mage.mag + GetItemMight(ITEM_FIRE) - kPlain.res);
    return 0;
}

/* No negative damage; a kill ends the battle before the counter; a crit triples the damage. */
static int Test_DamageAndDeath(struct Unit *sA, struct Unit *sT)
{
    struct TestStats wall = kPlain, weak = kPlain, killer = kPlain, pos;
    int base, tries, sawCrit = 0, sawNormal = 0;

    wall.def = 30;
    Prep(sA, CLASS_MERCENARY, &kPlain, ITEM_IRON_SWORD, AX, AY, TERRAIN_PLAINS);
    Prep(sT, CLASS_MERCENARY, &wall, ITEM_IRON_SWORD, TX, TY, TERRAIN_PLAINS);
    Simulate();
    CHECK(gBattleActor.battleAttack < wall.def);
    CHECK(BattleHitAt(0)->hpChange == 0);

    weak.hp = 5;
    Prep(sT, CLASS_MERCENARY, &weak, ITEM_IRON_SWORD, TX, TY, TERRAIN_PLAINS);
    Simulate();
    CHECK(gBattleTarget.unit.curHP == 0);
    CHECK(BattleHitAt(0)->info & BATTLE_HIT_INFO_KILLS_TARGET);
    CHECK(CountHits(1) == 0);                                              /* dead: no counter */

    /* real battles (RNG): certain hit (Skl 99), crit 49%; every crit deals 3x, every other hit 1x */
    killer.skl = 99;
    killer.lck = 0;
    pos = kPlain;
    pos.lck = 0;
    pos.hp = 60;
    Prep(sA, CLASS_MERCENARY, &killer, ITEM_IRON_SWORD, AX, AY, TERRAIN_PLAINS);
    Prep(sT, CLASS_MERCENARY, &pos, ITEM_IRON_SWORD, TX, TY, TERRAIN_PLAINS);
    base = killer.pow + GetItemMight(ITEM_IRON_SWORD) - pos.def;
    for (tries = 0; tries < 40 && !(sawCrit && sawNormal); tries++) {
        BattleGenerateReal(sA, sT);
        CHECK(gBattleActor.battleEffectiveHitRate == 100);
        CHECK(!(BattleHitAt(0)->attributes & BATTLE_HIT_ATTR_MISS));
        if (BattleHitAt(0)->attributes & BATTLE_HIT_ATTR_CRIT) {
            CHECK(BattleHitAt(0)->hpChange == 3 * base);
            sawCrit = 1;
        } else {
            CHECK(BattleHitAt(0)->hpChange == base);
            sawNormal = 1;
        }
    }
    CHECK(sawCrit && sawNormal);                                          /* 1 in 10^11 to fail */
    return 0;
}

/* test_units.c */
int Test_Pool(struct Unit *, struct Unit *);
int Test_LoadPoolUnits(struct Unit *, struct Unit *);
int Test_LevelCap(struct Unit *, struct Unit *);
int Test_NoStatCaps(struct Unit *, struct Unit *);
int Test_StatChoices(struct Unit *, struct Unit *);
int Test_PendingChoices(struct Unit *, struct Unit *);
int Test_NoDurability(struct Unit *, struct Unit *);
/* test_skills.c */
int Test_ClassSkillInSlot(struct Unit *, struct Unit *);
int Test_ThreeSlots(struct Unit *, struct Unit *);
int Test_NoDuplicateSkills(struct Unit *, struct Unit *);
int Test_SkillPrerequisites(struct Unit *, struct Unit *);
int Test_SkillCatalog(struct Unit *, struct Unit *);
int Test_SkillOffers(struct Unit *, struct Unit *);
int Test_RunClearsSkills(struct Unit *, struct Unit *);
int Test_EnemySkills(struct Unit *, struct Unit *);
/* test_weapons.c */
int Test_FusionRecipes(struct Unit *, struct Unit *);
int Test_Fuse(struct Unit *, struct Unit *);
int Test_Transfer(struct Unit *, struct Unit *);
int Test_FastProficiency(struct Unit *, struct Unit *);
int Test_SpecialWeapons(struct Unit *, struct Unit *);
/* test_shop.c */
int Test_ShopPrices(struct Unit *, struct Unit *);
int Test_ShopStock(struct Unit *, struct Unit *);
int Test_ShopBuy(struct Unit *, struct Unit *);
int Test_HealTeam(struct Unit *, struct Unit *);
int Test_BattleGold(struct Unit *, struct Unit *);
/* test_roster.c */
int Test_RollRecruits(struct Unit *, struct Unit *);
int Test_RecruitEmptySlot(struct Unit *, struct Unit *);
int Test_RecruitReplace(struct Unit *, struct Unit *);
int Test_RecruitBuild(struct Unit *, struct Unit *);
int Test_Deployment(struct Unit *, struct Unit *);
int Test_RunLost(struct Unit *, struct Unit *);
int Test_NewGameClearsRun(struct Unit *, struct Unit *);

typedef int (*ColMapTestFn)(struct Unit *actor, struct Unit *target);
static const ColMapTestFn kMapTests[] = {
    Test_Formulas, Test_Triangle, Test_Terrain, Test_Doubling, Test_Magic, Test_DamageAndDeath,
    Test_Pool, Test_LoadPoolUnits, Test_LevelCap, Test_NoStatCaps, Test_StatChoices,
    Test_PendingChoices,
    Test_RollRecruits, Test_RecruitEmptySlot, Test_RecruitReplace, Test_RecruitBuild,
    Test_Deployment, Test_RunLost, Test_NewGameClearsRun, Test_NoDurability,
    Test_ClassSkillInSlot, Test_ThreeSlots, Test_NoDuplicateSkills, Test_SkillPrerequisites,
    Test_SkillCatalog, Test_SkillOffers, Test_RunClearsSkills, Test_EnemySkills,
    Test_FusionRecipes, Test_Fuse, Test_Transfer, Test_FastProficiency, Test_SpecialWeapons,
    Test_ShopPrices, Test_ShopStock, Test_ShopBuy, Test_HealTeam, Test_BattleGold,
};

int ColTest_MapCount(void)
{
    return sizeof(kMapTests) / sizeof(kMapTests[0]);
}

static void CopyBytes(void *dst, const void *src, unsigned n)
{
    u8 *d = dst;
    const u8 *s = src;
    while (n--)
        *d++ = *s++;
}

/* Runs map test i with the two units and their tiles restored afterwards.
 * -1: bad index, -2: no blue or red unit on the map (not in a battle). */
int ColTest_MapRun(int i)
{
    struct Unit *sA, *sT, savedA, savedT;
    struct ColRunState savedRun;
    u8 terrainA, terrainT;
    int result;

    if (i < 0 || i >= ColTest_MapCount())
        return -1;
    sA = FirstLiving(FACTION_BLUE);
    sT = FirstLiving(FACTION_RED);
    if (!sA || !sT || !gBmMapTerrain)
        return -2;
    CopyBytes(&savedA, sA, sizeof(savedA));
    CopyBytes(&savedT, sT, sizeof(savedT));
    CopyBytes(&savedRun, &gColRun, sizeof(savedRun));
    terrainA = gBmMapTerrain[AY][AX];
    terrainT = gBmMapTerrain[TY][TX];

    result = kMapTests[i](sA, sT);

    gBmMapTerrain[AY][AX] = terrainA;
    gBmMapTerrain[TY][TX] = terrainT;
    CopyBytes(sA, &savedA, sizeof(savedA));
    CopyBytes(sT, &savedT, sizeof(savedT));
    CopyBytes(&gColRun, &savedRun, sizeof(savedRun));
    return result;
}
