/* 2001 MJ */

/*
 *  sysdeps.h - System dependent definitions for Unix
 *
 *  Basilisk II (C) 1997-2000 Christian Bauer
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef SYSDEPS_H
#define SYSDEPS_H

#ifndef __STDC__
#error "Your compiler is not ANSI. Get a real one."
#endif

#include "config.h"

#ifndef STDC_HEADERS
#error "You don't have ANSI C header files."
#endif

#ifdef OS_irix
#define OS_INCLUDES_DEFINED

#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <fcntl.h>
#include <termios.h>
#include <dirent.h>
#include <time.h>

#endif

#ifdef OS_openbsd
#define OS_INCLUDES_DEFINED

#include <unistd.h>
#include <sys/param.h>
#include <sys/mount.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <time.h>
#include <utime.h>
#include <dirent.h>
#include <fcntl.h>
#include <termios.h>

#endif

#ifdef OS_solaris
#define OS_INCLUDES_DEFINED

#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <termios.h>
#include <utime.h>
#include <alloca.h>

#endif

#ifndef OS_INCLUDES_DEFINED

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif

#ifdef HAVE_STAT_H
# include <stat.h>
#endif

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif

#ifdef HAVE_TYPES_H
# include <types.h>
#endif

#ifdef HAVE_SYS_MMAN_H
# include <sys/mman.h>
#endif

#ifdef HAVE_SYS_PARAM_H
# include <sys/param.h>
#endif

#ifdef HAVE_SYS_MOUNT_H
# include <sys/mount.h>
#endif

#ifdef HAVE_SYS_IOCTL_H
# include <sys/ioctl.h>
#endif

#ifdef HAVE_SYS_STATVFS_H
# include <sys/statvfs.h>
#endif

#ifdef HAVE_UTIME_H
# include <utime.h>
#endif

#ifdef HAVE_TERMIOS_H
# include <termios.h>
#endif

#ifdef HAVE_TERMIO_H
# include <termio.h>
#endif

#ifdef HAVE_ALLOCA_H
# include <alloca.h>
#endif

#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#endif

#ifdef TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#ifdef HAVE_DIRENT_H
# include <dirent.h>
#endif

#endif /* OS_INCLUDES_DEFINE */

#ifdef HAVE_GETOPT_H
# include <getopt.h>
#else
# include "getopt.h"
#endif

/* Atari and host address space are distinct */
#ifndef REAL_ADDRESSING
#define REAL_ADDRESSING 0
#endif

/* Using 68k emulator */
#define EMULATED_68K 1

/* The m68k emulator uses a prefetch buffer ? */
#define USE_PREFETCH_BUFFER 0

/* Data types */
typedef unsigned char uint8;
typedef signed char int8;
#if SIZEOF_SHORT == 2
typedef unsigned short uint16;
typedef short int16;
#elif SIZEOF_INT == 2
typedef unsigned int uint16;
typedef int int16;
#else
#error "No 2 byte type, you lose."
#endif
#if SIZEOF_INT == 4
typedef unsigned int uint32;
typedef int int32;
#elif SIZEOF_LONG == 4
typedef unsigned long uint32;
typedef long int32;
#else
#error "No 4 byte type, you lose."
#endif
#if SIZEOF_LONG == 8
typedef unsigned long uint64;
typedef long int64;
#define VAL64(a) (a ## l)
#define UVAL64(a) (a ## ul)
#elif SIZEOF_LONG_LONG == 8
typedef unsigned long long uint64;
typedef long long int64;
#define VAL64(a) (a ## LL)
#define UVAL64(a) (a ## uLL)
#else
#error "No 8 byte type, you lose."
#endif
#if SIZEOF_VOID_P == 4
typedef uint32 uintptr;
typedef int32 intptr;
#elif SIZEOF_VOID_P == 8
typedef uint64 uintptr;
typedef int64 intptr;
#else
#error "Unsupported size of pointer"
#endif

#define memptr uint32

/* UAE CPU data types */
#define uae_s8 int8
#define uae_u8 uint8
#define uae_s16 int16
#define uae_u16 uint16
#define uae_s32 int32
#define uae_u32 uint32
#define uae_s64 int64
#define uae_u64 uint64
typedef uae_u32 uaecptr;
typedef char flagtype;

/* Bochs data types */
#define Bit8u uint8
#define Bit16u uint16
#define Bit32u uint32

