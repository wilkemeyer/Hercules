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
#include "stdafx.h"
#include "../stdafx.h"

using namespace rgCore;
using namespace rgCore::Logging;


static char l_SymbolSearchPath[256] = {0},
			l_DumpPath[256] = {0};
static bool l_catchExceptions = false;
static bool l_writeMinidump = false;
static bool l_minidumpFullMem = false;

static HANDLE l_HPROC = nullptr;

#define DI_MAX 64
static DebugInfo l_di_data[DI_MAX] = {0};
__declspec(align(16)) static volatile LONG l_di_pos = 0;


static LONG WINAPI UnhandledExeptFilter(PEXCEPTION_POINTERS pExceptionInfo);




bool debug_init(){

	iniGetString(".\\Settings\\Debug.ini",  "Debug", "SymbolSearchPath",	"",					l_SymbolSearchPath, sizeof(l_SymbolSearchPath) );
	iniGetString(".\\Settings\\Debug.ini",  "Debug", "DumpPath",			".\\Crashdumps\\",	l_DumpPath, sizeof(l_DumpPath) );
	l_catchExceptions = iniGetBoolean(".\\Settings\\Debug.ini", "Debug", "CatchExceptions", false);
	l_writeMinidump = iniGetBoolean(".\\Settings\\Debug.ini", "Debug", "WriteMinidump", false);
	l_minidumpFullMem = iniGetBoolean(".\\Settings\\Debug.ini", "Debug", "MiniDumpFullMemory", false);

	CreateDirectory(l_DumpPath, NULL);

	l_HPROC = ::GetCurrentProcess();
	if(l_HPROC == nullptr){
		fatalError("DebugInit Failed", "Failed to receive the Process's HANDLE (GetCurrentProcess Failed) - %u", GetLastError() );
	}

	
	DWORD Options = SymGetOptions();
	Options |= SYMOPT_DEBUG;
	Options |= SYMOPT_DEFERRED_LOADS;
	SymSetOptions(Options);

	if( SymInitialize(l_HPROC,  l_SymbolSearchPath,  TRUE)  == FALSE){
		fatalError("DebugInit Failed",  "SymInitialize failed.  - %u", GetLastError() );
		return false;
	}

	if(l_catchExceptions == true){
		SetUnhandledExceptionFilter(UnhandledExeptFilter);
	}
	return true;
}//end: debug_init()


void debug_final(){
	if(l_HPROC != nullptr)	
		SymCleanup(l_HPROC);

	return;
}//end: debug_final()





DebugInfo *debugInfoGet(void *Address){
	DebugInfo *di;
__retry_slot:
	int pos = InterlockedIncrement(&l_di_pos);

	if(pos >= DI_MAX){
		InterlockedExchange(&l_di_pos, 0);
		goto __retry_slot;		
	}

	di = &l_di_data[pos];

	SYMBOL_INFO_PACKAGE sip;
	IMAGEHLP_LINE64 line;

	DWORD Displacement32;
	DWORD64 Displacement;

	Displacement32 = 0;
	line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
	if(SymGetLineFromAddr64(l_HPROC,  (DWORD64)Address,  &Displacement32,  &line) == FALSE ){
		strcpy(di->file, "<unknown>");
		di->line = 0;
	}else{
		char *p;
		p = &line.FileName[strlen(line.FileName)];
		while(1){
			if(*p == '/'){
				p++;
				break;
			}
			if(*p == '\\'){
				p++;			
				break;
			}
			if(p == line.FileName)
				break;
		p--;
		}

		strncpy(di->file, p, 0xfe);  di->file[0xfe] = 0;
		di->line = line.LineNumber;
	}


	// Lookup functionname
	Displacement = 0;
	sip.si.SizeOfStruct = sizeof(SYMBOL_INFO);
	sip.si.MaxNameLen = sizeof(sip.name);

	if(SymFromAddr(l_HPROC,  (DWORD64)Address, &Displacement, &sip.si) == FALSE){
		strcpy(di->func, "<unknown>");
	}else{
		strncpy(di->func, sip.si.Name, 0xfe);	di->func[0xfe] = 0;
	}

	di->address = (size_t)Address;

	return di;
}



