/* Arena debug menu (ARENA_SPEC 22, spec 81): Prepare > "Arena debug", only in debug and test
 * builds (ColDebugMenu is 1 there, 0 in the player ROM; Arenas.event).
 *
 *   Arena debug
 *   Arena: Volcanic Arena      A: next arena
 *   Weather: Ashfall           A: next weather
 *   Clear weather
 *   Fight here                 this battle restarts in that arena and weather
 *   Back
 *
 * "Fight here" stores the choice in the run state and asks the battle chapter's beginning event
 * to restart the chapter (Col_ArenaDebugRestart, BattleChapter.event), so the map, weather, fog,
 * spawns and enemies all load exactly as for a rolled arena. Hazard and sacred tiles are fixed
 * per arena (they are part of its design); set the arena to get them. */
#include "coliseum.h"
#include "proc.h"
#include "fontgrp.h"
#include "uimenu.h"
#include "hardware.h"
#include "event.h"
#include "functions.h"

extern const u8 ColDebugMenu;                   /* Arenas.event: 1 in debug/test builds */

struct ColArenaDebugUi {                        /* COL_RAM_BASE + 0x3A0: free after the chapter copy */
    u8 arena;
    u8 weather;
    u8 next;
    u8 restart;                                 /* "Fight here" chosen: the beginning event restarts */
};
#define gColArenaDebugUi (*(struct ColArenaDebugUi *)(COL_RAM_BASE + 0x3A0))

enum { NEXT_NONE, NEXT_ARENA, NEXT_WEATHER, NEXT_CLEAR, NEXT_FIGHT, NEXT_BACK };

#define END_MENU (MENU_ACT_SKIPCURSOR | MENU_ACT_END | MENU_ACT_SND6A | MENU_ACT_CLEAR)
#define ROW(avail, draw, select) { "", 0, 0, 0, 0, avail, draw, select, 0, 0, 0 }

int Col_ArenaDebugAvailable(void)
{
    return ColDebugMenu != 0;
}

static u16 *Tile(struct MenuProc *menu, struct MenuItemProc *item)
{
    return BG_GetMapBuffer(menu->frontBg) + TILEMAP_INDEX(item->xTile, item->yTile);
}

static void Line(struct MenuProc *m, struct MenuItemProc *i, int color, const char *a, const char *b)
{
    ClearText(&i->text);
    Text_InsertDrawString(&i->text, 0, color, a);
    if (b)
        Text_InsertDrawString(&i->text, 48, TEXT_COLOR_SYSTEM_BLUE, b);
    PutText(&i->text, Tile(m, i));
}

static u8 NotAnOption(struct MenuProc *m, struct MenuItemProc *i) { return MENU_ACT_SND6B; }
static int D_Title(struct MenuProc *m, struct MenuItemProc *i) { Line(m, i, TEXT_COLOR_SYSTEM_GOLD, "Arena debug", NULL); return 0; }
static int D_ArenaDraw(struct MenuProc *m, struct MenuItemProc *i)
{
    Line(m, i, TEXT_COLOR_SYSTEM_WHITE, "Arena", gColArenas[gColArenaDebugUi.arena].name);
    return 0;
}
static int D_WeatherDraw(struct MenuProc *m, struct MenuItemProc *i)
{
    Line(m, i, TEXT_COLOR_SYSTEM_WHITE, "Weather", Col_WeatherName(gColArenaDebugUi.weather));
    return 0;
}
static int D_ClearDraw(struct MenuProc *m, struct MenuItemProc *i) { Line(m, i, TEXT_COLOR_SYSTEM_WHITE, "Clear weather", NULL); return 0; }
static int D_FightDraw(struct MenuProc *m, struct MenuItemProc *i) { Line(m, i, TEXT_COLOR_SYSTEM_WHITE, "Fight here", NULL); return 0; }
static int D_BackDraw(struct MenuProc *m, struct MenuItemProc *i) { Line(m, i, TEXT_COLOR_SYSTEM_WHITE, "Back", NULL); return 0; }
static u8 D_Arena(struct MenuProc *m, struct MenuItemProc *i) { gColArenaDebugUi.next = NEXT_ARENA; return END_MENU; }
static u8 D_Weather(struct MenuProc *m, struct MenuItemProc *i) { gColArenaDebugUi.next = NEXT_WEATHER; return END_MENU; }
static u8 D_Clear(struct MenuProc *m, struct MenuItemProc *i) { gColArenaDebugUi.next = NEXT_CLEAR; return END_MENU; }
static u8 D_Fight(struct MenuProc *m, struct MenuItemProc *i) { gColArenaDebugUi.next = NEXT_FIGHT; return END_MENU; }
static u8 D_Back(struct MenuProc *m, struct MenuItemProc *i) { gColArenaDebugUi.next = NEXT_BACK; return END_MENU; }

