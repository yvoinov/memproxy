AM_CPPFLAGS = -I$(top_srcdir)/src
AM_CXXFLAGS = -Wl,--wrap=malloc,--wrap=free,--wrap=cfree,--wrap=realloc,--wrap=calloc,--wrap=memalign,--wrap=posix_memalign,--wrap=aligned_alloc,--wrap=valloc,--wrap=pvalloc,--wrap=malloc_usable_size,--wrap=malloc_trim

libmemproxy_la_CXXFLAGS = @CXXFLAGS@ -fPIC -DCONF_FILE=$(sysconfdir)/$(PACKAGE_NAME).conf
libmemproxy_la_LDFLAGS = @LDFLAGS@ -shared -version-info @MEMPROXY_CURRENT@:@MEMPROXY_REVISION@:@MEMPROXY_AGE@

lib_LTLIBRARIES = libmemproxy.la
libmemproxy_la_SOURCES = memproxy.h memproxy.cc

dist_sysconf_DATA = $(PACKAGE_NAME).conf

DISTCLEANFILES = \
	Makefile
