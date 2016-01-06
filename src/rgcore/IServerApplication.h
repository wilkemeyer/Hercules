#pragma once


namespace rgCore { 


class IServerApplication {
public:
	/**
	 * Synchronous Initializatin, called in the same thread as main()
	 */
	virtual void init() = 0;

	/** 
	 * Synchronous Finalizaton, called in the same thread as main
	 * right after main(); 
	 */
	virtual void final() = 0;
	
	/** 
	 * This Method is your Server's Main Thread
	 * you should'nt return except you want to terminate your
	 * application (or if it has been requested by a call to requestShutdown())
	 *
	 */ 
	virtual void main() = 0;


	/** 
	 * Must return the Application's name
	 * 
	 * The Application Name will also be used to determine 
	 * the configuration file that'd be used
	 */
	virtual const char *getApplicationName() = 0;

	/**
	 * Must return the Application's Type
	 *
	 */
	virtual serverinfo::ServerType getApplicationType() = 0;


	/**
	 * Called by rgCore whenever the application was requested to 
	 * Terminate by user.
	 *
	 * You should perform all neccesary steps for termination
	 * as + return from main();
	 */
	virtual void requestShutdown() = 0;
};

}