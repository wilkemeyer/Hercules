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
#include "pool.hpp"




// Platformlib Init:
void rgCore_init();
void rgCore_final();
