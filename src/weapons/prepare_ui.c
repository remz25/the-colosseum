/* "Prepare" menu before each battle (after the team screen / recruitment):
 *
 *   Prepare          Fight!     start the battle
 *                    Deploy     choose the 3 fighters (only with more than 3 alive, spec 8)
 *                    Transfer   give an item to another unit that can use it (spec 40)
 *                    Fuse       combine two weapons in one unit's inventory (spec 42)
 *
 * Transfer: pick the giver, the item, then the receiver (units that can't take it are grayed).
 * Fuse: pick a unit (grayed without a possible fusion), then the fusion (result name, and the
 * first source). Every list has Back. Phase 10's post-battle menu takes these over later. */
#include "coliseum.h"
#include "bmunit.h"
#include "bmitem.h"
#include "proc.h"
#include "fontgrp.h"
#include "uimenu.h"
#include "functions.h"
#include "hardware.h"
#include "icon.h"

enum { NEXT_NONE, NEXT_FIGHT, NEXT_DEPLOY, NEXT_TRANSFER, NEXT_FUSE, NEXT_BACK, NEXT_OK };
enum { MODE_GIVER, MODE_RECEIVER, MODE_FUSER };

#define MAX_FUSIONS 5

struct ColPrepUi {                              /* COL_UI_SCRATCH + 0x30 */
    u8 next;
    u8 mode;
    u8 unitSlot;                                /* giver / fuser roster slot */
    u8 itemSlot;                                /* item being given */
    u8 fusionCount;
    u8 fusions[MAX_FUSIONS][2];
};
#define gColPrepUi (*(struct ColPrepUi *)(COL_UI_SCRATCH + 0x30))

#define MENU_X  5
#define MENU_Y  2
#define MENU_W  19

void Col_OpenDeployMenu(struct Proc *parent);   /* roster_ui.c */

static u16 *Tile(struct MenuProc *menu, struct MenuItemProc *item, int dx)
{
    return BG_GetMapBuffer(menu->frontBg) + TILEMAP_INDEX(item->xTile + dx, item->yTile);
}

static void Label(struct MenuProc *m, struct MenuItemProc *i, int color, const char *s)
{
    ClearText(&i->text);
    Text_InsertDrawString(&i->text, 0, color, s);
    PutText(&i->text, Tile(m, i, 0));
}

static u8 NotAnOption(struct MenuProc *m, struct MenuItemProc *i) { return MENU_ACT_SND6B; }

#define END_MENU (MENU_ACT_SKIPCURSOR | MENU_ACT_END | MENU_ACT_SND6A | MENU_ACT_CLEAR)
#define ROW(avail, draw, select) { "", 0, 0, 0, 0, avail, draw, select, 0, 0, 0 }

static u8 Back(struct MenuProc *m, struct MenuItemProc *i)
{
    gColPrepUi.next = NEXT_BACK;
    return END_MENU;
}
static int BackDraw(struct MenuProc *m, struct MenuItemProc *i) { Label(m, i, TEXT_COLOR_SYSTEM_WHITE, "Back"); return 0; }

/* ---- the Prepare menu ---- */

static int P_Title(struct MenuProc *m, struct MenuItemProc *i) { Label(m, i, TEXT_COLOR_SYSTEM_GOLD, "Prepare"); return 0; }
static int P_FightDraw(struct MenuProc *m, struct MenuItemProc *i) { Label(m, i, TEXT_COLOR_SYSTEM_WHITE, "Fight!"); return 0; }
static int P_DeployDraw(struct MenuProc *m, struct MenuItemProc *i) { Label(m, i, TEXT_COLOR_SYSTEM_WHITE, "Deploy"); return 0; }
static int P_TransferDraw(struct MenuProc *m, struct MenuItemProc *i) { Label(m, i, TEXT_COLOR_SYSTEM_WHITE, "Transfer"); return 0; }
static int P_FuseDraw(struct MenuProc *m, struct MenuItemProc *i) { Label(m, i, TEXT_COLOR_SYSTEM_WHITE, "Fuse"); return 0; }
static u8 P_DeployAvail(const struct MenuItemDef *d, int n)
{
    return gColRun.rosterCount > COL_MAX_DEPLOY ? MENU_ENABLED : MENU_NOTSHOWN;
}
static u8 P_Fight(struct MenuProc *m, struct MenuItemProc *i) { gColPrepUi.next = NEXT_FIGHT; return END_MENU; }
static u8 P_Deploy(struct MenuProc *m, struct MenuItemProc *i) { gColPrepUi.next = NEXT_DEPLOY; return END_MENU; }
static u8 P_Transfer(struct MenuProc *m, struct MenuItemProc *i) { gColPrepUi.next = NEXT_TRANSFER; return END_MENU; }
static u8 P_Fuse(struct MenuProc *m, struct MenuItemProc *i) { gColPrepUi.next = NEXT_FUSE; return END_MENU; }

