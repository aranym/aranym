/*
 * ikbd.cpp - IKBD 6301 emulation code
 *
 * Copyright (c) 2001-2013 Petr Stehlik of ARAnyM dev team (see AUTHORS)
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
#include "ikbd.h"

#define DEBUG 0
#include "debug.h"

/*--- Defines ---*/

#define DEFAULT_INBUFFERLEN (1<<13)
#define DEFAULT_OUTBUFFERLEN (1<<4)

#define MOUSE_DELTA_MAX 	63
#define JOYSTICK_THRESHOLD	16384

/*--- Constructor/destructor of the IKBD class ---*/

IKBD::IKBD(memptr addr, uint32 size) : ACIA(addr, size)
{
	D(bug("ikbd: interface created at 0x%06x", getHWoffset()));

	inbufferlen = DEFAULT_INBUFFERLEN;
	outbufferlen = DEFAULT_OUTBUFFERLEN;

	inbuffer = new uint8[inbufferlen];
	outbuffer = new uint8[outbufferlen];

	reset();
};

IKBD::~IKBD()
{
	delete [] outbuffer;
	outbuffer = NULL;
	delete [] inbuffer;
	inbuffer = NULL;

	D(bug("ikbd: interface destroyed at 0x%06x", getHWoffset()));
}

/*--- IKBD i/o access functions ---*/

void IKBD::reset()
{
	inread = inwrite = 0;
	outwrite = 0;
	intype = IKBD_PACKET_UNKNOWN;

	/* Default: mouse on port 0 enabled */
	mouse_enabled = SDL_TRUE;
	mouserel_enabled = SDL_TRUE;
	yaxis_reversed = false;
	mousex = mousey = mouseb = 0;
	
	/* Default: joystick on port 0 disabled */
	joy_enabled[0] = SDL_FALSE;
	joy_state[0] = 0;
	
	/* Default: joystick on port 1 enabled */
	joy_enabled[1] = SDL_TRUE;
	joy_state[1] = 0;

	D(bug("ikbd: reset"));
}

uint8 IKBD::ReadStatus()
{
	D(bug("ikbd: ReadStatus()=0x%02x",sr));
	return sr;
}

void IKBD::WriteControl(uint8 value)
{
	cr = value;

	D(bug("ikbd: WriteControl(0x%02x)",cr));

#if DEBUG
	PrintControlRegister("ikbd",cr);
#endif
}

uint8 IKBD::ReadData()
{
	if (inread != inwrite) {

		rxdr = inbuffer[inread++];
		D(bug("ikbd: ReadData()=0x%02x at position %d",rxdr, inread-1));

		inread &= (inbufferlen-1);

		if (inread == inwrite) {
			/* Queue empty */

			/* Update MFP GPIP */
			getMFP()->setGPIPbit(0x10, 0x10);

			sr &= ~((1<<ACIA_SR_INTERRUPT)|(1<<ACIA_SR_RXFULL));

		} else {
			/* Still bytes to read ? */
			sr |= (1<<ACIA_SR_RXFULL);

			ThrowInterrupt();
		} 
	}

	return rxdr;
}

inline uint8 IKBD::int2bcd(int a)
{
	return (a % 10) + ((a / 10) << 4);
}

