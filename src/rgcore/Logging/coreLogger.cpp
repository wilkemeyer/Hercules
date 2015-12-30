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

namespace rgCore { namespace Logging {

coreLogger *coreLogger::m_inst = NULL;

void coreLogger::init(){

	m_inst = new coreLogger();
	m_inst->startLogger();


}//end: init()


void coreLogger::final(){

	if(m_inst != NULL){
		delete m_inst;	
		m_inst = NULL;
	}

}//end: final()


coreLogger *coreLogger::get(){
	return m_inst;
}//end: get()


coreLogger::coreLogger(){
	//
	// Enable Terminal Logging (as long as there is no GUI)
	//
	enableTerminalLogging(LOGLEVEL_ALL);


	//
	// TraceLog (Logs Everything)
	//
	{
		char logName[128];
		sprintf_s(logName, "%s.Trace.log", rgCore_getAppName() );

		enableFileLogging(logName, LOGLEVEL_ALL);
	}

	//
	// ErrorLog
	//
	{
		char logName[128];
		sprintf_s(logName, "%s.Error.log", rgCore_getAppName());

		enableFileLogging(logName, (LOGLEVEL_ERROR|LOGLEVEL_FATALERROR));
	}

}//end: constructor()



__int64 coreLogger::get_Tick(){
	return Time::tick_get();	// It's valid to call tick_get before its initialized, as the specification defines that case - it must return: 0 - 
}//end: get_Tick()


} }