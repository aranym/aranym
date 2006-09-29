/*
 * input.cpp - handling of keyboard/mouse input
 *
 * Copyright (c) 2001-2006 Petr Stehlik of ARAnyM dev team (see AUTHORS)
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
#include "input.h"
#include "aradata.h"		// for getAtariMouseXY
#include "host.h"			// for the HostScreen
#include "main.h"			// for RestartAll()
#include "ata.h"
#ifdef SDL_GUI
#  include "sdlgui.h"
#endif

#include "nf_objs.h"
#include "nfvdi.h"

#define DEBUG 0
#include "debug.h"

#include <SDL.h>

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
#define KEYSYM_MACSCANCODE 1
#define KEYSYM_SCANCODE	3
#define UNDEFINED_OFFSET	-1

#ifdef OS_darwin
#define KEYBOARD_TRANSLATION	KEYSYM_MACSCANCODE
#else
#define KEYBOARD_TRANSLATION	KEYSYM_SCANCODE
#endif


// according to Gerhard Stoll Milan defined scancode 76 for AltGr key
#define RALT_ATARI_SCANCODE		(bx_options.ikbd.altgr ? 76 : 0x38)

/*********************************************************************
 * Mouse handling
 *********************************************************************/

static bool grabbedMouse = false;
static bool hiddenMouse = false;
static bool capslockState = false;

static void hideMouse(bool hide)
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

void InputInit()
{
	// warp mouse to center of Atari 320x200 screen and grab it
	if (! bx_options.video.fullscreen)
		SDL_WarpMouse(320/2, 200/2);
	grabMouse(true);
	// hide mouse unconditionally
	hideMouse(true);
	// capslockState (yes, 'false' is correct)
	capslockState = false;
}

void InputReset()
{
	// FIXME: add? InputInit();
	// FIXME: how??? capslockState (detect)
	capslockState = (SDL_GetModState() & KMOD_CAPS) != 0;
}

