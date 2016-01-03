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
#ifndef MAP_NPC_H
#define MAP_NPC_H

#include "map.h" // struct block_list
#include "status.h" // struct status_change
#include "unit.h" // struct unit_data
#include "../common/hercules.h"
#include "../common/db.h"

struct view_data;

enum npc_parse_options {
	NPO_NONE  = 0x0,
	NPO_ONINIT  = 0x1,
	NPO_TRADER  = 0x2,
};

enum npc_shop_types {
	NST_ZENY,/* default */
	NST_CASH,/* official npc cash shop */
	NST_MARKET,/* official npc market type */
	NST_CUSTOM,
	/* */
	NST_MAX,
};

struct npc_timerevent_list {
	int timer,pos;
};
struct npc_label_list {
	char name[NAME_LENGTH];
	int pos;
};
struct npc_item_list {
	unsigned short nameid;
	unsigned int value;
	unsigned int qty;
};
struct npc_shop_data {
	unsigned char type;/* what am i */
	struct npc_item_list *item;/* list */
	unsigned short items;/* total */
};
struct npc_parse;
struct npc_data {
	struct block_list bl;
	struct unit_data *ud;
	struct view_data *vd;
	unsigned int option;
	struct npc_data *master_nd;
	short class_;
	short speed;
	char name[NAME_LENGTH+1];// display name
	char exname[NAME_LENGTH+1];// unique npc name
	int chat_id;
	int touching_id;
	int64 next_walktime;
	uint8 dir;
	uint8 area_size;

	unsigned size : 2;

	struct status_data status;
	unsigned short level;
	unsigned short stat_point;

	struct npc_parse *chatdb;
	const char *path; ///< Source path reference
	enum npc_subtype subtype;
	int src_id;
	union {
		struct {
			struct script_code *script;
			short xs,ys; // OnTouch area radius
			int guild_id;
			int timer,timerid,timeramount,rid;
			int64 timertick;
			struct npc_timerevent_list *timer_event;
			int label_list_num;
			struct npc_label_list *label_list;
			/* */
			struct npc_shop_data *shop;
			bool trader;
		} scr;
		struct { /* TODO duck this as soon as the new shop formatting is deemed stable */
			struct npc_item_list* shop_item;
			unsigned short count;
		} shop;
		struct {
			short xs,ys; // OnTouch area radius
			short x,y; // destination coords
			unsigned short mapindex; // destination map
		} warp;
		struct {
			struct mob_data *md;
			time_t kill_time;
			char killer_name[NAME_LENGTH];
		} tomb;
	} u;
};


#define START_NPC_NUM 110000000

enum actor_classes {
	FAKE_NPC = -1,
	WARP_CLASS = 45,
	HIDDEN_WARP_CLASS = 139,
	MOB_TOMB = 565,
	WARP_DEBUG_CLASS = 722,
	FLAG_CLASS = 722,
	INVISIBLE_CLASS = 32767,
};

// Old NPC range
#define MAX_NPC_CLASS 1000
// New NPC range
#define MAX_NPC_CLASS2_START 10001
#define MAX_NPC_CLASS2_END 10178

//Script NPC events.
enum npce_event {
	NPCE_LOGIN,
	NPCE_LOGOUT,
	NPCE_LOADMAP,
	NPCE_BASELVUP,
	NPCE_JOBLVUP,
	NPCE_DIE,
	NPCE_KILLPC,
	NPCE_KILLNPC,
	NPCE_MAX
};

// linked list of npc source files
struct npc_src_list {
	struct npc_src_list* next;
	char name[4]; // dynamic array, the structure is allocated with extra bytes (string length)
};

struct event_data {
	struct npc_data *nd;
	int pos;
};

struct npc_path_data {
	char* path;
	unsigned short references;
};

/* npc.c interface */
class CNpc {
public:
	/* */
	static struct npc_data *motd;
	static DBMap *ev_db; // const char* event_name -> struct event_data*
	static DBMap *ev_label_db; // const char* label_name (without leading "::") -> struct linkdb_node**   (key: struct npc_data*; data: struct event_data*)
	static DBMap *name_db; // const char* npc_name -> struct npc_data*
	static DBMap *path_db;
	static struct eri *timer_event_ers; //For the npc timer data. [Skotlex]
	static struct npc_data *fake_nd;
	static struct npc_src_list *src_files;
	static struct unit_data base_ud;
	/* npc trader global data, for ease of transition between the script, cleared on every usage */
	static bool trader_ok;
	static int trader_funds[2];

