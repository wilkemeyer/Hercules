/*
	The MIT License (MIT)

	Copyright (c) 2015 Florian Wilkemeyer <fw@f-ws.de>

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all
	copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
	SOFTWARE.
*/

#include "stdafx.h"

enum LargePageStatus g_largePageStatus = ROALLOC_LP_Disabled;

enum LargePageStatus roalloc_getLargePageStatus(){
	return g_largePageStatus;
}//end: roalloc_getLargePageStatus()



#if LINUX | ROALLOC_USE_TBB

static __forceinline void mbackend_init() {
	bool bLargePages = iniGetAppBoolean("roalloc", "Use Large Lages", false);

	if(bLargePages == true){
		g_largePageStatus = ROALLOC_LP_Enabled;
		if(TBBMALLOC_OK != scalable_allocation_mode(TBBMALLOC_USE_HUGE_PAGES, 1) ){
			g_largePageStatus = ROALLOC_LP_OSRefused;
		}
	}
}

static __forceinline void mbackend_final() {}

static __forceinline void *mbackend_allocAligned(size_t sz) {
	return scalable_aligned_malloc(sz, ARCH_MEMALIGN);
}//end: mbackend_allocAligned()

static __forceinline void *mbackend_reallocAligned(void *ptr, size_t newsz) {
	return scalable_aligned_realloc(ptr, newsz, ARCH_MEMALIGN);
}//end: mbackend_reallocAligned()

static __forceinline void mbackend_freeAligned(void *ptr) {
	return scalable_aligned_free(ptr);
}//end: mbackend_freeAligned()

static __forceinline size_t mbackend_msize(void *ptr){
	return scalable_msize(ptr);
}//end: mbackend_msize()

#else

static __forceinline void mbackend_init() {}
static __forceinline void mbackend_final() {}

static __forceinline void *mbackend_allocAligned(size_t sz) {
	return _aligned_malloc(sz, ARCH_MEMALIGN);
}//end: mbackend_allocAligned()

static __forceinline void *mbackend_reallocAligned(void *ptr, size_t newsz) {
	return _aligned_realloc(ptr, newsz, ARCH_MEMALIGN);
}//end: mbackend_reallocAligned()

static __forceinline void mbackend_freeAligned(void *ptr) {
	return _aligned_free(ptr);
}//end: mbackend_freeAligned()


#endif







#if ROALLOC_TRACE
static bool g_initialized = false;
static rgCore::util::mutex *g_lock = NULL;
static unsigned char g_DBGSIGNATURE[16];

typedef struct __declspec(align(ARCH_MEMALIGN)) RC_DBG_MEMBLOCK{
	void	*user_ptr;	/// address, the game has got 

	// Block List
	struct RC_DBG_MEMBLOCK *prev, *next;


	enum{
		ALLOC=0, REALLOC, STRDUP, CALLOC, REALLOCZ
	} last_action;
	
	
	// Values represent the user_ptr memory area
	size_t	currentSize;	// current size
	

	// Debug Info:
	const char	*func;
	const char	*file;
	int line;
	void *callee;

	DWORD magic;
} RC_DBG_MEMBLOCK;

static RC_DBG_MEMBLOCK *g_blockList = NULL;

//static RBDB_DBT *g_memblockDB = NULL;


