/*
 * $Header$
 *
 * ARAnym Team 2000
 */

/*
 *  main_unix.cpp - Startup code for Unix
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

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <getopt.h>
#include <errno.h>

#include <SDL/SDL.h>
#include <sys/mman.h>
#include <dirent.h>				// for extFS

#ifdef HAVE_PTHREADS
# include <pthread.h>
#endif

#if REAL_ADDRESSING || DIRECT_ADDRESSING
# include <sys/mman.h>
#endif

#if !EMULATED_68K && defined(__NetBSD__)
# include <m68k/sync_icache.h> 
# include <m68k/frame.h>
# include <sys/param.h>
# include <sys/sysctl.h>
struct sigstate {
	int ss_flags;
	struct frame ss_frame;
	struct fpframe ss_fpstate;
};
# define SS_FPSTATE  0x02
# define SS_USERREGS 0x04
#endif

#include "cpu_emulation.h"
#include "main.h"
#include "hardware.h"
#include "parameters.h"

#include "extfs.h"
#include "emul_op.h"			// for the extFS

#define DEBUG 1
#include "debug.h"

#define METADOS_DRV

// Constants
const char ROM_FILE_NAME[] = DATADIR "/ROM";
const int SIG_STACK_SIZE = SIGSTKSZ;	// Size of signal stack

// CPU and FPU type, addressing mode
int CPUType;
bool CPUIs68060;
int FPUType;

SDL_TimerID my_timer_id;

#if REAL_ADDRESSING
static bool lm_area_mapped = false;	// Flag: Low Memory area mmap()ped
static bool memory_mapped_from_zero = false; // Flag: Could allocate RAM area from 0
#endif

#if REAL_ADDRESSING || DIRECT_ADDRESSING
static uint32 mapped_ram_rom_size;	// Total size of mmap()ed RAM/ROM area
#endif

#ifndef HAVE_STRDUP
extern "C" char *strdup(const char *s)
{
	char *n = (char *)malloc(strlen(s) + 1);
	strcpy(n, s);
	return n;
}
#endif

void init_fdc();				// fdc.cpp

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

bool updateScreen = true;
#define MAXDRIVES	32
int drive_fd[MAXDRIVES];

void remove_floppy()
{
	if (drive_fd[0] >= 0) {
		close(drive_fd[0]);
		drive_fd[0] = -1;
		fprintf(stderr, "Floppy removed\n");
	}
}

void insert_floppy(bool rw = false)
{
	remove_floppy();
	drive_fd[0] = open("/dev/fd0", rw ? (O_RDWR | O_SYNC) : O_RDONLY);
	if (drive_fd[0] >= 0) {
		init_fdc();
		fprintf(stderr, "Floppy inserted %s\n",
				rw ? "read-write" : "read-only");
	}
	else {
		fprintf(stderr, "Inserting of floppy failed.\n");
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

void grabMouse(bool grab) {
	int current = SDL_WM_GrabInput(SDL_GRAB_QUERY);
	if (grab && current != SDL_GRAB_ON) {
		SDL_WM_GrabInput(SDL_GRAB_ON);
		grabbedMouse = true;
		hideMouse(true);
	}
	else if (!grab && current != SDL_GRAB_OFF) {
		SDL_WM_GrabInput(SDL_GRAB_OFF);
		grabbedMouse = false;
	}
}

static int sdl_mousex, sdl_mousey;
static int but = 0;
/* mouse from STonX's ikbd.c */
/*
static int ox=0, oy=0, px=0,py=0;

void ikbd_send_relmouse (void)
{
	// ikbd_send (0xf8 | (btn[0]<<1) | btn[1]);
	ikbd_send (0xf8 | but);
	ikbd_send (px-ox);
	ikbd_send (py-oy);
	ox = px;
	oy = py;
}

void ikbd_adjust (int dx, int dy)
{
	int tx=dx,ty=dy;

	while (dx < -128 || dx > 127 || dy < -128 || dy > 127)
	{
		if (dx < -128) tx=-128;
		else if (dx > 127) tx=127;
		if (dy < -128) ty=-128;
		else if (dy > 127) ty=127;
		// ikbd_send (0xf8 | (btn[0]<<1) | btn[1]);
		ikbd_send (0xf8 | but);
		ikbd_send (tx);
		ikbd_send (ty);
		dx -= tx;
		dy -= ty;
	}
	if (dy != 0 || dx != 0)
	{
		// ikbd_send (0xf8 | (btn[0]<<1) | btn[1]);
		ikbd_send (0xf8 | but);
		ikbd_send (dx);
		ikbd_send (dy);
	}
	ox=px;
	oy=py;
}

void ikbd_button (int button, int pressed)
{
    // handle two buton mice
    if(button == 3) button = 2;

	btn[button-1] = pressed;
#if KB_DEBUG
	fprintf (stderr, "IKBD sends button %d press (%d)\n",button,pressed);
#endif
	ikbd_send_relmouse();
}

void ikbd_pointer (int x, int y)
{
	px = x;
	py = y;
#if !NO_FLOODING
	ikbd_send_relmouse();
#endif
}

void updateXMouse(int xmouse_x, int xmouse_y) {
	// frequently check the TOS mouse position and
	// update it if the x mouse has moved and the ikbd
	// buffer is empty, this is done here, because the
	// buffer might have been full when the event occured
      
	  // get TOS mouse position
	  int tx = HWget_w(0xf90002); // LM_W(MEM(abase-GCURX));
	  int ty = HWget_w(0xf90000); // LM_W(MEM(abase-GCURY));

	  int old_shiftmod = 0;
	  int ikbd_inbuf = isIkbdBufEmpty() ? 0 : 1;

	  // compensate zoom in st mid
	  if(old_shiftmod == 1) ty *= 2;
	  
	  // match X and TOS mouse 
	  if((ikbd_inbuf == 0)&&
	     ((xmouse_x != tx)||(xmouse_y != ty))&&
	     (xmouse_x != -1)) {
	    // fprintf(stderr, "Adjusting %dx%d -> %dx%d\n", xmouse_x, xmouse_y, tx, ty);
	    
	    if(old_shiftmod == 1) 
	      ikbd_adjust (xmouse_x-tx, (xmouse_y-ty)/2);
	    else
	      ikbd_adjust (xmouse_x-tx, xmouse_y-ty);
	  }
}
end of STonX's borrowed code */


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
/*
				//eve.type/state/x,y/xrel,yrel
				int x, y;
				SDL_GetMouseState(&x, &y);
				xrel = x - sdl_mousex;
				yrel = y - sdl_mousey;
				// fprintf(stderr, "Mouse abs %dx%d, rel %dx%d nebo %dx%d\n", x, y, eve.xrel, eve.yrel, xrel, yrel);
				sdl_mousex = x;
				sdl_mousey = y;
*/
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
						fprintf(stderr, "Mouse entered our window\n");
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
					fprintf(stderr, "Mouse left our window\n");
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
			if (direct_truecolor) {
				// SDL_UpdateRect(SDL_GetVideoSurface(), 0, 0, getSDLScreenWidth(), getSDLScreenHeight());
			}
			else {
				if ( updateScreen )
					renderScreen();
			}
			refreshCounter = 0;
		}
	}
	return 10;					// come back in 10 milliseconds
}

