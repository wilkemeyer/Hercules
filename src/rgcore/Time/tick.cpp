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
#include "../stdafx.h"

namespace rgCore { namespace Time {
using namespace util;

static thread *g_hTickThread = NULL;
atomic::atomic_t g_running = 0;
static atomic::atomic64_t g_curTick = 0;	// value will be updated periodically by timer

//
static LARGE_INTEGER g_rdtsc_freq;	// Divided by 1000 to get a resolution of 1ms 
static LARGE_INTEGER g_rdtsc_start; // Start Count


static DWORD __stdcall periodicTickProc(thread *self){
	LARGE_INTEGER cur;

	// Tick Every MS
	while(1){
		if(atomic::CompareAndSwap(&g_running, 0, 0) == 0){
			break; // 
		}

		QueryPerformanceCounter(&cur);

		atomic::Exchange64(&g_curTick, ((cur.QuadPart - g_rdtsc_start.QuadPart) / g_rdtsc_freq.QuadPart));


		// Sleep 1 ms
		Sleep(1);
	}


	return 0;
}//end: periodicTickProc()


void tick_init(){
	
	static_assert(sizeof(tick_t) == sizeof(atomic::atomic64_t), "Assertion Failure - tick_t != atomic64_t - they should be equal due to performance reasons.");

	if(QueryPerformanceFrequency(&g_rdtsc_freq) == FALSE){
		ShowFatalError("Performance Counters (TSC) are not Supported byh this Server!\n");
		fatalError("Performance Counters (TSC) are not Supported by this Server\n");
		return;
	}

	// To get a resolution of MilliSeconds
	g_rdtsc_freq.QuadPart /= 1000;


	// Get the Initial Value.
	QueryPerformanceCounter(&g_rdtsc_start);

	//Set Running Flag
	atomic::Exchange(&g_running, 1);

	g_hTickThread = new thread("periodicTickThread", periodicTickProc);
	g_hTickThread->setTimeSteal(false);	// Don't allow other threads to interrupt work of this Thread.
	g_hTickThread->setPriority(thread::THRPRIO_HIGHEST);

}//end: tick_init()


void tick_final(){

	// 
	atomic::Exchange(&g_running, 0);


	if(g_hTickThread != NULL){
		g_hTickThread->wait();

		delete g_hTickThread;
		g_hTickThread = NULL;
	}

}//end: tick_final()


tick_t tick_get(){
	return g_curTick;
}//end: tick_get()



} }//namepsaces