#pragma once

//
// RG-Core Configuration
//


//// ==================================================================================
//// ==================================================================================
//// CONFIG/INI CONFIGURATION
//// ==================================================================================

// Default Application Config File Name 
#define INI_DEFAULT_APP_CONFIG "Settings/%s.ini"



//// ==================================================================================
//// ==================================================================================
//// ROALLOC CONFIGURATION
//// ==================================================================================

// Note:
// All ROALLOC_ Switches must be set to 0 to disable or 1 to enable!

// Use TBB as Backing Allocator
#define ROALLOC_USE_TBB 1

// Minimum blocksize that could be allocated thru roalloc 
#define ROALLOC_MINBLOCKSIZE 32

// Trace Memory Allocations
// will detect memory leaks (unfreed memory upon application termination)
// Note:
//		This will break MT (Parallel Allocation Support)
//		Only Enable if Required!
#define ROALLOC_TRACE 0


