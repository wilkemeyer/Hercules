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

using namespace rgCore;

///
/// Optmization Hint:  
///  use multiple queues (1 for each 8 workers),  
///  addjob will distribute it via RR
///	 this also requires:
///		- multiple condition variables
///		- multiple worker-locks.
///

#define MAX_DBWORKER 80
#define MAX_STMT_INPARAM 32

#if !defined(_M_AMD64)
#error To run on non AMD64 platform, youll have to port some fancy stuff with parameter binding, 
#error as SQLLEN is always defined as INT64,  and the code assumes that size_t is int64 ....  
#endif

class asyncDBTaskExecException : public rgCore::util::Exception{
public:
	asyncDBTaskExecException(int code, const char *str) : rgCore::util::Exception(code, str){};
	
};

typedef size_t stackType;

typedef struct asyncDBTask : atomic::SLIST_NODE{
	enum{ 
		NOP = 0,
		EXEC_STMT
	} type;

	///
	/// Parameters for Task Types above.
	///

	union{
	//
	// EXEC_STMT Parameters
	//
	struct{
		stackType	inParam[MAX_STMT_INPARAM];
		
		dbStmtIdentifier statementID;

		onAsyncStmtCompleteProc callback;

		// opaque params
		void *paramPtr;
		size_t paramInt;

	} execStmt;

	//
	//
	//

	};
} asyncDBTask;

struct dbConnectionConfig{
	char *name;
	char *DSN;
	BOOL bDefined;
} *g_dbConnections = NULL;

struct dbStatementConfig{
	BOOL bDefined;
	struct dbConnectionConfig *conn;

	enum paramType{
		BIN = 0,
		STR,
		INT8,
		INT16,
		INT32,
		INT64
	};

	struct inParam{
		struct inParam *next;
		enum paramType type;
	} *inParams;
	size_t nInParams;


	// Worker proc which fetches data..
	asyncDBStmtProc proc;
	
	//
	SQLCHAR *strStatement;
	SQLLEN szStatement;
} *g_dbStatements = NULL;

struct WorkerdbConn{
	HENV hEnv;
	HDBC hDbc;
	BOOL bUsed;
};

struct WorkerdbStmt{
	HSTMT hStmt;
	struct WorkerdbConn *conn;
	BOOL bUsed;
};

struct WorkerParam{
	atomic::SLIST_HEAD *taskQueue;
	WorkerdbConn *conn;
	WorkerdbStmt *stmt;
} g_WorkerParam[MAX_DBWORKER]; 


static atomic::SLIST_HEAD g_taskQueue;
static util::thread *g_hWorker[MAX_DBWORKER];
static size_t g_nWorker = 0;

static bool g_initialized = false;
static bool g_running = true;

static util::cond *g_workerCond = NULL;
static util::mutex *g_workerLock = NULL;

static size_t g_maximum_Connections = 0;
static size_t g_maximum_Statements = 0;

static Pool<asyncDBTask> *g_freePool = NULL;

static DWORD __stdcall asyncDBWorkerProc(util::thread *_self);
static void dbDisconnect(HENV hEnv, HDBC hDbc);