//
// ===========================================================================
// ===========================================================================
// ===========================================================================
static void hprintf(HANDLE Handle, const char *fmt, ...){
	DWORD outbytes;
	char tmpbuf[65536];
	int len;
	va_list ap;

	va_start(ap, fmt);
	len = vsnprintf(tmpbuf, 65535, fmt, ap);
	va_end(ap);
	if(Handle != INVALID_HANDLE_VALUE)
		WriteFile(Handle, tmpbuf, len, &outbytes, NULL);
}// 


static const char *GetExceptionString( DWORD dwCode )
{
    #define EXCEPTION( x ) case EXCEPTION_##x: return #x;

    switch ( dwCode )
    {
        EXCEPTION( ACCESS_VIOLATION )
            EXCEPTION( DATATYPE_MISALIGNMENT )
            EXCEPTION( BREAKPOINT )
            EXCEPTION( SINGLE_STEP )
            EXCEPTION( ARRAY_BOUNDS_EXCEEDED )
            EXCEPTION( FLT_DENORMAL_OPERAND )
            EXCEPTION( FLT_DIVIDE_BY_ZERO )
            EXCEPTION( FLT_INEXACT_RESULT )
            EXCEPTION( FLT_INVALID_OPERATION )
            EXCEPTION( FLT_OVERFLOW )
            EXCEPTION( FLT_STACK_CHECK )
            EXCEPTION( FLT_UNDERFLOW )
            EXCEPTION( INT_DIVIDE_BY_ZERO )
            EXCEPTION( INT_OVERFLOW )
            EXCEPTION( PRIV_INSTRUCTION )
            EXCEPTION( IN_PAGE_ERROR )
            EXCEPTION( ILLEGAL_INSTRUCTION )
            EXCEPTION( NONCONTINUABLE_EXCEPTION )
            EXCEPTION( STACK_OVERFLOW )
            EXCEPTION( INVALID_DISPOSITION )
            EXCEPTION( GUARD_PAGE )
            EXCEPTION( INVALID_HANDLE )
    }

    // If not one of the "known" exceptions, try to get the string
    // from NTDLL.DLL's message table.

    static char szBuffer[1024] = { 0 };

    FormatMessage( FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_HMODULE,
        GetModuleHandle( "NTDLL.DLL" ),
        dwCode, 0, szBuffer, sizeof( szBuffer ), 0 );

    return szBuffer;
}



//=============================================================================
// Given a linear address, locates the module, section, and offset containing
// that address.
//
// Note: the szModule paramater buffer is an output buffer of length specified
// by the len parameter (in characters!)
//=============================================================================
static BOOL GetLogicalAddress(PVOID addr, PTSTR szModule, DWORD len, DWORD& section, DWORD_PTR& offset )
{
    MEMORY_BASIC_INFORMATION mbi;

    if ( !VirtualQuery( addr, &mbi, sizeof(mbi) ) )
        return FALSE;

    DWORD_PTR hMod = (DWORD_PTR)mbi.AllocationBase;

    if ( !GetModuleFileName( (HMODULE)hMod, szModule, len ) )
        return FALSE;

    // Point to the DOS header in memory
    PIMAGE_DOS_HEADER pDosHdr = (PIMAGE_DOS_HEADER)hMod;

    // From the DOS header, find the NT (PE) header
    PIMAGE_NT_HEADERS pNtHdr = (PIMAGE_NT_HEADERS)(hMod + DWORD_PTR(pDosHdr->e_lfanew));

    PIMAGE_SECTION_HEADER pSection = IMAGE_FIRST_SECTION( pNtHdr );

    DWORD_PTR rva = (DWORD_PTR)addr - hMod;                         // RVA is offset from module load address

    // Iterate through the section table, looking for the one that encompasses
    // the linear address.
    for (   unsigned i = 0;
        i < pNtHdr->FileHeader.NumberOfSections;
        i++, pSection++ )
    {
        DWORD_PTR sectionStart = pSection->VirtualAddress;
        DWORD_PTR sectionEnd = sectionStart
            + DWORD_PTR(max(pSection->SizeOfRawData, pSection->Misc.VirtualSize));

        // Is the address in this section???
        if ( (rva >= sectionStart) && (rva <= sectionEnd) )
        {
            // Yes, address is in the section.  Calculate section and offset,
            // and store in the "section" & "offset" params, which were
            // passed by reference.
            section = i+1;
            offset = rva - sectionStart;
            return TRUE;
        }
    }

    return FALSE;                                           // Should never get here!
}


