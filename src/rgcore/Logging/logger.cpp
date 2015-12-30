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
#include "stdafx.h" // VS Bug.
#include "../stdafx.h"


using namespace rgCore::util;

namespace rgCore {
namespace Logging {


logger::logger() {

	// Proper Initialization.
	atomic::Exchange(&m_isInitialized, 0);

	m_thread_writer = NULL;

	m_qbegin	= NULL;
	m_qEnd		= NULL;

	m_dbg_appname = NULL;
	m_dbg_sock	= INVALID_SOCKET;
	memset(&m_dbg_addr, 0x00, sizeof(struct sockaddr_in));

	m_hlBegin	= NULL;
	m_hlEnd		= NULL;

	m_logToFileListBegin = NULL;
	m_termLogging_Mask	= LOGLEVEL_NONE;

	// 
	atomic::Exchange(&m_isInitialized, 1);

}//end logger()


void logger::startLogger(){

	// Create Writer Thread
	if(m_thread_writer == NULL){
		m_thread_writer = new thread("Logger - Worker", logger::WorkerProc, this);
	}

}//end: startLogger()


logger::~logger() {

	// Prevents furhter write calls being queued, as well as forces logger to shutdown
	atomic::Exchange(&m_isInitialized, 0);

	// Wait for Log Writer to exit 0.3sec
	if(m_thread_writer != NULL) {
		m_cond_worker.signal();

		m_thread_writer->wait(500);
		m_thread_writer->kill();
		delete m_thread_writer;
		m_thread_writer = NULL;
	}

	// Delete Reminaing Items from Queue (there shouldnt be anyone.. but ..)
	struct qElem *it, *itn;
	it = m_qbegin;
	while(1) {
		if(it == NULL)
			break;

		itn = it->next;

		free(it);

		it = itn;
	}

	m_qbegin = m_qEnd = NULL;


	// Cleanup opened files
	struct fileParam *fit, *fitn;
	fit = m_logToFileListBegin;
	while(1) {
		if(fit == NULL)
			break;

		fitn = fit->next;

		fclose(fit->fp);
		
		free((void*)(fit->fileName) );
		free(fit);
		
		fit = fitn;
	}


	// Cleanup LogHook List
	struct hookEntry *hit, *hitn;
	hit = m_hlBegin;
	while(1) {
		if(hit == NULL)
			break;

		hitn = hit->next;

		free(hit);

		hit = hitn;
	}
	m_hlBegin = m_hlEnd = NULL;


	// UDP Console Cleanup.
	if(m_dbg_sock != INVALID_SOCKET) {
		
		//
		size_t szPacket = strlen(m_dbg_appname) + 2;
		char *packet = (char*)malloc(szPacket);
		if(packet != NULL) {
			packet[0] = 'S'; // shutdown
			strcpy(&packet[1], m_dbg_appname);
			packet[strlen(m_dbg_appname) + 1] = '\0'; // +1 in offste results because of prepending Opcode.
			sendto(m_dbg_sock, packet, (int)szPacket, 0, (SOCKADDR*)&m_dbg_addr, sizeof(m_dbg_addr));

			free(packet);
		}


		if(m_dbg_appname != NULL) {
			free(m_dbg_appname);
			m_dbg_appname = NULL;
		}

		closesocket(m_dbg_sock);
		m_dbg_sock = INVALID_SOCKET;
		WSACleanup();
		
	}


}//end: ~logger()


void logger::flushLogging(){
	size_t c = 0;

	// Enforce the queue to be written to file.
	while(1){
		m_lock_queue.lock();
		if(m_qbegin != NULL){
			m_lock_queue.unlock();

			// Signal The Worker again
			m_cond_worker.signal();

			if(c++ > 15000){
				// More then 15k tries, probably a problem that the log is being spammed.. 
				break;
			}

		}else{
			m_lock_queue.unlock();

			break; // break while!
		}
	}

	
	// Flush All Opened Log Files to disk!
	m_lock_logToFileList.lock();
	for(auto it = m_logToFileListBegin; it != NULL; it = it->next){
	
		fflush(it->fp);

	}
	m_lock_logToFileList.unlock();


	////

}//end: flushLogging()


void logger::enableUDPConsole(const char *appName, const char *ip, unsigned short port) {

	if(m_dbg_sock != INVALID_SOCKET)
		return;

	if(WSAStartup(MAKEWORD(2, 0), &m_dbg_wsa) == SOCKET_ERROR){
		this->write(LOGLEVEL_ERROR, "enableUDPConsole: failed to bootstrap Winsock - got Error: %d\n", WSAGetLastError());
		return;
	}
	

	m_dbg_sock = socket(AF_INET, SOCK_DGRAM, 0);
	if(m_dbg_sock == INVALID_SOCKET) {
		WSACleanup();
		this->write(LOGLEVEL_ERROR, "enableUDPConsole: failed to create socket - got Error: %d\n", WSAGetLastError());
		return;
	}

	memset(&m_dbg_addr, 0x00, sizeof(struct sockaddr_in));
	m_dbg_addr.sin_family		= AF_INET;
	m_dbg_addr.sin_addr.s_addr	= inet_addr(ip);
	m_dbg_addr.sin_port			= htons(port);

	m_dbg_appname = strdup(appName);

	///
	///
	/// Debug Console Protocol overview
	///
	/// Log Message OP  (OP 'M') - Message
	///  <OP>.B <TIME>.I64 <LEVEL>.B <MSG>.?B
	///  
	/// Application Start OP (OP 'H') - HELLO! :) 
	///  <OP>.B <Appname>.?B
	///
	/// Application Shutdown OP (OP 'S') - Shutdown
	///  <OP>.B <Appname>.?B
	///

	this->write(LOGLEVEL_INFO, "Enabled UDP Logging to %s:%u\n", ip, port);

	// Send "startup' packet.
	size_t szPacket = strlen(m_dbg_appname) + 2;
	char *packet = (char*)malloc(szPacket);
	if(packet != NULL) {
		packet[0] = 'H'; // Hello!
		strcpy(&packet[1], m_dbg_appname);
		packet[strlen(m_dbg_appname) + 1] = '\0'; // +1 in offste results because of prepending Opcode.
		sendto(m_dbg_sock, packet, (int)szPacket, 0, (SOCKADDR*)&m_dbg_addr, sizeof(m_dbg_addr));

		free(packet);
	}


	// register Hook:
	registerHook(LOGLEVEL_ALL, this, writeLineToUDP);

}//end: enableUDPConsole()


void logger::sendUDPConsolePerfData(int tick, int FPS, int UIFPS) {
	if(m_dbg_sock == INVALID_SOCKET)
		return;

#pragma pack(push,1)
	struct PerfData {
		char OP; // 'P"
		int tick;
		int FPS;		
		int UIFPS;
	} data;
#pragma pack(pop)

	data.OP = 'P';
	data.tick = tick;
	data.FPS = FPS;
	data.UIFPS = UIFPS;

	sendto(m_dbg_sock, (char*)&data, (int)sizeof(PerfData), 0, (SOCKADDR*)&m_dbg_addr, sizeof(m_dbg_addr));

}//end: sendUDPConsolePerfData()


bool logger::enableFileLogging(const char *logFile, unsigned int loggedMask){
	struct fileParam *node;
	FILE *fp;

	///
	fp = fopen(logFile, "w");
	if(fp == NULL)
		return false;

	node = (struct fileParam*)malloc(sizeof(struct fileParam));
	if(node == NULL){
		fclose(fp);
		return false;
	}

	node->fileName = strdup(logFile);
	if(node->fileName == NULL){
		fclose(fp);
		free(node);
		return false;
	}

	node->fp		= fp;
	node->loggedMask= loggedMask;
	
	
	m_lock_logToFileList.lock();

	node->next = m_logToFileListBegin;
	m_logToFileListBegin = node;

	m_lock_logToFileList.unlock();


	// subscribe hook.
	registerHook(loggedMask, node, writeLineToFile);


	return true;
}//end: enableFileLogging()



void logger::enableTerminalLogging(unsigned int loggedMask){
	
	// Unregister old Terminal Registration if its already registered
	if(m_termLogging_Mask != LOGLEVEL_NONE){
		unregisterHook(m_termLogging_Mask, NULL, writeLineToTerminal);
	}

	// Set New Current Mask
	m_termLogging_Mask = loggedMask;

	// Register if not disabled:
	if(loggedMask != LOGLEVEL_NONE){
		registerHook(loggedMask, NULL, writeLineToTerminal);
	}

}//end: enableTerminalLogging()


void logger::write(enum LOGLEVEL level, const char *msg, ...) {
	char buf[16384]; // poor stack :/
	size_t szBuf;
	va_list ap;
	//

	if(atomic::CompareAndSwap(&m_isInitialized, 1, 1) != 1)
		return;


	va_start(ap, msg);
	szBuf = vsnprintf_s(buf, sizeof(buf), msg, ap);
	va_end(ap);
	
	buf[sizeof(buf)-1] = '\0';

	//
	size_t szRequired = sizeof(struct qElem);
	if(szBuf >= SZ_QELEM_LINEBUF) {
		szRequired += (szBuf - SZ_QELEM_LINEBUF) + 1;
	}
	
	struct qElem *q = (struct qElem*)malloc(szRequired);
	q->next			= NULL;
	q->level		= level;
	q->tick			= this->get_Tick();
	*(q->dbg_file)	= '\0';
	*(q->dbg_func)	= '\0';
	q->dbg_line		= 0;

	memcpy(q->line, buf, szBuf);
	q->line[szBuf]	= '\0';
	q->szLine		= szBuf;

	// Add to Queue
	m_lock_queue.lock();
	
	if(m_qEnd == NULL) {
		//
		m_qbegin	= q;
		m_qEnd		= q;
	} else {
		//
		m_qEnd->next	= q;
		m_qEnd			= q;
	}

	m_lock_queue.unlock();

	
	// Signal Writer 
	m_cond_worker.signal();

}//end: write()



void logger::writeEx(enum LOGLEVEL level, const char *file, int line, const char *func, const char *msg, ...){
	char buf[16384]; // poor stack :/
	size_t szBuf;
	va_list ap;
	//

	if(atomic::CompareAndSwap(&m_isInitialized, 1, 1) != 1)
		return;

	va_start(ap, msg);
	szBuf = vsnprintf_s(buf, sizeof(buf), msg, ap);
	va_end(ap);

	buf[sizeof(buf)-1] = '\0';


	//
	size_t szRequired = sizeof(struct qElem);
	if(szBuf >= SZ_QELEM_LINEBUF) {
		szRequired += (szBuf - SZ_QELEM_LINEBUF) + 1;
	}

	struct qElem *q = (struct qElem*)malloc(szRequired);
	q->next			= NULL;
	q->level		= level;
	q->tick			= this->get_Tick();

	// Debug Info
	size_t szFile = strlen(file);
	if(szFile >= 48) {
		q->dbg_file[0] = '.';
		q->dbg_file[1] = '.';
		q->dbg_file[2] = '.';
		memcpy(&q->dbg_file[3], &file[szFile - 44], 44);
		q->dbg_file[sizeof(q->dbg_file) - 1] = '\0';
	} else {
		memcpy(q->dbg_file, file, szFile);
		q->dbg_file[szFile] = '\0';
	}

	size_t szFunc = strlen(func);
	if(szFile >= 48) {
		q->dbg_func[0] = '.';
		q->dbg_func[1] = '.';
		q->dbg_func[2] = '.';
		memcpy(&q->dbg_func[3], &func[szFunc - 44], 44);
		q->dbg_func[sizeof(q->dbg_func) - 1] = '\0';
	} else {
		memcpy(q->dbg_func, func, szFunc);
		q->dbg_func[szFunc] = '\0';
	}

	q->dbg_line		= line;

	memcpy(q->line, buf, szBuf);
	q->line[szBuf]	= '\0';
	q->szLine		= szBuf;

	// Add to Queue
	m_lock_queue.lock();

	if(m_qEnd == NULL) {
		//
		m_qbegin	= q;
		m_qEnd		= q;
	} else {
		//
		m_qEnd->next	= q;
		m_qEnd			= q;

	}
	m_lock_queue.unlock();


	// Signal Writer 
	m_cond_worker.signal();

}//end: writeEx()


void logger::registerHook(unsigned int subscribedMask, void *paramPtr, logHookProc proc) {
	
	auto node = (struct hookEntry *)malloc(sizeof(struct hookEntry));
	if(node == NULL) {
		fatalError("Logger - Out of Memory\nHook Registration failed\n");
		return;
	}
	///

	node->next			= NULL;
	node->subscribedMask= subscribedMask;
	node->paramPtr		= paramPtr;
	node->proc			= proc;
	

	m_lock_hooklist.lock();

	// insert @end
	if(m_hlEnd == NULL){

		m_hlBegin = node;
		m_hlEnd = node;

	}else{

		m_hlEnd->next = node;
		m_hlEnd = node;
	}

	
	m_lock_hooklist.unlock();

}//end: registerHook()


void logger::unregisterHook(unsigned int subscribedMask, void *paramPtr, logHookProc proc) {

	m_lock_hooklist.lock();

	struct hookEntry *it, *itl;
	it = m_hlBegin;
	itl = NULL;
	
	while(1){
		if(it == NULL)
			break;

		if(it->subscribedMask == subscribedMask && it->proc == proc && it->paramPtr == paramPtr){
			
			if(itl == NULL){
				// begin of hooklist!
				
				m_hlBegin = it->next;
				
				if(it->next == NULL)
					m_hlEnd = NULL;

			}else{
				// 
				itl->next = it->next;
				if(m_hlEnd == it){ // we were old last, so set prev to last
					m_hlEnd = itl;
				}

			}

			free(it); // .

			break; // found
		}


		itl = it;
		it = it->next;
	}

	m_lock_hooklist.unlock();

}//end: unregisterHook()


struct logger::qElem *logger::copyQElem(const qElem *qe, bool use_roalloc, size_t offset, size_t length){
	size_t szRequired = sizeof(struct qElem);
	size_t szLine = qe->szLine;
	