static const struct MenuItemDef kPrepItems[] = {
    ROW(MenuAlwaysEnabled, P_Title, NotAnOption),
    ROW(MenuAlwaysEnabled, P_FightDraw, P_Fight),
    ROW(P_DeployAvail, P_DeployDraw, P_Deploy),
    ROW(MenuAlwaysEnabled, P_TransferDraw, P_Transfer),
    ROW(MenuAlwaysEnabled, P_FuseDraw, P_Fuse),
    { 0 },
};
static const struct MenuDef kPrepMenu = { .rect = { 10, 4, 9, 0 }, .menuItems = kPrepItems };

/* ---- unit list (giver / receiver / fuser) ---- */

static int UnitUsable(int slot)
{
    struct Unit *u = Col_RosterUnit(slot);
    int item;

    if (!u)
        return 0;
    switch (gColPrepUi.mode) {
    case MODE_GIVER:
        return u->items[0] != 0;
    case MODE_RECEIVER: {
        struct Unit *from = Col_RosterUnit(gColPrepUi.unitSlot);
        if (slot == gColPrepUi.unitSlot || !from)
            return 0;
        item = from->items[gColPrepUi.itemSlot];
        return Col_CanReceiveItem(u, item);
    }
    case MODE_FUSER: {
        int a, b;
        return Col_FindFusion(u, 0, &a, &b) >= 0;
    }
    }
    return 0;
}

static int U_Title(struct MenuProc *m, struct MenuItemProc *i)
{
    static const char *const kTitles[] = { "Transfer: who gives?", "Transfer: to whom?", "Fuse: whose weapons?" };
    Label(m, i, TEXT_COLOR_SYSTEM_GOLD, kTitles[gColPrepUi.mode]);
    return 0;
}
static u8 U_Avail(const struct MenuItemDef *d, int n)
{
    if (gColRun.roster[n - 1] >= COL_POOL_SIZE)
        return MENU_NOTSHOWN;
    return UnitUsable(n - 1) ? MENU_ENABLED : MENU_DISABLED;
}
static int U_Draw(struct MenuProc *m, struct MenuItemProc *i)
{
    struct Unit *u = Col_RosterUnit(i->itemNumber - 1);
    int color = i->availability == MENU_DISABLED ? TEXT_COLOR_SYSTEM_GRAY : TEXT_COLOR_SYSTEM_WHITE;

    ClearText(&i->text);
    if (u) {
        Text_InsertDrawString(&i->text, 0, color, GetStringFromIndex(UNIT_NAME_ID(u)));
        Text_InsertDrawString(&i->text, 56, color, GetStringFromIndex(u->pClassData->nameTextId));
        Text_InsertDrawString(&i->text, 112, TEXT_COLOR_SYSTEM_GOLD, "Lv");
        Text_InsertDrawNumberOrBlank(&i->text, 136, TEXT_COLOR_SYSTEM_BLUE, u->level);
    }
    PutText(&i->text, Tile(m, i, 0));
    return 0;
}
static u8 U_Select(struct MenuProc *m, struct MenuItemProc *i)
{
    int slot = i->itemNumber - 1;

    if (i->availability == MENU_DISABLED)
        return MENU_ACT_SND6B;
    if (gColPrepUi.mode == MODE_RECEIVER) {
        Col_TransferItem(Col_RosterUnit(gColPrepUi.unitSlot), gColPrepUi.itemSlot, Col_RosterUnit(slot));
    } else
        gColPrepUi.unitSlot = (u8)slot;
    gColPrepUi.next = NEXT_OK;
    return END_MENU;
}

