/*
 * main.cpp - startup/shutdown code
 *
 * Copyright (c) 2001-2006 Petr Stehlik of ARAnyM dev team (see AUTHORS)
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


#if defined(OS_cygwin)
// HACK: cygwin/mingw32 mix crap (needs to be the first SDL header included)
//       if not present the SDL_putenv uses cygwin implementation however
//       the video driver reading it uses SDL_getenv which comes from the SDL
//       build using mingw32...
# define _WIN32 1
# include <SDL_getenv.h>
# undef _WIN32
#endif

#include "sysdeps.h"
#include "cpu_emulation.h"
#include "main.h"
#include "input.h"
#include "hardware.h"
#include "parameters.h"
#include "host.h"			// for the HostScreen
#include "parameters.h"
#ifdef HEARTBEAT
#  include "version.h"		// for heartBeat
#endif
#include "natfeat/nf_objs.h"
#include "bootos_tos.h"
#include "bootos_emutos.h"
#include "bootos_linux.h"
#include "aranym_exception.h"

#define DEBUG 0
#include "debug.h"

#ifdef HAVE_NEW_HEADERS
# include <cstdlib>
#else
# include <stdlib.h>
#endif

#if RTC_TIMER
#include <linux/rtc.h>
#include <errno.h>
#endif

#include <SDL.h>

// hack for SDL < 1.2.10 - remove when distros upgrade their SDL!
#ifndef OS_cygwin
	#ifndef SDL_putenv
		#define SDL_putenv(x) putenv(x)
	#endif
#endif

#ifdef SDL_GUI
#include "sdlgui.h"
extern char *displayKeysym(SDL_keysym keysym, char *buffer);
#endif

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
static uint32 lastTicks = 0;

#if RTC_TIMER
static SDL_Thread *RTCthread = NULL;
static bool quit_rtc_loop = false;
#else
SDL_TimerID my_timer_id = NULL;
#endif

#if DEBUG
static int early_interrupts = 0;
static int multiple_interrupts = 0;
static int multiple_interrupts2 = 0;
static int multiple_interrupts3 = 0;
static int multiple_interrupts4 = 0;
static int max_mult_interrupts = 0;
static int total_interrupts = 0;
#endif

#ifdef SDL_GUI
bool isGuiAvailable;
#endif

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

#ifdef HEARTBEAT
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
#ifdef SDL_GUI
		char key[80];
		displayKeysym(bx_options.hotkeys.setup, key);
		snprintf(buf, sizeof(buf), "%s (press %s key for SETUP) %c", VERSION_STRING, key, beats[beat_idx++]);
#else
		snprintf(buf, sizeof(buf), "%s %c", VERSION_STRING, beats[beat_idx++]);
#endif /* SDL_GUI */
		if (beat_idx == strlen(beats))
			beat_idx = 0;

		SDL_WM_SetCaption(buf, NULL);
	}
}
#endif

/* VBL is fixed at 50 Hz in ARAnyM */
void do_vbl_irq()
{
#ifdef HEARTBEAT
	heartBeat();
#endif

	TriggerVBL();		// generate VBL

	// Thread safety patch
	host->hostScreen.lock();

	check_event();		// process keyboard and mouse events

	host->hostScreen.refresh();

	// Thread safety patch
	host->hostScreen.unlock();
}

/*
 * the following function is called from the CPU emulation anytime
 */
