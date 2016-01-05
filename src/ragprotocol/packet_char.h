#pragma once
#pragma pack(push,1)

enum {
	HEADER_CH_ENTER = 0x65,
	HEADER_CH_SELECT_CHAR = 0x66,
	HEADER_CH_MAKE_CHAR = 0x67,
	HEADER_CH_DELETE_CHAR = 0x68,
	HEADER_CH_DELETE_CHAR2 = 0x1fb,
	HEADER_CH_EXE_HASHCHECK = 0x20b,
	HEADER_CH_ENTER2 = 0x275,
	HEADER_CH_SELECT_CHAR_GOINGTOBEUSED = 0x28c,
	HEADER_CH_REQ_IS_VALID_CHARNAME = 0x28d,
	HEADER_CH_REQ_CHANGE_CHARNAME = 0x28f,
	HEADER_CH_ENTER_CHECKBOT = 0x7e5,
	HEADER_CH_CHECKBOT = 0x7e7,
	HEADER_CH_DELETE_CHAR3_RESERVED = 0x827,
	HEADER_CH_DELETE_CHAR3 = 0x829,
	HEADER_CH_DELETE_CHAR3_CANCEL = 0x82b,
	HEADER_CH_SELECT_ACCESSIBLE_MAPNAME = 0x841,
	HEADER_CH_WAITING_LOGIN = 0x8b0,
	HEADER_CH_SECOND_PASSWD_ACK = 0x8b8,
	HEADER_CH_MAKE_SECOND_PASSWD = 0x8ba,
	HEADER_CH_DELETE_SECOND_PASSWD = 0x8bc,
	HEADER_CH_EDIT_SECOND_PASSWD = 0x8be,
	HEADER_CH_NOT_AVAILABLE_SECOND_PASSWD = 0x8c3,
	HEADER_CH_AVAILABLE_SECOND_PASSWD = 0x8c5,
	HEADER_CH_REQ_CHANGE_CHARACTER_SLOT = 0x8d4,
	HEADER_CH_REQ_CHANGE_CHARACTERNAME = 0x8fc,
	HEADER_CH_ACK_CHANGE_CHARACTERNAME = 0x8fd,
	HEADER_CH_MAKE_CHAR_NOT_STATS = 0x970,
	HEADER_HC_ACCEPT_ENTER = 0x6b,
	HEADER_HC_REFUSE_ENTER = 0x6c,
	HEADER_HC_ACCEPT_MAKECHAR = 0x6d,
	HEADER_HC_REFUSE_MAKECHAR = 0x6e,
	HEADER_HC_ACCEPT_DELETECHAR = 0x6f,
	HEADER_HC_REFUSE_DELETECHAR = 0x70,
	HEADER_HC_NOTIFY_ZONESVR = 0x71,
	HEADER_HC_BLOCK_CHARACTER = 0x20d,
	HEADER_HC_REQUEST_CHARACTER_PASSWORD = 0x23e,
	HEADER_HC_CHARNOTBEENSELECTED = 0x28b,
	HEADER_HC_ACK_IS_VALID_CHARNAME = 0x28e,
	HEADER_HC_ACK_CHANGE_CHARNAME = 0x290,
	HEADER_HC_REFUSE_SELECTCHAR = 0x2ca,
	HEADER_HC_CHARACTER_LIST = 0x448,
	HEADER_HC_CHECKBOT = 0x7e8,
	HEADER_HC_CHECKBOT_RESULT = 0x7e9,
	HEADER_HC_DELETE_CHAR3_RESERVED = 0x828,
	HEADER_HC_DELETE_CHAR3 = 0x82a,
	HEADER_HC_DELETE_CHAR3_CANCEL = 0x82c,
	HEADER_HC_ACCEPT_ENTER2 = 0x82d,
	HEADER_HC_NOTIFY_ACCESSIBLE_MAPNAME = 0x840,
	HEADER_HC_WAITING_LOGIN = 0x8af,
	HEADER_HC_SECOND_PASSWD_REQ = 0x8b7,
	HEADER_HC_SECOND_PASSWD_LOGIN = 0x8b9,
	HEADER_HC_MAKE_SECOND_PASSWD = 0x8bb,
	HEADER_HC_DELETE_SECOND_PASSWD = 0x8bd,
	HEADER_HC_EDIT_SECOND_PASSWD = 0x8bf,
	HEADER_HC_NOT_AVAILABLE_SECOND_PASSWD = 0x8c4,
	HEADER_HC_AVAILABLE_SECOND_PASSWD = 0x8c6,
	HEADER_HC_ACK_CHANGE_CHARACTER_SLOT = 0x8d5,
	HEADER_HC_UPDATE_CHARINFO = 0x8e3,
};