	/* */
	static int init (bool minimal);
	static int final (void);
	/* */
	static int get_new_npc_id (void);
	static struct view_data* get_viewdata (int class_);
	static int isnear_sub (struct block_list *bl, va_list args);
	static bool isnear (struct block_list *bl);
	static int ontouch_event (struct map_session_data *sd, struct npc_data *nd);
	static int ontouch2_event (struct map_session_data *sd, struct npc_data *nd);
	static int onuntouch_event (struct map_session_data *sd, struct npc_data *nd);
	static int enable_sub (struct block_list *bl, va_list ap);
	static int enable (const char *name, int flag);
	static struct npc_data* name2id (const char *name);
	static int event_dequeue (struct map_session_data *sd);
	static DBData event_export_create (DBKey key, va_list args);
	static int event_export (struct npc_data *nd, int i);
	static int event_sub (struct map_session_data *sd, struct event_data *ev, const char *eventname);
	static void event_doall_sub (void *key, void *data, va_list ap);
	static int event_do (const char *name);
	static int event_doall_id (const char *name, int rid);
	static int event_doall (const char *name);
	static int event_do_clock (int tid, int64 tick, int id, intptr_t data);
	static void event_do_oninit ( bool reload );
	static int timerevent_export (struct npc_data *nd, int i);
	static int timerevent (int tid, int64 tick, int id, intptr_t data);
	static int timerevent_start (struct npc_data *nd, int rid);
	static int timerevent_stop (struct npc_data *nd);
	static void timerevent_quit (struct map_session_data *sd);
	static int64 gettimerevent_tick (struct npc_data *nd);
	static int settimerevent_tick (struct npc_data *nd, int newtimer);
	static int event (struct map_session_data *sd, const char *eventname, int ontouch);
	static int touch_areanpc_sub (struct block_list *bl, va_list ap);
	static int touchnext_areanpc (struct map_session_data *sd, bool leavemap);
	static int touch_areanpc (struct map_session_data *sd, int16 m, int16 x, int16 y);
	static int untouch_areanpc (struct map_session_data *sd, int16 m, int16 x, int16 y);
	static int touch_areanpc2 (struct mob_data *md);
	static int check_areanpc (int flag, int16 m, int16 x, int16 y, int16 range);
	static struct npc_data* checknear (struct map_session_data *sd, struct block_list *bl);
	static int globalmessage (const char *name, const char *mes);
	static void run_tomb (struct map_session_data *sd, struct npc_data *nd);
	static int click (struct map_session_data *sd, struct npc_data *nd);
	static int scriptcont (struct map_session_data *sd, int id, bool closing);
	static int buysellsel (struct map_session_data *sd, int id, int type);
	static int cashshop_buylist (struct map_session_data *sd, int points, int count, unsigned short *item_list);
	static int buylist_sub (struct map_session_data *sd, int n, unsigned short *item_list, struct npc_data *nd);
	static int cashshop_buy (struct map_session_data *sd, int nameid, int amount, int points);
	static int buylist (struct map_session_data *sd, int n, unsigned short *item_list);
	static int selllist_sub (struct map_session_data *sd, int n, unsigned short *item_list, struct npc_data *nd);
	static int selllist (struct map_session_data *sd, int n, unsigned short *item_list);
	static int remove_map (struct npc_data *nd);
	static int unload_ev (DBKey key, DBData *data, va_list ap);
	static int unload_ev_label (DBKey key, DBData *data, va_list ap);
	static int unload_dup_sub (struct npc_data *nd, va_list args);
	static void unload_duplicates (struct npc_data *nd);
	static int unload (struct npc_data *nd, bool single);
	static void clearsrcfile (void);
	static void addsrcfile (const char *name);
	static void delsrcfile (const char *name);
	static const char *retainpathreference (const char *filepath);
	static void releasepathreference (const char *filepath);
	static void parsename (struct npc_data *nd, const char *name, const char *start, const char *buffer, const char *filepath);
	static int parseview (const char *w4, const char *start, const char *buffer, const char *filepath);
	static bool viewisid (const char *viewid);
	static struct npc_data *create_npc (enum npc_subtype subtype, int m, int x, int y, uint8 dir, int16 class_);
	static struct npc_data* add_warp (char *name, short from_mapid, short from_x, short from_y, short xs, short ys, unsigned short to_mapindex, short to_x, short to_y);
	static const char *parse_warp (const char *w1, const char *w2, const char *w3, const char *w4, const char *start, const char *buffer, const char *filepath, int *retval);
	static const char *parse_shop (const char *w1, const char *w2, const char *w3, const char *w4, const char *start, const char *buffer, const char *filepath, int *retval);
	static const char *parse_unknown_object (const char *w1, const char *w2, const char *w3, const char *w4, const char *start, const char *buffer, const char *filepath, int *retval);
	static void convertlabel_db (struct npc_label_list *label_list, const char *filepath);
	static const char* skip_script (const char *start, const char *buffer, const char *filepath, int *retval);
	static const char *parse_script (const char *w1, const char *w2, const char *w3, const char *w4, const char *start, const char *buffer, const char *filepath, int options, int *retval);
	static void add_to_location (struct npc_data *nd);
	static bool duplicate_script_sub (struct npc_data *nd, const struct npc_data *snd, int xs, int ys, int options);
	static bool duplicate_shop_sub (struct npc_data *nd, const struct npc_data *snd, int xs, int ys, int options);
	static bool duplicate_warp_sub (struct npc_data *nd, const struct npc_data *snd, int xs, int ys, int options);
	static bool duplicate_sub (struct npc_data *nd, const struct npc_data *snd, int xs, int ys, int options);
	static const char *parse_duplicate (const char *w1, const char *w2, const char *w3, const char *w4, const char *start, const char *buffer, const char *filepath, int options, int *retval);
	static int duplicate4instance (struct npc_data *snd, int16 m);
	static void setcells (struct npc_data *nd);
	static int unsetcells_sub (struct block_list *bl, va_list ap);
	static void unsetcells (struct npc_data *nd);
	static void movenpc (struct npc_data *nd, int16 x, int16 y);
	static void setdisplayname (struct npc_data *nd, const char *newname);
	static void setclass (struct npc_data *nd, short class_);
	static int do_atcmd_event (struct map_session_data *sd, const char *command, const char *message, const char *eventname);
	static const char *parse_function (const char *w1, const char *w2, const char *w3, const char *w4, const char *start, const char *buffer, const char *filepath, int *retval);
	static void parse_mob2 (struct spawn_data *mobspawn);
	static const char *parse_mob (const char *w1, const char *w2, const char *w3, const char *w4, const char *start, const char *buffer, const char *filepath, int *retval);
	static const char *parse_mapflag (const char *w1, const char *w2, const char *w3, const char *w4, const char *start, const char *buffer, const char *filepath, int *retval);
	static void parse_unknown_mapflag (const char *name, const char *w3, const char *w4, const char *start, const char *buffer, const char *filepath, int *retval);
	static int parsesrcfile (const char *filepath, bool runOnInit);
	static int script_event (struct map_session_data *sd, enum npce_event type);
	static void read_event_script (void);
	static int path_db_clear_sub (DBKey key, DBData *data, va_list args);
	static int ev_label_db_clear_sub (DBKey key, DBData *data, va_list args);
	static int reload (void);
	static bool unloadfile (const char *filepath);
	static void do_clear_npc (void);
	static void debug_warps_sub (struct npc_data *nd);
	static void debug_warps (void);
	/* */
	static void trader_count_funds (struct npc_data *nd, struct map_session_data *sd);
	static bool trader_pay (struct npc_data *nd, struct map_session_data *sd, int price, int points);
	static void trader_update (int master);
	static int market_buylist (struct map_session_data* sd, unsigned short list_size, struct packet_npc_market_purchase *p);
	static bool trader_open (struct map_session_data *sd, struct npc_data *nd);
	static void market_fromsql (void);
	static void market_tosql (struct npc_data *nd, unsigned short index);
	static void market_delfromsql (struct npc_data *nd, unsigned short index);
	static void market_delfromsql_sub (const char *npcname, unsigned short index);
	static bool db_checkid (const int id);
	/**
	 * For the Secure NPC Timeout option (check config/Secure.h) [RR]
	 **/
	static int secure_timeout_timer (int tid, int64 tick, int id, intptr_t data);
};

