/*
 *  input.cpp - keyboard/mouse input code
 *
 *  ARAnyM (C) 2001-2002 Petr Stehlik of ARAnyM Team
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
#include "input.h"
#include "aradata.h"		// for getAtariMouseXY
#include "host.h"			// for the HostScreen

#define DEBUG 0
#include "debug.h"

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
#define KEYSYM_FRAMEBUFFER	1
#define KEYSYM_X11	2
#define KEYSYM_SCANCODE	3

#define KEYBOARD_TRANSLATION	KEYSYM_SCANCODE

/*********************************************************************
 * Mouse handling
 *********************************************************************/

static bool grabbedMouse = false;
static bool hiddenMouse = false;

void InputInit()
{
	// warp mouse to center of Atari 640x480 screen and grab it
	if (! bx_options.video.fullscreen)
		SDL_WarpMouse(640/2, 480/2);
	grabMouse(true);
	// hide mouse unconditionally
	hideMouse(true);
}

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

/*********************************************************************
 * Keyboard handling
 *********************************************************************/

#if KEYBOARD_TRANSLATION == KEYSYM_SYMTABLE
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
#endif /* KEYSYM_SYMTABLE */

#if KEYBOARD_TRANSLATION == KEYSYM_FRAMEBUFFER
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
#endif /* KEYSYM_FRAMEBUFFER */

#if KEYBOARD_TRANSLATION == KEYSYM_X11
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

#if KEYBOARD_TRANSLATION == KEYSYM_SCANCODE
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

		// Map Right Alt/Alt Gr/Control to the Atari keys
		case SDLK_RCTRL:	return 0x1d;	/* Control */
		case SDLK_MODE:
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

/*********************************************************************
 * Input event checking
 *********************************************************************/

static int but = 0;
void check_event()
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
