#pragma once

//
// Archetecture Specific Defines
//

#if defined(_M_X64) || defined(_M_AMD64) 
#define ARCH_AMD64 1
#define ARCH_MEMALIGN 16

#elif defined(_M_IX86)
#define ARCH_X86 1
#define ARCH_MEMALIGN 8

#endif
