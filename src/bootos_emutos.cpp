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

/*
 * new extended os header for EmuTOS,
 * available since 2019/10/31
 */
typedef struct
{
	uint32_t magic;           /* magic value 'OSEX' */
	uint32_t size;            /* size of this structure */
	uint32_t version;         /* version number, e.g. 0x00090c00 for 0.9.12 */
	uint32_t version_offset;  /* offset to version string */
    uint32_t feature_flags;   /* feature flags */
#define OSEX_LINEA 0x00000001
} OSEX_HEADER;

/*	EmuTOS ROM class */

EmutosBootOs::EmutosBootOs(void) ARANYM_THROWS(AranymException)
{
	emutos_patch(true);
}

/*--- Private functions ---*/

void EmutosBootOs::emutos_patch(bool cold) ARANYM_THROWS(AranymException)
{
	int found;
	int osex_header;
	uint32_t feature_flags = 0;

	if (strlen(bx_options.tos.emutos_path) == 0)
		throw AranymException("Path to EmuTOS ROM image file undefined");

	load(bx_options.tos.emutos_path);

	init(cold);

	infoprint("EmuTOS %02x%02x/%02x/%02x loading from '%s'... [OK]",
		ROMBaseHost[0x1a], ROMBaseHost[0x1b],
		ROMBaseHost[0x18], ROMBaseHost[0x19],
		bx_options.tos.emutos_path
	);
	if (ROMBaseHost[0x2c] != 'E' ||
		ROMBaseHost[0x2d] != 'T' ||
		ROMBaseHost[0x2e] != 'O' ||
		ROMBaseHost[0x2f] != 'S')
	{
		bug("warning: %s does not seem to be EmuTOS image", bx_options.tos.emutos_path);
	}

	osex_header = -1;
	for (int ptr = 0; ptr < 0x4000; ptr += 2)
	{
		if (ROMBaseHost[ptr +  0] == 'O' &&
			ROMBaseHost[ptr +  1] == 'S' &&
			ROMBaseHost[ptr +  2] == 'E' &&
			ROMBaseHost[ptr +  3] == 'X')
		{
			osex_header = ptr;
			break;
		}
	}

	if (osex_header >= 0)
	{
		uint32_t headersize = do_get_mem_long((uae_u32 *)(ROMBaseHost + osex_header + 4));
		D(bug("OSEX header (%u) found at 0x%08x", headersize, osex_header));
		if (headersize >= 20)
		{
			uint32_t version_offset = do_get_mem_long((uae_u32 *)(ROMBaseHost + osex_header + 12));
			feature_flags = do_get_mem_long((uae_u32 *)(ROMBaseHost + osex_header + 16));
			infoprint("EmuTOS version %u.%u.%u.%u %s",
				ROMBaseHost[osex_header + 8],
				ROMBaseHost[osex_header + 9],
				ROMBaseHost[osex_header + 10],
				ROMBaseHost[osex_header + 11],
				version_offset ? (const char *)ROMBaseHost + version_offset : "");
		}
	}

	if (feature_flags & OSEX_LINEA)
	{
		D(bug("OSEX_LINEA found"));
	} else
	{
		D(bug("No OSEX_LINEA found"));

		found = 0;
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
			D(bug("EmutosBootOs: blkdev_hdv_boot not found!"));
		} else if (found > 1)
		{
			D(bug("EmutosBootOs: blkdev_hdv_boot found %d times!", found));
		}
	}
}
