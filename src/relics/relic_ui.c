/* Relic menus (spec 33-35): Prepare > Relics, and the relic info screen the shop shows before
 * buying one.
 *
 *   Relics: whose?        the living roster (name, relics worn "n/2")
 *   Lute's relics         slot 1, slot 2 (name and rarity, or "(empty)")
 *   Slot 1: choose        "Unequip <relic>" when the slot holds one, then every relic that could go
 *                         there: the bag, and relics worn by other units (their name shown: taking
 *                         one transfers it, and a relic in this slot goes back to them - a swap).
 *                         5 per page, "More" pages on.
 *   <relic> <rarity>      info: every effect, good ones in green, drawbacks in gold, conditions
 *                         as a header ("Below 50% HP:"); then Equip / Unequip / Buy, and Back.
 *
 * Every list has Back (B does nothing, as in the other COLOSSEUM menus). */
#include "colosseum.h"
#include "bmunit.h"
#include "proc.h"
#include "fontgrp.h"
#include "uimenu.h"
#include "hardware.h"

enum { NEXT_NONE, NEXT_OK, NEXT_BACK, NEXT_MORE, NEXT_REMOVE, NEXT_ACTION };
enum { MODE_MANAGE, MODE_BUY };
enum { ACT_EQUIP, ACT_REMOVE, ACT_BUY };

#define MAX_CHOICES    (COL_RELIC_BAG + (COL_MAX_ROSTER - 1) * COL_RELIC_SLOTS)
#define PAGE_SIZE      5
#define MAX_INFO_LINES 6
#define SRC_WORN       0x80                     /* choice source: 0x80 | pool << 1 | slot */

struct ColRelicUi {                             /* COL_UI_SCRATCH + 0x50 (48 bytes free there) */
    u8 next;
    u8 mode;
    u8 unitSlot;                                /* roster slot being equipped */
    u8 relicSlot;                               /* 0 or 1 */
    u8 page;
    u8 count;                                   /* choices */
    u8 choice;                                  /* chosen choice (index into src) */
    u8 infoRelic;
    u8 infoAction;
    u8 bought;
    u8 lineCount;
    u8 src[MAX_CHOICES];                        /* bag index, or SRC_WORN | pool << 1 | slot */
    u8 lines[MAX_INFO_LINES];                   /* mod index; 0x80 | mod index = condition header */
};
#define gColRelicUi (*(struct ColRelicUi *)(COL_UI_SCRATCH + 0x50))
_Static_assert(sizeof(struct ColRelicUi) <= 0x30, "relic UI scratch must stay within 0x50-0x7F");

#define MENU_X  3
#define MENU_Y  2
#define MENU_W  23
#define COL_RIGHT 120                           /* rarity / wearer column */

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
    gColRelicUi.next = NEXT_BACK;
    return END_MENU;
}
static int BackDraw(struct MenuProc *m, struct MenuItemProc *i) { Label(m, i, TEXT_COLOR_SYSTEM_WHITE, "Back"); return 0; }

static int UnitPool(void)
{
    return gColRun.roster[gColRelicUi.unitSlot];
}

static const char *RelicName(int relic)
{
    const struct ColRelicDef *def = Col_RelicDef(relic);
    return def ? def->name : "";
}

static const char *PoolName(int pool)
{
    return GetStringFromIndex(GetCharacterData(gColPool[pool].charId)->nameTextId);
}

/* ---- unit list ---- */

static int WornCount(int pool)
{
    int s, n = 0;
    for (s = 0; s < COL_RELIC_SLOTS; s++)
        if (gColRun.relics[pool][s])
            n++;
    return n;
}

