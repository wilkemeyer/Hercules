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

//
void asyncDB_init();
void asyncDB_final();

/* Free Pool Stats */
void asyncDB_getTaskPoolCounters(size_t *numTotal, size_t *numFree, size_t *numPeak);


//
// Note to newConnection and newStatement
//  dont call this methods during runtime or concurrently - this may result in
//  unpredictable results..
//

#define DB_INVALID_CONNECTION -1
typedef int dbConnectionIdentifier;
dbConnectionIdentifier asyncDB_newConnection(const char *name, 
											  const char *DSN);


/**
 * Creates a new Statement (to be used for async Operations)
 * 
 * @param connection
 * @param stmt	the statement string including in parameter descriptions
 *				Example:
 *					"Select * FROM [user] WHERE [name] = %s AND [grade] = %d"
 *				
 *				Supported Parameter Specifiers for In Parameters:
 *					b	-> binary, requires two parameters [ ptr,  length ]
 *					s	-> null terminated cstring, requires one parameter  [ ptr, length ]
 *					d	-> signed int, requires one paramter [ int ]
 *					i	-> signed __int64, requires one parameter [ __int64 ]
  *					c	-> signed char, requires one parameter [ char ]
 *					w	-> signed short, requires one parameter [ short ]
 *					
 *					
 *	@param proc	the worker proc, must be given
 *
 * @return int Statement ID
 */
#define DB_INVALID_STATEMENT -1
typedef int dbStmtIdentifier;
typedef void* dbSysStmt;
typedef size_t (*asyncDBStmtProc)(dbSysStmt stmt, void *paramPtr, size_t paramInt); // must return number rows - fetched!

dbStmtIdentifier asyncDB_newStatement(int connection,
						 const char *stmt,
						 asyncDBStmtProc proc
						 );


///
/// API Functions to be used in asyncDBStmtProc
/// 

void _asyncDB_bind(dbSysStmt stmt, unsigned short ccol, unsigned short target_type, void *target, unsigned int buflen, size_t *strlen_or_ind);
#define asyncDB_bind_str(stmt, col, buf, szBuf, outlen) _asyncDB_bind(stmt, col, SQL_C_CHAR,     buf, szBuf, outlen)
#define asyncDB_bind_bin(stmt, col, buf, szBuf, outlen) _asyncDB_bind(stmt, col, SQL_C_BINARY,   buf, szBuf, outlen)

#define asyncDB_bind_int8(stmt, col, out)   _asyncDB_bind(stmt, col, SQL_C_STINYINT, out, sizeof(char), NULL)
#define asyncDB_bind_uint8(stmt, col, out)  _asyncDB_bind(stmt, col, SQL_C_UTINYINT, out, sizeof(char), NULL)
#define asyncDB_bind_int16(stmt, col, out)   _asyncDB_bind(stmt, col, SQL_C_SSHORT, out, sizeof(short), NULL)
#define asyncDB_bind_uint16(stmt, col, out)  _asyncDB_bind(stmt, col, SQL_C_USHORT, out, sizeof(short), NULL)
#define asyncDB_bind_int32(stmt, col, out)   _asyncDB_bind(stmt, col, SQL_C_SLONG, out, sizeof(int), NULL)
#define asyncDB_bind_uint32(stmt, col, out)  _asyncDB_bind(stmt, col, SQL_C_ULONG, out, sizeof(int), NULL)
#define asyncDB_bind_int64(stmt, col, out)   _asyncDB_bind(stmt, col, SQL_C_SBIGINT, out, sizeof(__int64), NULL)
#define asyncDB_bind_uint64(stmt, col, out)  _asyncDB_bind(stmt, col, SQL_C_UBIGINT, out, sizeof(unsigned __int64), NULL)

void asyncDB_exec(dbSysStmt stmt);
bool asyncDB_fetch(dbSysStmt stmt);





////
//// Api To fire up statements :)
////
typedef void (*onAsyncStmtCompleteProc)(size_t nRows, void *paramPtr, size_t paramInt);

