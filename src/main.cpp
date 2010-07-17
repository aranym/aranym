/*
 * main.cpp - startup/shutdown code
 *
 * Copyright (c) 2001-2008 Petr Stehlik of ARAnyM dev team (see AUTHORS)
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
# include "sdlgui.h"
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
static volatile bool using_rtc_timer = false;
static volatile bool quit_rtc_loop = false;
#endif
SDL_TimerID my_timer_id = NULL;

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

/* VBL is fixed at 50 Hz in ARAnyM */
void do_vbl_irq()
{
	TriggerVBL();		// generate VBL

	check_event();		// process keyboard and mouse events

	host->video->refresh();
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
	if (using_rtc_timer) {
		// do not generate multiple interrupts, let it synchronize over time
		count = 1;
	}
#endif

	int milliseconds = (count * ms_ticks);
	lastTicks += milliseconds;


#ifdef DEBUGGER
	if (!debugging || irqindebug)
#endif
	{
		getMFP()->IRQ(5, count);
		getSCC()->IRQ();	/* jc: after or before getMFP does not change anything */
	}

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
	int fd = open ("/dev/rtc", O_RDONLY);

	if (fd == -1) {
		perror("ARAnyM RTC Timer: /dev/rtc");
		return 1;
	}

	int retval = ioctl(fd, RTC_IRQP_SET, 256); // 256 Hz
	if (retval == -1) {
		perror("ARAnyM RTC Timer: ioctl(256 Hz)");
		close(fd);
		return 2;
	}

	/* Enable periodic interrupts */
	retval = ioctl(fd, RTC_PIE_ON, 0);
	if (retval == -1) {
		perror("ARAnyM RTC Timer; ioctl(PIE_ON)");
		close(fd);
		return 3;
	}

	using_rtc_timer = true;

	while(! quit_rtc_loop) {
		unsigned long data;

		/* This blocks */
		retval = read(fd, &data, sizeof(data));
		if (retval == -1) {
			perror("ARAnyM RTC Timer: read");
			close(fd);
			return 4;
		}
		TriggerInternalIRQ();
	}

	/* Disable periodic interrupts */
	retval = ioctl(fd, RTC_PIE_OFF, 0);
	if (retval == -1) {
		perror("ARAnyM RTC Timer: ioctl(PIE_OFF)");
	}

	close(fd);

	return 0;
}

static void KillRTCTimer(void)
{
	if (RTCthread != NULL) {
		quit_rtc_loop = true;
		SDL_Delay(50);	// give it a time to safely finish the timer thread
		SDL_KillThread(RTCthread);
		RTCthread = NULL;
	}
}
#endif

/*
 * my_callback_function() is called every 10 miliseconds (~ 100 Hz)
 */
