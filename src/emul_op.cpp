/*
 *  emul_op.cpp - 68k opcodes for ROM patches
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

#include "sysdeps.h"
#include "cpu_emulation.h"
#include "main.h"
#include "emul_op.h"
#include "araobjs.h"
#include "parameters.h"

#ifdef ENABLE_MON
#include "mon.h"
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
#ifdef ENABLE_MON
			char *arg[4] = {"mon", "-m", "-r", NULL};
			mon(3, arg);
#endif
			QuitEmulator();
			break;
		}

		case M68K_EMUL_OP_VIDEO_OPEN:		// Video driver functions
			panicbug("Obsolete VIDEO driver! Use NatFeat driver instead!\n");
			QuitEmulator();

		case M68K_EMUL_OP_VIDEO_CONTROL:	// DEPRECATED
			panicbug("Obsolete VIDEO driver! Use NatFeat driver instead!\n");
			QuitEmulator();
			break;

		case M68K_EMUL_OP_VIDEO_DRIVER:
			panicbug("Obsolete VIDEO driver! Use NatFeat driver instead!\n");
			QuitEmulator();
			break;

#ifdef EXTFS_SUPPORT
		case M68K_EMUL_OP_EXTFS_COMM:		// External file system routines
			extFS.dispatch( ReadInt32(r->a[7]), r );  // SO
			break;

		case M68K_EMUL_OP_EXTFS_HFS:
			extFS.dispatchXFS( ReadInt32(r->a[7]), r );  // SO
			break;
#endif

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

		case M68K_EMUL_OP_DEBUGUTIL:	// for EmuTOS - code 0x7135
		{
			uint32 textAddr = ReadAtariInt32(r->a[7]+4);
			if (textAddr >=0 && textAddr < (RAMSize + ROMSize + FastRAMSize)) {
				uint8 *textPtr = Atari2HostAddr(textAddr);
				printf("%s", textPtr);
				fflush(stdout);
			}
			else {
				D(bug("Wrong debugText addr: %u", textAddr));
			}
		}
			break;

		case M68K_EMUL_OP_DMAREAD:	// DEPRECATED (for EmuTOS - code 0x7136)
			panicbug("Obsolete DMA driver! Use NatFeat driver instead!\n");
			QuitEmulator();
			break;

		case M68K_EMUL_OP_XHDI:	// for EmuTOS - code 0x7137
			panicbug("Obsolete XHDI driver! Use NatFeat driver instead!\n");
			QuitEmulator();
			break;

		case M68K_EMUL_OP_AUDIO:
			panicbug("Obsolete AUDIO driver! Use NatFeat driver instead!\n");
			QuitEmulator();
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
		case M68K_EMUL_OP_MONa:
		case M68K_EMUL_OP_MONb:
		case M68K_EMUL_OP_MONc:
		case M68K_EMUL_OP_MONd:
		case M68K_EMUL_OP_MONe:
			fprintf(stderr, "Monitor %08x\n", opcode);
			fprintf(stderr, "d0 %08lx d1 %08lx d2 %08lx d3 %08lx\n"
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
#ifdef ENABLE_MON
			char *arg[4] = {"mon", "-m", "-r", NULL};
			mon(3, arg);
#endif
			QuitEmulator();
			break;
	}
}
