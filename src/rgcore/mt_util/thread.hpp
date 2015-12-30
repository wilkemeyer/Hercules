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


#include "../common/showmsg.h"
namespace rgCore {
	namespace util {
		class thread;

		//
		// @TODO _ReadWriteBarrier not supported on < NT5.2 - FIX IT.
		//

		typedef DWORD (__stdcall *threadProc)(thread *);

		class thread {
		public:
			__forceinline thread(const char *name, threadProc startAddress, void *startParameter = NULL) {

				// 
				m_threadProc = startAddress;
				m_param = startParameter;
				m_exitParam = NULL;
				m_threadid = 0;


				// Set Thread name
				if(name != NULL && *name != '\0') {
					m_name = strdup(name);
				} else {
					char tmp[64];
#ifdef _DEBUG
#ifdef ARCH_AMD64
					sprintf(tmp, "UnnamedThread_0x%p", startAddress);
#else
					sprintf(tmp, "UnnamedThread_0x%x", startAddress);
#endif // ARCH_AMD64
#else
					strcpy(tmp, "UnnamedThread");
#endif
					// 

					m_name = strdup(name);
				}
				// end: set Name


				//_ReadWriteBarrier();
				atomic::Exchange(&m_isRunning, 1);


				m_handle = CreateThread(NULL, 0, threadMain, this, CREATE_SUSPENDED, NULL);
				if(m_handle == NULL || m_handle == INVALID_HANDLE_VALUE){
					atomic::Exchange(&m_isRunning, 0);
					ShowError("Thread: failed to create new Thread '%s'\n", m_name);
					return; // @TODO fatal?
				}

				//
				m_threadid = GetThreadId(m_handle);

				// set prio to normal.
				setPriority(THRPRIO_NORMAL);
				
				// Priority Boost settings?
				BOOL val = FALSE;
				m_prioBoost = false;
				if(GetThreadPriorityBoost(m_handle, &val) != TRUE){
					// assuming its enabled
					m_prioBoost = true;
				}else{
					if(val == TRUE)
						m_prioBoost = true;
				}

				// Resume Thread!
				ResumeThread(m_handle);


			}//end: thread()


			__forceinline ~thread(){
				
				TerminateThread(m_handle, EXIT_FAILURE);
				CloseHandle(m_handle);

				if(m_name != NULL){
					free(m_name);
				}
				
			}//end: ~thread()


			__forceinline void setName(const char *name){

				if(m_name != NULL){
					free(m_name);
					//m_name = NULL;
				}
				
				
				
			}//end: setName()


			__forceinline void setNameInDebugger(){
				//
				// Set's the name of the thread for VC Debugger  
				// https://msdn.microsoft.com/en-us/library/xcb2z8hs.aspx
#ifdef _DEBUG
				const DWORD MS_VC_EXCEPTION=0x406D1388;

#pragma pack(push,8)
				typedef struct tagTHREADNAME_INFO {
					DWORD dwType; // Must be 0x1000.
					LPCSTR szName; // Pointer to name (in user addr space).
					DWORD dwThreadID; // Thread ID (-1=caller thread).
					DWORD dwFlags; // Reserved for future use, must be zero.
				} THREADNAME_INFO;
#pragma pack(pop)

				THREADNAME_INFO info;
				info.dwType = 0x1000;
				info.dwThreadID = -1;
				info.dwFlags = 0;
				info.szName = m_name;

				__try {
					RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
				} __except(EXCEPTION_EXECUTE_HANDLER) {
				}
#endif //endif: _DEBUG?
			}//end: setNameInDebugger()

			__forceinline static DWORD __stdcall threadMain(void *__ThisThreadData) {
				thread *_this = (thread*)__ThisThreadData;
				DWORD ret = 0;

				_this->setNameInDebugger();

#ifdef _DEBUG
				if(Logging::g_globalLogger != NULL)
					ShowDebug("New Thread '%s'\n", _this->m_name);
#endif
				ret = _this->m_threadProc(_this);

				atomic::Exchange(&_this->m_isRunning, 0);

#ifdef _DEBUG
				if(Logging::g_globalLogger != NULL)
					ShowDebug("Thread Terminated '%s' (ExitCode: %u)\n", _this->m_name, ret);
#endif
				return ret;
			}//end: threadMain


