#pragma once


namespace rgCore { namespace gui {

class win_ui {
public:
	win_ui();
	~win_ui();

	void idleLoop();
	void requestDestroy();

private:
	HWND m_hWnd;
	HINSTANCE m_hInst;
	HMENU m_mainMenu, m_subMenu;
	bool m_bIsAutorefresh;


private:
	friend class infobox;
	// Text Utility Functions
	void TextOutf(HDC dc, bool underlined, int x, int y, const char *msg, ...);
	HPEN m_blackpen;
	HFONT m_font_underline;
	HFONT m_font_normal;
	HFONT m_font_fixed;


private: 
	HWND m_hInputEdit;
	WNDPROC m_OrigInputEditProc;
	static LRESULT CALLBACK inputWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	
	void disableInput();
	void enableInput();

private:
	static LRESULT CALLBACK wndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	
	void onWMCreate();
	void onWMPaint();


private: 
	// 
	// Own Window Messages 
	//
	enum {
		// Window Timers
		CID_TIMER_AUTOREFRESH = 17000,

		// Commands, usually handled by command handlers with same name as constant
		CID_FILE_SHUTDOWN,
		CID_WINDOW_AUTOREFRESH,
		CID_WINDOW_CLEAR
	};
	//
	// Other Window Elements
	//
	enum {
		IDC_INPUT_EDIT_CONTROL = 15000,
	};

	void onTimerAutorefresh();

	void onCMDWindowAutoRefresh();
	void onCMDWindowClear();

	

private:
	//
	// On Screen Log
	//
	struct Logging::coreLogger::qElem *m_logLineFirst, *m_logLineLast;
	util::spinlock *m_logLine_lock;
	size_t m_logLine_num;

	/* Callback for logging hook */
	static void logsys_onLogLine(void *paramPtrUIInst, const struct Logging::coreLogger::qElem *inLine);
	void logsys_onLogLineInsertLine(struct Logging::coreLogger::qElem *line);

	void logsys_drawLog(HDC hdc); // onDraw()
	void logsys_clearHistory(); // 


};


} }