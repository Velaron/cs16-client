/*
extdll.h - must be included into the all ui files
Copyright (C) 2010 Uncle Mike

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#ifndef EXTDLL_H
#define EXTDLL_H

#ifdef _WIN32
#define EXPORT __declspec(dllexport)
#else
#define EXPORT
#endif


// shut-up compiler warnings
#ifdef MSC_VER
#pragma warning(disable : 4305)	// int or float data truncation
#pragma warning(disable : 4201)	// nameless struct/union
#pragma warning(disable : 4514)	// unreferenced inline function removed
#pragma warning(disable : 4100)	// unreferenced formal parameter
#pragma warning(disable : 4244)	// conversion from 'float' to 'int', possible loss of data
#endif
// Misc C-runtime library headers
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>

// C++11 stuff
#ifndef MY_COMPILER_SUCKS

#define FINAL final
#define CONSTEXPR constexpr

// Not needed anymore
// #include <functional>
#else

#define FINAL
#define CONSTEXPR

#endif
#ifdef bound
#undef bound
#endif

#define bound( min, num, max )	((num) >= (min) ? ((num) < (max) ? (num) : (max)) : (min))
#define Q_min( a, b )	(((a) < (b)) ? (a) : (b))
#define Q_max( a, b ) (((a) < (b)) ? (b) : (a))

#ifndef FALSE
#define FALSE	0
#endif

#ifndef TRUE
#define TRUE	(!FALSE)
#endif

#if !defined(_WIN32) && !defined(__MINGW32__)
#define stricmp		strcasecmp
#define strnicmp	strncasecmp
#else
#define strnicmp	_strnicmp
#define stricmp		_stricmp
#define snprintf	_snprintf
#endif

typedef int (*cmpfunc)( const void *a, const void *b );
typedef int BOOL;
typedef unsigned char byte;
typedef unsigned int uint;

#include "menu_int.h"

#endif//EXTDLL_H
