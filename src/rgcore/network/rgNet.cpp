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

using namespace rgCore;
using namespace rgCore::network;

////
//
static bool g_initialized = false;

enum LENTABLE{				// Note:
	LENTABLE_CA = 0,		//	This enum must implement all lentables, which are also defined in the PACKETTABLE anum (bitmask)
	LENTABLE_CH = 1,		//
	LENTABLE_CZ = 2,
	LENTABLE_ZC = 3,
	LENTABLE_ZH = 4,
	LENTABLE_OTHER = 5,
	LENTABLE_HA = 6,
	LENTABLE_AH = 7,
	LENTABLE_NUM = 8
};

static const char *LENTABLE_NAMES[LENTABLE_NUM] = {
	"CA", // Client -> Account Server
	"CH", // Client -> Character Server
	"CZ", // Client -> Zone Server
	"ZC", // Zone -> Client
	"ZH",  // Zone -> Character Server (Dispatcher)
	"OTHER", // For other Servers 
	"HA", // Character Server -> Account Server
	"AH"  // Account Server -> Character Server
};

static struct packetLenTable{
	int len;	// -1 => Dynamic, 
				// -2 => unknown,
				// otherwise length up to 64kbyte

	void *(*getNetBufferProc)(size_t);	//	to acquire the matching netbuffer for this packet :)

	onPacketCompleteCallback onComplete;
} g_packetHandler[LENTABLE_NUM][0xffff];

#define PACKETLEN_VARIABLE -1
#define PACKETLEN_UNKNOWN -2

namespace rgCore { namespace network {

typedef struct __declspec(align(16)) rgSession : public SLIST_ENTRY {
	SOCKET					s;
	ConnectionIdentifier	id;	
	enum{	DISCONNECTED = 0,
			INCOMMING,
			LISTENER }		type;

	bool					ipv6;
	union{
		struct sockaddr_in		v4;
		struct sockaddr_in6		v6;
	} addr;

	// Current lentable this connection entry operates with.
	enum LENTABLE table;

	// 
	void *callback;	// Incase of:
					//	- Listener:   onConnect() callback (see onConnectCallback Type)
					//  - Connection: onDisconnect() callback (see onDisconnectCallback type)

	//
	// List of Pending Writes..
	PendingIO *wlBegin, *wlEnd;
	CRITICAL_SECTION wlLock;
	__declspec(align(16)) volatile LONGLONG nWritePending;

	//
	__declspec(align(16)) volatile LONGLONG nPendingIO;
	__declspec(align(16)) volatile LONGLONG bClosePending;

	__declspec(align(16)) volatile LONGLONG bWaitForRemoteDisconect;

	/* Receive state. */
	__declspec(align(16)) volatile LONGLONG bWaitForOP;
	__declspec(align(16)) volatile LONGLONG bRecvable; // Default True, after packet handling successeded, queue another recv ..

	//
	size_t lastAction;  //tick.


	// Support for session based application data storage [FWI: 2013-05-21]
	__declspec(align(16)) void *data;

} rgSession;
} }
#define MAX_CONNECTION 0x3fffc

static Pool<rgSession> *g_SessionPool;
static __declspec(align(16)) LONGLONG g_numV4Connections, g_numV6Connections, g_numPeakV4Connections, g_numPeakV6Connections;
static rgSession *g_session[MAX_CONNECTION]; // ID -> Session structure.


static LPFN_CONNECTEX connectEx;
static WSADATA g_wsaData;
static SOCKET g_extensionSocket; // used to get new winapi functions 

static size_t g_config_nWorker, g_config_maxConnections, g_config_listenBacklog, g_config_gcTimeout, g_config_gcInterval;
static bool g_config_TCP_NODELAY;
static HANDLE g_hCompletionPort;
#define MAX_WORKER 64
static HANDLE g_hWorkerThread[MAX_WORKER];

static void makeRecv(rgSession *sess);
static void doAccept(PendingIO *pio);
static void doDisconnect(rgSession *sess);
static DWORD WINAPI rgNet_workerThread(LPVOID /*param*/);
static void rgNet_onGCSession(rgSession *sess);
static __int64 g_MaxID = 0;

void rgNet_init(){

	putLog("rgNet Initialization Begin\n");

	//
	PendingIOMgr_init(); // allocate pending io stuff/...

	//
	// initialize all packet len tables with 'unknown'
	//
	for(size_t i = 0; i < LENTABLE_NUM; i++){
		struct packetLenTable *tbl = g_packetHandler[i];
		for(size_t op = 0; op < 0xffff; op++){
			tbl[op].len = PACKETLEN_UNKNOWN;
			tbl[op].getNetBufferProc = NULL;
			tbl[op].onComplete = NULL;
		}
	}


	// Initialize Winsock
	int err = WSAStartup(MAKEWORD(2, 2), &g_wsaData);
	if(err != 0){
		putErr("rgNet_init() -> WSAStartup failed (%d)\n", err);
		exit(1);
	}

	// validate ver...
	if(LOBYTE(g_wsaData.wVersion) != 2 || HIBYTE(g_wsaData.wVersion) != 2){
		putErr("rgNet_init() -> WSAStartup failed to request version 2.2\n");
		WSACleanup();
		exit(1);
	}
	

	//
	// Get New Winsock Functions
	//
	{
		g_extensionSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

		if(g_extensionSocket == INVALID_SOCKET){
			putErr("rgNet_init() -> socket() failed, (g_extensionSocket) Err: %d\n",WSAGetLastError());
			WSACleanup();
			exit(1);
		}
		
		//
		// connectEx
		//
		GUID guid = WSAID_CONNECTEX;
		DWORD nBytesGet = 0;
		if( WSAIoctl(g_extensionSocket, SIO_GET_EXTENSION_FUNCTION_POINTER, &guid, sizeof(guid), &connectEx, sizeof(&connectEx), &nBytesGet, NULL, NULL)  != 0 ){
			putErr("rgNet_init() -> WSAIoctl() failed, [tried to receive connectEx pointer], Errno: %d\n", WSAGetLastError() );
			closesocket(g_extensionSocket);
			WSACleanup();
			exit(1);
		}

		if(nBytesGet != sizeof(&connectEx)){
			putErr("rgNet_init() -> WSAIoctl() failed, [tried to receive connectEx pointer], pointer size mismatch!\n");
			closesocket(g_extensionSocket);
			WSACleanup();
			exit(1);
		}
	}

	
	//
	// Create rgSession Pool && Intiialize idmap array -> empty.
	//
	g_config_maxConnections = iniGetAppInteger("Network", "Maximum Connections", 10000);
	if(g_config_maxConnections > MAX_CONNECTION){
		putErr("Maximum Connections > %u,  enforing limit of %u, to allow a higher number of conrurrent connections - please recompile with altered 'MAX_CONNECTION' constant\n", g_config_maxConnections, MAX_CONNECTION, MAX_CONNECTION);
		g_config_maxConnections = MAX_CONNECTION;
	}
	putLog("g_config_maxConnections: %u\n", (DWORD)g_config_maxConnections);
	
	// g_session[] array -> initial setup.
	for(size_t i = 0; i < MAX_CONNECTION; i++)
		g_session[i] = NULL; 

	//
	g_config_gcTimeout = 1000 * iniGetAppInteger("Network", "Garbage Collection Connection Reuse", 30);

	putLog("g_config_gcTimeout: %u\n", g_config_gcTimeout);

	// Initialize Stat Counters -> 0
	InterlockedExchange64(&g_numV4Connections, 0);
	InterlockedExchange64(&g_numV6Connections, 0);
	InterlockedExchange64(&g_numPeakV4Connections, 0);
	InterlockedExchange64(&g_numPeakV6Connections, 0);

	g_SessionPool = new Pool<rgSession>("rgSession", 1024, g_config_maxConnections, true,  
		// Initialize Node
		//
		[](rgSession *sess){
		
			InitializeCriticalSectionEx(&sess->wlLock, 8000, CRITICAL_SECTION_NO_DEBUG_INFO);
			sess->wlBegin = NULL;
			sess->wlEnd = NULL;

			sess->id = g_MaxID++;
			sess->type = rgSession::DISCONNECTED;
			sess->s = INVALID_SOCKET;

			sess->data = NULL;
			//putDbg("-> %u\n", sess->id);

	},
		// Finalie Node (Cleanup / Shutdown)
		//
		[](rgSession *sess){

			DeleteCriticalSection(&sess->wlLock);
	},
		// "onNewSegmentAlloc"
		// 
		[](size_t newTotalNum){
			size_t oldLimit = g_config_maxConnections;
		
			if(newTotalNum >= MAX_CONNECTION){ // ow!! 
				putErr("FATAL: g_SessionPool -> onNewSegmentAlloc: tried to resize to: %u, Built UPPER limit is: %u!\n", newTotalNum, MAX_CONNECTION);
				putErr("+++ Requesting abort()\n");
				abort();
			}

			// Zero ID 2 Sess slots.
			for(size_t i = oldLimit; i < newTotalNum; i++)
				g_session[i] = NULL;

			
			// Re 'set' config value ..
			g_config_maxConnections = newTotalNum;
			
			// If already initialized this means the core has to reallocate during runtime - BAD - so print warning!
			if(g_initialized == true){
				putWrn("g_config_maxConnections: %u\n", (DWORD)g_config_maxConnections);
			}

	});

	//
	// SessionPool is running GC!
	//
	g_SessionPool->setupGC(g_config_gcTimeout, rgNet_onGCSession);


	//
	// Create Completion Port 
	//
	g_config_nWorker = iniGetAppInteger("Network", "Worker Count", 4); 
	if(g_config_nWorker > MAX_WORKER) g_config_nWorker = MAX_WORKER;

	g_hCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, NULL, (DWORD)g_config_nWorker);
	if(g_hCompletionPort == NULL){
		putErr("rgNet_init() -> CreateIoCompletionPort() failed, errno: %d\n", GetLastError());
		delete g_SessionPool;
		closesocket(g_extensionSocket);
		WSACleanup();
		exit(1);
	}
	
	putLog("g_config_nWorker: %u\n", (DWORD)g_config_nWorker);



	//
	// Spawn Worker Threads
	//
	for(size_t i = 0; i < g_config_nWorker; i++){
		g_hWorkerThread[i] = CreateThread(NULL, 0, rgNet_workerThread, NULL, 0, NULL);
		if(g_hWorkerThread[i] == NULL){
			putErr("rgNet_init() -> CreateThread() failed, errno: %d\n", GetLastError());
			for(--i; i != 0; i--){
				TerminateThread(g_hWorkerThread[i], 1);
				CloseHandle(g_hWorkerThread[i]);
			}
			delete g_SessionPool;
			CloseHandle(g_hCompletionPort);
			closesocket(g_extensionSocket);
			WSACleanup();
			exit(1);
		}
	}


	// 
	g_config_listenBacklog = iniGetAppInteger("Network", "Default Listen Backlog", 50);
	putLog("g_config_listenBacklog: %u\n", g_config_listenBacklog);

	g_config_TCP_NODELAY = iniGetAppBoolean("Network", "Default TCP_NODELAY", false);
	putLog("TCP_NODELAY: %s\n", (g_config_TCP_NODELAY==true)?"true":"false");


	putLog("rgNet Initialization Finished\n");
	g_initialized = true;

}//end: rgNet_init()


