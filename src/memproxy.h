#pragma once

#if !__cplusplus >= 201103L || !__cplusplus >= 199711L
#	error This program needs at least a C++11 compliant compiler
#endif

#ifdef HAVE_CONFIG_H
#	include "autoconf.h"
#endif

#include <cstdlib>	// For std::size_t
#include <cstdint>	// For std::uintptr_t
#include <fstream>
#include <string>
#include <unordered_set>

#ifndef _GNU_SOURCE
#	define _GNU_SOURCE
#endif

#if !HAVE_DLFCN_H
#	error Require dlfcn.h to build
#else
#	include <dlfcn.h>	// For dlsym/dlopen
#endif

#if !HAVE_UNISTD_H
#	error Require unistd.h to build
#else
#	include <unistd.h>	// For sysconf()
#endif

#if !HAVE_SYS_MMAN_H
#	error Require sys/mman.h to build
#else
#	include <sys/mman.h>
#endif

#include <errno.h>

// Solaris libc.so.1
#define MEMPROXY_LIBC "libc.so.1"

// Hints to tell the compiler if a condition is likely or unlikely to be true.
#if defined(MEMPROXY_LIKELY) || defined(MEMPROXY_UNLIKELY)
#	if !defined(MEMPROXY_LIKELY) || !defined(MEMPROXY_UNLIKELY)
#		error MEMPROXY_LIKELY and MEMPROXY_UNLIKELY should either both be provided, or both left undefined.
#	endif
#else
#	if defined(__GNUC__) || defined(__clang__) || defined(__SUNPRO_CC)
#		define MEMPROXY_LIKELY(x) __builtin_expect(!!(x), 1)
#		define MEMPROXY_UNLIKELY(x) __builtin_expect(!!(x), 0)
#	else
#		define MEMPROXY_LIKELY(x) (x)
#		define MEMPROXY_UNLIKELY(x) (x)
#	endif
#endif

// Executable name limit
#define NAME_CHUNK 16

// Custom allocator API function names
// Note: Do not define interposed malloc/realloc/free etc. Use internal API instead
#define CUSTOM_MALLOC   "TCMallocInternalMalloc"
#define CUSTOM_REALLOC  "TCMallocInternalRealloc"
#define CUSTOM_CALLOC   "TCMallocInternalCalloc"
#define CUSTOM_FREE     "TCMallocInternalFree"
#define CUSTOM_MEMALIGN "TCMallocInternalMemalign"
#define CUSTOM_SIZE     "TCMallocInternalMallocSize"

#ifndef CONF_FILE
#	define CONF_FILE "/etc/memproxy.conf"
#endif
#define STRINGIZE2(x) #x
#define STRINGIZE(x) STRINGIZE2(x)
#define CONFIG STRINGIZE(CONF_FILE)

namespace {

bool g_Exists { false }, g_Init { false };

using uInt_type = std::size_t;
using voidPtr_type = void*;

class CheckProgramInList {
protected:
	CheckProgramInList() noexcept {
		std::unordered_set<std::string> v_list;
		std::ifstream v_fd = std::ifstream(CONFIG, std::ios_base::binary|std::ios_base::in);
		if (v_fd.is_open()) {
			std::string v_data;
			while (std::getline(v_fd, v_data)) {
				if (v_data[0] == '#' || v_data[0] == ';') continue;	// Skip comments
				v_list.emplace(v_data);
			}
			if (v_list.find(getRuntimeNchunk()) != v_list.end())
				g_Exists = true;
			v_fd.close();
		}
	};
private:
	std::string getRuntimeNchunk(uInt_type p_size = NAME_CHUNK);
};

class MemoryProxyFunctions : CheckProgramInList {
public:
	using func1_type = voidPtr_type (*)(uInt_type);			/* func1_type Type 1: malloc */
	using func2_type = voidPtr_type (*)(voidPtr_type, uInt_type);	/* func2_type Type 2: realloc */
	using func3_type = voidPtr_type (*)(uInt_type, uInt_type);	/* func3_type Type 3: calloc */
	using func4_type = void (*)(voidPtr_type);			/* func4_type Type 4: free */
	using func5_type = voidPtr_type (*)(uInt_type, uInt_type);	/* func5_type Type 5: memalign */
	using func6_type = uInt_type (*)(voidPtr_type);			/* func6_type Type 6: malloc_usable_size */

