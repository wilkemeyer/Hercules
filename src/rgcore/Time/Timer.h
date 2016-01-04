#pragma once

#include "time.h"

namespace rgCore { namespace Time {


class timer {
public:
	#define TIMER_INVALID_ID -1

	static void init();
	static void final();
	static void getCounters(size_t *numTotal, size_t *numFree, size_t *numPeak);

	/** 
	 * TimerProc type
	 *
	 * @return considered when intervaltimer,  return value of true will 'self-delete' the timer
	 */
	typedef bool (*TimerProc)(int timerID, void *paramPtr, size_t paramInt);

	static int add(tick_t execTick, TimerProc proc, void *paramPtr, size_t paramInt);
	static int addInterval(tick_t execTick, tick_t interval, TimerProc proc, void *paramPtr, size_t paramInt);

	/** 
	 * Deletes the given timer by ID (obtained as return value from timer_add / addInterval)
	 * 
	 * @Note self-deletion within a timerproc of a interval timer
	 * @note is not possible, in this case, use the return value of t
	 * @note he timer proc instead.
	 */
	static void del(int timerID);
};

}} 