void rgNet_final(){
	if(g_initialized != true)
		return;


	//
	// @TODO: Disconnect all connections!
	//

	// Free rgSession list / pool.
	delete g_SessionPool;


	// @TODO: Terminate threads in a clean way!
	for(size_t i = 0 ; i < g_config_nWorker; i++){
		TerminateThread(g_hWorkerThread[i], 1);
		CloseHandle(g_hWorkerThread[i]);
	}

	g_initialized = false;

	CloseHandle(g_hCompletionPort);
	closesocket(g_extensionSocket);
	WSACleanup();
	PendingIOMgr_final();
	
}//end: rgNet_final()


void rgNet_getSessionCounters(size_t *numV4, size_t *numV6, size_t *peakV4, size_t *peakV6){
	if(g_initialized == false){
		*numV4 = 0;
		*numV6 = 0;
		*peakV4 = 0;
		*peakV6 = 0;
		return;
	}

	//
	*numV4 = g_numV4Connections;
	*numV6 = g_numV6Connections;
	*peakV4 = g_numPeakV4Connections;
	*peakV6 = g_numPeakV6Connections;

}//end: rgNet_getSessionCounters()


ConnectionIdentifier rgNet_getMaxConnectionID(){
	return (g_config_maxConnections-1);
}//end: rgNet_getMaxConnectionID()


static void makePendingAccept(rgSession *sess){
	BOOL bRet;
	int err, sz, sockaddrsz, family;
	char opt;
	PendingIO *pio;

	if(sess->ipv6 == true){
		family = AF_INET6;
		sockaddrsz = sizeof( struct sockaddr_in6 );
	}else{
		family = AF_INET;
		sockaddrsz = sizeof( struct sockaddr_in );
	}


	pio = new PendingIO();
	pio->type = PendingIO::PT_ACCEPT;
	pio->sess = sess;


retrysocket:
	pio->accept.newFD = WSASocket(family, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);

	if(pio->accept.newFD == INVALID_SOCKET){
		putErr("makePendingAccept() -> cannot create new Socket; retrying in 50ms (WSAERR: %u)\n", WSAGetLastError() );
		Sleep(50);
		goto retrysocket;
	}

	//
	// Set newFD's send buffer to 0 
	//
	sz = 0;
	if(setsockopt(pio->accept.newFD, SOL_SOCKET, SO_SNDBUF, (char*)&sz, sizeof(sz)) == SOCKET_ERROR){
		putErr("makePendingAccept() -> cannot set SO_SNDBUF to 0 - skipping flag  (WSAERR: %u)\n", WSAGetLastError() );
	}

	//
	// Disable Naggle
	//
	if(g_config_TCP_NODELAY == true){
		opt = 1;
		if(setsockopt(pio->accept.newFD, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(char)) == SOCKET_ERROR){
			putErr("makePendingAccept() -> cannot set TCP_NODELAY = 1 - skipping flag  (WSAERR: %u)\n", WSAGetLastError() );
		}
	}

retryaccept:
	bRet = AcceptEx(sess->s,  pio->accept.newFD, pio->accept.addrBuf, 0, sockaddrsz+16, sockaddrsz+16, &pio->accept.dwNumBytesReceived, (OVERLAPPED*)pio );
	if(bRet == FALSE){
		err = WSAGetLastError();
		if(err != ERROR_IO_PENDING){
			//
			// Shouldnt happen??
			//
			putErr("makePendingAccept() -> AcceptEx returned FALSE, but not IO_PENDING (WSAERR: %u) - Throttled 50ms (retry accept)\n", err);
			Sleep(50);
			goto retryaccept;
		}
	}
	// true case can be skipped, because true is the sample as ERROR_IO_PENDING :)

}//end: makePendingAccept()


static void doAccept(PendingIO *pio){
	rgSession *sess; // this represents the newly accepted connection.

	//
	// @TODO:
	// check for connection count limit against builtin MAX_CONNECTION const!
	// to prevent core force shutdown due to max conn limit reach ..
	//

	sess = g_SessionPool->get();
	if(sess == NULL){
		putErr("doAccept() -> connection limit reached - dropping\n");
		closesocket(pio->accept.newFD);
		return;
	}

	// 
	// Fill the Remote Address field..
	//
	{
		int remote_len, local_len;
		union {
			struct sockaddr_in	*v4;
			struct sockaddr_in6	*v6;
		} local, remote;

		if(pio->sess->ipv6 == true){ // an v6 listener only procudes v6 connections!
			remote_len = sizeof(struct sockaddr_in6);
			local_len = sizeof(struct sockaddr_in6);
		}else{
			remote_len = sizeof(struct sockaddr_in);
			local_len = sizeof(struct sockaddr_in);
		}

		GetAcceptExSockaddrs(pio->accept.addrBuf, 
								0, 
								local_len + 16, 
								remote_len + 16,
								(sockaddr**)&local, &local_len,
								(sockaddr**)&remote, &remote_len);

		// copy address ..
		if(pio->sess->ipv6 == true){
#ifdef _DEBUG	// assert.
			if(remote_len > sizeof(struct sockaddr_in6)){
				putErr("doAccept() - ipv6 - remote addr len failed.\n");
				exit(1);
			}
#endif
			sess->addr.v6.sin6_family = AF_INET6;
			memcpy(sess->addr.v6.sin6_addr.u.Byte, remote.v6->sin6_addr.u.Byte, 16);
			sess->addr.v6.sin6_port = remote.v6->sin6_port;

			sess->ipv6 = true;
		}else{
#ifdef _DEBUG	// assert.
			if(remote_len > sizeof(struct sockaddr_in)){
				putErr("doAccept() - ipv4 - remote addr len failed.\n");
				exit(1);
			}
#endif

			sess->addr.v4.sin_family = AF_INET;
			sess->addr.v4.sin_addr.S_un.S_addr = remote.v4->sin_addr.S_un.S_addr;
			sess->addr.v4.sin_port = remote.v4->sin_port;
			
			sess->ipv6 = false;
		}
	}

	//
	// Assign other values. .
	//
	sess->s		= pio->accept.newFD;
	sess->type	= rgSession::INCOMMING;
	sess->table	= pio->sess->table;
	sess->nPendingIO = 0;
	sess->nWritePending = 0;
	sess->bClosePending = FALSE;
	sess->bWaitForRemoteDisconect = FALSE;
	sess->callback = NULL;

	//
	// Create IOcp
	//
	if(CreateIoCompletionPort( (HANDLE)sess->s, g_hCompletionPort, sess->id, 0) != g_hCompletionPort){
		putErr("doAccept() -> failed to create iocp for new connection - dropping connection errno: %u\n", GetLastError());
		closesocket(sess->s);

		// return the sess entry
		sess->s = INVALID_SOCKET;
		sess->type = rgSession::DISCONNECTED;

		sess->lastAction = Time::tick_get();
		g_SessionPool->put(sess);

		return;
	}


	//
	// assign to 'connected' table
	//
	g_session[ sess->id ] = sess; 
	_WriteBarrier();


	// 
	// Call onconnect event handler
	//
	if( ((onConnectCallback)pio->sess->callback)( sess-> id) == false ) {
		// 
		// rejected the connection :(
		//
		
		closesocket(sess->s);

		g_session[ sess->id ] = NULL;
		_WriteBarrier();

		sess->s = INVALID_SOCKET;
		sess->type = rgSession::DISCONNECTED;
		sess->lastAction = Time::tick_get();
		g_SessionPool->put(sess);

		return;
	}


	//
	// Statistics!
	// (FWI: i know that this code isnt 100% thread sync, but as this are only counters...)
	//
	if(sess->ipv6 == true){
		// v6
		InterlockedIncrement64(&g_numV6Connections);
		if(g_numV6Connections > g_numPeakV6Connections){
			InterlockedExchange64(&g_numPeakV6Connections, g_numV6Connections);
		}

	}else{
		// v4
		InterlockedIncrement64(&g_numV4Connections);
		if(g_numV4Connections > g_numPeakV4Connections){
			InterlockedExchange64(&g_numPeakV4Connections, g_numV4Connections);
		}

	}


	//
	// Setup initial Recv ..
	//
	makeRecv( sess );

}//end: doAccept();

