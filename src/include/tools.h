/*
 * tools.h - non-32bit CPU and miscelany utilities
 *
 * Copyright (c) 2001-2003 STanda of ARAnyM developer team (see AUTHORS)
 *
 * This file is part of the ARAnyM project which builds a new and powerful
 * TOS/FreeMiNT compatible virtual machine running on almost any hardware.
 *
 * ARAnyM is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * ARAnyM is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ARAnyM; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef _TOOLS_H
#define _TOOLS_H

#include "sysdeps.h"

# include <map>


// minimun and maximum macros
#ifndef MIN
#define MIN(_a,_b) ((_a)<(_b)?(_a):(_b))
#endif
#ifndef MAX
#define MAX(_a,_b) ((_a)>(_b)?(_a):(_b))
#endif


//
// some additional std*.h like functions
//
extern "C" {

	static inline char* strapply( char* str, int (*functor)(int) )
	{
		char* pos = str;
		while ( (*pos = (char)functor( (int)*pos )) != 0 )
			pos++;

		return str;
	}

	static inline char* strd2upath( char* dest, char* src )
	{
		char* result = dest;
		while( *src ) {
			*dest++ = (((*src == '\\') || (*src == '/')) ? DIRSEPARATOR[0] : *src);
			src++;
		}
		*dest=0;

		return result;
	}
}



// enables the conversions also on the 32bit system
//    (in case the system doesn't need them)
#define DEBUG_FORCE_NON32BIT 0

// denies any 32bit <-> ANYbit conversions
//    (explicitly denies it on any system)
#define DEBUG_DISABLE_NON32BIT 0



// single define to force 32bit algorithms
#if DEBUG_FORCE_NON32BIT && !DEBUG_DISABLE_NON32BIT
#define DEBUG_NON32BIT 1
#else
#define DEBUG_NON32BIT 0
#endif


/**
 * Provides bijective mapping between uint32 and e.g. void*
 *   (or int or any other type)
 *
 * It is a need when there is not enough space on the emulated
 * side and we need to store native pointers there (hostfs)
 *
 * Also the filedescriptor number is int (which is not always 32bit)
 * and therefore we need to handle them this way.
 */
template <class nativeType>
class NativeTypeMapper
{
	std::map<uint32,nativeType> a2n;
	std::map<nativeType,uint32> n2a;

  public:
	void putNative( nativeType value ) {
		// test if present
		if ( n2a.find( value ) != n2a.end() )
			return;

		// cast to the number (not a pointer) type
		// of the same size as the void*. Then cut the lowest
		// 32bits as the default hash value
		uint32 aValue = (uintptr)value & 0xffffffffUL;

#if DEBUG_FORCE_NON32BIT
		// easier NativeTypeMapper functionality debugging
		aValue &= 0x1fL;
#endif
		// make the aValue unique (test if present and increase if positive)
		while ( a2n.find( aValue ) != a2n.end() ) {
#if DEBUG_FORCE_NON32BIT
			fprintf(stderr,"NTM: Conflicting mapping %x [%d]\n", aValue, a2n.size());
#endif
			aValue+=7;
		}

#if DEBUG_FORCE_NON32BIT
		fprintf(stderr,"NTM: mapping %x [%d]\n", aValue, a2n.size());
#endif
		// put the values into maps (both direction search possible)
		a2n.insert( std::make_pair( aValue, value ) );
		n2a.insert( std::make_pair( value, aValue ) );
	}

	void removeNative( nativeType value ) {
		typename std::map<nativeType,uint32>::iterator it = n2a.find( value );

		// remove if present
		if ( it != n2a.end() ) {
			// remove the 32bit -> native mapping
			a2n.erase( it->second );
			// and now the native -> 32bit
			n2a.erase( value );
		}
	}

	nativeType getNative( uint32 from ) {
		return a2n.find( from )->second;
	}

	uint32 get32bit( nativeType from ) {
		return n2a.find( from )->second;
	}
};


// if the void* is not 4 byte long or if the map debugging is on
//    and if it is not explicitely turned off
#if SIZEOF_VOID_P != 4 || DEBUG_NON32BIT

# define MAPNEWVOIDP(x)   memptrMapper.putNative(x)
# define MAPDELVOIDP(x)   memptrMapper.removeNative(x)
# define MAP32TOVOIDP(x)  memptrMapper.getNative(x)
# define MAPVOIDPTO32(x)  ((memptr)memptrMapper.get32bit(x))

  extern NativeTypeMapper<void*> memptrMapper;

#else

# define MAPNEWVOIDP(x)
# define MAPDELVOIDP(x)
# define MAP32TOVOIDP(x)  x
# define MAPVOIDPTO32(x)  x

#endif


char *safe_strncpy(char *dest, const char *src, size_t size);
char *safe_strncat(char *dest, const char *src, size_t size);

#ifdef OS_cygwin
char* cygwin_path_to_win32(char *, size_t);
#endif

char *my_canonicalize_file_name(const char *filename, bool append_slash);

#endif  // _TOOLS_H