void asyncDB_init(){
	
	putLog("asyncDB Initialization Begin\n");

	// Free Pool.
	size_t allocStep = iniGetAppInteger("AsyncDB", "Task Allocation Step", 512);
	size_t initAlloc = iniGetAppInteger("AsyncDB", "Task Initial Allocation", 1024);
	if(allocStep < 128) allocStep = 128;
	else if(allocStep > 65536) allocStep = 65536;
	if(initAlloc < 128) initAlloc = 128;
	else if(initAlloc > 134217728) initAlloc = 134217728;
	putLog("asyncDB Task Queue  allocStep: %u\n", allocStep);
	putLog("asyncDB Task Queue  initAlloc: %u\n", initAlloc);

	g_freePool = new Pool<asyncDBTask>("AsyncDB Task", allocStep, initAlloc, true);

	//
	atomic::SListInitializeHead(&g_taskQueue);

	//
	g_maximum_Connections = iniGetAppInteger("AsyncDB", "Maximum Connections", 256);
	g_maximum_Statements = iniGetAppInteger("AsyncDB", "Maximum Statements", 128);
	if(g_maximum_Connections <= 0) g_maximum_Connections = 8;
	else if(g_maximum_Connections > 1024) g_maximum_Connections = 1024;
	if(g_maximum_Statements <= 0) g_maximum_Statements = 64;
	else if(g_maximum_Statements > 8192) g_maximum_Statements = 8192;
	putLog("asyncDB g_maximum_Connections: %I64d\n", g_maximum_Connections);
	putLog("asyncDB g_maximum_Statements: %I64d\n", g_maximum_Statements);

	//
	// 
	//
	g_dbConnections = (struct dbConnectionConfig*) roalloc( sizeof(struct dbConnectionConfig) * g_maximum_Connections);
	g_dbStatements = (struct dbStatementConfig*) roalloc( sizeof(struct dbStatementConfig) * g_maximum_Statements);
	memset(g_dbConnections, 0x00, sizeof(dbConnectionConfig)*g_maximum_Connections);
	memset(g_dbStatements, 0x00, sizeof(struct dbStatementConfig)*g_maximum_Statements);


	g_workerCond = new util::cond();
	g_workerLock = new util::mutex();

	g_nWorker = iniGetAppInteger("AsyncDB", "Worker Count", 12);
	if(g_nWorker <= 0)
		g_nWorker = 1;
	else if(g_nWorker > MAX_DBWORKER)
		g_nWorker = MAX_DBWORKER;


	g_running = true;

	for(size_t i = 0; i < g_nWorker; i++){
		// 
		// Also initialze workerParam struct!
		//
		g_WorkerParam[i].taskQueue = &g_taskQueue;
		g_WorkerParam[i].stmt = (struct WorkerdbStmt*)roalloc( sizeof(struct WorkerdbStmt) * g_maximum_Statements );
		g_WorkerParam[i].conn = (struct WorkerdbConn*)roalloc( sizeof(struct WorkerdbConn) * g_maximum_Connections );

		memset(g_WorkerParam[i].conn, 0x00, sizeof(struct WorkerdbConn)*g_maximum_Connections);
		memset(g_WorkerParam[i].stmt, 0x00, sizeof(struct WorkerdbStmt)*g_maximum_Statements);

		char nBuf[64];
		sprintf_s(nBuf, "asyncDB Worker#%zu", i);

		g_hWorker[i] = new util::thread(nBuf, asyncDBWorkerProc, &g_WorkerParam[i]);
		if(g_hWorker[i] == NULL){
			fatalError("asyncDB_init() -> CreateThread failed!");
		}
	}

	putLog("asyncDB g_nWorker: %I64d\n", g_nWorker);
	

	putLog("asyncDB Initialization Finished\n");

	g_initialized = true;
}//end: asyncdb_init()


void asyncDB_final(){
	if(g_initialized == false)
		return;

	g_running = false;
	g_workerCond->broadcast();
	for(size_t i = 0; i < g_nWorker; i++){
		g_hWorker[i]->wait();
		delete g_hWorker[i]; 
		g_hWorker[i] = NULL;
	}
	g_nWorker = 0;


	//
	delete g_workerLock;
	delete g_workerCond;

	//
	delete g_freePool; // this also releases all .. pending jobs :D 
	g_freePool = NULL;

	for(size_t i = 0; i < g_maximum_Statements; i++){
		if(g_dbStatements[i].bDefined == FALSE)
			continue;
		
		// free statement text.
		rofree(g_dbStatements[i].strStatement);

		// free inparam definitions
		{
			struct dbStatementConfig::inParam *iter, *iter_next;
			iter = g_dbStatements[i].inParams;
			while(1){
				if(iter == NULL) break;
				iter_next = iter->next;
				rofree(iter);
				iter = iter_next;
			}
		}

	}
	rofree(g_dbStatements); g_dbStatements = NULL;


	for(size_t i = 0; i < g_maximum_Connections; i++){
		if(g_dbConnections[i].bDefined == FALSE)
				continue;

		rofree(g_dbConnections[i].name);
		rofree(g_dbConnections[i].DSN);

	}
	rofree(g_dbConnections); g_dbConnections = NULL;
	


	g_initialized = false;	
}//end: asyncDB_final()