	func1_type m_Malloc;	/* Arg type 1 */
	func2_type m_Realloc;	/* Arg type 2 */
	func3_type m_Calloc;	/* Arg type 3 */
	func4_type m_Free;	/* Arg type 4 */
	func5_type m_Memalign;	/* Arg type 5 */
	func6_type m_Malloc_usable_size;	/* Arg type 6 */

	voidPtr_type malloc_internal(uInt_type p_size)
	{
		return reinterpret_cast<voidPtr_type>((reinterpret_cast<std::uintptr_t>(mmap(nullptr, p_size, PROT_READ|PROT_WRITE, MAP_ANON|MAP_PRIVATE, -1, 0)) + 1) & ~1);
	}

	static MemoryProxyFunctions& GetInstance() {
		static MemoryProxyFunctions inst;
		return inst;
	}

	MemoryProxyFunctions(MemoryProxyFunctions &other) = delete;
	void operator=(const MemoryProxyFunctions &) = delete;

	~MemoryProxyFunctions() {}
private:
	/* Memory functions names */
	static constexpr const char* m_c_func1 = CUSTOM_MALLOC;
	static constexpr const char* m_c_func2 = CUSTOM_REALLOC;
	static constexpr const char* m_c_func3 = CUSTOM_CALLOC;
	static constexpr const char* m_c_func4 = CUSTOM_FREE;
	static constexpr const char* m_c_func5 = CUSTOM_MEMALIGN;
	static constexpr const char* m_c_func6 = CUSTOM_SIZE;

	static constexpr const char* m_c_func12 = "malloc";
	static constexpr const char* m_c_func22 = "realloc";
	static constexpr const char* m_c_func32 = "calloc";
	static constexpr const char* m_c_func42 = "free";
	static constexpr const char* m_c_func52 = "memalign";
	static constexpr const char* m_c_func62 = "malloc_usable_size";

	MemoryProxyFunctions() noexcept : CheckProgramInList() {
		if (!g_Exists) {
			m_Malloc = reinterpret_cast<func1_type>(dlsym(RTLD_NEXT, m_c_func1));
			m_Realloc = reinterpret_cast<func2_type>(dlsym(RTLD_NEXT, m_c_func2));
			m_Calloc = reinterpret_cast<func3_type>(dlsym(RTLD_NEXT, m_c_func3));
			m_Free = reinterpret_cast<func4_type>(dlsym(RTLD_NEXT, m_c_func4));
			m_Memalign = reinterpret_cast<func5_type>(dlsym(RTLD_NEXT, m_c_func5));
			m_Malloc_usable_size = reinterpret_cast<func6_type>(dlsym(RTLD_NEXT, m_c_func6));
		} else {
			voidPtr_type v_handle = dlopen(MEMPROXY_LIBC, RTLD_NOW);
			m_Malloc = reinterpret_cast<func1_type>(dlsym(v_handle, m_c_func12));
			m_Realloc = reinterpret_cast<func2_type>(dlsym(v_handle, m_c_func22));
			m_Calloc = reinterpret_cast<func3_type>(dlsym(v_handle, m_c_func32));
			m_Free = reinterpret_cast<func4_type>(dlsym(v_handle, m_c_func42));
			m_Memalign = reinterpret_cast<func5_type>(dlsym(v_handle, m_c_func52));
			m_Malloc_usable_size = reinterpret_cast<func6_type>(dlsym(v_handle, m_c_func62));
			dlclose(v_handle);
		}
		g_Init = true;
	}
};

MemoryProxyFunctions& mpf = MemoryProxyFunctions::GetInstance();

}	/* namespace */
