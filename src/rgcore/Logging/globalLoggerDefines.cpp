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

#include "stdafx.h"
#include "../stdafx.h"

namespace rgCore { namespace Logging {
	
ILogger *g_globalLogger = NULL;



/*
// @TODO: 

#include "../../3rdparty/libconfig/libconfig.h"
void showmsg_showConfigWarning(config_setting_t *config, const char *string, ...) {
	StringBuf buf;
	va_list ap;
	StrBuf->Init(&buf);
	StrBuf->AppendStr(&buf, string);
	StrBuf->Printf(&buf, " (%s:%d)\n", config_setting_source_file(config), config_setting_source_line(config));
	va_start(ap, string);
	vShowMessage_(MSG_WARNING, StrBuf->Value(&buf), ap);
	va_end(ap);
	StrBuf->Destroy(&buf);
}*/


} }