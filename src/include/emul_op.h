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
const uint16 M68K_ILLEGAL = 0x4afc;
const uint16 M68K_NOP = 0x4e71;
const uint16 M68K_RTS = 0x4e75;
const uint16 M68K_RTD = 0x4e74;
const uint16 M68K_RTR = 0x4e77;
const uint16 M68K_JMP = 0x4ef9;
const uint16 M68K_JMP_A0 = 0x4ed0;
const uint16 M68K_JSR = 0x4eb9;
const uint16 M68K_JSR_A0 = 0x4e90;

// Extended opcodes
enum {
	M68K_EXEC_RETURN = 0x7100,		// Extended opcodes (illegal moveq form)
	M68K_EMUL_BREAK,			// 0x7101 - breakpoint
	M68K_EMUL_OP_SHUTDOWN,
	M68K_EMUL_OP_RESET,
	M68K_EMUL_OP_CLKNOMEM,
	M68K_EMUL_OP_READ_XPRAM,
	M68K_EMUL_OP_READ_XPRAM2,
	M68K_EMUL_OP_PATCH_BOOT_GLOBS,
	M68K_EMUL_OP_FIX_BOOTSTACK,		// 0x7108
	M68K_EMUL_OP_FIX_MEMSIZE,
	M68K_EMUL_OP_INSTALL_DRIVERS,
	M68K_EMUL_OP_SERD,
	M68K_EMUL_OP_SONY_OPEN,
	M68K_EMUL_OP_SONY_PRIME,
	M68K_EMUL_OP_SONY_CONTROL,
	M68K_EMUL_OP_SONY_STATUS,
	M68K_EMUL_OP_DISK_OPEN,			// 0x7110
	M68K_EMUL_OP_DISK_PRIME,
	M68K_EMUL_OP_DISK_CONTROL,
	M68K_EMUL_OP_DISK_STATUS,
	M68K_EMUL_OP_CDROM_OPEN,
	M68K_EMUL_OP_CDROM_PRIME,
	M68K_EMUL_OP_CDROM_CONTROL,
	M68K_EMUL_OP_CDROM_STATUS,
	M68K_EMUL_OP_VIDEO_OPEN,		// 0x7118 - obsolete
	M68K_EMUL_OP_VIDEO_CONTROL,		// 0x7119 - obsolete
	M68K_EMUL_OP_VIDEO_STATUS,
	M68K_EMUL_OP_SERIAL_OPEN,
	M68K_EMUL_OP_SERIAL_PRIME,
	M68K_EMUL_OP_SERIAL_CONTROL,
	M68K_EMUL_OP_SERIAL_STATUS,
	M68K_EMUL_OP_SERIAL_CLOSE,
	M68K_EMUL_OP_ETHER_OPEN,		// 0x7120
	M68K_EMUL_OP_ETHER_CONTROL,
	M68K_EMUL_OP_ETHER_READ_PACKET,
	M68K_EMUL_OP_ADBOP,
	M68K_EMUL_OP_INSTIME,
	M68K_EMUL_OP_RMVTIME,
	M68K_EMUL_OP_PRIMETIME,
	M68K_EMUL_OP_MICROSECONDS,
	M68K_EMUL_OP_SCSI_DISPATCH,		// 0x7128
	M68K_EMUL_OP_IRQ,
	M68K_EMUL_OP_PUT_SCRAP,			// 0x712a - used in TOS ROM patch
	M68K_EMUL_OP_CHECKLOAD,
	M68K_EMUL_OP_AUDIO,			// 0x712c - obsolete
	M68K_EMUL_OP_EXTFS_COMM,		// 0x712d - Extfs support
	M68K_EMUL_OP_EXTFS_HFS,			// 0x712e - Extfs support
	M68K_EMUL_OP_BLOCK_MOVE,
	M68K_EMUL_OP_SOUNDIN_OPEN,		// 0x7130
	M68K_EMUL_OP_SOUNDIN_PRIME,
	M68K_EMUL_OP_SOUNDIN_CONTROL,
	M68K_EMUL_OP_SOUNDIN_STATUS,
	M68K_EMUL_OP_SOUNDIN_CLOSE,
	M68K_EMUL_OP_DEBUGUTIL,			// 0x7135 - EmuTOS konsole output
	M68K_EMUL_OP_DMAREAD,			// 0x7136 - obsolete
	M68K_EMUL_OP_XHDI,			// 0x7137 - obsolete
	M68K_EMUL_OP_VIDEO_DRIVER,		// 0x7138 - obsolete
	M68K_EMUL_OP_CPUDEBUG_ON,		// 0x7139 - cpu_debugging on
	M68K_EMUL_OP_CPUDEBUG_OFF,		// 0x713a - cpu_debugging off
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
extern void EmulOp(uint16 opcode, struct M68kRegisters *r);	// Execute EMUL_OP opcode (called by 68k emulator or Line-F trap handler)

#endif