/* Stats of Task Item Pool */
void asyncDB_getTaskPoolCounters(size_t *numTotal, size_t *numFree, size_t *numPeak){
	if(g_initialized == false){
		*numTotal = 0;
		*numFree = 0;
		*numPeak = 0;
		return;
	}

	g_freePool->getCounters(numTotal, numFree, numPeak);

}//end: asyncDB_getTaskPoolCounters()


/* Adds the given task to the taskqueue */
static __forceinline void taskPut(asyncDBTask *task){
	atomic::SListPush(&g_taskQueue, task);
	g_workerCond->signal();
}//end: taskPut()


static __forceinline bool dbResultOK(SQLRETURN rc, bool noDataAllowed){
	if( ( SQL_SUCCESS == rc ) || ( SQL_SUCCESS_WITH_INFO == rc ) || ( noDataAllowed && ( SQL_NO_DATA == rc ) ) )
		return true;
	else
		return false;
}//end: dbResultOK()


static const char *dbGetMessage(const char *msg, char *buf, short buflen, SQLSMALLINT handleType, SQLHANDLE handle){
	SQLCHAR sqlState[32],
			txtError[8192];
	SQLSMALLINT len;
	SQLINTEGER nativeerror;

	SQLRETURN rc = SQLGetDiagRec(handleType, handle, 1, sqlState, &nativeerror, txtError, sizeof(txtError), &len);
	if(false == dbResultOK(rc, false) )
		sprintf_s(buf, buflen, "%s: dbGetMessage() -> Unable to retrieve ODBC Error Code: %d", msg, rc);
	else
		sprintf_s(buf, buflen, "%s: [%s] %s", msg, sqlState, txtError);

	return buf;
}//end: dbGetMessage()


static bool dbConnect(HENV *hEnv, HDBC *hDbc, const char *linkName, const char *DSN){
	SQLRETURN rc;
	SQLCHAR strOut[1024];
	SQLSMALLINT iOut;

	//putLog("dbConnect() to %s ....", linkName);

	rc = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, hEnv);
	if( dbResultOK(rc, false) ){
		rc = SQLSetEnvAttr(*hEnv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);
		if(false == dbResultOK(rc, false)){
			char msg[4096];
			putLog(dbGetMessage("failed. [Error while requesting ODBC Version 3]", msg, sizeof(msg), SQL_HANDLE_ENV, *hEnv));
			SQLFreeHandle(SQL_HANDLE_ENV, *hEnv);
			return false;
		}
	}else{
		putLog("dbConnect %s failed. - SQLAllocHandle() for ENV failed!\n", linkName);
		return false;
	}
	
	
	rc = SQLAllocHandle(SQL_HANDLE_DBC, *hEnv, hDbc);
	if( false == dbResultOK(rc, false) ){
		SQLFreeHandle(SQL_HANDLE_ENV, *hEnv);
		putLog("dbConnect %s failed. - ASQLAllocHandle() for DBC failed!\n", linkName);
		return false;
	}

	rc = SQLDriverConnect(*hDbc, NULL, (SQLCHAR*)DSN, SQL_NTS, strOut, 1024, &iOut, SQL_DRIVER_NOPROMPT);
	if(false == dbResultOK(rc, false)){
		putLog("dbConnect %s -> %s\n", linkName, dbGetMessage("failed. [SQLConnect()]", (char*)strOut, sizeof(strOut), SQL_HANDLE_DBC, *hDbc));
		SQLFreeHandle(SQL_HANDLE_DBC, *hDbc);
		SQLFreeHandle(SQL_HANDLE_ENV, *hEnv);
		return false;
	}

	//putLog("ok\n");

return true;
}//end: dbConnect()