static int U_Title(struct MenuProc *m, struct MenuItemProc *i) { Label(m, i, TEXT_COLOR_SYSTEM_GOLD, "Relics: whose?"); return 0; }
static u8 U_Avail(const struct MenuItemDef *d, int n)
{
    return gColRun.roster[n - 1] < COL_POOL_SIZE && Col_RosterUnit(n - 1) ? MENU_ENABLED : MENU_NOTSHOWN;
}
static int U_Draw(struct MenuProc *m, struct MenuItemProc *i)
{
    int pool = gColRun.roster[i->itemNumber - 1];

    ClearText(&i->text);
    Text_InsertDrawString(&i->text, 0, TEXT_COLOR_SYSTEM_WHITE, PoolName(pool));
    Text_InsertDrawString(&i->text, 64, TEXT_COLOR_SYSTEM_GOLD, "Relics");
    Text_InsertDrawNumberOrBlank(&i->text, 112, TEXT_COLOR_SYSTEM_BLUE, WornCount(pool));
    Text_InsertDrawString(&i->text, 120, TEXT_COLOR_SYSTEM_GRAY, "/2");
    PutText(&i->text, Tile(m, i, 0));
    return 0;
}
static u8 U_Select(struct MenuProc *m, struct MenuItemProc *i)
{
    gColRelicUi.unitSlot = (u8)(i->itemNumber - 1);
    gColRelicUi.next = NEXT_OK;
    return END_MENU;
}

static const struct MenuItemDef kUnitItems[] = {
    ROW(MenuAlwaysEnabled, U_Title, NotAnOption),
    ROW(U_Avail, U_Draw, U_Select), ROW(U_Avail, U_Draw, U_Select), ROW(U_Avail, U_Draw, U_Select),
    ROW(U_Avail, U_Draw, U_Select), ROW(U_Avail, U_Draw, U_Select),
    ROW(MenuAlwaysEnabled, BackDraw, Back),
    { 0 },
};
static u8 U_Help(struct MenuProc *m, struct MenuItemProc *i)
{
    if (i->itemNumber >= 1 && i->itemNumber <= COL_MAX_ROSTER)
        Col_HelpWornRelics(i, gColRun.roster[i->itemNumber - 1]);
    return 0;
}
static const struct MenuDef kUnitMenu = { .rect = { MENU_X, MENU_Y, MENU_W, 0 }, .menuItems = kUnitItems, COL_HELP_MENU(U_Help) };

/* ---- the unit's two slots ---- */

static int S_Title(struct MenuProc *m, struct MenuItemProc *i)
{
    char buf[32];
    const char *name = PoolName(UnitPool()), *tail = "'s relics";
    int n = 0;

    while (name[n] && n < 20) {                 /* "Lute's relics" */
        buf[n] = name[n];
        n++;
    }
    while (*tail)
        buf[n++] = *tail++;
    buf[n] = 0;
    Label(m, i, TEXT_COLOR_SYSTEM_GOLD, buf);
    return 0;
}
static int S_Draw(struct MenuProc *m, struct MenuItemProc *i)
{
    int relic = gColRun.relics[UnitPool()][i->itemNumber - 1];

    ClearText(&i->text);
    if (relic) {
        Text_InsertDrawString(&i->text, 0, TEXT_COLOR_SYSTEM_WHITE, RelicName(relic));
        Text_InsertDrawString(&i->text, COL_RIGHT, TEXT_COLOR_SYSTEM_GRAY, Col_RelicRarityName(Col_RelicDef(relic)->rarity));
    } else
        Text_InsertDrawString(&i->text, 0, TEXT_COLOR_SYSTEM_GRAY, "(empty)");
    PutText(&i->text, Tile(m, i, 0));
    return 0;
}
static u8 S_Select(struct MenuProc *m, struct MenuItemProc *i)
{
    gColRelicUi.relicSlot = (u8)(i->itemNumber - 1);
    gColRelicUi.next = NEXT_OK;
    return END_MENU;
}

static const struct MenuItemDef kSlotItems[] = {
    ROW(MenuAlwaysEnabled, S_Title, NotAnOption),
    ROW(MenuAlwaysEnabled, S_Draw, S_Select), ROW(MenuAlwaysEnabled, S_Draw, S_Select),
    ROW(MenuAlwaysEnabled, BackDraw, Back),
    { 0 },
};
static u8 S_Help(struct MenuProc *m, struct MenuItemProc *i)
{
    if (i->itemNumber >= 1 && i->itemNumber <= COL_RELIC_SLOTS)
        Col_HelpRelic(i, gColRun.relics[UnitPool()][i->itemNumber - 1]);
    return 0;
}
static const struct MenuDef kSlotMenu = { .rect = { MENU_X, MENU_Y, MENU_W, 0 }, .menuItems = kSlotItems, COL_HELP_MENU(S_Help) };

