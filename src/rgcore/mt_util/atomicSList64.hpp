#pragma once


#if !defined(ARCH_AMD64)	// protect 
#error "This Header can only be included in AMD64 Arch!" 
#endif

namespace rgCore {
namespace atomic{
	//
	// ARCH_AMD64 VERSION
	//

	typedef atomic_align struct SLIST_NODE {
		SLIST_NODE *SLNext;
	} SLIST_NODE;

	typedef atomic64_align struct SLIST_HEAD {
		union {
			struct {
				SLIST_NODE *begin;
				//
				//atomic_t ticket;
				size_t ticket;
			} data;

			atomic128_t asInt; //
		};
	} SLIST_HEAD;


	/**
	 * Initializes the list's head, must be called once
	 * @note there is no finalization proc as there is nothing to clean-up.
	 */
	static __forceinline void SListInitializeHead(volatile SLIST_HEAD *pHead) {

		pHead->data.begin	= NULL;
		pHead->data.ticket	= 42434445;

	}//end: InitializeHead()


	/**
	 * Insert's a new Node in front of the List
	 */
	static __forceinline void SListPush(volatile SLIST_HEAD *pHead, SLIST_NODE *pNode){
		volatile SLIST_HEAD oldHead;
		volatile SLIST_HEAD newHead;
		volatile atomic128_t _tmpOld;

		_beginPush:
		Exchange128(&oldHead.asInt, &pHead->asInt, &_tmpOld); // fetch the old one.

		// new ticket no is old ticket no + 1.
		newHead.data.ticket = (oldHead.data.ticket + 1);
		// begin is the 'to insert' node.
		newHead.data.begin = pNode;
		// 'to insert's node next is oldHead's begin.
		pNode->SLNext = oldHead.data.begin;


		/// -- try to replace the head.
		if(CompareAndSwap128(&pHead->asInt, &newHead.asInt, &oldHead.asInt) != 1) {
			goto _beginPush; // retry.
		}

	}//end: SListPush()


	/**
	 * Insert's a existing list in front of the list
	 */
	static __forceinline void SListPushSList(volatile SLIST_HEAD *pHead, SLIST_NODE *pFirstNode, SLIST_NODE *pLastNode) {
		volatile SLIST_HEAD oldHead;
		volatile SLIST_HEAD newHead;
		volatile atomic128_t _tmpOld;

		_beginPushSList:
		Exchange128(&oldHead.asInt, &pHead->asInt, &_tmpOld); // fetch the old one.

		// new ticket no is old ticket no + 1.
		newHead.data.ticket = (oldHead.data.ticket + 1);
		// new Begin points to the FirstNode
		newHead.data.begin = pFirstNode;
		// lastNode's next to the old List Begin.
		pLastNode->SLNext = oldHead.data.begin;


		/// -- try to replace the head.
		if(CompareAndSwap128(&pHead->asInt, &newHead.asInt, &oldHead.asInt) != 1) {
			goto _beginPushSList; // retry.
		}

	}//end: SListPushSList()


	/** 
	 * Fetches One element, if the list is empty, NULL gets returned.
	 */
	static __forceinline SLIST_NODE *SListPop(volatile SLIST_HEAD *pHead) {
		volatile SLIST_HEAD oldHead;
		volatile SLIST_HEAD newHead;
		volatile atomic128_t _tmpOld;

		_beginPop:
		Exchange128(&oldHead.asInt, &pHead->asInt, &_tmpOld); // fetch the old one.

		if(oldHead.data.begin == NULL) {
			// Empty list.
			return NULL;
		}

		newHead.data.ticket = (oldHead.data.ticket + 1);
		newHead.data.begin = oldHead.data.begin->SLNext;

		/// -- try to replace the head.
		if(CompareAndSwap128(&pHead->asInt, &newHead.asInt, &oldHead.asInt) != 1) {
			goto _beginPop; // retry.
		}

		return oldHead.data.begin;
	}//end: SListPop()


	/** 
	 * Clears the List, the first entry (incase of an empty NULL) gets returned
	 */
	static __forceinline SLIST_NODE *SListFlush(volatile SLIST_HEAD *pHead) {
		volatile SLIST_HEAD oldHead;
		volatile SLIST_HEAD newHead;
		volatile atomic128_t _tmpOld;

		_beginPop:
		Exchange128(&oldHead.asInt, &pHead->asInt, &_tmpOld); // fetch the old one.

		if(oldHead.data.begin == NULL) {
			// Empty list.
			return NULL;
		}

		newHead.data.ticket = (oldHead.data.ticket + 1);
		newHead.data.begin = NULL; // empty list.

		/// -- try to replace the head.
		if(CompareAndSwap128(&pHead->asInt, &newHead.asInt, &oldHead.asInt) != 1) {
			goto _beginPop; // retry.
		}

		return oldHead.data.begin;
	}//end: SListFlush()


}
}
