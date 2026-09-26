/* On-target tests for Phase 9 (spec 33-36, docs/RELICS.md): relic pool, equip / unequip /
 * transfer / bag, flat and percentage stats, battle rates, conditions, adjacent-ally auras,
 * damage percentages, HP cost per attack, gold, leaving the run, saves.
 *
 * Map tests (ColTest_MapRun restores gColRun and the two units). Relics are worn by pool
 * characters, so the actor is turned into one (Gilliam, or Lute for magic). Combat checks
 * compare the same battle with and without the relic, so the character's own skills cancel out. */
#include "coliseum.h"
#include "bmunit.h"
#include "bmitem.h"
#include "bmmap.h"
#include "bmbattle.h"
#include "bmsave.h"

#define CHECK(cond) do { if (!(cond)) return __LINE__; } while (0)

#define TEST_CHAR          0x80
#define POOL_GILLIAM       3
#define POOL_LUTE          9
#define POOL_GERIK         1
#define CLASS_MERCENARY    0x0F
#define CLASS_MAGE         0x25
#define ITEM_IRON_SWORD    0x01
#define ITEM_FIRE          0x38
#define UNIT_MAGIC_OFFSET  0x3A
#define TEST_CON           20
#define SS_BATTLE_HITS     0x0203AAC0
#define HitAt(i)           ((const u8 *)(SS_BATTLE_HITS + (i) * 8))
#define HitDamage(i)       (((const struct BattleHit *)HitAt(i))->hpChange)
#define HitAttackerHp(i)   ((s8)HitAt(i)[5])
#define AX 6
#define AY 4
#define TX 7
#define TY 4

enum {
    IRON_HEART = 1, WARRIORS_BAND, SWIFT_FEATHER, SCHOLARS_LENS, EAGLE_EYE, STURDY_BOOTS,
    BLOODIED_BAND, GUARDIANS_CREST, MAGES_RING, GOLDEN_THREAD, BERSERKERS_FANG,
    BLOOD_PACT, WIND_SOUL, ARCANE_BLOOD, FORTRESS_HEART,
};

/* MSG getters (asm labels without the Thumb bit): the full modified stat, as the game reads it */
extern const u8 prMagGetter[], prMovGetter[];
#define GETTER(label, u) (((int (*)(struct Unit *))((u32)(label) | 1))(u))

struct S { s8 hp, pow, skl, spd, def, res, lck, mag; };
static const struct S kPlain = { 30, 8, 10, 7, 5, 2, 6, 0 };

static void Prep(struct Unit *u, int charId, int classId, const struct S *s, int item, int x, int y)
{
    int i;

    u->pCharacterData = GetCharacterData(charId);
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
    u->movBonus = 0;
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
    gBmMapTerrain[y][x] = TERRAIN_PLAINS;
}

static void ClearRelics(void)
{
    int p, s;
    for (p = 0; p < COL_POOL_SIZE; p++)
        for (s = 0; s < COL_RELIC_SLOTS; s++)
            gColRun.relics[p][s] = 0;
    for (p = 0; p < COL_RELIC_BAG; p++)
        gColRun.relicBag[p] = 0;
}

static void Wear(int pool, int a, int b)
{
    gColRun.relics[pool][0] = (u8)a;
    gColRun.relics[pool][1] = (u8)b;
}

#define Simulate(a, t) BattleGenerateSimulation(a, t, AX, AY, 0)

/* ---- the pool ---- */