void InputExit()
{
	grabMouse(false);	// release mouse
	hideMouse(false);	// show it
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
	if (getARADATA()->isAtariMouseDriver()) {
		int x = getARADATA()->getAtariMouseX(); 
		int y = getARADATA()->getAtariMouseY();
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
#if KEYBOARD_TRANSLATION == KEYSYM_MACSCANCODE
static int keyboardTable[0x80] = {
/* 0-7 */ -1, 0x35, 0x12, 0x13, 0x14, 0x15, 0x17, 0x16,
/* 8-f */ 0x1A, 0x1C, 0x19, 0x1D, 0x1B, 0x18, 0x33, 0x30,
/*10-17*/ 0xC, 0xD, 0xE, 0xF, 0x11, 0x10, 0x20, 0x22,
/*18-1f*/ 0x1F, 0x23, 0x21, 0x1E, 0x24,	-1 /*LCTRL*/, 0x00, 0x01,
/*20-27*/ 0x02, 0x03, 0x05, 0x04, 0x26, 0x28, 0x25,	0x29,
/*28-2f*/ 0x27, 0x2A, -1 /*LSHIFT*/, 0x0A, 0x06, 0x07, 0x08, 0x09,
/*30-37*/ 0x0B, 0x2D, 0x2E, 0x2B, 0x2F, 0x2C, -1 /*RSHIFT*/, -1,
/*38-3f*/ -1 /*LALT*/, 0x31, -1 /*CAPSLOCK*/, 0x7A, 0x78, 0x63, 0x76, 0x60,
/*40-47*/ 0x61, 0x62, 0x64, 0x65, 0x6D, -1, -1,	0x73,
/*48-4f*/ 0x7E, -1, 0x4E, 0x7B, -1, 0x7C, 0x45, -1,
/*50-57*/ 0x7D, -1, 0x77, 0x75, -1, -1, -1, -1,
/*58-5f*/ -1, -1, -1, -1, -1, -1, -1, -1,
/*60-67*/ 0x32, 0x6F, 0x72, -1 /* NumLock */ , 0x51, 0x4B, 0x43, 0x59,
/*68-6f*/ 0x5B, 0x5C, 0x56, 0x57, 0x58, 0x53, 0x54, 0x55,
/*70-77*/ 0x52, 0x41, 0x4C, -1, -1, -1, -1, -1,
/*78-7f*/ -1, -1, -1, -1, -1, -1, -1, -1
};

static int keysymToAtari(SDL_keysym keysym)
{
 
//fprintf (stdout, "scancode: %x - sym: %x - char: %s\n", keysym.scancode, keysym.sym, SDL_GetKeyName (keysym.sym));

	int sym = keysym.scancode;

	switch (keysym.sym) {
	  case SDLK_LMETA:
	  case SDLK_RMETA:
	    return 0;
		break;
	  case SDLK_MODE: /* passthru */ /* Alt Gr key according to SDL docs */
	  case SDLK_RALT:
		return RALT_ATARI_SCANCODE;
	  case SDLK_LALT:
	    return 0x38;
		break;
	  case SDLK_LSHIFT:
	    return 0x2A;
		break;
	  case SDLK_RSHIFT:
	    return 0x36;
		break;
	  case SDLK_RCTRL:
	  case SDLK_LCTRL:
	    return 0x1D;
	  default:
	    break;
	}
	for (int i = 0; i < 0x73; i++) {
		if (keyboardTable[i] == sym) {
//		fprintf (stdout, "scancode mac:%x - scancode atari: %x\n", keysym.scancode, i);
			return i;
		}
	}

	return 0;	/* invalid scancode */
}
#endif /* KEYSYM_MACSCANCODE */

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
	/*60-67*/ SDLK_BACKQUOTE, SDLK_F12, SDLK_F11, 0 /* NumLock */ ,
		SDLK_KP_MINUS, SDLK_KP_DIVIDE, SDLK_KP_MULTIPLY, SDLK_KP7,
/*68-6f*/ SDLK_KP8, SDLK_KP9, SDLK_KP4, SDLK_KP5, SDLK_KP6, SDLK_KP1,
		SDLK_KP2, SDLK_KP3,
/*70-77*/ SDLK_KP0, SDLK_KP_PERIOD, SDLK_KP_ENTER, 0, 0, 0, 0, 0,
/*78-7f*/ 0, 0, 0, 0, 0, 0, 0, 0
};

static int keysymToAtari(SDL_keysym keysym)
{
 
	int sym = keysym.sym;
	if (sym == SDLK_RALT || sym == SDLK_MODE /* Alt Gr */)
		return RALT_ATARI_SCANCODE;
	// map right Control key to the left one
	if (sym == SDLK_RCTRL)
		sym = SDLK_LCTRL;
	for (int i = 0; i < 0x73; i++) {
		if (keyboardTable[i] == sym) {
			return i;
		}
	}

	return 0;	/* invalid scancode */
}
#endif /* KEYSYM_SYMTABLE */

#if KEYBOARD_TRANSLATION == KEYSYM_SCANCODE
// Heuristic analysis to find out the obscure scancode offset
static int findScanCodeOffset(SDL_keysym keysym)
{
	int scanPC = keysym.scancode;
	int offset = UNDEFINED_OFFSET;

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
	if (offset != UNDEFINED_OFFSET) {
		printf("Detected scancode offset = %d (key: '%s' with scancode $%02x)\n", offset, SDL_GetKeyName(keysym.sym), scanPC);
	}

	return offset;
}

static int keysymToAtari(SDL_keysym keysym)
{
	static int offset = UNDEFINED_OFFSET;

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

		// keys not found on original Atari keyboard
		case SDLK_RCTRL:	return 0x1d;	/* map right Control to Atari control */
		case SDLK_MODE: /* passthru */ /* Alt Gr key according to SDL docs */
		case SDLK_RALT:		return RALT_ATARI_SCANCODE;

		default:
		{
			// Process remaining keys: assume that it's PC101 keyboard
			// and that it is compatible with Atari ST keyboard (basically
			// same scancodes but on different platforms with different
			// base offset (framebuffer = 0, X11 = 8).
			// Try to detect the offset using a little bit of black magic.
			// If offset is known then simply pass the scancode.
			int scanPC = keysym.scancode;
			if (offset == UNDEFINED_OFFSET) {
				offset = findScanCodeOffset(keysym);
				if (offset == UNDEFINED_OFFSET) {
					panicbug("Unknown key: scancode = %d ($%02x), keycode = '%s' ($%02x)", scanPC, scanPC, SDL_GetKeyName(keysym.sym), keysym.sym);
					return 0;	// unknown scancode
				}
			}

			// offset is defined so pass the scancode directly
			return scanPC - offset;
		}
	}
}
#endif /* KEYSYM_SCANCODE */

