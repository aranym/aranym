/*
 * $Header$
 */

/*
 *  main.cpp - Startup/shutdown code
 *
 *  Basilisk II (C) 1997-2000 Christian Bauer
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "sysdeps.h"
#include <SDL/SDL.h>
#include <signal.h>

#include "cpu_emulation.h"
#include "main.h"
#include "hardware.h"
#include "parameters.h"
#include "hostscreen.h"
#include "host.h"			// for the HostScreen
#include "araobjs.h"		// for the ExtFs
#include "aradata.h"		// for getAtariMouseXY
#include "md5.h"
#include "romdiff.h"

#define DEBUG 1
#include "debug.h"

#ifdef ENABLE_MON
#include "mon.h"

static uint32 mon_read_byte_b2(uint32 adr)
{
	return ReadAtariInt8(adr);
}

static void mon_write_byte_b2(uint32 adr, uint32 b)
{
	WriteAtariInt8(adr, b);
}
#endif

//For starting debugger
extern bool canGrabAgain;
void setactvdebug(int) {
	grabMouse(false);
	hideMouse(false);
	canGrabAgain = false;
	activate_debugger();
#ifdef NEWDEBUG
	signal(SIGINT, setactvdebug);
#endif
}

#define METADOS_DRV

// CPU and FPU type, addressing mode
int CPUType;
bool CPUIs68060;
int FPUType;

void init_fdc();				// fdc.cpp
void setVirtualTimer(void);		// basilisk_glue

SDL_TimerID my_timer_id;

extern int irqindebug;
extern long maxInnerCounter;	// hack to edit newcpu.cpp's counter refresh

static int keyboardTable[0x80] = {
/* 0-7 */ 0, SDLK_ESCAPE, SDLK_1, SDLK_2, SDLK_3, SDLK_4, SDLK_5,
		SDLK_6,
/* 8-f */ SDLK_7, SDLK_8, SDLK_9, SDLK_0, SDLK_EQUALS, SDLK_QUOTE,
		SDLK_BACKSPACE, SDLK_TAB,
/*10-17*/ SDLK_q, SDLK_w, SDLK_e, SDLK_r, SDLK_t, SDLK_y, SDLK_u,
		SDLK_i,
/*18-1f*/ SDLK_o, SDLK_p, SDLK_LEFTPAREN, SDLK_RIGHTPAREN, SDLK_RETURN,
		SDLK_LCTRL, SDLK_a, SDLK_s,
/*20-27*/ SDLK_d, SDLK_f, SDLK_g, SDLK_h, SDLK_j, SDLK_k, SDLK_l,
		SDLK_SEMICOLON,
/*28-2f*/ SDLK_QUOTE, SDLK_HASH, SDLK_LSHIFT, SDLK_BACKQUOTE, SDLK_z,
		SDLK_x, SDLK_c, SDLK_v,
/*30-37*/ SDLK_b, SDLK_n, SDLK_m, SDLK_COMMA, SDLK_PERIOD, SDLK_SLASH,
		SDLK_RSHIFT, 0,
/*38-3f*/ SDLK_LALT, SDLK_SPACE, SDLK_CAPSLOCK, SDLK_F1, SDLK_F2,
		SDLK_F3, SDLK_F4, SDLK_F5,
/*40-47*/ SDLK_F6, SDLK_F7, SDLK_F8, SDLK_F9, SDLK_F10, 0, 0,
		SDLK_HOME,
/*48-4f*/ SDLK_UP, 0, SDLK_KP_MINUS, SDLK_LEFT, 0, SDLK_RIGHT,
		SDLK_KP_PLUS, 0,
/*50-57*/ SDLK_DOWN, 0, SDLK_INSERT, SDLK_DELETE, 0, 0, 0, 0,
/*58-5f*/ 0, 0, 0, 0, 0, 0, 0, 0,
	/*60-67*/ SDLK_LESS, SDLK_F12, SDLK_F11, 0 /* NumLock */ ,
		SDLK_KP_MINUS, SDLK_KP_DIVIDE, SDLK_KP_MULTIPLY, SDLK_KP7,
/*68-6f*/ SDLK_KP8, SDLK_KP9, SDLK_KP4, SDLK_KP5, SDLK_KP6, SDLK_KP1,
		SDLK_KP2, SDLK_KP3,
/*70-77*/ SDLK_KP0, SDLK_KP_PERIOD, SDLK_KP_ENTER, 0, 0, 0, 0, 0,
/*78-7f*/ 0, 0, 0, 0, 0, 0, 0, 0
};

