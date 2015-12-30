#pragma once


#ifdef PCRE_SUPPORT
#include <pcre/include/pcre.h>
#endif




#define HERCULES_CORE

// Config
#include "../config/core.h" 


// Core 
#include "../common/winapi.h"
#include "../common/cbasetypes.h"
#include "../common/core.h"
#include "../common/conf.h"
#include "../common/ers.h"
#include "../common/grfio.h"
#include "../common/memmgr.h"
#include "../common/md5calc.h"
#include "../common/nullpo.h"
#include "../common/random.h"
#include "../common/showmsg.h"
#include "../common/socket.h"
#include "../common/strlib.h"
#include "../common/sysinfo.h"
#include "../common/timer.h"
#include "../common/utils.h"
#include "../common/console.h"
#include "../common/sql.h"

// MMO.h
#include "../common/mmo.h" // MAX_CARTS


// rgCore
#include "../rgcore/rgcore_api.h"



// Locals
#include "atcommand.h"
#include "battle.h"
#include "battleground.h"
#include "channel.h"
#include "chat.h"
#include "chrif.h"
#include "clif.h"
#include "duel.h"
#include "date.h"
#include "elemental.h"
#include "guild.h"
#include "homunculus.h"
#include "intif.h"
#include "itemdb.h"
#include "instance.h"
#include "irc-bot.h"
#include "log.h"
#include "mail.h"
#include "map.h"
#include "mapreg.h"
#include "mercenary.h"
#include "mob.h"
#include "npc.h"
#include "path.h"
#include "party.h"
#include "pc.h"
#include "pc_groups.h" // groupid2name
#include "pet.h"
#include "quest.h"
#include "script.h"
#include "searchstore.h"
#include "skill.h"
#include "status.h"
#include "storage.h"
#include "trade.h"
#include "unit.h"