static void dbDisconnect(HENV hEnv, HDBC hDbc){
			SQLDisconnect(hDbc);
			SQLFreeHandle(SQL_HANDLE_DBC, hDbc);
			SQLFreeHandle(SQL_HANDLE_ENV, hEnv);
}//end: dbDisconnect()


static bool dbPrepStmt(HDBC hDbc, HSTMT *hStmt, const char *statement, size_t len){
	SQLRETURN rc;

	rc = SQLAllocHandle(SQL_HANDLE_STMT, hDbc, hStmt);
	if(false == dbResultOK(rc, false) ){
		char buf[4096];
		putLog("dbPrepStmt(%s) failed, Allochandle failed! -> %s\n", statement, dbGetMessage("", buf, sizeof(buf), SQL_HANDLE_DBC, hDbc));
		return false;
	}

	// Prepare!
	rc = SQLPrepare( *hStmt, (SQLCHAR*)statement, (SQLINTEGER)len);
	if(false == dbResultOK(rc, false) ){
		char buf[4096];
		putLog("dbPrepStmt(%s) failed, DB -> %s\n", statement, dbGetMessage("", buf, sizeof(buf), SQL_HANDLE_STMT, *hStmt));
		SQLFreeHandle(SQL_HANDLE_STMT, *hStmt);
		return false;
	}

	return true;
}//end: dbPrepStmt()


