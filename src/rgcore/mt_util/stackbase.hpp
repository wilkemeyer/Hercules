#pragma once
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


// Something Evil & Ugly ..
//


namespace rgCore { namespace util {

__forceinline void *getStackBase() {
#if defined(ARCH_X86)
	NT_TIB* pTib;
	__asm {
		MOV EAX, FS:[18h]
		MOV pTib, EAX
	}

	return pTib->StackBase;


#elif defined (ARCH_AMD64)
#ifdef NO_VISTA
	PNT_TIB64 pTib = reinterpret_cast<PNT_TIB64>(NtCurrentTeb());
	return (void*) pTib->StackBase;
#else

	// Vista compatible version. (should also work on NT5.3 AMD64)

	PNT_TIB64 pTib = (PNT_TIB64)__readgsqword(0x30); 
	return (void*)pTib->StackBase;

#endif // NO_VISTA?

#else
#error @OTOD - UNKNOWN PATFORM
#endif



	// Webkit Code (StackBounds.cpp)
	/*
	#if CPU(X86) && COMPILER(MSVC)
		// offset 0x18 from the FS segment register gives a pointer to
		// the thread information block for the current thread
		NT_TIB* pTib;
		__asm {
			MOV EAX, FS:[18h]
			MOV pTib, EAX
		}
		m_origin = static_cast<void*>(pTib->StackBase);
	#elif CPU(X86) && COMPILER(GCC)
		// offset 0x18 from the FS segment register gives a pointer to
		// the thread information block for the current thread
		NT_TIB* pTib;
		asm ( "movl %%fs:0x18, %0\n"
				: "=r" (pTib)
			);
		m_origin = static_cast<void*>(pTib->StackBase);
	#elif CPU(X86_64)
		PNT_TIB64 pTib = reinterpret_cast<PNT_TIB64>(NtCurrentTeb());
		m_origin = reinterpret_cast<void*>(pTib->StackBase);
	#else
	#error Need a way to get the stack bounds on this platform (Windows)
	#endif
		// Looks like we should be able to get pTib->StackLimit
		m_bound = estimateStackBound(m_origin);
	*/
}//end: getStackBase()

} }