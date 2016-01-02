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


// Implementation Global's:
Pool<hashdb::HASHDB_ENTRY> *hashdb::m_hashdbEntryPool = NULL;


void hashdb::globalInit(){
	size_t allocStep, initialAlloc;
	bool bUseLargePages = false;

	// @TODO - core config.
	allocStep = 24;
	initialAlloc = 32;

	m_hashdbEntryPool = new Pool<hashdb::HASHDB_ENTRY>("HASHDB/Global Entry", allocStep, initialAlloc, bUseLargePages);

}//end: globalInit()


void hashdb::globalFinal(){

	if(m_hashdbEntryPool != NULL){
		delete m_hashdbEntryPool;
		m_hashdbEntryPool = NULL;
	}
	
}//end: globalFinal()


//
hashdb::hashdb(const char *name, bool threadSafe, int nodeCount, int nodeAllocStep) : 
 rbdbBase(name, threadSafe, nodeCount, nodeAllocStep) {

}//end: constructor()


hashdb::~hashdb(){
	RBDB_DBN *p, *pn, *stack[1024];
	register size_t i, sp;



	// Cleanup logic.
	db_free_lock();

	for(i = 0; i < RBDB_HTSZ; i++) {
		p = m_ht[i];
		if(p == NULL)
			continue;

		sp = 0;

		while(1) {
			if((pn = p->left) != NULL) {
				if(p->right)
					stack[sp++] = p->right;
			} else {
				if(p->right)
					pn = p->right;
				else {
					if(sp == 0)
						break;

					pn = stack[--sp];
				}
			}

			// Callback for proper cleanup 
			this->onForcedDestroyNode(p);

			m_nodePool->put(p);

			p = pn;
		}//while(1)

		if(p != NULL) {
			// Callback for proper cleanup 
			this->onForcedDestroyNode(p);

			m_nodePool->put(p);

		}
	}//for each item on heap 

	db_free_unlock();

	// [FWI] - maybe we should also check the freeList .. 
	//

}//end: destructor


void hashdb::onForcedDestroyNode(RBDB_DBN *p){
	HASHDB_ENTRY *it, *itn;
	
	it = (HASHDB_ENTRY*)p->data;
	while(1){
		if(it == NULL)
			break;
		
		itn = it->next;

		freeHashDBEntry(it);

		it = itn;
	}

}//end: onForcedDestroyNode()


RBDB_KEY hashdb::calculateHash(const void *data, size_t szData) {
#if defined(ARCH_AMD64)
	static RBDB_KEY randbox[] ={
		0x49848f1bf012f6ae, 0xe6255dbae9089ab6, 0x36da5bdcfead7bb3, 0x47bf94e969232f74,
		0x8cbcce229ee5016c, 0x559fc06aec4c304d, 0xd268f5361d2917b5, 0xe10af79ac1af4d69,
		0xc1af4d69e10af79a, 0x1d2917b5d268f536, 0xec4c304d559fc06a, 0x9ee5016c8cbcce22,
		0x69232f7447bf94e9, 0xfead7bb336da5bdc, 0xe9089ab6e6255dba, 0xf012f6ae49848f1b,
	};
#else
	static RBDB_KEY randbox[] ={
		0x49848f1b, 0xe6255dba, 0x36da5bdc, 0x47bf94e9,
		0x8cbcce22, 0x559fc06a, 0xd268f536, 0xe10af79a,
		0xc1af4d69, 0x1d2917b5, 0xec4c304d, 0x9ee5016c,
		0x69232f74, 0xfead7bb3, 0xe9089ab6, 0xf012f6ae,
	};
#endif	
	register RBDB_KEY acc = 0;

	for(size_t i = 0; i < szData; i++){
		unsigned char key = ((unsigned char*)data)[i];
		acc ^= randbox[(key + acc) & 0xf];
		acc = (acc << 1) | (acc >> 31);
		acc &= 0xffffffffffffffff;
		acc ^= randbox[((key++ >> 4) + acc) & 0xf];
		acc = (acc << 2) | (acc >> 30);
		acc &= 0xffffffffffffffff;
	}
	return acc;
}//end: calculateHash()


