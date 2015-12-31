#include "stdafx.h"

using namespace rgCore;
using namespace rgCore::Time;
using namespace rgCore::gui;

//
struct rgCoreSettings rgCore_settings;


static char g_appName[32] = "NotSet";
static win_ui *g_winUI = NULL;


static void rgCore_readConfig(){

	// Memory Pool Global Configuration:
	rgCore_settings.poolAllowLargePages		= iniGetAppBoolean("Pool", "Allow Large Pages", false);
	rgCore_settings.poolEnforceLargePages	= iniGetAppBoolean("Pool", "Enforce Large Pages", false);


}//end: rgCore_readConfig()


void rgCore_init(const char *appName){

	// Assign AppName
	strncpy_s(g_appName, appName, sizeof(g_appName));

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


}//end: rgCore_init()


void rgCore_final() {

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

	timer::final();
	tick_final();
	roalloc_final();

	debug_final();

	// Finalize Global Core Logger:
	Logging::g_globalLogger = NULL;
	Logging::coreLogger::final();
	
}//end: rgCore_final()


void rgCore_releaseIdleLoop(){
	// 
	//
	if(g_winUI != NULL){
		g_winUI->requestDestroy();
	}

}//end: rgCore_beginFinal()

void rgCore_idleLoop(){
	
	g_winUI->idleLoop();

}//end: rgCore_idleLoop()


const char *rgCore_getAppName(){
	return g_appName;
}