int Test_RelicPool(struct Unit *sA, struct Unit *sT)
{
    int id, m, count[COL_RELIC_RARITY_COUNT + 1] = { 0 }, seen[COL_RELIC_RARITY_COUNT + 1] = { 0 };

    CHECK(Col_RelicCount() == 15);
    CHECK(Col_RelicDef(0) == NULL && Col_RelicDef(16) == NULL);
    for (id = 1; id <= Col_RelicCount(); id++) {
        const struct ColRelicDef *d = Col_RelicDef(id);
        CHECK(d && d->name[0] && d->rarity >= 1 && d->rarity <= COL_RELIC_RARITY_COUNT);
        CHECK(d->mods[0].kind != COL_RM_END);
        for (m = 0; m < COL_RELIC_MODS && d->mods[m].kind; m++) {
            char buf[40];
            CHECK(d->mods[m].kind < COL_RM_KIND_COUNT && d->mods[m].amount != 0);
            Col_RelicModText(&d->mods[m], buf);
            CHECK(buf[0]);                                         /* every effect has a text */
        }
        count[d->rarity]++;
    }
    CHECK(count[1] == 6 && count[2] == 5 && count[3] == 3 && count[4] == 1 && count[5] == 0 && count[6] == 0);
    CHECK(Col_RelicRarityName(6)[0] == 'M');                      /* Mythic is supported */
    for (m = 0; m < 400; m++) {
        id = Col_RelicRoll();
        CHECK(id >= 1 && id <= Col_RelicCount());
        seen[Col_RelicDef(id)->rarity]++;
    }
    CHECK(seen[1] > seen[3] && seen[3] > 0 && seen[4] > 0 && !seen[5] && !seen[6]);
    return 0;
}

/* Effect texts: positive and negative, flat and percent, conditions shown separately. */
int Test_RelicTexts(struct Unit *sA, struct Unit *sT)
{
    char buf[40];
    const char *s;
    int good;

#define STREQ(a, b) ({ const char *x_ = (a), *y_ = (b); while (*x_ && *x_ == *y_) x_++, y_++; *x_ == *y_; })
    good = Col_RelicModText(&Col_RelicDef(IRON_HEART)->mods[0], buf);
    CHECK(good && STREQ(buf, "+5 Def"));
    good = Col_RelicModText(&Col_RelicDef(IRON_HEART)->mods[1], buf);
    CHECK(!good && STREQ(buf, "-3 Spd"));
    good = Col_RelicModText(&Col_RelicDef(WIND_SOUL)->mods[1], buf);
    CHECK(!good && STREQ(buf, "-15% Def"));
    good = Col_RelicModText(&Col_RelicDef(FORTRESS_HEART)->mods[0], buf);
    CHECK(good && STREQ(buf, "-20% damage taken"));
    good = Col_RelicModText(&Col_RelicDef(BLOOD_PACT)->mods[2], buf);
    CHECK(!good && STREQ(buf, "Lose 2 HP per attack"));
    good = Col_RelicModText(&Col_RelicDef(GUARDIANS_CREST)->mods[0], buf);
    CHECK(good && STREQ(buf, "Adjacent allies +3 Def"));
    good = Col_RelicModText(&Col_RelicDef(GOLDEN_THREAD)->mods[0], buf);
    CHECK(good && STREQ(buf, "+25% battle gold"));
    s = Col_RelicDef(BLOODIED_BAND)->name;
    CHECK(STREQ(s, "Bloodied Band") && Col_RelicDef(BLOODIED_BAND)->mods[0].cond == COL_RC_BELOW_HALF_HP);
    return 0;
}

/* ---- equip, unequip, transfer, bag ---- */