hashdb::HASHDB_ENTRY *hashdb::allocateHashDBEntry(size_t szKey) {
	HASHDB_ENTRY *p;

	if(szKey <= HASHDB_ENTRY_KEYPAYLOAD){
		// Fits into pooled entry's

		p = m_hashdbEntryPool->get();
		p->szKey = szKey;

	
	}else{
		// needs custom allocation :(
		p = (HASHDB_ENTRY*)roalloc( (sizeof(HASHDB_ENTRY) + (szKey - HASHDB_ENTRY_KEYPAYLOAD)) );
		p->szKey = szKey;

	}

	return p;
}//end: allocateHashDBEntry()


void hashdb::freeHashDBEntry(HASHDB_ENTRY *he){

	if(he->szKey <= HASHDB_ENTRY_KEYPAYLOAD){
		// from pool
		m_hashdbEntryPool->put(he);

	}else{
		// Custom allocated
		rofree(he);

	}

}//end: freeHashDBEntry()



// this one dont overwrites; 
// please note that insert2 is for 99% the same code, so pelase fix there bugs also if you'll find one here!
void *hashdb::insertNoOverwrite(const void *key, size_t szKey, void *data){
	RBDB_KEY hash;
	PHASHDB_ENTRY p, piter;
	void *ret;

	ret = NULL;
	hash = calculateHash(key, szKey);


	if(m_bIsThreadsafe == true){
		RBDB_LOCK_LOCKF(m_lock);
	}
	
	p = (PHASHDB_ENTRY)db_fetch(hash);
	if(p != NULL) {

		// okay found, know check for hash collision => otherwise return; because we're not overwriting!
		for(piter = p; piter != NULL; piter = piter->next) {
			if(szKey == piter->szKey && memcmp(key, piter->key, szKey) == 0) {
				ret = piter->data;
				//break;
			}
		}

		if(ret == NULL) {
			// if its null, we have'nt a match, 
			// so this is a true hash collision, justa add a new node and be happy with it ..
			piter = allocateHashDBEntry(szKey);
			piter->data = data;
			piter->hash = hash;
			//piter->szKey // Initialzied by allocate function
			memcpy(piter->key, key, szKey);

			// p <-> [piter] <-> p->next

			if(p->next != NULL)
				p->next->prev = piter; 	// [piter] <- p->next

			piter->next = p->next; 	// [piter] -> p->next
			piter->prev =  p; 		// p <- [piter]
			p->next = piter;		// p -> [piter]


			ret = data;
		}

	} else {
		// not in table.
		piter = allocateHashDBEntry(szKey);

		piter->data = data;
		piter->hash = hash;
		//piter->szKey // initialize by allocate function
		memcpy(piter->key, key, szKey);

		piter->next = NULL;
		piter->prev = NULL;

		db_insert(hash, piter, DB_INSERT_FLAG_OVERWRITE); // overwrite is uneccessary; but safe is safe - should neverh appen (Except threading race condition where syncronization was defect.)

		ret = data;
	}


	if(m_bIsThreadsafe == true){
		RBDB_LOCK_UNLOCKF(m_lock);
	}

	return ret;
}//end: insertNoOverwrite()


// This one overwrites,
// note is for 99% the same code as above (insertNoOverwrite) please fix there also bugs if you'Ll find one!
void *hashdb::insert(const void *key, size_t szKey, void *data) {
	RBDB_KEY hash;
	PHASHDB_ENTRY p, piter;
	void *ret;

	ret = NULL;
	hash = calculateHash(key, szKey);

	if(m_bIsThreadsafe == true){
		RBDB_LOCK_LOCKF(m_lock);
	}

	p = (PHASHDB_ENTRY)db_fetch(hash);
	if(p != NULL) {

		// okay found, know check for hash collision => otherwise return; because we're not overwriting!
		for(piter = p; piter != NULL; piter = piter->next) {
			if(szKey == piter->szKey && memcmp(key, piter->key, szKey) == 0) {
				// We're overwriting! 
				ret = piter->data;

				piter->data = data; // Overwrite.
				break;
			}
		}

		if(ret == NULL) {
			// if its null, we have'nt a match, 
			// so this is a true hash collision, justa add a new node and be happy with it ..
			piter = allocateHashDBEntry(szKey);
			piter->data = data;
			piter->hash = hash;
			//piter->szKey // set by allocate function
			memcpy(piter->key, key, szKey);
			
			// p <-> [piter] <-> p->next

			if(p->next != NULL)
				p->next->prev = piter; 	// [piter] <- p->next

			piter->next = p->next; 	// [piter] -> p->next
			piter->prev =  p; 		// p <- [piter]
			p->next = piter;		// p -> [piter]


			ret = data;
		}

	} else {
		// not in table.
		piter = allocateHashDBEntry(szKey);

		piter->data = data;
		piter->hash = hash;
		//piter->szKey // set by allocate() function
		memcpy(piter->key, key, szKey);

		piter->next = NULL;
		piter->prev = NULL;

		db_insert(hash, piter, DB_INSERT_FLAG_OVERWRITE); // overwrite is uneccessary; but safe is safe - should neverh appen (Except threading race condition where syncronization was defect.)

		ret = data;
	}


	if(m_bIsThreadsafe == true){
		RBDB_LOCK_UNLOCKF(m_lock);
	}

	return ret;
}//end: insert()


