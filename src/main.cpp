/*
 * $Header$
 *
 * 2001 MJ
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

#include "cpu_emulation.h"
#include "main.h"
#include "hardware.h"
#include "parameters.h"
#include "extfs.h"
#include "emul_op.h"			// for the extFS

#define DEBUG 1
#include "debug.h"

#define METADOS_DRV

// CPU and FPU type, addressing mode
int CPUType;
bool CPUIs68060;
int FPUType;

// Constants
const char ROM_FILE_NAME[] = DATADIR "/ROM";

#define ErrorAlert(a)	fprintf(stderr, a)

void init_fdc();				// fdc.cpp

SDL_TimerID my_timer_id;

extern int irqindebug;

static int keyboardTable[0x80] = {
/* 0-7 */ 0, SDLK_ESCAPE, SDLK_1, SDLK_2, SDLK_3, SDLK_4, SDLK_5, SDLK_6,
/* 8-f */ SDLK_7, SDLK_8, SDLK_9, SDLK_0, SDLK_EQUALS, SDLK_QUOTE, SDLK_BACKSPACE, SDLK_TAB,
/*10-17*/ SDLK_q, SDLK_w, SDLK_e, SDLK_r, SDLK_t, SDLK_y, SDLK_u, SDLK_i,
/*18-1f*/ SDLK_o, SDLK_p, SDLK_LEFTPAREN, SDLK_RIGHTPAREN, SDLK_RETURN, SDLK_LCTRL, SDLK_a, SDLK_s,
/*20-27*/ SDLK_d, SDLK_f, SDLK_g, SDLK_h, SDLK_j, SDLK_k, SDLK_l, SDLK_SEMICOLON,
/*28-2f*/ SDLK_QUOTE, SDLK_HASH, SDLK_LSHIFT, SDLK_BACKQUOTE, SDLK_z, SDLK_x, SDLK_c, SDLK_v,
/*30-37*/ SDLK_b, SDLK_n, SDLK_m, SDLK_COMMA, SDLK_PERIOD, SDLK_SLASH, SDLK_RSHIFT, 0,
/*38-3f*/ SDLK_LALT, SDLK_SPACE, SDLK_CAPSLOCK, SDLK_F1, SDLK_F2, SDLK_F3, SDLK_F4, SDLK_F5,
/*40-47*/ SDLK_F6, SDLK_F7, SDLK_F8, SDLK_F9, SDLK_F10, 0, 0, SDLK_HOME,
/*48-4f*/ SDLK_UP, 0, SDLK_KP_PLUS, SDLK_LEFT, 0, SDLK_RIGHT, SDLK_KP_MINUS, 0,
/*50-57*/ SDLK_DOWN, 0, SDLK_INSERT, SDLK_DELETE, 0, 0, 0, 0,
/*58-5f*/ 0, 0, 0, 0, 0, 0, 0, 0,
/*60-67*/ SDLK_LESS, SDLK_PAGEDOWN, SDLK_PAGEUP, 0 /* NumLock */ , SDLK_KP_DIVIDE, SDLK_KP_MULTIPLY, SDLK_KP_MINUS, SDLK_KP7,
/*68-6f*/ SDLK_KP8, SDLK_KP9, SDLK_KP4, SDLK_KP5, SDLK_KP6, SDLK_KP1, SDLK_KP2, SDLK_KP3,
/*70-77*/ SDLK_KP0, SDLK_KP_PERIOD, SDLK_KP_ENTER, 0, 0, 0, 0, 0,
/*78-7f*/ 0, 0, 0, 0, 0, 0, 0, 0
};

#define MAXDRIVES	32
int drive_fd[MAXDRIVES];

void remove_floppy()
{
	if (drive_fd[0] >= 0) {
		close(drive_fd[0]);
		drive_fd[0] = -1;
		D(bug("Floppy removed"));
	}
}

void insert_floppy(bool rw = false)
{
	remove_floppy();
	drive_fd[0] = open("/dev/fd0", rw ? (O_RDWR | O_SYNC) : O_RDONLY);
	if (drive_fd[0] >= 0) {
		init_fdc();
		D(bug("Floppy inserted %s", rw ? "read-write" : "read-only"));
	}
	else {
		D(bug("Inserting of floppy failed."));
	}
}