static const struct MenuItemDef kUnitItems[] = {
    ROW(MenuAlwaysEnabled, U_Title, NotAnOption),
    ROW(U_Avail, U_Draw, U_Select), ROW(U_Avail, U_Draw, U_Select), ROW(U_Avail, U_Draw, U_Select),
    ROW(U_Avail, U_Draw, U_Select), ROW(U_Avail, U_Draw, U_Select),
    ROW(MenuAlwaysEnabled, BackDraw, Back),
    { 0 },
};
static const struct MenuDef kUnitMenu = { .rect = { MENU_X, MENU_Y, MENU_W, 0 }, .menuItems = kUnitItems };

/* ---- item list (transfer) ---- */

static int I_Title(struct MenuProc *m, struct MenuItemProc *i) { Label(m, i, TEXT_COLOR_SYSTEM_GOLD, "Transfer: which item?"); return 0; }
static u8 I_Avail(const struct MenuItemDef *d, int n)
{
    struct Unit *u = Col_RosterUnit(gColPrepUi.unitSlot);
    return u && u->items[n - 1] ? MENU_ENABLED : MENU_NOTSHOWN;
}
static int I_Draw(struct MenuProc *m, struct MenuItemProc *i)
{
    struct Unit *u = Col_RosterUnit(gColPrepUi.unitSlot);
    ClearText(&i->text);
    DrawItemMenuLine(&i->text, u->items[i->itemNumber - 1], 0, Tile(m, i, 0));
    return 0;
}
static u8 I_Select(struct MenuProc *m, struct MenuItemProc *i)
{
    gColPrepUi.itemSlot = (u8)(i->itemNumber - 1);
    gColPrepUi.next = NEXT_OK;
    return END_MENU;
}

static const struct MenuItemDef kItemItems[] = {
    ROW(MenuAlwaysEnabled, I_Title, NotAnOption),
    ROW(I_Avail, I_Draw, I_Select), ROW(I_Avail, I_Draw, I_Select), ROW(I_Avail, I_Draw, I_Select),
    ROW(I_Avail, I_Draw, I_Select), ROW(I_Avail, I_Draw, I_Select),
    ROW(MenuAlwaysEnabled, BackDraw, Back),
    { 0 },
};
static const struct MenuDef kItemMenu = { .rect = { MENU_X, MENU_Y, MENU_W, 0 }, .menuItems = kItemItems };

/* ---- fusion list ---- */

static int F_Title(struct MenuProc *m, struct MenuItemProc *i) { Label(m, i, TEXT_COLOR_SYSTEM_GOLD, "Fuse into:"); return 0; }
static u8 F_Avail(const struct MenuItemDef *d, int n)
{
    return n - 1 < gColPrepUi.fusionCount ? MENU_ENABLED : MENU_NOTSHOWN;
}
static int F_Draw(struct MenuProc *m, struct MenuItemProc *i)
{
    struct Unit *u = Col_RosterUnit(gColPrepUi.unitSlot);
    const u8 *pair = gColPrepUi.fusions[i->itemNumber - 1];
    int a = u->items[pair[0]], b = u->items[pair[1]];
    int result = Col_FusionResult(a, b);

    ClearText(&i->text);
    Text_InsertDrawString(&i->text, 16, TEXT_COLOR_SYSTEM_BLUE, GetItemName(result));
    Text_InsertDrawString(&i->text, 80, TEXT_COLOR_SYSTEM_GRAY, (a & 0xFF) == (b & 0xFF) ? "2x" : "+");
    Text_InsertDrawString(&i->text, 96, TEXT_COLOR_SYSTEM_GRAY, GetItemName((a & 0xFF) == (b & 0xFF) ? a : b));
    PutText(&i->text, Tile(m, i, 0));
    DrawIcon(Tile(m, i, 0), GetItemIconId(result), TILEREF(0, 4));
    return 0;
}
static u8 F_Select(struct MenuProc *m, struct MenuItemProc *i)
{
    const u8 *pair = gColPrepUi.fusions[i->itemNumber - 1];
    Col_Fuse(Col_RosterUnit(gColPrepUi.unitSlot), pair[0], pair[1]);
    gColPrepUi.next = NEXT_OK;
    return END_MENU;
}