static void WriteStackDump(HANDLE ReportFile, PCONTEXT tcr, HANDLE Thread){
	SYMBOL_INFO_PACKAGE sip;
    IMAGEHLP_LINE64 lineInfo = { sizeof(IMAGEHLP_LINE) };
    DWORD dwLineDisplacement;
	STACKFRAME64 sf;
	DWORD64 symDisplacement = 0;
	DWORD section;
	DWORD_PTR offset;
	char buf[2048];

	memset(&sf, 0x00, sizeof(STACKFRAME64));

	 hprintf(ReportFile, "Call Stack:\r\n");
	 hprintf(ReportFile, "Address           Frame             Function             SourceFile\r\n");
	 sf.AddrPC.Offset		= tcr->Rip;
	 sf.AddrPC.Mode			= AddrModeFlat;
	 sf.AddrStack.Offset    = tcr->Rsp;
     sf.AddrStack.Mode      = AddrModeFlat;
     sf.AddrFrame.Offset    = tcr->Rbp;
     sf.AddrFrame.Mode      = AddrModeFlat;

	 while(1){
		 if(!StackWalk64(IMAGE_FILE_MACHINE_AMD64, l_HPROC, 
			 Thread, 
			 &sf, 
			 &tcr, 
			 NULL, 
			 SymFunctionTableAccess64, 
			 SymGetModuleBase64,
			 NULL) 
			)
				 break;
		 
		 if(sf.AddrFrame.Offset == 0)
			 break;
		
		 hprintf(ReportFile, "%016I64X  %016I64X  ", sf.AddrPC.Offset, sf.AddrFrame.Offset);

		 sip.si.SizeOfStruct = sizeof(SYMBOL_INFO);
		 sip.si.MaxNameLen = sizeof(sip.name);
	 	 //
		
		 if(SymFromAddr(l_HPROC, sf.AddrPC.Offset, &symDisplacement, &sip.si) )
			 hprintf(ReportFile, "%hs+%I64X", sip.si.Name, symDisplacement);
		 else{
			 // print address when no name found ~ 
			 GetLogicalAddress((PVOID)sf.AddrPC.Offset, buf,  2047, section, offset);
			 buf[2047] = '\0';
			 hprintf(ReportFile, "%04X:%016I64X %s", section, offset, buf);
		 }

		 if(SymGetLineFromAddr64(l_HPROC, sf.AddrPC.Offset, &dwLineDisplacement, &lineInfo))
			 hprintf(ReportFile, "  %s:%u", lineInfo.FileName, lineInfo.LineNumber);

		 hprintf(ReportFile, "\r\n");

	 }


}



// Used to Draw the Error to window 
struct CrashInfo{
	bool gotType;
	DWORD type;

	bool gotModule;
	char module[512];
	DWORD64 address, section, offset;
	DebugInfo di;


	bool gotFolder;
	char FolderName[1024];

	bool cd_Started;
	bool cd_Done;

	bool gotRegs;
	struct{
		  DWORD64 rax, rbx, rcx, rdx, rsi, rdi, 
				  r8, r9, r10, r11, r12, r13, r14,
				  r15;
          
		  DWORD64 segCS, rip, 
				  segSS, rsp, rbp,
				  segDS, segES, segFS, segGS,
				  flags;			
	} regs;
	

	bool md_Started;
	bool md_Done;
	bool done;
} CrashInfo;




