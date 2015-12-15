#pragma once
#if defined(ARCH_AMD64)
#define ATOMIC_ALIGNMENT 8
#define ATOMIC64_ALIGNMENT 16
#elif defined(ARCH_X86)
#define ATOMIC_ALIGNMENT 4
#define ATOMIC64_ALIGNMENT 8
#else
#error "Unsupported Archetecture"
#endif
/** Arch-Wordsize Alignment **/
#define atomic_align __declspec(align(ATOMIC_ALIGNMENT))
/** Double Arch-Wordsize Alignment **/
#define atomic64_align __declspec(align(ATOMIC64_ALIGNMENT)) 

namespace rgCore {
	namespace atomic {

		typedef volatile atomic_align LONG atomic_t;
		typedef volatile atomic64_align __int64 atomic64_t;
		typedef volatile atomic_align void* atomic_ptr;
		
#ifdef ARCH_AMD64
		typedef volatile struct {
			atomic64_align LONG64  v[2];
		} atomic128_t;
#endif
		/**
		 * Compares and Swaps, returns the initial/old value of *dest.
		 */
		static __forceinline atomic_t CompareAndSwap(volatile atomic_t *dest, atomic_t exchange, atomic_t comparand) {
			return _InterlockedCompareExchange(dest, exchange, comparand);
		}//end: CAS

		/**
		 * Compares and Swaps, returns the initial/old value of *dest, 64BIT Operation
		 */
		static __forceinline atomic64_t CompareAndSwap64(volatile atomic64_t *dest, __int64 exchange, __int64 comparand) {
			return _InterlockedCompareExchange64(dest, exchange, comparand);
		}//end: CAS64

#ifdef ARCH_AMD64
		/** 
		 * Compares and Swaps 128 Bit Values
		 * 
		 * @param *dest				-> ptr to destination, considered as two 64bit ints
		 * @param exchangeHigh		-> high part of exchange value
		 * @param exchangeLow		-> low part of exchange value
		 * @Param comparandResult	-> value to compare to, considered as two 64bit ints
		 * 
		 * @return the result, 0 if *comparedResult is not equal to *dest,  1 if equal to orig. value
		 *
		 * @note The function compares the Destination value with the ComparandResult value:
		 * @note	If the Destination value is equal to the ComparandResult value, the ExchangeHigh and ExchangeLow values 
		 *			are stored in the array specified by Destination, and also in the array specified by ComparandResult.
		 * @note	Otherwise, the ExchangeHigh and ExchangeLow values are only stored in the array specified by ComparandResult.
		 */
		static __forceinline unsigned char CompareAndSwap128(volatile atomic128_t *dest, atomic128_t *exchange, atomic128_t *comparandResult) {
			return _InterlockedCompareExchange128(dest->v, exchange->v[1], exchange->v[0], (LONG64*) comparandResult->v);
		}
#endif

		/**
		 * Compares and Swaps, returns the initial/old value of *dest
		 */
		static __forceinline atomic_ptr CompareAndSwapPtr(volatile atomic_ptr *dest, atomic_ptr exchange, atomic_ptr comparand) {
			//return InterlockedCompareExchangePointer(dest, exchange, comparand);
#ifdef ARCH_AMD64
			return (atomic_ptr)_InterlockedCompareExchange64((LONGLONG volatile *)dest, (LONGLONG)exchange, (LONGLONG)comparand);
#else
			return (atomic_ptr)_InterlockedCompareExchange((LONG volatile *)dest, (LONG)exchange, (LONG)comparand);
#endif
		}//end: CAS PTR



		/** 
		 * Exchange / 'Set', returns the initial/old value of *dest
		 */
		static __forceinline atomic_t Exchange(volatile atomic_t *dest, atomic_t NewValue) {
			return _InterlockedExchange(dest, NewValue);
		}//end: Exchange

