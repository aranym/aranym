/*
 * input.cpp - handling of keyboard/mouse input
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
#include "input.h"
#include "aradata.h"		// for getAtariMouseXY
#include "host.h"			// for the HostScreen
#include "main.h"			// for RestartAll()
#include "ata.h"
#ifdef SDL_GUI
# include "gui-sdl/sdlgui.h"
# include "gui-sdl/dialog.h"
#endif

#ifdef NFVDI_SUPPORT
# include "nf_objs.h"
# include "nfvdi.h"
#endif

#define DEBUG 0
#include "debug.h"

#include "SDL_compat.h"
#include "clipbrd.h"

/* Joysticks */

enum {
	ARANYM_JOY_IKBD0=0,
	ARANYM_JOY_IKBD1,
	ARANYM_JOY_JOYPADA,
	ARANYM_JOY_JOYPADB
};

#define OPEN_JOYSTICK(host_number, array_number) \
	if (host_number>=0) {	\
		sdl_joysticks[array_number] = \
			SDL_JoystickOpen(host_number);	\
		if (!sdl_joysticks[array_number]) {	\
			panicbug("Could not open joystick %d",	\
				host_number);	\
		}	\
	}

SDL_Joystick *sdl_joysticks[4]={
	NULL, NULL, NULL, NULL
};



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
#define RALT_ATARI_SCANCODE		(bx_options.ikbd.altgr ? 0x4c : 0x38)

/*********************************************************************
 * Mouse handling
 *********************************************************************/

static SDL_bool grabbedMouse = SDL_FALSE;
static SDL_bool hiddenMouse = SDL_FALSE;
SDL_Cursor *aranym_cursor = NULL;
SDL_Cursor *empty_cursor = NULL;

static const char *arrow[] = {
  /* width height num_colors chars_per_pixel */
  "    32    32        3            1",
  /* colors */
  "X c #000000",
  ". c #ffffff",
  "  c None",
  /* pixels */
  "X                               ",
  "XX                              ",
  "X.X                             ",
  "X..X                            ",
  "X...X                           ",
  "X....X                          ",
  "X.....X                         ",
  "X......X                        ",
  "X.......X                       ",
  "X........X                      ",
  "X.....XXXXX                     ",
  "X..X..X                         ",
  "X.X X..X                        ",
  "XX  X..X                        ",
  "X    X..X                       ",
  "     X..X                       ",
  "      X..X                      ",
  "      X..X                      ",
  "       XX                       ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "0,0"
};

static SDL_Cursor *init_system_cursor(const char *image[])
{
  int i, row, col;
  Uint8 data[4*32];
  Uint8 mask[4*32];
  int hot_x, hot_y;

  i = -1;
  for ( row=0; row<32; ++row ) {
    for ( col=0; col<32; ++col ) {
      if ( col % 8 ) {
        data[i] <<= 1;
        mask[i] <<= 1;
      } else {
        ++i;
        data[i] = mask[i] = 0;
      }
      switch (image[4+row][col]) {
        case 'X':
          data[i] |= 0x01;
          mask[i] |= 0x01;
          break;
        case '.':
          mask[i] |= 0x01;
          break;
        case ' ':
          break;
      }
    }
  }
  sscanf(image[4+row], "%d,%d", &hot_x, &hot_y);
  return SDL_CreateCursor(data, mask, 32, 32, hot_x, hot_y);
}


static SDL_Cursor *init_empty_cursor()
{
	Uint8 data[4*16];
	Uint8 mask[4*16];
	
	memset(data, 0, sizeof(data));
	memset(mask, 0, sizeof(mask));
	return SDL_CreateCursor(data, mask, 16, 16, 0, 0);
}


#if SDL_VERSION_ATLEAST(2, 0, 0)
static int SDLCALL event_filter(void * /* userdata */, SDL_Event *event)
#else
static int SDLCALL event_filter(const SDL_Event *event)
#endif
{
#if defined(NFCLIPBRD_SUPPORT)
	if (filter_aclip(event) == 0) return 0;
#endif
	UNUSED(event);
	return 1;
}

void InputInit()
{
	aranym_cursor = init_system_cursor(arrow);
	empty_cursor = init_empty_cursor();
	SDL_SetCursor(aranym_cursor);
	if (bx_options.startup.grabMouseAllowed) {
		// warp mouse to center of Atari 320x200 screen and grab it
		grabMouse(SDL_TRUE);
		// hide mouse unconditionally
		hideMouse(SDL_TRUE);
		if (! bx_options.video.fullscreen)
		{
			host->video->WarpMouse(0, 0);
		}
	}

#if defined (OS_darwin) && !SDL_VERSION_ATLEAST(2, 0, 0)
	// Make sure ALT+click is not interpreted as SDL_MIDDLE_BUTTON
	SDL_putenv((char*)"SDL_HAS3BUTTONMOUSE=1");
#endif
	
	/* Open joysticks */
	OPEN_JOYSTICK(bx_options.joysticks.ikbd0, ARANYM_JOY_IKBD0);
	OPEN_JOYSTICK(bx_options.joysticks.ikbd1, ARANYM_JOY_IKBD1);
	OPEN_JOYSTICK(bx_options.joysticks.joypada, ARANYM_JOY_JOYPADA);
	OPEN_JOYSTICK(bx_options.joysticks.joypadb, ARANYM_JOY_JOYPADB);

	/* Enable the special window hook events */
	SDL_EventState(SDL_SYSWMEVENT, SDL_ENABLE);
#if SDL_VERSION_ATLEAST(2, 0, 0)
	SDL_SetEventFilter(event_filter, NULL);
#else
	SDL_SetEventFilter(event_filter);
#endif
}

void InputReset()
{
	// FIXME: add? InputInit();
	// FIXME: how??? capslockState (detect)
	host->video->CapslockState((SDL_GetModState() & KMOD_CAPS) != 0);
}

