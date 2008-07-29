/*
	DSP M56001 emulation
	Disassembler

	(C) 2003-2008 ARAnyM developer team

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
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef DSP_DISASM_H
#define DSP_DISASM_H

#ifdef __cplusplus
extern "C" {
#endif

/* Functions */
void dsp56k_disasm_init(dsp_core_t *my_dsp_core);
void dsp56k_disasm(void);

/* Registers change */
void dsp56k_disasm_reg_read(void);
void dsp56k_disasm_reg_compare(void);

/* Function to mark register as changed */
void dsp56k_disasm_force_reg_changed(int num_dsp_reg);

#ifdef __cplusplus
}
#endif

#endif /* DSP_DISASM_H */