void *_roalloc(size_t sz, const char *file, unsigned int line, const char *func, void *callee, bool zeroed){
	size_t szBlock;
	void *block;

	if(g_initialized != true){
		FatalError("%s:%u @ %s -> call to alloc() before subsystem has been initialized!\n", file, line, func);
		return NULL;
	}


	//
	// Invlaid call?
	//
	if(sz == 0){	
		showmsg->showDebug("%s:%u @ %s -> call to alloc() for 0 bytes!",file,line,func);
		FatalError("%s:%u @ %s -> call to alloc() for 0 bytes!",file,line,func);
	}

	// Minimum: 128byte.
	if(sz < ROALLOC_MINBLOCKSIZE)
		sz = ROALLOC_MINBLOCKSIZE;


	//
	// Memory Layout of a block:
	// [RC_DBG_MEMBLOCK] [user memory] [g_SIGNATURE].16b
	//
	szBlock = sizeof(RC_DBG_MEMBLOCK) + sz + 16; 

	// protect heap allocation as well as rbdb.
	g_lock->lock();


	//
	// Try to allocate memory:
	//
	size_t i = 0;
	while(1){
		
		block = mbackend_allocAligned( szBlock );
		if(block != NULL)
			break;

		if(++i == 20){
			g_lock->unlock();

			showmsg->showDebug("%s:%u @ %s -> Out of Memory - %u Bytes (User Size: %u)\n", file, line, func, szBlock, sz);
			FatalError("%s:%u @ %s -> Out of Memory - %u Bytes (User Size: %u)\n", file, line, func, szBlock, sz);
			return NULL; // fatal error never returns.
		}

		Sleep(16);
	}
	
	//
	// got memory:
	//
	RC_DBG_MEMBLOCK *dbg = (RC_DBG_MEMBLOCK*)block;
	dbg->user_ptr	=  (void*) ( ((char*)block) + sizeof(RC_DBG_MEMBLOCK) );
	
	if(zeroed)
		dbg->last_action = RC_DBG_MEMBLOCK::CALLOC;
	else
		dbg->last_action = RC_DBG_MEMBLOCK::ALLOC;

	dbg->currentSize = sz;
	
	dbg->func = func;
	dbg->file = file;
	dbg->line = line;
	dbg->callee = callee;
	dbg->magic = 0xDEADBEEF;


	// 
	// Set SIGNATURE after user block
	//
	memcpy( ((char*)dbg->user_ptr) + sz, g_DBGSIGNATURE,  16);

	//
	// Insert @ blocklist
	//
	dbg->prev = NULL;
	dbg->next = g_blockList;
	
	if(dbg->next != NULL)
		dbg->next->prev = dbg;
	
	g_blockList = dbg;
	//


	g_lock->unlock();

	// For CALLOC implementation
	if(zeroed == true){
		memset(dbg->user_ptr, 0x00, dbg->currentSize);
	}

	return dbg->user_ptr;
}//end: _roalloc()



