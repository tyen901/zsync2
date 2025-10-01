/*
 *   zsync - client side rsync over http
 *   Copyright (C) 2004,2005,2007,2009 Colin Phipps <cph@moria.org.uk>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the Artistic License v2 (see the accompanying
 *   file COPYING for the full license terms), or, at your option, any later
 *   version of the same license.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   COPYING file for details.
 */

#ifndef ZSGLOBAL_H
#define ZSGLOBAL_H


#ifdef HAVE_CONFIG_H
#  include "config.h"
#undef HAVE_CONFIG_H
#endif

#if defined(__GNUC__) && defined (__OpenBSD__)
#  define ZS_DECL_BOUNDED(x,y,z) __attribute__((__bounded__(x,y,z)))
#else
#  define ZS_DECL_BOUNDED(x,y,z)
#endif /* ZS_DECL_BOUNDED */

/* Attribute compatibility macros for non-GNU compilers (MSVC) */
#ifndef PACKED_ATTR
# if defined(__GNUC__)
#  define PACKED_ATTR __attribute__((packed))
#  define PURE_ATTR __attribute__((pure))
# else
#  define PACKED_ATTR
#  define PURE_ATTR
# endif
#endif

/* Windows type compatibility: provide ssize_t if missing; rely on system off_t */
#if defined(_WIN32) || defined(_MSC_VER)
# include <io.h>
# include <fcntl.h>
# include <stdint.h>
# include <sys/types.h>
# ifndef ssize_t
#include <stddef.h>
/* ssize_t is not standard on MSVC; use ptrdiff_t which is pointer-sized and suitable */
typedef ptrdiff_t ssize_t;
# endif
# include <time.h>
# include <direct.h>
# include <stdlib.h>
# include <stdio.h>
# include <string.h>
# include <sys/utime.h>
# include <io.h>
# include <errno.h>
# ifndef mode_t
/* Provide a mode_t on Windows if one isn't available */
typedef unsigned int mode_t;
# endif

/* Provide a gmtime_r compatibility macro for Windows which has gmtime_s */
# ifndef HAVE_GMTIME_R
/* gmtime_s returns zero on success; emulate gmtime_r semantics */
# define gmtime_r(timep, result) (gmtime_s((result), (timep)) == 0 ? (result) : NULL)
# endif



/* Minimal fmemopen implementation for Windows: write buffer to a tmpfile and return FILE* */
#if 0
/* Note: we intentionally keep a small fmemopen shim available below for Windows
 * which writes the supplied buffer into a temporary file so callers can read
 * from it using the standard FILE* API. The implementation writes the buffer
 * into the tmpfile when the caller requests read access (mode contains 'r').
 */
#endif
# ifndef HAVE_FMEMOPEN
static inline FILE* fmemopen(void* buf, size_t size, const char* mode) {
	if (!buf) return NULL;
	FILE* f = tmpfile();
	if (!f) return NULL;
	/* If mode indicates read, write the buffer into the temp file so reads succeed */
	if (strchr(mode, 'r')) {
		if (fwrite(buf, 1, size, f) != size) {
			fclose(f);
			return NULL;
		}
		rewind(f);
	}
	return f;
}
# endif

/* Provide link and realpath wrappers */
/* On Windows make sure winsock2 is included before windows.h to avoid
	conflicts where winsock.h and winsock2.h get pulled in the same unit.
	Define WIN32_LEAN_AND_MEAN to reduce windows.h surface. */
#if defined(_WIN32)
# ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
#  define ZS_DEFINED_WIN32_LEAN_AND_MEAN
# endif
# include <winsock2.h>
# include <ws2tcpip.h>
# include <windows.h>
# ifdef ZS_DEFINED_WIN32_LEAN_AND_MEAN
#  undef WIN32_LEAN_AND_MEAN
#  undef ZS_DEFINED_WIN32_LEAN_AND_MEAN
# endif
#endif
# ifndef HAVE_LINK
static inline int link(const char* existing, const char* newp) {
	/* Use CreateHardLinkA to create a hard link on Windows */
	if (CreateHardLinkA(newp, existing, NULL)) return 0;
	return -1;
}
# endif

# ifndef HAVE_REALPATH
static inline char* realpath(const char* path, char* resolved_path) {
	/* use _fullpath; if resolved_path is NULL, _fullpath allocates via malloc */
	return _fullpath(resolved_path, path, _MAX_PATH);
}
# endif

/* On MSVC, many POSIX names are provided with an underscore prefix and the
 * non-underscored names are deprecated. Provide safe macro mappings so the
 * rest of the codebase can continue to use the POSIX names without pulling
 * deprecation warnings when building with MSVC. These are no-ops on other
 * platforms. */
#if defined(_MSC_VER)
# ifndef strdup
#  define strdup _strdup
# endif
# ifndef open
#  define open _open
# endif
# ifndef close
#  define close _close
# endif
# ifndef unlink
#  define unlink _unlink
# endif
# ifndef popen
#  define popen _popen
# endif
# ifndef pclose
#  define pclose _pclose
# endif
# ifndef getcwd
#  define getcwd _getcwd
# endif
# ifndef mkdir
#  define mkdir _mkdir
# endif
# ifndef strcasecmp
#  define strcasecmp _stricmp
# endif
# ifndef strncasecmp
#  define strncasecmp _strnicmp
# endif
# ifndef ftruncate
#  /* Declare ftruncate so callers compile; definition is provided in
#   * src/windows_compat.c on Windows and maps to the platform-specific
#   * implementation. Use a 64-bit length parameter to support large files. */
int ftruncate(int fd, long long length);
# endif

/* Provide a prototype for strptime when building on Windows; an implementation
 * is provided in src/windows_compat.c and this declaration avoids implicit
 * declaration warnings on MSVC. */
# ifndef HAVE_STRPTIME
char *strptime(const char *s, const char *format, struct tm *tm);
# endif
#endif /* _MSC_VER */

#endif /* _WIN32 || _MSC_VER */

#endif /* ZSGLOBAL_H */
