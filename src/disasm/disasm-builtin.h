/*
 * disasm-builtin.h - declaration of internal disassemblers
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
 
#ifdef __cplusplus
extern "C" {
#endif

/* Method to initialize a disassemble_info struct.  This should be
   called by all applications creating such a struct.  */
void arm_disassemble_init(disassemble_info **info, void *stream, fprintf_ftype fprintf_func);
void x86_disassemble_init(disassemble_info **info, void *stream, fprintf_ftype fprintf_func);

/* Disassemble one instruction at the given target address. 
   Return number of octets processed.  */
int arm_print_insn(bfd_vma, disassemble_info *);
int x86_print_insn(bfd_vma, disassemble_info *);

void x86_disassemble_set_option(disassemble_info *info, const char *options);

#ifdef __cplusplus
}
#endif