void InputExit()
{
	grabMouse(SDL_FALSE);	// release mouse
	hideMouse(SDL_FALSE);	// show it

	/* Close joysticks */
	int i;
	for (i=0; i<4; i++) {
		if (sdl_joysticks[i]) {
			SDL_JoystickClose(sdl_joysticks[i]);
		}
	}

	SDL_FreeCursor(aranym_cursor);
	aranym_cursor = NULL;
	SDL_FreeCursor(empty_cursor);
	empty_cursor = NULL;
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
/*60-67*/ 0x32, 0x6F, 0x67, -1 /* NumLock */ , 0x51, 0x4B, 0x43, 0x59,
/*68-6f*/ 0x5B, 0x5C, 0x56, 0x57, 0x58, 0x53, 0x54, 0x55,
/*70-77*/ 0x52, 0x41, 0x4C, -1, -1, -1, -1, -1,
/*78-7f*/ -1, -1, -1, -1, -1, -1, -1, -1
};

static int keysymToAtari(SDL_Keysym keysym)
{
    // panicbug("scancode: %x - sym: %x - char: %s", keysym.scancode, keysym.sym, SDL_GetKeyName (keysym.sym));

	int sym = keysym.scancode;

	switch (keysym.sym) {
	  case SDLK_LGUI:
	  case SDLK_RGUI:
		#if MAP_META_TO_CONTROL
		  return 0x1D;
		#else		
		  return 0;
		#endif
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
		#if MAP_CONTROL_TO_CONTROL
		  return 0x1D;
		#else		
		  return 0;
		#endif
	  default:
	    break;
	}
	for (int i = 0; i < 0x73; i++) {
		if (keyboardTable[i] == sym) {
			//panicbug ("scancode mac:%x - scancode atari: %x", keysym.scancode, i);
			return i;
		}
	}
	if (keysym.scancode != 0)
	bug("keycode: %d (0x%x), scancode %d (0x%x), keysym '%s' is not mapped",
		keysym.sym, keysym.sym,
		keysym.scancode, keysym.scancode,
		SDL_GetKeyName(keysym.sym));
	
	return 0;	/* invalid scancode */
}
#endif /* KEYSYM_MACSCANCODE */

#if KEYBOARD_TRANSLATION == KEYSYM_SYMTABLE
static SDL_Keycode keyboardTable[0x80] = {
/* 0-7 */ 0,              SDLK_ESCAPE,     SDLK_1,         SDLK_2,            SDLK_3,             SDLK_4,         SDLK_5,           SDLK_6,
/* 8-f */ SDLK_7,         SDLK_8,          SDLK_9,         SDLK_0,            SDLK_EQUALS,        SDLK_QUOTE,     SDLK_BACKSPACE,   SDLK_TAB,
/*10-17*/ SDLK_q,         SDLK_w,          SDLK_e,         SDLK_r,            SDLK_t,             SDLK_y,         SDLK_u,           SDLK_i,
/*18-1f*/ SDLK_o,         SDLK_p,          SDLK_LEFTPAREN, SDLK_RIGHTPAREN,   SDLK_RETURN,        SDLK_LCTRL,     SDLK_a,           SDLK_s,
/*20-27*/ SDLK_d,         SDLK_f,          SDLK_g,         SDLK_h,            SDLK_j,             SDLK_k,         SDLK_l,           SDLK_SEMICOLON,
/*28-2f*/ SDLK_QUOTE,     SDLK_HASH,       SDLK_LSHIFT,    SDLK_BACKQUOTE,    SDLK_z,             SDLK_x,         SDLK_c,           SDLK_v,
/*30-37*/ SDLK_b,         SDLK_n,          SDLK_m,         SDLK_COMMA,        SDLK_PERIOD,        SDLK_SLASH,     SDLK_RSHIFT,      SDLK_PRINTSCREEN,
/*38-3f*/ SDLK_LALT,      SDLK_SPACE,      SDLK_CAPSLOCK,  SDLK_F1,           SDLK_F2,            SDLK_F3,        SDLK_F4,          SDLK_F5,
/*40-47*/ SDLK_F6,        SDLK_F7,         SDLK_F8,        SDLK_F9,           SDLK_F10,           0,              0,                SDLK_HOME,
/*48-4f*/ SDLK_UP,        SDLK_PAGEUP,     SDLK_KP_MINUS,  SDLK_LEFT,         0,                  SDLK_RIGHT,     SDLK_KP_PLUS,     SDLK_END,
/*50-57*/ SDLK_DOWN,      SDLK_PAGEDOWN,   SDLK_INSERT,    SDLK_DELETE,       0,                  0,              0,                0,
/*58-5f*/ 0,              0,               0,              0,                 0,                  0,              0,                0,
/*60-67*/ SDLK_LESS,      SDLK_F12,        SDLK_F11,       SDLK_KP_LEFTPAREN, SDLK_KP_RIGHTPAREN, SDLK_KP_DIVIDE, SDLK_KP_MULTIPLY, SDLK_KP_7,
/*68-6f*/ SDLK_KP_8,      SDLK_KP_9,       SDLK_KP_4,      SDLK_KP_5,         SDLK_KP_6,          SDLK_KP_1,      SDLK_KP_2,        SDLK_KP_3,
/*70-77*/ SDLK_KP_0,      SDLK_KP_PERIOD,  SDLK_KP_ENTER,  0,                 0,                  0,              0,                0,
/*78-7f*/ 0,              0,               0,              0,                 0,                  0,              0,                0
};

static int keysymToAtari(SDL_Keysym keysym)
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

#if !SDL_VERSION_ATLEAST(2, 0, 0)
static int findScanCodeOffset(SDL_keysym keysym)
{
	unsigned int scanPC = keysym.scancode;
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
		case SDLK_RSHIFT:	offset = scanPC - 0x36; break;
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
		D(bug("Detected scancode offset = %d (key: '%s' with scancode $%02x)", offset, SDL_GetKeyName(keysym.sym), scanPC));
	}

	return offset;
}
#endif