extern CNpc *npc;

#ifdef HERCULES_CORE
void npc_defaults(void);
#endif // HERCULES_CORE


/* comes from npc_chat.c */
#ifdef PCRE_SUPPORT
#include <pcre/include/pcre.h>
#endif // PCRE_SUPPORT

/**
 * Structure containing all info associated with a single pattern block
 */
struct pcrematch_entry {
#ifdef PCRE_SUPPORT
	struct pcrematch_entry* next;
	char* pattern;
	pcre* pcre_;
	pcre_extra* pcre_extra_;
	char* label;
#else // not PCRE_SUPPORT
	UNAVAILABLE_STRUCT;
#endif // PCRE_SUPPORT
};

/**
 * A set of patterns that can be activated and deactived with a single command
 */
struct pcrematch_set {
#ifdef PCRE_SUPPORT
	struct pcrematch_set* prev;
	struct pcrematch_set* next;
	struct pcrematch_entry* head;
	int setid;
#else // not PCRE_SUPPORT
	UNAVAILABLE_STRUCT;
#endif // PCRE_SUPPORT
};

/**
 * Entire data structure hung off a NPC
 */
struct npc_parse {
#ifdef PCRE_SUPPORT
	struct pcrematch_set* active;
	struct pcrematch_set* inactive;
#else // not PCRE_SUPPORT
	UNAVAILABLE_STRUCT;
#endif // PCRE_SUPPORT
};