int Test_RelicEquip(struct Unit *sA, struct Unit *sT)
{
    int i;

    Col_RunNew(3);
    CHECK(Col_RelicBagCount() == 0);
    CHECK(Col_RelicBagAdd(IRON_HEART) && Col_RelicBagAdd(EAGLE_EYE) && !Col_RelicBagAdd(0) && !Col_RelicBagAdd(99));
    CHECK(Col_RelicBagCount() == 2);

    /* equip from the bag; the old relic takes the new one's place in the bag */
    CHECK(Col_RelicEquipFromBag(POOL_GILLIAM, 0, 0));
    CHECK(gColRun.relics[POOL_GILLIAM][0] == IRON_HEART && Col_RelicBagCount() == 1);
    CHECK(Col_RelicEquipFromBag(POOL_GILLIAM, 0, 1));
    CHECK(gColRun.relics[POOL_GILLIAM][0] == EAGLE_EYE && Col_RelicBagCount() == 1);
    CHECK(!Col_RelicEquipFromBag(POOL_GILLIAM, 2, 0) && !Col_RelicEquipFromBag(POOL_GILLIAM, 1, 5));

    /* transfer to another character, then a swap */
    CHECK(Col_RelicMove(POOL_GILLIAM, 0, POOL_LUTE, 1));
    CHECK(gColRun.relics[POOL_GILLIAM][0] == 0 && gColRun.relics[POOL_LUTE][1] == EAGLE_EYE);
    for (i = 0; i < COL_RELIC_BAG && !gColRun.relicBag[i]; i++)
        ;
    CHECK(Col_RelicEquipFromBag(POOL_GILLIAM, 0, i));                  /* Iron Heart */
    CHECK(Col_RelicMove(POOL_LUTE, 1, POOL_GILLIAM, 0));               /* swap */
    CHECK(gColRun.relics[POOL_GILLIAM][0] == EAGLE_EYE && gColRun.relics[POOL_LUTE][1] == IRON_HEART);
    CHECK(!Col_RelicMove(POOL_GILLIAM, 1, POOL_LUTE, 0));              /* empty source */

    /* unequip: back into the bag, never lost */
    CHECK(Col_RelicUnequip(POOL_GILLIAM, 0) && gColRun.relics[POOL_GILLIAM][0] == 0);
    CHECK(Col_RelicBagCount() == 1 && !Col_RelicUnequip(POOL_GILLIAM, 0));

    /* a full bag refuses more; unequip is refused rather than losing the relic */
    for (i = 0; i < COL_RELIC_BAG; i++)
        Col_RelicBagAdd(SWIFT_FEATHER);
    CHECK(Col_RelicBagCount() == COL_RELIC_BAG && !Col_RelicBagAdd(SWIFT_FEATHER));
    CHECK(!Col_RelicUnequip(POOL_LUTE, 1) && gColRun.relics[POOL_LUTE][1] == IRON_HEART);

    /* duplicates: the same relic twice on one character stacks */
    ClearRelics();
    Prep(sA, gColPool[POOL_GILLIAM].charId, CLASS_MERCENARY, &kPlain, ITEM_IRON_SWORD, AX, AY);
    i = GetUnitDefense(sA);
    Wear(POOL_GILLIAM, IRON_HEART, IRON_HEART);
    CHECK(GetUnitDefense(sA) == i + 10);
    CHECK(Col_UnitRelic(sA, 0) == IRON_HEART && Col_UnitRelic(sA, 1) == IRON_HEART && Col_UnitRelic(sA, 2) == 0);

    /* enemies never wear relics, even with a pool character's ID */
    sT->pCharacterData = GetCharacterData(gColPool[POOL_GILLIAM].charId);
    CHECK(Col_UnitRelic(sT, 0) == 0 && Col_RelicModTotal(sT, COL_RM_DEF) == 0);
    return 0;
}

/* The wearer leaves the run (death, replacement): its relics go back to the bag. */
int Test_RelicsLeaveRun(struct Unit *sA, struct Unit *sT)
{
    int gone;

    /* the replaced character must not be on the map (replacement clears its unit) */
    for (gone = 0; gone < COL_POOL_SIZE && GetUnitFromCharIdAndFaction(gColPool[gone].charId, FACTION_BLUE); gone++)
        ;
    CHECK(gone < COL_POOL_SIZE && gone != POOL_GILLIAM);
    Col_RunNew(4);
    gColRun.roster[0] = POOL_GILLIAM; gColRun.roster[1] = (u8)gone;
    gColRun.rosterCount = 2;
    Wear(POOL_GILLIAM, IRON_HEART, BLOOD_PACT);
    Wear(gone, MAGES_RING, 0);
    Col_RosterRemoveDead(0);
    CHECK(gColRun.relics[POOL_GILLIAM][0] == 0 && gColRun.relics[POOL_GILLIAM][1] == 0);
    CHECK(Col_RelicBagCount() == 2);
    Col_RemoveFromRoster(1);
    CHECK(gColRun.relics[gone][0] == 0 && Col_RelicBagCount() == 3);
    Col_RunNew(5);                                                  /* a new run has none */
    CHECK(Col_RelicBagCount() == 0);
    return 0;
}

