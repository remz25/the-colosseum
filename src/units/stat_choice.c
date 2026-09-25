/* The level-up stat choice menu (spec 23): after the growth rolls, pick 1 of 3 random +1 stats.
 *
 *   +------------------+
 *   | Lute       Lv  6 |   title (not selectable)
 *   | +1 Mag        9  |   the stat's value after the bonus
 *   | +1 Spd        8  |
 *   | +1 Mag        9  |   duplicates allowed
 *   +------------------+
 *
 * Shown through events (the event waits while the menu is open):
 *   - after a player-phase battle: the Skill System's post-combat loop calls
 *     Col_PostCombatStatChoice (PostBattleCalcLoop.event), which runs ColEvt_StatChoice;
 *   - at the start of each player phase (level-ups on the enemy phase) and at the end of a
 *     battle: the battle chapter's events call Col_StartStatChoices (ASMC).
 * Every owed level gets its own menu; B does nothing (a choice must be made). */
#include "coliseum.h"
#include "bmunit.h"
#include "proc.h"
#include "fontgrp.h"
#include "uimenu.h"
#include "event.h"
#include "functions.h"
#include "hardware.h"

struct ColChoiceUi {
    struct Unit *unit;
    s8 slot;
    u8 options[COL_STAT_CHOICES];
};
#define gColChoiceUi (*(struct ColChoiceUi *)COL_UI_SCRATCH)

#define MENU_X      9
#define MENU_Y      3
#define MENU_W      13
#define VALUE_X     80      /* numbers are drawn right-aligned at this pixel */

extern const u16 ColEvt_StatChoice[];       /* src/units/Units.event */

static void Put(struct MenuProc *menu, struct MenuItemProc *item)
{
    PutText(&item->text, BG_GetMapBuffer(menu->frontBg) + TILEMAP_INDEX(item->xTile, item->yTile));
}

static int Title_Draw(struct MenuProc *menu, struct MenuItemProc *item)
{
    struct Unit *unit = gColChoiceUi.unit;

    Text_InsertDrawString(&item->text, 0, TEXT_COLOR_SYSTEM_GOLD, GetStringFromIndex(UNIT_NAME_ID(unit)));
    Text_InsertDrawString(&item->text, 52, TEXT_COLOR_SYSTEM_GOLD, "Lv");
    Text_InsertDrawNumberOrBlank(&item->text, VALUE_X, TEXT_COLOR_SYSTEM_BLUE,
                                 gColRun.choiceLevel[gColChoiceUi.slot] + 1);
    Put(menu, item);
    return 0;
}

static u8 Title_Select(struct MenuProc *menu, struct MenuItemProc *item)
{
    return MENU_ACT_SND6B;                          /* not an option */
}

static int Option_Draw(struct MenuProc *menu, struct MenuItemProc *item)
{
    int stat = gColChoiceUi.options[item->itemNumber - 1];

    Text_InsertDrawString(&item->text, 0, TEXT_COLOR_SYSTEM_WHITE, "+1");
    Text_InsertDrawString(&item->text, 16, TEXT_COLOR_SYSTEM_WHITE, Col_StatName(stat));
    Text_InsertDrawNumberOrBlank(&item->text, VALUE_X, TEXT_COLOR_SYSTEM_BLUE,
                                 Col_GetStat(gColChoiceUi.unit, stat) + 1);
    Put(menu, item);
    return 0;
}

static u8 Option_Select(struct MenuProc *menu, struct MenuItemProc *item)
{
    Col_TakeStatChoice(gColChoiceUi.slot, gColChoiceUi.options[item->itemNumber - 1]);
    return MENU_ACT_SKIPCURSOR | MENU_ACT_END | MENU_ACT_SND6A | MENU_ACT_CLEAR;
}

#define OPTION { "", 0, 0, 0, 0, MenuAlwaysEnabled, Option_Draw, Option_Select, 0, 0, 0 }

static const struct MenuItemDef kChoiceItems[] = {
    { "", 0, 0, 0, 0, MenuAlwaysEnabled, Title_Draw, Title_Select, 0, 0, 0 },
    OPTION, OPTION, OPTION,
    { 0 },
};

static const struct MenuDef kChoiceMenu = {
    .rect = { MENU_X, MENU_Y, MENU_W, 0 },
    .style = 0,
    .menuItems = kChoiceItems,
};

/* One menu per owed level, until none is owed. */
static void Choices_Next(struct Proc *proc)
{
    struct MenuProc *menu;
    int slot = Col_FindPendingChoice();

    if (slot < 0) {
        Proc_Goto(proc, 1);
        return;
    }
    gColChoiceUi.slot = (s8)slot;
    gColChoiceUi.unit = Col_RosterUnit(slot);
    Col_RollStatChoices(gColChoiceUi.options);
    ResetTextFont();                            /* one menu per owed level: free the last one's text */
    menu = StartMenu(&kChoiceMenu, proc);
    menu->itemCurrent = 1;                          /* cursor on the first option */
}

static const struct ProcCmd kProcScr_StatChoices[] = {
    PROC_NAME("ColStatChoice"),
    PROC_LABEL(0),
    PROC_CALL(Choices_Next),
    PROC_YIELD,
    PROC_GOTO(0),
    PROC_LABEL(1),
    PROC_END,
};

/* ASMC: the calling event waits until every owed choice is made. */
void Col_StartStatChoices(struct Proc *eventProc)
{
    if (Col_FindPendingChoice() >= 0)
        Proc_StartBlocking(kProcScr_StatChoices, eventProc);
}

/* Skill System post-combat loop entry (r0 = actor, r1 = target), player phase. */
void Col_PostCombatStatChoice(struct Unit *actor, struct Unit *target)
{
    if (Col_FindPendingChoice() >= 0)
        CallEvent(ColEvt_StatChoice, 1);
}
