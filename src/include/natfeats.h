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
#if NATFEAT_PHYS_ADDR
	memcpy(dst, Atari2HostAddr(src), n);
#else
	uint8 *dest = (uint8 *)dst;
	while ( n-- )
		*dest++ = (char)ReadInt8( (uint32)src++ );
#endif
}

static inline void Host2Atari_memcpy(memptr dest, const void *src, size_t n)
{
#if NATFEAT_PHYS_ADDR
	memcpy(Atari2HostAddr(dest), src, n);
#else
	uint8 *source = (uint8 *)src;
	while ( n-- )
		WriteInt8( dest++, *source++ );
#endif
}

static inline void Atari2HostSafeStrncpy( char *dest, memptr source, size_t count )
{
#if NATFEAT_PHYS_ADDR
	safe_strncpy(dest, (const char*)Atari2HostAddr(source), count);
#else
	while ( count > 1 && (*dest = (char)ReadInt8( source++ )) != 0 ) {
		count--;
		dest++;
	}
	if (count > 0)
		*dest = '\0';
#endif
}

static inline void Host2AtariSafeStrncpy( memptr dest, char *source, size_t count )
{
#if NATFEAT_PHYS_ADDR
	safe_strncpy((char *)Atari2HostAddr(dest), source, count);
#else
	while ( count > 1 && *source ) {
		WriteInt8( dest++, (uint8)*source++ );
		count--;
	}
	if (count > 0)
		WriteInt8( dest, 0 );
#endif
}
#endif /* _NATFEATS_H */
