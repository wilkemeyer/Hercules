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


// Shared Read - Exclusive Write Lock/Mutex
// Note: the implementation does not allow reentrance! 
//	     also it wont support upgrading from Read to Write Lock
//		 Both cases above will result in dead-lock.
//


namespace rgCore { namespace util {

class __declspec(align(ARCH_MEMALIGN)) rwlock {
public:
	__forceinline rwlock(){
		InitializeSRWLock(&m_srwlock);
	}

	__forceinline ~rwlock(){
		//
	}


	// Shared:
	__forceinline void lockRead(){
		AcquireSRWLockShared(&m_srwlock);
	}
	
	__forceinline bool tryLockRead(){
		if(TryAcquireSRWLockShared(&m_srwlock) == TRUE){
			return true;
		}
		return false;
	}

	__forceinline void unlockRead(){
		ReleaseSRWLockShared(&m_srwlock);
	}


	// Exclusive:
	__forceinline void lockWrite(){
		AcquireSRWLockExclusive(&m_srwlock);
	}

	__forceinline bool tryLockWrite(){
		if(TryAcquireSRWLockExclusive(&m_srwlock) == TRUE){
			return true;
		}
		return false;
	}

	__forceinline void unlockWrite(){
		ReleaseSRWLockExclusive(&m_srwlock);
	}

private:
	SRWLOCK m_srwlock;

};


} } //end namespaces