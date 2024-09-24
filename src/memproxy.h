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

#if !defined(__FreeBSD__) && !defined(__OpenBSD__)
#	include <malloc.h>
#endif
#include <errno.h>

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
#define CUSTOM_TRIM     "TCMallocInternalMallocTrim"

#ifndef CONF_FILE
#	define CONF_FILE "/etc/memproxy.conf"
#endif
#define STRINGIZE2(x) #x
#define STRINGIZE(x) STRINGIZE2(x)
#define CONFIG STRINGIZE(CONF_FILE)

namespace {

bool g_Exists { false }, g_Init { false };

using uInt_t = std::size_t;
using voidPtr_t = void*;

class CheckProgramInList {
protected:
	CheckProgramInList() noexcept {
		std::unordered_set<std::string> v_list;
		std::ifstream v_fd = std::ifstream(CONFIG, std::ios_base::binary|std::ios_base::in);
		if (v_fd.is_open()) {
			std::string v_data;
			while (std::getline(v_fd, v_data)) {
				if (v_data[0] == '#' || v_data[0] == ';') continue;	// Skip comment
				v_list.emplace(v_data);
			}
			if (v_list.find(getRuntimeNchunk()) != v_list.end())
				g_Exists = true;
			v_fd.close();
		}
	};
private:
	std::string getRuntimeNchunk(uInt_t p_size = NAME_CHUNK);
};

class MemoryProxyFunctions : CheckProgramInList {
public:
	using func1_t = voidPtr_t (*)(uInt_t);			/* func1_t Type 1: malloc */
	using func2_t = voidPtr_t (*)(voidPtr_t, uInt_t);	/* func2_t Type 2: realloc */
	using func3_t = voidPtr_t (*)(uInt_t, uInt_t);		/* func3_t Type 3: calloc */
	using func4_t = void (*)(voidPtr_t);			/* func4_t Type 4: free */
	using func5_t = voidPtr_t (*)(uInt_t, uInt_t);		/* func5_t Type 5: memalign */
	using func6_t = uInt_t (*)(voidPtr_t);			/* func6_t Type 6: malloc_usable_size */
	#if defined(__linux__)
	using func7_t = int (*)(uInt_t);			/* func7_t Type 7: malloc_trim */
	#endif

	func1_t m_Malloc;	/* Arg type 1 */
	func2_t m_Realloc;	/* Arg type 2 */
	func3_t m_Calloc;	/* Arg type 3 */
	func4_t m_Free;		/* Arg type 4 */
	func5_t m_Memalign;	/* Arg type 5 */
	func6_t m_Malloc_usable_size;	/* Arg type 6 */
	#if defined(__linux__)
	func7_t m_Malloc_trim;	/* Arg type 7 */
	#endif

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
	#if defined(__linux__)
	static constexpr const char* m_c_func7 = CUSTOM_TRIM;
	#endif

	/* Memory functions names */
	static constexpr const char* m_c_func12 = "malloc";
	static constexpr const char* m_c_func22 = "realloc";
	static constexpr const char* m_c_func32 = "calloc";
	static constexpr const char* m_c_func42 = "free";
	static constexpr const char* m_c_func52 = "memalign";
	static constexpr const char* m_c_func62 = "malloc_usable_size";
	#if defined(__linux__)
	static constexpr const char* m_c_func72 = "malloc_trim";
	#endif

	MemoryProxyFunctions() noexcept : CheckProgramInList() {
		if (!g_Exists) {
			m_Malloc = reinterpret_cast<func1_t>(dlsym(RTLD_NEXT, m_c_func1));
			m_Realloc = reinterpret_cast<func2_t>(dlsym(RTLD_NEXT, m_c_func2));
			m_Calloc = reinterpret_cast<func3_t>(dlsym(RTLD_NEXT, m_c_func3));
			m_Free = reinterpret_cast<func4_t>(dlsym(RTLD_NEXT, m_c_func4));
			m_Memalign = reinterpret_cast<func5_t>(dlsym(RTLD_NEXT, m_c_func5));
			m_Malloc_usable_size = reinterpret_cast<func6_t>(dlsym(RTLD_NEXT, m_c_func6));
			#if defined(__linux__)
			m_Malloc_trim = reinterpret_cast<func7_t>(dlsym(RTLD_NEXT, m_c_func7));
			#endif
		} else {
			m_Malloc = reinterpret_cast<func1_t>(dlsym(RTLD_NEXT, m_c_func12));
			m_Realloc = reinterpret_cast<func2_t>(dlsym(RTLD_NEXT, m_c_func22));
			m_Calloc = reinterpret_cast<func3_t>(dlsym(RTLD_NEXT, m_c_func32));
			m_Free = reinterpret_cast<func4_t>(dlsym(RTLD_NEXT, m_c_func42));
			m_Memalign = reinterpret_cast<func5_t>(dlsym(RTLD_NEXT, m_c_func52));
			m_Malloc_usable_size = reinterpret_cast<func6_t>(dlsym(RTLD_NEXT, m_c_func62));
			#if defined(__linux__)
			m_Malloc_trim = reinterpret_cast<func7_t>(dlsym(RTLD_NEXT, m_c_func72));
			#endif
		}
		g_Init = true;
	}
};

MemoryProxyFunctions& mpf = MemoryProxyFunctions::GetInstance();

}	/* namespace */