typedef struct CHARACTER_LIST {
	unsigned long dwGID;
	unsigned char SlotIdx;
} CHARACTER_LIST;




// C->H

typedef struct PACKET_CH_DELETE_CHAR3_EXT {
	short PacketType;
	short PacketLength;
	unsigned long GID;
} PACKET_CH_DELETE_CHAR3_EXT;

typedef struct PACKET_CH_DELETE_CHAR3 {
	short PacketType;
	unsigned long GID;
	char Birth[6];
} PACKET_CH_DELETE_CHAR3;

typedef struct PACKET_CH_REQ_IS_VALID_CHARNAME {
	short PacketType;
	unsigned long dwAID;
	unsigned long dwGID;
	char szCharName[24];
} PACKET_CH_REQ_IS_VALID_CHARNAME;

typedef struct PACKET_CH_DELETE_CHAR3_RESERVED {
	short PacketType;
	unsigned long GID;
} PACKET_CH_DELETE_CHAR3_RESERVED;

typedef struct PACKET_CH_MAKE_SECOND_PASSWD {
	short PacketType;
	unsigned long AID;
	char SecondPWIdx[4];
} PACKET_CH_MAKE_SECOND_PASSWD;

typedef struct PACKET_CH_DELETE_CHAR {
	short PacketType;
	unsigned long GID;
	char key[40];
} PACKET_CH_DELETE_CHAR;

typedef struct PACKET_CH_WAITING_LOGIN {
	short PacketType;
	unsigned long AID;
	int AuthCode;
	unsigned long userLevel;
	short clientType;
	unsigned char Sex;
} PACKET_CH_WAITING_LOGIN;

typedef struct PACKET_CH_MAKE_CHAR_NOT_STATS {
	short PacketType;
	unsigned char name[24];
	unsigned char CharNum;
	short headPal;
	short head;
} PACKET_CH_MAKE_CHAR_NOT_STATS;

typedef struct PACKET_CH_DELETE_CHAR3_CANCEL {
	short PacketType;
	unsigned long GID;
} PACKET_CH_DELETE_CHAR3_CANCEL;

typedef struct PACKET_CH_EDIT_SECOND_PASSWD {
	short PacketType;
	unsigned long AID;
	char oldSecondPWIdx[4];
	char newSecondPWIdx[4];
} PACKET_CH_EDIT_SECOND_PASSWD;

typedef struct PACKET_CH_AVAILABLE_SECOND_PASSWD {
	short PacketType;
	unsigned long AID;
} PACKET_CH_AVAILABLE_SECOND_PASSWD;

typedef struct PACKET_CH_REQ_CHARINFO_PER_PAGE {
	short PacketType;
	unsigned long SeqNum;
} PACKET_CH_REQ_CHARINFO_PER_PAGE;

typedef struct PACKET_CH_CHARLIST_REQ {
	short PacketType;
} PACKET_CH_CHARLIST_REQ;

typedef struct PACKET_CH_ENTER {
	short PacketType;
	unsigned long AID;
	int AuthCode;
	unsigned long userLevel;
	short clientType;
	unsigned char Sex;
} PACKET_CH_ENTER;

typedef struct PACKET_CH_ENTER2 {
	short PacketType;
	unsigned long AID;
	int AuthCode;
	unsigned long userLevel;
	short clientType;
	unsigned char Sex;
	char macData[16];
	int iAccountSID;
} PACKET_CH_ENTER2;

typedef struct PACKET_CH_DELETE_CHAR2 {
	short PacketType;
	unsigned long GID;
	char key[50];
} PACKET_CH_DELETE_CHAR2;

typedef struct PACKET_CH_ENTER_CHECKBOT {
	short PacketType;
	short PacketLength;
} PACKET_CH_ENTER_CHECKBOT;

