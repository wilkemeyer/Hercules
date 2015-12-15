#pragma once

namespace rgCore {
	namespace util {

		class cond {
		public:
			enum SIGNAL_TYPE {
				SIGNAL_SINGLE = 0,
				SIGNAL_BROADCAST = 1
			};



			__forceinline cond() {

				// initialize everything properly.
				m_waiters = 0;
				m_events[SIGNAL_SINGLE]		= CreateEvent(	NULL, FALSE,
															FALSE, NULL );
				m_events[SIGNAL_BROADCAST]	= CreateEvent(	NULL, TRUE,
															FALSE, NULL);

			}//end: cond()



			__forceinline ~cond() {
				CloseHandle(m_events[SIGNAL_SINGLE]);
				CloseHandle(m_events[SIGNAL_BROADCAST]);
			}//end: ~cond()



			/** 
			 * Waits for signalled state
			 * 
			 * @param *m  
			 * @param timeout_ms	timeout in milliseconds or INIFINITE (winapi)
			 *
			 * @return true if timed out
			 */
			__forceinline bool wait(mutex *m, DWORD timeout_ms) {
				DWORD result;
				int last_waiter;

				// @OPTIMIZE: use Interlocked Operation.
				m_waiters_lock.lock();
				m_waiters++;
				m_waiters_lock.unlock();

				m->unlock();


				result = WaitForMultipleObjects(2, m_events, FALSE, timeout_ms);

				m_waiters_lock.lock();
				m_waiters--;
				last_waiter = (result == WAIT_OBJECT_0 + SIGNAL_BROADCAST) && (m_waiters == 0);
				m_waiters_lock.unlock();

				if(last_waiter) {
					ResetEvent(m_events[SIGNAL_BROADCAST]);
				}


				m->lock();

				return (result == WAIT_TIMEOUT)?true:false;

			}//end: wait()



			/**
			 * Notify one waiting thread
			 */
			__forceinline void signal() {
				int have_waiters;

				m_waiters_lock.lock();
				have_waiters = m_waiters > 0;
				m_waiters_lock.unlock();

				// @HINT may remove this condition as it 'could' leave into race condition 
				// 
				if(have_waiters) {
					SetEvent(m_events[SIGNAL_SINGLE]);
				}

			}//end: signal()



			/**
			* Notify all waiting threads
			*/
			__forceinline void broadcast() {
				int have_waiters;

				m_waiters_lock.lock();
				have_waiters = m_waiters > 0;
				m_waiters_lock.unlock();

				// @HINT may remove this condition as it 'could' leave into race condition 
				// 
				if(have_waiters) {
					SetEvent(m_events[SIGNAL_BROADCAST]);
				}

			}//end: signal()


		private:
			HANDLE m_events[2];
			size_t m_waiters;
			mutex m_waiters_lock;
		};

	}
}