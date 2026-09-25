/* Roster menus, run at the start of each battle by the battle chapter's beginning event
 * (Col_BattleStartFlow, ASMC; the event waits for them):
 *
 *   1. New run (spec 7): "Your team" shows the 3 random characters; Begin accepts (no reroll).
 *   2. Recruitment (spec 9), when a 3-win reward is due. Until the reward menu exists (Phase 10)
 *      the 3-win reward is always a recruitment offer: up to 3 candidates or Decline; with a full
 *      roster, "Replace whom?" picks who leaves (Back returns to the candidates).
 *   3. Deployment (spec 8), when more than 3 are alive: toggle units, Fight with exactly 3.
 *
 * Rows: "Name     Class        Lv 12" (a '*' marks deployed units in the deployment menu). */
#include "coliseum.h"
#include "bmunit.h"
#include "proc.h"
#include "fontgrp.h"
#include "uimenu.h"
#include "functions.h"
#include "hardware.h"

enum { RECRUIT_IDLE, RECRUIT_OPEN, RECRUIT_REPLACE, RECRUIT_BACK, RECRUIT_DONE };

struct ColRosterUi {                            /* COL_UI_SCRATCH + 0x10 (stat choice uses 0x00) */
    u8 candidates[COL_RECRUIT_CHOICES];
    u8 count;
    u8 state;
    u8 chosen;                                  /* candidate pool index being recruited */
    u8 pick;                                    /* deployment menu: bit = roster slot */
};
#define gColRosterUi (*(struct ColRosterUi *)(COL_UI_SCRATCH + 0x10))

#define MENU_X      5
#define MENU_Y      2
#define MENU_W      19
#define X_NAME      8
#define X_CLASS     56
#define X_LV        112
#define X_LEVEL     136                         /* right edge of the level number */

/* ---- drawing ---- */

static void Put(struct MenuProc *menu, struct MenuItemProc *item)
{
    PutText(&item->text, BG_GetMapBuffer(menu->frontBg) + TILEMAP_INDEX(item->xTile, item->yTile));
}

static void DrawRow(struct MenuProc *menu, struct MenuItemProc *item, int pool, int level, int marked)
{
    const struct CharacterData *c = GetCharacterData(gColPool[pool].charId);
    int classId = gColPool[pool].classId ? gColPool[pool].classId : c->defaultClass;

    ClearText(&item->text);
    if (marked)
        Text_InsertDrawString(&item->text, 0, TEXT_COLOR_SYSTEM_GREEN, "*");
    Text_InsertDrawString(&item->text, X_NAME, TEXT_COLOR_SYSTEM_WHITE, GetStringFromIndex(c->nameTextId));
    Text_InsertDrawString(&item->text, X_CLASS, TEXT_COLOR_SYSTEM_WHITE,
                          GetStringFromIndex(GetClassData(classId)->nameTextId));
    Text_InsertDrawString(&item->text, X_LV, TEXT_COLOR_SYSTEM_GOLD, "Lv");
    Text_InsertDrawNumberOrBlank(&item->text, X_LEVEL, TEXT_COLOR_SYSTEM_BLUE, level);
    Put(menu, item);
}

static void DrawLabel(struct MenuProc *menu, struct MenuItemProc *item, int color, const char *s)
{
    ClearText(&item->text);
    Text_InsertDrawString(&item->text, 0, color, s);
    Put(menu, item);
}

static int SlotLevel(int slot)
{
    struct Unit *u = Col_RosterUnit(slot);
    return u ? u->level : COL_START_LEVEL;
}

static u8 NotAnOption(struct MenuProc *menu, struct MenuItemProc *item)
{
    return MENU_ACT_SND6B;
}

#define END_MENU (MENU_ACT_SKIPCURSOR | MENU_ACT_END | MENU_ACT_SND6A | MENU_ACT_CLEAR)
#define ROW(avail, draw, select) { "", 0, 0, 0, 0, avail, draw, select, 0, 0, 0 }

/* ---- 1. new run: your team ---- */

static int Team_Title(struct MenuProc *m, struct MenuItemProc *i) { DrawLabel(m, i, TEXT_COLOR_SYSTEM_GOLD, "Your team"); return 0; }
static int Team_Row(struct MenuProc *m, struct MenuItemProc *i)
{
    DrawRow(m, i, gColRun.roster[i->itemNumber - 1], COL_START_LEVEL, 0);
    return 0;
}
static int Team_BeginDraw(struct MenuProc *m, struct MenuItemProc *i) { DrawLabel(m, i, TEXT_COLOR_SYSTEM_WHITE, "Begin"); return 0; }
static u8 Team_Begin(struct MenuProc *m, struct MenuItemProc *i) { return END_MENU; }

