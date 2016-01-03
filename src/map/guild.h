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
#ifndef MAP_GUILD_H
#define MAP_GUILD_H

#include "map.h" // EVENT_NAME_LENGTH, TBL_PC
#include "../common/hercules.h"
#include "../common/db.h"
#include "../common/mmo.h"

/**
 * Defines
 **/
#define GUILD_SEND_XY_INVERVAL  5000 // Interval of sending coordinates and HP
#define GUILD_PAYEXP_INVERVAL   10000 //Interval (maximum survival time of the cache, in milliseconds)
#define MAX_GUILD_SKILL_REQUIRE 5

/**
 * Structures
 **/
struct eventlist {
	char name[EVENT_NAME_LENGTH];
	struct eventlist *next;
};

/**
 * Guardian data
 * For quick linking to a guardian's info. [Skotlex]
 **/
struct guardian_data {
	int number; //0-MAX_GUARDIANS-1 = Guardians. MAX_GUARDIANS = Emperium.

	struct guild *g;
	struct guild_castle* castle;
};
struct guild_expcache {
	int guild_id, account_id, char_id;
	uint64 exp;
};
struct s_guild_skill_tree {
	int id;
	int max;
	struct {
		short id;
		short lv;
	} need[MAX_GUILD_SKILL_REQUIRE];
};


class CGuild {
public:
	/* */
	static DBMap* db; // int guild_id -> struct guild*
	static DBMap* castle_db; // int castle_id -> struct guild_castle*
	static DBMap* expcache_db; // int char_id -> struct guild_expcache*
	static DBMap* infoevent_db; // int guild_id -> struct eventlist*
						 /* */
	static struct eri *expcache_ers; //For handling of guild exp payment.
							  /* */
	static struct s_guild_skill_tree skill_tree[MAX_GUILDSKILL];
	/* guild flags cache */
	static struct npc_data **flags;
	static unsigned short flags_count;