static int keysymToAtari(SDL_Keysym keysym)
{
#if SDL_VERSION_ATLEAST(2, 0, 0)
	switch( (unsigned int) keysym.scancode) {
		case SDL_SCANCODE_ESCAPE: return 0x01;
		case SDL_SCANCODE_1:	return 0x02;
		case SDL_SCANCODE_2:	return 0x03;
		case SDL_SCANCODE_3:	return 0x04;
		case SDL_SCANCODE_4:	return 0x05;
		case SDL_SCANCODE_5:	return 0x06;
		case SDL_SCANCODE_6:	return 0x07;
		case SDL_SCANCODE_7:	return 0x08;
		case SDL_SCANCODE_8:	return 0x09;
		case SDL_SCANCODE_9:	return 0x0a;
		case SDL_SCANCODE_0:	return 0x0b;
		case SDL_SCANCODE_MINUS: return 0x0c;
		case SDL_SCANCODE_EQUALS: return 0x0d;
		case SDL_SCANCODE_BACKSPACE:	return 0x0e;
		case SDL_SCANCODE_TAB:	return 0x0f;
		case SDL_SCANCODE_Q:	return 0x10;
		case SDL_SCANCODE_W:	return 0x11;
		case SDL_SCANCODE_E:	return 0x12;
		case SDL_SCANCODE_R:	return 0x13;
		case SDL_SCANCODE_T:	return 0x14;
		case SDL_SCANCODE_Y:	return 0x15;
		case SDL_SCANCODE_U:	return 0x16;
		case SDL_SCANCODE_I:	return 0x17;
		case SDL_SCANCODE_O:	return 0x18;
		case SDL_SCANCODE_P:	return 0x19;
		case SDL_SCANCODE_LEFTBRACKET: return 0x1a;
		case SDL_SCANCODE_RIGHTBRACKET: return 0x1b;
		case SDL_SCANCODE_RETURN:	return 0x1c;
		case SDL_SCANCODE_LCTRL:	return 0x1d;
		case SDL_SCANCODE_A:	return 0x1e;
		case SDL_SCANCODE_S:	return 0x1f;
		case SDL_SCANCODE_D:	return 0x20;
		case SDL_SCANCODE_F:	return 0x21;
		case SDL_SCANCODE_G:	return 0x22;
		case SDL_SCANCODE_H:	return 0x23;
		case SDL_SCANCODE_J:	return 0x24;
		case SDL_SCANCODE_K:	return 0x25;
		case SDL_SCANCODE_L:	return 0x26;
		case SDL_SCANCODE_SEMICOLON: return 0x27;
		case SDL_SCANCODE_APOSTROPHE: return 0x28;
		case SDL_SCANCODE_GRAVE: return 0x29;
		case SDL_SCANCODE_LSHIFT:	return 0x2a;
		case SDL_SCANCODE_BACKSLASH: return 0x2b;
		case SDL_SCANCODE_Z:	return 0x2c;
		case SDL_SCANCODE_X:	return 0x2d;
		case SDL_SCANCODE_C:	return 0x2e;
		case SDL_SCANCODE_V:	return 0x2f;
		case SDL_SCANCODE_B:	return 0x30;
		case SDL_SCANCODE_N:	return 0x31;
		case SDL_SCANCODE_M:	return 0x32;
		case SDL_SCANCODE_COMMA: return 0x33;
		case SDL_SCANCODE_PERIOD: return 0x34;
		case SDL_SCANCODE_SLASH: return 0x35;
		case SDL_SCANCODE_RSHIFT:	return 0x36;
		case SDL_SCANCODE_PRINTSCREEN: return 0x37;
		case SDL_SCANCODE_LALT:	return 0x38;
		case SDL_SCANCODE_SPACE:	return 0x39;
		case SDL_SCANCODE_CAPSLOCK:	return 0x3a;
		case SDL_SCANCODE_F1:	return 0x3b;
		case SDL_SCANCODE_F2:	return 0x3c;
		case SDL_SCANCODE_F3:	return 0x3d;
		case SDL_SCANCODE_F4:	return 0x3e;
		case SDL_SCANCODE_F5:	return 0x3f;
		case SDL_SCANCODE_F6:	return 0x40;
		case SDL_SCANCODE_F7:	return 0x41;
		case SDL_SCANCODE_F8:	return 0x42;
		case SDL_SCANCODE_F9:	return 0x43;
		case SDL_SCANCODE_F10:	return 0x44;

		case SDL_SCANCODE_NONUSBACKSLASH: return 0x60;
		case SDL_SCANCODE_KP_LEFTPAREN: return 0x63;
		case SDL_SCANCODE_KP_RIGHTPAREN: return 0x64;
		
		case SDL_SCANCODE_SCROLLLOCK: return 0x00;
		case SDL_SCANCODE_PAUSE: return 0x00;
	}
#endif
	switch((unsigned int) keysym.sym) {
		// Numeric Pad
		case SDLK_KP_DIVIDE:	return 0x65;	/* Numpad / */
		case SDLK_KP_MULTIPLY:	return 0x66;	/* NumPad * */
		case SDLK_KP_7:	return 0x67;	/* NumPad 7 */
		case SDLK_KP_8:	return 0x68;	/* NumPad 8 */
		case SDLK_KP_9:	return 0x69;	/* NumPad 9 */
		case SDLK_KP_4:	return 0x6a;	/* NumPad 4 */
		case SDLK_KP_5:	return 0x6b;	/* NumPad 5 */
		case SDLK_KP_6:	return 0x6c;	/* NumPad 6 */
		case SDLK_KP_1:	return 0x6d;	/* NumPad 1 */
		case SDLK_KP_2:	return 0x6e;	/* NumPad 2 */
		case SDLK_KP_3:	return 0x6f;	/* NumPad 3 */
		case SDLK_KP_0:	return 0x70;	/* NumPad 0 */
		case SDLK_KP_PERIOD:	return 0x71;	/* NumPad . */
		case SDLK_KP_ENTER:	return 0x72;	/* NumPad Enter */
		case SDLK_KP_MINUS:	return 0x4a;	/* NumPad - */
		case SDLK_KP_PLUS:	return 0x4e;	/* NumPad + */

		// Special Keys
		case SDLK_F11:	return 0x62;	/* F11 => Help */
		case SDLK_F12:	return 0x61;	/* F12 => Undo */
		case SDLK_HOME:	return 0x47;	/* Home */
		case SDLK_UP:	return 0x48;	/* Arrow Up */
		case SDLK_PAGEUP: return 0x49;	/* Page Up */
		case SDLK_LEFT:	return 0x4b;	/* Arrow Left */
		case SDLK_RIGHT:	return 0x4d;	/* Arrow Right */
		case SDLK_END:	return 0x4f;	/* Milan's scancode for End */
		case SDLK_DOWN:	return 0x50;	/* Arrow Down */
		case SDLK_PAGEDOWN:	return 0x51;	/* Page Down */
		case SDLK_INSERT:	return 0x52;	/* Insert */
		case SDLK_DELETE:	return 0x53;	/* Delete */

		case SDLK_NUMLOCKCLEAR: return 0x63;
		
		case SDLK_BACKQUOTE:
		case SDLK_LESS: return 0x60;	/* a '<>' key next to short left Shift */

		// keys not found on original Atari keyboard
		case SDLK_RCTRL:	return 0x1d;	/* map right Control to Atari control */
		case SDLK_MODE: /* passthru */ /* Alt Gr key according to SDL docs */
		case SDLK_RALT:		return RALT_ATARI_SCANCODE;
	}

#if !SDL_VERSION_ATLEAST(2, 0, 0)
	static int offset = UNDEFINED_OFFSET;

	// Process remaining keys: assume that it's PC101 keyboard
	// and that it is compatible with Atari ST keyboard (basically
	// same scancodes but on different platforms with different
	// base offset (framebuffer = 0, X11 = 8).
	// Try to detect the offset using a little bit of black magic.
	// If offset is known then simply pass the scancode.
	int scanPC = keysym.scancode;
	if (offset == UNDEFINED_OFFSET /* || scanPC == 0 */) {
		offset = findScanCodeOffset(keysym);
	}

	// offset is defined so pass the scancode directly
	if (offset != UNDEFINED_OFFSET && scanPC > offset)
		return scanPC - offset;
#endif

	if (keysym.scancode != 0)
	bug("keycode: %d (0x%x), scancode %d (0x%x), keysym '%s' is not mapped",
		keysym.sym, keysym.sym,
		keysym.scancode, keysym.scancode,
		SDL_GetKeyName(keysym.sym));
	return 0;
}
#endif /* KEYSYM_SCANCODE */

