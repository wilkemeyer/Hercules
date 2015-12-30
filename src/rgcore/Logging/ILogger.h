#pragma once

//
// Logger Interface
//
namespace rgCore { namespace Logging {

class ILogger {
public:

		enum LOGLEVEL {	// Adding new types may require changes to various implementations (logType2Str@logger)
			LOGLEVEL_NONE		= 0,
			// RG/blockz Types:
			LOGLEVEL_INFO		= (1 << 0),
			LOGLEVEL_WARNING	= (1 << 1),
			LOGLEVEL_ERROR		= (1 << 2),
			LOGLEVEL_MEMMGR		= (1 << 3),
			LOGLEVEL_DEBUG		= (1 << 4),

			// Athena Compat Types:
			LOGLEVEL_NOTICE		= (1 << 5),
			LOGLEVEL_SQL		= (1 << 6),
			LOGLEVEL_STATUS		= (1 << 7),
			LOGLEVEL_FATALERROR	= (1 << 8),

			LOGLEVEL_ALL		= 0xffff
		};

		virtual void write(enum LOGLEVEL level, const char *msg, ...) = 0;
		virtual void writeEx(enum LOGLEVEL level, const char *file, int line, const char *func, const char *msg, ...) = 0;
};


} }
