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


static bool dbResultOk( SQLRETURN rc, bool noDataAllowed ){
	if( ( SQL_SUCCESS == rc ) || ( SQL_SUCCESS_WITH_INFO == rc ) || ( noDataAllowed && ( SQL_NO_DATA == rc ) ) )
		return true;
	else
		return false;
}

namespace rgCore { namespace sqldb {

syncDBException::syncDBException(const char *msg, int sqlerrcode){
	strncpy_s(this->errormsg, msg, 4096);
	this->sqlerrocde = sqlerrcode;
}


syncDBException::syncDBException(SQLSMALLINT dbHandleType, SQLHANDLE dbHandle){
	SQLCHAR sqlstate[10];
	SQLINTEGER nativeerror;
	SQLCHAR errortxt[3072];
	SQLSMALLINT errorlen = 3072;
	SQLRETURN rc = SQLGetDiagRec( dbHandleType, dbHandle, 1, sqlstate, &nativeerror, errortxt, 3072, &errorlen );

	if( !dbResultOk( rc, false ) ){
		sprintf_s(this->errormsg, "Unable to retrieve ODBC Error Code: %d", rc);
		this->sqlerrocde = rc;
	}else{
		size_t i;
		this->native = nativeerror;
		this->sqlerrocde = rc;

		i = sprintf_s(this->errormsg, "[%s] %s", sqlstate, errortxt);
		this->errormsg[i] = 0;

	}
}

const char* syncDBException::getMessage(){
	return this->errormsg;
}

int syncDBException::getSqlErrCode(){
	return this->sqlerrocde;
}

int syncDBException::getNative(){
	return this->native;
}



// ===
syncDB::syncDB(const char *DSN){
	
	SQLRETURN rc;
	SQLCHAR strOut[1024];
	SQLSMALLINT iOut = 0;

	this->initDone = false;

	rc = SQLAllocHandle( SQL_HANDLE_ENV, SQL_NULL_HANDLE, &this->env );

	if( this->dbResultOk( rc, false ) )	{
		rc = SQLSetEnvAttr( this->env, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0 );
		if( !this->dbResultOk( rc, false ) )
			throw new syncDBException( SQL_HANDLE_ENV, this->env );
	}

	if( this->dbResultOk( rc, false ) )	{
		rc = SQLAllocHandle( SQL_HANDLE_DBC, this->env, &this->dbc );
		if( !this->dbResultOk( rc, false ) )
			throw new syncDBException( SQL_HANDLE_ENV, this->env );
	}

	if( dbResultOk( rc, false ) )
	{
		rc = SQLDriverConnect( this->dbc, NULL, (SQLCHAR*)DSN, SQL_NTS, strOut, 1024, &iOut, SQL_DRIVER_NOPROMPT );
		if( !this->dbResultOk( rc, false ) )
			throw new syncDBException( SQL_HANDLE_DBC, this->dbc );
	}



  if(! dbResultOk( rc, false ) )
		throw new syncDBException(SQL_HANDLE_DBC, this->dbc);

  this->initDone = true;
}

syncDB::~syncDB(){
	if(this->initDone != true)
			return;

	SQLDisconnect( this->dbc);	
	SQLFreeHandle(SQL_HANDLE_DBC, this->dbc);
	SQLFreeHandle(SQL_HANDLE_ENV, this->env );

	this->initDone = false;
}


bool syncDB::dbResultOk(SQLRETURN rc, bool noDataAllowed){
	return ::dbResultOk(rc, noDataAllowed);
}



syncDBStatement *syncDB::stmt(const char *statement){
	syncDBStatement *stmt;

	stmt = new syncDBStatement(this->dbc, statement);
	
	return stmt;
}




syncDBStatement::syncDBStatement(HDBC dbc, const char *statement){
	SQLRETURN rc;

	this->executed = false;

	rc = SQLAllocHandle(SQL_HANDLE_STMT, dbc, &this->stmt);
	if(!dbResultOk(rc, false))
		throw new syncDBException(SQL_HANDLE_DBC, dbc);

	rc = SQLPrepare(this->stmt, (SQLCHAR*)statement, SQL_NTS);
	if(!dbResultOk(rc, false))
		throw new syncDBException(SQL_HANDLE_STMT, this->stmt); //possible memleak @ allochandle, but - how?

}

syncDBStatement::~syncDBStatement(){
	
	// fix for leaks.
	if(this->executed != false)
		this->cleanup();

	SQLFreeHandle(SQL_HANDLE_STMT, this->stmt);
}


void syncDBStatement::cleanup(){
	if(this->executed == true){
		SQLFreeStmt(this->stmt, SQL_UNBIND);
		SQLFreeStmt(this->stmt, SQL_RESET_PARAMS);
		SQLFreeStmt(this->stmt, SQL_CLOSE);		// no, this will not close the connection nor statement!
												// it just closes the current cursor .. next execute call will allocate a new one :)
		this->executed = false; // :)
	}
}


void syncDBStatement::exec(){
	SQLRETURN rc;

	this->executed = true;

	rc = SQLExecute(this->stmt);
	if(!dbResultOk(rc, false))
		throw new syncDBException(SQL_HANDLE_STMT, this->stmt);


}


void syncDBStatement::param(unsigned short param,
					unsigned short sql_type,
					unsigned short val_type,
					void *val,
					unsigned int buflen,
					size_t *strlen_or_ind)
{
	SQLRETURN rc;

	rc = SQLBindParameter(this->stmt, param, SQL_PARAM_INPUT, sql_type, val_type, buflen, 0, val, 0, (SQLLEN*)strlen_or_ind);
	if(false == dbResultOk(rc, false) || rc != SQL_SUCCESS)
		throw new syncDBException(SQL_HANDLE_STMT, this->stmt);
}


void syncDBStatement::bind(unsigned short col, 
					unsigned short target_type,
					void *target,
					unsigned int buflen,
					size_t *strlen_or_ind) {
	SQLRETURN rc;
	SQLLEN temp = buflen;
	if(strlen_or_ind == NULL)
		strlen_or_ind = (size_t*)&temp;

	rc = SQLBindCol(this->stmt, col, target_type, target, buflen, (SQLLEN*)strlen_or_ind);
	if(!dbResultOk(rc, false))
		throw new syncDBException(SQL_HANDLE_STMT, this->stmt);

}


bool syncDBStatement::fetch(){
	SQLRETURN rc = SQLFetch(stmt);
	
	if(rc == SQL_SUCCESS)
		return true;

return false;
}


} }  // end namespaces
