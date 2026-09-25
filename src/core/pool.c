/* The playable character pool (spec 10: 15 characters, all eligible from the start).
 *
 * PLACEHOLDER (Phase 3): 15 vanilla FE8 characters, one per class the spec lists, so the roster,
 * battles and tests can be built now. Phase 4 replaces this table with COLISEUM's own 15
 * characters (names, growths, personal skills, starting equipment); nothing else changes. */
#include "coliseum.h"

/* charId: FE8 character; class 0 = the character's default class; items: starting inventory. */
const struct ColPoolEntry gColPool[COL_POOL_SIZE] = {
    { 0x20, 0, { 0x01, 0x6C } },  /* Joshua     Myrmidon    Iron Sword */
    { 0x14, 0, { 0x01, 0x6C } },  /* Gerik      Mercenary   Iron Sword */
    { 0x0A, 0, { 0x1F, 0x6C } },  /* Garcia     Fighter     Iron Axe */
    { 0x03, 0, { 0x14, 0x6C } },  /* Gilliam    Knight      Iron Lance */
    { 0x04, 0, { 0x14, 0x01, 0x6C } }, /* Franz Cavalier    Iron Lance, Iron Sword */
    { 0x08, 0, { 0x2D, 0x6C } },  /* Neimi      Archer      Iron Bow */
    { 0x09, 0, { 0x01, 0x6C } },  /* Colm       Thief       Iron Sword */
    { 0x06, 0, { 0x14, 0x6C } },  /* Vanessa    Peg. Knight Iron Lance */
    { 0x0E, 0, { 0x14, 0x6C } },  /* Cormag     Wyvern      Iron Lance */
    { 0x0C, 0, { 0x38, 0x6C } },  /* Lute       Mage        Fire */
    { 0x1F, 0, { 0x45, 0x6C } },  /* Knoll      Shaman      Flux */
    { 0x0D, 0, { 0x4B, 0x6C } },  /* Natasha    Cleric      Heal */
    { 0x13, 0, { 0x3F, 0x6C } },  /* Artur      Monk        Lightning */
    { 0x07, 0, { 0x1F, 0x6C } },  /* Ross       Journeyman  Iron Axe */
    { 0x19, 0, { 0x4B, 0x6C } },  /* L'Arachel  Troubadour  Heal */
};
