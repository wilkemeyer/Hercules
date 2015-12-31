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

namespace rgCore { namespace network {

enum NB_TYPES : size_t{
	NB_64B = 0,
	NB_256B ,
	NB_1K ,
	NB_2K ,
	NB_4K ,
	NB_8K ,
	NB_DYNAMIC,
	NB_NUM // number of types
};


struct __declspec(align(16)) netBufferImpl{
	enum NB_TYPES type;
	atomic::atomic_t refCount;
};

struct netBuffer64B : netBufferImpl {
	char data[64];
};

struct netBuffer256B : netBufferImpl {
	char data[256];
};

struct netBuffer1K : netBufferImpl {
	char data[1024];
};

struct netBuffer2K : netBufferImpl {
	char data[2048];
};

struct netBuffer4K : netBufferImpl {
	char data[4096];
};

struct netBuffer8K : netBufferImpl {
	char data[8192];
};


///
//static Pool<netBuffer> *g_Pool[NB_NUM];
static Pool<netBuffer64B> *g_Pool64B;
static Pool<netBuffer256B> *g_Pool256B;
static Pool<netBuffer1K> *g_Pool1K;
static Pool<netBuffer2K> *g_Pool2K;
static Pool<netBuffer4K> *g_Pool4K;
static Pool<netBuffer8K> *g_Pool8K;

static bool g_initialized = false;


/* Initializes the given pool type */
template <class T> static Pool<T>* initPool(enum NB_TYPES type, const char *name){
	size_t allocStep, initialAllocate;
	Pool<T>::cbInitializeNode initProc;

	switch(type){
	case NB_64B:	initProc = [](T *buf){ buf->type = NB_64B; };		break;
	case NB_256B:	initProc = [](T *buf){ buf->type = NB_256B; };		break;
	case NB_1K:		initProc = [](T *buf){ buf->type = NB_1K; };		break;
	case NB_2K:		initProc = [](T *buf){ buf->type = NB_2K; };		break;
	case NB_4K:		initProc = [](T *buf){ buf->type = NB_4K; };		break;
	case NB_8K:		initProc = [](T *buf){ buf->type = NB_8K; };		break;
	default: fatalError("netBuffer -> initPool() invalid Buffer Type: %u - %s !! FATAL\n", type, name); break;
	}

	char pName[64], stepName[64], initialName[64];
	sprintf_s(pName, "Netbuffer %s", name);
	sprintf_s(stepName, "Step %s", name);
	sprintf_s(initialName, "Alloc %s", name);

	allocStep		= iniGetAppInteger("NetBuffer", stepName, 64);
	initialAllocate	= iniGetAppInteger("NetBuffer", initialName, 64);

	putLog("[%s] allocStep: %u, initialAllocation: %u\n", pName, allocStep, initialAllocate);

	
	return new Pool<T>(pName, allocStep, initialAllocate, true, initProc);

}//end: initPool()


static void paranoidTest(){
	//netBufferImpl *nb			= 0x00;
	netBuffer64B *nb64		= 0x00;
	netBuffer256B *nb256	= 0x00;
	netBuffer1K *nb1k		= 0x00;
	netBuffer2K *nb2k		= 0x00;
	netBuffer4K *nb4k		= 0x00;
	netBuffer8K *nb8k		= 0x00;
	

	if( (((char*)(&nb64->data)) - sizeof(netBufferImpl)) != 0){
		fatalError("netBuffer - paranoidTest failed @ 64B\n");
	}

	if( (((char*)(&nb256->data)) - sizeof(netBufferImpl)) != 0){
		fatalError("netBuffer - paranoidTest failed @ 256B\n");
	}

	if( (((char*)(&nb1k->data)) - sizeof(netBufferImpl)) != 0){
		fatalError("netBuffer - paranoidTest failed @ 1K\n");
	}

	if( (((char*)(&nb2k->data)) - sizeof(netBufferImpl)) != 0){
		fatalError("netBuffer - paranoidTest failed @ 2K\n");
	}

	if( (((char*)(&nb4k->data)) - sizeof(netBufferImpl)) != 0){
		fatalError("netBuffer - paranoidTest failed @ 4K\n");
	}

	if( (((char*)(&nb8k->data)) - sizeof(netBufferImpl)) != 0){
		fatalError("netBuffer - paranoidTest failed @ 8K\n");
	}


}//end: paranoidTest()


void netBuffer::init(){
	paranoidTest();

	putLog("netBuffer Initialization Begin\n");
	g_Pool64B = initPool<netBuffer64B>(NB_64B, "64B");
	g_Pool256B = initPool<netBuffer256B>(NB_256B, "256B");
	g_Pool1K = initPool<netBuffer1K>(NB_1K, "1K");
	g_Pool2K = initPool<netBuffer2K>(NB_2K, "2K");
	g_Pool4K = initPool<netBuffer4K>(NB_4K, "4K");
	g_Pool8K = initPool<netBuffer8K>(NB_8K, "8K");
	putLog("netBuffer Initialization Finished\n");

	g_initialized = true;

}//end: netBuffer::init()


void netBuffer::final(){
	if(g_initialized != true)
		return;
	
	g_initialized = false;
	
	delete g_Pool8K;
	delete g_Pool4K;
	delete g_Pool2K;
	delete g_Pool1K;
	delete g_Pool256B;
	delete g_Pool64B;
	
}//end: netBuffer::final()


void netBuffer::getCounters64B(size_t *numTotal, size_t *numFree, size_t *numPeak){
	if(g_initialized == false){
		*numTotal = 0;
		*numFree = 0;
		*numPeak = 0;
		return;
	}

	g_Pool64B->getCounters(numTotal, numFree, numPeak);
}//end

void netBuffer::getCounters256B(size_t *numTotal, size_t *numFree, size_t *numPeak){
	if(g_initialized == false){
		*numTotal = 0;
		*numFree = 0;
		*numPeak = 0;
		return;
	}

	g_Pool256B->getCounters(numTotal, numFree, numPeak);
}//end

void netBuffer::getCounters1K(size_t *numTotal, size_t *numFree, size_t *numPeak){
	if(g_initialized == false){
		*numTotal = 0;
		*numFree = 0;
		*numPeak = 0;
		return;
	}

	g_Pool1K->getCounters(numTotal, numFree, numPeak);
}//end

void netBuffer::getCounters2K(size_t *numTotal, size_t *numFree, size_t *numPeak){
	if(g_initialized == false){
		*numTotal = 0;
		*numFree = 0;
		*numPeak = 0;
		return;
	}

	g_Pool2K->getCounters(numTotal, numFree, numPeak);
}//end

void netBuffer::getCounters4K(size_t *numTotal, size_t *numFree, size_t *numPeak){
	if(g_initialized == false){
		*numTotal = 0;
		*numFree = 0;
		*numPeak = 0;
		return;
	}

	g_Pool4K->getCounters(numTotal, numFree, numPeak);
}//end

void netBuffer::getCounters8K(size_t *numTotal, size_t *numFree, size_t *numPeak){
	if(g_initialized == false){
		*numTotal = 0;
		*numFree = 0;
		*numPeak = 0;
		return;
	}

	g_Pool8K->getCounters(numTotal, numFree, numPeak);
}//end



void *netBuffer::get_64B(size_t /*len*/){
	netBuffer64B *nb = g_Pool64B->get();
	atomic::Exchange(&nb->refCount, 1);
	return nb->data;
}//end: netBuffer::get_64B()


void *netBuffer::get_256B(size_t /*len*/){
	netBuffer256B *nb = g_Pool256B->get();
	atomic::Exchange(&nb->refCount, 1);
	return nb->data;
}//end: netBuffer::get_256B()


void *netBuffer::get_1K(size_t /*len*/){
	netBuffer1K *nb = g_Pool1K->get();
	atomic::Exchange(&nb->refCount, 1);
	return nb->data;
}//end: netBuffer::get_1K()


void *netBuffer::get_2K(size_t /*len*/){
	netBuffer2K *nb = g_Pool2K->get();
	atomic::Exchange(&nb->refCount, 1);
	return nb->data;
}//end: netBuffer::get_2K()


void *netBuffer::get_4K(size_t /*len*/){
	netBuffer4K *nb = g_Pool4K->get();
	atomic::Exchange(&nb->refCount, 1);
	return nb->data;
}//end: netBuffer::get_4K()


void *netBuffer::get_8K(size_t /*len*/){
	netBuffer8K *nb = g_Pool8K->get();
	atomic::Exchange(&nb->refCount, 1);
	return nb->data;
}//end: netBuffer::get_8K()


void *netBuffer::get(size_t len){

	if(len <= 64)
		return netBuffer::get_64B(0);
	else if(len <= 256){
		return netBuffer::get_256B(0);
	}else if(len <= 1024)
		return netBuffer::get_1K(0);
	else if(len <= 2048)
		return netBuffer::get_2K(0);
	else if(len <= 4096)
		return netBuffer::get_4K(0);
	else if(len <= 8192)
		return netBuffer::get_8K(0);
	else{
		// 
		putErr("netBuffer::get() -> %I64d Bytes, non-pooled length, allocating via romalloc()\n", len);

		// 
		// Memory Layout:
		// [struct netBuffer] [ data ]
		char *data = (char*)roalloc( sizeof(struct netBufferImpl) + len );
		struct netBufferImpl *nb = (struct netBufferImpl*)data;
		nb->type = NB_DYNAMIC;
		atomic::Exchange(&nb->refCount, 1);

		return ( data + sizeof(struct netBufferImpl) );
	}

}//end: netBuffer::get()


/* Returns the given netbuffer to its corresponding pool, regardless of refcount! */
__forceinline static void freeSub(netBufferImpl *nb){
	
	switch(nb->type){
	case NB_64B:	g_Pool64B->put( (netBuffer64B*) nb );		break;
	case NB_256B:	g_Pool256B->put( (netBuffer256B*) nb );		break;
	case NB_1K:		g_Pool1K->put( (netBuffer1K*) nb );			break;
	case NB_2K:		g_Pool2K->put( (netBuffer2K*) nb );			break;
	case NB_4K:		g_Pool4K->put( (netBuffer4K*) nb );			break;
	case NB_8K:		g_Pool8K->put( (netBuffer8K*) nb );			break;
	case NB_DYNAMIC:	rofree( (void*) nb );		break;
	default: putErr("netBuffer, freeSub() -> got invalid Type %u\n", nb->type); break;
	}
	
}//end: freeSub()


void netBuffer::put(void *ptr){
	struct netBufferImpl *nb;

	nb = (struct netBufferImpl*) (((char*)ptr) - sizeof(struct netBufferImpl));

	if(atomic::Dec(&nb->refCount) > 0)
		return;

	freeSub(nb);

}//end: netBuffer::put()


void netBuffer::incRef(void *ptr){
	struct netBufferImpl *nb;

	nb = (struct netBufferImpl*) (((char*)ptr) - sizeof(struct netBufferImpl));

	atomic::Inc(&nb->refCount);

}//end: netBuffer::incRef()


void netBuffer::lock(void *ptr){
	struct netBufferImpl *nb;

	nb = (struct netBufferImpl*) (((char*)ptr) - sizeof(struct netBufferImpl));
	
	atomic::Add(&nb->refCount, 0x7FFFF);

}//end: netBuffer::lock()


void netBuffer::unlock(void *ptr){
	struct netBufferImpl *nb;

	nb = (struct netBufferImpl*) (((char*)ptr) - sizeof(struct netBufferImpl));
	
	if( atomic::Add(&nb->refCount, -(0x7FFFF)) <= 0 ){
		freeSub(nb);
	}

}//end: netBuffer::unlock()


} }
