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

namespace rgCore { namespace network {

static Pool<PendingIO> *g_PendingIOPool = NULL;
static bool g_initialized = false;

void PendingIOMgr_init(){
	size_t initialAllocation, stepAllocation;
	

	initialAllocation = iniGetAppInteger("Network", "PendingIO Initial Allocation", 2048);
	stepAllocation = iniGetAppInteger("Network", "PendingIO Allocation Step", 512);

	g_PendingIOPool = new Pool<PendingIO>("PendingIO", stepAllocation, initialAllocation, true, [](PendingIO *pio){ memset(pio, 0x00, sizeof(OVERLAPPED)); });

	g_initialized = true;
}//end: PendingIOMgr_init()


void PendingIOMgr_final(){
	if(g_initialized != true)
		return;

	g_initialized = false;

	delete g_PendingIOPool;

}//end: PendingIOMgr_final()


/* Stats of Pool */
void PendingIOMgr_getCounters(size_t *numTotal, size_t *numFree, size_t *numPeak){
	if(g_initialized == false){
		*numTotal = 0;
		*numFree = 0;
		*numPeak = 0;
		return;
	}

	g_PendingIOPool->getCounters(numTotal, numFree, numPeak);

}//end: PendingIOMgr_getCounters()


void *PendingIO::operator new(size_t sz){
#ifdef _DEBUG 
	if(sz > g_PendingIOPool->get_szElem() ){
		putErr("%s: Pool szElem doesnt match requested size! %u > %u\n", __FUNCTION__, sz,g_PendingIOPool->get_szElem());
	}
#endif

	return g_PendingIOPool->get();
}//end: PendingIO::new()


void PendingIO::operator delete(void *ptr){

	// Cleanup before return
	// OVERLAPPED struct must be clean for reuse.
	memset(ptr, 0x00, sizeof(OVERLAPPED));

	g_PendingIOPool->put((PendingIO*)ptr);
}//end: operator delete()

} }