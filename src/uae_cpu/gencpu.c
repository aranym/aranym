/*
 * gencpu.c - m68k emulation generator
 *
 * Copyright (c) 2009 ARAnyM dev team (see AUTHORS)
 * 
 * Inspired by Christian Bauer's Basilisk II
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
/*
 * UAE - The Un*x Amiga Emulator
 *
 * MC68000 emulation generator
 *
 * This is a fairly stupid program that generates a lot of case labels that
 * can be #included in a switch statement.
 * As an alternative, it can generate functions that handle specific
 * MC68000 instructions, plus a prototype header file and a function pointer
 * array to look up the function for an opcode.
 * Error checking is bad, an illegal table68k file will cause the program to
 * call abort().
 * The generated code is sometimes sub-optimal, an optimizing compiler should
 * take care of this.
 *
 * Copyright 1995, 1996 Bernd Schmidt
 */

#define CC_FOR_BUILD 1

#include "sysdeps.h"
#include "readcpu.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#undef abort

#define BOOL_TYPE "int"
#define VERIFY_MMU_GENAMODE	0

static FILE *headerfile;
static FILE *stblfile;
static FILE *functblfile;

static int using_prefetch;
static int using_exception_3;
static int cpu_level;

/* For the current opcode, the next lower level that will have different code.
 * Initialized to -1 for each opcode. If it remains unchanged, indicates we
 * are done with that opcode.  */
static int next_cpu_level;

static int *opcode_map;
static int *opcode_next_clev;
static int *opcode_last_postfix;
static unsigned long *counts;

#define GENA_GETV_NO_FETCH	0
#define GENA_GETV_FETCH		1
#define GENA_GETV_FETCH_ALIGN 2
#define GENA_MOVEM_DO_INC	0
#define GENA_MOVEM_NO_INC	1
#define GENA_MOVEM_MOVE16	2

#define XLATE_LOG	0
#define XLATE_PHYS	1
#define XLATE_SFC	2
#define XLATE_DFC	3
static char * mem_prefix[4] = { "", "phys_", "sfc_", "dfc_" };

/* Define the minimal 680x0 where NV flags are not affected by xBCD instructions.  */
#define xBCD_KEEPS_N_FLAG 4
#define xBCD_KEEPS_V_FLAG 3

static void read_counts (void)
{
    FILE *file;
    unsigned long opcode, count, total;
    char name[20];
    int nr = 0;
    memset (counts, 0, 65536 * sizeof *counts);

    file = fopen ("frequent.68k", "r");
    if (file) {
	int c = fscanf (file, "Total: %lu\n", &total);
	assert(c == 1);
	while (fscanf (file, "%lx: %lu %s\n", &opcode, &count, name) == 3) {
	    opcode_next_clev[nr] = 4;
	    opcode_last_postfix[nr] = -1;
	    opcode_map[nr++] = opcode;
	    counts[opcode] = count;
	}
	fclose (file);
    }
    if (nr == nr_cpuop_funcs)
	return;
    for (opcode = 0; opcode < 0x10000; opcode++) {
	if (table68k[opcode].handler == -1 && table68k[opcode].mnemo != i_ILLG
	    && counts[opcode] == 0)
	{
	    opcode_next_clev[nr] = 4;
	    opcode_last_postfix[nr] = -1;
	    opcode_map[nr++] = opcode;
	    counts[opcode] = count;
	}
    }
    if (nr != nr_cpuop_funcs)
	abort ();
}

static char endlabelstr[80];
static int endlabelno = 0;
static int need_endlabel;

static int n_braces = 0;
static int m68k_pc_offset = 0;

static void start_brace (void)
{
    n_braces++;
    printf ("{");
}

static void close_brace (void)
{
    assert (n_braces > 0);
    n_braces--;
    printf ("}");
}

static void finish_braces (void)
{
    while (n_braces > 0)
	close_brace ();
}

static void pop_braces (int to)
{
    while (n_braces > to)
	close_brace ();
}

static int bit_size (int size)
{
    switch (size) {
     case sz_byte: return 8;
     case sz_word: return 16;
     case sz_long: return 32;
     default: abort ();
    }
    return 0;
}

static const char *bit_mask (int size)
{
    switch (size) {
     case sz_byte: return "0xff";
     case sz_word: return "0xffff";
     case sz_long: return "0xffffffff";
     default: abort ();
    }
    return 0;
}

static const char *gen_nextilong (void)
{
    static char buffer[80];
    int r = m68k_pc_offset;

    m68k_pc_offset += 4;

    if (using_prefetch)
	sprintf (buffer, "get_ilong_prefetch(%d)", r);
    else
	sprintf (buffer, "get_ilong(%d)", r);
    return buffer;
}

static const char *gen_nextiword (void)
{
    static char buffer[80];
    int r = m68k_pc_offset;

    m68k_pc_offset += 2;

    if (using_prefetch)
	sprintf (buffer, "get_iword_prefetch(%d)", r);
    else
	sprintf (buffer, "get_iword(%d)", r);
    return buffer;
}

static const char *gen_nextibyte (void)
{
    static char buffer[80];
    int r = m68k_pc_offset;
    m68k_pc_offset += 2;

    if (using_prefetch)
	sprintf (buffer, "get_ibyte_prefetch(%d)", r);
    else
	sprintf (buffer, "get_ibyte(%d)", r);
    return buffer;
}

static void fill_prefetch_0 (void)
{
    if (using_prefetch)
	printf ("fill_prefetch_0 ();\n");
}

static void fill_prefetch_2 (void)
{
    if (using_prefetch)
	printf ("fill_prefetch_2 ();\n");
}

static void swap_opcode (void)
{
  printf("#if defined(HAVE_GET_WORD_UNSWAPPED) && !defined(FULLMMU)\n");
  printf ("\topcode = do_byteswap_16(opcode);\n");
  printf("#endif\n");
}

static void real_opcode (int *have)
{
	if (!*have)
	{
		printf("#if defined(HAVE_GET_WORD_UNSWAPPED) && !defined(FULLMMU)\n");
		printf ("\tuae_u32 real_opcode = do_byteswap_16(opcode);\n");
		printf("#else\n");
		printf ("\tuae_u32 real_opcode = opcode;\n");
		printf("#endif\n");
		*have = 1;
	}
}

static void sync_m68k_pc (void)
{
    if (m68k_pc_offset == 0)
	return;
    printf ("m68k_incpc(%d);\n", m68k_pc_offset);
    switch (m68k_pc_offset) {
     case 0:
	/*fprintf (stderr, "refilling prefetch at 0\n"); */
	break;
     case 2:
	fill_prefetch_2 ();
	break;
     default:
	fill_prefetch_0 ();
	break;
    }
    m68k_pc_offset = 0;
}

static void gen_set_fault_pc (void)
{
    sync_m68k_pc();
    printf ("regs.fault_pc = m68k_getpc ();\n");
    m68k_pc_offset = 0;
}

/* getv == 1: fetch data; getv != 0: check for odd address. If movem != 0,
 * the calling routine handles Apdi and Aipi modes.
 * gb-- movem == 2 means the same thing but for a MOVE16 instruction */

/* fixup indicates if we want to fix up adress registers in pre decrement
 * or post increment mode now (0) or later (1). A value of 2 will then be
 * used to do the actual fix up. This allows to do all memory readings
 * before any register is modified, and so to rerun operation without
 * side effect in case a bus fault is generated by any memory access.
 * XJ - 2006/11/13 */
static void genamode2 (amodes mode, char *reg, wordsizes size, char *name, int getv, int movem, int xlateflag, int fixup)
{
    if (fixup != 2)
    {
    start_brace ();
    switch (mode) {
     case Dreg:
	if (movem)
	    abort ();
	if (getv == GENA_GETV_FETCH)
	    switch (size) {
	     case sz_byte:
		printf("\n#if defined(AMIGA) && !defined(WARPUP)\n");
		/* sam: I don't know why gcc.2.7.2.1 produces a code worse */
		/* if it is not done like that: */
		printf ("\tuae_s8 %s = ((uae_u8*)&m68k_dreg(regs, %s))[3];\n", name, reg);
		printf("#else\n");
		printf ("\tuae_s8 %s = m68k_dreg(regs, %s);\n", name, reg);
		printf("#endif\n");
		break;
	     case sz_word:
		printf("\n#if defined(AMIGA) && !defined(WARPUP)\n");
		printf ("\tuae_s16 %s = ((uae_s16*)&m68k_dreg(regs, %s))[1];\n", name, reg);
		printf("#else\n");
		printf ("\tuae_s16 %s = m68k_dreg(regs, %s);\n", name, reg);
		printf("#endif\n");
		break;
	     case sz_long:
		printf ("\tuae_s32 %s = m68k_dreg(regs, %s);\n", name, reg);
		break;
	     default:
		abort ();
	    }
	return;
     case Areg:
	if (movem)
	    abort ();
	if (getv == GENA_GETV_FETCH)
	    switch (size) {
	     case sz_word:
		printf ("\tuae_s16 %s = m68k_areg(regs, %s);\n", name, reg);
		break;
	     case sz_long:
		printf ("\tuae_s32 %s = m68k_areg(regs, %s);\n", name, reg);
		break;
	     default:
		abort ();
	    }
	return;
     case Aind:
	printf ("\tuaecptr %sa = m68k_areg(regs, %s);\n", name, reg);
	break;
     case Aipi:
	printf ("\tuaecptr %sa = m68k_areg(regs, %s);\n", name, reg);
	break;
     case Apdi:
	switch (size) {
	 case sz_byte:
	    if (movem)
		printf ("\tuaecptr %sa = m68k_areg(regs, %s);\n", name, reg);
	    else
		printf ("\tuaecptr %sa = m68k_areg(regs, %s) - areg_byteinc[%s];\n", name, reg, reg);
	    break;
	 case sz_word:
	    if (movem)
		printf ("\tuaecptr %sa = m68k_areg(regs, %s);\n", name, reg);
	    else
		printf ("\tuaecptr %sa = m68k_areg(regs, %s) - 2;\n", name, reg);
	    break;
	 case sz_long:
	    if (movem)
		printf ("\tuaecptr %sa = m68k_areg(regs, %s);\n", name, reg);
	    else
		printf ("\tuaecptr %sa = m68k_areg(regs, %s) - 4;\n", name, reg);
	    break;
	 default:
	    abort ();
	}
	break;
     case Ad16:
	printf ("\tuaecptr %sa = m68k_areg(regs, %s) + (uae_s32)(uae_s16)%s;\n", name, reg, gen_nextiword ());
	break;
     case Ad8r:
	if (cpu_level > 1) {
	    if (next_cpu_level < 1)
		next_cpu_level = 1;
	    sync_m68k_pc ();
	    start_brace ();
	    printf ("\tuaecptr %sa = get_disp_ea_020(m68k_areg(regs, %s), next_iword());\n", name, reg);
	} else
	    printf ("\tuaecptr %sa = get_disp_ea_000(m68k_areg(regs, %s), %s);\n", name, reg, gen_nextiword ());

	break;
     case PC16:
	printf ("\tuaecptr %sa = m68k_getpc () + %d;\n", name, m68k_pc_offset);
	printf ("\t%sa += (uae_s32)(uae_s16)%s;\n", name, gen_nextiword ());
	break;
     case PC8r:
	if (cpu_level > 1) {
	    if (next_cpu_level < 1)
		next_cpu_level = 1;
	    sync_m68k_pc ();
	    start_brace ();
	    printf ("\tuaecptr tmppc = m68k_getpc();\n");
	    printf ("\tuaecptr %sa = get_disp_ea_020(tmppc, next_iword());\n", name);
	} else {
	    printf ("\tuaecptr tmppc = m68k_getpc() + %d;\n", m68k_pc_offset);
	    printf ("\tuaecptr %sa = get_disp_ea_000(tmppc, %s);\n", name, gen_nextiword ());
	}

	break;
     case absw:
	printf ("\tuaecptr %sa = (uae_s32)(uae_s16)%s;\n", name, gen_nextiword ());
	break;
     case absl:
	printf ("\tuaecptr %sa = %s;\n", name, gen_nextilong ());
	break;
     case imm:
	if (getv != GENA_GETV_FETCH)
	    abort ();
	switch (size) {
	 case sz_byte:
	    printf ("\tuae_s8 %s = %s;\n", name, gen_nextibyte ());
	    break;
	 case sz_word:
	    printf ("\tuae_s16 %s = %s;\n", name, gen_nextiword ());
	    break;
	 case sz_long:
	    printf ("\tuae_s32 %s = %s;\n", name, gen_nextilong ());
	    break;
	 default:
	    abort ();
	}
	return;
     case imm0:
	if (getv != GENA_GETV_FETCH)
	    abort ();
	printf ("\tuae_s8 %s = %s;\n", name, gen_nextibyte ());
	return;
     case imm1:
	if (getv != GENA_GETV_FETCH)
	    abort ();
	printf ("\tuae_s16 %s = %s;\n", name, gen_nextiword ());
	return;
     case imm2:
	if (getv != GENA_GETV_FETCH)
	    abort ();
	printf ("\tuae_s32 %s = %s;\n", name, gen_nextilong ());
	return;
     case immi:
	if (getv != GENA_GETV_FETCH)
	    abort ();
	printf ("\tuae_u32 %s = %s;\n", name, reg);
	return;
     default:
	abort ();
    }

    /* We get here for all non-reg non-immediate addressing modes to
     * actually fetch the value. */

    if (using_exception_3 && getv != GENA_GETV_NO_FETCH && size != sz_byte) {	    
	printf ("\tif ((%sa & 1) != 0) {\n", name);
	printf ("\t\tlast_fault_for_exception_3 = %sa;\n", name);
	printf ("\t\tlast_op_for_exception_3 = opcode;\n");
	printf ("\t\tlast_addr_for_exception_3 = m68k_getpc() + %d;\n", m68k_pc_offset);
	printf ("\t\tException(3, 0);\n");
	printf ("\t\tgoto %s;\n", endlabelstr);
	printf ("\t}\n");
	need_endlabel = 1;
	start_brace ();
    }

    if (getv == GENA_GETV_FETCH) {
	switch (size) {
	 case sz_byte: break;
	 case sz_word: break;
	 case sz_long: break;
	 default: abort ();
	}
	start_brace ();
	printf("\n#ifdef FULLMMU\n");
	switch (size) {
	case sz_byte: printf ("\tuae_s8 %s = %sget_byte(%sa);\n", name, mem_prefix[xlateflag], name); break;
	case sz_word: printf ("\tuae_s16 %s = %sget_word(%sa);\n", name, mem_prefix[xlateflag], name); break;
	case sz_long: printf ("\tuae_s32 %s = %sget_long(%sa);\n", name, mem_prefix[xlateflag], name); break;
	 default: abort ();
	}
	printf("#else\n");
	switch (size) {
	case sz_byte: printf ("\tuae_s8 %s = phys_get_byte(%sa);\n", name, name); break;
	case sz_word: printf ("\tuae_s16 %s = phys_get_word(%sa);\n", name, name); break;
	case sz_long: printf ("\tuae_s32 %s = phys_get_long(%sa);\n", name, name); break;
	 default: abort ();
	}
	printf("#endif\n");
    }

    /* We now might have to fix up the register for pre-dec or post-inc
     * addressing modes. */
    if (!movem)
	switch (mode) {
	 case Aipi:
	    if (fixup == 1)
	    {
		printf ("\tfixup.flag = 1;\n");
		printf ("\tfixup.reg = %s;\n", reg);
		printf ("\tfixup.value = m68k_areg(regs, %s);\n", reg);
	    }
	    switch (size) {
	     case sz_byte:
		printf ("\tm68k_areg(regs, %s) += areg_byteinc[%s];\n", reg, reg);
		break;
	     case sz_word:
		printf ("\tm68k_areg(regs, %s) += 2;\n", reg);
		break;
	     case sz_long:
		printf ("\tm68k_areg(regs, %s) += 4;\n", reg);
		break;
	     default:
		abort ();
	    }
	    break;
	 case Apdi:
	    if (fixup == 1)
	    {
		printf ("\tfixup.flag = 1;\n");
		printf ("\tfixup.reg = %s;\n", reg);
		printf ("\tfixup.value = m68k_areg(regs, %s);\n", reg);
	    }
	    printf ("\tm68k_areg (regs, %s) = %sa;\n", reg, name);
	    break;
	 default:
	    break;
	}

    }
    else /* (fixup != 2) */
    {
    if (!movem)
	switch (mode) {
	 case Aipi:
	 case Apdi:
	    printf ("\tfixup.flag = 0;\n");
	    break;
	 default:
	    break;
	}
    }
}

