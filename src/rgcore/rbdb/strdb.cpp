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
#include "stdafx.h" // VS Bug.
#include "../stdafx.h"

namespace rgCore { namespace rbdb { 

strdb::strdb(const char *name, bool threadSafe, int nodeCount, int nodeAllocStep) :
	hashdb(name, threadSafe, nodeCount, nodeAllocStep) {
	/////
}//end: constructor()'

strdb::~strdb() {
}//end: destructor()


void *strdb::insert(const char *key, size_t szKey, void *data){

	if(szKey == 0){
		szKey = strlen(key);
	}

	return hashdb::insert(key, (szKey + 1), data);

}//end: insert()


void *strdb::insertNoOverwrite(const char *key, size_t szKey, void *data) {

	if(szKey == 0) {
		szKey = strlen(key);
	}

	return hashdb::insertNoOverwrite(key, (szKey + 1), data);

}//end: insertNoOverwrite()


void *strdb::fetch(const char *key, size_t szKey){
	
	if(szKey == 0) {
		szKey = strlen(key);
	}

	return hashdb::fetch(key, (szKey + 1) );

}//end: fetch()


void *strdb::remove(const char *key, size_t szKey){
	
	if(szKey == 0) {
		szKey = strlen(key);
	}

	return hashdb::remove(key, (szKey + 1));

}//end: remove()


bool strdb::foreach(onSTRDBForeachProc proc, ...){
	PHASHDB_ENTRY strdb_p, strdb_p_next;
	RBDB_DBN *p, *pn, *stack[1024];
	size_t i, sp;  //, count;
	va_list ap;


	if(m_bIsThreadsafe == true) {
		RBDB_LOCK_LOCKF(m_lock);
	}
	//count = table->NumElements;

	db_free_lock();
	va_start(ap, proc);

	for(i = 0; i < RBDB_HTSZ; i++) {
		p = m_ht[i];
		if(p == NULL)
			continue;

		sp = 0;
		while(1) {
			if(p->deleted != true) {

				// STRDB!
				strdb_p = (PHASHDB_ENTRY)p->data;
				while(1) {
					if(strdb_p == NULL)
						break;

					// store next,  this value could be deleted using strdb_delete.
					strdb_p_next = strdb_p->next;

					if(proc(strdb_p->key, (strdb_p->szKey - 1), strdb_p->data, ap) != false) {	// szKey - 1 = Don't report \0 to foreachproc
						va_end(ap);
						db_free_unlock();

						if(m_bIsThreadsafe == true) {
							RBDB_LOCK_UNLOCKF(m_lock);
						}

						return true;
					}

					strdb_p = strdb_p_next;
				}


			}

			//count--;

			if((pn = p->left) != NULL) {
				if(p->right != NULL)
					stack[sp++] = p->right;

				p = pn;
			} else {
				if(p->right != NULL) {
					p = p->right;
				} else {
					if(sp == 0)
						break;

					p = stack[--sp];
				}
			}

		}//while(1)

	}//end each heap entry	


	va_end(ap);
	db_free_unlock();

	if(m_bIsThreadsafe == true) {
		RBDB_LOCK_UNLOCKF(m_lock);
	}



	//	if(count != 0)
	//		table->fatalerrFunc("rbdb_foreach: Table %s is inconsistent (Left: %u)", table->Name, count);

	return false;

}//end: foreach()




} } //end namespaces