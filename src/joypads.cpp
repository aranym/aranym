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
#include "memory-uae.h"
#include "joypads.h"

#define DEBUG 0
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

/* multiplexer mask, num joypad, shift, mask */
const int JOYPADS::multiplexer0[8][4]={
	{0xfffe,	0,	0,	3},
	{0xfffd,	0,	2,	3},
	{0xfffb,	0,	4,	3},
	{0xfff7,	0,	6,	3},
	{0xffef,	1,	-2,	3<<2},
	{0xffdf,	1,	0,	3<<2},
	{0xffbf,	1,	2,	3<<2},
	{0xff7f,	1,	4,	3<<2},
};

const int JOYPADS::multiplexer1[8][4]={
	{0xfffe,	0,	8,	15},
	{0xfffd,	0,	12,	15},
	{0xfffb,	0,	16,	15},
	{0xfff7,	0,	20,	15},
	{0xffef,	1,	4,	15<<4},
	{0xffdf,	1,	8,	15<<4},
	{0xffbf,	1,	12,	15<<4},
	{0xff7f,	1,	16,	15<<4},
};

/* Buttons, for mapping */
const int JOYPADS::buttons[17]={
	JP_FIRE0,	JP_FIRE1,	JP_FIRE2,	JP_PAUSE,
	JP_OPTION,	JP_KP0,		JP_KP1,		JP_KP2,
	JP_KP3,		JP_KP4,		JP_KP5,		JP_KP6,
	JP_KP7,		JP_KP8,		JP_KP9,		JP_KPMULT,
	JP_KPNUM
};

JOYPADS::JOYPADS(memptr addr, uint32 size) : BASE_IO(addr, size)
{
	D(bug("joypads: interface created at 0x%06x", getHWoffset()));

	/* Read button mappings for joypads */
	int i, tmp[17];

	for (i=0; i<17; i++) {
		joypada_mapping[i] = joypadb_mapping[i] = i;
	}

	sscanf(bx_options.joysticks.joypada_mapping,
		"%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d",
		&tmp[0], &tmp[1], &tmp[2], &tmp[3], &tmp[4], &tmp[5],
		&tmp[6], &tmp[7], &tmp[8], &tmp[9], &tmp[10], &tmp[11],
		&tmp[12], &tmp[13], &tmp[14], &tmp[15], &tmp[16]
	);

	for (i=0; i<17; i++) {
		if ((tmp[i]>=0) && tmp[i]<17) {
			joypada_mapping[tmp[i]]=i;
		}
	}

	sscanf(bx_options.joysticks.joypadb_mapping,
		"%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d",
		&tmp[0], &tmp[1], &tmp[2], &tmp[3], &tmp[4], &tmp[5],
		&tmp[6], &tmp[7], &tmp[8], &tmp[9], &tmp[10], &tmp[11],
		&tmp[12], &tmp[13], &tmp[14], &tmp[15], &tmp[16]
	);

	for (i=0; i<17; i++) {
		if ((tmp[i]>=0) && tmp[i]<17) {
			joypadb_mapping[tmp[i]]=i;
		}
	}

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
	Uint32 state = 0;
	int state_mask = 0, state_shift = 0, i;
	uae_u8 value = 0xff;

	switch (addr-getHWoffset()) {
		case 0x01:
			for (i=0; i<8; i++) {
				if (multiplexer0[i][0] == mask) {
					state = host_state[multiplexer0[i][1]];
					state_shift = multiplexer0[i][2];
					state_mask = multiplexer0[i][3];
					break;
				}
			}
			break;
		case 0x02:
			for (i=0; i<8; i++) {
				if (multiplexer1[i][0] == mask) {
					state = host_state[multiplexer1[i][1]];
					state_shift = multiplexer1[i][2];
					state_mask = multiplexer1[i][3];
					break;
				}
			}
			break;
	}

	if (state_shift<0) {
		value = ~((state<<(-state_shift)) & state_mask);
	} else {
		value = ~((state>>state_shift) & state_mask);
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
	switch(numaxis) {
		case 0:
			host_state[numjoy] &= ~((1<<JP_LEFT)|(1<<JP_RIGHT));
			if (value<-JOYSTICK_THRESHOLD) {
				host_state[numjoy] |= 1<<JP_LEFT;
			}
			if (value>JOYSTICK_THRESHOLD) {
				host_state[numjoy] |= 1<<JP_RIGHT;
			}
			break;
		case 1:
			host_state[numjoy] &= ~((1<<JP_UP)|(1<<JP_DOWN));
			if (value<-JOYSTICK_THRESHOLD) {
				host_state[numjoy] |= 1<<JP_UP;
			}
			if (value>JOYSTICK_THRESHOLD) {
				host_state[numjoy] |= 1<<JP_DOWN;
			}
			break;
	}

	D(bug("joypad[%d]=0x%08x", numjoy, host_state[numjoy]));
}

void JOYPADS::sendJoystickHat(int numjoy, int value)
{
	host_state[numjoy] &= ~((1<<JP_UP)|(1<<JP_DOWN)|(1<<JP_LEFT)|(1<<JP_RIGHT));

	if (value & SDL_HAT_LEFT) host_state[numjoy] |= 1<<JP_LEFT;
	if (value & SDL_HAT_RIGHT) host_state[numjoy] |= 1<<JP_RIGHT;
	if (value & SDL_HAT_UP) host_state[numjoy] |= 1<<JP_UP;
	if (value & SDL_HAT_DOWN) host_state[numjoy] |= 1<<JP_DOWN;

	D(bug("joypad[%d]=0x%08x", numjoy, host_state[numjoy]));
}

void JOYPADS::sendJoystickButton(int numjoy, int which, int pressed)
{
	int numbit = JP_UNDEF0;
	int *maptable = (numjoy==0 ? joypada_mapping : joypadb_mapping);

	/* Get bit to clear/set */
	if ((which>=0) && (which<17)) {
		numbit = buttons[maptable[which]];
	}

	if (pressed) {
		host_state[numjoy] |= 1<<numbit;
	} else {
		host_state[numjoy] &= ~(1<<numbit);
	}

	D(bug("joypad[%d]=0x%08x", numjoy, host_state[numjoy]));
}


/*
vim:ts=4:sw=4:
*/
