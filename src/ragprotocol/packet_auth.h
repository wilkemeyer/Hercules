#pragma once
#pragma pack(push, 1)

enum {
	HEADER_CA_LOGIN = 0x64,
	HEADER_CA_REPLY_PNGAMEROOM = 0x1bf,
	HEADER_CA_REQ_HASH = 0x1db,
	HEADER_CA_LOGIN2 = 0x1dd,
	HEADER_CA_LOGIN3 = 0x1fa,
	HEADER_CA_CONNECT_INFO_CHANGED = 0x200,
	HEADER_CA_EXE_HASHCHECK = 0x204,
	HEADER_CA_REQ_GAME_GUARD_CHECK = 0x258,
	HEADER_CA_ACK_LOGIN_OLDEKEY = 0x264,
	HEADER_CA_ACK_LOGIN_NEWEKEY = 0x265,
	HEADER_CA_ACK_LOGIN_CARDPASS = 0x266,
	HEADER_CA_ACK_LOGIN_ACCOUNT_INFO = 0x271,
	HEADER_CA_LOGIN_PCBANG = 0x277,
	HEADER_CA_LOGIN4 = 0x27c,
	HEADER_CA_CLIENT_TYPE = 0x27f,
	HEADER_CA_LOGIN_CHANNEL = 0x2b0,
	HEADER_CAH_ACK_GAME_GUARD = 0x3de,
	HEADER_CA_OTP_AUTH_REQ = 0x822,
	HEADER_CA_SSO_LOGIN_REQ = 0x825,
	HEADER_CA_LOGIN5 = 0x8cc,
	HEADER_AC_ACCEPT_LOGIN = 0x69,
	HEADER_AC_REFUSE_LOGIN = 0x6a,
	HEADER_AC_ASK_PNGAMEROOM = 0x1be,
	HEADER_AC_ACK_HASH = 0x1dc,
	HEADER_AC_NOTIFY_ERROR = 0x1f1,
	HEADER_AC_EVENT_RESULT = 0x23d,
	HEADER_AC_ACK_GAME_GUARD = 0x259,
	HEADER_AC_REQ_LOGIN_OLDEKEY = 0x261,
	HEADER_AC_REQ_LOGIN_NEWEKEY = 0x262,
	HEADER_AC_REQ_LOGIN_CARDPASS = 0x263,
	HEADER_AC_ACK_EKEY_FAIL_NOTEXIST = 0x267,
	HEADER_AC_ACK_EKEY_FAIL_NOTUSESEKEY = 0x268,
	HEADER_AC_ACK_EKEY_FAIL_NOTUSEDEKEY = 0x269,
	HEADER_AC_ACK_EKEY_FAIL_AUTHREFUSE = 0x26a,
	HEADER_AC_ACK_EKEY_FAIL_INPUTEKEY = 0x26b,
	HEADER_AC_ACK_EKEY_FAIL_NOTICE = 0x26c,
	HEADER_AC_ACK_EKEY_FAIL_NEEDCARDPASS = 0x26d,
	HEADER_AC_ACK_AUTHEKEY_FAIL_NOTMATCHCARDPASS = 0x26e,
	HEADER_AC_ACK_FIRST_LOGIN = 0x26f,
	HEADER_AC_REQ_LOGIN_ACCOUNT_INFO = 0x270,
	HEADER_AC_ACK_PT_ID_INFO = 0x272,
	HEADER_AC_ACCEPT_LOGIN2 = 0x276,
	HEADER_AC_REQUEST_SECOND_PASSWORD = 0x2ad,
	HEADER_AC_OTP_USER = 0x821,
	HEADER_AC_OTP_AUTH_ACK = 0x823,
	HEADER_AC_SSO_LOGIN_ACK = 0x826,
	HEADER_AC_REFUSE_LOGIN_R2 = 0x83e,
	HEADER_AC_REALNAME_AUTH = 0x8b2,
	HEADER_AC_SHUTDOWN_INFO = 0x8e4
};




// C -> A

