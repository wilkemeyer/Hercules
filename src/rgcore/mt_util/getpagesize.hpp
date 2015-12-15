#pragma once

//

__forceinline static int getpagesize(void) {
	SYSTEM_INFO system_info;
	static int pgsz = -1;

	if(pgsz < 0) {
		GetSystemInfo(&system_info);
		pgsz = system_info.dwPageSize;
	}

	return pgsz;
}//end: getpagesize()
