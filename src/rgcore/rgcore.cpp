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

using namespace rgCore;
using namespace rgCore::Time;
using namespace rgCore::gui;
using namespace rgCore::serverinfo;

//
namespace rgCore {
static IServerApplication *g_pApp = NULL;

struct rgCoreSettings rgCore_settings;
static win_ui *g_winUI = NULL;




static void rgCore_readConfig(){

	// Memory Pool Global Configuration:
	rgCore_settings.poolAllowLargePages		= iniGetAppBoolean("Pool", "Allow Large Pages", false);
	rgCore_settings.poolEnforceLargePages	= iniGetAppBoolean("Pool", "Enforce Large Pages", false);


}//end: rgCore_readConfig()


static void rgCore_init(IServerApplication *pApp){
	// Assign Ptr To Instance
	g_pApp = pApp;

	// Clean & Read Core Settings:
	memset(&rgCore_settings, 0x00, sizeof(struct rgCoreSettings));
	rgCore_readConfig();

	// Global Core Logger:
	Logging::coreLogger::init();
	Logging::g_globalLogger = Logging::coreLogger::get();

	// Initialize Debug 
	debug_init();

	// Roalloc:
	{
		roalloc_init();
		
		// Check Large page Status + may disable Pool Large Pages 
		if(roalloc_getLargePageStatus() == ROALLOC_LP_OSRefused){
			rgCore_settings.poolAllowLargePages		= false;
			rgCore_settings.poolEnforceLargePages	= false;

			putMemMgr("Force Disabled Memory Pool Large Pages, as Main Allocator failed to enable (OS Refused)\n");
		}


		char *strStatus;
		switch(roalloc_getLargePageStatus()){
			case ROALLOC_LP_Enabled: strStatus = "Enabled"; break;
			case ROALLOC_LP_Disabled: strStatus = "Disabled"; break;
			case ROALLOC_LP_OSRefused: strStatus = "Disabled by OS"; break;
			default: strStatus = "Unknown?"; break;
		}

		putMemMgr("Large Page Support: %s\n", strStatus);

	}

	
	// Initialzie Other Subsystems:
	tick_init();
	timer::init();
	rbdb::rbdbBase::globalInit();
	asyncDB_init();

	// Network 
	network::netBuffer::init();
	network::PendingIOMgr_init();
	rgNet_init();


	// Create GUI & Hide Console WIndow
	{
		HWND cHwnd = GetConsoleWindow();
		if(cHwnd != NULL){
			ShowWindow(cHwnd, SW_HIDE);
		}
	}
	g_winUI = new win_ui();


	// Initilize ServerInfo
	{
		int MySID = (int)iniGetAppInteger("Identification", "MYSID", 0);
		char dsn[256];
		iniGetAppString("Identification", "DSN ServerInfo", "Driver={SQL Server};Server=localhost;Database=GlobalInfo;Uid=Ragnarok;Pwd=Ragnarok", dsn, sizeof(dsn));

		g_ServerInfo = new ServerInfo(MySID, g_pApp->getApplicationType(), dsn);
	}

}//end: rgCore_init()


static void rgCore_final() {

	if(g_ServerInfo != NULL){
		delete g_ServerInfo;
		g_ServerInfo = NULL;
	}

	// Destroy UI & Show Console Window
	if(g_winUI != NULL){
		delete g_winUI;
		g_winUI = NULL;

		{
			HWND cHwnd = GetConsoleWindow();
			if(cHwnd != NULL) {
				ShowWindow(cHwnd, SW_SHOWNORMAL);
			}
		}

	}

	rgNet_final();
	network::PendingIOMgr_final();
	network::netBuffer::final();
	

	//
	asyncDB_final();
	rbdb::rbdbBase::globalFinal();
	timer::final();
	tick_final();
	roalloc_final();

	debug_final();

	// Finalize Global Core Logger:
	Logging::g_globalLogger = NULL;
	Logging::coreLogger::final();
	
}//end: rgCore_final()


static void rgCore_releaseIdleLoop(){
	// 
	//
	if(g_winUI != NULL){
		g_winUI->requestDestroy();
	}

}//end: rgCore_beginFinal()

static void rgCore_idleLoop(){
	
	g_winUI->idleLoop();

}//end: rgCore_idleLoop()


DWORD __stdcall appMain(util::thread *_self){

	g_pApp->init();

	// Will block 
	g_pApp->main();

	//
	g_pApp->final();


	// Terminate the UI
	rgCore_releaseIdleLoop();


	return 0;
}//end: appMain()


int rgCore_run(IServerApplication *pApp){
	util::thread *hAppMain;
	
	rgCore_init(pApp);

	
	// Spawn Thread
	hAppMain = new util::thread("ServerApplication-Main", appMain);
	
	// Set Thread Prio Boost
	if( iniGetAppBoolean("serverMain Thread", "Disable Priority Boost", false) == true){
		hAppMain->setTimeSteal(false);
	}
	



	// Wait for UI to break
	g_winUI->idleLoop();


	// 
	hAppMain->wait();
	delete hAppMain;



	// Finalize the core
	rgCore_final();


	return 0;
}//end: rgCore_run()


IServerApplication *rgCore_getApplication(){
	return g_pApp;
}//end: rgCore_getApplication()


const char *rgCore_getAppName(){
	return g_pApp->getApplicationName();
}//end: rgCore_getAppName()

}//end namespace