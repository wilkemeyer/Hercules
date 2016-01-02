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
Pool<RBDB_DBN> *rbdbBase::m_globalNodePool = NULL;
Pool<RBDB_DBF> *rbdbBase::m_globalFreePool = NULL;
static bool g_bUseLargePagesForPool = false;

//
// Global initialization/finalization
// housekeeping for global/shared ressources 
// 
void rbdbBase::globalInit(){
	size_t n_dbnInitial, n_dbnStep;
	size_t n_dbfInitial, n_dbfStep;
	bool bLarge = g_bUseLargePagesForPool;

	n_dbnInitial		= 2048;
	n_dbnStep		= 1024;

	n_dbfInitial		= 128;
	n_dbfStep		= 64;

	m_globalNodePool = new Pool<RBDB_DBN>("RBDB/Global DBN", n_dbnStep, n_dbnInitial, bLarge, 
											[](RBDB_DBN *n){
												//memset(n, 0x00, sizeof(RBDB_DBN));	// @paranioa  / TODO cleanup implementaiton so this is not neccessary!
											});

	m_globalFreePool = new Pool<RBDB_DBF>("RBDB/Global DBF", n_dbfStep, n_dbfInitial, bLarge,
											[](RBDB_DBF *n){
												//memset(n, 0x00, sizeof(RBDB_DBN));	// @paranioa  / TODO cleanup implementaiton so this is not neccessary!
											});

	// Sub-Implementation specific global stuff
	hashdb::globalInit();


}//end: globalInit()


void rbdbBase::globalFinal(){

	// Finalize Sub-Implementation specific global stuff
	hashdb::globalFinal();
	
	if(m_globalNodePool != NULL){
		delete m_globalNodePool;
		m_globalNodePool = NULL;
	}

	if(m_globalFreePool != NULL){
		delete m_globalFreePool;
		m_globalFreePool = NULL;
	}

}//end: globalFinal()


void bldinOnFatalError(const char *msg, ...){
	char msgbuf[4096];
	va_list ap;

	va_start(ap, msg);
	vsnprintf_s(msgbuf, sizeof(msgbuf), msg, ap);
	va_end(ap);

	fatalError("RBDB Fatal Error!\nMsg: %s\n", msgbuf);
	return; ///

}//end: bldinOnFatalError


//
// Implementation Beign
//

rbdbBase::rbdbBase(const char *name, bool threadSafe, int nodeCount, int nodeAllocStep){
	if(name == NULL || *name == '\0')
		name = "Unknown";

	for(size_t i = 0; i < RBDB_HTSZ; i++)
		m_ht[i] = NULL;

	m_freeList			= NULL;
	m_nNodes			= 0;
	m_freeLockCount	= 0;
	m_bIsThreadsafe	= threadSafe;

	if(m_bIsThreadsafe == true){
		RBDB_LOCK_INITF(m_lock);
	}

	m_fatalerror		= bldinOnFatalError;
	m_name			= rostrdup(name);
	
	
	if(nodeCount == 0){
		// No own Node Pool - using global Pool!
		m_nodePool	= rbdbBase::m_globalNodePool;

	}else{
		// Settings for own Node Pool Specified
		char tmpName[128];
		sprintf_s(tmpName, "RBDB/%s DBN", m_name);
		m_nodePool	= new Pool<RBDB_DBN>(tmpName, nodeAllocStep, nodeCount, g_bUseLargePagesForPool, 
										 [](RBDB_DBN *n){
											//memset(n, 0x00, sizeof(RBDB_DBN)); // @TODO / cleanup 
										});

	}


}//end: main constructor()


rbdbBase::~rbdbBase(){
	RBDB_DBN *p, *pn, *stack[1024];
	register size_t i, sp;

	if(m_nNodes > 0){
		putErr("RBDB(%s) -> Contains Nodes during Destruction!\n", m_name);
	}

	/// Cleanup 

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

	//
	rofree(m_name);

	if(m_bIsThreadsafe == true){
		RBDB_LOCK_DESTROY(m_lock);
	}

	if(m_nodePool != rbdbBase::m_globalNodePool){
		// has own node pool - destroy!
		delete m_nodePool;
		m_nodePool = NULL;
	}


}//end: main destructor()


void rbdbBase::onForcedDestroyNode(RBDB_DBN *p){
	// UNUSED
}//end: onForcedDestroyNode()


