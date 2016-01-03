/**
 * This file is part of Hercules.
 * http://herc.ws - http://github.com/HerculesWS/Hercules
 *
 * Copyright (C) 2013-2015  Hercules Dev Team
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

/**
 * Base Author: shennetsind @ http://herc.ws
 */
#ifndef MAP_IRC_BOT_H
#define MAP_IRC_BOT_H

#include "../common/hercules.h"

#define IRC_NICK_LENGTH 40
#define IRC_IDENT_LENGTH 40
#define IRC_HOST_LENGTH 63
#define IRC_FUNC_LENGTH 30
#define IRC_MESSAGE_LENGTH 500

struct channel_data;

struct irc_func {
	char name[IRC_FUNC_LENGTH];
	void (*func)(int, char*, char*, char*, char*);
};

class CIrcbot {
public:
	static int fd;
	static bool isIn, isOn;
	static int64 last_try;
	static unsigned char fails;
	static uint32 ip;
	static unsigned short port;
	/* */
	static struct channel_data *channel;
	/* */
	static struct _FuncList{
		struct irc_func **list;
		unsigned int size;
	} funcs;

	/* */
	static void init (bool minimal);
	static void final (void);
	/* */
	static int parse (int fd);
	static void parse_sub (int fd, char *str);
	static void parse_source (char *source, char *nick, char *ident, char *host);
	/* */
	static struct irc_func* func_search (char* function_name);
	/* */
	static int connect_timer (int tid, int64 tick, int id, intptr_t data);
	static int identify_timer (int tid, int64 tick, int id, intptr_t data);
	static int join_timer (int tid, int64 tick, int id, intptr_t data);
	/* */
	static void send(char *str);
	static void relay (const char *name, const char *msg);
	/* */
	static void pong (int fd, char *cmd, char *source, char *target, char *msg);
	static void privmsg (int fd, char *cmd, char *source, char *target, char *msg);
	static void userjoin (int fd, char *cmd, char *source, char *target, char *msg);
	static void userleave (int fd, char *cmd, char *source, char *target, char *msg);
	static void usernick (int fd, char *cmd, char *source, char *target, char *msg);
};
extern CIrcbot *ircbot;

#ifdef HERCULES_CORE
void ircbot_defaults(void);
#endif // HERCULES_CORE


#endif /* MAP_IRC_BOT_H */
