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
#include <dirent.h>		// for extFS

#include "cpu_emulation.h"
#include "main.h"
#include "hardware.h"
#include "parameters.h"

#include "extfs.h"
#include "emul_op.h" // for the extFS

#define DEBUG 1
#include "debug.h"

#define METADOS_DRV
#define CONVPLANES
#ifdef CONVPLANES
// Temporary color palette table...
#include "../colorpalette.cpp"

char   tos_colors[] = {0, 255, 1, 2, 4, 6, 3, 5, 7,  8,  9, 10, 12, 14, 11, 13};
char   vdi_colors[] = {0,   2, 3, 6, 4, 7, 5, 8, 9, 10, 11, 14, 12, 15, 13, 1};
uint32 sdl_colors[256];

#endif // CONVPLANES

// Constants
const char ROM_FILE_NAME[] = DATADIR"/ROM";
const int SIG_STACK_SIZE = SIGSTKSZ;	// Size of signal stack

// CPU and FPU type, addressing mode
int CPUType;
int FPUType;

SDL_TimerID my_timer_id;

static uint32 mapped_ram_rom_size;		// Total size of mmap()ed RAM/ROM area

void init_fdc();	// fdc.cpp

extern int irqindebug;

bool grab_mouse = false;

SDL_Surface *surf = NULL;

#define UPDATERECT
#ifdef UPDATERECT
bool UpdateScreen = true;
#define REFRESH_FREQ	1
#define COPYVRAM
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
/*60-67*/SDLK_LESS, SDLK_PAGEDOWN, SDLK_PAGEUP, 0 /* NumLock */, SDLK_KP_DIVIDE, SDLK_KP_MULTIPLY, SDLK_KP_MINUS, SDLK_KP7,
/*68-6f*/SDLK_KP8, SDLK_KP9, SDLK_KP4, SDLK_KP5, SDLK_KP6, SDLK_KP1, SDLK_KP2, SDLK_KP3,
/*70-72*/SDLK_KP0, SDLK_KP_PERIOD, SDLK_KP_ENTER};

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
			if (sym == SDLK_RCTRL)
				sym = SDLK_LCTRL;
			if (sym == SDLK_RALT)
				sym = SDLK_LALT;
			for(int i=0; i < 0x73; i++) {
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
				if (xrel < -127 || xrel > 127)
					xrel = 0;
				yrel = eve.yrel;
				if (yrel < -127 || yrel > 127)
					yrel = 0;
			}
			else if (type == SDL_MOUSEBUTTONDOWN) {
			// eve.type/state/button
				if (event.button.button == SDL_BUTTON_RIGHT)
					but |= 1;
				else if (event.button.button == SDL_BUTTON_LEFT)
					but |= 2;
			}
			else if (type == SDL_MOUSEBUTTONUP) {
				if (event.button.button == SDL_BUTTON_RIGHT)
					but &= ~1;
				else if (event.button.button == SDL_BUTTON_LEFT)
					but &= ~2;
			}
			if (xrel || yrel || lastbut != but) {
				ikbd_send(0xf8 | but);
				ikbd_send(xrel);
				ikbd_send(yrel);
				// fprintf(stderr, "Mouse: %dx%d\t", xrel, yrel);
			}
		}
/*
		else if (event.type == SDL_ACTIVEEVENT) {
			if (event.state == SDL_APPMOUSEFOCUS) {
				if (event.gain) {
					// mouse entered our window -> update the Atari mouse position

				}
			}
		}
*/
		else if (event.type == SDL_QUIT)
			QuitEmulator();
	}
}

