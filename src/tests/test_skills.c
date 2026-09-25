/* On-target tests for Phase 6 (spec 27-32): 1 personal + 3 slots, class skills in a slot,
 * no duplicates, prerequisites, the catalog, rarity-weighted offers, skills reset per run.
 * Map tests, run through ColTest_MapRun. Skills live per character (BWL), so the tests use pool
 * characters that are not on the map and put their skill bytes back afterwards. */
#include "coliseum.h"
#include "bmunit.h"

#define CHECK(cond) do { if (!(cond)) return __LINE__; } while (0)

#define BWL_SKILLS(pid)  ((u8 *)(0x0203E884 + (pid) * 0x10 + 1))
#define FORGET_SHORT     (*(u16 *)0x0202BCDE)    /* Skill System: skill waiting to be learned */

extern const struct ColSkillInfo gColSkillCatalog[];
extern const u8 SkillAdder[];                                  /* Skill System (addSkill.s, Thumb) */
extern void InitSkillBuffers(void);
#define CallSkillAdder(u, s) (((int (*)(struct Unit *, int))((u32)SkillAdder | 1))((u), (s)))
extern int SkillTester(struct Unit *unit, int skill);         /* Skill System (SkillTester.c) */

/* pool index by FE8 character id */
static int Pool(int charId)
{
    int i;
    for (i = 0; i < COL_POOL_SIZE; i++)
        if (gColPool[i].charId == charId)
            return i;
    return -1;
}

static int OnMap(int charId)
{
    return GetUnitFromCharIdAndFaction(charId, FACTION_BLUE) != NULL;
}

/* A test unit of `charId` (off the map) with its skill bytes saved in `saved`. */
static struct Unit *Make(int charId, u8 saved[4])
{
    struct Unit *u;
    int i;
    if (OnMap(charId) || Pool(charId) < 0)
        return NULL;
    for (i = 0; i < 4; i++) {
        saved[i] = BWL_SKILLS(charId)[i];
        BWL_SKILLS(charId)[i] = 0;
    }
    u = Col_LoadPoolUnit(Pool(charId));             /* loading teaches the class skill */
    return u;
}

static void Done(struct Unit *u, int charId, const u8 saved[4])
{
    int i;
    if (u)
        ClearUnit(u);
    for (i = 0; i < 4; i++)
        BWL_SKILLS(charId)[i] = saved[i];
}

#define CH_FRANZ   0x04    /* Cavalier: class skill Canto */
#define CH_NEIMI   0x08    /* Archer */
#define CH_GARCIA  0x0A    /* Fighter */
#define CH_JOSHUA  0x20    /* Myrmidon */
#define CH_LUTE    0x0C    /* Mage */

/* Two candidates of which at least one is off the map. */
static int PickOffMap(int a, int b)
{
    return !OnMap(a) ? a : !OnMap(b) ? b : 0;
}

/* Class skill is learned into slot 1 (not implicit); it can be replaced and is then gone. */
int Test_ClassSkillInSlot(struct Unit *x, struct Unit *y)
{
    u8 saved[4];
    int ch = PickOffMap(CH_FRANZ, 0x06 /* Vanessa: Pegasus, Canto */), result = 0;
    struct Unit *u;
    int canto;

    CHECK(ch);
    u = Make(ch, saved);
    CHECK(u);
    canto = Col_SlotSkill(u, 0);
    if (!canto || Col_SlotSkill(u, 1) || Col_FreeSkillSlot(u) != 1)
        result = __LINE__;
    else if (!SkillTester(u, canto))
        result = __LINE__;
    else {
        BWL_SKILLS(ch)[0] = 0;                        /* replaced: Canto gone for real */
        InitSkillBuffers();
        if (SkillTester(u, canto))
            result = __LINE__;
    }
    Done(u, ch, saved);
    return result;
}