	/* */
	static void init (bool minimal);
	static void final (void);
	/* */
	static int skill_get_max (int id);
	/* */
	static int checkskill (struct guild *g,int id);
	static int check_skill_require (struct guild *g,int id); // [Komurka]
	static int checkcastles (struct guild *g); // [MouseJstr]
	static bool isallied (int guild_id, int guild_id2); //Checks alliance based on guild Ids. [Skotlex]
	/* */
	static struct guild *search (int guild_id);
	static struct guild *searchname (char *str);
	static struct guild_castle *castle_search (int gcid);
	/* */
	static struct guild_castle *mapname2gc (const char* mapname);
	static struct guild_castle *mapindex2gc (short map_index);
	/* */
	static struct map_session_data *getavailablesd (struct guild *g);
	static int getindex (struct guild *g,int account_id,int char_id);
	static int getposition (struct guild *g, struct map_session_data *sd);
	static unsigned int payexp (struct map_session_data *sd,unsigned int exp);
	static int getexp (struct map_session_data *sd,int exp); // [Celest]
	/* */
	static int create (struct map_session_data *sd, const char *name);
	static int created (int account_id,int guild_id);
	static int request_info (int guild_id);
	static int recv_noinfo (int guild_id);
	static int recv_info (struct guild *sg);
	static int npc_request_info (int guild_id,const char *ev);
	static int invite (struct map_session_data *sd,struct map_session_data *tsd);
	static int reply_invite (struct map_session_data *sd,int guild_id,int flag);
	static void member_joined (struct map_session_data *sd);
	static int member_added (int guild_id,int account_id,int char_id,int flag);
	static int leave (struct map_session_data *sd,int guild_id,int account_id,int char_id,const char *mes);
	static int member_withdraw (int guild_id,int account_id,int char_id,int flag,const char *name,const char *mes);
	static int expulsion (struct map_session_data *sd,int guild_id,int account_id,int char_id,const char *mes);
	static int skillup (struct map_session_data* sd, uint16 skill_id);
	static void block_skill (struct map_session_data *sd, int time);
	static int reqalliance (struct map_session_data *sd,struct map_session_data *tsd);
	static int reply_reqalliance (struct map_session_data *sd,int account_id,int flag);
	static int allianceack (int guild_id1,int guild_id2,int account_id1,int account_id2,int flag,const char *name1,const char *name2);
	static int delalliance (struct map_session_data *sd,int guild_id,int flag);
	static int opposition (struct map_session_data *sd,struct map_session_data *tsd);
	static int check_alliance (int guild_id1, int guild_id2, int flag);
	/* */
	static int send_memberinfoshort (struct map_session_data *sd,int online);
	static int recv_memberinfoshort (int guild_id,int account_id,int char_id,int online,int lv,int class_);
	static int change_memberposition (int guild_id,int account_id,int char_id,short idx);
	static int memberposition_changed (struct guild *g,int idx,int pos);
	static int change_position (int guild_id,int idx,int mode,int exp_mode,const char *name);
	static int position_changed (int guild_id,int idx,struct guild_position *p);
	static int change_notice (struct map_session_data *sd,int guild_id,const char *mes1,const char *mes2);
	static int notice_changed (int guild_id,const char *mes1,const char *mes2);
	static int change_emblem (struct map_session_data *sd,int len,const char *data);
	static int emblem_changed (int len,int guild_id,int emblem_id,const char *data);
	static int send_message (struct map_session_data *sd,const char *mes,int len);
	static int recv_message (int guild_id,int account_id,const char *mes,int len);
	static int send_dot_remove (struct map_session_data *sd);
	static int skillupack (int guild_id,uint16 skill_id,int account_id);
	static int dobreak (struct map_session_data *sd,char *name);
	static int broken (int guild_id,int flag);
	static int gm_change (int guild_id, struct map_session_data *sd);
	static int gm_changed (int guild_id, int account_id, int char_id);
	/* */
	static void castle_map_init (void);
	static int castledatasave (int castle_id,int index,int value);
	static int castledataloadack (int len, struct guild_castle *gc);
	static void castle_reconnect (int castle_id, int index, int value);
	/* */
	static void agit_start (void);
	static void agit_end (void);
	static void agit2_start (void);
	static void agit2_end (void);
	/* guild flag cachin */
	static void flag_add (struct npc_data *nd);
	static void flag_remove (struct npc_data *nd);
	static void flags_clear (void);
	/* guild aura */
	static void aura_refresh (struct map_session_data *sd, uint16 skill_id, uint16 skill_lv);
	/* item bound [Mhalicot]*/
	static void retrieveitembound (int char_id,int aid,int guild_id);
	/* */
	static int payexp_timer (int tid, int64 tick, int id, intptr_t data);
	static TBL_PC* sd_check (int guild_id, int account_id, int char_id);
	static bool read_guildskill_tree_db (char* split[], int columns, int current);
	static bool read_castledb (char* str[], int columns, int current);
	static int payexp_timer_sub (DBKey key, DBData *data, va_list ap);
	static int send_xy_timer_sub (DBKey key, DBData *data, va_list ap);
	static int send_xy_timer (int tid, int64 tick, int id, intptr_t data);
	static DBData create_expcache (DBKey key, va_list args);
	static int eventlist_db_final (DBKey key, DBData *data, va_list ap);
	static int expcache_db_final (DBKey key, DBData *data, va_list ap);
	static int castle_db_final (DBKey key, DBData *data, va_list ap);
	static int broken_sub (DBKey key, DBData *data, va_list ap);
	static int castle_broken_sub (DBKey key, DBData *data, va_list ap);
	static void makemember (struct guild_member *m,struct map_session_data *sd);
	static int check_member (struct guild *g);
	static int get_alliance_count (struct guild *g,int flag);
	static void castle_reconnect_sub (void *key, void *data, va_list ap);
};

extern CGuild *guild;

#ifdef HERCULES_CORE
void guild_defaults(void);
#endif // HERCULES_CORE


#endif /* MAP_GUILD_H */
