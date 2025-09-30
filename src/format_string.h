/*
 *   zsync - client side rsync over http
 *   Copyright (C) 2004,2005 Colin Phipps <cph@moria.org.uk>
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

#include <inttypes.h>

/* Provide safe, explicit printf format macros for various platforms.
 * On MSVC the PRIu macros from inttypes.h may not be available or may
 * expand in a way that leaves raw tokens in the source (which confuses
 * the parser when used outside string literals). Force sensible defaults
 * for MSVC, otherwise prefer the standard PRIu* macros when present.
 */
#ifdef _MSC_VER
/* MSVC: use long long specifiers which are supported by MSVC's printf
	implementation and by modern MSVC toolsets. */
# define OFF_T_PF "%llu"
# define SIZE_T_PF "%zu"
#else
# ifdef PRIu32
#  define SIZE_T_PF "%zd"
# else
#  define SIZE_T_PF "%u"
# endif

# ifdef PRIu64
#  define OFF_T_PF "%" PRIu64
# else
#  define OFF_T_PF "%llu"
# endif
#endif
