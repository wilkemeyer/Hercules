#pragma once

#pragma once

// Including SDKDDKVer.h defines the highest available Windows platform.

// If you wish to build your application for a previous Windows platform, include WinSDKVer.h and
// set the _WIN32_WINNT macro to the platform you wish to support before including SDKDDKVer.h.

#include <winsdkver.h>
#define _WIN32_WINNT NTDDI_WINXP
#define PSAPI_VERSION 1
#include <SDKDDKVer.h>

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NONSTDC_NO_WARNINGS


#include <Windows.h>
#include <WinSock2.h>
#include <Psapi.h>
#include <intrin.h>	// Interlocked
#include <string.h>	// box
#include <stdio.h>	// vsnprintf
#include <stdarg.h>	// vsnprintf
#include <stdlib.h>	// EXIT_FAILURE
#include <time.h>