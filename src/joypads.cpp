/*
	Joypads port emulation

	ARAnyM (C) 2008 Patrice Mandin

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
#include "hardware.h"
#include "cpu_emulation.h"
#include "memory.h"
#include "joypads.h"

#define DEBUG 1
#include "debug.h"

JOYPADS::JOYPADS(memptr addr, uint32 size) : BASE_IO(addr, size)
{
	D(bug("joypads: interface created at 0x%06x", getHWoffset()));

	reset();
}

JOYPADS::~JOYPADS()
{
	D(bug("joypads: interface destroyed at 0x%06x", getHWoffset()));
}

void JOYPADS::reset()
{
	D(bug("joypads: reset"));

	mask = 0xffff;
}

uae_u8 JOYPADS::handleRead(uaecptr addr)
{
	uae_u8 value = 0xff;

	/*switch(addr-getHWoffset()) {
		case 0:
			break;
		case 2:
			break;
	}*/
	
	D(bug("joypads: Read 0x%02x from 0x%08x",value,addr));

	return value;
}

void JOYPADS::handleWrite(uaecptr addr, uae_u8 value)
{
	D(bug("joypads: Write 0x%02x to 0x%08x",value,addr));

	switch(addr-getHWoffset()) {
		case 0x02:
			mask = (mask & 0xff) | (value<<8);
			break;
		case 0x03:
			mask = (mask & 0xff00) | value;
			break;
	}
}

/*
vim:ts=4:sw=4:
*/
