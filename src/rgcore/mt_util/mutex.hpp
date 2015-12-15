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


namespace rgCore {
namespace util {

	class __declspec(align(8)) mutex {
	public:
		__forceinline mutex() {
			InitializeCriticalSection(&m_cs);
		}//end: mutex()


		__forceinline ~mutex() {
			DeleteCriticalSection(&m_cs);
		}//end: ~mutex()


		__forceinline void lock() {
			EnterCriticalSection(&m_cs);
		}//end: lock()

		__forceinline void unlock() {
			LeaveCriticalSection(&m_cs);
		}//end: unlock()

		__forceinline bool tryLock() {
			if(TryEnterCriticalSection(&m_cs) == TRUE) {
				return true;
			} else {
				return false;
			}
		}//end: tryLocK()

	private:
		CRITICAL_SECTION m_cs;

	};

}
}