void IKBD::WriteData(uint8 value)
{
	if (outbuffer==NULL)
		return;

	outbuffer[outwrite++] = txdr = value;
	D(bug("ikbd: WriteData(0x%02x)",txdr));

#if 0
	printf("ikbd: ");
	for (int i=0;i<outwrite;i++) {
		printf("0x%02x ",outbuffer[i]);			
	}
	printf("\n");
#endif

	switch (outwrite) {
		case 1:
			switch(outbuffer[0]) {
				case 0x08:
					D(bug("ikbd: Set mouse relative mode"));
					mouse_enabled = SDL_TRUE;
					mouserel_enabled = SDL_TRUE;
					joy_enabled[1] = SDL_TRUE;
					outwrite = 0;
					break;
				case 0x0d:
					D(bug("ikbd: Get absolute mouse position"));
					outwrite = 0;
					break;
				case 0x0f:
					D(bug("ikbd: Set Y origin bottom"));
					yaxis_reversed = true;
					outwrite = 0;
					break;
				case 0x10:
					D(bug("ikbd: Set Y origin top"));
					yaxis_reversed = false;
					outwrite = 0;
					break;
				case 0x11:
					D(bug("ikbd: Resume transfer"));
					outwrite = 0;
					break;
				case 0x12:
					D(bug("ikbd: Disable mouse"));
					mouse_enabled = SDL_FALSE;
					outwrite = 0;
					break;
				case 0x13:
					D(bug("ikbd: Pause transfer"));
					outwrite = 0;
					break;
				case 0x14:
					D(bug("ikbd: Joystick autoread on"));
					outwrite = 0;
					break;
				case 0x15:
					D(bug("ikbd: Joystick autoread off"));
					outwrite = 0;
					break;
				case 0x16:
					D(bug("ikbd: Read joystick"));
					{
						// send a joystick packet to make GFA Basic happy
						intype = IKBD_PACKET_JOYSTICK;
						send(0xfe | 1);
						send(joy_state[1]);
					}
					outwrite = 0;
					break;
				case 0x18:
					D(bug("ikbd: Joystick fire read continuously"));
					outwrite = 0;
					break;
				case 0x1a:
					D(bug("ikbd: Joysticks disabled"));
					joy_enabled[0] = joy_enabled[1] = SDL_FALSE;
					outwrite = 0;
					break;
				case 0x1c:
					{
						D(bug("ikbd: Read date/time"));

						// Get current time
						time_t tim = time(NULL);
						struct tm *curtim = bx_options.gmtime ? gmtime(&tim) : localtime(&tim);

						// Return packet
						send(0xfc);

						// Return time-of-day clock as yy-mm-dd-hh-mm-ss as BCD
						send(int2bcd(curtim->tm_year % 100));
						send(int2bcd(curtim->tm_mon+1));
						send(int2bcd(curtim->tm_mday));
						send(int2bcd(curtim->tm_hour));
						send(int2bcd(curtim->tm_min));
						send(int2bcd(curtim->tm_sec));

						outwrite = 0;
					}
					break;
			}
			break;
		case 2:
			switch(outbuffer[0]) {
				case 0x07:
					D(bug("ikbd: Set mouse report mode"));
					outwrite = 0;
					break;
				case 0x17:
					D(bug("ikbd: Joystick position read continuously"));
					outwrite = 0;
					break;
				case 0x80:
					if (outbuffer[1]==0x01) {
						reset();
						outwrite = 0;
						send(0xf1); // if all's well code 0xF1 is returned, else the break codes of all keys making contact 
					}
					break;
			}
			break;
		case 3:
			switch(outbuffer[0]) {
				case 0x0a:
					D(bug("ikbd: Set mouse emulation via keyboard"));
					mouse_enabled = SDL_TRUE;
					joy_enabled[1] = SDL_TRUE;
					outwrite = 0;
					break;
				case 0x0b:
					D(bug("ikbd: Set mouse threshold"));
					outwrite = 0;
					break;
				case 0x0c:
					D(bug("ikbd: Set mouse accuracy"));
					outwrite = 0;
					break;
				case 0x20:
					D(bug("ikbd: Write to IKBD ram"));
					/* FIXME: we should ignore a given number of following bytes */
					outwrite = 0;
					break;
				case 0x21:
					D(bug("ikbd: Read from IKBD ram"));
					outwrite = 0;
					break;
			}
			break;
		case 4:
			switch(outbuffer[0]) {
				case 0x09:
					D(bug("ikbd: Set mouse absolute mode"));
					mouse_enabled = SDL_TRUE;
					mouserel_enabled = SDL_FALSE;
					joy_enabled[1] = SDL_TRUE;
					outwrite = 0;
					break;
			}
			break;
		case 5:
			switch(outbuffer[0]) {
				case 0x0e:
					D(bug("ikbd: Set mouse position"));
					outwrite = 0;
					break;
			}
			break;
		case 6:
			switch(outbuffer[0]) {
				case 0x19:
					D(bug("ikbd: Set joystick keyboard emulation"));
					outwrite = 0;
					break;
				case 0x1b:
					D(bug("ikbd: Set date/time"));
					outwrite = 0;
					break;
			}
			break;
		default:
			outwrite = 0;
			break;
	}
}

