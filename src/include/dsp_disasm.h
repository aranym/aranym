/*
 *	DSP56K emulation
 *	disassembler
 *
 *	Patrice Mandin
 */

#ifndef _DSP_DISASM_H_
#define _DSP_DISASM_H_

/* Functions */
void dsp56k_disasm(void);

/* Registers change */
void dsp56k_disasm_reg_read(void);
void dsp56k_disasm_reg_compare(void);

#endif
