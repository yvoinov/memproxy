#                                               -*- Autoconf -*-
#  @configure_input@
#
#  Configuration scipt for ltalloc Library
#
# Process this file with autoconf to produce a configure script.

m4_define([VERSION_MAJOR],[2])
m4_define([VERSION_MINOR],[0])
m4_define([VERSION_MICRO],[1])

AC_INIT(memproxy, m4_defn([VERSION_MAJOR]).m4_defn([VERSION_MINOR]).m4_defn([VERSION_MICRO]), [], memproxy)

MEMPROXY_CURRENT=2
MEMPROXY_REVISION=1
MEMPROXY_AGE=1
# 1.0.0 had 1:0:0
# 1.0.1 had 1:1:0
# 1.0.2 had 1:2:0
# 1.0.3 had 1:3:0
# 1.0.4 had 1:4:0
# 1.0.5 had 1:5:0
# 1.0.6 had 1:6:0
# 1.0.7 had 1:7:0
# 1.0.8 had 1:8:0
# 1.0.9 had 1:9:0
# 1.0.10 had 1:10:0
# 1.0.11 had 1:11:0
# 1.0.12 had 1:12:0
# 1.0.13 had 1:13:0
# 1.0.14 had 1:14:0
# 1.0.15 had 1:15:0
# 2.1.0 had 2:0:1
# 2.1.1 had 2:1:1

#   Current  -- the number of the binary API that we're implementing
#   Revision -- which iteration of the implementation of the binary
#               API are we supplying?
#   Age      -- How many previous binary API versions do we also
#               support?
#
# If we release a new version that does not change the binary API,
# increment Revision.
#
# If we release a new version that changes the binary API, but does
# not break programs compiled against the old binary API, increment
# Current and Age.  Set Revision to 0, since this is the first
# implementation of the new API.
#
# Otherwise, we're changing the binary API and breaking backward
# compatibility with old binaries.  Increment Current.  Set Age to 0,
# since we're backward compatible with no previous APIs.  Set Revision
# to 0 too.
AC_SUBST(MEMPROXY_CURRENT)
AC_SUBST(MEMPROXY_REVISION)
AC_SUBST(MEMPROXY_AGE)

AC_CONFIG_AUX_DIR(cfgaux)
AC_CONFIG_MACRO_DIRS([m4])
AC_CONFIG_FILES([Makefile src/Makefile])
AC_CONFIG_HEADERS([src/autoconf.h:src/autoconf.h.in])

AC_USE_SYSTEM_EXTENSIONS
AM_INIT_AUTOMAKE([1.16 subdir-objects])
AM_MAINTAINER_MODE
#AM_SILENT_RULES([yes])

m4_include([acinclude/ax_cxx_compile_stdcxx.m4])
m4_include([acinclude/ax_cxx_namespaces.m4])
m4_include([acinclude/ax_cxx_have_stl.m4])
m4_include([acinclude/ax_compiler_vendor.m4])
m4_include([acinclude/ax_check_compile_flag.m4])
m4_include([acinclude/ax_check_library.m4])

dnl Detect the canonical host and target build environment
AC_CANONICAL_HOST
dnl Get system canonical name
AC_DEFINE_UNQUOTED(OS, "${host}", [cpu-machine-OS])

dnl http://www.gnu.org/savannah-checkouts/gnu/autoconf/manual/autoconf-2.69/html_node/C_002b_002b-Compiler.html
: ${CXXFLAGS=""}
PRESET_CXXFLAGS="$CXXFLAGS"
: ${LDFLAGS=""}
PRESET_LDFLAGS="$LDFLAGS"

dnl Checks for programs
AC_PROG_CXX([clang++ g++])
AC_LANG([C++])
AX_CXX_HAVE_STL
AM_PROG_AR
AC_PROG_RANLIB
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AX_COMPILER_VENDOR

AC_PROG_EGREP
if test "x$ac_cv_path_EGREP" = "xno"; then
  AC_MSG_ERROR([egrep was not found; install it first])
fi

LT_INIT

dnl Check STL
if test "x$ax_cv_cxx_have_stl" = "xno"; then
  AC_MSG_ERROR([STL was not found; it is require to build])
fi

dnl If the user did not specify a C++ version.
user_cxx=`echo "$PRESET_CXXFLAGS" | $EGREP -o -E "\-std="`
if test "x$user_cxx" = "x"; then
  dnl Check for C++11 compiler support
  AX_CXX_COMPILE_STDCXX(11, [noext], [mandatory])
fi

dnl If the user did not specify optimization level
if test "$ax_cv_cxx_compiler_vendor" = "sun"; then
  if test -z `echo "$PRESET_CXXFLAGS" | $EGREP -o -E "\-fast|\-xO"`; then
    CXXFLAGS="$CXXFLAGS -xO4"
    LDFLAGS="$LDFLAGS -xO4"
  fi
else
  if test -z `echo "$PRESET_CXXFLAGS" | $EGREP -o -E "\-O"`; then
    CXXFLAGS="$CXXFLAGS -O3"
  fi
fi

dnl Do the stuff needed for compiler-specific optimization/platform settings
if test "$ax_cv_cxx_compiler_vendor" = "sun"; then
  if test -z `echo "$PRESET_CXXFLAGS" | $EGREP -o -E "\-xatomic"`; then
    CXXFLAGS="$CXXFLAGS -xatomic=studio"
  fi
  if test -z `echo "$PRESET_CXXFLAGS" | $EGREP -o -E "\-xtarget"`; then
   CXXFLAGS="$CXXFLAGS -xtarget=generic"
  fi
else
  if test -z `echo "$PRESET_CXXFLAGS" | $EGREP -o -E "\-mtune"`; then
    CXXFLAGS="$CXXFLAGS -mtune=native"
  fi
fi

if test "$ax_cv_cxx_compiler_vendor" = "sun"; then
    dnl Check linkopt (Oracle Developer Studio)
    AX_CHECK_COMPILE_FLAG([-xlinkopt], [CXXFLAGS="$CXXFLAGS -xlinkopt=2"; LDFLAGS="$LDFLAGS -xlinkopt=2"; ac_cv_lto_supported=yes], [ac_cv_lto_supported=no], [])
elif test "$ax_cv_cxx_compiler_vendor" = "clang"; then
    dnl Check ThinLTO (CLang)
    AX_CHECK_COMPILE_FLAG([-flto], [CXXFLAGS="$CXXFLAGS -flto=thin"; LDFLAGS="$LDFLAGS -flto=thin"; ac_cv_lto_supported=yes], [ac_cv_lto_supported=no], [])
elif test "$ax_cv_cxx_compiler_vendor" = "gnu"; then
    dnl Check LTO (GCC)
    AX_CHECK_COMPILE_FLAG([-flto], [CXXFLAGS="$CXXFLAGS -flto"; LDFLAGS="$LDFLAGS -flto"; ac_cv_lto_supported=yes], [ac_cv_lto_supported=no], [])
fi

if test x"$ac_cv_lto_supported" = xno; then
  AC_MSG_NOTICE([LTO/Link optimization not supported])
fi

dnl Check libdl and header
AX_CHECK_LIBRARY([LIBDL], [dlfcn.h], [dl], [LDFLAGS="$LDFLAGS -ldl"], [AC_MSG_NOTICE([libdl not found])])

dnl Check required headers
AC_CHECK_HEADERS([sys/mman.h], [], [AC_MSG_ERROR([Require sys/mman.h to build])])

AC_SUBST(AM_CXXFLAGS)
AC_SUBST(AM_LDFLAGS)
AC_OUTPUT