void invoke200HzInterrupt()
{
	int ms_ticks = getMFP()->timerC_ms_ticks();

	/* syncing to 200 Hz */
	uint32 newTicks = (uint32) host->clock->getClock();

	// correct lastTicks at start-up
	if (lastTicks == 0) { lastTicks = newTicks - ms_ticks; }

	int count = (newTicks - lastTicks) / ms_ticks;
	if (count == 0) {
#if DEBUG
		early_interrupts++;
#endif
		return;
	}

#if DEBUG
	total_interrupts++;
	if (count > 1) {
		multiple_interrupts++;
		if (count == 2) multiple_interrupts2++;
		if (count == 3) multiple_interrupts3++;
		if (count == 4) multiple_interrupts4++;
		if (count > max_mult_interrupts) {
			max_mult_interrupts = count;
			D(bug("Max multiple interrupts increased to %d", count));
		}
	}
#endif

#if RTC_TIMER
	// do not generate multiple interrupts, let it synchronize over time
	count = 1;
#endif

	int milliseconds = (count * ms_ticks);
	lastTicks += milliseconds;

#ifdef DEBUGGER
	if (!debugging || irqindebug)
#endif
		getMFP()->IRQ(5, count);

#define VBL_MS	20
	static int VBL_counter = 0;
	VBL_counter += milliseconds;
	if (VBL_counter >= VBL_MS) {
		VBL_counter -= VBL_MS;
		do_vbl_irq();
	}
}

#if RTC_TIMER
static int rtc_timer_thread(void * /*ptr*/) {
	int fd, retval;
	unsigned long data;
	// struct timeval tv;

	fd = open ("/dev/rtc", O_RDONLY);

	if (fd == -1) {
		perror("/dev/rtc");
		exit(errno);
	}

	retval = ioctl(fd, RTC_IRQP_SET, 256); // 256 Hz
	if (retval == -1) {
		perror("ioctl");
		exit(errno);
	}

	/* Enable periodic interrupts */
	retval = ioctl(fd, RTC_PIE_ON, 0);
	if (retval == -1) {
		perror("ioctl");
		exit(errno);
	}

	while(! quit_rtc_loop) {
		/* This blocks */
		retval = read(fd, &data, sizeof(unsigned long));
		if (retval == -1) {
			perror("read");
			exit(errno);
		}
		TriggerInternalIRQ();
	}

	/* Disable periodic interrupts */
	retval = ioctl(fd, RTC_PIE_OFF, 0);
	if (retval == -1) {
		perror("ioctl");
		exit(errno);
	}

	close(fd);

	return 0;
}

#else

/*
 * my_callback_function() is called every 10 miliseconds (~ 100 Hz)
 */