void *hashdb::fetch(const void *key, size_t szKey){
	PHASHDB_ENTRY p;
	void *ret;
	RBDB_KEY hash;

	hash = calculateHash(key, szKey);
	ret = NULL;


	if(m_bIsThreadsafe == true){
		RBDB_LOCK_LOCKREAD(m_lock);
	}

	p = (PHASHDB_ENTRY)db_fetch(hash);
	if(p != NULL) {

		if(p->next == NULL) {
			ret = p->data;
		} else {

			// ow .. collision x.x
			for(; p != NULL; p = p->next) {
				if(szKey == p->szKey && memcmp(p->key, key, szKey) == 0) {
					ret = p->data;
					break; // found!
				}
			}// foreach over all subnodes in elem.

			if(ret == NULL) // should not happen!
				m_fatalerror("hashdb::fetch -> hash found, multiple subnodes in tree for this hash; but key was not found ! - ??\n");

		}// 
	}

	if(m_bIsThreadsafe == true){
		RBDB_LOCK_UNLOCKREAD(m_lock);
	}

	return ret;
}//end: fetch()


void *hashdb::remove(const void *key, size_t szKey){
	PHASHDB_ENTRY p;
	void *ret;
	RBDB_KEY hash;

	hash = calculateHash(key, szKey);
	ret = NULL;


	if(m_bIsThreadsafe == true){
		RBDB_LOCK_LOCKF(m_lock);
	}

	p = (PHASHDB_ENTRY)db_fetch(hash);
	if(p != NULL) {
		if(p->next == NULL) {
			if(p != db_delete(hash)) {
				m_fatalerror("strdb_delete: 1st way (no coll) - db_delete returned different than db_fetch\n");
				ret = NULL;
			} else {

				ret = p->data;

				freeHashDBEntry(p);
			}

		} else {
			// Collision; mutliple results on hash
			// 
			for(; p != NULL; p = p->next) {

				if(szKey == p->szKey && memcmp(p->key, key, szKey) == 0) {
					// Unlink!

					if(p->prev == NULL) {
						// First elem in elem ;  we have to reinsert the next ndoe as antry to tree.
						p->next->prev = NULL; // unlink!
						db_insert(hash, p->next, DB_INSERT_FLAG_OVERWRITE);

					} else {
						// okay; otherwise we have to unlink ourselves..
						p->prev->next = p->next;
						if(p->next != NULL)
							p->next->prev = p->prev;

					}

					ret = p->data;

					freeHashDBEntry(p);

					break;
				}

			}//foreach over all subnodes	

		}// if p->next == NULL (is the only one?)


	} // exists


	if(m_bIsThreadsafe == true){
		RBDB_LOCK_UNLOCKF(m_lock);
	}

	return ret;
}//end: remove()


bool hashdb::foreach(onHASHDBForeachProc proc, ...){
	PHASHDB_ENTRY strdb_p, strdb_p_next;
	RBDB_DBN *p, *pn, *stack[1024];
	size_t i, sp;  //, count;
	va_list ap;


	if(m_bIsThreadsafe == true){
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

					if(proc(strdb_p->key, strdb_p->szKey, strdb_p->data, ap) != false) {
						va_end(ap);
						db_free_unlock();

						if(m_bIsThreadsafe == true){
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


} } // end namespaces()
