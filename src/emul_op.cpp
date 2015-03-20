/*
 *  emul_op.cpp - 68k opcodes for ARAnyM specific hacking
 *
 *  Copyright (c) 2001-2006 Petr Stehlik of ARAnyM dev team (see AUTHORS)
 *
 *  based on Basilisk II (C) 1997-2000 Christian Bauer
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

#include "sysdeps.h"
#include "cpu_emulation.h"
#include "main.h"
#include "emul_op.h"
#include "parameters.h"
#include "bootos.h"

#ifdef JIT_DEBUG
# include "compiler/compemu.h"
#endif

#define DEBUG 0
#include "debug.h"


/*
 *  Execute EMUL_OP opcode (called by 68k emulator or Illegal Instruction trap handler)
 */

void EmulOp(uint16 opcode, M68kRegisters *r)
{
	// D(bug("EmulOp %04x\n", opcode));
	switch (opcode) {
		case M68K_EMUL_BREAK: {				// Breakpoint
			printf("*** Breakpoint\n");
			printf("d0 %08lx d1 %08lx d2 %08lx d3 %08lx\n"
			       "d4 %08lx d5 %08lx d6 %08lx d7 %08lx\n"
			       "a0 %08lx a1 %08lx a2 %08lx a3 %08lx\n"
			       "a4 %08lx a5 %08lx a6 %08lx a7 %08lx\n"
			       "sr %04x\n",
			       (unsigned long)r->d[0],
			       (unsigned long)r->d[1],
			       (unsigned long)r->d[2],
			       (unsigned long)r->d[3],
			       (unsigned long)r->d[4],
			       (unsigned long)r->d[5],
			       (unsigned long)r->d[6],
			       (unsigned long)r->d[7],
			       (unsigned long)r->a[0],
			       (unsigned long)r->a[1],
			       (unsigned long)r->a[2],
			       (unsigned long)r->a[3],
			       (unsigned long)r->a[4],
			       (unsigned long)r->a[5],
			       (unsigned long)r->a[6],
			       (unsigned long)r->a[7],
			       r->sr);
			QuitEmulator();
			break;
		}

		case M68K_EMUL_RESET:	// used in Linux/m68k reboot routine
			bootOs->reset(false);	// reload linux kernel
			Reset680x0();		// reset CPU so it fetches correct SP && PC
			break;

		// VT52 Xconout
		case M68K_EMUL_OP_PUT_SCRAP:
			{
				static bool Esc = false;
				static bool inverse = false;
				static int params = 0;
				
				uae_u8 value = r->d[1];
				D(bug("XConout printing '%c' (%d/$%x)", value, value, value));
				if (Esc) {
					if (value == 'p')
						inverse = true;
					if (value == 'q')
						inverse = false;
					else if (value == 'K')
						; /* delete to end of line (I guess) */
					else if (value == 'Y')
						params = 2;
					Esc = false;
				}
				else {
					if (params > 0)
						params--;
					else {
						if (value == 27)
							Esc = true;
						else {
							fprintf(stdout, "%c", (value == 32 && inverse) ? '_' : value);
							fflush(stdout);
						}
					}
				}
			}
			break;

		case M68K_EMUL_OP_CPUDEBUG_ON:
			cpu_debugging = true;
			break;

		case M68K_EMUL_OP_CPUDEBUG_OFF:
			cpu_debugging = false;
			break;

		case M68K_EMUL_OP_MON0:
		case M68K_EMUL_OP_MON1:
		case M68K_EMUL_OP_MON2:
		case M68K_EMUL_OP_MON3:
		case M68K_EMUL_OP_MON4:
		case M68K_EMUL_OP_MON5:
		case M68K_EMUL_OP_MON6:
		case M68K_EMUL_OP_MON7:
		case M68K_EMUL_OP_MON8:
		case M68K_EMUL_OP_MON9:
			fprintf(stderr, "Monitor %08x\n", opcode);
			fprintf(stderr, "d0 %08lx d1 %08lx d2 %08lx d3 %08lx\n"
					"d4 %08lx d5 %08lx d6 %08lx d7 %08lx\n"
					"a0 %08lx a1 %08lx a2 %08lx a3 %08lx\n"
					"a4 %08lx a5 %08lx a6 %08lx a7 %08lx\n"
					"sr %04x\n"
					"pc %08lx instr %08lx\n",
					(unsigned long)r->d[0],
					(unsigned long)r->d[1],
					(unsigned long)r->d[2],
					(unsigned long)r->d[3],
					(unsigned long)r->d[4],
					(unsigned long)r->d[5],
					(unsigned long)r->d[6],
					(unsigned long)r->d[7],
					(unsigned long)r->a[0],
					(unsigned long)r->a[1],
					(unsigned long)r->a[2],
					(unsigned long)r->a[3],
					(unsigned long)r->a[4],
					(unsigned long)r->a[5],
					(unsigned long)r->a[6],
					(unsigned long)r->a[7],
					r->sr,
					(unsigned long)m68k_getpc(),
					(unsigned long)ReadInt32(m68k_getpc()));
			break;
		case M68K_EMUL_OP_MONa:
		case M68K_EMUL_OP_MONb:
		case M68K_EMUL_OP_MONc:
		case M68K_EMUL_OP_MONd:
		case M68K_EMUL_OP_MONe:
			fprintf(stderr, "Monitor %08x\n", opcode);
#ifdef JIT_DEBUG
			compiler_dumpstate();
#else
			fprintf(stderr, "d0 %08lx d1 %08lx d2 %08lx d3 %08lx\n"
					"d4 %08lx d5 %08lx d6 %08lx d7 %08lx\n"
					"a0 %08lx a1 %08lx a2 %08lx a3 %08lx\n"
					"a4 %08lx a5 %08lx a6 %08lx a7 %08lx\n"
					"sr %04x\n"
					"pc %08lx instr %08lx\n",
					(unsigned long)r->d[0],
					(unsigned long)r->d[1],
					(unsigned long)r->d[2],
					(unsigned long)r->d[3],
					(unsigned long)r->d[4],
					(unsigned long)r->d[5],
					(unsigned long)r->d[6],
					(unsigned long)r->d[7],
					(unsigned long)r->a[0],
					(unsigned long)r->a[1],
					(unsigned long)r->a[2],
					(unsigned long)r->a[3],
					(unsigned long)r->a[4],
					(unsigned long)r->a[5],
					(unsigned long)r->a[6],
					(unsigned long)r->a[7],
					r->sr,
					(unsigned long)m68k_getpc(),
					(unsigned long)ReadInt32(m68k_getpc()));
#endif
			break;
		case M68K_EMUL_OP_DEBUGGER:
#ifdef DEBUGGER
			activate_debugger();
#endif
			break;
		default:
			printf("FATAL: EMUL_OP called with bogus opcode %08x\n", opcode);
			printf("d0 %08lx d1 %08lx d2 %08lx d3 %08lx\n"
			       "d4 %08lx d5 %08lx d6 %08lx d7 %08lx\n"
			       "a0 %08lx a1 %08lx a2 %08lx a3 %08lx\n"
			       "a4 %08lx a5 %08lx a6 %08lx a7 %08lx\n"
			       "sr %04x\n",
			       (unsigned long)r->d[0],
			       (unsigned long)r->d[1],
			       (unsigned long)r->d[2],
			       (unsigned long)r->d[3],
			       (unsigned long)r->d[4],
			       (unsigned long)r->d[5],
			       (unsigned long)r->d[6],
			       (unsigned long)r->d[7],
			       (unsigned long)r->a[0],
			       (unsigned long)r->a[1],
			       (unsigned long)r->a[2],
			       (unsigned long)r->a[3],
			       (unsigned long)r->a[4],
			       (unsigned long)r->a[5],
			       (unsigned long)r->a[6],
			       (unsigned long)r->a[7],
			       r->sr);
			QuitEmulator();
			break;
	}
}
/* vim:ts=4:sw=4
 */
