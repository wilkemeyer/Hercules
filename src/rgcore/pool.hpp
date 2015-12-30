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


#include "rgcore.h"

#define ALIGN_TO(x, a) (x + ( a - ( x % a) ) )
#define ALIGN_TO_8(x) ALIGN_TO(x, 16)

#define PI_NODE_MAGIC 0xDEAD04347001 // DEAD MEMPOOL

// Note:
// memory allocation will be READWRITE,
// if large pages -> RW/EXEC, as on some OS's this is required.
//

// Avaliable Pool Settings:
//  ASSERT_POOL   -> Check pool enry upon return for validness (default on _DEBUG build)
//	POOL_PUT_MEMZERO   -> Zero memory when returning element to pool

// this may crap some pools.
// (It is not recommended to use this option, instead use a pool with GC, 
//	and implement the node-specific cleanups in onGc proc ..)
#undef POOL_PUT_MEMZERO


#ifdef _DEBUG
#define ASSERT_POOL
#endif

using namespace rgCore;

template <class T>
class Pool : public rgCore::roAlloc {

public:
	typedef void(*cbInitializeNode)(T *node);							///
	typedef void(*cbFinalizeNode)(T *node);								//  note that any of theese three callbacks can ocure parallel!
	typedef void(*cbBeforeSegmentAllocation)(size_t newTotalNum);		///
	typedef void(*cbOnGC)(T *node);										// Called when Garbage Collection will queue the node for re-use

	////
	//// CONSTRUCTOR
	//// 
	__forceinline Pool(const char *Name,			// Pool name
					   size_t allocStep,			// Number of Elemets allocate per step in oom situation
					   size_t initialAllocNum,		// Number of Initial Allocated elements.
					   bool largePages = false,		// Use Large Pages?
					   cbInitializeNode cbInitialize = NULL,	// Initialization callback called once upon node allocation
					   cbFinalizeNode cbFinalize = NULL,		// Finialization callback, called when pool gets finalized for each node 
					   cbBeforeSegmentAllocation cbBeforeSegmentAlloc = NULL
					   ) {
		size_t szElem = sizeof(T);

		if(allocStep < 32)
			allocStep = 32;
		if(Name == NULL || *Name == '\0')
			Name = "Unnamed";


		// Correct the allocStep by considerign the pageSize and the sgement Header Size
		{
			size_t szSegmentHeader	= ALIGN_TO_8(sizeof(Pool<T>::Segment));
			size_t szNodeHeader		= ALIGN_TO_8(sizeof(Pool<T>::Node));
			szElem					= ALIGN_TO_8(szElem); // in param.
			size_t szPage			= (largePages == true)?GetLargePageMinimum():getpagesize();	

			size_t	szSegment = szSegmentHeader + (allocStep * (szElem + szNodeHeader));
			size_t  szUnused = szPage - (szSegment % szPage);	// unused space when aligned to space,

			// try to find out how many elems would fit in this unused space 
			// and add this number to the allocStep
			// this gives us a 100% guarantee that the segment will perfectly fill out the pages
			allocStep += (szUnused / (szNodeHeader + szElem));


		}

		// Initial steps :)
		if(initialAllocNum == 0)
			initialAllocNum = allocStep;

		size_t initNumSteps = (initialAllocNum / allocStep);
		if((initialAllocNum % allocStep) != 0)
			initNumSteps++;

		//
		// Assign values to members
		//
		m_name = strdup(Name);

		m_bLargePages	= largePages;
		m_bGarbageCollection = false;

		m_numFree		= 0;
		m_numTotal		= 0;
		m_numPeak		= 0;

		m_szElem		= szElem;
		m_allocStep		= allocStep;
		m_gcTimerID		= -1;

		atomic::Exchange(&m_gcRunningLock, 0);

		
		atomic::SListInitializeHead(&m_segmentList);
		atomic::SListInitializeHead(&m_nodeFreeList);

		m_cbNodeInit = cbInitialize;
		m_cbNodeFinal = cbFinalize;
		m_cbBeforeSegmentAlloc = cbBeforeSegmentAlloc;
		m_cbOnGarbageCollect = NULL;

		// 
		for(size_t i = 0; i < initNumSteps; i++)
			segmentAllocate();

	}//end: Pool()