/* ---- choosing a relic for the slot ---- */

static int ChoiceRelic(int c)
{
    int s = gColRelicUi.src[c];
    if (s & SRC_WORN)
        return gColRun.relics[(s & 0x7F) >> 1][s & 1];
    return gColRun.relicBag[s];
}

static void BuildChoices(void)
{
    int i, slot, s, n = 0, own = UnitPool();

    for (i = 0; i < COL_RELIC_BAG; i++)
        if (gColRun.relicBag[i])
            gColRelicUi.src[n++] = (u8)i;
    for (slot = 0; slot < COL_MAX_ROSTER; slot++) {
        int pool = gColRun.roster[slot];
        if (pool >= COL_POOL_SIZE || pool == own)
            continue;
        for (s = 0; s < COL_RELIC_SLOTS; s++)
            if (gColRun.relics[pool][s])
                gColRelicUi.src[n++] = (u8)(SRC_WORN | pool << 1 | s);
    }
    gColRelicUi.count = (u8)n;
    gColRelicUi.page = 0;
}

static int CurrentRelic(void)
{
    return gColRun.relics[UnitPool()][gColRelicUi.relicSlot];
}

static int C_Title(struct MenuProc *m, struct MenuItemProc *i)
{
    ClearText(&i->text);
    Text_InsertDrawString(&i->text, 0, TEXT_COLOR_SYSTEM_GOLD, gColRelicUi.relicSlot ? "Slot 2:" : "Slot 1:");
    Text_InsertDrawString(&i->text, 40, TEXT_COLOR_SYSTEM_GOLD, gColRelicUi.count ? "choose a relic" : "no relics to equip");
    PutText(&i->text, Tile(m, i, 0));
    return 0;
}
static u8 C_RemoveAvail(const struct MenuItemDef *d, int n) { return CurrentRelic() ? MENU_ENABLED : MENU_NOTSHOWN; }
static int C_RemoveDraw(struct MenuProc *m, struct MenuItemProc *i)
{
    ClearText(&i->text);
    Text_InsertDrawString(&i->text, 0, TEXT_COLOR_SYSTEM_WHITE, "Unequip");
    Text_InsertDrawString(&i->text, 56, TEXT_COLOR_SYSTEM_BLUE, RelicName(CurrentRelic()));
    PutText(&i->text, Tile(m, i, 0));
    return 0;
}
static u8 C_Remove(struct MenuProc *m, struct MenuItemProc *i)
{
    gColRelicUi.next = NEXT_REMOVE;
    return END_MENU;
}

static int ChoiceOf(int n)                      /* choice index of row n (rows 2..6), or -1 */
{
    int c = gColRelicUi.page * PAGE_SIZE + (n - 2);
    return c < gColRelicUi.count ? c : -1;
}
static u8 C_Avail(const struct MenuItemDef *d, int n) { return ChoiceOf(n) >= 0 ? MENU_ENABLED : MENU_NOTSHOWN; }
static int C_Draw(struct MenuProc *m, struct MenuItemProc *i)
{
    int c = ChoiceOf(i->itemNumber), s = gColRelicUi.src[c];

    ClearText(&i->text);
    Text_InsertDrawString(&i->text, 0, TEXT_COLOR_SYSTEM_WHITE, RelicName(ChoiceRelic(c)));
    Text_InsertDrawString(&i->text, COL_RIGHT, TEXT_COLOR_SYSTEM_GRAY, (s & SRC_WORN) ? PoolName((s & 0x7F) >> 1) : "Bag");
    PutText(&i->text, Tile(m, i, 0));
    return 0;
}
static u8 C_Select(struct MenuProc *m, struct MenuItemProc *i)
{
    gColRelicUi.choice = (u8)ChoiceOf(i->itemNumber);
    gColRelicUi.next = NEXT_OK;
    return END_MENU;
}
static u8 C_MoreAvail(const struct MenuItemDef *d, int n) { return gColRelicUi.count > PAGE_SIZE ? MENU_ENABLED : MENU_NOTSHOWN; }
static int C_MoreDraw(struct MenuProc *m, struct MenuItemProc *i) { Label(m, i, TEXT_COLOR_SYSTEM_WHITE, "More..."); return 0; }
static u8 C_More(struct MenuProc *m, struct MenuItemProc *i)
{
    gColRelicUi.next = NEXT_MORE;
    return END_MENU;
}