/* Alignment restrictions */
#if defined(__i386__) || defined(__powerpc__) || defined(__m68k__)
# define CPU_CAN_ACCESS_UNALIGNED
#endif

/* UAE CPU defines */
#ifdef WORDS_BIGENDIAN

#ifdef CPU_CAN_ACCESS_UNALIGNED

/* Big-endian CPUs which can do unaligned accesses */
static inline uae_u32 do_get_mem_long(uae_u32 *a) {return *a;}
static inline uae_u32 do_get_mem_word(uae_u16 *a) {return *a;}
static inline void do_put_mem_long(uae_u32 *a, uae_u32 v) {*a = v;}
static inline void do_put_mem_word(uae_u16 *a, uae_u32 v) {*a = v;}

#else /* CPU_CAN_ACCESS_UNALIGNED */

#if (defined(sgi) && !defined(__GNUC__))
/* The SGI MIPSPro compilers can do unaligned accesses given enough hints.
 * They will automatically inline these routines. */
#ifdef __cplusplus
extern "C" { /* only the C compiler does unaligned accesses */
#endif
extern uae_u32 do_get_mem_long(uae_u32 *a);
extern uae_u32 do_get_mem_word(uae_u16 *a);
extern void do_put_mem_long(uae_u32 *a, uae_u32 v);
extern void do_put_mem_word(uae_u16 *a, uae_u32 v);
#ifdef __cplusplus
}
#endif

#else /* sgi && !GNUC */

/* Big-endian CPUs which can not do unaligned accesses (this is not the most efficient way to do this...) */
static inline uae_u32 do_get_mem_long(uae_u32 *a) {uint8 *b = (uint8 *)a; return (b[0] << 24) | (b[1] << 16) | (b[2] << 8) | b[3];}
static inline uae_u32 do_get_mem_word(uae_u16 *a) {uint8 *b = (uint8 *)a; return (b[0] << 8) | b[1];}
static inline void do_put_mem_long(uae_u32 *a, uae_u32 v) {uint8 *b = (uint8 *)a; b[0] = v >> 24; b[1] = v >> 16; b[2] = v >> 8; b[3] = v;}
static inline void do_put_mem_word(uae_u16 *a, uae_u32 v) {uint8 *b = (uint8 *)a; b[0] = v >> 8; b[1] = v;}
#endif /* sgi */

#endif /* CPU_CAN_ACCESS_UNALIGNED */

#else /* WORDS_BIGENDIAN */

#ifdef __i386__

/* Intel x86 */
#define X86_PPRO_OPT
static inline uae_u32 do_get_mem_long(uae_u32 *a) {uint32 retval; __asm__ ("bswap %0" : "=r" (retval) : "0" (*a) : "cc"); return retval;}
#ifdef X86_PPRO_OPT
static inline uae_u32 do_get_mem_word(uae_u16 *a) {uint32 retval; __asm__ ("movzwl %w1,%k0\n\tshll $16,%k0\n\tbswapl %k0\n" : "=&r" (retval) : "m" (*a) : "cc"); return retval;}
#else
static inline uae_u32 do_get_mem_word(uae_u16 *a) {uint32 retval; __asm__ ("xorl %k0,%k0\n\tmovw %w1,%w0\n\trolw $8,%w0" : "=&r" (retval) : "m" (*a) : "cc"); return retval;}
#endif
#define HAVE_GET_WORD_UNSWAPPED
#define do_get_mem_word_unswapped(a) ((uae_u32)*((uae_u16 *)(a)))
static inline void do_put_mem_long(uae_u32 *a, uae_u32 v) {__asm__ ("bswap %0" : "=r" (v) : "0" (v) : "cc"); *a = v;}
#ifdef X86_PPRO_OPT
static inline void do_put_mem_word(uae_u16 *a, uae_u32 v) {__asm__ ("bswapl %0" : "=&r" (v) : "0" (v << 16) : "cc"); *a = v;}
#else
static inline void do_put_mem_word(uae_u16 *a, uae_u32 v) {__asm__ ("rolw $8,%0" : "=r" (v) : "0" (v) : "cc"); *a = v;}
#endif
#define HAVE_OPTIMIZED_BYTESWAP_32
/* bswap doesn't affect condition codes */
static inline uae_u32 do_byteswap_32(uae_u32 v) {__asm__ ("bswap %0" : "=r" (v) : "0" (v)); return v;}
#define HAVE_OPTIMIZED_BYTESWAP_16
#ifdef X86_PPRO_OPT
static inline uae_u32 do_byteswap_16(uae_u32 v) {__asm__ ("bswapl %0" : "=&r" (v) : "0" (v << 16) : "cc"); return v;}
#else
static inline uae_u32 do_byteswap_16(uae_u32 v) {__asm__ ("rolw $8,%0" : "=r" (v) : "0" (v) : "cc"); return v;}
#endif

