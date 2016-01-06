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

namespace rgCore {
	class IServerApplication;

	/** 
	 * Must be called from your entrypoint
	 * will block until the application terminates
	 *
	 * Returns: 0 if the application terminated upon request otherwise errorcode
	 */
	int rgCore_run(IServerApplication *pApp);


	IServerApplication *rgCore_getApplication();
	const char *rgCore_getAppName();


	//
	// --
	//
	struct rgCoreSettings {
		bool poolAllowLargePages;
		bool poolEnforceLargePages;
	};
	extern struct rgCoreSettings rgCore_settings;


}