typedef struct PACKET_CH_REQ_CHANGE_CHARNAME {
	short PacketType;
	unsigned long dwGID;
} PACKET_CH_REQ_CHANGE_CHARNAME;

typedef struct PACKET_CH_DELETE_SECOND_PASSWD {
	short PacketType;
	unsigned long AID;
	char SecondPWIdx[4];
} PACKET_CH_DELETE_SECOND_PASSWD;

typedef struct PACKET_CH_SELECT_CHAR {
	short PacketType;
	unsigned char CharNum;
} PACKET_CH_SELECT_CHAR;

typedef struct PACKET_CH_REQ_CHANGE_CHARACTER_SLOT {
	short PacketType;
	short beforeCharNum;
	short AfterCharNum;
	short CurChrSlotCnt;
} PACKET_CH_REQ_CHANGE_CHARACTER_SLOT;

typedef struct PACKET_CH_SELECT_CHAR_GOINGTOBEUSED {
	short PacketType;
	unsigned long dwAID;
	int nCountSelectedChar;
	unsigned long ardwSelectedGID[9];
} PACKET_CH_SELECT_CHAR_GOINGTOBEUSED;

typedef struct PACKET_CH_MAKE_CHAR {
	short PacketType;
	unsigned char name[24];
	unsigned char Str;
	unsigned char Agi;
	unsigned char Vit;
	unsigned char Int;
	unsigned char Dex;
	unsigned char Luk;
	unsigned char CharNum;
	short headPal;
	short head;
} PACKET_CH_MAKE_CHAR;

typedef struct PACKET_CH_CHECKBOT {
	short PacketType;
	short PacketLength;
	unsigned long dwAID;
	char szStringInfo[24];
} PACKET_CH_CHECKBOT;

typedef struct PACKET_CH_NOT_AVAILABLE_SECOND_PASSWD {
	short PacketType;
	unsigned long AID;
	char SecondPWIdx[4];
} PACKET_CH_NOT_AVAILABLE_SECOND_PASSWD;

typedef struct PACKET_CH_EXE_HASHCHECK {
	short PacketType;
	unsigned char ClientType;
	unsigned char HashValue[16];
} PACKET_CH_EXE_HASHCHECK;

typedef struct PACKET_CH_REQ_CHANGE_CHARACTERNAME {
	short PacketType;
	unsigned long dwGID;
	char szCharName[24];
} PACKET_CH_REQ_CHANGE_CHARACTERNAME;

typedef struct PACKET_CH_SELECT_ACCESSIBLE_MAPNAME {
	short PacketType;
	unsigned char CharNum;
	unsigned char mapListNum;
} PACKET_CH_SELECT_ACCESSIBLE_MAPNAME;

typedef struct PACKET_CH_SECOND_PASSWD_ACK {
	short PacketType;
	unsigned long AID;
	char SecondPWIdx[4];
} PACKET_CH_SECOND_PASSWD_ACK;





// H->C

typedef struct PACKET_HC_ACCEPT_MAKECHAR_NEO_UNION {
	short PacketType;
	CHARACTER_INFO_NEO_UNION charinfo;
} PACKET_HC_ACCEPT_MAKECHAR_NEO_UNION;

typedef struct PACKET_HC_DELETE_CHAR3 {
	short PacketType;
	unsigned long GID;
	int Result;
	int code;
} PACKET_HC_DELETE_CHAR3;

typedef struct PACKET_HC_DELETE_CHAR3_CANCEL {
	short PacketType;
	unsigned long GID;
	int Result;
	int code;
} PACKET_HC_DELETE_CHAR3_CANCEL;

typedef struct PACKET_HC_NOTIFY_ACCESSIBLE_MAPNAME {
	short PacketType;
	short PacketLength;
} PACKET_HC_NOTIFY_ACCESSIBLE_MAPNAME;

typedef struct PACKET_HC_WAITING_LOGIN {
	short PacketType;
	unsigned long AID;
	int CurWaitingNum;
} PACKET_HC_WAITING_LOGIN;

typedef struct PACKET_HC_ACK_CHANGE_CHARACTERNAME {
	short PacketType;
	unsigned long dwResult;
} PACKET_HC_ACK_CHANGE_CHARACTERNAME;

