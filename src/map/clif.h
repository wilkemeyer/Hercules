/**
 * This file is part of Hercules.
 * http://herc.ws - http://github.com/HerculesWS/Hercules
 *
 * Copyright (C) 2012-2015  Hercules Dev Team
 * Copyright (C)  Athena Dev Teams
 *
 * Hercules is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef MAP_CLIF_H
#define MAP_CLIF_H

#include "map.h"
#include "packets_struct.h"
#include "../common/hercules.h"
#include "../common/mmo.h"


/**
 * Declarations
 **/
struct battleground_data;
struct channel_data;
struct chat_data;
struct eri;
struct flooritem_data;
struct guild;
struct homun_data;
struct item;
struct item_data;
struct map_session_data;
struct mercenary_data;
struct mob_data;
struct npc_data;
struct party_booking_ad_info;
struct party_data;
struct pet_data;
struct quest;
struct s_vending;
struct skill_cd;
struct skill_unit;
struct unit_data;
struct view_data;

/**
 * Defines
 **/
#define packet_len(cmd) packet_db[cmd].len
#define P2PTR(fd) RFIFO2PTR(fd)
#define clif_menuskill_clear(sd) ((sd)->menuskill_id = (sd)->menuskill_val = (sd)->menuskill_val2 = 0)
#define clif_disp_onlyself(sd,mes,len) clif->disp_message( &(sd)->bl, (mes), (len), SELF )
#define MAX_ROULETTE_LEVEL 7 /** client-defined value **/
#define MAX_ROULETTE_COLUMNS 9 /** client-defined value **/
#define RGB2BGR(c) ((c & 0x0000FF) << 16 | (c & 0x00FF00) | (c & 0xFF0000) >> 16)

#define COLOR_RED     0xff0000U
#define COLOR_GREEN   0x00ff00U
#define COLOR_WHITE   0xffffffU
#define COLOR_DEFAULT COLOR_GREEN

/**
 * Enumerations
 **/
enum {// packet DB
	MIN_PACKET_DB  = 0x0064,
	MAX_PACKET_DB  = 0x0F00,
	MAX_PACKET_POS = 20,
};

typedef enum send_target {
	ALL_CLIENT,
	ALL_SAMEMAP,
	AREA,               // area
	AREA_WOS,           // area, without self
	AREA_WOC,           // area, without chatrooms
	AREA_WOSC,          // area, without own chatrooms
	AREA_CHAT_WOC,      // hearable area, without chatrooms
	CHAT,               // current chatroom
	CHAT_WOS,           // current chatroom, without self
	PARTY,
	PARTY_WOS,
	PARTY_SAMEMAP,
	PARTY_SAMEMAP_WOS,
	PARTY_AREA,
	PARTY_AREA_WOS,
	GUILD,
	GUILD_WOS,
	GUILD_SAMEMAP,
	GUILD_SAMEMAP_WOS,
	GUILD_AREA,
	GUILD_AREA_WOS,
	GUILD_NOBG,
	DUEL,
	DUEL_WOS,
	SELF,

	BG,                 // BattleGround System
	BG_WOS,
	BG_SAMEMAP,
	BG_SAMEMAP_WOS,
	BG_AREA,
	BG_AREA_WOS,

	BG_QUEUE,
} send_target;

typedef enum broadcast_flags {
	BC_ALL         =    0,
	BC_MAP         =    1,
	BC_AREA        =    2,
	BC_SELF        =    3,
	BC_TARGET_MASK = 0x07,

	BC_PC          = 0x00,
	BC_NPC         = 0x08,
	BC_SOURCE_MASK = 0x08, // BC_PC|BC_NPC

	BC_YELLOW      = 0x00,
	BC_BLUE        = 0x10,
	BC_WOE         = 0x20,
	BC_COLOR_MASK  = 0x30, // BC_YELLOW|BC_BLUE|BC_WOE

	BC_DEFAULT     = BC_ALL|BC_PC|BC_YELLOW
} broadcast_flags;

typedef enum emotion_type {
	E_GASP = 0,     // /!
	E_WHAT,         // /?
	E_HO,
	E_LV,
	E_SWT,
	E_IC,
	E_AN,
	E_AG,
	E_CASH,         // /$
	E_DOTS,         // /...
	E_SCISSORS,     // /gawi --- 10
	E_ROCK,         // /bawi
	E_PAPER,        // /bo
	E_KOREA,
	E_LV2,
	E_THX,
	E_WAH,
	E_SRY,
	E_HEH,
	E_SWT2,
	E_HMM,          // --- 20
	E_NO1,
	E_NO,           // /??
	E_OMG,
	E_OH,
	E_X,
	E_HLP,
	E_GO,
	E_SOB,
	E_GG,
	E_KIS,          // --- 30
	E_KIS2,
	E_PIF,
	E_OK,
	E_MUTE,         // red /... used for muted characters
	E_INDONESIA,
	E_BZZ,          // /bzz, /stare
	E_RICE,
	E_AWSM,         // /awsm, /cool
	E_MEH,
	E_SHY,          // --- 40
	E_PAT,          // /pat, /goodboy
	E_MP,           // /mp, /sptime
	E_SLUR,
	E_COM,          // /com, /comeon
	E_YAWN,         // /yawn, /sleepy
	E_GRAT,         // /grat, /congrats
	E_HP,           // /hp, /hptime
	E_PHILIPPINES,
	E_MALAYSIA,
	E_SINGAPORE,    // --- 50
	E_BRAZIL,
	E_FLASH,        // /fsh
	E_SPIN,         // /spin
	E_SIGH,
	E_DUM,          // /dum
	E_LOUD,         // /crwd
	E_OTL,          // /otl, /desp
	E_DICE1,
	E_DICE2,
	E_DICE3,        // --- 60
	E_DICE4,
	E_DICE5,
	E_DICE6,
	E_INDIA,
	E_LUV,          // /love
	E_RUSSIA,
	E_VIRGIN,
	E_MOBILE,
	E_MAIL,
	E_CHINESE,      // --- 70
	E_ANTENNA1,
	E_ANTENNA2,
	E_ANTENNA3,
	E_HUM,
	E_ABS,
	E_OOPS,
	E_SPIT,
	E_ENE,
	E_PANIC,
	E_WHISP,        // --- 80
	E_YUT1,
	E_YUT2,
	E_YUT3,
	E_YUT4,
	E_YUT5,
	E_YUT6,
	E_YUT7,
	/* ... */
	E_MAX
} emotion_type;

typedef enum clr_type {
	CLR_OUTSIGHT = 0,
	CLR_DEAD,
	CLR_RESPAWN,
	CLR_TELEPORT,
	CLR_TRICKDEAD,
} clr_type;

enum map_property { // clif_map_property
	MAPPROPERTY_NOTHING       = 0,
	MAPPROPERTY_FREEPVPZONE   = 1,
	MAPPROPERTY_EVENTPVPZONE  = 2,
	MAPPROPERTY_AGITZONE      = 3,
	MAPPROPERTY_PKSERVERZONE  = 4, // message "You are in a PK area. Please beware of sudden attacks." in color 0x9B9BFF (light red)
	MAPPROPERTY_PVPSERVERZONE = 5,
	MAPPROPERTY_DENYSKILLZONE = 6,
};

enum map_type { // clif_map_type
	MAPTYPE_VILLAGE              = 0,
	MAPTYPE_VILLAGE_IN           = 1,
	MAPTYPE_FIELD                = 2,
	MAPTYPE_DUNGEON              = 3,
	MAPTYPE_ARENA                = 4,
	MAPTYPE_PENALTY_FREEPKZONE   = 5,
	MAPTYPE_NOPENALTY_FREEPKZONE = 6,
	MAPTYPE_EVENT_GUILDWAR       = 7,
	MAPTYPE_AGIT                 = 8,
	MAPTYPE_DUNGEON2             = 9,
	MAPTYPE_DUNGEON3             = 10,
	MAPTYPE_PKSERVER             = 11,
	MAPTYPE_PVPSERVER            = 12,
	MAPTYPE_DENYSKILL            = 13,
	MAPTYPE_TURBOTRACK           = 14,
	MAPTYPE_JAIL                 = 15,
	MAPTYPE_MONSTERTRACK         = 16,
	MAPTYPE_PORINGBATTLE         = 17,
	MAPTYPE_AGIT_SIEGEV15        = 18,
	MAPTYPE_BATTLEFIELD          = 19,
	MAPTYPE_PVP_TOURNAMENT       = 20,
	//Map types 21 - 24 not used.
	MAPTYPE_SIEGE_LOWLEVEL       = 25,
	//Map types 26 - 28 remains opens for future types.
	MAPTYPE_UNUSED               = 29,
};