dbStmtIdentifier asyncDB_newStatement(dbConnectionIdentifier connection,
									 const char *stmt,
									 asyncDBStmtProc proc)
{
	dbStmtIdentifier id = DB_INVALID_STATEMENT;

	//
	// Get free Statement identifier
	//
	for(size_t i = 0; i < g_maximum_Statements; i++){
		if(g_dbStatements[i].bDefined == FALSE){
			id = (dbStmtIdentifier)i;
			break;
		}
	}

	if(id == DB_INVALID_STATEMENT){
		putErr("asyncDB_newStatement() -> Out of Statement Slots, please increase Maximum Statements in Configuration!\n");
		return id;
	}

	if(connection < 0 || connection >= g_maximum_Connections ||  g_dbConnections[connection].bDefined == FALSE){
		putErr("asyncDB_newStatement() -> Invalid ConnectionIdentifier '%u'\n", connection);
		return DB_INVALID_STATEMENT;
	}


	//
	// Parse Statement to get in parameters, and replace %x with ? .. 
	// 
	//
	size_t szStmt = strlen(stmt);
	char *pDest = (char*)roalloc( szStmt + 1 );
	register char *p = pDest;

	const char *stmt_bak = stmt; // for debug / offset printing.
	struct dbStatementConfig::inParam *inParamBegin = NULL, *inParamLast = NULL;
	size_t nInParam = 0;

	bool esc = false;
	while(1){
		register char c = *stmt++;
		
		if(c == '\0'){
			*p++ = c;
			break;
		}else if( c == '%' ){
			
			if(esc == true){
				//
				// handle %% case 
				//
				*p++ = c;
				esc = false;
			}else{
				// % begin.
				esc = true;
			}

		}else if(esc == true){
			enum dbStatementConfig::paramType type;


			if(c == 'b'){
				// 
				// param binary.
				// [ptr, len]
				//
				type = dbStatementConfig::BIN;

			}else if(c == 's'){
				//
				// param string.
				// [ptr, len]
				//
				type = dbStatementConfig::STR;

			}else if(c == 'd'){
				//
				// param int
				// [int]
				//
				type = dbStatementConfig::INT32;

			}else if(c == 'i'){
				//
				// param int64
				// [__int64]
				//
				type = dbStatementConfig::INT64;

			}else if(c == 'c'){
				//
				// param char (byte)
				// [char]
				//
				type = dbStatementConfig::INT8;

			}else if(c == 'w'){
				//
				// param int
				// [short]
				//
				type = dbStatementConfig::INT16;

			}else{
				putErr("asyncDB_newStatement() -> '%s', Error near offset %u - '%c' is an unknown Input Mode char.\n", stmt_bak, (stmt - stmt_bak), c);
				struct dbStatementConfig::inParam *iter, *iter_next;
				iter = inParamBegin;
				while(1){
					if(iter == NULL) break;
					iter_next = iter->next;

					rofree(iter);
					iter = iter_next;
				}
				rofree(pDest);
				return DB_INVALID_STATEMENT;
			}

			*p++ = '?';
			esc = false;

			struct dbStatementConfig::inParam *paramDef;
			paramDef = (struct dbStatementConfig::inParam*) roalloc( sizeof(struct dbStatementConfig::inParam) );
			paramDef->next = NULL;
			paramDef->type = type;

			if(inParamLast == NULL){
				inParamBegin = paramDef;
				inParamLast = paramDef;
			}else{
				inParamLast->next = paramDef;
				inParamLast = paramDef;
			}

			nInParam++;

		}else{
			*p++ = c;
		}
	}




	//
	// Check Limits.
	//
	if(nInParam > MAX_STMT_INPARAM){
		putErr("asyncDB_newStatement() -> '%s' Too many IN-Parameters (has: %u, Max Supported: %u), recompile with hihger MAX_STMT_INPARAM\n", nInParam, MAX_STMT_INPARAM);
		goto err;
	}
	if(0){
err:	// a bit dirty...
			{
				struct dbStatementConfig::inParam *iter, *iter_next;
				iter = inParamBegin;
				while(1){
					if(iter == NULL) break;
					iter_next = iter->next;

					rofree(iter);
					iter = iter_next;
				}
				rofree(pDest);
			}

			return DB_INVALID_STATEMENT;
	}


	//
	// Assign values:
	//
	g_dbStatements[id].strStatement = (SQLCHAR*)pDest;
	g_dbStatements[id].szStatement	= (SQLLEN)(p - pDest);
	g_dbStatements[id].inParams		= inParamBegin;
	g_dbStatements[id].nInParams	= nInParam;
	g_dbStatements[id].proc			= proc;	
	g_dbStatements[id].bDefined		= TRUE;
	g_dbStatements[id].conn			= &g_dbConnections[connection];

	//
	// Allocate / Prepare statement for all workers.. 
	//
	for(size_t i = 0; i < g_nWorker; i++){
		struct WorkerParam *wp = &g_WorkerParam[i];

		if( false == dbPrepStmt( wp->conn[connection].hDbc,  &wp->stmt[id].hStmt, (char*) g_dbStatements[id].strStatement, g_dbStatements[id].szStatement)){
			//
			// free all statements @ previous workers..
			//
			if(i > 0)
				for(--i; i >= 0 ; i--){
					wp = &g_WorkerParam[i];
					SQLFreeHandle(SQL_HANDLE_STMT, wp->stmt[id].hStmt);
					wp->stmt[id].bUsed = FALSE;
				}

			g_dbStatements[id].bDefined = FALSE; //!

			goto err; // returns with invalid conn and frees everything!
		}

		wp->stmt[id].conn = &wp->conn[connection];
		wp->stmt[id].bUsed = TRUE;

	}


	// Debug:
	putLog("asyncDB_newStatement() -> '%s' (%u inParameters)\n", g_dbStatements[id].strStatement, nInParam);

	return id;
}


