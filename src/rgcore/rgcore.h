#pragma once
#define HERCULES_CORE


// Platform/OS:
#include "../common/winapi.h"

// Config:
#include "../config/rgcore.h"

// 3rdparty Libs:
// TBB
#define __TBBMALLOC_NO_IMPLICIT_LINKAGE 1 // prevent lib linkage (as our tbb release's lib names differ from vanilla's)
#include "../../3rdparty/tbb/include/tbb/scalable_allocator.h"
#include "../../3rdparty/tbb/include/tbb/cache_aligned_allocator.h"


// Log
#include "Logging/ILogger.h"
#include "Logging/globalLoggerDefines.h"

//
// Locals
//
#include "arch.h"

// Config Parsers
#include "ini.hpp"

// Time/Tick
#include "tick.h"

// Exception/Error(Fatal)
#include "exception/Exception.hpp"
#include "exception/fatal.h"

// Threading/Synchronization 
#include "mt_util/box.hpp"
#include "mt_util/atomic.hpp"
#include "mt_util/thread.hpp"
#include "mt_util/mutex.hpp"
#include "mt_util/cond.hpp"
#include "mt_util/getpagesize.hpp"
#include "mt_util/dynLib.hpp"
#include "mt_util/stackbase.hpp"

#include "mt_util/spinlock.hpp"

#if defined(ARCH_AMD64)
#include "mt_util/atomicSList64.hpp"
#elif defined(ARCH_X86)
#include "mt_util/atomicSList.hpp"
#endif


// Memory Allocation:
#include "roalloc.h"

// Logger:
#include "Logging/Logger.h"
#include "Logging/coreLogger.h"	// Actual  Generic Implementation (showmsg replacement)



#include "pool.hpp"



// 
// Call / Procesing:
//	- rgCore_init()
//	-> recommended to create a thread for applicaiton specific initialization!
//	- rgCore_idleLoop()
//	-> (CURRENTLY HARDCODED) GUI Close Button will make applicaiton-specific callback 
//	-> to releasse rgCore_idleLoop  call rgCore_releaseIdleLoop (in application-specific callback, after you've finialized your -application specific- stuff)
//  - rg_final()
//


// Platformlib Init:
void rgCore_init(const char *appName);
void rgCore_final();

// Note: calling this proc will close the GUI! 
// any further (core-specific) debug output will be logged to file (if enabled)
void rgCore_releaseIdleLoop();

// Must be Called in Main Thread
// Will Block until beginFinal() gets called
void rgCore_idleLoop();

const char *rgCore_getAppName();