static const struct MenuItemDef kChoiceItems[] = {
    ROW(MenuAlwaysEnabled, C_Title, NotAnOption),
    ROW(C_RemoveAvail, C_RemoveDraw, C_Remove),
    ROW(C_Avail, C_Draw, C_Select), ROW(C_Avail, C_Draw, C_Select), ROW(C_Avail, C_Draw, C_Select),
    ROW(C_Avail, C_Draw, C_Select), ROW(C_Avail, C_Draw, C_Select),
    ROW(C_MoreAvail, C_MoreDraw, C_More),
    ROW(MenuAlwaysEnabled, BackDraw, Back),
    { 0 },
};
static u8 C_Help(struct MenuProc *m, struct MenuItemProc *i)
{
    int c;
    if (i->itemNumber == 1)                     /* Unequip: the relic worn now */
        Col_HelpRelic(i, CurrentRelic());
    else if ((c = ChoiceOf(i->itemNumber)) >= 0 && i->itemNumber >= 2 && i->itemNumber <= 6)
        Col_HelpRelic(i, ChoiceRelic(c));
    return 0;
}
static const struct MenuDef kChoiceMenu = { .rect = { MENU_X, 0, MENU_W, 0 }, .menuItems = kChoiceItems, COL_HELP_MENU(C_Help) };

/* ---- relic info ---- */

static void BuildLines(void)
{
    const struct ColRelicDef *def = Col_RelicDef(gColRelicUi.infoRelic);
    int m, cond = COL_RC_ALWAYS, n = 0;

    for (m = 0; def && m < COL_RELIC_MODS && def->mods[m].kind != COL_RM_END && n < MAX_INFO_LINES; m++) {
        if (def->mods[m].cond != cond) {
            cond = def->mods[m].cond;
            if (cond != COL_RC_ALWAYS)
                gColRelicUi.lines[n++] = (u8)(0x80 | m);
            if (n >= MAX_INFO_LINES)
                break;
        }
        gColRelicUi.lines[n++] = (u8)m;
    }
    gColRelicUi.lineCount = (u8)n;
}

static int Info_Title(struct MenuProc *m, struct MenuItemProc *i)
{
    const struct ColRelicDef *def = Col_RelicDef(gColRelicUi.infoRelic);

    ClearText(&i->text);
    Text_InsertDrawString(&i->text, 0, TEXT_COLOR_SYSTEM_BLUE, def->name);
    Text_InsertDrawString(&i->text, COL_RIGHT, TEXT_COLOR_SYSTEM_GOLD, Col_RelicRarityName(def->rarity));
    PutText(&i->text, Tile(m, i, 0));
    return 0;
}
static u8 Info_LineAvail(const struct MenuItemDef *d, int n) { return n - 1 < gColRelicUi.lineCount ? MENU_ENABLED : MENU_NOTSHOWN; }
static int Info_LineDraw(struct MenuProc *m, struct MenuItemProc *i)
{
    const struct ColRelicDef *def = Col_RelicDef(gColRelicUi.infoRelic);
    int line = gColRelicUi.lines[i->itemNumber - 1];
    const struct ColRelicMod *mod = &def->mods[line & 0x7F];
    char buf[32];

    ClearText(&i->text);
    if (line & 0x80) {
        Text_InsertDrawString(&i->text, 0, TEXT_COLOR_SYSTEM_WHITE,
                              mod->cond == COL_RC_BELOW_HALF_HP ? "Below 50% HP:" : "When active:");
    } else {
        int good = Col_RelicModText(mod, buf);
        Text_InsertDrawString(&i->text, mod->cond ? 8 : 0, good ? TEXT_COLOR_SYSTEM_GREEN : TEXT_COLOR_SYSTEM_GOLD, buf);
    }
    PutText(&i->text, Tile(m, i, 0));
    return 0;
}

