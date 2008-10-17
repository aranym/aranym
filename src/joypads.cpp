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

#define JOYSTICK_THRESHOLD	16384

enum {
	JP_PAUSE,	JP_FIRE0,	JP_UNDEF0,	JP_FIRE1,
	JP_UNDEF1,	JP_FIRE2,	JP_UNDEF2,	JP_OPTION,

	JP_UP,		JP_DOWN, 	JP_LEFT, 	JP_RIGHT,
	JP_KPMULT,	JP_KP7,		JP_KP4,		JP_KP1,
	JP_KP0,		JP_KP8,		JP_KP5,		JP_KP2,
	JP_KPNUM,	JP_KP9,		JP_KP6,		JP_KP3,
};

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
	host_state[0] =
	host_state[1] = 0;
}

uae_u8 JOYPADS::handleRead(uaecptr addr)
{
	Uint32 state = 0, state_mask = 0xffffffff;
	int shift = 0;

	switch (addr-getHWoffset()) {
		case 0x01:
			switch(mask) {
				case 0xfffe:
					state = host_state[0];
					state_mask = 3;
					break;
				case 0xfffd:
					state = host_state[0];
					shift = 2;
					state_mask = 3;
					break;
				case 0xfffb:
					state = host_state[0];
					shift = 4;
					state_mask = 3;
					break;
				case 0xfff7:
					state = host_state[0];
					shift = 6;
					state_mask = 3;
					break;
				case 0xffef:
					state = host_state[1];
					shift = -2;
					state_mask = 3<<2;
					break;
				case 0xffdf:
					state = host_state[1];
					state_mask = 3<<2;
					break;
				case 0xffbf:
					state = host_state[1];
					shift = 2;
					state_mask = 3<<2;
					break;
				case 0xff7f:
					state = host_state[1];
					shift = 4;
					state_mask = 3<<2;
					break;
			}
		case 0x02:
			switch(mask) {
				case 0xfffe:
					state = host_state[0];
					shift = 8;
					state_mask = 15;
					break;
				case 0xfffd:
					state = host_state[0];
					shift = 12;
					state_mask = 15;
					break;
				case 0xfffb:
					state = host_state[0];
					shift = 16;
					state_mask = 15;
					break;
				case 0xfff7:
					state = host_state[0];
					shift = 20;
					state_mask = 15;
					break;
				case 0xffef:
					state = host_state[1];
					shift = 4;
					state_mask = 15<<4;
					break;
				case 0xffdf:
					state = host_state[1];
					shift = 8;
					state_mask = 15<<4;
					break;
				case 0xffbf:
					state = host_state[1];
					shift = 12;
					state_mask = 15<<4;
					break;
				case 0xff7f:
					state = host_state[1];
					shift = 16;
					state_mask = 15<<4;
					break;
			}
	}

	uae_u8 value = 0xff;
	if (shift<0) {
		value = ~((state<<(-shift)) & mask);
	} else {
		value = ~((state>>shift) & mask);
	}	
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

void JOYPADS::sendJoystickAxis(int numjoy, int numaxis, int value)
{
}

void JOYPADS::sendJoystickHat(int numjoy, int value)
{
}

void JOYPADS::sendJoystickButton(int numjoy, int which, int pressed)
{
}


/*
vim:ts=4:sw=4:
*/