void rbdbBase::db_rebalance_erase(RBDB_DBN *z, RBDB_DBN **root) {
	RBDB_DBN *y, *x, *x_parent, *w;
	RBDB_DBN_COLOR tmp;

	y = z;
	x = NULL;
	x_parent = NULL;

	if(y->left == NULL)
		x = y->right;
	else if(y->right == NULL)
		x = y->left;
	else {
		y = y->right;
		while(y->left != NULL)
			y = y->left;

		x = y->right;
	}

	if(y != z) {
		z->left->parent = y;
		y->left = z->left;
		if(y != z->right) {
			x_parent = y->parent;
			if(x)
				x->parent = y->parent;
			y->parent->left = x;
			y->right = z->right;
			z->right->parent = y;
		} else {
			x_parent = y;
		}

		if(*root == z)
			*root = y;
		else if(z->parent->left == z)
			z->parent->left = y;
		else
			z->parent->right = y;

		y->parent = z->parent;

		{
			tmp = y->color;
			y->color = z->color;
			z->color = tmp;
		}

		y = z;

	} else { // y != z if - !
		x_parent = y->parent;
		if(x)
			x->parent = y->parent;

		if(*root == z)
			*root = x;
		else if(z->parent->left == z)
			z->parent->left = x;
		else
			z->parent->right = x;
	}

	if(y->color != DBN_RED) {
		while(x != *root && (x == NULL || x->color == DBN_BLACK)) {
			if(x == x_parent->left) {
				w = x_parent->right;
				if(w->color == DBN_RED) {
					w->color = DBN_BLACK;
					x_parent->color = DBN_RED;
					db_rotate_left(x_parent, root);
					w = x_parent->right;
				}
				if((w->left == NULL || w->left->color == DBN_BLACK) && (w->right == NULL || w->right->color == DBN_BLACK)) {
					w->color = DBN_RED;
					x = x_parent;
					x_parent = x_parent->parent;
				} else {
					if(w->right == NULL || w->right->color == DBN_BLACK) {
						if(w->left)
							w->left->color = DBN_BLACK;

						w->color = DBN_RED;
						db_rotate_right(w, root);
						w = x_parent->right;
					}

					w->color = x_parent->color;
					x_parent->color = DBN_BLACK;
					if(w->right)
						w->right->color = DBN_BLACK;

					db_rotate_left(x_parent, root);

					break;
				}
			} else {
				w = x_parent->left;
				if(w->color == DBN_RED) {
					w->color = DBN_BLACK;
					x_parent->color = DBN_RED;
					db_rotate_right(x_parent, root);
					w = x_parent->left;
				}

				if((w->right == NULL || w->right->color == DBN_BLACK) && (w->left == NULL || w->left->color == DBN_BLACK)) {
					w->color = DBN_RED;
					x = x_parent;
					x_parent = x_parent->parent;
				} else {
					if(w->left == NULL || w->left->color == DBN_BLACK) {
						if(w->right)
							w->right->color = DBN_BLACK;
						w->color = DBN_RED;
						db_rotate_left(w, root);
						w = x_parent->left;
					}

					w->color = x_parent->color;
					x_parent->color = DBN_BLACK;
					if(w->left)
						w->left->color = DBN_BLACK;
					db_rotate_right(x_parent, root);
					break;
				}
			}//if(x == x_parent->left)
		}//while for x != *root.....

		if(x)
			x->color = DBN_BLACK;

	}//y->color != DBN_RED  

}//end: db_rebalance_erase


void rbdbBase::db_rebalance(RBDB_DBN *p, RBDB_DBN **root) {
	RBDB_DBN *y;

	p->color = DBN_RED;

	while(p != *root && p->parent->color == DBN_RED) {
		if(p->parent == p->parent->parent->left) {
			y = p->parent->parent->right;
			if(y && y->color == DBN_RED) {
				p->parent->color = DBN_BLACK;
				y->color = DBN_BLACK;
				p->parent->parent->color = DBN_RED;
				p = p->parent->parent;
			} else {
				if(p == p->parent->right) {
					p = p->parent;
					db_rotate_left(p, root);
				}
				p->parent->color = DBN_BLACK;
				p->parent->parent->color = DBN_RED;
				db_rotate_left(p->parent->parent, root);
			}
		}
	}//while

	(*root)->color = DBN_BLACK;

}//end: db_rebalance


void rbdbBase::db_rotate_left(RBDB_DBN *p, RBDB_DBN **root) {
	RBDB_DBN *y;

	y = p->right;
	p->right = y->left;
	if(y->left != NULL)
		y->left->parent = p;
	y->parent = p->parent;

	if(p == *root)
		*root = y;
	else if(p == p->parent->left)
		p->parent->left = y;
	else
		p->parent->right = y;

	y->left = p;
	p->parent = y;

}//end: db_rotate_left


void rbdbBase::db_rotate_right(RBDB_DBN *p, RBDB_DBN **root) {
	RBDB_DBN *y;

	y = p->left;
	p->left = y->right;
	if(y->right != NULL)
		y->right->parent = p;
	y->parent = p->parent;

	if(p == *root)
		*root = y;
	else if(p == p->parent->right)
		p->parent->right = y;
	else
		p->parent->left = y;
	y->right = p;
	p->parent = y;

}//end: db_rotate_right


void rbdbBase::db_free_lock() {
	atomic::Inc(&m_freeLockCount);
}//end: db_free_lock