/* Handler for outgoing connections */
static void doHandleConnect(rgSession *sess, onConnectionEstablished cb, bool state){
	

	// Notify the Application about state ...
	if(cb != NULL)
		cb(sess->id, state);


	// Incase of successful connection, makeRecv()!
	if(state == true)
		makeRecv(sess);

}//end: doHandleConnect()


/**
 * Initial Setup of Socket for Recving ro protocol stuff..
 * called after a connectiion has been newly established...
 */
static void makeRecv(rgSession *sess){
	PendingIO *pio;
	int ret;

	// Only queue new IO when connection is not set to close!
	if(InterlockedCompareExchange64(&sess->bClosePending, TRUE, TRUE) == TRUE)
		return;


	InterlockedIncrement64(&sess->nPendingIO);

	InterlockedExchange64(&sess->bWaitForOP, TRUE);
	InterlockedExchange64(&sess->bRecvable, TRUE);

	pio = new PendingIO();
	pio->type = PendingIO::PT_READ;
	pio->sess = sess;

	//
	pio->read.status		= pio->read.PS_NEEDOP;
	pio->read.buf.buf	= (char*)&pio->read.head[HEAD_OP];
	pio->read.buf.len	= 2;
	pio->read.netBuf	= NULL;
	pio->read.dwBytesRead = 0;
	pio->read.dwReadFlags = MSG_WAITALL; // Only report when completed or .. errror

	ret = WSARecv(	sess->s,			// cconnection's socket
					&pio->read.buf,		// buffer
					1,					// number of buffers
					&pio->read.dwBytesRead,	
					&pio->read.dwReadFlags, 
					(LPOVERLAPPED)pio,
					NULL);

	if(ret == SOCKET_ERROR){
		if(WSAGetLastError() != ERROR_IO_PENDING){
			//
			// Recv returned error (read failed)
			//
			delete pio;	// free pendingio slot
			InterlockedExchange64(&sess->bClosePending, TRUE); // disconnect.
			CancelIoEx((HANDLE)sess->s, NULL);
			InterlockedDecrement64(&sess->nPendingIO);

			doDisconnect(sess);
		}
	}


}//end: makeRecv()


void rgNet_rawRead(ConnectionIdentifier cid, size_t len, char *destNetBuffer, onRawReadCompleteCallback onCompletion){
	rgSession *sess = g_session[cid];
	PendingIO *pio;
	int ret;

	_ReadBarrier();
	if(sess == NULL)
		return;

	if(sess->type == rgSession::DISCONNECTED || sess->s == INVALID_SOCKET)	// call to already disconnected connection
		return;

	if(InterlockedCompareExchange64(&sess->bClosePending, TRUE, TRUE) == TRUE)
		return; // disconnect is pending..


	// disable normal RO packet parser
	rgNet_disableRecv(cid);		// @OPTIMIZE: ... for heavier use this should be optmiize (2+ followed raw reads this isnt efficient) [FWI]


	InterlockedIncrement64(&sess->nPendingIO);

	pio = new PendingIO();
	pio->type = PendingIO::PT_READRAW;
	pio->sess = sess;

	//
	pio->readraw.callback	= (void*)onCompletion; // ref cbk
	pio->readraw.netBuf		= destNetBuffer;
	pio->readraw.buf.buf	= destNetBuffer;
	pio->readraw.buf.len	= (ULONG)len;
	pio->readraw.dwBytesRead = 0;
	pio->readraw.dwReadFlags = MSG_WAITALL;	// Only report when complted or error / no direct action is reported.

	ret = WSARecv(	sess->s,			// cconnection's socket
					&pio->readraw.buf,		// buffer
					1,					// number of buffers
					&pio->readraw.dwBytesRead,	
					&pio->readraw.dwReadFlags, 
					(LPOVERLAPPED)pio,
					NULL);

	if(ret == SOCKET_ERROR){
		if(WSAGetLastError() != ERROR_IO_PENDING){
			//
			// Recv returned error (readraw failed)
			//
			delete pio;	// free pendingio slot
			InterlockedExchange64(&sess->bClosePending, TRUE); // disconnect.
			CancelIoEx((HANDLE)sess->s, NULL);
			InterlockedDecrement64(&sess->nPendingIO);

			doDisconnect(sess);
		}
	}

}//end: rgNet_rawRead()


void rgNet_disableRecv(ConnectionIdentifier cid){
	rgSession *sess = g_session[cid];
	_ReadBarrier();
	if(sess == NULL)
		return;

	if(sess->type == rgSession::DISCONNECTED || sess->s == INVALID_SOCKET)	// call to already disconnected connection
		return;

	if(InterlockedCompareExchange64(&sess->bClosePending, TRUE, TRUE) == TRUE)
		return; // disconnect is pending..

	// after completion no recv will be queued, 
	InterlockedExchange64(&sess->bRecvable, FALSE); // disable

}//end: rgNet_disableRecv()


void rgNet_makeRecv(ConnectionIdentifier cid){
	rgSession *sess = g_session[cid];
	_ReadBarrier();
	if(sess == NULL)
		return;

	if(sess->type == rgSession::DISCONNECTED || sess->s == INVALID_SOCKET)	// call to already disconnected connection
		return;

	if(InterlockedCompareExchange64(&sess->bClosePending, TRUE, TRUE) == TRUE)
		return; // disconnect is pending..

	if(InterlockedCompareExchange64(&sess->bRecvable, TRUE, TRUE) == TRUE){
		putErr("rgNet_makeRecv(%u) -> Connection is already recvable! - Ignoring Call\n", cid);
		return;
	}

	//
	makeRecv( sess );

}//end: rgNet_makeRecv()


static void makeSend(rgSession *sess){
	PendingIO *pio;
	int ret;

	// Only queue new IO when connection is not set to close!
	if(InterlockedCompareExchange64(&sess->bClosePending, TRUE, TRUE) == TRUE)
		return;


	EnterCriticalSection(&sess->wlLock); // maybe blocked ..

	if(InterlockedCompareExchange64(&sess->nWritePending, 0, 0) != 0){
		LeaveCriticalSection(&sess->wlLock);
		return;
	}

	pio = sess->wlBegin;
	if(pio == NULL){
		LeaveCriticalSection(&sess->wlLock);
		return;
	}

	sess->wlBegin = pio->write.next;
	if(sess->wlBegin == NULL)
		sess->wlEnd = NULL;

	InterlockedIncrement64(&sess->nWritePending);
	InterlockedIncrement64(&sess->nPendingIO);

	LeaveCriticalSection(&sess->wlLock);

	// -> not needed by WSASend only @ recv.
	//pio->write.dwWriteFlags = MSG_WAITALL;
	//

	//
	//
	//
	ret = WSASend( sess->s, 
					&pio->write.buf,
					1,
					&pio->write.dwBytesWritten,
					pio->write.dwWriteFlags,
					(LPOVERLAPPED)pio,
					NULL);

	if(ret == SOCKET_ERROR){
		if(WSAGetLastError() != ERROR_IO_PENDING){
			//
			// Recv returned error (read failed)
			//
			netBuffer::put( pio->write.buf.buf );
			delete pio;	// free pendingio slot
			InterlockedExchange64(&sess->bClosePending, TRUE); // disconnect.
			CancelIoEx((HANDLE)sess->s, NULL);
			InterlockedDecrement64(&sess->nPendingIO);

			doDisconnect(sess);
		}		
	}


}//end: makeSend()


