/*
 * ikbd.h - IKBD 6301 emulation code - declaration
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

#ifndef _IKBD_H
#define _IKBD_H

#include <SDL.h>
#include <SDL_thread.h>

#include "acia.h"

/*--- Defines ---*/

enum ikbd_packet_t {
	IKBD_PACKET_UNKNOWN=0,
	IKBD_PACKET_KEYBOARD,
	IKBD_PACKET_MOUSE,
	IKBD_PACKET_JOYSTICK
};

#define IKBD_JOY_UP 	0
#define IKBD_JOY_DOWN 	1
#define IKBD_JOY_LEFT 	2
#define IKBD_JOY_RIGHT 	3
#define IKBD_JOY_FIRE 	7

/*--- IKBD class ---*/

class IKBD: public ACIA {
	private:
		/* IKBD keyboard state */

		/* IKBD mouse state */
		int mouse_enabled;
		int mouserel_enabled;
		int mousex, mousey, mouseb;
		
		/* IKBD joysticks state */
		int joy_enabled[2];
		int joy_state[2];

		/* Buffer when writing to IKBD */
		uae_u8 *outbuffer;	
		int outbufferlen;

		int	outwrite;

		/* Buffer when reading from IKBD */
		uae_u8 *inbuffer;
		int inbufferlen;
		int inwrite, inread;
		ikbd_packet_t intype;	/* Latest type of packet in buffer */

		uint8 int2bcd(int a);
		void ThrowInterrupt(void);
		void MergeMousePacket(int *relx, int *rely, int buttons);

		void send(uae_u8 value);

	public:
		IKBD(memptr addr, uint32 size);
		// IKBD(int inlen, int outlen);	/* Params are length (2^x) of in/out buffers */
		~IKBD();
		void reset(void);

		uae_u8 ReadStatus(void);
		void WriteControl(uae_u8 value);
		uae_u8 ReadData(void);
		void WriteData(uae_u8 value);

		void SendKey(uae_u8 scancode);
		void SendMouseMotion(int relx, int rely, int buttons);
		void SendJoystickAxis(int numjoy, int numaxis, int value);
		void SendJoystickHat(int numjoy, int value);
		void SendJoystickButton(int numjoy, int pressed);
};

#endif /* _IKBD_H */