/*********************************************************************
 * Input event checking
 *********************************************************************/
static bool pendingQuit = false;
static int but = 0;
static bool mouseOut = false;

#ifdef SDL_GUI
extern bool isGuiAvailable;	// from main.cpp
static bool cur_fullscreen;

void open_GUI(void)
{
	if (!isGuiAvailable || !SDLGui_isClosed()) {
		return;
	}

	cur_fullscreen = bx_options.video.fullscreen;

	/* Always ungrab+show mouse */
	hiddenMouse = hideMouse(SDL_FALSE);
	grabbedMouse = grabMouse(SDL_FALSE);
	
	SDLGui_Open(NULL);
#if SDL_VERSION_ATLEAST(2, 0, 0)
	host->video->gui_window = host->video->window;
	host->video->gui_window_id = SDL_GetWindowID(host->video->gui_window);
#endif
}

void close_GUI(void)
{
	HostScreen *video;

	if (!isGuiAvailable /*|| SDLGui_isClosed()*/) {
		return;
	}
	if (host == NULL || (video = host->video) == NULL)
		return;
	
	SDLGui_Close();
#if SDL_VERSION_ATLEAST(2, 0, 0)
	if (video->gui_window && video->gui_window != video->window)
		SDL_DestroyWindow(video->gui_window);
	video->gui_window = NULL;
	video->gui_window_id = 0;
#endif

	// small hack to toggle fullscreen from the SETUP GUI
	if (bx_options.video.fullscreen != cur_fullscreen) {
		bx_options.video.fullscreen = cur_fullscreen;

		video->toggleFullScreen();
		if (bx_options.video.fullscreen && !grabbedMouse)
			video->grabTheMouse();
	}

	/* Restore mouse cursor state */
	if (hiddenMouse) {
		hideMouse(SDL_TRUE);
	}
	if (grabbedMouse) {
		grabMouse(SDL_TRUE);
	}

	video->forceRefreshScreen();
}

#endif /* SDL_GUI */

SDL_bool grabMouse(SDL_bool grab)
{
	if (host && host->video) return host->video->grabMouse(grab);
	return SDL_FALSE;
}

SDL_bool hideMouse(SDL_bool hide)
{
	if (host && host->video) return host->video->hideMouse(hide);
	return SDL_FALSE;
}

HOTKEY check_hotkey(int state, SDL_Keycode sym)
{
#define CHECK_HOTKEY(Hotkey) \
	if (bx_options.hotkeys.Hotkey.sym != 0 && sym == bx_options.hotkeys.Hotkey.sym && state == bx_options.hotkeys.Hotkey.mod) \
		return HOTKEY_ ## Hotkey; \
	if (bx_options.hotkeys.Hotkey.sym == 0 && bx_options.hotkeys.Hotkey.mod != 0 && state == bx_options.hotkeys.Hotkey.mod) \
		return HOTKEY_ ## Hotkey
	CHECK_HOTKEY(setup);
	CHECK_HOTKEY(quit);
	CHECK_HOTKEY(warmreboot);
	CHECK_HOTKEY(coldreboot);
	CHECK_HOTKEY(debug);
	CHECK_HOTKEY(ungrab);
	CHECK_HOTKEY(screenshot);
	CHECK_HOTKEY(fullscreen);
	CHECK_HOTKEY(sound);
#undef CHECK_HOTKEY
	return HOTKEY_none;
}