/* Relics survive the save (SRAM chunk), and a v4 save (before relics) still loads. */
int Test_RelicSave(struct Unit *sA, struct Unit *sT)
{
    u8 *sram = (u8 *)GetSaveWriteAddr(2) + 0x11F0;                 /* game save chunk (ExModularSave.event) */

    Col_RunNew(6);
    Wear(POOL_LUTE, ARCANE_BLOOD, FORTRESS_HEART);
    Col_RelicBagAdd(GOLDEN_THREAD);
    Col_SaveRunChunk(sram, COL_RUN_SIZE);
    Col_RunClear();
    Col_LoadRunChunk(sram, COL_RUN_SIZE);
    CHECK(Col_RunIsValid() && gColRun.active);
    CHECK(gColRun.relics[POOL_LUTE][0] == ARCANE_BLOOD && gColRun.relics[POOL_LUTE][1] == FORTRESS_HEART);
    CHECK(gColRun.relicBag[0] == GOLDEN_THREAD);

    Col_RunNew(7);                                                  /* a v4 run: upgraded, kept */
    Col_AddGold(55);
    gColRun.version = 4;
    Col_SaveRunChunk(sram, COL_RUN_SIZE);
    Col_RunClear();
    Col_LoadRunChunk(sram, COL_RUN_SIZE);
    CHECK(Col_RunIsValid() && gColRun.active && gColRun.gold == 55 && gColRun.version == COL_RUN_VERSION);
    CHECK(Col_RelicBagCount() == 0);
    return 0;
}

/* ---- stats ---- */

/* Flat stat relics through the game's stat getters (stat screen and combat both use them). */
int Test_RelicFlatStats(struct Unit *sA, struct Unit *sT)
{
    int str, mag, spd, def, res, lck, mov;

    ClearRelics();
    Prep(sA, gColPool[POOL_GILLIAM].charId, CLASS_MERCENARY, &kPlain, ITEM_IRON_SWORD, AX, AY);
    ((u8 *)sA)[UNIT_MAGIC_OFFSET] = 4;
    sA->res = 6;                                                    /* above every drawback: no 0 floor */
    str = GetUnitPower(sA); mag = GETTER(prMagGetter, sA); spd = GetUnitSpeed(sA);
    def = GetUnitDefense(sA); res = GetUnitResistance(sA); lck = GetUnitLuck(sA); mov = GETTER(prMovGetter, sA);

    Wear(POOL_GILLIAM, IRON_HEART, WARRIORS_BAND);
    CHECK(GetUnitDefense(sA) == def + 5 && GetUnitSpeed(sA) == spd - 3);
    CHECK(GetUnitPower(sA) == str + 5 && GetUnitResistance(sA) == res - 3);
    Wear(POOL_GILLIAM, SWIFT_FEATHER, SCHOLARS_LENS);
    CHECK(GetUnitSpeed(sA) == spd + 5 && GetUnitDefense(sA) == def - 3);
    CHECK(GETTER(prMagGetter, sA) == mag + 5 && GetUnitPower(sA) == str - 3);
    Wear(POOL_GILLIAM, STURDY_BOOTS, FORTRESS_HEART);
    CHECK(GETTER(prMovGetter, sA) == mov && GetUnitDefense(sA) == def - 5 && GetUnitSpeed(sA) == spd - 5);
    Wear(POOL_GILLIAM, STURDY_BOOTS, 0);
    CHECK(GETTER(prMovGetter, sA) == mov + 1);
    Wear(POOL_GILLIAM, GOLDEN_THREAD, GUARDIANS_CREST);
    CHECK(GetUnitLuck(sA) == lck - 5 && GetUnitSpeed(sA) == spd - 2);
    CHECK(GetUnitDefense(sA) == def);                              /* the wearer's own crest: no Def */
    Wear(POOL_GILLIAM, IRON_HEART, 0);                             /* stats never below 0 */
    sA->spd = 1;
    CHECK(GetUnitSpeed(sA) == 0);
    return 0;
}