#elif defined(CPU_CAN_ACCESS_UNALIGNED)

/* Other little-endian CPUs which can do unaligned accesses */
static inline uae_u32 do_get_mem_long(uae_u32 *a) {uint32 x = *a; return (x >> 24) | (x >> 8) & 0xff00 | (x << 8) & 0xff0000 | (x << 24);}
static inline uae_u32 do_get_mem_word(uae_u16 *a) {uint16 x = *a; return (x >> 8) | (x << 8);}
static inline void do_put_mem_long(uae_u32 *a, uae_u32 v) {*a = (v >> 24) | (v >> 8) & 0xff00 | (v << 8) & 0xff0000 | (v << 24);}
static inline void do_put_mem_word(uae_u16 *a, uae_u32 v) {*a = (v >> 8) | (v << 8);}

#else /* CPU_CAN_ACCESS_UNALIGNED */

/* Other little-endian CPUs which can not do unaligned accesses (this needs optimization) */
static inline uae_u32 do_get_mem_long(uae_u32 *a) {uint8 *b = (uint8 *)a; return (b[0] << 24) | (b[1] << 16) | (b[2] << 8) | b[3];}
static inline uae_u32 do_get_mem_word(uae_u16 *a) {uint8 *b = (uint8 *)a; return (b[0] << 8) | b[1];}
static inline void do_put_mem_long(uae_u32 *a, uae_u32 v) {uint8 *b = (uint8 *)a; b[0] = v >> 24; b[1] = v >> 16; b[2] = v >> 8; b[3] = v;}
static inline void do_put_mem_word(uae_u16 *a, uae_u32 v) {uint8 *b = (uint8 *)a; b[0] = v >> 8; b[1] = v;}

#endif /* CPU_CAN_ACCESS_UNALIGNED */

#endif /* WORDS_BIGENDIAN */

#ifndef HAVE_OPTIMIZED_BYTESWAP_32
static inline uae_u32 do_byteswap_32(uae_u32 v)
	{ return (((v >> 24) & 0xff) | ((v >> 8) & 0xff00) | ((v & 0xff) << 24) | ((v & 0xff00) << 8)); }
#endif

#ifndef HAVE_OPTIMIZED_BYTESWAP_16
static inline uae_u32 do_byteswap_16(uae_u32 v)
	{ return (((v >> 8) & 0xff) | ((v & 0xff) << 8)); }
#endif

#define do_get_mem_byte(a) ((uae_u32)*((uae_u8 *)(a)))
#define do_put_mem_byte(a, v) (*(uae_u8 *)(a) = (v))

#define call_mem_get_func(func, addr) ((*func)(addr))
#define call_mem_put_func(func, addr, v) ((*func)(addr, v))
#define __inline__ inline
#define CPU_EMU_SIZE 0
#undef NO_INLINE_MEMORY_ACCESS
#undef MD_HAVE_MEM_1_FUNCS
#define write_log printf

#ifndef ASM_VOLATILE
#define ASM_VOLATILE __asm__ __volatile__
#endif

#ifdef X86_ASSEMBLY
#define ASM_SYM_FOR_FUNC(a) __asm__(a)
#else
#define ASM_SYM_FOR_FUNC(a)
#endif

#ifndef REGPARAM
# define REGPARAM
#endif
#define REGPARAM2

#undef NOT_MALLOC

#if REAL_ADDRESSING
# define NOT_MALLOC
#endif

#undef KNOWN_ALLOC

#if REAL_ADDRESSING || DIRECT_ADDRESSING || FIXED_ADDRESSING
# define KNOWN_ALLOC	1
#else
# define KNOWN_ALLOC	0
#endif

#ifndef HAVE_STRDUP
extern "C" char *strdup(const char *s);
#endif

#endif
