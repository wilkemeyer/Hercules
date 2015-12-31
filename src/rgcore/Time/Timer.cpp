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
#include "stdafx.h" // VS Bug.
#include "../stdafx.h"

#define TIMER_MAXWORKER 16
#define HEAPSIZE 4096

namespace rgCore { namespace Time {



//
typedef struct TimerListEntry {
	struct TimerListEntry *tlnext, *tlprev;

	const int id;
	bool deleted;

	tick_t execTick;
	tick_t interval;

	timer::TimerProc proc;

	void *paramPtr;
	size_t paramInt;

} TimerListEntry;

static Pool<TimerListEntry>	*g_timerPool;
static TimerListEntry	*heap[HEAPSIZE],
						*listBegin,
						**idMap;


static atomic::atomic_t numTotal = 0;// current highest timer id
static util::spinlock *g_idMapResizeLock = NULL;
static util::mutex *g_lock_list = NULL, *g_lock_worker = NULL;
static util::cond *cond_List = NULL;
static util::thread *hWorkerThread[TIMER_MAXWORKER];
static size_t nWorker = 0;
static bool g_initialized = false;
static bool g_running = true;


static DWORD __stdcall timerWorkerProc(util::thread * /*self*/);


void timer::init(){
	numTotal = 0;
	idMap = NULL;
	g_idMapResizeLock = new util::spinlock();
	g_lock_list = new util::mutex();
	g_lock_worker = new util::mutex();


	size_t	allocStep	= iniGetAppInteger("rgCoreTimer", "Node Allocation Step", 1024),
			initialAlloc= iniGetAppInteger("rgCoreTimer", "Node Initial Allocation", 2048);


	g_timerPool = new Pool<TimerListEntry>("rtCoreTimerListEntry", allocStep, initialAlloc, true, [](TimerListEntry *tle) {
												//
												// This callback gets fired every node thats allocated.
												//
												((int)tle->id) = atomic::Inc(&numTotal) - 1;
												tle->tlnext = NULL;
												tle->tlprev = NULL;
												tle->deleted = true;
											},
											NULL,
											[](size_t newTotalNumber){
												//
												// This callback gets fired when a segment alloc gets fired
												// here's the right time to resize the idMap!
												//
												g_idMapResizeLock->lock();

												if(idMap == NULL){
													idMap = (TimerListEntry**)roalloc( sizeof(TimerListEntry*) * newTotalNumber);
												}else{
													idMap = (TimerListEntry**)rorealloc(idMap, sizeof(TimerListEntry*) * newTotalNumber);
												}

												for(size_t i = numTotal;  i < newTotalNumber;  i++){
													idMap[i] = NULL;
												}
												
												g_idMapResizeLock->unlock();

											}
		);
	
		memset(heap, 0x00, sizeof(TimerListEntry*) * HEAPSIZE);
		
		listBegin = NULL;


		//
		cond_List = new util::cond();


		nWorker = iniGetAppInteger("rgCoreTimer", "Worker Count", 2);
		if(nWorker > TIMER_MAXWORKER)
			nWorker = TIMER_MAXWORKER;
		else if(nWorker == 0)
			nWorker = 1;

		for(size_t i = 0; i < nWorker; i++){
			char nameBuf[64];
			sprintf_s(nameBuf, "rgCoreTimer Worker#%zu", i);

			hWorkerThread[i] = new util::thread(nameBuf, timerWorkerProc);
			if(hWorkerThread[i] == NULL){
				fatalError("Creating Timer Worker Thread Failed\n");
			}
		}

	g_initialized = true;
}//end: timer_init()


void timer::final(){
	if(g_initialized == false)
		return;

	g_running = false; 
	cond_List->broadcast();
	for(size_t i = 0; i < nWorker; i++){
		hWorkerThread[i]->wait();
		delete hWorkerThread[i];
		hWorkerThread[i] = NULL;
	}

	//
	// Free tiemr Pool! && idMap
	delete g_timerPool;
	rofree(idMap);
	idMap = NULL;

	// Delete Condition Var
	delete cond_List; cond_List = NULL;

	//delete locks.
	delete g_lock_list; g_lock_list = NULL;
	delete g_lock_worker; g_lock_worker = NULL;
	delete g_idMapResizeLock; g_idMapResizeLock = NULL;

	g_initialized = false;
}//end: timer_final()


/* Stats of Pool */
/*void timer::getCounters(size_t *numTotal, size_t *numFree, size_t *numPeak){
	if(g_initialized == false){
		*numTotal = 0;
		*numFree = 0;
		*numPeak = 0;
		return;
	}

	g_timerPool->getCounters(numTotal, numFree, numPeak);

}//end: timer_getCounters()
*/

////
//// timerList Methods ( list )
////

static inline void tl_add(TimerListEntry *tle){
	bool replaceHeap = false;
	TimerListEntry *iter;
	int hash = tle->execTick % HEAPSIZE;

	g_lock_list->lock();

	iter = heap[hash];
	if(iter == NULL)
		iter = listBegin;

	// 
	// 
	if(iter == NULL){
		//
		// No timers registered, this is the first one. 
		//
		listBegin = tle;
		tle->tlnext = tle->tlprev = NULL;

		replaceHeap = true;


	}else{
		//
		// Other timer(s) exist, so check the execTick, loop forward or backward 
		// until we can insert this,
		// Special Note:   if there're timers in List with the same ExecTick as the new one,  the new one gets inserted behined.
		//

		if(iter->execTick == tle->execTick){
			// Same Exectick, 
			// loop forward.. until iter->tlnext is later than tle's or next is NULL (no next)
			//
			while(1){
				if(iter->tlnext == NULL || iter->tlnext->execTick > tle->execTick)
					break;

				iter = iter->tlnext;
			}

			// Insert: 
			// [iter] <-> [tle] <-> [iter->tlnext]
			if(iter->tlnext != NULL)	 // [iter->tlnext->tlprev] -> [tle]
				iter->tlnext->tlprev = tle;
			
			tle->tlnext = iter->tlnext;  // [tle->tlnext] -> [iter->tlnext]
			tle->tlprev = iter;			 // [tle->tlprev] -> [iter]
			iter->tlnext = tle;			 // [iter->tlnext] -> [tle]

			replaceHeap = true; // so furhter 'equal' execTick additions dont must iterate forward over tons of timers.
		
		
		}else if(tle->execTick < iter->execTick){
			// if tle's execTick is before iter's execTick, scroll backward till 
			// iter's time is >= tle->execTick
			//
			while(1){
				if(iter->tlprev == NULL || iter->tlprev->execTick <= tle->execTick)
					break;

				iter = iter->tlprev;
			}
		
			// Insert:
			// [iter->tlprev] <-> [tle] <-> [iter]
			if(iter->tlprev != NULL){
				iter->tlprev->tlnext = tle;
			}else{ // listBegin?
				if(iter == listBegin){
					listBegin = tle;
				}
			}

			tle->tlprev = iter->tlprev;
			tle->tlnext = iter;

			iter->tlprev = tle;

		}else if(tle->execTick > iter->execTick){
			// 
			//
			//
			while(1){
				if(iter->tlnext == NULL || iter->tlnext->execTick > tle->execTick)
					break;

				iter = iter->tlnext;
			}
		
			// Insert:
			// [iter] <-> [tle] <-> [iter->tlnext]
			if(iter->tlnext != NULL)
				iter->tlnext->tlprev = tle;

			tle->tlnext = iter->tlnext;
			tle->tlprev = iter;

			iter->tlnext = tle;

			replaceHeap = true;

		}


	}

	// add to IDMap
	g_idMapResizeLock->lock();
	idMap[tle->id] = tle;
	g_idMapResizeLock->unlock();


	// on/to heap?
	if(replaceHeap == true || heap[hash] == NULL)
		heap[hash] = tle;

	g_lock_list->unlock();

}//end: tl_add




static inline TimerListEntry *tl_del(TimerListEntry *tle){
	int hash = tle->execTick % HEAPSIZE;

	//
	// NOTE: THIS FUNCTION TAKES NO CARE ABouT  LIST LOCK!
	//

	// - remove from idmap
	// - remove from heap
	// - remove from list
	// - fix list Begin

	// Remvoe from IDMAP
	g_idMapResizeLock->lock();
	idMap[tle->id] = NULL;
	g_idMapResizeLock->unlock();

	// Clean Heap!
	if(tle->tlprev != NULL && tle->tlprev->execTick == tle->execTick)	// if the previous timer has the same exectime, set it to the heapslot!
		heap[hash] = tle->tlprev;
	else
		heap[hash] = NULL; // clean heapslot.


	if(tle->tlnext != NULL)
		tle->tlnext->tlprev = tle->tlprev;
	
	if(tle->tlprev != NULL)
		tle->tlprev->tlnext = tle->tlnext;
	else // when tle->tlprev is NULL, it MUST be the listBegin
		listBegin = tle->tlnext;

	return tle;
}//end: tl_del



static __forceinline void RunTimer(TimerListEntry *tle){

	if(true == tle->proc(tle->id, tle->paramPtr, tle->paramInt))
		tle->deleted = true;

	// if this is an interval timer readd it!
	if(tle->interval != -1 && tle->deleted == false){
		tle->execTick += tle->interval;
		tl_add(tle); //is lock aware.
		cond_List->broadcast(); // wake worker otherwise it could happen that they'll oversleep the corrected execution :(
	}else{
		// otherwise return this element to the freelist!
		g_timerPool->put(tle);
	}

}//end: RunTimer()


static DWORD __stdcall timerWorkerProc(util::thread * /*self*/){
	TimerListEntry *tle;
	tick_t curTick, topExecTick;

	while(g_running){
		curTick = tick_get();
		tle = NULL;

		g_lock_list->lock();

		if(listBegin != NULL){
			topExecTick = listBegin->execTick;

			if(topExecTick <= curTick){
				tle = tl_del(listBegin);

				if(listBegin != NULL)	// get the net top's exectick
					topExecTick = listBegin->execTick;
			}

		}else{
			topExecTick = curTick + 30000;	// symboloc value,
											// no timer in list, so simply sleep 30 seks .. (a timerAdd will signal the cond var)
		}

		g_lock_list->unlock();


		/// Got A job!
		if(tle != NULL){

			if(tle->deleted == true){ // Job has been already deleted.. 
				g_timerPool->put(tle);
				continue;
			}

			RunTimer(tle);

			continue; // try to fetch the next job .. :)
		}


		// calc sleep time:
		// topExecTick is the time of the next timer to execute  (absolute tick value)
		//
		g_lock_worker->lock();
		cond_List->wait(g_lock_worker, (DWORD)(topExecTick - curTick));
		g_lock_worker->unlock();

	}//end while ..

	return 0;
}//end: timerWorkerProc()







int timer::add(tick_t execTick, TimerProc proc, void *paramPtr, size_t paramInt){
	TimerListEntry *tle = g_timerPool->get();
	
	tle->execTick = execTick;
	tle->interval = -1;

	tle->deleted = false;

	tle->paramInt = paramInt;
	tle->paramPtr = paramPtr;
	tle->proc = proc;
	

	// add to timerList
	tl_add(tle);


	// tell worker to work! :)
	cond_List->broadcast();

	return tle->id;
}//end: timer_add()


int timer::addInterval(tick_t execTick, tick_t interval, TimerProc proc, void *paramPtr, size_t paramInt){
	TimerListEntry *tle = g_timerPool->get();
	
	tle->execTick = execTick;
	tle->interval = interval;

	tle->deleted = false;

	tle->paramInt = paramInt;
	tle->paramPtr = paramPtr;
	tle->proc = proc;

	// add to timerList
	tl_add(tle);


	// tell worker to work! :)
	cond_List->broadcast();

	return tle->id;
}//end: timer_addInterval()


void timer::del(int timerID){
#ifdef _DEBUG
	if(timerID < 0 || timerID >= (int)numTotal){
		putErr("timer_del(%d): invalid ID! must be within 0 and %u\n", numTotal);
		exit(1);
	}
#endif

	//
	// Worst case without sync:
	//  timer has been already fetcehd for execution while we're going to set deleted to true
	// result:
	//	tiemr gets executed, and afterwards deleted :~ ,  so we dont care about this case.
	//

	//
	g_idMapResizeLock->lock();
	TimerListEntry *tle = idMap[timerID];
	g_idMapResizeLock->unlock();

	if(tle != NULL)
		tle->deleted = true;
	

}//end: timer_del()


} } //end: namespaces