static void genamode (amodes mode, char *reg, wordsizes size, char *name, int getv, int movem, int xlateflag)
{
	genamode2 (mode, reg, size, name, getv, movem, xlateflag, 0);
}

static void genastore (char *from, amodes mode, char *reg, wordsizes size, char *to, int xlateflag)
{
    switch (mode) {
     case Dreg:
	switch (size) {
	 case sz_byte:
	    printf ("\tm68k_dreg(regs, %s) = (m68k_dreg(regs, %s) & ~0xff) | ((%s) & 0xff);\n", reg, reg, from);
	    break;
	 case sz_word:
	    printf ("\tm68k_dreg(regs, %s) = (m68k_dreg(regs, %s) & ~0xffff) | ((%s) & 0xffff);\n", reg, reg, from);
	    break;
	 case sz_long:
	    printf ("\tm68k_dreg(regs, %s) = (%s);\n", reg, from);
	    break;
	 default:
	    abort ();
	}
	break;
     case Areg:
	switch (size) {
	 case sz_word:
	    fprintf (stderr, "Foo\n");
	    printf ("\tm68k_areg(regs, %s) = (uae_s32)(uae_s16)(%s);\n", reg, from);
	    break;
	 case sz_long:
	    printf ("\tm68k_areg(regs, %s) = (%s);\n", reg, from);
	    break;
	 default:
	    abort ();
	}
	break;
     case Aind:
     case Aipi:
     case Apdi:
     case Ad16:
     case Ad8r:
     case absw:
     case absl:
     case PC16:
     case PC8r:
	gen_set_fault_pc ();
	printf("#ifdef FULLMMU\n");
	switch (size) {
	 case sz_byte:
	    printf ("\t%sput_byte(%sa,%s);\n", mem_prefix[xlateflag], to, from);
	    printf("#else\n");
	    printf ("\tput_byte(%sa,%s);\n", to, from);
	    break;
	 case sz_word:
	    if (cpu_level < 2 && (mode == PC16 || mode == PC8r))
		abort ();
	    printf ("\t%sput_word(%sa,%s);\n", mem_prefix[xlateflag], to, from);
	    printf("#else\n");
	    printf ("\tput_word(%sa,%s);\n", to, from);
	    break;
	 case sz_long:
	    if (cpu_level < 2 && (mode == PC16 || mode == PC8r))
		abort ();
	    printf ("\t%sput_long(%sa,%s);\n", mem_prefix[xlateflag], to, from);
	    printf("#else\n");
	    printf ("\tput_long(%sa,%s);\n", to, from);
	    break;
	 default:
	    abort ();
	}
	printf("#endif\n");
	break;
     case imm:
     case imm0:
     case imm1:
     case imm2:
     case immi:
	abort ();
	break;
     default:
	abort ();
    }
}

static void genmovemel (uae_u16 opcode)
{
    char getcode1[100];
    char getcode2[100];
    int size = table68k[opcode].size == sz_long ? 4 : 2;
	
    if (table68k[opcode].size == sz_long) {
		strcpy (getcode1, "");
		strcpy (getcode2, "get_long(srca)");
    } else {
		strcpy (getcode1, "(uae_s32)(uae_s16)");
		strcpy (getcode2, "get_word(srca)");
    }

    printf ("\tuae_u16 mask = %s;\n", gen_nextiword ());
    printf ("\tunsigned int dmask = mask & 0xff, amask = (mask >> 8) & 0xff;\n");
    genamode (table68k[opcode].dmode, "dstreg", table68k[opcode].size, "src", GENA_GETV_FETCH_ALIGN, GENA_MOVEM_NO_INC, XLATE_LOG);
    start_brace ();
    printf("\n#ifdef FULLMMU\n");
    printf ("\twhile (dmask) { m68k_dreg(regs, movem_index1[dmask]) = %s%s; srca += %d; dmask = movem_next[dmask]; }\n",
	    getcode1, getcode2, size);
    printf ("\twhile (amask) { m68k_areg(regs, movem_index1[amask]) = %s%s; srca += %d; amask = movem_next[amask]; }\n",
	    getcode1, getcode2, size);
    printf("#else\n");
    printf ("\twhile (dmask) { m68k_dreg(regs, movem_index1[dmask]) = %sphys_%s; srca += %d; dmask = movem_next[dmask]; }\n",
	    getcode1, getcode2, size);
    printf ("\twhile (amask) { m68k_areg(regs, movem_index1[amask]) = %sphys_%s; srca += %d; amask = movem_next[amask]; }\n",
	    getcode1, getcode2, size);
    printf("#endif\n");

    if (table68k[opcode].dmode == Aipi)
	printf ("\tm68k_areg(regs, dstreg) = srca;\n");
}

static void genmovemle (uae_u16 opcode)
{
    char putcode[100];
    int size = table68k[opcode].size == sz_long ? 4 : 2;

    if (table68k[opcode].size == sz_long) {
	strcpy (putcode, "put_long(srca,");
    } else {
	strcpy (putcode, "put_word(srca,");
    }

    printf ("\tuae_u16 mask = %s;\n", gen_nextiword ());
    genamode (table68k[opcode].dmode, "dstreg", table68k[opcode].size, "src",
			GENA_GETV_FETCH_ALIGN, GENA_MOVEM_NO_INC, XLATE_LOG);
    sync_m68k_pc ();

    start_brace ();
    if (table68k[opcode].dmode == Apdi) {
	printf ("\tuae_u16 amask = mask & 0xff, dmask = (mask >> 8) & 0xff;\n");
	printf("#ifdef FULLMMU\n");
	printf ("\twhile (amask) { srca -= %d; %s m68k_areg(regs, movem_index2[amask])); amask = movem_next[amask]; }\n",
		size, putcode);
	printf ("\twhile (dmask) { srca -= %d; %s m68k_dreg(regs, movem_index2[dmask])); dmask = movem_next[dmask]; }\n",
		size, putcode);
	printf("#else\n");
	printf ("\twhile (amask) { srca -= %d; phys_%s m68k_areg(regs, movem_index2[amask])); amask = movem_next[amask]; }\n",
		size, putcode);
	printf ("\twhile (dmask) { srca -= %d; phys_%s m68k_dreg(regs, movem_index2[dmask])); dmask = movem_next[dmask]; }\n",
		size, putcode);
	printf("#endif\n");
	printf ("\tm68k_areg(regs, dstreg) = srca;\n");
    } else {
	printf ("\tuae_u16 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;\n");
	printf("#ifdef FULLMMU\n");
	printf ("\twhile (dmask) { %s m68k_dreg(regs, movem_index1[dmask])); srca += %d; dmask = movem_next[dmask]; }\n",
		putcode, size);
	printf ("\twhile (amask) { %s m68k_areg(regs, movem_index1[amask])); srca += %d; amask = movem_next[amask]; }\n",
		putcode, size);
	printf("#else\n");
	printf ("\twhile (dmask) { phys_%s m68k_dreg(regs, movem_index1[dmask])); srca += %d; dmask = movem_next[dmask]; }\n",
		putcode, size);
	printf ("\twhile (amask) { phys_%s m68k_areg(regs, movem_index1[amask])); srca += %d; amask = movem_next[amask]; }\n",
		putcode, size);
	printf("#endif\n");
    }
}

static void duplicate_carry (void)
{
    printf ("\tCOPY_CARRY();\n");
}

typedef enum {
    flag_logical_noclobber, flag_logical, flag_add, flag_sub, flag_cmp, flag_addx, flag_subx, flag_z, flag_zn,
    flag_av, flag_sv
} flagtypes;

static void genflags_normal (flagtypes type, wordsizes size, char *value, char *src, char *dst)
{
    char vstr[100], sstr[100], dstr[100];
    char usstr[100], udstr[100];
    char unsstr[100], undstr[100];

    switch (size) {
     case sz_byte:
	strcpy (vstr, "((uae_s8)(");
	strcpy (usstr, "((uae_u8)(");
	break;
     case sz_word:
	strcpy (vstr, "((uae_s16)(");
	strcpy (usstr, "((uae_u16)(");
	break;
     case sz_long:
	strcpy (vstr, "((uae_s32)(");
	strcpy (usstr, "((uae_u32)(");
	break;
     default:
	abort ();
    }
    strcpy (unsstr, usstr);

    strcpy (sstr, vstr);
    strcpy (dstr, vstr);
    strcat (vstr, value);
    strcat (vstr, "))");
    strcat (dstr, dst);
    strcat (dstr, "))");
    strcat (sstr, src);
    strcat (sstr, "))");

    strcpy (udstr, usstr);
    strcat (udstr, dst);
    strcat (udstr, "))");
    strcat (usstr, src);
    strcat (usstr, "))");

    strcpy (undstr, unsstr);
    strcat (unsstr, "-");
    strcat (undstr, "~");
    strcat (undstr, dst);
    strcat (undstr, "))");
    strcat (unsstr, src);
    strcat (unsstr, "))");

    switch (type) {
     case flag_logical_noclobber:
     case flag_logical:
     case flag_z:
     case flag_zn:
     case flag_av:
     case flag_sv:
     case flag_addx:
     case flag_subx:
	break;

     case flag_add:
	printf ("uae_u32 %s = %s + %s;\n", value, dstr, sstr);
	break;
     case flag_sub:
     case flag_cmp:
	printf ("uae_u32 %s = %s - %s;\n", value, dstr, sstr);
	break;
    }

    switch (type) {
     case flag_logical_noclobber:
     case flag_logical:
     case flag_z:
     case flag_zn:
	break;

     case flag_add:
     case flag_sub:
     case flag_addx:
     case flag_subx:
     case flag_cmp:
     case flag_av:
     case flag_sv:
	printf ("\t" BOOL_TYPE " flgs = %s < 0;\n", sstr);
	printf ("\t" BOOL_TYPE " flgo = %s < 0;\n", dstr);
	printf ("\t" BOOL_TYPE " flgn = %s < 0;\n", vstr);
	break;
    }

    switch (type) {
     case flag_logical:
	printf ("\tCLEAR_CZNV();\n");
	printf ("\tSET_ZFLG (%s == 0);\n", vstr);
	printf ("\tSET_NFLG (%s < 0);\n", vstr);
	break;
     case flag_logical_noclobber:
	printf ("\tSET_ZFLG (%s == 0);\n", vstr);
	printf ("\tSET_NFLG (%s < 0);\n", vstr);
	break;
     case flag_av:
	printf ("\tSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));\n");
	break;
     case flag_sv:
	printf ("\tSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));\n");
	break;
     case flag_z:
	printf ("\tSET_ZFLG (GET_ZFLG () & (%s == 0));\n", vstr);
	break;
     case flag_zn:
	printf ("\tSET_ZFLG (GET_ZFLG () & (%s == 0));\n", vstr);
	printf ("\tSET_NFLG (%s < 0);\n", vstr);
	break;
     case flag_add:
	printf ("\tSET_ZFLG (%s == 0);\n", vstr);
	printf ("\tSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));\n");
	printf ("\tSET_CFLG (%s < %s);\n", undstr, usstr);
	duplicate_carry ();
	printf ("\tSET_NFLG (flgn != 0);\n");
	break;
     case flag_sub:
	printf ("\tSET_ZFLG (%s == 0);\n", vstr);
	printf ("\tSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));\n");
	printf ("\tSET_CFLG (%s > %s);\n", usstr, udstr);
	duplicate_carry ();
	printf ("\tSET_NFLG (flgn != 0);\n");
	break;
     case flag_addx:
	printf ("\tSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));\n"); /* minterm SON: 0x42 */
	printf ("\tSET_CFLG (flgs ^ ((flgs ^ flgo) & (flgo ^ flgn)));\n"); /* minterm SON: 0xD4 */
	duplicate_carry ();
	break;
     case flag_subx:
	printf ("\tSET_VFLG ((flgs ^ flgo) & (flgo ^ flgn));\n"); /* minterm SON: 0x24 */
	printf ("\tSET_CFLG (flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));\n"); /* minterm SON: 0xB2 */
	duplicate_carry ();
	break;
     case flag_cmp:
	printf ("\tSET_ZFLG (%s == 0);\n", vstr);
	printf ("\tSET_VFLG ((flgs != flgo) && (flgn != flgo));\n");
	printf ("\tSET_CFLG (%s > %s);\n", usstr, udstr);
	printf ("\tSET_NFLG (flgn != 0);\n");
	break;
    }
}

static void genflags (flagtypes type, wordsizes size, char *value, char *src, char *dst)
{
    /* Temporarily deleted 68k/ARM flag optimizations.  I'd prefer to have
       them in the appropriate m68k.h files and use just one copy of this
       code here.  The API can be changed if necessary.  */
    int done = 0;
    
    start_brace ();
    printf("\n#ifdef OPTIMIZED_FLAGS\n");
    switch (type) {
     case flag_add:
     case flag_sub:
	printf ("\tuae_u32 %s;\n", value);
	break;
     default:
	break;
    }

    /* At least some of those casts are fairly important! */
    switch (type) {
     case flag_logical_noclobber:
	printf ("\t{uae_u32 oldcznv = GET_CZNV() & ~(FLAGVAL_Z | FLAGVAL_N);\n");
	if (strcmp (value, "0") == 0) {
	    printf ("\tSET_CZNV (olcznv | FLAGVAL_Z);\n");
	} else {
	    switch (size) {
	     case sz_byte: printf ("\toptflag_testb ((uae_s8)(%s));\n", value); break;
	     case sz_word: printf ("\toptflag_testw ((uae_s16)(%s));\n", value); break;
	     case sz_long: printf ("\toptflag_testl ((uae_s32)(%s));\n", value); break;
	    }
	    printf ("\tIOR_CZNV (oldcznv);\n");
	}
	printf ("\t}\n");
	done = 1;
	break;

     case flag_logical:
	if (strcmp (value, "0") == 0) {
	    printf ("\tSET_CZNV (FLAGVAL_Z);\n");
	} else {
	    switch (size) {
	     case sz_byte: printf ("\toptflag_testb ((uae_s8)(%s));\n", value); break;
	     case sz_word: printf ("\toptflag_testw ((uae_s16)(%s));\n", value); break;
	     case sz_long: printf ("\toptflag_testl ((uae_s32)(%s));\n", value); break;
	    }
	}
	done = 1;
	break;

     case flag_add:
	switch (size) {
	 case sz_byte: printf ("\toptflag_addb (%s, (uae_s8)(%s), (uae_s8)(%s));\n", value, src, dst); break;
	 case sz_word: printf ("\toptflag_addw (%s, (uae_s16)(%s), (uae_s16)(%s));\n", value, src, dst); break;
	 case sz_long: printf ("\toptflag_addl (%s, (uae_s32)(%s), (uae_s32)(%s));\n", value, src, dst); break;
	}
	done = 1;
	break;

     case flag_sub:
	switch (size) {
	 case sz_byte: printf ("\toptflag_subb (%s, (uae_s8)(%s), (uae_s8)(%s));\n", value, src, dst); break;
	 case sz_word: printf ("\toptflag_subw (%s, (uae_s16)(%s), (uae_s16)(%s));\n", value, src, dst); break;
	 case sz_long: printf ("\toptflag_subl (%s, (uae_s32)(%s), (uae_s32)(%s));\n", value, src, dst); break;
	}
	done = 1;
	break;

     case flag_cmp:
	switch (size) {
	 case sz_byte: printf ("\toptflag_cmpb ((uae_s8)(%s), (uae_s8)(%s));\n", src, dst); break;
	 case sz_word: printf ("\toptflag_cmpw ((uae_s16)(%s), (uae_s16)(%s));\n", src, dst); break;
	 case sz_long: printf ("\toptflag_cmpl ((uae_s32)(%s), (uae_s32)(%s));\n", src, dst); break;
	}
	done = 1;
	break;
	
     default:
	break;
    }
    if (done)
	printf("#else\n");
    else
    	printf("#endif\n");
    genflags_normal (type, size, value, src, dst);
    if (done)
	printf("#endif\n");
}