bool grabbedMouse = false;
bool hiddenMouse = false;
bool canGrabAgain = true;

void hideMouse(bool hide) {
	if (hide) {
		SDL_ShowCursor(SDL_DISABLE);
		hiddenMouse = true;
	}
	else if (!hide) {
		SDL_ShowCursor(SDL_ENABLE);
		hiddenMouse = false;
	}
}

bool grabMouse(bool grab) {
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

static int but = 0;

static void check_event(void)
{
	static bool pendingQuit = false;

	if (pendingQuit)
		QuitEmulator();

	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		int type = event.type;
		if (type == SDL_KEYDOWN || type == SDL_KEYUP) {
			bool pressed = (type == SDL_KEYDOWN);
			int sym = event.key.keysym.sym;
			bool shifted = SDL_GetModState() & KMOD_SHIFT;

			if (pressed) {
				// process some hotkeys
				if (sym == SDLK_PAUSE && shifted) {
					pendingQuit = true;
				}
				else if (sym == SDLK_F11)
					insert_floppy(shifted);
				else if (sym == SDLK_F12)
					remove_floppy();
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
					ikbd_send(i);
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
					else
						but |= 1;
				}
				else if (event.button.button == SDL_BUTTON_LEFT)
					but |= 2;
			}
			else if (type == SDL_MOUSEBUTTONUP) {
				if (event.button.button == SDL_BUTTON_RIGHT)
					but &= ~1;
				else if (event.button.button == SDL_BUTTON_LEFT)
					but &= ~2;
			}
#if 0
			if (type == SDL_MOUSEMOTION) {
				int x, y;
				SDL_GetMouseState(&x, &y);
				updateXMouse(x, y);
			}
#else
			else if (type == SDL_MOUSEMOTION) {
				SDL_MouseMotionEvent eve = event.motion;
				xrel = eve.xrel;
				yrel = eve.yrel;

				if (xrel < -127 || xrel > 127)
					xrel = 0;
				if (yrel < -127 || yrel > 127)
					yrel = 0;
			}
			if (xrel || yrel || lastbut != but) {
				ikbd_send(0xf8 | but);
				ikbd_send(xrel);
				ikbd_send(yrel);
				// fprintf(stderr, "Mouse: %dx%d\tLineA=%dx%d\t", xrel, yrel, HWget_w(0xf90000), HWget_w(0xf90002));
			}
#endif
		}
		else if (event.type == SDL_ACTIVEEVENT) {
			if (event.active.state == SDL_APPMOUSEFOCUS) {
				if (event.active.gain) {
					if ((SDL_GetAppState() & SDL_APPINPUTFOCUS) && canGrabAgain) {
						D(bug("Mouse entered our window"));
						hideMouse(true);
						if (false /* are we able to sync TOS and host mice? */) {
							// sync the position of ST mouse with the X mouse cursor (or vice-versa?)
						}
						else {
							// we got to grab the mouse completely, otherwise they'd be out of sync
							grabMouse(true);
						}
					}
				}
				else {
					D(bug("Mouse left our window"));
					canGrabAgain = true;
					hideMouse(false);
				}
			}
		}
		else if (event.type == SDL_QUIT) {
			pendingQuit = true;
		}
	}
}


/*
 * my_callback_function() is called every 10 miliseconds (~ 100 Hz)
 */
Uint32 my_callback_function(Uint32 interval, void *param)
{
	static int VBL_counter = 0;
	static int refreshCounter = 0;

	if (!debugging || irqindebug)
		MakeMFPIRQ(5);				// TimerC interrupt (synchronized to 200 Hz internally)

	if (++VBL_counter == 2) {		// divided by 2 => 50 Hz VBL
		VBL_counter = 0;

		if (!debugging || irqindebug) {
			check_event();				// process keyboard and mouse events
			TriggerVBL();				// generate VBL
		}

		if (++refreshCounter == 2) {	// divided by 2 again ==> 25 Hz screen update
			if (! direct_truecolor) {
				renderScreen();
			}
			refreshCounter = 0;
		}
	}
	return 10;					// come back in 10 milliseconds
}
/*
 *  Initialize everything, returns false on error
 */

