#include "stdafx.h"

#if defined(ARCH_AMD64)

// DBGHelp:
#pragma comment(lib, "..\\..\\3rdparty\\dbghelp_x64\\lib\\dbghelp.lib")

// Networking/Socket
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "Mswsock.lib")

// 
#if defined(_DEBUG)
#pragma comment(lib, "..\\..\\3rdparty\\tbb\\bin\\Win64\\Debug\\tbbmalloc_debug64.lib")
#else
#pragma comment(lib, "..\\..\\3rdparty\\tbb\\bin\\Win64\\Release\\tbbmalloc64.lib")
#endif

#elif defined(ARCH_X86)

#if defined(_DEBUG)
#pragma comment(lib, "..\\..\\3rdparty\\tbb\\bin\\Win32\\Debug\\tbbmalloc_debug.lib")
#else
#pragma comment(lib, "..\\..\\3rdparty\\tbb\\bin\\Win32\\Release\\tbbmalloc.lib")
#endif

#else
#error rgCore: Unsupported ARCH!
#endif