dbConnectionIdentifier asyncDB_newConnection( const char *name, const char *DSN ){
	dbConnectionIdentifier id = DB_INVALID_CONNECTION;

	for(size_t i = 0; i < g_maximum_Connections; i++){
		if(g_dbConnections[i].bDefined == FALSE){
			id = (dbConnectionIdentifier)i; 
			break;
		}
	}

	if(id == DB_INVALID_CONNECTION){
		putErr("asyncDB_newConnection() -> '%s' (DSN: %s) - Out of Connections, Please increase Maximum Connections in Configuration!\n", name, DSN);
		return id;
	}

	size_t szDSN, szName;
	szName = strlen(name);
	szDSN = strlen(DSN);
	g_dbConnections[id].name = (char*)roalloc( szName + 1 );
	memcpy(g_dbConnections[id].name, name, szName+1);

	g_dbConnections[id].DSN = (char*)roalloc( szDSN + 1 );
	memcpy(g_dbConnections[id].DSN, DSN, szDSN+1);

	putLog("dbConnect %s.. ", name);

	for(size_t i = 0; i < g_nWorker; i++){
		if(dbConnect( &g_WorkerParam[i].conn[id].hEnv,  &g_WorkerParam[i].conn[id].hDbc, name, DSN) == false){
			
			// cleanup already established connecitons :(
			if( i > 0)
				for( --i; i >= 0; i--){
					dbDisconnect(g_WorkerParam[i].conn[id].hEnv, g_WorkerParam[i].conn[id].hDbc);
					g_WorkerParam[i].conn[id].bUsed = FALSE;
				}

			//
			// Free strings..
			//
			rofree( g_dbConnections[id].name );
			rofree( g_dbConnections[id].DSN ); 
			putErr("asyncDB_newConnection() -> '%s' (DSN: %s) - Failed to establish Connection for Worker #%u!\n", name, DSN, i);
			return DB_INVALID_CONNECTION;

		}

		g_WorkerParam[i].conn[id].bUsed = TRUE;

		putLog(".");
	}

	putLog(" ok\n");

	g_dbConnections[id].bDefined = TRUE;


	return id;
}//end: asyncDB_newConnection()


void asyncDB_exec(dbSysStmt stmt){
	WorkerdbStmt *hStmt = (WorkerdbStmt*)stmt;

	SQLRETURN rc;
	rc = SQLExecute(hStmt->hStmt);
	if( false == dbResultOK(rc,false) ){
		char buf[4096];
		dbGetMessage("asyncDB_exec() failed, Message -> ", buf, sizeof(buf), SQL_HANDLE_STMT, hStmt->hStmt);
		throw new asyncDBTaskExecException(1, buf);
	}

}//end: asyncDB_exec()

bool asyncDB_fetch(dbSysStmt stmt){
	WorkerdbStmt *hStmt = (WorkerdbStmt*)stmt;
	SQLRETURN rc;

	rc = SQLFetch(hStmt->hStmt);
	if(rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO)
		return true;
	else if(rc == SQL_NO_DATA)
		return false;	// normal behavior!
	else{
		char buf[4096];
		dbGetMessage("", buf, sizeof(buf), SQL_HANDLE_STMT, hStmt->hStmt);
		putErr("++ asyncDB_fetch() failed with the following message: \n");
		putErr("++ %s\n", buf);
	}

	return false;
}//end: asyncDB_fetch()


void asyncDB_queueStatement(dbStmtIdentifier stmt, 
							onAsyncStmtCompleteProc	callback,
							void *paramPtr,
							size_t paramInt, 
							...)
{
	//
	//
	asyncDBTask *task = g_freePool->get();
	va_list ap;
	size_t param;

	task->type = asyncDBTask::EXEC_STMT;
	task->execStmt.statementID = stmt;
	task->execStmt.paramPtr = paramPtr;
	task->execStmt.paramInt = paramInt;
	task->execStmt.callback = callback;

	// copy in-parameters.
	param = 0;
	va_start(ap, paramInt);
	for(struct dbStatementConfig::inParam *iter = g_dbStatements[stmt].inParams;   iter != NULL;   iter = iter->next){
		size_t n;
		switch(iter->type){
		case dbStatementConfig::BIN:	n = 2; /* [ptr] [len] */  break;
		case dbStatementConfig::STR:	n = 2; /* [ptr_cstring] [len] */  break;
		case dbStatementConfig::INT8:	n = 1; /* [num] */ break;
		case dbStatementConfig::INT16:	n = 1; /* [num] */ break;
		case dbStatementConfig::INT32:	n = 1; /* [num] */ break;
		case dbStatementConfig::INT64:	n = 1; /* [num] */ break;
		default: n = 0; break; // unknown? wtf?
		}

		for(size_t i = 0; i < n; i++)
			task->execStmt.inParam[param++] = va_arg(ap, size_t);

	}
	va_end(ap);

	taskPut(task);

}