typedef struct PACKET_CA_LOGIN {
	short			PacketType;
	unsigned int	Version;
	unsigned char	ID[0x18];
	unsigned char	Passwd[0x18];
	unsigned char	clienttype;

	// ClienType may be one of the following:
	enum CLIENTTYPE {
		CLIENTTYPE_KOREAN = 0x0,
		CLIENTTYPE_ENGLISH = 0x1,
		CLIENTTYPE_SAKRAY = 0x2,
		CLIENTTYPE_JAPAN = 0x3,
		CLIENTTYPE_CHINA = 0x4,
		CLIENTTYPE_TAIWAN = 0x5,
		CLIENTTYPE_HONGKONG = 0x6,
		CLIENTTYPE_THAI = 0x7,
		CLIENTTYPE_LOCAL = 0x8,
		CLIENTTYPE_JAPAN_SAKRAY = 0x9,
		CLIENTTYPE_THAI_SAKRAY = 0xa,
		CLIENTTYPE_TAIWAN_SAKRAY = 0xb,
		CLIENTTYPE_INDONESIA = 0xc,
		CLIENTTYPE_INDONESIA_SAKRAY = 0xd,
		CLIENTTYPE_ENGLISH_SAKRAY = 0xe,
		CLIENTTYPE_PHILIPPINE = 0xf,
		CLIENTTYPE_MALAYSIA = 0x10,
		CLIENTTYPE_SINGAPORE = 0x11,
		CLIENTTYPE_PHILIPPINE_SAKRAY = 0x12,
		CLIENTTYPE_THAI_FREE = 0x13,
		CLIENTTYPE_GERMANY = 0x14,
		CLIENTTYPE_INDIA = 0x15,
		CLIENTTYPE_BRAZIL = 0x16,
		CLIENTTYPE_AUSTRALIA = 0x17,
		CLIENTTYPE_KOREAN_PK = 0x18,
		CLIENTTYPE_RUSSIA = 0x19,
		CLIENTTYPE_VIETNAM = 0x1a,
		CLIENTTYPE_PHILIPPINE_PK = 0x1b,
		CLIENTTYPE_JAPAN_PK = 0x1c,
		CLIENTTYPE_THAI_PK = 0x1d,
		CLIENTTYPE_CHILE = 0x1e,
		CLIENTTYPE_FRANCE = 0x1f,
		CLIENTTYPE_VIETNAM_PK = 0x20,
		CLIENTTYPE_VIETNAM_SAKRAY = 0x21,
		CLIENTTYPE_INDONESIA_PK = 0x22,
		CLIENTTYPE_UAE = 0x23,
		MAX_CLIENTTYPE = 0x24
	};
} PACKET_CA_LOGIN;

typedef struct PACKET_CA_LOGIN_PCBANG {
	short			PacketType;
	unsigned int	Version;
	unsigned char	ID[0x18];
	unsigned char	Passwd[0x18];
	unsigned char	clienttype;
	unsigned char	m_szIP[0x10];
	unsigned char	m_szMacAddr[0xd];
} PACKET_CA_LOGIN_PCBANG;

typedef struct PACKET_CA_LOGIN_CHANNEL {
	short			PacketType;
	unsigned int	Version;
	unsigned char	ID[0x18];
	unsigned char	Passwd[0x18];
	unsigned char	clienttype;
	unsigned char	m_szIP[0x10];
	unsigned char	m_szMacAddr[0xd];
	unsigned char	Channeling_Corp;
} PACKET_CA_LOGIN_CHANNEL;

typedef struct PACKET_CA_EXE_HASHCHECK {
	short			PacketType;
	unsigned char	md5[0x10];
} PACKET_CA_EXE_HASHCHECK;

typedef struct PACKET_CA_REQ_HASH {
	short PacketType;
} PACKET_CA_REQ_HASH;

typedef struct PACKET_CA_PT_ID_INFO {
	short PacketType;
	char szPTID[21];
	char szPTNumID[21];
} PACKET_CA_PT_ID_INFO;

typedef struct PACKET_CA_LOGIN_OTP {
	short PacketType;
	short PacketLength;
	unsigned long Version;
	unsigned char clienttype;
	int TransactionID;
} PACKET_CA_LOGIN_OTP;

typedef struct PACKET_CA_CLIENT_TYPE {
	short PacketType;
	short ClientType;
	int nVer;
} PACKET_CA_CLIENT_TYPE;

typedef struct PACKET_CA_PT_EKEY {
	short PacketType;
	char m_SeedValue[9];
	char m_EKey[9];
} PACKET_CA_PT_EKEY;

typedef struct PACKET_CA_LOGIN2 {
	short PacketType;
	unsigned long Version;
	unsigned char ID[24];
	unsigned char PasswdMD5[16];
	unsigned char clienttype;
} PACKET_CA_LOGIN2;

