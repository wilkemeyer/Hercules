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
#ifndef COMMON_MEMMGR_H
#define COMMON_MEMMGR_H

#include "hercules.h"
#include "../rgcore/rgcore.h"

#define ALC_MARK __FILE__, __LINE__, __func__


// Proxy -> rgCore::roalloc
#define aMalloc(sz)				roalloc(sz)
#define aCalloc(m, n)			rocalloc(m, n)
#define aRealloc(ptr, newsz)	rorealloc(ptr, newsz)
#define aReallocz(ptr, newsz)	roreallocz(ptr, newsz)
#define aStrdup(ptr)			rostrdup(ptr)
#define aFree(ptr)				rofree(ptr)
#define aVerify(ptr)			(true)	// Checks if the given ptr is a valid pointer


/////////////// Buffer Creation /////////////////
// Full credit for this goes to Shinomori [Ajarn]

#ifdef __GNUC__ // GCC has variable length arrays

#define CREATE_BUFFER(name, type, size) type name[size]
#define DELETE_BUFFER(name) (void)0

#else // others don't, so we emulate them

#define CREATE_BUFFER(name, type, size) type *name = (type *) aCalloc((size), sizeof(type))
#define DELETE_BUFFER(name) aFree(name)

#endif

////////////// Others //////////////////////////
// should be merged with any of above later
#define CREATE(result, type, number) ((result) = (type *) aCalloc((number), sizeof(type)))
#define RECREATE(result, type, number) ((result) = (type *) aReallocz((result), sizeof(type) * (number)))

////////////////////////////////////////////////

#endif /* COMMON_MEMMGR_H */