void asyncDB_queueStatement(dbStmtIdentifier stmt,				
							onAsyncStmtCompleteProc	callback,	// NULL is valid, when your statement's data dont need a procesing afterwards (for example: logging)
							void *paramPtr,
							size_t paramInt,
							...);






/**
 * EXAMPLE Code, 'how to use this api'
 */
#if 0
static dbConnectionIdentifier g_dbh_userDB;
static dbStmtIdentifier g_stmt_getUser;


struct userData{
	char name[25];
	int AID;
};


size_t myGetUserStmtProc(dbSysStmt stmt, void *paramPtr, size_t paramInt){
	struct userData *data = (struct userData*)paramPtr;
	size_t szUserName;

	asyncDB_bind_str(stmt, 1, data->name, sizeof(data->name), &szUserName);
	asyncDB_bind_int32(stmt, 2, &data->AID);

	data->AID = 0;	// due to Padding the int32 may be a int64, and upper bits 33-64 are set to random crap, 
					// pushing this integer to the stack may result in strange values, so enforce all bits  set to Zero.

	if( asyncDB_fetch(stmt) ){
		//
		// Got a row :)
		//
		data->name[ szUserName ] = '\0';	// Terminate cstring Correctly 

		return 1;	// one row!

	}else{
		//
		// Got no data :(
		//

		return 0;	// zero rows!
	}

}//end: myGetUserStmtProc()


void userDataLoadedCallback(size_t nRows, void *paramPtr, size_t paramInt){
	struct userData *data = (struct userData*)paramPtr;
	
	if(nRows == 0){
		//
		// :( not found?
		//
		printf(" No Row Found..\n");
	
	}else{
		//
		// Yeah :)
		//

		printf("User Details\n");
		printf(" AID: %u\n", data->AID);
		printf(" Name: %s\n", data->name); 
		//...

	}


}//end: userDataLoadedCallback()


int main(){

	//
	// Establish a Database Connection
	//
	g_dbh_userDB = asyncDB_newConnection("Test", "Driver={SQL Server};Server=192.168.200.10;Database=user;Uid=test;Pwd=pAssWord");
	if(g_dbh_userDB == DB_INVALID_CONNECTION){
		fprintf(stderr, "Cannot Connect to Database - fatal!\n");
		return 1;
	}

	//
	// Prepare the getUser Statement
	//
	g_stmt_getUser = asyncDB_newStatement(dbConn, "SELECT AID, name FROM [user] WHERE [name] = %s;",  myGetUserStmtProc);
	if(g_stmt_getUser == DB_INVALID_STATEMENT){
		printf(stderr, "Cannot Prepare the very important 'getUser' statement - fatal!\n");
		return 1;
	}


	//
	// Allocate Memory to store the result 
	// (this may stored in session...  in non-example-code)
	struct userData *data = (struct userData) malloc(sizeof(struct userData));

	asyncDB_queueStatement(g_stmt_getUser, 
							userDataLoadedCallback,	
							data,	// opaque: paramPtr
							0,		// opaque: paramInt

							// parameter for statement parmeter '%s'
							"Sirius_Black", 
							strlen("Sirius_Black")
							//

							);



	

}//end: main()



//
// As you may have noted by reading the code above.
//  you have full control over 'single-row' or 'mutli-row' results
//	just call asyncDB_fetch() mutliple times,  it returns boolean false when nothing has been fetched.
//
//  the opauqe Parameters 'paramPtr' and 'paramInt'  are passed to 
//	the statement Proc (myGetUserStmtProc)
//	as well as the 'task callback' Proc (userDataLoadedCallback)
//
// The taskCallback (userDataLoadedCallback in our example)  is not required
//	this is useful for statements which wont return any result, 
//	or statements where you're simply not interested in the result
//	for example: Logging to SQL ...
//
//
//

#endif
/** END EXAMPLE CODE **/