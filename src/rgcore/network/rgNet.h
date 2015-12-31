#pragma once


void rgNet_init();
void rgNet_final();

/* Stats / Counters of Session Pool (= Connections) */
void rgNet_getSessionCounters(size_t *numV4, size_t *numV6, size_t *peakV4, size_t *peakV6);


typedef size_t ConnectionIdentifier;
#define INVALID_CONNECTION -1


// When adding a new table:
//  you'll have to add it also to INTPACKETLENTABLE enum @ rgNet.cpp
//	and add it to the rgNet_registerPacket() function!
//
typedef enum PACKETTABLE {
		PACKETTABLE_NONE = 0,			//
		PACKETTABLE_CA = ( 1 << 0 ),	//	Client		->		Account Server 
		PACKETTABLE_CH = ( 1 << 1 ),	//	Client		->		Character Server
		PACKETTABLE_CZ = ( 1 << 2 ),	//	Client		->		Zone Server
		PACKETTABLE_ZC = ( 1 << 3 ),	//	Zone Server	->		Client	
		PACKETTABLE_ZH = ( 1 << 4 ),	//	Zone Server	->		Character Server
		PACKETTABLE_OTHER = ( 1 << 5),
		PACKETTABLE_HA = ( 1 << 6),		// Character Server -> Account Server
		PACKETTABLE_AH = ( 1 << 7),		// Account Server -> Character Server
		PACKETTABLE_MAX
} PACKETTABLE;

typedef enum PACKETTYPE {
	PACKETTYPE_VARIABLE	= 0x0,
	PACKETTYPE_FIXED	= 0x1,
	PACKETTYPE_UNKNOWN	= 0xff,
} PACKETTYPE;

/* parameters: connectionID,  opcode,  len,  buffer */
typedef void (*onPacketCompleteCallback)(ConnectionIdentifier, char *);
typedef bool (*onConnectCallback)(ConnectionIdentifier);	// false disconnects / rejects the connection.
typedef void (*onDisconectCallback)(ConnectionIdentifier, void*); // 2nd: per-session data
typedef void (*onConnectionEstablished)(ConnectionIdentifier, bool); // 2nd: false -> failed to connect.
typedef void (*onRawReadCompleteCallback)(ConnectionIdentifier cid, size_t len, char *buf); // Callback type for rawRead() operation.

#define rgNet_registerPacket(op, tableMask, type, len, callback) rgNet_registerPacketEx(op, tableMask, type, len, callback, false)
void rgNet_registerPacketEx(WORD op,
							DWORD tableMask, // mask of PACKETTABLE_xx elements.
							PACKETTYPE	type,
							WORD len, // used if type == PACKETTYPE_FIXED
							onPacketCompleteCallback callback,
							bool overWriteExistingDefinition // if set to true, doesnt complain about already registered definition / simply overwrite it silently.
							);




/* Sends contents of a netBuffer, after completion, returns the netBuffer to Pool */
void rgNet_send(ConnectionIdentifier c, void *buffer, ULONG len);	// Note: you'll have to increase the netbuffers refcnt!, net_send doesnt do this for you.


void rgNet_setOnDisconnect(ConnectionIdentifier c, onDisconectCallback onDisconnectProc);

/* Sets the Packet Table to lookup completed packets @ packet table */
void rgNet_setPacketTable(ConnectionIdentifier c, PACKETTABLE newPacketTable);	// Note: ONLY do this in 'onPacketCompleteCallback' procs! otherwise this can end up in chaos/crash!!


// Creates a new TCP listener
ConnectionIdentifier rgNet_addListener(	const char *address,			// Address
										unsigned short	port,			// port
										bool ipv6,						// Ipv6 Listener? (address must point to a valid ipv6 address)
										PACKETTABLE	initialPacketTable,	// New Connections will get this packetTable after successfull connection
										onConnectCallback onConnectProc
										);



/* Disconnects the Given Connection */
void rgNet_disconnect(ConnectionIdentifier cid);

/* Sets connection to 'wait for remote close' mode,  in this state a recieved packet would result in instant disconnect by dispatcher side,  and send's are skipped */
void rgNet_setWaitForRemoteDisconnect(ConnectionIdentifier cid);


/* Gets the ip in human-readable form, destBuffer should have 40 bytes to be able to receive a full ipv6 address, total bytes written are returned */
void rgNet_getIpStr(ConnectionIdentifier cid, char *destBuffer, size_t szDestBuffer);

/* Gets the IP (v4!) address as DWORD */
DWORD rgNet_getIpV4dw(ConnectionIdentifier cid);

/**  
 * Enqueue RAW read (bypass ro parser)
 * Note: this call will implicitly call rgNet_disableRecv()
 * to disable the default ro-protocol parser
 * You'll have to enable ro-protocol parsing (if needed) manually by
 * calling makeRecv() - Typically in callback of rawRead ..
 *
 * Important note: this function could only be called (useful) in packet completion routines
 * otherwise it may have no effect!
 * ( Same behaviour as applies to rgNet_disableRecv() )
 */
void rgNet_rawRead(ConnectionIdentifier cid, size_t len, char *destNetBuffer, onRawReadCompleteCallback onCompletion);


/* Will Disable Pending Recv,
 * Note: This call can only be issued in Incomming Packet Handler! 
 * Useful when processing tasks async, and you want to ensure that 
 * no other handler can be issued while async task is running.
 */
void rgNet_disableRecv(ConnectionIdentifier cid);


/* Will Queue a Pending Recv
 * Used when disabled Recving with disableRecv()!
 */
void rgNet_makeRecv(ConnectionIdentifier cid);


/* Gets the Highest Possible Connection ID (MaxConnecitons is returnvalue + 1) */
ConnectionIdentifier rgNet_getMaxConnectionID();


// Establishs a new connection
ConnectionIdentifier rgNet_connect( const char *address,
									unsigned short port,
									const char *fromAddress,
									unsigned short fromPort,
									bool ipv6,
									PACKETTABLE initialPacketTable,
									onConnectionEstablished cbConnect
									);



//
// Per Session - Application Data Pointer -
// Threadsafe using interlocked operations, 
// also checks for 'connected' state etc etc ..
// 
// Note: 
//  a local lookup array 'may' be faster in some cases.
//


/** 
 * Receive's the per-session applicaiton data pointer
 * Note: thread safe.
 */
void *rgNet_getData(ConnectionIdentifier cid);


/**
 * Sets the per-session application data pointer
 * Note: thread Safe.
 */
void rgNet_setData(ConnectionIdentifier cid, void *data);
