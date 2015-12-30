#pragma once

namespace rgCore { namespace util {

class spinlock {
public:
	//
	// Simple spinlock implementation
	//   NO SUPPORT FOR REENTRANCE! 
	// - no forced context switching -
	//
	//

	__forceinline spinlock(){

		atomic::Exchange(&m_lock, 0);

	}//end: constructor

	__forceinline ~spinlock(){
#ifdef _DEBUG
		if(atomic::CompareAndSwap(&m_lock, 0, 0) != 0){
			ShowDebug("~spinlock() -> lock is not fully released\n");
			__debugbreak();
		}
#endif
		//
	}//end: destructor


	__forceinline void lock(){

		while(1){
			if(atomic::CompareAndSwap(&m_lock, 1, 0) == 0){
				break; // got the lock
			}
		}

	}//end: enter()
	

	__forceinline bool tryLock(){

		if(atomic::CompareAndSwap(&m_lock, 1, 0) == 0) {
			return true;
		}

		return false;
	}//end: tryEnter()


	__forceinline void unlock(){
		
		atomic::Exchange(&m_lock, 0);

	}//end: leave()



private:
	atomic::atomic_t m_lock;

};

} }
