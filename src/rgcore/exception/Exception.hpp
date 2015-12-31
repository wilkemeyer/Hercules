#pragma once

namespace rgCore {
	namespace util {

		class Exception {
		public:

			__forceinline Exception() {
				*m_func = '\0';
				*m_file = '\0';
				*m_message = '\0';
				m_code = 0;
			}


			__forceinline Exception(const char *format, ...) {
				*m_func = '\0';
				*m_file = '\0';
				
				va_list ap;

				va_start(ap, format);
				size_t szMsg = vsnprintf_s(m_message, sizeof(m_message), format, ap);
				va_end(ap);
				m_message[sizeof(m_message)-1] = '\0';
			}//end: Exception()

			__forceinline Exception(int code, const char *format, ...) {
				*m_func = '\0';
				*m_file = '\0';
				m_code = code;

				va_list ap;

				va_start(ap, format);
				size_t szMsg = vsnprintf_s(m_message, sizeof(m_message), format, ap);
				va_end(ap);
				m_message[sizeof(m_message)-1] = '\0';
			}//end: Exception()

			__forceinline Exception(const char *format, va_list ap) {
				*m_func = '\0';
				*m_file = '\0';


				size_t szMsg = vsnprintf_s(m_message, sizeof(m_message), format, ap);

				m_message[sizeof(m_message)-1] = '\0';
			}//end: Exception()


			__forceinline Exception(const char *file, int line, const char *func, const char *format, ...) {

				va_list ap;

				va_start(ap, format);
				size_t szMsg = vsnprintf_s(m_message, sizeof(m_message), format, ap);
				va_end(ap);
				m_message[sizeof(m_message)-1] = '\0';

				strncpy(m_file, file, sizeof(m_file));
				strncpy(m_func, func, sizeof(m_func));

				m_file[sizeof(m_file)-1] = '\0';
				m_func[sizeof(m_func)-1] = '\0';

				m_line = line;

			}//end Exception() - with debug info



			__forceinline Exception(const char *file, int line, const char *func, const char *format, va_list ap) {


				size_t szMsg = vsnprintf_s(m_message, sizeof(m_message), format, ap);

				m_message[sizeof(m_message)-1] = '\0';

				strncpy(m_file, file, sizeof(m_file));
				strncpy(m_func, func, sizeof(m_func));

				m_file[sizeof(m_file)-1] = '\0';
				m_func[sizeof(m_func)-1] = '\0';

				m_line = line;

			}//end Exception() - with debug info

			

			
			__forceinline const char *getFile() {
				return m_file;
			}

			__forceinline const char *getFunc() {
				return m_func;
			}

			__forceinline int getLine() {
				return m_line;
			}

			__forceinline const char *getMessage() {
				return m_message;
			}//end: getMessage()
			
			__forceinline int getCode(){
				return m_code;
			}


		protected:

			__forceinline void setMessage(const char *format, va_list ap) {
				*m_func = '\0';
				*m_file = '\0';

				size_t szMsg = vsnprintf_s(m_message, sizeof(m_message), format, ap);


				m_message[sizeof(m_message)-1] = '\0';
			}//end: setMessage(0

			__forceinline void setMessage(const char *format, ...) {
				*m_func = '\0';
				*m_file = '\0';

				va_list ap;

				va_start(ap, format);
				size_t szMsg = vsnprintf_s(m_message, sizeof(m_message), format, ap);
				va_end(ap);
				m_message[sizeof(m_message)-1] = '\0';
			}//end: setMessage(0


			__forceinline void setDebugInfo(const char *file, int line, const char *func) {

				strncpy(m_file, file, sizeof(m_file));
				strncpy(m_func, func, sizeof(m_func));

				m_file[sizeof(m_file)-1] = '\0';
				m_func[sizeof(m_func)-1] = '\0';

				m_line = line;

			}//end: setDebugInfo()
			

		private:
			char m_message[16384];
			char m_file[128];
			char m_func[128];
			int m_line;
			int m_code;


		};//end: class Exception

	}
}