/*
 * disasm-glue.cpp - interface to either opcode library or builtin disassemblers
 *
 * Copyright (c) 2001-2018 Thorsten Otto of ARAnyM dev team (see AUTHORS)
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
 
#include "sysdeps.h"

#include "memory-uae.h"
#include "newcpu.h"
#include "m68k.h"
#include "cpu_emulation.h"
#include "disasm-glue.h"

#ifdef HAVE_DISASM_M68K /* rest of file */

/*
 * kept here rather than in debug.cpp
 * (which normally is the only place where it is used),
 * so we can enable a disassembler without debugger support,
 * and call some functions from gdb.
 */
m68k_disasm_info disasm_info;


#ifdef DISASM_USE_OPCODES
#include <dis-asm.h>
#include <stdarg.h>

/*
 * newer versions of dis-asm.h don't declare the
 * functions :(
 */
#ifdef __cplusplus
extern "C" {
#endif
extern int print_insn_m68k(bfd_vma, disassemble_info *);
#ifdef __cplusplus
}
#endif

/*
 * on 64-bit hosts, libopcodes prints all addresses with 16 digits,
 * even if we are disassembling for a 32bit processor
 */
static void print_32bit_address (bfd_vma addr, struct disassemble_info *info)
{
	(*info->fprintf_func) (info->stream, "0x%08x", (unsigned int)addr);
}

struct opcodes_info {
	char linebuf[128];
	size_t bufsize;
	size_t linepos;
	disassemble_info *opcodes_info;
};


static int opcodes_printf(void *info, const char *format, ...)
{
	struct opcodes_info *opcodes_info = (struct opcodes_info *)info;
	va_list args;
	int len;
	size_t remain;
	
	va_start(args, format);
	remain = opcodes_info->bufsize - opcodes_info->linepos - 1;
	len = vsnprintf(opcodes_info->linebuf + opcodes_info->linepos, remain, format, args);
	if (len > 0)
	{
		if ((size_t)len > remain)
			len = remain;
		opcodes_info->linepos += len;
	}
	va_end(args);
	return len;
}

#else

#include "disasm-bfd.h"
#include "disasm-builtin.h"

#endif /* DISASM_USE_OPCODES */


void m68k_disasm_init(m68k_disasm_info *info, enum m68k_cpu cpu)
{
	/*
	 * we always want disassembly for at least '40, because the BIOS contains
	 * opcodes for it.
	 */
	if (cpu < CPU_68040)
		info->cpu = CPU_68040;
	else
		info->cpu = cpu;
	info->fpu = FPU_68881;
	info->mmu = cpu >= CPU_68040 ? MMU_68040 : MMU_NONE;
	info->is_64bit = FALSE;
	info->memory_vma = m68k_getpc();
	info->application_data = NULL;
	info->opcode[0] = '\0';
	info->operands[0] = '\0';
	info->comments[0] = '\0';
	info->num_oper = 0;
	info->num_insn_words = 0;
	info->disasm_data = NULL;
#ifdef DISASM_USE_OPCODES
	{
		struct opcodes_info *opcodes_info;
		
		opcodes_info = (struct opcodes_info *)info->disasm_data;
		if (opcodes_info == NULL)
		{
			opcodes_info = (struct opcodes_info *)calloc(sizeof(*opcodes_info));
			opcodes_info->opcodes_info = (struct disassemble_info *)calloc(sizeof(struct disassemble_info), 1);
			info->disasm_data = opcodes_info;
		}
		INIT_DISASSEMBLE_INFO(opcodes_info->opcodes_info, info->disasm_data, (fprintf_ftype)opcodes_printf);
		opcodes_info->opcodes_info->arch = bfd_arch_m68k;
		opcodes_info->opcodes_info.buffer = (unsigned char *)phys_get_real_address(0);
		opcodes_info->opcodes_info.buffer_length = 2;
		opcodes_info->opcodes_info.buffer_vma = 0;
		opcodes_info->opcodes_info.print_address_func = print_32bit_address;
		switch (info->cpu)
		{
		case CPU_68000:
			opcodes_info->opcodes_info->mach = bfd_mach_m68000;
			break;
		case CPU_68008:
			opcodes_info->opcodes_info->mach = bfd_mach_m68008;
			break;
		case CPU_68010:
			opcodes_info->opcodes_info->mach = bfd_mach_m68010;
			break;
		case CPU_68020:
			opcodes_info->opcodes_info->mach = bfd_mach_m68020;
			break;
		case CPU_68030:
			opcodes_info->opcodes_info->mach = bfd_mach_m68030;
			break;
		case CPU_68040:
			opcodes_info->opcodes_info->mach = bfd_mach_m68040;
			break;
		case CPU_68060:
			opcodes_info->opcodes_info->mach = bfd_mach_m68060;
			break;
		case CPU_68302:
			opcodes_info->opcodes_info->mach = bfd_mach_m68000;
			break;
		case CPU_68331:
		case CPU_68332:
		case CPU_68333:
			opcodes_info->opcodes_info->mach = bfd_mach_m68020;
			break;
		case CPU_CPU32:
			opcodes_info->opcodes_info->mach = bfd_mach_cpu32;
			break;
		case CPU_5200:
		case CPU_5202:
		case CPU_5204:
		case CPU_5206:
			opcodes_info->opcodes_info->mach = bfd_mach_mcf_isa_c_nodiv;
			break;
		case CPU_5206e:
			opcodes_info->opcodes_info->mach = bfd_mach_mcf_isa_c_mac;
			break;
		case CPU_5207:
		case CPU_5208:
			opcodes_info->opcodes_info->mach = bfd_mach_mcf_isa_aplus_emac;
			break;
		case CPU_521x:
			opcodes_info->opcodes_info->mach = bfd_mach_mcf_isa_aplus_mac;
			break;
		case CPU_5249:
			opcodes_info->opcodes_info->mach = bfd_mach_mcf_isa_a_mac;
			break;
		case CPU_528x:
			opcodes_info->opcodes_info->mach = bfd_mach_mcf_isa_aplus_emac;
			break;
		case CPU_5307:
			opcodes_info->opcodes_info->mach = bfd_mach_mcf_isa_a_mac;
			break;
		case CPU_537x:
			opcodes_info->opcodes_info->mach = bfd_mach_mcf_isa_a_emac;
			break;
		case CPU_5407:
			opcodes_info->opcodes_info->mach = bfd_mach_mcf_isa_b_mac;
			break;
		case CPU_547x:
			opcodes_info->opcodes_info->mach = bfd_mach_mcf_isa_b_emac;
			break;
		case CPU_548x:
		case CPU_CFV4:
		case CPU_CFV4e:
			opcodes_info->opcodes_info->mach = bfd_mach_mcf_isa_b_float_emac;
			break;
		default:
			break;
		}
	}
#endif
}


