/*
 * disasm-arm.cpp - arm disassembler (using opcodes library)
 *
 * Copyright (c) 2017 ARAnyM developer team
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
 *
 * 2017-09-10 : Initial version - Thorsten Otto
 *
 */

#include "sysdeps.h"
#include "cpu_emulation.h"
#include "disasm-glue.h"

#ifdef HAVE_DISASM_ARM /* rest of file */

#ifdef DISASM_USE_OPCODES

#include <dis-asm.h>

#else

#include "disasm/disasm-arm.h"

#endif /* DISASM_USE_OPCODES */

struct opcodes_info {
	char linebuf[128];
	size_t bufsize;
	size_t linepos;
	disassemble_info opcodes_info;
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


const uint8 *arm_disasm(const uint8 *ainstr, char *buf)
{
	struct opcodes_info info;
	int len;
	int i;
	char *opcode;
	char *p, *p2;
	
	info.linepos = 0;
	info.bufsize = sizeof(info.linebuf);
#ifdef DISASM_USE_OPCODES
	INIT_DISASSEMBLE_INFO(info.opcodes_info, &info, opcodes_printf);
	info.opcodes_info.buffer = (bfd_byte *)ainstr;
	info.opcodes_info.buffer_length = 16;
	info.opcodes_info.buffer_vma = (uintptr)ainstr;
#ifdef CPU_arm
	info.opcodes_info.arch = bfd_arch_arm;
	info.opcodes_info.mach = bfd_mach_arm_unknown;
	disassemble_init_for_target(&info.opcodes_info);
	len = print_insn_little_arm(info.opcodes_info.buffer_vma, &info.opcodes_info);
#else
	info.opcodes_info.arch = bfd_arch_aarch64;
	info.opcodes_info.mach = bfd_mach_aarch64;
	disassemble_init_for_target(&info.opcodes_info);
	len = print_insn_aarch64(info.opcodes_info.buffer_vma, &info.opcodes_info);
#endif
#endif

#ifdef DISASM_USE_BUILTIN
	arm_disassemble_init(&info.opcodes_info, &info, opcodes_printf);
#ifdef CPU_arm
	len = arm_print_insn(ainstr, &info.opcodes_info);
#else
	len = aarch64_print_insn(ainstr, &info.opcodes_info);
#endif
#endif

	info.linebuf[info.linepos] = '\0';

#ifdef CPU_arm
	sprintf(buf, "[%08x]", (uintptr)ainstr);
#else
	sprintf(buf, "[%016lx]", (uintptr)ainstr);
#endif
	for (i = 0; i < 12 && (i + 4) <= len; i += 4)
	{
		sprintf(buf + strlen(buf), " %08x",
			((uint32_t)ainstr[i + 0]) |
			((uint32_t)ainstr[i + 1] << 8) |
			((uint32_t)ainstr[i + 2] << 16) |
			((uint32_t)ainstr[i + 3] << 24));
	}
	for (; i < 12 && i < len; i++)
	{
		sprintf(buf + strlen(buf), " %02x", ainstr[i]);
	}
	opcode = info.linebuf;
	p = strchr(opcode, ' ');
	p2 = strchr(opcode, '\t');
	if (p == NULL || p2 < p)
		p = p2;
	if (p)
	{
		*p++ = '\0';
		while (*p == ' ')
			p++;
	}
	sprintf(buf + strlen(buf), "  %-10s", opcode);
	if (p)
		strcat(buf, p);
	if (len <= 0) /* make sure we advance in case there was an error */
		len = 1;
	ainstr += len;
	return ainstr;
}

#else

extern int i_dont_care_that_ISOC_doesnt_like_empty_sourcefiles;

#endif /* HAVE_DISASM_ARM */
