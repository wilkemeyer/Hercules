#include "stdafx.h"

using namespace rgCore;
using namespace rgCore::Time;

static char g_appName[32] = "NotSet";


void rgCore_init(const char *appName){

	// Assign AppName
	strncpy_s(g_appName, appName, sizeof(g_appName));

	// Global Core Logger:
	Logging::coreLogger::init();
	Logging::g_globalLogger = Logging::coreLogger::get();

	//
	roalloc_init();
	tick_init();

}//end: rgCore_init()


void rgCore_final() {

	tick_final();
	roalloc_final();

	// Finalize Global Core Logger:
	Logging::g_globalLogger = NULL;
	Logging::coreLogger::final();
	
}//end: rgCore_final()


const char *rgCore_getAppName(){
	return g_appName;
}