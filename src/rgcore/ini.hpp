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


/** 
 * Copyright (c) 2010  ROorg / Florian Wilkemeyer <fw@f-ws.de>
 *
 * Contact:
 *  info@roorg.net
 *
 */

__forceinline bool _ini_parseBooleanString(const char *str){
	if(::strcmpi(str, "true") == 0 ||
		::strcmpi(str, "yes") == 0 ||
		::strcmpi(str, "1") == 0)
			return true;
return false;
}



__forceinline void iniGetString(const char *fileName,  const char *sectionName,  const char *varName, 
									const char *defaultValue, 
									char *destBuffer, 
									size_t szDestBuffer){
		
		::GetPrivateProfileStringA(sectionName, varName, defaultValue, destBuffer, (unsigned int)szDestBuffer, fileName);
}

// Tries to Read the value from app config 
// if not found tries to read the value in defaults configuration - otherwise the specified defaultValue will be used
__forceinline void iniGetAppString(const char *section, const char *varName, const char *defaultValue, char *destBuffer, size_t szDestBuffer){
	char fn[256];

	sprintf_s(fn, INI_DEFAULT_APP_CONFIG, rgCore::rgCore_getAppName());

	
	iniGetString(fn, section, varName, "\xfe", destBuffer, szDestBuffer);
	if(destBuffer[0] == '\xfe'){
		iniGetString(INI_DEFAULT_CONFIG, section, varName, defaultValue, destBuffer, szDestBuffer);
	}
	
}//end: iniGetAppString()


__forceinline bool iniGetBoolean(const char *fileName,  const char *sectionName,  const char *varName, 
								bool defaultValue){
		
	char buf[64];
	iniGetString(fileName, sectionName, varName, "", buf, 64);
	if(buf[0] == '\0')
		return defaultValue;

	return _ini_parseBooleanString(buf);
}


// Tries to Read the value from app config 
// if not found tries to read the value in defaults configuration - otherwise the specified defaultValue will be used
__forceinline bool iniGetAppBoolean(const char *section, const char *varName, bool defaultValue) {
	char fn[256];
	char buf[64];

	sprintf_s(fn, INI_DEFAULT_APP_CONFIG, rgCore::rgCore_getAppName());



	iniGetString(fn, section, varName, "", buf, sizeof(buf));
	if(buf[0] == '\0'){
		iniGetString(INI_DEFAULT_CONFIG, section, varName, "", buf, sizeof(buf));
		if(buf[0] == '\0'){
			return defaultValue;
		}else{
			return _ini_parseBooleanString(buf);
		}
	}else{
		return _ini_parseBooleanString(buf);
	}


	// never reached.
	return false;
}//end: iniGetAppBoolean()

/**
	* <summary>	Gets the given section/key from given ini-file, and returns integer
	* 				Supports HEX numbers with 0x prefix or h suffix
	* 				and k/m/g suffixes for Kibi, Mibi, Gibi (1024)
	* 				and K/M/G suffixes for Kilo, Mega, Giga (1000). </summary>
	*
	* <remarks>	Wilkemeyer, 09.07.2011. </remarks>
	*
	* <param name="fileName">	  	Filename of the file. </param>
	* <param name="sectionName"> 	Name of the section. </param>
	* <param name="varName">	  	Name of the variable. </param>
	* <param name="defaultValue">	The default value. </param>
	*
	* <returns>	. </returns>
	*/
__forceinline __int64 iniGetInteger(const char *fileName,  const char *sectionName,  const char *varName, 
									__int64 defaultValue){
	char buf[64], default_val[64];
	char *p = buf;
	__int64 len;
	int base = 10;
	int mult = 1;

	// default value!
	sprintf_s(default_val, "%I64d", defaultValue);

	iniGetString(fileName, sectionName, varName, default_val, buf, 64);
	
	len = strlen(buf);
	chk:
	if(len > 1){
		// Suffix k m g ...? 
		switch(buf[len-1]){
			case 'k':  buf[--len] = 0;  mult = 1024;  goto chk;   break;
			case 'm':  buf[--len] = 0;  mult = 1024*1024;  goto chk;   break;
			case 'g':  buf[--len] = 0;  mult = 1024*1024*1024;  goto chk;   break;

			case 'K':  buf[--len] = 0;  mult = 1000;  goto chk;   break;
			case 'M':  buf[--len] = 0;  mult = 1000*1000;  goto chk;   break;
			case 'G':  buf[--len] = 0;  mult = 1000*1000*1000;  goto chk;   break;

			case 'h':  buf[--len] = 0; base = 16; break;

		}

		if(buf[0] == '0' && buf[1] == 'x'){
			p += 2; 
			len -= 2;
			base = 16;
		}

	}
		
	return ::_strtoi64(p, NULL, base);
}


// Tries to Read the value from app config 
// if not found tries to read the value in defaults configuration - otherwise the specified defaultValue will be used
__forceinline __int64 iniGetAppInteger(const char *section, const char *varName, __int64 defaultValue) {
	char fn[256];
	__int64 ret;

	sprintf_s(fn, INI_DEFAULT_APP_CONFIG, rgCore::rgCore_getAppName());


	ret =  iniGetInteger(fn, section, varName, 0xFFFFFFFFFFFFFFA2);
	if(ret == 0xFFFFFFFFFFFFFFA2){
		ret =  iniGetInteger(INI_DEFAULT_CONFIG, section, varName, defaultValue);
	}


	return ret;
}//end: iniGetAppBoolean()