struct npc_chat_interface {
#ifdef PCRE_SUPPORT
	int (*sub) (struct block_list* bl, va_list ap);
	void (*finalize) (struct npc_data* nd);
	void (*def_pattern) (struct npc_data* nd, int setid, const char* pattern, const char* label);
	struct pcrematch_entry* (*create_pcrematch_entry) (struct pcrematch_set* set);
	void (*delete_pcreset) (struct npc_data* nd, int setid);
	void (*deactivate_pcreset) (struct npc_data* nd, int setid);
	void (*activate_pcreset) (struct npc_data* nd, int setid);
	struct pcrematch_set* (*lookup_pcreset) (struct npc_data* nd, int setid);
	void (*finalize_pcrematch_entry) (struct pcrematch_entry* e);
#else // not PCRE_SUPPORT
	UNAVAILABLE_STRUCT;
#endif // PCRE_SUPPORT
};

/**
 * pcre interface (libpcre)
 * so that plugins may share and take advantage of the core's pcre
 * should be moved into core/perhaps its own file once hpm is enhanced for login/char
 **/
struct pcre_interface {
#ifdef PCRE_SUPPORT
	pcre *(*compile) (const char *pattern, int options, const char **errptr, int *erroffset, const unsigned char *tableptr);
	pcre_extra *(*study) (const pcre *code, int options, const char **errptr);
	int (*exec) (const pcre *code, const pcre_extra *extra, PCRE_SPTR subject, int length, int startoffset, int options, int *ovector, int ovecsize);
	void (*free) (void *ptr);
	int (*copy_substring) (const char *subject, int *ovector, int stringcount, int stringnumber, char *buffer, int buffersize);
	void (*free_substring) (const char *stringptr);
	int (*copy_named_substring) (const pcre *code, const char *subject, int *ovector, int stringcount, const char *stringname, char *buffer, int buffersize);
	int (*get_substring) (const char *subject, int *ovector, int stringcount, int stringnumber, const char **stringptr);
#else // not PCRE_SUPPORT
	UNAVAILABLE_STRUCT;
#endif // PCRE_SUPPORT
};

/**
 * Also defaults libpcre
 **/
#ifdef HERCULES_CORE
void npc_chat_defaults(void);
#endif // HERCULES_CORE

HPShared struct npc_chat_interface *npc_chat;
HPShared struct pcre_interface *libpcre;

#endif /* MAP_NPC_H */
