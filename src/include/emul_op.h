/*
 *  emul_op.h - 68k opcodes for ROM patches
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

#ifndef EMUL_OP_H
#define EMUL_OP_H

// 68k opcodes
/*
const uint16 M68K_ILLEGAL = 0x4afc;
const uint16 M68K_NOP = 0x4e71;
const uint16 M68K_RTS = 0x4e75;
const uint16 M68K_RTD = 0x4e74;
const uint16 M68K_RTR = 0x4e77;
const uint16 M68K_JMP = 0x4ef9;
const uint16 M68K_JMP_A0 = 0x4ed0;
const uint16 M68K_JSR = 0x4eb9;
const uint16 M68K_JSR_A0 = 0x4e90;
*/

// Extended opcodes
enum {
/*
	M68K_EXEC_RETURN = 0x7100,		// Extended opcodes (illegal moveq form)
*/
	M68K_EMUL_BREAK = 0x7101,		// 0x7101 - breakpoint
	M68K_EMUL_RESET = 0x7102,
 	M68K_EMUL_EXIT = 0x7103,
	M68K_EMUL_INIT = 0x7104,
	M68K_EMUL_OP_PUT_SCRAP = 0x712a,	// 0x712a - used in TOS ROM patch
	M68K_EMUL_OP_CPUDEBUG_ON = 0x7139,	// 0x7139 - cpu_debugging on
	M68K_EMUL_OP_CPUDEBUG_OFF = 0x713a,	// 0x713a - cpu_debugging off
	M68K_EMUL_OP_JIT = 0x713b,			/* 0x713b - JIT state */
	M68K_EMUL_OP_MAX,			// highest number
	M68K_EMUL_OP_MON0 = 0x71f0,		// Monitor instructions
	M68K_EMUL_OP_MON1 = 0x71f1,
	M68K_EMUL_OP_MON2 = 0x71f2,
	M68K_EMUL_OP_MON3 = 0x71f3,
	M68K_EMUL_OP_MON4 = 0x71f4,
	M68K_EMUL_OP_MON5 = 0x71f5,
	M68K_EMUL_OP_MON6 = 0x71f6,
	M68K_EMUL_OP_MON7 = 0x71f7,
	M68K_EMUL_OP_MON8 = 0x71f8,
	M68K_EMUL_OP_MON9 = 0x71f9,
	M68K_EMUL_OP_MONa = 0x71fa,
	M68K_EMUL_OP_MONb = 0x71fb,
	M68K_EMUL_OP_MONc = 0x71fc,
	M68K_EMUL_OP_MONd = 0x71fd,
	M68K_EMUL_OP_MONe = 0x71fe,
	M68K_EMUL_OP_DEBUGGER = 0x71ff		// Run debuger
};

// Functions
extern bool EmulOp(uint16 opcode, struct M68kRegisters *r);	// Execute EMUL_OP opcode (called by 68k emulator or Line-F trap handler)

#endif
