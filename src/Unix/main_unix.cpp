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
#include <errno.h>

#include <SDL/SDL.h>
#include <sys/mman.h>

#include "cpu_emulation.h"
#include "timer.h"
#include "version.h"
#include "main.h"

#define DEBUG 1
#include "debug.h"

// Constants
const char ROM_FILE_NAME[] = "ROM";
const int SIG_STACK_SIZE = SIGSTKSZ;	// Size of signal stack

// CPU and FPU type, addressing mode
int CPUType;
int FPUType;

static int zero_fd = -1;				// FD of /dev/zero

SDL_TimerID my_timer_id;

static uint32 mapped_ram_rom_size;		// Total size of mmap()ed RAM/ROM area

void init_fdc();	// fdc.cpp

extern int irqindebug;

#define UPDATERECT
#ifdef UPDATERECT
bool UpdateScreen = true;
bool convert_bitplanes = false;
#define REFRESH_FREQ	1
#endif // UPDATERECT

static int keyboardTable[0x80] = {
/* 0-7 */0, SDLK_ESCAPE, SDLK_1, SDLK_2, SDLK_3, SDLK_4, SDLK_5, SDLK_6,
/* 8-f */SDLK_7, SDLK_8, SDLK_9, SDLK_0, SDLK_EQUALS, SDLK_QUOTE, SDLK_BACKSPACE, SDLK_TAB,
/*10-17*/SDLK_q, SDLK_w, SDLK_e, SDLK_r, SDLK_t, SDLK_y, SDLK_u, SDLK_i,
/*18-1f*/SDLK_o, SDLK_p, SDLK_LEFTPAREN, SDLK_RIGHTPAREN, SDLK_RETURN, SDLK_LCTRL, SDLK_a, SDLK_s,
/*20-27*/SDLK_d, SDLK_f, SDLK_g, SDLK_h, SDLK_j, SDLK_k, SDLK_l, SDLK_SEMICOLON,
/*28-2f*/SDLK_QUOTE, SDLK_HASH, SDLK_LSHIFT, SDLK_BACKQUOTE, SDLK_z, SDLK_x, SDLK_c, SDLK_v,
/*30-37*/SDLK_b, SDLK_n, SDLK_m, SDLK_COMMA, SDLK_PERIOD, SDLK_SLASH, SDLK_RSHIFT, 0,
/*38-3f*/SDLK_LALT, SDLK_SPACE, SDLK_CAPSLOCK, SDLK_F1, SDLK_F2, SDLK_F3, SDLK_F4, SDLK_F5,
/*40-47*/SDLK_F6, SDLK_F7, SDLK_F8, SDLK_F9, SDLK_F10, 0, 0, SDLK_HOME,
/*48-4f*/SDLK_UP, 0, SDLK_KP_PLUS, SDLK_LEFT, 0, SDLK_RIGHT, SDLK_KP_MINUS, 0,
/*50-57*/SDLK_DOWN, 0, SDLK_INSERT, SDLK_DELETE, SDLK_F11, SDLK_F12, 0, 0,
/*58-5f*/0, 0, 0, 0, 0, 0, 0, 0,
/*60-67*/SDLK_LESS, SDLK_PAGEDOWN, SDLK_PAGEUP};

static int buttons[3]={0,0,0};

static void check_event(void)
{
	SDL_Event event;
	while(SDL_PollEvent(&event)) {
		int type = event.type;
		if (type == SDL_KEYDOWN || type == SDL_KEYUP) {
			bool pressed = (type == SDL_KEYDOWN);
			int sym = event.key.keysym.sym;
			if (sym == SDLK_END)
				QuitEmulator();
			for(int i=0; i < 0x62; i++) {
				if (keyboardTable[i] == sym) {
					if (! pressed)
						i |= 0x80;
					ikbd_send(i);
					break;
				}
			}
		}
		else if (type == SDL_MOUSEBUTTONDOWN || type == SDL_MOUSEBUTTONUP || type == SDL_MOUSEMOTION) {
			int xrel = 0;
			int yrel = 0;
			static int but = 0;
			int lastbut = but;
			if (type == SDL_MOUSEMOTION) {
				SDL_MouseMotionEvent eve = event.motion;
				//eve.type/state/x,y/xrel,yrel
				xrel = eve.xrel;
				yrel = eve.yrel;
			}
			else if (type == SDL_MOUSEBUTTONDOWN) {
			// eve.type/state/button
				but = 1;
			}
			else if (type == SDL_MOUSEBUTTONUP) {
				but = 0;
			}
			if (xrel || yrel || lastbut != but) {
				ikbd_send(0xf8 | but << 1);
				ikbd_send(xrel);
				ikbd_send(yrel);
			}
		}
		else if (event.type == SDL_QUIT)
			QuitEmulator();
	}
}