/*
 *  Main program
 */
#define ErrorAlert(a)	fprintf(stderr, a)

#ifdef __cplusplus
extern "C"
#endif
int main(int argc, char **argv)
{
	program_name = argv[0];
	int i = decode_switches(argc, argv);

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
		fprintf(stderr, "SDL initialization failed.\n");
		return 1;
	}
	atexit(SDL_Quit);

	// grab mouse
	if (grab_mouse) {
		grabMouse(true);
	}

	drive_fd[0] = drive_fd[1] = drive_fd[2] = -1;

	insert_floppy();

	// Initialize variables
	RAMBaseHost = NULL;
	ROMBaseHost = NULL;
	//TTRAMBaseHost = NULL;
	srand(time(NULL));
	tzset();

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
	D(bug("ST-RAM starts at %p (%08x)\n", RAMBaseHost, RAMBase));
	D(bug("TOS ROM starts at %p (%08x)\n", ROMBaseHost, ROMBase));
	//D(bug("TT-RAM starts at %p (%08x)\n", TTRAMBaseHost, TTRAMBase));

	// Load TOS ROM
	int rom_fd = open(rom_path ? rom_path : ROM_FILE_NAME, O_RDONLY);
	if (rom_fd < 0) {
		ErrorAlert("ROM file not found\n");
		QuitEmulator();
	}
	printf("Reading ROM file...\n");
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

	if (direct_truecolor) {
		// Patch TOS (enforce VideoRAM at 0xf0000000)
		printf("Patching TOS for direct VIDEL output...\n");
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

	// Initialize everything
	if (!InitAll())
		QuitEmulator();
	D(bug("Initialization complete\n"));

	// hide mouse unconditionally
	hideMouse(true);

	my_timer_id = SDL_AddTimer(10, my_callback_function, NULL);

	// Start 68k and jump to ROM boot routine
	D(bug("Starting emulation...\n"));
	Start680x0();

	QuitEmulator();

	return 0;
}


/*
 *  Quit emulator
 */

void QuitEmulator(void)
{
	D(bug("QuitEmulator\n"));

	SDL_RemoveTimer(my_timer_id);

	// Exit 680x0 emulation
	Exit680x0();

	remove_floppy();

	// Deinitialize everything
	ExitAll();

	// Free ROM/RAM areas
	free(RAMBaseHost);

	SDL_VideoQuit();

	exit(0);
}


/*
 *  Code was patched, flush caches if neccessary (i.e. when using a real 680x0
 *  or a dynamically recompiling emulator)
 */

void FlushCodeCache(void *start, uint32 size)
{
}


/*
 * $Log$
 * Revision 1.28  2001/06/21 20:20:01  standa
 * XMOUSEHACK define removed. This problem was solved directly within the
 * SDL x11 driver. SDL_GrabInput() function was patched to put the
 * pointer_mode argument to XGrabInput() call as synchronous instead of
 * asynchronous. So we are waiting for SDL authors opinion.
 *
 * Revision 1.27  2001/06/18 20:02:50  standa
 * XMOUSEHACK define... just a test.
 *
 * Revision 1.26  2001/06/18 13:21:55  standa
 * Several template.cpp like comments were added.
 * HostScreen SDL encapsulation class.
 *
 *
 */