void m68k_disasm_exit(m68k_disasm_info *info)
{
	if (info == NULL)
		return;
	if (info->disasm_data != NULL)
	{
		free(info->disasm_data);
		info->disasm_data = NULL;
	}
}


int m68k_disasm_insn(m68k_disasm_info *info)
{
	int len;
	int i;
	memptr start;
	
	info->num_oper = 0;
	info->opcode[0] = '\0';
	info->operands[0] = '\0';
	info->comments[0] = '\0';
	info->num_insn_words = 0;
	{
		SAVE_EXCEPTION;
		
		{
			TRY(prb)
			{
#ifdef DISASM_USE_OPCODES
				{
					char *p;
					struct opcodes_info *opcodes_info = (struct opcodes_info *)info->disasm_data;
					
					opcodes_info->linepos = 0;
					opcodes_info->bufsize = sizeof(opcodes_info->linebuf);
					opcodes_info->opcodes_info->buffer = phys_get_real_address(info->memory_vma);
					opcodes_info->opcodes_info->buffer_length = 22;
					opcodes_info->opcodes_info->buffer_vma = info->memory_vma;
					len = print_insn_m68k(info->memory_vma, opcodes_info->opcodes_info);
					opcodes_info->linebuf[opcodes_info->linepos] = '\0';
					p = strchr(opcodes_info->linebuf, ' ');
					if (p != NULL)
					{
						*p++ = '\0';
						strncpy(info->opcode, opcodes_info->linebuf, sizeof(info->opcode) - 1);
						/*
						 * how to get the number of operands from libopcodes?
						 * the following doesn't work, because registers
						 * are not data references
						 * FIXME
						 */
						if (opcodes_info->opcodes_info->insn_type == dis_dref2)
							info->num_oper = 2;
						else
							info->num_oper = 1;
						while (*p == ' ')
							p++;
						strncpy(info->operands, p, sizeof(info->operands) - 1);
						/*
						 * on 64-bit hosts, libopcodes prints all addresses with 16 digits,
						 * even if we are disassembling for a 32bit processor
						 */
						while ((p = strstr(info->operands, "000000000")) != NULL)
							memmove(p + 1, p + 9, strlen(p + 9) + 1);
						while ((p = strstr(info->operands, "fffffffff")) != NULL)
							memmove(p + 1, p + 9, strlen(p + 9) + 1);
					} else
					{
						strncpy(info->opcode, opcodes_info->linebuf, sizeof(info->opcode) - 1);
					}
				}
#endif
#ifdef DISASM_USE_BUILTIN
				len = m68k_disasm_builtin(info);
#endif
				info->opcode[sizeof(info->opcode) - 1] = '\0';
				info->operands[sizeof(info->operands) - 1] = '\0';
				if (len == 0) /* make sure we advance in case there was an error */
					len = 2;
				info->num_insn_words = len / 2;
				start = info->memory_vma;
				for (i = 0; i < info->num_insn_words; i++, start += 2)
					info->insn_words[i] = phys_get_word(start);
				info->memory_vma = start;
			} CATCH(prb)
			{
				strcpy(info->opcode, "<invalid address>");
				len = -1;
			}
		}
		RESTORE_EXCEPTION;
	}
	return len;
}