Uint32 my_callback_function(Uint32 /*interval*/, void * /*param*/)
{
	TriggerInternalIRQ();
	return 10;					// come back in 10 milliseconds
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

void SetWMIcon(void)
{
	char path[1024];
	getDataFilename("wm_icon.bmp", path, sizeof(path));
	SDL_Surface *icon = SDL_LoadBMP(path);
	if (icon != NULL) {
		uint8 mask[] = {0x00, 0x3f, 0xfc, 0x00,
						0x00, 0xff, 0xfe, 0x00,
						0x01, 0xff, 0xff, 0x80,
						0x07, 0xff, 0xff, 0xe0,
						0x0f, 0xff, 0xff, 0xf0,
						0x1f, 0xff, 0xff, 0xf8,
						0x1f, 0xff, 0xff, 0xf8,
						0x3f, 0xff, 0xff, 0xfc,

						0x7f, 0xff, 0xff, 0xfe,
						0x7f, 0xff, 0xff, 0xfe,
						0xff, 0xff, 0xff, 0xff,
						0xff, 0xff, 0xff, 0xff,
						0xff, 0xff, 0xff, 0xff,
						0xff, 0xff, 0xff, 0xff,
						0xff, 0xff, 0xff, 0xff,
						0xff, 0xff, 0xff, 0xff,

						0xff, 0xff, 0xff, 0xff,
						0xff, 0xff, 0xff, 0xff,
						0xff, 0xff, 0xff, 0xff,
						0xff, 0xff, 0xff, 0xff,
						0xff, 0xff, 0xff, 0xff,
						0x7f, 0xff, 0xff, 0xfe,
						0x7f, 0xff, 0xff, 0xfe,
						0x3f, 0xff, 0xff, 0xfe,

						0x3f, 0xff, 0xff, 0xfc,
						0x1f, 0xff, 0xff, 0xf8,
						0x0f, 0xff, 0xff, 0xf8,
						0x0f, 0xff, 0xff, 0xf0,
						0x07, 0xff, 0xff, 0xe0,
						0x01, 0xff, 0xff, 0x80,
						0x00, 0xff, 0xff, 0x00,
						0x00, 0x1f, 0xfc, 0x00};
/*
		uint8 masK[] = {0x01, 0x80, 0x07, 0xfc,
						0x01, 0x80, 0x07, 0xfc,
						0x00, 0x00, 0x3e, 0x03,
						0x00, 0x00, 0x3e, 0x03,
						0x00, 0x00, 0x3e, 0x03,
						0x00, 0x7c, 0xf0, 0x03,
						0x00, 0x7c, 0xf0, 0x03,
						0xc0, 0x7c, 0xc0, 0x1c,

						0xc0, 0x7c, 0xc0, 0x1c,
						0x07, 0x83, 0xc0, 0x63,
						0x07, 0x83, 0xc0, 0x63,
						0x07, 0x9e, 0x31, 0x9c,
						0x07, 0x9e, 0x31, 0x9c,
						0x07, 0x9e, 0x31, 0x9c,
						0x00, 0x7c, 0xfe, 0x00,
						0x00, 0x7c, 0xfe, 0x00,

						0x07, 0xe3, 0x31, 0xe0,
						0x07, 0xe3, 0x31, 0xe0,
						0x3e, 0x1f, 0xff, 0xfc,
						0x3e, 0x1f, 0xff, 0xfc,
						0x38, 0x03, 0x3e, 0x00,
						0xf8, 0x03, 0x3e, 0x00,
						0xf8, 0x03, 0x3e, 0x00,
						0xc0, 0x1c, 0xf1, 0xff,

						0xc0, 0x1c, 0xf1, 0xff,
						0xc0, 0x60, 0xf1, 0xe0,
						0xc0, 0x60, 0xf1, 0xe0,
						0xc1, 0x9c, 0x31, 0x9f,
						0xc1, 0x9c, 0x31, 0x9f,
						0xc1, 0x60, 0x31, 0x9f,
						0x3e, 0x60, 0x01, 0x9c,
						0x3e, 0x60, 0x01, 0x9c};
*/
		SDL_WM_SetIcon(icon, mask);
		SDL_FreeSurface(icon);
	}
	else {
		infoprint("WM Icon not found at %s", path);
	}
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

	// work around a bug fix in Debian's libsdl1.2-dev - BTS #317010
	putenv((char*)"SDL_DISABLE_LOCK_KEYS=1");

 	int sdlInitParams = SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK
		| SDL_INIT_NOPARACHUTE;
#if NFCDROM_SUPPORT
	sdlInitParams |= SDL_INIT_CDROM;
#endif
	sdlInitParams |= SDL_INIT_TIMER;
	if (SDL_Init(sdlInitParams) != 0) {
		panicbug("SDL initialization failed: %s", SDL_GetError());
		return false;
	}
	atexit(SDL_Quit);

	SetWMIcon();

	// set preferred window position
	const char *wpos = bx_options.video.window_pos;
	if (strlen(wpos) > 0) {
		if (strncasecmp(wpos, "center", strlen("center")) == 0) {
			SDL_putenv((char*)"SDL_VIDEO_CENTERED=1");
		}
		else {
			static char var[64];
			snprintf(var, sizeof(var), "SDL_VIDEO_WINDOW_POS=%s", wpos);
			SDL_putenv(var);
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
		open_GUI();
		do {
			check_event();	// process mouse & keyboard events
			host->video->refresh();
			SDL_Delay(20);
		} while(!SDLGui_isClosed());
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
	if (RTCthread != NULL) {
		SDL_Delay(50); // give the timer thread time to initialize
	}

	if (using_rtc_timer) {
		infoprint("Using RTC Timer");
	}
	else {
		KillRTCTimer();
#endif
	my_timer_id = SDL_AddTimer(10, my_callback_function, NULL);
	if (my_timer_id == NULL) {
		panicbug("SDL Timer does not work!");
		return false;
	}
#if RTC_TIMER
	}
#endif

	if (! InitOS())
		return false;

	host->video->bootDone();
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

	InputExit();

	// Exit Time Manager
#if RTC_TIMER
	KillRTCTimer();
#endif
	if (my_timer_id) {
		SDL_RemoveTimer(my_timer_id);
		my_timer_id = NULL;
		SDL_Delay(100);	// give it a time to safely finish the timer thread
	}

	D(bug("200 Hz IRQ statistics: max multiple irqs %d, total multiple irq ratio %02.2lf%%, 2xirq ration %02.2lf%%, 3xirq ratio %02.2lf%%, 4xirq ration %02.2lf%%, early irq ratio %02.2lf%%", max_mult_interrupts, multiple_interrupts*100.0 / total_interrupts, multiple_interrupts2*100.0 / total_interrupts, multiple_interrupts3*100.0 / total_interrupts, multiple_interrupts4*100.0 / total_interrupts, early_interrupts * 100.0 / total_interrupts));

#ifdef SDL_GUI
	close_GUI();
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

	// memory init should be added here?

	/*
	 * Emulated Atari hardware and virtual hardware provided by NativeFeatures
	 * is initialized by the RESET instruction in the AtariReset() handler
	 * in the aranym_glue.cpp so it doesn't have to be initialized here.
	 * The RESET instruction is at beginning on every operating system (TOS
	 * and EmuTOS for sure, and it is added in our integrated LILO as well)
	 */

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

	host->video->bootDone();
}

/*
vim:ts=4:sw=4:
*/
