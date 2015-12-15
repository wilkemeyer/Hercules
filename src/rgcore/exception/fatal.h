#pragma once

//
// Fatal Error Handling (or .. not handling :/)
//

class FatalErrorException: public rgCore::util::Exception {
public:
	__forceinline FatalErrorException(const char *format, ...) {
		va_list ap;

		va_start(ap, format);
		this->setMessage(format, ap);
		va_end(ap);

	}

	__forceinline FatalErrorException(const char *file, int line, const char *func, const char *format, ...) {
		va_list ap;

		va_start(ap, format);
		this->setMessage(format, ap);
		va_end(ap);

		this->setDebugInfo(file, line, func);
	}
};

#ifdef _DEBUG
#define FatalError(Message, ...) throw new FatalErrorException(__FILE__, __LINE__, __FUNCTION__, Message, __VA_ARGS__)
#define fatalError(Message, ...) throw new FatalErrorException(__FILE__, __LINE__, __FUNCTION__, Message, __VA_ARGS__)
#else
#define FatalError(Message, ...) throw new FatalErrorException(Message, __VA_ARGS__)
#define fatalError(Message, ...) throw new FatalErrorException(Message, __VA_ARGS__)
#endif