static void GenerateCrashDump(HANDLE ReportFile, PEXCEPTION_POINTERS ep){
	PEXCEPTION_RECORD pr;
	PCONTEXT cr;
	CONTEXT tcr;
	SYSTEMTIME systime;
//	STACKFRAME64 sf;
	DWORD64 symDisplacement = 0;
	DWORD section;
	DWORD_PTR offset;
	char buf[2048];
    HANDLE hThreadSnap;
    THREADENTRY32 te32;
    DWORD dwOwnerPID;
	HANDLE ThreadHandle;

	pr = ep->ExceptionRecord;
	cr = ep->ContextRecord;

	//
	CrashInfo.type = pr->ExceptionCode;
	CrashInfo.gotType = true;
	//

	GetLocalTime(&systime);
	hprintf(ReportFile, "Date: %04u-%02u-%02u - %02u:%02u:%02u\r\n", systime.wYear, systime.wMonth, systime.wDay, systime.wHour, systime.wMinute, systime.wSecond);
	hprintf(ReportFile, "======================================\r\n");
	//hprintf(ReportFile, "Version: %u.%u.%u (%u), Debug: %s\r\n", version->epi_major, version->epi_minor, version->epi_patch, version->epi_ver, (version->is_debug == true)?"Yes":"No");
	/*if(version != nullptr){
		hprintf(ReportFile, "Build Date: %s (%u)\r\n", version->builddateSTR, version->builddate);
		hprintf(ReportFile, "Build Host: %s\r\n", version->buildhost);
		hprintf(ReportFile, "Build NO: #%u\r\n", version->buildno);
	}*/
	hprintf(ReportFile, "Compiler Version: %u\r\n", _MSC_FULL_VER);
	hprintf(ReportFile, "=======================================\r\n\r\n");
	
	hprintf(ReportFile, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\r\n");
	hprintf(ReportFile, "Exception Code: %08X (%s)\r\n", pr->ExceptionCode, GetExceptionString(pr->ExceptionCode));
	if(GetLogicalAddress(pr->ExceptionAddress, buf, 2048, section, offset) != FALSE){
		buf[2047] = '\0';
		hprintf(ReportFile, "Fault Address: %016I64X %02X:%016I64X %s\r\n", pr->ExceptionAddress, section, offset, buf);

		strncpy(CrashInfo.module, buf, 512);
		CrashInfo.address = (DWORD64)pr->ExceptionAddress; 
		CrashInfo.offset = offset;
		CrashInfo.section = section;
		CrashInfo.gotModule = true;
		
		memcpy(&CrashInfo.di, debugInfoGet(pr->ExceptionAddress), sizeof(DebugInfo));
	}	
	hprintf(ReportFile, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\r\n\r\n");
	
	hprintf(ReportFile, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\r\n");
	hprintf(ReportFile, "Registers:\r\n");
    hprintf(ReportFile, "RAX:%016I64X\r\n"
						"RBX:%016I64X\r\n"
						"RCX:%016I64X\r\n"
						"RDX:%016I64X\r\n"
						"RSI:%016I64X\r\n"
						"RDI:%016I64X\r\n"
						"R8: %016I64X\r\n"
						"R9: %016I64X\r\n"
						"R10:%016I64X\r\n"
						"R11:%016I64X\r\n"
						"R12:%016I64X\r\n"
						"R13:%016I64X\r\n"
						"R14:%016I64X\r\n"
						"R15:%016I64X\r\n",
						cr->Rax, cr->Rbx, cr->Rcx,
						cr->Rdx, cr->Rsi, cr->Rdi,
						cr->R8, cr->R9, cr->R10,
						cr->R11, cr->R12, cr->R13,
						cr->R14, cr->R14, cr->R15);

     hprintf(ReportFile, "CS:RIP:%04X:%016I64X\r\n", cr->SegCs, cr->Rip );
	 hprintf(ReportFile, "SS:RSP:%04X:%016X   RBP:%08X\r\n", cr->SegSs, cr->Rsp, cr->Rbp );
     hprintf(ReportFile, "DS:%04X  ES:%04X  FS:%04X  GS:%04X\r\n", cr->SegDs, cr->SegEs, cr->SegFs, cr->SegGs );
     hprintf(ReportFile, "Flags:%08X\r\n", cr->EFlags );	
 	 hprintf(ReportFile, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\r\n\r\n");

	 CrashInfo.regs.rax = cr->Rax;		CrashInfo.regs.rbx = cr->Rbx;			CrashInfo.regs.rcx = cr->Rcx;
	 CrashInfo.regs.rdx = cr->Rdx;		CrashInfo.regs.rsi = cr->Rsi;			CrashInfo.regs.rdi = cr->Rdi;
	 CrashInfo.regs.r8  = cr->R8;		CrashInfo.regs.r9  = cr->R9;			CrashInfo.regs.r10 = cr->R10;
	 CrashInfo.regs.r11 = cr->R11;		CrashInfo.regs.r12 = cr->R12;			CrashInfo.regs.r13 = cr->R13;
	 CrashInfo.regs.r14 = cr->R14;		CrashInfo.regs.r15 = cr->R15;			//CrashInfo.regs.r16 = cr->R10;
	 CrashInfo.regs.segCS = cr->SegCs;	CrashInfo.regs.rip = cr->Rip;
	 CrashInfo.regs.segSS = cr->SegSs;	CrashInfo.regs.rsp = cr->Rsp;			CrashInfo.regs.rbp = cr->Rbp;
	 CrashInfo.regs.segDS = cr->SegDs;	CrashInfo.regs.segES = cr->SegEs;		CrashInfo.regs.segFS = cr->SegFs;
	 CrashInfo.regs.segGS = cr->SegGs;
	 CrashInfo.regs.flags = cr->EFlags;

	 CrashInfo.gotRegs = true;


	 tcr = *cr;
	 WriteStackDump(ReportFile, &tcr, GetCurrentThread());
     hprintf(ReportFile, "\r\n");

	 // For Every Thread!
	 dwOwnerPID = GetCurrentProcessId();
	 hThreadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
	 if(hThreadSnap != INVALID_HANDLE_VALUE){
		te32.dwSize = sizeof(THREADENTRY32);
		if(Thread32First( hThreadSnap, &te32 )){
			do{
				if(te32.th32OwnerProcessID == dwOwnerPID){
					tcr.ContextFlags = 0xffffffff;
					
					DWORD threadFlags = THREAD_GET_CONTEXT | THREAD_QUERY_INFORMATION;
					if(GetCurrentThreadId() != te32.th32ThreadID)
						threadFlags |= THREAD_SUSPEND_RESUME;

					ThreadHandle = OpenThread(threadFlags, false, te32.th32ThreadID);
					if(ThreadHandle && (GetCurrentThreadId() != te32.th32ThreadID))
						SuspendThread(ThreadHandle); // pause thread, (halt exec)

					if(ThreadHandle && GetThreadContext(ThreadHandle, &tcr)){
						hprintf(ReportFile, "~~~~~~~~~~~~~~~~~~~~~~~~~~\r\n");
						hprintf(ReportFile, " Thread ID: %u\r\n", te32.th32ThreadID);
						WriteStackDump(ReportFile, &tcr, ThreadHandle);
						hprintf(ReportFile, "\r\n\r\n");
					}

					if(ThreadHandle){
						CloseHandle(ThreadHandle);
					}

				}
			}while(Thread32Next(hThreadSnap, &te32) );
		}
	 CloseHandle(hThreadSnap);
	 }
		
	
}// end GenerateCrashDump


/*
void debug_draw_BSOD(void *hdc){
	HDC dc = (HDC)hdc;
	int x=0;

	SetBkColor(dc, RGB(0x00, 0x00,0xAA));
	SetTextColor(dc, RGB(0xFF, 0xFF, 0xFF));
	TextOut(dc, 8, 10, "An unexpected error has occured, the Application has been terminated to prevent Damages.", 88);
	x = 32;

	if(CrashInfo.gotType == true){
		char buf[1024];
		int buflen = sprintf(buf, "0x%08X: %s", CrashInfo.type, GetExceptionString(CrashInfo.type) );
		TextOut(dc, 12, x, buf, buflen);
		x+=20;
	}

	if(CrashInfo.gotModule == true){
		char buf[1024];
		int buflen = sprintf(buf, "***  %s - Address %016X offset %016X section %016X", CrashInfo.module, CrashInfo.address, CrashInfo.offset, CrashInfo.section);
		TextOut(dc, 8, x, buf, buflen);
		x+=16;
		buflen = sprintf(buf, "***  File: %s:%u Function: %s", CrashInfo.di.file, CrashInfo.di.line, CrashInfo.di.func);
		TextOut(dc, 8, x, buf, buflen);
		x+=30;
	}

	if(CrashInfo.gotRegs == true){
		char buf[6][384];
		int buflen[6];

		buflen[0] = sprintf(buf[0], "RAX: %016X    RBX: %016X    RCX: %016X    RDX: %016X", CrashInfo.regs.rax, CrashInfo.regs.rbx, CrashInfo.regs.rcx, CrashInfo.regs.rdx);
		buflen[1] = sprintf(buf[1], "RSI: %016X    RDI: %016X     R8: %016X     R9: %016X", CrashInfo.regs.rsi, CrashInfo.regs.rdi, CrashInfo.regs.r8, CrashInfo.regs.r9);
		buflen[2] = sprintf(buf[2], "R10: %016X    R11: %016X    R12: %016X    R13: %016X", CrashInfo.regs.r10, CrashInfo.regs.r11, CrashInfo.regs.r12, CrashInfo.regs.r13);
		buflen[3] = sprintf(buf[3], "R14: %016X    R15: %016X ", CrashInfo.regs.r14, CrashInfo.regs.r15);
		buflen[4] = sprintf(buf[4], "RIP: %016X    RSP: %016X    RBP: %016X", CrashInfo.regs.rip, CrashInfo.regs.rsp, CrashInfo.regs.rbp);
		buflen[5] = sprintf(buf[5], "FLAGS: %016X",	CrashInfo.regs.flags);

		TextOut(dc, 90, x, buf[0], buflen[0]);
		x+=16;
		TextOut(dc, 90, x, buf[1], buflen[1]);
		x+=16;
		TextOut(dc, 90, x, buf[2], buflen[2]);
		x+=16;
		TextOut(dc, 90, x, buf[3], buflen[3]);
		x+=16;
		TextOut(dc, 90, x, buf[4], buflen[4]);
		x+=16;
		TextOut(dc, 90, x, buf[5], buflen[5]);
		x+=16;

		x+=30;
	}

	// 
	TextOut(dc, 8, x, "=========== Last Log Output ===========", 39);
	x += 16;
	x += log_BSODDrawToWND(hdc, 8, x);
	TextOut(dc, 8, x, "====================================", 36);
	x+= 30;

	if(CrashInfo.cd_Started == true){
		TextOut(dc, 8, x, "Beginning write of CrashDump", 28);
		x+=16;
	}

	if(CrashInfo.cd_Done == true){
		TextOut(dc, 8, x, "CrashDump complete.", 20);
		x+=20;
	}


	if(l_writeMinidump == true){
		if(CrashInfo.md_Started == true){
			if(l_minidumpFullMem == false)
				TextOut(dc,8, x, "Beginning write of minidump", 27);
			else
				TextOut(dc,8, x, "Beginning write of minidump (with full memory)",  46);
			x+=16;
		}
		if(CrashInfo.md_Done == true){
			TextOut(dc,8, x, "Minidump complete.", 18);
			x+=20;
		}
	}else{
		TextOut(dc, 8, x, "Minidump disabled.", 18);
		x+=20;
	}

	if(CrashInfo.done == true){
		if(CrashInfo.gotFolder == true){
			char buf[1536];
			x+= 10;
			int buflen =  sprintf(buf, "*** Dump Folder: %s", CrashInfo.FolderName);
			TextOut(dc, 8, x, buf, buflen);
			x+= 20;
		}

		TextOut(dc, 8, x, "Software halted.", 16);
		x+=16;

	}

}
*/


// ================
static LONG WINAPI UnhandledExeptFilter(PEXCEPTION_POINTERS pExceptionInfo){
	SYSTEMTIME systime;
	HANDLE ReportFile = INVALID_HANDLE_VALUE;
	MINIDUMP_EXCEPTION_INFORMATION MI;
	char folder_name[256];
	char buf[256];
		
	CrashInfo.gotType = false;
	CrashInfo.gotFolder = false;
	CrashInfo.gotModule = false;
	CrashInfo.gotRegs = false;
	CrashInfo.md_Done = false;
	CrashInfo.md_Started = false;
	CrashInfo.done = false;
	CrashInfo.cd_Started = false;
	CrashInfo.cd_Done = false;

//	threadBSODHaltEverything();
	//SendMessage(g_HWND, WM_COMMAND, LOWORD(15200), 0); // set BSOD

	// Create Crashdump Folder
	GetLocalTime(&systime);
	sprintf(folder_name, "%s%s [%04u-%02u-%02u_%02u-%02u-%02u]\\", l_DumpPath, rgCore_getAppName(), systime.wYear, systime.wMonth, systime.wDay, systime.wHour, systime.wMinute, systime.wSecond);
	if(CreateDirectory(folder_name, NULL) == FALSE){
		MessageBox(NULL, "Cannot Create Folder for Tracelog, Crashlog and Dump", "Exception handling error", MB_ICONSTOP | MB_OK);
		return EXCEPTION_CONTINUE_SEARCH; // ~
	}
	// =========================================

	strncpy(CrashInfo.FolderName, folder_name, 1024);
	CrashInfo.gotFolder = true;

	
	// Flush Log & Copy Log Files
	coreLogger *cl = (coreLogger*)g_globalLogger;
	if(cl != NULL){

		cl->flushLogging();
		cl->copyFileLogFiles(folder_name);

	}


	// =========================================


	// Generate own Crash Dump with stacktrace (like WeathyExcept Handler)
	CrashInfo.cd_Started = true;
	sprintf(buf, "%sCrashDump.txt", folder_name);
	ReportFile = CreateFile(buf, GENERIC_WRITE, 0, 0, OPEN_ALWAYS, FILE_FLAG_WRITE_THROUGH, NULL);
	if(ReportFile != INVALID_HANDLE_VALUE){
		SetFilePointer(ReportFile, 0, 0, FILE_END);
		GenerateCrashDump(ReportFile, pExceptionInfo);
		CloseHandle(ReportFile);
		ReportFile = INVALID_HANDLE_VALUE;
	}
	// =========================================
	CrashInfo.cd_Done = true;



	// Generate / Write MINIDUMP!
	if(l_writeMinidump == true){
		sprintf(buf, "%sMiniDump.dmp", folder_name);
	
		MI.ClientPointers = NULL;
		MI.ExceptionPointers = pExceptionInfo;
		MI.ThreadId = GetCurrentThreadId();
		
		MINIDUMP_TYPE t;
		if(l_minidumpFullMem == false)
			t = (MINIDUMP_TYPE)(MiniDumpNormal | MiniDumpWithDataSegs  | MiniDumpFilterMemory | MiniDumpScanMemory | MiniDumpWithIndirectlyReferencedMemory |  MiniDumpWithProcessThreadData | MiniDumpWithFullMemoryInfo | MiniDumpWithThreadInfo | MiniDumpWithCodeSegs | MiniDumpIgnoreInaccessibleMemory);
		else
			t = (MINIDUMP_TYPE)(MiniDumpNormal | MiniDumpWithDataSegs  | MiniDumpFilterMemory | MiniDumpScanMemory | MiniDumpWithIndirectlyReferencedMemory |  MiniDumpWithProcessThreadData | MiniDumpWithFullMemory | MiniDumpWithFullMemoryInfo | MiniDumpWithThreadInfo | MiniDumpWithCodeSegs | MiniDumpIgnoreInaccessibleMemory);

		CrashInfo.md_Started = true;
		ReportFile = CreateFile(buf, GENERIC_WRITE, 0, 0, OPEN_ALWAYS, FILE_FLAG_WRITE_THROUGH, NULL);
		MiniDumpWriteDump(l_HPROC, GetCurrentProcessId(), ReportFile,
							t, 
							&MI, NULL, NULL);
		CloseHandle(ReportFile);
		// =========================================

		CrashInfo.md_Done = true;
	}


	// 
	CrashInfo.done = true;

	if(MessageBox(NULL, "Unexpected error occured\n\nDo you want to Debug?", "Exception", MB_ICONSTOP | MB_YESNO) == IDYES)
			return 0;
	else
			exit(EXIT_FAILURE);

return 0;
}