/* 3 slots: a 4th skill needs a replacement; the Skill System's adder also stops at 3. */
int Test_ThreeSlots(struct Unit *x, struct Unit *y)
{
    u8 saved[4];
    int ch = PickOffMap(CH_JOSHUA, CH_GARCIA), result = 0;
    u16 forget = FORGET_SHORT;
    struct Unit *u;

    CHECK(ch);
    u = Make(ch, saved);
    CHECK(u);
    u->skl = 20;
    u->spd = 20;
    {
        const struct ColSkillInfo *e;
        int learned = 0;
        for (e = gColSkillCatalog; e->skill && learned < 3; e++)
            if (Col_LearnSkill(u, e->skill, -1))
                learned++;
        if (learned != 3 || Col_FreeSkillSlot(u) != -1)
            result = __LINE__;
        else {
            for (; e->skill; e++)
                if (Col_CanLearnSkill(u, e->skill))
                    break;
            if (!e->skill)
                result = __LINE__;
            else if (Col_LearnSkill(u, e->skill, -1))
                result = __LINE__;                      /* no free slot */
            else if (!Col_LearnSkill(u, e->skill, 1) || Col_SlotSkill(u, 1) != e->skill)
                result = __LINE__;                      /* replacing slot 2 works */
            else if (!Col_UnitHasSkill(u, Col_PersonalSkill(u)))
                result = __LINE__;                      /* personal untouched */
            else {
                int more = 0;
                for (e = gColSkillCatalog; e->skill; e++)
                    if (Col_CanLearnSkill(u, e->skill)) {
                        more = e->skill;
                        break;
                    }
                FORGET_SHORT = 0;
                CallSkillAdder(u, more);
                if (BWL_SKILLS(ch)[3] != 0 || FORGET_SHORT != (0x8000 | more))
                    result = __LINE__;                  /* adder: 4th goes to "forget" */
            }
        }
    }
    FORGET_SHORT = forget;
    Done(u, ch, saved);
    return result;
}

/* spec 31: no duplicates (personal or slot). */
int Test_NoDuplicateSkills(struct Unit *x, struct Unit *y)
{
    u8 saved[4];
    int ch = PickOffMap(CH_JOSHUA, CH_GARCIA), result = 0, skill;
    struct Unit *u;

    CHECK(ch);
    u = Make(ch, saved);
    CHECK(u);
    skill = gColSkillCatalog[0].skill;
    if (!Col_LearnSkill(u, skill, -1) || Col_LearnSkill(u, skill, -1) || Col_CanLearnSkill(u, skill))
        result = __LINE__;
    else if (Col_CanLearnSkill(u, Col_PersonalSkill(u)))
        result = __LINE__;
    Done(u, ch, saved);
    return result;
}

/* spec 30: weapon, mounted, promoted, stat prerequisites; enemy-only never offered. */
int Test_SkillPrerequisites(struct Unit *x, struct Unit *y)
{
    const struct ColSkillInfo *e;
    int bowSkill = 0, mountSkill = 0, promoSkill = 0, statSkill = 0, enemySkill = 0;
    int result = 0;

    for (e = gColSkillCatalog; e->skill; e++) {
        if (e->weapons == 8 && !e->stat && !e->flags) bowSkill = e->skill;
        if (e->flags == COL_SKF_MOUNTED && !e->weapons && !e->stat) mountSkill = e->skill;
        if ((e->flags & COL_SKF_PROMOTED)) promoSkill = e->skill;
        if (e->stat == COL_STAT_SKL + 1 && !e->weapons && !e->flags) statSkill = e->skill;
        if (e->flags & COL_SKF_ENEMY_ONLY) enemySkill = e->skill;
    }
    CHECK(bowSkill && mountSkill && promoSkill && statSkill && enemySkill);

    {
        u8 s1[4], s2[4];
        int archer = PickOffMap(CH_NEIMI, 0), foot = PickOffMap(CH_GARCIA, CH_JOSHUA);
        struct Unit *a = archer ? Make(archer, s1) : NULL;
        struct Unit *b = Make(foot, s2);
        if (!b)
            result = __LINE__;
        else {
            if (a && !Col_CanLearnSkill(a, bowSkill)) result = __LINE__;
            if (Col_CanLearnSkill(b, bowSkill)) result = __LINE__;       /* no bow */
            if (Col_CanLearnSkill(b, mountSkill)) result = __LINE__;     /* on foot */
            if (Col_CanLearnSkill(b, promoSkill)) result = __LINE__;     /* not promoted */
            if (Col_CanLearnSkill(b, enemySkill)) result = __LINE__;
            b->skl = Col_SkillInfo(statSkill)->statMin - 1;
            if (Col_CanLearnSkill(b, statSkill)) result = __LINE__;
            b->skl++;
            if (!Col_CanLearnSkill(b, statSkill)) result = __LINE__;
        }
        if (b) Done(b, foot, s2);
        if (a) Done(a, archer, s1);
    }
    return result;
}