int m68k_disasm_to_buf(m68k_disasm_info *info, char *buf, int allbytes)
{
	int len;
	int i;
	memptr start = info->memory_vma;
	const int words_per_line = 5;
	
	len = m68k_disasm_insn(info);
	sprintf(buf, "[%08x]", start);
	for (i = 0; i < words_per_line && i < info->num_insn_words; i++)
	{
		sprintf(buf + strlen(buf), " %04x", info->insn_words[i]);
	}
	for (; i < words_per_line; i++)
		strcat(buf, "     ");
#if 0
	{
		char *p;
	
		p = strchr(info->operands, ',');
		sprintf(buf + strlen(buf), " %d", info->num_oper);
		if (info->num_oper == 0 && *info->operands != '\0')
			strcat(buf, "? ");
		else if (info->num_oper != 0 && *info->operands == '\0')
			strcat(buf, "? ");
		else if (p == NULL && info->num_oper == 2)
			strcat(buf, "? ");
		else if (p != NULL && info->num_oper == 1)
			strcat(buf, "! ");
		else
			strcat(buf, "  ");
	}
#endif
	sprintf(buf + strlen(buf), "  %-10s", info->opcode);
	strcat(buf, info->operands);
	if (*info->comments != '\0')
	{
		strcat(buf, " ; ");
		strcat(buf, info->comments);
	}
	if (info->num_insn_words > words_per_line && allbytes)
	{
		strcat(buf, "\n");
		sprintf(buf + strlen(buf), "[%08x]", start + words_per_line * 2);
		for (i = words_per_line; i < info->num_insn_words; i++)
		{
			sprintf(buf + strlen(buf), " %04x", info->insn_words[i]);
		}
	}
	return len;
}



/*
 * Utility functions that can be called from GDB.
 */
memptr gdb_dis(memptr start, unsigned int count)
{
	char buf[256];
	memptr save_vma;
	int size;
	
	if (RAMBaseHost == NULL)
	{
		fprintf(stderr, "memory not yet initialized\n");
		return 0;
	}
	if (count == 0)
		count = 1;
	save_vma = disasm_info.memory_vma;
	disasm_info.memory_vma = start;
	do
	{
		size = m68k_disasm_to_buf(&disasm_info, buf, TRUE);
		puts(buf);
		--count;
	} while (size >= 0 && count);
	start = disasm_info.memory_vma;
	disasm_info.memory_vma = save_vma;
	return start;
}


memptr gdb_pc(void)
{
	return m68k_getpc();
}


void gdb_regs(void)
{
	int i;
	memptr u, s;
	FILE *f = stdout;
	
	for (i = 0; i < 8; i = i + 4)
	{
		fprintf(f, "D%d: %04x %04x ", i + 0, (regs.regs[i + 0] >> 16) & 0xffff, regs.regs[i + 0] & 0xffff);
		fprintf(f, "D%d: %04x %04x ", i + 1, (regs.regs[i + 1] >> 16) & 0xffff, regs.regs[i + 1] & 0xffff);
		fprintf(f, "D%d: %04x %04x ", i + 2, (regs.regs[i + 2] >> 16) & 0xffff, regs.regs[i + 2] & 0xffff);
		fprintf(f, "D%d: %04x %04x ", i + 3, (regs.regs[i + 3] >> 16) & 0xffff, regs.regs[i + 3] & 0xffff);
		fprintf(f, "\n");
	}
	for (i = 0; i < 8; i = i + 4)
	{
		fprintf(f, "A%d: %04x %04x ", i + 0, (regs.regs[i +  8] >> 16) & 0xffff, regs.regs[i +  8] & 0xffff);
		fprintf(f, "A%d: %04x %04x ", i + 1, (regs.regs[i +  9] >> 16) & 0xffff, regs.regs[i +  9] & 0xffff);
		fprintf(f, "A%d: %04x %04x ", i + 2, (regs.regs[i + 10] >> 16) & 0xffff, regs.regs[i + 10] & 0xffff);
		fprintf(f, "A%d: %04x %04x ", i + 3, (regs.regs[i + 11] >> 16) & 0xffff, regs.regs[i + 11] & 0xffff);
		fprintf(f, "\n");
	}
	if (regs.sr & 0x2000)
	{
		u = regs.usp;
		s = regs.isp;
		fprintf(f, "Supervisor Mode: USP: %08x\n", u);
	} else
	{
		u = regs.usp;
		s = regs.isp;
		fprintf(f, "User Mode: SSP: %08x\n", s);
	}
	fprintf(f, "SR:%04x  ", regs.sr);
	if (regs.sr & 0x10)
		fprintf(f, "X");
	else
		fprintf(f, "-");
	if (regs.sr & 0x08)
		fprintf(f, "N");
	else
		fprintf(f, "-");
	if (regs.sr & 0x04)
		fprintf(f, "Z");
	else
		fprintf(f, "-");
	if (regs.sr & 0x02)
		fprintf(f, "V");
	else
		fprintf(f, "-");
	if (regs.sr & 0x01)
		fprintf(f, "C");
	else
		fprintf(f, "-");
	fprintf(f, "\n");
}

#endif /* HAVE_DISASM_M68K */