static const struct MenuItemDef kFuseItems[] = {
    ROW(MenuAlwaysEnabled, F_Title, NotAnOption),
    ROW(F_Avail, F_Draw, F_Select), ROW(F_Avail, F_Draw, F_Select), ROW(F_Avail, F_Draw, F_Select),
    ROW(F_Avail, F_Draw, F_Select), ROW(F_Avail, F_Draw, F_Select),
    ROW(MenuAlwaysEnabled, BackDraw, Back),
    { 0 },
};
static const struct MenuDef kFuseMenu = { .rect = { MENU_X, MENU_Y, MENU_W, 0 }, .menuItems = kFuseItems };

/* ---- the flow ---- */

#define L_PREPARE  0
#define L_TRANSFER 1
#define L_FUSE     2
#define L_END      9

static void Open(const struct MenuDef *def, struct Proc *proc)
{
    struct MenuProc *menu;

    ResetTextFont();                            /* free the previous menu's text tiles (vanilla does) */
    menu = StartMenu(def, proc);
    menu->itemCurrent = 1;
    gColPrepUi.next = NEXT_NONE;
}

static void Prep_Open(struct Proc *proc) { Open(&kPrepMenu, proc); }

static void Prep_Dispatch(struct Proc *proc)
{
    switch (gColPrepUi.next) {
    case NEXT_FIGHT:
        Proc_Goto(proc, L_END);
        break;
    case NEXT_DEPLOY:
        Col_OpenDeployMenu(proc);               /* back to Prepare when it closes */
        break;
    case NEXT_TRANSFER:
        gColPrepUi.mode = MODE_GIVER;
        Proc_Goto(proc, L_TRANSFER);
        break;
    case NEXT_FUSE:
        gColPrepUi.mode = MODE_FUSER;
        Proc_Goto(proc, L_FUSE);
        break;
    }
}

static void OpenUnits(struct Proc *proc) { Open(&kUnitMenu, proc); }

static void Tr_Item(struct Proc *proc)
{
    if (gColPrepUi.next != NEXT_OK) {
        Proc_Goto(proc, L_PREPARE);
        return;
    }
    Open(&kItemMenu, proc);
}

static void Tr_Receiver(struct Proc *proc)
{
    if (gColPrepUi.next != NEXT_OK) {
        Proc_Goto(proc, L_PREPARE);
        return;
    }
    gColPrepUi.mode = MODE_RECEIVER;
    Open(&kUnitMenu, proc);
}

static void Fu_Pairs(struct Proc *proc)
{
    struct Unit *u;
    int from = 0, s1, s2, n;

    if (gColPrepUi.next != NEXT_OK) {
        Proc_Goto(proc, L_PREPARE);
        return;
    }
    u = Col_RosterUnit(gColPrepUi.unitSlot);
    gColPrepUi.fusionCount = 0;
    while (u && gColPrepUi.fusionCount < MAX_FUSIONS && (n = Col_FindFusion(u, from, &s1, &s2)) >= 0) {
        gColPrepUi.fusions[gColPrepUi.fusionCount][0] = (u8)s1;
        gColPrepUi.fusions[gColPrepUi.fusionCount][1] = (u8)s2;
        gColPrepUi.fusionCount++;
        from = n + 1;
    }
    Open(&kFuseMenu, proc);
}

static const struct ProcCmd kProcScr_Prepare[] = {
    PROC_NAME("ColPrepare"),
    PROC_LABEL(L_PREPARE),
    PROC_CALL(Prep_Open),
    PROC_YIELD,
    PROC_CALL(Prep_Dispatch),
    PROC_YIELD,
    PROC_GOTO(L_PREPARE),

    PROC_LABEL(L_TRANSFER),
    PROC_CALL(OpenUnits),
    PROC_YIELD,
    PROC_CALL(Tr_Item),
    PROC_YIELD,
    PROC_CALL(Tr_Receiver),
    PROC_YIELD,
    PROC_GOTO(L_PREPARE),

    PROC_LABEL(L_FUSE),
    PROC_CALL(OpenUnits),
    PROC_YIELD,
    PROC_CALL(Fu_Pairs),
    PROC_YIELD,
    PROC_GOTO(L_PREPARE),

    PROC_LABEL(L_END),
    PROC_END,
};

/* From the battle-start flow (roster_ui.c): blocks it until Fight! is chosen. */
void Col_StartPrepare(struct Proc *parent)
{
    Col_EnsureRosterUnits();                    /* Transfer/Fuse need the units (first battle) */
    Proc_StartBlocking(kProcScr_Prepare, parent);
}