static void force_range_for_rox (const char *var, wordsizes size)
{
    /* Could do a modulo operation here... which one is faster? */
    switch (size) {
     case sz_long:
	printf ("\tif (%s >= 33) %s -= 33;\n", var, var);
	break;
     case sz_word:
	printf ("\tif (%s >= 34) %s -= 34;\n", var, var);
	printf ("\tif (%s >= 17) %s -= 17;\n", var, var);
	break;
     case sz_byte:
	printf ("\tif (%s >= 36) %s -= 36;\n", var, var);
	printf ("\tif (%s >= 18) %s -= 18;\n", var, var);
	printf ("\tif (%s >= 9) %s -= 9;\n", var, var);
	break;
    }
}

static const char *cmask (wordsizes size)
{
    switch (size) {
     case sz_byte: return "0x80";
     case sz_word: return "0x8000";
     case sz_long: return "0x80000000";
     default: abort (); return NULL;
    }
}

static int source_is_imm1_8 (struct instr *i)
{
    return i->stype == 3;
}

static void gen_opcode (unsigned long int opcode)
{
    struct instr *curi = table68k + opcode;

    start_brace ();
#if 0
    printf ("uae_u8 *m68k_pc = m68k_getpc();\n");
#endif
    m68k_pc_offset = 2;
    switch (curi->plev) {
     case 0: /* not privileged */
	break;
     case 1: /* unprivileged only on 68000 */
	if (cpu_level == 0)
	    break;
	if (next_cpu_level < 0)
	    next_cpu_level = 0;

	/* fall through */
     case 2: /* priviledged */
	printf ("if (!regs.s) { Exception(8,0); goto %s; }\n", endlabelstr);
	need_endlabel = 1;
	start_brace ();
	break;
     case 3: /* privileged if size == word */
	if (curi->size == sz_byte)
	    break;
	printf ("if (!regs.s) { Exception(8,0); goto %s; }\n", endlabelstr);
	need_endlabel = 1;
	start_brace ();
	break;
    }
    switch (curi->mnemo) {
     case i_OR:
     case i_AND:
     case i_EOR:
	genamode (curi->smode, "srcreg", curi->size, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	genamode (curi->dmode, "dstreg", curi->size, "dst", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	printf ("\tsrc %c= dst;\n", curi->mnemo == i_OR ? '|' : curi->mnemo == i_AND ? '&' : '^');
	genflags (flag_logical, curi->size, "src", "", "");
	genastore ("src", curi->dmode, "dstreg", curi->size, "dst", XLATE_LOG);
	break;
     case i_ORSR:
     case i_EORSR:
	printf ("\tMakeSR();\n");
	genamode (curi->smode, "srcreg", curi->size, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	if (curi->size == sz_byte) {
	    printf ("\tsrc &= 0xFF;\n");
	}
	printf ("\tregs.sr %c= src;\n", curi->mnemo == i_EORSR ? '^' : '|');
	printf ("\tMakeFromSR();\n");
	break;
     case i_ANDSR:
	printf ("\tMakeSR();\n");
	genamode (curi->smode, "srcreg", curi->size, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	if (curi->size == sz_byte) {
	    printf ("\tsrc |= 0xFF00;\n");
	}
	printf ("\tregs.sr &= src;\n");
	printf ("\tMakeFromSR();\n");
	break;
     case i_SUB:
	genamode (curi->smode, "srcreg", curi->size, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	genamode (curi->dmode, "dstreg", curi->size, "dst", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	start_brace ();
	genflags (flag_sub, curi->size, "newv", "src", "dst");
	genastore ("newv", curi->dmode, "dstreg", curi->size, "dst", XLATE_LOG);
	break;
     case i_SUBA:
	genamode (curi->smode, "srcreg", curi->size, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	genamode (curi->dmode, "dstreg", sz_long, "dst", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	start_brace ();
	printf ("\tuae_u32 newv = dst - src;\n");
	genastore ("newv", curi->dmode, "dstreg", sz_long, "dst", XLATE_LOG);
	break;
     case i_SUBX:
	genamode2 (curi->smode, "srcreg", curi->size, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG, 1);
	genamode (curi->dmode, "dstreg", curi->size, "dst", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	genamode2 (curi->smode, "srcreg", curi->size, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG, 2);
	start_brace ();
	printf ("\tuae_u32 newv = dst - src - (GET_XFLG () ? 1 : 0);\n");
	genflags (flag_subx, curi->size, "newv", "src", "dst");
	genflags (flag_zn, curi->size, "newv", "", "");
	genastore ("newv", curi->dmode, "dstreg", curi->size, "dst", XLATE_LOG);
	break;
     case i_SBCD:
	genamode2 (curi->smode, "srcreg", curi->size, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG, 1);
	genamode (curi->dmode, "dstreg", curi->size, "dst", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	genamode2 (curi->smode, "srcreg", curi->size, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG, 2);
	start_brace ();
	printf ("\tuae_u16 newv_lo = (dst & 0xF) - (src & 0xF) - (GET_XFLG () ? 1 : 0);\n");
	printf ("\tuae_u16 newv_hi = (dst & 0xF0) - (src & 0xF0);\n");
	printf ("\tuae_u16 newv, tmp_newv;\n");
	printf ("\tint bcd = 0;\n");
	printf ("\tnewv = tmp_newv = newv_hi + newv_lo;\n");
	printf ("\tif (newv_lo & 0xF0) { newv -= 6; bcd = 6; };\n");
	printf ("\tif ((((dst & 0xFF) - (src & 0xFF) - (GET_XFLG () ? 1 : 0)) & 0x100) > 0xFF) { newv -= 0x60; }\n");
	printf ("\tSET_CFLG ((((dst & 0xFF) - (src & 0xFF) - bcd - (GET_XFLG () ? 1 : 0)) & 0x300) > 0xFF);\n");
	duplicate_carry ();
	/* Manual says bits NV are undefined though a real 68030 doesn't change V and 68040/060 don't change both */
	if (cpu_level >= xBCD_KEEPS_N_FLAG) {
		if (next_cpu_level < xBCD_KEEPS_N_FLAG)
			next_cpu_level = xBCD_KEEPS_N_FLAG - 1;
		genflags (flag_z, curi->size, "newv", "", "");
	} else {
		genflags (flag_zn, curi->size, "newv", "", "");
	}
	if (cpu_level >= xBCD_KEEPS_V_FLAG) {
		if (next_cpu_level < xBCD_KEEPS_V_FLAG)
			next_cpu_level = xBCD_KEEPS_V_FLAG - 1;
	} else {
		printf ("\tSET_VFLG ((tmp_newv & 0x80) != 0 && (newv & 0x80) == 0);\n");
	}
	genastore ("newv", curi->dmode, "dstreg", curi->size, "dst", XLATE_LOG);
	break;
     case i_ADD:
	genamode (curi->smode, "srcreg", curi->size, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	genamode (curi->dmode, "dstreg", curi->size, "dst", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	start_brace ();
	genflags (flag_add, curi->size, "newv", "src", "dst");
	genastore ("newv", curi->dmode, "dstreg", curi->size, "dst", XLATE_LOG);
	break;
     case i_ADDA:
	genamode (curi->smode, "srcreg", curi->size, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	genamode (curi->dmode, "dstreg", sz_long, "dst", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	start_brace ();
	printf ("\tuae_u32 newv = dst + src;\n");
	genastore ("newv", curi->dmode, "dstreg", sz_long, "dst", XLATE_LOG);
	break;
     case i_ADDX:
	genamode2 (curi->smode, "srcreg", curi->size, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG, 1);
	genamode (curi->dmode, "dstreg", curi->size, "dst", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	genamode2 (curi->smode, "srcreg", curi->size, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG, 2);
	start_brace ();
	printf ("\tuae_u32 newv = dst + src + (GET_XFLG () ? 1 : 0);\n");
	genflags (flag_addx, curi->size, "newv", "src", "dst");
	genflags (flag_zn, curi->size, "newv", "", "");
	genastore ("newv", curi->dmode, "dstreg", curi->size, "dst", XLATE_LOG);
	break;
     case i_ABCD:
	genamode2 (curi->smode, "srcreg", curi->size, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG, 1);
	genamode (curi->dmode, "dstreg", curi->size, "dst", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	genamode2 (curi->smode, "srcreg", curi->size, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG, 2);
	start_brace ();
	printf ("\tuae_u16 newv_lo = (src & 0xF) + (dst & 0xF) + (GET_XFLG () ? 1 : 0);\n");
	printf ("\tuae_u16 newv_hi = (src & 0xF0) + (dst & 0xF0);\n");
	printf ("\tuae_u16 newv, tmp_newv;\n");
	printf ("\tint cflg;\n");
	printf ("\tnewv = tmp_newv = newv_hi + newv_lo;\n");
	printf ("\tif (newv_lo > 9) { newv += 6; }\n");
	printf ("\tcflg = (newv & 0x3F0) > 0x90;\n");
	printf ("\tif (cflg) newv += 0x60;\n");
	printf ("\tSET_CFLG (cflg);\n");
	duplicate_carry ();
	/* Manual says bits NV are undefined though a real 68030 doesn't change V and 68040/060 don't change both */
	if (cpu_level >= xBCD_KEEPS_N_FLAG) {
		if (next_cpu_level < xBCD_KEEPS_N_FLAG)
			next_cpu_level = xBCD_KEEPS_N_FLAG - 1;
		genflags (flag_z, curi->size, "newv", "", "");
	} else {
		genflags (flag_zn, curi->size, "newv", "", "");
	}
	if (cpu_level >= xBCD_KEEPS_V_FLAG) {
		if (next_cpu_level < xBCD_KEEPS_V_FLAG)
			next_cpu_level = xBCD_KEEPS_V_FLAG - 1;
	} else {
		printf ("\tSET_VFLG ((tmp_newv & 0x80) == 0 && (newv & 0x80) != 0);\n");
	}
	genastore ("newv", curi->dmode, "dstreg", curi->size, "dst", XLATE_LOG);
	break;
     case i_NEG:
	genamode (curi->smode, "srcreg", curi->size, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	start_brace ();
	genflags (flag_sub, curi->size, "dst", "src", "0");
	genastore ("dst", curi->smode, "srcreg", curi->size, "src", XLATE_LOG);
	break;
     case i_NEGX:
	genamode (curi->smode, "srcreg", curi->size, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	start_brace ();
	printf ("\tuae_u32 newv = 0 - src - (GET_XFLG () ? 1 : 0);\n");
	genflags (flag_subx, curi->size, "newv", "src", "0");
	genflags (flag_zn, curi->size, "newv", "", "");
	genastore ("newv", curi->smode, "srcreg", curi->size, "src", XLATE_LOG);
	break;
     case i_NBCD:
	genamode (curi->smode, "srcreg", curi->size, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	start_brace ();
	printf ("\tuae_u16 newv_lo = - (src & 0xF) - (GET_XFLG () ? 1 : 0);\n");
	printf ("\tuae_u16 newv_hi = - (src & 0xF0);\n");
	printf ("\tuae_u16 newv;\n");
	printf ("\tint cflg, tmp_newv;\n");
	printf ("\ttmp_newv = newv_hi + newv_lo;\n");
	printf ("\tif (newv_lo > 9) { newv_lo -= 6; }\n");
	printf ("\tnewv = newv_hi + newv_lo;\n");
	printf ("\tcflg = (newv & 0x1F0) > 0x90;\n");
	printf ("\tif (cflg) newv -= 0x60;\n");
	printf ("\tSET_CFLG (cflg);\n");
	duplicate_carry();
	/* Manual says bits NV are undefined though a real 68030 doesn't change V and 68040/060 don't change both */
	if (cpu_level >= xBCD_KEEPS_N_FLAG) {
		if (next_cpu_level < xBCD_KEEPS_N_FLAG)
			next_cpu_level = xBCD_KEEPS_N_FLAG - 1;
		genflags (flag_z, curi->size, "newv", "", "");
	} else {
		genflags (flag_zn, curi->size, "newv", "", "");
	}
	if (cpu_level >= xBCD_KEEPS_V_FLAG) {
		if (next_cpu_level < xBCD_KEEPS_V_FLAG)
			next_cpu_level = xBCD_KEEPS_V_FLAG - 1;
	} else {
		printf ("\tSET_VFLG ((tmp_newv & 0x80) != 0 && (newv & 0x80) == 0);\n");
	}
	genastore ("newv", curi->smode, "srcreg", curi->size, "src", XLATE_LOG);
	break;
     case i_CLR:
	genamode (curi->smode, "srcreg", curi->size, "src", GENA_GETV_FETCH_ALIGN, GENA_MOVEM_DO_INC, XLATE_LOG);
	genflags (flag_logical, curi->size, "0", "", "");
	genastore ("0", curi->smode, "srcreg", curi->size, "src", XLATE_LOG);
	break;
     case i_NOT:
	genamode (curi->smode, "srcreg", curi->size, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	start_brace ();
	printf ("\tuae_u32 dst = ~src;\n");
	genflags (flag_logical, curi->size, "dst", "", "");
	genastore ("dst", curi->smode, "srcreg", curi->size, "src", XLATE_LOG);
	break;
     case i_TST:
	genamode (curi->smode, "srcreg", curi->size, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	genflags (flag_logical, curi->size, "src", "", "");
	break;
     case i_BTST:
	genamode (curi->smode, "srcreg", curi->size, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	genamode (curi->dmode, "dstreg", curi->size, "dst", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	if (curi->size == sz_byte)
	    printf ("\tsrc &= 7;\n");
	else
	    printf ("\tsrc &= 31;\n");
	printf ("\tSET_ZFLG (1 ^ ((dst >> src) & 1));\n");
	break;
     case i_BCHG:
	genamode (curi->smode, "srcreg", curi->size, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	genamode (curi->dmode, "dstreg", curi->size, "dst", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	if (curi->size == sz_byte)
	    printf ("\tsrc &= 7;\n");
	else
	    printf ("\tsrc &= 31;\n");
	printf ("\tdst ^= (1 << src);\n");
	printf ("\tSET_ZFLG (((uae_u32)dst & (1 << src)) >> src);\n");
	genastore ("dst", curi->dmode, "dstreg", curi->size, "dst", XLATE_LOG);
	break;
     case i_BCLR:
	genamode (curi->smode, "srcreg", curi->size, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	genamode (curi->dmode, "dstreg", curi->size, "dst", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	if (curi->size == sz_byte)
	    printf ("\tsrc &= 7;\n");
	else
	    printf ("\tsrc &= 31;\n");
	printf ("\tSET_ZFLG (1 ^ ((dst >> src) & 1));\n");
	printf ("\tdst &= ~(1 << src);\n");
	genastore ("dst", curi->dmode, "dstreg", curi->size, "dst", XLATE_LOG);
	break;
     case i_BSET:
	genamode (curi->smode, "srcreg", curi->size, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	genamode (curi->dmode, "dstreg", curi->size, "dst", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	if (curi->size == sz_byte)
	    printf ("\tsrc &= 7;\n");
	else
	    printf ("\tsrc &= 31;\n");
	printf ("\tSET_ZFLG (1 ^ ((dst >> src) & 1));\n");
	printf ("\tdst |= (1 << src);\n");
	genastore ("dst", curi->dmode, "dstreg", curi->size, "dst", XLATE_LOG);
	break;
     case i_CMPM:
     case i_CMP:
	genamode (curi->smode, "srcreg", curi->size, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	genamode (curi->dmode, "dstreg", curi->size, "dst", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	start_brace ();
	genflags (flag_cmp, curi->size, "newv", "src", "dst");
	break;
     case i_CMPA:
	genamode (curi->smode, "srcreg", curi->size, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	genamode (curi->dmode, "dstreg", sz_long, "dst", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	start_brace ();
	genflags (flag_cmp, sz_long, "newv", "src", "dst");
	break;
	/* The next two are coded a little unconventional, but they are doing
	 * weird things... */
     case i_MVPRM:
	genamode (curi->smode, "srcreg", curi->size, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);

	printf ("\tuaecptr memp = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)%s;\n", gen_nextiword ());
	if (curi->size == sz_word) {
	    printf ("\tput_byte(memp, src >> 8); put_byte(memp + 2, src);\n");
	} else {
	    printf ("\tput_byte(memp, src >> 24); put_byte(memp + 2, src >> 16);\n");
	    printf ("\tput_byte(memp + 4, src >> 8); put_byte(memp + 6, src);\n");
	}
	break;
     case i_MVPMR:
	printf ("\tuaecptr memp = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)%s;\n", gen_nextiword ());
	genamode (curi->dmode, "dstreg", curi->size, "dst", GENA_GETV_FETCH_ALIGN, GENA_MOVEM_DO_INC, XLATE_LOG);
	if (curi->size == sz_word) {
	    printf ("\tuae_u16 val  = get_byte(memp) << 8;\n");
	    printf ("\t        val |= get_byte(memp + 2);\n");
	} else {
	    printf ("\tuae_u32 val  = get_byte(memp) << 24;\n");
	    printf ("\t        val |= get_byte(memp + 2) << 16;\n");
	    printf ("\t        val |= get_byte(memp + 4) << 8;\n");
	    printf ("\t        val |= get_byte(memp + 6);\n");
	}
	genastore ("val", curi->dmode, "dstreg", curi->size, "dst", XLATE_LOG);
	break;
     case i_MOVE:
	genamode2 (curi->smode, "srcreg", curi->size, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG, 1);
	genamode (curi->dmode, "dstreg", curi->size, "dst", GENA_GETV_FETCH_ALIGN, GENA_MOVEM_DO_INC, XLATE_LOG);
	genamode2 (curi->smode, "srcreg", curi->size, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG, 2);
	genflags (flag_logical, curi->size, "src", "", "");
	genastore ("src", curi->dmode, "dstreg", curi->size, "dst", XLATE_LOG);
	break;
     case i_MOVEA:
	genamode (curi->smode, "srcreg", curi->size, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	genamode (curi->dmode, "dstreg", curi->size, "dst", GENA_GETV_FETCH_ALIGN, GENA_MOVEM_DO_INC, XLATE_LOG);
	if (curi->size == sz_word) {
	    printf ("\tuae_u32 val = (uae_s32)(uae_s16)src;\n");
	} else {
	    printf ("\tuae_u32 val = src;\n");
	}
	genastore ("val", curi->dmode, "dstreg", sz_long, "dst", XLATE_LOG);
	break;
     case i_MVSR2:
	genamode (curi->smode, "srcreg", sz_word, "src", GENA_GETV_FETCH_ALIGN, GENA_MOVEM_DO_INC, XLATE_LOG);
	printf ("\tMakeSR();\n");
	if (curi->size == sz_byte)
	    genastore ("regs.sr & 0xff", curi->smode, "srcreg", sz_word, "src", XLATE_LOG);
	else
	    genastore ("regs.sr", curi->smode, "srcreg", sz_word, "src", XLATE_LOG);
	break;
     case i_MV2SR:
	genamode (curi->smode, "srcreg", sz_word, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	if (curi->size == sz_byte)
	    printf ("\tMakeSR();\n\tregs.sr &= 0xFF00;\n\tregs.sr |= src & 0xFF;\n");
	else {
	    printf ("\tregs.sr = src;\n");
	}
	printf ("\tMakeFromSR();\n");
	break;
     case i_SWAP:
	genamode (curi->smode, "srcreg", sz_long, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	start_brace ();
	printf ("\tuae_u32 dst = ((src >> 16)&0xFFFF) | ((src&0xFFFF)<<16);\n");
	genflags (flag_logical, sz_long, "dst", "", "");
	genastore ("dst", curi->smode, "srcreg", sz_long, "src", XLATE_LOG);
	break;
     case i_EXG:
	genamode (curi->smode, "srcreg", curi->size, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	genamode (curi->dmode, "dstreg", curi->size, "dst", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	genastore ("dst", curi->smode, "srcreg", curi->size, "src", XLATE_LOG);
	genastore ("src", curi->dmode, "dstreg", curi->size, "dst", XLATE_LOG);
	break;
     case i_EXT:
	genamode (curi->smode, "srcreg", sz_long, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	start_brace ();
	switch (curi->size) {
	case sz_byte: printf ("\tuae_u32 dst = (uae_s32)(uae_s8)src;\n"); break;
	case sz_word: printf ("\tuae_u16 dst = (uae_s16)(uae_s8)src;\n"); break;
	case sz_long: printf ("\tuae_u32 dst = (uae_s32)(uae_s16)src;\n"); break;
	default: abort ();
	}
	genflags (flag_logical,
		  curi->size == sz_word ? sz_word : sz_long, "dst", "", "");
	genastore ("dst", curi->smode, "srcreg",
		   curi->size == sz_word ? sz_word : sz_long, "src", XLATE_LOG);
	break;
     case i_MVMEL:
	genmovemel (opcode);
	break;
     case i_MVMLE:
	genmovemle (opcode);
	break;
     case i_TRAP:
	genamode (curi->smode, "srcreg", curi->size, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	gen_set_fault_pc ();
	printf ("\tException(src+32,0);\n");
	break;
     case i_MVR2USP:
	genamode (curi->smode, "srcreg", curi->size, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	printf ("\tregs.usp = src;\n");
	break;
     case i_MVUSP2R:
	genamode (curi->smode, "srcreg", curi->size, "src", GENA_GETV_FETCH_ALIGN, GENA_MOVEM_DO_INC, XLATE_LOG);
	genastore ("regs.usp", curi->smode, "srcreg", curi->size, "src", XLATE_LOG);
	break;
     case i_RESET:
	printf ("\tAtariReset();\n");
	break;
     case i_NOP:
	break;
     case i_STOP:
	genamode (curi->smode, "srcreg", curi->size, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	/*
	 * STOP undocumented features:
	 * if SR is not set:
	 * 68000 (68010?): Update SR, increase PC and then cause privilege violation exception (handled in newcpu)
	 * 68000 (68010?): Traced STOP also runs 4 cycles faster.
	 * 68020 68030: STOP works normally
	 * 68040 68060: Immediate privilege violation exception
	 */
	printf ("\tuae_u16 sr = src;\n");
	if (cpu_level >= 4) {
		printf("\tif (!(sr & 0x2000)) {\n");
		printf ("m68k_incpc(%d);\n", m68k_pc_offset);
		printf("\t\tException(8,0); goto %s;\n", endlabelstr);
		printf("\t}\n");
	}
	printf("\tregs.sr = sr;\n");
	printf ("\tMakeFromSR();\n");
	printf ("\tm68k_setstopped(1);\n");
	sync_m68k_pc ();
	/* STOP does not prefetch anything */
	/* did_prefetch = -1; */
	break;
     case i_RTE:
	if (cpu_level == 0) {
	    genamode (Aipi, "7", sz_word, "sr", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	    genamode (Aipi, "7", sz_long, "pc", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	    printf ("\tregs.sr = sr; m68k_setpc_rte(pc);\n");
	    fill_prefetch_0 ();
	    printf ("\tMakeFromSR();\n");
	} else {
		if (next_cpu_level < 0)
		next_cpu_level = 0;
		printf ("\tex_rte();\n");
	}
	/* PC is set and prefetch filled. */
	m68k_pc_offset = 0;
	break;
     case i_RTD:
	genamode (curi->smode, "srcreg", curi->size, "offs", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	genamode (Aipi, "7", sz_long, "pc", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	printf ("\tm68k_areg(regs, 7) += offs;\n");
	printf ("\tm68k_setpc_rte(pc);\n");
	fill_prefetch_0 ();
	/* PC is set and prefetch filled. */
	m68k_pc_offset = 0;
	break;
     case i_LINK:
	genamode (curi->dmode, "dstreg", curi->size, "offs", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	genamode (Apdi, "7", sz_long, "old", GENA_GETV_FETCH_ALIGN, GENA_MOVEM_DO_INC, XLATE_LOG);
	genamode (curi->smode, "srcreg", sz_long, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	genastore ("m68k_areg(regs, 7)", curi->smode, "srcreg", sz_long, "src", XLATE_LOG);
	printf ("\tm68k_areg(regs, 7) += offs;\n");
	genastore ("src", Apdi, "7", sz_long, "old", XLATE_LOG);
	break;
     case i_UNLK:
	genamode (curi->smode, "srcreg", curi->size, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	printf ("\tm68k_areg(regs, 7) = src;\n");
	genamode (Aipi, "7", sz_long, "old", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	genastore ("old", curi->smode, "srcreg", curi->size, "src", XLATE_LOG);
	break;
     case i_RTS:
	printf ("\tm68k_do_rts();\n");
	fill_prefetch_0 ();
	m68k_pc_offset = 0;
	break;
     case i_TRAPV:
	printf ("\tuaecptr oldpc = m68k_getpc();\n");
	sync_m68k_pc ();
	printf ("\tif (GET_VFLG ()) { Exception(7,oldpc); goto %s; }\n", endlabelstr);
	need_endlabel = 1;
	break;
     case i_RTR:
	printf ("\tMakeSR();\n");
	genamode2 (Aipi, "7", sz_word, "sr", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG, 1);
	genamode (Aipi, "7", sz_long, "pc", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	genamode2 (Aipi, "7", sz_word, "sr", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG, 2);
	printf ("\tregs.sr &= 0xFF00; sr &= 0xFF;\n");
	printf ("\tregs.sr |= sr; m68k_setpc(pc);\n");
	fill_prefetch_0 ();
	printf ("\tMakeFromSR();\n");
	m68k_pc_offset = 0;
	break;
     case i_JSR:
	genamode (curi->smode, "srcreg", curi->size, "src", GENA_GETV_NO_FETCH, GENA_MOVEM_DO_INC, XLATE_PHYS);
	printf ("\tm68k_do_jsr(m68k_getpc() + %d, srca);\n", m68k_pc_offset);
	fill_prefetch_0 ();
	m68k_pc_offset = 0;
	break;
     case i_JMP:
	genamode (curi->smode, "srcreg", curi->size, "src", GENA_GETV_NO_FETCH, GENA_MOVEM_DO_INC, XLATE_PHYS);
	printf ("\tm68k_setpc(srca);\n");
	fill_prefetch_0 ();
	m68k_pc_offset = 0;
	break;
     case i_BSR:
	genamode (curi->smode, "srcreg", curi->size, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_PHYS);
	printf ("\tuae_s32 s = (uae_s32)src + 2;\n");
	if (using_exception_3) {
	    printf ("\tif (src & 1) {\n");
	    printf ("\tlast_addr_for_exception_3 = m68k_getpc() + 2;\n");
	    printf ("\t\tlast_fault_for_exception_3 = m68k_getpc() + s;\n");
	    printf ("\t\tlast_op_for_exception_3 = opcode; Exception(3,0); goto %s;\n", endlabelstr);
	    printf ("\t}\n");
	    need_endlabel = 1;
	}
	printf ("\tm68k_do_bsr(m68k_getpc() + %d, s);\n", m68k_pc_offset);
	fill_prefetch_0 ();
	m68k_pc_offset = 0;
	break;
     case i_Bcc:
	if (curi->size == sz_long) {
	    if (cpu_level < 2) {
		printf ("\tm68k_incpc(2);\n");
		printf ("\tif (!cctrue(%d)) goto %s;\n", curi->cc, endlabelstr);
		printf ("\t\tlast_addr_for_exception_3 = m68k_getpc() + 2;\n");
		printf ("\t\tlast_fault_for_exception_3 = m68k_getpc() + 1;\n");
		printf ("\t\tlast_op_for_exception_3 = opcode; Exception(3,0); goto %s;\n", endlabelstr);
		need_endlabel = 1;
	    } else {
		if (next_cpu_level < 1)
		    next_cpu_level = 1;
	    }
	}
	genamode (curi->smode, "srcreg", curi->size, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_PHYS);
	printf ("\tif (!cctrue(%d)) goto didnt_jump_%lx;\n", curi->cc, opcode);
	if (using_exception_3) {
	    printf ("\tif (src & 1) {\n");
	    printf ("\t\tlast_addr_for_exception_3 = m68k_getpc() + 2;\n");
	    printf ("\t\tlast_fault_for_exception_3 = m68k_getpc() + 2 + (uae_s32)src;\n");
	    printf ("\t\tlast_op_for_exception_3 = opcode; Exception(3,0); goto %s;\n", endlabelstr);
	    printf ("\t}\n");
	    need_endlabel = 1;
	}
	printf ("\tm68k_incpc ((uae_s32)src + 2);\n");
	fill_prefetch_0 ();
	printf ("return;\n");
	printf ("didnt_jump_%lx:;\n", opcode);
	need_endlabel = 1;
	break;
     case i_LEA:
	genamode (curi->smode, "srcreg", curi->size, "src", GENA_GETV_NO_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	genamode (curi->dmode, "dstreg", curi->size, "dst", GENA_GETV_FETCH_ALIGN, GENA_MOVEM_DO_INC, XLATE_LOG);
	genastore ("srca", curi->dmode, "dstreg", curi->size, "dst", XLATE_LOG);
	break;
     case i_PEA:
	genamode (curi->smode, "srcreg", curi->size, "src", GENA_GETV_NO_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	genamode (Apdi, "7", sz_long, "dst", GENA_GETV_FETCH_ALIGN, GENA_MOVEM_DO_INC, XLATE_LOG);
	genastore ("srca", Apdi, "7", sz_long, "dst", XLATE_LOG);
	break;
     case i_DBcc:
	genamode (curi->smode, "srcreg", curi->size, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	genamode (curi->dmode, "dstreg", curi->size, "offs", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);

	printf ("\tif (!cctrue(%d)) {\n", curi->cc);
	genastore ("(src-1)", curi->smode, "srcreg", curi->size, "src", XLATE_LOG);

	printf ("\t\tif (src) {\n");
	if (using_exception_3) {
	    printf ("\t\t\tif (offs & 1) {\n");
	    printf ("\t\t\tlast_addr_for_exception_3 = m68k_getpc() + 2;\n");
	    printf ("\t\t\tlast_fault_for_exception_3 = m68k_getpc() + 2 + (uae_s32)offs + 2;\n");
	    printf ("\t\t\tlast_op_for_exception_3 = opcode; Exception(3,0); goto %s;\n", endlabelstr);
	    printf ("\t\t}\n");
	    need_endlabel = 1;
	}
	printf ("\t\t\tm68k_incpc((uae_s32)offs + 2);\n");
	fill_prefetch_0 ();
	printf ("return;\n");
	printf ("\t\t}\n");
	printf ("\t}\n");
	need_endlabel = 1;
	break;
     case i_Scc:
	genamode (curi->smode, "srcreg", curi->size, "src", GENA_GETV_FETCH_ALIGN, GENA_MOVEM_DO_INC, XLATE_LOG);
	start_brace ();
	printf ("\tint val = cctrue(%d) ? 0xff : 0;\n", curi->cc);
	genastore ("val", curi->smode, "srcreg", curi->size, "src", XLATE_LOG);
	break;
     case i_DIVU:
	printf ("\tuaecptr oldpc = m68k_getpc();\n");
	genamode (curi->smode, "srcreg", sz_word, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	genamode (curi->dmode, "dstreg", sz_long, "dst", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	sync_m68k_pc ();
	/* Clear V flag when dividing by zero - Alcatraz Odyssey demo depends
	 * on this (actually, it's doing a DIVS).  */
	printf ("\tif (src == 0) { SET_VFLG (0); Exception (5, oldpc); goto %s; } else {\n", endlabelstr);
	printf ("\tuae_u32 newv = (uae_u32)dst / (uae_u32)(uae_u16)src;\n");
	printf ("\tuae_u32 rem = (uae_u32)dst %% (uae_u32)(uae_u16)src;\n");
	/* The N flag appears to be set each time there is an overflow.
	 * Weird. */
	printf ("\tif (newv > 0xffff) { SET_VFLG (1); SET_NFLG (1); SET_CFLG (0); } else\n\t{\n");
	genflags (flag_logical, sz_word, "newv", "", "");
	printf ("\tnewv = (newv & 0xffff) | ((uae_u32)rem << 16);\n");
	genastore ("newv", curi->dmode, "dstreg", sz_long, "dst", XLATE_LOG);
	printf ("\t}\n");
	printf ("\t}\n");
	need_endlabel = 1;
	break;
     case i_DIVS:
	printf ("\tuaecptr oldpc = m68k_getpc();\n");
	genamode (curi->smode, "srcreg", sz_word, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	genamode (curi->dmode, "dstreg", sz_long, "dst", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	sync_m68k_pc ();
	printf ("\tif (src == 0) { SET_VFLG (0); Exception(5,oldpc); goto %s; } else {\n", endlabelstr);
	printf ("\tuae_s32 newv = (uae_s32)dst / (uae_s32)(uae_s16)src;\n");
	printf ("\tuae_u16 rem = (uae_s32)dst %% (uae_s32)(uae_s16)src;\n");
	printf ("\tif ((newv & 0xffff8000) != 0 && (newv & 0xffff8000) != 0xffff8000) { SET_VFLG (1); SET_NFLG (1); SET_CFLG (0); } else\n\t{\n");
	printf ("\tif (((uae_s16)rem < 0) != ((uae_s32)dst < 0)) rem = -rem;\n");
	genflags (flag_logical, sz_word, "newv", "", "");
	printf ("\tnewv = (newv & 0xffff) | ((uae_u32)rem << 16);\n");
	genastore ("newv", curi->dmode, "dstreg", sz_long, "dst", XLATE_LOG);
	printf ("\t}\n");
	printf ("\t}\n");
	need_endlabel = 1;
	break;
     case i_MULU:
	genamode (curi->smode, "srcreg", sz_word, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	genamode (curi->dmode, "dstreg", sz_word, "dst", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	start_brace ();
	printf ("\tuae_u32 newv = (uae_u32)(uae_u16)dst * (uae_u32)(uae_u16)src;\n");
	genflags (flag_logical, sz_long, "newv", "", "");
	genastore ("newv", curi->dmode, "dstreg", sz_long, "dst", XLATE_LOG);
	break;
     case i_MULS:
	genamode (curi->smode, "srcreg", sz_word, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	genamode (curi->dmode, "dstreg", sz_word, "dst", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	start_brace ();
	printf ("\tuae_u32 newv = (uae_s32)(uae_s16)dst * (uae_s32)(uae_s16)src;\n");
	genflags (flag_logical, sz_long, "newv", "", "");
	genastore ("newv", curi->dmode, "dstreg", sz_long, "dst", XLATE_LOG);
	break;
     case i_CHK:
	printf ("\tuaecptr oldpc = m68k_getpc();\n");
	genamode (curi->smode, "srcreg", curi->size, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	genamode (curi->dmode, "dstreg", curi->size, "dst", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	printf ("\tif ((uae_s32)dst < 0) { SET_NFLG (1); Exception(6,oldpc); goto %s; }\n", endlabelstr);
	printf ("\telse if (dst > src) { SET_NFLG (0); Exception(6,oldpc); goto %s; }\n", endlabelstr);
	need_endlabel = 1;
	break;

     case i_CHK2:
	printf ("\tuaecptr oldpc = m68k_getpc();\n");
	genamode (curi->smode, "srcreg", curi->size, "extra", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	genamode (curi->dmode, "dstreg", curi->size, "dst", GENA_GETV_FETCH_ALIGN, GENA_MOVEM_DO_INC, XLATE_LOG);
	printf ("\t{uae_s32 upper,lower,reg = regs.regs[(extra >> 12) & 15];\n");
	switch (curi->size) {
	 case sz_byte:
	    printf ("\tlower=(uae_s32)(uae_s8)get_byte(dsta); upper = (uae_s32)(uae_s8)get_byte(dsta+1);\n");
	    printf ("\tif ((extra & 0x8000) == 0) reg = (uae_s32)(uae_s8)reg;\n");
	    break;
	 case sz_word:
	    printf ("\tlower=(uae_s32)(uae_s16)get_word(dsta); upper = (uae_s32)(uae_s16)get_word(dsta+2);\n");
	    printf ("\tif ((extra & 0x8000) == 0) reg = (uae_s32)(uae_s16)reg;\n");
	    break;
	 case sz_long:
	    printf ("\tlower=get_long(dsta); upper = get_long(dsta+4);\n");
	    break;
	 default:
	    abort ();
	}
	printf ("\tSET_ZFLG (upper == reg || lower == reg);\n");
	printf ("\tSET_CFLG_ALWAYS (lower <= upper ? reg < lower || reg > upper : reg > upper || reg < lower);\n");
	printf ("\tif ((extra & 0x800) && GET_CFLG ()) { Exception(6,oldpc); goto %s; }\n}\n", endlabelstr);
	need_endlabel = 1;
	break;

     case i_ASR:
	genamode (curi->smode, "srcreg", curi->size, "cnt", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	genamode (curi->dmode, "dstreg", curi->size, "data", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	start_brace ();
	switch (curi->size) {
	 case sz_byte: printf ("\tuae_u32 val = (uae_u8)data;\n"); break;
	 case sz_word: printf ("\tuae_u32 val = (uae_u16)data;\n"); break;
	 case sz_long: printf ("\tuae_u32 val = data;\n"); break;
	 default: abort ();
	}
	printf ("\tuae_u32 sign = (%s & val) >> %d;\n", cmask (curi->size), bit_size (curi->size) - 1);
	printf ("\tcnt &= 63;\n");
	printf ("\tCLEAR_CZNV();\n");
	printf ("\tif (cnt >= %d) {\n", bit_size (curi->size));
	printf ("\t\tval = %s & (uae_u32)-sign;\n", bit_mask (curi->size));
	printf ("\t\tSET_CFLG (sign);\n");
	duplicate_carry ();
	if (source_is_imm1_8 (curi))
	    printf ("\t} else {\n");
	else
	    printf ("\t} else if (cnt > 0) {\n");
	printf ("\t\tval >>= cnt - 1;\n");
	printf ("\t\tSET_CFLG (val & 1);\n");
	duplicate_carry ();
	printf ("\t\tval >>= 1;\n");
	printf ("\t\tval |= (%s << (%d - cnt)) & (uae_u32)-sign;\n",
		bit_mask (curi->size),
		bit_size (curi->size));
	printf ("\t\tval &= %s;\n", bit_mask (curi->size));
	printf ("\t}\n");
	genflags (flag_logical_noclobber, curi->size, "val", "", "");
	genastore ("val", curi->dmode, "dstreg", curi->size, "data", XLATE_LOG);
	break;
     case i_ASL:
	genamode (curi->smode, "srcreg", curi->size, "cnt", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	genamode (curi->dmode, "dstreg", curi->size, "data", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	start_brace ();
	switch (curi->size) {
	 case sz_byte: printf ("\tuae_u32 val = (uae_u8)data;\n"); break;
	 case sz_word: printf ("\tuae_u32 val = (uae_u16)data;\n"); break;
	 case sz_long: printf ("\tuae_u32 val = data;\n"); break;
	 default: abort ();
	}
	printf ("\tcnt &= 63;\n");
	printf ("\tCLEAR_CZNV();\n");
	printf ("\tif (cnt >= %d) {\n", bit_size (curi->size));
	printf ("\t\tSET_VFLG (val != 0);\n");
	printf ("\t\tSET_CFLG (cnt == %d ? val & 1 : 0);\n",
		bit_size (curi->size));
	duplicate_carry ();
	printf ("\t\tval = 0;\n");
	if (source_is_imm1_8 (curi))
	    printf ("\t} else {\n");
	else
	    printf ("\t} else if (cnt > 0) {\n");
	printf ("\t\tuae_u32 mask = (%s << (%d - cnt)) & %s;\n",
		bit_mask (curi->size),
		bit_size (curi->size) - 1,
		bit_mask (curi->size));
	printf ("\t\tSET_VFLG ((val & mask) != mask && (val & mask) != 0);\n");
	printf ("\t\tval <<= cnt - 1;\n");
	printf ("\t\tSET_CFLG ((val & %s) >> %d);\n", cmask (curi->size), bit_size (curi->size) - 1);
	duplicate_carry ();
	printf ("\t\tval <<= 1;\n");
	printf ("\t\tval &= %s;\n", bit_mask (curi->size));
	printf ("\t}\n");
	genflags (flag_logical_noclobber, curi->size, "val", "", "");
	genastore ("val", curi->dmode, "dstreg", curi->size, "data", XLATE_LOG);
	break;
     case i_LSR:
	genamode (curi->smode, "srcreg", curi->size, "cnt", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	genamode (curi->dmode, "dstreg", curi->size, "data", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	start_brace ();
	switch (curi->size) {
	 case sz_byte: printf ("\tuae_u32 val = (uae_u8)data;\n"); break;
	 case sz_word: printf ("\tuae_u32 val = (uae_u16)data;\n"); break;
	 case sz_long: printf ("\tuae_u32 val = data;\n"); break;
	 default: abort ();
	}
	printf ("\tcnt &= 63;\n");
	printf ("\tCLEAR_CZNV();\n");
	printf ("\tif (cnt >= %d) {\n", bit_size (curi->size));
	printf ("\t\tSET_CFLG ((cnt == %d) & (val >> %d));\n",
		bit_size (curi->size), bit_size (curi->size) - 1);
	duplicate_carry ();
	printf ("\t\tval = 0;\n");
	if (source_is_imm1_8 (curi))
	    printf ("\t} else {\n");
	else
	    printf ("\t} else if (cnt > 0) {\n");
	printf ("\t\tval >>= cnt - 1;\n");
	printf ("\t\tSET_CFLG (val & 1);\n");
	duplicate_carry ();
	printf ("\t\tval >>= 1;\n");
	printf ("\t}\n");
	genflags (flag_logical_noclobber, curi->size, "val", "", "");
	genastore ("val", curi->dmode, "dstreg", curi->size, "data", XLATE_LOG);
	break;
     case i_LSL:
	genamode (curi->smode, "srcreg", curi->size, "cnt", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	genamode (curi->dmode, "dstreg", curi->size, "data", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	start_brace ();
	switch (curi->size) {
	 case sz_byte: printf ("\tuae_u32 val = (uae_u8)data;\n"); break;
	 case sz_word: printf ("\tuae_u32 val = (uae_u16)data;\n"); break;
	 case sz_long: printf ("\tuae_u32 val = data;\n"); break;
	 default: abort ();
	}
	printf ("\tcnt &= 63;\n");
	printf ("\tCLEAR_CZNV();\n");
	printf ("\tif (cnt >= %d) {\n", bit_size (curi->size));
	printf ("\t\tSET_CFLG (cnt == %d ? val & 1 : 0);\n",
		bit_size (curi->size));
	duplicate_carry ();
	printf ("\t\tval = 0;\n");
	if (source_is_imm1_8 (curi))
	    printf ("\t} else {\n");
	else
	    printf ("\t} else if (cnt > 0) {\n");
	printf ("\t\tval <<= (cnt - 1);\n");
	printf ("\t\tSET_CFLG ((val & %s) >> %d);\n", cmask (curi->size), bit_size (curi->size) - 1);
	duplicate_carry ();
	printf ("\t\tval <<= 1;\n");
	printf ("\tval &= %s;\n", bit_mask (curi->size));
	printf ("\t}\n");
	genflags (flag_logical_noclobber, curi->size, "val", "", "");
	genastore ("val", curi->dmode, "dstreg", curi->size, "data", XLATE_LOG);
	break;
     case i_ROL:
	genamode (curi->smode, "srcreg", curi->size, "cnt", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	genamode (curi->dmode, "dstreg", curi->size, "data", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	start_brace ();
	switch (curi->size) {
	 case sz_byte: printf ("\tuae_u32 val = (uae_u8)data;\n"); break;
	 case sz_word: printf ("\tuae_u32 val = (uae_u16)data;\n"); break;
	 case sz_long: printf ("\tuae_u32 val = data;\n"); break;
	 default: abort ();
	}
	printf ("\tcnt &= 63;\n");
	printf ("\tCLEAR_CZNV();\n");
	if (source_is_imm1_8 (curi))
	    printf ("{");
	else
	    printf ("\tif (cnt > 0) {\n");
	printf ("\tuae_u32 loval;\n");
	printf ("\tcnt &= %d;\n", bit_size (curi->size) - 1);
	printf ("\tloval = val >> (%d - cnt);\n", bit_size (curi->size));
	printf ("\tval <<= cnt;\n");
	printf ("\tval |= loval;\n");
	printf ("\tval &= %s;\n", bit_mask (curi->size));
	printf ("\tSET_CFLG (val & 1);\n");
	printf ("}\n");
	genflags (flag_logical_noclobber, curi->size, "val", "", "");
	genastore ("val", curi->dmode, "dstreg", curi->size, "data", XLATE_LOG);
	break;
     case i_ROR:
	genamode (curi->smode, "srcreg", curi->size, "cnt", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	genamode (curi->dmode, "dstreg", curi->size, "data", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	start_brace ();
	switch (curi->size) {
	 case sz_byte: printf ("\tuae_u32 val = (uae_u8)data;\n"); break;
	 case sz_word: printf ("\tuae_u32 val = (uae_u16)data;\n"); break;
	 case sz_long: printf ("\tuae_u32 val = data;\n"); break;
	 default: abort ();
	}
	printf ("\tcnt &= 63;\n");
	printf ("\tCLEAR_CZNV();\n");
	if (source_is_imm1_8 (curi))
	    printf ("{");
	else
	    printf ("\tif (cnt > 0) {");
	printf ("\tuae_u32 hival;\n");
	printf ("\tcnt &= %d;\n", bit_size (curi->size) - 1);
	printf ("\thival = val << (%d - cnt);\n", bit_size (curi->size));
	printf ("\tval >>= cnt;\n");
	printf ("\tval |= hival;\n");
	printf ("\tval &= %s;\n", bit_mask (curi->size));
	printf ("\tSET_CFLG ((val & %s) >> %d);\n", cmask (curi->size), bit_size (curi->size) - 1);
	printf ("\t}\n");
	genflags (flag_logical_noclobber, curi->size, "val", "", "");
	genastore ("val", curi->dmode, "dstreg", curi->size, "data", XLATE_LOG);
	break;
     case i_ROXL:
	genamode (curi->smode, "srcreg", curi->size, "cnt", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	genamode (curi->dmode, "dstreg", curi->size, "data", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	start_brace ();
	switch (curi->size) {
	 case sz_byte: printf ("\tuae_u32 val = (uae_u8)data;\n"); break;
	 case sz_word: printf ("\tuae_u32 val = (uae_u16)data;\n"); break;
	 case sz_long: printf ("\tuae_u32 val = data;\n"); break;
	 default: abort ();
	}
	printf ("\tcnt &= 63;\n");
	printf ("\tCLEAR_CZNV();\n");
	if (source_is_imm1_8 (curi))
	    printf ("{");
	else {
	    force_range_for_rox ("cnt", curi->size);
	    printf ("\tif (cnt > 0) {\n");
	}
	printf ("\tcnt--;\n");
	printf ("\t{\n\tuae_u32 carry;\n");
	printf ("\tuae_u32 loval = val >> (%d - cnt);\n", bit_size (curi->size) - 1);
	printf ("\tcarry = loval & 1;\n");
	printf ("\tval = (((val << 1) | GET_XFLG ()) << cnt) | (loval >> 1);\n");
	printf ("\tSET_XFLG (carry);\n");
	printf ("\tval &= %s;\n", bit_mask (curi->size));
	printf ("\t} }\n");
	printf ("\tSET_CFLG (GET_XFLG ());\n");
	genflags (flag_logical_noclobber, curi->size, "val", "", "");
	genastore ("val", curi->dmode, "dstreg", curi->size, "data", XLATE_LOG);
	break;
     case i_ROXR:
	genamode (curi->smode, "srcreg", curi->size, "cnt", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	genamode (curi->dmode, "dstreg", curi->size, "data", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	start_brace ();
	switch (curi->size) {
	 case sz_byte: printf ("\tuae_u32 val = (uae_u8)data;\n"); break;
	 case sz_word: printf ("\tuae_u32 val = (uae_u16)data;\n"); break;
	 case sz_long: printf ("\tuae_u32 val = data;\n"); break;
	 default: abort ();
	}
	printf ("\tcnt &= 63;\n");
	printf ("\tCLEAR_CZNV();\n");
	if (source_is_imm1_8 (curi))
	    printf ("{");
	else {
	    force_range_for_rox ("cnt", curi->size);
	    printf ("\tif (cnt > 0) {\n");
	}
	printf ("\tcnt--;\n");
	printf ("\t{\n\tuae_u32 carry;\n");
	printf ("\tuae_u32 hival = (val << 1) | GET_XFLG ();\n");
	printf ("\thival <<= (%d - cnt);\n", bit_size (curi->size) - 1);
	printf ("\tval >>= cnt;\n");
	printf ("\tcarry = val & 1;\n");
	printf ("\tval >>= 1;\n");
	printf ("\tval |= hival;\n");
	printf ("\tSET_XFLG (carry);\n");
	printf ("\tval &= %s;\n", bit_mask (curi->size));
	printf ("\t} }\n");
	printf ("\tSET_CFLG (GET_XFLG ());\n");
	genflags (flag_logical_noclobber, curi->size, "val", "", "");
	genastore ("val", curi->dmode, "dstreg", curi->size, "data", XLATE_LOG);
	break;
     case i_ASRW:
	genamode (curi->smode, "srcreg", curi->size, "data", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	start_brace ();
	switch (curi->size) {
	 case sz_byte: printf ("\tuae_u32 val = (uae_u8)data;\n"); break;
	 case sz_word: printf ("\tuae_u32 val = (uae_u16)data;\n"); break;
	 case sz_long: printf ("\tuae_u32 val = data;\n"); break;
	 default: abort ();
	}
	printf ("\tuae_u32 sign = %s & val;\n", cmask (curi->size));
	printf ("\tuae_u32 cflg = val & 1;\n");
	printf ("\tval = (val >> 1) | sign;\n");
	genflags (flag_logical, curi->size, "val", "", "");
	printf ("\tSET_CFLG (cflg);\n");
	duplicate_carry ();
	genastore ("val", curi->smode, "srcreg", curi->size, "data", XLATE_LOG);
	break;
     case i_ASLW:
	genamode (curi->smode, "srcreg", curi->size, "data", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	start_brace ();
	switch (curi->size) {
	 case sz_byte: printf ("\tuae_u32 val = (uae_u8)data;\n"); break;
	 case sz_word: printf ("\tuae_u32 val = (uae_u16)data;\n"); break;
	 case sz_long: printf ("\tuae_u32 val = data;\n"); break;
	 default: abort ();
	}
	printf ("\tuae_u32 sign = %s & val;\n", cmask (curi->size));
	printf ("\tuae_u32 sign2;\n");
	printf ("\tval <<= 1;\n");
	genflags (flag_logical, curi->size, "val", "", "");
	printf ("\tsign2 = %s & val;\n", cmask (curi->size));
	printf ("\tSET_CFLG (sign != 0);\n");
	duplicate_carry ();

	printf ("\tSET_VFLG (GET_VFLG () | (sign2 != sign));\n");
	genastore ("val", curi->smode, "srcreg", curi->size, "data", XLATE_LOG);
	break;
     case i_LSRW:
	genamode (curi->smode, "srcreg", curi->size, "data", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	start_brace ();
	switch (curi->size) {
	 case sz_byte: printf ("\tuae_u32 val = (uae_u8)data;\n"); break;
	 case sz_word: printf ("\tuae_u32 val = (uae_u16)data;\n"); break;
	 case sz_long: printf ("\tuae_u32 val = data;\n"); break;
	 default: abort ();
	}
	printf ("\tuae_u32 carry = val & 1;\n");
	printf ("\tval >>= 1;\n");
	genflags (flag_logical, curi->size, "val", "", "");
	printf ("SET_CFLG (carry);\n");
	duplicate_carry ();
	genastore ("val", curi->smode, "srcreg", curi->size, "data", XLATE_LOG);
	break;
     case i_LSLW:
	genamode (curi->smode, "srcreg", curi->size, "data", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	start_brace ();
	switch (curi->size) {
	 case sz_byte: printf ("\tuae_u8 val = data;\n"); break;
	 case sz_word: printf ("\tuae_u16 val = data;\n"); break;
	 case sz_long: printf ("\tuae_u32 val = data;\n"); break;
	 default: abort ();
	}
	printf ("\tuae_u32 carry = val & %s;\n", cmask (curi->size));
	printf ("\tval <<= 1;\n");
	genflags (flag_logical, curi->size, "val", "", "");
	printf ("SET_CFLG (carry >> %d);\n", bit_size (curi->size) - 1);
	duplicate_carry ();
	genastore ("val", curi->smode, "srcreg", curi->size, "data", XLATE_LOG);
	break;
     case i_ROLW:
	genamode (curi->smode, "srcreg", curi->size, "data", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	start_brace ();
	switch (curi->size) {
	 case sz_byte: printf ("\tuae_u8 val = data;\n"); break;
	 case sz_word: printf ("\tuae_u16 val = data;\n"); break;
	 case sz_long: printf ("\tuae_u32 val = data;\n"); break;
	 default: abort ();
	}
	printf ("\tuae_u32 carry = val & %s;\n", cmask (curi->size));
	printf ("\tval <<= 1;\n");
	printf ("\tif (carry)  val |= 1;\n");
	genflags (flag_logical, curi->size, "val", "", "");
	printf ("SET_CFLG (carry >> %d);\n", bit_size (curi->size) - 1);
	genastore ("val", curi->smode, "srcreg", curi->size, "data", XLATE_LOG);
	break;
     case i_RORW:
	genamode (curi->smode, "srcreg", curi->size, "data", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	start_brace ();
	switch (curi->size) {
	 case sz_byte: printf ("\tuae_u8 val = data;\n"); break;
	 case sz_word: printf ("\tuae_u16 val = data;\n"); break;
	 case sz_long: printf ("\tuae_u32 val = data;\n"); break;
	 default: abort ();
	}
	printf ("\tuae_u32 carry = val & 1;\n");
	printf ("\tval >>= 1;\n");
	printf ("\tif (carry) val |= %s;\n", cmask (curi->size));
	genflags (flag_logical, curi->size, "val", "", "");
	printf ("SET_CFLG (carry);\n");
	genastore ("val", curi->smode, "srcreg", curi->size, "data", XLATE_LOG);
	break;
     case i_ROXLW:
	genamode (curi->smode, "srcreg", curi->size, "data", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	start_brace ();
	switch (curi->size) {
	 case sz_byte: printf ("\tuae_u8 val = data;\n"); break;
	 case sz_word: printf ("\tuae_u16 val = data;\n"); break;
	 case sz_long: printf ("\tuae_u32 val = data;\n"); break;
	 default: abort ();
	}
	printf ("\tuae_u32 carry = val & %s;\n", cmask (curi->size));
	printf ("\tval <<= 1;\n");
	printf ("\tif (GET_XFLG ()) val |= 1;\n");
	genflags (flag_logical, curi->size, "val", "", "");
	printf ("SET_CFLG (carry >> %d);\n", bit_size (curi->size) - 1);
	duplicate_carry ();
	genastore ("val", curi->smode, "srcreg", curi->size, "data", XLATE_LOG);
	break;
     case i_ROXRW:
	genamode (curi->smode, "srcreg", curi->size, "data", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	start_brace ();
	switch (curi->size) {
	 case sz_byte: printf ("\tuae_u8 val = data;\n"); break;
	 case sz_word: printf ("\tuae_u16 val = data;\n"); break;
	 case sz_long: printf ("\tuae_u32 val = data;\n"); break;
	 default: abort ();
	}
	printf ("\tuae_u32 carry = val & 1;\n");
	printf ("\tval >>= 1;\n");
	printf ("\tif (GET_XFLG ()) val |= %s;\n", cmask (curi->size));
	genflags (flag_logical, curi->size, "val", "", "");
	printf ("SET_CFLG (carry);\n");
	duplicate_carry ();
	genastore ("val", curi->smode, "srcreg", curi->size, "data", XLATE_LOG);
	break;
     case i_MOVEC2:
	genamode (curi->smode, "srcreg", curi->size, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	start_brace ();
	printf ("\tint regno = (src >> 12) & 15;\n");
	printf ("\tuae_u32 *regp = regs.regs + regno;\n");
	printf ("\tif (!m68k_movec2(src & 0xFFF, regp)) goto %s;\n", endlabelstr);
	break;
     case i_MOVE2C:
	genamode (curi->smode, "srcreg", curi->size, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	start_brace ();
	printf ("\tint regno = (src >> 12) & 15;\n");
	printf ("\tuae_u32 *regp = regs.regs + regno;\n");
	printf ("\tif (!m68k_move2c(src & 0xFFF, regp)) goto %s;\n", endlabelstr);
	break;
     case i_CAS:
	{
	    int old_brace_level;
	    genamode (curi->smode, "srcreg", curi->size, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	    genamode (curi->dmode, "dstreg", curi->size, "dst", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	    start_brace ();
	    printf ("\tint ru = (src >> 6) & 7;\n");
	    printf ("\tint rc = src & 7;\n");
	    genflags (flag_cmp, curi->size, "newv", "m68k_dreg(regs, rc)", "dst");
	    sync_m68k_pc ();
	    printf ("\tif (GET_ZFLG ())");
	    old_brace_level = n_braces;
	    start_brace ();
	    genastore ("(m68k_dreg(regs, ru))", curi->dmode, "dstreg", curi->size, "dst", XLATE_LOG);
	    pop_braces (old_brace_level);
	    printf ("else");
	    start_brace ();
	    switch (curi->size) {
	    case sz_byte:
		printf ("\tm68k_dreg(regs, rc) = (m68k_dreg(regs, rc) & ~0xff) | (dst & 0xff);\n");
		break;
	    case sz_word:
		printf ("\tm68k_dreg(regs, rc) = (m68k_dreg(regs, rc) & ~0xffff) | (dst & 0xffff);\n");
		break;
	    default:
		printf ("\tm68k_dreg(regs, rc) = dst;\n");
		break;
	    }
	    pop_braces (old_brace_level);
	}
	break;
     case i_CAS2:
	genamode (curi->smode, "srcreg", curi->size, "extra", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	printf ("\tuae_u32 rn1 = regs.regs[(extra >> 28) & 15];\n");
	printf ("\tuae_u32 rn2 = regs.regs[(extra >> 12) & 15];\n");
	if (curi->size == sz_word) {
	    int old_brace_level = n_braces;
	    printf ("\tuae_u32 rc1 = (extra >> 16) & 7;\n");
	    printf ("\tuae_u32 rc2 = extra & 7;\n");
	    printf ("\tuae_u16 dst1 = get_word(rn1), dst2 = get_word(rn2);\n");
	    genflags (flag_cmp, curi->size, "newv", "m68k_dreg(regs, rc1)", "dst1");
	    printf ("\tif (GET_ZFLG ()) {\n");
	    genflags (flag_cmp, curi->size, "newv", "m68k_dreg(regs, rc2)", "dst2");
	    printf ("\tif (GET_ZFLG ()) {\n");
	    printf ("\tput_word(rn1, m68k_dreg(regs, (extra >> 22) & 7));\n");
	    printf ("\tput_word(rn2, m68k_dreg(regs, (extra >> 6) & 7));\n");
	    printf ("\t}}\n");
	    pop_braces (old_brace_level);
	    printf ("\tif (! GET_ZFLG ()) {\n");
	    printf ("\tm68k_dreg(regs, rc2) = (m68k_dreg(regs, rc2) & ~0xffff) | (dst2 & 0xffff);\n");
	    printf ("\tm68k_dreg(regs, rc1) = (m68k_dreg(regs, rc1) & ~0xffff) | (dst1 & 0xffff);\n");
	    printf ("\t}\n");
	} else {
	    int old_brace_level = n_braces;
	    printf ("\tuae_u32 rc1 = (extra >> 16) & 7;\n");
	    printf ("\tuae_u32 rc2 = extra & 7;\n");
	    printf ("\tuae_u32 dst1 = get_long(rn1), dst2 = get_long(rn2);\n");
	    genflags (flag_cmp, curi->size, "newv", "m68k_dreg(regs, rc1)", "dst1");
	    printf ("\tif (GET_ZFLG ()) {\n");
	    genflags (flag_cmp, curi->size, "newv", "m68k_dreg(regs, rc2)", "dst2");
	    printf ("\tif (GET_ZFLG ()) {\n");
	    printf ("\tput_long(rn1, m68k_dreg(regs, (extra >> 22) & 7));\n");
	    printf ("\tput_long(rn2, m68k_dreg(regs, (extra >> 6) & 7));\n");
	    printf ("\t}}\n");
	    pop_braces (old_brace_level);
	    printf ("\tif (! GET_ZFLG ()) {\n");
	    printf ("\tm68k_dreg(regs, rc2) = dst2;\n");
	    printf ("\tm68k_dreg(regs, rc1) = dst1;\n");
	    printf ("\t}\n");
	}
	break;
     case i_MOVES:
	{
		int old_brace_level;

		genamode (curi->smode, "srcreg", curi->size, "extra", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
		start_brace();
		printf ("\tif (extra & 0x0800)\n");	/* from reg to ea */
		{
			int old_m68k_pc_offset = m68k_pc_offset;
			/* use DFC */
			old_brace_level = n_braces;
			start_brace ();
			printf ("\tuae_u32 src = regs.regs[(extra >> 12) & 15];\n");
			genamode (curi->dmode, "dstreg", curi->size, "dst", GENA_GETV_FETCH_ALIGN, GENA_MOVEM_DO_INC, XLATE_DFC);
			genastore ("src", curi->dmode, "dstreg", curi->size, "dst", XLATE_DFC);
			pop_braces (old_brace_level);
			m68k_pc_offset = old_m68k_pc_offset;
		}
		printf ("else");	/* from ea to reg */
		{
			/* use SFC */
			start_brace ();
			genamode (curi->dmode, "dstreg", curi->size, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_SFC);
			printf ("\tif (extra & 0x8000) {\n");	/* address/data */
			switch (curi->size) {
				case sz_byte: printf ("\tm68k_areg(regs, (extra >> 12) & 7) = (uae_s32)(uae_s8)src;\n"); break;
				case sz_word: printf ("\tm68k_areg(regs, (extra >> 12) & 7) = (uae_s32)(uae_s16)src;\n"); break;
				case sz_long: printf ("\tm68k_areg(regs, (extra >> 12) & 7) = src;\n"); break;
				default: abort ();
			}
			printf ("\t} else {\n");
			genastore ("src", Dreg, "(extra >> 12) & 7", curi->size, "", XLATE_LOG);
			printf ("\t}\n");
			sync_m68k_pc();
			pop_braces (old_brace_level);
		}
	}
	break;
     case i_BKPT:		/* only needed for hardware emulators */
	sync_m68k_pc ();
	printf ("\top_illg(opcode);\n");
	break;
     case i_CALLM:		/* not present in 68030 */
	sync_m68k_pc ();
	printf ("\top_illg(opcode);\n");
	break;
     case i_RTM:		/* not present in 68030 */
	sync_m68k_pc ();
	printf ("\top_illg(opcode);\n");
	break;
     case i_TRAPcc:
	printf ("\tuaecptr oldpc = m68k_getpc();\n");
	if (curi->smode != am_unknown && curi->smode != am_illg)
	    genamode (curi->smode, "srcreg", curi->size, "dummy", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	sync_m68k_pc ();
	printf ("\tif (cctrue(%d)) { Exception(7,oldpc); goto %s; }\n", curi->cc, endlabelstr);
	need_endlabel = 1;
	break;
     case i_DIVL:
	printf ("\tuaecptr oldpc = m68k_getpc();\n");
	genamode (curi->smode, "srcreg", curi->size, "extra", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	genamode (curi->dmode, "dstreg", curi->size, "dst", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	sync_m68k_pc ();
	printf ("\tm68k_divl(opcode, dst, extra, oldpc);\n");
	break;
     case i_MULL:
	genamode (curi->smode, "srcreg", curi->size, "extra", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	genamode (curi->dmode, "dstreg", curi->size, "dst", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	sync_m68k_pc ();
	printf ("\tm68k_mull(opcode, dst, extra);\n");
	break;
     case i_BFTST:
     case i_BFEXTU:
     case i_BFCHG:
     case i_BFEXTS:
     case i_BFCLR:
     case i_BFFFO:
     case i_BFSET:
     case i_BFINS:
	genamode (curi->smode, "srcreg", curi->size, "extra", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	genamode (curi->dmode, "dstreg", sz_long, "dst", GENA_GETV_FETCH_ALIGN, GENA_MOVEM_DO_INC, XLATE_LOG);
	start_brace ();
	printf ("\tuae_u32 bdata[2];");
	printf ("\tuae_s32 offset = extra & 0x800 ? m68k_dreg(regs, (extra >> 6) & 7) : (extra >> 6) & 0x1f;\n");
	printf ("\tint width = (((extra & 0x20 ? m68k_dreg(regs, extra & 7) : extra) -1) & 0x1f) +1;\n");
	if (curi->dmode == Dreg) {
	    printf ("\tuae_u32 tmp = m68k_dreg(regs, dstreg);\n");
	    printf ("\toffset &= 0x1f;\n");
	    printf ("\ttmp = (tmp << offset) | (tmp >> (32 - offset));\n");
	    printf ("\tbdata[0] = tmp & ((1 << (32 - width)) - 1);\n");
	} else {
	    printf ("\tuae_u32 tmp;\n");
	    printf ("\tdsta += offset >> 3;\n");
	    printf ("\ttmp = get_bitfield(dsta, bdata, offset, width);\n");
	}
	printf ("\tSET_NFLG_ALWAYS (((uae_s32)tmp) < 0 ? 1 : 0);\n");
	if (curi->mnemo == i_BFEXTS)
		printf ("\ttmp = (uae_s32)tmp >> (32 - width);\n");
	else
		printf ("\ttmp >>= (32 - width);\n");
	printf ("\tSET_ZFLG (tmp == 0); SET_VFLG (0); SET_CFLG (0);\n");
	switch (curi->mnemo) {
	 case i_BFTST:
	    break;
	 case i_BFEXTU:
	 case i_BFEXTS:
	    printf ("\tm68k_dreg(regs, (extra >> 12) & 7) = tmp;\n");
	    break;
	 case i_BFCHG:
	    printf ("\ttmp = tmp ^ (0xffffffffu >> (32 - width));\n");
	    break;
	 case i_BFCLR:
	    printf ("\ttmp = 0;\n");
	    break;
	 case i_BFFFO:
	    printf ("\t{ uae_u32 mask = 1 << (width-1);\n");
	    printf ("\twhile (mask) { if (tmp & mask) break; mask >>= 1; offset++; }}\n");
	    printf ("\tm68k_dreg(regs, (extra >> 12) & 7) = offset;\n");
	    break;
	 case i_BFSET:
	    printf ("\ttmp = 0xffffffffu >> (32 - width);\n");
	    break;
	 case i_BFINS:
	    printf ("\ttmp = m68k_dreg(regs, (extra >> 12) & 7);\n");
	    printf ("\ttmp = tmp & (0xffffffffu >> (32 - width));\n");
	    printf ("\tSET_NFLG_ALWAYS (tmp & (1 << (width - 1)) ? 1 : 0);\n");
	    printf ("\tSET_ZFLG (tmp == 0);\n");
	    break;
	 default:
	    break;
	}
	if (curi->mnemo == i_BFCHG
	    || curi->mnemo == i_BFCLR
	    || curi->mnemo == i_BFSET
	    || curi->mnemo == i_BFINS) {
	    if (curi->dmode == Dreg) {
		printf ("\ttmp = bdata[0] | (tmp << (32 - width));\n");
	        printf ("\tm68k_dreg(regs, dstreg) = (tmp >> offset) | (tmp << (32 - offset));\n");
	    } else {
		printf ("\tput_bitfield(dsta, bdata, tmp, offset, width);\n");
	    }
	}
	break;
     case i_PACK:
	if (curi->smode == Dreg) {
	    printf ("\tuae_u16 val = m68k_dreg(regs, srcreg) + %s;\n", gen_nextiword ());
	    printf ("\tm68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & 0xffffff00) | ((val >> 4) & 0xf0) | (val & 0xf);\n");
	} else {
	    printf ("\tuae_u16 val;\n");
	    printf ("\tval = (uae_u16)get_byte(m68k_areg(regs, srcreg) - areg_byteinc[srcreg]);\n");
	    printf ("\tval = (val | ((uae_u16)get_byte(m68k_areg(regs, srcreg) - 2 * areg_byteinc[srcreg]) << 8)) + %s;\n", gen_nextiword ());
	    printf ("\tm68k_areg(regs, srcreg) -= 2;\n");
	    printf ("\tm68k_areg(regs, dstreg) -= areg_byteinc[dstreg];\n");
	    gen_set_fault_pc ();
	    printf ("\tput_byte(m68k_areg(regs, dstreg),((val >> 4) & 0xf0) | (val & 0xf));\n");
	}
	break;
     case i_UNPK:
	if (curi->smode == Dreg) {
	    printf ("\tuae_u16 val = m68k_dreg(regs, srcreg);\n");
	    printf ("\tval = (((val << 4) & 0xf00) | (val & 0xf)) + %s;\n", gen_nextiword ());
	    printf ("\tm68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & 0xffff0000) | (val & 0xffff);\n");
	} else {
	    printf ("\tuae_u16 val;\n");
	    printf ("\tval = (uae_u16)get_byte(m68k_areg(regs, srcreg) - areg_byteinc[srcreg]);\n");
	    printf ("\tval = (((val << 4) & 0xf00) | (val & 0xf)) + %s;\n", gen_nextiword ());
	    printf ("\tm68k_areg(regs, srcreg) -= areg_byteinc[srcreg];\n");
	    printf ("\tm68k_areg(regs, dstreg) -= 2;\n");
	    gen_set_fault_pc ();
	    printf ("\tput_word(m68k_areg(regs, dstreg), val);\n");
	}
	break;
     case i_TAS:
	genamode (curi->smode, "srcreg", curi->size, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	genflags (flag_logical, curi->size, "src", "", "");
	printf ("\tsrc |= 0x80;\n");
	genastore ("src", curi->smode, "srcreg", curi->size, "src", XLATE_LOG);
	break;
     case i_FPP:
	genamode (curi->smode, "srcreg", curi->size, "extra", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	sync_m68k_pc ();
	swap_opcode ();
	printf ("\tfpuop_arithmetic(opcode, extra);\n");
	break;
     case i_FDBcc:
	genamode (curi->smode, "srcreg", curi->size, "extra", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	sync_m68k_pc ();
	swap_opcode ();
	printf ("\tfpuop_dbcc(opcode, extra);\n");
	break;
     case i_FScc:
	genamode (curi->smode, "srcreg", curi->size, "extra", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	sync_m68k_pc ();
	swap_opcode ();
	printf ("\tfpuop_scc(opcode, extra);\n");
	break;
     case i_FTRAPcc:
	sync_m68k_pc ();
	start_brace ();
	printf ("\tuaecptr oldpc = m68k_getpc();\n");
	printf ("\tuae_u16 extra = %s;\n", gen_nextiword());
	if (curi->smode != am_unknown && curi->smode != am_illg)
	    genamode (curi->smode, "srcreg", curi->size, "dummy", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	sync_m68k_pc ();
	swap_opcode ();
	printf ("\tfpuop_trapcc(opcode, oldpc, extra);\n");
	break;
     case i_FBcc:
	sync_m68k_pc ();
	start_brace ();
	printf ("\tuaecptr pc = m68k_getpc();\n");
	genamode (curi->dmode, "srcreg", curi->size, "extra", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	sync_m68k_pc ();
	swap_opcode ();
	printf ("\tfpuop_bcc(opcode, pc, extra);\n");
	break;
     case i_FSAVE:
	sync_m68k_pc ();
	swap_opcode ();
	printf ("\tfpuop_save(opcode);\n");
	break;
     case i_FRESTORE:
	sync_m68k_pc ();
	swap_opcode ();
	printf ("\tfpuop_restore(opcode);\n");
	break;
     case i_CINVL:
	printf ("\tflush_internals();\n");
        printf("#ifdef USE_JIT\n");
 	printf ("\tif (real_opcode&0x80)\n"
		"\t\tflush_icache();\n");
	printf("#endif\n");
	break;
     case i_CINVP:
	printf ("\tflush_internals();\n");
        printf("#ifdef USE_JIT\n");
	printf ("\tif (real_opcode&0x80)\n"
		"\t\tflush_icache();\n");
	printf("#endif\n");
	break;
     case i_CINVA:
	printf ("\tflush_internals();\n");
        printf("#ifdef USE_JIT\n");
	printf ("\tif (real_opcode&0x80)\n"
		"\t\tflush_icache();\n");
	printf("#endif\n");
	break;
     case i_CPUSHL:
	printf ("\tflush_internals();\n");
        printf("#ifdef USE_JIT\n");
	printf ("\tif (real_opcode&0x80)\n"
		"\t\tflush_icache();\n");
	printf("#endif\n");
	break;
     case i_CPUSHP:
	printf ("\tflush_internals();\n");
        printf("#ifdef USE_JIT\n");
	printf ("\tif (real_opcode&0x80)\n"
		"\t\tflush_icache();\n");
	printf("#endif\n");
	break;
     case i_CPUSHA:
	printf ("\tflush_internals();\n");
        printf("#ifdef USE_JIT\n");
	printf ("\tif (real_opcode&0x80)\n"
		"\t\tflush_icache();\n");
	printf("#endif\n");
	break;
     case i_MOVE16:
	 if ((opcode & 0xfff8) == 0xf620) {
	     /* MOVE16 (Ax)+,(Ay)+ */
	     printf ("\tuaecptr mems = m68k_areg(regs, srcreg) & ~15, memd;\n");
	     printf ("\tdstreg = (%s >> 12) & 7;\n", gen_nextiword());
	     printf ("\tmemd = m68k_areg(regs, dstreg) & ~15;\n");
	     printf ("\tput_long(memd, get_long(mems));\n");
	     printf ("\tput_long(memd+4, get_long(mems+4));\n");
	     printf ("\tput_long(memd+8, get_long(mems+8));\n");
	     printf ("\tput_long(memd+12, get_long(mems+12));\n");
	     printf ("\tif (srcreg != dstreg)\n");
	     printf ("\tm68k_areg(regs, srcreg) += 16;\n");
	     printf ("\tm68k_areg(regs, dstreg) += 16;\n");
	 } else {
	     /* Other variants */
	     genamode (curi->smode, "srcreg", curi->size, "mems", GENA_GETV_NO_FETCH, GENA_MOVEM_MOVE16, XLATE_LOG);
	     genamode (curi->dmode, "dstreg", curi->size, "memd", GENA_GETV_NO_FETCH, GENA_MOVEM_MOVE16, XLATE_LOG);
	     printf ("\tmemsa &= ~15;\n");
	     printf ("\tmemda &= ~15;\n");
	     printf ("\tput_long(memda, get_long(memsa));\n");
	     printf ("\tput_long(memda+4, get_long(memsa+4));\n");
	     printf ("\tput_long(memda+8, get_long(memsa+8));\n");
	     printf ("\tput_long(memda+12, get_long(memsa+12));\n");
	     if ((opcode & 0xfff8) == 0xf600)
                 printf ("\tm68k_areg(regs, srcreg) += 16;\n");
	     else if ((opcode & 0xfff8) == 0xf608)
		 printf ("\tm68k_areg(regs, dstreg) += 16;\n");
	 }
	 break;
     case i_MMUOP:
	genamode (curi->smode, "srcreg", curi->size, "extra", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, XLATE_LOG);
	sync_m68k_pc ();
	swap_opcode ();
	printf ("\tmmu_op(opcode,extra);\n");
	break;

     case i_EMULOP_RETURN:
	printf ("\tm68k_emulop_return();\n");
	m68k_pc_offset = 0;
	break;
	
     case i_EMULOP:
	printf ("\n");
	swap_opcode ();
	printf ("\tm68k_emulop(opcode);\n");
	break;

     case i_NATFEAT_ID:
	printf ("\n");
	printf ("\tm68k_natfeat_id();\n");
	break;

     case i_NATFEAT_CALL:
	printf ("\n");
	printf ("\tm68k_natfeat_call();\n");
	break;

     default:
	abort ();
	break;
    }
    finish_braces ();
    sync_m68k_pc ();
}

static void generate_includes (FILE * f)
{
    fprintf (f, "#include \"sysdeps.h\"\n");
    fprintf (f, "#include \"m68k.h\"\n");
    fprintf (f, "#include \"memory-uae.h\"\n");
    fprintf (f, "#include \"readcpu.h\"\n");
    fprintf (f, "#include \"newcpu.h\"\n");
    fprintf (f, "#ifdef USE_JIT\n");
    fprintf (f, "#include \"compiler/compemu.h\"\n");
    fprintf (f, "#endif\n");
    fprintf (f, "#include \"fpu/fpu.h\"\n");
    fprintf (f, "#include \"cputbl.h\"\n");
    fprintf (f, "#include \"cpu_emulation.h\"\n");
    fprintf (f, "#include \"debug.h\"\n");

    fprintf (f, "#define SET_CFLG_ALWAYS(x) SET_CFLG(x)\n");
    fprintf (f, "#define SET_NFLG_ALWAYS(x) SET_NFLG(x)\n");
    fprintf (f, "#define CPUFUNC_FF(x) x##_ff\n");
    fprintf (f, "#define CPUFUNC_NF(x) x##_nf\n");
    fprintf (f, "#define CPUFUNC(x) CPUFUNC_FF(x)\n");
	
    fprintf (f, "#ifdef NOFLAGS\n");
    fprintf (f, "# include \"noflags.h\"\n");
    fprintf (f, "#endif\n");
}

static int postfix;

struct gencputbl {
    char handler[80];
    uae_u16 specific;
    uae_u16 opcode;
    int namei;
};
struct gencputbl cpustbl[65536];
static int n_cpustbl;

static char *decodeEA (amodes mode, wordsizes size)
{
	static char buffer[80];

	buffer[0] = 0;
	switch (mode){
	case Dreg:
		strcpy (buffer,"Dn");
		break;
	case Areg:
		strcpy (buffer,"An");
		break;
	case Aind:
		strcpy (buffer,"(An)");
		break;
	case Aipi:
		strcpy (buffer,"(An)+");
		break;
	case Apdi:
		strcpy (buffer,"-(An)");
		break;
	case Ad16:
		strcpy (buffer,"(d16,An)");
		break;
	case Ad8r:
		strcpy (buffer,"(d8,An,Xn)");
		break;
	case PC16:
		strcpy (buffer,"(d16,PC)");
		break;
	case PC8r:
		strcpy (buffer,"(d8,PC,Xn)");
		break;
	case absw:
		strcpy (buffer,"(xxx).W");
		break;
	case absl:
		strcpy (buffer,"(xxx).L");
		break;
	case imm:
		switch (size){
		case sz_byte:
			strcpy (buffer,"#<data>.B");
			break;
		case sz_word:
			strcpy (buffer,"#<data>.W");
			break;
		case sz_long:
			strcpy (buffer,"#<data>.L");
			break;
		default:
			break;
		}
		break;
	case imm0:
		strcpy (buffer,"#<data>.B");
		break;
	case imm1:
		strcpy (buffer,"#<data>.W");
		break;
	case imm2:
		strcpy (buffer,"#<data>.L");
		break;
	case immi:
		strcpy (buffer,"#<data>");
		break;

	default:
		break;
	}
	return buffer;
}

static char *outopcode (const char *name, int opcode)
{
	static char out[100];
	struct instr *ins;

	ins = &table68k[opcode];
	strcpy (out, name);
	if (ins->smode == immi)
		strcat (out, "Q");
	if (ins->size == sz_byte)
		strcat (out,".B");
	if (ins->size == sz_word)
		strcat (out,".W");
	if (ins->size == sz_long)
		strcat (out,".L");
	strcat (out," ");
	if (ins->suse)
		strcat (out, decodeEA (ins->smode, ins->size));
	if (ins->duse) {
		if (ins->suse) strcat (out,",");
		strcat (out, decodeEA (ins->dmode, ins->size));
	}
	return out;
}


static void generate_one_opcode (int rp)
{
    int i;
    uae_u16 smsk, dmsk;
    int opcode = opcode_map[rp];
	int have_realopcode = 0;
	const char *name;

    if (table68k[opcode].mnemo == i_ILLG
	|| table68k[opcode].clev > cpu_level)
	return;

    for (i = 0; lookuptab[i].name[0]; i++) {
	if (table68k[opcode].mnemo == lookuptab[i].mnemo)
	    break;
    }

    if (table68k[opcode].handler != -1)
	return;

    name = lookuptab[i].name;
    if (opcode_next_clev[rp] != cpu_level) {
    	sprintf(cpustbl[n_cpustbl].handler, "CPUFUNC(op_%x_%d)", opcode, opcode_last_postfix[rp]);
    	cpustbl[n_cpustbl].specific = 0;
    	cpustbl[n_cpustbl].opcode = opcode;
    	cpustbl[n_cpustbl].namei = i;
	fprintf (stblfile, "{ %s, %d, %d }, /* %s */\n", cpustbl[n_cpustbl].handler, cpustbl[n_cpustbl].specific, opcode, name);
	n_cpustbl++;
	return;
    }

	if (table68k[opcode].flagdead == 0)
	/* force to the "ff" variant since the instruction doesn't set at all the condition codes */
    sprintf (cpustbl[n_cpustbl].handler, "CPUFUNC_FF(op_%x_%d)", opcode, postfix);
	else
    sprintf (cpustbl[n_cpustbl].handler, "CPUFUNC(op_%x_%d)", opcode, postfix);
   	cpustbl[n_cpustbl].specific = 0;
   	cpustbl[n_cpustbl].opcode = opcode;
   	cpustbl[n_cpustbl].namei = i;
	fprintf (stblfile, "{ %s, %d, %d }, /* %s */\n", cpustbl[n_cpustbl].handler, cpustbl[n_cpustbl].specific, opcode, name);
	n_cpustbl++;

    fprintf (headerfile, "extern cpuop_func op_%x_%d_nf;\n", opcode, postfix);
    fprintf (headerfile, "extern cpuop_func op_%x_%d_ff;\n", opcode, postfix);
    
    printf ("/* %s */\n", outopcode (name, opcode));
    printf ("void REGPARAM2 CPUFUNC(op_%x_%d)(uae_u32 opcode) /* %s */\n{\n", opcode, postfix, name);
    printf ("\tcpuop_begin();\n");
	/* gb-- The "nf" variant for an instruction that doesn't set the condition
	   codes at all is the same as the "ff" variant, so we don't need the "nf"
	   variant to be compiled since it is mapped to the "ff" variant in the
	   smalltbl. */
    if (table68k[opcode].flagdead == 0)
	printf ("#ifndef NOFLAGS\n");

    switch (table68k[opcode].stype) {
     case 0: smsk = 7; break;
     case 1: smsk = 255; break;
     case 2: smsk = 15; break;
     case 3: smsk = 7; break;
     case 4: smsk = 7; break;
     case 5: smsk = 63; break;
     case 6: smsk = 255; break;
     case 7: smsk = 3; break;
     default: abort ();
    }
    dmsk = 7;

    next_cpu_level = -1;
    if (table68k[opcode].suse
	&& table68k[opcode].smode != imm && table68k[opcode].smode != imm0
	&& table68k[opcode].smode != imm1 && table68k[opcode].smode != imm2
	&& table68k[opcode].smode != absw && table68k[opcode].smode != absl
	&& table68k[opcode].smode != PC8r && table68k[opcode].smode != PC16
	/* gb-- We don't want to fetch the EmulOp code since the EmulOp()
	   routine uses the whole opcode value. Maybe all the EmulOps
	   could be expanded out but I don't think it is an improvement */
	&& table68k[opcode].stype != 6
	)
    {
	if (table68k[opcode].spos == -1) {
	    if (((int) table68k[opcode].sreg) >= 128)
		printf ("\tuae_u32 srcreg = (uae_s32)(uae_s8)%d;\n", (int) table68k[opcode].sreg);
	    else
		printf ("\tuae_u32 srcreg = %d;\n", (int) table68k[opcode].sreg);
	} else {
	    char source[100];
	    int pos = table68k[opcode].spos;

#if 0
	    /* Check that we can do the little endian optimization safely.  */
	    if (pos < 8 && (smsk >> (8 - pos)) != 0)
		abort ();
#endif
		real_opcode(&have_realopcode);
	    if (pos)
		sprintf (source, "((real_opcode >> %d) & %d)", pos, smsk);
	    else
		sprintf (source, "(real_opcode & %d)", smsk);
	    if (table68k[opcode].stype == 3)
		printf ("\tuae_u32 srcreg = imm8_table[%s];\n", source);
	    else if (table68k[opcode].stype == 1)
		printf ("\tuae_u32 srcreg = (uae_s32)(uae_s8)%s;\n", source);
	    else
		printf ("\tuae_u32 srcreg = %s;\n", source);
	}
    }
    if (table68k[opcode].duse
	/* Yes, the dmode can be imm, in case of LINK or DBcc */
	&& table68k[opcode].dmode != imm && table68k[opcode].dmode != imm0
	&& table68k[opcode].dmode != imm1 && table68k[opcode].dmode != imm2
	&& table68k[opcode].dmode != absw && table68k[opcode].dmode != absl)
    {
	if (table68k[opcode].dpos == -1) {
	    if (((int) table68k[opcode].dreg) >= 128)
		printf ("\tuae_u32 dstreg = (uae_s32)(uae_s8)%d;\n", (int) table68k[opcode].dreg);
	    else
		printf ("\tuae_u32 dstreg = %d;\n", (int) table68k[opcode].dreg);
	} else {
	    int pos = table68k[opcode].dpos;
#if 0
	    /* Check that we can do the little endian optimization safely.  */
	    if (pos < 8 && (dmsk >> (8 - pos)) != 0)
		abort ();
#endif
		real_opcode(&have_realopcode);
	    if (pos)
		printf ("\tuae_u32 dstreg = (real_opcode >> %d) & %d;\n",
			pos, dmsk);
	    else
		printf ("\tuae_u32 dstreg = real_opcode & %d;\n", dmsk);
	}
    }
    need_endlabel = 0;
    endlabelno++;
    sprintf (endlabelstr, "endlabel%d", endlabelno);
    gen_opcode (opcode);
    if (need_endlabel)
	printf ("%s: ;\n", endlabelstr);
    if (table68k[opcode].flagdead == 0)
	printf ("\n#endif\n");
    printf ("\tcpuop_end();\n");
    printf ("}\n");
    opcode_next_clev[rp] = next_cpu_level;
    opcode_last_postfix[rp] = postfix;
}

static void generate_func (void)
{
    int i, j, rp;

    using_prefetch = 0;
    using_exception_3 = 0;

    for (i = 0; i < 1; i++) {
	cpu_level = 4 - i;
	postfix = i;
	fprintf (stblfile, "const struct cputbl CPUFUNC(op_smalltbl_%d)[] = {\n", postfix);

	/* sam: this is for people with low memory (eg. me :)) */
	printf ("\n"
		"#if !defined(PART_1) && !defined(PART_2) && "
	 	"!defined(PART_3) && !defined(PART_4) && "
		"!defined(PART_5) && !defined(PART_6) && "
		"!defined(PART_7) && !defined(PART_8)"
		"\n"
	        "#define PART_1 1\n"
	        "#define PART_2 1\n"
	        "#define PART_3 1\n"
	        "#define PART_4 1\n"
	        "#define PART_5 1\n"
	        "#define PART_6 1\n"
	        "#define PART_7 1\n"
	        "#define PART_8 1\n"
	        "#endif\n\n");
	rp = 0;
	n_cpustbl = 0;
	for(j=1;j<=8;++j) {
		int k = (j*nr_cpuop_funcs)/8;
		printf ("#ifdef PART_%d\n",j);
		for (; rp < k; rp++)
		   generate_one_opcode (rp);
		printf ("#endif\n\n");
	}
	fprintf (stblfile, "{ 0, 0, 0 }};\n");
    }
}

static struct {
	const char *handler;
	const char *name;
} cpufunctbl[65536];
static char const op_illg_1[] = "op_illg_1";
static char const illegal[] = "ILLEGAL";

static void generate_functbl (void)
{
	int i;
	unsigned int opcode;
	int cpu_level = 4;
	struct gencputbl *tbl = cpustbl;

	for (opcode = 0; opcode < 65536; opcode++)
	{
		cpufunctbl[opcode].handler = op_illg_1;
		cpufunctbl[opcode].name = illegal;
	}
	for (i = 0; i < n_cpustbl; i++)
	{
		if (! tbl[i].specific)
		{
			cpufunctbl[tbl[i].opcode].handler = tbl[i].handler;
			cpufunctbl[tbl[i].opcode].name = lookuptab[tbl[i].namei].name;
		}
	}
	for (opcode = 0; opcode < 65536; opcode++)
	{
		const char *f;

		if (table68k[opcode].mnemo == i_ILLG || (unsigned)table68k[opcode].clev > (unsigned)cpu_level)
			continue;

		if (table68k[opcode].handler != -1)
		{
			f = cpufunctbl[table68k[opcode].handler].handler;
			if (f == op_illg_1)
				abort();
			cpufunctbl[opcode].handler = f;
			cpufunctbl[opcode].name = cpufunctbl[table68k[opcode].handler].name;
		}
	}
	for (i = 0; i < n_cpustbl; i++)
	{
		if (tbl[i].specific)
		{
			cpufunctbl[tbl[i].opcode].handler = tbl[i].handler;
			cpufunctbl[tbl[i].opcode].name = lookuptab[tbl[i].namei].name;
		}
	}
	
	fprintf(functblfile, "\n");
	fprintf(functblfile, "cpuop_func *cpufunctbl[65536] = {\n");
	fprintf(functblfile, "#if !defined(HAVE_GET_WORD_UNSWAPPED) || defined(FULLMMU)\n");
	for (opcode = 0; opcode < 65536; opcode++)
	{
		fprintf(functblfile, "\t%s%s /* %s */\n", cpufunctbl[opcode].handler, opcode < 65535 ? "," : "", cpufunctbl[opcode].name);
	}
	fprintf(functblfile, "#else\n");
	for (opcode = 0; opcode < 65536; opcode++)
	{
		unsigned int map = do_byteswap_16(opcode);
		fprintf(functblfile, "\t%s%s /* %s */\n", cpufunctbl[map].handler, opcode < 65535 ? "," : "", cpufunctbl[map].name);
	}
	fprintf(functblfile, "#endif\n");
	fprintf(functblfile, "};\n");
}

#if (defined(OS_cygwin) || defined(OS_mingw)) && defined(EXTENDED_SIGSEGV)
void cygwin_mingw_abort()
{
#undef abort
	abort();
}
#endif

int main(void)
{
    init_table68k ();

    opcode_map = (int *) malloc (sizeof (int) * nr_cpuop_funcs);
    opcode_last_postfix = (int *) malloc (sizeof (int) * nr_cpuop_funcs);
    opcode_next_clev = (int *) malloc (sizeof (int) * nr_cpuop_funcs);
    counts = (unsigned long *) malloc (65536 * sizeof (unsigned long));
    read_counts ();

    /* It would be a lot nicer to put all in one file (we'd also get rid of
     * cputbl.h that way), but cpuopti can't cope.  That could be fixed, but
     * I don't dare to touch the 68k version.  */

    if ((headerfile = fopen ("cputbl.h", "wb")) == NULL)
    	abort();
    if ((stblfile = fopen ("cpustbl.cpp", "wb")) == NULL)
    	abort(); 
    if ((functblfile = fopen ("cpufunctbl.cpp", "wb")) == NULL)
    	abort(); 
    if (freopen ("cpuemu.cpp", "wb", stdout) == NULL)
    	abort();

    generate_includes (stdout);
    generate_includes (stblfile);
    generate_includes (functblfile);
    generate_func ();
    generate_functbl ();
    free (table68k);
    fclose(headerfile);
    fclose(stblfile);
    fclose(functblfile);
    return 0;
}
