/* Roster menus, run at the start of each battle by the battle chapter's beginning event
 * (Col_BattleStartFlow, ASMC; the event waits for them):
 *
 *   1. New run (spec 7): "Your team" shows the 3 random characters and the 3 starting relics
 *      (rare in gold); Begin accepts (no reroll).
 *   2. The 3-win reward (spec 51-53, reward.c), when due: up to 3 kinds, pick one (no skipping).
 *      Recruit: up to 3 candidates or Decline (back to the reward choice); with a full roster,
 *      "Replace whom?" picks who leaves (Back returns to the candidates). Skill / Promotion: "who
 *      gets it?" (units that can't are grayed; Back returns to the choice), then the learn menu /
 *      the promotion item. Heal and Gold apply at once.
 *   3. Prepare (prepare_ui.c): Fight! / Deploy / Transfer / Fuse. Deployment (spec 8), when
 *      more than 3 are alive: toggle units, Fight with exactly 3.
 *
 * Rows: "Name     Class        Lv 12" (a '*' marks deployed units in the deployment menu). */
#include "colosseum.h"
#include "bmunit.h"
#include "proc.h"
#include "fontgrp.h"
#include "uimenu.h"
#include "functions.h"
#include "hardware.h"

enum { RECRUIT_IDLE, RECRUIT_OPEN, RECRUIT_REPLACE, RECRUIT_BACK, RECRUIT_DONE,
       RECRUIT_START,                           /* Recruit chosen as the 3-win reward */
       RECRUIT_DECLINED,                        /* Decline: back to the reward choice */
       REWARD_PICK,                             /* Skill / Promotion chosen: who gets it? */
       REWARD_PICK_BACK, REWARD_PICKED };

