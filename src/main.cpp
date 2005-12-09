/*
 * main.cpp - startup/shutdown code
 *
 * Copyright (c) 2001-2005 Petr Stehlik of ARAnyM dev team (see AUTHORS)
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
#include "natfeat/nf_objs.h"
#include "audio.h"
#ifdef ENABLE_LILO
#include "lilo.h"
#endif
#ifdef ENABLE_CXX_EXCEPTIONS
#include "rom.h"
#endif

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

extern AUDIO	*audio;

#ifdef SDL_GUI
#include "sdlgui.h"
extern char *displayKeysym(SDL_keysym keysym, char *buffer);
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
static uint32 lastTicks = 0;
#define USE_GETTICKS 1		// undefine this if your ARAnyM time goes slower

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

/* VBL is fixed at 50 Hz in ARAnyM */
void do_vbl_irq()
{
#define VIDEL_REFRESH	bx_options.video.refresh	/* VIDEL screen is refreshed once in 2 VBL interrupts ==> 25 Hz */

	static int refreshCounter = 0;

	heartBeat();

	// Thread safety patch
	hostScreen.lock();

	check_event();		// process keyboard and mouse events
	TriggerVBL();		// generate VBL

	if (++refreshCounter == VIDEL_REFRESH) {// divided by 2 again ==> 25 Hz screen update
		getVIDEL()->renderScreen();
#ifdef SDL_GUI
		if (hostScreen.isGUIopen()) {
			static int blendRefresh = 0;
			if (blendRefresh++ > 5) {
				blendRefresh = 0;
				hostScreen.blendBackgrounds();
			}
		}
#endif /* SDL_GUI */
		refreshCounter = 0;
	}

	// Thread safety patch
	hostScreen.unlock();
}

/*
 * the following function is called from the CPU emulation anytime
 */
