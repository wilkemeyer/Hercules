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
#ifndef MAP_MAPREG_H
#define MAP_MAPREG_H

#include "script.h" // struct reg_db
#include "../common/hercules.h"
#include "../common/db.h"

struct eri;

/** Container for a mapreg value */
struct mapreg_save {
	int64 uid;         ///< Unique ID
	union {
		int i;     ///< Numeric value
		char *str; ///< String value
	} u;
	bool is_string;    ///< true if it's a string, false if it's a number
	bool save;         ///< Whether a save operation is pending
};

class CMapreg {
public:
	struct reg_db regs;
	/* */
	bool skip_insert;
	/* */
	struct eri *ers; //[Ind/Hercules]
	/* */
	char table[32];
	/* */
	bool dirty; ///< Whether there are modified regs to be saved
	/* */
	static void init (void);
	static void final (void);
	/* */
	static int readreg (int64 uid);
	static char* readregstr (int64 uid);
	static bool setreg (int64 uid, int val);
	static bool setregstr (int64 uid, const char *str);
	static void load (void);
	static void save (void);
	static int save_timer (int tid, int64 tick, int id, intptr_t data);
	static int destroyreg (DBKey key, DBData *data, va_list ap);
	static void reload (void);
	static bool config_read (const char *w1, const char *w2);
};
extern CMapreg *mapreg;

#ifdef HERCULES_CORE
void mapreg_defaults(void);
#endif // HERCULES_CORE


#endif /* MAP_MAPREG_H */