//
// Ansi C90 realloc beahvoir:
//	oldptr = NULL,  newsz = given -> malloc()
//	oldptr = given,	newsz = 0 -> free()
//	
//
void *_rorealloc(void *oldptr, size_t newsz, const char *file, unsigned int line, const char *func, void *callee, bool zeroed){

	if(g_initialized != true){
		FatalError("%s:%u @ %s -> call to realloc() before subsystem has been initialized!\n", file, line, func);
		return NULL;
	}


	if(oldptr == NULL){
	
		if(newsz == 0){
			// invalid call to realloc()
			showmsg->showDebug("%s:%u @ %s -> invalid call, oldptr = NULL newsz = 0\n", file,line,func);
			FatalError("%s:%u @ %s -> invalid call, oldptr = NULL newsz = 0\n", file,line,func);
			return NULL;//notreach.

		}else{
			// call to realloc to allocate memory!
			return _roalloc(newsz, file, line, func, callee, zeroed);

		}

	}else{
		//
		// oldptr is given
		//

		if(newsz == 0){
			// 
			// request for free!
			//
			_rofree(oldptr, file, line, func);
			return NULL; // C90.

		}else{
			//
			// Request for resize!
			//
			
			// get real block addr
			void *block = ( ((char*)oldptr) - sizeof(RC_DBG_MEMBLOCK) );
			
			
			// get dbg
			RC_DBG_MEMBLOCK *dbg = (RC_DBG_MEMBLOCK*)block;

			// validate magic
			if(dbg->magic != 0xDEADBEEF){
				// 
				//
				//
				if(dbg->magic == 0xDEADDEAD){
					// already freed!
					showmsg->showDebug("%s:%u @ %s -> REALLOC (oldptr: %p) invalid address, magic validation failed! BLOCK IS ALREADY FREED()!! (Requested new size: %u)\n", file,line,func, oldptr, newsz);
					FatalError("%s:%u @ %s -> REALLOC (oldptr: %p) invalid address, magic validation failed! BLOCK IS ALREADY FREED()!! (Requested new size: %u)\n", file,line,func, oldptr, newsz);

				}else{
					// unknown / corrupted 
					showmsg->showDebug("%s:%u @ %s -> REALLOC (oldptr: %p) invalid address, magic validation failed! (Requested new size: %u)\n", file,line,func, oldptr, newsz);
					FatalError("%s:%u @ %s -> REALLOC (oldptr: %p) invalid address, magic validation failed! (Requested new size: %u)\n", file,line,func, oldptr, newsz);

				}
				return NULL;
			}
		
			// validate signature
			if( memcmp( ((char*)dbg->user_ptr) + dbg->currentSize,  g_DBGSIGNATURE, 16) != 0 ){
				showmsg->showDebug("%s:%u @ %s -> REALLOC (oldptr: %p) Signature check failed, OVERFLOW! (Requested new size: %u)\n", file,line,func, oldptr, newsz);
				FatalError("%s:%u @ %s -> REALLOC (oldptr: %p) Signature check failed, OVERFLOW! (Requested new size: %u)\n", file,line,func, oldptr, newsz);
				return NULL;
			}

			// Ignore shrink requests (as ansi C does, too)
			if( newsz <= dbg->currentSize )
				return oldptr; // return old address. (application thinks -> OK)
			
			// 
			// Increase size.
			//
			size_t szNewBlock = sizeof(RC_DBG_MEMBLOCK) + newsz + 16;
			void *newblock;

			// Set the DBG magic to deleted as the realloc call may free the origin address 
			dbg->magic = 0xDEADDEAD;

			g_lock->lock();


			//
			// Unlink frmo blocklist
			//
			if(dbg->prev == NULL)
				g_blockList = dbg->next;
			else
				dbg->prev->next = dbg->next;

			if(dbg->next != NULL)
				dbg->next->prev = dbg->prev;
			//
			dbg->next = dbg->prev = NULL;


			size_t i = 0;
			while(1){

				newblock = mbackend_reallocAligned(block, szNewBlock);
				if(newblock != NULL)
					break;

				if(++i == 20){
					g_lock->unlock();
					showmsg->showDebug("%s:%u @ %s -> REALLOC [%u => %u Bytes] OUT OF MEMORY, (oldptr: %p)\n", file,line,func, dbg->currentSize, newsz, oldptr);
					FatalError("%s:%u @ %s -> REALLOC [%u => %u Bytes] OUT OF MEMORY, (oldptr: %p)\n", file,line,func, dbg->currentSize, newsz, oldptr);
					return NULL;
				}

				Sleep(16);
			}


			// Check if CRT could increase the block (so we dont have to remove / insert @ blockDB)
			if(newblock != oldptr){
				// we have to adjust our local dbg ptr to newblock base addr.
				// also update user_ptr member to new base addr
				dbg = (RC_DBG_MEMBLOCK*)newblock;
				dbg->user_ptr = ( ((char*)newblock) + sizeof(RC_DBG_MEMBLOCK) );
			}


			// zero the new range if requested
			if(zeroed == true && newsz > dbg->currentSize){
				memset(((char*)dbg->user_ptr) + dbg->currentSize, 0x00, (newsz - dbg->currentSize));
			}

			// 
			// Update DBG info!
			//
			dbg->currentSize	= newsz;
			if(zeroed)
				dbg->last_action	= RC_DBG_MEMBLOCK::REALLOCZ;
			else
				dbg->last_action	= RC_DBG_MEMBLOCK::REALLOC;
			dbg->magic			= 0xDEADBEEF;

			dbg->func = func;
			dbg->file = file;
			dbg->line = line;
			dbg->callee = callee;

			//
			// Add to blockList
			//
			//dbg->prev = NULL; // already NULL @ remove.
			dbg->next = g_blockList;
			if(dbg->next != NULL)
				dbg->next->prev = dbg;

			g_blockList = dbg;



			// add signature @ end of user block.
			memcpy( ((char*)dbg->user_ptr) + newsz, g_DBGSIGNATURE,  16);
			
			//
			g_lock->unlock();


			//
			return dbg->user_ptr;

		}//endif: newsz > 0? (realloc to free)
	}//endif: oldptr given? (realloc to alloc)

	// this is unreachable.
	return NULL;
}//end: _rorealloc()


