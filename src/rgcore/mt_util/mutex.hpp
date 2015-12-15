#pragma once

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