struct ColRosterUi {                            /* COL_UI_SCRATCH + 0x10 (stat choice uses 0x00) */
    u8 candidates[COL_RECRUIT_CHOICES];
    u8 count;
    u8 state;
    u8 chosen;                                  /* candidate pool index being recruited */
    u8 pick;                                    /* deployment menu: bit = roster slot */
    u8 reward;                                  /* the 3-win reward kind chosen */
    u8 target;                                  /* roster slot that gets it */
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
static int Team_Relic(struct MenuProc *m, struct MenuItemProc *i)   /* the starting relics (bag 0-2) */
{
    const struct ColRelicDef *def = Col_RelicDef(gColRun.relicBag[i->itemNumber - 4]);
    char line[32] = "Relic: ";
    char *p = line + 7;
    const char *s = def ? def->name : "-";

    while (*s && p < line + sizeof(line) - 1)
        *p++ = *s++;
    *p = 0;
    DrawLabel(m, i, def && def->rarity >= COL_RELIC_RARE ? TEXT_COLOR_SYSTEM_GOLD : TEXT_COLOR_SYSTEM_BLUE, line);
    return 0;
}
static int Team_BeginDraw(struct MenuProc *m, struct MenuItemProc *i) { DrawLabel(m, i, TEXT_COLOR_SYSTEM_WHITE, "Begin"); return 0; }
static u8 Team_Begin(struct MenuProc *m, struct MenuItemProc *i) { return END_MENU; }

static const struct MenuItemDef kTeamItems[] = {
    ROW(MenuAlwaysEnabled, Team_Title, NotAnOption),
    ROW(MenuAlwaysEnabled, Team_Row, NotAnOption),
    ROW(MenuAlwaysEnabled, Team_Row, NotAnOption),
    ROW(MenuAlwaysEnabled, Team_Row, NotAnOption),
    ROW(MenuAlwaysEnabled, Team_Relic, NotAnOption),
    ROW(MenuAlwaysEnabled, Team_Relic, NotAnOption),
    ROW(MenuAlwaysEnabled, Team_Relic, NotAnOption),
    ROW(MenuAlwaysEnabled, Team_BeginDraw, Team_Begin),
    { 0 },
};
static u8 Team_Help(struct MenuProc *m, struct MenuItemProc *i)
{
    if (i->itemNumber >= 1 && i->itemNumber <= 3)
        Col_HelpPool(i, gColRun.roster[i->itemNumber - 1]);
    else if (i->itemNumber >= 4 && i->itemNumber <= 6)
        Col_HelpRelic(i, gColRun.relicBag[i->itemNumber - 4]);
    return 0;
}
static const struct MenuDef kTeamMenu = { .rect = { MENU_X, MENU_Y, MENU_W, 0 }, .menuItems = kTeamItems, COL_HELP_MENU(Team_Help) };

/* ---- 2. recruitment ---- */

static void RewardTaken(void)
{
    gColRosterUi.state = RECRUIT_DONE;
    if (gColRun.rewardDue)
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
    gColRosterUi.state = RECRUIT_DECLINED;      /* the reward is still to choose */
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
static u8 Recruit_Help(struct MenuProc *m, struct MenuItemProc *i)
{
    if (i->itemNumber >= 1 && i->itemNumber <= gColRosterUi.count)
        Col_HelpPool(i, gColRosterUi.candidates[i->itemNumber - 1]);
    return 0;
}
static const struct MenuDef kRecruitMenu = { .rect = { MENU_X, MENU_Y, MENU_W, 0 }, .menuItems = kRecruitItems, COL_HELP_MENU(Recruit_Help) };

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
static u8 Roster_Help(struct MenuProc *m, struct MenuItemProc *i)
{
    if (i->itemNumber >= 1 && i->itemNumber <= COL_MAX_ROSTER)
        Col_HelpUnit(i, Col_RosterUnit(i->itemNumber - 1));
    return 0;
}
static const struct MenuDef kReplaceMenu = { .rect = { MENU_X, MENU_Y, MENU_W, 0 }, .menuItems = kReplaceItems, COL_HELP_MENU(Roster_Help) };

/* ---- 2b. the 3-win reward ---- */

static int Reward_Title(struct MenuProc *m, struct MenuItemProc *i) { DrawLabel(m, i, TEXT_COLOR_SYSTEM_GOLD, "3 wins! Choose a reward"); return 0; }
static u8 Reward_Avail(const struct MenuItemDef *d, int n)
{
    return gColRun.rewardKinds[n - 1] != COL_REWARD_NONE ? MENU_ENABLED : MENU_NOTSHOWN;
}
static int Reward_Row(struct MenuProc *m, struct MenuItemProc *i)
{
    ClearText(&i->text);
    switch (gColRun.rewardKinds[i->itemNumber - 1]) {
    case COL_REWARD_RECRUIT:
        Text_InsertDrawString(&i->text, 0, TEXT_COLOR_SYSTEM_WHITE, "Recruit a fighter");
        break;
    case COL_REWARD_SKILL:
        Text_InsertDrawString(&i->text, 0, TEXT_COLOR_SYSTEM_WHITE, "Skill:");
        Text_InsertDrawString(&i->text, 40, TEXT_COLOR_SYSTEM_BLUE, Col_SkillName(gColRun.rewardSkill));
        break;
    case COL_REWARD_PROMOTION:
        Text_InsertDrawString(&i->text, 0, TEXT_COLOR_SYSTEM_WHITE, "Promotion (a seal)");
        break;
    case COL_REWARD_HEAL:
        Text_InsertDrawString(&i->text, 0, TEXT_COLOR_SYSTEM_WHITE, "Heal everyone fully");
        break;
    case COL_REWARD_GOLD:
        Text_InsertDrawString(&i->text, 0, TEXT_COLOR_SYSTEM_WHITE, "Gold:");
        Text_InsertDrawNumberOrBlank(&i->text, 64, TEXT_COLOR_SYSTEM_BLUE, gColRun.rewardGold);
        Text_InsertDrawString(&i->text, 72, TEXT_COLOR_SYSTEM_GOLD, "G");
        break;
    }
    Put(m, i);
    return 0;
}
static u8 Reward_Pick(struct MenuProc *m, struct MenuItemProc *i)
{
    int kind = gColRun.rewardKinds[i->itemNumber - 1];

    gColRosterUi.reward = (u8)kind;
    switch (kind) {
    case COL_REWARD_RECRUIT:
        gColRosterUi.state = RECRUIT_START;
        break;
    case COL_REWARD_SKILL:
    case COL_REWARD_PROMOTION:
        gColRosterUi.state = REWARD_PICK;
        break;
    case COL_REWARD_HEAL:
        Col_TakeHealReward();
        gColRosterUi.state = RECRUIT_DONE;
        break;
    case COL_REWARD_GOLD:
        Col_TakeGoldReward();
        gColRosterUi.state = RECRUIT_DONE;
        break;
    }
    return END_MENU;
}

static const struct MenuItemDef kRewardItems[] = {
    ROW(MenuAlwaysEnabled, Reward_Title, NotAnOption),
    ROW(Reward_Avail, Reward_Row, Reward_Pick),
    ROW(Reward_Avail, Reward_Row, Reward_Pick),
    ROW(Reward_Avail, Reward_Row, Reward_Pick),
    { 0 },
};
static u8 Reward_Help(struct MenuProc *m, struct MenuItemProc *i)
{
    if (i->itemNumber < 1 || i->itemNumber > 3)
        return 0;
    switch (gColRun.rewardKinds[i->itemNumber - 1]) {
    case COL_REWARD_SKILL:
        Col_HelpSkill(i, gColRun.rewardSkill);
        break;
    case COL_REWARD_RECRUIT:
        Col_HelpText(i, "Choose 1 of up to 3 new fighters.\nWith a full roster, someone leaves.");
        break;
    case COL_REWARD_PROMOTION:
        Col_HelpText(i, "A unit of level 10+ gets a seal.\nUse it from Items to promote\n(keeps its level).");
        break;
    case COL_REWARD_HEAL:
        Col_HelpText(i, "Every living fighter\nto full HP.");
        break;
    case COL_REWARD_GOLD:
        Col_HelpText(i, "Gold for the shop and Recover.");
        break;
    }
    return 0;
}
static const struct MenuDef kRewardMenu = { .rect = { MENU_X, MENU_Y, MENU_W, 0 }, .menuItems = kRewardItems, COL_HELP_MENU(Reward_Help) };

static int Who_Can(int slot)
{
    struct Unit *u = Col_RosterUnit(slot);

    if (!u || (u->state & US_DEAD))
        return 0;
    return gColRosterUi.reward == COL_REWARD_SKILL ? Col_CanLearnRewardSkill(u) : Col_CanPromote(u);
}
static int Who_Title(struct MenuProc *m, struct MenuItemProc *i)
{
    DrawLabel(m, i, TEXT_COLOR_SYSTEM_GOLD,
              gColRosterUi.reward == COL_REWARD_SKILL ? "Who learns it?" : "Who promotes? (Lv 10+)");
    return 0;
}
static u8 Who_Avail(const struct MenuItemDef *d, int n)
{
    if (gColRun.roster[n - 1] >= COL_POOL_SIZE)
        return MENU_NOTSHOWN;
    return Who_Can(n - 1) ? MENU_ENABLED : MENU_DISABLED;
}
static int Who_Row(struct MenuProc *m, struct MenuItemProc *i)
{
    int slot = i->itemNumber - 1;
    DrawRow(m, i, gColRun.roster[slot], SlotLevel(slot), 0);
    if (i->availability == MENU_DISABLED) {     /* gray the name of units that can't */
        struct Unit *u = Col_RosterUnit(slot);
        if (u)
            Text_InsertDrawString(&i->text, X_NAME, TEXT_COLOR_SYSTEM_GRAY, GetStringFromIndex(UNIT_NAME_ID(u)));
        Put(m, i);
    }
    return 0;
}
static u8 Who_Pick(struct MenuProc *m, struct MenuItemProc *i)
{
    if (i->availability == MENU_DISABLED)
        return MENU_ACT_SND6B;
    gColRosterUi.target = (u8)(i->itemNumber - 1);
    gColRosterUi.state = REWARD_PICKED;
    return END_MENU;
}
static int Who_BackDraw(struct MenuProc *m, struct MenuItemProc *i) { DrawLabel(m, i, TEXT_COLOR_SYSTEM_WHITE, "Back"); return 0; }
static u8 Who_Back(struct MenuProc *m, struct MenuItemProc *i)
{
    gColRosterUi.state = REWARD_PICK_BACK;
    return END_MENU;
}

static const struct MenuItemDef kWhoItems[] = {
    ROW(MenuAlwaysEnabled, Who_Title, NotAnOption),
    ROW(Who_Avail, Who_Row, Who_Pick),
    ROW(Who_Avail, Who_Row, Who_Pick),
    ROW(Who_Avail, Who_Row, Who_Pick),
    ROW(Who_Avail, Who_Row, Who_Pick),
    ROW(Who_Avail, Who_Row, Who_Pick),
    ROW(MenuAlwaysEnabled, Who_BackDraw, Who_Back),
    { 0 },
};
static const struct MenuDef kWhoMenu = { .rect = { MENU_X, MENU_Y, MENU_W, 0 }, .menuItems = kWhoItems, COL_HELP_MENU(Roster_Help) };

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
static const struct MenuDef kDeployMenu = { .rect = { MENU_X, MENU_Y, MENU_W, 0 }, .menuItems = kDeployItems, COL_HELP_MENU(Roster_Help) };

/* ---- the flow ---- */

static void Open(const struct MenuDef *def, struct Proc *proc, int cursor)
{
    struct MenuProc *menu;

    ResetTextFont();                            /* free the previous menu's text tiles (vanilla does) */
    menu = StartMenu(def, proc);
    menu->itemCurrent = (u8)cursor;             /* 1: the first row after the title */
}

static void Flow_Team(struct Proc *proc)
{
    gColRosterUi.state = RECRUIT_IDLE;
    if (Col_RunIsValid() && gColRun.active)
        return;
    Col_StartRun();
    Open(&kTeamMenu, proc, 7);                  /* on Begin */
}

void Col_OpenSkillMenu(struct Proc *parent, struct Unit *unit, int skill);   /* skill_ui.c */

/* The 3-win reward (label 1): rolled once, then chosen. */
static void Flow_Reward(struct Proc *proc)
{
    if (!gColRun.rewardDue) {
        Proc_Goto(proc, 4);
        return;
    }
    Col_RollReward();
    if (!Col_RewardCount()) {                   /* cannot happen (Gold is always valid) */
        Col_TakeReward();
        Proc_Goto(proc, 4);
        return;
    }
    gColRosterUi.state = RECRUIT_IDLE;
    Open(&kRewardMenu, proc, 1);
}

static void Flow_RewardChosen(struct Proc *proc)
{
    switch (gColRosterUi.state) {
    case RECRUIT_START:
        Proc_Goto(proc, 2);
        return;
    case REWARD_PICK:
        Open(&kWhoMenu, proc, 1);
        return;
    }
    Proc_Goto(proc, 4);                         /* Heal / Gold: done */
}

static void Flow_RewardPicked(struct Proc *proc)
{
    struct Unit *u;
    char line[32];
    const char *s;
    int k = 0;

    if (gColRosterUi.state == REWARD_PICK_BACK) {
        Proc_Goto(proc, 1);
        return;
    }
    if (gColRosterUi.state != REWARD_PICKED)
        return;
    u = Col_RosterUnit(gColRosterUi.target);
    if (gColRosterUi.reward == COL_REWARD_SKILL) {
        int skill = gColRun.rewardSkill;
        Col_TakeReward();
        Col_OpenSkillMenu(proc, u, skill);      /* learn / replace / don't learn */
        return;
    }
    if (Col_GivePromotionSeal(u)) {
        for (s = GetStringFromIndex(UNIT_NAME_ID(u)); *s && k < 16; s++)
            line[k++] = *s;
        for (s = " got a seal."; *s && k < 31; s++)
            line[k++] = *s;
        line[k] = 0;
        Col_ShowNotice(proc, "Promotion", line, "Use it from Items.");
    }
}

static void Flow_Recruit(struct Proc *proc)
{
    if (gColRosterUi.state != RECRUIT_BACK) {
        if (gColRosterUi.state != RECRUIT_START)
            return;
        gColRosterUi.count = (u8)Col_RollRecruits(gColRosterUi.candidates);
        if (!gColRosterUi.count) {              /* everyone already recruited (checked before) */
            Proc_Goto(proc, 1);
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
        Proc_Goto(proc, 2);
    else if (gColRosterUi.state == RECRUIT_DECLINED)
        Proc_Goto(proc, 1);                     /* back to the reward choice */
}

/* "Replace whom?" for recruiting `pool` into a full roster from another flow (shop).
 * Col_ReplaceMenuRecruited() tells afterwards whether someone was replaced. */
void Col_OpenReplaceMenu(struct Proc *parent, int pool)
{
    gColRosterUi.chosen = (u8)pool;
    gColRosterUi.state = RECRUIT_REPLACE;
    Open(&kReplaceMenu, parent, 1);
}

int Col_ReplaceMenuRecruited(void)
{
    return gColRosterUi.state == RECRUIT_DONE;
}

/* The deployment menu (from the Prepare menu's Deploy, prepare_ui.c). */
void Col_OpenDeployMenu(struct Proc *parent)
{
    if (gColRun.rosterCount <= COL_MAX_DEPLOY)
        return;                                 /* 3 or fewer alive: everyone fights */
    gColRosterUi.pick = Col_DeploymentMask();
    Open(&kDeployMenu, parent, 1);
}

void Col_StartPrepare(struct Proc *parent);     /* prepare_ui.c */
void Col_AnnounceEncounter(struct Proc *parent); /* notice_ui.c: "ELITE BATTLE!" */

static void Flow_Announce(struct Proc *proc)
{
    Col_AnnounceEncounter(proc);
}

static void Flow_Arena(struct Proc *proc)
{
    Col_AnnounceArena(proc);                    /* arena, weather, tiles (notice_ui.c) */
}

static void Flow_Prepare(struct Proc *proc)
{
    Col_StartPrepare(proc);
}

static const struct ProcCmd kProcScr_BattleStart[] = {
    PROC_NAME("ColBattleStart"),
    PROC_CALL(Flow_Team),
    PROC_YIELD,
    PROC_LABEL(1),
    PROC_CALL(Flow_Reward),
    PROC_YIELD,
    PROC_CALL(Flow_RewardChosen),
    PROC_YIELD,
    PROC_CALL(Flow_RewardPicked),
    PROC_YIELD,
    PROC_GOTO(4),
    PROC_LABEL(2),
    PROC_CALL(Flow_Recruit),
    PROC_YIELD,
    PROC_CALL(Flow_Replace),
    PROC_YIELD,
    PROC_CALL(Flow_RecruitAgain),
    PROC_LABEL(4),
    PROC_CALL(Flow_Announce),
    PROC_YIELD,
    PROC_CALL(Flow_Arena),
    PROC_YIELD,
    PROC_CALL(Flow_Prepare),
    PROC_YIELD,
    PROC_END,
};

/* ASMC from the battle chapter's beginning event, before Col_PrepareBattle. */
void Col_BattleStartFlow(struct Proc *eventProc)
{
    Proc_StartBlocking(kProcScr_BattleStart, eventProc);
}