		/** 
		 * Exchange / 'Set', return the initial/old value of *dest, 64BIT Operation.
		 */
		static __forceinline atomic64_t Exchange64(volatile atomic64_t *dest, __int64 NewValue) {
#if defined(ARCH_AMD64)
			return _InterlockedExchange64(dest, NewValue);
#else
			// [FWI] As there is _no_ xchg8b instruction in x86, this is the only way how it's possible. :/
			//
			LONGLONG Old;

			do {
				Old = *dest;
			} while(_InterlockedCompareExchange64(dest,
				NewValue,
				Old) != Old);

			return Old;
#endif
		}//end: Exchange64

#if defined(ARCH_AMD64)
		/**
		* Exchange / 'Set', return the initial/old value of *dest, 128 Bit.
		*/
		static __forceinline void Exchange128(volatile atomic128_t *dest, atomic128_t *NewValue, atomic128_t *oldVal) {
			// [FWI] As there is _no_ xchg16b instruction in amd64, this is the only way how it's possible. :/
			//

			do {
				oldVal->v[0] = dest->v[0];
				oldVal->v[1] = dest->v[1];
			} while(_InterlockedCompareExchange128(dest->v,
				NewValue->v[1],
				NewValue->v[0],
				(LONG64*)oldVal->v) != 1);

		}//end: Exchange128
#endif

		/** 
		 * Exchange / 'Set', returns the initial/old value of *dest
		 */
		static __forceinline atomic_ptr ExchangePtr(volatile atomic_ptr *dest, atomic_ptr NewValue) {
			//return InterlockedExchangePointer(dest, NewValue);
#ifdef ARCH_AMD64
			return (atomic_ptr)_InterlockedExchange64((LONGLONG volatile*)dest, (LONGLONG)NewValue);
#else
			return (atomic_ptr)_InterlockedExchange((LONG volatile*)dest, (LONG)NewValue);
#endif
		}//end: ExchangePtr


		/**
		 * Adds Value to Addend, returns the old value _before_ addition (initial)
		 */
		static __forceinline atomic64_t ExchangeAdd(volatile atomic_t *Addend, int Value) {
			return _InterlockedExchangeAdd(Addend, Value);
		}//end: ExchangeAdd

		 /**
		 * Adds Value to Addend, returns the old value _before_ addition (initial)
		 */
		static __forceinline atomic64_t ExchangeAdd64(volatile atomic64_t *Addend, __int64 Value) {
			return _InterlockedExchangeAdd64(Addend, Value);
		}//end: ExchangeAdd64


		/**
		 * Adds Value to Addend, returns the old value addition 
		 */
		static __forceinline atomic64_t Add(volatile atomic_t *Addend, int Value) {
			return _InterlockedAdd(Addend, Value);
		}//end: ExchangeAdd

		/**
		 * Adds Value to Addend, returns the old value before addition
		 */
		static __forceinline atomic64_t Add64(volatile atomic64_t *Addend, __int64 Value) {
			return _InterlockedAdd64(Addend, Value);
		}//end: ExchangeAdd




		/** 
		 * Increments by 1, returns the value before incrementation
		 */
		static __forceinline atomic_t Inc(volatile atomic_t *Addend) {
			return _InterlockedIncrement(Addend);
		}//end: Inc

		/**
		 * Decrements by 1, returns the vlaue before decrementation
		 */
		static __forceinline atomic_t Dec(volatile atomic_t *Addend) {
			return _InterlockedDecrement(Addend);
		}//end: Dec

		/**
		 * Increments by 1, returns the value before incrementation
		 */
		static __forceinline atomic64_t Inc64(volatile atomic64_t *Addend) {
			return _InterlockedIncrement64(Addend);
		}//end: Inc64

		 /**
		 * Decrements by 1, returns the vlaue before decrementation
		 */
		static __forceinline atomic64_t Dec64(volatile atomic64_t *Addend) {
			return _InterlockedDecrement64(Addend);
		}//end: Dec64


	}
}