/*********************************************************************
 * Input event checking
 *********************************************************************/
static bool pendingQuit = false;
static bool canGrabMouseAgain = true;
static int but = 0;
static bool mouseOut = false;

#ifdef SDL_GUI
extern bool isGuiAvailable;	// from main.cpp
static SDL_Thread *GUIthread = NULL;
static const int GUI_RETURN_INFO = (SDL_USEREVENT+1);

// running in a different thread
static int open_gui(void * /*ptr*/)
{
	hostScreen.openGUI();
	int status = GUImainDlg();

	// the status is sent to event checking thread by the USEREVENT+1 message
	SDL_Event ev;
	ev.type = GUI_RETURN_INFO;
	ev.user.code = status;	// STATUS_SHUTDOWN or STATUS_REBOOT
	ev.user.data1 = NULL;
	SDL_PeepEvents(&ev, 1, SDL_ADDEVENT, SDL_EVENTMASK(GUI_RETURN_INFO));

	hostScreen.closeGUI();
	return 0;
}

bool start_GUI_thread()
{
	if (isGuiAvailable && !hostScreen.isGUIopen()
#ifdef ENABLE_OPENGL
		&& !bx_options.opengl.enabled
#endif
	) {
		GUIthread = SDL_CreateThread(open_gui, NULL);
	}
	return (GUIthread != NULL);
}

void kill_GUI_thread()
{
	if (GUIthread != NULL) {
		SDL_KillThread(GUIthread);
		GUIthread = NULL;
	}
}
#endif /* SDL_GUI */