typedef enum useskill_fail_cause { // clif_skill_fail
	USESKILL_FAIL_LEVEL = 0,
	USESKILL_FAIL_SP_INSUFFICIENT = 1,
	USESKILL_FAIL_HP_INSUFFICIENT = 2,
	USESKILL_FAIL_STUFF_INSUFFICIENT = 3,
	USESKILL_FAIL_SKILLINTERVAL = 4,
	USESKILL_FAIL_MONEY = 5,
	USESKILL_FAIL_THIS_WEAPON = 6,
	USESKILL_FAIL_REDJAMSTONE = 7,
	USESKILL_FAIL_BLUEJAMSTONE = 8,
	USESKILL_FAIL_WEIGHTOVER = 9,
	USESKILL_FAIL = 10,
	USESKILL_FAIL_TOTARGET = 11,
	USESKILL_FAIL_ANCILLA_NUMOVER = 12,
	USESKILL_FAIL_HOLYWATER = 13,
	USESKILL_FAIL_ANCILLA = 14,
	USESKILL_FAIL_DUPLICATE_RANGEIN = 15,
	USESKILL_FAIL_NEED_OTHER_SKILL = 16,
	USESKILL_FAIL_NEED_HELPER = 17,
	USESKILL_FAIL_INVALID_DIR = 18,
	USESKILL_FAIL_SUMMON = 19,
	USESKILL_FAIL_SUMMON_NONE = 20,
	USESKILL_FAIL_IMITATION_SKILL_NONE = 21,
	USESKILL_FAIL_DUPLICATE = 22,
	USESKILL_FAIL_CONDITION = 23,
	USESKILL_FAIL_PAINTBRUSH = 24,
	USESKILL_FAIL_DRAGON = 25,
	USESKILL_FAIL_POS = 26,
	USESKILL_FAIL_HELPER_SP_INSUFFICIENT = 27,
	USESKILL_FAIL_NEER_WALL = 28,
	USESKILL_FAIL_NEED_EXP_1PERCENT = 29,
	USESKILL_FAIL_CHORUS_SP_INSUFFICIENT = 30,
	USESKILL_FAIL_GC_WEAPONBLOCKING = 31,
	USESKILL_FAIL_GC_POISONINGWEAPON = 32,
	USESKILL_FAIL_MADOGEAR = 33,
	USESKILL_FAIL_NEED_EQUIPMENT_KUNAI = 34,
	USESKILL_FAIL_TOTARGET_PLAYER = 35,
	USESKILL_FAIL_SIZE = 36,
	USESKILL_FAIL_CANONBALL = 37,
	//XXX_USESKILL_FAIL_II_MADOGEAR_ACCELERATION = 38,
	//XXX_USESKILL_FAIL_II_MADOGEAR_HOVERING_BOOSTER = 39,
	//XXX_USESKILL_FAIL_MADOGEAR_HOVERING = 40,
	//XXX_USESKILL_FAIL_II_MADOGEAR_SELFDESTRUCTION_DEVICE = 41,
	//XXX_USESKILL_FAIL_II_MADOGEAR_SHAPESHIFTER = 42,
	USESKILL_FAIL_GUILLONTINE_POISON = 43,
	//XXX_USESKILL_FAIL_II_MADOGEAR_COOLING_DEVICE = 44,
	//XXX_USESKILL_FAIL_II_MADOGEAR_MAGNETICFIELD_GENERATOR = 45,
	//XXX_USESKILL_FAIL_II_MADOGEAR_BARRIER_GENERATOR = 46,
	//XXX_USESKILL_FAIL_II_MADOGEAR_OPTICALCAMOUFLAGE_GENERATOR = 47,
	//XXX_USESKILL_FAIL_II_MADOGEAR_REPAIRKIT = 48,
	//XXX_USESKILL_FAIL_II_MONKEY_SPANNER = 49,
	USESKILL_FAIL_MADOGEAR_RIDE = 50,
	USESKILL_FAIL_SPELLBOOK = 51,
	USESKILL_FAIL_SPELLBOOK_DIFFICULT_SLEEP = 52,
	USESKILL_FAIL_SPELLBOOK_PRESERVATION_POINT = 53,
	USESKILL_FAIL_SPELLBOOK_READING = 54,
	//XXX_USESKILL_FAIL_II_FACE_PAINTS = 55,
	//XXX_USESKILL_FAIL_II_MAKEUP_BRUSH = 56,
	USESKILL_FAIL_CART = 57,
	//XXX_USESKILL_FAIL_II_THORNS_SEED = 58,
	//XXX_USESKILL_FAIL_II_BLOOD_SUCKER_SEED = 59,
	USESKILL_FAIL_NO_MORE_SPELL = 60,
	//XXX_USESKILL_FAIL_II_BOMB_MUSHROOM_SPORE = 61,
	//XXX_USESKILL_FAIL_II_GASOLINE_BOOMB = 62,
	//XXX_USESKILL_FAIL_II_OIL_BOTTLE = 63,
	//XXX_USESKILL_FAIL_II_EXPLOSION_POWDER = 64,
	//XXX_USESKILL_FAIL_II_SMOKE_POWDER = 65,
	//XXX_USESKILL_FAIL_II_TEAR_GAS = 66,
	//XXX_USESKILL_FAIL_II_HYDROCHLORIC_ACID_BOTTLE = 67,
	//XXX_USESKILL_FAIL_II_HELLS_PLANT_BOTTLE = 68,
	//XXX_USESKILL_FAIL_II_MANDRAGORA_FLOWERPOT = 69,
	USESKILL_FAIL_MANUAL_NOTIFY = 70,
	USESKILL_FAIL_NEED_ITEM = 71,
	USESKILL_FAIL_NEED_EQUIPMENT = 72,
	USESKILL_FAIL_COMBOSKILL = 73,
	USESKILL_FAIL_SPIRITS = 74,
	USESKILL_FAIL_EXPLOSIONSPIRITS = 75,
	USESKILL_FAIL_HP_TOOMANY = 76,
	USESKILL_FAIL_NEED_ROYAL_GUARD_BANDING = 77,
	USESKILL_FAIL_NEED_EQUIPPED_WEAPON_CLASS = 78,
	USESKILL_FAIL_EL_SUMMON = 79,
	USESKILL_FAIL_RELATIONGRADE = 80,
	USESKILL_FAIL_STYLE_CHANGE_FIGHTER = 81,
	USESKILL_FAIL_STYLE_CHANGE_GRAPPLER = 82,
	USESKILL_FAIL_THERE_ARE_NPC_AROUND = 83,
	USESKILL_FAIL_NEED_MORE_BULLET = 84,
}useskill_fail_cause;

enum clif_messages {
	MSG_ITEM_CANT_OBTAIN_WEIGHT  = 0x034, ///< You cannot carry more items because you are overweight.
	MSG_ITEM_NEED_STANDING       = 0x297, ///< You cannot use this item while sitting.
	MSG_MERCENARY_EXPIRED        = 0x4f2, ///< The mercenary contract has expired.
	MSG_MERCENARY_DIED           = 0x4f3, ///< The mercenary has died.
	MSG_MERCENARY_RELEASED       = 0x4f4, ///< You have released the mercenary.
	MSG_MERCENARY_ESCAPED        = 0x4f5, ///< The mercenary has run away.
	MSG_SKILL_CANT_USE_AREA      = 0x536, ///< This skill cannot be used within this area
	MSG_ITEM_CANT_USE_AREA       = 0x537, ///< This item cannot be used within this area.
	MSG_EQUIP_NOT_PUBLIC         = 0x54d, ///< This character's equipment information is not open to the public.
	MSG_ITEM_NEED_MADO           = 0x59b, ///< Item can only be used when Mado Gear is mounted.
	MSG_ITEM_NEED_CART           = 0x5ef, ///< Usable only when cart is put on
	MSG_RUNE_STONE_MAX_AMOUNT    = 0x61b, ///< Cannot create Rune stone more than the maximum amount.
	MSG_SKILL_POINTS_LEFT_JOB1   = 0x61e, ///< You must consume all '%d' remaining points in your 1st Job tab.
	MSG_SKILL_POINTS_LEFT_JOB2   = 0x61f, ///< You must consume all '%d' remaining points in your 2nd Job tab. 1st Tab is already done.
	MSG_SKILL_ITEM_NOT_FOUND     = 0x623, // FIXME[Haru]: This seems to be 0x622 in the msgstringtable files I found.
	MSG_SKILL_SUCCESS            = 0x627, // FIXME[Haru]: This seems to be 0x626 in the msgstringtable files I found.
	MSG_SKILL_FAILURE            = 0x628, // FIXME[Haru]: This seems to be 0x627 in the msgstringtable files I found.
	MSG_SKILL_ITEM_NEED_IDENTIFY = 0x62d, ///< Unable to use unchecked items as materials.
	MSG_ITEM_CANT_EQUIP_LVL      = 0x6ed, // FIXME[Haru]: This seems to be 0x6ee in the msgstringtable files I found.
	MSG_ITEM_CANT_USE_LVL        = 0x6ee, // FIXME[Haru]: This seems to be 0x6ef in the msgstringtable files I found.
	MSG_COOKING_LIST_FAIL        = 0x625, // FIXME[Haru]: This might be a wrong message ID. Not sure what it should be.
	MSG_SECONDS_UNTIL_USE        = 0x746, ///< %d seconds left until you can use
	MSG_NPC_WORK_IN_PROGRESS     = 0x783, // FIXME[Haru]: This seems to be 0x784 in the msgstringtable files I found.
	MSG_REINS_CANT_USE_MOUNTED   = 0x78b, // FIXME[Haru]: This seems to be 0x785 in the msgstringtalbe files I found.
};

/**
 * Used to answer CZ_PC_BUY_CASH_POINT_ITEM (clif_parse_cashshop_buy)
 **/
enum cashshop_error {
	ERROR_TYPE_NONE             = 0, ///< The deal has successfully completed. (ERROR_TYPE_NONE)
	ERROR_TYPE_NPC              = 1, ///< The Purchase has failed because the NPC does not exist. (ERROR_TYPE_NPC)
	ERROR_TYPE_SYSTEM           = 2, ///< The Purchase has failed because the Kafra Shop System is not working correctly. (ERROR_TYPE_SYSTEM)
	ERROR_TYPE_INVENTORY_WEIGHT = 3, ///< You are over your Weight Limit. (ERROR_TYPE_INVENTORY_WEIGHT)
	ERROR_TYPE_EXCHANGE         = 4, ///< You cannot purchase items while you are in a trade. (ERROR_TYPE_EXCHANGE)
	ERROR_TYPE_ITEM_ID          = 5, ///< The Purchase has failed because the Item Information was incorrect. (ERROR_TYPE_ITEM_ID)
	ERROR_TYPE_MONEY            = 6, ///< You do not have enough Kafra Credit Points. (ERROR_TYPE_MONEY)
	// Unofficial type names
	ERROR_TYPE_QUANTITY         = 7, ///< You can purchase up to 10 items. (ERROR_TYPE_QUANTITY)
	ERROR_TYPE_NOT_ALL          = 8, ///< Some items could not be purchased. (ERROR_TYPE_NOT_ALL)
};

