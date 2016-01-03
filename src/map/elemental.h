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
#ifndef MAP_ELEMENTAL_H
#define MAP_ELEMENTAL_H

#include "status.h" // struct status_data, struct status_change
#include "unit.h" // struct unit_data
#include "../common/hercules.h"
#include "../common/mmo.h" // NAME_LENGTH


/**
 * Enums
 */
/// Elemental IDs
enum elemental_id {
	ELEID_EL_AGNI_S   = 2114, ///<   EL_AGNI_S / Agni
	ELEID_EL_AGNI_M   = 2115, ///<   EL_AGNI_M / Agni
	ELEID_EL_AGNI_L   = 2116, ///<   EL_AGNI_L / Agni
	ELEID_EL_AQUA_S   = 2117, ///<   EL_AQUA_S / Aqua
	ELEID_EL_AQUA_M   = 2118, ///<   EL_AQUA_M / Aqua
	ELEID_EL_AQUA_L   = 2119, ///<   EL_AQUA_L / Aqua
	ELEID_EL_VENTUS_S = 2120, ///< EL_VENTUS_S / Ventus
	ELEID_EL_VENTUS_M = 2121, ///< EL_VENTUS_M / Ventus
	ELEID_EL_VENTUS_L = 2122, ///< EL_VENTUS_L / Ventus
	ELEID_EL_TERA_S   = 2123, ///<   EL_TERA_S / Tera
	ELEID_EL_TERA_M   = 2124, ///<   EL_TERA_M / Tera
	ELEID_EL_TERA_L   = 2125, ///<   EL_TERA_L / Tera
};

/**
 * Defines
 **/
#define MIN_ELETHINKTIME 100
#define MIN_ELEDISTANCE 2
#define MAX_ELEDISTANCE 5
#define EL_MODE_AGGRESSIVE (MD_CANMOVE|MD_AGGRESSIVE|MD_CANATTACK)
#define EL_MODE_ASSIST (MD_CANMOVE|MD_ASSIST)
#define EL_MODE_PASSIVE MD_CANMOVE
#define EL_SKILLMODE_PASIVE 0x1
#define EL_SKILLMODE_ASSIST 0x2
#define EL_SKILLMODE_AGGRESSIVE 0x4
#define MAX_ELEMENTAL_CLASS 12
#define EL_CLASS_BASE ELEID_EL_AGNI_S
#define EL_CLASS_MAX (EL_CLASS_BASE+MAX_ELEMENTAL_CLASS-1)

#define elemental_stop_walking(ed, type) (unit->stop_walking(&(ed)->bl, (type)))
#define elemental_stop_attack(ed)        (unit->stop_attack(&(ed)->bl))

/**
 * Structures
 **/
struct elemental_skill {
	unsigned short id, lv;
	short mode;
};

struct s_elemental_db {
	int class_;
	char sprite[NAME_LENGTH], name[NAME_LENGTH];
	unsigned short lv;
	short range2, range3;
	struct status_data status;
	struct view_data vd;
	struct elemental_skill skill[MAX_ELESKILLTREE];
};

struct elemental_data {
	struct block_list bl;
	struct unit_data ud;
	struct view_data *vd;
	struct status_data base_status, battle_status;
	struct status_change sc;
	struct regen_data regen;

	struct s_elemental_db *db;
	struct s_elemental elemental;

	struct map_session_data *master;
	int summon_timer;
	int skill_timer;

	int64 last_thinktime, last_linktime, last_spdrain_time;
	short min_chase;
	int target_id, attacked_id;
};

/*=====================================
* Interface : elemental.h
* Generated by HerculesInterfaceMaker
* created by Susu
*-------------------------------------*/
class CElemental {
public:

	/* vars */
	static struct s_elemental_db db[MAX_ELEMENTAL_CLASS]; // Elemental Database

	/* */
	static int init (bool minimal);
	static void final (void);
	/* funcs */
	static bool _class (int class_);
	static struct view_data * get_viewdata (int class_);

	static int create (struct map_session_data *sd, int class_, unsigned int lifetime);
	static int data_received (struct s_elemental *ele, bool flag);
	static int save (struct elemental_data *ed);

	static int change_mode_ack (struct elemental_data *ed, int mode);
	static int change_mode (struct elemental_data *ed, int mode);

	static void heal (struct elemental_data *ed, int hp, int sp);
	static int dead (struct elemental_data *ed);

	static int _delete (struct elemental_data *ed, int reply);
	static void summon_stop (struct elemental_data *ed);

	static int get_lifetime (struct elemental_data *ed);

	static int unlocktarget (struct elemental_data *ed);
	static int skillnotok (uint16 skill_id, struct elemental_data *ed);
	static int set_target (struct map_session_data *sd, struct block_list *bl);
	static int clean_single_effect (struct elemental_data *ed, uint16 skill_id);
	static int clean_effect (struct elemental_data *ed);
	static int action (struct elemental_data *ed, struct block_list *bl, int64 tick);
	static struct skill_condition skill_get_requirements (uint16 skill_id, uint16 skill_lv);

	static int read_skilldb (void);
	static void reload_db (void);
	static void reload_skilldb (void);

	static int search_index (int class_);
	static void summon_init (struct elemental_data *ed);
	static int summon_end_timer (int tid, int64 tick, int id, intptr_t data);
	static int ai_sub_timer_activesearch (struct block_list *bl, va_list ap);
	static int ai_sub_timer (struct elemental_data *ed, struct map_session_data *sd, int64 tick);
	static int ai_sub_foreachclient (struct map_session_data *sd, va_list ap);
	static int ai_timer (int tid, int64 tick, int id, intptr_t data);
	static int read_db (void);
};
extern CElemental *elemental;

#ifdef HERCULES_CORE
void elemental_defaults(void);
#endif // HERCULES_CORE


#endif /* MAP_ELEMENTAL_H */