/*--- Functions called to transmit an input event from host to IKBD ---*/

void IKBD::SendKey(uint8 scancode)
{
	intype = IKBD_PACKET_KEYBOARD;
	send(scancode);
}

void IKBD::SendMouseMotion(int relx, int rely, int buttons, bool slow)
{
	if (!mouse_enabled)
		return;

	if (!slow && (abs(relx) > 2048 || abs(rely) > 2048)) {
		bug("IKBD: ignoring insane mouse motion: [%d, %d]", relx, rely);
		return;
	}

	// Generate several mouse packets if motion is too long
	do {
		int movex, movey;
		
		if (slow)
		{
			movex = relx > 0 ? 1 : relx < 0 ? -1 : 0;
			movey = rely > 0 ? 1 : rely < 0 ? -1 : 0;
		} else
		{
			movex = (abs(relx) > MOUSE_DELTA_MAX) ? ( (relx > 0) ? MOUSE_DELTA_MAX : -MOUSE_DELTA_MAX ) : relx;
			movey = (abs(rely) > MOUSE_DELTA_MAX) ? ( (rely > 0) ? MOUSE_DELTA_MAX : -MOUSE_DELTA_MAX ) : rely;
		}
		relx -= movex;
		rely -= movey;

		/* Merge with the previous mouse packet ? */
		if (!slow)
			MergeMousePacket(&movex, &movey, buttons);

		/* Send the packet */
		intype = IKBD_PACKET_MOUSE;
		send(0xf8 | (buttons & 3));
		send(movex);
		send(yaxis_reversed ? -movey : movey);
		// bug("IKBD: creating mouse packet [%d (%d), %d (%d), %d]", movex, relx, (yaxis_reversed ? -movey : movey), rely, buttons & 3);

	} while (relx || rely);
}

void IKBD::MergeMousePacket(int *relx, int *rely, int buttons)
{
	int mouse_inwrite;
	int distx, disty;

	/* Check if it was a mouse packet */
	if (intype != IKBD_PACKET_MOUSE)
		return;

	/* And ReadData() is not waiting our packet */
	if (inread == inwrite)
		return;

	/* Where is the previous mouse packet ? */
	mouse_inwrite = (inwrite-3) & (inbufferlen-1);

	/* Check if not currently read */
	if ((inread == mouse_inwrite) ||
		(inread == ((mouse_inwrite+1) & (inbufferlen-1))) ||
		(inread == ((mouse_inwrite+2) & (inbufferlen-1))) )
		return;

	/* Check mouse packet header */
	if ((inbuffer[mouse_inwrite]<=0xf8) && (inbuffer[mouse_inwrite]>=0xfb))
		return;

	/* Check if same buttons are pressed */
	if (inbuffer[mouse_inwrite] != (0xf8 | (buttons & 3)))
		return;

	/* Check if distances are not too far */
	distx = *relx + inbuffer[(mouse_inwrite+1) & (inbufferlen-1)];
	if (abs(distx) > MOUSE_DELTA_MAX)
		return;

	disty = *rely + inbuffer[(mouse_inwrite+2) & (inbufferlen-1)];
	if (abs(disty) > MOUSE_DELTA_MAX)
		return;

	/* Replace previous packet */
	inwrite = mouse_inwrite;
	*relx = distx;
	*rely = disty;
}

