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
#ifndef MAP_ATCOMMAND_H
#define MAP_ATCOMMAND_H

#include "pc_groups.h"
#include "../common/hercules.h"
#include "../common/conf.h"
#include "../common/db.h"


/**
 * Declarations
 **/
struct map_session_data;
struct AtCommandInfo;
struct block_list;

/**
 * Defines
 **/
#define ATCOMMAND_LENGTH 50
#define MAX_MSG 1500
#define msg_txt(idx) atcommand->msg(idx)
#define msg_sd(sd,msg_number) atcommand->msgsd((sd),(msg_number))
#define msg_fd(fd,msg_number) atcommand->msgfd((fd),(msg_number))

/**
 * Enumerations
 **/
typedef enum {
	COMMAND_ATCOMMAND = 1,
	COMMAND_CHARCOMMAND = 2,
} AtCommandType;

/**
 * Typedef
 **/
typedef bool (*AtCommandFunc)(const int fd, struct map_session_data* sd, const char* command, const char* message,struct AtCommandInfo *info);
typedef struct AtCommandInfo AtCommandInfo;
typedef struct AliasInfo AliasInfo;

/**
 * Structures
 **/
struct AliasInfo {
	AtCommandInfo *command;
	char alias[ATCOMMAND_LENGTH];
};

struct AtCommandInfo {
	char command[ATCOMMAND_LENGTH];
	AtCommandFunc func;
	char *at_groups;/* quick @commands "can-use" lookup */
	char *char_groups;/* quick @charcommands "can-use" lookup */
	char *help;/* quick access to this @command's help string */
	bool log;/* whether to log this command or not, regardless of group settings */
};

struct atcmd_binding_data {
	char command[ATCOMMAND_LENGTH];
	char npc_event[ATCOMMAND_LENGTH];
	int group_lv;
	int group_lv_char;
	bool log;
};

/**
 * Interface
 **/
class CAtcommand {
public:
	static unsigned char at_symbol;
	static unsigned char char_symbol;
	/* atcommand binding */
	static struct atcmd_binding_data** binding;
	static int binding_count;
	/* other vars */
	static DBMap* db; //name -> AtCommandInfo
	static DBMap* alias_db; //alias -> AtCommandInfo
	/**
	 * msg_table[lang_id][msg_id]
	 * Server messages (0-499 reserved for GM commands, 500-999 reserved for others)
	 **/
	static char*** msg_table;
	static uint8 max_message_table;


	/* */
	static void init (bool minimal);
	static void final (void);
	/* */
	static bool exec (const int fd, struct map_session_data *sd, const char *message, bool player_invoked);
	static bool create (char *name, AtCommandFunc func);
	static bool can_use (struct map_session_data *sd, const char *command);
	static bool can_use2 (struct map_session_data *sd, const char *command, AtCommandType type);
	static void load_groups (GroupSettings **groups, config_setting_t **commands_, size_t sz);
	static AtCommandInfo* exists (const char* name);
	static bool msg_read (const char *cfg_name, bool allow_override);
	static void final_msg (void);
	/* atcommand binding */
	static struct atcmd_binding_data* get_bind_byname (const char* name);
	/* */
	static AtCommandInfo* get_info_byname (const char *name); // @help
	static const char* check_alias (const char *aliasname); // @help
	static void get_suggestions (struct map_session_data* sd, const char *name, bool is_atcmd_cmd); // @help
	static void config_read (const char* config_filename);
	/* command-specific subs */
	static int stopattack (struct block_list *bl,va_list ap);
	static int pvpoff_sub (struct block_list *bl,va_list ap);
	static int pvpon_sub (struct block_list *bl,va_list ap);
	static int atkillmonster_sub (struct block_list *bl, va_list ap);
	static void raise_sub (struct map_session_data* sd);
	static void get_jail_time (int jailtime, int* year, int* month, int* day, int* hour, int* minute);
	static int cleanfloor_sub (struct block_list *bl, va_list ap);
	static int mutearea_sub (struct block_list *bl,va_list ap);
	static void getring (struct map_session_data* sd);
	static void channel_help (int fd, const char *command, bool can_create);
	/* */
	static void commands_sub (struct map_session_data* sd, const int fd, AtCommandType type);
	static void cmd_db_clear (void);
	static int cmd_db_clear_sub (DBKey key, DBData *data, va_list args);
	static void doload (void);
	static void base_commands (void);
	static bool add (char *name, AtCommandFunc func, bool replace);
	static const char* msg (int msg_number);
	static void expand_message_table (void);
	static const char* msgfd (int fd, int msg_number);
	static const char* msgsd (struct map_session_data *sd, int msg_number);
};
extern CAtcommand *atcommand;

#ifdef HERCULES_CORE
void atcommand_defaults(void);
#endif // HERCULES_CORE


/* stay here */
#define ACMD(x) static bool atcommand_ ## x (const int fd, struct map_session_data* sd, const char* command, const char* message, struct AtCommandInfo *info) __attribute__((nonnull (2, 3, 4, 5))); \
    static bool atcommand_ ## x (const int fd, struct map_session_data* sd, const char* command, const char* message, struct AtCommandInfo *info)

#endif /* MAP_ATCOMMAND_H */