enum CASH_SHOP_TABS {
	CASHSHOP_TAB_NEW        = 0,
	CASHSHOP_TAB_POPULAR    = 1,
	CASHSHOP_TAB_LIMITED    = 2,
	CASHSHOP_TAB_RENTAL     = 3,
	CASHSHOP_TAB_PERPETUITY = 4,
	CASHSHOP_TAB_BUFF       = 5,
	CASHSHOP_TAB_RECOVERY   = 6,
	CASHSHOP_TAB_ETC        = 7,
	CASHSHOP_TAB_MAX,
};

enum CASH_SHOP_BUY_RESULT {
	CSBR_SUCCESS            = 0x0,
	CSBR_SHORTTAGE_CASH     = 0x2,
	CSBR_UNKONWN_ITEM       = 0x3,
	CSBR_INVENTORY_WEIGHT   = 0x4,
	CSBR_INVENTORY_ITEMCNT  = 0x5,
	CSBR_RUNE_OVERCOUNT     = 0x9,
	CSBR_EACHITEM_OVERCOUNT = 0xa,
	CSBR_UNKNOWN            = 0xb,
};

enum BATTLEGROUNDS_QUEUE_ACK {
	BGQA_SUCCESS = 1,
	BGQA_FAIL_QUEUING_FINISHED,
	BGQA_FAIL_BGNAME_INVALID,
	BGQA_FAIL_TYPE_INVALID,
	BGQA_FAIL_PPL_OVERAMOUNT,
	BGQA_FAIL_LEVEL_INCORRECT,
	BGQA_DUPLICATE_REQUEST,
	BGQA_PLEASE_RELOGIN,
	BGQA_NOT_PARTY_GUILD_LEADER,
	BGQA_FAIL_CLASS_INVALID,
	/* not official way to respond (gotta find packet?) */
	BGQA_FAIL_DESERTER,
	BGQA_FAIL_COOLDOWN,
	BGQA_FAIL_TEAM_COUNT,
};

enum BATTLEGROUNDS_QUEUE_NOTICE_DELETED {
	BGQND_CLOSEWINDOW = 1,
	BGQND_FAIL_BGNAME_WRONG = 3,
	BGQND_FAIL_NOT_QUEUING = 11,
};

enum e_BANKING_DEPOSIT_ACK {
	BDA_SUCCESS  = 0x0,
	BDA_ERROR    = 0x1,
	BDA_NO_MONEY = 0x2,
	BDA_OVERFLOW = 0x3,
};
enum e_BANKING_WITHDRAW_ACK {
	BWA_SUCCESS       = 0x0,
	BWA_NO_MONEY      = 0x1,
	BWA_UNKNOWN_ERROR = 0x2,
};

/* because the client devs were replaced by monkeys. */
enum e_EQUIP_ITEM_ACK {
#if PACKETVER >= 20120925
	EIA_SUCCESS = 0x0,
	EIA_FAIL_LV = 0x1,
	EIA_FAIL    = 0x2,
#else
	EIA_SUCCESS = 0x1,
	EIA_FAIL_LV = 0x2,
	EIA_FAIL    = 0x0,
#endif
};

/* and again. because the client devs were replaced by monkeys. */
enum e_UNEQUIP_ITEM_ACK {
#if PACKETVER >= 20120925
	UIA_SUCCESS = 0x0,
	UIA_FAIL    = 0x1,
#else
	UIA_SUCCESS = 0x1,
	UIA_FAIL    = 0x0,
#endif
};

enum e_trade_item_ok {
	TIO_SUCCESS    = 0x0,
	TIO_OVERWEIGHT = 0x1,
	TIO_CANCEL     = 0x2,
	/* feedback-friendly code that causes the client not to display a error message */
	TIO_INDROCKS   = 0x9,
};

enum RECV_ROULETTE_ITEM_REQ {
	RECV_ITEM_SUCCESS =  0x0,
	RECV_ITEM_FAILED =  0x1,
	RECV_ITEM_OVERCOUNT =  0x2,
	RECV_ITEM_OVERWEIGHT =  0x3,
};

enum RECV_ROULETTE_ITEM_ACK {
	RECV_ITEM_NORMAL =  0x0,
	RECV_ITEM_LOSING =  0x1,
};

enum GENERATE_ROULETTE_ACK {
	GENERATE_ROULETTE_SUCCESS =  0x0,
	GENERATE_ROULETTE_FAILED =  0x1,
	GENERATE_ROULETTE_NO_ENOUGH_POINT =  0x2,
	GENERATE_ROULETTE_LOSING =  0x3,
};

enum OPEN_ROULETTE_ACK{
	OPEN_ROULETTE_SUCCESS =  0x0,
	OPEN_ROULETTE_FAILED =  0x1,
};

enum CLOSE_ROULETTE_ACK {
	CLOSE_ROULETTE_SUCCESS =  0x0,
	CLOSE_ROULETTE_FAILED =  0x1,
};

/**
 * Reason for item deletion (clif->delitem)
 */
enum delitem_reason {
	DELITEM_NORMAL         = 0, /// Normal
	DELITEM_SKILLUSE       = 1, /// Item used for a skill
	DELITEM_FAILREFINE     = 2, /// Refine failed
	DELITEM_MATERIALCHANGE = 3, /// Material changed
	DELITEM_TOSTORAGE      = 4, /// Moved to storage
	DELITEM_TOCART         = 5, /// Moved to cart
	DELITEM_SOLD           = 6, /// Item sold
	DELITEM_ANALYSIS       = 7, /// Consumed by Four Spirit Analysis (SO_EL_ANALYSIS) skill
};

/*
* Merge items reasons
*/

enum mergeitem_reason {
	MERGEITEM_SUCCESS =  0x0,
	MERGEITEM_FAILD =  0x1,
	MERGEITEM_MAXCOUNTFAILD =  0x2,
};

/**
 * Structures
 **/
typedef void (*pFunc)(int, struct map_session_data *); //cant help but put it first
struct s_packet_db {
	short len;
	pFunc func;
	short pos[MAX_PACKET_POS];
};

struct hCSData {
	unsigned short id;
	unsigned int price;
};

struct cdelayed_damage {
	struct packet_damage p;
	struct block_list bl;
};

struct merge_item {
    int16 position;
    int16 nameid;
};

/**
 * Clif.c Interface
 **/
class CClif {
public:
	/* vars */
	static uint32 map_ip;
	static uint32 bind_ip;
	static uint16 map_port;
	static char map_ip_str[128];
	static int map_fd;
	/* for clif_clearunit_delayed */
	static struct eri *delay_clearunit_ers;
	/* Cash Shop [Ind/Hercules] */
	static struct _CashShop {
		struct hCSData **data[CASHSHOP_TAB_MAX];
		unsigned int item_count[CASHSHOP_TAB_MAX];
	} cs;
	/* roulette data */
	static struct _RouletteData {
		int *nameid[MAX_ROULETTE_LEVEL];//nameid
		int *qty[MAX_ROULETTE_LEVEL];//qty of nameid
		int items[MAX_ROULETTE_LEVEL];//number of items in the list for each
	} rd;
	/* */
	static unsigned int cryptKey[3];
	/* */
	static bool ally_only;
	/* */
	static struct eri *delayed_damage_ers;

	/* core */
	static int init (bool minimal);
	static void final (void);
	static bool setip (const char* ip);
	static bool setbindip (const char* ip);
	static void setport (uint16 port);
	static uint32 refresh_ip (void);
	static bool send (const void* buf, int len, struct block_list* bl, enum send_target type);
	static int send_sub (struct block_list *bl, va_list ap);
	static int send_actual (int fd, void *buf, int len);
	static int parse (int fd);
	static const struct s_packet_db *packet (int packet_id);

	typedef unsigned short(*parse_cmd_Proc)(int fd, struct map_session_data *sd);
	typedef unsigned short(*decrypt_cmd_Proc)(int cmd, struct map_session_data *sd);

	static parse_cmd_Proc parse_cmd;
	static decrypt_cmd_Proc decrypt_cmd;