/* Percentages: added together, applied after flat changes, rounded to nearest. */
int Test_RelicPercentStats(struct Unit *sA, struct Unit *sT)
{
    struct S s = kPlain;

    CHECK(Col_RelicPercent(10, 20) == 12 && Col_RelicPercent(7, -15) == 6 && Col_RelicPercent(1, -20) == 1);
    CHECK(Col_RelicPercent(15, 15) == 17 && Col_RelicPercent(5, 10) == 6 && Col_RelicPercent(20, 25) == 25);
    CHECK(Col_RelicPercent(0, 50) == 0 && Col_RelicPercent(3, -15) == 3 && Col_RelicPercent(4, -15) == 3);

    ClearRelics();
    s.spd = 10; s.def = 7; s.pow = 8;
    Prep(sA, gColPool[POOL_GILLIAM].charId, CLASS_MERCENARY, &s, ITEM_IRON_SWORD, AX, AY);
    CHECK(GetUnitSpeed(sA) == 10 && GetUnitDefense(sA) == 7 && GetUnitPower(sA) == 8);   /* test is meaningful */
    Wear(POOL_GILLIAM, WIND_SOUL, 0);
    CHECK(GetUnitSpeed(sA) == 12 && GetUnitDefense(sA) == 6);      /* 10 x 1.2; 7 x 0.85 = 5.95 */
    Wear(POOL_GILLIAM, BLOOD_PACT, 0);
    CHECK(GetUnitPower(sA) == 10 && GetUnitSpeed(sA) == 11);       /* 9.6; 11 */
    Wear(POOL_GILLIAM, BLOOD_PACT, WIND_SOUL);
    CHECK(GetUnitSpeed(sA) == 13);                                 /* +10% +20% = +30% */
    Wear(POOL_GILLIAM, SWIFT_FEATHER, WIND_SOUL);
    CHECK(GetUnitSpeed(sA) == 18 && GetUnitDefense(sA) == 3);      /* (10+5) x 1.2; (7-3) x 0.85 = 3.4 */
    Wear(POOL_GILLIAM, WIND_SOUL, WIND_SOUL);
    CHECK(GetUnitDefense(sA) == 5);                                /* -30% of 7 = 4.9 */
    return 0;
}

/* ---- combat ---- */

/* Hit / Avoid / Crit and Str in real battle calculations. */
int Test_RelicBattleRates(struct Unit *sA, struct Unit *sT)
{
    int hit, avo, crit, atk, def;

    ClearRelics();
    Prep(sA, gColPool[POOL_GILLIAM].charId, CLASS_MERCENARY, &kPlain, ITEM_IRON_SWORD, AX, AY);
    Prep(sT, TEST_CHAR, CLASS_MERCENARY, &kPlain, ITEM_IRON_SWORD, TX, TY);
    Simulate(sA, sT);
    hit = gBattleActor.battleHitRate; avo = gBattleActor.battleAvoidRate; crit = gBattleActor.battleCritRate;
    atk = gBattleActor.battleAttack; def = gBattleActor.battleDefense;

    Wear(POOL_GILLIAM, EAGLE_EYE, 0);
    Simulate(sA, sT);
    CHECK(gBattleActor.battleHitRate == hit + 10 && gBattleActor.battleAvoidRate == avo - 5);
    CHECK(gBattleActor.battleCritRate == crit && gBattleActor.battleAttack == atk);
    CHECK(gBattleTarget.battleHitRate > 0);

    Wear(POOL_GILLIAM, BERSERKERS_FANG, 0);
    Simulate(sA, sT);
    CHECK(gBattleActor.battleCritRate == crit + 5 && gBattleActor.battleAttack == atk + 5);
    CHECK(gBattleActor.battleDefense == def - 5 && gBattleActor.battleHitRate == hit);
    CHECK(HitDamage(0) == atk + 5 - gBattleTarget.battleDefense);

    /* the defender wearing Eagle Eye gets its rates too */
    Wear(POOL_GILLIAM, EAGLE_EYE, 0);
    Prep(sA, gColPool[POOL_GILLIAM].charId, CLASS_MERCENARY, &kPlain, ITEM_IRON_SWORD, TX, TY);
    Prep(sT, TEST_CHAR, CLASS_MERCENARY, &kPlain, ITEM_IRON_SWORD, AX, AY);
    Simulate(sT, sA);
    CHECK(gBattleTarget.battleHitRate == hit + 10 && gBattleTarget.battleAvoidRate == avo - 5);
    return 0;
}

