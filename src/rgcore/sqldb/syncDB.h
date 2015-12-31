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

namespace rgCore { namespace sqldb {
/**
 ** NOTE ABOUT syncDB
 ** 
 ** Implementation is based on legacy db() class (which was based on Aegis DB Wrapper)
 ** written for old AniRO's Account && Character Server
 ** it is NOT THREADSAFE, you'll have to deal with it as-is.
 ** 
 ** Its basicly very simple, just wrapped ODBC for 'easy use'
 **
 **
 **/

class syncDBException{
private:
char errormsg[4096];
	int sqlerrocde;
	int native;

public:
	syncDBException(const char *msg, int sqlerrcode);
	syncDBException(SQLSMALLINT dbHandleType, SQLHANDLE dbHandle);
	const char *getMessage();
	int getSqlErrCode();
	int getNative();

};




class syncDBStatement{
	friend class syncDB;

private:
	syncDBStatement(HDBC dbc, const char *statement);
	bool executed;

	HSTMT stmt;

public:
	~syncDBStatement();
	bool fetch();
	void exec();
	void cleanup(); 
		

	void bind(unsigned short col, 
				unsigned short target_type,
				void *target,
				unsigned int buflen,
				size_t *strlen_or_ind = NULL);

		
	template <size_t _Size>	inline void bindstr(unsigned short col, char (&_Dst)[_Size], int *outlen = NULL){
		this->bind(col, SQL_C_CHAR, _Dst, _Size, outlen);
	}
	template <size_t _Size>	inline void bindstr(unsigned short col, unsigned char (&_Dst)[_Size], int *outlen = NULL){
		this->bind(col, SQL_C_CHAR, _Dst, _Size, outlen);
	}
		
	inline void bindstr(unsigned short col, char *_Dst, int BufferSize, size_t *outlen = NULL){
		this->bind(col, SQL_C_CHAR, _Dst, BufferSize, outlen);
	}
	inline void bindstr(unsigned short col, unsigned char *_Dst, int BufferSize, size_t *outlen = NULL){
		this->bind(col, SQL_C_CHAR, _Dst, BufferSize, outlen);
	}
		

		
	template <size_t _Size>	inline void bindbin(unsigned short col, char (&_Dst)[_Size], size_t *outlen = NULL){
		this->bind(col, SQL_C_BINARY, _Dst, _Size, outlen);
	}
	template <size_t _Size>	inline void bindbin(unsigned short col, unsigned char (&_Dst)[_Size], size_t *outlen = NULL){
		this->bind(col, SQL_C_BINARY, _Dst, _Size, outlen);
	}

	inline void bindbin(unsigned short col, char *_Dst, int BufferSize, size_t *outlen = NULL){
		this->bind(col, SQL_C_BINARY, _Dst, BufferSize, outlen);
	}
	inline void bindbin(unsigned short col, unsigned char *_Dst, int BufferSize, size_t *outlen = NULL){
		this->bind(col, SQL_C_BINARY, _Dst, BufferSize, outlen);
	}
		
		

	inline void bindint(unsigned short col, char *_Dst){
		this->bind(col, SQL_C_STINYINT, _Dst, sizeof(char), NULL);
	}
	inline void bindint(unsigned short col, unsigned char *_Dst){
		this->bind(col, SQL_C_UTINYINT, _Dst, sizeof(char), NULL);
	}
		
	inline void bindint(unsigned short col, short *_Dst){
		this->bind(col, SQL_C_SSHORT, _Dst, sizeof(short), NULL);
	}
	inline void bindint(unsigned short col, unsigned short *_Dst){
		this->bind(col, SQL_C_USHORT, _Dst, sizeof(short), NULL);
	}

	inline void bindint(unsigned short col, int *_Dst){
		this->bind(col, SQL_C_SLONG, _Dst, sizeof(int), NULL);
	}
	inline void bindint(unsigned short col, unsigned int *_Dst){
		this->bind(col, SQL_C_ULONG, _Dst, sizeof(int), NULL);
	}

	inline void bindint(unsigned short col, __int64 *_Dst){
		this->bind(col, SQL_C_SBIGINT, _Dst, sizeof(__int64), NULL);
	}
	inline void bindint(unsigned short col, unsigned __int64 *_Dst){
		this->bind(col, SQL_C_UBIGINT, _Dst, sizeof(__int64), NULL);
	}



	void param(unsigned short param,
				unsigned short sql_type,
				unsigned short val_type,
				void *val,
				unsigned int buflen,
				size_t *strlen_or_ind);


	inline void parambin(unsigned short param, char *srcBuf, size_t &szBuf){
		this->param(param, SQL_C_BINARY, SQL_VARBINARY, srcBuf, (unsigned int)szBuf, &szBuf);
	}
		

	inline void paramint(unsigned short param, unsigned int &value){
		this->param(param, SQL_C_SLONG, SQL_INTEGER, &value, sizeof(int), NULL);
	}

};
	


// legacy DB Interface
class syncDB{
private:
	HENV env;
	HDBC dbc;
	bool initDone;
	bool dbResultOk(SQLRETURN rc, bool noDataAllowed);


public:
	syncDB(const char *DSN);
	~syncDB();


	syncDBStatement *stmt(const char *statement);

};


} }