	///
	/// DESTRUCTOR
	///
	__forceinline ~Pool() {

		if(m_numFree < m_numTotal) {
			ShowError("~Pool<%s>:  Pool is not Empty!\n", m_name);
		}


		if(m_gcTimerID != -1) {
			timer->_delete_nocheck(m_gcTimerID);

			// Ensure that no GC is running during finalzation:
			while(1) {
				if(atomic::CompareAndSwap(&m_gcRunningLock, 0, 0) == 0)
					break;
				SwitchToThread();
			}

			m_gcTimerID = -1; // blockz::Time::ITimerSystem::TIMER_INVALID_ID;
		}


		while(1) {
			Pool<T>::Segment *seg;
			seg = (Pool<T>::Segment*)atomic::SListPop(&m_segmentList);
			if(seg == NULL)
				break;

			if(m_cbNodeFinal != NULL) {
				char *p = (char*)seg->basePtr;
				p += ALIGN_TO_8(sizeof(Pool<T>::Segment));

				// nodes follow
				for(size_t i = 0; i < m_allocStep; i++) {
					Pool<T>::Node *node = (Pool<T>::Node*)p;
					p += (ALIGN_TO_8(sizeof(Pool<T>::Node)) + m_szElem);

					m_cbNodeFinal((T*)node->ptr);

				}

			}//finalize all nodes.


			VirtualFree(seg->basePtr, seg->size, MEM_RELEASE);
		}


		free((void*)m_name);
	}//end: ~Pool()


	///
	/// Set Pool GC Mode 
	/// (Enables GarbageCollection Mode)
	/// @param timeWaitForReuse - time in MS (Ticks)
	///
	void setupGC(size_t timeWaitForReuse, cbOnGC cbOnGarbageCollect) {

		if(timeWaitForReuse < 4000)
			timeWaitForReuse = 4000;

		// 
		atomic::SListInitializeHead(&m_nodeWaitForGCList);

		m_bGarbageCollection = true;
		m_gcTimeout = timeWaitForReuse;
		m_cbOnGarbageCollect = cbOnGarbageCollect;
		m_gcTimerID = timer->add_interval(timer->gettick_nocache()+timeWaitForReuse, Pool<T>::gcProcProxy, 0, this, timeWaitForReuse);

	}//end: setupGC()




public:
	////
	//// Get an element
	////
	__forceinline T *get() {
		Pool<T>::Node *node;

		while(1) {
			node = (Pool<T>::Node*)atomic::SListPop(&m_nodeFreeList);
			if(node != NULL)
				break;

			SwitchToThread(); // switch to another thread, 
		}

		if(atomic::Dec(&m_numFree) == 0) {
			// :( pool empty, we have to realloc!

			segmentAllocate();

		}

		//  peak may be not 100%  accurate.
		int numUse = (m_numTotal - m_numFree);
		if(numUse > m_numPeak)
			atomic::Exchange(&m_numPeak, numUse);

		//putLog("Get %s\n",m_name);

		return (T*)node->ptr;
	}//end: get()



	__forceinline void put(T *ptr) {
		size_t szNode = ALIGN_TO_8(sizeof(Pool<T>::Node));
		Pool<T>::Node *node;

		// Get the node elem
		node = (Pool<T>::Node*)(((char*)ptr) - szNode);

#ifdef ASSERT_POOL
		if(node->pool != this || node->magic != PI_NODE_MAGIC) {
			ShowError("Pool<%s>::put(): ASSERT %s fail\n", m_name, (node->pool != this)?"pool":"magic");
			//
			if(node->magic == PI_NODE_MAGIC)
				ShowError("this %s != node %s\n", this->m_name, node->pool->m_name);
			return;
		}
#endif

		//putLog("PUT %s\n",m_name);
#ifdef POOL_PUT_MEMZERO
		memset(node->ptr, 0x00, ALIGN_TO_8(sizeof(T)));
#endif


		if(m_bGarbageCollection == true) {
			// GC Enabled, push to gc list
			node->gcReuseTick = timer->gettick_nocache() + m_gcTimeout;

			atomic::SListPush(&m_nodeWaitForGCList, node);

		} else {
			// no GC, direct reuse allowed, push to nodeFreeList
			atomic::SListPush(&m_nodeFreeList, node);

			// Update stats counter
			atomic::Inc(&m_numFree);
		}




	}//end: ptr;



	////
	////  Get element size, should be only used for dfebug stuff
	////
	__forceinline size_t get_szElem() {
		return m_szElem;
	}//end: m_szElem


	///
	///  Get Counters (stats)
	///
	void getCounters(size_t *numTotal, size_t *numFree, size_t *numPeak) {
		*numTotal	= m_numTotal;
		*numFree	= m_numFree;
		*numPeak	= m_numPeak;
	}//end: getCounters()


