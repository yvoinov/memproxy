/**
 * Memory allocations calls proxifier
 */

#include "memproxy.h"

namespace {

/* Implementations */
std::string CheckProgramInList::getRuntimeNchunk(uInt_type p_size) {
	std::string v_name;
	if (getexecname() != 0)
		v_name = getexecname();
	if (v_name.find("/") != std::string::npos)
		if (v_name.size() > p_size)
			return v_name.substr(v_name.find_last_of("/") + 1, p_size);
		else
			return v_name.substr(v_name.find_last_of("/") + 1);
	else return v_name;
}

inline voidPtr_type check_ptr(voidPtr_type ptr)
{
	if (MEMPROXY_UNLIKELY(!ptr))
		return nullptr;
	else return ptr;
}

inline voidPtr_type check_ptr_errno(voidPtr_type ptr)
{
	if (MEMPROXY_UNLIKELY(!ptr)) {
		errno = ENOMEM;
		return nullptr;
	} else return ptr;
}

inline uInt_type get_page_size()
{
	static uInt_type pagesize { 0 };
	if (!pagesize) pagesize = uInt_type(sysconf(_SC_PAGE_SIZE));
	return pagesize;
}

inline voidPtr_type malloc_impl(uInt_type size)
{
	if (!g_Init)
		return mpf.malloc_internal(size);
	return mpf.m_Malloc(size);
}

inline void free_impl(voidPtr_type ptr)
{
	mpf.m_Free(ptr);
}

inline voidPtr_type calloc_impl(uInt_type n, uInt_type size)
{
	if (!mpf.m_Calloc)	/* Requires calloc replacement to stop recursion during dlsym inner calloc call */
		return mpf.malloc_internal(n * size);
	return mpf.m_Calloc(n, size);
}

inline voidPtr_type realloc_impl(voidPtr_type ptr, uInt_type size)
{
	return mpf.m_Realloc(ptr, size);
}

inline voidPtr_type memalign_impl(uInt_type alignment, uInt_type size)
{
	if (MEMPROXY_UNLIKELY(((alignment % sizeof(void*)) || (alignment & (alignment - 1)) || alignment == 0))) {
		errno = EINVAL;
		return nullptr;
	}
	return check_ptr_errno(mpf.m_Memalign(alignment, size));
}

inline int posix_memalign_impl(voidPtr_type* memptr, uInt_type alignment, uInt_type size)
{
	if (MEMPROXY_UNLIKELY(((alignment % sizeof(void*)) || (alignment & (alignment - 1)) || alignment == 0)))
		return EINVAL;
	void* ptr;
	ptr = mpf.m_Memalign(alignment, size);
	if (MEMPROXY_UNLIKELY(!ptr))
		return ENOMEM;
	else {
		*memptr = ptr;
		return 0;
	}
}

inline voidPtr_type aligned_alloc_impl(uInt_type alignment, uInt_type size)
{
	return mpf.m_Memalign(alignment, size);
}

inline voidPtr_type valloc_impl(uInt_type size)
{
	return check_ptr_errno(mpf.m_Memalign(get_page_size(), size));
}

inline voidPtr_type pvalloc_impl(uInt_type size)
{
	if (MEMPROXY_UNLIKELY(size == 0)) size = get_page_size();	// pvalloc(0) should allocate one page, according to https://man.cx/libmpatrol(3)
	return check_ptr_errno(mpf.m_Memalign(get_page_size(), size));
}

}	/* namespace */

extern "C" {

void* malloc(std::size_t size)
{
	return malloc_impl(size);
}

void free(void* ptr)
{
	free_impl(ptr);
}

void* calloc(std::size_t n, std::size_t size)
{
	return calloc_impl(n, size);
}

void cfree(void* ptr)
{
	free_impl(ptr);
}

void* realloc(void* ptr, std::size_t size)
{
	return realloc_impl(ptr, size);
}

void* memalign(std::size_t alignment, std::size_t size)
{
	return memalign_impl(alignment, size);
}

int posix_memalign(void** memptr, std::size_t alignment, std::size_t size)
{
	return posix_memalign_impl(memptr, alignment, size);
}

void* aligned_alloc(std::size_t alignment, std::size_t size)
{
	return aligned_alloc_impl(alignment, size);
}

void* valloc(std::size_t size)
{
	return valloc_impl(size);
}

void* pvalloc(std::size_t size)
{
	return pvalloc_impl(size);
}

std::size_t malloc_usable_size(void *ptr)
{
	return mpf.m_Malloc_usable_size(ptr);
}

} //for extern "C"
