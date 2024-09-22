/**
 * Memory allocations calls proxifier
 */

#include "memproxy.h"

namespace {

/* Implementations */
std::string MemoryProxyFunctions::getRuntimeNchunk(uInt_t p_size) {
	std::string v_name;
	#if defined(__FreeBSD__) || defined(__OpenBSD__)
	if (const char* v_name_c = getprogname())
		v_name = v_name_c;
	#elif defined(__sun__)
	if (getexecname() != 0)
		v_name = getexecname();
	#else
	if (const char* v_name_c = std::getenv("_"))
		v_name = v_name_c;
	#endif
	if (v_name.find("/") != std::string::npos)
		if (v_name.size() > p_size)
			return v_name.substr(v_name.find_last_of("/") + 1, p_size);
		else
			return v_name.substr(v_name.find_last_of("/") + 1);
	else return v_name;
}

inline voidPtr_t check_ptr(voidPtr_t ptr)
{
	if (MEMPROXY_UNLIKELY(!ptr))
		return nullptr;
	else return ptr;
}

inline voidPtr_t check_ptr_errno(voidPtr_t ptr)
{
	if (MEMPROXY_UNLIKELY(!ptr)) {
		errno = ENOMEM;
		return nullptr;
	} else return ptr;
}

inline uInt_t get_page_size()
{
	static uInt_t pagesize { 0 };
	if (!pagesize) pagesize = uInt_t(sysconf(_SC_PAGE_SIZE));
	return pagesize;
}

inline voidPtr_t malloc_impl(uInt_t size)
{
	return mpf.m_Malloc(size);
}

inline void free_impl(voidPtr_t ptr)
{
	mpf.m_Free(ptr);
}

inline voidPtr_t calloc_impl(uInt_t n, uInt_t size)
{
	if (MEMPROXY_UNLIKELY(!mpf.m_Calloc))
		return reinterpret_cast<voidPtr_t>((reinterpret_cast<std::uintptr_t>(mmap(nullptr, n * size, PROT_READ|PROT_WRITE, MAP_ANON|MAP_PRIVATE, -1, 0)) + 1) & ~1);
	return mpf.m_Calloc(n, size);
}

#if !defined __GLIBC__ || (__GLIBC__ == 2 && __GLIBC_MINOR__ < 26)
inline void cfree_impl(voidPtr_t ptr)
{
	mpf.m_Free(ptr);
}
#endif

inline voidPtr_t realloc_impl(voidPtr_t ptr, uInt_t size)
{
	return mpf.m_Realloc(ptr, size);
}

inline voidPtr_t memalign_impl(uInt_t alignment, uInt_t size)
{
	if (MEMPROXY_UNLIKELY(((alignment % sizeof(void*)) || (alignment & (alignment - 1)) || alignment == 0))) {
		errno = EINVAL;
		return nullptr;
	}
	return check_ptr_errno(mpf.m_Memalign(alignment, size));
}

inline int posix_memalign_impl(voidPtr_t* memptr, uInt_t alignment, uInt_t size)
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

inline voidPtr_t aligned_alloc_impl(uInt_t alignment, uInt_t size)
{
	return check_ptr(mpf.m_Memalign(alignment, size));
}

inline voidPtr_t valloc_impl(uInt_t size)
{
	return check_ptr_errno(mpf.m_Memalign(get_page_size(), size));	//get_page_size() returns OS page size
}

inline voidPtr_t pvalloc_impl(uInt_t size)
{
	if (MEMPROXY_UNLIKELY(size == 0)) size = get_page_size();	// pvalloc(0) should allocate one page, according to https://man.cx/libmpatrol(3)
	return check_ptr_errno(mpf.m_Memalign(get_page_size(), size));
}

}	/* namespace */

extern "C" {

void* __wrap_malloc(std::size_t size)
{
	return malloc_impl(size);
}

void __wrap_free(void* ptr)
{
	free_impl(ptr);
}

void* __wrap_calloc(std::size_t n, std::size_t size)
{
	return calloc_impl(n, size);
}

#if !defined __GLIBC__ || (__GLIBC__ == 2 && __GLIBC_MINOR__ < 26)
void __wrap_cfree(void* ptr)
{
	free_impl(ptr);
}
#endif

void* __wrap_realloc(void* ptr, std::size_t size)
{
	return realloc_impl(ptr, size);
}

void* __wrap_memalign(std::size_t alignment, std::size_t size)
{
	return memalign_impl(alignment, size);
}

int __wrap_posix_memalign(void** memptr, std::size_t alignment, std::size_t size)
{
	return posix_memalign_impl(memptr, alignment, size);
}

void* __wrap_aligned_alloc(std::size_t alignment, std::size_t size)
{
	return aligned_alloc_impl(alignment, size);
}

void* __wrap_valloc(std::size_t size)
{
	return valloc_impl(size);
}

void* __wrap_pvalloc(std::size_t size)
{
	return pvalloc_impl(size);
}

std::size_t __wrap_malloc_usable_size(void *ptr)
{
	return mpf.m_Malloc_usable_size(ptr);
}

#if defined(__linux__)
int __wrap_malloc_trim(std::size_t pad)
{
	mpf.m_Malloc_trim(pad);
	return 0;
}
#endif

#if !defined(__FreeBSD__) && !defined(__OpenBSD__) && !defined(__NetBSD__)
struct mallinfo mallinfo(void)
{
	struct mallinfo m = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	return m;
}
#endif

int mallopt(int param, int value)
{
	static_cast<void>(param);
	static_cast<void>(value);
	return 1;
}

void malloc_stats(void)
{
}

void* malloc_get_state(void)
{
	return nullptr;
}

int malloc_set_state(void *state)
{
	static_cast<void>(state);
	return 0;
}

} //for extern "C"
