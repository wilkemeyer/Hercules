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

namespace rgCore { namespace serverinfo {
ServerInfo *g_ServerInfo = NULL;


ServerInfo::ServerInfo(int MySID,ServerType type, const char *dsn) { 
	sqldb::syncDBStatement *stmt = NULL;
	sqldb::syncDB *odbc = NULL;
	

	m_myself = NULL;
	memset(m_data, 0x00, sizeof(m_data));


	try {
		odbc = new sqldb::syncDB(dsn);
		stmt  = odbc->stmt("SELECT SID, Type, IP, Port, DestinationOneSID, DestinationTwoSID, SvrName, PrivateIP, PrivatePort FROM ServerInfo");
		
		//
		struct SERVERINFO si;

		stmt->bindint(1, &si.SID);
		stmt->bindint(2, &si.Type);
		stmt->bindstr(3, si.IPstr, sizeof(si.IPstr));
		stmt->bindint(4, &si.Port);
		stmt->bindint(5, &si.DestinationOneSID);
		stmt->bindint(6, &si.DestinationTwoSID);
		stmt->bindstr(7, si.Name, sizeof(si.Name));
		stmt->bindstr(8, si.PrivateIPstr, sizeof(si.PrivateIPstr));
		stmt->bindint(9, &si.PrivatePort);

		stmt->exec();


		bool foundMySelf = false; 
		size_t cnt = 0;
		putLog("ServerInfo Init\n");

		while(1){
			memset(&si, 0x00, sizeof(si));

			if(stmt->fetch() == false){
				break;
			}


			// SID out of range
			if(si.SID > SERVERID_MAX){
				putErr("ServerInfo: Server '%s' (%s:%u) SID(%u) > SERVERID_MAX! - Skipping Definition!\n", si.Name, si.IPstr, si.Port, si.SID);
				continue;
			}

			if(m_data[si.SID] != NULL){
				putErr("ServerInfo: SID(%u) Defined Multiple Times! - Skipping Server '%s' (%s:%u)\n", si.SID, si.Name, si.IPstr, si.Port);
				continue;
			}

			if(si.SID == 0){
				putErr("ServerInfo: Server '%s' has SID(%u) - ServerID Zero must be undefined! - Skipping\n", si.Name, si.SID);
			}


			m_data[si.SID] = (struct SERVERINFO*)roalloc(sizeof(struct SERVERINFO));
			
			// Copy Data
			memcpy(m_data[si.SID], &si, sizeof(struct SERVERINFO));
			
			// Assign DWORD IP Values
			si.ip			= inet_addr(si.IPstr);
			si.PrivateIP	= inet_addr(si.PrivateIPstr);


			// Print:
			/*
			{
				char t;
				switch(si.Type){
					case 0: t = '?'; break;
					case 1: t = 'A'; break;
					case 2: t = 'C'; break;
					case 3: t = 'Z'; break;
					case 4: t = 'I'; break;
					default: t = '?'; break;
				}

				char s = '-';
				if(MySID != 0 && MySID == si.SID){
					s = '=';	// Self
					foundMySelf = true;
				}

				putLog("- %-4u  %c  %-16s %u  %-4u %-4u  %-20s", si.SID, t, si.IPstr, si.Port, si.DestinationOneSID, si.DestinationTwoSID, si.Name);
			}*/

			cnt++;
		}

		if(foundMySelf == true){
			if(m_data[MySID]->Type != type){
				putErr("ServerInfo: Own Server Definition Type Mismatch! Expected Type(%u) got Type(%u) for SID(%u)\n", type, m_data[MySID]->Type, MySID);
			}


			m_myself = m_data[MySID];

			putLog("ServerInfo: MySelf Found SID(%u) IP(%s) Port(%u) PrivateIP(%s) PrivatePort(%u)\n", MySID, m_data[MySID]->IPstr, m_data[MySID]->Port, m_data[MySID]->PrivateIPstr, m_data[MySID]->PrivatePort);

		}else{

			if(MySID != 0){
				putErr("ServerInfo: Own Server Definition Missing! SID(%u) Not Found in ServerInfo!\n", MySID);
			}
		}



		putLog("ServerInfo Loaded %u Definitions!\n", cnt);

	}catch(sqldb::syncDBException *e){
		putErr("ServerInfo DB Initialization Failed - Check DSN/DB! - Error: (%u) %s\n", e->getSqlErrCode(), e->getMessage());
	
	}


	

	// Free DB Object
	if(odbc != NULL)
		delete odbc;

	if(stmt != NULL)
		delete stmt;
}


ServerInfo::~ServerInfo() {

	for(size_t i  = 0; i < (SERVERID_MAX+1); i++){
		if(m_data[i] != NULL){
			rofree(m_data[i]);
			//
			m_data[i] = NULL;
		}
	}

}


struct SERVERINFO *ServerInfo::getSelf(){
	return m_myself;
}


struct SERVERINFO *ServerInfo::getBySID(int SID){
	struct SERVERINFO *sinfo; 
		if(SID > SERVERID_MAX) {
#ifdef _DEBUG 
				auto dbg = debugInfoGet(_ReturnAddress()); 
				putErr("ServerInfo::getBySID(%u) Out of Range ( > SERVERID_MAX ) - called by %s in %s:%u\n", SID, dbg->func, dbg->file, dbg->line); 
#endif 
				sinfo = NULL; 
		} else {
				sinfo = m_data[SID]; 
		}

	return sinfo;
}


ServerType ServerInfo::getServerType(int SID) {
	struct SERVERINFO *sinfo;
	if(SID > SERVERID_MAX) {
#ifdef _DEBUG 
		auto dbg = debugInfoGet(_ReturnAddress());
		putErr("ServerInfo::getBySID(%u) Out of Range ( > SERVERID_MAX ) - called by %s in %s:%u\n", SID, dbg->func, dbg->file, dbg->line);
#endif 
		sinfo = NULL;
	} else {
		sinfo = m_data[SID];
	}

	if(sinfo == NULL)
		return stUnknown;

	return (ServerType)(sinfo->Type);
}



} } //end namespaces