void update_screen() {
#ifdef COPYVRAM
			if (SDL_MUSTLOCK(surf))
				if (SDL_LockSurface(surf)<0) {
					printf("Couldn't lock surface to refresh!\n");
					return;
			}

			VideoRAMBaseHost = (uint8 *)surf->pixels;
			uint16 *fvram = (uint16*)get_real_address_direct(vram_addr/*ReadMacInt32(0x44e)*/);
			uint16 *hvram = (uint16 *)VideoRAMBaseHost;
			int mode = getVideoMode();

			uint8 destBPP  = surf->format->BytesPerPixel;
			if (mode < 16) {
			    //BEGIN
			    //
			    // FIXME:
			    // This should be done ONLY when the screen mode
			    // changes... e.g. in the videl emulation
			    uint8  col1st = 1;
			    uint8  col3rd = 3;

			    if (mode == 1) // set the black in 1 plane mode to the index 1
				col1st = 15;
			    else if (mode == 2) // set the black to the index 3 in 2 plane mode (4 colors)
			        col3rd = 15;

			    sdl_colors[1] = SDL_MapRGB(surf->format,
						       (uint8)colors[vdi_colors[col1st]*3],
						       (uint8)colors[vdi_colors[col1st]*3+1],
						       (uint8)colors[vdi_colors[col1st]*3+2]);
			    sdl_colors[3] = SDL_MapRGB(surf->format,
						       (uint8)colors[vdi_colors[col3rd]*3],
						       (uint8)colors[vdi_colors[col3rd]*3+1],
						       (uint8)colors[vdi_colors[col3rd]*3+2]);
			    //
			    //END

			    // The SDL colors blitting...
			    // FIXME: The destBPP tests should probably not
			    //        be inside the loop... (reorganise?)
			    uint16 words[mode];
			    for(int i=0; i < (80*480/2); i++) {
			            // perform byteswap (prior to plane to chunky conversion)
			            for(int l=0; l<mode; l++) {
				        uint16 b = fvram[i*mode+l];
					words[l] = (b >> 8) | ((b & 0xff) << 8);	// byteswap
				    }

				    // To support other formats see
				    // the SDL video examples (docs 1.1.7)
				    if (destBPP==4) {
				      // bitplane to chunky conversion
				      for(int j=0; j<16; j++) {
					uint8 color = 0;
					for(int l=mode-1; l>=0; l--) {
					  color <<= 1;
					  color |= (words[l] >> (15-j)) & 1;
					}
					((uint32*)hvram)[(i*16+j)] = (uint32)sdl_colors[color];
				      }
				    } else if (destBPP==2) {
				      // bitplane to chunky conversion
				      for(int j=0; j<16; j++) {
					uint8 color = 0;
					for(int l=mode-1; l>=0; l--) {
					  color <<= 1;
					  color |= (words[l] >> (15-j)) & 1;
					}
					((uint16*)hvram)[(i*16+j)] = (uint16)sdl_colors[color];
				      }
				    } // FIXME: support for othe than 2 or 4 BPP is missing
			    }


			    /*
			    for(int i=0; i < (80*480/2); i++) {
				    for(int j=0; j<16; j++) {
				        uint8 color;
					color = 0;
				        for(int l=mode-1; l>=0; l--) {
					  uint16 b = fvram[i*mode+l];
					  b = (b >> 8) | ((b & 0xff) << 8);	// byteswap
					  color <<= 1;
					  color |= (b >> (15-j)) & 1;
					}
					hvram[(i*16+j)] = (uint16)sdl_colors[color];
				    }
			    }
			    */

			} else {
			  // Falcon TC (High Color)
			    if (destBPP==2) {
				// This is not enough (memcpy)
			        // memcpy(hvram, fvram, 640*480*2);

			        for(int i=0; i < 640*480; i++)
				      // byteswap
				    ((uint16*)hvram)[i] = 
				      (((uint16*)fvram)[i] >> 8) | ((((uint16*)fvram)[i] & 0xff) << 8);

			    } else if (destBPP==4) {
			        for(int i=0; i < 640*480; i++)
				    // The byteswap is done by correct shifts (not so obvious) 
			            ((uint32*)hvram)[i] = SDL_MapRGB(
						 surf->format,
						 (uint8)((fvram[i]>>5) & 0xf8),
						 (uint8)(((fvram[i] & 0x07) << 5) | ((fvram[i]>>11) & 0x3c)),
						 (uint8)(fvram[i] & 0xf8) );
			    } // FIXME: support for othe than 2 or 4 BPP is missing

#if 0
			  SDL_Rect fvrect = { 0, 0, 640, 480 };
			  //			  fvrect.x = fvrect.y = 0;
			  //			  fvrect.w = 640;
			  //			  fvrect.h = 480;

			  SDL_Surface *fvsurf = SDL_CreateRGBSurfaceFrom(
				 (void*)fvram, 640, 480, 16, 640*2, 0xf800, 0x07e0, 0x001f, 0x0000 );
			  SDL_SetColorKey(fvsurf, 0, 0);
			  SDL_SetAlpha(fvsurf, 0, 0);
			
			  SDL_BlitSurface(fvsurf, &fvrect, surf, &fvrect);
			  SDL_FreeSurface(fvsurf);
#endif
			}
			        //for(int i=0; i < 640*480*2; i++) hvram[i] = 0xffff - fvram[i];
#endif	// COPYVRAM

			SDL_UpdateRect(SDL_GetVideoSurface(), 0, 0, 640, 480);
}