typedef struct PACKET_CA_PT_ACCOUNT_INFO {
	short PacketType;
	short sex;
	short bPoint;
	char E_mail[34];
} PACKET_CA_PT_ACCOUNT_INFO;

typedef struct PACKET_CA_CONNECT_INFO_CHANGED {
	short PacketType;
	unsigned char ID[24];
} PACKET_CA_CONNECT_INFO_CHANGED;

typedef struct PACKET_CA_REQ_GAME_GUARD_CHECK {
	short PacketType;
} PACKET_CA_REQ_GAME_GUARD_CHECK;

typedef struct PACKET_CA_ACK_NEW_USER {
	short PacketType;
	short Sex;
} PACKET_CA_ACK_NEW_USER;

typedef struct PACKET_CA_OTP_AUTH_REQ {
	short PacketType;
	char OTPCode[7];
} PACKET_CA_OTP_AUTH_REQ;

typedef struct PACKET_CA_SSO_LOGIN_REQ {
	short PacketType;
	short PacketLength;
	unsigned long Version;
	unsigned char clienttype;
	char ID[24];
	char MacAddr[17];
	char IpAddr[15];
} PACKET_CA_SSO_LOGIN_REQ;

typedef struct PACKET_CA_LOGIN3 {
	short PacketType;
	unsigned long Version;
	unsigned char ID[24];
	unsigned char PasswdMD5[16];
	unsigned char clienttype;
	unsigned char ClientInfo;
} PACKET_CA_LOGIN3;

typedef struct PACKET_CA_PT_EKEY_FAIL {
	short PacketType;
	short errorCode;
} PACKET_CA_PT_EKEY_FAIL;

typedef struct PACKET_CAH_ACK_GAME_GUARD {
	short PacketType;
	unsigned long AuthData[4];
} PACKET_CAH_ACK_GAME_GUARD;

typedef struct PACKET_CA_ACK_MOBILE_OTP {
	short PacketType;
	short PacketLength;
	unsigned long AID;
} PACKET_CA_ACK_MOBILE_OTP;

typedef struct PACKET_CA_LOGIN4 {
	short PacketType;
	unsigned long Version;
	unsigned char ID[24];
	unsigned char PasswdMD5[16];
	unsigned char clienttype;
	char macData[13];
} PACKET_CA_LOGIN4;


typedef struct PACKET_CA_LOGIN_CHN {
	short PacketType;
	short PacketLength;
	unsigned long Version;
	unsigned char PasswdMD5[32];
	unsigned char clienttype;
} PACKET_CA_LOGIN_CHN;

typedef struct PACKET_CA_LOGIN5 {
	short PacketType;
	unsigned long Version;
	unsigned char ID[51];
	unsigned char Passwd[51];
	unsigned char clienttype;
} PACKET_CA_LOGIN5;

typedef struct PACKET_CA_PT_CARDPASS {
	short PacketType;
	char m_cardPass[28];
} PACKET_CA_PT_CARDPASS;

typedef struct PACKET_CA_REPLY_PNGAMEROOM {
	short PacketType;
	unsigned char Permission;
} PACKET_CA_REPLY_PNGAMEROOM;




// A -> C

typedef struct PACKET_AC_SSO_LOGIN_ACK {
	short PacketType;
	short Result;
} PACKET_AC_SSO_LOGIN_ACK;

typedef struct PACKET_AC_ACK_GAME_GUARD {
	short PacketType;
	unsigned char ucAnswer;
} PACKET_AC_ACK_GAME_GUARD;

typedef struct PACKET_AC_OTP_AUTH_ACK {
	short PacketType;
	short PacketLength;
	short LoginResult;
} PACKET_AC_OTP_AUTH_ACK;

typedef struct PACKET_AC_ASK_PNGAMEROOM {
	short PacketType;
} PACKET_AC_ASK_PNGAMEROOM;

typedef struct PACKET_AC_ACCEPT_LOGIN2 {
	short PacketType;
	short PacketLength;
	int AuthCode;
	unsigned long AID;
	unsigned long userLevel;
	unsigned long lastLoginIP;
	char lastLoginTime[26];
	unsigned char Sex;
	int iAccountSID;
} PACKET_AC_ACCEPT_LOGIN2;

typedef struct PACKET_AC_REALNAME_AUTH {
	short PacketType;
	short PacketLength;
	short AccountArea;
} PACKET_AC_REALNAME_AUTH;