/* spec 28: every catalog entry valid, rarities 1-5 all present, no duplicates, names exist. */
int Test_SkillCatalog(struct Unit *x, struct Unit *y)
{
    const struct ColSkillInfo *e, *f;
    int seen = 0, enemy = 0, n = 0;

    for (e = gColSkillCatalog; e->skill; e++, n++) {
        CHECK(e->skill != 0xFF);
        CHECK(e->rarity >= 1 && e->rarity <= COL_RARITY_COUNT);
        CHECK(e->stat <= COL_STAT_COUNT);
        CHECK(Col_SkillName(e->skill)[0] != 0);
        seen |= 1 << e->rarity;
        if (e->flags & COL_SKF_ENEMY_ONLY)
            enemy++;
        for (f = gColSkillCatalog; f != e; f++)
            CHECK(f->skill != e->skill);
    }
    CHECK(seen == 0x3E);
    CHECK(enemy > 0 && n > 30);
    return 0;
}

/* Offers: distinct, learnable, rarity-weighted (Common far more often than Legendary). */
int Test_SkillOffers(struct Unit *x, struct Unit *y)
{
    u8 saved[4], out[3];
    int ch = PickOffMap(CH_LUTE, 0x1F /* Knoll */), counts[COL_RARITY_COUNT + 1] = { 0 };
    int i, result = 0;
    struct Unit *u;

    CHECK(ch);
    u = Make(ch, saved);
    CHECK(u);
    u->_u3A = 20;                                     /* Mag 20: magic Legendaries possible */
    u->skl = u->spd = 20;
    for (i = 0; i < 400 && !result; i++) {
        int n = Col_RollSkillOffers(u, out, 3);
        if (n != 3 || out[0] == out[1] || out[1] == out[2] || out[0] == out[2])
            result = __LINE__;
        else if (!Col_CanLearnSkill(u, out[0]) || !Col_CanLearnSkill(u, out[2]))
            result = __LINE__;
        else
            counts[Col_SkillInfo(out[0])->rarity]++;
    }
    if (!result && !(counts[1] > counts[3] && counts[3] > counts[5] && counts[5] > 0))
        result = __LINE__;
    Done(u, ch, saved);
    return result;
}

/* A new run clears every pool character's learned skills. */
int Test_RunClearsSkills(struct Unit *x, struct Unit *y)
{
    u8 saved[COL_POOL_SIZE][4];
    int p, i, result = 0;

    for (p = 0; p < COL_POOL_SIZE; p++)
        for (i = 0; i < 4; i++) {
            saved[p][i] = BWL_SKILLS(gColPool[p].charId)[i];
            BWL_SKILLS(gColPool[p].charId)[i] = (u8)(10 + i);
        }
    Col_ClearRunSkills();
    for (p = 0; p < COL_POOL_SIZE && !result; p++)
        for (i = 0; i < 4; i++)
            if (BWL_SKILLS(gColPool[p].charId)[i])
                result = __LINE__;
    for (p = 0; p < COL_POOL_SIZE; p++)
        for (i = 0; i < 4; i++)
            BWL_SKILLS(gColPool[p].charId)[i] = saved[p][i];
    return result;
}

/* spec 32: normal enemies (generic 0x80) get the floor-gated list by level, and their class
 * skill from the class list. */
extern const u8 ColEnemySkillList[];

int Test_EnemySkills(struct Unit *x, struct Unit *t)
{
    int level1 = ColEnemySkillList[0], skill1 = ColEnemySkillList[1];
    int level3 = ColEnemySkillList[4], skill3 = ColEnemySkillList[5];

    CHECK(t->pCharacterData->number == 0x80);
    t->level = (s8)(level1 - 1);
    InitSkillBuffers();
    CHECK(!SkillTester(t, skill1));
    t->level = (s8)level1;
    InitSkillBuffers();
    CHECK(SkillTester(t, skill1) && !SkillTester(t, skill3));
    t->level = (s8)level3;
    InitSkillBuffers();
    CHECK(SkillTester(t, skill1) && SkillTester(t, skill3));

    t->pClassData = GetClassData(0x05);                      /* Cavalier: Canto (ID 1) */
    InitSkillBuffers();
    CHECK(SkillTester(t, 1));
    t->pClassData = GetClassData(0x0F);                      /* Mercenary: no class skill */
    InitSkillBuffers();
    CHECK(!SkillTester(t, 1));
    InitSkillBuffers();
    return 0;
}