/* Note:  DONT delete pio after hadnling this event! its getting reused */
static void doRecv(PendingIO *pio){
	rgSession *sess = pio->sess;
	struct packetLenTable *ple;
	int ret, err;

	/// 
	// Check for 'connection close requested?'
	if(InterlockedCompareExchange64(&sess->bClosePending, TRUE, TRUE) == TRUE){
		// 
		// closePending, cleanup pendingIO / do nothing.
		//
		delete pio;
		InterlockedDecrement64(&sess->nPendingIO);

		doDisconnect(sess); // call disconnect handler, our decrement may has released the last pending io!
		return;
	}


	switch(pio->read.status){
	case pio->read.PS_NEEDOP:
		//
		// Got an event in needop state
		// (Opcode Arrived)
		InterlockedExchange64(&sess->bWaitForOP, FALSE); // no longer capable for doing table changes.

		ple = &g_packetHandler[ sess->table ][ pio->read.head[HEAD_OP] ];
		if(ple->len == PACKETLEN_UNKNOWN){
			//
			// Unknown packet received!
			//
			
			putLog("Got Unknown op: 0x%04x\n", pio->read.head[HEAD_OP]);

			delete pio;
			InterlockedExchange64(&sess->bClosePending, TRUE); // disconnect.
			CancelIoEx((HANDLE)sess->s, NULL);
			InterlockedDecrement64(&sess->nPendingIO);

			
			doDisconnect(sess);
			return;

		}else if(ple->len == PACKETLEN_VARIABLE){
			//
			// Variable Packet, next state: need size
			//
			memset(pio, 0x00, sizeof(OVERLAPPED)); // according to msdn, required.
			pio->read.status	= pio->read.PS_NEEDSZ;
			pio->read.buf.buf	= (char*)&pio->read.head[HEAD_LEN];
			pio->read.buf.len	= 2;
			pio->read.dwBytesRead = 0;
			pio->read.dwReadFlags = MSG_WAITALL;

			// queue recv()
			ret = WSARecv( sess->s, 
							&pio->read.buf, 
							1,
							&pio->read.dwBytesRead,
							&pio->read.dwReadFlags,
							(LPOVERLAPPED)pio,
							NULL);

			if(ret == SOCKET_ERROR){
				err = WSAGetLastError();
				if(err != ERROR_IO_PENDING){
					// 
					//
					//
					delete pio;
					InterlockedExchange64(&sess->bClosePending, TRUE); // disconnect.
					CancelIoEx((HANDLE)sess->s, NULL);
					InterlockedDecrement64(&sess->nPendingIO);

					doDisconnect(sess);
					return;
				}
			}
		}else if(ple->len == 2){
			//
			// Static Packet lenght, 2 bytes (no payload / data), complete immediatly!
			//
			pio->read.netBuf = (char*)netBuffer::get_64B(2);
			*((WORD*)pio->read.netBuf) = pio->read.head[HEAD_OP];	// copy opcode to netbuffer.

			goto packetComplete;

		}else{
			//
			// Static Packet, next state: need data!
			//
			memset(pio, 0x00, sizeof(OVERLAPPED));
			pio->read.status	= pio->read.PS_NEEDDATA;
			pio->read.netBuf	= (char*)ple->getNetBufferProc( ple->len ); // get netbuffer!
			*((WORD*)pio->read.netBuf) = pio->read.head[HEAD_OP];	// copy opcode to netbuffer.

			pio->read.buf.len	= (ple->len - 2); // -2 as we already have to opcode! 
			pio->read.buf.buf	= pio->read.netBuf + 2;

			pio->read.head[HEAD_LEN] = ple->len;	// also assign value to header.
			pio->read.dwBytesRead = 0;
			pio->read.dwReadFlags = MSG_WAITALL;

			// queue
			ret = WSARecv( sess->s,
							&pio->read.buf,
							1,
							&pio->read.dwBytesRead,
							&pio->read.dwReadFlags,
							(LPOVERLAPPED)pio,
							NULL);
			
			if(ret == SOCKET_ERROR){
				err = WSAGetLastError();
				if(err != ERROR_IO_PENDING){
					//
					//
					//
					netBuffer::put( pio->read.netBuf );

					delete pio;
					InterlockedExchange64(&sess->bClosePending, TRUE); // disconnect.
					CancelIoEx((HANDLE)sess->s, NULL);
					InterlockedDecrement64(&sess->nPendingIO);

					doDisconnect(sess);
					return;
				}
			}
			
		}

		break;

	case pio->read.PS_NEEDSZ:
		//
		// gotan event in needsz state
		//  - size arrived :)
		
		// check received parameter
		if(pio->read.head[HEAD_LEN] < 4){
			// 
			// Variable packets must have a length of 4, at least 
			// (OP.W, LEN.W)
			//
			delete pio;
			InterlockedExchange64(&sess->bClosePending, TRUE); // disconnect.
			CancelIoEx((HANDLE)sess->s, NULL);
			InterlockedDecrement64(&sess->nPendingIO);

			doDisconnect(sess);
			return;

		}else if( pio->read.head[HEAD_LEN] == 4 ){
			//
			// packet has no payload (data), complete immediatly!
			//
			pio->read.netBuf = (char*)netBuffer::get_64B(4);
			*((DWORD*)pio->read.netBuf) =  *((DWORD*)&pio->read.head); // copy opcode & length, using a movd op. :)

			goto packetComplete;

		}

		//
		// next state: need data!
		//
		memset(pio, 0x00, sizeof(OVERLAPPED));
		pio->read.status	= pio->read.PS_NEEDDATA;
		
		pio->read.netBuf	= (char*)netBuffer::get( pio->read.buf.len );
		*((DWORD*)pio->read.netBuf) =  *((DWORD*)&pio->read.head); // copy opcode & length, using a movd op. :)

		pio->read.buf.len	=  (pio->read.head[HEAD_LEN] - 4); // -4 => dont include op + len, as we already have them!
		pio->read.buf.buf	= pio->read.netBuf + 4; // @ pos, 4 

		pio->read.dwBytesRead = 0;
		pio->read.dwReadFlags = MSG_WAITALL;

		ret = WSARecv(sess->s, 
						&pio->read.buf,
						1,
						&pio->read.dwBytesRead,
						&pio->read.dwReadFlags,
						(LPOVERLAPPED)pio,
						NULL);

		if(ret == SOCKET_ERROR){
			err = WSAGetLastError();
			if(err != ERROR_IO_PENDING){
				//
				//
				//
				netBuffer::put( pio->read.netBuf );

				delete pio;
				InterlockedExchange64(&sess->bClosePending, TRUE); // disconnect.
				CancelIoEx((HANDLE)sess->s, NULL);
				InterlockedDecrement64(&sess->nPendingIO);

				doDisconnect(sess);
				return;
			}
		}

		break;


	case pio->read.PS_NEEDDATA:
		//
		// Data Arrived, next action -> call callback :)
		//
packetComplete:

		//
		// Is connection in 'wait for remote close' state? 
		//
		if(InterlockedCompareExchange64(&sess->bWaitForRemoteDisconect, TRUE, TRUE) == TRUE){
			//
			// It is, its not allowed to receive packets! 
			//
			delete pio;
			CancelIoEx((HANDLE)sess->s, NULL);
			InterlockedDecrement64(&sess->nPendingIO);
			doDisconnect(sess);
			return; 
		}

		//
		// Set bWaitForOp = true (as its the next state), so the onComplete proc can do changes on the sessions state.. (lentable)
		//
		InterlockedExchange64(&sess->bWaitForOP, TRUE);

		//
		// Call Callback!
		//
		ple = &g_packetHandler[ sess->table ][ pio->read.head[HEAD_OP] ];
		if(ple->onComplete != NULL)
			ple->onComplete(sess->id, pio->read.netBuf );
		
		// There's a net buffer in any case in this state.
		netBuffer::put( (void*) pio->read.netBuf );
		pio->read.netBuf	= NULL; // in this state, we have no netbuffer attached :)


		// 
		// check for disconnect .. 
		//  a packetHandler routine can disconnect a connection! :)
		//
		if(InterlockedCompareExchange64(&sess->bClosePending, TRUE, TRUE) == TRUE){
			// release the io buffer ..
			delete pio;
			CancelIoEx((HANDLE)sess->s, NULL);
			InterlockedDecrement64(&sess->nPendingIO);
			doDisconnect(sess);
			return;
		}

		//
		// a Packet Handler can set a connection to non-recv state!
		//
		if(InterlockedCompareExchange64(&sess->bRecvable, FALSE, FALSE) == FALSE){
			// release the io buffer
			delete pio;
			InterlockedDecrement64(&sess->nPendingIO);	// bugfix? in case of disableRecv() there 'd be no furhter recv() queued
														// so the pendingIO count needs to be decremented,  as a rgNet_makeRecv wil call makeRecv()  which w'd increment the counter [FWI] // 2013-02-13
			return; // no further pending read, app needs to call regNet_makeRecv()
		}


		// 
		// next step: need Op!
		//
		memset(pio, 0x00, sizeof(OVERLAPPED));
		pio->read.status	= pio->read.PS_NEEDOP;
		pio->read.buf.buf	= (char*)&pio->read.head[HEAD_OP];
		pio->read.buf.len	= 2;
		pio->read.dwBytesRead = 0;
		pio->read.dwReadFlags = MSG_WAITALL;

		ret = WSARecv(sess->s, 
						&pio->read.buf,
						1,
						&pio->read.dwBytesRead,
						&pio->read.dwReadFlags,
						(LPOVERLAPPED)pio,
						NULL);
		
		if(ret == SOCKET_ERROR){
			err = WSAGetLastError();
			if(err != ERROR_IO_PENDING){
				//
				//
				//
				delete pio;
				InterlockedExchange64(&sess->bClosePending, TRUE); // disconnect.
				CancelIoEx((HANDLE)sess->s, NULL);
				InterlockedDecrement64(&sess->nPendingIO);

				doDisconnect(sess);
				return;
			}
		}

		break;
	}

}//end: doRecv()


