/* Interface between the opcode library and its callers.

   Copyright (C) 1999-2015 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street - Fifth Floor,
   Boston, MA 02110-1301, USA.

   Written by Cygnus Support, 1993.

   The opcode library (libopcodes.a) provides instruction decoders for
   a large variety of instruction sets, callable with an identical
   interface, for making instruction-processing programs more independent
   of the instruction set being processed.  */

#ifndef __DISASM_X86_H__
#define __DISASM_X86_H__ 1

#ifdef __cplusplus
extern "C" {
#endif

#include "disasm-bfd.h"
#include "i386.h"

#if defined(HAVE_SIGSETJMP)
#define OPCODES_SIGJMP_BUF		sigjmp_buf
#define OPCODES_SIGSETJMP(buf)		sigsetjmp((buf), 0)
#define OPCODES_SIGLONGJMP(buf,val)	siglongjmp((buf), (val))
#else
#define OPCODES_SIGJMP_BUF		jmp_buf
#define OPCODES_SIGSETJMP(buf)		setjmp(buf)
#define OPCODES_SIGLONGJMP(buf,val)	longjmp((buf), (val))
#endif


enum address_mode
{
	mode_16bit,
	mode_32bit,
	mode_64bit
};

/* This struct is passed into the instruction decoding routine,
   and is passed back out into each callback.  The various fields are used
   for conveying information from your main routine into your callbacks,
   for passing information into the instruction decoders (such as the
   addresses of the callback functions), or for passing information
   back from the instruction decoders to their callers.

   It must be initialized before it is first passed; this can be done
   by hand, or using one of the initialization macros below.  */

enum x86_64_isa
{
	amd64 = 0,
	intel64
};

#define MAX_CODE_LENGTH 15

struct disassemble_info {
	fprintf_ftype fprintf_func;
	void *stream;

	/* For use by the disassembler.
	   The top 16 bits are reserved for public use (and are documented here).
	   The bottom 16 bits are for the internal use of the disassembler.  */
	unsigned long flags;
	/* Set if the disassembler has determined that there are one or more
	   relocations associated with the instruction being disassembled.  */
#define INSN_HAS_RELOC	 (1 << 31)
	/* Set if the user has requested the disassembly of data as well as code.  */
#define DISASSEMBLE_DATA (1 << 30)
	/* Set if the user has specifically set the machine type encoded in the
	   mach field of this structure.  */
#define USER_SPECIFIED_MACHINE_TYPE (1 << 29)

	/* Function used to get bytes to disassemble.  MEMADDR is the
	   address of the stuff to be disassembled, MYADDR is the address to
	   put the bytes in, and LENGTH is the number of bytes to read.
	   INFO is a pointer to this struct.
	   Returns an errno value or 0 for success.  */
	int (*read_memory_func) (bfd_vma memaddr, bfd_byte *myaddr, size_t length, disassemble_info *dinfo);

	/* Function which should be called if we get an error that we can't
	   recover from.  STATUS is the errno value from read_memory_func and
	   MEMADDR is the address that we were trying to read.  INFO is a
	   pointer to this struct.  */
	void (*memory_error_func) (int status, bfd_vma memaddr, disassemble_info *dinfo);

	/* Function called to print ADDR.  */
	void (*print_address_func) (bfd_vma addr, disassemble_info *dinfo);

	/* These are for buffer_read_memory.  */
	bfd_byte *buffer;
	bfd_vma buffer_vma;
	size_t buffer_length;

	/* This variable may be set by the instruction decoder.  It suggests
	    the number of bytes objdump should display on a single line.  If
	    the instruction decoder sets this, it should always set it to
	    the same value in order to get reasonable looking output.  */
	int bytes_per_line;

	/* Results from instruction decoders.  Not all decoders yet support
	   this information.  This info is set each time an instruction is
	   decoded, and is only valid for the last such instruction.

	   To determine whether this decoder supports this information, set
	   insn_info_valid to 0, decode an instruction, then check it.  */

	char insn_info_valid;		/* Branch info has been set. */
	char branch_delay_insns;	/* How many sequential insn's will run before
				   a branch takes effect.  (0 = normal) */
	char data_size;		/* Size of data reference in insn, in bytes */
	enum dis_insn_type insn_type;	/* Type of instruction */
	bfd_vma target;		/* Target address of branch or dref, if known;
				   zero if unknown.  */
	bfd_vma target2;		/* Second target address for dref2 */

	/* Command line options specific to the target disassembler.  */
	const char *disassembler_options;

	/* If non-zero then try not disassemble beyond this address, even if
	   there are values left in the buffer.  This address is the address
	   of the nearest symbol forwards from the start of the disassembly,
	   and it is assumed that it lies on the boundary between instructions.
	   If an instruction spans this address then this is an error in the
	   file being disassembled.  */
	bfd_vma stop_vma;

	struct
	{
		/* Points to first byte not fetched.  */
		bfd_byte *max_fetched;
		bfd_byte the_buffer[MAX_MNEM_SIZE];
		bfd_vma insn_start;
		int orig_sizeflag;
		OPCODES_SIGJMP_BUF bailout;
	} dis_private;

	char *obufp;
	char obuf[100];

	/* Flags for the prefixes for the current instruction.  See below.  */
	int prefixes;

	/* Flags for prefixes which we somehow handled when printing the
	   current instruction.  */
	int used_prefixes;

	enum address_mode address_mode;

	/* REX prefix the current instruction.  See below.  */
	int rex;

	/* Bits of REX we've already used.  */
	int rex_used;

	/* REX bits in original REX prefix ignored.  */
	int rex_ignored;

	char op_out[MAX_OPERANDS][100];

	int op_ad, op_index[MAX_OPERANDS];

	int two_source_ops;

	bfd_vma op_address[MAX_OPERANDS];

	int op_riprel[MAX_OPERANDS];

	char *mnemonicendp;

	bfd_vma start_pc;

	char intel_syntax;

	char intel_mnemonic;

	char open_char;

	char close_char;

	char separator_char;

	char scale_char;

	unsigned char *start_codep;

	unsigned char *insn_codep;

	unsigned char *codep;

	unsigned char *end_codep;

	int last_lock_prefix;
	int last_repz_prefix;
	int last_repnz_prefix;
	int last_data_prefix;
	int last_addr_prefix;
	int last_rex_prefix;
	int last_seg_prefix;
	int fwait_prefix;

	/* The active segment register prefix.  */
	int active_seg_prefix;

	/* We can up to 14 prefixes since the maximum instruction length is
	   15bytes.  */
	int all_prefixes[MAX_CODE_LENGTH - 1];

	enum x86_64_isa isa64;

	const char *const *names64;
	const char *const *names32;
	const char *const *names16;
	const char *const *names8;
	const char *const *names8rex;
	const char *const *names_seg;
	const char *index64;
	const char *index32;
	const char *const *index16;
	const char *const *names_mask;
	const char *const *names_mm;
	const char *const *names_bnd;
	const char *const *names_xmm;
	const char *const *names_ymm;
	const char *const *names_zmm;

	struct
	{
		int mod;
		int reg;
		int rm;
	} modrm;
	unsigned char need_modrm;

	struct
	{
		int scale;
		int index;
		int base;
	} sib;
	
	struct
	{
		int register_specifier;
		int length;
		int prefix;
		int w;
		int evex;
		int r;
		int v;
		int mask_register_specifier;
		int zeroing;
		int ll;
		int b;
	} vex;
	
	unsigned char need_vex;
	unsigned char need_vex_reg;
	unsigned char vex_w_done;
	unsigned char vex_imm8;

};


#ifdef __cplusplus
}
#endif

#endif /* __DISASM_X86_H__ */
