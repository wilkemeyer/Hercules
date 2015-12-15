#pragma once

namespace rgCore {
namespace sys {
	
class dynLib {
public:

	/** 
	 * Loads the given Library
	 * 
	 * @param *sharedLib name of shared library
	 *
	 * @trows blockz::util::Exception if sharedlib fails to load
	 */
	__forceinline dynLib(const char *sharedlib){
		
		m_name = NULL;
		m_dll = LoadLibrary(sharedlib);
		if(m_dll == NULL){
			throw new rgCore::util::Exception("Loading of Shared Library '%s' failed (GetLastError: %u)", sharedlib, GetLastError());
		}

		m_name = strdup(sharedlib);

	}//end: constructor


	__forceinline ~dynLib(){
		
		if(m_dll != NULL){
			FreeLibrary(m_dll);
			m_dll = NULL;
		}

		if(m_name != NULL){
			free(m_name);
			m_name = NULL;
		}

	}//end: destructor


	/**
	 * Retrieves the address of an exported function or variable from the specified dynamic-link library (DLL).
	 *
	 * @return NULL on error otherwise procAddress
	 */
	typedef void (*DYNLIBPROC)(void);
	__forceinline DYNLIBPROC getProcAddress(const char *procName) {

		DYNLIBPROC ret = (DYNLIBPROC)GetProcAddress(m_dll, procName);

		if(ret == NULL){
			showmsg->showError("%s: failed to resolve '%s' (GetLastError: %u)\n", m_name, procName, GetLastError());
		}

		return ret;
	}//end: getProcAddress()

	
	/**
	 * Retrieves the default platform-specific filename extension for shared librarys
	 */
	static __forceinline const char *getFileNameExtension() {
		return "dll";
	}//end: getFileNameExtension()

private:
	HMODULE m_dll;
	char *m_name;
};

}
}