/* Bloodied Band: +5 Str and +5 Crit only while below 50% HP; switches as HP changes. */
int Test_RelicBloodiedBand(struct Unit *sA, struct Unit *sT)
{
    int str, crit, atk;

    ClearRelics();
    Prep(sA, gColPool[POOL_GILLIAM].charId, CLASS_MERCENARY, &kPlain, ITEM_IRON_SWORD, AX, AY);
    Prep(sT, TEST_CHAR, CLASS_MERCENARY, &kPlain, ITEM_IRON_SWORD, TX, TY);
    Simulate(sA, sT);
    crit = gBattleActor.battleCritRate; atk = gBattleActor.battleAttack;
    str = GetUnitPower(sA);

    Wear(POOL_GILLIAM, BLOODIED_BAND, 0);
    CHECK(GetUnitPower(sA) == str);                                 /* full HP */
    sA->curHP = 15;                                                 /* exactly 50%: not below */
    CHECK(GetUnitPower(sA) == str);
    Simulate(sA, sT);
    CHECK(gBattleActor.battleCritRate == crit && gBattleActor.battleAttack == atk);
    sA->curHP = 14;                                                 /* below 50% */
    CHECK(GetUnitPower(sA) == str + 5);
    Simulate(sA, sT);
    CHECK(gBattleActor.battleCritRate == crit + 5 && gBattleActor.battleAttack == atk + 5);
    sA->curHP = 30;                                                 /* healed: gone again */
    CHECK(GetUnitPower(sA) == str);
    Simulate(sA, sT);
    CHECK(gBattleActor.battleCritRate == crit && gBattleActor.battleAttack == atk);
    return 0;
}

static struct Unit *OtherBlue(struct Unit *not)
{
    int i;
    for (i = FACTION_BLUE + 1; i < FACTION_BLUE + 0x40; i++) {
        struct Unit *u = GetUnit(i);
        if (u && u != not && u->pCharacterData && !(u->state & (US_DEAD | US_NOT_DEPLOYED)))
            return u;
    }
    return NULL;
}

/* Guardian's Crest: adjacent allies +3 Def against physical attacks; not the wearer; not
 * against magic; gone when the ally is not adjacent. */
int Test_RelicGuardian(struct Unit *sA, struct Unit *sT)
{
    struct Unit *ally = OtherBlue(sA), saved;
    u8 cell, *p;
    const struct CharacterData *allyChar;
    int def, i, result = 0;

    if (!ally)
        return __LINE__;
    for (p = (u8 *)&saved, i = 0; i < (int)sizeof(saved); i++)
        p[i] = ((u8 *)ally)[i];
    cell = gBmMapUnit[TY + 1][TX];
    allyChar = ally->pCharacterData;

    /* sA (Gilliam) defends at (TX, TY); the ally (as Gerik) may stand below it at (TX, TY + 1) */
    ClearRelics();
    ally->pCharacterData = GetCharacterData(gColPool[POOL_GERIK].charId);
    Prep(sA, gColPool[POOL_GILLIAM].charId, CLASS_MERCENARY, &kPlain, ITEM_IRON_SWORD, TX, TY);
    Prep(sT, TEST_CHAR, CLASS_MERCENARY, &kPlain, ITEM_IRON_SWORD, AX, AY);
    gBmMapUnit[TY + 1][TX] = 0;
    Simulate(sT, sA);
    def = gBattleTarget.battleDefense;

    Wear(POOL_GERIK, GUARDIANS_CREST, 0);
    Simulate(sT, sA);
    if (gBattleTarget.battleDefense != def) { result = __LINE__; goto out; }     /* not adjacent */
    gBmMapUnit[TY + 1][TX] = ally->index;
    Simulate(sT, sA);
    if (gBattleTarget.battleDefense != def + 3) { result = __LINE__; goto out; } /* adjacent ally */
    if (HitDamage(0) != gBattleActor.battleAttack - (def + 3)) { result = __LINE__; goto out; }

    Wear(POOL_GERIK, 0, 0);                                         /* the wearer itself: nothing */
    Wear(POOL_GILLIAM, GUARDIANS_CREST, 0);
    Simulate(sT, sA);
    if (gBattleTarget.battleDefense != def) { result = __LINE__; goto out; }

    Wear(POOL_GILLIAM, 0, 0);                                       /* magic: Res, no bonus */
    Wear(POOL_GERIK, GUARDIANS_CREST, 0);
    {
        struct S mage = kPlain;
        mage.mag = 9;
        Prep(sT, TEST_CHAR, CLASS_MAGE, &mage, ITEM_FIRE, AX, AY);
    }
    Simulate(sT, sA);
    if (gBattleTarget.battleDefense != kPlain.res + gBattleTarget.terrainResistance) { result = __LINE__; goto out; }
out:
    gBmMapUnit[TY + 1][TX] = cell;
    for (p = (u8 *)&saved, i = 0; i < (int)sizeof(saved); i++)
        ((u8 *)ally)[i] = p[i];
    ally->pCharacterData = allyChar;
    return result;
}

