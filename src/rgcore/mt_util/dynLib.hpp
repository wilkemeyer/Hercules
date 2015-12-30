#pragma once
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
			ShowError("%s: failed to resolve '%s' (GetLastError: %u)\n", m_name, procName, GetLastError());
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