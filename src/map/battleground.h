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
#ifndef MAP_BATTLEGROUND_H
#define MAP_BATTLEGROUND_H

#include "map.h" // EVENT_NAME_LENGTH
#include "../common/hercules.h"
#include "../common/db.h"
#include "../common/mmo.h" // struct party

struct block_list;
struct map_session_data;

/**
 * Defines
 **/
#define MAX_BG_MEMBERS 30
#define BG_DELAY_VAR_LENGTH 30

/**
 * Enumerations
 **/
enum bg_queue_types {
	BGQT_INVALID    = 0x0,
	BGQT_INDIVIDUAL = 0x1,
	BGQT_PARTY      = 0x2,
	/* yup no 0x3 */
	BGQT_GUILD      = 0x4,
};

enum bg_team_leave_type {
	BGTL_LEFT = 0x0,
	BGTL_QUIT = 0x1,
	BGTL_AFK  = 0x2,
};

struct battleground_member_data {
	unsigned short x, y;
	struct map_session_data *sd;
	unsigned afk : 1;
	struct point source;/* where did i come from before i join? */
};

struct battleground_data {
	unsigned int bg_id;
	unsigned char count;
	struct battleground_member_data members[MAX_BG_MEMBERS];
	// BG Cementery
	unsigned short mapindex, x, y;
	// Logout Event
	char logout_event[EVENT_NAME_LENGTH];
	char die_event[EVENT_NAME_LENGTH];
};

struct bg_arena {
	char name[NAME_LENGTH];
	unsigned char id;
	char npc_event[EVENT_NAME_LENGTH];
	short min_level, max_level;
	short prize_win, prize_loss, prize_draw;
	short min_players;
	short max_players;
	short min_team_players;
	char delay_var[NAME_LENGTH];
	unsigned short maxDuration;
	int queue_id;
	int begin_timer;
	int fillup_timer;
	int game_timer;
	unsigned short fillup_duration;
	unsigned short pregame_duration;
	bool ongoing;
	enum bg_queue_types allowed_types;
};

class CBg {
public:
	static bool queue_on;
	/* */
	static int mafksec, afk_timer_id;
	static char gdelay_var[BG_DELAY_VAR_LENGTH];
	/* */
	static struct bg_arena **arena;
	static unsigned char arenas;
	/* */
	static DBMap *team_db; // int bg_id -> struct battleground_data*
	static unsigned int team_counter; // Next bg_id
	/* */
	static void init (bool minimal);
	static void final (void);
	/* */
	static struct bg_arena *name2arena (char *name);
	static void queue_add (struct map_session_data *sd, struct bg_arena *arena, enum bg_queue_types type);
	static enum BATTLEGROUNDS_QUEUE_ACK can_queue (struct map_session_data *sd, struct bg_arena *arena, enum bg_queue_types type);
	static int id2pos (int queue_id, int account_id);
	static void queue_pc_cleanup (struct map_session_data *sd);
	static void begin (struct bg_arena *arena);
	static int begin_timer (int tid, int64 tick, int id, intptr_t data);
	static void queue_pregame (struct bg_arena *arena);
	static int fillup_timer (int tid, int64 tick, int id, intptr_t data);
	static void queue_ready_ack (struct bg_arena *arena, struct map_session_data *sd, bool response);
	static void match_over (struct bg_arena *arena, bool canceled);
	static void queue_check (struct bg_arena *arena);
	static struct battleground_data* team_search (int bg_id);
	static struct map_session_data* getavailablesd (struct battleground_data *bgd);
	static bool team_delete (int bg_id);
	static bool team_warp (int bg_id, unsigned short map_index, short x, short y);
	static void send_dot_remove (struct map_session_data *sd);
	static bool team_join (int bg_id, struct map_session_data *sd);
	static int team_leave (struct map_session_data *sd, enum bg_team_leave_type flag);
	static bool member_respawn (struct map_session_data *sd);
	static int create (unsigned short map_index, short rx, short ry, const char *ev, const char *dev);
	static int team_get_id (struct block_list *bl);
	static bool send_message (struct map_session_data *sd, const char *mes, int len);
	static int send_xy_timer_sub (DBKey key, DBData *data, va_list ap);
	static int send_xy_timer (int tid, int64 tick, int id, intptr_t data);
	static int afk_timer (int tid, int64 tick, int id, intptr_t data);
	static int team_db_final (DBKey key, DBData *data, va_list ap);
	/* */
	static enum bg_queue_types str2teamtype (const char *str);
	/* */
	static void config_read (void);
};

extern CBg *bg;


#ifdef HERCULES_CORE
void battleground_defaults(void);
#endif // HERCULES_CORE


#endif /* MAP_BATTLEGROUND_H */