static const struct MenuItemDef kDebugItems[] = {
    ROW(MenuAlwaysEnabled, D_Title, NotAnOption),
    ROW(MenuAlwaysEnabled, D_ArenaDraw, D_Arena),
    ROW(MenuAlwaysEnabled, D_WeatherDraw, D_Weather),
    ROW(MenuAlwaysEnabled, D_ClearDraw, D_Clear),
    ROW(MenuAlwaysEnabled, D_FightDraw, D_Fight),
    ROW(MenuAlwaysEnabled, D_BackDraw, D_Back),
    { 0 },
};
static const struct MenuDef kDebugMenu = { .rect = { 5, 2, 19, 0 }, .menuItems = kDebugItems };

static void Dbg_Open(struct Proc *proc)
{
    static const u8 kCursor[] = { 1, 1, 2, 3, 4, 5 };
    struct MenuProc *menu;

    ResetTextFont();
    menu = StartMenu(&kDebugMenu, proc);
    menu->itemCurrent = kCursor[gColArenaDebugUi.next < sizeof(kCursor) ? gColArenaDebugUi.next : 0];
}

static void Dbg_Dispatch(struct Proc *proc)
{
    switch (gColArenaDebugUi.next) {
    case NEXT_ARENA:
        gColArenaDebugUi.arena = (u8)((gColArenaDebugUi.arena + 1) % Col_ArenaCount());
        return;
    case NEXT_WEATHER:
        gColArenaDebugUi.weather = (u8)((gColArenaDebugUi.weather + 1) % COL_WX_COUNT);
        return;
    case NEXT_CLEAR:
        gColArenaDebugUi.weather = COL_WX_CLEAR;
        return;
    case NEXT_FIGHT:
        Col_SetArena(gColArenaDebugUi.arena, gColArenaDebugUi.weather);
        gColArenaDebugUi.restart = 1;
        Proc_End(proc);
        return;
    default:
        Proc_End(proc);
        return;
    }
}

static const struct ProcCmd kProcScr_ArenaDebug[] = {
    PROC_NAME("ColArenaDebug"),
    PROC_LABEL(0),
    PROC_CALL(Dbg_Open),
    PROC_YIELD,
    PROC_CALL(Dbg_Dispatch),
    PROC_YIELD,
    PROC_GOTO(0),
    PROC_END,
};

/* From the Prepare menu (prepare_ui.c). */
void Col_StartArenaDebug(struct Proc *parent)
{
    gColArenaDebugUi.arena = gColRun.arena < Col_ArenaCount() ? gColRun.arena : 0;
    gColArenaDebugUi.weather = gColRun.weather < COL_WX_COUNT ? gColRun.weather : COL_WX_CLEAR;
    gColArenaDebugUi.next = NEXT_NONE;
    gColArenaDebugUi.restart = 0;
    Proc_StartBlocking(kProcScr_ArenaDebug, parent);
}

int Col_ArenaDebugPending(void)                /* Prepare closes at once when set */
{
    return gColArenaDebugUi.restart;
}

/* ASMC in the battle chapter's beginning event, after the battle-start menus: sC = 1 when
 * "Fight here" was chosen (the event then restarts the chapter). */
void Col_ArenaDebugRestart(struct Proc *eventProc)
{
    gEventSlots[0xC] = gColArenaDebugUi.restart && Col_ArenaDebugAvailable();
    gColArenaDebugUi.restart = 0;
}
