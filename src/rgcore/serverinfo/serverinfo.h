#pragma once

namespace rgCore { namespace serverinfo {

#define SERVERID_MAX 1023

typedef enum ServerType {
	stUnknown = 0,
	stAccount = 1,
	stCharacter = 2,
	stZone = 3,
	stInter = 4,
	//
	stOther = 50
} ServerType;


struct SERVERINFO {
	int		SID;		// See: SERVERID_MAX
	int		Type;

	DWORD	ip;
	char	IPstr[16];
	int		Port;

	int DestinationOneSID;
	int DestinationTwoSID;

	char	Name[20];

	DWORD	PrivateIP;
	char	PrivateIPstr[16];
	int		PrivatePort;
};


class ServerInfo : public roAlloc {
public:
	ServerInfo(int MySID, ServerType type, const char *dsn);
	~ServerInfo();

public:
	/** 
	 * returns the 'own' / myself ServerInfo Data
	 */
	struct SERVERINFO *getSelf();

	/**
	 * returns the Data for the given SID
	 */
	struct SERVERINFO *getBySID(int SID);


	/** 
	 * Returns the type for the given SID
	 */
	ServerType getServerType(int SID);




private:
	struct SERVERINFO	*m_data[SERVERID_MAX+1];
	struct SERVERINFO  *m_myself;



};

extern ServerInfo *g_ServerInfo;


} } //end namespaces
