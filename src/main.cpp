/*
 * main.cpp - startup/shutdown code
 *
 * Copyright (c) 2001-2004 Petr Stehlik of ARAnyM dev team (see AUTHORS)
 * 
 * Inspired by Christian Bauer's Basilisk II
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
#include "cpu_emulation.h"
#include "main.h"
#include "input.h"
#include "hardware.h"
#include "parameters.h"
#include "host.h"			// for the HostScreen
#include "aramd5.h"
#include "romdiff.h"
#include "parameters.h"
#include "version.h"		// for heartBeat
#include "ata.h"			// for init()
#ifdef ENABLE_LILO
#include "lilo.h"
#endif

#define DEBUG 1
#include "debug.h"

#ifdef HAVE_NEW_HEADERS
# include <cstdlib>
#else
# include <stdlib.h>
#endif

#include <SDL.h>

#ifdef SDL_GUI
#include "sdlgui.h"
extern bool start_GUI_thread();
extern void kill_GUI_thread();
#endif

#ifdef ETHERNET_SUPPORT
#include "natfeat/ethernet.h"
extern ETHERNETDriver Ethernet;
#endif

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
#ifdef OS_irix
void setactvdebug()
#else
void setactvdebug(int)
#endif
{
	grabMouse(false);

#ifdef DEBUGGER
	activate_debugger();
#endif
}

// CPU and FPU type, addressing mode
int CPUType;
bool CPUIs68060;
int FPUType;

// Timer stuff
static uint32 lastTicks;
#define USE_GETTICKS 1		// undefine this if your ARAnyM time goes slower

SDL_TimerID my_timer_id = NULL;

bool isGuiAvailable;

uint32 InterruptFlags = 0;
SDL_mutex *InterruptFlagLock;

void SetInterruptFlag(uint32 flag)
{
	if (SDL_LockMutex(InterruptFlagLock) == -1) {
		panicbug("Internal error! LockMutex returns -1.");
		abort();
	}
        InterruptFlags |= flag;
	if (SDL_UnlockMutex(InterruptFlagLock) == -1) {
		panicbug("Internal error! UnlockMutex returns -1.");
		abort();
	}
}

void ClearInterruptFlag(uint32 flag)
{
	if (SDL_LockMutex(InterruptFlagLock) == -1) {
		panicbug("Internal error! LockMutex returns -1.");
		abort();
	}
	InterruptFlags &= ~flag;
	if (SDL_UnlockMutex(InterruptFlagLock) == -1) {
		panicbug("Internal error! UnlockMutex returns -1.");
		abort();
	}
}

/*
 * called in VBL
 * indicates that the ARAnyM is alive and kicking
 */
void heartBeat()
{
	if (bx_options.video.fullscreen)
		return;	// think of different heart beat indicator

	static int vblCounter = 0;
	if (++vblCounter == 50) {
		vblCounter = 0;

		static char beats[] = "\\|/-\\|/-";
		static unsigned int beat_idx = 0;
		char buf[sizeof(VERSION_STRING)+128];
		sprintf(buf, "%s (press Pause key for GUI) %c", VERSION_STRING, beats[beat_idx++]);
		if (beat_idx == strlen(beats))
			beat_idx = 0;

		SDL_WM_SetCaption(buf, NULL);
	}
}

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
#if USE_GETTICKS
	uint32 newTicks = SDL_GetTicks();
#else
	struct timeval tv;
	gettimeofday(&tv, NULL);
	uint32 newTicks = tv.tv_sec * 1000 + tv.tv_usec / 1000;
#endif
	int count = (newTicks - lastTicks) / 5;	// miliseconds / 5 = 200 Hz
	if (count == 0)
		return;
	
#ifdef DEBUGGER
	if (!debugging || irqindebug)
#endif
		getMFP()->IRQ(5, count);

	lastTicks += (count * 5);

	VBL_counter += count;
	if (VBL_counter >= VBL_IN_TIMERC) {	// divided by 4 => 50 Hz VBL
		VBL_counter -= VBL_IN_TIMERC;

		heartBeat();

		// Thread safety patch
		hostScreen.lock();

		check_event();		// process keyboard and mouse events
		TriggerVBL();		// generate VBL

		if (++refreshCounter == VIDEL_REFRESH) {// divided by 2 again ==> 25 Hz screen update
			getVIDEL()->renderScreen();
			if (hostScreen.isGUIopen()) {
				static int blendRefresh = 0;
				if (blendRefresh++ > 5) {
					blendRefresh = 0;
					hostScreen.blendBackgrounds();
				}
			}
			refreshCounter = 0;
		}

		// Thread safety patch
		hostScreen.unlock();
	}
}

/*
 * my_callback_function() is called every 10 miliseconds (~ 100 Hz)
 */
Uint32 my_callback_function(Uint32 /*interval*/, void * /*param*/)
{
	TriggerInternalIRQ();
	return 10;					// come back in 10 milliseconds
}