bool grabbedMouse = false;
bool hiddenMouse = false;
bool canGrabAgain = true;

void hideMouse(bool hide)
{
	if (hide) {
		SDL_ShowCursor(SDL_DISABLE);
		hiddenMouse = true;
	}
	else if (!hide) {
		SDL_ShowCursor(SDL_ENABLE);
		hiddenMouse = false;
	}
}

bool grabMouse(bool grab)
{
	int current = SDL_WM_GrabInput(SDL_GRAB_QUERY);
	if (grab && current != SDL_GRAB_ON) {
		SDL_WM_GrabInput(SDL_GRAB_ON);
		grabbedMouse = true;
		hideMouse(true);
	}
	else if (!grab && current != SDL_GRAB_OFF) {
		SDL_WM_GrabInput(SDL_GRAB_OFF);
		grabbedMouse = false;
		hideMouse(false);
	}
	return (current == SDL_GRAB_ON);
}


//  /* Print modifier info */
//  void PrintModifiers( SDLMod mod ){
//  printf( "Modifers: " );

//  /* If there are none then say so and return */
//  if( mod == KMOD_NONE ){
//      printf( "None\n" );
//      return;
//  }

//  /* Check for the presence of each SDLMod value */
//  /* This looks messy, but there really isn't    */
//  /* a clearer way.                              */
//  if( mod & KMOD_NUM ) printf( "NUMLOCK " );
//  if( mod & KMOD_CAPS ) printf( "CAPSLOCK " );
//  if( mod & KMOD_LCTRL ) printf( "LCTRL " );
//  if( mod & KMOD_RCTRL ) printf( "RCTRL " );
//  if( mod & KMOD_RSHIFT ) printf( "RSHIFT " );
//  if( mod & KMOD_LSHIFT ) printf( "LSHIFT " );
//  if( mod & KMOD_RALT ) printf( "RALT " );
//  if( mod & KMOD_LALT ) printf( "LALT " );
//  if( mod & KMOD_CTRL ) printf( "CTRL " );
//  if( mod & KMOD_SHIFT ) printf( "SHIFT " );
//  if( mod & KMOD_ALT ) printf( "ALT " );
//  printf( "\n" );
//  }

//  /* Print all information about a key event */
//  void PrintKeyInfo( SDL_KeyboardEvent *key ){
//  /* Is it a release or a press? */
//  if( key->type == SDL_KEYUP )
//      printf( "Release:- " );
//  else
//      printf( "Press:- " );

//  /* Print the hardware scancode first */
//  printf( "Scancode: 0x%02X", key->keysym.scancode );
//  /* Print the name of the key */
//  printf( ", Name: %s", SDL_GetKeyName( key->keysym.sym ) );
//  printf( ", Sym: %d", key->keysym.sym );
//  /* We want to print the unicode info, but we need to make */
//  /* sure its a press event first (remember, release events */
//  /* don't have unicode info                                */
//  if( key->type == SDL_KEYDOWN ){
//      /* If the Unicode value is less than 0x80 then the    */
//      /* unicode value can be used to get a printable       */
//      /* representation of the key, using (char)unicode.    */
//      printf(", Unicode: " );
//      if( key->keysym.unicode < 0x80 && key->keysym.unicode > 0 ){
//      printf( "%c (0x%04X)", (char)key->keysym.unicode,
//          key->keysym.unicode );
//      }
//      else{
//      printf( "? (0x%04X)", key->keysym.unicode );
//      }
//  }
//  printf( "\n" );
//  /* Print modifier info */
//  PrintModifiers( key->keysym.mod );
//  }


static int but = 0;