	if(offset>=szLine){
		offset = szLine;
		szLine = 0;
	}else{
		szLine -= offset;
	}

	if(length > 0 && szLine > length){
		szLine = length;
	}else{
		length = szLine;
	}


	if(szLine >= SZ_QELEM_LINEBUF) {
		szRequired += (szLine - SZ_QELEM_LINEBUF) + 1;
	}

	struct qElem *qout = NULL;

	if(use_roalloc == true){
		qout = (struct qElem*)roalloc(szRequired);
	}else{
		qout = (struct qElem*)malloc(szRequired);
	}

	
	if(offset > 0 || length > 0){
		// Copy struct infodata	
		memcpy(qout, qe, sizeof(struct qElem) - SZ_QELEM_LINEBUF);

		// Copy text
		memcpy(qout->line, &qe->line[offset], length);
		qout->line[length] = '\0';

		qout->szLine = length;

	}else{
		// copy whole data 
		memcpy(qout, qe, szRequired);

	}

	return qout;
}//end: copyQElem()


void logger::copyFileLogFiles(const char *directory) {
	char destName[512];

	m_lock_logToFileList.lock();
	m_lock_queue.lock(); // prevent logging more stuff


	for(auto it = m_logToFileListBegin; it != NULL; it = it->next) {

		sprintf_s(destName, "%s%s", directory, it->fileName);

		// ensure that the path exists
		for(char *p = destName; *p != '\0'; p++) {
			char c = *p;
			if(c == '/' || c == '\\') {
				*p = '\0';
				CreateDirectory(destName, NULL);
				*p = c;
			}
		}

		CopyFile(it->fileName, destName, FALSE);
	}

	m_lock_queue.unlock();
	m_lock_logToFileList.unlock();

}//end: copyFileLogLogFiles()


const char *logger::logType2Str(enum LOGLEVEL lt){
	// Note: A type String Couldn't be larger than 15 Bytes
	// If you require to add a type which's name is larger than this 
	// you'll have to check the whole log-implementation (especially write* procs)
	//

	switch(lt){
		case LOGLEVEL_INFO:		return "INFO";
		case LOGLEVEL_WARNING:	return "WARN";
		case LOGLEVEL_ERROR:	return "ERROR";
		case LOGLEVEL_MEMMGR:	return "MEMMGR";
		case LOGLEVEL_DEBUG:	return "DEBUG";
		case LOGLEVEL_SQL:		return "SQL";
		case LOGLEVEL_NOTICE:	return "NOTICE";
		case LOGLEVEL_STATUS:	return "STATUS";
		case LOGLEVEL_FATALERROR: return "FATAL ERROR";
		default:				return "UNKNOWN";
	}

}//end: logType2Str()


void logger::writeLineToFile(void *paramPtr, const struct qElem *elem) {
	char extraBuf[96], typeBuf[16];
	size_t szExtraBuf;
	struct fileParam *param = (struct fileParam*)paramPtr;

	sprintf_s(typeBuf, "(%s)", logType2Str(elem->level));

	szExtraBuf = sprintf(extraBuf, "[%0.5f] %-9s ", ((double)elem->tick) / 1000.0, typeBuf );



	fwrite(extraBuf, szExtraBuf, 1, param->fp);
	fwrite(elem->line, elem->szLine, 1, param->fp);

	// force sync write.
	fflush(param->fp);

}//end: writeLineToFile()


void logger::writeLineToUDP(void *paramPtr, const struct qElem *elem) {
	logger *_this = (logger*)paramPtr;

#pragma pack(push,1)
	struct header {
		char op;
		__int64 tick;
		unsigned char level;
	};
#pragma pack(pop)


	size_t szPacket = sizeof(struct header) + elem->szLine + 1;
	char *packet = (char*)malloc(szPacket);
	if(packet != NULL) {
		struct header *head = (struct header*)packet;
		char *ptrMsg = &packet[sizeof(struct header)];

		head->op = 'M'; // message.
		head->tick = elem->tick;
		head->level = elem->level;

		memcpy(ptrMsg, elem->line, elem->szLine);
		ptrMsg[elem->szLine] = '\0';

		sendto(_this->m_dbg_sock, packet, (int)szPacket, 0, (SOCKADDR*)&_this->m_dbg_addr, sizeof(_this->m_dbg_addr));

		free(packet);
	}

}//end: writeLineToUDP()


void logger::writeLineToTerminal(void *termParam, const struct qElem *line) {
	char extraBuf[96], typeBuf[16];
	size_t szExtraBuf;
	sprintf_s(typeBuf, "(%s)", logType2Str(line->level));

	szExtraBuf = sprintf(extraBuf, "[%0.5f] %-9s ", ((double)line->tick) / 1000.0, typeBuf);


	printf("%s%s", extraBuf, line->line);

}//end: writeLineToTerminal()


DWORD _stdcall logger::WorkerProc(thread *t) {
	logger *_this = (logger*)t->getStartParameter();
	struct qElem *elem = NULL;

	while(1) {
		
		// Try to fetch one line from queue.
		_this->m_lock_queue.lock();
			
		elem = _this->m_qbegin; 
		if(elem != NULL) {
			// got one
			if(elem->next == NULL) {
				// last elem, set pointers null
				_this->m_qbegin	= NULL;
				_this->m_qEnd	= NULL;
			} else {
				// 
				_this->m_qbegin = elem->next;
			}

		}

		_this->m_lock_queue.unlock();

		//
		if(elem != NULL) {
			// got one line
			
			_this->m_lock_hooklist.lock();
			for(auto it = _this->m_hlBegin; it != NULL; it = it->next) {
				
				if(it->subscribedMask & elem->level)
					it->proc(it->paramPtr, elem);

			}
			_this->m_lock_hooklist.unlock();

			free(elem);

			continue; // try to fetch another.
		}


		// 
		if(atomic::CompareAndSwap(&_this->m_isInitialized, 1, 1) != 1) {
			break;// exit loop.			
		}


		// wait for condition var gets singalled (sleep thread)
		_this->m_lock_workercond.lock();
		_this->m_cond_worker.wait(&_this->m_lock_workercond, 3000);
		_this->m_lock_workercond.unlock();
	}//


	return 0;
}//end: WriterProc()



}//end namespace util
}//end namespace blockz