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
#ifndef MAP_TRADE_H
#define MAP_TRADE_H

#include "../common/hercules.h"

//Max distance from traders to enable a trade to take place.
//TODO: battle_config candidate?
#define TRADE_DISTANCE 2

struct map_session_data;

class CTrade {
public:
	static void request (struct map_session_data *sd, struct map_session_data *target_sd);
	static void ack (struct map_session_data *sd,int type);
	static int check_impossible (struct map_session_data *sd);
	static int check (struct map_session_data *sd, struct map_session_data *tsd);
	static void additem (struct map_session_data *sd,short index,short amount);
	static void addzeny (struct map_session_data *sd,int amount);
	static void ok (struct map_session_data *sd);
	static void cancel (struct map_session_data *sd);
	static void commit (struct map_session_data *sd);
};
extern CTrade *trade;
#ifdef HERCULES_CORE
void trade_defaults(void);
#endif // HERCULES_CORE


#endif /* MAP_TRADE_H */
