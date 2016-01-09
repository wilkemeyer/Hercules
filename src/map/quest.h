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
#ifndef MAP_QUEST_H
#define MAP_QUEST_H


#include "map.h" // TBL_PC
#include "../common/hercules.h"
#include "../common/conf.h"

#define MAX_QUEST_DB (60355+1) // Highest quest ID + 1

struct quest_dropitem {
	int mob_id;
	int nameid;
	int rate;
};

struct quest_objective {
	int mob;
	int count;
};

struct quest_db {
	int id;
	unsigned int time;
	int objectives_count;
	struct quest_objective *objectives;
	int dropitem_count;
	struct quest_dropitem *dropitem;
	//char name[NAME_LENGTH];
};

// Questlog check types
enum quest_check_type {
	HAVEQUEST, ///< Query the state of the given quest
	PLAYTIME,  ///< Check if the given quest has been completed or has yet to expire
	HUNTING,   ///< Check if the given hunting quest's requirements have been met
};

class CQuest {
public:
	static struct quest_db **db_data; ///< Quest database
	static struct quest_db dummy;                  ///< Dummy entry for invalid quest lookups
	/* */
	static void init (bool minimal);
	static void final (void);
	static void reload (void);
	/* */

	static struct quest_db *db (int quest_id);
	static int pc_login (struct map_session_data *sd);
	static int add (struct map_session_data *sd, int quest_id);
	static int change (struct map_session_data *sd, int qid1, int qid2);
	static int _delete (struct map_session_data *sd, int quest_id);
	static int update_objective_sub (struct block_list *bl, va_list ap);
	static void update_objective (struct map_session_data *sd, int mob_id);
	static int update_status (struct map_session_data *sd, int quest_id, enum quest_state qs);
	static int check (struct map_session_data *sd, int quest_id, enum quest_check_type type);
	static void clear (void);
	static int read_db (void);
	static struct quest_db *read_db_sub (config_setting_t *cs, int n, const char *source);

};
extern CQuest *quest;

#ifdef HERCULES_CORE
void quest_defaults(void);
#endif // HERCULES_CORE


#endif /* MAP_QUEST_H */