/* Mage's Ring / Arcane Blood: magic damage only; Fortress Heart: damage the wearer receives. */
int Test_RelicDamage(struct Unit *sA, struct Unit *sT)
{
    struct S mage = kPlain, wall = kPlain, hitter = kPlain;
    int base, phys, counter;

    ClearRelics();
    mage.mag = 15; mage.pow = 0;
    wall.res = 0; wall.hp = 60;
    Prep(sA, gColPool[POOL_LUTE].charId, CLASS_MAGE, &mage, ITEM_FIRE, AX, AY);
    Prep(sT, TEST_CHAR, CLASS_MERCENARY, &wall, ITEM_IRON_SWORD, TX, TY);
    Simulate(sA, sT);
    base = HitDamage(0);
    CHECK(base == gBattleActor.battleAttack - gBattleTarget.battleDefense && base >= 15);

    Wear(POOL_LUTE, MAGES_RING, 0);
    Simulate(sA, sT);
    CHECK(HitDamage(0) == Col_RelicPercent(base, 15));
    Wear(POOL_LUTE, ARCANE_BLOOD, 0);                               /* Mag +20%, then +10% damage */
    Simulate(sA, sT);
    CHECK(gBattleActor.battleAttack == Col_RelicPercent(mage.mag, 20) + GetItemMight(ITEM_FIRE));
    CHECK(HitDamage(0) == Col_RelicPercent(gBattleActor.battleAttack - gBattleTarget.battleDefense, 10));
    Wear(POOL_LUTE, ARCANE_BLOOD, MAGES_RING);                      /* +10% +15% = +25% */
    Simulate(sA, sT);
    CHECK(HitDamage(0) == Col_RelicPercent(gBattleActor.battleAttack - gBattleTarget.battleDefense, 25));

    /* physical attacks: the magic damage bonus does nothing */
    Wear(POOL_LUTE, 0, 0);
    Prep(sA, gColPool[POOL_LUTE].charId, CLASS_MERCENARY, &kPlain, ITEM_IRON_SWORD, AX, AY);
    Simulate(sA, sT);
    phys = HitDamage(0);
    Wear(POOL_LUTE, MAGES_RING, ARCANE_BLOOD);
    Simulate(sA, sT);
    CHECK(HitDamage(0) == phys);

    /* Fortress Heart: the counter the wearer receives is 20% lower; its own damage is not */
    Wear(POOL_LUTE, 0, 0);
    hitter.pow = 10;
    Prep(sA, gColPool[POOL_LUTE].charId, CLASS_MERCENARY, &kPlain, ITEM_IRON_SWORD, AX, AY);
    Prep(sT, TEST_CHAR, CLASS_MERCENARY, &hitter, ITEM_IRON_SWORD, TX, TY);
    Simulate(sA, sT);
    phys = HitDamage(0);
    counter = HitDamage(1);
    CHECK(counter == 10);                                           /* 10 + 5 - 5 */
    Wear(POOL_LUTE, FORTRESS_HEART, 0);
    Simulate(sA, sT);
    CHECK(HitDamage(0) == phys);
    CHECK(HitDamage(1) == 8);
    return 0;
}

static int CountAttacks(int retaliation)
{
    int i, n = 0;
    for (i = 0; i < 31; i++) {
        const struct BattleHit *h = (const struct BattleHit *)HitAt(i);
        if (h->info & BATTLE_HIT_INFO_END)
            break;
        if (!!(h->info & BATTLE_HIT_INFO_RETALIATION) == retaliation)
            n++;
    }
    return n;
}

