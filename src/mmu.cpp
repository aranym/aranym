/*
 * mmu.cpp - Atari Falcon MMU emulation code
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
#include "cpu_emulation.h"
#include "memory-uae.h"
#include "mmu.h"
#include "parameters.h"

MMU::MMU(memptr addr, uint32 size) : BASE_IO(addr, size) {}

void MMU::reset()
{
	memctl = 0;
}

uint8 MMU::handleRead(memptr addr) {
	addr -= getHWoffset();
	switch(addr) {
		case 1: return memctl;
		case 6: return bx_options.video.monitor == 1 ? 0xe6 : 0xa6;	// a6 = 14MB, 96 = 4MB on VGA
		case 7: return 0x61;
	}

	return 0;
}

void MMU::handleWrite(memptr addr, uint8 value) {
	addr -= getHWoffset();
	switch(addr) {
		case 1: memctl = value; break;
		case 6: break;
		case 7: break;
	}
}
