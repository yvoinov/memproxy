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

#if !defined(__FreeBSD__) && !defined(__OpenBSD__)
#	include <malloc.h>
#endif
#include <errno.h>

#ifndef _GNU_SOURCE
#	define _GNU_SOURCE
#endif

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
#define CUSTOM_MALLOC   "ltmalloc"
#define CUSTOM_REALLOC  "ltrealloc"
#define CUSTOM_CALLOC   "ltcalloc"
#define CUSTOM_FREE     "ltfree"
#define CUSTOM_MEMALIGN "ltmemalign"
#define CUSTOM_SIZE     "ltmsize"
#define CUSTOM_TRIM     "ltsqueeze"

// Solaris has libc.so.1
// Linux has libc.so.6
// FreeBSD has libc.so.7
// OpenBSD has libc.so.9x.0
#if defined(__linux__)
#	define MEMPROXY_LIBC "libc.so.6"
#elif defined(__sun__)
#	define MEMPROXY_LIBC "libc.so.1"
#elif defined(__FreeBSD__)
#	define MEMPROXY_LIBC "libc.so.7"
#elif defined(__OpenBSD__)
#	define MEMPROXY_LIBC "libc.so.97.0"
#else
	#error Unsupported OS
#endif

#ifndef CONF_FILE
#	define CONF_FILE "/etc/memproxy.conf"
#endif
#define STRINGIZE2(x) #x
#define STRINGIZE(x) STRINGIZE2(x)
#define CONFIG STRINGIZE(CONF_FILE)

namespace {

bool g_Exists { false };

using uInt_t = std::size_t;
using voidPtr_t = void*;

class FunctionsPtrTypes {
protected:
	using func1_t = voidPtr_t (*)(uInt_t);			/* func1_t Type 1: malloc */
	using func2_t = voidPtr_t (*)(voidPtr_t, uInt_t);	/* func2_t Type 2: realloc */
	using func3_t = voidPtr_t (*)(uInt_t, uInt_t);		/* func3_t Type 3: calloc */
	using func4_t = void (*)(voidPtr_t);			/* func4_t Type 4: free */
	using func5_t = voidPtr_t (*)(uInt_t, uInt_t);		/* func5_t Type 5: memalign */
	using func6_t = uInt_t (*)(voidPtr_t);			/* func6_t Type 6: malloc_usable_size */
	#if defined(__linux__)
	using func7_t = int (*)(uInt_t);			/* func7_t Type 7: malloc_trim */
	#endif
};

class MemoryProxyFunctions1 : FunctionsPtrTypes {	// Memory functions from preloaded library
public:
	func1_t m_cMalloc;	/* Arg type 1 */
	func2_t m_cRealloc;	/* Arg type 2 */
	func3_t m_cCalloc;	/* Arg type 3 */
	func4_t m_cFree;	/* Arg type 4 */
	func5_t m_cMemalign;	/* Arg type 5 */
	func6_t m_cMalloc_usable_size;	/* Arg type 6 */
	#if defined(__linux__)
	func7_t m_cMalloc_trim;	/* Arg type 7 */
	#endif

	static MemoryProxyFunctions1& GetInstance() {
		static MemoryProxyFunctions1 inst;
		return inst;
	}

	MemoryProxyFunctions1(MemoryProxyFunctions1 &other) = delete;
	void operator=(const MemoryProxyFunctions1 &) = delete;

	~MemoryProxyFunctions1() {}

private:
	MemoryProxyFunctions1() noexcept {	// It makes no sense to generate stack unwinding, in case of an exception, recursion to malloc will still occur here.
		m_cMalloc = reinterpret_cast<func1_t>(dlsym(RTLD_NEXT, m_c_func1));
		m_cRealloc = reinterpret_cast<func2_t>(dlsym(RTLD_NEXT, m_c_func2));
		m_cCalloc = reinterpret_cast<func3_t>(dlsym(RTLD_NEXT, m_c_func3));
		m_cFree = reinterpret_cast<func4_t>(dlsym(RTLD_NEXT, m_c_func4));
		m_cMemalign = reinterpret_cast<func5_t>(dlsym(RTLD_NEXT, m_c_func5));
		m_cMalloc_usable_size = reinterpret_cast<func6_t>(dlsym(RTLD_NEXT, m_c_func6));
		#if defined(__linux__)
		m_cMalloc_trim = reinterpret_cast<func7_t>(reinterpret_cast<std::uintptr_t>(dlsym(RTLD_NEXT, m_c_func7)));
		#endif
		// Note: We're cannot output anything here due to allocations under the hood; also, throw
		// also allocation, so you're got recursive dump.
		if (!dlerror()) return;	/* If custom allocator not preloaded, throw */
	}

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
};

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

class MemoryProxyFunctions2 : FunctionsPtrTypes, CheckProgramInList {	// Memory functions from libC
public:
	func1_t m_Malloc;
	func2_t m_Realloc;
	func3_t m_Calloc;
	func4_t m_Free;
	func5_t m_Memalign;
	func6_t m_Malloc_usable_size;
	#if defined(__linux__)
	func7_t m_Malloc_trim;
	#endif

	static MemoryProxyFunctions2& GetInstance() {
		static MemoryProxyFunctions2 inst;
		return inst;
	}

	MemoryProxyFunctions2(MemoryProxyFunctions2 &other) = delete;
	void operator=(const MemoryProxyFunctions2 &) = delete;

	~MemoryProxyFunctions2() {}

private:
	MemoryProxyFunctions2() noexcept : CheckProgramInList() {
		voidPtr_t v_handle = dlopen(MEMPROXY_LIBC, RTLD_NOW);
		m_Malloc = reinterpret_cast<func1_t>(dlsym(v_handle, m_c_func12));
		m_Realloc = reinterpret_cast<func2_t>(dlsym(v_handle, m_c_func22));
		m_Calloc = reinterpret_cast<func3_t>(dlsym(v_handle, m_c_func32));
		m_Free = reinterpret_cast<func4_t>(dlsym(v_handle, m_c_func42));
		m_Memalign = reinterpret_cast<func5_t>(dlsym(v_handle, m_c_func52));
		m_Malloc_usable_size = reinterpret_cast<func6_t>(dlsym(v_handle, m_c_func62));
		#if defined(__linux__)
		m_Malloc_trim = reinterpret_cast<func7_t>(reinterpret_cast<std::uintptr_t>(dlsym(v_handle, m_c_func72)));
		#endif
		if (!dlerror()) return;	/* If libC not preloaded, throw */
	}

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
};

}	/* namespace */