static int ActionEnabled(void)
{
    switch (gColRelicUi.infoAction) {
    case ACT_REMOVE: return Col_RelicBagCount() < COL_RELIC_BAG;
    default:         return 1;              /* equip: a swap never needs room; buy: checked by the shop */
    }
}
static u8 Info_ActionAvail(const struct MenuItemDef *d, int n) { return ActionEnabled() ? MENU_ENABLED : MENU_DISABLED; }
static int Info_ActionDraw(struct MenuProc *m, struct MenuItemProc *i)
{
    static const char *const kAction[] = { "Equip", "Unequip", "Buy" };
    int color = i->availability == MENU_DISABLED ? TEXT_COLOR_SYSTEM_GRAY : TEXT_COLOR_SYSTEM_WHITE;

    ClearText(&i->text);
    Text_InsertDrawString(&i->text, 0, color, kAction[gColRelicUi.infoAction]);
    if (gColRelicUi.infoAction == ACT_REMOVE && color == TEXT_COLOR_SYSTEM_GRAY)
        Text_InsertDrawString(&i->text, 56, TEXT_COLOR_SYSTEM_GRAY, "(bag full)");
    PutText(&i->text, Tile(m, i, 0));
    return 0;
}
static u8 Info_Action(struct MenuProc *m, struct MenuItemProc *i)
{
    if (i->availability == MENU_DISABLED)
        return MENU_ACT_SND6B;
    gColRelicUi.next = NEXT_ACTION;
    return END_MENU;
}

static const struct MenuItemDef kInfoItems[] = {
    ROW(MenuAlwaysEnabled, Info_Title, NotAnOption),
    ROW(Info_LineAvail, Info_LineDraw, NotAnOption), ROW(Info_LineAvail, Info_LineDraw, NotAnOption),
    ROW(Info_LineAvail, Info_LineDraw, NotAnOption), ROW(Info_LineAvail, Info_LineDraw, NotAnOption),
    ROW(Info_LineAvail, Info_LineDraw, NotAnOption), ROW(Info_LineAvail, Info_LineDraw, NotAnOption),
    ROW(Info_ActionAvail, Info_ActionDraw, Info_Action),
    ROW(MenuAlwaysEnabled, BackDraw, Back),
    { 0 },
};
static const struct MenuDef kInfoMenu = { .rect = { MENU_X, 0, MENU_W, 0 }, .menuItems = kInfoItems };

/* ---- the flow ---- */

#define L_UNITS   0
#define L_SLOTS   1
#define L_CHOICES 2
#define L_INFO    3
#define L_END     9

static void Open(const struct MenuDef *def, struct Proc *proc, int cursor)
{
    struct MenuProc *menu;

    ResetTextFont();                            /* free the previous menu's text tiles */
    menu = StartMenu(def, proc);
    menu->itemCurrent = (u8)cursor;
    gColRelicUi.next = NEXT_NONE;
}

static void Units_Open(struct Proc *proc) { Open(&kUnitMenu, proc, 1); }
static void Units_Dispatch(struct Proc *proc)
{
    if (gColRelicUi.next != NEXT_OK)
        Proc_Goto(proc, L_END);
}

static void Slots_Open(struct Proc *proc) { Open(&kSlotMenu, proc, 1); }
static void Slots_Dispatch(struct Proc *proc)
{
    if (gColRelicUi.next != NEXT_OK) {
        Proc_Goto(proc, L_UNITS);
        return;
    }
    BuildChoices();
}