	///
	/// SEGMENT ALLOCATOR
	///
private:
	__forceinline void segmentAllocate() {

		// Memory Layout:
		// - node:
		//		[node_header] [node_data]
		//  
		// segment itself:
		//		[segment_header] [m_allocStep * node] 
		// 

		size_t	szSegmentHeader = ALIGN_TO_8(sizeof(Pool<T>::Segment)),
			szNodeHeader	= ALIGN_TO_8(sizeof(Pool<T>::Node));

		//
		size_t	szAllocBytes	= szSegmentHeader + (m_allocStep * (szNodeHeader + m_szElem));
		DWORD	dwAllocFlags	= (MEM_COMMIT | MEM_RESERVE);
		//size_t	szPage			= (m_bLargePages == true)?GetLargePageMinimum():4096;
		size_t	szPage			= getpagesize();

		// Allocate always WHOLE pages
		szAllocBytes = ALIGN_TO(szAllocBytes, szPage);



		if(m_bLargePages == true)
			dwAllocFlags |= MEM_LARGE_PAGES;


		//
		char *basePtr;

		// 
		size_t cnt = 0, cnt1450 = 0;
		while(1) {
			cnt++;

			basePtr = (char*)VirtualAlloc(NULL, szAllocBytes, dwAllocFlags, ((dwAllocFlags & MEM_LARGE_PAGES)?PAGE_EXECUTE_READWRITE:PAGE_READWRITE));
			if(basePtr != NULL)
				break;

			DWORD err = GetLastError();
			if(err == ERROR_NO_SYSTEM_RESOURCES && ++cnt1450 < 32) {
				SwitchToThread();// yield. in order to allow the system to grant ressources..s
				continue;
			}
			

			ShowFatalError("Pool<%s>::segmentAllocate(): VirtualAlloc failed! (Errcode: %u) - Bytes Total: %u\n", m_name, err, szAllocBytes);
	

			if(err == ERROR_NOT_ENOUGH_MEMORY) {
				//
				// We'll give it a try .. 
				//
				if(cnt < 16) {
					SwitchToThread();
					continue;
				} else {
					char msg[512];
					//debug_logProcessMemoryStats(); // dump process memory stats to log.
					sprintf(msg, "Out Of Memory\n\nTried to allocate %zu Bytes for Pool<%s>\n", szAllocBytes, m_name);
					fatalError(msg);
				}
			}

			// 16 Times errored -> Abort Process, End-Users won't like Freezed Processes which're eating up ALL system ressources.
			if(cnt > 16) {
				char msg[512];
				//debug_logProcessMemoryStats(); // dump process memory stats to log.
				sprintf(msg, "Where is the Guru?!\n\nSegment Allocation Failed\nPool<%s>\nError: %u\nBytes: %zu\n", m_name, err, szAllocBytes);
				fatalError(msg);
			}


			if(err == ERROR_PRIVILEGE_NOT_HELD) {
				//  No Permission to 'Lock pages in memory',  to allow: Open Security Policy manager => Local Policies => User Rights Assignment => Lock pages in Memory, add the user.
				ShowError("Pool<%s>::segmentAllocate(): Allocation of LargePages failed, this happens when the Current user has no Permission to LOCK Pages in Memory (to allow: Open Security Policy manager => Local Policies => User Rights Assignment => Lock pages in Memory, add the user)\n", m_name);
				dwAllocFlags &= ~MEM_LARGE_PAGES;
				continue;
			}


			if(err == ERROR_NO_SYSTEM_RESOURCES) {
				ShowError("Pool<%s>::segmentAllocate(): Allocation of large Pages failed, system reported out of ressources, allocating normal pages, please reboot the system!\n", m_name);
				dwAllocFlags &= ~MEM_LARGE_PAGES;
				continue;
			}



	
			Sleep(32);
		}


		//
		// If the pool has set callback for 'before segment allocate'
		// its now time to fire it, as this is the point before items 're get added...
		//
		if(m_cbBeforeSegmentAlloc != NULL)
			m_cbBeforeSegmentAlloc(m_numTotal + m_allocStep);

		//
		// Zero the whole segment before we're going to use it.
		//
		memset(basePtr, 0x00, szAllocBytes);

		//
		// Got the memory :)
		//
		Pool<T>::Segment *segment = (Pool<T>::Segment*)basePtr;
		segment->basePtr = basePtr;
		segment->size = szAllocBytes;
		atomic::SListPush(&m_segmentList, segment); // add to segment list

		// 
		basePtr += szSegmentHeader;

		// Create Element's
		for(size_t i = 0; i < m_allocStep; i++) {
			Pool<T>::Node *node = (Pool<T>::Node*)basePtr;
			basePtr += szNodeHeader;

			node->ptr = (void*)basePtr;

			basePtr += m_szElem;

#ifdef ASSERT_POOL
			node->pool = this;
			node->magic = PI_NODE_MAGIC;
#endif

			if(m_cbNodeInit != NULL)
				m_cbNodeInit((T*)node->ptr);

			// Add Node:
			atomic::SListPush(&m_nodeFreeList, node);

			// Increment Stat Counters.
			atomic::Inc(&m_numTotal);
			atomic::Inc(&m_numFree);
		}

	}//end: segmentAllocate()



private:
	///
	/// GC Proc
	static int gcProcProxy(int timerID, __int64 tick, int id, intptr_t data) {
		Pool<T> *_this = (Pool*)data;
		return _this->gcProc();
	}//end: gcProcProxy()


