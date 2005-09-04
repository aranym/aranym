/*
 * fakeio.cpp - memory mapped peripheral emulation code
 *
 * Copyright (c) 2001-2005 Petr Stehlik of ARAnyM dev team (see AUTHORS)
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
#include "memory.h"
#include "icio.h"
#include "parameters.h"

#ifdef HW_SIGSEGV
# define ReadFakeIOPlace(a) do_get_mem_byte((uint8 *)(a - HWBase + FakeIOBaseHost))
# define WriteFakeIOPlace(a, v) do_put_mem_byte((uint8 *)(a - HWBase + FakeIOBaseHost), v);
#else
// exactly same code as with previous deprecated Atari2HostAddr().
// Maybe that correct solution is to move this HW_SIGSEGV #ifdef into
// the ReadHWMem functions...
# define ReadFakeIOPlace(a) ReadHWMemInt8(a)
# define WriteFakeIOPlace(a, v) WriteHWMemInt8(a, v)
#endif

uint8 BASE_IO::handleRead(memptr addr) {
	return ReadFakeIOPlace(addr); // fetch from underlying RAM
}

uint16 BASE_IO::handleReadW(memptr addr) {
	return (handleRead(addr) << 8) | handleRead(addr+1);
}

uint32 BASE_IO::handleReadL(memptr addr) {
	return (handleReadW(addr) << 16) | handleReadW(addr+2);
}

void BASE_IO::handleWrite(memptr addr, uint8 value) {
	WriteFakeIOPlace(addr, value); // store to underlying RAM
}

void BASE_IO::handleWriteW(memptr addr, uint16 value) {
	handleWrite(addr, value >> 8);
	handleWrite(addr+1, value);
}

void BASE_IO::handleWriteL(memptr addr, uint32 value) {
	handleWriteW(addr, value >> 16);
	handleWriteW(addr+2, value);
}

/*
vim:ts=4:sw=4:
*/