static void check_event(void)
{
	bool pendingQuit = false;
	static bool wasShiftPressed = false;		// for correct emulation of PageUp/Down
	static bool mouseOut = false;

	if (!fullscreen && mouseOut) {
		// host mouse moved but the Atari mouse did not => mouse is
		// probably at the Atari screen border. Ungrab it and warp the host mouse at
		// the same location so the mouse moves smoothly.
		grabMouse(false);	// release mouse
		hideMouse(false);	// show it
		SDL_WarpMouse(aradata.getAtariMouseX(), aradata.getAtariMouseY());
		D(bug("Mouse left our window"));
		mouseOut = false;
	}

	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		int type = event.type;
		if (type == SDL_KEYDOWN || type == SDL_KEYUP) {
			// D(bug(PrintKeyInfo((SDL_KeyboardEvent*)&event)));

			bool pressed = (type == SDL_KEYDOWN);
			int sym = event.key.keysym.sym;
			bool shifted = SDL_GetModState() & KMOD_SHIFT;
			bool alternated = SDL_GetModState() & KMOD_ALT;

			if (pressed) {
				// process special hotkeys
				if (sym == SDLK_PAUSE) {
					if (shifted)
						pendingQuit = true;
#ifdef DEBUGGER
					else if (start_debug && alternated) {
						
						// release mouse
						grabMouse(false);
						// show it
						hideMouse(false);
						// let user quit the window before it's grabbed again
						canGrabAgain = false;
						// activate debugger
						activate_debugger();
					}
#endif
				}

				if (alternated && sym == SDLK_PRINT) {
					hostScreen.makeSnapshot();
				}

				else if (sym == SDLK_PAGEUP) {
					ikbd.send(0x2a);	// press and hold LShift	// WARNING - shift might have been pressed already, in such case do not release it after user releases PAGEUP
					ikbd.send(0x48);	// press keyUp
				}
				else if (sym == SDLK_PAGEDOWN) {
					ikbd.send(0x2a);	// press and hold LShift
					ikbd.send(0x50);	// press keyDown
				}
			}
			else {
				if (sym == SDLK_PAGEUP) {
					ikbd.send(0xc8);	// release keyUp
					ikbd.send(0xaa);	// release LShift
				}
				else if (sym == SDLK_PAGEDOWN) {
					ikbd.send(0xd0);	// release keyDown
					ikbd.send(0xaa);	// release LShift
				}
			}
			// map right Control and Alternate keys to the left ones
			if (sym == SDLK_RCTRL)
				sym = SDLK_LCTRL;
			if (sym == SDLK_RALT)
				sym = SDLK_LALT;

			// send all pressed keys to IKBD
			for (int i = 0; i < 0x73; i++) {
				if (keyboardTable[i] == sym) {
					if (!pressed)
						i |= 0x80;
					ikbd.send(i);
					break;
				}
			}
		}
		else if (type == SDL_MOUSEBUTTONDOWN || type == SDL_MOUSEBUTTONUP
				 || type == SDL_MOUSEMOTION && grabbedMouse) {
			int xrel = 0;
			int yrel = 0;
			int lastbut = but;
			if (type == SDL_MOUSEBUTTONDOWN) {
				// eve.type/state/button
				if (event.button.button == SDL_BUTTON_RIGHT) {
					if (SDL_GetModState() & KMOD_CTRL) {
						// right mouse button + Control key = grab/ungrab mouse
						grabMouse(false);	// release mouse
						hideMouse(false);	// show it
						canGrabAgain = false;	// let user quit the window before it's grabbed again
					}
					else {
						if (grabbedMouse)
							but |= 1;
					}
				}
				else if (event.button.button == SDL_BUTTON_LEFT) {
					if (grabbedMouse)
						but |= 2;
				}
			}
			else if (type == SDL_MOUSEBUTTONUP) {
				if (event.button.button == SDL_BUTTON_RIGHT)
					but &= ~1;
				else if (event.button.button == SDL_BUTTON_LEFT)
					but &= ~2;
			}
			else if (type == SDL_MOUSEMOTION) {
				SDL_MouseMotionEvent eve = event.motion;
				xrel = eve.xrel;
				yrel = eve.yrel;

				if (xrel < -127 || xrel > 127)
					xrel = 0;
				if (yrel < -127 || yrel > 127)
					yrel = 0;
			}

			// send the mouse data packet
			if (xrel || yrel || lastbut != but) {
				ikbd.send(0xf8 | but);
				ikbd.send(xrel);
				ikbd.send(yrel);
			}

			if (! fullscreen) {
				// check whether user doesn't try to go out of window (top or left)
				if ((xrel < 0 && aradata.getAtariMouseX() == 0) ||
					(yrel < 0 && aradata.getAtariMouseY() == 0))
					mouseOut = true;

				// warning - hardcoded values of screen size - stupid
				if ((xrel > 0 && aradata.getAtariMouseX() >= (int32)hostScreen.getWidth() - 1) ||
					(yrel > 0 && aradata.getAtariMouseY() >= (int32)hostScreen.getHeight() - 1))
					mouseOut = true;
			}
		}
		else if (event.type == SDL_ACTIVEEVENT) {
			if (event.active.state == SDL_APPMOUSEFOCUS)
			{
				if (event.active.gain) {
					if ((SDL_GetAppState() & SDL_APPINPUTFOCUS)
						&& canGrabAgain) {
						D(bug("Mouse entered our window"));
						hideMouse(true);
						if (false)// are we able to sync TOS and host mice? 
						{
							// sync the position of ST mouse with the X mouse cursor (or vice-versa?)
						}
						else {
							// we got to grab the mouse completely, otherwise they'd be out of sync
							grabMouse(true);
						}
					}
				}
/*
				else {
					if (grabbedMouse) {
						D(bug("Mouse left our window"));
						canGrabAgain = true;
						hideMouse(false);
					}
				}
*/
			}
		}

		else if (event.type == SDL_QUIT) {
			pendingQuit = true;
		}
	}

	if (pendingQuit)
		QuitEmulator();
}

