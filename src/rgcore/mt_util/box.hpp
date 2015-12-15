#pragma once

namespace rgCore {
	namespace util {
	
		static __forceinline void ErrorBox(const char *format, ...) {
			char msg[8192];
			va_list ap;

			va_start(ap, format);
				
			vsnprintf(msg, sizeof(msg), format, ap);

			va_end(ap);

			msg[sizeof(msg)-1] = '\0';


			MessageBox(NULL, msg, "Error", MB_ICONERROR | MB_OK);

		}//end: ErrorBox()

	}
}