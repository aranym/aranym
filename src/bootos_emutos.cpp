/*
	ROM / OS loader, EmuTOS

	ARAnyM (C) 2005-2006 Patrice Mandin

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

#include "sysdeps.h"
#include "cpu_emulation.h"
#include "bootos_emutos.h"
#include "aranym_exception.h"
#include "emul_op.h"

#define DEBUG 0
#include "debug.h"

/*	EmuTOS ROM class */

EmutosBootOs::EmutosBootOs(void) ARANYM_THROWS(AranymException)
{
	emutos_patch(true);
}

/*--- Private functions ---*/

void EmutosBootOs::emutos_patch(bool cold) ARANYM_THROWS(AranymException)
{
	if (strlen(bx_options.tos.emutos_path) == 0)
		throw AranymException("Path to EmuTOS ROM image file undefined");

	load(bx_options.tos.emutos_path);

	init(cold);

	infoprint("EmuTOS %02x%02x/%02x/%02x loading from '%s'... [OK]",
		ROMBaseHost[0x1a], ROMBaseHost[0x1b],
		ROMBaseHost[0x18], ROMBaseHost[0x19],
		bx_options.tos.emutos_path
	);
	
	int found = 0;
	for (int ptr = 0; ptr < 0x10000; ptr += 2)
	{
		if (ROMBaseHost[ptr +  0] == 0x3f &&    /* move.w #-1,-(a7) */
			ROMBaseHost[ptr +  1] == 0x3c &&
			ROMBaseHost[ptr +  2] == 0xff &&
			ROMBaseHost[ptr +  3] == 0xff &&
			ROMBaseHost[ptr +  4] == 0x4e &&    /* jsr _kbshift */
			ROMBaseHost[ptr +  5] == 0xb9 &&
			ROMBaseHost[ptr +  6] == 0x00 &&
			ROMBaseHost[ptr +  7] == 0xe0 &&
			ROMBaseHost[ptr + 10] == 0x54 &&    /* addq.l #2,a7 */
			ROMBaseHost[ptr + 11] == 0x8f &&
			((ROMBaseHost[ptr + 12] == 0x08 &&  /* btst #3,d0 */
			  ROMBaseHost[ptr + 13] == 0x00 &&
			  ROMBaseHost[ptr + 14] == 0x00 &&
			  ROMBaseHost[ptr + 15] == 0x03) ||
			 (ROMBaseHost[ptr + 12] == 0x44 &&
			  ROMBaseHost[ptr + 13] == 0xc0 &&
			  ROMBaseHost[ptr + 14] == 0x6b &&
			  ROMBaseHost[ptr + 15] == 0x14)))
		{
			D(bug("blkdev_hdv_boot 1 found at %08x", ptr + ROMBase));
			ROMBaseHost[ptr +  0] = 0x48; // movem.l d1-d2/a0-a2,-(a7)
			ROMBaseHost[ptr +  1] = 0xe7;
			ROMBaseHost[ptr +  2] = 0x60;
			ROMBaseHost[ptr +  3] = 0xe0;
			ROMBaseHost[ptr +  4] = 0xa0; // Linea_init
			ROMBaseHost[ptr +  5] = 0x00;
			ROMBaseHost[ptr +  6] = M68K_EMUL_INIT >> 8;
			ROMBaseHost[ptr +  7] = M68K_EMUL_INIT & 0xff;
			ROMBaseHost[ptr +  8] = 0x4c; // movem.l (a7)+,d1-d2/a0-a2
			ROMBaseHost[ptr +  9] = 0xdf;
			ROMBaseHost[ptr + 10] = 0x07;
			ROMBaseHost[ptr + 11] = 0x06;
			ROMBaseHost[ptr + 12] = 0x70; // moveq #0,d0
			ROMBaseHost[ptr + 13] = 0x00;
			ROMBaseHost[ptr + 14] = 0x4e; // nop
			ROMBaseHost[ptr + 15] = 0x71;
			found++;
		} else if (
		    ROMBaseHost[ptr +  0] == 0x08 && /* btst #1,d0 */
			ROMBaseHost[ptr +  1] == 0x00 &&
			ROMBaseHost[ptr +  2] == 0x00 &&
			ROMBaseHost[ptr +  3] == 0x01 &&
			ROMBaseHost[ptr +  4] == 0x67 && /* beq *+6 */
			ROMBaseHost[ptr +  5] == 0x06 &&
			ROMBaseHost[ptr +  6] == 0x42 && /* clr.w _bootdev */
			ROMBaseHost[ptr +  7] == 0x79 &&
			ROMBaseHost[ptr +  8] == 0x00 &&
			ROMBaseHost[ptr +  9] == 0x00 &&
			ROMBaseHost[ptr + 10] == 0x04 &&
			ROMBaseHost[ptr + 11] == 0x46)
		{
			D(bug("blkdev_hdv_boot 2 found at %08x", ptr + ROMBase));
			ROMBaseHost[ptr +  0] = 0x48; // movem.l d1-d2/a0-a2,-(a7)
			ROMBaseHost[ptr +  1] = 0xe7;
			ROMBaseHost[ptr +  2] = 0x60;
			ROMBaseHost[ptr +  3] = 0xe0;
			ROMBaseHost[ptr +  4] = 0xa0; // Linea_init
			ROMBaseHost[ptr +  5] = 0x00;
			ROMBaseHost[ptr +  6] = M68K_EMUL_INIT >> 8;
			ROMBaseHost[ptr +  7] = M68K_EMUL_INIT & 0xff;
			ROMBaseHost[ptr +  8] = 0x4c; // movem.l (a7)+,d1-d2/a0-a2
			ROMBaseHost[ptr +  9] = 0xdf;
			ROMBaseHost[ptr + 10] = 0x07;
			ROMBaseHost[ptr + 11] = 0x06;
			ROMBaseHost[ptr + 12] = 0x70; // moveq #0,d0
			ROMBaseHost[ptr + 13] = 0x00;
			ROMBaseHost[ptr + 14] = 0x4e; // nop
			ROMBaseHost[ptr + 15] = 0x71;
			found++;
		}
	}
	if (found == 0)
	{
		bug("EmutosBootOs: blkdev_hdv_boot not found!");
	} else if (found > 1)
	{
		bug("EmutosBootOs: blkdev_hdv_boot found %d times!", found);
	}
}
/* vim:ts=4:sw=4
 */