void IKBD::SendJoystickAxis(int numjoy, int numaxis, int value)
{
	uint8 newjoy_state;

	if (!joy_enabled[numjoy])
		return;

	newjoy_state = joy_state[numjoy];

	switch(numaxis) {
		case 0:
			newjoy_state &= ~((1<<IKBD_JOY_LEFT)|(1<<IKBD_JOY_RIGHT));
			if (value<-JOYSTICK_THRESHOLD) {
				newjoy_state |= 1<<IKBD_JOY_LEFT;
			}
			if (value>JOYSTICK_THRESHOLD) {
				newjoy_state |= 1<<IKBD_JOY_RIGHT;
			}
			break;
		case 1:
			newjoy_state &= ~((1<<IKBD_JOY_UP)|(1<<IKBD_JOY_DOWN));
			if (value<-JOYSTICK_THRESHOLD) {
				newjoy_state |= 1<<IKBD_JOY_UP;
			}
			if (value>JOYSTICK_THRESHOLD) {
				newjoy_state |= 1<<IKBD_JOY_DOWN;
			}
			break;
	}

	if (joy_state[numjoy] != newjoy_state) {
		joy_state[numjoy] = newjoy_state;
		intype = IKBD_PACKET_JOYSTICK;
		send(0xfe | numjoy);
		send(joy_state[numjoy]);
	}
}

void IKBD::SendJoystickHat(int numjoy, int value)
{
	uint8 newjoy_state;

	if (!joy_enabled[numjoy])
		return;

	/* Keep current button press */
	newjoy_state = joy_state[numjoy] & (1<<IKBD_JOY_FIRE);

	if (value & SDL_HAT_LEFT) newjoy_state |= 1<<IKBD_JOY_LEFT;
	if (value & SDL_HAT_RIGHT) newjoy_state |= 1<<IKBD_JOY_RIGHT;
	if (value & SDL_HAT_UP) newjoy_state |= 1<<IKBD_JOY_UP;
	if (value & SDL_HAT_DOWN) newjoy_state |= 1<<IKBD_JOY_DOWN;

	if (joy_state[numjoy] != newjoy_state) {
		joy_state[numjoy] = newjoy_state;
		intype = IKBD_PACKET_JOYSTICK;
		send(0xfe | numjoy);
		send(joy_state[numjoy]);
	}
}

void IKBD::SendJoystickButton(int numjoy, int pressed)
{
	uint8 newjoy_state;

	if (!joy_enabled[numjoy])
		return;

	newjoy_state = joy_state[numjoy];

	newjoy_state &= ~(1<<IKBD_JOY_FIRE);
	newjoy_state |= (pressed & 1)<<IKBD_JOY_FIRE;

	if (joy_state[numjoy] != newjoy_state) {
		joy_state[numjoy] = newjoy_state;
		intype = IKBD_PACKET_JOYSTICK;
		send(0xfe | numjoy);
		send(joy_state[numjoy]);
	}
}

/*--- Queue manager, host side ---*/

void IKBD::send(uint8 value)
{
	if (inbuffer == NULL)
		return;

	/* Add new byte to IKBD buffer */
	D(bug("ikbd: send(0x%02x) at position %d",value,inwrite));
	inbuffer[inwrite++] = value;

	inwrite &= (inbufferlen-1);
	
	if (inread == inwrite) {
		/* Queue full, need cleanup */
		switch (inbuffer[inread]) {
			/* Remove a packet */
			case 0xf6:
				inread+=8;
				break;
			case 0xf7:
				inread+=6;
				break;
			case 0xf8:
			case 0xf9:
			case 0xfa:
			case 0xfb:
				inread+=3;
				break;
			case 0xfc:
				inread+=7;
				break;
			case 0xfe:
			case 0xff:
				inread+=2;
				break;
			/* Remove a key press/release */
			default:
				inread++;
				break;
		}
		inread &= (inbufferlen-1);
	}

	sr |= (1<<ACIA_SR_RXFULL);

	ThrowInterrupt();
}

void IKBD::ThrowInterrupt(void)
{
	/* Check MFP IER */
	if ((getMFP()->handleRead(0xfffa09) & 0x40)==0)
		return;

	/* Check ACIA interrupt on reception */
	if ((cr & (1<<ACIA_CR_REC_INTER))==0)
		return;
		
	/* set Interrupt Request */
	sr |= (1<<ACIA_SR_INTERRUPT);

	/* signal ACIA interrupt */
	getMFP()->setGPIPbit(0x10, 0x10);	/* Force GPIP value transition so MFP layer generates interrupt */
	getMFP()->setGPIPbit(0x10, 0);
}

// don't remove this modeline with intended formatting for vim:ts=4:sw=4:
