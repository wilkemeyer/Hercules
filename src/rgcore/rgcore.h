#pragma once
// 
// Call / Procesing:
//	- rgCore_init()
//	-> recommended to create a thread for applicaiton specific initialization!
//	- rgCore_idleLoop()
//	-> (CURRENTLY HARDCODED) GUI Close Button will make applicaiton-specific callback 
//	-> to releasse rgCore_idleLoop  call rgCore_releaseIdleLoop (in application-specific callback, after you've finialized your -application specific- stuff)
//  - rg_final()
//


// Platformlib Init:
void rgCore_init(const char *appName);
void rgCore_final();

// Note: calling this proc will close the GUI! 
// any further (core-specific) debug output will be logged to file (if enabled)
void rgCore_releaseIdleLoop();

// Must be Called in Main Thread
// Will Block until beginFinal() gets called
void rgCore_idleLoop();

const char *rgCore_getAppName();
