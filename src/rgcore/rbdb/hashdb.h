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

class hashdb : protected rbdbBase {
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
	hashdb(const char *name, bool threadSafe = true, int nodeCount = 0, int nodeAllocStep = 0);
	virtual ~hashdb();

	//
	/**
	* Inserts a new DataSet to DB
	* This call will overwrite existing entries!
	*
	* @return NULL		- Fatal Error
	* @return *data		- data stored, no collision
	* @return !*data	- existing entry was overwritten, the old dataset has been returned
	*/
	void *insert(const void *key, size_t szKey, void *data);

	/**
	* Inserts a new dataset to DB
	* This call will _NOT_ Overwrite existing entries!
	*
	* @return NULL		- Fatal Error
	* @return *data		- data stored
	* @return !*data	- collision, existing entry was returned - new data NOT inserted!
	*/
	void *insertNoOverwrite(const void *key, size_t szKey, void *data);

	/**
	* Fetches a dataset from DB
	*
	* @return NULL		- not found
	* @return !NULL		- requested dataset
	*/
	void *fetch(const void *key, size_t szKey);

	/**
	* Deletes a dataset from DB
	*
	* @return NULL		- key does not exist, nothing deleted
	* @return !NULL		- pointer to removed dataset.
	*/
	void *remove(const void *key, size_t szKey);


	/**
	* Callback returns true to stop the iteration
	*/
	typedef bool(*onHASHDBForeachProc)(const void *key, size_t szKey, void *data, va_list ap);

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
	bool foreach(onHASHDBForeachProc proc, ...);


protected:
	virtual void onForcedDestroyNode(RBDB_DBN *p);

private:
	/** 
	 * Hash Function
	 * 
	 */ 
	virtual RBDB_KEY calculateHash(const void *data, size_t szData);


private:
	// Initialization of global stuff, called by rbdbBase
	friend class rgCore::rbdb::rbdbBase;

	static void globalInit();
	static void globalFinal();

protected:
	// Just some hints to the following define
	// Its the maximum key-length the pooled hashdb entrys will
	// held, if a key exceeds the given length - the hashdb_entry 
	// will be allocated using roalloc/rofree()
	#define HASHDB_ENTRY_KEYPAYLOAD 48

	// 
	struct HASHDB_ENTRY {
		struct HASHDB_ENTRY *next, *prev;
		void *data; // user pointer

		RBDB_KEY hash;

		size_t szKey;	// This value is set thru allocateHashDBEntry() Proc, NEVER touch it! 
		char key[HASHDB_ENTRY_KEYPAYLOAD];	// _must_ be the last member.
	};
	typedef struct HASHDB_ENTRY HASHDB_ENTRY;
	typedef HASHDB_ENTRY* PHASHDB_ENTRY;


private:
	static Pool<HASHDB_ENTRY> *m_hashdbEntryPool;
	



private:
	HASHDB_ENTRY *allocateHashDBEntry(size_t szKey);
	void freeHashDBEntry(HASHDB_ENTRY *he);

};


} }//end namespaces