void _rofree(void *ptr, const char *file, unsigned int line, const char *func){
	RC_DBG_MEMBLOCK *dbg;
	void *block;

	if(g_initialized != true){
		FatalError("%s:%u @ %s -> call to free() before subsystem has been initialized!\n", file, line, func);
		return;
	}

	if(ptr == NULL){
		showmsg->showDebug("%s:%u @ %s -> tried to free NULL \n", file, line, func);
		FatalError("%s:%u @ %s -> tried to free NULL  \n", file, line, func);
		return;
	}

	// get block from userptr
	block = (void*) ( ((char*)ptr) - sizeof(RC_DBG_MEMBLOCK) );
	dbg = (RC_DBG_MEMBLOCK*)block;

	// Check magic
	if(dbg->magic != 0xDEADBEEF){
		if(dbg->magic == 0xDEADDEAD){
			// double free??
			showmsg->showDebug("%s:%u @ %s -> FREE, tried to free already freed block! (magic validation failed) UserPTR: %p \n", file,line,func,ptr);
			FatalError("%s:%u @ %s -> FREE, tried to free already freed block! (magic validation failed) UserPTR: %p \n", file,line,func,ptr);
			return;
		
		}else{
			// 
			showmsg->showDebug("%s:%u @ %s -> FREE, unknown or corrupted block! - (magic validation failed) UserPTR: %p \n", file,line,func,ptr);
			FatalError("%s:%u @ %s -> FREE, unknown or corrupted block! - (magic validation failed) UserPTR: %p \n", file,line,func,ptr);
			return;

		}
		
	}


	// Check Signature (overflow protection)
	if( memcmp( ((char*)dbg->user_ptr) + dbg->currentSize,  g_DBGSIGNATURE, 16) != 0 ){
		showmsg->showDebug("%s:%u @ %s -> FREE (ptr: %p) Signature check failed, OVERFLOW!\n", file,line,func, ptr);
		FatalError("%s:%u @ %s -> FREE (ptr: %p) Signature check failed, OVERFLOW!\n", file,line,func, ptr);
		return;
	}

	//
	// Block seems to be fully correct, free .. 
	//

	// set new magic to prevent memory operations on that block after free 
	dbg->magic = 0xDEADDEAD;

	g_lock->lock();

	//
	// Unlink from blocklist.
	//
	if(dbg->prev == NULL){
		g_blockList = dbg->next;
	}else{
		dbg->prev->next = dbg->next;
	}

	if(dbg->next != NULL)
		dbg->next->prev = dbg->prev;
	//
	dbg->next = dbg->prev = NULL;


	//
	// free:
	//
	mbackend_freeAligned(block);


	g_lock->unlock();

}//end: _rofree()



char *_rostrdup(const char *pStr, const char *file, unsigned int line, const char *func, void *callee){

	if(g_initialized != true){
		FatalError("%s:%u @ %s -> call to rostrdup() before subsystem has been initialized!\n", file, line, func);
		return NULL;
	}


	// the lazy way ..
	size_t len = strlen(pStr) + 1;
	void *user_ptr = _roalloc( len, file, line, func, callee, false );

	memcpy(user_ptr, pStr, len);

	// Update DBG info to set action to strdup..
	RC_DBG_MEMBLOCK *dbg =  (RC_DBG_MEMBLOCK*) ( ((char*)user_ptr) - sizeof(RC_DBG_MEMBLOCK) );
	dbg->last_action = RC_DBG_MEMBLOCK::STRDUP;


	return (char*)user_ptr;
}//end: _rostrdup()