/*
 * the following function is called from the CPU emulation
 * each 5 milliseconds. The interrupt is "buffered".
 */
void invoke200HzInterrupt()
{
	static int VBL_counter = 0;
	static int refreshCounter = 0;

	mfp.IRQ(5);			// TimerC interrupt 

	if (++VBL_counter == 4) {	// divided by 4 => 50 Hz VBL
		VBL_counter = 0;

		check_event();		// process keyboard and mouse events
		TriggerVBL();		// generate VBL

		if (++refreshCounter == 2) {// divided by 2 again ==> 25 Hz screen update
			videl.renderScreen();
			refreshCounter = 0;
		}
	}
}

/*
 * Load, check and patch the TOS 4.04 ROM file
 */
bool InitTOSROM(void)
{
	// read ROM file
	D(bug("Reading ROM file..."));
	FILE *f = fopen(rom_path, "rb");
	if (f == NULL) {
		ErrorAlert("ROM file not found\n");
		return false;
	}

	int RealROMSize = 512 * 1024;
	if (fread(ROMBaseHost, 1, RealROMSize, f) != (size_t)RealROMSize) {
		ErrorAlert("ROM file reading error. Make sure the ROM image file size is 524288 bytes (512 kB).\n");
		fclose(f);
		return false;
	}
	fclose(f);

	// check if this is the correct 68040 aware TOS ROM version
	D(bug("Checking ROM version.."));
	unsigned char TOS68040[16] = {0xd6,0x4b,0x00,0x16,0x01,0xf6,0xc6,0xd7,0x47,0x51,0xde,0x63,0xbb,0x35,0xed,0xe3};
	unsigned char TOS404[16] = {0xe5,0xea,0x0f,0x21,0x6f,0xb4,0x46,0xf1,0xc4,0xa4,0xf4,0x76,0xbc,0x5f,0x03,0xd4};
	MD5 md5;
	if (! md5.compareSum(ROMBaseHost, RealROMSize, TOS68040)) {
		// this is not our ROM
		// check if it's at least the original TOS 4.04
		if (md5.compareSum(ROMBaseHost, RealROMSize, TOS404)) {
			// patch it for 68040 compatibility
			D(bug("Patching ROM for 68040 compatibility.."));
			int ptr, i=0;
			while((ptr=tosdiff[i].pointer) >= 0)
				ROMBaseHost[ptr] += tosdiff[i++].difference;
		}
		else {
			ErrorAlert("Wrong TOS version. You need the original TOS 4.04\n");
			return false;
		}
	}

	// patch cookies
	ROMBaseHost[0x00416] = bx_options.cookies._mch >> 24;
	ROMBaseHost[0x00417] = (bx_options.cookies._mch >> 16) & 0xff;
	ROMBaseHost[0x00418] = (bx_options.cookies._mch >> 8) & 0xff;
	ROMBaseHost[0x00419] = (bx_options.cookies._mch) & 0xff;

	// patch it for direct TC mode
	if (direct_truecolor) {
		// Patch TOS (enforce VIDEL VideoRAM at ARANYMVRAMSTART)
		D(bug("Patching TOS for direct VIDEL output..."));

		ROMBaseHost[35752] = 0x2e;
		ROMBaseHost[35753] = 0x3c;
		ROMBaseHost[35754] = ARANYMVRAMSTART >> 24;
		ROMBaseHost[35755] = (ARANYMVRAMSTART >> 16) & 0xff;
		ROMBaseHost[35756] = (ARANYMVRAMSTART >> 8) & 0xff;
		ROMBaseHost[35757] = (ARANYMVRAMSTART) & 0xff;
		ROMBaseHost[35758] = 0x60;
		ROMBaseHost[35759] = 6;
		ROMBaseHost[35760] = 0x4e;
		ROMBaseHost[35761] = 0x71;
	}

	return true;
}