	int gcProc() {
		//Pool<T> *_this = (Pool*)data;
		Pool<T>::Node *iter, *skipBegin, *skipEnd;
		size_t nSkipped, curTick, nRecycled;

		// Prevent running concurrently..
		while(1) {
			if(atomic::CompareAndSwap(&this->m_gcRunningLock, 1, 0) == 0)
				break;

			SwitchToThread();

		}//m_gcRunningLock

		//plog_putDbg("---ACHTUNG ACHTUNG \n");

		//
		//putDbg("Pool<%s> - GC iteration!\n", _this->m_name);
		//

		nSkipped = 0;
		nRecycled = 0;
		skipBegin = NULL;
		skipEnd = NULL;

		curTick = timer->gettick_nocache();


		while(1) {
			// 
			// Fetch item from gc Waiting List
			//
			iter = (Pool<T>::Node*)atomic::SListPop(&this->m_nodeWaitForGCList);
			if(iter == NULL)
				break;

			if(iter->gcReuseTick > curTick) {
				//
				// reuseTick is in future, so the elem gets skipped 
				// (dont reuse - yet -)
				//
				iter->SLNext = NULL;

				if(nSkipped++ == 0) {
					skipBegin	= iter;
					skipEnd		= iter;
				} else {
					skipEnd->SLNext = iter;
					skipEnd = iter;
				}

			} else {
				//
				// Node can be reused 
				//


				if(this->m_cbOnGarbageCollect != NULL) {
					this->m_cbOnGarbageCollect((T*)iter->ptr);
				}


				//
				// Put node to freeList
				// 
				atomic::SListPush(&this->m_nodeFreeList, iter);

				// Update stats Coutner
				atomic::Inc(&this->m_numFree);

				nRecycled++;
			}

		}//endwhile() 

		// 
		// Skipped nodes = nodes that're not old enough for reuse, must be readded to gc wait list..
		//
		if(nSkipped > 0) {
			atomic::SListPushSList(&this->m_nodeWaitForGCList, skipBegin, skipEnd);
		}

		//
		//putDbg("Pool<%s> GC Done! (%u Skipped, %u Recycled)\n", _this->m_name, nSkipped, nRecycled);
		//


		// Release lock
		atomic::Exchange(&this->m_gcRunningLock, 0);

		return 0;
	}//end: gcProc()



private:
	////
	//// Types
	////
	typedef atomic64_align struct Node: atomic::SLIST_NODE {
		void		*ptr;	// elem ptr

		__int64	gcReuseTick;	// used when GC is enabled, REUSE Timeout. (gettick() >= gcReuseTick == could be reused)

#ifdef ASSERT_POOL
		Pool	*pool;
		__int64		magic;
#endif
	} Node;

	typedef atomic64_align struct Segment: atomic::SLIST_NODE {
		void		*basePtr;
		size_t		size;
	} Segment;


private:
	bool		m_bLargePages;
	bool		m_bGarbageCollection;
	const char *m_name;
	size_t		m_szElem;
	size_t		m_allocStep;
	
	
	int					m_gcTimerID;
	__int64				m_gcTimeout;	// Time before node can be reused when running in gc mode.s

	cbInitializeNode		m_cbNodeInit;
	cbFinalizeNode		m_cbNodeFinal;
	cbBeforeSegmentAllocation m_cbBeforeSegmentAlloc;
	cbOnGC				m_cbOnGarbageCollect;

	atomic::atomic_t		m_gcRunningLock;	// Spinlock / to prevent worst case scenarios during pool finalization..

	atomic::atomic_t		m_numTotal,
						m_numFree,
						m_numPeak;

	atomic::SLIST_HEAD	m_segmentList,
						m_nodeFreeList,
						m_nodeWaitForGCList;


};



/**
template <class T>
class Poolable {
private:
Pool<T> *_poolablePool;

public:
void *operator new(size_t sz){
return _poolablePool->get();
}

void operator delete(void *ptr){
_poolablePool->put(ptr);
}

};
*/
