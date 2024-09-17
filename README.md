# Memproxy
[![CodeQL](https://github.com/yvoinov/memproxy/actions/workflows/codeql-analysis.yml/badge.svg)](https://github.com/yvoinov/memproxy/actions/workflows/codeql-analysis.yml) [![License](https://img.shields.io/badge/License-BSD%203--Clause-blue.svg)](https://github.com/yvoinov/memproxy/blob/main/LICENSE)

## Concepts

Memproxy is designed to route requests for memory allocation and deallocation as a plugin to custom allocator. It is intended to handle exceptions - applications whose allocation requests must be routed to the system allocator.

The principle of operation lies in the global preloading of the proxy before the custom allocator. When loading, the library determines the name of the executable file that initiated the loading and checks it against the exclusion list specified in the configuration file.

If the filename is in the list, a boolean flag is set and all allocation/deallocation requests are redirected to the system allocator.

If the application is not on the exclusion list, allocation requests are processed in the standard way for the custom allocator, according to the list of the custom allocator wrapper.

## Build and installation


### Build memproxy

To make and install memproxy run:

```sh
# ./configure 'CXXFLAGS=-m64'
```
or
```sh
# ./configure 'CXXFLAGS=-m32'
```
then
```sh
# make
# make install-strip
```

Installation prefix by default is /usr/local. Logging library libmemproxy.so will install into $PREFIX/lib.

## Using memproxy

### Prerequisites

Most modern OS require to permit libraries/path to be used with LD_PRELOAD. To run libmemproxy, make sure you configured access to installation directory for dynamic linker.

Some examples:

#### Solaris
Run (for 32 bit memproxy):
```sh
# crle -c /var/ld/ld.config -l /lib:/usr/lib:/usr/local/lib -s /lib/secure:/usr/lib/secure:/usr/lib:/usr/local/lib
```
and/or (for 64 bit memproxy):
```sh
# crle -64 -c /var/ld/64/ld.config -l /lib/64:/usr/lib/64:/usr/local/lib -s /lib/secure/64:/usr/lib/secure/64:/usr/local/lib
```
#### Linux

Run the command:
```sh
# echo "/usr/local/lib" > /etc/ld.so.conf.d/memproxy.conf
```
then run ldconfig as root or reboot your machine

or

add /usr/local/lib to /etc/ld.so.conf, then run ldconfig as root.

#### FreeBSD/OpenBSD

As root execute command:
```sh
# ldconfig -R /usr/local/lib
```
and then run ldconfig as root or reboot system.

Note: A custom allocator can have its own prerequisites.

#### Running memproxy
After the preparation is complete, you are ready to proxify your application.

Since the easiest way to intercept memory allocation functions cross-platform is to use LD_PRELOAD, you must load the proxy library before custom allocator (after building memproxy of the appropriate bit size):
```sh
# export LD_PRELOAD=libmemproxy.so:lib_custom_alloc_name.so
```
where lib_custom_alloc_name is, for example, libjemalloc.

Note: Some platforms uses LD_PRELOAD_32/LD_PRELOAD_64/LDR_PRELOAD/LDR_PRELOAD64 environment variables instead.

The recommended practice, however, is to use memproxy globally (if the operating system allows it), in the /etc/ld.so.preload preload list.

In this case, the /etc/ld.so.preload file should contain two entries, as shown below:
```sh
# echo "libmemproxy.so:lib_custom_alloc_name.so" > /etc/ld.so.preload
```
In some cases, you may need to add the full absolute path to the libraries.

Also  keep  in  mind  you  must  edit  custom  allocator  API  function names in accordance with the API of the preloaded allocator in memproxy.h.

Note 1: Do not define interposed malloc/realloc/free etc. Use internal API instead.

Note 2: malloc_trim is absent in many implementations of custom allocators, so we do not check for the presence of the function. Accordingly, dlsym will return a null pointer and interposition of the corresponding function will not work.

#### Important Linux note
The usage method described above works on operating systems with non-allocating dlopen(). Linux, however, performs calls to calloc and malloc during the dlopen call.

Accordingly, for the correct operation of the memproxy on Linux, it is necessary to use a non-obvious trick. Namely, to explicit preload libc immediately after memproxy, like this:
```sh
# echo "libmemproxy.so:/usr/lib/libc.so.6:lib_custom_alloc_name.so" > /etc/ld.so.preload
```
Important - you must specify not a symbolic link, but a full absolute path to libc. To determine it, run the command:
```sh
# find / -name libc.so.*
```
Remember, you need to select the library of the correct bit depth (in case of multilib).

With the specified preload sequence and the correct definition of the custom allocator's internal API functions, allocation calls will be correctly routed depending on the g_Exists flag.

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

## Notes

Please note that memproxy is a plugin for custom allocator. If you don't load the allocator, it will crash because the library object cannot be constructed. Also, if it is not possible to load libC for your platform, it will also crash. In this case, you need to check the name of the libC library and, if necessary, adjust the corresponding macro in the source code.

## Restrictions

For technical reasons, I had to refuse to display error messages when checking the loading of the allocator and libC libraries. Any output statements contain malloc under the hood and lead to recursion. Thus, when segfaulting at startup, the first thing to check is that LD_PRELOAD is correct.
