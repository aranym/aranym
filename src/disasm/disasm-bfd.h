/* Main header file for the bfd library -- portable access to object files.

   Copyright (C) 1990-2015 Free Software Foundation, Inc.

   Contributed by Cygnus Support.

   This file is part of BFD, the Binary File Descriptor library.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston, MA 02110-1301, USA.  */

#ifndef __BFD_H_SEEN__
#define __BFD_H_SEEN__ 1

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The word size used by BFD on the host.  This may be 64 with a 32
   bit target if the host is 64 bit, or if other 64 bit targets have
   been selected with --enable-targets, or if --enable-64-bit-bfd.  */
#define BFD_ARCH_SIZE 64

/* The word size of the default bfd target.  */
#define BFD_DEFAULT_TARGET_SIZE 64

/* Boolean type used in bfd.  Too many systems define their own
   versions of "boolean" for us to safely typedef a "boolean" of
   our own.  Using an enum for "bfd_boolean" has its own set of
   problems, with strange looking casts required to avoid warnings
   on some older compilers.  Thus we just use an int.

   General rule: Functions which are bfd_boolean return TRUE on
   success and FALSE on failure (unless they're a predicate).  */

typedef int bfd_boolean;
#ifndef FALSE
#define FALSE 0
#define TRUE 1
#endif

#if BFD_ARCH_SIZE >= 64

typedef uint64_t bfd_vma;
typedef int64_t bfd_signed_vma;

#else

/* Represent a target address.  Also used as a generic unsigned type
   which is guaranteed to be big enough to hold any arithmetic types
   we need to deal with.  */
typedef uint32_t bfd_vma;

/* A generic signed type which is guaranteed to be big enough to hold any
   arithmetic types we need to deal with.  Can be assumed to be compatible
   with bfd_vma in the same way that signed and unsigned ints are compatible
   (as parameters, in assignment, etc).  */
typedef int32_t bfd_signed_vma;

#endif

typedef unsigned int flagword;	/* 32 bits of flags */
typedef unsigned char bfd_byte;

enum bfd_endian { BFD_ENDIAN_BIG, BFD_ENDIAN_LITTLE, BFD_ENDIAN_UNKNOWN };

typedef int (*fprintf_ftype) (void *, const char*, ...) __attribute__((format(printf, 2, 3)));

enum dis_insn_type
{
	dis_noninsn,		/* Not a valid instruction.  */
	dis_nonbranch,		/* Not a branch instruction.  */
	dis_branch,			/* Unconditional branch.  */
	dis_condbranch,		/* Conditional branch.  */
	dis_jsr,			/* Jump to subroutine.  */
	dis_condjsr,		/* Conditional jump to subroutine.  */
	dis_dref,			/* Data reference instruction.  */
	dis_dref2			/* Two data references in instruction.  */
};

typedef struct disassemble_info disassemble_info;

#ifdef __cplusplus
}
#endif

#endif /* __BFD_H_SEEN__ */
