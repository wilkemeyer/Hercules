// Copyright (c) Athena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#ifndef COMMON_MEMMGR_H
#define COMMON_MEMMGR_H

#include "hercules.h"
#include "../rgcore/rgcore.h"

#define ALC_MARK __FILE__, __LINE__, __func__


// Proxy -> rgCore::roalloc
#define aMalloc(sz)				roalloc(sz)
#define aCalloc(m, n)			rocalloc(m, n)
#define aRealloc(ptr, newsz)	rorealloc(ptr, newsz)
#define aReallocz(ptr, newsz)	rorealloc(ptr, newsz)
#define aStrdup(ptr)			rostrdup(ptr)
#define aFree(ptr)				rofree(ptr)
#define aVerify(ptr)			(true)	// Checks if the given ptr is a valid pointer


/////////////// Buffer Creation /////////////////
// Full credit for this goes to Shinomori [Ajarn]

#ifdef __GNUC__ // GCC has variable length arrays

#define CREATE_BUFFER(name, type, size) type name[size]
#define DELETE_BUFFER(name)

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
