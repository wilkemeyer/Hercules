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

namespace rgCore { namespace rbdb {

/** 
 * Note -> All 'key' length parameters
 * should NOT include the terminating \0 (same result as strlen() would give)
 * you can omit the length, by specifying 0- the implementation will call strlen() by itself.
 */
class strdb : private hashdb {
public:
	/**
	* creates a new Red Black Tree,
	* Collision Safe - Hash based (used for binary data)
	* Aware for storing HUGE key data, each key gets 'duplicated'
	* to hashdb - to ensure its not modified while its referenced in hashdb
	*
	* @param name			name of the pool
	* @param threadSafe		should the pool be thread safe?
	* @param nodeCount		Initial Allocation of nodes, if set to 0 - uses global node pool
	* @param nodeAllocStep	Node Pool reallocation count
	*
	*/
	strdb(const char *name, bool threadSafe = true, int nodeCount = 0, int nodeAllocStep = 0);
	virtual ~strdb();

	//
	/**
	* Inserts a new DataSet to DB
	* This call will overwrite existing entries!
	*
	* @return NULL		- Fatal Error
	* @return *data		- data stored, no collision
	* @return !*data	- existing entry was overwritten, the old dataset has been returned
	*/
	void *insert(const char *key, size_t szKey, void *data);

	/**
	* Inserts a new dataset to DB
	* This call will _NOT_ Overwrite existing entries!
	*
	* @return NULL		- Fatal Error
	* @return *data		- data stored
	* @return !*data	- collision, existing entry was returned - new data NOT inserted!
	*/
	void *insertNoOverwrite(const char *key, size_t szKey, void *data);

	/**
	* Fetches a dataset from DB
	*
	* @return NULL		- not found
	* @return !NULL		- requested dataset
	*/
	void *fetch(const char *key, size_t szKey);

	/**
	* Deletes a dataset from DB
	*
	* @return NULL		- key does not exist, nothing deleted
	* @return !NULL		- pointer to removed dataset.
	*/
	void *remove(const char *key, size_t szKey);


	/**
	* Callback returns true to stop the iteration
	*
	* @note - IMPORTANT - 
	*		the 'szKey' parameter does not include \0 !
	*
	*/
	typedef bool(*onSTRDBForeachProc)(const char *key, size_t szKey, void *data, va_list ap);

	/**
	* Iterates over the DB
	*
	* @note you're allowed to delete/insert/modify the DB
	* @note but keep in mind, its less efficient and shouldnt be done for mas operations
	*
	* @note if the callback-proc 'proc' returns true - the iteration will 'stop'
	*
	* @return return value of callback
	*/
	bool foreach(onSTRDBForeachProc proc, ...);

protected:
	//void onForcedDestroyNode(RBDB_DBN *p);
};

} }//end namespaces
