/* The playable character pool (spec 10: 15 characters, all eligible from the start, plus 5
 * original characters approved by the developer on 2026-09-26: 20).
 *
 * 15 FE8 characters (their portraits and animations exist), one per class the spec lists plus a
 * second Myrmidon (spec 10: characters sharing a class differ by skill, growths, equipment).
 * Their level-5 bases, growths (incl. magic) and personal skills live in the tables:
 * Tables/NightmareModules/CharactersClasses/CharacterTable.csv, MagCharEditor.csv and
 * Skills/PersonalSkillEditor.csv (values and how they were derived: BALANCE_NOTES.md).
 * Names and identities may still change with the story (Phase 14). */
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
    { 0x16, 0, { 0x01, 0x6C } },  /* Marisa     Myrmidon    Iron Sword */
    { 0x19, 0, { 0x4B, 0x6C } },  /* L'Arachel  Troubadour  Heal */
    /* Original characters (2026-09-26): FE-Repo portraits in the slots of FE8 characters who are
     * not in the pool; name, description, bases, growths, magic, personal skill and death quote
     * replaced (CharacterTable.csv, MagCharEditor.csv, PersonalSkillEditor.csv, coliseum.txt). */
    { 0x07, 0x42, { 0x1F, 0x6C } },  /* Morrow   Pirate      Iron Axe   (Ross's slot)    */
    { 0x18, 0x25, { 0x38, 0x6C } },  /* Silas    Mage        Fire       (Ewan's slot)    */
    { 0x11, 0x19, { 0x2D, 0x6C } },  /* Hale     Archer      Iron Bow   (Kyle's slot)    */
    { 0x15, 0x4D, { 0x6C } },        /* Selene   Dancer      (Dance)    (Tethys's slot)  */
    { 0x05, 0x45, { 0x4B, 0x6C } },  /* Idris    Priest      Heal       (Moulder's slot) */
};
