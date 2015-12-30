#pragma once

// OS/Winapi:
#include "../common/winapi.h"

#define HERCULES_CORE

// Config:
#include "../config/core.h"


// Core:
#include "../common/cbasetypes.h"
#include "../common/conf.h"
#include "../common/core.h"
#include "../common/db.h"
#include "../common/memmgr.h"
#include "../common/md5calc.h"
#include "../common/nullpo.h"
#include "../common/random.h"
#include "../common/showmsg.h"
#include "../common/socket.h"
#include "../common/strlib.h"
#include "../common/timer.h"
#include "../common/utils.h"
#include "../common/console.h"
#include "../common/sql.h"
#include "../common/mapindex.h"

// MMO.h
#include "../common/mmo.h"

// rgCore
#include "../rgcore/rgcore.h"

// Locals
#include "char.h"
#include "geoip.h"
#include "int_auction.h"
#include "int_elemental.h"
#include "int_guild.h"
#include "int_homun.h"
#include "int_mail.h"
#include "int_mercenary.h"
#include "int_party.h"
#include "int_pet.h"
#include "int_quest.h"
#include "int_storage.h"
#include "inter.h"
#include "loginif.h"
#include "mapif.h"
#include "pincode.h"
