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


// 
// This file is not supposed to be used outside the 'internal' netcode
//
namespace rgCore { namespace network {

struct rgSession;

class PendingIO : OVERLAPPED {
public:

	enum type {
		PT_ACCEPT = 0, 
		PT_READ, 
		PT_READRAW,	// @hack - used to 'skip' 4 byte packets receiving on connect to Z/H
		PT_WRITE,
		PT_CONNECT
	} type;

	// parent connection
	rgSession *sess;
	
union{
	//
	// Accept Parameters
	//
	struct{
		SOCKET	newFD;
		DWORD	dwNumBytesReceived;
		BYTE	addrBuf[ 2 * (sizeof(sockaddr_in6) + 16) ]; // dont ask why ...  just look at msdn -> AcceptEx() ... 
	} accept;


	//
	// Read Parameters
	//
	struct {
		enum status {
			PS_NEEDOP,	// need opcode
			PS_NEEDSZ,	// need size (when dynamic length packet)
			PS_NEEDDATA // need data
		} status;
#define HEAD_OP 0
#define HEAD_LEN 1
		WORD head[2]; 
		
		WSABUF buf;
		DWORD dwBytesRead;
		DWORD dwReadFlags;

		char *netBuf; 
	} read;


	//
	// READRAW Parameters
	// (for own protocol implementations)
	struct{
		WSABUF buf;
		DWORD dwBytesRead;
		DWORD dwReadFlags;

		char *netBuf;
		
		void *callback; // When done, callback - see rgNet.h for API / declaration
	} readraw;


	//
	// Write Parameters
	//
	struct {
		PendingIO *next;
		WSABUF buf;
		DWORD dwBytesWritten;
		DWORD dwWriteFlags;
	} write;


	//
	// Connect Parameters
	//
	struct {
		void *callback; // To prevent errors the sess->callback handler wouldnt be used here!
	} connect;

};	


	/////
	// Never allocate mpre than One elementh!
	void *operator new(size_t sz);
	void operator delete(void *ptr);

};



void PendingIOMgr_init();
void PendingIOMgr_final();

/* Stats / Counters of Pool */
void PendingIOMgr_getCounters(size_t *numTotal, size_t *numFree, size_t *numPeak);

} } // end namespaces