void rbdbBase::db_free_unlock() {
	RBDB_DBF *f, *fn;

	if(m_freeLockCount > 0)
		atomic::Dec(&m_freeLockCount);


	if(m_freeLockCount == 0) {
		f = m_freeList;

		while(f != NULL) {
			fn = f->next;

			db_rebalance_erase(f->z, f->root);
			
			m_nodePool->put(f->z);
			m_globalFreePool->put(f);
			
			atomic::Dec(&m_nNodes);

			f = fn;
		}

		m_freeList = NULL;

	}// if free_lock == 0 (release locked deleted entrys)

}//end: db_free_unlock


void *rbdbBase::db_insert(RBDB_KEY key, void *data, unsigned int flags) {
	RBDB_DBN *p, *priv;
	RBDB_DBF *f, *fp;
	register RBDB_KEY hash_tmp, c;
	void *olddata;

	hash_tmp = key % RBDB_HTSZ;

	for(c = 0, priv = NULL, p = m_ht[hash_tmp]; p;) {
		// -----
		if((key ^ p->key) & RBDB_KEY_SIGNSWAP) {
			if(key < 0)
				c = -1;
			else
				c = 1;
		} else
			c = key - p->key;
		// -----

		if(c == 0) {
			// The 'to insert key' is already known in table
			// this shouldnt happen here; 
			// if the node has been deleted and its on the deleted list:
			// 	delete it from deleted list; 
			//	reactivate the node ..
			// 
			if(p->deleted == true) {

				f = m_freeList;
				fp = NULL; // previous ptr; to fix the next ref.
				while(f != NULL) {
					if(f->z == p) {
						if(fp == NULL) { // begin of freelist
							m_freeList = f->next;
						} else {
							fp->next = f->next; // remove from list
						}

						m_globalFreePool->put(f);
						break;	// 2013-10-17 - [FWI] added break - as the element has been found to be removed from free list. 
					}

					fp = f;
					f = f->next;
				}//end while f! = NULL 

				// Fix the node; 
				// set it to not deleted and change data ptr
				p->deleted = false;
				p->data = data;
				return data; // done insert

			} else {
				// Handle by flags on collision
				if(flags & DB_INSERT_FLAG_OVERWRITE) {
					olddata = p->data;
					p->data = data;
					return olddata;
				}

				return p->data;
			}

		} // c == 0  means, key found in tree. 

		priv = p;
		if(c < 0)
			p = p->left;
		else
			p = p->right;

	} // for each ht entry	


	// Insert =>
	p = m_nodePool->get();
	p->parent = NULL;
	p->left = NULL;
	p->right = NULL;

	p->key = key;
	p->data = data;

	//	p->color = RBDB_DBN_RED;		// FWI: commented out, as all code-paths initialize the property.
	p->deleted = false;

	if(c == 0) {
		m_ht[hash_tmp] = p;
		p->color = DBN_BLACK;
	} else {
		p->color = DBN_RED;

		if(c < 0) {
			priv->left = p;
			p->parent = priv;
		} else {
			priv->right = p;
			p->parent = priv;
		}

		//..
		if(priv->color == DBN_RED) {
			db_rebalance(p, &m_ht[hash_tmp]);
		}

	}

	atomic::Inc(&m_nNodes);

	return data;
}//end: db_insert


void *rbdbBase::db_delete(RBDB_KEY key) {
	RBDB_DBN *p;
	RBDB_DBF *f;
	register RBDB_KEY hash_tmp, c;
	void *data;

	hash_tmp = key % RBDB_HTSZ;
	for(p = m_ht[hash_tmp]; p;) {
		// ----
		if((key ^ p->key) & RBDB_KEY_SIGNSWAP) {
			if(key < 0)
				c = -1;
			else
				c = 1;
		} else
			c = key - p->key;
		// ----

		if(c == 0)
			break;
		else if(c < 0)
			p = p->left;
		else
			p = p->right;
	}

	if(p == NULL)
		return NULL;

	data = p->data;

	// locked?
	if(m_freeLockCount > 0) {
		// queue:

		f = m_globalFreePool->get();

		f->z = p;
		f->root = &m_ht[hash_tmp];

		f->next = m_freeList;
		m_freeList = f;

		p->deleted = true;
		p->data = NULL; // dereference

	} else {
		db_rebalance_erase(p, &m_ht[hash_tmp]);

		m_nodePool->put(p);

		atomic::Dec(&m_nNodes);
	}

	return data;
}//end: db_delete


void *rbdbBase::db_fetch(RBDB_KEY key) {
	RBDB_DBN *p;
	register RBDB_KEY hash_tmp, c;

	hash_tmp = key % RBDB_HTSZ;
	for(p = m_ht[hash_tmp]; p;) {
		// ----
		if((key ^ p->key) & RBDB_KEY_SIGNSWAP) {
			if(key < 0)
				c = -1;
			else
				c = 1;
		} else
			c = key - p->key;

		if(c == 0)
			return p->data;
		else if(c < 0)
			p = p->left;
		else
			p = p->right;

	}

	return NULL;
}//end: db_fetch




} } //end namespaces
