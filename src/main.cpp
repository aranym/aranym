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
#include <SDL.h>

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
#include "parameters.h"

#define DEBUG 0
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
#endif	/* ENABLE_MON */

//For starting debugger
void setactvdebug(int) {
	grabMouse(false);
	activate_debugger();
#ifdef NEWDEBUG
	signal(SIGINT, setactvdebug);
#endif
}

void init_fdc();

#ifdef EXTFS_SUPPORT
#define METADOS_DRV
#endif

// CPU and FPU type, addressing mode
int CPUType;
bool CPUIs68060;
int FPUType;

// Timer stuff
static long lastTicks;
#ifdef USE_TIMERS
SDL_TimerID my_timer_id;
#endif

////////////////////////////////////////////////
// Input stuff (keyboard and mouse) begins here

static bool grabbedMouse = false;
static bool hiddenMouse = false;

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
	if (bx_options.startup.grabMouseAllowed && grab && current != SDL_GRAB_ON) {
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


void grabTheMouse()
{
#if DEBUG
	int x,y;
	SDL_GetMouseState(&x, &y);
	D(bug("Mouse entered our window at [%d,%d]", x, y));
#endif
	hideMouse(true);
	if (false) {	// are we able to sync TOS and host mice? 
		// sync the position of ST mouse with the X mouse cursor (or vice-versa?)
	}
	else {
		// we got to grab the mouse completely, otherwise they'd be out of sync
		grabMouse(true);
	}
}

void releaseTheMouse()
{
	grabMouse(false);	// release mouse
	hideMouse(false);	// show it
	if (aradata.isAtariMouseDriver()) {
		int x = aradata.getAtariMouseX(); 
		int y = aradata.getAtariMouseY();
		SDL_WarpMouse(x, y);
		D(bug("Mouse left our window at [%d,%d]", x, y));
	}
	else {
		D(bug("Mouse left our window"));
	}
}

/*
 * Four different types of keyboard translation:
 *
 * SYMTABLE = table of symbols (the original method) - never worked properly
 * FRAMEBUFFER = host to Atari scancode table - tested under framebuffer
 * X11 = scancode table but with different host scancode offset (crappy SDL)
 * SCANCODE = scancode table with heuristic detection of scancode offset
 *
 */
#define KEYSYM_SYMTABLE	0
#define KEYSYM_FRAMEBUFFER	0
#define KEYSYM_X11	0
#define KEYSYM_SCANCODE	1

#if KEYSYM_SYMTABLE
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

int keysymToAtari(SDL_keysym keysym)
{
	int sym = keysym.sym;
	// map right Control and Alternate keys to the left ones
	if (sym == SDLK_RCTRL)
		sym = SDLK_LCTRL;
	if (sym == SDLK_RALT)
		sym = SDLK_LALT;
	for (int i = 0; i < 0x73; i++) {
		if (keyboardTable[i] == sym) {
			return i;
		}
	}

	return 0;	/* invalid scancode */
}
#endif	/* KEYSYM_SYMTABLE */

#if KEYSYM_FRAMEBUFFER
int keysymToAtari(SDL_keysym keysym)
{
	int scanPC = keysym.scancode;
	// map right Control and Alternate keys to the left ones
	if (scanPC == 0x61)		/* Right Control */
		scanPC = 0x1d;
	else if (scanPC == 0x64)	/* Right Alternate */
		scanPC = 0x38;

	/*
	 * surprisingly, PC101 is identical to Atari keyboard,
	 * at least in the range <ESC, F10> (and NumPad '*'
	 * is the single exception that confirms the rule)
	 */
	if (scanPC >= 1 /* ESC */ && scanPC <= 0x44 /* F10 */) {
		if (scanPC == 0x37)	/* NumPad '*' */
			return 0x66;	/* strict layout: 0x65 */
		return scanPC;
	}
	switch(scanPC) {
		case 0x47:	return 0x67;	/* NumPad 7 */
		case 0x48:	return 0x68;	/* NumPad 8 */
		case 0x49:	return 0x69;	/* NumPad 9 */
		case 0x4a:	return 0x4a;	/* NumPad - */
		case 0x4b:	return 0x6a;	/* NumPad 4 */
		case 0x4c:	return 0x6b;	/* NumPad 5 */
		case 0x4d:	return 0x6c;	/* NumPad 6 */
		case 0x4e:	return 0x4e;	/* NumPad + */
		case 0x4f:	return 0x6d;	/* NumPad 1 */
		case 0x50:	return 0x6e;	/* NumPad 2 */
		case 0x51:	return 0x6f;	/* NumPad 3 */
		case 0x52:	return 0x70;	/* NumPad 0 */
		case 0x53:	return 0x71;	/* NumPad . */
		case 0x62:	return 0x65;	/* NumPad / */	/* strict layout: 0x64 */

		case 0x57:	return 0x62;	/* F11 => Help */
		case 0x58:	return 0x61;	/* F12 => Undo */
		case 0x66:	return 0x47;	/* Home */
		case 0x6b:	return 0x60;	/* End => "<>" on German Atari kbd */
		case 0x67:	return 0x48;	/* Arrow Up */
		case 0x6c:	return 0x4b;	/* Arrow Left */
		case 0x69:	return 0x4d;	/* Arrow Right */
		case 0x6a:	return 0x50;	/* Arrow Down */
		case 0x6e:	return 0x52;	/* Insert */
		case 0x6f:	return 0x53;	/* Delete */
		case 0x60:	return 0x72;	/* NumPad Enter */

		default:	return 0;		/* invalid scancode */
	}
}
#endif	/* KEYSYM_FRAMEBUFFER */

#if KEYSYM_X11
int keysymToAtari(SDL_keysym keysym)
{
	int scanPC = keysym.scancode;
	// map right Control and Alternate keys to the left ones
	if (scanPC == 0x6d)		/* Right Control */
		scanPC = 0x25;
	else if (scanPC == 0x71)	/* Right Alternate */
		scanPC = 0x40;

	/*
	 * unfortunately SDL on X11 doesn't return physical scancodes.
	 * For the basic range <ESC,F10> there's an offset = 8.
	 * The rest of keys have rather random offsets.
	 */
	if (scanPC >= 9 /* ESC */ && scanPC <= 0x4c /* F10 */) {
		if (scanPC == 0x3f)	/* NumPad '*' */
			return 0x66;	/* strict layout: 0x65 */
		return scanPC - 8;		/* remove the offset */
	}
	switch(scanPC) {
		case 0x4f:	return 0x67;	/* NumPad 7 */
		case 0x50:	return 0x68;	/* NumPad 8 */
		case 0x51:	return 0x69;	/* NumPad 9 */
		case 0x52:	return 0x4a;	/* NumPad - */
		case 0x53:	return 0x6a;	/* NumPad 4 */
		case 0x54:	return 0x6b;	/* NumPad 5 */
		case 0x55:	return 0x6c;	/* NumPad 6 */
		case 0x56:	return 0x4e;	/* NumPad + */
		case 0x57:	return 0x6d;	/* NumPad 1 */
		case 0x58:	return 0x6e;	/* NumPad 2 */
		case 0x59:	return 0x6f;	/* NumPad 3 */
		case 0x5a:	return 0x70;	/* NumPad 0 */
		case 0x5b:	return 0x71;	/* NumPad . */
		case 0x70:	return 0x65;	/* NumPad / */	/* strict layout: 0x64 */

		case 0x5f:	return 0x62;	/* F11 => Help */
		case 0x60:	return 0x61;	/* F12 => Undo */
		case 0x61:	return 0x47;	/* Home */
		case 0x67:	return 0x60;	/* End => "<>" on German Atari kbd */
		case 0x62:	return 0x48;	/* Arrow Up */
		case 0x64:	return 0x4b;	/* Arrow Left */
		case 0x66:	return 0x4d;	/* Arrow Right */
		case 0x68:	return 0x50;	/* Arrow Down */
		case 0x6a:	return 0x52;	/* Insert */
		case 0x6b:	return 0x53;	/* Delete */
		case 0x6c:	return 0x72;	/* NumPad Enter */

		default:	return 0;		/* invalid scancode */
	}
}
#endif /* KEYSYM_X11 */

#if KEYSYM_SCANCODE
int keysymToAtari(SDL_keysym keysym)
{
	static int offset = -1;		// uninitialized scancode offset

	switch(keysym.sym) {
		// Numeric Pad
		case SDLK_KP_DIVIDE:	return 0x65;	/* Numpad / */
		case SDLK_KP_MULTIPLY:	return 0x66;	/* NumPad * */
		case SDLK_KP7:	return 0x67;	/* NumPad 7 */
		case SDLK_KP8:	return 0x68;	/* NumPad 8 */
		case SDLK_KP9:	return 0x69;	/* NumPad 9 */
		case SDLK_KP_MINUS:	return 0x4a;	/* NumPad - */
		case SDLK_KP4:	return 0x6a;	/* NumPad 4 */
		case SDLK_KP5:	return 0x6b;	/* NumPad 5 */
		case SDLK_KP6:	return 0x6c;	/* NumPad 6 */
		case SDLK_KP_PLUS:	return 0x4e;	/* NumPad + */
		case SDLK_KP1:	return 0x6d;	/* NumPad 1 */
		case SDLK_KP2:	return 0x6e;	/* NumPad 2 */
		case SDLK_KP3:	return 0x6f;	/* NumPad 3 */
		case SDLK_KP0:	return 0x70;	/* NumPad 0 */
		case SDLK_KP_PERIOD:	return 0x71;	/* NumPad . */
		case SDLK_KP_ENTER:	return 0x72;	/* NumPad Enter */

		// Special Keys
		case SDLK_F11:	return 0x62;	/* F11 => Help */
		case SDLK_F12:	return 0x61;	/* F12 => Undo */
		case SDLK_HOME:	return 0x47;	/* Home */
		case SDLK_END:	return 0x60;	/* End => "<>" on German Atari kbd */
		case SDLK_UP:	return 0x48;	/* Arrow Up */
		case SDLK_LEFT:	return 0x4b;	/* Arrow Left */
		case SDLK_RIGHT:	return 0x4d;	/* Arrow Right */
		case SDLK_DOWN:	return 0x50;	/* Arrow Down */
		case SDLK_INSERT:	return 0x52;	/* Insert */
		case SDLK_DELETE:	return 0x53;	/* Delete */

		// Map Right Alt/Control to the Atari keys
		case SDLK_RCTRL:	return 0x1d;	/* Control */
		case SDLK_RALT:		return 0x38;	/* Alternate */

		default:
		{
			// Process remaining keys: assume that it's PC101 keyboard
			// and that it is compatible with Atari ST keyboard (basically
			// same scancodes but on different platforms with different
			// base offset (framebuffer = 0, X11 = 8).
			// Try to detect the offset using a little bit of black magic.
			// If offset is known then simply pass the scancode.
			int scanPC = keysym.scancode;
			if (offset == -1) {
				// Heuristic analysis to find out the obscure scancode offset
				switch(keysym.sym) {
					case SDLK_ESCAPE:	offset = scanPC - 0x01; break;
					case SDLK_1:	offset = scanPC - 0x02; break;
					case SDLK_2:	offset = scanPC - 0x03; break;
					case SDLK_3:	offset = scanPC - 0x04; break;
					case SDLK_4:	offset = scanPC - 0x05; break;
					case SDLK_5:	offset = scanPC - 0x06; break;
					case SDLK_6:	offset = scanPC - 0x07; break;
					case SDLK_7:	offset = scanPC - 0x08; break;
					case SDLK_8:	offset = scanPC - 0x09; break;
					case SDLK_9:	offset = scanPC - 0x0a; break;
					case SDLK_0:	offset = scanPC - 0x0b; break;
					case SDLK_BACKSPACE:	offset = scanPC - 0x0e; break;
					case SDLK_TAB:	offset = scanPC - 0x0f; break;
					case SDLK_RETURN:	offset = scanPC - 0x1c; break;
					case SDLK_SPACE:	offset = scanPC - 0x39; break;
					case SDLK_q:	offset = scanPC - 0x10; break;
					case SDLK_w:	offset = scanPC - 0x11; break;
					case SDLK_e:	offset = scanPC - 0x12; break;
					case SDLK_r:	offset = scanPC - 0x13; break;
					case SDLK_t:	offset = scanPC - 0x14; break;
					case SDLK_y:	offset = scanPC - 0x15; break;
					case SDLK_u:	offset = scanPC - 0x16; break;
					case SDLK_i:	offset = scanPC - 0x17; break;
					case SDLK_o:	offset = scanPC - 0x18; break;
					case SDLK_p:	offset = scanPC - 0x19; break;
					case SDLK_a:	offset = scanPC - 0x1e; break;
					case SDLK_s:	offset = scanPC - 0x1f; break;
					case SDLK_d:	offset = scanPC - 0x20; break;
					case SDLK_f:	offset = scanPC - 0x21; break;
					case SDLK_g:	offset = scanPC - 0x22; break;
					case SDLK_h:	offset = scanPC - 0x23; break;
					case SDLK_j:	offset = scanPC - 0x24; break;
					case SDLK_k:	offset = scanPC - 0x25; break;
					case SDLK_l:	offset = scanPC - 0x26; break;
					case SDLK_z:	offset = scanPC - 0x2c; break;
					case SDLK_x:	offset = scanPC - 0x2d; break;
					case SDLK_c:	offset = scanPC - 0x2e; break;
					case SDLK_v:	offset = scanPC - 0x2f; break;
					case SDLK_b:	offset = scanPC - 0x30; break;
					case SDLK_n:	offset = scanPC - 0x31; break;
					case SDLK_m:	offset = scanPC - 0x32; break;
					case SDLK_CAPSLOCK:	offset = scanPC - 0x3a; break;
					case SDLK_LSHIFT:	offset = scanPC - 0x2a; break;
					case SDLK_LCTRL:	offset = scanPC - 0x1d; break;
					case SDLK_LALT:	offset = scanPC - 0x38; break;
					case SDLK_F1:	offset = scanPC - 0x3b; break;
					case SDLK_F2:	offset = scanPC - 0x3c; break;
					case SDLK_F3:	offset = scanPC - 0x3d; break;
					case SDLK_F4:	offset = scanPC - 0x3e; break;
					case SDLK_F5:	offset = scanPC - 0x3f; break;
					case SDLK_F6:	offset = scanPC - 0x40; break;
					case SDLK_F7:	offset = scanPC - 0x41; break;
					case SDLK_F8:	offset = scanPC - 0x42; break;
					case SDLK_F9:	offset = scanPC - 0x43; break;
					case SDLK_F10:	offset = scanPC - 0x44; break;
					default:	break;
				}
				if (offset != -1) {
					printf("Detected scancode offset = %d (key: '%s' with scancode $%02x)\n", offset, SDL_GetKeyName(keysym.sym), scanPC);
				}
			}

			if (offset >= 0) {
				// offset is defined so pass the scancode directly
				return scanPC - offset;
			}
			else {
				fprintf(stderr, "Unknown key: scancode = %d ($%02x), keycode = '%s' ($%02x)\n", scanPC, scanPC, SDL_GetKeyName(keysym.sym), keysym.sym);
				return 0;	// unknown scancode
			}
		}
	}
}
#endif /* KEYSYM_SCANCODE */

static int but = 0;
static void check_event(void)
{
	bool pendingQuit = false;
	static bool mouseOut = false;
	static bool canGrabMouseAgain = true;


	if (!bx_options.video.fullscreen && mouseOut) {
		// host mouse moved but the Atari mouse did not => mouse is
		// probably at the Atari screen border. Ungrab it and warp the host mouse at
		// the same location so the mouse moves smoothly.
		releaseTheMouse();
		mouseOut = false;
	}

	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		int type = event.type;
		if (type == SDL_KEYDOWN || type == SDL_KEYUP) {
			bool pressed = (type == SDL_KEYDOWN);
			SDL_keysym keysym = event.key.keysym;
			SDLKey sym = keysym.sym;
			int state = keysym.mod;	// SDL_GetModState();
			bool shifted = state & KMOD_SHIFT;
			bool controlled = state & KMOD_CTRL;
			bool alternated = state & KMOD_ALT;
			bool send2Atari = true;

			// process special hotkeys
			if (pressed) {
				switch(sym) {
					case SDLK_ESCAPE:
						if (controlled && alternated) {
							releaseTheMouse();
							canGrabMouseAgain = false;	// let it leave our window
							send2Atari = false;
						}
						break;

					case SDLK_PAUSE:
						if (shifted) {
							pendingQuit = true;
							send2Atari = false;
						}
#ifdef DEBUGGER
						else if (bx_options.startup.debugger && alternated) {
							releaseTheMouse();
							canGrabMouseAgain = false;	// let it leave our window
							// activate debugger
							activate_debugger();
							send2Atari = false;
						}
#endif
						break;

					case SDLK_PRINT:
						if (alternated) {
							hostScreen.makeSnapshot();
							send2Atari = false;
						}
						break;

					default: break;
				}
			}

			// map special keys to Atari range of scancodes
			if (sym == SDLK_PAGEUP) {
				if (pressed) {
					if (! shifted)
						ikbd.send(0x2a);	// press and hold LShift
					ikbd.send(0x48);	// press keyUp
				}
				else {
					ikbd.send(0xc8);	// release keyUp
					if (! shifted)
						ikbd.send(0xaa);	// release LShift
				}
				send2Atari = false;
			}
			else if (sym == SDLK_PAGEDOWN) {
				if (pressed) {
					if (! shifted)
						ikbd.send(0x2a);	// press and hold LShift
					ikbd.send(0x50);	// press keyDown
				}
				else {
					ikbd.send(0xd0);	// release keyDown
					if (! shifted)
						ikbd.send(0xaa);	// release LShift
				}
				send2Atari = false;
			}

			// send all pressed keys to IKBD
			if (send2Atari) {
				int scanAtari = keysymToAtari(keysym);
				D(bug("Host scancode = %d ($%02x), Atari scancode = %d ($%02x), keycode = '%s' ($%02x)", keysym.scancode, keysym.scancode, scanAtari, scanAtari, SDL_GetKeyName(sym), sym));
				if (scanAtari > 0) {
					if (!pressed)
						scanAtari |= 0x80;
					ikbd.send(scanAtari);
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
					if (grabbedMouse)
							but |= 1;
				}
				else if (event.button.button == SDL_BUTTON_LEFT) {
					if (grabbedMouse)
						but |= 2;
					else
						grabTheMouse();
				}
				else if (event.button.button == 4) {	/* mouse wheel Up */
					ikbd.send(0x48);	// press keyUp
					return;
				}
				else if (event.button.button == 5) {	/* mouse wheel Down */
					ikbd.send(0x50);	// press keyDown
					return;
				}
			}
			else if (type == SDL_MOUSEBUTTONUP) {
				if (event.button.button == SDL_BUTTON_RIGHT)
					but &= ~1;
				else if (event.button.button == SDL_BUTTON_LEFT)
					but &= ~2;
				else if (event.button.button == 4) {	/* mouse wheel Up */
					ikbd.send(0xc8);	// release keyUp
					return;
				}
				else if (event.button.button == 5) {	/* mouse wheel Down */
					ikbd.send(0xd0);	// release keyDown
					return;
				}
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

			if (! bx_options.video.fullscreen && aradata.isAtariMouseDriver()) {
				// check whether user doesn't try to go out of window (top or left)
				if ((xrel < 0 && aradata.getAtariMouseX() == 0) ||
					(yrel < 0 && aradata.getAtariMouseY() == 0))
					mouseOut = true;

				// same check but for bottom and right side of our window
				if ((xrel > 0 && aradata.getAtariMouseX() >= (int32)hostScreen.getWidth() - 1) ||
					(yrel > 0 && aradata.getAtariMouseY() >= (int32)hostScreen.getHeight() - 1))
					mouseOut = true;
			}
		}
		else if (event.type == SDL_ACTIVEEVENT) {
			// if the mouse left our window we will let it be grabbed next time it comes back
			if (event.active.state == SDL_APPMOUSEFOCUS && !event.active.gain)
				canGrabMouseAgain = true;
			else {
				// if the mouse is comming back after it left our window and
				// if we have input focus and
				// if we can grab the mouse automatically and
				// if the Atari mouse driver works then let's grab it!
				if (SDL_GetAppState() & (SDL_APPMOUSEFOCUS | SDL_APPINPUTFOCUS)) {
					if (bx_options.autoMouseGrab && canGrabMouseAgain && aradata.isAtariMouseDriver())
						grabTheMouse();
				}
			}

#if DEBUG
			if (event.active.state == SDL_APPMOUSEFOCUS) {
				D(bug("We %s mouse focus", event.active.gain ? "got" : "lost"));
			}
			else if (event.active.state == SDL_APPINPUTFOCUS) {
				D(bug("We %s input focus", event.active.gain ? "got" : "lost"));
			}
#endif
		}

		else if (event.type == SDL_QUIT) {
			pendingQuit = true;
		}
	}

	if (pendingQuit)
		QuitEmulator();
}

// Input stuff (keyboard and mouse) ends here
//////////////////////////////////////////////

/*
 * the following function is called from the CPU emulation anytime
 * or it is called from the timer interrupt * approx. each 10 milliseconds.
 */
void invoke200HzInterrupt()
{
#define VBL_IN_TIMERC	4	/* VBL happens once in 4 TimerC 200 Hz interrupts ==> 50 Hz VBL */
#define VIDEL_REFRESH	bx_options.video.refresh	/* VIDEL screen is refreshed once in 2 VBL interrupts ==> 25 Hz */

	static int VBL_counter = 0;
	static int refreshCounter = 0;

	/* syncing to 200 Hz */
	long newTicks = SDL_GetTicks();
	int count = (newTicks - lastTicks) / 5;	// miliseconds / 5 = 200 Hz
	if (count == 0)
		return;
	if (!debugging || irqindebug)
		mfp.IRQ(5, count);
	lastTicks += (count * 5);

	VBL_counter += count;
	if (VBL_counter >= VBL_IN_TIMERC) {	// divided by 4 => 50 Hz VBL
		VBL_counter -= VBL_IN_TIMERC;

#ifdef USE_TIMERS
		// Thread safety patch
		hostScreen.lock();
#endif

		check_event();		// process keyboard and mouse events
		TriggerVBL();		// generate VBL

		if (++refreshCounter == VIDEL_REFRESH) {// divided by 2 again ==> 25 Hz screen update
			videl.renderScreen();
			refreshCounter = 0;
		}

#ifdef USE_TIMERS
		// Thread safety patch
		hostScreen.unlock();
#endif
	}
}

#ifdef USE_TIMERS
/*
 * my_callback_function() is called every 10 miliseconds (~ 100 Hz)
 */
Uint32 my_callback_function(Uint32 interval, void *param)
{
	invoke200HzInterrupt();
	return 10;					// come back in 10 milliseconds
}
#endif /* USE_TIMERS */

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
	unsigned char TOS68040WinX[16] = {0xd6,0x4b,0x00,0x16,0x01,0xf6,0xc6,0xd7,0x47,0x51,0xde,0x63,0xbb,0x35,0xed,0xe3};

	unsigned char TOS68040[16] = {0x6b,0x9f,0x43,0x5e,0xdc,0x46,0xdc,0x26,0x6f,0x2c,0x87,0xc2,0x6c,0x63,0xf9,0xd8};
	unsigned char TOS404[16] = {0xe5,0xea,0x0f,0x21,0x6f,0xb4,0x46,0xf1,0xc4,0xa4,0xf4,0x76,0xbc,0x5f,0x03,0xd4};
	MD5 md5;
	unsigned char loadedTOS[16];
	md5.computeSum(ROMBaseHost, RealROMSize, loadedTOS);
	if (memcmp(loadedTOS, TOS68040, 16) == 0) {
		fprintf(stderr, "68040 friendly TOS 4.04 found\n");
	}
	else if (memcmp(loadedTOS, TOS68040WinX, 16) == 0) {
		fprintf(stderr, "68040 friendly WinX enhanced TOS 4.04 found\n");
	}
	else if (memcmp(loadedTOS, TOS404, 16) == 0) {
		fprintf(stderr, "Original TOS 4.04 found\n");

		// patch it for 68040 compatibility
		D(bug("Patching ROM for 68040 compatibility.."));
		int ptr, i=0;
		while((ptr=tosdiff[i].pointer) >= 0)
			ROMBaseHost[ptr] += tosdiff[i++].difference;

#if 0
		// optional saving of patched TOS ROM
		FILE *f = fopen("TOS68040", "wb");
		if (f != NULL) {
			fwrite(ROMBaseHost, RealROMSize, 1, f);
			fclose(f);
		}
#endif
	}
	else {
		ErrorAlert("Wrong TOS version. You need the original TOS 4.04\n");
		return false;
	}

	// patch cookies
	ROMBaseHost[0x00416] = bx_options.tos.cookie_mch >> 24;
	ROMBaseHost[0x00417] = (bx_options.tos.cookie_mch >> 16) & 0xff;
	ROMBaseHost[0x00418] = (bx_options.tos.cookie_mch >> 8) & 0xff;
	ROMBaseHost[0x00419] = (bx_options.tos.cookie_mch) & 0xff;

	// patch TOS 4.04 to show FastRAM memory test
	if (FastRAMSize > 0) {
		uint32 ramtop = (FastRAMBase + FastRAMSize);
		ROMBaseHost[0x001CC] = 0x4E;
		ROMBaseHost[0x001CD] = 0xF9;	// JMP <abs.addr>
		ROMBaseHost[0x001CE] = 0x00;
		ROMBaseHost[0x001CF] = 0xE7;
		ROMBaseHost[0x001D0] = 0xFF;
		ROMBaseHost[0x001D1] = 0x00;	// abs.addr = $E7FF00
		ROMBaseHost[0x7FF00] = 0x21;
		ROMBaseHost[0x7FF01] = 0xFC;	// MOVE.L #imm, abs.addr.w
		ROMBaseHost[0x7FF02] = ramtop >> 24;
		ROMBaseHost[0x7FF03] = ramtop >> 16;
		ROMBaseHost[0x7FF04] = ramtop >> 8;
		ROMBaseHost[0x7FF05] = ramtop;
		ROMBaseHost[0x7FF06] = 0x05;
		ROMBaseHost[0x7FF07] = 0xA4;	// abs.addr.w = $5A4
		ROMBaseHost[0x7FF08] = 0x4E;
		ROMBaseHost[0x7FF09] = 0xF9;	// JMP <abs.addr>
		ROMBaseHost[0x7FF0A] = 0x00;
		ROMBaseHost[0x7FF0B] = 0xE0;
		ROMBaseHost[0x7FF0C] = 0x01;
		ROMBaseHost[0x7FF0D] = 0xD2;	// abs.addr = $E001D2
	}

	// Xconout patch
	if (bx_options.tos.console_redirect) {
		ROMBaseHost[0x8d44] = ROMBaseHost[0x8d50] = 0x71;
		ROMBaseHost[0x8d45] = ROMBaseHost[0x8d51] = 0x2a;
		ROMBaseHost[0x8d46] = ROMBaseHost[0x8d52] = 0x4e;
		ROMBaseHost[0x8d47] = ROMBaseHost[0x8d53] = 0x75;
	}

#ifdef DIRECT_TRUECOLOR
	// patch it for direct TC mode
	if (bx_options.video.direct_truecolor) {
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
#endif

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
		D(bug("Reading EmuTOS: '%s'", emutos_path));
		FILE *f = fopen(emutos_path, "rb");
		if (f == NULL) {
			ErrorAlert("EmuTOS not found\n");
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

	int sdlInitParams = SDL_INIT_VIDEO;
#ifdef USE_TIMERS
	sdlInitParams |= SDL_INIT_TIMER;
#endif
	if (SDL_Init(sdlInitParams) != 0) {
		ErrorAlert("SDL initialization failed.");
		return false;
	}

	// check video output device and if it's a framebuffer
	// then enforce full screen
	char driverName[32];
	SDL_VideoDriverName( driverName, sizeof(driverName)-1 );
	D(bug("Video driver name: %s", driverName));
	if ( strstr( driverName, "fb" ) )		// fullscreen on framebuffer
		bx_options.video.fullscreen = true;

	// Be sure that the atexit function do not double any cleanup already done
	// thus I changed the SDL_Quit to ExitAll & removed the ExitAll from QuitEmulator
	atexit(ExitAll);

	CPUType = 4;
	FPUType = 1;

	// Setting "SP & PC"
	for (int i = 0; i < 8; i++) RAMBaseHost[i] = ROMBaseHost[i];

	init_fdc();

#ifdef METADOS_DRV
	// install the drives
	extFS.init();
#endif							// METADOS_DRV

	// Init HW
	HWInit();

	// warp mouse to center of Atari 640x480 screen and grab it
	if (! bx_options.video.fullscreen)
		SDL_WarpMouse(640/2, 480/2);
	grabMouse(true);

	// Init 680x0 emulation
	if (!Init680x0())
		return false;

#ifdef DEBUGGER
	if (bx_options.startup.debugger) {
		D(bug("Activate debugger..."));
		activate_debugger();
	}
#endif

	// hide mouse unconditionally
	hideMouse(true);

	// timer init
	lastTicks = SDL_GetTicks();
#ifdef USE_TIMERS
	my_timer_id = SDL_AddTimer(10, my_callback_function, NULL);
	printf("Using timers\n");
#endif

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
	// Exit Time Manager
#if USE_TIMERS
	SDL_RemoveTimer(my_timer_id);
#endif

#if ENABLE_MON
	// Deinitialize mon
	mon_exit();
#endif

	SDL_VideoQuit();

	SDL_Quit();
}


/*
 * $Log$
 * Revision 1.56  2002/01/03 23:10:41  joy
 * redirect xconout to host console
 *
 * Revision 1.55  2001/12/29 17:11:40  joy
 * cleaned up
 *
 * Revision 1.54  2001/12/27 22:31:49  joy
 * TOS patch to enable FastRAM visual check
 *
 * Revision 1.53  2001/12/22 18:13:24  joy
 * most video related parameters moved to bx_options.video struct.
 * --refresh <x> added
 *
 * Revision 1.52  2001/12/17 09:56:56  joy
 * VBL is at precise 50 Hz now. And screen refresh at 25 Hz.
 *
 * Revision 1.51  2001/12/17 08:33:00  standa
 * Thread synchronization added. The check_event and fvdidriver actions are
 * synchronized each to other.
 *
 * Revision 1.50  2001/12/17 00:26:13  joy
 * SDL Timer is back! Use --enable-sdltimer to emulate TimerC (200 Hz interrupt) using a separate timer thread.
 *
 * Revision 1.49  2001/12/11 21:08:25  standa
 * The SDL_WarpMouse() was causing Floating Point error on fbcon.
 * It was moved behind the HWInit() to be called after SDL_SetVideoMode (correct
 * place to call it).
 * The fullscreen detection was added for "fb" drivers to not to cause mouse cursor
 * redraw problems when using Clocky(TM) and touching the screen edges.
 *
 * Revision 1.48  2001/12/07 17:29:13  milan
 * e00000 -> 0 back in InitAll
 *
 * Revision 1.47  2001/12/06 01:06:54  joy
 * disable mouse grabbing at start
 *
 * Revision 1.46  2001/12/06 00:25:22  milan
 * ndebug corrected, sync
 *
 * Revision 1.45  2001/12/02 01:31:20  joy
 * keyboard conversion rewritten again, this time with heuristic analysis to detect the scancode2scancode conversion offset of SDL automatically.
 *
 * Revision 1.44  2001/11/28 22:52:17  joy
 * keyboard conversion based on scancodes now.
 * page up/down takes care of previously held Shift
 *
 * Revision 1.43  2001/11/21 13:29:51  milan
 * cleanning & portability
 *
 * Revision 1.42  2001/11/20 23:29:26  milan
 * Extfs now not needed for ARAnyM's compilation
 *
 * Revision 1.41  2001/11/19 17:50:28  joy
 * second MD5 check removed
 *
 * Revision 1.40  2001/11/11 22:03:55  joy
 * direct truecolor is optional (compile time configurable)
 *
 * Revision 1.39  2001/11/07 21:18:25  milan
 * SDL_CFLAGS in CXXFLAGS now.
 *
 * Revision 1.38  2001/11/06 20:36:54  milan
 * MMU's corrections
 *
 * Revision 1.37  2001/11/06 13:35:51  joy
 * now recognizes TOS from CVS 2001-05-02 and also the new one (WinX free).
 *
 * Revision 1.36  2001/11/06 11:45:14  joy
 * updated MD5 checksum for new romdiff.cpp
 * optional saving of patched TOS ROM
 *
 * Revision 1.35  2001/11/01 16:03:54  joy
 * mouse behavior fixed:
 * 1) you can release mouse grab by pressing Alt+Ctrl+Esc (VMware compatible)
 * 2) you can grab mouse by click the left mouse button (VMware compatible)
 * 3) if the Atari mouse driver is active, you can release mouse by moving the mouse off the window and by moving back you grab it again. This can be disabled by AutoGrabMouse=false.
 *
 * Revision 1.34  2001/10/29 20:00:05  standa
 * The button 4 and 5 mapped to KeyUp and KeyDown.
 *
 * Revision 1.33  2001/10/29 11:16:07  joy
 * emutos gets a special care
 *
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
