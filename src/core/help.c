/* R-button help in COLOSSEUM's menus (developer request, 2026-09-27): pressing R on a menu row
 * shows FE8's help box with what the row is, without choosing it; moving the cursor updates it;
 * R or B closes it (FE8's own menu help mode: MenuDef.onRPress = MenuAutoHelpBoxSelect, and
 * onHelpBox = the menu's function, which calls one of these).
 *
 *   skill   the Skill System's description ("Vantage: Always strike first...")
 *   relic   "Iron Heart (Common)" and one line per effect, conditions after them
 *   item    FE8's weapon / item help box (Mt, Hit, Wt, Rng... or the description)
 *   pool    a character: their personal skill's description
 *   text    any other short text
 *
 * Relic and other built text: the anti-Huffman patch lets a text ID point at a plain string
 * (0x80000000 | address), so text ID ColText_Help points at a RAM buffer
 * (COL_RAM_BASE + 0x400, Colosseum.event) that is filled before the help box opens. FE8 caches the
 * last decoded text ID (0x0202A6AC), so the cache is cleared first. */
#include "colosseum.h"
#include "bmunit.h"
#include "bmitem.h"
#include "uimenu.h"
#include "statscreen.h"

#define HELP_BUF      ((char *)(COL_RAM_BASE + 0x400))
#define HELP_BUF_SIZE 0x100
#define HELP_NEWLINE  1                         /* FE8 text: new line */
#define gLastMsgId    (*(u32 *)0x0202A6AC)      /* GetStringFromIndex's cache (anti-Huffman patch) */

extern const u16 ColHelpTextId;                 /* Colosseum.event: the text ID of the RAM buffer */
extern const u16 SkillDescTable[];
extern const u8 PersonalSkillTable[];
void StartHelpBox(int x, int y, int msgId);

static void Box(struct MenuItemProc *item, int msgId)
{
    gLastMsgId = 0xFFFFFFFF;
    StartHelpBox(item->xTile * 8, item->yTile * 8, msgId);
}

static char *Put(char *out, const char *s)
{
    while (*s && out < HELP_BUF + HELP_BUF_SIZE - 2)
        *out++ = *s++;
    *out = 0;
    return out;
}

void Col_HelpText(struct MenuItemProc *item, const char *text)
{
    char *out = HELP_BUF;

    for (; *text && out < HELP_BUF + HELP_BUF_SIZE - 1; text++)
        *out++ = *text == '\n' ? HELP_NEWLINE : *text;
    *out = 0;
    Box(item, ColHelpTextId);
}

void Col_HelpSkill(struct MenuItemProc *item, int skill)
{
    if (skill > 0 && skill < 0xFF && SkillDescTable[skill])
        Box(item, SkillDescTable[skill]);
}

/* Writes "Name (Rarity)" and the effect lines of `relic` at out; returns the end. */
static char *RelicLines(char *out, int relic)
{
    const struct ColRelicDef *def = Col_RelicDef(relic);
    char buf[32];
    int m;

    if (!def)
        return out;
    out = Put(out, def->name);
    out = Put(out, " (");
    out = Put(out, Col_RelicRarityName(def->rarity));
    out = Put(out, ")");
    for (m = 0; m < COL_RELIC_MODS && def->mods[m].kind != COL_RM_END; m++) {
        *out++ = HELP_NEWLINE;
        Col_RelicModText(&def->mods[m], buf);
        out = Put(out, buf);
        if (def->mods[m].cond == COL_RC_BELOW_HALF_HP)
            out = Put(out, " below 50% HP");
    }
    return out;
}

void Col_HelpRelic(struct MenuItemProc *item, int relic)
{
    if (!Col_RelicDef(relic))
        return;
    RelicLines(HELP_BUF, relic);
    Box(item, ColHelpTextId);
}

/* Both relics a character wears (relic menu's unit list). */
void Col_HelpWornRelics(struct MenuItemProc *item, int pool)
{
    char *out = HELP_BUF;
    int s, any = 0;

    *out = 0;
    for (s = 0; pool >= 0 && pool < COL_POOL_SIZE && s < COL_RELIC_SLOTS; s++) {
        int relic = gColRun.relics[pool][s];
        if (!Col_RelicDef(relic))
            continue;
        if (any)
            *out++ = HELP_NEWLINE;
        out = RelicLines(out, relic);
        any = 1;
    }
    if (!any)
        Put(HELP_BUF, "No relics equipped.");
    Box(item, ColHelpTextId);
}

void Col_HelpItem(struct MenuItemProc *item, int itemId)
{
    if (itemId)
        StartItemHelpBox(item->xTile * 8, item->yTile * 8, itemId);
}

/* A pool character: their personal skill (the rest is on the screen). */
void Col_HelpPool(struct MenuItemProc *item, int pool)
{
    int skill;

    if (pool < 0 || pool >= COL_POOL_SIZE)
        return;
    skill = PersonalSkillTable[gColPool[pool].charId];
    if (skill > 0 && skill < 0xFF && SkillDescTable[skill])
        Box(item, SkillDescTable[skill]);
}

/* The same for a unit: personal skill, then the slot skills by name. */
void Col_HelpUnit(struct MenuItemProc *item, struct Unit *unit)
{
    char *out = HELP_BUF;
    int s, skill;

    if (!unit)
        return;
    skill = Col_PersonalSkill(unit);
    out = Put(out, "Personal: ");
    out = Put(out, skill ? Col_SkillName(skill) : "-");
    for (s = 0; s < COL_SKILL_SLOTS; s++) {
        skill = Col_SlotSkill(unit, s);
        if (!skill)
            continue;
        *out++ = HELP_NEWLINE;
        out = Put(out, "Skill: ");
        out = Put(out, Col_SkillName(skill));
    }
    Box(item, ColHelpTextId);
}
