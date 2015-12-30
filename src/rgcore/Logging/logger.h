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

#include "ILogger.h"

namespace rgCore {
namespace Logging {

	class logger : public ILogger {
	public:
		logger();
		virtual ~logger();

		// Spawns the Worker Thread,
		// as the threading implementation relys on the fact that a logging
		// instance has been initialized we can't do this in our constructor
		void startLogger(); 


		// will enable remote logging. (can only be initialized once!)
		void enableUDPConsole(const char *appName, const char *ip, unsigned short port);
		void sendUDPConsolePerfData(int tick, int FPS, int UIFPS);

		// Write line to log:
		void write(enum LOGLEVEL level, const char *msg, ...);
		void writeEx(enum LOGLEVEL level, const char *file, int line, const char *func, const char *msg, ...);

		
		/** 
		 * Enables logging to file
		 *
		 * @param *logFile		log file Name
		 * @param *loggedMask	logged mask for this file (see: LOGLEVEL_..)
		 *
		 * @return boolean		true if logfile could be created
		 */
		bool enableFileLogging(const char *logFile, unsigned int loggedMask);


		/**
		 * Enables logging to STDOUT
		 *
		 * @param loggedMask		the Mask to be logged to Terminal (NONE to disable terminal logging)
		 * 
		 * @note: Terminal Logging is default Disabled!
		 *
		 */
		void enableTerminalLogging(unsigned int loggedMask);


		/** 
		 * Tick Provider Program start must be Tick 0
		 */
		virtual __int64 get_Tick(void) = 0;

	public:
		//
		// Methods used by Debug Subsystem
		// (you should know what u're doing, theese may be dangerous)
		
		// Flushes the current log queue to files
		void flushLogging();

		// Copies the current log-files to the given directory
		// Note: it will create the directory tree if it does not exist!
		// Note: the given directory name must be terminated with /! (or \ on win32)
		void copyFileLogFiles(const char *directory);


	public:
		///
		/// Log Hook Support
		///

		#define SZ_QELEM_LINEBUF 64
		typedef struct qElem {
			struct qElem *next;
			enum LOGLEVEL level;
			__int64 tick;

			// extended debug info:
			int dbg_line;
			char dbg_file[48];
			char dbg_func[48];
			//

			size_t szLine;
			char line[SZ_QELEM_LINEBUF];	// note: changing this size requires changes to code!
		} qElem;


		// Hook Proc
		typedef void(*logHookProc)(void *paramPtr, const qElem *line);


		/** 
		 * Registers a log hook (called for each log message)
		 * 
		 * @note will be called async (in writerThread), so there may be a minimal delay betweel call to write() and hook-call.
		 *
		 */
		void registerHook(unsigned int subscribedMask, void *paramPtr, logHookProc proc);
		
		/**
		 * Unregisters a log hook registered as combination opf paramPtr and proc
		 */
		void unregisterHook(unsigned int subscribedMask, void *paramPtr, logHookProc proc);

	public: 
		/**
		 * Copies the given qElem
		 *
		 * @param *qe			input element to duplciate
		 * @param use_roalloc	use roalloc() to allocate the copy (otherwise platform malloc() will be used)
		 * @param offset		copy line text beginning at offset ..
		 * @param length		copy line text from offset in length of ...  (if set to 0, the whole line gets copied)
		 *
		 * @note this method will create an exact copy, including ->next !
		 *
		 */
		static qElem *copyQElem(const qElem *qe, bool use_roalloc = false, size_t offset=0, size_t length=0);


	private:
		struct hookEntry {
			struct hookEntry *next;
			
			unsigned int subscribedMask; // LOGLEVEL_..

			logHookProc proc;
			void *paramPtr;
		};
		


	private:
		// Thread Writer Routine.
		static DWORD _stdcall WorkerProc(rgCore::util::thread *t);
	

	private:
		//
		// Logging To File - stuff:
		//
		struct qElem;

		struct fileParam {
			struct fileParam *next;
			FILE *fp;
			const char *fileName;
			unsigned int loggedMask;
		};
		

		// file logger Proc:
		static void writeLineToFile(void *fileParam, const struct qElem *line);

		// 
		static const char *logType2Str(enum LOGLEVEL lt);

	private:
		//
		// Logging to UDP - stuff:
		//
		static void writeLineToUDP(void *udpParam, const struct qElem *line);

	private:
		// 
		// Terminal Logging - stuff:
		//
		unsigned int m_termLogging_Mask;
		static void writeLineToTerminal(void *termParam, const struct qElem *line);


	private:
		rgCore::util::thread *m_thread_writer;
		
		struct fileParam *m_logToFileListBegin; 
		rgCore::util::spinlock m_lock_logToFileList;


		struct qElem *m_qbegin, *m_qEnd;

		rgCore::util::spinlock m_lock_queue;
		rgCore::util::spinlock m_lock_hooklist;

		rgCore::util::mutex m_lock_workercond;
		rgCore::util::cond m_cond_worker;
		
		struct hookEntry *m_hlBegin, *m_hlEnd; //hooklist begin

		rgCore::atomic::atomic_t m_isInitialized; // must be 1.


		// Support for UDP / Remote Logging
		char *m_dbg_appname;
		WSADATA m_dbg_wsa;
		SOCKET m_dbg_sock;
		struct sockaddr_in m_dbg_addr;
	};

}
}