typedef struct PACKET_HC_CHECKBOT_RESULT {
	short PacketType;
	short PacketLength;
	unsigned char Resut;
} PACKET_HC_CHECKBOT_RESULT;

typedef struct PACKET_HC_ACK_CHANGE_CHARNAME {
	short PacketType;
	short sResult;
} PACKET_HC_ACK_CHANGE_CHARNAME;

typedef struct PACKET_HC_REFUSE_MAKECHAR {
	short PacketType;
	unsigned char ErrorCode;
	enum REFUSEMAKECHAR_ERRORCODE {
		REFUSEMAKECHAR_NAME_TAKEN = 0x00,
		REFUSEMAKECHAR_UNDERAGED = 0x01,
		REFUSEMAKECHAR_NO_PERMISSION_TO_USE_SLOT = 0x03,
		REFUSEMAKECHAR_REFUSED = 0x04
	};
} PACKET_HC_REFUSE_MAKECHAR;

typedef struct PACKET_HC_REFUSE_ENTER {
	short PacketType;
	unsigned char ErrorCode;
} PACKET_HC_REFUSE_ENTER;

typedef struct PACKET_HC_ACK_CHANGE_CHARACTER_SLOT {
	short PacketType;
	short PacketLength;
	short Reason;
	short AfterChrSlotCnt;
} PACKET_HC_ACK_CHANGE_CHARACTER_SLOT;

typedef struct PACKET_HC_UPDATE_CHARINFO {
	short PacketType;
	CHARACTER_INFO charinfo;
} PACKET_HC_UPDATE_CHARINFO;

typedef struct PACKET_HC_QUEUE_ORDER {
	short PacketType;
	short PacketLength;
	unsigned long m_AID;
	unsigned long m_QueueOrder;
} PACKET_HC_QUEUE_ORDER;

typedef struct PACKET_HC_ACCEPT_ENTER_ORG {
	short PacketType;
	short PacketLength;
	char m_extension[20];
} PACKET_HC_ACCEPT_ENTER_ORG;

typedef struct PACKET_HC_CHARACTER_LIST {
	short PacketType;
	short PacketLength;
} PACKET_HC_CHARACTER_LIST;

typedef struct PACKET_HC_BLOCK_CHARACTER {
	short PacketType;
	short PacketLength;
} PACKET_HC_BLOCK_CHARACTER;

typedef struct PACKET_HC_ACCEPT_ENTER_BILL_EXT {
	short PacketType;
	short PacketLength;
	char m_extension[20];
} PACKET_HC_ACCEPT_ENTER_BILL_EXT;

typedef struct PACKET_HC_ACCEPT_ENTER_FRANCE {
	short PacketType;
	short PacketLength;
	short wExtInfo;
} PACKET_HC_ACCEPT_ENTER_FRANCE;

typedef struct PACKET_HC_ACCEPT_DELETECHAR {
	short PacketType;
} PACKET_HC_ACCEPT_DELETECHAR;

typedef struct PACKET_HC_ACCEPT_ENTER {
	short PacketType;
	short PacketLength;
} PACKET_HC_ACCEPT_ENTER;

typedef struct PACKET_HC_ACK_CHARINFO_PER_PAGE {
	short PacketType;
	short PacketLength;
} PACKET_HC_ACK_CHARINFO_PER_PAGE;

typedef struct PACKET_HC_ACCEPT_MAKECHAR_UNION {
	short PacketType;
	CHARACTER_INFO_UNION charinfo;
} PACKET_HC_ACCEPT_MAKECHAR_UNION;

typedef struct PACKET_HC_REFUSE_SELECTCHAR {
	short PacketType;
	unsigned char ErrorCode;
} PACKET_HC_REFUSE_SELECTCHAR;

typedef struct PACKET_HC_AVAILABLE_SECOND_PASSWD {
	short PacketType;
	short Result;
} PACKET_HC_AVAILABLE_SECOND_PASSWD;

typedef struct PACKET_HC_ACK_IS_VALID_CHARNAME {
	short PacketType;
	short sResult;
} PACKET_HC_ACK_IS_VALID_CHARNAME;

