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

		case M68K_EMUL_OP_SHUTDOWN:			// Quit emulator
			QuitEmulator();
			break;

		case M68K_EMUL_OP_RESET: {			// MacOS reset
			D(bug("*** RESET ***\n"));
			if (FPUType)
				r->d[2] |= 0x10000000;									// Set FPU flag if FPU present
			else
				r->d[2] &= 0xefffffff;									// Clear FPU flag if no FPU present
			break;
		}
		case M68K_EMUL_OP_VIDEO_OPEN:		// Video driver functions
// MJ			r->d[0] = VideoDriverOpen(r->a[0], r->a[1]);
			{
				static bool Esc = false;
				static bool inverse = false;
				static int params = 0;

				uae_u8 value = r->d[1];
				fprintf(stderr, "XConOut printing '%c' (%d/$%x)\n", value, value, value);
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

		case M68K_EMUL_OP_VIDEO_CONTROL:
			fVDIDrv.dispatch( ReadInt32(r->a[7]), r );  // SO
#ifdef __CYGWIN__
			invoke200HzInterrupt();	/* Windows has a broken threading - we have to call it manually */
#endif /* __CYGWIN__ */
			break;

#ifdef EXTFS_SUPPORT
		case M68K_EMUL_OP_EXTFS_COMM:		// External file system routines
			extFS.dispatch( ReadInt32(r->a[7]), r );  // SO
#ifdef __CYGWIN__
			invoke200HzInterrupt();	/* Windows has a broken threading - we have to call it manually */
#endif /* __CYGWIN__ */
			break;

		case M68K_EMUL_OP_EXTFS_HFS:
			extFS.dispatchXFS( ReadInt32(r->a[7]), r );  // SO
#ifdef __CYGWIN__
			invoke200HzInterrupt();	/* Windows has a broken threading - we have to call it manually */
#endif /* __CYGWIN__ */
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
			}
			else {
				D(bug("Wrong debugText addr: %u", textAddr));
			}
		}
			break;

		case M68K_EMUL_OP_DMAREAD:	// for EmuTOS - code 0x7136
		{
			int dev = ReadInt32(r->a[7]+14);
			long buf = ReadInt32(r->a[7]+10);
			int cnt = ReadInt32(r->a[7]+8);
			long recnr = ReadInt32(r->a[7]+4);
			D(bug("ARAnyM DMAread(start=%ld, count=%d, buffer=%ld, device=%d)", recnr, cnt, buf, dev));

			bx_disk_options_t *disk;
			switch(dev) {
				case 16:	disk = &bx_options.diskc; break;
				case 17:	disk = &bx_options.diskd; break;
				default:	disk = NULL; break;
			}

			if (disk == NULL) {
				r->d[0] = (uint32)-15L;	// EUNDEV (unknown device)
				break;
			}

			FILE *f = fopen(disk->path, "rb");
			if (f != NULL) {
				const int secsize = 512;
				const int size = secsize*cnt;
				uint8 *hostbuf = Atari2HostAddr(buf);
				fseek(f, recnr*secsize, SEEK_SET);
				fread(hostbuf, size, 1, f);
				fclose(f);
				if (! disk->byteswap) {
					for(int i=0; i<size; i++) {
						int tmp = hostbuf[i];
						hostbuf[i] = hostbuf[i+1];
						hostbuf[++i] = tmp;
					}
				}
			}
			r->d[0] = 0;	// 0 = no error
		}
			break;

		case M68K_EMUL_OP_XHDI:	// for EmuTOS - code 0x7137
			// D(bug("ARAnyM XHDI(%u)\n", get_word(r->a[7]+4, true)));
			switch(ReadInt32(r->a[7]+4)) {
				case 14:	/* XHGetCapatity */
					{
						// UWORD major, UWORD minor, ULONG *blocks, ULONG *blocksize)
						uint16 major = ReadInt32(r->a[7]+6);
						uint16 minor = ReadInt32(r->a[7]+8);
						uint32 blocks = ReadInt32(r->a[7]+10);
						uint32 blocksize = ReadInt32(r->a[7]+14);
						D(bug("ARAnyM XHGetCapacity(major=%u, minor=%u, blocks=%lu, blocksize=%lu)", major, minor, blocks, blocksize));

						if (minor != 0) {
							r->d[0] = (uint32)-15L;	// EUNDEV (unknown device)
							break;
						}

						bx_disk_options_t *disk;
						switch(major) {
							case 16:	disk = &bx_options.diskc; break;
							case 17:	disk = &bx_options.diskd; break;
							default:	disk = NULL; break;
						}

						if (disk == NULL)
							r->d[0] = (uint32)-15L;	// EUNDEV (unknown device)

						struct stat buf;
						if (! stat(disk->path, &buf)) {
							long t_blocks = buf.st_size >> 9;	// 512 bytes block FIXME
							D(bug("t_blocks = %ld\n", t_blocks));
							if (blocks > 0)
								WriteAtariInt32(blocks, t_blocks);
							if (blocksize > 0)
								WriteAtariInt32(blocksize, 512);	// FIXME
							r->d[0] = 0L;
						}
						else {
							r->d[0] = (uint32)-15L;	// actually file error
						}
					}
					break;

				default:
					r->d[0] = (uint32)-32L; // EINVFN
					break;
			}
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
