# Memproxy
[![CodeQL](https://github.com/yvoinov/memproxy/actions/workflows/codeql-analysis.yml/badge.svg)](https://github.com/yvoinov/memproxy/actions/workflows/codeql-analysis.yml) [![License](https://img.shields.io/badge/License-BSD%203--Clause-blue.svg)](https://github.com/yvoinov/memproxy/blob/main/LICENSE)

# This repository is archived and is no longer being developed.

## Concepts

Memproxy is designed to route requests for memory allocation and deallocation as a plugin to custom allocator. It is intended to handle exceptions - applications whose allocation requests must be routed to the system allocator.

The principle of operation lies in the global preloading of the proxy before the custom allocator. When loading, the library determines the name of the executable file that initiated the loading and checks it against the exclusion list specified in the configuration file.

If the filename is in the list, a boolean flag is set and all allocation/deallocation requests are redirected to the system allocator.

If the application is not on the exclusion list, allocation requests are processed in the standard way for the custom allocator, according to the list of the custom allocator wrapper.

**Note**: This library works fine only on Solaris. For this reason, it can only be compiled on this platform.

## Build and installation


### Build memproxy

To make and install memproxy run:

```sh
# ./configure 'CXXFLAGS=-m64' --libdir=/usr/local/lib/64
```
or

```sh
# ./configure 'CXXFLAGS=-m32' --libdir=/usr/local/lib
```
then

```sh
# make
# make install-strip
```
Installation prefix by default is /usr/local. Logging library libmemproxy.so will install into $PREFIX/lib.

## Using memproxy

### Prerequisites

#### Solaris
Run (for 32 bit memproxy):

```sh
# crle -c /var/ld/ld.config -l /opt/csw/lib:/lib:/usr/lib -s /lib/secure:/usr/lib/secure:/usr/lib:/usr/local/lib
```
and/or (for 64 bit memproxy):

```sh
# crle -64 -c /var/ld/64/ld.config -l /opt/csw/lib/64:/lib/64:/usr/lib/64 -s /lib/secure/64:/usr/lib/secure/64:/usr/local/lib/64
```

To avoid possible problems, library runtimes should be included in search paths. Since the global preload also affects system  services, you should add the library search paths to the lists of safe libraries. For security reasons, it may be necessary to install the proxyifier in separate directories.

Also  remember that runtimes in different versions of Solaris may have different paths.

Note: A custom allocator can have its own prerequisites.

#### Running memproxy
After the preparation is complete, you are ready to proxify your application.

Since  the  easiest  way to intercept memory allocation functions cross-platform
is    to   use   LD_PRELOAD,   you   can   load  the proxy library first before
libc  and custom allocator (after building memproxy of the appropriate bit size)
exactly in that order:

```sh
# export LD_PRELOAD_32=libmemproxy.so:/usr/lib/libc.so:lib_custom_alloc_name.so
```

or

```sh
# export LD_PRELOAD_64=libmemproxy.so:/usr/lib/libc.so:lib_custom_alloc_name.so
```

where lib_custom_alloc_name is, for example, libjemalloc.

The recommended practice, however, is to use memproxy globally via crle.

## Configuration

Exclusion list is defined in $(CONFIG_DIR)/memproxy.conf. The configuration file is a list of executable binary file names (no absolute or relative path), consecutively, separated by a carriage return. Note that CR/LF is not allowed; if such characters are present in the configuration file, all or part of the values will be silently ignored.

Config contents example:

```sh
#ls
;df
;du
top
telegram
wget
```
Note: Strings starting from # or ; threats as comment and will be ignored.
