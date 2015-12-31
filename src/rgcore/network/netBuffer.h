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

namespace rgCore { namespace network {


class netBuffer {
public:

	static void init();
	static void final();

	// Stats:
	static void getCounters64B(size_t *numTotal, size_t *numFree, size_t *numPeak);
	static void getCounters256B(size_t *numTotal, size_t *numFree, size_t *numPeak);
	static void getCounters1K(size_t *numTotal, size_t *numFree, size_t *numPeak);
	static void getCounters2K(size_t *numTotal, size_t *numFree, size_t *numPeak);
	static void getCounters4K(size_t *numTotal, size_t *numFree, size_t *numPeak);
	static void getCounters8K(size_t *numTotal, size_t *numFree, size_t *numPeak);



	static void *get_64B(size_t /*len*/);		// Note: len is ignored, as you're requesting a specific buffer!
	static void *get_256B(size_t /*len*/);		// Note: len is ignored, as you're requesting a specific buffer!
	static void *get_1K(size_t /*len*/);		// Note: len is ignored, as you're requesting a specific buffer!
	static void *get_2K(size_t /*len*/);		// Note: len is ignored, as you're requesting a specific buffer!
	static void *get_4K(size_t /*len*/);		// Note: len is ignored, as you're requesting a specific buffer!
	static void *get_8K(size_t /*len*/);		// Note: len is ignored, as you're requesting a specific buffer!

	/* */
	static void *get(size_t len);


	// Returns a netbuffer.
	static void put(void *ptr);

	/** 
	 * Locks a netbuffer, which prevents the buffer from being free'd on put
	 * This function is required for multi-send actions on the same buffer
	 */
	static void lock(void *ptr);

	/**
	 * Releases a netbuffer, which allows the buffer to be freed
	 * Note: if the buffer has been totally derefenced before, 
	 * unlock will free the given buffer imeediatly!
	 */
	static void unlock(void *ptr);


	static void incRef(void *ptr);

	/* */
	//#define deref(ptr) put(ptr)
	static  __forceinline void decRef(void *ptr) {
		put(ptr);
	}

};



class scopeBuffer{
public:
	void *m_ptr;
	size_t m_sz;
	
	__forceinline scopeBuffer(size_t sz){
		m_sz = sz;
		m_ptr = netBuffer::get(m_sz);
	}

	__forceinline scopeBuffer(size_t fixedSize, size_t numVariable, size_t variableSize){
		m_sz = fixedSize + (numVariable * variableSize);
		m_ptr = netBuffer::get(m_sz);
	}

	__forceinline ~scopeBuffer(){
		netBuffer::put(m_ptr);
	}


	
	__forceinline void send(ConnectionIdentifier c){
		netBuffer::incRef(m_ptr);
		rgNet_send(c, m_ptr, (ULONG)m_sz);
	}
	
};

template <typename T>
class scopeBufferStatic : public scopeBuffer{
public:
	__forceinline scopeBufferStatic() : scopeBuffer( sizeof(T) ) { }		
	__forceinline T *getPtr(){ return (T*)m_ptr; }

};


#define SCOPEBUFFER(t) scopeBuffer __sc(sizeof(t)); t *out = (t*)__sc.m_ptr
#define SCOPEBUFFER_SEND(c) __sc.send(c)


} }