void roalloc_init(){

	// 
	g_lock = new rgCore::util::mutex;

	// Create Signature
	for(size_t i = 0; i < 16; i++){
		g_DBGSIGNATURE[i] = (unsigned char) ((i * 13) - 7);
	}


	mbackend_init();

	g_blockList = NULL;
	g_initialized = true;

}//end: roalloc_init()


static void report_memleak(FILE *fp, RC_DBG_MEMBLOCK *block){
	char *lastActionStr;

	switch(block->last_action){
	case RC_DBG_MEMBLOCK::ALLOC:
		lastActionStr = "alloc";
		break;

	case RC_DBG_MEMBLOCK::REALLOC:
		lastActionStr = "realloc";
		break;

	case RC_DBG_MEMBLOCK::STRDUP:
		lastActionStr = "strdup";
	break;

	case RC_DBG_MEMBLOCK::CALLOC:
		lastActionStr = "calloc";
	break;

	case RC_DBG_MEMBLOCK::REALLOCZ:
		lastActionStr = "reallocz";
	break;

	default:
		lastActionStr = "unknown";
		break;
	}

	// Strip Path from filename, so only the filename gets printed.
	const char *fileNiceName, *p;
	fileNiceName = block->file;
	p = block->file;
	while(1){
		register char c = *p++;

		if(c == '\0') break;
		if(c == '/' || c == '\\') fileNiceName = p;
	}
	/*
	struct {
		char func[64];
		char file[64];
		int line;
	}  callerInfo;

	if(block->callee != NULL) {
		debug_addr2caller(block->callee, callerInfo.file, sizeof(callerInfo.file), &callerInfo.line, callerInfo.func, sizeof(callerInfo.func));
		block->func = callerInfo.func;
		block->line = callerInfo.line;
		block->file = callerInfo.file;
	}
	*/
	showmsg->showDebug("Memory Leak [%p]:  %zu Bytes, Allocated thru %s in Proc '%s' -> %s:%u\n", block->user_ptr, block->currentSize, lastActionStr, block->func, fileNiceName, block->line);
	
	if(block->last_action == RC_DBG_MEMBLOCK::STRDUP)
		showmsg->showDebug("Memory Leak [%p]:  String Contents '%s'\n", block->user_ptr, block->user_ptr);


	if(fp != NULL){
		fprintf(fp, "Memory Leak [%p]:  %zu Bytes, Allocated thru %s in Proc '%s' -> %s:%u\n", block->user_ptr, block->currentSize, lastActionStr, block->func, fileNiceName, block->line);
		if(block->last_action == RC_DBG_MEMBLOCK::STRDUP)
			fprintf(fp, "Memory Leak [%p]:  String Contents '%s'\n", block->user_ptr, (char*) block->user_ptr);

	}

}//end: report_memleak()


void roalloc_final(){
	if(g_initialized != true)
		return;
	
	// 
	g_initialized = false;

	//
	// Loop over blocklist (leaks)
	//
	if(g_blockList != NULL){ //memleaks :(
		FILE *fp;
		
		fp = fopen("memleak.log", "w");
		
		size_t leakBytes = 0;
		size_t numLeaks = 0;

		RC_DBG_MEMBLOCK *iter, *iter_next;
		iter = g_blockList;
		while(1){
			if(iter == NULL)
				break;

			iter_next = iter->next;

			//
			report_memleak(fp, iter);
			//

			numLeaks++;
			leakBytes += iter->currentSize;

			// free block:
			mbackend_freeAligned(iter);
			//
			iter = iter_next;
		}//


		showmsg->showDebug("Memory Leak Statistics:\n");
		showmsg->showDebug(" Leaks Total: %zu\n", numLeaks);
		showmsg->showDebug(" Leaked Bytes: %zu\n", leakBytes);

		//
		if(fp != NULL){
			fprintf(fp, "\n");
			fprintf(fp,"Memory Leak Statistics:\n");
			fprintf(fp," Leaks Total: %zu\n", numLeaks);
			fprintf(fp," Leaked Bytes: %zu\n", leakBytes);
			
			fclose(fp);
		}
	}//endif memleaks?

	delete g_lock;
	g_lock = NULL;

	mbackend_final();

}//end: roalloc_final()

