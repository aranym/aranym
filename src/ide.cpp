/*
 * ide.cpp - Falcon IDE emulation code - wrapper for bochs' ata.cpp
 *
 * Copyright (c) 2001-2004 Petr Stehlik of ARAnyM dev team (see AUTHORS)
 * 
 * This file is part of the ARAnyM project which builds a new and powerful
 * TOS/FreeMiNT compatible virtual machine running on almost any hardware.
 *
 * ARAnyM is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * ARAnyM is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ARAnyM; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


#include "sysdeps.h"
#include "hardware.h"
#include "parameters.h"
#include "ide.h"
#include "ata.h"

#define BX_INSERTED	true	// copied from emu_bochs.h

IDE::IDE(memptr addr, uint32 size) : BASE_IO(addr, size) {
	reset();
}

void IDE::reset() {
	// convert aranym's "present" and "isCDROM" to bochs' "status" and "type"
	int channel = 0;
	for(int device=0; device<2; device++) {
    	if (bx_options.atadevice[channel][device].present) {
    		bx_options.atadevice[channel][device].status = BX_INSERTED;
			bx_options.atadevice[channel][device].type = (bx_options.atadevice[channel][device].isCDROM) ? IDE_CDROM : IDE_DISK;
		}
		else {
			bx_options.atadevice[channel][device].type = IDE_NONE;
		}
	}

	// init bochs emulation
	bx_hard_drive.init();
}

uae_u8 IDE::handleRead(uaecptr addr) {
	return bx_hard_drive.read_handler(&bx_hard_drive, addr, 1);
}

void IDE::handleWrite(uaecptr addr, uae_u8 value) {
	bx_hard_drive.write_handler(&bx_hard_drive, addr, value, 1);
}

uae_u16 IDE::handleReadW(uaecptr addr) {
	if (addr == 0xf00002) {
		addr = 0xf00000; // according to Xavier $f00002 is mapped to the same IDE register as $f00000
	}
	return bx_hard_drive.read_handler(&bx_hard_drive, addr, 2);
}

void IDE::handleWriteW(uaecptr addr, uae_u16 value) {
	if (addr == 0xf00002) {
		addr = 0xf00000; // according to Xavier $f00002 is mapped to the same IDE register as $f00000
	}
	bx_hard_drive.write_handler(&bx_hard_drive, addr, value, 2);
}

uae_u32 IDE::handleReadL(uaecptr addr) {
	return bx_hard_drive.read_handler(&bx_hard_drive, addr, 4);
}

void IDE::handleWriteL(uaecptr addr, uae_u32 value) {
	bx_hard_drive.write_handler(&bx_hard_drive, addr, value, 4);
}
