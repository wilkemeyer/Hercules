#pragma once

#define HERCULES_CORE

// OS
#include "winapi.h"

// Libs/Deps:
#include <libconfig/libconfig.h>
#include <zlib.h>
#ifdef HAVE_EXECINFO
#include <execinfo.h>
#endif // HAVE_EXECINFO


// MySQL:
#include <mysql.h>


// Config
#include "../config/core.h"


// rgCore:
#include "../rgcore/rgcore_api.h"


// Locals:

#include "cbasetypes.h"
#include "conf.h"
#include "console.h"
#include "core.h"
#include "db.h"
#include "des.h"
#include "ers.h"
#include "grfio.h"
#include "hercules.h"
#include "mapindex.h"
#include "md5calc.h"
#include "memmgr.h"
#include "nullpo.h"
#include "random.h"
#include "showmsg.h"
#include "socket.h"
#include "sql.h"
#include "strlib.h"
#include "sysinfo.h"
#include "timer.h"
#include "utils.h"


// MMO.h:
#include "mmo.h"