typedef struct PACKET_AC_REQ_NEW_USER {
	short PacketType;
} PACKET_AC_REQ_NEW_USER;

typedef struct PACKET_AC_PT_ACCOUNT_INFO {
	short PacketType;
} PACKET_AC_PT_ACCOUNT_INFO;

typedef struct PACKET_AC_SHUTDOWN_NOTIFY {
	short PacketType;
	long Time;
	long ServerTime;
} PACKET_AC_SHUTDOWN_NOTIFY;

typedef struct PACKET_AC_REFUSE_LOGIN_R2 {
	short PacketType;
	int ErrorCode;
	char blockDate[20];
} PACKET_AC_REFUSE_LOGIN_R2;

typedef struct PACKET_AC_SHUTDOWN_INFO {
	short PacketType;
	long Time;
} PACKET_AC_SHUTDOWN_INFO;

typedef struct PACKET_AC_REQ_MOBILE_OTP {
	short PacketType;
	unsigned long AID;
} PACKET_AC_REQ_MOBILE_OTP;

typedef struct PACKET_AC_REQUEST_SECOND_PASSWORD {
	short PacketType;
	short Result;
	unsigned long dwSeed;
} PACKET_AC_REQUEST_SECOND_PASSWORD;

typedef struct PACKET_AC_REFUSE_LOGIN3 {
	short PacketType;
	unsigned char ErrorCode;
	long BlockedReaminSEC;
} PACKET_AC_REFUSE_LOGIN3;

/*
typedef struct PACKET_AC_ACCEPT_LOGIN {
	short PacketType;
	short PacketLength;
	int AuthCode;
	unsigned long AID;
	unsigned long userLevel;
	unsigned long lastLoginIP;
	char lastLoginTime[26];
	unsigned char Sex;
} PACKET_AC_ACCEPT_LOGIN; */

typedef struct PACKET_AC_EVENT_RESULT {
	short PacketType;
	unsigned long EventItemCount;
} PACKET_AC_EVENT_RESULT;

typedef struct PACKET_AC_PT_LOGIN_INFO {
	short PacketType;
	char m_SeedValue[9];
} PACKET_AC_PT_LOGIN_INFO;

typedef struct PACKET_AC_ACK_HASH {
	short PacketType;
	short PacketLength;
} PACKET_AC_ACK_HASH;

/*typedef struct PACKET_AC_NOTIFY_ERROR {
	short PacketType;
	short PacketLength;
} PACKET_AC_NOTIFY_ERROR;*/

typedef struct PACKET_AC_OTP_USER {
	short PacketType;
} PACKET_AC_OTP_USER;

typedef struct PACKET_AC_ACCEPT_LOGIN {
	short			PacketType;
	unsigned short	PacketLength;
	int				AuthCode;
	unsigned int	AID;
	unsigned int	userLevel;
	unsigned int	lastLoginIP;
	char			lastLoginTime[0x1A];
	unsigned char	sex;

	struct SERVER_ADDR {
		unsigned int	ip;
		unsigned short	port;
		char			name[0x14];
		unsigned short	usercount;
		unsigned short	state;			// 1 -> Maintenance
		unsigned short	property;		// 1 -> 'new'
	};
} PACKET_AC_ACCEPT_LOGIN;