#define CHECK_HOTKEY(Hotkey) ((bx_options.hotkeys.Hotkey.sym == 0 || sym == bx_options.hotkeys.Hotkey.sym) && masked_mod == bx_options.hotkeys.Hotkey.mod)
static void process_keyboard_event(SDL_Event &event)
{
	SDL_keysym keysym = event.key.keysym;
	SDLKey sym = keysym.sym;
	int state = SDL_GetModState(); // keysym.mod does not deliver single mod key presses for some reason

#ifdef SDL_GUI
	if (hostScreen.isGUIopen()) {
		SDL_Event ev;
		ev.type = SDL_USEREVENT;	// map key down/up event to user event
		ev.user.code = event.type;
		ev.user.data1 = (void *)(uintptr)sym;
		ev.user.data2 = (void *)(uintptr)state;
		SDL_PeepEvents(&ev, 1, SDL_ADDEVENT, SDL_EVENTMASK(SDL_USEREVENT));
		return;	// don't pass the key events to emulation
	}
#endif /* SDL_GUI */

#if FLIGHT_RECORDER
	static bool flight_was_active = false;
	bool flight_should_be_active = (state & (KMOD_SHIFT)) == KMOD_SHIFT;
	if (flight_was_active != flight_should_be_active) {
		cpu_flight_recorder(flight_should_be_active);
		flight_was_active = flight_should_be_active;
		panicbug("Flight was %sactivated!", (flight_should_be_active ? "" : "DE"));
	}
#endif

	bool pressed = (event.type == SDL_KEYDOWN);
	bool shifted = state & KMOD_SHIFT;
	// bool controlled = state & KMOD_CTRL;
	// bool alternated = state & KMOD_ALT;
	bool capslocked = state & KMOD_CAPS;
	bool send2Atari = true;

	if (sym == SDLK_CAPSLOCK) send2Atari = false;
	if (capslockState != capslocked) {
		// SDL sends SDLK_CAPSLOCK keydown to turn it on and keyup to off.
		// TOS handles it just like any other keypress (down&up)
		//  ->	we handle this differently here
		getIKBD()->SendKey(0x3a);	// press CapsLock
		getIKBD()->SendKey(0xba);	// release CapsLock
		capslockState = capslocked;
	}

	// process special hotkeys
	if (pressed) {
		int masked_mod = state & HOTKEYS_MOD_MASK;

		if (CHECK_HOTKEY(quit)) {
			pendingQuit = true;
			send2Atari = false;
		}
		else if (CHECK_HOTKEY(reboot)) {
			RestartAll();	// force Cold Reboot
			send2Atari = false;
		}
#ifdef SDL_GUI
		else if (CHECK_HOTKEY(setup)) {
			/* release shifters (if any) */
			if ( bx_options.hotkeys.setup.mod & KMOD_LSHIFT )
				getIKBD()->SendKey(0x80 | 0x2a);
			if ( bx_options.hotkeys.setup.mod & KMOD_RSHIFT )
				getIKBD()->SendKey(0x80 | 0x36);
			if ( bx_options.hotkeys.setup.mod & KMOD_CTRL )
				getIKBD()->SendKey(0x80 | 0x1d);
			if ( bx_options.hotkeys.setup.mod & KMOD_LALT )
				getIKBD()->SendKey(0x80 | 0x38);
			if ( bx_options.hotkeys.setup.mod & KMOD_MODE|KMOD_RALT )
				getIKBD()->SendKey(0x80 | RALT_ATARI_SCANCODE);
			
			start_GUI_thread();
			send2Atari = false;
		}
#endif
#ifdef DEBUGGER
		else if (CHECK_HOTKEY(debug)) {
			if (bx_options.startup.debugger) {
				releaseTheMouse();
				canGrabMouseAgain = false;	// let it leave our window
				// activate debugger
				activate_debugger();
			}
			send2Atari = false;
		}
#endif
		else if (CHECK_HOTKEY(ungrab)) {
			if ( bx_options.video.fullscreen )
				hostScreen.toggleFullScreen();
			releaseTheMouse();
			canGrabMouseAgain = false;	// let it leave our window
			send2Atari = false;
		}
		else if (CHECK_HOTKEY(screenshot)) {
			hostScreen.makeSnapshot();
			send2Atari = false;
		}
		else if (CHECK_HOTKEY(fullscreen)) {
			hostScreen.toggleFullScreen();
			if (bx_options.video.fullscreen && !grabbedMouse)
				grabTheMouse();
			send2Atari = false;
		}
	}

	// map special keys to Atari range of scancodes
	if (sym == SDLK_PAGEUP) {
		if (pressed) {
			if (! shifted)
				getIKBD()->SendKey(0x2a);	// press and hold LShift
			getIKBD()->SendKey(0x48);	// press keyUp
		}
		else {
			getIKBD()->SendKey(0xc8);	// release keyUp
			if (! shifted)
				getIKBD()->SendKey(0xaa);	// release LShift
		}
		send2Atari = false;
	}
	else if (sym == SDLK_PAGEDOWN) {
		if (pressed) {
			if (! shifted)
				getIKBD()->SendKey(0x2a);	// press and hold LShift
			getIKBD()->SendKey(0x50);	// press keyDown
		}
		else {
			getIKBD()->SendKey(0xd0);	// release keyDown
			if (! shifted)
				getIKBD()->SendKey(0xaa);	// release LShift
		}
		send2Atari = false;
	}


	// send all pressed keys to IKBD
	if (send2Atari) {
		int scanAtari = keysymToAtari(keysym);
		D(bug("Host scancode = %d ($%02x), Atari scancode = %d ($%02x), keycode = '%s' ($%02x)", keysym.scancode, keysym.scancode, pressed ? scanAtari : scanAtari|0x80, pressed ? scanAtari : scanAtari|0x80, SDL_GetKeyName(sym), sym));
		if (scanAtari > 0) {
			if (!pressed)
				scanAtari |= 0x80;
			getIKBD()->SendKey(scanAtari);
		}
	}
}