	/* auth */
	static void authok (struct map_session_data *sd);
	static void authrefuse (int fd, uint8 error_code);
	static void authfail_fd (int fd, int type);
	static void charselectok (int id, uint8 ok);
	/* item-related */
	static void dropflooritem (struct flooritem_data* fitem);
	static void clearflooritem (struct flooritem_data *fitem, int fd);
	static void additem (struct map_session_data *sd, int n, int amount, int fail);
	static void dropitem (struct map_session_data *sd,int n,int amount);
	static void delitem (struct map_session_data *sd,int n,int amount, short reason);
	static void takeitem (struct block_list* src, struct block_list* dst);
	static void item_equip (short idx, struct EQUIPITEM_INFO *p, struct item *i, struct item_data *id, int eqp_pos);
	static void item_normal (short idx, struct NORMALITEM_INFO *p, struct item *i, struct item_data *id);
	static void arrowequip (struct map_session_data *sd,int val);
	static void arrow_fail (struct map_session_data *sd,int type);
	static void use_card (struct map_session_data *sd,int idx);
	static void cart_additem (struct map_session_data *sd,int n,int amount,int fail);
	static void cart_delitem (struct map_session_data *sd,int n,int amount);
	static void equipitemack (struct map_session_data *sd,int n,int pos,enum e_EQUIP_ITEM_ACK result);
	static void unequipitemack (struct map_session_data *sd,int n,int pos,enum e_UNEQUIP_ITEM_ACK result);
	static void useitemack (struct map_session_data *sd,int index,int amount,bool ok);
	static void addcards (unsigned char* buf, struct item* item);
	static void addcards2 (unsigned short *cards, struct item* item);
	static void item_sub (unsigned char *buf, int n, struct item *i, struct item_data *id, int equip);
	static void getareachar_item (struct map_session_data* sd,struct flooritem_data* fitem);
	static void cart_additem_ack (struct map_session_data *sd, int flag);
	static void cashshop_load (void);
	static void package_announce (struct map_session_data *sd, unsigned short nameid, unsigned short containerid);
	static void item_drop_announce (struct map_session_data *sd, unsigned short nameid, char *monsterName);
	/* unit-related */
	static void clearunit_single (int id, clr_type type, int fd);
	static void clearunit_area (struct block_list* bl, clr_type type);
	static void clearunit_delayed (struct block_list* bl, clr_type type, int64 tick);
	static void walkok (struct map_session_data *sd);
	static void move (struct unit_data *ud);
	static void move2 (struct block_list *bl, struct view_data *vd, struct unit_data *ud);
	static void blown (struct block_list *bl);
	static void slide (struct block_list *bl, int x, int y);
	static void fixpos (struct block_list *bl);
	static void changelook (struct block_list *bl,int type,int val);
	static void changetraplook (struct block_list *bl,int val);
	static void refreshlook (struct block_list *bl,int id,int type,int val,enum send_target target);
	static void sendlook (struct block_list *bl, int id, int type, int val, int val2, enum send_target target);
	static void class_change (struct block_list *bl,int class_,int type);
	static void skill_delunit (struct skill_unit *su);
	static void skillunit_update (struct block_list* bl);
	static int clearunit_delayed_sub (int tid, int64 tick, int id, intptr_t data);
	static void set_unit_idle (struct block_list* bl, struct map_session_data *tsd,enum send_target target);
	static void spawn_unit (struct block_list* bl, enum send_target target);
	static void spawn_unit2 (struct block_list* bl, enum send_target target);
	static void set_unit_idle2 (struct block_list* bl, struct map_session_data *tsd, enum send_target target);
	static void set_unit_walking (struct block_list* bl, struct map_session_data *tsd,struct unit_data* ud, enum send_target target);
	static int calc_walkdelay (struct block_list *bl,int delay, int type, int damage, int div_);
	static void getareachar_skillunit (struct block_list *bl, struct skill_unit *su, enum send_target target);
	static void getareachar_unit (struct map_session_data* sd,struct block_list *bl);
	static void clearchar_skillunit (struct skill_unit *su, int fd);
	static int getareachar (struct block_list* bl,va_list ap);
	static void graffiti_entry (struct block_list *bl, struct skill_unit *su, enum send_target target);
	/* main unit spawn */
	static bool spawn (struct block_list *bl);
	/* map-related */
	static void changemap (struct map_session_data *sd, short m, int x, int y);
	static void changemapcell (int fd, int16 m, int x, int y, int type, enum send_target target);
	static void map_property (struct map_session_data* sd, enum map_property property);
	static void pvpset (struct map_session_data *sd, int pvprank, int pvpnum,int type);
	static void map_property_mapall (int mapid, enum map_property property);
	static void bossmapinfo (int fd, struct mob_data *md, short flag);
	static void map_type (struct map_session_data* sd, enum map_type type);
	static void maptypeproperty2 (struct block_list *bl,enum send_target t);
	/* multi-map-server */
	static void changemapserver (struct map_session_data* sd, unsigned short map_index, int x, int y, uint32 ip, uint16 port);
	/* npc-shop-related */
	static void npcbuysell (struct map_session_data* sd, int id);
	static void buylist (struct map_session_data *sd, struct npc_data *nd);
	static void selllist (struct map_session_data *sd);
	static void cashshop_show (struct map_session_data *sd, struct npc_data *nd);
	static void npc_buy_result (struct map_session_data* sd, unsigned char result);
	static void npc_sell_result (struct map_session_data* sd, unsigned char result);
	static void cashshop_ack (struct map_session_data* sd, int error);
	/* npc-script-related */
	static void scriptmes (struct map_session_data *sd, int npcid, const char *mes);
	static void scriptnext (struct map_session_data *sd,int npcid);
	static void scriptclose (struct map_session_data *sd, int npcid);
	static void scriptmenu (struct map_session_data* sd, int npcid, const char* mes);
	static void scriptinput (struct map_session_data *sd, int npcid);
	static void scriptinputstr (struct map_session_data *sd, int npcid);
	static void cutin (struct map_session_data* sd, const char* image, int type);
	static void sendfakenpc (struct map_session_data *sd, int npcid);
	static void scriptclear (struct map_session_data *sd, int npcid);
	/* client-user-interface-related */
	static void viewpoint (struct map_session_data *sd, int npc_id, int type, int x, int y, int id, int color);
	static int damage (struct block_list* src, struct block_list* dst, int sdelay, int ddelay, int64 damage, short div, unsigned char type, int64 damage2);
	static void sitting (struct block_list* bl);
	static void standing (struct block_list* bl);
	static void arrow_create_list (struct map_session_data *sd);
	static void refresh_storagewindow (struct map_session_data *sd);
	static void refresh (struct map_session_data *sd);
	static void fame_blacksmith (struct map_session_data *sd, int points);
	static void fame_alchemist (struct map_session_data *sd, int points);
	static void fame_taekwon (struct map_session_data *sd, int points);
	static void ranklist (struct map_session_data *sd, enum fame_list_type type);
	static void update_rankingpoint (struct map_session_data *sd, enum fame_list_type type, int points);
	static void pRanklist (int fd, struct map_session_data *sd);
	static void hotkeys (struct map_session_data *sd);
	static int insight (struct block_list *bl,va_list ap);
	static int outsight (struct block_list *bl,va_list ap);
	static void skillcastcancel (struct block_list* bl);
	static void skill_fail (struct map_session_data *sd,uint16 skill_id,enum useskill_fail_cause cause,int btype);
	static void skill_cooldown (struct map_session_data *sd, uint16 skill_id, unsigned int duration);
	static void skill_memomessage (struct map_session_data* sd, int type);
	static void skill_mapinfomessage (struct map_session_data *sd, int type);
	static void skill_produce_mix_list (struct map_session_data *sd, int skill_id, int trigger);
	static void cooking_list (struct map_session_data *sd, int trigger, uint16 skill_id, int qty, int list_type);
	static void autospell (struct map_session_data *sd,uint16 skill_lv);
	static void combo_delay (struct block_list *bl,int wait);
	static void status_changeImpl (struct block_list *bl, int type, int flag, int tick, int val1, int val2, int val3);
	
	typedef void (*status_change_Proc)(struct block_list *bl, int type, int flag, int tick, int val1, int val2, int val3);
	static status_change_Proc status_change;

