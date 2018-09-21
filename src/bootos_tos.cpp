/*
	ROM / OS loader, TOS

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
#include "aramd5.h"
#include "bootos_tos.h"
#include "romdiff.h"
#include "aranym_exception.h"
#include "emul_op.h"

#define DEBUG 0
#include "debug.h"

/*	TOS ROM class */

TosBootOs::TosBootOs(void) ARANYM_THROWS(AranymException)
{
	tos_patch(true);
}

void TosBootOs::reset(bool cold) ARANYM_THROWS(AranymException)
{
	tos_patch(cold);
}

/*--- Private functions ---*/

void TosBootOs::tos_patch(bool cold) ARANYM_THROWS(AranymException)
{
	if (strlen(bx_options.tos.tos_path) == 0) {
		throw AranymException("Path to TOS ROM image file undefined");
	}

	load(bx_options.tos.tos_path);

	init(cold);

	// check if this is the correct 68040 aware TOS ROM version
	D(bug("Checking TOS version.."));
	static unsigned char const TOS404[16] = {0xe5,0xea,0x0f,0x21,0x6f,0xb4,0x46,0xf1,0xc4,0xa4,0xf4,0x76,0xbc,0x5f,0x03,0xd4};
	MD5 md5;
	unsigned char loadedTOS[16];
	md5.computeSum(ROMBaseHost, RealROMSize, loadedTOS);
	if (memcmp(loadedTOS, TOS404, 16) != 0) {
		throw AranymException("Wrong TOS version. You need the original TOS 4.04.");
	}

	// patch it for 68040 compatibility
	D(bug("Patching TOS 4.04 for 68040 compatibility.."));
	unsigned int ptr, i;
	for (i = 0; (ptr=tosdiff[i].start) > 0; i++)
		memcpy(&ROMBaseHost[ptr], tosdiff[i].patch, tosdiff[i].len);

	// patch cookies
	// _MCH
	ROMBaseHost[0x00416] = bx_options.tos.cookie_mch >> 24;
	ROMBaseHost[0x00417] = (bx_options.tos.cookie_mch >> 16) & 0xff;
	ROMBaseHost[0x00418] = (bx_options.tos.cookie_mch >> 8) & 0xff;
	ROMBaseHost[0x00419] = (bx_options.tos.cookie_mch) & 0xff;
	// _SND
#if DSP_EMULATION
	ROMBaseHost[0x00437] = 0x0E;	/* DSP, XBIOS, DMA8 */
#else
	ROMBaseHost[0x00437] = 0x06;	/* XBIOS, DMA8 */
#endif

	if (FastRAMSize > 0) {
		int i;

		// patch to show FastRAM memory test
		i = 0x001CC;
		ROMBaseHost[i++] = 0x4E;
		ROMBaseHost[i++] = 0xF9;	// JMP <abs.addr>
		ROMBaseHost[i++] = 0x00;	// Can't use JSR, stack pointer
		ROMBaseHost[i++] = 0xE7;	// is not initialized yet.
		ROMBaseHost[i++] = 0xFF;
		ROMBaseHost[i++] = 0x00;	// abs.addr = $E7FF00

		i = 0x7FF00;
		uint32 ramtop = (FastRAMBase + FastRAMSize);
		ROMBaseHost[i++] = 0x21;
		ROMBaseHost[i++] = 0xFC;	// MOVE.L #imm, abs.addr.w
		ROMBaseHost[i++] = ramtop >> 24;
		ROMBaseHost[i++] = ramtop >> 16;
		ROMBaseHost[i++] = ramtop >> 8;
		ROMBaseHost[i++] = ramtop;
		ROMBaseHost[i++] = 0x05;
		ROMBaseHost[i++] = 0xA4;	// abs.addr.w = $5A4 - ramtop
		ROMBaseHost[i++] = 0x4E;
		ROMBaseHost[i++] = 0xF9;	// JMP <abs.addr>
		ROMBaseHost[i++] = 0x00;
		ROMBaseHost[i++] = 0xE0;
		ROMBaseHost[i++] = 0x01;
		ROMBaseHost[i++] = 0xD2;	// abs.addr = $E001D2

		// Patch to make FastRAM available to GEMDOS
		i = 0x0096E;
		ROMBaseHost[i++] = 0x4E;
		ROMBaseHost[i++] = 0xB9;	// JSR <abs.addr>
		ROMBaseHost[i++] = 0x00;
		ROMBaseHost[i++] = 0xE7;
		ROMBaseHost[i++] = 0xFF;
		ROMBaseHost[i++] = 0x0E;	// abs.addr = $E7FF0E

		i = 0x7FF0E;
		// Declare FastRAM with Maddalt()
		ROMBaseHost[i++] = 0x2F;
		ROMBaseHost[i++] = 0x3C;	// MOVE.L #imm, -(sp)
		ROMBaseHost[i++] = FastRAMSize >> 24;
		ROMBaseHost[i++] = FastRAMSize >> 16;
		ROMBaseHost[i++] = FastRAMSize >> 8;
		ROMBaseHost[i++] = FastRAMSize;
		ROMBaseHost[i++] = 0x2F;
		ROMBaseHost[i++] = 0x3C;	// MOVE.L #imm, -(sp)
		ROMBaseHost[i++] = FastRAMBase >> 24;
		ROMBaseHost[i++] = FastRAMBase >> 16;
		ROMBaseHost[i++] = FastRAMBase >> 8;
		ROMBaseHost[i++] = FastRAMBase;
		ROMBaseHost[i++] = 0x3F;
		ROMBaseHost[i++] = 0x3C;	// MOVE.W #imm, -(sp)
		ROMBaseHost[i++] = 0x00;
		ROMBaseHost[i++] = 0x14;	// imm = $14 - Maddalt()
		ROMBaseHost[i++] = 0x4E;
		ROMBaseHost[i++] = 0x41;	// TRAP	#1
		ROMBaseHost[i++] = 0x4F;
		ROMBaseHost[i++] = 0xEF;	// LEA d16(sp), sp
		ROMBaseHost[i++] = 0x00;
		ROMBaseHost[i++] = 0x0A;	// d16 = $A

		// Allocate 64k _FRB buffer
		ROMBaseHost[i++] = 0x42;
		ROMBaseHost[i++] = 0x67;	// CLR.W -(sp) - ST-Ram only
		ROMBaseHost[i++] = 0x2F;
		ROMBaseHost[i++] = 0x3C;	// MOVE.L #imm, -(sp)
		ROMBaseHost[i++] = 0x00;
		ROMBaseHost[i++] = 0x01;
		ROMBaseHost[i++] = 0x20;
		ROMBaseHost[i++] = 0x00;	// imm = 64k + 8k
		ROMBaseHost[i++] = 0x3F;
		ROMBaseHost[i++] = 0x3C;	// MOVE.W #imm, -(sp)
		ROMBaseHost[i++] = 0x00;
		ROMBaseHost[i++] = 0x44;	// imm = $44 - Mxalloc()
		ROMBaseHost[i++] = 0x4E;
		ROMBaseHost[i++] = 0x41;	// TRAP	#1
		ROMBaseHost[i++] = 0x50;
		ROMBaseHost[i++] = 0x4F;	// ADDQ.W #8,sp
		ROMBaseHost[i++] = 0x06;
		ROMBaseHost[i++] = 0x80;	// ADDI.L #imm, d0
		ROMBaseHost[i++] = 0x00;
		ROMBaseHost[i++] = 0x00;
		ROMBaseHost[i++] = 0x1F;
		ROMBaseHost[i++] = 0xFF;	// imm = 8k - 1
		ROMBaseHost[i++] = 0x02;
		ROMBaseHost[i++] = 0x40;	// ANDI.W #imm, d0
		ROMBaseHost[i++] = 0xE0;
		ROMBaseHost[i++] = 0x00;	// imm = $E000 - 8k alignment

		// Store _FRB cookie
		ROMBaseHost[i++] = 0x20;
		ROMBaseHost[i++] = 0x78;	// MOVEA.L addr.w, a0
		ROMBaseHost[i++] = 0x05;
		ROMBaseHost[i++] = 0xA0;	// addr.w = 0x5A0 - cookie jar
		ROMBaseHost[i++] = 0x51;
		ROMBaseHost[i++] = 0x48;	// SUBQ.W #8,a0
		// .find_last_cookie:
		ROMBaseHost[i++] = 0x50;
		ROMBaseHost[i++] = 0x48;	// ADDQ.W #8,a0
		ROMBaseHost[i++] = 0x4A;
		ROMBaseHost[i++] = 0x90;	// TST.L (a0)
		ROMBaseHost[i++] = 0x66;
		ROMBaseHost[i++] = 0xFA;	// BNE.S .find_last_cookie
		ROMBaseHost[i++] = 0x20;
		ROMBaseHost[i++] = 0xFC;	// MOVE.L #imm, (a0)+
		ROMBaseHost[i++] = 0x5F;
		ROMBaseHost[i++] = 0x46;
		ROMBaseHost[i++] = 0x52;
		ROMBaseHost[i++] = 0x42;	// imm = '_FRB'
		ROMBaseHost[i++] = 0x21;
		ROMBaseHost[i++] = 0x50;	// MOVE.L (a0), 8(a0)
		ROMBaseHost[i++] = 0x00;
		ROMBaseHost[i++] = 0x08;	// d16 = 8 - copy jar size
		ROMBaseHost[i++] = 0x20;
		ROMBaseHost[i++] = 0xC0;	// MOVE.L d0,(a0)+
		ROMBaseHost[i++] = 0x42;
		ROMBaseHost[i++] = 0x90;	// CLR.L (a0)

		// Code overwritten by JSR
		ROMBaseHost[i++] = 0x70;
		ROMBaseHost[i++] = 0x03;	// MOVEQ.L #3,d0
		ROMBaseHost[i++] = 0x4E;
		ROMBaseHost[i++] = 0xB9;	// JSR <abs.addr>
		ROMBaseHost[i++] = 0x00;
		ROMBaseHost[i++] = 0xE0;
		ROMBaseHost[i++] = 0x0B;
		ROMBaseHost[i++] = 0xD2;	// abs.addr = $E00BD2
		ROMBaseHost[i++] = 0x4E;
		ROMBaseHost[i++] = 0x75;	// RTS
	}

	// Xconout patch
	if (bx_options.tos.redirect_CON) {
		ROMBaseHost[0x8d44] = ROMBaseHost[0x8d50] = M68K_EMUL_OP_PUT_SCRAP >> 8;
		ROMBaseHost[0x8d45] = ROMBaseHost[0x8d51] = M68K_EMUL_OP_PUT_SCRAP & 0xff;
		ROMBaseHost[0x8d46] = ROMBaseHost[0x8d52] = 0x4e;	// RTS
		ROMBaseHost[0x8d47] = ROMBaseHost[0x8d53] = 0x75;
	}
	else {
		ROMBaseHost[0x8d44] = 0xc2;
		ROMBaseHost[0x8d45] = 0x7c;
		ROMBaseHost[0x8d46] = 0x00;
		ROMBaseHost[0x8d47] = 0xff;
		ROMBaseHost[0x8d50] = 0x28;
		ROMBaseHost[0x8d51] = 0x79;
		ROMBaseHost[0x8d52] = 0x00;
		ROMBaseHost[0x8d53] = 0xe4;
	}

	infoprint("TOS 4.04 loading... [OK]");
}
