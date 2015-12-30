#include "stdafx.h"

using namespace rgCore;
using namespace rgCore::Time;
using namespace rgCore::gui;

static char g_appName[32] = "NotSet";
static win_ui *g_winUI = NULL;

void rgCore_init(const char *appName){

	// Assign AppName
	strncpy_s(g_appName, appName, sizeof(g_appName));

	// Global Core Logger:
	Logging::coreLogger::init();
	Logging::g_globalLogger = Logging::coreLogger::get();

	// Initialize Debug 
	debug_init();

	//
	roalloc_init();
	tick_init();

	
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