/* Blood Pact: the wearer loses 2 HP on each of its attacks (counters too), never below 1. */
int Test_RelicBloodPact(struct Unit *sA, struct Unit *sT)
{
    struct S wall = kPlain;
    int n;

    ClearRelics();
    wall.hp = 60; wall.pow = 0; wall.def = 20;                       /* no damage either way */
    Prep(sA, gColPool[POOL_GILLIAM].charId, CLASS_MERCENARY, &kPlain, ITEM_IRON_SWORD, AX, AY);
    Prep(sT, TEST_CHAR, CLASS_MERCENARY, &wall, ITEM_IRON_SWORD, TX, TY);
    Simulate(sA, sT);
    CHECK(gBattleActor.unit.curHP == kPlain.hp);                    /* without the relic */
    CHECK(HitAttackerHp(0) == 0);

    Wear(POOL_GILLIAM, BLOOD_PACT, 0);
    Simulate(sA, sT);
    n = CountAttacks(0);
    CHECK(n >= 1 && HitAttackerHp(0) == -2);
    CHECK(gBattleActor.unit.curHP == kPlain.hp - 2 * n);
    CHECK(gBattleTarget.unit.curHP == wall.hp);                     /* target unaffected */

    /* as the defender: its counters cost HP too */
    Prep(sA, gColPool[POOL_GILLIAM].charId, CLASS_MERCENARY, &kPlain, ITEM_IRON_SWORD, TX, TY);
    Prep(sT, TEST_CHAR, CLASS_MERCENARY, &wall, ITEM_IRON_SWORD, AX, AY);
    Simulate(sT, sA);
    CHECK(gBattleTarget.unit.curHP == kPlain.hp - 2 * CountAttacks(1));

    /* never kills the wearer: 3 HP and two attacks -> 1 HP */
    Prep(sA, gColPool[POOL_GILLIAM].charId, CLASS_MERCENARY, &kPlain, ITEM_IRON_SWORD, AX, AY);
    Prep(sT, TEST_CHAR, CLASS_MERCENARY, &wall, ITEM_IRON_SWORD, TX, TY);
    sA->spd = 20;                                                   /* doubles */
    sA->curHP = 3;
    Simulate(sA, sT);
    CHECK(CountAttacks(0) == 2);
    CHECK(gBattleActor.unit.curHP == 1);
    sA->curHP = 1;
    Simulate(sA, sT);
    CHECK(gBattleActor.unit.curHP == 1);
    return 0;
}

/* Golden Thread: +25% battle gold for each worn by a living roster member (spec 36). */
int Test_RelicGold(struct Unit *sA, struct Unit *sT)
{
    Col_RunNew(8);
    gColRun.roster[0] = POOL_GILLIAM; gColRun.roster[1] = POOL_LUTE;
    gColRun.rosterCount = 2;
    CHECK(Col_RelicApplyGold(400) == 400);
    Wear(POOL_GILLIAM, GOLDEN_THREAD, 0);
    CHECK(Col_RelicApplyGold(400) == 500 && Col_RelicApplyGold(301) == 376);
    Wear(POOL_LUTE, GOLDEN_THREAD, 0);
    CHECK(Col_RelicApplyGold(400) == 600);                         /* +25% +25% */
    Wear(POOL_GERIK, GOLDEN_THREAD, GOLDEN_THREAD);                /* not in the roster: no effect */
    CHECK(Col_RelicApplyGold(400) == 600);
    CHECK(Col_BattleGold(COL_ENC_NORMAL) <= 300);                   /* the base amount is unchanged */
    return 0;
}

/* The shop sells relics (not every round now that the stock is fully random): valid relics,
 * priced by rarity. */
int Test_RelicShop(struct Unit *sA, struct Unit *sT)
{
    int round, i, found, total = 0;

    for (round = 0; round < 20; round++) {
        Col_RunNew(100 + round);
        Col_ShopGenerate();
        for (found = 0, i = 0; i < gColRun.shopCount; i++) {
            const struct ColShopEntry *e = &gColRun.shop[i];
            if (e->category != COL_SHOP_RELIC)
                continue;
            CHECK(Col_RelicDef(e->value));
            CHECK(e->basePrice >= 500);
            found++;
        }
        total += found;
    }
    CHECK(total >= 10);                                     /* ~2 relics per stock on average */
    return 0;
}
