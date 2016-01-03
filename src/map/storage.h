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
#ifndef MAP_STORAGE_H
#define MAP_STORAGE_H

#include "../common/hercules.h"
#include "../common/db.h"

struct guild_storage;
struct item;
struct map_session_data;

/**
 * Acceptable values for map_session_data.state.storage_flag
 */
enum storage_flag {
	STORAGE_FLAG_CLOSED = 0, // Closed
	STORAGE_FLAG_NORMAL = 1, // Normal Storage open
	STORAGE_FLAG_GUILD  = 2, // Guild Storage open
};

struct storage_interface {
	/* */
	void (*reconnect) (void);
	/* */
	int (*delitem) (struct map_session_data* sd, int n, int amount);
	int (*open) (struct map_session_data *sd);
	int (*add) (struct map_session_data *sd,int index,int amount);
	int (*get) (struct map_session_data *sd,int index,int amount);
	int (*additem) (struct map_session_data* sd, struct item* item_data, int amount);
	int (*addfromcart) (struct map_session_data *sd,int index,int amount);
	int (*gettocart) (struct map_session_data *sd,int index,int amount);
	void (*close) (struct map_session_data *sd);
	void (*pc_quit) (struct map_session_data *sd, int flag);
	int (*comp_item) (const void *i1_, const void *i2_);
	void (*sortitem) (struct item* items, unsigned int size);
	int (*reconnect_sub) (DBKey key, DBData *data, va_list ap);
};

class CGstorage {
public:
	struct DBMap* db; // int guild_id -> struct guild_storage*
	/* */
	static struct guild_storage *ensure (int guild_id);
	/* */
	static void init (bool minimal);
	static void final (void);
	/* */
	static int _delete (int guild_id);
	static int open (struct map_session_data *sd);
	static int additem (struct map_session_data *sd,struct guild_storage *stor,struct item *item_data,int amount);
	static int delitem (struct map_session_data *sd,struct guild_storage *stor,int n,int amount);
	static int add (struct map_session_data *sd,int index,int amount);
	static int get (struct map_session_data *sd,int index,int amount);
	static int addfromcart (struct map_session_data *sd,int index,int amount);
	static int gettocart (struct map_session_data *sd,int index,int amount);
	static int close (struct map_session_data *sd);
	static int pc_quit (struct map_session_data *sd,int flag);
	static int save (int account_id, int guild_id, int flag);
	static int saved (int guild_id); //Ack from char server that guild store was saved.
	static DBData create (DBKey key, va_list args);
};
extern CGstorage *gstorage;

#ifdef HERCULES_CORE
void storage_defaults(void);
void gstorage_defaults(void);
#endif // HERCULES_CORE

HPShared struct storage_interface *storage;

#endif /* MAP_STORAGE_H */