#else
//
// WIN32 - NON Tracing Allocator
//

void *_roalloc(size_t sz, bool zeroed){
	register size_t i;
	void *ptr;

	if(sz < ROALLOC_MINBLOCKSIZE)
		sz = ROALLOC_MINBLOCKSIZE;

	i = 0;
	while(1){
		ptr = mbackend_allocAligned(sz);
		
		if(ptr == NULL){
			// no memory by os.
			if(++i == 20){
				//putMemMgr("OOM: %u Bytes (alloc)\n", sz);
				
				ShowFatalError("Out of Memory - failed to allocate %u Bytes\n", sz);
				exit(1); //
				return NULL;
			}
			Sleep(16);

		}else{
			// got memory:
			if(zeroed == true) {
				memset(ptr, 0x00, sz);
			}

			return ptr;
		}
	}

	// NeaverReach:
	return NULL;
}//end: _roalloc()



void *_rorealloc(void *ptr, size_t newSize, bool zeroed){
	size_t i;
	void *ret;

	if(ptr == NULL){

		if(newSize < ROALLOC_MINBLOCKSIZE)
			newSize = ROALLOC_MINBLOCKSIZE;

		ret = mbackend_allocAligned(newSize);
		
		if(zeroed == true){
			memset(ret, 0x00, newSize);
		}

	}else{
		if(newSize == 0){
			// Act like ANSI (c90)
			// enforce this beahvoir :)
			mbackend_freeAligned(ptr);
			return NULL;

		}else if(newSize < ROALLOC_MINBLOCKSIZE){
			// As we hae at least 64bytes allocated on malloc call,  
			// we can simply return the given ptr!
			return ptr;

		}

	}
	
	i = 0;
	while(1){

		size_t oldSize = mbackend_msize(ptr);
		if(newSize <= oldSize)	// ignore shrink requests!
			return ptr;

		ret = mbackend_reallocAligned( ptr, newSize );
		if(ret == NULL){
			if(++i == 20){
				//putMemMgr("OOM: %u bytes (realloc)\n", newSize);

				ShowFatalError("Out of Memory - cannot allocate %u Bytes\n", newSize);
				exit(1);

				return NULL;
			}
			Sleep(16);
		}else{
			//got memory:

			if(zeroed == true && newSize > oldSize){
				memset(((char*)ret) + oldSize, 0x00, (newSize - oldSize));
				
			}

			return ret;

		}
	}

	// neverReach:
	return NULL;
}//end: _rorealloc()


 void _rofree(void *ptr){
	mbackend_freeAligned(ptr);
 }//end: _rofree()


 char *_rostrdup(const char *pStr){
	 size_t len = strlen(pStr) + 1;
	 char *ptr = (char*)_roalloc(len, false);

	 memcpy(ptr, pStr, len);

	 return ptr;
 }//end: _rostrdup()


 ///
 void roalloc_init(){

	 mbackend_init();

 }//end: roalloc_init()

 void roalloc_final(){

	 mbackend_final();

 }//end: roalloc_final()


#endif //wendif release/ebug



 //
 // RBDB Wrappers
 //
 void *roalloc_rbdb(size_t sz) {
	 return roalloc(sz);
 }
 void rofree_rbdb(void *memblock) {
	 rofree(memblock);
 }

