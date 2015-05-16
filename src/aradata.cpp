/*
 * aradata.cpp - ARAnyM special HW registers
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
#include "newcpu.h"
#include "memory-uae.h"
#include "aradata.h"
#include "parameters.h"

ARADATA::ARADATA(memptr addr, uint32 size) : BASE_IO(addr, size)
{
	abase = 0;
	linea_trap = 0;
	reset();
}

int ARADATA::getAtariMouseX()
{
	if (abase)
		return ReadHWMemInt16(abase - 0x25a);
	return mouse_x;
}

int ARADATA::getAtariMouseY()
{
	if (abase)
		return ReadHWMemInt16(abase - 0x258);
	return mouse_y;
}

void ARADATA::reset()
{
	mouseDriver = false;
	mouse_x = -1;
	mouse_y = -1;
	WriteHWMemInt32(40, 0);
}

void ARADATA::setAbase(void)
{
	if (boot_lilo) return;
	uae_u32 new_abase;
	uae_u32 new_linea_trap;
	new_linea_trap = ReadHWMemInt32(40);
	/*
	 * Prevent linea68000() from being called too early;
	 * after a reboot the exception vector might still point
	 * to the memory of some program that hooked it up before
	 * but is now gone
	 */
	if (new_linea_trap != 0 && (linea_trap == 0 || linea_trap == new_linea_trap))
	{
		/* prevent it also from being called recursively on slow machines */
		linea_trap = 1;
		new_abase = linea68000(0xa000);
		abase = new_abase;
		linea_trap = new_linea_trap;
	}
}

uint8 ARADATA::handleRead(memptr addr) {
	addr -= getHWoffset();
	switch(addr) {
		case 0: return '_';
		case 1: return 'A';
		case 2: return 'R';
		case 3: return 'A';
		case 4: return 0;	/* VERSION_MAJOR */
		case 5: return 0;	/* VERSION_MINOR */
		case 6: return FastRAMSize >> 24;
		case 7: return FastRAMSize >> 16;
		case 8: return FastRAMSize >> 8;
		case 9: return FastRAMSize;
	}

	return 0;
}

void ARADATA::handleWrite(memptr addr, uint8 value) {
	addr -= getHWoffset();
	switch(addr) {
		case 14: mouse_x = (mouse_x & 0xff) | (value << 8); break;
		case 15: mouse_x = (mouse_x & 0xff00) | value; break;
		case 16: mouse_y = (mouse_y & 0xff) | (value << 8); break;
		case 17: mouse_y = (mouse_y & 0xff00) | value; break;
	}
	mouseDriver = true;
}