static void process_mouse_event(SDL_Event event)
{
	static NF_Base* fvdi = NULL;
	bool fvdi_events = false;
	bool mouse_exit = false;

	if (!fvdi) {
		unsigned int i;
		for(i = 0; i < nf_objs_cnt; i++) {
			if (strcasecmp("fVDI", nf_objects[i]->name()) == 0) {
			    fvdi = nf_objects[i];
			    break;
			}
		}
	}

#ifdef SDL_GUI
	if (hostScreen.isGUIopen()) {
		int typ = event.type;
		if (typ == SDL_MOUSEBUTTONDOWN || typ == SDL_MOUSEBUTTONUP) {
			SDL_Event ev;
			ev.type = SDL_USEREVENT;	// map button down/up to user event
			ev.user.code = typ;
			ev.user.data1 = (void *)(uintptr)event.button.x;
			ev.user.data2 = (void *)(uintptr)event.button.y;
			SDL_PeepEvents(&ev, 1, SDL_ADDEVENT, SDL_EVENTMASK(SDL_USEREVENT));
		}
		return;	// don't pass the mouse events to emulation
	}
#endif /* SDL_GUI */

	int xrel = 0;
	int yrel = 0;
	int lastbut = but;
	if (event.type == SDL_MOUSEBUTTONDOWN) {
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
			if (fvdi != NULL && fvdi->dispatch(0xc00100ff) == 0)
				return;

			if (bx_options.ikbd.wheel_eiffel) {
				getIKBD()->SendKey(0xF6);
				getIKBD()->SendKey(0x05);
				getIKBD()->SendKey(0x00);
				getIKBD()->SendKey(0x00);
				getIKBD()->SendKey(0x00);
				getIKBD()->SendKey(0x00);
				getIKBD()->SendKey(0x00);
				getIKBD()->SendKey(0x59);
			}
			else {
				getIKBD()->SendKey(0x48);	// press keyUp
			}
			return;
		}
		else if (event.button.button == 5) {	/* mouse wheel Down */
			if (fvdi != NULL && fvdi->dispatch(0xc0010001) == 0)
				return;

			if (bx_options.ikbd.wheel_eiffel) {
				getIKBD()->SendKey(0xF6);
				getIKBD()->SendKey(0x05);
				getIKBD()->SendKey(0x00);
				getIKBD()->SendKey(0x00);
				getIKBD()->SendKey(0x00);
				getIKBD()->SendKey(0x00);
				getIKBD()->SendKey(0x00);
				getIKBD()->SendKey(0x5A);
			}
			else {
				getIKBD()->SendKey(0x50);	// press keyDown
			}
			return;
		}
	}
	else if (event.type == SDL_MOUSEBUTTONUP) {
		if (event.button.button == SDL_BUTTON_RIGHT)
			but &= ~1;
		else if (event.button.button == SDL_BUTTON_LEFT)
			but &= ~2;
		else if (event.button.button == 4) {	/* mouse wheel Up */
			if (!bx_options.ikbd.wheel_eiffel)
				getIKBD()->SendKey(0xc8);	// release keyUp
			return;
		}
		else if (event.button.button == 5) {	/* mouse wheel Down */
			if (!bx_options.ikbd.wheel_eiffel)
				getIKBD()->SendKey(0xd0);	// release keyDown
			return;
		}
	}
	else if (event.type == SDL_MOUSEMOTION) {
		SDL_MouseMotionEvent eve = event.motion;
		xrel = eve.xrel;
		yrel = eve.yrel;

		if (fvdi != NULL) {
			if (fvdi->dispatch(0x80008000 | (eve.x << 16) | (eve.y)) == 0)
				fvdi_events = true;
		}
		// Can't use the method below to get out of the window
		// if the events are reported directly, since it only
		// works with hidden mouse pointer.
		// So, define top left corner as exit point.
		mouse_exit = (eve.x == 0) && (eve.y == 0);
	}

	if (lastbut != but) {
		if (fvdi != NULL && (fvdi->dispatch(0xb0000000 | but) == 0))
			fvdi_events = true;
	}

	if (fvdi_events) {
		if (!bx_options.video.fullscreen && mouse_exit)
			mouseOut = true;
		return;
	}

	// send the mouse data packet
	if (xrel || yrel || lastbut != but) {
		if (xrel < -250 || xrel > 250 || yrel < -250 || yrel > 250) {
			bug("Reseting weird mouse packet: %d, %d, %d", xrel, yrel, but);
			xrel = yrel = 0;	// reset the values otherwise ikbd gets crazy
		}
		getIKBD()->SendMouseMotion(xrel, yrel, but);
	}

	if (! bx_options.video.fullscreen && getARADATA()->isAtariMouseDriver()) {
		// check whether user doesn't try to go out of window (top or left)
		if ((xrel < 0 && getARADATA()->getAtariMouseX() == 0) ||
			(yrel < 0 && getARADATA()->getAtariMouseY() == 0))
			mouseOut = true;

		// same check but for bottom and right side of our window
		if ((xrel > 0 && getARADATA()->getAtariMouseX() >= (int32)hostScreen.getWidth() - 1) ||
			(yrel > 0 && getARADATA()->getAtariMouseY() >= (int32)hostScreen.getHeight() - 1))
			mouseOut = true;
	}
}

