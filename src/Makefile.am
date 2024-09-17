AM_CPPFLAGS = -I$(top_srcdir)/src
libmemproxy_la_CXXFLAGS = @CXXFLAGS@ -fPIC -DCONF_FILE=$(sysconfdir)/$(PACKAGE_NAME).conf
libmemproxy_la_LDFLAGS = @LDFLAGS@ -Wl,--wrap=malloc -Wl,--wrap=free -Wl,--wrap=cfree -Wl,--wrap=realloc -Wl,--wrap=calloc -Wl,--wrap=malloc_usable_size -shared -version-info @MEMPROXY_CURRENT@:@MEMPROXY_REVISION@:@MEMPROXY_AGE@

lib_LTLIBRARIES = libmemproxy.la
libmemproxy_la_SOURCES = memproxy.h memproxy.cc

dist_sysconf_DATA = $(PACKAGE_NAME).conf

DISTCLEANFILES = \
	Makefile