Uint32 my_callback_function(Uint32 interval, void *param)
{
	static int VBL_counter = 0;
	static int Refresh_counter = 0;

	if (!debugging || irqindebug)
		MakeMFPIRQ(5);

	if (++VBL_counter == 4) {
		VBL_counter = 0;

		if (!debugging || irqindebug) {
			TriggerVBL();
			check_event();
		}
#ifdef UPDATERECT
		if (UpdateScreen && ++Refresh_counter == REFRESH_FREQ)
		{
			Refresh_counter = 0;
			// memcpy(VideoRAMBaseHost, RAMBaseHost+0x369f00, 640*480*2);
			// convert bitplanes
			if (convert_bitplanes) {
				uint8 *fvram = get_real_address(0x800000); //HWget_l(0x44e);
				uint16 *hvram = (uint16 *)VideoRAMBaseHost;
				for(int i=0; i < (80*480); i++) {
					uint8 b = fvram[i];
					for(int j=0; j<8; j++) {
						uint16 v = 0xffff;
						if (b & (1 << (7-j)))
							v = 0;
						hvram[(i*8+j)] = v;
					}
				}
			}
			SDL_UpdateRect(SDL_GetVideoSurface(), 0, 0, 640, 480);
		}
#endif
	}
	return 5;
}

/*
 *  Ersatz functions
 */

int SelectVideoMode() {
	SDL_Rect **modes;
	int i;

	/* Get available fullscreen/hardware modes */
	modes=SDL_ListModes(NULL, SDL_FULLSCREEN|SDL_HWSURFACE);

	/* Check is there are any modes available */
	if(modes == (SDL_Rect **)0){
		printf("No modes available!\n");
		exit(-1);
	}

	/* Check if or resolution is restricted */
	if(modes == (SDL_Rect **)-1){
		printf("All resolutions available.\n");
	}
	else {
		/* Print valid modes */
		printf("Available Modes\n");
		for(i=0;modes[i];++i)
			printf("  %d x %d\n", modes[i]->w, modes[i]->h);
	}
}

/*
 *  Main program
 */
#define ErrorAlert(a)	fprintf(stderr, a)
#define MAXDRIVES	32
int drive_fd[MAXDRIVES];
SDL_Surface *surf = NULL;

