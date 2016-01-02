#pragma once
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

#if defined(ARCH_AMD64)
typedef unsigned __int64 RBDB_KEY;
typedef unsigned __int64 RBDB_HOSTINT;
#define RBDB_KEY_SIGNSWAP 0x8000000000000000	
#elif defined(ARCH_X86)
typedef unsigned int RBDB_KEY;
typedef unsigned int RBDB_HOSTINT;
#define RBDB_KEY_SIGNSWAP 0x80000000
#else
#error - Unsupported Archtecture! -
#endif

//

#define RBDB_LOCK_HANDLE rgCore::util::rwlock*
#define RBDB_LOCK_INITF(mtx) mtx = new rgCore::util::rwlock()
#define RBDB_LOCK_DESTROY(mtx) delete mtx
#define RBDB_LOCK_LOCKREAD(mtx) mtx->lockRead()
#define RBDB_LOCK_UNLOCKREAD(mtx) mtx->unlockRead()
#define RBDB_LOCK_LOCKF(mtx) mtx->lockWrite()
#define RBDB_LOCK_UNLOCKF(mtx) mtx->unlockWrite()




namespace rgCore { namespace rbdb {
///

// orig: 256 + 27
#define RBDB_HTSZ 3840 + 405


typedef enum RBDB_DBN_COLOR {
	DBN_RED = 0,
	DBN_BLACK = 1
} RBDB_DBN_COLOR;
///
/// Internal Structures 
///

typedef struct RBDB_DBN { // Tree Node
	struct RBDB_DBN		*parent, *left, *right;
	RBDB_DBN_COLOR		color;
	bool					deleted;
	RBDB_KEY				key;
	void 				*data;
} RBDB_DBN;


typedef struct RBDB_DBF {	// Free Node (used for recusrive tree modificaiton while tree is locked)
	struct RBDB_DBF 	*next;

	RBDB_DBN		*z;
	RBDB_DBN		**root;
} RBDB_DBF;



/**
*/
typedef void(*rbdb_fatalerrorproc)(const char *msg, ...);


/** 
 * Base of all DB's (rbdb, strdb, etc)
 * This class is not intendend to be used directly
 *
 * see all derived implementations for more details
 */
class rbdbBase {
public:
	/**
	* creates a new Red Black Tree,
	*
	* @param name			name of the pool
	* @param threadSafe		should the pool be thread safe?
	* @param nodeCount		Initial Allocation of nodes, if set to 0 - uses global node pool
	* @param nodeAllocStep	Node Pool reallocation count
	*
	*/
	rbdbBase(const char *name, bool threadSafe = true, int nodeCount = 0, int nodeAllocStep = 0);
	virtual ~rbdbBase();

	void setOnFatalErrorHandler(rbdb_fatalerrorproc proc);


protected:
	RBDB_DBN		*m_ht[RBDB_HTSZ];
	RBDB_DBF		*m_freeList;

	Pool<struct RBDB_DBN> *m_nodePool;	// Note: if same as m_globalNodePool - dont cleanup in destructor! 
	
	atomic::atomic_t m_nNodes; 
	atomic::atomic_t m_freeLockCount;

	bool m_bIsThreadsafe;

	RBDB_LOCK_HANDLE m_lock;
	rbdb_fatalerrorproc m_fatalerror;

	char *m_name;


public:
	static void globalInit(); // Initialize Pool Implementation (called by platformlib)
	static void globalFinal(); // Finalze Pool Implementation (called by platformlib)

protected:
	static Pool<struct RBDB_DBN> *m_globalNodePool;
	static Pool<struct RBDB_DBF> *m_globalFreePool;

protected:
	// Internal Tree Methods
	// Note they're not thread-safe! 
	// you'll have to handle locking.
	//
	enum DB_INSERT_FLAG {
		DB_INSERT_FLAG_NONE = 0,
		DB_INSERT_FLAG_OVERWRITE = 1
	};

	void *db_insert(RBDB_KEY key, void *data, unsigned int flags);
	void *db_delete(RBDB_KEY key);
	void *db_fetch(RBDB_KEY key);
	void db_free_lock();
	void db_free_unlock();
	static void db_rebalance_erase(RBDB_DBN *z, RBDB_DBN **root);
	static void db_rebalance(RBDB_DBN *p, RBDB_DBN **root);
	static void db_rotate_left(RBDB_DBN *p, RBDB_DBN **root);
	static void db_rotate_right(RBDB_DBN *p, RBDB_DBN **root);

public:
	// Called when a tree-node gets 'forced'-destroyed (currently only during destruction with unclean tree)
	virtual void onForcedDestroyNode(RBDB_DBN *p) = 0;

};


} }