static const struct MenuItemDef kTeamItems[] = {
    ROW(MenuAlwaysEnabled, Team_Title, NotAnOption),
    ROW(MenuAlwaysEnabled, Team_Row, NotAnOption),
    ROW(MenuAlwaysEnabled, Team_Row, NotAnOption),
    ROW(MenuAlwaysEnabled, Team_Row, NotAnOption),
    ROW(MenuAlwaysEnabled, Team_BeginDraw, Team_Begin),
    { 0 },
};
static const struct MenuDef kTeamMenu = { .rect = { MENU_X, MENU_Y, MENU_W, 0 }, .menuItems = kTeamItems };

/* ---- 2. recruitment ---- */

static void RewardTaken(void)
{
    gColRosterUi.state = RECRUIT_DONE;
    Col_TakeReward();
}

static int Recruit_Title(struct MenuProc *m, struct MenuItemProc *i)
{
    DrawLabel(m, i, TEXT_COLOR_SYSTEM_GOLD, Col_FreeRosterSlot() >= 0 ? "Recruit" : "Recruit (roster full)");
    return 0;
}
static u8 Recruit_RowAvail(const struct MenuItemDef *d, int n)
{
    return n - 1 < gColRosterUi.count ? MENU_ENABLED : MENU_NOTSHOWN;
}
static int Recruit_Row(struct MenuProc *m, struct MenuItemProc *i)
{
    DrawRow(m, i, gColRosterUi.candidates[i->itemNumber - 1], Col_RecruitLevel(), 0);
    return 0;
}
static u8 Recruit_Pick(struct MenuProc *m, struct MenuItemProc *i)
{
    int pool = gColRosterUi.candidates[i->itemNumber - 1];

    if (Col_FreeRosterSlot() >= 0) {
        Col_Recruit(pool, -1);
        RewardTaken();
    } else {
        gColRosterUi.chosen = (u8)pool;
        gColRosterUi.state = RECRUIT_REPLACE;
    }
    return END_MENU;
}
static int Recruit_DeclineDraw(struct MenuProc *m, struct MenuItemProc *i) { DrawLabel(m, i, TEXT_COLOR_SYSTEM_WHITE, "Decline"); return 0; }
static u8 Recruit_Decline(struct MenuProc *m, struct MenuItemProc *i)
{
    RewardTaken();
    return END_MENU;
}

static const struct MenuItemDef kRecruitItems[] = {
    ROW(MenuAlwaysEnabled, Recruit_Title, NotAnOption),
    ROW(Recruit_RowAvail, Recruit_Row, Recruit_Pick),
    ROW(Recruit_RowAvail, Recruit_Row, Recruit_Pick),
    ROW(Recruit_RowAvail, Recruit_Row, Recruit_Pick),
    ROW(MenuAlwaysEnabled, Recruit_DeclineDraw, Recruit_Decline),
    { 0 },
};
static const struct MenuDef kRecruitMenu = { .rect = { MENU_X, MENU_Y, MENU_W, 0 }, .menuItems = kRecruitItems };

static int Replace_Title(struct MenuProc *m, struct MenuItemProc *i) { DrawLabel(m, i, TEXT_COLOR_SYSTEM_GOLD, "Replace whom?"); return 0; }
static u8 Roster_RowAvail(const struct MenuItemDef *d, int n)
{
    return gColRun.roster[n - 1] < COL_POOL_SIZE ? MENU_ENABLED : MENU_NOTSHOWN;
}
static int Replace_Row(struct MenuProc *m, struct MenuItemProc *i)
{
    int slot = i->itemNumber - 1;
    DrawRow(m, i, gColRun.roster[slot], SlotLevel(slot), 0);
    return 0;
}
static u8 Replace_Pick(struct MenuProc *m, struct MenuItemProc *i)
{
    Col_Recruit(gColRosterUi.chosen, i->itemNumber - 1);
    RewardTaken();
    return END_MENU;
}
static int Replace_BackDraw(struct MenuProc *m, struct MenuItemProc *i) { DrawLabel(m, i, TEXT_COLOR_SYSTEM_WHITE, "Back"); return 0; }
static u8 Replace_Back(struct MenuProc *m, struct MenuItemProc *i)
{
    gColRosterUi.state = RECRUIT_BACK;
    return END_MENU;
}

static const struct MenuItemDef kReplaceItems[] = {
    ROW(MenuAlwaysEnabled, Replace_Title, NotAnOption),
    ROW(Roster_RowAvail, Replace_Row, Replace_Pick),
    ROW(Roster_RowAvail, Replace_Row, Replace_Pick),
    ROW(Roster_RowAvail, Replace_Row, Replace_Pick),
    ROW(Roster_RowAvail, Replace_Row, Replace_Pick),
    ROW(Roster_RowAvail, Replace_Row, Replace_Pick),
    ROW(MenuAlwaysEnabled, Replace_BackDraw, Replace_Back),
    { 0 },
};
static const struct MenuDef kReplaceMenu = { .rect = { MENU_X, MENU_Y, MENU_W, 0 }, .menuItems = kReplaceItems };

/* ---- 3. deployment ---- */

