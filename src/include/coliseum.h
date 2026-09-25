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
/* Layout: 0x000-0x0FF run state (saved), 0x100-0x1FF reserved (Legacy, Phase 13),
 *         0x200-0x27F UI scratch (not saved; only valid while a COLISEUM menu is open). */
#define COL_UI_SCRATCH      (COL_RAM_BASE + 0x200)

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
#define COL_MAX_LEVEL           30   /* spec 21 (Class_Level_Cap_Table: 30 for every class) */
#define COL_STAT_CEILING        127  /* spec 21: no stat caps; 127 is the s8 limit (GAME_DESIGN.md) */
#define COL_STAT_CHOICES        3    /* spec 23: level-up choices offered */
#define COL_RECRUIT_CHOICES     3    /* spec 9: recruits offered */
#define COL_SKILL_SLOTS         3    /* spec 27: + 1 personal skill = 4 */
#define COL_RARITY_COUNT        5    /* spec 28: Common, Uncommon, Rare, Epic, Legendary */

enum ColEncounter {
    COL_ENC_NORMAL = 0,
    COL_ENC_ELITE  = 1,
    COL_ENC_BOSS   = 2,
};

/* One shop stock entry (shop.c). */
#define COL_SHOP_SIZE    8
struct ColShopEntry {
    u8  category;                   /* enum ColShopCategory */
    u8  value;                      /* pool index / skill ID / item ID */
    u16 basePrice;                  /* before the per-purchase increase */
};
enum ColShopCategory {
    COL_SHOP_RECRUIT = 1, COL_SHOP_SKILL, COL_SHOP_WEAPON, COL_SHOP_RELIC,
    COL_SHOP_HEAL, COL_SHOP_PROMOTION, COL_SHOP_CONSUMABLE,
};

/* The run state (spec 76). Saved in the game save and the suspend save (save chunks in
 * EngineHacks/Necessary/ExpandedModularSave/ExModularSave.event). Fixed size: new fields go
 * into `reserved` and bump COL_RUN_VERSION. */
#define COL_RUN_MAGIC    0x524C4F43   /* "COLR" */
#define COL_RUN_VERSION  4
#define COL_RUN_SIZE     0x100

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
    /* 2D */ u8  choiceLevel[COL_MAX_ROSTER]; /* level up to which the unit's stat choices were
                                                  made (spec 23); below its level = choice owed */
    /* 32 */ u8  shopCount;         /* entries in this battle's shop stock (spec 38) */
    /* 33 */ u8  shopSold;          /* bit = stock entry already bought */
    /* 34 */ struct ColShopEntry shop[COL_SHOP_SIZE];   /* rolled once per battle: no reroll */
    /* 54 */ u8  reserved[COL_RUN_SIZE - 0x54];
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
struct Unit;
struct Unit *Col_RosterUnit(int slot);         /* the map unit of roster slot `slot`, or NULL */
struct Unit *Col_LoadPoolUnit(int pool);       /* creates pool character `pool` at level 5 (blue) */
void Col_EnsureRosterUnits(void);              /* every roster member exists (hidden until placed) */

/* units/stats.c: stats without caps (spec 21) and level-up stat choices (spec 23) */
enum ColStat {
    COL_STAT_HP, COL_STAT_STR, COL_STAT_MAG, COL_STAT_SKL,
    COL_STAT_SPD, COL_STAT_LCK, COL_STAT_DEF, COL_STAT_RES,
    COL_STAT_COUNT
};
int  Col_GetStat(struct Unit *unit, int stat);          /* the unit's own stat (no bonuses) */
int  Col_AddStat(struct Unit *unit, int stat, int n);   /* returns the amount actually added */
void Col_RollStatChoices(u8 out[COL_STAT_CHOICES]);     /* random stats, duplicates allowed */
const char *Col_StatName(int stat);
int  Col_FindPendingChoice(void);                       /* roster slot owed a choice, or -1 */
void Col_TakeStatChoice(int slot, int stat);            /* applies +1 and records the level */

/* roster/roster.c */
void Col_StartRun(void);                        /* spec 7: 3 random characters, all deployed */
int  Col_FreeRosterSlot(void);                  /* -1 if the roster is full */
int  Col_RollRecruits(u8 out[COL_RECRUIT_CHOICES]);   /* returns the number offered */
int  Col_RecruitLevel(void);
void Col_ScaleUnitToLevel(struct Unit *unit, int level);
void Col_RemoveFromRoster(int slot);            /* replaced: leaves the run (not dead) */
struct Unit *Col_Recruit(int pool, int replaceSlot);  /* replaceSlot -1: into an empty slot */
int  Col_DeployTarget(void);                    /* units to deploy: min(3, living roster) */
u8   Col_DeploymentMask(void);                  /* bit = roster slot */
int  Col_SetDeployment(u8 mask);                /* 1 if valid and applied */
void Col_RunLost(void);                         /* ends the run; invalidates its saves */

/* skills/skills.c (catalog in skills/Skills.event) */
struct ColSkillInfo {
    u8 skill;
    u8 rarity;                      /* 1 Common .. 5 Legendary */
    u8 weapons;                     /* weapon types (bit = FE8 type), 0 = any */
    u8 flags;                       /* COL_SKF_* */
    u8 stat;                        /* enum ColStat + 1, 0 = none */
    u8 statMin;
};
#define COL_SKF_PROMOTED    1
#define COL_SKF_MOUNTED     2
#define COL_SKF_ENEMY_ONLY  4
int  Col_PersonalSkill(struct Unit *unit);
int  Col_SlotSkill(struct Unit *unit, int slot);        /* 0 if empty */
int  Col_UnitHasSkill(struct Unit *unit, int skill);     /* personal or a slot */
int  Col_FreeSkillSlot(struct Unit *unit);               /* -1 if all 3 are used */
const struct ColSkillInfo *Col_SkillInfo(int skill);     /* NULL if not in the catalog */
int  Col_CanLearnSkill(struct Unit *unit, int skill);
int  Col_LearnSkill(struct Unit *unit, int skill, int slot);   /* slot < 0: first free */
void Col_ClearRunSkills(void);
int  Col_RollSkillOffers(struct Unit *unit, u8 *out, int n);
int  Col_OfferSkill(struct Unit *unit, int skill);      /* skill_ui.c: learn / replace menu */
const char *Col_SkillName(int skill);

/* weapons/weapons.c */
int  Col_FusionResult(int itemA, int itemB);            /* 0 if the pair doesn't fuse */
int  Col_Fuse(struct Unit *unit, int s1, int s2);       /* 1 if fused (result in s1) */
int  Col_FindFusion(struct Unit *unit, int from, int *s1, int *s2);
int  Col_CanReceiveItem(struct Unit *to, int item);
int  Col_TransferItem(struct Unit *from, int slot, struct Unit *to);

/* shop/shop.c */
int  Col_BattleGold(int encounter);
void Col_SyncPartyGold(void);                           /* run gold -> FE8's party gold */
int  Col_ShopPrice(int basePrice);                      /* +10% per purchase this run, capped */
void Col_ShopGenerate(void);
int  Col_ShopEntryPrice(int index);
int  Col_ShopCanAfford(int index);
int  Col_ShopPay(int index);                            /* 1 if paid (after delivering) */
void Col_HealTeam(int percent);

/* save chunk functions (Expanded Modular Save): (sram address, size) */
void Col_SaveRunChunk(void *sram, unsigned size);
void Col_LoadRunChunk(void *sram, unsigned size);

#endif