Uint32 my_callback_function(Uint32 /*interval*/, void * /*param*/)
{
	TriggerInternalIRQ();
	return 10;					// come back in 10 milliseconds
}
#endif

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

	/* Try Linux/m68k */
	if (boot_lilo) {
		try {
			bootOs = new LinuxBootOs();
			return true;
		} catch (AranymException &e) {
			/* Could not init Linux/m68k */
			panicbug(e.getErrorMessage());
		}
	}

	/* Try TOS */
	if (!boot_emutos) {
		try {
			bootOs = new TosBootOs();
			return true;
		} catch (AranymException &e) {
			/* Could not init TOS */
			panicbug(e.getErrorMessage());
		}
	}

	/* Try EmuTOS */
	try {
		bootOs = new EmutosBootOs();
		return true;
	} catch (AranymException &e) {
		/* Could not init EmuTOS */
		panicbug(e.getErrorMessage());
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

	// work around a bug fix in Debian's libsdl1.2-dev - BTS #317010
	putenv("SDL_DISABLE_LOCK_KEYS=1");

 	int sdlInitParams = SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK;
#if NFCDROM_SUPPORT
	sdlInitParams |= SDL_INIT_CDROM;
#endif
#if !RTC_TIMER
	sdlInitParams |= SDL_INIT_TIMER;
#endif
	if (SDL_Init(sdlInitParams) != 0) {
		panicbug("SDL initialization failed: %s", SDL_GetError());
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

	// set preferred window position
	const char *wpos = bx_options.video.window_pos;
	if (strlen(wpos) > 0) {
		if (strncasecmp(wpos, "center", strlen("center")) == 0) {
			SDL_putenv("SDL_VIDEO_CENTERED=1");
		}
		else {
			static char var[64];
			snprintf(var, sizeof(var), "SDL_VIDEO_WINDOW_POS=%s", wpos);
			SDL_putenv(var);
		}
	}

	// Check if at least one joystick present, open it
	if (SDL_NumJoysticks()>0) {
		sdl_joystick=SDL_JoystickOpen(0);
		if (!sdl_joystick) {
			panicbug("Could not open joystick #0");
		}
	}

	host = new Host();

	// For InterruptFlag controling
	InterruptFlagLock = SDL_CreateMutex();

	CPUType = 4;
	FPUType = 1;

	// Init HW
	HWInit();

	// Init NF
	NFCreate();

	InputInit();

	// Init 680x0 emulation
	if (!Init680x0())
		return false;

#ifdef SDL_GUI
	isGuiAvailable = SDLGui_Init();

	if (isGuiAvailable && startupGUI) {
		start_GUI_thread();
		do {
			host->hostScreen.lock();
			check_event();			// process mouse & keyboard events
			host->hostScreen.unlock();

			SDL_Delay(20);			// 50 Hz input events rate is OK
		}
		while(host->hostScreen.isGUIopen());
	}
#endif /* SDL_GUI */

#ifdef DEBUGGER
	if (bx_options.startup.debugger) {
		D(bug("Activate debugger..."));
		activate_debugger();
	}
#endif

	// timer init
#if RTC_TIMER
	RTCthread = SDL_CreateThread(rtc_timer_thread, NULL);
	infoprint("Using RTC");
#else
	my_timer_id = SDL_AddTimer(10, my_callback_function, NULL);
	if (my_timer_id == NULL) {
		panicbug("SDL Timer does not work!");
		return false;
	}
#endif

	return true;
}


/*
 *  Deinitialize everything
 */

void ExitAll(void)
{
	delete bootOs;

	/* Pause audio before killing hw then host */
	SDL_PauseAudio(SDL_TRUE);

 	/* Close opened joystick */
 	if (SDL_NumJoysticks()>0) {
 		if (SDL_JoystickOpened(0)) {
 			SDL_JoystickClose(sdl_joystick);
 		}
 	}

	InputExit();

	// Exit Time Manager
#if RTC_TIMER
	if (RTCthread != NULL) {
		quit_rtc_loop = true;
		SDL_Delay(100);	// give it a time to safely finish the timer thread
		SDL_KillThread(RTCthread);
		RTCthread = NULL;
	}
#else
	if (my_timer_id) {
		SDL_RemoveTimer(my_timer_id);
		my_timer_id = NULL;
		SDL_Delay(100);	// give it a time to safely finish the timer thread
	}
#endif

	D(bug("200 Hz IRQ statistics: max multiple irqs %d, total multiple irq ratio %02.2lf%%, 2xirq ration %02.2lf%%, 3xirq ratio %02.2lf%%, 4xirq ration %02.2lf%%, early irq ratio %02.2lf%%", max_mult_interrupts, multiple_interrupts*100.0 / total_interrupts, multiple_interrupts2*100.0 / total_interrupts, multiple_interrupts3*100.0 / total_interrupts, multiple_interrupts4*100.0 / total_interrupts, early_interrupts * 100.0 / total_interrupts));

#ifdef SDL_GUI
	kill_GUI_thread();
	SDLGui_UnInit();
#endif

	// Natfeats
	NFDestroy();

	// hardware
	HWExit();

	delete host;
	host = NULL;

	SDL_Quit();
}

void RestartAll()
{
	lastTicks = 0;

	// memory init

	// NF reset provided by the RESET instruction hook
	// HW reset provided by the RESET instruction hook

	// Host reset
	host->reset();

	// OS init
	try {
		bootOs->reset();
	} catch (AranymException &e) {
		panicbug(e.getErrorMessage());
	}

	// CPU init
	Restart680x0();
}

/*
vim:ts=4:sw=4:
*/