static int PickCount(void)
{
    int slot, n = 0;
    for (slot = 0; slot < COL_MAX_ROSTER; slot++)
        if (gColRosterUi.pick & (1 << slot))
            n++;
    return n;
}

static int Deploy_Title(struct MenuProc *m, struct MenuItemProc *i) { DrawLabel(m, i, TEXT_COLOR_SYSTEM_GOLD, "Choose 3 fighters"); return 0; }
static int Deploy_Row(struct MenuProc *m, struct MenuItemProc *i)
{
    int slot = i->itemNumber - 1;
    DrawRow(m, i, gColRun.roster[slot], SlotLevel(slot), gColRosterUi.pick & (1 << slot));
    return 0;
}
static u8 Deploy_Toggle(struct MenuProc *m, struct MenuItemProc *i)
{
    int bit = 1 << (i->itemNumber - 1);

    if (gColRosterUi.pick & bit)
        gColRosterUi.pick &= (u8)~bit;
    else if (PickCount() < Col_DeployTarget())
        gColRosterUi.pick |= (u8)bit;
    else
        return MENU_ACT_SND6B;                  /* already 3: remove someone first */
    Deploy_Row(m, i);
    return MENU_ACT_SND6A;
}
static int Deploy_FightDraw(struct MenuProc *m, struct MenuItemProc *i) { DrawLabel(m, i, TEXT_COLOR_SYSTEM_WHITE, "Fight!"); return 0; }
static u8 Deploy_Fight(struct MenuProc *m, struct MenuItemProc *i)
{
    if (!Col_SetDeployment(gColRosterUi.pick))
        return MENU_ACT_SND6B;                  /* not exactly 3 (or all, with fewer alive) */
    return END_MENU;
}

static const struct MenuItemDef kDeployItems[] = {
    ROW(MenuAlwaysEnabled, Deploy_Title, NotAnOption),
    ROW(Roster_RowAvail, Deploy_Row, Deploy_Toggle),
    ROW(Roster_RowAvail, Deploy_Row, Deploy_Toggle),
    ROW(Roster_RowAvail, Deploy_Row, Deploy_Toggle),
    ROW(Roster_RowAvail, Deploy_Row, Deploy_Toggle),
    ROW(Roster_RowAvail, Deploy_Row, Deploy_Toggle),
    ROW(MenuAlwaysEnabled, Deploy_FightDraw, Deploy_Fight),
    { 0 },
};
static const struct MenuDef kDeployMenu = { .rect = { MENU_X, MENU_Y, MENU_W, 0 }, .menuItems = kDeployItems };

/* ---- the flow ---- */

static void Open(const struct MenuDef *def, struct Proc *proc, int cursor)
{
    struct MenuProc *menu = StartMenu(def, proc);
    menu->itemCurrent = (u8)cursor;             /* 1: the first row after the title */
}

static void Flow_Team(struct Proc *proc)
{
    gColRosterUi.state = RECRUIT_IDLE;
    if (Col_RunIsValid() && gColRun.active)
        return;
    Col_StartRun();
    Open(&kTeamMenu, proc, 4);                  /* on Begin */
}

static void Flow_Recruit(struct Proc *proc)
{
    if (gColRosterUi.state != RECRUIT_BACK) {
        if (!gColRun.rewardDue)
            return;
        gColRosterUi.count = (u8)Col_RollRecruits(gColRosterUi.candidates);
        if (!gColRosterUi.count) {              /* everyone already recruited this run */
            Col_TakeReward();
            return;
        }
    }
    gColRosterUi.state = RECRUIT_OPEN;
    Open(&kRecruitMenu, proc, 1);
}

static void Flow_Replace(struct Proc *proc)
{
    if (gColRosterUi.state == RECRUIT_REPLACE)
        Open(&kReplaceMenu, proc, 1);
}

static void Flow_RecruitAgain(struct Proc *proc)
{
    if (gColRosterUi.state == RECRUIT_BACK)
        Proc_Goto(proc, 1);
}

static void Flow_Deploy(struct Proc *proc)
{
    if (gColRun.rosterCount <= COL_MAX_DEPLOY)
        return;                                 /* 3 or fewer alive: everyone fights */
    gColRosterUi.pick = Col_DeploymentMask();
    Open(&kDeployMenu, proc, 1);
}

static const struct ProcCmd kProcScr_BattleStart[] = {
    PROC_NAME("ColBattleStart"),
    PROC_CALL(Flow_Team),
    PROC_YIELD,
    PROC_LABEL(1),
    PROC_CALL(Flow_Recruit),
    PROC_YIELD,
    PROC_CALL(Flow_Replace),
    PROC_YIELD,
    PROC_CALL(Flow_RecruitAgain),
    PROC_CALL(Flow_Deploy),
    PROC_YIELD,
    PROC_END,
};

/* ASMC from the battle chapter's beginning event, before Col_PrepareBattle. */
void Col_BattleStartFlow(struct Proc *eventProc)
{
    Proc_StartBlocking(kProcScr_BattleStart, eventProc);
}