void asyncDB_stmt_cleanup(dbSysStmt stmt){
	WorkerdbStmt *hStmt = (WorkerdbStmt*)stmt;

	SQLFreeStmt(hStmt->hStmt, SQL_UNBIND);
	SQLFreeStmt(hStmt->hStmt, SQL_RESET_PARAMS);
	SQLFreeStmt(hStmt->hStmt, SQL_CLOSE);	// no, this will not close the connection nor statement!
											// it just closes the current cursor .. next execute call will allocate a new one :)

}//end: asyncdb_stmt_celanup()


void _asyncDB_bind(dbSysStmt stmt, unsigned short ccol, unsigned short target_type, void *target, unsigned int buflen, size_t *strlen_or_ind){
	WorkerdbStmt *hStmt = (WorkerdbStmt*)stmt;
	SQLRETURN rc;

	rc = SQLBindCol(hStmt->hStmt, ccol, target_type, target, buflen, (SQLLEN*)strlen_or_ind);

	if(false == dbResultOK(rc, false) || rc != SQL_SUCCESS){
		char buf[4096];
		char debug[128];
		sprintf(debug, "_asyncDB_bind(%u) (buflen: %u) failed, Message -> ", ccol, buflen);
		dbGetMessage(debug, buf, sizeof(buf), SQL_HANDLE_STMT, hStmt->hStmt);
		throw new asyncDBTaskExecException(2, buf);
	}

}//end: _asyncBD_bind()


static void dbBindParam(HSTMT hStmt, unsigned short col, unsigned short sql_type, unsigned short val_type, void *val, unsigned int buflen, size_t *strlen_or_ind){
	SQLRETURN rc;

	rc = SQLBindParameter(hStmt, col, SQL_PARAM_INPUT, sql_type, val_type, buflen, 0, val, 0, (SQLLEN*)strlen_or_ind);
	if(false == dbResultOK(rc, false) || rc != SQL_SUCCESS){
		char buf[4096];
		char debug[128];
		
		sprintf(debug, "dbBindParam(%u) (buflen: %u) failed, Message -> ", col, buflen);

		dbGetMessage(debug, buf, sizeof(buf), SQL_HANDLE_STMT, hStmt);
		throw new asyncDBTaskExecException(3, buf);
	}

}//end: dbBindParam()