/*
 * input_callback() is called every 20 ms and processes the input events
 * for the initial SDL GUI popup.
 */
Uint32 input_callback(Uint32 /*interval*/, void * /*param*/)
{
	check_event();	// process keyboard and mouse events
	return 20;		// 50 Hz
}

/*
 * Load, check and patch the TOS 4.04 ROM image file
 */
bool InitTOSROM(void)
{
	if (strlen(rom_path) == 0)
		return false;

	// read ROM file
	D(bug("Reading TOS image '%s'", rom_path));
	FILE *f = fopen(rom_path, "rb");
	if (f == NULL) {
		panicbug("TOS image '%s' not found.", rom_path);
		return false;
	}

	RealROMSize = 512 * 1024;
	if (fread(ROMBaseHost, 1, RealROMSize, f) != (size_t)RealROMSize) {
		panicbug("TOS image '%s' reading error.\nMake sure the file is readable and its size is 524288 bytes (512 kB).", rom_path);
		fclose(f);
		return false;
	}
	fclose(f);

	// check if this is the correct 68040 aware TOS ROM version
	D(bug("Checking TOS version.."));
	unsigned char TOS404[16] = {0xe5,0xea,0x0f,0x21,0x6f,0xb4,0x46,0xf1,0xc4,0xa4,0xf4,0x76,0xbc,0x5f,0x03,0xd4};
	MD5 md5;
	unsigned char loadedTOS[16];
	md5.computeSum(ROMBaseHost, RealROMSize, loadedTOS);
	if (memcmp(loadedTOS, TOS404, 16) == 0) {
		// patch it for 68040 compatibility
		D(bug("Patching TOS 4.04 for 68040 compatibility.."));
		int ptr, i=0;
		while((ptr=tosdiff[i].offset) >= 0)
			ROMBaseHost[ptr] = tosdiff[i++].newvalue;
	}
	else {
		panicbug("Wrong TOS version. You need the original TOS 4.04.");
		return false;
	}

	// patch cookies
	// _MCH
	ROMBaseHost[0x00416] = bx_options.tos.cookie_mch >> 24;
	ROMBaseHost[0x00417] = (bx_options.tos.cookie_mch >> 16) & 0xff;
	ROMBaseHost[0x00418] = (bx_options.tos.cookie_mch >> 8) & 0xff;
	ROMBaseHost[0x00419] = (bx_options.tos.cookie_mch) & 0xff;
	// _SND
#if DSP_EMULATION
	ROMBaseHost[0x00437] = 0x0C;	/* DSP emulation and XBIOS routines */
#else
	ROMBaseHost[0x00437] = 0x04;	/* no hardware, only XBIOS routines */
#endif

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
	if (bx_options.tos.redirect_CON) {
		ROMBaseHost[0x8d44] = ROMBaseHost[0x8d50] = 0x71;
		ROMBaseHost[0x8d45] = ROMBaseHost[0x8d51] = 0x2a;
		ROMBaseHost[0x8d46] = ROMBaseHost[0x8d52] = 0x4e;
		ROMBaseHost[0x8d47] = ROMBaseHost[0x8d53] = 0x75;
	}
	if (bx_options.tos.redirect_PRT) {
		ROMBaseHost[0x23ec] = 0x32;
		ROMBaseHost[0x23ed] = 0x2f;
		ROMBaseHost[0x23ee] = 0x00;
		ROMBaseHost[0x23ef] = 0x06;
		ROMBaseHost[0x23f0] = 0x71;
		ROMBaseHost[0x23f1] = 0x2a;
		ROMBaseHost[0x23f2] = 0x4e;
		ROMBaseHost[0x23f3] = 0x75;
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

	infoprint("TOS 4.04 loading... [OK]");
	return true;
}

/*
 * Load EmuTOS image file
 */
bool InitEmuTOS(void)
{
	if (strlen(emutos_path) == 0)
		return false;

	// read EmuTOS file
	D(bug("Reading EmuTOS from '%s'", emutos_path));
	FILE *f = fopen(emutos_path, "rb");
	if (f == NULL) {
		panicbug("EmuTOS image '%s' not found.", emutos_path);
		return false;
	}
	RealROMSize = 512 * 1024;
	bool bEmuOK = (fread(ROMBaseHost, 1, RealROMSize, f) > 0);
	fclose(f);
	if (bEmuOK) {
		infoprint("EmuTOS %02x%02x/%02x/%02x loading from '%s'... [OK]",
			ROMBaseHost[0x18], ROMBaseHost[0x19],
			ROMBaseHost[0x1a],
			ROMBaseHost[0x1b],
			emutos_path);
	}
	else
		panicbug("EmuTOS image '%s' reading error.", rom_path);
	return bEmuOK;
}

/*
 * Initialize the Operating System - Linux, TOS 4.04 or EmuTOS
 */
bool InitOS(void)
{
	/*
	 * First try to boot a linux kernel if enabled,
	 * then try TOS 4.04 if EmuTOS is disabled,
	 * then finally try EmuTOS.
	 * Note that EmuTOS should always be available so this will be
	 * a nice fallback.
	 */
#ifdef ENABLE_LILO
	if (boot_lilo && LiloInit())
		return true;
#endif

	bool isOS = false;
	if (!boot_emutos && InitTOSROM())
		isOS = true;
	else if (InitEmuTOS())
		isOS = true;

	if (isOS) {
		// Setting "SP & PC" for TOS and EmuTOS
		for (int i = 0; i < 8; i++)
			RAMBaseHost[i] = ROMBaseHost[i];
		return true;
	}

	panicbug("No operating system found. ARAnyM can not boot!");
	panicbug("Visit http://emutos.sourceforge.net/ and get your copy of EmuTOS now.");
	return false;
}


/*
 *  Initialize everything, returns false on error
 */

bool InitAll(void)
{
#ifndef NOT_MALLOC
	if (ROMBaseHost == NULL) {
		if ((RAMBaseHost = (uint8 *)malloc(RAMSize + ROMSize + HWSize + FastRAMSize)) == NULL) {
			panicbug("Not enough free memory.");
			return false;
		}
		ROMBaseHost = (uint8 *)(RAMBaseHost + ROMBase);
		HWBaseHost = (uint8 *)(RAMBaseHost + HWBase);
		FastRAMBaseHost = (uint8 *)(RAMBaseHost + FastRAMBase);
	}
#endif

	if (!InitMEM())
		return false;

	if (! InitOS())
		return false;

 	int sdlInitParams = SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK | SDL_INIT_TIMER;
#if NFCDROM_SUPPORT
	sdlInitParams |= SDL_INIT_CDROM;
#endif
	if (SDL_Init(sdlInitParams) != 0) {
		panicbug("SDL initialization failed.");
		return false;
	}
	atexit(SDL_Quit);

	// check video output device and if it's a framebuffer
	// then enforce full screen
	char driverName[32];
	SDL_VideoDriverName( driverName, sizeof(driverName)-1 );
	D(bug("Video driver name: %s", driverName));
	if ( strstr( driverName, "fb" ) )		// fullscreen on framebuffer
		bx_options.video.fullscreen = true;

	// Check if at least one joystick present, open it
	if (SDL_NumJoysticks()>0) {
		sdl_joystick=SDL_JoystickOpen(0);
		if (!sdl_joystick) {
			panicbug("Could not open joystick #0");
		}
	}

	// For InterruptFlag controling
	InterruptFlagLock = SDL_CreateMutex();

	CPUType = 4;
	FPUType = 1;

#ifdef ETHERNET_SUPPORT
	Ethernet.init();
#endif

	// Init HW
	HWInit();

	InputInit();

	// Init 680x0 emulation
	if (!Init680x0())
		return false;

#ifdef SDL_GUI
	isGuiAvailable = SDLGui_Init();

	if (isGuiAvailable && startupGUI) {
		start_GUI_thread();
		do {
			hostScreen.lock();
			check_event();			// process mouse & keyboard events
			hostScreen.unlock();

			SDL_Delay(20);			// 50 Hz input events rate is OK
		}
		while(hostScreen.isGUIopen());
	}
#endif

#ifdef DEBUGGER
	if (bx_options.startup.debugger) {
		D(bug("Activate debugger..."));
		activate_debugger();
	}
#endif

	// timer init
#if USE_GETTICKS
	lastTicks = SDL_GetTicks();
#else
	struct timeval tv;
	gettimeofday(&tv, NULL);
	lastTicks = tv.tv_sec * 1000 + tv.tv_usec / 1000;
#endif

	my_timer_id = SDL_AddTimer(10, my_callback_function, NULL);
	if (my_timer_id == NULL) {
		panicbug("SDL Timer does not work!");
		return false;
	}

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
#ifdef ENABLE_LILO
	if (boot_lilo) {
		LiloShutdown();
	}
#endif

#ifdef ETHERNET_SUPPORT
	Ethernet.exit();
#endif

 	/* Close opened joystick */
 	if (SDL_NumJoysticks()>0) {
 		if (SDL_JoystickOpened(0)) {
 			SDL_JoystickClose(sdl_joystick);
 		}
 	}

	// Exit Time Manager
	if (my_timer_id) {
		SDL_RemoveTimer(my_timer_id);
		my_timer_id = NULL;
		SDL_Delay(100);	// give it a time to safely finish the timer thread
	}

#ifdef SDL_GUI
	kill_GUI_thread();
	SDLGui_UnInit();
#endif

	// hardware
	HWExit();

#if ENABLE_MON
	// Deinitialize mon
	mon_exit();
#endif

	SDL_VideoQuit();
}

void RestartAll()
{
	// memory init

	// HW init
	getIDE()->init();

	// OS init
	InitOS();

	// CPU init
	Restart680x0();
}