	static void insert_card (struct map_session_data *sd,int idx_equip,int idx_card,int flag);
	static void inventorylist (struct map_session_data *sd);
	static void equiplist (struct map_session_data *sd);
	static void cartlist (struct map_session_data *sd);
	static void favorite_item (struct map_session_data* sd, unsigned short index);
	static void clearcart (int fd);
	static void item_identify_list (struct map_session_data *sd);
	static void item_identified (struct map_session_data *sd,int idx,int flag);
	static void item_repair_list (struct map_session_data *sd, struct map_session_data *dstsd, int lv);
	static void item_repaireffect (struct map_session_data *sd, int idx, int flag);
	static void item_damaged (struct map_session_data* sd, unsigned short position);
	static void item_refine_list (struct map_session_data *sd);
	static void item_skill (struct map_session_data *sd,uint16 skill_id,uint16 skill_lv);
	static void mvp_item (struct map_session_data *sd,int nameid);
	static void mvp_exp (struct map_session_data *sd, unsigned int exp);
	static void mvp_noitem (struct map_session_data* sd);
	static void changed_dir (struct block_list *bl, enum send_target target);
	static void charnameack (int fd, struct block_list *bl);
	static void monster_hp_bar ( struct mob_data* md, struct map_session_data *sd );
	static int hpmeter (struct map_session_data *sd);
	static void hpmeter_single (int fd, int id, unsigned int hp, unsigned int maxhp);
	static int hpmeter_sub (struct block_list *bl, va_list ap);
	static void upgrademessage (int fd, int result, int item_id);
	static void get_weapon_view (struct map_session_data* sd, unsigned short *rhand, unsigned short *lhand);
	static void gospel_info (struct map_session_data *sd, int type);
	static void feel_req (int fd, struct map_session_data *sd, uint16 skill_lv);
	static void starskill (struct map_session_data* sd, const char* mapname, int monster_id, unsigned char star, unsigned char result);
	static void feel_info (struct map_session_data* sd, unsigned char feel_level, unsigned char type);
	static void hate_info (struct map_session_data *sd, unsigned char hate_level,int class_, unsigned char type);
	static void mission_info (struct map_session_data *sd, int mob_id, unsigned char progress);
	static void feel_hate_reset (struct map_session_data *sd);
	static void partytickack (struct map_session_data* sd, bool flag);
	static void equiptickack (struct map_session_data* sd, int flag);
	static void viewequip_ack (struct map_session_data* sd, struct map_session_data* tsd);
	static void equpcheckbox (struct map_session_data* sd);
	static void displayexp (struct map_session_data *sd, unsigned int exp, char type, bool is_quest);
	static void font (struct map_session_data *sd);
	static void progressbar (struct map_session_data * sd, unsigned int color, unsigned int second);
	static void progressbar_abort (struct map_session_data * sd);
	static void showdigit (struct map_session_data* sd, unsigned char type, int value);
	static int elementalconverter_list (struct map_session_data *sd);
	static int spellbook_list (struct map_session_data *sd);
	static int magicdecoy_list (struct map_session_data *sd, uint16 skill_lv, short x, short y);
	static int poison_list (struct map_session_data *sd, uint16 skill_lv);
	static int autoshadowspell_list (struct map_session_data *sd);
	static int skill_itemlistwindow ( struct map_session_data *sd, uint16 skill_id, uint16 skill_lv );
	static void sc_load (struct block_list *bl, int tid, enum send_target target, int type, int val1, int val2, int val3);
	static void sc_end (struct block_list *bl, int tid, enum send_target target, int type);
	static void initialstatus (struct map_session_data *sd);
	static void cooldown_list (int fd, struct skill_cd* cd);
	/* player-unit-specific-related */
	static void updatestatus (struct map_session_data *sd,int type);
	static void changestatus (struct map_session_data* sd,int type,int val);
	static void statusupack (struct map_session_data *sd,int type,int ok,int val);
	static void movetoattack (struct map_session_data *sd,struct block_list *bl);
	static void solved_charname (int fd, int charid, const char* name);
	static void charnameupdate (struct map_session_data *ssd);
	static int delayquit (int tid, int64 tick, int id, intptr_t data);
	static void getareachar_pc (struct map_session_data* sd,struct map_session_data* dstsd);
	static void disconnect_ack (struct map_session_data* sd, short result);
	static void PVPInfo (struct map_session_data* sd);
	static void blacksmith (struct map_session_data* sd);
	static void alchemist (struct map_session_data* sd);
	static void taekwon (struct map_session_data* sd);
	static void ranking_pk (struct map_session_data* sd);
	static void quitsave (int fd,struct map_session_data *sd);
	/* visual effects client-side */
	static void misceffect (struct block_list* bl,int type);
	static void changeoption (struct block_list* bl);
	static void changeoption2 (struct block_list* bl);
	static void emotion (struct block_list *bl,int type);
	static void talkiebox (struct block_list* bl, const char* talkie);
	static void wedding_effect (struct block_list *bl);
	static void divorced (struct map_session_data* sd, const char* name);
	static void callpartner (struct map_session_data *sd);
	static int skill_damage (struct block_list *src, struct block_list *dst, int64 tick, int sdelay, int ddelay, int64 damage, int div, uint16 skill_id, uint16 skill_lv, int type);
	static int skill_nodamage (struct block_list *src,struct block_list *dst,uint16 skill_id,int heal,int fail);
	static void skill_poseffect (struct block_list *src, uint16 skill_id, int val, int x, int y, int64 tick);
	static void skill_estimation (struct map_session_data *sd,struct block_list *dst);
	static void skill_warppoint (struct map_session_data* sd, uint16 skill_id, uint16 skill_lv, unsigned short map1, unsigned short map2, unsigned short map3, unsigned short map4);
	static void skillcasting (struct block_list* bl, int src_id, int dst_id, int dst_x, int dst_y, uint16 skill_id, int property, int casttime);
	static void produce_effect (struct map_session_data* sd,int flag,int nameid);
	static void devotion (struct block_list *src, struct map_session_data *tsd);
	static void spiritball (struct block_list *bl);
	static void spiritball_single (int fd, struct map_session_data *sd);
	static void bladestop (struct block_list *src, int dst_id, int active);
	static void mvp_effect (struct map_session_data *sd);
	static void heal (int fd,int type,int val);
	static void resurrection (struct block_list *bl,int type);
	static void refine (int fd, int fail, int index, int val);
	static void weather (int16 m);
	static void specialeffect (struct block_list* bl, int type, enum send_target target);
	static void specialeffect_single (struct block_list* bl, int type, int fd);
	static void specialeffect_value (struct block_list* bl, int effect_id, int num, send_target target);
	static void millenniumshield (struct block_list *bl, short shields );
	static void spiritcharm (struct map_session_data *sd);
	static void charm_single (int fd, struct map_session_data *sd);
	static void snap ( struct block_list *bl, short x, short y );
	static void weather_check (struct map_session_data *sd);
	/* sound effects client-side */
	static void playBGM (struct map_session_data* sd, const char* name);
	static void soundeffect (struct map_session_data* sd, struct block_list* bl, const char* name, int type);
	static void soundeffectall (struct block_list* bl, const char* name, int type, enum send_target coverage);
	/* chat/message-related */
	static void GlobalMessage (struct block_list* bl, const char* message);
	static void createchat (struct map_session_data* sd, int flag);
	static void dispchat (struct chat_data* cd, int fd);
	static void joinchatfail (struct map_session_data *sd,int flag);
	static void joinchatok (struct map_session_data *sd,struct chat_data* cd);
	static void addchat (struct chat_data* cd,struct map_session_data *sd);
	static void changechatowner (struct chat_data* cd, struct map_session_data* sd);
	static void clearchat (struct chat_data *cd,int fd);
	static void leavechat (struct chat_data* cd, struct map_session_data* sd, bool flag);
	static void changechatstatus (struct chat_data* cd);
	static void wis_message (int fd, const char* nick, const char* mes, size_t mes_len);
	static void wis_end (int fd, int flag);
	static void disp_message (struct block_list* src, const char* mes, size_t len, enum send_target target);
	static void broadcast (struct block_list* bl, const char* mes, size_t len, int type, enum send_target target);
	static void broadcast2 (struct block_list* bl, const char* mes, size_t len, unsigned int fontColor, short fontType, short fontSize, short fontAlign, short fontY, enum send_target target);
	static void messagecolor_self (int fd, uint32 color, const char *msg);
	static void messagecolor (struct block_list* bl, uint32 color, const char* msg);
	static void disp_overhead (struct block_list *bl, const char* mes);
	static void msgtable (struct map_session_data* sd, unsigned short msg_id);
	static void msgtable_num (struct map_session_data *sd, unsigned short msg_id, int value);
	static void msgtable_skill (struct map_session_data *sd, uint16 skill_id, int msg_id);
	static void message (const int fd, const char* mes);
	static void messageln (const int fd, const char* mes);
	/* message+s(printf) */
	static void messages (const int fd, const char *mes, ...) __attribute__((format(printf, 2, 3)));
	static bool process_message (struct map_session_data *sd, int format, char **name_, size_t *namelen_, char **message_, size_t *messagelen_);
	static void wisexin (struct map_session_data *sd,int type,int flag);
	static void wisall (struct map_session_data *sd,int type,int flag);
	static void PMIgnoreList (struct map_session_data* sd);
	static void ShowScript (struct block_list* bl, const char* message);
	/* trade handling */
	static void traderequest (struct map_session_data* sd, const char* name);
	static void tradestart (struct map_session_data* sd, uint8 type);
	static void tradeadditem (struct map_session_data* sd, struct map_session_data* tsd, int index, int amount);
	static void tradeitemok (struct map_session_data* sd, int index, int fail);
	static void tradedeal_lock (struct map_session_data* sd, int fail);
	static void tradecancelled (struct map_session_data* sd);
	static void tradecompleted (struct map_session_data* sd, int fail);
	static void tradeundo (struct map_session_data* sd);
	/* vending handling */
	static void openvendingreq (struct map_session_data* sd, int num);
	static void showvendingboard (struct block_list* bl, const char* message, int fd);
	static void closevendingboard (struct block_list* bl, int fd);
	static void vendinglist (struct map_session_data* sd, unsigned int id, struct s_vending* vending_list);
	static void buyvending (struct map_session_data* sd, int index, int amount, int fail);
	static void openvending (struct map_session_data* sd, int id, struct s_vending* vending_list);
	static void vendingreport (struct map_session_data* sd, int index, int amount);
	/* storage handling */
	static void storagelist (struct map_session_data* sd, struct item* items, int items_length);
	static void updatestorageamount (struct map_session_data* sd, int amount, int max_amount);
	static void storageitemadded (struct map_session_data* sd, struct item* i, int index, int amount);
	static void storageitemremoved (struct map_session_data* sd, int index, int amount);
	static void storageclose (struct map_session_data* sd);
	/* skill-list handling */
	static void skillinfoblock (struct map_session_data *sd);
	static void skillup (struct map_session_data *sd, uint16 skill_id, int skill_lv, int flag);
	static void skillinfo (struct map_session_data *sd,int skill_id, int inf);
	static void addskill (struct map_session_data *sd, int id);
	static void deleteskill (struct map_session_data *sd, int id);
	/* party-specific */
	static void party_created (struct map_session_data *sd,int result);
	static void party_member_info (struct party_data *p, struct map_session_data *sd);
	static void party_info (struct party_data* p, struct map_session_data *sd);
	static void party_invite (struct map_session_data *sd,struct map_session_data *tsd);
	static void party_inviteack (struct map_session_data* sd, const char* nick, int result);
	static void party_option (struct party_data *p,struct map_session_data *sd,int flag);
	static void party_withdraw (struct party_data* p, struct map_session_data* sd, int account_id, const char* name, int flag);
	static void party_message (struct party_data* p, int account_id, const char* mes, int len);
	static void party_xy (struct map_session_data *sd);
	static void party_xy_single (int fd, struct map_session_data *sd);
	static void party_hp (struct map_session_data *sd);
	static void party_xy_remove (struct map_session_data *sd);
	static void party_show_picker (struct map_session_data * sd, struct item * item_data);
	static void partyinvitationstate (struct map_session_data* sd);
	static void PartyLeaderChanged (struct map_session_data *sd, int prev_leader_aid, int new_leader_aid);
	/* guild-specific */
	static void guild_created (struct map_session_data *sd,int flag);
	static void guild_belonginfo (struct map_session_data *sd, struct guild *g);
	static void guild_masterormember (struct map_session_data *sd);
	static void guild_basicinfo (struct map_session_data *sd);
	static void guild_allianceinfo (struct map_session_data *sd);
	static void guild_memberlist (struct map_session_data *sd);
	static void guild_skillinfo (struct map_session_data* sd);
	static void guild_send_onlineinfo (struct map_session_data *sd); //[LuzZza]
	static void guild_memberlogin_notice (struct guild *g,int idx,int flag);
	static void guild_invite (struct map_session_data *sd,struct guild *g);
	static void guild_inviteack (struct map_session_data *sd,int flag);
	static void guild_leave (struct map_session_data *sd,const char *name,const char *mes);
	static void guild_expulsion (struct map_session_data* sd, const char* name, const char* mes, int account_id);
	static void guild_positionchanged (struct guild *g,int idx);
	static void guild_memberpositionchanged (struct guild *g,int idx);
	static void guild_emblem (struct map_session_data *sd,struct guild *g);
	static void guild_emblem_area (struct block_list* bl);
	static void guild_notice (struct map_session_data* sd, struct guild* g);
	static void guild_message (struct guild *g,int account_id,const char *mes,int len);
	static void guild_reqalliance (struct map_session_data *sd,int account_id,const char *name);
	static void guild_allianceack (struct map_session_data *sd,int flag);
	static void guild_delalliance (struct map_session_data *sd,int guild_id,int flag);
	static void guild_oppositionack (struct map_session_data *sd,int flag);
	static void guild_broken (struct map_session_data *sd,int flag);
	static void guild_xy (struct map_session_data *sd);
	static void guild_xy_single (int fd, struct map_session_data *sd);
	static void guild_xy_remove (struct map_session_data *sd);
	static void guild_positionnamelist (struct map_session_data *sd);
	static void guild_positioninfolist (struct map_session_data *sd);
	static void guild_expulsionlist (struct map_session_data* sd);
	static bool validate_emblem (const uint8* emblem, unsigned long emblem_len);
	/* battleground-specific */
	static void bg_hp (struct map_session_data *sd);
	static void bg_xy (struct map_session_data *sd);
	static void bg_xy_remove (struct map_session_data *sd);
	static void bg_message (struct battleground_data *bgd, int src_id, const char *name, const char *mes, size_t len);
	static void bg_updatescore (int16 m);
	static void bg_updatescore_single (struct map_session_data *sd);
	static void sendbgemblem_area (struct map_session_data *sd);
	static void sendbgemblem_single (int fd, struct map_session_data *sd);
	/* instance-related */
	static int instance (int instance_id, int type, int flag);
	static void instance_join (int fd, int instance_id);
	static void instance_leave (int fd);
	/* pet-related */
	static void catch_process (struct map_session_data *sd);
	static void pet_roulette (struct map_session_data *sd,int data);
	static void sendegg (struct map_session_data *sd);
	static void send_petstatus (struct map_session_data *sd);
	static void send_petdata (struct map_session_data* sd, struct pet_data* pd, int type, int param);
	static void pet_emotion (struct pet_data *pd,int param);
	static void pet_food (struct map_session_data *sd,int foodid,int fail);
	/* friend-related */
	static int friendslist_toggle_sub (struct map_session_data *sd,va_list ap);
	static void friendslist_send (struct map_session_data *sd);
	static void friendslist_reqack (struct map_session_data *sd, struct map_session_data *f_sd, int type);
	static void friendslist_toggle (struct map_session_data *sd,int account_id, int char_id, int online);
	static void friendlist_req (struct map_session_data* sd, int account_id, int char_id, const char* name);
	/* gm-related */
	static void GM_kickack (struct map_session_data *sd, int result);
	static void GM_kick (struct map_session_data *sd,struct map_session_data *tsd);
	static void manner_message (struct map_session_data* sd, uint32 type);
	static void GM_silence (struct map_session_data* sd, struct map_session_data* tsd, uint8 type);
	static void account_name (struct map_session_data* sd, int account_id, const char* accname);
	static void check (int fd, struct map_session_data* pl_sd);
	/* hom-related */
	static void hominfo (struct map_session_data *sd, struct homun_data *hd, int flag);
	static void homskillinfoblock (struct map_session_data *sd);
	static void homskillup (struct map_session_data *sd, uint16 skill_id);
	static void hom_food (struct map_session_data *sd,int foodid,int fail);
	static void send_homdata (struct map_session_data *sd, int state, int param);
	/* questlog-related */
	static void quest_send_list (struct map_session_data *sd);
	static void quest_send_mission (struct map_session_data *sd);
	static void quest_add (struct map_session_data *sd, struct quest *qd);
	static void quest_delete (struct map_session_data *sd, int quest_id);
	static void quest_update_status (struct map_session_data *sd, int quest_id, bool active);
	static void quest_update_objective (struct map_session_data *sd, struct quest *qd);
	static void quest_show_event (struct map_session_data *sd, struct block_list *bl, short state, short color);
	/* mail-related */
	static void mail_window (int fd, int flag);
	static void mail_read (struct map_session_data *sd, int mail_id);
	static void mail_delete (int fd, int mail_id, short fail);
	static void mail_return (int fd, int mail_id, short fail);
	static void mail_send (int fd, bool fail);
	static void mail_new (int fd, int mail_id, const char *sender, const char *title);
	static void mail_refreshinbox (struct map_session_data *sd);
	static void mail_getattachment (int fd, uint8 flag);
	static void mail_setattachment (int fd, int index, uint8 flag);
	/* auction-related */
	static void auction_openwindow (struct map_session_data *sd);
	static void auction_results (struct map_session_data *sd, short count, short pages, uint8 *buf);
	static void auction_message (int fd, unsigned char flag);
	static void auction_close (int fd, unsigned char flag);
	static void auction_setitem (int fd, int index, bool fail);
	/* mercenary-related */
	static void mercenary_info (struct map_session_data *sd);
	static void mercenary_skillblock (struct map_session_data *sd);
	static void mercenary_message (struct map_session_data* sd, int message);
	static void mercenary_updatestatus (struct map_session_data *sd, int type);
	/* item rental */
	static void rental_time (int fd, int nameid, int seconds);
	static void rental_expired (int fd, int index, int nameid);
	/* party booking related */
	static void PartyBookingRegisterAck (struct map_session_data *sd, int flag);
	static void PartyBookingDeleteAck (struct map_session_data* sd, int flag);
	static void PartyBookingSearchAck (int fd, struct party_booking_ad_info** results, int count, bool more_result);
	static void PartyBookingUpdateNotify (struct map_session_data* sd, struct party_booking_ad_info* pb_ad);
	static void PartyBookingDeleteNotify (struct map_session_data* sd, int index);
	static void PartyBookingInsertNotify (struct map_session_data* sd, struct party_booking_ad_info* pb_ad);
	static void PartyRecruitRegisterAck (struct map_session_data *sd, int flag);
	static void PartyRecruitDeleteAck (struct map_session_data* sd, int flag);
	static void PartyRecruitSearchAck (int fd, struct party_booking_ad_info** results, int count, bool more_result);
	static void PartyRecruitUpdateNotify (struct map_session_data* sd, struct party_booking_ad_info* pb_ad);
	static void PartyRecruitDeleteNotify (struct map_session_data* sd, int index);
	static void PartyRecruitInsertNotify (struct map_session_data* sd, struct party_booking_ad_info* pb_ad);
	/* Group Search System Update */
	static void PartyBookingVolunteerInfo (int index, struct map_session_data *sd);
	static void PartyBookingRefuseVolunteer (unsigned int aid, struct map_session_data *sd);
	static void PartyBookingCancelVolunteer (int index, struct map_session_data *sd);
	static void PartyBookingAddFilteringList (int index, struct map_session_data *sd);
	static void PartyBookingSubFilteringList (int gid, struct map_session_data *sd);
	/* buying store-related */
	static void buyingstore_open (struct map_session_data* sd);
	static void buyingstore_open_failed (struct map_session_data* sd, unsigned short result, unsigned int weight);
	static void buyingstore_myitemlist (struct map_session_data* sd);
	static void buyingstore_entry (struct map_session_data* sd);
	static void buyingstore_entry_single (struct map_session_data* sd, struct map_session_data* pl_sd);
	static void buyingstore_disappear_entry (struct map_session_data* sd);
	static void buyingstore_disappear_entry_single (struct map_session_data* sd, struct map_session_data* pl_sd);
	static void buyingstore_itemlist (struct map_session_data* sd, struct map_session_data* pl_sd);
	static void buyingstore_trade_failed_buyer (struct map_session_data* sd, short result);
	static void buyingstore_update_item (struct map_session_data* sd, unsigned short nameid, unsigned short amount);
	static void buyingstore_delete_item (struct map_session_data* sd, short index, unsigned short amount, int price);
	static void buyingstore_trade_failed_seller (struct map_session_data* sd, short result, unsigned short nameid);
	/* search store-related */
	static void search_store_info_ack (struct map_session_data* sd);
	static void search_store_info_failed (struct map_session_data* sd, unsigned char reason);
	static void open_search_store_info (struct map_session_data* sd);
	static void search_store_info_click_ack (struct map_session_data* sd, short x, short y);
	/* elemental-related */
	static void elemental_info (struct map_session_data *sd);
	static void elemental_updatestatus (struct map_session_data *sd, int type);
	/* bgqueue */
	static void bgqueue_ack (struct map_session_data *sd, enum BATTLEGROUNDS_QUEUE_ACK response, unsigned char arena_id);
	static void bgqueue_notice_delete (struct map_session_data *sd, enum BATTLEGROUNDS_QUEUE_NOTICE_DELETED response, char *name);
	static void bgqueue_update_info (struct map_session_data *sd, unsigned char arena_id, int position);
	static void bgqueue_joined (struct map_session_data *sd, int pos);
	static void bgqueue_pcleft (struct map_session_data *sd);
	static void bgqueue_battlebegins (struct map_session_data *sd, unsigned char arena_id, enum send_target target);
	/* misc-handling */
	static void adopt_reply (struct map_session_data *sd, int type);
	static void adopt_request (struct map_session_data *sd, struct map_session_data *src, int p_id);
	static void readbook (int fd, int book_id, int page);
	static void notify_time (struct map_session_data* sd, int64 time);
	static void user_count (struct map_session_data* sd, int count);
	static void noask_sub (struct map_session_data *src, struct map_session_data *target, int type);
	static void bc_ready (void);
	/* Hercules Channel System */
	static void channel_msg (struct channel_data *chan, struct map_session_data *sd, char *msg);
	static void channel_msg2 (struct channel_data *chan, char *msg);
	static int undisguise_timer (int tid, int64 tick, int id, intptr_t data);
	/* Bank System [Yommy/Hercules] */
	static void bank_deposit (struct map_session_data *sd, enum e_BANKING_DEPOSIT_ACK reason);
	static void bank_withdraw (struct map_session_data *sd,enum e_BANKING_WITHDRAW_ACK reason);
	/* */
	static void show_modifiers (struct map_session_data *sd);
	/* */
	static void notify_bounditem (struct map_session_data *sd, unsigned short index);
	/* */
	static int delay_damage (int64 tick, struct block_list *src, struct block_list *dst, int sdelay, int ddelay, int64 in_damage, short div, unsigned char type);
	static int delay_damage_sub (int tid, int64 tick, int id, intptr_t data);
	/* NPC Market */
	static void npc_market_open (struct map_session_data *sd, struct npc_data *nd);
	static void npc_market_purchase_ack (struct map_session_data *sd, struct packet_npc_market_purchase *req, unsigned char response);
	/* */
	static bool parse_roulette_db (void);
	static void roulette_generate_ack (struct map_session_data *sd, unsigned char result, short stage, short prizeIdx, short bonusItemID);
	/* Merge Items */
	static void openmergeitem (int fd, struct map_session_data *sd);
	static void cancelmergeitem (int fd, struct map_session_data *sd);
	static int comparemergeitem (const void *a, const void *b);
	static void ackmergeitems (int fd, struct map_session_data *sd);