static void doRecvRaw( PendingIO *pio ){
	rgSession *sess = pio->sess;

	//
	// Is connection in 'wait for remote close' state? 
	//
	if(InterlockedCompareExchange64(&sess->bWaitForRemoteDisconect, TRUE, TRUE) == TRUE){
		//
		// It is, its not allowed to receive packets! 
		//
		delete pio;
		CancelIoEx((HANDLE)sess->s, NULL);
		InterlockedDecrement64(&sess->nPendingIO);
		doDisconnect(sess);
		return; 
	}

	// otherwise ->
	//  PIO must be deleted
	// 

	if(pio->readraw.callback != NULL)
		((onRawReadCompleteCallback)pio->readraw.callback)( sess->id, pio->readraw.buf.len, pio->readraw.netBuf );
		

	InterlockedDecrement64(&sess->nPendingIO);
	delete pio;

}//end: doRecvraw()


static void doDisconnect(rgSession *sess){
	//
	// @TODO
	//
	//
	// cleanup : iocp,  socket
	// clear all pending io!

	//
	// Check if the session has already been disconnected
	// due to concurrency this may happen
	// Client Disconnects (Network event) && Applicaiton wants to disconnect, too
	//
	rgSession *tmpSession = g_session[ sess->id ];
	_ReadBarrier();
	if(tmpSession == NULL)
		return; // 


	if(InterlockedCompareExchange64(&sess->bClosePending, TRUE, TRUE) != TRUE){	// do nothing if not set to closePending
		printf(" dc but closePending != true!\n");
		return;
	}

	if(InterlockedCompareExchange64(&sess->nPendingIO, 0, 0) != 0){
		//printf(" dc but pending num is %u != 0  -  %u\n", sess->nPendingIO, sess->nWritePending);
		// this case is fully valid. :)
		return;
	}



#ifdef _DEBUG
	if(sess->s == INVALID_SOCKET){
		putErr("DoDisconnect CID(%u) - INVALID_SOCKET = true!\n", sess->id);
		return;
	}
#endif


	//
	// Remove from global session array!
	//
	g_session[sess->id] = NULL;
	_WriteBarrier();
	
	//
	// Call onDisconnect if set and if not listening type socket!
	//
	if(sess->type != rgSession::LISTENER && 
		sess->callback != NULL){
			((onDisconectCallback)sess->callback)( sess->id, sess->data );
	}

	//
	// release all writes from write list..
	//
	PendingIO *iter, *iter_next;
	iter = sess->wlBegin;
	while(1){
		if(iter == NULL)
			break;

		iter_next = iter->write.next;

		netBuffer::put( iter->write.buf.buf );
		delete iter;

		iter = iter_next;
	}

	//
	// Statistics!
	//
	if(sess->ipv6 == true){
		InterlockedDecrement64(&g_numV6Connections);
	}else{
		InterlockedDecrement64(&g_numV4Connections);
	}



	// 
	// Close socket, release icop binding .
	// release rgSession entry!
	//
	closesocket(sess->s);

	sess->bClosePending = FALSE;
	sess->s = INVALID_SOCKET;
	sess->type = rgSession::DISCONNECTED;
	sess->nPendingIO = 0;
	sess->nWritePending = 0;
	sess->wlBegin = NULL;
	sess->wlEnd = NULL;
	sess->lastAction = Time::tick_get();
	sess->data = NULL; 

	g_SessionPool->put(sess);
}//end: doDisconnect()


