/* COLISEUM core definitions. See ARCHITECTURE.md.
 * Built with FE-CLib (Tools/FE-Clib/include) and linked into the ROM with lyn. */
#ifndef COLISEUM_H
#define COLISEUM_H

#include "global.h"

/* ---- RAM ----------------------------------------------------------------
 * COLISEUM's EWRAM block: 0x0203F600-0x0203FDFF (2 KiB). Above everything vanilla FE8 keeps
 * in EWRAM (last symbol gLoadUnitBuffer 0x0203EFB8) and the Skill System's blocks (debuffs
 * 0x0203F100-0x0203F540, single bytes at 0x0203F080-84 and 0x0203FFDF+). A test checks the
 * game never writes here (TEST_STATUS.md). */
#define COL_RAM_BASE        0x0203F600
#define COL_RAM_SIZE        0x800

/* ---- Rules fixed by the spec (docs/COLISEUM_SPEC.md; changing them needs developer approval) ---- */
#define COL_MAX_ROSTER          5    /* spec 8  */
#define COL_MAX_DEPLOY          3    /* spec 8  */
#define COL_POOL_SIZE           15   /* spec 10: playable characters */
#define COL_RECOVER_PER_FLOOR   3    /* spec 54 */
#define COL_WINS_PER_REWARD     3    /* spec 51: 3-win reward */
#define COL_NORMAL_FIGHTS_FLOOR 9    /* spec 5: battles 1-9, then the boss is fight 10 */
#define COL_TURN_LIMIT          20   /* spec 14 */
#define COL_GOLD_MAX            999999
#define COL_START_LEVEL         5    /* spec 21 */
#define COL_MAX_LEVEL           30   /* spec 21 */

enum ColEncounter {
    COL_ENC_NORMAL = 0,
    COL_ENC_ELITE  = 1,
    COL_ENC_BOSS   = 2,
};

/* The run state (spec 76). Saved in the game save and the suspend save (save chunks in
 * EngineHacks/Necessary/ExpandedModularSave/ExModularSave.event). Fixed size: new fields go
 * into `reserved` and bump COL_RUN_VERSION. */
#define COL_RUN_MAGIC    0x524C4F43   /* "COLR" */
#define COL_RUN_VERSION  2
#define COL_RUN_SIZE     64

struct ColRunState {
    /* 00 */ u32 magic;
    /* 04 */ u16 version;
    /* 06 */ u8  active;            /* 1 while a run is in progress */
    /* 07 */ u8  floor;             /* Coliseum floor, from 1 */
    /* 08 */ u8  normalWins;        /* normal victories on this floor, 0-9 */
    /* 09 */ u8  winsToReward;      /* normal victories since the last 3-win reward, 0-2 */
    /* 0A */ u8  eliteDue;          /* 1: the next encounter is an Elite */
    /* 0B */ u8  rewardDue;         /* 1: a 3-win reward is waiting to be chosen */
    /* 0C */ u8  recoverCharges;    /* 0-3 */
    /* 0D */ u8  rosterCount;
    /* 0E */ u8  roster[COL_MAX_ROSTER];   /* pool indices (0-14) of living roster members; 0xFF empty */
    /* 13 */ u8  deployed[COL_MAX_DEPLOY]; /* roster slots deployed next battle; 0xFF empty */
    /* 16 */ u16 deadMask;          /* bit = pool index: died this run (spec 12) */
    /* 18 */ u16 recruitedMask;     /* bit = pool index: recruited this run (spec 9: never twice) */
    /* 1A */ u16 shopPurchases;     /* purchases this run (price scaling, spec 39) */
    /* 1C */ u32 gold;
    /* 20 */ u32 seed;              /* run seed */
    /* 24 */ u32 battlesWon;        /* total victories this run (history) */
    /* 28 */ u8  hp[COL_MAX_ROSTER];/* HP after the last battle per roster slot; 0 = full (spec 20) */
    /* 2D */ u8  reserved[COL_RUN_SIZE - 0x2D];
};

/* The playable pool (pool.c). */
struct ColPoolEntry {
    u8 charId;
    u8 classId;                     /* 0 = the character's default class */
    u8 items[4];
};
extern const struct ColPoolEntry gColPool[COL_POOL_SIZE];

#define gColRun (*(struct ColRunState *)COL_RAM_BASE)

/* run_state.c */
void Col_RunClear(void);
void Col_RunNew(u32 seed);
int  Col_RunIsValid(void);
int  Col_NextEncounter(void);
void Col_OnVictory(int encounter);
void Col_TakeReward(void);
int  Col_AddGold(int amount);          /* returns the gold actually added (0 if it would overflow the max) */
int  Col_SpendGold(u32 amount);        /* 1 if paid */
int  Col_CanRecover(u32 cost);
int  Col_UseRecover(u32 cost);         /* 1 if used: pays, spends a charge (healing is done by the caller) */
void Col_RosterRemoveDead(int slot);   /* spec 12: gone for this run; frees the slot and its deployment */
void Col_AutoDeploy(void);             /* fill empty deployment slots from living reserves */

/* battle.c (ASMC from the battle chapter's events) */
void Col_PrepareBattle(void);
void Col_OnBattleWon(void);

/* save chunk functions (Expanded Modular Save): (sram address, size) */
void Col_SaveRunChunk(void *sram, unsigned size);
void Col_LoadRunChunk(void *sram, unsigned size);

#endif
