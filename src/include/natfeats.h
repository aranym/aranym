/*
 * natfeats.h - common functions for all NatFeats
 *
 * Copyright (c) 2001-2013 Petr Stehlik of ARAnyM dev team (see AUTHORS)
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

#ifndef _NATFEATS_H
#define _NATFEATS_H

#include "sysdeps.h"
#include "cpu_emulation.h"

// NatFeats CPU handlers
extern uint32 nf_get_id(memptr);
extern int32 nf_call(memptr, bool);

// NatFeats call for getting parameters
extern uint32 nf_getparameter(int);

// should NatFeats work with physical (not MMU mapped) addresses
#define NATFEAT_PHYS_ADDR	1

// should NatFeats use direct memcpy() to/from guest provided pointer (dangerous, user program can kill ARAnyM)
#define NATFEAT_LIBC_MEMCPY	0

#if NATFEAT_PHYS_ADDR
#  define ReadNFInt8	ReadAtariInt8
#  define ReadNFInt16	ReadAtariInt16
#  define ReadNFInt32	ReadAtariInt32
#  define WriteNFInt8	WriteAtariInt8
#  define WriteNFInt16	WriteAtariInt16
#  define WriteNFInt32	WriteAtariInt32
#else
#  define ReadNFInt8	ReadInt8
#  define ReadNFInt16	ReadInt16
#  define ReadNFInt32	ReadInt32
#  define WriteNFInt8	WriteInt8
#  define WriteNFInt16	WriteInt16
#  define WriteNFInt32	WriteInt32
#endif

static inline void Atari2Host_memcpy(void *dst, memptr src, size_t n)
{
#if NATFEAT_LIBC_MEMCPY
	if (! ValidAtariAddr(src, false, n))
		BUS_ERROR(src);

	memcpy(dst, Atari2HostAddr(src), n);
#else
	uint8 *dest = (uint8 *)dst;
	while ( n-- )
		*dest++ = (char)ReadNFInt8( (uint32)src++ );
#endif
}

static inline void Host2Atari_memcpy(memptr dest, const void *src, size_t n)
{
#if NATFEAT_LIBC_MEMCPY
	if (! ValidAtariAddr(dest, true, n))
		BUS_ERROR(dest);

	memcpy(Atari2HostAddr(dest), src, n);
#else
	uint8 *source = (uint8 *)src;
	while ( n-- )
		WriteNFInt8( dest++, *source++ );
#endif
}

static inline void Atari2HostSafeStrncpy(char *dest, memptr source, size_t count)
{
#if NATFEAT_LIBC_MEMCPY
	if (! ValidAtariAddr(source, false, count))
		BUS_ERROR(source);

	safe_strncpy(dest, (const char*)Atari2HostAddr(source), count);
#else
	while ( count > 1 && (*dest = (char)ReadNFInt8( source++ )) != 0 ) {
		count--;
		dest++;
	}
	if (count > 0)
		*dest = '\0';
#endif
}

static inline void Host2AtariSafeStrncpy(memptr dest, const char *source, size_t count)
{
#if NATFEAT_LIBC_MEMCPY
	if (! ValidAtariAddr(dest, true, count))
		BUS_ERROR(dest);

	safe_strncpy((char *)Atari2HostAddr(dest), source, count);
#else
	while ( count > 1 && *source ) {
		WriteNFInt8( dest++, (uint8)*source++ );
		count--;
	}
	if (count > 0)
		WriteNFInt8( dest, 0 );
#endif
}
#endif /* _NATFEATS_H */
