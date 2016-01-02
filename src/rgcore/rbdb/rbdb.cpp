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

rbdb::rbdb(const char *name, bool threadSafe, int nodeCount, int nodeAllocStep) :
	rbdbBase(name, threadSafe, nodeCount, nodeAllocStep){ 
	//
}//end: constructor()


rbdb::~rbdb(){ 

}//end: destructor()


void *rbdb::insert(RBDB_KEY key, void *data){
	void *retdata;

	if(m_bIsThreadsafe == true){
		RBDB_LOCK_LOCKF(m_lock);
	}

	retdata = db_insert(key, data, rbdbBase::DB_INSERT_FLAG_OVERWRITE);

	if(m_bIsThreadsafe == true){
		RBDB_LOCK_UNLOCKF(m_lock);
	}

	return retdata;
}//end: insert()


void *rbdb::insertNoOverwrite(RBDB_KEY key, void *data) {
	void *retdata;

	if(m_bIsThreadsafe == true) {
		RBDB_LOCK_LOCKF(m_lock);
	}

	retdata = db_insert(key, data, rbdbBase::DB_INSERT_FLAG_NONE);

	if(m_bIsThreadsafe == true) {
		RBDB_LOCK_UNLOCKF(m_lock);
	}

	return retdata;
}//end: insertNoOverwrite()


void *rbdb::fetch(RBDB_KEY key){
	void *data;

	if(m_bIsThreadsafe == true){
		RBDB_LOCK_LOCKREAD(m_lock);
	}

	data = db_fetch(key);

	if(m_bIsThreadsafe == true){
		RBDB_LOCK_UNLOCKREAD(m_lock);
	}

	return data;
}//end: fetch()


void *rbdb::remove(RBDB_KEY key){
	void *data;

	if(m_bIsThreadsafe == true) {
		RBDB_LOCK_LOCKF(m_lock);
	}

	data = db_delete(key);

	if(m_bIsThreadsafe == true) {
		RBDB_LOCK_UNLOCKF(m_lock);
	}

	return data;
}//end: remove()


bool rbdb::foreach(onRBDBForeachProc proc, ...){
	RBDB_DBN *p, *pn, *stack[1024];
	size_t i, sp;  //, count;
	va_list ap;
	
	
	if(m_bIsThreadsafe == true){
		RBDB_LOCK_LOCKF(m_lock);
	}	
	//count = table->NumElements;

	db_free_lock();
	va_start(ap, proc);
	
	for(i = 0; i < RBDB_HTSZ; i++){
		p = m_ht[i];
		if(p == NULL)
			continue;
		
		sp = 0;
		while(1){
			if(p->deleted != true){
				if(proc(p->key, p->data, ap) != false){
					va_end(ap);
					db_free_unlock();
					
					if(m_bIsThreadsafe == true) {
						RBDB_LOCK_UNLOCKF(m_lock);
					}

					return true;
				}
			}
			
			//count--;
			
			if((pn = p->left) != NULL){
				if(p->right != NULL)
					stack[sp++] = p->right;
				
				p = pn;
			}else{
				if(p->right != NULL){
					p = p->right;
				}else{
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

return 0;
}//end: foreach()


void rbdb::onForcedDestroyNode(RBDB_DBN *p){
	// UNUSED, as we have no payload on dataptr or something.
	// rbdb is just a wrapper for rbdbBase.
}//end: onForcedDestroyNode()


} } //end namespaces
