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
#ifndef MAP_UNIT_H
#define MAP_UNIT_H

#include "clif.h"  // clr_type
#include "path.h" // struct walkpath_data
#include "skill.h" // 'MAX_SKILLTIMERSKILL, struct skill_timerskill, struct skill_unit_group, struct skill_unit_group_tickset
#include "../common/hercules.h"

struct map_session_data;
struct block_list;

/**
 * Bitmask values usable as a flag in unit_stopwalking
 */
enum unit_stopwalking_flag {
	STOPWALKING_FLAG_NONE     = 0x00,
	STOPWALKING_FLAG_FIXPOS   = 0x01, ///< Issue a fixpos packet afterwards
	STOPWALKING_FLAG_ONESTEP  = 0x02, ///< Force the unit to move one cell if it hasn't yet
	STOPWALKING_FLAG_NEXTCELL = 0x04, ///< Enable moving to the next cell when unit was already half-way there
	                                  ///  (may cause on-touch/place side-effects, such as a scripted map change)
	STOPWALKING_FLAG_MASK     = 0xff, ///< Mask all of the above
	// Note: Upper bytes are reserved for duration.
};

struct unit_data {
	struct block_list *bl;
	struct walkpath_data walkpath;
	struct skill_timerskill *skilltimerskill[MAX_SKILLTIMERSKILL];
	struct skill_unit_group *skillunit[MAX_SKILLUNITGROUP];
	struct skill_unit_group_tickset skillunittick[MAX_SKILLUNITGROUPTICKSET];
	short attacktarget_lv;
	short to_x,to_y;
	short skillx,skilly;
	uint16 skill_id,skill_lv;
	int   skilltarget;
	int   skilltimer;
	int   target;
	int   target_to;
	int   attacktimer;
	int   walktimer;
	int   chaserange;
	bool  stepaction; //Action should be executed on step [Playtester]
	int   steptimer; //Timer that triggers the action [Playtester]
	uint16 stepskill_id,stepskill_lv; //Remembers skill that should be casted on step [Playtester]
	int64 attackabletime;
	int64 canact_tick;
	int64 canmove_tick;
	uint8 dir;
	unsigned char walk_count;
	unsigned char target_count;
	struct {
		unsigned change_walk_target : 1 ;
		unsigned skillcastcancel : 1 ;
		unsigned attack_continue : 1 ;
		unsigned step_attack : 1;
		unsigned walk_easy : 1 ;
		unsigned running : 1;
		unsigned speed_changed : 1;
	} state;
};

struct view_data {
#ifdef __64BIT__
	uint32 class_; // FIXME: This shouldn't really depend on the architecture.
#else // not __64BIT__
	uint16 class_;
#endif // __64BIT__
	uint16 weapon,
		shield, //Or left-hand weapon.
		robe,
		head_top,
		head_mid,
		head_bottom,
		hair_style,
		hair_color,
		cloth_color;
	char sex;
	unsigned dead_sit : 2;
};

class CUnit {
public:
	static int init (bool minimal);
	static int final (void);
	/* */
	static struct unit_data* bl2ud (struct block_list *bl);
	static struct unit_data* bl2ud2 (struct block_list *bl);
	static int attack_timer (int tid, int64 tick, int id, intptr_t data);
	static int walktoxy_timer (int tid, int64 tick, int id, intptr_t data);
	static int walktoxy_sub (struct block_list *bl);
	static int delay_walktoxy_timer (int tid, int64 tick, int id, intptr_t data);
	static int walktoxy (struct block_list *bl, short x, short y, int flag);
	static int walktobl_sub (int tid, int64 tick, int id, intptr_t data);
	static int walktobl (struct block_list *bl, struct block_list *tbl, int range, int flag);
	static bool run (struct block_list *bl, struct map_session_data *sd, enum sc_type type);
	static void run_hit (struct block_list *bl, struct status_change *sc, struct map_session_data *sd, enum sc_type type);
	static int escape (struct block_list *bl, struct block_list *target, short dist);
	static int movepos (struct block_list *bl, short dst_x, short dst_y, int easy, bool checkpath);
	static int setdir (struct block_list *bl, unsigned char dir);
	static uint8 getdir (struct block_list *bl);
	static int blown (struct block_list *bl, int dx, int dy, int count, int flag);
	static int warp (struct block_list *bl, short m, short x, short y, clr_type type);
	static int stop_walking (struct block_list *bl, int type);
	static int skilluse_id (struct block_list *src, int target_id, uint16 skill_id, uint16 skill_lv);
	static int step_timer (int tid, int64 tick, int id, intptr_t data);
	static void stop_stepaction (struct block_list *bl);
	static int is_walking (struct block_list *bl);
	static int can_move (struct block_list *bl);
	static int resume_running (int tid, int64 tick, int id, intptr_t data);
	static int set_walkdelay (struct block_list *bl, int64 tick, int delay, int type);
	static int skilluse_id2 (struct block_list *src, int target_id, uint16 skill_id, uint16 skill_lv, int casttime, int castcancel);
	static int skilluse_pos (struct block_list *src, short skill_x, short skill_y, uint16 skill_id, uint16 skill_lv);
	static int skilluse_pos2 (struct block_list *src, short skill_x, short skill_y, uint16 skill_id, uint16 skill_lv, int casttime, int castcancel);
	static int set_target (struct unit_data *ud, int target_id);
	static void stop_attack (struct block_list *bl);
	static int unattackable (struct block_list *bl);
	static int attack (struct block_list *src, int target_id, int continuous);
	static int cancel_combo (struct block_list *bl);
	static bool can_reach_pos (struct block_list *bl, int x, int y, int easy);
	static bool can_reach_bl (struct block_list *bl, struct block_list *tbl, int range, int easy, short *x, short *y);
	static int calc_pos (struct block_list *bl, int tx, int ty, uint8 dir);
	static int attack_timer_sub (struct block_list *src, int tid, int64 tick);
	static int skillcastcancel (struct block_list *bl, int type);
	static void dataset (struct block_list *bl);
	static int counttargeted (struct block_list *bl);
	static int fixdamage (struct block_list *src, struct block_list *target, int sdelay, int ddelay, int64 damage, short div, unsigned char type, int64 damage2);
	static int changeviewsize (struct block_list *bl, short size);
	static int remove_map (struct block_list *bl, clr_type clrtype, const char *file, int line, const char *func);
	static void remove_map_pc (struct map_session_data *sd, clr_type clrtype);
	static void free_pc (struct map_session_data *sd);
	static int free (struct block_list *bl, clr_type clrtype);
};
extern CUnit *unit;

#ifdef HERCULES_CORE
extern const short dirx[8];
extern const short diry[8];

void unit_defaults(void);
#endif // HERCULES_CORE


#endif /* MAP_UNIT_H */
