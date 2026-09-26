/* Notices: the Elite announcement before an Elite battle (spec 47) and enemy drops (encounters.c).
 *
 *   +------------------------------+
 *   | Enemy dropped!               |   title (gold)
 *   | Relic: Iron Heart            |   line 1 (blue)
 *   | Added to the relic bag.      |   line 2 (white)
 *   | OK                           |
 *   +------------------------------+
 *
 * Drops are claimed by Col_StartDropClaims (ASMC; the event waits): after player-phase combats
 * (ColEvt_StatChoice), at the start of each player phase (enemy-phase kills) and at the end of
 * the battle. Gold and relics are given at once; a skill drop is a skill the killer can learn,
 * offered through the "learn a skill" menu (they may decline). A relic that doesn't fit in a
 * full bag, or a skill nobody can learn, becomes gold instead. */
#include "coliseum.h"
#include "bmunit.h"
#include "proc.h"
#include "fontgrp.h"
#include "uimenu.h"
#include "hardware.h"

#define LINE_MAX 28

struct ColNoticeUi {                            /* COL_UI_SCRATCH + 0x80 (0x80 bytes) */
    char title[LINE_MAX];
    char line1[LINE_MAX];
    char line2[LINE_MAX];
    struct Unit *learner;                       /* skill drop: who learns it */
    u8 skill;
    u8 pad[3];
};
#define gColNoticeUi (*(struct ColNoticeUi *)(COL_UI_SCRATCH + 0x80))
_Static_assert(sizeof(struct ColNoticeUi) <= 0x80, "notice scratch must stay within 0x280-0x2FF");

void Col_OpenSkillMenu(struct Proc *parent, struct Unit *unit, int skill);   /* skill_ui.c */

static char *Cat(char *out, const char *s)
{
    char *end = out;
    while (*end)
        end++;
    while (*s && end - out < LINE_MAX - 1)
        *end++ = *s++;
    *end = 0;
    return out;
}

static void Set(char *dst, const char *a, const char *b)
{
    dst[0] = 0;
    Cat(dst, a);
    if (b)
        Cat(dst, b);
}

static void Number(char *out, int n)            /* appends n */
{
    char digits[8];
    int k = 0;
    char s[2] = { 0, 0 };
    do {
        digits[k++] = (char)('0' + n % 10);
        n /= 10;
    } while (n);
    while (k) {
        s[0] = digits[--k];
        Cat(out, s);
    }
}

static u16 *Tile(struct MenuProc *menu, struct MenuItemProc *item)
{
    return BG_GetMapBuffer(menu->frontBg) + TILEMAP_INDEX(item->xTile, item->yTile);
}

static int Draw(struct MenuProc *m, struct MenuItemProc *i, int color, const char *s)
{
    ClearText(&i->text);
    Text_InsertDrawString(&i->text, 0, color, s);
    PutText(&i->text, Tile(m, i));
    return 0;
}

static int TitleDraw(struct MenuProc *m, struct MenuItemProc *i) { return Draw(m, i, TEXT_COLOR_SYSTEM_GOLD, gColNoticeUi.title); }
static int Line1Draw(struct MenuProc *m, struct MenuItemProc *i) { return Draw(m, i, TEXT_COLOR_SYSTEM_BLUE, gColNoticeUi.line1); }
static int Line2Draw(struct MenuProc *m, struct MenuItemProc *i) { return Draw(m, i, TEXT_COLOR_SYSTEM_WHITE, gColNoticeUi.line2); }
static int OkDraw(struct MenuProc *m, struct MenuItemProc *i) { return Draw(m, i, TEXT_COLOR_SYSTEM_WHITE, "OK"); }
static u8 Line2Avail(const struct MenuItemDef *d, int n) { return gColNoticeUi.line2[0] ? MENU_ENABLED : MENU_NOTSHOWN; }
static u8 NotAnOption(struct MenuProc *m, struct MenuItemProc *i) { return MENU_ACT_SND6B; }
static u8 Ok(struct MenuProc *m, struct MenuItemProc *i)
{
    return MENU_ACT_SKIPCURSOR | MENU_ACT_END | MENU_ACT_SND6A | MENU_ACT_CLEAR;
}

#define ROW(avail, draw, select) { "", 0, 0, 0, 0, avail, draw, select, 0, 0, 0 }
static const struct MenuItemDef kNoticeItems[] = {
    ROW(MenuAlwaysEnabled, TitleDraw, NotAnOption),
    ROW(MenuAlwaysEnabled, Line1Draw, NotAnOption),
    ROW(Line2Avail, Line2Draw, NotAnOption),
    ROW(MenuAlwaysEnabled, OkDraw, Ok),
    { 0 },
};
static const struct MenuDef kNoticeMenu = { .rect = { 4, 4, 22, 0 }, .menuItems = kNoticeItems, .onBPress = Ok };