ConnectionIdentifier rgNet_addListener(const char *address,  unsigned short port,  bool ipv6,  PACKETTABLE initialPacketTable, onConnectCallback onConnectProc){
	rgSession *sess;
	
	sess = g_SessionPool->get();
	if(sess == NULL){
		//
		// Out of sessions... (connection limit reached)
		//
		putErr("rgNet_addListener('%s', %u) -> connection Limit reached, - cannot add listener\n", address, port);
		return INVALID_CONNECTION;
	}

	sess->s = WSASocket((ipv6==true)?AF_INET6:AF_INET,  SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if(sess->s == INVALID_SOCKET){
		putErr("rgNet_addListener('%s', %u) -> WSASocket() failed: %u\n", address, port, WSAGetLastError() );
		sess->lastAction = Time::tick_get();
		g_SessionPool->put( sess );
		return INVALID_CONNECTION;
	}

	int sz = 0;
	if(setsockopt(sess->s, SOL_SOCKET, SO_SNDBUF, (char*)&sz, sizeof(sz)) == SOCKET_ERROR){
		putErr("rgNet_addListener('%s', %u) -> cannot set SO_SNDBUF to 0: %u\n", address, port, WSAGetLastError());
		closesocket(sess->s);
		sess->s = INVALID_SOCKET;
		sess->lastAction = Time::tick_get();
		g_SessionPool->put(sess);
		return INVALID_CONNECTION;
	}

	//
	// Bind!  
	//
	int ret;
	memset(&sess->addr, 0x00, sizeof(sess->addr) );
	if(ipv6 == true){
		//
		// ipv6
		//
		sess->addr.v6.sin6_family = AF_INET6;
		inet_pton(AF_INET6, address, &sess->addr.v6.sin6_addr);
		sess->addr.v6.sin6_port = htons(port);
		ret = bind(sess->s, (struct sockaddr*)&sess->addr.v6,  sizeof(sess->addr.v6) );

	}else{
		//
		// ipv4
		//
		sess->addr.v4.sin_family = AF_INET;
		sess->addr.v4.sin_addr.S_un.S_addr = inet_addr(address);
		sess->addr.v4.sin_port = htons(port);
		ret = bind(sess->s, (struct sockaddr*)&sess->addr.v4,  sizeof(sess->addr.v4) );

	}


	if(ret == SOCKET_ERROR){
		putErr("rgNet_addListner('%s', %u) -> bind() failed! %u\n", address, port, WSAGetLastError() );
		closesocket(sess->s);
		sess->s = INVALID_SOCKET;
		sess->lastAction = Time::tick_get();
		g_SessionPool->put(sess);

		return INVALID_CONNECTION;
	}


	//
	// Listen
	//
	if(listen(sess->s, (int)g_config_listenBacklog) == SOCKET_ERROR){
		putErr("rgNet_addListner('%s', %u) -> listen() failed! %u\n", address, port, WSAGetLastError() );
		closesocket(sess->s);
		sess->s = INVALID_SOCKET;
		sess->lastAction = Time::tick_get();
		g_SessionPool->put(sess);
		return INVALID_CONNECTION;		
	}


	// set values @ session.
	sess->ipv6 = ipv6;
	sess->type = rgSession::LISTENER;
	sess->nPendingIO = 0;
	sess->bClosePending = FALSE;
	sess->callback = onConnectProc;

	if(initialPacketTable & PACKETTABLE_CA) sess->table = LENTABLE_CA;
	else if(initialPacketTable & PACKETTABLE_CH) sess->table = LENTABLE_CH;
	else if(initialPacketTable & PACKETTABLE_CZ) sess->table = LENTABLE_CZ;
	else if(initialPacketTable & PACKETTABLE_ZC) sess->table = LENTABLE_ZC;
	else if(initialPacketTable & PACKETTABLE_ZH) sess->table = LENTABLE_ZH;
	else if(initialPacketTable & PACKETTABLE_OTHER) sess->table = LENTABLE_OTHER;
	else if(initialPacketTable & PACKETTABLE_HA) sess->table = LENTABLE_HA;
	else if(initialPacketTable & PACKETTABLE_AH) sess->table = LENTABLE_AH;
	else { putErr("rgNet_addListener() -> unknown initialPacketTable Type (%u)\n", initialPacketTable); }

	//
	// Make IOcp.
	//
	if( CreateIoCompletionPort((HANDLE)sess->s,  g_hCompletionPort,  sess->id,  0)  !=  g_hCompletionPort ){
		putErr("rgNet_addListner('%s', %u) -> createIoCompletionPort failed %u\n", address, port, GetLastError() );
		closesocket(sess->s);
		sess->s = INVALID_SOCKET;
		sess->lastAction = Time::tick_get();
		g_SessionPool->put(sess);
		return INVALID_CONNECTION;		
	}

	// add to active session array.
	g_session[ sess->id ] = sess;
	_WriteBarrier();

	// Make Pending Accept :)
	makePendingAccept(sess);


	// 
	putLog("rgNet_addListen('%s', %u) -> OK!\n", address, port);

	return sess->id;
}//end: rgNet_addlListener()


static DWORD WINAPI rgNet_workerThread(LPVOID /*param*/){
	DWORD dwBytesTransfered;
	unsigned __int64 ulKey;
	BOOL bRet;
	PendingIO *pio;
	rgSession *sess;


	while(1){ //@TODO: while(1) _ while(running) ? for termination?!
		bRet = GetQueuedCompletionStatus(g_hCompletionPort, &dwBytesTransfered, &ulKey, (LPOVERLAPPED*)&pio, INFINITE);

		if(bRet == FALSE){
			//
			// io Cancled (disconnect), or system error
			//
			if(pio == NULL){
				//
				// OS Error
				//
				putErr("GetQueuedCompletionStatus() -> got FALSE! Error: %u\n", GetLastError());
				Sleep(8);	//  not fine, but safer.
				continue;

			}else{
				//
				// Aborted IO Completion 
				//
				sess = pio->sess;
				
				InterlockedExchange64(&sess->bClosePending, TRUE);
				CancelIoEx((HANDLE)sess->s, NULL);
				InterlockedDecrement64(&sess->nPendingIO);

				
				// 
				// as the pio may have a netbuffer associated, check the type and release the ressources if needed!
				//
				if(pio->type == PendingIO::PT_WRITE){
					//
					// Write operations always ref a buffer.
					//
					netBuffer::put( pio->write.buf.buf );
					InterlockedDecrement64( &sess->nWritePending );
				
				}else if(pio->type == PendingIO::PT_READ){
					//
					// Read operations may ref a buffer (depends on state)
					//
					if(pio->read.netBuf != NULL)
						netBuffer::put( pio->read.netBuf );

				}else if(pio->type == PendingIO::PT_READRAW){
					//
					// Raw Read Operations may ref a buffer ..
					//
					if(pio->readraw.netBuf != NULL)
						netBuffer::put( pio->readraw.netBuf );

				}else if(pio->type == PendingIO::PT_CONNECT){
					//
					// Connect operation fail!
					//
					doHandleConnect( sess, (onConnectionEstablished)pio->connect.callback, false );

				}

				delete pio;

				doDisconnect(sess);

				continue;
			}

		}else{
			// 
			// Successfully dequeued an item
			//
			sess = pio->sess;

			switch(pio->type){
			case PendingIO::PT_READ:
				if(dwBytesTransfered != pio->read.buf.len){ 
					// 
					// Data didnt received completle, this can only happen if the connection gets interrupted 
					// (because we set MSG_WAITALL as recv flag)
					//
					if(pio->read.netBuf != NULL)
						netBuffer::put( pio->read.netBuf );

					delete pio;
					InterlockedExchange64(&sess->bClosePending, TRUE);
					CancelIoEx((HANDLE)sess->s, NULL);
					InterlockedDecrement64(&sess->nPendingIO);

					doDisconnect(sess);

					continue;

				}else{
					// 
					// Got data, call doRecv :) to handle it..
					//
					doRecv(pio);
					continue;
				}

				break;

			case PendingIO::PT_READRAW:
				//
				// Data received completly, this happens if the connection
				// gets interrupted 
				// (occures becauseof MSG_WAITALL as rcv flag)
				//
				if(dwBytesTransfered != pio->readraw.buf.len){
					if(pio->readraw.netBuf != NULL)
						netBuffer::put( pio->readraw.netBuf );

					delete pio;
					InterlockedExchange64(&sess->bClosePending, TRUE);
					CancelIoEx((HANDLE)sess->s, NULL);
					InterlockedDecrement64(&sess->nPendingIO);

					doDisconnect(sess);

					continue;

				}else{
					//
					// Raw Read Completed :)
					//
					doRecvRaw( pio );
					continue;
				}

				break;

			case PendingIO::PT_WRITE:
				EnterCriticalSection( &sess->wlLock );
				
				netBuffer::put( pio->write.buf.buf );	// return netbuffer
				

				if(dwBytesTransfered != pio->write.buf.len){
					//
					// didnt wrote data completly, this means connection interrupt during write!
					// dc.
					//			
					delete pio;
					InterlockedExchange64(&sess->bClosePending, TRUE);
					CancelIoEx((HANDLE)sess->s, NULL);
					InterlockedDecrement64(&sess->nPendingIO);
					InterlockedDecrement64( &sess->nWritePending );

					doDisconnect(sess);					

				}else{
					delete pio;

					if( InterlockedCompareExchange64( &sess->bClosePending, TRUE, TRUE) == TRUE){
						//
						// Close pending!
						// (dont requeue)
						//
						InterlockedDecrement64(&sess->nPendingIO);	
						InterlockedDecrement64( &sess->nWritePending );
						doDisconnect(sess); // because this was maybe the last 'waiting' io.

					}else{
						//
						// ?
						//
						pio = sess->wlBegin;
						if(pio != NULL){
							sess->wlBegin = pio->write.next;
							if(sess->wlBegin == NULL)
								sess->wlEnd = NULL;
	
							int ret = WSASend( sess->s,
												&pio->write.buf,
												1,
												&pio->write.dwBytesWritten,
												pio->write.dwWriteFlags,
												(LPOVERLAPPED)pio,
												NULL);

							if(ret == SOCKET_ERROR){
								if(WSAGetLastError() != ERROR_IO_PENDING){
									netBuffer::put( pio->write.buf.buf );
									delete pio;
									InterlockedExchange64(&sess->bClosePending, TRUE); // disconnect.
									CancelIoEx((HANDLE)sess->s, NULL);
									InterlockedDecrement64(&sess->nPendingIO);
									InterlockedDecrement64( &sess->nWritePending );
									doDisconnect(sess);
								}
							}

							//
							// we dont have to increment any counters
							// as they were still incremented by last send.
							//

						}else{
							//
							// nothing to write, so decrement nPendingIO for this connection by 1!
							//
							InterlockedDecrement64(&sess->nPendingIO);
							InterlockedDecrement64( &sess->nWritePending );
						}
					
					}


				}

				LeaveCriticalSection( &sess->wlLock ); // release connections write lock..


				break;

			////
			case PendingIO::PT_ACCEPT:
				doAccept(pio);
				delete pio;
				InterlockedDecrement64(&sess->nPendingIO);

				makePendingAccept(sess);

				break;


			////
			case PendingIO::PT_CONNECT:
				//
				// Outgoing connection established! :)
				//

				// dec 
				InterlockedDecrement64(&sess->nPendingIO);

				// After Decrement nPendingIO, so a doDisconnect would work...
				doHandleConnect( sess, (onConnectionEstablished)pio->connect.callback, true );
				

				// 
				delete pio;

				break;

			}//endswitch( type ) in ok 

		}//endif (iocp -> ok / failure)

	
	}//endwhile(1)


	return 0;
}//end: rgNet_workerThread()



static void rgNet_registerPacketSub(LENTABLE lenTable, WORD op, PACKETTYPE type, WORD len, onPacketCompleteCallback callback, bool overwriteExistingDefintion){
	struct packetLenTable *elem = &g_packetHandler[lenTable][op];

	if(overwriteExistingDefintion == false && elem->len != PACKETLEN_UNKNOWN){ // already registered, show warning!
		putWrn("rgNet_registerPacket(%s, 0x%04x) -> Packet already known! (Overwriting old definition)\n", LENTABLE_NAMES[lenTable], op);
	}

	//
	if(type == PACKETTYPE_FIXED){	
		//
		// Fixed Length Packet 
		//
		elem->len = len;

		// link a matching netbuffer allocation routine (for faster allocation :) 
		if(len <= 64)
			elem->getNetBufferProc = netBuffer::get_64B;
		else if(len <= 256)
			elem->getNetBufferProc = netBuffer::get_256B;
		else if(len <= 1024)
			elem->getNetBufferProc = netBuffer::get_1K;
		else if(len <= 2048)
			elem->getNetBufferProc = netBuffer::get_2K;
		else if(len <= 4096)
			elem->getNetBufferProc = netBuffer::get_4K;
		else if(len <= 8192)
			elem->getNetBufferProc = netBuffer::get_8K;
		else{
			//
			// ugly case, when this happens there should be a new pool added to core ...
			//
			putWrn("rgNet_registerPacket(%s, 0x%04x) -> Fixed Length Packet with length of %u Bytes, no BufferPool avaiable, consider to add a new Pool!\n", LENTABLE_NAMES[lenTable], op);
			
			elem->getNetBufferProc = netBuffer::get; // in this case, use the variable getter, which is also able to allocate non pool sizes..
		}

		elem->onComplete = callback;

	}else{
		//
		// Dynamic Length Packet
		//
		elem->len = PACKETLEN_VARIABLE;
		elem->getNetBufferProc = netBuffer::get;
		elem->onComplete = callback;

	}


}//end: rgNet_registerPacketSub()


void rgNet_registerPacketEx(WORD op, DWORD tableMask,  PACKETTYPE type,  WORD len,  onPacketCompleteCallback callback, bool overwriteExistingDefintion){

	if(tableMask & PACKETTABLE_CA)
		rgNet_registerPacketSub( LENTABLE_CA,  op,  type,  len,  callback, overwriteExistingDefintion );
	
	if(tableMask & PACKETTABLE_CH)
		rgNet_registerPacketSub( LENTABLE_CH,  op,  type,  len,  callback, overwriteExistingDefintion );

	if(tableMask & PACKETTABLE_CZ)
		rgNet_registerPacketSub( LENTABLE_CZ,  op,  type,  len,  callback,overwriteExistingDefintion );

	if(tableMask & PACKETTABLE_ZC)
		rgNet_registerPacketSub( LENTABLE_ZC,  op,  type,  len,  callback, overwriteExistingDefintion );

	if(tableMask & PACKETTABLE_ZH)
		rgNet_registerPacketSub( LENTABLE_ZH,  op,  type,  len,  callback, overwriteExistingDefintion );

	if(tableMask & PACKETTABLE_OTHER) 
		rgNet_registerPacketSub( LENTABLE_OTHER, op,	type,	len,	callback, overwriteExistingDefintion );

	if(tableMask & PACKETTABLE_HA) 
		rgNet_registerPacketSub( LENTABLE_HA, op,	type,	len,	callback, overwriteExistingDefintion );

	if(tableMask & PACKETTABLE_AH) 
		rgNet_registerPacketSub( LENTABLE_AH, op,	type,	len,	callback, overwriteExistingDefintion );

}//end: rgNet_registerPacket()


void rgNet_send(ConnectionIdentifier c, void *buffer, ULONG len){
	rgSession *sess = g_session[c];
	SOCKET s;
	bool bSendNeeded;

	_ReadBarrier();
#ifdef _DEBUG
	if(c < 0 || c >= g_config_maxConnections){
		putErr("net_send(): invalid connection id #%u\n", c);
		netBuffer::put( buffer );
		return;
	}
#endif

	if(sess == NULL){
		netBuffer::put( buffer );
		return;
	}

	if(InterlockedCompareExchange64(&sess->bWaitForRemoteDisconect, TRUE, TRUE) == TRUE){
		//
		// Connection waits for remote close, so no more packets are sent.
		//
		netBuffer::put( buffer ); 
		return;
	}


	s = sess->s;


	PendingIO *pio;
	pio = new PendingIO();
	pio->sess = sess;
	
	pio->type = PendingIO::PT_WRITE;
	pio->write.buf.buf = (char*)buffer;
	pio->write.buf.len = (ULONG)len;
	pio->write.next = NULL;
	pio->write.dwWriteFlags = 0;
	pio->write.dwBytesWritten = 0;

	// add to send list.
	EnterCriticalSection(&sess->wlLock);

	//
	// recheck -> connection valid?
	//
	if(sess->s != s){
		delete pio;
		netBuffer::put( buffer );
		LeaveCriticalSection(&sess->wlLock);
	}


	if(sess->wlBegin == NULL){
		//
		// first elem.
		//
		sess->wlBegin = pio;
		sess->wlEnd = pio;
		bSendNeeded = true;

	}else{
		//
		// Already elem's in list. just attach to the end, .. 
		//

		sess->wlEnd->write.next = pio;
		sess->wlEnd = pio;
		bSendNeeded = false;
	}

	LeaveCriticalSection(&sess->wlLock);

	//
	// Queue IO if needed. 
	// 
	if(bSendNeeded == true)
		makeSend( sess );		
	

}//end: rgNet_send()


ConnectionIdentifier rgNet_connect( const char *address,
									unsigned short port,
									const char *fromAddress,
									unsigned short fromPort,
									bool ipv6,
									PACKETTABLE initialPacketTable,
									onConnectionEstablished cbConnect
									)
{
	rgSession *sess;
	int namelen;

	//
	// @TODO:
	// check for connection count limit against builtin MAX_CONNECTION const!
	// to prevent core force shutdown due to max conn limit reach ..
	//

	sess = g_SessionPool->get();
	if(sess == NULL){
		putErr("rgNet_connect() -> connection limit reached - ingoring call!\n");
		return INVALID_CONNECTION;
	}


	// Fill Address field:
	sess->ipv6 = ipv6;
	if(ipv6 == true){
		// v6:
		namelen = sizeof(sockaddr_in6);
		memset(&sess->addr.v6, 0x00, namelen);

		if( inet_pton(AF_INET6, address, &sess->addr.v6.sin6_addr) != 1 ){
			putErr("rgNet_connect() -> '%s' (ipv6) inet_pton() failed! (address syntax error?) - ignoring call\n");
			g_SessionPool->put(sess);
			return INVALID_CONNECTION;
		}

		sess->addr.v6.sin6_family = AF_INET6;
		sess->addr.v6.sin6_port = htons(port);

	}else{
		// v4:		
		namelen = sizeof(sockaddr_in);

		sess->addr.v4.sin_addr.S_un.S_addr = inet_addr(address);
		sess->addr.v4.sin_family = AF_INET;
		sess->addr.v4.sin_port = htons(port);

	}

	////
	if(ipv6 == true)
		sess->s = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
	else
		sess->s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	// Socket creation failed :(
	if(sess->s == INVALID_SOCKET){
		putErr("rgNet_connect(%s, %u) -> socket() failed (Err: %u)\n", address, port, WSAGetLastError());
		g_SessionPool->put(sess);
		return INVALID_CONNECTION;		
	}


	///
	/// Bind 
	///
	if(ipv6 == true){
		// v6
		sockaddr_in6 local;
		memset(&local, 0x00, sizeof(local));
		local.sin6_family = AF_INET6;
		if( inet_pton(AF_INET6, fromAddress, &local.sin6_addr) != 1){
			closesocket(sess->s);
			putErr("rgNet_connect(%s, %u) -> parsing (v6) fromAddress failed! '%s'\n", address, port, fromAddress);
			g_SessionPool->put(sess);
			return INVALID_CONNECTION;
		}
			
		local.sin6_port = htons(fromPort);

		if(bind(sess->s, (sockaddr*)&local, sizeof(sockaddr_in6)) != 0){
			putErr("rgNet_connect(%s, %u) -> failed to bind (v6) 'from' address (%s, %u)\n", address, port, fromAddress, fromPort);
			closesocket(sess->s);
			g_SessionPool->put(sess);
			return INVALID_CONNECTION;
		}


	}else{
		// v4
		sockaddr_in local;
		memset(&local, 0x00, sizeof(local));

		local.sin_family = AF_INET;
		local.sin_addr.S_un.S_addr = inet_addr(fromAddress);
		local.sin_port = htons(fromPort);

		if(bind(sess->s, (sockaddr*)&local, sizeof(sockaddr_in)) != 0){
			putErr("rgNet_connect(%s, %u) -> failed to bind 'from' address (%s, %u)\n", address, port, fromAddress, fromPort);
			closesocket(sess->s);
			g_SessionPool->put(sess);
			return INVALID_CONNECTION;
		}

	}


	///
	sess->nPendingIO = 0;
	sess->nWritePending = 0;
	sess->bClosePending = FALSE;
	sess->bWaitForRemoteDisconect = FALSE;
	sess->callback = NULL;

	if(initialPacketTable & PACKETTABLE_CA) sess->table = LENTABLE_CA;
	else if(initialPacketTable & PACKETTABLE_CH) sess->table = LENTABLE_CH;
	else if(initialPacketTable & PACKETTABLE_CZ) sess->table = LENTABLE_CZ;
	else if(initialPacketTable & PACKETTABLE_ZC) sess->table = LENTABLE_ZC;
	else if(initialPacketTable & PACKETTABLE_ZH) sess->table = LENTABLE_ZH;
	else if(initialPacketTable & PACKETTABLE_OTHER) sess->table = LENTABLE_OTHER;
	else if(initialPacketTable & PACKETTABLE_HA) sess->table = LENTABLE_HA;
	else if(initialPacketTable & PACKETTABLE_AH) sess->table = LENTABLE_AH;
	else { putErr("rgNet_connect(%s,%u) -> unknown newPacketTable Type (%u)\n", address, port, initialPacketTable); }


	if(CreateIoCompletionPort( (HANDLE)sess->s, g_hCompletionPort, sess->id, 0) != g_hCompletionPort){
		putErr("rgNet_connect(%s,%u) -> failed to create iocp for new connection - dropping connection errno: %u\n", address, port, GetLastError());
		closesocket(sess->s);

		// return the sess entry
		sess->s = INVALID_SOCKET;
		sess->type = rgSession::DISCONNECTED;

		sess->lastAction = Time::tick_get();
		g_SessionPool->put(sess);

		return INVALID_CONNECTION;
	}


	//
	// Update STAT counters (as doDisconenct is used afterwards, which decrements the counters)
	//
	if(sess->ipv6 == true){
		// v6
		InterlockedIncrement64(&g_numV6Connections);
		if(g_numV6Connections > g_numPeakV6Connections){
			InterlockedExchange64(&g_numPeakV6Connections, g_numV6Connections);
		}

	}else{
		// v4
		InterlockedIncrement64(&g_numV4Connections);
		if(g_numV4Connections > g_numPeakV4Connections){
			InterlockedExchange64(&g_numPeakV4Connections, g_numV4Connections);
		}

	}

	//
	// assign to 'connected' table
	//
	g_session[ sess->id ] = sess; 
	_WriteBarrier();

	//
	//  Queue connect() call!
	//
	PendingIO *pio = new PendingIO();
	pio->type = PendingIO::PT_CONNECT;
	pio->sess = sess;
	
	pio->connect.callback = cbConnect;

	InterlockedIncrement64(&sess->nPendingIO);

	BOOL ret = connectEx(sess->s, (sockaddr*)&sess->addr, namelen, NULL, 0, NULL, (LPOVERLAPPED)pio );
	if(ret != TRUE){
		int err = WSAGetLastError();
		if(err != ERROR_IO_PENDING){
			//
			// 
			//
			putErr("rgNet_connect(%s, %u) -> WinSock Error: %u\n", address, port, err);

			delete pio;	// free pendingio slot
			InterlockedExchange64(&sess->bClosePending, TRUE); // disconnect.
			CancelIoEx((HANDLE)sess->s, NULL);
			InterlockedDecrement64(&sess->nPendingIO);

			doDisconnect(sess);

			return INVALID_CONNECTION;
		}
	}

	return sess->id;
}//end: rgNet_connect()


void rgNet_setOnDisconnect(ConnectionIdentifier c, onDisconectCallback onDisconnectProc){
	rgSession *sess = g_session[c];
	_ReadBarrier();
	if(sess == NULL)
		return;

	if(sess->type == rgSession::LISTENER)	// invalid operation for listener type sockets!
		return;

	sess->callback = onDisconnectProc;

}//end: rgNet_setOnDisconnect()


void rgNet_setPacketTable(ConnectionIdentifier c, PACKETTABLE newPacketTable){
	rgSession *sess = g_session[c];
	_ReadBarrier();
	if(sess == NULL)
		return;

	if(sess->type == rgSession::DISCONNECTED || sess->s == INVALID_SOCKET)	// call to already disconnected
		return;

	if(InterlockedCompareExchange64(&sess->bClosePending, TRUE, TRUE) == TRUE)
		return; // useless.. 

	if(InterlockedCompareExchange64(&sess->bWaitForOP, FALSE, FALSE) == FALSE){
		putErr("rgNet_setPacketTable() -> %u is not bWaitForOP.\n", c);
		return;
	}


	if(newPacketTable & PACKETTABLE_CA) sess->table = LENTABLE_CA;
	else if(newPacketTable & PACKETTABLE_CH) sess->table = LENTABLE_CH;
	else if(newPacketTable & PACKETTABLE_CZ) sess->table = LENTABLE_CZ;
	else if(newPacketTable & PACKETTABLE_ZC) sess->table = LENTABLE_ZC;
	else if(newPacketTable & PACKETTABLE_ZH) sess->table = LENTABLE_ZH;
	else { putErr("rgNet_setPacketTable() -> unknown newPacketTable Type (%u)\n", newPacketTable); }

}//end: rgNet_SetPacketTable()


void rgNet_disconnect(ConnectionIdentifier cid){
	rgSession *sess = g_session[cid];
	_ReadBarrier();
	if(sess == NULL)
		return;

	if(sess->type == rgSession::DISCONNECTED || sess->s == INVALID_SOCKET)	// call to already disconnected
		return;

	if(InterlockedCompareExchange64(&sess->bClosePending, TRUE, TRUE) == TRUE)
		return; // disconnect is pending, already

	if(sess->type == rgSession::LISTENER){
		putLog("rgNet_disconnect() -> called on Listener! [TESTING]\n");
	}

	InterlockedExchange64(&sess->bClosePending, TRUE); // disconnect.
	if(sess->s != INVALID_SOCKET)	// double check.
		CancelIoEx((HANDLE)sess->s, NULL);

	// When nothing is pending, this call could eventually disconnect the conneciton immediatly.
	doDisconnect(sess); 

}//end: rgNet_disconnect()


void rgNet_setWaitForRemoteDisconnect(ConnectionIdentifier cid){
	rgSession *sess = g_session[cid];
	_ReadBarrier();
	if(sess == NULL)
		return;

	if(sess->type == rgSession::DISCONNECTED || sess->type == rgSession::LISTENER || sess->s == INVALID_SOCKET)
		return;


	if(InterlockedCompareExchange64(&sess->bClosePending, TRUE, TRUE) == TRUE)
		return; // disconnect is pending, already

	InterlockedExchange64(&sess->bWaitForRemoteDisconect, TRUE);
	
}//end: rgNet_setWaitForRemoteDisconnect()


void rgNet_getIpStr(ConnectionIdentifier cid, char *destBuffer, size_t szDestBuffer){
	rgSession *sess = g_session[cid];
	_ReadBarrier();
	if(sess == NULL)
		return;

	if(InterlockedCompareExchange64(&sess->bClosePending, TRUE, TRUE) == TRUE){
		*destBuffer = '\0';
		return;
	}

	memset(destBuffer, 0x00, szDestBuffer);

	if(sess->ipv6 == true)
		inet_ntop(AF_INET6, &sess->addr.v6.sin6_addr, destBuffer, szDestBuffer);
	else
		inet_ntop(AF_INET, &sess->addr.v4.sin_addr, destBuffer, szDestBuffer);

}//end: rgNet_getIpStr()


DWORD rgNet_getIpV4dw(ConnectionIdentifier cid){
	rgSession *sess = g_session[cid];
	_ReadBarrier();
	if(sess == NULL)
		return 0;

	if(InterlockedCompareExchange64(&sess->bClosePending, TRUE, TRUE) == TRUE)
		return 0;

	if(sess->ipv6 == false)
		return sess->addr.v4.sin_addr.S_un.S_addr;

	return 0;
}//end: rgNet_getIpV4dw()


static void rgNet_onGCSession(rgSession *sess){
		// clean the elem.
		sess->type			= rgSession::DISCONNECTED;
		sess->lastAction	= 0;
		sess->nPendingIO	= 0;
		sess->nWritePending = 0;
		sess->callback		= NULL;
		sess->wlBegin		= NULL;
		sess->wlEnd			= NULL;
		sess->s				= INVALID_SOCKET;
		sess->bWaitForRemoteDisconect	= FALSE;
		sess->bClosePending	= FALSE;
		sess->bWaitForOP	= FALSE;
		sess->bRecvable		= FALSE;
}//end: rgNet_onGCSession()
						


void rgNet_setData(ConnectionIdentifier cid, void *data){
	rgSession *sess = g_session[cid];
	_ReadBarrier();
	if(sess == NULL)
		return;

	if(sess->type == rgSession::DISCONNECTED || sess->s == INVALID_SOCKET)
		return;


	if(InterlockedCompareExchange64(&sess->bClosePending, TRUE, TRUE) == TRUE)
		return; // pending disconnect - impossible to set data.

	//
	InterlockedExchangePointer(&sess->data, data);

}//end: rgNet_setData()



void *rgNet_getData(ConnectionIdentifier cid){
	rgSession *sess = g_session[cid];
	_ReadBarrier();
	if(sess == NULL)
		return NULL;

	if(sess->type == rgSession::DISCONNECTED || sess->s == INVALID_SOCKET)
		return NULL;


	if(InterlockedCompareExchange64(&sess->bClosePending, TRUE, TRUE) == TRUE)
		return NULL; // pending disconnect - impossible to get data.

	void *data = sess->data;
	_ReadBarrier();
	
	return data;
}//end: rgNet_getData()