typedef struct PACKET_HC_ACCEPT_MAKECHAR {
	short PacketType;
	CHARACTER_INFO charinfo;
} PACKET_HC_ACCEPT_MAKECHAR;

typedef struct PACKET_HC_DELETE_SECOND_PASSWD {
	short PacketType;
	short Result;
	unsigned long Seed;
} PACKET_HC_DELETE_SECOND_PASSWD;

typedef struct PACKET_HC_ACCEPT_ENTER2 {
	short PacketType;
	short PacketLength;
	unsigned char NormalSlotNum;
	unsigned char PremiumSlotNum;
	unsigned char BillingSlotNum;
	unsigned char ProducibleSlotNum;
	unsigned char ValidSlotNum;
	char m_extension[20];
} PACKET_HC_ACCEPT_ENTER2;

typedef struct PACKET_HC_CHECKBOT {
	short PacketType;
	short PacketLength;
} PACKET_HC_CHECKBOT;

typedef struct PACKET_HC_ACCEPT_ENTER_NEO {
	short PacketType;
	short PacketLength;
	unsigned char TotalSlotNum;
	unsigned char PremiumStartSlot;
	unsigned char PremiumEndSlot;
} PACKET_HC_ACCEPT_ENTER_NEO;

typedef struct PACKET_HC_MAKE_SECOND_PASSWD {
	short PacketType;
	short Result;
	unsigned long Seed;
} PACKET_HC_MAKE_SECOND_PASSWD;

typedef struct PACKET_HC_CHARLIST_NOTIFY {
	short PacketType;
	int TotalCnt;
} PACKET_HC_CHARLIST_NOTIFY;

typedef struct PACKET_HC_ACCEPT_MAKECHAR_NEO {
	short PacketType;
	CHARACTER_INFO_NEO charinfo;
} PACKET_HC_ACCEPT_MAKECHAR_NEO;

typedef struct ZSERVER_ADDR {
	unsigned long ip;
	short port;
} ZSERVER_ADDR;

typedef struct PACKET_HC_NOTIFY_ZONESVR {
	short PacketType;
	unsigned long GID;
	unsigned char mapName[16];
	ZSERVER_ADDR addr;
} PACKET_HC_NOTIFY_ZONESVR;

typedef struct PACKET_HC_REFUSE_DELETECHAR {
	short PacketType;
	unsigned char ErrorCode;
	enum REFUSEDELETECHAR_ERRORCODE {
		REFUSEDELETECHAR_INCORRECT_SECRET = 0x00,
		REFUSEDELETECHAR_CANT_BE_DELETED = 0x01,
		REFUSEDELETECHAR_DISBAND_FROM_PARTY_AND_GUILD_BEFORE = 0x02,
		REFUSEDELETECHAR_DENIED = 0x03
	};
} PACKET_HC_REFUSE_DELETECHAR;

typedef struct PACKET_HC_DELETE_CHAR3_RESERVED {
	short PacketType;
	unsigned long GID;
	int Result;
	long DeleteReservedDate;
	int code;
} PACKET_HC_DELETE_CHAR3_RESERVED;

typedef struct PACKET_HC_NOT_AVAILABLE_SECOND_PASSWD {
	short PacketType;
	short Result;
	unsigned long Seed;
} PACKET_HC_NOT_AVAILABLE_SECOND_PASSWD;

typedef struct PACKET_HC_EDIT_SECOND_PASSWD {
	short PacketType;
	short Result;
	unsigned long Seed;
} PACKET_HC_EDIT_SECOND_PASSWD;

typedef struct PACKET_HC_SECRETSCAN_DATA {
	short PacketType;
	short PacketLength;
} PACKET_HC_SECRETSCAN_DATA;

typedef struct PACKET_HC_REQUEST_CHARACTER_PASSWORD {
	short PacketType;
	short Result;
	unsigned long dummyValue;
} PACKET_HC_REQUEST_CHARACTER_PASSWORD;

typedef struct PACKET_HC_SECOND_PASSWD_LOGIN {
	short PacketType;
	unsigned long Seed;
	unsigned long AID;
	short Result;
} PACKET_HC_SECOND_PASSWD_LOGIN;

#pragma pack(pop)