Uint32 my_callback_function(Uint32 interval, void *param)
{
	static int VBL_counter = 0;
	static int Refresh_counter = 0;

	if (!debugging || irqindebug)
		MakeMFPIRQ(5);

	if ((VBL_counter % 2) == 1) {
		if (!debugging || irqindebug) {
			TriggerVBL();
			check_event();	// check keyboard/mouse 100 times per second
		}
	}
	if (++VBL_counter == 4) {
		VBL_counter = 0;

#ifdef UPDATERECT
		if (UpdateScreen && ++Refresh_counter == REFRESH_FREQ)
		{
			Refresh_counter = 0;
			update_screen();
		}
#endif	// UPDATERECT
	}
	return 5;	// come back in 5 miliseconds, if possible
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

	return 0;
}

/*
 *  Main program
 */
#define ErrorAlert(a)	fprintf(stderr, a)
#define MAXDRIVES	32
int drive_fd[MAXDRIVES];

int main(int argc, char **argv)
{
	char str[256];

	program_name = argv[0];
	int i = decode_switches (argc, argv);

#ifdef METADOS_DRV
	// install the drives
	extFS.install( 'M', "/home/atari", true );
	extFS.install( 'N', "/home/standa", false );
#endif // METADOS_DRV

	if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER) != 0) {
		fprintf(stderr, "SDL initialization failed.\n");
		return 1;
	}
	atexit(SDL_Quit);
	SelectVideoMode();
	int sdl_videoparams = SDL_HWSURFACE;
	if (fullscreen)
		sdl_videoparams |= SDL_FULLSCREEN;
	surf = SDL_SetVideoMode(640, 480, 16, sdl_videoparams);
	SDL_WM_SetCaption("aranym pre-alpha", "ARAnyM");
	fprintf(stderr, "Line Length = %d\n", surf->pitch);
	fprintf(stderr, "Must Lock? %s\n", SDL_MUSTLOCK(surf) ? "YES" : "NO");
	if (SDL_MUSTLOCK(surf)) {
		SDL_LockSurface(surf);
#ifdef UPDATERECT
		UpdateScreen = true;
#endif // UPDATERECT
	}

	// grab mouse
	if (grab_mouse) {
		SDL_ShowCursor(SDL_DISABLE);
		SDL_WM_GrabInput(SDL_GRAB_ON);
	}

	VideoRAMBaseHost = (uint8 *)surf->pixels;
	fprintf(stderr, "surf->pixels = %x, getVideoSurface() = %x\n", VideoRAMBaseHost, SDL_GetVideoSurface()->pixels);
	if (SDL_MUSTLOCK(surf))
		SDL_UnlockSurface(surf);

	fprintf(stderr,
		"Pixel format: \tmasks  r %04x, g %04x, b %04x\n"
		"\t\tshifts r %d, g %d, b %d\n"
		"\t\tlosses r %d, g %d, b %d\n",
		surf->format->Rmask,
		surf->format->Gmask,
		surf->format->Bmask,
		surf->format->Rshift,
		surf->format->Gshift,
		surf->format->Bshift,
		surf->format->Rloss,
		surf->format->Gloss,
		surf->format->Bloss );