int main(int argc, char **argv)
{
	char str[256];

	if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER) != 0)
		return 1;
	atexit(SDL_Quit);
	SelectVideoMode();
	surf = SDL_SetVideoMode(640, 480, 16, /*SDL_FULLSCREEN|*/SDL_HWSURFACE);
	SDL_WM_SetCaption("aranym pre-alpha", "ARAnyM");
	fprintf(stderr, "Line Length = %d\n", surf->pitch);
	fprintf(stderr, "Must Lock? %s\n", SDL_MUSTLOCK(surf) ? "YES" : "NO");
	if (SDL_MUSTLOCK(surf)) {
		SDL_LockSurface(surf);
#ifdef UPDATERECT
		UpdateScreen = true;
#endif // UPDATERECT
	}
	VideoRAMBaseHost = (uint8 *)surf->pixels;
	fprintf(stderr, "surf->pixels = %x, getVideoSurface() = %x\n", VideoRAMBaseHost, SDL_GetVideoSurface()->pixels);
	if (SDL_MUSTLOCK(surf))
		SDL_UnlockSurface(surf);

	drive_fd[0] = drive_fd[1] = drive_fd[2] = -1;

	drive_fd[0] = open("/dev/fd0", O_RDONLY);
	if (drive_fd[0] >= 0)
    	init_fdc();

	// Initialize variables
	RAMBaseHost = NULL;
	ROMBaseHost = NULL;
	srand(time(NULL));
	tzset();

	// Open /dev/zero
	zero_fd = open("/dev/zero", O_RDWR);
	if (zero_fd < 0) {
		sprintf(str, "Couldn't open /dev/zero\n", strerror(errno));
		ErrorAlert(str);
		QuitEmulator();
	}

	// Read RAM size
	RAMSize = 14 * 1024 * 1024;	// Round down to 1MB boundary
	if (RAMSize < 1024*1024) {
		RAMSize = 1024*1024;
	}

	const uint32 page_size = getpagesize();
	const uint32 page_mask = page_size - 1;
	const uint32 aligned_ram_size = (RAMSize + page_mask) & ~page_mask;
	mapped_ram_rom_size = aligned_ram_size + 0x100000 + 0x100000;

	// Create areas for Mac RAM and ROM
	// gb-- Overkill, needs to be cleaned up. Probably explode it for either
	// real or direct addressing mode.
	RAMBaseHost = (uint8 *)mmap(0, mapped_ram_rom_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, zero_fd, 0);
	if (RAMBaseHost == (uint8 *)MAP_FAILED) {
		ErrorAlert("Not enough memory\n");
		QuitEmulator();
	}
	ROMBaseHost = RAMBaseHost + aligned_ram_size;
	// ROMBaseHost = RAMBaseHost + 0xe00000;

	// Initialize MEMBaseDiff now so that Host2MacAddr in the Video module
	// will return correct results
	RAMBase = 0;
	ROMBase = RAMBase + aligned_ram_size;
	// ROMBase = RAMBase + 0xe00000;

	InitMEMBaseDiff(RAMBaseHost, RAMBase);
	InitVMEMBaseDiff(VideoRAMBaseHost, VideoRAMBase);
	D(bug("ST-RAM starts at %p (%08x)\n", RAMBaseHost, RAMBase));
	D(bug("TOS ROM starts at %p (%08x)\n", ROMBaseHost, ROMBase));
	D(bug("VideoRAM starts at %p (%08x)\n", VideoRAMBaseHost, VideoRAMBase));
	fprintf(stderr, "Physical VRAM = %x\n", VideoRAMBaseHost);
	
	// Get rom file path from preferences
	const char *rom_path = "ROM";

	// Load TOS ROM
	int rom_fd = open(rom_path ? rom_path : ROM_FILE_NAME, O_RDONLY);
	if (rom_fd < 0) {
		ErrorAlert("ROM file not found\n");
		QuitEmulator();
	}
	printf("Reading ROM file...\n");
	ROMSize = lseek(rom_fd, 0, SEEK_END);
	if (ROMSize != 512*1024) {
		ErrorAlert("Invalid ROM size\n");
		close(rom_fd);
		QuitEmulator();
	}
	lseek(rom_fd, 0, SEEK_SET);
	if (read(rom_fd, ROMBaseHost, ROMSize) != (ssize_t)ROMSize) {
		ErrorAlert("ROM file reading error\n");
		close(rom_fd);
		QuitEmulator();
	}

	// Patch TOS

	// Initialize everything
	if (!InitAll())
		QuitEmulator();
	D(bug("Initialization complete\n"));

	my_timer_id = SDL_AddTimer(5, my_callback_function, NULL);

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

	// Exit 680x0 emulation
	Exit680x0();

	SDL_RemoveTimer(my_timer_id);

	// Deinitialize everything
	ExitAll();

	// Free ROM/RAM areas
	if (RAMBaseHost != (uint8 *)MAP_FAILED) {
		munmap((caddr_t)RAMBaseHost, mapped_ram_rom_size);
		RAMBaseHost = NULL;
	}

	// Close /dev/zero
	if (zero_fd > 0)
		close(zero_fd);

	exit(0);
}


/*
 *  Code was patched, flush caches if neccessary (i.e. when using a real 680x0
 *  or a dynamically recompiling emulator)
 */

void FlushCodeCache(void *start, uint32 size)
{
}