typedef struct PACKET_AC_REFUSE_LOGIN {
	short			PacketType;
	unsigned char	ErrorCode;
	char			blockData[0x14];

	// ErrorCode may be one of the following:
	enum REFUSE_ERRORCODE {
		REFUSE_INVALID_ID = 0x0,
		REFUSE_INVALID_PASSWD = 0x1,
		REFUSE_ID_EXPIRED = 0x2,
		ACCEPT_ID_PASSWD = 0x3,
		REFUSE_NOT_CONFIRMED = 0x4,
		REFUSE_INVALID_VERSION = 0x5,
		REFUSE_BLOCK_TEMPORARY = 0x6,
		REFUSE_BILLING_NOT_READY = 0x7,
		REFUSE_NONSAKRAY_ID_BLOCKED = 0x8,
		REFUSE_BAN_BY_DBA = 0x9,
		REFUSE_EMAIL_NOT_CONFIRMED = 0xa,
		REFUSE_BAN_BY_GM = 0xb,
		REFUSE_TEMP_BAN_FOR_DBWORK = 0xc,
		REFUSE_SELF_LOCK = 0xd,
		REFUSE_NOT_PERMITTED_GROUP = 0xe,
		REFUSE_WAIT_FOR_SAKRAY_ACTIVE = 0xf,
		REFUSE_NOT_CHANGED_PASSWD = 0x10,
		REFUSE_BLOCK_INVALID = 0x11,
		REFUSE_WARNING = 0x12,
		REFUSE_NOT_OTP_USER_INFO = 0x13,
		REFUSE_OTP_AUTH_FAILED = 0x14,
		REFUSE_SSO_AUTH_FAILED = 0x15,
		REFUSE_NOT_ALLOWED_IP_ON_TESTING = 0x16,
		REFUSE_OVER_BANDWIDTH = 0x17,
		REFUSE_OVER_USERLIMIT = 0x18,
		REFUSE_UNDER_RESTRICTION = 0x19,
		REFUSE_BY_OUTER_SERVER = 0x1a,
		REFUSE_BY_UNIQUESERVER_CONNECTION = 0x1b,
		REFUSE_BY_AUTHSERVER_CONNECTION = 0x1c,
		REFUSE_BY_BILLSERVER_CONNECTION = 0x1d,
		REFUSE_BY_AUTH_WAITING = 0x1e,
		REFUSE_DELETED_ACCOUNT = 0x63,
		REFUSE_ALREADY_CONNECT = 0x64,
		REFUSE_TEMP_BAN_HACKING_INVESTIGATION = 0x65,
		REFUSE_TEMP_BAN_BUG_INVESTIGATION = 0x66,
		REFUSE_TEMP_BAN_DELETING_CHAR = 0x67,
		REFUSE_TEMP_BAN_DELETING_SPOUSE_CHAR = 0x68,
		REFUSE_USER_PHONE_BLOCK = 0x69,
		ACCEPT_LOGIN_USER_PHONE_BLOCK = 0x6a,
		ACCEPT_LOGIN_CHILD = 0x6b,
		REFUSE_IS_NOT_FREEUSER = 0x6c,
		REFUSE_INVALID_ONETIMELIMIT = 0x6d,
		REFUSE_CHANGE_PASSWD_FORCE = 0x6e,
		REFUSE_OUTOFDATE_PASSWORD = 0x6f,
		REFUSE_NOT_CHANGE_ACCOUNTID = 0xf0,
		REFUSE_NOT_CHANGE_CHARACTERID = 0xf1,
		REFUSE_SSO_AUTH_BLOCK_USER = 0x1394,
		REFUSE_SSO_AUTH_GAME_APPLY = 0x1395,
		REFUSE_SSO_AUTH_INVALID_GAMENUM = 0x1396,
		REFUSE_SSO_AUTH_INVALID_USER = 0x1397,
		REFUSE_SSO_AUTH_OTHERS = 0x1398,
		REFUSE_SSO_AUTH_INVALID_AGE = 0x1399,
		REFUSE_SSO_AUTH_INVALID_MACADDRESS = 0x139a,
		REFUSE_SSO_AUTH_BLOCK_ETERNAL = 0x13c6,
		REFUSE_SSO_AUTH_BLOCK_ACCOUNT_STEAL = 0x13c7,
		REFUSE_SSO_AUTH_BLOCK_BUG_INVESTIGATION = 0x13c8,
		REFUSE_SSO_NOT_PAY_USER = 0x13ba,
		REFUSE_SSO_ALREADY_LOGIN_USER = 0x13bb,
		REFUSE_SSO_CURRENT_USED_USER = 0x13bc,
		REFUSE_SSO_OTHER_1 = 0x13bd,
		REFUSE_SSO_DROP_USER = 0x13be,
		REFUSE_SSO_NOTHING_USER = 0x13bf,
		REFUSE_SSO_OTHER_2 = 0x13c0,
		REFUSE_SSO_WRONG_RATETYPE_1 = 0x13c1,
		REFUSE_SSO_EXTENSION_PCBANG_TIME = 0x13c2,
		REFUSE_SSO_WRONG_RATETYPE_2 = 0x13c3
	};
} PACKET_AC_REFUSE_LOGIN;


typedef struct PACKET_AC_NOTIFY_ERROR {
	short			PacketType;
	unsigned short	PacketLength;

	// Followed by an ErrorText
} PACKET_AC_NOTIFY_ERROR;

#pragma pack(pop)
