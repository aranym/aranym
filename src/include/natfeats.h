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

// should NatFeats use direct memcpy() to/from guest provided pointer (fast but less safe)
#define NATFEAT_LIBC_MEMCPY	1

#if NATFEAT_PHYS_ADDR
#  define ReadNFInt8	ReadAtariInt8
#  define ReadNFInt16	ReadAtariInt16
#  define ReadNFInt32	ReadAtariInt32
#  define ReadNFInt64	ReadAtariInt64
#  define WriteNFInt8	WriteAtariInt8
#  define WriteNFInt16	WriteAtariInt16
#  define WriteNFInt32	WriteAtariInt32
#  define WriteNFInt64	WriteAtariInt64
#else
#  define ReadNFInt8	ReadInt8
#  define ReadNFInt16	ReadInt16
#  define ReadNFInt32	ReadInt32
#  define ReadNFInt64	ReadInt64
#  define WriteNFInt8	WriteInt8
#  define WriteNFInt16	WriteInt16
#  define WriteNFInt32	WriteInt32
#  define WriteNFInt64	WriteInt64
#endif

static inline void Atari2Host_memcpy(void *_dst, memptr src, size_t count)
{
#if NATFEAT_LIBC_MEMCPY && NATFEAT_PHYS_ADDR
	memptr src_end = src + count - 1;
	if (! ValidAtariAddr(src, false, 1))
		BUS_ERROR(src);
	if (! ValidAtariAddr(src_end, false, 1))
		BUS_ERROR(src_end);

	memcpy(_dst, Atari2HostAddr(src), count);
#else
	uint8 *dst = (uint8 *)_dst;
	while ( count-- )
		*dst++ = (char)ReadNFInt8( src++ );
#endif
}

static inline void Host2Atari_memcpy(memptr dst, const void *_src, size_t count)
{
#if NATFEAT_LIBC_MEMCPY && NATFEAT_PHYS_ADDR
	memptr dst_end = dst + count - 1;
	if (! ValidAtariAddr(dst, true, 1))
		BUS_ERROR(dst);
	if (! ValidAtariAddr(dst_end, true, 1))
		BUS_ERROR(dst_end);

	memcpy(Atari2HostAddr(dst), _src, count);
#else
	uint8 *src = (uint8 *)_src;
	while ( count-- )
		WriteNFInt8( dst++, *src++ );
#endif
}

static inline void Atari2HostSafeStrncpy(char *dst, memptr src, size_t count)
{
#if NATFEAT_LIBC_MEMCPY && NATFEAT_PHYS_ADDR
	memptr src_end = src + count - 1;
	if (! ValidAtariAddr(src, false, 1))
		BUS_ERROR(src);
	if (! ValidAtariAddr(src_end, false, 1))
		BUS_ERROR(src_end);

	safe_strncpy(dst, (const char*)Atari2HostAddr(src), count);
#else
	while ( count > 1 && (*dst = (char)ReadNFInt8( src++ )) != 0 ) {
		count--;
		dst++;
	}
	if (count > 0)
		*dst = '\0';
#endif
}

static inline void Host2AtariSafeStrncpy(memptr dst, const char *src, size_t count)
{
#if NATFEAT_LIBC_MEMCPY && NATFEAT_PHYS_ADDR
	memptr dst_end = dst + count - 1;
	if (! ValidAtariAddr(dst, true, 1))
		BUS_ERROR(dst);
	if (! ValidAtariAddr(dst_end, true, 1))
		BUS_ERROR(dst_end);

	safe_strncpy((char *)Atari2HostAddr(dst), src, count);
#else
	while ( count > 1 && *src ) {
		WriteNFInt8( dst++, (uint8)*src++ );
		count--;
	}
	if (count > 0)
		WriteNFInt8( dst, 0 );
#endif
}

static inline size_t Atari2HostSafeStrlen(memptr src)
{
	if (!src) return 0;
	size_t count = 1;
#if NATFEAT_LIBC_MEMCPY && NATFEAT_PHYS_ADDR
	for (;;)
	{
		if (! ValidAtariAddr(src, false, 1))
			BUS_ERROR(src);
		char c = ReadNFInt8(src);
		if (!c)
			break;
		count++;
		src += 1;
	}
#else
	while ((char)ReadNFInt8( src++ )) != 0 ) {
		count++;
	}
#endif
	return count;
}

void Atari2HostUtf8Copy(char *dst, memptr src, size_t count);
void Host2AtariUtf8Copy(memptr dst, const char *src, size_t count);
void charset_conv_error(unsigned short ch);

#endif /* _NATFEATS_H */
