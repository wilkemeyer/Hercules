#pragma once

#if ROALLOC_TRACE
//
// Trace Enable Version
//
#define roalloc(sz) _roalloc(sz, __FILE__, __LINE__, __FUNCTION__, NULL, false)
#define rocalloc(szmemb, num) _roalloc((szmemb * num), __FILE__, __LINE__, __FUNCTION__, NULL, true)
#define rorealloc(oldptr, newsz) _rorealloc(oldptr, newsz, __FILE__, __LINE__, __FUNCTION__, NULL, false)
#define roreallocz(oldptr, newsz) _rorealloc(oldptr, newsz, __FILE__, __LINE__, __FUNCTION__, NULL, true)
#define rofree(ptr) _rofree(ptr, __FILE__, __LINE__, __FUNCTION__)
#define rostrdup(pcstring) _rostrdup(pcstring, __FILE__, __LINE__, __FUNCTION__, NULL)

void *_roalloc(size_t sz, const char *file, unsigned int line, const char *func, void *callee, bool zeroed);
void *_rorealloc(void *oldptr, size_t newsz, const char *file, unsigned int line, const char *func, void *callee, bool zeroed);
void _rofree(void *ptr, const char *file, unsigned int line, const char *func);
char *_rostrdup(const char *pStr, const char *file, unsigned int line, const char *func, void *callee);
#else
//
// Normal Version
//
#define roalloc(sz) _roalloc(sz, false)
#define rocalloc(szmemb, num) _roalloc((szmemb * num), true)
#define rorealloc(oldptr, newsz) _rorealloc(oldptr, newsz, false)
#define roreallocz(oldptr, newsz) _rorealloc(oldptr, newsz, true)
#define rofree(ptr) _rofree(ptr) //ptr = NULL
#define rostrdup(pcstring) _rostrdup(pcstring)
void *_roalloc(size_t sz, bool zeroed);
void *_rorealloc(void *oldptr, size_t newsz, bool zeroed);
void _rofree(void *ptr);
char *_rostrdup(const char *pStr);
#endif


///
/// Wrappers for RBDB
///
void *roalloc_rbdb(size_t sz);
void rofree_rbdb(void *memblock);


/////
///// C++ Operators
/////
#include <new>
namespace rgCore {
class roAlloc {
public:
	__forceinline void * operator new(std::size_t size) {
#if ROALLOC_TRACE
		void *ptr = _roalloc(size, __FILE__, __LINE__, __FUNCTION__, _ReturnAddress(), false );
#else
		void *ptr = _roalloc(size, false);
#endif
		if(ptr == NULL) {
			throw new std::bad_alloc();
		}

		return ptr;
	}//end: operator new()


	__forceinline void*  operator new(std::size_t size, const std::nothrow_t& nothrow_constant){
#if ROALLOC_TRACE
		void *ptr = _roalloc(size, __FILE__, __LINE__, __FUNCTION__, _ReturnAddress(), false);
#else
		void *ptr = _roalloc(size, false);
#endif
		return ptr;
	}//end: operator new() - no throw version



	__forceinline void operator delete (void *ptr) {
		rofree(ptr);
	}//end: operator delete()


	__forceinline void operator delete (void* ptr, const std::nothrow_t& nothrow_constant) {
		rofree(ptr);
	}//end: operator delete() - no throw version

};
}

void roalloc_init();
void roalloc_final();