static void task_execStmt(WorkerdbStmt *stmt, asyncDBTask *task){
	size_t rows;

	try{
		struct dbStatementConfig *conf = &g_dbStatements[task->execStmt.statementID];
		//
		// Bind IN Parameters:
		//
		size_t paramPos = 0;
		unsigned short col = 1;
		for(struct dbStatementConfig::inParam *iter = conf->inParams;  iter != NULL;  iter = iter->next){
			
			switch(iter->type){
			case dbStatementConfig::BIN:
				{
				char *ptr;
				unsigned int len;

				ptr = (char*)task->execStmt.inParam[paramPos++];			// Hint:
				len = (unsigned int)task->execStmt.inParam[paramPos++];		//  dont remove this local assignments, oterhweise the compiler may crap the paramPos++ by optimizing it out (due to compiler bug) and place paramPos+=2 after the call. 

				dbBindParam(stmt->hStmt, col++, SQL_C_BINARY, SQL_VARBINARY, ptr, len, NULL);	// NOTE: binary type may need sqllen / IND param!! @@ TRY / FIXME
				}
				break;

			case dbStatementConfig::STR:
				{
				char *ptr;
				unsigned int len;

				ptr = (char*)task->execStmt.inParam[paramPos++];			// Hint:
				len = (unsigned int)task->execStmt.inParam[paramPos++];		//  dont remove this local assignments, oterhweise the compiler may crap the paramPos++ by optimizing it out (due to compiler bug) and place paramPos+=2 after the call. 

				dbBindParam(stmt->hStmt, col++, SQL_C_CHAR, SQL_VARCHAR, ptr, len, NULL);
				}
				break;
	
			case dbStatementConfig::INT8:
				dbBindParam(stmt->hStmt, col++, SQL_C_STINYINT, SQL_TINYINT, &task->execStmt.inParam[paramPos++], sizeof(char), NULL);
				break;

			case dbStatementConfig::INT16:
				dbBindParam(stmt->hStmt, col++, SQL_C_SSHORT, SQL_SMALLINT, &task->execStmt.inParam[paramPos++], sizeof(short), NULL);
				break;

			case dbStatementConfig::INT32:
				dbBindParam(stmt->hStmt, col++, SQL_C_SLONG, SQL_INTEGER, &task->execStmt.inParam[paramPos++], sizeof(int), NULL);
				break;

			case dbStatementConfig::INT64:
				dbBindParam(stmt->hStmt, col++, SQL_C_SBIGINT, SQL_BIGINT, &task->execStmt.inParam[paramPos++], sizeof(__int64), NULL);
				break;

			}

		}

		// Call user Provided Callback for this statement, to fetchd ata
		// this proc is reliable for:
		//  exec, fetc
		rows = conf->proc( (dbSysStmt)stmt, task->execStmt.paramPtr, task->execStmt.paramInt );

		// Cleanup ..
		asyncDB_stmt_cleanup((dbSysStmt)stmt);

	}catch(asyncDBTaskExecException *e){
		rows = 0;
		
		putErr("++ task_execStmt(%u) failed with code %u\n", task->execStmt.statementID, e->getCode());
		putErr("++ task_execStmt(%u) Message: %s\n", task->execStmt.statementID, e->getMessage());

		// code overview:
		// 1 -> exec failed
		// 2 -> bind failed
		// 3 -> bindParam failed

		delete e;
	}




	//
	// call callback of the task!
	//
	if(task->execStmt.callback != NULL) // null is valid, as some statements may dont have a result or processing afterwards (for example: logging to db)
		task->execStmt.callback(rows, task->execStmt.paramPtr, task->execStmt.paramInt);


}//end: task_execStmt()



static DWORD __stdcall asyncDBWorkerProc(util::thread *_self){
	struct WorkerParam *workerParam = (struct WorkerParam*) (_self->getStartParameter());

	//
	// Idle Loop.
	//
	while(1){
		asyncDBTask *task = (asyncDBTask*)atomic::SListPop(workerParam->taskQueue);
		 
		if(task != NULL){
			
			switch(task->type){
				// nothing
			case asyncDBTask::NOP: break;
			
				// exec stmt :)
			case asyncDBTask::EXEC_STMT:	task_execStmt( &workerParam->stmt[task->execStmt.statementID], task); 	break;

				// whenever this happens, this is an indicator of corrupted memory.. or .. code-fault.
			default: putErr("asyncDBWorkerProc() -> got Task with type '%u', which is unhandled / unknown?! - skipping.\n", task->type); break;
			}

			g_freePool->put( task );
			continue; // ..
		}

		if(g_running == false)	// this check must be placed below task->run() .
			break;				// otherwise a clean software shutdown would cause data-loss.

		g_workerLock->lock();
		g_workerCond->wait(g_workerLock, 12000);
		g_workerLock->unlock();
	}





	//
	// Release all Statements and Connections.
	//
	for(size_t i = 0; i < g_maximum_Statements; i++){
		if(workerParam->stmt[i].bUsed == TRUE){
			SQLFreeHandle(SQL_HANDLE_STMT, workerParam->stmt[i].hStmt);
			workerParam->stmt[i].bUsed = FALSE;
		}
	}

	for(size_t i = 0; i < g_maximum_Connections; i++){
		if(workerParam->conn[i].bUsed == TRUE){
			dbDisconnect(workerParam->conn[i].hEnv, workerParam->conn[i].hDbc);
			workerParam->conn[i].bUsed = FALSE;
		}
	}



	//
	// Free Local Parameters for Statements and Connections!
	//
	rofree(workerParam->stmt);
	rofree(workerParam->conn);


	return 0;
}//end: asyncDBWorkerProc()

