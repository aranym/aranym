/*
 * $Header$
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
#include "cpu_emulation.h"
#include "main.h"
#include "input.h"
#include "hardware.h"
#include "parameters.h"
#include "hostscreen.h"
#include "host.h"			// for the HostScreen
#include "araobjs.h"		// for the ExtFs
#include "md5.h"
#include "romdiff.h"
#include "parameters.h"
#include "version.h"		// for heartBeat

#define DEBUG 0
#include "debug.h"

#include <csignal>
#include <cstdlib>
#include <SDL.h>

#ifdef SDL_GUI
#include "sdlgui.h"
extern SDL_Thread *GUIthread;
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
# ifdef NEWDEBUG
	signal(SIGINT, setactvdebug);
# endif
#endif
}

void init_fdc();

#ifdef EXTFS_SUPPORT
#define METADOS_DRV
#endif

// CPU and FPU type, addressing mode
int CPUType;
bool CPUIs68060;
int FPUType;

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
// Timer stuff
static uint32 lastTicks;
#define USE_GETTICKS 1		// undefine this if your ARAnyM time goes slower

bool invoke200HzInterrupt()
{
	bool pendingQuit = false;

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
		return false;
	
#ifdef DEBUGGER
	if (!debugging || irqindebug)
#endif
		mfp.IRQ(5, count);

	lastTicks += (count * 5);

	VBL_counter += count;
	if (VBL_counter >= VBL_IN_TIMERC) {	// divided by 4 => 50 Hz VBL
		VBL_counter -= VBL_IN_TIMERC;

		heartBeat();

		// Thread safety patch (remove it once the fVDI screen output is in the main thread)
		hostScreen.lock();

		pendingQuit = check_event();// process keyboard and mouse events
		TriggerVBL();		// generate VBL

		if (++refreshCounter == VIDEL_REFRESH) {// divided by 2 again ==> 25 Hz screen update
			if (! hostScreen.isGUIopen())
				videl.renderScreen();
			refreshCounter = 0;
		}

		// Thread safety patch (remove it once the fVDI screen output is in the main thread)
		hostScreen.unlock();
	}

	return pendingQuit;
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
	// _SND (disable DSP and DMA)
	// TODO

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

	printf("TOS 4.04 loading... [OK]\n");
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
		printf("EmuTOS %02x.%02x.%04x loading... [OK]\n",
			ROMBaseHost[0x18],
			ROMBaseHost[0x19],
			(ROMBaseHost[0x1a] << 8) | ROMBaseHost[0x1b]);
	}
	else
		panicbug("EmuTOS image '%s' reading error.", rom_path);
	return bEmuOK;
}

/*
 * Initialize the Operating System - either the EmuTOS or TOS 4.04
 */
bool InitOS(void)
{
	/*
	 * first try TOS 4.04 and if it fails give the EmuTOS a try.
	 * Note that EmuTOS will always be available so this will be
	 * a nice fallback.
	 */
	if (InitTOSROM())
		return true;
	else if (InitEmuTOS())
		return true;

	panicbug("No operating system found. ARAnyM can not boot!");
	printf("Visit http://emutos.sourceforge.net/ and get your copy of EmuTOS now.\n");
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

	int sdlInitParams = SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK;
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
			printf("Could not open joystick #0\n");
		}
	}

	// For InterruptFlag controling
	InterruptFlagLock = SDL_CreateMutex();

	CPUType = 4;
	FPUType = 1;

	// Setting "SP & PC"
	for (int i = 0; i < 8; i++) RAMBaseHost[i] = ROMBaseHost[i];

	init_fdc();

#ifdef METADOS_DRV
	// install the drives
	extFS.init();
#endif							// METADOS_DRV

	// Init HW
	HWInit();

	InputInit();

#ifdef SDL_GUI
	isGuiAvailable = SDLGui_Init();
#endif

	// Init 680x0 emulation
	if (!Init680x0())
		return false;

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

#if ENABLE_MON
	// Initialize mon
	mon_init();
	mon_read_byte = mon_read_byte_b2;
	mon_write_byte = mon_write_byte_b2;
#endif

	return true;
}

SDL_Thread *CPUthread = NULL;

int start_cpu(void *ptr)
{
	Start680x0();
	return 0;
}

void main_loop()
{
	CPUthread = SDL_CreateThread(start_cpu, NULL);
	D(bug("CPU started\n"));

	bool bQuit = false;
	while(! bQuit) {
		// usleep(10000);	// give unused time slices back to OS
		SDL_Delay(10);
		bQuit = invoke200HzInterrupt();
	}

	// Terminate CPU thread
	// Quit680x0();
	if (CPUthread != NULL) {
		SDL_KillThread(CPUthread);
		CPUthread = NULL;
	}

}


