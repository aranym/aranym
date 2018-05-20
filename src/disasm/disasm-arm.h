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

#ifndef __DISASM_ARM_H__
#define __DISASM_ARM_H__ 1

#ifdef __cplusplus
extern "C" {
#endif

#include "disasm-bfd.h"
#include "arm.h"

/* Cached mapping symbol state. */
enum map_type
{
	MAP_ARM,
	MAP_THUMB,
	MAP_DATA
};

struct opcode32
{
	arm_feature_set arch;				/* Architecture defining this insn. */
	uint32_t value;						/* If arch is 0 then value is a sentinel. */
	uint32_t mask;						/* Recognise insn if (op & mask) == value. */
	const char *assembler;				/* How to disassemble this insn. */
};

struct opcode16
{
	arm_feature_set arch;				/* Architecture defining this insn. */
	uint16_t value, mask;				/* Recognise insn if (op & mask) == value. */
	const char *assembler;				/* How to disassemble this insn. */
};


struct disassemble_info {
	fprintf_ftype fprintf_func;
	void *stream;

	/* Function used to get bytes to disassemble.  MEMADDR is the
	   address of the stuff to be disassembled, MYADDR is the address to
	   put the bytes in, and LENGTH is the number of bytes to read.
	   INFO is a pointer to this struct.
	   Returns an errno value or 0 for success. */
	int (*read_memory_func) (bfd_vma memaddr, bfd_byte *myaddr, size_t length, disassemble_info *dinfo);

	/* Function which should be called if we get an error that we can't
	   recover from.  STATUS is the errno value from read_memory_func and
	   MEMADDR is the address that we were trying to read.  INFO is a
	   pointer to this struct. */
	void (*memory_error_func) (int status, bfd_vma memaddr, disassemble_info *dinfo);

	/* Function called to print ADDR. */
	void (*print_address_func) (bfd_vma addr, disassemble_info *dinfo);

	/* Function called to determine if there is a symbol at the given ADDR.
	   If there is, the function returns 1, otherwise it returns 0.
	   This is used by ports which support an overlay manager where
	   the overlay number is held in the top part of an address.  In
	   some circumstances we want to include the overlay number in the
	   address, (normally because there is a symbol associated with
	   that address), but sometimes we want to mask out the overlay bits. */
	int (*symbol_at_address_func) (bfd_vma addr, struct disassemble_info *dinfo);

	/* These are for buffer_read_memory. */
	const bfd_byte *buffer;
	bfd_vma buffer_vma;
	size_t buffer_length;

	/* This variable may be set by the instruction decoder.  It suggests
	    the number of bytes objdump should display on a single line.  If
	    the instruction decoder sets this, it should always set it to
	    the same value in order to get reasonable looking output. */
	int bytes_per_line;

	/* The next two variables control the way objdump displays the raw data. */
	/* For example, if bytes_per_line is 8 and bytes_per_chunk is 4, the */
	/* output will look like this:
	   00:   00000000 00000000
	   with the chunks displayed according to "display_endian". */
	int bytes_per_chunk;
	enum bfd_endian display_endian;

	/* Command line options specific to the target disassembler. */
	const char *disassembler_options;

	/* Symbol table provided for targets that want to look at it.  This is
	   used on Arm to find mapping symbols and determine Arm/Thumb code. */
	int symtab_pos;
	int symtab_size;

	/* For use by the disassembler.
	   The top 16 bits are reserved for public use (and are documented here).
	   The bottom 16 bits are for the internal use of the disassembler. */
	unsigned long flags;
	/* Set if the disassembler has determined that there are one or more
	   relocations associated with the instruction being disassembled. */
#define INSN_HAS_RELOC	 (1 << 31)
	/* Set if the user has requested the disassembly of data as well as code. */
#define DISASSEMBLE_DATA (1 << 30)
	/* Set if the user has specifically set the machine type encoded in the
	   mach field of this structure. */
#define USER_SPECIFIED_MACHINE_TYPE (1 << 29)

	/* If non-zero then try not disassemble beyond this address, even if
	   there are values left in the buffer.  This address is the address
	   of the nearest symbol forwards from the start of the disassembly,
	   and it is assumed that it lies on the boundary between instructions.
	   If an instruction spans this address then this is an error in the
	   file being disassembled. */
	bfd_vma stop_vma;

	struct
	{
		/* The features to use when disassembling optional instructions. */
		arm_feature_set features;
	
		/* Whether any mapping symbols are present in the provided symbol
		  table.  -1 if we do not know yet, otherwise 0 or 1. */
		int has_mapping_symbols;
	
		/* Track the last type (although this doesn't seem to be useful) */
		enum map_type last_type;
	
		/* Tracking symbol table information */
		int last_mapping_sym;
		bfd_vma last_mapping_addr;
	} priv;

	unsigned int regname_selected;

	bfd_boolean force_thumb;

	/* Current IT instruction state.  This contains the same state as the IT
	   bits in the CPSR. */
	unsigned int ifthen_state;

	/* IT state for the next instruction. */
	unsigned int ifthen_next_state;
	
	/* The address of the insn for which the IT state is valid. */
	bfd_vma ifthen_address;
	
#define IFTHEN_COND ((info->ifthen_state >> 4) & 0xf)
	/* Indicates that the current Conditional state is unconditional or outside
	   an IT block. */
#define COND_UNCOND 16


};

#ifdef __cplusplus
}
#endif

#endif /* __DISASM_ARM_H__ */