void invoke200HzInterrupt()
{
	int ms_ticks = getMFP()->timerC_ms_ticks();

	/* syncing to 200 Hz */
#if USE_GETTICKS
	uint32 newTicks = SDL_GetTicks();
#else
	struct timeval tv;
	gettimeofday(&tv, NULL);
	uint32 newTicks = tv.tv_sec * 1000 + tv.tv_usec / 1000;
#endif

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

#define VBL_MS	40
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

#ifndef ENABLE_CXX_EXCEPTIONS
/*
 * Load, check and patch the TOS 4.04 ROM image file
 */
bool InitTOSROM(void)
{
	if (strlen(bx_options.tos_path) == 0)
		return false;

	// read ROM file
	D(bug("Reading TOS image '%s'", bx_options.tos_path));
	FILE *f = fopen(bx_options.tos_path, "rb");
	if (f == NULL) {
		panicbug("TOS image '%s' not found.", bx_options.tos_path);
		return false;
	}

	RealROMSize = 512 * 1024;
	if (fread(ROMBaseHost, 1, RealROMSize, f) != (size_t)RealROMSize) {
		panicbug("TOS image '%s' reading error.\nMake sure the file is readable and its size is 524288 bytes (512 kB).", bx_options.tos_path);
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
	ROMBaseHost[0x00437] = 0x0E;	/* DSP, XBIOS, DMA8 */
#else
	ROMBaseHost[0x00437] = 0x06;	/* XBIOS, DMA8 */
#endif

	/*
		Remove TOS check for joypads port that set DMA audio bit to 0 if not
		present
	*/
	ROMBaseHost[0x0043e] = 0x4e;	/* btst #7,d0 (d0 is value on joypad port) */
	ROMBaseHost[0x0043f] = 0x71;
	ROMBaseHost[0x00440] = 0x4e;
	ROMBaseHost[0x00441] = 0x71;
	ROMBaseHost[0x00442] = 0x4e;	/* bne.s 0xe00448 */
	ROMBaseHost[0x00443] = 0x71;
	ROMBaseHost[0x00444] = 0x4e;	/* bclr #1,d1 (d1 is value of _SND cookie) */
	ROMBaseHost[0x00445] = 0x71;
	ROMBaseHost[0x00446] = 0x4e;
	ROMBaseHost[0x00447] = 0x71;

	if (FastRAMSize > 0) {
		int i;

		// patch to show FastRAM memory test
		i = 0x001CC;
		ROMBaseHost[i++] = 0x4E;
		ROMBaseHost[i++] = 0xF9;	// JMP <abs.addr>
		ROMBaseHost[i++] = 0x00;	// Can't use JSR, stack pointer
		ROMBaseHost[i++] = 0xE7;	// is not initialized yet.
		ROMBaseHost[i++] = 0xFF;
		ROMBaseHost[i++] = 0x00;	// abs.addr = $E7FF00

		i = 0x7FF00;
		uint32 ramtop = (FastRAMBase + FastRAMSize);
		ROMBaseHost[i++] = 0x21;
		ROMBaseHost[i++] = 0xFC;	// MOVE.L #imm, abs.addr.w
		ROMBaseHost[i++] = ramtop >> 24;
		ROMBaseHost[i++] = ramtop >> 16;
		ROMBaseHost[i++] = ramtop >> 8;
		ROMBaseHost[i++] = ramtop;
		ROMBaseHost[i++] = 0x05;
		ROMBaseHost[i++] = 0xA4;	// abs.addr.w = $5A4 - ramtop
		ROMBaseHost[i++] = 0x4E;
		ROMBaseHost[i++] = 0xF9;	// JMP <abs.addr>
		ROMBaseHost[i++] = 0x00;
		ROMBaseHost[i++] = 0xE0;
		ROMBaseHost[i++] = 0x01;
		ROMBaseHost[i++] = 0xD2;	// abs.addr = $E001D2

		// Patch to make FastRAM available to GEMDOS
		i = 0x0096E;
		ROMBaseHost[i++] = 0x4E;
		ROMBaseHost[i++] = 0xB9;	// JSR <abs.addr>
		ROMBaseHost[i++] = 0x00;
		ROMBaseHost[i++] = 0xE7;
		ROMBaseHost[i++] = 0xFF;
		ROMBaseHost[i++] = 0x0E;	// abs.addr = $E7FF0E

		i = 0x7FF0E;
		// Declare FastRAM with Maddalt()
		ROMBaseHost[i++] = 0x2F;
		ROMBaseHost[i++] = 0x3C;	// MOVE.L #imm, -(sp)
		ROMBaseHost[i++] = FastRAMSize >> 24;
		ROMBaseHost[i++] = FastRAMSize >> 16;
		ROMBaseHost[i++] = FastRAMSize >> 8;
		ROMBaseHost[i++] = FastRAMSize;
		ROMBaseHost[i++] = 0x2F;
		ROMBaseHost[i++] = 0x3C;	// MOVE.L #imm, -(sp)
		ROMBaseHost[i++] = FastRAMBase >> 24;
		ROMBaseHost[i++] = FastRAMBase >> 16;
		ROMBaseHost[i++] = FastRAMBase >> 8;
		ROMBaseHost[i++] = FastRAMBase;
		ROMBaseHost[i++] = 0x3F;
		ROMBaseHost[i++] = 0x3C;	// MOVE.W #imm, -(sp)
		ROMBaseHost[i++] = 0x00;
		ROMBaseHost[i++] = 0x14;	// imm = $14 - Maddalt()
		ROMBaseHost[i++] = 0x4E;
		ROMBaseHost[i++] = 0x41;	// TRAP	#1
		ROMBaseHost[i++] = 0x4F;
		ROMBaseHost[i++] = 0xEF;	// LEA d16(sp), sp
		ROMBaseHost[i++] = 0x00;
		ROMBaseHost[i++] = 0x0A;	// d16 = $A

		// Allocate 64k _FRB buffer
		ROMBaseHost[i++] = 0x42;
		ROMBaseHost[i++] = 0x67;	// CLR.W -(sp) - ST-Ram only
		ROMBaseHost[i++] = 0x2F;
		ROMBaseHost[i++] = 0x3C;	// MOVE.L #imm, -(sp)
		ROMBaseHost[i++] = 0x00;
		ROMBaseHost[i++] = 0x01;
		ROMBaseHost[i++] = 0x20;
		ROMBaseHost[i++] = 0x00;	// imm = 64k + 8k
		ROMBaseHost[i++] = 0x3F;
		ROMBaseHost[i++] = 0x3C;	// MOVE.W #imm, -(sp)
		ROMBaseHost[i++] = 0x00;
		ROMBaseHost[i++] = 0x44;	// imm = $44 - Mxalloc()
		ROMBaseHost[i++] = 0x4E;
		ROMBaseHost[i++] = 0x41;	// TRAP	#1
		ROMBaseHost[i++] = 0x50;
		ROMBaseHost[i++] = 0x4F;	// ADDQ.W #8,sp
		ROMBaseHost[i++] = 0x06;
		ROMBaseHost[i++] = 0x80;	// ADDI.L #imm, d0
		ROMBaseHost[i++] = 0x00;
		ROMBaseHost[i++] = 0x00;
		ROMBaseHost[i++] = 0x1F;
		ROMBaseHost[i++] = 0xFF;	// imm = 8k - 1
		ROMBaseHost[i++] = 0x02;
		ROMBaseHost[i++] = 0x40;	// ANDI.W #imm, d0
		ROMBaseHost[i++] = 0xE0;
		ROMBaseHost[i++] = 0x00;	// imm = $E000 - 8k alignment

		// Store _FRB cookie
		ROMBaseHost[i++] = 0x20;
		ROMBaseHost[i++] = 0x78;	// MOVEA.L addr.w, a0
		ROMBaseHost[i++] = 0x05;
		ROMBaseHost[i++] = 0xA0;	// addr.w = 0x5A0 - cookie jar
		ROMBaseHost[i++] = 0x51;
		ROMBaseHost[i++] = 0x48;	// SUBQ.W #8,a0
		// .find_last_cookie:
		ROMBaseHost[i++] = 0x50;
		ROMBaseHost[i++] = 0x48;	// ADDQ.W #8,a0
		ROMBaseHost[i++] = 0x4A;
		ROMBaseHost[i++] = 0x90;	// TST.L (a0)
		ROMBaseHost[i++] = 0x66;
		ROMBaseHost[i++] = 0xFA;	// BNE.S .find_last_cookie
		ROMBaseHost[i++] = 0x20;
		ROMBaseHost[i++] = 0xFC;	// MOVE.L #imm, (a0)+
		ROMBaseHost[i++] = 0x5F;
		ROMBaseHost[i++] = 0x46;
		ROMBaseHost[i++] = 0x52;
		ROMBaseHost[i++] = 0x42;	// imm = '_FRB'
		ROMBaseHost[i++] = 0x21;
		ROMBaseHost[i++] = 0x50;	// MOVE.L (a0), 8(a0)
		ROMBaseHost[i++] = 0x00;
		ROMBaseHost[i++] = 0x08;	// d16 = 8 - copy jar size
		ROMBaseHost[i++] = 0x20;
		ROMBaseHost[i++] = 0xC0;	// MOVE.L d0,(a0)+
		ROMBaseHost[i++] = 0x42;
		ROMBaseHost[i++] = 0x90;	// CLR.L (a0)

		// Code overwritten by JSR
		ROMBaseHost[i++] = 0x70;
		ROMBaseHost[i++] = 0x03;	// MOVEQ.L #3,d0
		ROMBaseHost[i++] = 0x4E;
		ROMBaseHost[i++] = 0xB9;	// JSR <abs.addr>
		ROMBaseHost[i++] = 0x00;
		ROMBaseHost[i++] = 0xE0;
		ROMBaseHost[i++] = 0x0B;
		ROMBaseHost[i++] = 0xD2;	// abs.addr = $E00BD2
		ROMBaseHost[i++] = 0x4E;
		ROMBaseHost[i++] = 0x75;	// RTS
	}

	// Xconout patch
	if (bx_options.tos.redirect_CON) {
		ROMBaseHost[0x8d44] = ROMBaseHost[0x8d50] = 0x71;
		ROMBaseHost[0x8d45] = ROMBaseHost[0x8d51] = 0x2a;
		ROMBaseHost[0x8d46] = ROMBaseHost[0x8d52] = 0x4e;
		ROMBaseHost[0x8d47] = ROMBaseHost[0x8d53] = 0x75;
	}

	infoprint("TOS 4.04 loading... [OK]");
	return true;
}

/*
 * Load EmuTOS image file
 */
bool InitEmuTOS(void)
{
	if (strlen(bx_options.emutos_path) == 0)
		return false;

	// read EmuTOS file
	D(bug("Reading EmuTOS from '%s'", bx_options.emutos_path));
	FILE *f = fopen(bx_options.emutos_path, "rb");
	if (f == NULL) {
		panicbug("EmuTOS image '%s' not found.", bx_options.emutos_path);
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
			bx_options.emutos_path);
	}
	else
		panicbug("EmuTOS image '%s' reading error.", bx_options.tos_path);
	return bEmuOK;
}
#endif /* ENABLE_CXX_EXCEPTIONS */

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

#ifdef ENABLE_CXX_EXCEPTIONS
	 
	bool result = true;

	try {
		if (boot_lilo) {
			rom = new LinuxRom();
		} else if (!boot_emutos) {
			rom = new TosRom();
		} else {
			rom = new EmutosRom();
		}
	} catch (RomInitializationException &e) {
		panicbug("No operating system found. ARAnyM can not boot!");
		panicbug("Visit http://emutos.sourceforge.net/ and get your copy of EmuTOS now.");
		result = false;
	}

	return result;

#else

# ifdef ENABLE_LILO
	if (boot_lilo && LiloInit())
		return true;
# endif

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
#endif /* ENABLE_CXX_EXCEPTIONS */
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
#if NFCDROM_SUPPORT
	sdlInitParams |= SDL_INIT_CDROM;
#endif
#if !RTC_TIMER
	sdlInitParams |= SDL_INIT_TIMER;
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

	audio = new AUDIO();

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
			hostScreen.lock();
			check_event();			// process mouse & keyboard events
			hostScreen.unlock();

			SDL_Delay(20);			// 50 Hz input events rate is OK
		}
		while(hostScreen.isGUIopen());
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
#else
	my_timer_id = SDL_AddTimer(10, my_callback_function, NULL);
	if (my_timer_id == NULL) {
		panicbug("SDL Timer does not work!");
		return false;
	}
#endif

#ifdef GDBSTUB
	if (bx_options.startup.debugger) {
		D(bug("Activate gdbstub..."));
		activate_debugger();
	}
#endif

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
#ifdef ENABLE_CXX_EXCEPTIONS
	delete rom;
#else
# ifdef ENABLE_LILO
	if (boot_lilo) {
		LiloShutdown();
	}
# endif
#endif

 	/* Close opened joystick */
 	if (SDL_NumJoysticks()>0) {
 		if (SDL_JoystickOpened(0)) {
 			SDL_JoystickClose(sdl_joystick);
 		}
 	}

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

#if ENABLE_MON
	// Deinitialize mon
	mon_exit();
#endif

	delete audio;

	SDL_Quit();
}

void RestartAll()
{
	// memory init

	// NF reset provided by the RESET instruction hook
	// HW reset provided by the RESET instruction hook

	// OS init
#ifdef ENABLE_CXX_EXCEPTIONS
	rom->reset();
#else
	InitOS();
#endif

	// CPU init
	Restart680x0();
}

/*
vim:ts=4:sw=4:
*/