/*
 *  Deinitialize everything
 */

void ExitAll(void)
{
	/* Close opened joystick */
	if (SDL_NumJoysticks()>0) {
		if (SDL_JoystickOpened(0)) {
			SDL_JoystickClose(sdl_joystick);
		}
	}

	// Terminate CPU thread
	if (CPUthread != NULL) {
		SDL_KillThread(CPUthread);
		CPUthread = NULL;
	}

#ifdef SDL_GUI
	if (GUIthread != NULL) {
		SDL_KillThread(GUIthread);
		GUIthread = NULL;
	}
	SDLGui_UnInit();
#endif

#if ENABLE_MON
	// Deinitialize mon
	mon_exit();
#endif

	SDL_VideoQuit();
}


/*
 * $Log$
 * Revision 1.81  2002/09/15 15:17:15  joy
 * CPU to separate thread
 *
 * Revision 1.80  2002/09/12 21:18:09  joy
 * romdiff -> romset
 *
 * Revision 1.79  2002/08/01 22:26:19  joy
 * fixed EmuTOS loading. Prints version of EmuTOS now
 *
 * Revision 1.78  2002/08/01 15:33:19  joy
 * EmuTOS image can be just 256 kB long now
 *
 * Revision 1.77  2002/07/20 12:42:04  joy
 * hint for invoking the GUI displayed in the window
 *
 * Revision 1.76  2002/07/20 11:33:27  joy
 * OS loading revamped. Now it first tries TOS, then EmuTOS and if both fail a warning and an advice how to obtain EmuTOS is displayed.
 *
 * Revision 1.75  2002/07/19 23:29:56  joy
 * don't crash when GUI font is not loaded
 *
 * Revision 1.74  2002/07/15 18:24:15  milan
 * extended sigsegv handler upgraded
 *
 * Revision 1.73  2002/06/26 22:09:22  joy
 * don't try to init extern variables
 * do not support patched TOS 4.04 files
 *
 * Revision 1.72  2002/06/26 20:59:43  joy
 * render VIDEL only if SDL GUI is not open
 *
 * Revision 1.71  2002/06/24 19:29:33  joy
 * give it a time to finish timer thread
 *
 * Revision 1.70  2002/06/24 19:10:35  joy
 * It should be safe now to call ExitAll() anytime (even if InitAll() was not called yet)
 *
 * Revision 1.69  2002/06/07 20:58:06  joy
 * heart beat added
 *
 * Revision 1.68  2002/05/14 19:09:50  milan
 * Better structure of vm_alloc, not own OS dependent includes
 * Some debug outputs added for JIT compiler
 * A small reform in memory allocation, one stupid bug (of course, I'm the author) found in QuitEmulator (vm_release(nil))
 *
 * Revision 1.67  2002/04/29 11:44:35  joy
 * "ROM" => "TOS"
 * ErrorAlert() -> panicbug()
 * always display filename when reporting a file error
 * suggest to check readability of the TOS file image since Unix unzip does not set the rw- flags correctly
 *
 * Revision 1.66  2002/04/22 18:30:50  milan
 * header files reform
 *
 * Revision 1.65  2002/04/21 20:45:29  joy
 * SDL GUI support added.
 * EmuTOS loading: file length checked, must be 512 kB.
 *
 * Revision 1.63  2002/04/13 21:55:50  joy
 * TOS patch for redirecting printer output added
 *
 * Revision 1.62  2002/02/23 13:40:41  joy
 * input related code separated from main.cpp
 *
 * Revision 1.61  2002/01/18 20:37:47  milan
 * FixedSizeFastRAM & Makefile fixed
 *
 * Revision 1.60  2002/01/15 21:27:07  joy
 * gettimeofday() for platforms where SDL_GetTicks() returns wrong values
 *
 * Revision 1.59  2002/01/11 11:55:41  joy
 * SDL_GetTicks() returns unsigned value! Thanks to Olivier for the fix. This could lead to stop after 23 days of aranym run :-(
 *
 * Revision 1.58  2002/01/09 19:14:12  milan
 * Preliminary support for SGI/Irix
 *
 * Revision 1.57  2002/01/08 16:13:18  joy
 * config variables moved from global ones to bx_options struct.
 *
 * Revision 1.56  2002/01/03 23:10:41  joy
 * redirect xconout to host console
 *
 * Revision 1.55  2001/12/29 17:11:40  joy
 * cleaned up
 *
 * Revision 1.54  2001/12/27 22:31:49  joy
 * TOS patch to enable FastRAM visual check
 *
 * Revision 1.53  2001/12/22 18:13:24  joy
 * most video related parameters moved to bx_options.video struct.
 * --refresh <x> added
 *
 * Revision 1.52  2001/12/17 09:56:56  joy
 * VBL is at precise 50 Hz now. And screen refresh at 25 Hz.
 *
 * Revision 1.51  2001/12/17 08:33:00  standa
 * Thread synchronization added. The check_event and fvdidriver actions are
 * synchronized each to other.
 *
 * Revision 1.50  2001/12/17 00:26:13  joy
 * SDL Timer is back! Use --enable-sdltimer to emulate TimerC (200 Hz interrupt) using a separate timer thread.
 *
 * Revision 1.49  2001/12/11 21:08:25  standa
 * The SDL_WarpMouse() was causing Floating Point error on fbcon.
 * It was moved behind the HWInit() to be called after SDL_SetVideoMode (correct
 * place to call it).
 * The fullscreen detection was added for "fb" drivers to not to cause mouse cursor
 * redraw problems when using Clocky(TM) and touching the screen edges.
 *
 * Revision 1.48  2001/12/07 17:29:13  milan
 * e00000 -> 0 back in InitAll
 *
 * Revision 1.47  2001/12/06 01:06:54  joy
 * disable mouse grabbing at start
 *
 * Revision 1.46  2001/12/06 00:25:22  milan
 * ndebug corrected, sync
 *
 * Revision 1.45  2001/12/02 01:31:20  joy
 * keyboard conversion rewritten again, this time with heuristic analysis to detect the scancode2scancode conversion offset of SDL automatically.
 *
 * Revision 1.44  2001/11/28 22:52:17  joy
 * keyboard conversion based on scancodes now.
 * page up/down takes care of previously held Shift
 *
 * Revision 1.43  2001/11/21 13:29:51  milan
 * cleanning & portability
 *
 * Revision 1.42  2001/11/20 23:29:26  milan
 * Extfs now not needed for ARAnyM's compilation
 *
 * Revision 1.41  2001/11/19 17:50:28  joy
 * second MD5 check removed
 *
 * Revision 1.40  2001/11/11 22:03:55  joy
 * direct truecolor is optional (compile time configurable)
 *
 * Revision 1.39  2001/11/07 21:18:25  milan
 * SDL_CFLAGS in CXXFLAGS now.
 *
 * Revision 1.38  2001/11/06 20:36:54  milan
 * MMU's corrections
 *
 * Revision 1.37  2001/11/06 13:35:51  joy
 * now recognizes TOS from CVS 2001-05-02 and also the new one (WinX free).
 *
 * Revision 1.36  2001/11/06 11:45:14  joy
 * updated MD5 checksum for new romdiff.cpp
 * optional saving of patched TOS ROM
 *
 * Revision 1.35  2001/11/01 16:03:54  joy
 * mouse behavior fixed:
 * 1) you can release mouse grab by pressing Alt+Ctrl+Esc (VMware compatible)
 * 2) you can grab mouse by click the left mouse button (VMware compatible)
 * 3) if the Atari mouse driver is active, you can release mouse by moving the mouse off the window and by moving back you grab it again. This can be disabled by AutoGrabMouse=false.
 *
 * Revision 1.34  2001/10/29 20:00:05  standa
 * The button 4 and 5 mapped to KeyUp and KeyDown.
 *
 * Revision 1.33  2001/10/29 11:16:07  joy
 * emutos gets a special care
 *
 * Revision 1.32  2001/10/29 08:15:45  milan
 * some changes around debuggers
 *
 * Revision 1.31  2001/10/25 19:56:01  standa
 * The Log and Header CVS tags in the Log removed. Was recursing.
 *
 * Revision 1.30  2001/10/25 15:03:20  joy
 * move floppy stuff from main to fdc
 *
 * Revision 1.29  2001/10/25 12:25:28  joy
 * request the ROM to be 512 kB long. On the web there are just 256 kB long images...
 *
 * Revision 1.28  2001/10/23 21:26:15  standa
 * The hostScreen size is used to handle the mouseOut flag.
 *
 * Revision 1.27  2001/10/18 14:27:24  joy
 * TOS 4.04 is patched in runtime
 *
 * Revision 1.26  2001/10/18 13:46:34  joy
 * detect the ROM version. Knows both our 68040 friendly ABTOS and the original TOS 4.04.
 *
 * Revision 1.25  2001/10/16 19:38:44  milan
 * Integration of BasiliskII' cxmon, FastRAM in aranymrc etc.
 *
 * Revision 1.24  2001/10/12 07:56:14  standa
 * Pacal to C conversion ;( sorry for that.
 *
 * Revision 1.23  2001/10/09 19:25:19  milan
 * MemAlloc's rewriting
 *
 *
 */
