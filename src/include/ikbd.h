/*
 *	IKBD 6301 emulation
 *
 *	Joy 2001
 *	Patrice Mandin
 */

#ifndef _IKBD_H
#define _IKBD_H

#include <SDL.h>
#include <SDL_thread.h>

#include "acia.h"

#define HW_IKBD 0xfffc00

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

class IKBD: public ACIA {
	private:
		/* IKBD keyboard state */

		/* IKBD mouse state */
		int mouse_enabled;
		int mouserel_enabled;
		int mousex, mousey, mouseb;
		
		/* IKBD joystick 0 state */
		int joy0_enabled;
		int joy0_state;
		
		/* IKBD joystick 1 state */
		int joy1_enabled;
		int joy1_state;

		/* Buffer when writing to IKBD */
		uae_u8 *outbuffer;	
		int outbufferlen;

		int	outwrite;

		/* Buffer when reading from IKBD */
		uae_u8 *inbuffer;
		int inbufferlen;
		int inwrite, inread;
		ikbd_packet_t intype;	/* Latest type of packet in buffer */

		// Read/Write lock
		SDL_mutex   *rwLock;	

		uint8 int2bcd(int a);
		void ThrowInterrupt(void);
		void MergeMousePacket(int *relx, int *rely, int buttons);

	public:
		IKBD(void);
		IKBD(int inlen, int outlen);	/* Params are length (2^x) of in/out buffers */
		~IKBD(void);
		void reset(void);

		uae_u8 ReadStatus(void);
		void WriteControl(uae_u8 value);
		uae_u8 ReadData(void);
		void WriteData(uae_u8 value);

		void SendKey(uae_u8 scancode);
		void SendMouseMotion(int relx, int rely, int buttons);
		void SendJoystickAxis(int numaxis, int value);
		void SendJoystickButton(int pressed);

		void send(uae_u8 value);
};

#endif /* _IKBD_H */
