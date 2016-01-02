#pragma once


//
// Generic 'Global' Core Logger 
// Added for easier athena -> rgCore migration
// (Basicly replaces the old showmsg system)
//
 
namespace rgCore { namespace Logging {

/**
 * Global coreLogger Instance
 * It could be used even in initialization routines of rgCore
 * except the logging-system itself (which should be clear..)
 *
 * Its initialized in rgCore_init/rgCore_final..
 *
 * Its also Safe to cast this to Logging::coreLogger
 */
extern ILogger *g_globalLogger;


} }


//
// ROorg 
//

#ifdef _DEBUG
#define putLog(format, ...)		rgCore::Logging::g_globalLogger->writeEx(rgCore::Logging::ILogger::LOGLEVEL_INFO,		__FILE__, __LINE__, __FUNCTION__, format, __VA_ARGS__)
#define putWrn(format, ...)		rgCore::Logging::g_globalLogger->writeEx(rgCore::Logging::ILogger::LOGLEVEL_WARNING,	__FILE__, __LINE__, __FUNCTION__, format, __VA_ARGS__)
#define putErr(format, ...)		rgCore::Logging::g_globalLogger->writeEx(rgCore::Logging::ILogger::LOGLEVEL_ERROR,		__FILE__, __LINE__, __FUNCTION__, format, __VA_ARGS__)
#define putMemMgr(format, ...)	rgCore::Logging::g_globalLogger->writeEx(rgCore::Logging::ILogger::LOGLEVEL_MEMMGR,		__FILE__, __LINE__, __FUNCTION__, format, __VA_ARGS__)
#define putDbg(format, ...)		rgCore::Logging::g_globalLogger->writeEx(rgCore::Logging::ILogger::LOGLEVEL_DEBUG,		__FILE__, __LINE__, __FUNCTION__, format, __VA_ARGS__)
#else
#define putLog(format, ...)		rgCore::Logging::g_globalLogger->write(rgCore::Logging::ILogger::LOGLEVEL_INFO,		format, __VA_ARGS__)
#define putWrn(format, ...)		rgCore::Logging::g_globalLogger->write(rgCore::Logging::ILogger::LOGLEVEL_WARNING,	format, __VA_ARGS__)
#define putErr(format, ...)		rgCore::Logging::g_globalLogger->write(rgCore::Logging::ILogger::LOGLEVEL_ERROR,	format, __VA_ARGS__)
#define putMemMgr(format, ...)	rgCore::Logging::g_globalLogger->write(rgCore::Logging::ILogger::LOGLEVEL_MEMMGR,	format, __VA_ARGS__)
#define putDbg(format, ...)	
#endif



//
// Athena showmsg (LEGACY) Compatibility:
// 

#define ClearScreen()
#define vShowMessage(fmt, list)	// @TODO -> list = ap
#define ShowMessage(fmt, ...) (rgCore::Logging::g_globalLogger->writeEx(rgCore::Logging::ILogger::LOGLEVEL_INFO,	__FILE__,	__LINE__,	__FUNCTION__, fmt,  ##__VA_ARGS__))
#define ShowStatus(fmt, ...) (rgCore::Logging::g_globalLogger->writeEx(rgCore::Logging::ILogger::LOGLEVEL_STATUS,	__FILE__,	__LINE__,	__FUNCTION__, fmt,  ##__VA_ARGS__))
#define ShowSQL(fmt, ...) (rgCore::Logging::g_globalLogger->writeEx(rgCore::Logging::ILogger::LOGLEVEL_SQL,			__FILE__,	__LINE__,	__FUNCTION__, fmt,  ##__VA_ARGS__))
#define ShowInfo(fmt, ...) (rgCore::Logging::g_globalLogger->writeEx(rgCore::Logging::ILogger::LOGLEVEL_INFO,		__FILE__,	__LINE__,	__FUNCTION__, fmt,  ##__VA_ARGS__))
#define ShowNotice(fmt, ...) (rgCore::Logging::g_globalLogger->writeEx(rgCore::Logging::ILogger::LOGLEVEL_NOTICE,	__FILE__,	__LINE__,	__FUNCTION__, fmt,  ##__VA_ARGS__))
#define ShowWarning(fmt, ...) (rgCore::Logging::g_globalLogger->writeEx(rgCore::Logging::ILogger::LOGLEVEL_WARNING,	__FILE__,	__LINE__,	__FUNCTION__, fmt,  ##__VA_ARGS__))
#define ShowDebug(fmt, ...) (rgCore::Logging::g_globalLogger->writeEx(rgCore::Logging::ILogger::LOGLEVEL_DEBUG,		__FILE__,	__LINE__,	__FUNCTION__, fmt,  ##__VA_ARGS__))
#define ShowError(fmt, ...) (rgCore::Logging::g_globalLogger->writeEx(rgCore::Logging::ILogger::LOGLEVEL_ERROR,		__FILE__,	__LINE__,	__FUNCTION__, fmt,  ##__VA_ARGS__))
#define ShowFatalError(fmt, ...) (rgCore::Logging::g_globalLogger->writeEx(rgCore::Logging::ILogger::LOGLEVEL_FATALERROR,	__FILE__,	__LINE__,	__FUNCTION__, fmt,  ##__VA_ARGS__))
#define ShowConfigWarning(config, fmt, ...) // TODO


//
// Nullifiy All - old - Escape codes:
//
#define CL_RESET      ""
#define CL_CLS        ""
#define CL_CLL        ""

// font settings
#define CL_BOLD       ""
#define CL_NORM       CL_RESET
#define CL_NORMAL     CL_RESET
#define CL_NONE       CL_RESET

// background color
#define CL_BG_BLACK   ""
#define CL_BG_RED     ""
#define CL_BG_GREEN   ""
#define CL_BG_YELLOW  ""
#define CL_BG_BLUE    ""
#define CL_BG_MAGENTA ""
#define CL_BG_CYAN    ""
#define CL_BG_WHITE   ""
// foreground color and normal font (normal color on windows)
#define CL_LT_BLACK   ""
#define CL_LT_RED     ""
#define CL_LT_GREEN   ""
#define CL_LT_YELLOW  ""
#define CL_LT_BLUE    ""
#define CL_LT_MAGENTA ""
#define CL_LT_CYAN    ""
#define CL_LT_WHITE   ""
// foreground color and bold font (bright color on windows)
#define CL_BT_BLACK   ""
#define CL_BT_RED     ""
#define CL_BT_GREEN   ""
#define CL_BT_YELLOW  ""
#define CL_BT_BLUE    ""
#define CL_BT_MAGENTA ""
#define CL_BT_CYAN    ""
#define CL_BT_WHITE   ""

// foreground color and bold font (bright color on windows)
#define CL_WHITE   CL_BT_WHITE
#define CL_GRAY    CL_BT_BLACK
#define CL_RED     CL_BT_RED
#define CL_GREEN   CL_BT_GREEN
#define CL_YELLOW  CL_BT_YELLOW
#define CL_BLUE    CL_BT_BLUE
#define CL_MAGENTA CL_BT_MAGENTA
#define CL_CYAN    CL_BT_CYAN

//
//
//
#define CL_SPACE   "           "   // space aquivalent of the print messages