static void process_keyboard_event(const SDL_Event &event)
{
	SDL_Keysym keysym = event.key.keysym;
	SDL_Keycode sym = keysym.sym;
	int state;
	HostScreen *video;
	
	if (host == NULL || (video = host->video) == NULL)
		return;
	
	if ((keysym.mod & HOTKEYS_MOD_MASK) == 0)
		state  = SDL_GetModState(); // keysym.mod does not deliver single mod key presses for some reason
	else
		state = keysym.mod;	// May be send by SDL_PushEvent
		
#ifdef SDL_GUI
	if (!SDLGui_isClosed()
#if SDL_VERSION_ATLEAST(2, 0, 0)
		&& event.key.windowID == video->gui_window_id
#endif
		)
	{
		switch (SDLGui_DoEvent(event)) {
			case Dialog::GUI_CLOSE:
				close_GUI();
				break;
			case Dialog::GUI_WARMREBOOT:
				close_GUI();
				RestartAll();
				break;
			case Dialog::GUI_COLDREBOOT:
				close_GUI();
				RestartAll(true);
				break;
			case Dialog::GUI_SHUTDOWN:
				close_GUI();
				pendingQuit = true;
				break;
			default:
				break;
		}
		return;	// don't pass the key events to emulation
	}
#endif /* SDL_GUI */

#ifdef FLIGHT_RECORDER
	static bool flight_is_active = false;
	bool flight_turn_on = (state & (KMOD_SHIFT)) == KMOD_RSHIFT;
	bool flight_turn_off = (state & (KMOD_SHIFT)) == KMOD_LSHIFT;
	bool flight_changed = (flight_turn_on && !flight_is_active) ||
						  (flight_turn_off && flight_is_active);
	if (flight_changed) {
		flight_is_active = ! flight_is_active;
		cpu_flight_recorder(flight_is_active);
		panicbug("Flight was %sactivated!", (flight_is_active ? "" : "DE"));
	}
#endif

	bool pressed = (event.type == SDL_KEYDOWN);
	bool shifted = state & KMOD_SHIFT;
	// bool controlled = state & KMOD_CTRL;
	// bool alternated = state & KMOD_ALT;
	bool capslocked = state & KMOD_CAPS;
	bool send2Atari = true;

	if (sym == SDLK_CAPSLOCK) send2Atari = false;
	if (video->CapslockState() != capslocked) {
		// SDL sends SDLK_CAPSLOCK keydown to turn it on and keyup to off.
		// TOS handles it just like any other keypress (down&up)
		//  ->	we handle this differently here
		getIKBD()->SendKey(0x3a);	// press CapsLock
		getIKBD()->SendKey(0xba);	// release CapsLock
		video->CapslockState(capslocked);
	}

#if SDL_VERSION_ATLEAST(2, 0, 0)
	if (event.key.windowID != video->window_id)
		return;
	if (event.key.repeat > 0)
		return;
#endif

#if defined(_WIN32) || defined(__CYGWIN__)
	/* SDL on windows does not report KMOD_CTRL on right ctrl key */
	if (keysym.sym == SDLK_RCTRL)
		state = pressed ? (state | KMOD_CTRL) : (state & ~KMOD_CTRL);
#endif

	// process special hotkeys
	int masked_mod = state & HOTKEYS_MOD_MASK;
	HOTKEY hotkey = check_hotkey(masked_mod, sym);
	if (pressed) {
		switch (hotkey)
		{
		case HOTKEY_none:
			break;
		case HOTKEY_quit:
			pendingQuit = true;
			send2Atari = false;
			break;
		case HOTKEY_warmreboot:
			RestartAll();	// force Warm Reboot
			send2Atari = false;
			break;
		case HOTKEY_coldreboot:
			RestartAll(true);	// force Cold Reboot
			send2Atari = false;
			break;
		case HOTKEY_setup:
#ifdef SDL_GUI
			/* release shifters (if any) */
			if ( bx_options.hotkeys.setup.mod & KMOD_LSHIFT )
				getIKBD()->SendKey(0x80 | 0x2a);
			if ( bx_options.hotkeys.setup.mod & KMOD_RSHIFT )
				getIKBD()->SendKey(0x80 | 0x36);
			if ( bx_options.hotkeys.setup.mod & KMOD_CTRL )
				getIKBD()->SendKey(0x80 | 0x1d);
			if ( bx_options.hotkeys.setup.mod & KMOD_LALT )
				getIKBD()->SendKey(0x80 | 0x38);
			if ( bx_options.hotkeys.setup.mod & (KMOD_MODE|KMOD_RALT) )
				getIKBD()->SendKey(0x80 | RALT_ATARI_SCANCODE);
			
			open_GUI();
			send2Atari = false;
#endif
			break;
		case HOTKEY_debug:
#ifdef DEBUGGER
			// activate debugger
			activate_debugger();
			send2Atari = false;
#endif
			break;
		case HOTKEY_ungrab:
			if ( bx_options.video.fullscreen )
				video->toggleFullScreen();
			video->releaseTheMouse();
			video->CanGrabMouseAgain(false);	// let it leave our window
			send2Atari = false;
			break;
		case HOTKEY_screenshot:
			video->doScreenshot();
			send2Atari = false;
			break;
		case HOTKEY_fullscreen:
			video->toggleFullScreen();
			if (bx_options.video.fullscreen && !video->GrabbedMouse())
				video->grabTheMouse();
			send2Atari = false;
			break;
		case HOTKEY_sound:
			host->audio.ToggleAudio();
			send2Atari = false;
			break;
		}
	} else if (hotkey != HOTKEY_none)
	{
		send2Atari = false;
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

#ifdef NFVDI_SUPPORT
static NF_Base* fvdi = NULL;
#endif


static void send_wheelup(bool clicked)
{
#ifdef NFVDI_SUPPORT
	if (clicked && fvdi != NULL && fvdi->dispatch(0xc00100ff) == 0)
		return;
#endif
	if (bx_options.ikbd.wheel_eiffel) {
		if (clicked) {
			getIKBD()->SendKey(0xF6);
			getIKBD()->SendKey(0x05);
			getIKBD()->SendKey(0x00);
			getIKBD()->SendKey(0x00);
			getIKBD()->SendKey(0x00);
			getIKBD()->SendKey(0x00);
			getIKBD()->SendKey(0x00);
			getIKBD()->SendKey(0x59);
		}
	}
	else {
		int releaseKeyMask = (clicked ? 0x00 : 0x80);
		getIKBD()->SendKey(0x48 | releaseKeyMask);	// keyUp
	}
}


static void send_wheeldown(bool clicked)
{
#ifdef NFVDI_SUPPORT
	if (clicked && fvdi != NULL && fvdi->dispatch(0xc0010001) == 0)
		return;
#endif
	if (bx_options.ikbd.wheel_eiffel) {
		if (clicked) {
			getIKBD()->SendKey(0xF6);
			getIKBD()->SendKey(0x05);
			getIKBD()->SendKey(0x00);
			getIKBD()->SendKey(0x00);
			getIKBD()->SendKey(0x00);
			getIKBD()->SendKey(0x00);
			getIKBD()->SendKey(0x00);
			getIKBD()->SendKey(0x5A);
		}
	}
	else {
		int releaseKeyMask = (clicked ? 0x00 : 0x80);
		getIKBD()->SendKey(0x50 | releaseKeyMask);	// keyDown
	}
}


static void process_mouse_event(const SDL_Event &event)
{
	HostScreen *video;

	if (host == NULL || (video = host->video) == NULL)
		return;
	
#ifdef NFVDI_SUPPORT
	bool mouse_exit = false;
	bool fvdi_events = false;

	if (!fvdi) {
		fvdi = NFGetDriver("fVDI");
	}
#endif

#ifdef SDL_GUI
	if (!SDLGui_isClosed()
#if SDL_VERSION_ATLEAST(2, 0, 0)
		&& event.key.windowID == video->gui_window_id
#endif
		)
	{
		switch (SDLGui_DoEvent(event)) {
			case Dialog::GUI_CLOSE:
				close_GUI();
				break;
			case Dialog::GUI_WARMREBOOT:
				close_GUI();
				RestartAll();
				break;
			case Dialog::GUI_COLDREBOOT:
				close_GUI();
				RestartAll(true);
				break;
			case Dialog::GUI_SHUTDOWN:
				close_GUI();
				pendingQuit = true;
				break;
			default:
				break;
		}
		return;	// don't pass the mouse events to emulation
	}
#endif /* SDL_GUI */

	if (!video->GrabbedMouse()) {
		if ((event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP) && event.button.button == SDL_BUTTON_LEFT) {
			D(bug("Left mouse click in our window => grab the mouse"));
			video->RememberAtariMouseCursorPosition();
			video->grabTheMouse();
		}
		return;
	}

	int xrel = 0;
	int yrel = 0;
	int lastbut = but;
	if (event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP) {
		bool clicked = (event.type == SDL_MOUSEBUTTONDOWN);
		int releaseKeyMask = (clicked ? 0x00 : 0x80);
		switch(event.button.button) {
			case SDL_BUTTON_LEFT:
				if (clicked)
					but |= 2;
				else
					but &= ~2;
				break;

			case SDL_BUTTON_RIGHT:
				if (clicked)
					but |= 1;
				else
					but &= ~1;
				break;

			case SDL_BUTTON_MIDDLE:
				if (bx_options.ikbd.wheel_eiffel) {
					D(bug("Middle mouse button"));
					getIKBD()->SendKey(0x37 | releaseKeyMask);
				}
				else if (clicked) {
					// ungrab on middle mouse button click
					if ( bx_options.video.fullscreen )
						video->toggleFullScreen();
					video->releaseTheMouse();
					video->CanGrabMouseAgain(false);	// let it leave our window
				}
				return;

#if !SDL_VERSION_ATLEAST(2, 0, 0)
			case SDL_BUTTON_WHEELUP:
				send_wheelup(clicked);
				return;

			case SDL_BUTTON_WHEELDOWN:
				send_wheeldown(clicked);
				return;
#endif

			default:
				D(bug("Unknown mouse button: %d", event.button.button));
		}
	}
	else if (event.type == SDL_MOUSEMOTION) {
		if (!video->IgnoreMouseMotionEvent()) {
			xrel = event.motion.xrel;
			yrel = event.motion.yrel;
		}
		else video->IgnoreMouseMotionEvent(false);

#ifdef NFVDI_SUPPORT
		if (fvdi != NULL) {
			if (fvdi->dispatch(0x80008000 | (event.motion.x << 16) | (event.motion.y)) == 0)
				fvdi_events = true;
		}

		// Can't use the method below to get out of the window
		// if the events are reported directly, since it only
		// works with hidden mouse pointer.
		// So, define top left corner as exit point.
		mouse_exit = (event.motion.x == 0) && (event.motion.y == 0);
#endif
#ifdef __ANDROID__
		if (getARADATA()->isAtariMouseDriver()) {
			xrel = event.motion.x - getARADATA()->getAtariMouseX();
			yrel = event.motion.y - getARADATA()->getAtariMouseY();
		}
#endif
		
		if (!bx_options.startup.grabMouseAllowed && (
			(xrel <= 0 && event.motion.x <= 0) ||
			(yrel <= 0 && event.motion.y <= 0) ||
			(xrel >= 0 && event.motion.x >= video->getWidth() - 1) ||
			(yrel >= 0 && event.motion.y >= video->getHeight() - 1)))
			mouseOut = true;
	}
#if SDL_VERSION_ATLEAST(2, 0, 0)
	else if (event.type == SDL_MOUSEWHEEL) {
		if (event.wheel.y > 0)
		{
			send_wheelup(true);
			send_wheelup(false);
		} else if (event.wheel.y < 0)
		{
			send_wheeldown(true);
			send_wheeldown(false);
		}
	}
#endif

#ifdef NFVDI_SUPPORT
	if (lastbut != but) {
		if (fvdi != NULL && (fvdi->dispatch(0xb0000000 | but) == 0))
			fvdi_events = true;
	}

	if (fvdi_events) {
#if DEBUG
		static bool reported = false;
		if (!reported) {
			D(bug("Using nfvdi direct mouse support"));
			reported = true;
		}
#endif
		if (!bx_options.video.fullscreen && mouse_exit && !bx_options.startup.grabMouseAllowed)
			mouseOut = true;
		return;
	}
#endif

	// send the mouse data packet
	if (xrel || yrel || lastbut != but) {
#if 0
		if (xrel < -250 || xrel > 250 || yrel < -250 || yrel > 250) {
			D(bug("Resetting suspicious mouse packet: position %dx%d, buttons %d", xrel, yrel, but));
			xrel = yrel = 0;	// reset the values otherwise ikbd goes crazy
		}
#endif
		getIKBD()->SendMouseMotion(xrel, yrel, but, false);
	}

	if (! bx_options.video.fullscreen && !bx_options.startup.grabMouseAllowed && getARADATA()->isAtariMouseDriver()) {
		// check whether user doesn't try to go out of window (top or left)
		if ((xrel < 0 && getARADATA()->getAtariMouseX() == 0) ||
			(yrel < 0 && getARADATA()->getAtariMouseY() == 0))
			mouseOut = true;

		// same check but for bottom and right side of our window
		int hostw = video->getWidth();
		int hosth = video->getHeight();

		if ((xrel > 0 && getARADATA()->getAtariMouseX() >= hostw - 1) ||
			(yrel > 0 && getARADATA()->getAtariMouseY() >= hosth - 1))
			mouseOut = true;
	}
}

/*--- Video resize event ---*/

static void process_resize_event(int w, int h)
{
	if (!host || !host->video) {
		return;
	}

	/* Use new size as fixed size */
	if (bx_options.autozoom.fixedsize) {
		bx_options.autozoom.width = w;
		bx_options.autozoom.height = h;
	}

	host->video->resizeWindow(w, h);
}

static void process_active_event(const SDL_Event &event)
{
	HostScreen *video;

	if (host == NULL || (video = host->video) == NULL)
		return;
	
#if SDL_VERSION_ATLEAST(2, 0, 0)
	if (event.window.event == SDL_WINDOWEVENT_NONE ||
		event.window.event == SDL_WINDOWEVENT_SHOWN ||
		event.window.event == SDL_WINDOWEVENT_HIDDEN ||
		event.window.event == SDL_WINDOWEVENT_EXPOSED ||
		event.window.event == SDL_WINDOWEVENT_MOVED)
		return;
	if (event.window.event == SDL_WINDOWEVENT_CLOSE)
	{
#ifdef SDL_GUI
		if (event.window.windowID == video->gui_window_id && !SDLGui_isClosed())
			close_GUI();
#endif
		return;
	}
	
	bool app_focus = event.window.event == SDL_WINDOWEVENT_MINIMIZED || event.window.event == SDL_WINDOWEVENT_RESTORED || event.window.event == SDL_WINDOWEVENT_SHOWN || event.window.event == SDL_WINDOWEVENT_HIDDEN;
	bool mouse_focus = event.window.event == SDL_WINDOWEVENT_ENTER || event.window.event == SDL_WINDOWEVENT_LEAVE;
	bool input_focus = event.window.event == SDL_WINDOWEVENT_FOCUS_GAINED || event.window.event == SDL_WINDOWEVENT_FOCUS_LOST;
	bool gained = event.window.event == SDL_WINDOWEVENT_RESTORED || event.window.event == SDL_WINDOWEVENT_SHOWN || event.window.event == SDL_WINDOWEVENT_ENTER || event.window.event == SDL_WINDOWEVENT_FOCUS_GAINED;
	if ((event.window.event == SDL_WINDOWEVENT_RESIZED ||
		 event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) &&
		event.window.windowID == video->window_id)
	{
		process_resize_event(event.window.data1, event.window.data2);
	}
#else
	bool app_focus = (event.active.state & SDL_APPACTIVE) != 0;
	bool mouse_focus = (event.active.state & SDL_APPMOUSEFOCUS) != 0;
	bool input_focus = (event.active.state & SDL_APPINPUTFOCUS) != 0;
	bool gained = event.active.gain != 0;
#endif
#if DEBUG
	uint32 ticks = SDL_GetTicks();
#endif

	if (app_focus) {
		D(bug("%d: ARAnyM window is being %s", ticks, gained ? "restored" : "minimized"));
	}

	if (input_focus) {
		D(bug("%d: ARAnyM window is %s input focus", ticks, gained ? "gaining" : "losing"));
	}

	// if it's mouse focus event
	if (mouse_focus) {
		D(bug("%d: Mouse pointer is %s ARAnyM window", ticks, gained ? "entering" : "leaving"));
		bool allowMouseGrab = true;
#ifdef SDL_GUI
		// Disable grab, if Setup GUI is open
		// because else the mouse goes invisible if the window is reentered
		allowMouseGrab = SDLGui_isClosed();
#endif
		if (allowMouseGrab) {
			// if we can grab the mouse automatically
			// and if the Atari mouse driver works
			D(bug("Proceed only if AtariMouseDriver is working"));
			if (getARADATA()->isAtariMouseDriver()) {
				// if the mouse pointer is leaving our window
				if (!gained) {
					// allow grabbing it when it will be returning
					D(bug("Host mouse is indeed leaving us"));
					if (video->CanGrabMouseAgain()) {
						// video->RememberAtariMouseCursorPosition();
					}
				}
				// if the mouse pointer is entering our window
				else {
					D(bug("Host mouse is returning!"));
					// if grabbing the mouse is allowed
					if (video->CanGrabMouseAgain() && (video->HasInputFocus() || bx_options.startup.grabMouseAllowed)) {
						D(bug("canGrabMouseAgain allows autograb"));
						// then grab it
						video->grabTheMouse();
					}
				}
			}
		}
	}
}

/*--- Joystick event ---*/

static void process_joystick_event(const SDL_Event &event)
{
	switch(event.type) {
		case SDL_JOYAXISMOTION:
			if (event.jaxis.which==bx_options.joysticks.ikbd0) {
				getIKBD()->SendJoystickAxis(0, event.jaxis.axis, event.jaxis.value);
			} else if (event.jaxis.which==bx_options.joysticks.ikbd1) {
				getIKBD()->SendJoystickAxis(1, event.jaxis.axis, event.jaxis.value);
			} else if (event.jaxis.which==bx_options.joysticks.joypada) {
				getJOYPADS()->sendJoystickAxis(0, event.jaxis.axis, event.jaxis.value);
			} else if (event.jaxis.which==bx_options.joysticks.joypadb) {
				getJOYPADS()->sendJoystickAxis(1, event.jaxis.axis, event.jaxis.value);
			}
			break;		
		case SDL_JOYHATMOTION:
			if (event.jaxis.which==bx_options.joysticks.ikbd0) {
				getIKBD()->SendJoystickHat(0, event.jhat.value);
			} else if (event.jaxis.which==bx_options.joysticks.ikbd1) {
				getIKBD()->SendJoystickHat(1, event.jhat.value);
			} else if (event.jaxis.which==bx_options.joysticks.joypada) {
				getJOYPADS()->sendJoystickHat(0, event.jhat.value);
			} else if (event.jaxis.which==bx_options.joysticks.joypadb) {
				getJOYPADS()->sendJoystickHat(1, event.jhat.value);
			}
			break;		
		case SDL_JOYBUTTONDOWN:
		case SDL_JOYBUTTONUP:
			if (event.jaxis.which==bx_options.joysticks.ikbd0) {
				getIKBD()->SendJoystickButton(0,
					event.jbutton.state==SDL_PRESSED);
			} else if (event.jaxis.which==bx_options.joysticks.ikbd1) {
				getIKBD()->SendJoystickButton(1,
					event.jbutton.state==SDL_PRESSED);
			} else if (event.jaxis.which==bx_options.joysticks.joypada) {
				getJOYPADS()->sendJoystickButton(0, event.jbutton.button,
					event.jbutton.state==SDL_PRESSED);
			} else if (event.jaxis.which==bx_options.joysticks.joypadb) {
				getJOYPADS()->sendJoystickButton(1, event.jbutton.button,
					event.jbutton.state==SDL_PRESSED);
			}
			break;		
	}
}

///////
// main function for checking keyboard, mouse and joystick events
// called from main.cpp every 20 ms
void check_event()
{
	HostScreen *video;
	
	if (host == NULL || (video = host->video) == NULL)
		return;
	
	if (!bx_options.video.fullscreen && mouseOut) {
		// host mouse moved but the Atari mouse did not => mouse is
		// probably at the Atari screen border. Ungrab it and warp the host mouse at
		// the same location so the mouse moves smoothly.
		video->releaseTheMouse();
		mouseOut = false;
	}

	SDL_Event event;

	while (SDL_PollEvent(&event)) {
		switch(event.type) {
			case SDL_KEYDOWN:
			case SDL_KEYUP:
				process_keyboard_event(event);
				break;
			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
			case SDL_MOUSEMOTION:
#if SDL_VERSION_ATLEAST(2, 0, 0)
			case SDL_MOUSEWHEEL:
#endif
				process_mouse_event(event);
				break;

			case SDL_JOYBUTTONDOWN:
			case SDL_JOYBUTTONUP:
			case SDL_JOYHATMOTION:
			case SDL_JOYAXISMOTION:
				process_joystick_event(event);
				break;

#if SDL_VERSION_ATLEAST(2, 0, 0)
			case SDL_WINDOWEVENT:
#else
			case SDL_ACTIVEEVENT:
#endif
				process_active_event(event);
				break;

#if SDL_VERSION_ATLEAST(2, 0, 0)
			case SDL_APP_TERMINATING:
			case SDL_APP_LOWMEMORY:
			case SDL_APP_WILLENTERBACKGROUND:
			case SDL_APP_DIDENTERBACKGROUND:
			case SDL_APP_WILLENTERFOREGROUND:
			case SDL_APP_DIDENTERFOREGROUND:
				/* SDL2FIXME: TODO */
				break;
			case SDL_FINGERDOWN:
			case SDL_FINGERUP:
			case SDL_FINGERMOTION:
				/* SDL2FIXME: TODO */
				break;
#endif

#if SDL_VERSION_ATLEAST(2, 0, 0)
			/* resize handled in process_active_event() */
#else
			case SDL_VIDEORESIZE:
				process_resize_event(event.resize.w, event.resize.h);
				break;
#endif

			case SDL_QUIT:
#ifdef SDL_GUI
				if (!SDLGui_isClosed()) {
					close_GUI();
				}
#endif
				pendingQuit = true;
				break;
		}
	}

	if (pendingQuit) {
		Quit680x0();	// forces CPU to quit the loop
	}
}

// don't remove this modeline with intended formatting for vim:ts=4:sw=4:

