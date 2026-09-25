/* "Learn a skill" menu (spec 29): a unit is offered one skill.
 *
 *   +----------------------------+
 *   | Lute: learn Luna           |   title (unit, the new skill's icon and name)
 *   | [i] Canto          replace |   the 3 slots; picking one learns the skill there
 *   | [i] Vantage        replace |   (an empty slot just learns it)
 *   |     (empty)                |
 *   | Don't learn                |
 *   +----------------------------+
 *
 * The personal skill is not listed: it can never be replaced (spec 27). B does nothing.
 * Start it with Col_OfferSkill(unit, skill), which runs ColEvt_SkillOffer (the event waits for
 * the menu). Later phases call it from rewards, the shop, Elites and Bosses. */
#include "coliseum.h"
#include "bmunit.h"
#include "proc.h"
#include "fontgrp.h"
#include "uimenu.h"
#include "event.h"
#include "functions.h"
#include "hardware.h"
#include "icon.h"
#include "player_interface.h"

#define SKILL_ICON(id)  ((1 << 8) + (id))

struct ColSkillUi {                             /* COL_UI_SCRATCH + 0x20 */
    struct Unit *unit;
    u8 skill;
    u8 learned;                                 /* 1 if the offer was taken */
};
#define gColSkillUi (*(struct ColSkillUi *)(COL_UI_SCRATCH + 0x20))

#define MENU_X  6
#define MENU_Y  3
#define MENU_W  17

extern const u16 ColEvt_SkillOffer[];           /* src/skills/Skills.event */

static u16 *Tile(struct MenuProc *menu, struct MenuItemProc *item, int dx)
{
    return BG_GetMapBuffer(menu->frontBg) + TILEMAP_INDEX(item->xTile + dx, item->yTile);
}

static void Put(struct MenuProc *menu, struct MenuItemProc *item)
{
    PutText(&item->text, Tile(menu, item, 0));
}

static u8 NotAnOption(struct MenuProc *menu, struct MenuItemProc *item)
{
    return MENU_ACT_SND6B;
}

static int Title_Draw(struct MenuProc *menu, struct MenuItemProc *item)
{
    struct Unit *unit = gColSkillUi.unit;

    ClearText(&item->text);
    Text_InsertDrawString(&item->text, 0, TEXT_COLOR_SYSTEM_GOLD, GetStringFromIndex(UNIT_NAME_ID(unit)));
    Text_InsertDrawString(&item->text, 48, TEXT_COLOR_SYSTEM_WHITE, "learn");
    Text_InsertDrawString(&item->text, 90, TEXT_COLOR_SYSTEM_BLUE, Col_SkillName(gColSkillUi.skill));
    Put(menu, item);
    DrawIcon(Tile(menu, item, 9), SKILL_ICON(gColSkillUi.skill), TILEREF(0, 4));
    return 0;
}

static int Slot_Draw(struct MenuProc *menu, struct MenuItemProc *item)
{
    int skill = Col_SlotSkill(gColSkillUi.unit, item->itemNumber - 1);

    ClearText(&item->text);
    if (skill) {
        Text_InsertDrawString(&item->text, 16, TEXT_COLOR_SYSTEM_WHITE, Col_SkillName(skill));
        Text_InsertDrawString(&item->text, 88, TEXT_COLOR_SYSTEM_GRAY, "replace");
        Put(menu, item);
        DrawIcon(Tile(menu, item, 0), SKILL_ICON(skill), TILEREF(0, 4));
    } else {
        Text_InsertDrawString(&item->text, 16, TEXT_COLOR_SYSTEM_GRAY, "(empty)");
        Put(menu, item);
    }
    return 0;
}

static u8 Slot_Select(struct MenuProc *menu, struct MenuItemProc *item)
{
    int slot = item->itemNumber - 1;

    /* an empty slot is only chosen when it is the first free one */
    if (!Col_SlotSkill(gColSkillUi.unit, slot))
        slot = Col_FreeSkillSlot(gColSkillUi.unit);
    if (!Col_LearnSkill(gColSkillUi.unit, gColSkillUi.skill, slot))
        return MENU_ACT_SND6B;
    gColSkillUi.learned = 1;
    return MENU_ACT_SKIPCURSOR | MENU_ACT_END | MENU_ACT_SND6A | MENU_ACT_CLEAR;
}

static int Decline_Draw(struct MenuProc *menu, struct MenuItemProc *item)
{
    ClearText(&item->text);
    Text_InsertDrawString(&item->text, 0, TEXT_COLOR_SYSTEM_WHITE, "Don't learn");
    Put(menu, item);
    return 0;
}

static u8 Decline_Select(struct MenuProc *menu, struct MenuItemProc *item)
{
    return MENU_ACT_SKIPCURSOR | MENU_ACT_END | MENU_ACT_SND6A | MENU_ACT_CLEAR;
}

#define ROW(draw, select) { "", 0, 0, 0, 0, MenuAlwaysEnabled, draw, select, 0, 0, 0 }

static const struct MenuItemDef kSkillItems[] = {
    ROW(Title_Draw, NotAnOption),
    ROW(Slot_Draw, Slot_Select),
    ROW(Slot_Draw, Slot_Select),
    ROW(Slot_Draw, Slot_Select),
    ROW(Decline_Draw, Decline_Select),
    { 0 },
};
static const struct MenuDef kSkillMenu = { .rect = { MENU_X, MENU_Y, MENU_W, 0 }, .menuItems = kSkillItems };

/* ASMC (ColEvt_SkillOffer): the offer set by Col_OfferSkill. */
void Col_StartSkillOffer(struct Proc *eventProc)
{
    struct MenuProc *menu;
    int free;

    if (!gColSkillUi.unit || !Col_CanLearnSkill(gColSkillUi.unit, gColSkillUi.skill))
        return;
    EndPlayerPhaseSideWindows();                /* the unit window's portrait shares VRAM with icons */
    ResetTextFont();
    menu = StartMenu(&kSkillMenu, eventProc);
    free = Col_FreeSkillSlot(gColSkillUi.unit);
    menu->itemCurrent = (u8)(free >= 0 ? free + 1 : 1);   /* the free slot, or the first to replace */
}

/* The same menu inside another flow (shop): blocks `parent`; Col_SkillMenuLearned() after. */
void Col_OpenSkillMenu(struct Proc *parent, struct Unit *unit, int skill)
{
    gColSkillUi.unit = unit;
    gColSkillUi.skill = (u8)skill;
    gColSkillUi.learned = 0;
    Col_StartSkillOffer(parent);
}

int Col_SkillMenuLearned(void)
{
    return gColSkillUi.learned;
}

/* Offer `skill` to `unit`: returns 0 at once if it can't be learned (prerequisites, duplicate). */
int Col_OfferSkill(struct Unit *unit, int skill)
{
    if (!unit || !Col_CanLearnSkill(unit, skill))
        return 0;
    gColSkillUi.unit = unit;
    gColSkillUi.skill = (u8)skill;
    gColSkillUi.learned = 0;
    CallEvent(ColEvt_SkillOffer, 1);
    return 1;
}