#ifdef CONVPLANES
	// Convert the table from 0..1000 RGB to 0..255 RBG format
	for(int i=0; i < sizeof(colors)/sizeof(*colors); i++)
	  colors[i] = ((( (uint16)colors[i] << 6 ) - 63) / 250) & 0xff;

	// Prepare the native format color values
	//
	// FIXME: This should be done on every VDI palette change
	//        and this pass should work with TOS built in palette
	//        (I don't know where to get the TOS palette, so I've stolen it from fVDI)
	{
	  uint16 vdi2pix[256];
	  int  i = 0;

	  for(i = 0; i < 16; i++)
	      vdi2pix[i] = vdi_colors[i];
	  for(; i < 256; i++)
	      vdi2pix[i] = i;

	  // map the colortable into the correct pixel format
	  for(int i=0; i < 256; i++) {
	      fprintf(stderr, "map color %03d -> %03d (#%02x%02x%02x)\n",
		      (uint8)i, (uint8)vdi2pix[i],
		      (uint8)colors[vdi2pix[i]*3],
		      (uint8)colors[vdi2pix[i]*3+1],
		      (uint8)colors[vdi2pix[i]*3+2] );

	      sdl_colors[i] = SDL_MapRGB(surf->format,
					 (uint8)colors[vdi2pix[i]*3],
					 (uint8)colors[vdi2pix[i]*3+1],
					 (uint8)colors[vdi2pix[i]*3+2]);
	  }
	}

#if 0
	// These are really needed values for 32bit
	// (to not to scan for transparent pixels & alpha chanelling)
	SDL_SetColorKey(surf, 0, 0);
	SDL_SetAlpha(surf, 0, 0);
#endif
#endif
	drive_fd[0] = drive_fd[1] = drive_fd[2] = -1;

	drive_fd[0] = open("/dev/fd0", O_RDONLY);
	if (drive_fd[0] >= 0)
    	init_fdc();

	// Initialize variables
	RAMBaseHost = NULL;
	ROMBaseHost = NULL;
	//TTRAMBaseHost = NULL;
	srand(time(NULL));
	tzset();

	// Create areas for Atari ST-RAM, ROM, HW registers and TT-RAM
	int MB = (1 << 20);
	RAMBaseHost = (uint8 *)malloc((16+32)*MB);
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
	InitVMEMBaseDiff(VideoRAMBaseHost, VideoRAMBase);
	D(bug("ST-RAM starts at %p (%08x)\n", RAMBaseHost, RAMBase));
	D(bug("TOS ROM starts at %p (%08x)\n", ROMBaseHost, ROMBase));
	//D(bug("TT-RAM starts at %p (%08x)\n", TTRAMBaseHost, TTRAMBase));
	D(bug("VideoRAM starts at %p (%08x)\n", VideoRAMBaseHost, VideoRAMBase));
	// Get rom file path from preferences
//	const char *rom_path = "ROM";

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

#ifndef COPYVRAM
	// Patch TOS (enforce VideoRAM at 0xf0000000)
	ROMBaseHost[35752]=0x2e;
	ROMBaseHost[35753]=0x3c;
   	ROMBaseHost[35754]=0xf0;
   	ROMBaseHost[35755]=0;
   	ROMBaseHost[35756]=0;
   	ROMBaseHost[35757]=0;
   	ROMBaseHost[35758]=0x60;
   	ROMBaseHost[35759]=6;
   	ROMBaseHost[35760]=0x4e;
   	ROMBaseHost[35761]=0x71;
#endif

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
	free(RAMBaseHost);

	exit(0);
}


/*
 *  Code was patched, flush caches if neccessary (i.e. when using a real 680x0
 *  or a dynamically recompiling emulator)
 */

void FlushCodeCache(void *start, uint32 size)
{
}