			/** 
			 * Terminates the Calling Thread (other wait to just return from threadproc)
			 * 
			 * _must_ Called from 'within' the thread.
			 */
			__forceinline void exit(DWORD exitCode, void *exitParam) {
				m_exitParam = exitParam;
				
				//_ReadWriteBarrier();

				ExitThread(exitCode);

				atomic::Exchange(&m_isRunning, 0);
			}//end: exit()


			/** 
			 * Checks if the Thread Proc has returned yet
			 *
			 * @return int nonzero -> running
			 */
			__forceinline int isRunning(){
				return m_isRunning;
			}//end: isRunning()


			/**
			 * Returns the Start 'parameter' passed on thread creation
			 *
			 * Usually call from 'within' the thread.
			 */
			__forceinline void *getStartParameter() {
				//_ReadWriteBarrier();
				return m_param;
			}//end: getStartparameter()


			/**
			 * Waits for thread termination
			 *
			 * Called from 'outside'.
			 * 
			 * @param timeout_ms  timeout - of INIFINITE
			 *
			 */
			__forceinline void wait(DWORD timeout_ms = INFINITE) {
				WaitForSingleObject(m_handle, timeout_ms);
			}//end: wait()


			
			/**
			 * Kills the Thread
			 *
			 * Called frmo 'outside'
			 *
			 */
			__forceinline void kill() {
				TerminateThread(m_handle, 0);
			}//end: kill()



			/** 
			 * Gets the Exit Paramter a thread may set with while terminating 
			 *
			 */
			__forceinline void *getExitParam() {
				//_ReadWriteBarrier();
				return m_exitParam;
			}//end: getExitParam()


			typedef enum THRPRIO {
				THRPRIO_LOWEST = 0,
				THRPRIO_LOW, 
				THRPRIO_NORMAL, 
				THRPRIO_HIGH, 
				THRPRIO_HIGHEST
			} THRPRIO;


			/**
			 * Enables/Disables OS Time Stealing (PrioBoost on Win32)
			 * 
			 * @return old state
			 */
			bool setTimeSteal(bool enable){
				bool oldval = m_prioBoost;

				// current setting = requested setting - ignoring.
				if(m_prioBoost == enable){
					return m_prioBoost;
				}


				//
				if(SetThreadPriorityBoost(m_handle, (enable==true)?FALSE:TRUE) != FALSE) {
					
					m_prioBoost = enable;
					
				}else{
					ShowError("Thread '%s' can't set PriorityBoost - Error: %u\n", m_name, GetLastError());
				}
				
				return oldval;
			}//end: setTimeSteal()


			/**
			 * Sets the Thread Priority in the OS's Scheduler
			 *
			 * @return the old priority
			 */
			THRPRIO setPriority(THRPRIO newPrio){
				int winPrio;

				// requested prio - already set - ingoring.
				if(m_prio == newPrio){
					return m_prio; 
				}

				switch(newPrio){ // @optimize: static mapping/lookup table.
					case THRPRIO_LOWEST:	winPrio = THREAD_PRIORITY_IDLE;	break;
					case THRPRIO_LOW:		winPrio = THREAD_PRIORITY_BELOW_NORMAL; break;
					case THRPRIO_NORMAL:	winPrio = THREAD_PRIORITY_NORMAL; break;
					case THRPRIO_HIGH:		winPrio = THREAD_PRIORITY_ABOVE_NORMAL; break;
					case THRPRIO_HIGHEST:	winPrio = THREAD_PRIORITY_TIME_CRITICAL; break;
					default:
						ShowDebug("Thread '%s' unknown Priority %u assuming NORMAL Priority\n", m_name, newPrio);
						winPrio = THREAD_PRIORITY_NORMAL;
						break;
				}

				if(SetThreadPriority(m_handle, winPrio) != FALSE){
					THRPRIO oldPrio = m_prio;

					m_prio = newPrio;

					return oldPrio;
				}else{
					ShowError("Thread '%s' can't set Priority - Error: %u\n", m_name, GetLastError());
				}
				

				// not modified.
				return m_prio;
			}//end: setPriority()


			/** 
			 * Returns Thread's Name (set by constructor parameter)
			 *
			 */ 
			const char *getName(){
				return m_name;
			}


		private:
			HANDLE m_handle;
			DWORD m_threadid;
			threadProc m_threadProc;
			void *m_param;
			void *m_exitParam;
			atomic_align atomic::atomic_t m_isRunning;
			//
			bool m_prioBoost;
			THRPRIO m_prio;	//@
			// debug:
			char *m_name;
		};

	}
}