bool InitAll(void)
{
	init_ide();	// fix this

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
		ErrorAlert("SDL initialization failed.");
		return 1;
	}
	atexit(SDL_Quit);

	// Initialize variables
	RAMBaseHost = NULL;
	ROMBaseHost = NULL;
	//TTRAMBaseHost = NULL;

#ifdef NDEBUG
	if (start_debug) ndebug::init();
#endif

	// Create areas for Atari ST-RAM, ROM, HW registers and TT-RAM
	int MB = (1 << 20);
	RAMBaseHost = (uint8 *) malloc((16 + 32) * MB);
	if (RAMBaseHost == NULL) {
		ErrorAlert("Not enough memory\n");
		QuitEmulator();
	}
	ROMBaseHost = RAMBaseHost + ROMBase;
	//TTRAMBaseHost = RAMBaseHost + TTRAMBase;

	// Initialize MEMBaseDiff now so that Host2MacAddr in the Video module
	// will return correct results
	// ROMBase = RAMBase + 0xe00000;

	InitMEMBaseDiff(RAMBaseHost, RAMBase);
	D(bug("ST-RAM starts at %p (%08x)", RAMBaseHost, RAMBase));
	D(bug("TOS ROM starts at %p (%08x)", ROMBaseHost, ROMBase));
	//D(bug("TT-RAM starts at %p (%08x)", TTRAMBaseHost, TTRAMBase));

	// Load TOS ROM
	int rom_fd = open(rom_path ? rom_path : ROM_FILE_NAME, O_RDONLY);
	if (rom_fd < 0) {
		ErrorAlert("ROM file not found\n");
		QuitEmulator();
	}
	D(bug("Reading ROM file..."));
	ROMSize = lseek(rom_fd, 0, SEEK_END);
	if (ROMSize != 512 * 1024) {
		ErrorAlert("Invalid ROM size\n");
		close(rom_fd);
		QuitEmulator();
	}
	lseek(rom_fd, 0, SEEK_SET);
	if (read(rom_fd, ROMBaseHost, ROMSize) != (ssize_t) ROMSize) {
		ErrorAlert("ROM file reading error\n");
		close(rom_fd);
		QuitEmulator();
	}

	CPUType = 4;
	FPUType = 1;

	// The fullscreen mode implies mouse grab (standa)
	if (fullscreen)
		grab_mouse = true;

	// grab mouse
	if (grab_mouse) {
		grabMouse(true);
	}

	drive_fd[0] = drive_fd[1] = drive_fd[2] = -1;

	// do not insert floppy automatically
	// insert_floppy();

	// Setting "SP & PC"
	put_long_direct(0x00000000,get_long_direct(ROMBase));
	put_long_direct(0x00000004,get_long_direct(ROMBase+4));

	if (direct_truecolor) {
		// Patch TOS (enforce VideoRAM at 0xf0000000)
		D(bug("Patching TOS for direct VIDEL output..."));
		ROMBaseHost[35752] = 0x2e;
		ROMBaseHost[35753] = 0x3c;
		ROMBaseHost[35754] = 0xf0;
		ROMBaseHost[35755] = 0;
		ROMBaseHost[35756] = 0;
		ROMBaseHost[35757] = 0;
		ROMBaseHost[35758] = 0x60;
		ROMBaseHost[35759] = 6;
		ROMBaseHost[35760] = 0x4e;
		ROMBaseHost[35761] = 0x71;
	}

#ifdef METADOS_DRV
	// install the drives
	extFS.init();
#endif							// METADOS_DRV

	// Init HW
	HWInit();

	// Init 680x0 emulation (this also activates the memory system which is needed for PatchROM())
	if (!Init680x0())
		return false;

	if (start_debug) {
		D(bug("Activate debugger..."));
		activate_debugger();
	}

	// hide mouse unconditionally
	hideMouse(true);

	// timer init
	my_timer_id = SDL_AddTimer(10, my_callback_function, NULL);

	return true;
}


/*
 *  Deinitialize everything
 */

void ExitAll(void)
{
	// Exit Time Manager
	SDL_RemoveTimer(my_timer_id);

	// remove floppy (flush buffers)
	remove_floppy();

	// Free ROM/RAM areas
	free(RAMBaseHost);

	SDL_VideoQuit();
}