static void Choices_Open(struct Proc *proc) { Open(&kChoiceMenu, proc, CurrentRelic() && gColRelicUi.count ? 2 : 1); }
static void Choices_Dispatch(struct Proc *proc)
{
    switch (gColRelicUi.next) {
    case NEXT_MORE:
        gColRelicUi.page = (u8)((gColRelicUi.page + 1) * PAGE_SIZE < gColRelicUi.count ? gColRelicUi.page + 1 : 0);
        Proc_Goto(proc, L_CHOICES);
        return;
    case NEXT_REMOVE:
        gColRelicUi.infoRelic = (u8)CurrentRelic();
        gColRelicUi.infoAction = ACT_REMOVE;
        break;
    case NEXT_OK:
        gColRelicUi.infoRelic = (u8)ChoiceRelic(gColRelicUi.choice);
        gColRelicUi.infoAction = ACT_EQUIP;
        break;
    default:
        Proc_Goto(proc, L_SLOTS);
        return;
    }
    BuildLines();
}

static void Info_Open(struct Proc *proc) { Open(&kInfoMenu, proc, 1 + gColRelicUi.lineCount); }
static void Info_Dispatch(struct Proc *proc)
{
    int pool, s;

    if (gColRelicUi.mode == MODE_BUY) {
        gColRelicUi.bought = gColRelicUi.next == NEXT_ACTION;
        Proc_Goto(proc, L_END);
        return;
    }
    if (gColRelicUi.next != NEXT_ACTION) {
        Proc_Goto(proc, L_CHOICES);
        return;
    }
    pool = UnitPool();
    if (gColRelicUi.infoAction == ACT_REMOVE)
        Col_RelicUnequip(pool, gColRelicUi.relicSlot);
    else {
        s = gColRelicUi.src[gColRelicUi.choice];
        if (s & SRC_WORN)
            Col_RelicMove((s & 0x7F) >> 1, s & 1, pool, gColRelicUi.relicSlot);
        else
            Col_RelicEquipFromBag(pool, gColRelicUi.relicSlot, s);
    }
    Proc_Goto(proc, L_SLOTS);
}

static void Start(struct Proc *proc)
{
    Proc_Goto(proc, gColRelicUi.mode == MODE_BUY ? L_INFO : L_UNITS);
}

static const struct ProcCmd kProcScr_Relics[] = {
    PROC_NAME("ColRelics"),
    PROC_CALL(Start),

    PROC_LABEL(L_UNITS),
    PROC_CALL(Units_Open),
    PROC_YIELD,
    PROC_CALL(Units_Dispatch),

    PROC_LABEL(L_SLOTS),
    PROC_CALL(Slots_Open),
    PROC_YIELD,
    PROC_CALL(Slots_Dispatch),

    PROC_LABEL(L_CHOICES),
    PROC_CALL(Choices_Open),
    PROC_YIELD,
    PROC_CALL(Choices_Dispatch),

    PROC_LABEL(L_INFO),
    PROC_CALL(Info_Open),
    PROC_YIELD,
    PROC_CALL(Info_Dispatch),
    PROC_GOTO(L_SLOTS),

    PROC_LABEL(L_END),
    PROC_END,
};

/* Prepare > Relics (prepare_ui.c): blocks `parent` until Back. */
void Col_OpenRelicMenu(struct Proc *parent)
{
    Col_EnsureRosterUnits();
    gColRelicUi.mode = MODE_MANAGE;
    Proc_StartBlocking(kProcScr_Relics, parent);
}

/* The shop's relic entry: its info screen with Buy / Back. Blocks `parent`; then
 * Col_RelicBuyConfirmed() tells whether Buy was chosen (the shop delivers and pays). */
void Col_OpenRelicBuy(struct Proc *parent, int relic)
{
    gColRelicUi.mode = MODE_BUY;
    gColRelicUi.bought = 0;
    gColRelicUi.infoRelic = (u8)relic;
    gColRelicUi.infoAction = ACT_BUY;
    BuildLines();
    Proc_StartBlocking(kProcScr_Relics, parent);
}

int Col_RelicBuyConfirmed(void)
{
    return gColRelicUi.bought;
}