static void OpenNotice(struct Proc *proc)
{
    struct MenuProc *menu;
    ResetTextFont();
    menu = StartMenu(&kNoticeMenu, proc);
    menu->itemCurrent = (u8)(gColNoticeUi.line2[0] ? 3 : 2);   /* on OK */
}

/* ---- drops ---- */

static struct Unit *DropLearner(const struct ColDrop *d)
{
    struct Unit *u = d->killer ? GetUnit(d->killer) : NULL;
    int slot;

    if (u && u->pCharacterData && UNIT_FACTION(u) == FACTION_BLUE && !(u->state & US_DEAD) && u->curHP > 0)
        return u;
    for (slot = 0; slot < COL_MAX_DEPLOY; slot++) {               /* unknown killer: a fighter */
        u = gColRun.deployed[slot] < COL_MAX_ROSTER ? Col_RosterUnit(gColRun.deployed[slot]) : NULL;
        if (u && !(u->state & US_DEAD) && u->curHP > 0)
            return u;
    }
    return NULL;
}

static void GoldInstead(int gold, const char *why)
{
    Col_AddGold(gold);
    Col_SyncPartyGold();
    Set(gColNoticeUi.line1, "", "");
    Number(gColNoticeUi.line1, gold);
    Cat(gColNoticeUi.line1, " gold");
    Set(gColNoticeUi.line2, why, NULL);
}

/* Applies drop `i` and fills the notice. Returns 1 if a skill menu should follow. */
static int Claim(int i)
{
    struct ColDrop *d = &gColRun.drops[i];
    int kind = d->kind, skill;

    d->kind = COL_DROP_NONE;                    /* claimed once, whatever happens */
    Set(gColNoticeUi.title, "Enemy dropped!", NULL);
    gColNoticeUi.line2[0] = 0;
    gColNoticeUi.learner = NULL;
    switch (kind) {
    case COL_DROP_GOLD:
        GoldInstead(d->value * 10, NULL);
        return 0;
    case COL_DROP_RELIC:
        if (!Col_RelicDef(d->value) || !Col_RelicBagAdd(d->value)) {
            GoldInstead(300, "(relic bag full: sold)");
            return 0;
        }
        Set(gColNoticeUi.line1, "Relic: ", Col_RelicDef(d->value)->name);
        Set(gColNoticeUi.line2, "Added to the relic bag.", NULL);
        return 0;
    case COL_DROP_SKILL:
        gColNoticeUi.learner = DropLearner(d);
        if (!gColNoticeUi.learner || !Col_RollSkillOffers(gColNoticeUi.learner, (u8 *)&gColNoticeUi.skill, 1)) {
            GoldInstead(300, "(no skill to learn: sold)");
            return 0;
        }
        skill = gColNoticeUi.skill;
        Set(gColNoticeUi.line1, "Skill: ", Col_SkillName(skill));
        Set(gColNoticeUi.line2, GetStringFromIndex(UNIT_NAME_ID(gColNoticeUi.learner)), " can learn it.");
        return 1;
    }
    return 0;
}

static void Drops_Next(struct Proc *proc)
{
    int i = Col_PendingDrop();

    if (i < 0) {
        Proc_Goto(proc, 9);
        return;
    }
    Claim(i);
    OpenNotice(proc);
}

static void Drops_Skill(struct Proc *proc)
{
    if (gColNoticeUi.learner)
        Col_OpenSkillMenu(proc, gColNoticeUi.learner, gColNoticeUi.skill);   /* learn / replace / decline */
    gColNoticeUi.learner = NULL;
}

static const struct ProcCmd kProcScr_Drops[] = {
    PROC_NAME("ColDrops"),
    PROC_LABEL(0),
    PROC_CALL(Drops_Next),
    PROC_YIELD,
    PROC_CALL(Drops_Skill),
    PROC_YIELD,
    PROC_GOTO(0),
    PROC_LABEL(9),
    PROC_END,
};

/* ASMC: the calling event waits until every drop of a dead enemy is claimed. */
void Col_StartDropClaims(struct Proc *eventProc)
{
    if (Col_PendingDrop() >= 0)
        Proc_StartBlocking(kProcScr_Drops, eventProc);
}

/* ---- the Elite announcement (battle-start flow, roster_ui.c) ---- */

static const struct ProcCmd kProcScr_Elite[] = {
    PROC_NAME("ColElite"),
    PROC_CALL(OpenNotice),
    PROC_YIELD,
    PROC_END,
};

void Col_AnnounceEncounter(struct Proc *parent)
{
    const struct ColEliteSetup *elite = Col_CurrentElite();

    if (!elite)
        return;
    Set(gColNoticeUi.title, "ELITE BATTLE!", NULL);
    Set(gColNoticeUi.line1, elite->name, NULL);
    Set(gColNoticeUi.line2, "Win: 2x gold and a relic.", NULL);
    Proc_StartBlocking(kProcScr_Elite, parent);
}