/*
 * Initialize the Operating System - either the EmuTOS or TOS 4.04
 */
bool InitOS(void)
{
	// EmuTOS is the future. That's why I give it the precedence over the TOS ROM
	if (strlen(emutos_path) > 0) {
		// read EmuTOS file
		D(bug("Reading EmuTOS..."));
		FILE *f = fopen(emutos_path, "rb");
		if (f == NULL) {
			ErrorAlert("EmuTOS file not found\n");
			return false;
		}
		fread(ROMBaseHost, 1, ROMSize, f);
		fclose(f);
		return true;
	}
	else {
		return InitTOSROM();
	}
}


/*
 *  Initialize everything, returns false on error
 */

bool InitAll(void)
{
#ifndef NOT_MALLOC
	if (ROMBaseHost == NULL) {
		if ((RAMBaseHost = (uint8 *)malloc(RAMSize + ROMSize + FastRAMSize)) == NULL) {
			ErrorAlert("Not enough free memory.\n");
			return false;
		}
		ROMBaseHost = (uint8 *)(RAMBaseHost + ROMBase);
		FastRAMBaseHost = (uint8 *)(RAMBaseHost + FastRAMBase);
	}
#endif

	if (!InitMEM())
		return false;

	if (! InitOS())
		return false;

	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		ErrorAlert("SDL initialization failed.");
		return false;
	}
	// Be sure that the atexit function do not double any cleanup already done
	// thus I changed the SDL_Quit to ExitAll & removed the ExitAll from QuitEmulator
	atexit(ExitAll);

	CPUType = 4;
	FPUType = 1;

	maxInnerCounter = 10000;	// finetune this! Slower machines require lower number

	// Setting "SP & PC"
	WriteAtariInt32(0x00000000, ReadAtariInt32(ROMBase));
	WriteAtariInt32(0x00000004, ReadAtariInt32(ROMBase + 4));

	//  SDL_EnableUNICODE(1);

	// warp mouse to center of Atari screen and grab it
	if (! fullscreen)
		SDL_WarpMouse(640/2, 480/2);
	grabMouse(true);

	init_fdc();

#ifdef METADOS_DRV
	// install the drives
	extFS.init();
#endif							// METADOS_DRV

	// Init HW
	HWInit();

	// Init 680x0 emulation
	if (!Init680x0())
		return false;

#ifdef DEBUGGER
	if (start_debug) {
		D(bug("Activate debugger..."));
		activate_debugger();
	}
#endif

	// hide mouse unconditionally
	hideMouse(true);

	// timer init
	setVirtualTimer();
	
#if ENABLE_MON
	// Initialize mon
	mon_init();
	mon_read_byte = mon_read_byte_b2;
	mon_write_byte = mon_write_byte_b2;
#endif

	return true;
}


/*
 *  Deinitialize everything
 */

void ExitAll(void)
{
#if ENABLE_MON
	// Deinitialize mon
	mon_exit();
#endif

	SDL_VideoQuit();

	SDL_Quit();
}


/*
 * $Log$
 * Revision 1.32  2001/10/29 08:15:45  milan
 * some changes around debuggers
 *
 * Revision 1.31  2001/10/25 19:56:01  standa
 * The Log and Header CVS tags in the Log removed. Was recursing.
 *
 * Revision 1.30  2001/10/25 15:03:20  joy
 * move floppy stuff from main to fdc
 *
 * Revision 1.29  2001/10/25 12:25:28  joy
 * request the ROM to be 512 kB long. On the web there are just 256 kB long images...
 *
 * Revision 1.28  2001/10/23 21:26:15  standa
 * The hostScreen size is used to handle the mouseOut flag.
 *
 * Revision 1.27  2001/10/18 14:27:24  joy
 * TOS 4.04 is patched in runtime
 *
 * Revision 1.26  2001/10/18 13:46:34  joy
 * detect the ROM version. Knows both our 68040 friendly ABTOS and the original TOS 4.04.
 *
 * Revision 1.25  2001/10/16 19:38:44  milan
 * Integration of BasiliskII' cxmon, FastRAM in aranymrc etc.
 *
 * Revision 1.24  2001/10/12 07:56:14  standa
 * Pacal to C conversion ;( sorry for that.
 *
 * Revision 1.23  2001/10/09 19:25:19  milan
 * MemAlloc's rewriting
 *
 *
 */
