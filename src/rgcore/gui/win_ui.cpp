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

namespace rgCore { namespace gui {

using namespace rgCore::Logging;

#define WNDCLASSNAME "roorgServerWindowCls"
#define WND_WIDTH 1024
#define WND_HEIGHT 800
#define WND_MAX_LOGLINES (WND_HEIGHT - 80)/16
#define WND_MAX_LOGLINELENGTH 72

win_ui::win_ui(){
	WNDCLASSEX cls;


	m_hInst = (HINSTANCE)GetModuleHandle(NULL);
	m_hWnd = NULL;

	m_font_normal = NULL;
	m_font_underline = NULL;
	m_blackpen = NULL;
	

	m_logLineFirst	= NULL;
	m_logLineLast	= NULL;
	m_logLine_num	= 0;


	m_logLine_lock = new util::spinlock();


	// Default Settings:
	m_bIsAutorefresh = true;
	

	// Window Class ->
	memset(&cls, 0x00, sizeof(cls));
	cls.cbSize = sizeof(WNDCLASSEX);
	cls.style = CS_HREDRAW | CS_VREDRAW;
	cls.lpfnWndProc = win_ui::wndProc;
	cls.cbWndExtra = 0;
	cls.cbClsExtra = 0;
	cls.hIcon = NULL; // LoadIcon(m_hInst, (LPCSTR)IDI_ICON3);
	cls.hIconSm = NULL; // LoadIcon(m_hInst, (LPCSTR)IDI_ICON2);
	cls.hCursor = LoadCursor(NULL, IDC_ARROW);
	cls.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	cls.lpszMenuName = NULL;
	cls.lpszClassName = WNDCLASSNAME;
	cls.hInstance = m_hInst;

	////
	RegisterClassEx(&cls);

	// Create Window
	char wndTitle[256];
#ifdef _DEBUG
	sprintf(wndTitle, "%s (DEBUG) - %s %s", rgCore_getAppName(), __DATE__, __TIME__);
#else
	sprintf(wndTitle, "%s - %s %s", rgCore_getAppName(), __DATE__, __TIME__);
#endif
	m_hWnd = CreateWindow(WNDCLASSNAME,
						wndTitle, 
						WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
						CW_USEDEFAULT,
						CW_USEDEFAULT,
						WND_WIDTH,
						WND_HEIGHT,
						NULL,
						NULL,
						m_hInst,
						this);

	if(m_hWnd == NULL) {
		fatalError("Window Creation Failed (%u)\n", GetLastError());
		return;
	}



	// Initialize Text utils
	m_blackpen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
	{
		LOGFONT lf;
		memset(&lf, 0x00, sizeof(LOGFONT));
		
		//
		lf.lfHeight = 0;
		lf.lfWidth = 0;
		lf.lfEscapement = 0;
		lf.lfOrientation = 0;
		lf.lfWeight = FW_DONTCARE;
		lf.lfItalic = FALSE;
		lf.lfUnderline = TRUE;
		lf.lfStrikeOut = FALSE;
		lf.lfCharSet = ANSI_CHARSET;
		lf.lfOutPrecision = OUT_DEFAULT_PRECIS;
		lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
		lf.lfQuality = DEFAULT_QUALITY;
		lf.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
		memcpy(lf.lfFaceName, "System", 7);

		// Underlined Version
		m_font_underline = CreateFontIndirect(&lf);
		
		// Normal Version
		lf.lfUnderline = FALSE;
		m_font_normal = CreateFontIndirect(&lf);

		// Fixed Font
		memcpy(lf.lfFaceName, "Fixedsys", 8);
		m_font_fixed = CreateFontIndirect(&lf);

	}

	// Init Infobox
	infobox::init();

	// Set Log Hook
	((coreLogger*)g_globalLogger)->registerHook(coreLogger::LOGLEVEL_ALL, this, win_ui::logsys_onLogLine);

	ShowWindow(m_hWnd, SW_SHOWNORMAL);
	UpdateWindow(m_hWnd);

}//end: constructor


win_ui::~win_ui(){

	// Unregister Log Hook
	((coreLogger*)g_globalLogger)->unregisterHook(coreLogger::LOGLEVEL_ALL, this, win_ui::logsys_onLogLine);

	// Final Infobox
	infobox::final();

	// Free up allocated ressources
	if(m_blackpen != NULL)
		DeleteObject(m_blackpen);

	if(m_font_normal != NULL)
		DeleteObject(m_font_normal);

	if(m_font_underline != NULL)
		DeleteObject(m_font_underline);


	// Force Destroy Window
	if(m_hWnd != NULL)
		DestroyWindow(m_hWnd);

	// Unregister window Class
	UnregisterClass(WNDCLASSNAME, m_hInst);

	
	// Clear History free's up all lines in memory.
	this->logsys_clearHistory();

	// Delete the mutex for log printing
	delete m_logLine_lock;


}//end: destructor


void win_ui::requestDestroy() {

	if(this->m_hWnd) {
		PostMessage(this->m_hWnd, WM_QUIT, 0, 0);
		DestroyWindow(this->m_hWnd);
	}

}//end: requestDestroy()


void win_ui::idleLoop(){
	MSG msg;
	
	// Enter Window Main Loop:
	while(GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	putLog("win_ui::idleLoop() -> exit.\n");

}//end: idleLoop()

void win_ui::TextOutf(HDC dc, bool underlined, int x, int y, const char *msg, ...) {
	va_list ap;
	char buf[2048];
	int len;
	HGDIOBJ prevobj;

	va_start(ap, msg);
	len = vsnprintf(buf, sizeof(buf), msg, ap);
	va_end(ap);

	if(underlined == true)
		prevobj = SelectObject(dc, m_font_underline);
	else
		prevobj = SelectObject(dc, m_font_normal);

	TextOut(dc, x, y, buf, len);

	if(underlined == true)
		SelectObject(dc, prevobj);

}

void win_ui::onWMCreate() {

	//
	// The Input Edit Control
	//
	m_hInputEdit = CreateWindowEx(0,
								  "edit",
								  "",
								  WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_LEFT,
								  14, WND_HEIGHT-10-22-44, WND_WIDTH-6-18, 22,
								  m_hWnd,
								  (HMENU)IDC_INPUT_EDIT_CONTROL,
								  m_hInst,
								  NULL);

	//
	SetWindowLongPtr(m_hInputEdit, GWLP_USERDATA, (LONG_PTR) this);

	// Replace the Window proc
	m_OrigInputEditProc = (WNDPROC)GetWindowLongPtr(m_hInputEdit, GWLP_WNDPROC);
	SetWindowLongPtr(m_hInputEdit, GWLP_WNDPROC, (LONG_PTR) win_ui::inputWndProc);
	
	//
	//this->disableInput();


	//
	// Menu
	//
	m_mainMenu = CreateMenu();

	m_subMenu = CreatePopupMenu();
	AppendMenu(m_subMenu, MF_STRING, CID_FILE_SHUTDOWN, "Shutdown");
	AppendMenu(m_mainMenu, MF_STRING | MF_POPUP, (UINT_PTR)m_subMenu, "&File");


	m_subMenu = CreatePopupMenu();
	AppendMenu(m_subMenu, MF_STRING, CID_WINDOW_AUTOREFRESH, "Periodic Refresh");
	AppendMenu(m_subMenu, MF_STRING, CID_WINDOW_CLEAR, "Clear Window");
	AppendMenu(m_mainMenu, MF_STRING | MF_POPUP, (UINT_PTR)m_subMenu, "&Window");

	/*

	SubMenu = CreatePopupMenu();
	AppendMenu(SubMenu, MF_STRING, CID_DEBUG_MEMPOOL_STATS, "MEMPOOL: Show All Stats");
	AppendMenu(MainMenu,  MF_STRING|MF_POPUP, (UINT_PTR)SubMenu, "&Debug");


	SubMenu = CreatePopupMenu();
	AppendMenu(SubMenu, MF_STRING, CID_LOG_RESET_ERRORCNT, "Reset ErrorCount");
	AppendMenu(MainMenu, MF_STRING | MF_POPUP, (UINT_PTR)SubMenu, "&Log");

	AppendMenu(MainMenu, MF_STRING, NULL, "&Manage");

	//SubMenu = CreatePopupMenu();
	//AppendMenu(SubMenu, MF_STRING, 17600, "Spawn Additional Worker");
	//AppendMenu(MainMenu, MF_STRING|MF_POPUP, (UINT_PTR)SubMenu, "&Worker");

	SubMenu = CreatePopupMenu();
	AppendMenu(SubMenu, MF_STRING, CID_MSGFILTER_BASE+LOGTYPE_DEBUG, "Show Debug");
	AppendMenu(SubMenu, MF_STRING, CID_MSGFILTER_BASE+LOGTYPE_MSG, "Show Message");
	AppendMenu(SubMenu, MF_STRING, CID_MSGFILTER_BASE+LOGTYPE_WARNING, "Show Warning");
	AppendMenu(SubMenu, MF_STRING, CID_MSGFILTER_BASE+LOGTYPE_ERROR, "Show Error");
	AppendMenu(SubMenu, MF_STRING, CID_MSG_ENABLE_LINEINFO, "Always show Lineinfo");
	AppendMenu(MainMenu, MF_STRING | MF_POPUP, (UINT_PTR)SubMenu, "Message&filter");


	if(logGuiLineInfoEnabled() == true)
	CheckMenuItem(MainMenu, CID_MSG_ENABLE_LINEINFO, MF_BYCOMMAND | MF_CHECKED);


	SubMenu = CreatePopupMenu();
	AppendMenu(SubMenu, MF_STRING, CID_VERSION, "Version");
	AppendMenu(SubMenu, MF_STRING, CID_COPYRIGHT, "Copyright");
	AppendMenu(MainMenu, MF_STRING | MF_POPUP, (UINT_PTR)SubMenu, "&About");
	*/

	SetMenu(m_hWnd, m_mainMenu);

	// Timer will be always added.
	SetTimer(m_hWnd, CID_TIMER_AUTOREFRESH, 1000, NULL);

	// Autorefresh will be on by Default
	m_bIsAutorefresh = true;
	CheckMenuItem(m_mainMenu, CID_WINDOW_AUTOREFRESH, MF_BYCOMMAND | MF_CHECKED);


}//end: onWmCreate()


void win_ui::disableInput(){
	Edit_Enable(m_hInputEdit, false);
}//end: disableInput()


void win_ui::enableInput(){
	Edit_Enable(m_hInputEdit, true);
}//end: enableInput()


LRESULT CALLBACK win_ui::inputWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	auto *_this = (win_ui*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

	switch(uMsg){
		case WM_CHAR:
			if(wParam == '\r' || wParam == '\n'){
				char buf[256];
				GetWindowText(_this->m_hInputEdit, buf, sizeof(buf));
				
				putLog("Got: '%s'", buf);
				
				
				Edit_SetText(_this->m_hInputEdit, "");
				return 0;
			}
		break;
	}

	return _this->m_OrigInputEditProc(hWnd, uMsg, wParam, lParam);
}//end: inputWndProc()


LRESULT CALLBACK win_ui::wndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	auto *_this = (win_ui*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

	switch(uMsg) {
		case WM_CREATE: {
			_this = (win_ui*)((CREATESTRUCT*)lParam)->lpCreateParams;
			_this->m_hWnd = hWnd;
			SetWindowLongPtr(_this->m_hWnd, GWLP_USERDATA, (LONG_PTR)_this);
			_this->onWMCreate();
		} break;

		///
		case WM_PAINT:
			_this->onWMPaint();
		break;

		/// Rightclick refreshs content.
		case WM_RBUTTONDOWN:
			InvalidateRect((HWND)_this->m_hWnd, NULL, TRUE);
		break;

		///
		case WM_TIMER: {
			_this->onTimerAutorefresh();			
		} break;

		///
		case WM_NOTIFY: {
			switch(((LPNMHDR)lParam)->code){
				case EN_CHANGE:
				case EN_UPDATE:
				return 0;
				break;
			}
		} break;

		///
		case WM_CLOSE:
		_lCHECKCLOSE:
			putLog("CLOSEWINDOW VIA WM_CLOSE/MARK\n");

					//

#ifdef _DEBUG
					goto _lFORCEQUIT;
#endif

					if(MessageBox(hWnd, "Close Ok?", "Attention", MB_ICONWARNING | MB_OKCANCEL) == IDOK) {
						_lFORCEQUIT:
						core->request_shutdown();	// Sets the termination flag in athena thread 



						return 0;
					}
					break;



					///
		case WM_COMMAND:
			switch(LOWORD(wParam)) {
				case CID_FILE_SHUTDOWN:			goto  _lCHECKCLOSE; break;

				case CID_WINDOW_AUTOREFRESH:	_this->onCMDWindowAutoRefresh(); break;
				case CID_WINDOW_CLEAR:			_this->onCMDWindowClear(); break;
			}
		break;


		/// 
		case WM_DESTROY:
			putLog("WM_DESTROY!\n");
			PostQuitMessage(0);
			return 0;
		break;

		/// unhandled msg:
		default:
			return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	return 0;
}//end: wndProc()


//////
void win_ui::onWMPaint(){
	PAINTSTRUCT ps;
	HDC hdc, bufHDC;
	HBITMAP bufBMP, oldBMP;
	RECT rc;

	hdc = BeginPaint(m_hWnd, &ps);

	GetClientRect(m_hWnd, &rc);

	bufHDC = CreateCompatibleDC(ps.hdc);
	bufBMP = CreateCompatibleBitmap(ps.hdc, rc.right-rc.left, rc.bottom-rc.top);
	oldBMP = (HBITMAP)SelectObject(bufHDC, bufBMP);

	FillRect(bufHDC, &rc, WHITE_BRUSH);
	//SetBkMode(bufHDC, TRANSPARENT);
	//SetTextColor(bufHDC, GetSysColor(COLOR_WINDOWTEXT));
	///
	
	//
	this->logsys_drawLog(bufHDC);
	
	// Termination line Over Console Input Box
	{
		HPEN hPenOld = (HPEN)SelectObject(bufHDC, m_blackpen);
		MoveToEx(bufHDC, 0, WND_HEIGHT-80, NULL);
		LineTo(bufHDC, WND_WIDTH, WND_HEIGHT-80);
		SelectObject(bufHDC, hPenOld);

		TextOutf(bufHDC, false, 3, WND_HEIGHT-75, ">"); // 
	}

	
	// Draw Infobox
	infobox::draw(this, bufHDC);
	


	// Copy Buffer to real HDC
	BitBlt(ps.hdc,
		   rc.left, rc.top,
		   rc.right-rc.left, rc.bottom-rc.top,
		   bufHDC,
		   0, 0,
		   SRCCOPY);

	EndPaint(m_hWnd, &ps);
	
	SelectObject(bufHDC, oldBMP);
	DeleteObject(bufBMP);
	DeleteDC(bufHDC);

}//end: onWMPaint()


void win_ui::onTimerAutorefresh(){

	if(this->m_bIsAutorefresh == true)
		InvalidateRect(m_hWnd, NULL, TRUE);

}//end: onTimerAutorefresh()


void win_ui::onCMDWindowClear(){
	// clear log buffer!

	this->logsys_clearHistory();

}//end: onCMDWindowClear()


void win_ui::onCMDWindowAutoRefresh(){

	if(m_bIsAutorefresh == true) {
		putDbg("onCmdWindowAutoRefresh: True->False\n");
		m_bIsAutorefresh = false;
		CheckMenuItem(m_mainMenu, CID_WINDOW_AUTOREFRESH, MF_BYCOMMAND | MF_UNCHECKED);
	} else {
		putDbg("onCmdWindowAutoRefresh: False->True\n");
		m_bIsAutorefresh = true;
		CheckMenuItem(m_mainMenu, CID_WINDOW_AUTOREFRESH, MF_BYCOMMAND | MF_CHECKED);
	}

}//end: onCMDAutoRefresh()



//////
void win_ui::logsys_drawLog(HDC hdc){
	// Called in WM_PAINT
	HGDIOBJ hPrevFont = SelectFont(hdc, this->m_font_fixed);
	int c = 0, xoffset;


	this->m_logLine_lock->lock();

	for(auto it = this->m_logLineFirst; it != NULL; it = it->next){
		
		switch(it->level){
			case coreLogger::LOGLEVEL_DEBUG:
				xoffset = 2;
				SetTextColor(hdc, RGB(0, 0, 255));
			break;

			case coreLogger::LOGLEVEL_WARNING:
				xoffset = 2;
				SetTextColor(hdc, RGB(0xff, 0x66, 0x00));
			break;

			case coreLogger::LOGLEVEL_ERROR:
			case coreLogger::LOGLEVEL_FATALERROR:
				xoffset = 61;
				SetTextColor(hdc, RGB(0xff, 0x00, 0x00));
				TextOut(hdc, 2, c*16, "[Error]", 7);
			break;
	
			default:
				xoffset = 2;
				SetTextColor(hdc, RGB(0, 0, 0));
			break;

		}


		// print timestamp
		{
			char timebuf[64];
			time_t tc = (time_t)it->tick;
			struct tm *t = localtime(&tc);

			size_t len = sprintf(timebuf, "(%02u:%02u:%02u)", t->tm_hour, t->tm_min, t->tm_sec);
			TextOut(hdc, 2, c*16, timebuf, (int)len);
			xoffset+= 84;
		}

		TextOut(hdc, xoffset, c*16, it->line, (int) (it->szLine) );


		c++;
	}
	
	this->m_logLine_lock->unlock();


	// Restore old font
	SelectFont(hdc, hPrevFont);

}//end: logsys_drawLog()


void win_ui::logsys_onLogLineInsertLine(struct Logging::coreLogger::qElem *line){
	struct coreLogger::qElem *tmp;

	line->next = NULL;

	if(this->m_logLine_num == WND_MAX_LOGLINES) {
		// At limit.
		// drop first,  append to last.

		// drop first (make new first, old firsts->next)
		tmp = this->m_logLineFirst;
		this->m_logLineFirst = tmp->next;
		rofree(tmp);

		// Insert new @ end
		this->m_logLineLast->next = line;
		this->m_logLineLast = line;

	} else {
		// Just Insert

		if(this->m_logLineFirst == NULL) {
			// first inserted line:
			this->m_logLineFirst	= line;
			this->m_logLineLast	= line;

		} else {
			// insert @end
			this->m_logLineLast->next = line;
			this->m_logLineLast = line;

		}

		this->m_logLine_num++;

	}

}//end: logsys_onLogLineInsertLine()



void win_ui::logsys_onLogLine(void *paramPtrUIInst, const struct Logging::coreLogger::qElem *inLine){
	auto _this = (win_ui*)paramPtrUIInst;
	struct coreLogger::qElem *line;

	
	if(inLine->szLine <=  WND_MAX_LOGLINELENGTH){
		line = coreLogger::copyQElem(inLine, true);

		// Replace tick with normal system time 
		line->tick = (rgCore::Time::tick_t) time(NULL);

		_this->m_logLine_lock->lock();
		_this->logsys_onLogLineInsertLine(line);
		_this->m_logLine_lock->unlock();

	}else{
		// Incomming log line is > than max supported per line - split -
		size_t remain = inLine->szLine;
		size_t pos = 0;

		_this->m_logLine_lock->lock();
		while(remain != 0){
			size_t tocopy = remain;
			if(tocopy > WND_MAX_LOGLINELENGTH)
				tocopy = WND_MAX_LOGLINELENGTH;

			line = coreLogger::copyQElem(inLine, true, pos, tocopy);
			pos += tocopy;
			remain -= tocopy;

			// Replace tick with normal system time 
			line->tick = (rgCore::Time::tick_t) time(NULL);


			_this->logsys_onLogLineInsertLine(line);

		}
		_this->m_logLine_lock->unlock();
	}


	




	// Invalidate main wnd in order to get wm_paint called
	InvalidateRect(_this->m_hWnd, NULL, TRUE);

}//end: logsys_onLogLine()


void win_ui::logsys_clearHistory(){

	this->m_logLine_lock->lock();
	
	struct coreLogger::qElem	*tmp = this->m_logLineFirst, 
								*tmpn;
	while(1){
		if(tmp == NULL)
			break;

		tmpn = tmp->next;

		rofree(tmp);

		tmp = tmpn;
	}

	this->m_logLineFirst	= NULL;
	this->m_logLineLast		= NULL;
	this->m_logLine_num = 0;

	this->m_logLine_lock->unlock();

	// Invalidate main wnd in order to get wm_paint called
	InvalidateRect(this->m_hWnd, NULL, TRUE);

}//end: logsys_clearHistory()



} }