static void process_active_event(SDL_Event event)
{
	// if we have input focus
	if (SDL_GetAppState() & SDL_APPINPUTFOCUS) {

		// if it's mouse focus event
		if (event.active.state == SDL_APPMOUSEFOCUS) {

			// if we can grab the mouse automatically
			// and if the Atari mouse driver works
			if (bx_options.autoMouseGrab && getARADATA()->isAtariMouseDriver()) {

				// if the mouse has just left our window
				if (!event.active.gain) {
					// allow grabbing it when it will be returning
					canGrabMouseAgain = true;
				}
				// if the mouse is entering our window
				else {
					// if grabbing the mouse is allowed
					if (canGrabMouseAgain) {
						// then grab it
						grabTheMouse();
					}
				}
			}
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

/*--- Joystick event ---*/

SDL_Joystick *sdl_joystick;

static void process_joystick_event(SDL_Event event)
{
	switch(event.type) {
		case SDL_JOYAXISMOTION:
			getIKBD()->SendJoystickAxis(1, event.jaxis.axis, event.jaxis.value);
			break;		
		case SDL_JOYHATMOTION:
			getIKBD()->SendJoystickHat(1, event.jhat.value);
			break;		
		case SDL_JOYBUTTONDOWN:
		case SDL_JOYBUTTONUP:
			getIKBD()->SendJoystickButton(1, event.jbutton.state==SDL_PRESSED);
			break;		
	}
}

///////
// main function for checking keyboard, mouse and joystick events
// called from main.cpp every 20 ms
void check_event()
{
	if (!bx_options.video.fullscreen && mouseOut) {
		// host mouse moved but the Atari mouse did not => mouse is
		// probably at the Atari screen border. Ungrab it and warp the host mouse at
		// the same location so the mouse moves smoothly.
		releaseTheMouse();
		mouseOut = false;
	}

	SDL_Event event;
	int eventmask = SDL_EVENTMASK(SDL_KEYDOWN)
					| SDL_EVENTMASK(SDL_KEYUP)
					| SDL_EVENTMASK(SDL_MOUSEBUTTONDOWN)
					| SDL_EVENTMASK(SDL_MOUSEBUTTONUP)
					| SDL_EVENTMASK(SDL_MOUSEMOTION)
					| SDL_EVENTMASK(SDL_JOYAXISMOTION)
					| SDL_EVENTMASK(SDL_JOYHATMOTION)
					| SDL_EVENTMASK(SDL_JOYBUTTONDOWN)
					| SDL_EVENTMASK(SDL_JOYBUTTONUP)
					| SDL_EVENTMASK(SDL_ACTIVEEVENT)
#ifdef SDL_GUI
					| SDL_EVENTMASK(GUI_RETURN_INFO)
#endif
					| SDL_EVENTMASK(SDL_QUIT);

	SDL_PumpEvents();
	while (SDL_PeepEvents(&event, 1, SDL_GETEVENT, eventmask)) {
		int type = event.type;

		if (type == SDL_KEYDOWN || type == SDL_KEYUP) {
			process_keyboard_event(event);
		}
		else if (type == SDL_MOUSEBUTTONDOWN || type == SDL_MOUSEBUTTONUP
				 || (type == SDL_MOUSEMOTION && grabbedMouse)) {
			process_mouse_event(event);
		}
		else if (type == SDL_JOYAXISMOTION || type == SDL_JOYBUTTONDOWN
			|| type == SDL_JOYBUTTONUP || type == SDL_JOYHATMOTION) {
			process_joystick_event(event);
		}
		else if (type == SDL_ACTIVEEVENT) {
			process_active_event(event);
		}
#ifdef SDL_GUI
		else if (type == GUI_RETURN_INFO) {
			int status = event.user.code;
			if (status == STATUS_SHUTDOWN)
				pendingQuit = true;
			else if (status == STATUS_REBOOT)
				RestartAll();
		}
#endif /* SDL_GUI */
		else if (type == SDL_QUIT) {
			pendingQuit = true;
		}
	}

	if (pendingQuit) {
		Quit680x0();	// forces CPU to quit the loop
	}
}

/*
vim:ts=4:sw=4:
*/