	/*------------------------
	 *- Parse Incoming Packet
	 *------------------------*/
	static void pWantToConnection (int fd, struct map_session_data *sd);
	static void pLoadEndAck (int fd,struct map_session_data *sd);
	static void pTickSend (int fd, struct map_session_data *sd);
	static void pHotkey (int fd, struct map_session_data *sd);
	static void pProgressbar (int fd, struct map_session_data * sd);
	static void pWalkToXY (int fd, struct map_session_data *sd);
	static void pQuitGame (int fd, struct map_session_data *sd);
	static void pGetCharNameRequest (int fd, struct map_session_data *sd);
	static void pGlobalMessage (int fd, struct map_session_data* sd);
	static void pMapMove (int fd, struct map_session_data *sd);
	static void pChangeDir (int fd, struct map_session_data *sd);
	static void pEmotion (int fd, struct map_session_data *sd);
	static void pHowManyConnections (int fd, struct map_session_data *sd);
	static void pActionRequest (int fd, struct map_session_data *sd);
	static void pActionRequest_sub (struct map_session_data *sd, int action_type, int target_id, int64 tick);
	static void pRestart (int fd, struct map_session_data *sd);
	static void pWisMessage (int fd, struct map_session_data* sd);
	static void pBroadcast (int fd, struct map_session_data* sd);
	static void pTakeItem (int fd, struct map_session_data *sd);
	static void pDropItem (int fd, struct map_session_data *sd);
	static void pUseItem (int fd, struct map_session_data *sd);
	static void pEquipItem (int fd,struct map_session_data *sd);
	static void pUnequipItem (int fd,struct map_session_data *sd);
	static void pNpcClicked (int fd,struct map_session_data *sd);
	static void pNpcBuySellSelected (int fd,struct map_session_data *sd);
	static void pNpcBuyListSend (int fd, struct map_session_data* sd);
	static void pNpcSellListSend (int fd,struct map_session_data *sd);
	static void pCreateChatRoom (int fd, struct map_session_data* sd);
	static void pChatAddMember (int fd, struct map_session_data* sd);
	static void pChatRoomStatusChange (int fd, struct map_session_data* sd);
	static void pChangeChatOwner (int fd, struct map_session_data* sd);
	static void pKickFromChat (int fd,struct map_session_data *sd);
	static void pChatLeave (int fd, struct map_session_data* sd);
	static void pTradeRequest (int fd,struct map_session_data *sd);
	static void pTradeAck (int fd,struct map_session_data *sd);
	static void pTradeAddItem (int fd,struct map_session_data *sd);
	static void pTradeOk (int fd,struct map_session_data *sd);
	static void pTradeCancel (int fd,struct map_session_data *sd);
	static void pTradeCommit (int fd,struct map_session_data *sd);
	static void pStopAttack (int fd,struct map_session_data *sd);
	static void pPutItemToCart (int fd,struct map_session_data *sd);
	static void pGetItemFromCart (int fd,struct map_session_data *sd);
	static void pRemoveOption (int fd,struct map_session_data *sd);
	static void pChangeCart (int fd,struct map_session_data *sd);
	static void pStatusUp (int fd,struct map_session_data *sd);
	static void pSkillUp (int fd,struct map_session_data *sd);
	static void pUseSkillToId (int fd, struct map_session_data *sd);
	static void pUseSkillToId_homun (struct homun_data *hd, struct map_session_data *sd, int64 tick, uint16 skill_id, uint16 skill_lv, int target_id);
	static void pUseSkillToId_mercenary (struct mercenary_data *md, struct map_session_data *sd, int64 tick, uint16 skill_id, uint16 skill_lv, int target_id);
	static void pUseSkillToPos (int fd, struct map_session_data *sd);
	static void pUseSkillToPosSub (int fd, struct map_session_data *sd, uint16 skill_lv, uint16 skill_id, short x, short y, int skillmoreinfo);
	static void pUseSkillToPos_homun (struct homun_data *hd, struct map_session_data *sd, int64 tick, uint16 skill_id, uint16 skill_lv, short x, short y, int skillmoreinfo);
	static void pUseSkillToPos_mercenary (struct mercenary_data *md, struct map_session_data *sd, int64 tick, uint16 skill_id, uint16 skill_lv, short x, short y, int skillmoreinfo);
	static void pUseSkillToPosMoreInfo (int fd, struct map_session_data *sd);
	static void pUseSkillMap (int fd, struct map_session_data* sd);
	static void pRequestMemo (int fd,struct map_session_data *sd);
	static void pProduceMix (int fd,struct map_session_data *sd);
	static void pCooking (int fd,struct map_session_data *sd);
	static void pRepairItem (int fd, struct map_session_data *sd);
	static void pWeaponRefine (int fd, struct map_session_data *sd);
	static void pNpcSelectMenu (int fd,struct map_session_data *sd);
	static void pNpcNextClicked (int fd,struct map_session_data *sd);
	static void pNpcAmountInput (int fd,struct map_session_data *sd);
	static void pNpcStringInput (int fd, struct map_session_data* sd);
	static void pNpcCloseClicked (int fd,struct map_session_data *sd);
	static void pItemIdentify (int fd,struct map_session_data *sd);
	static void pSelectArrow (int fd,struct map_session_data *sd);
	static void pAutoSpell (int fd,struct map_session_data *sd);
	static void pUseCard (int fd,struct map_session_data *sd);
	static void pInsertCard (int fd,struct map_session_data *sd);
	static void pSolveCharName (int fd, struct map_session_data *sd);
	static void pResetChar (int fd, struct map_session_data *sd);
	static void pLocalBroadcast (int fd, struct map_session_data* sd);
	static void pMoveToKafra (int fd, struct map_session_data *sd);
	static void pMoveFromKafra (int fd,struct map_session_data *sd);
	static void pMoveToKafraFromCart (int fd, struct map_session_data *sd);
	static void pMoveFromKafraToCart (int fd, struct map_session_data *sd);
	static void pCloseKafra (int fd, struct map_session_data *sd);
	static void pStoragePassword (int fd, struct map_session_data *sd);
	static void pCreateParty (int fd, struct map_session_data *sd);
	static void pCreateParty2 (int fd, struct map_session_data *sd);
	static void pPartyInvite (int fd, struct map_session_data *sd);
	static void pPartyInvite2 (int fd, struct map_session_data *sd);
	static void pReplyPartyInvite (int fd,struct map_session_data *sd);
	static void pReplyPartyInvite2 (int fd,struct map_session_data *sd);
	static void pLeaveParty (int fd, struct map_session_data *sd);
	static void pRemovePartyMember (int fd, struct map_session_data *sd);
	static void pPartyChangeOption (int fd, struct map_session_data *sd);
	static void pPartyMessage (int fd, struct map_session_data* sd);
	static void pPartyChangeLeader (int fd, struct map_session_data* sd);
	static void pPartyBookingRegisterReq (int fd, struct map_session_data* sd);
	static void pPartyBookingSearchReq (int fd, struct map_session_data* sd);
	static void pPartyBookingDeleteReq (int fd, struct map_session_data* sd);
	static void pPartyBookingUpdateReq (int fd, struct map_session_data* sd);
	static void pPartyRecruitRegisterReq (int fd, struct map_session_data* sd);
	static void pPartyRecruitSearchReq (int fd, struct map_session_data* sd);
	static void pPartyRecruitDeleteReq (int fd, struct map_session_data* sd);
	static void pPartyRecruitUpdateReq (int fd, struct map_session_data* sd);
	static void pCloseVending (int fd, struct map_session_data* sd);
	static void pVendingListReq (int fd, struct map_session_data* sd);
	static void pPurchaseReq (int fd, struct map_session_data* sd);
	static void pPurchaseReq2 (int fd, struct map_session_data* sd);
	static void pOpenVending (int fd, struct map_session_data* sd);
	static void pCreateGuild (int fd,struct map_session_data *sd);
	static void pGuildCheckMaster (int fd, struct map_session_data *sd);
	static void pGuildRequestInfo (int fd, struct map_session_data *sd);
	static void pGuildChangePositionInfo (int fd, struct map_session_data *sd);
	static void pGuildChangeMemberPosition (int fd, struct map_session_data *sd);
	static void pGuildRequestEmblem (int fd,struct map_session_data *sd);
	static void pGuildChangeEmblem (int fd,struct map_session_data *sd);
	static void pGuildChangeNotice (int fd, struct map_session_data* sd);
	static void pGuildInvite (int fd,struct map_session_data *sd);
	static void pGuildReplyInvite (int fd,struct map_session_data *sd);
	static void pGuildLeave (int fd,struct map_session_data *sd);
	static void pGuildExpulsion (int fd,struct map_session_data *sd);
	static void pGuildMessage (int fd, struct map_session_data* sd);
	static void pGuildRequestAlliance (int fd, struct map_session_data *sd);
	static void pGuildReplyAlliance (int fd, struct map_session_data *sd);
	static void pGuildDelAlliance (int fd, struct map_session_data *sd);
	static void pGuildOpposition (int fd, struct map_session_data *sd);
	static void pGuildBreak (int fd, struct map_session_data *sd);
	static void pPetMenu (int fd, struct map_session_data *sd);
	static void pCatchPet (int fd, struct map_session_data *sd);
	static void pSelectEgg (int fd, struct map_session_data *sd);
	static void pSendEmotion (int fd, struct map_session_data *sd);
	static void pChangePetName (int fd, struct map_session_data *sd);
	static void pGMKick (int fd, struct map_session_data *sd);
	static void pGMKickAll (int fd, struct map_session_data* sd);
	static void pGMShift (int fd, struct map_session_data *sd);
	static void pGMRemove2 (int fd, struct map_session_data* sd);
	static void pGMRecall (int fd, struct map_session_data *sd);
	static void pGMRecall2 (int fd, struct map_session_data* sd);
	static void pGM_Monster_Item (int fd, struct map_session_data *sd);
	static void pGMHide (int fd, struct map_session_data *sd);
	static void pGMReqNoChat (int fd,struct map_session_data *sd);
	static void pGMRc (int fd, struct map_session_data* sd);
	static void pGMReqAccountName (int fd, struct map_session_data *sd);
	static void pGMChangeMapType (int fd, struct map_session_data *sd);
	static void pGMFullStrip (int fd, struct map_session_data *sd);
	static void pPMIgnore (int fd, struct map_session_data* sd);
	static void pPMIgnoreAll (int fd, struct map_session_data *sd);
	static void pPMIgnoreList (int fd,struct map_session_data *sd);
	static void pNoviceDoriDori (int fd, struct map_session_data *sd);
	static void pNoviceExplosionSpirits (int fd, struct map_session_data *sd);
	static void pFriendsListAdd (int fd, struct map_session_data *sd);
	static void pFriendsListReply (int fd, struct map_session_data *sd);
	static void pFriendsListRemove (int fd, struct map_session_data *sd);
	static void pPVPInfo (int fd,struct map_session_data *sd);
	static void pBlacksmith (int fd,struct map_session_data *sd);
	static void pAlchemist (int fd,struct map_session_data *sd);
	static void pTaekwon (int fd,struct map_session_data *sd);
	static void pRankingPk (int fd,struct map_session_data *sd);
	static void pFeelSaveOk (int fd,struct map_session_data *sd);
	static void pChangeHomunculusName (int fd, struct map_session_data *sd);
	static void pHomMoveToMaster (int fd, struct map_session_data *sd);
	static void pHomMoveTo (int fd, struct map_session_data *sd);
	static void pHomAttack (int fd,struct map_session_data *sd);
	static void pHomMenu (int fd, struct map_session_data *sd);
	static void pAutoRevive (int fd, struct map_session_data *sd);
	static void pCheck (int fd, struct map_session_data *sd);
	static void pMail_refreshinbox (int fd, struct map_session_data *sd);
	static void pMail_read (int fd, struct map_session_data *sd);
	static void pMail_getattach (int fd, struct map_session_data *sd);
	static void pMail_delete (int fd, struct map_session_data *sd);
	static void pMail_return (int fd, struct map_session_data *sd);
	static void pMail_setattach (int fd, struct map_session_data *sd);
	static void pMail_winopen (int fd, struct map_session_data *sd);
	static void pMail_send (int fd, struct map_session_data *sd);
	static void pAuction_cancelreg (int fd, struct map_session_data *sd);
	static void pAuction_setitem (int fd, struct map_session_data *sd);
	static void pAuction_register (int fd, struct map_session_data *sd);
	static void pAuction_cancel (int fd, struct map_session_data *sd);
	static void pAuction_close (int fd, struct map_session_data *sd);
	static void pAuction_bid (int fd, struct map_session_data *sd);
	static void pAuction_search (int fd, struct map_session_data* sd);
	static void pAuction_buysell (int fd, struct map_session_data* sd);
	static void pcashshop_buy (int fd, struct map_session_data *sd);
	static void pAdopt_request (int fd, struct map_session_data *sd);
	static void pAdopt_reply (int fd, struct map_session_data *sd);
	static void pViewPlayerEquip (int fd, struct map_session_data* sd);
	static void pEquipTick (int fd, struct map_session_data* sd);
	static void pquestStateAck (int fd, struct map_session_data * sd);
	static void pmercenary_action (int fd, struct map_session_data* sd);
	static void pBattleChat (int fd, struct map_session_data* sd);
	static void pLessEffect (int fd, struct map_session_data* sd);
	static void pItemListWindowSelected (int fd, struct map_session_data* sd);
	static void pReqOpenBuyingStore (int fd, struct map_session_data* sd);
	static void pReqCloseBuyingStore (int fd, struct map_session_data* sd);
	static void pReqClickBuyingStore (int fd, struct map_session_data* sd);
	static void pReqTradeBuyingStore (int fd, struct map_session_data* sd);
	static void pSearchStoreInfo (int fd, struct map_session_data* sd);
	static void pSearchStoreInfoNextPage (int fd, struct map_session_data* sd);
	static void pCloseSearchStoreInfo (int fd, struct map_session_data* sd);
	static void pSearchStoreInfoListItemClick (int fd, struct map_session_data* sd);
	static void pDebug (int fd,struct map_session_data *sd);
	static void pSkillSelectMenu (int fd, struct map_session_data *sd);
	static void pMoveItem (int fd, struct map_session_data *sd);
	static void pDull (int fd, struct map_session_data *sd);
	/* BGQueue */
	static void pBGQueueRegister (int fd, struct map_session_data *sd);
	static void pBGQueueCheckState (int fd, struct map_session_data *sd);
	static void pBGQueueRevokeReq (int fd, struct map_session_data *sd);
	static void pBGQueueBattleBeginAck (int fd, struct map_session_data *sd);
	/* RagExe Cash Shop [Ind/Hercules] */
	static void pCashShopOpen (int fd, struct map_session_data *sd);
	static void pCashShopClose (int fd, struct map_session_data *sd);
	static void pCashShopReqTab (int fd, struct map_session_data *sd);
	static void pCashShopSchedule (int fd, struct map_session_data *sd);
	static void pCashShopBuy (int fd, struct map_session_data *sd);
	static void pPartyTick (int fd, struct map_session_data *sd);
	static void pGuildInvite2 (int fd, struct map_session_data *sd);
	/* Group Search System Update */
	static void pPartyBookingAddFilter (int fd, struct map_session_data *sd);
	static void pPartyBookingSubFilter (int fd, struct map_session_data *sd);
	static void pPartyBookingReqVolunteer (int fd, struct map_session_data *sd);
	static void pPartyBookingRefuseVolunteer (int fd, struct map_session_data *sd);
	static void pPartyBookingCancelVolunteer (int fd, struct map_session_data *sd);
	/* Bank System [Yommy/Hercules] */
	static void pBankDeposit (int fd, struct map_session_data *sd);
	static void pBankWithdraw (int fd, struct map_session_data *sd);
	static void pBankCheck (int fd, struct map_session_data *sd);
	static void pBankOpen (int fd, struct map_session_data *sd);
	static void pBankClose (int fd, struct map_session_data *sd);
	/* Roulette System [Yommy/Hercules] */
	static void pRouletteOpen (int fd, struct map_session_data *sd);
	static void pRouletteInfo (int fd, struct map_session_data *sd);
	static void pRouletteClose (int fd, struct map_session_data *sd);
	static void pRouletteGenerate (int fd, struct map_session_data *sd);
	static void pRouletteRecvItem (int fd, struct map_session_data *sd);
	/* */
	static void pNPCShopClosed (int fd, struct map_session_data *sd);
	/* NPC Market (by Ind after an extensive debugging of the packet, only possible thanks to Yommy <3) */
	static void pNPCMarketClosed (int fd, struct map_session_data *sd);
	static void pNPCMarketPurchase (int fd, struct map_session_data *sd);
	/* */
	static void add_random_options (unsigned char* buf, struct item* item);
	static void pHotkeyRowShift (int fd, struct map_session_data *sd);
	static void dressroom_open (struct map_session_data *sd, int view);
};

extern CClif *clif;

#ifdef HERCULES_CORE
void clif_defaults(void);
#endif // HERCULES_CORE


#endif /* MAP_CLIF_H */
