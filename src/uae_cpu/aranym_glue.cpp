/* 2001 MJ */
/*
 *  aranym_glue.cpp - Glue UAE CPU to ARAnyM
 *
 *  based on
 *
 *  Basilisk II (C) 1997-2001 Christian Bauer
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
#include "newcpu.h"
#ifdef USE_JIT
# include "compiler/compemu.h"
#endif
#include "debug.h"

// RAM and ROM pointers
memptr RAMBase = 0;	// RAM base (Atari address space) gb-- init is important
uint8 *RAMBaseHost;	// RAM base (host address space)
uint32 RAMSize = 0x00e00000;	// Size of RAM

memptr ROMBase = 0x00e00000;	// ROM base (Atari address space)
uint8 *ROMBaseHost;	// ROM base (host address space)
uint32 ROMSize = 0x00100000;	// Size of ROM

uint32 RealROMSize;	// Real size of ROM

memptr HWBase = 0x00f00000;	// HW base (Atari address space)
uint8 *HWBaseHost;	// HW base (host address space)
uint32 HWSize = 0x00100000;    // Size of HW space

memptr FastRAMBase = 0x01000000;		// Fast-RAM base (Atari address space)
uint8 *FastRAMBaseHost;	// Fast-RAM base (host address space)

#ifdef HW_SIGSEGV
uint8 *FakeIOBaseHost;
#endif

#ifdef FIXED_VIDEORAM
memptr VideoRAMBase = ARANYMVRAMSTART;  // VideoRAM base (Atari address space)
#else
memptr VideoRAMBase;                    // VideoRAM base (Atari address space)
#endif
uint8 *VideoRAMBaseHost;// VideoRAM base (host address space)
//uint32 VideoRAMSize;	// Size of VideoRAM

#ifndef NOT_MALLOC
uintptr MEMBaseDiff;	// Global offset between a Atari address and its Host equivalent
#endif

uintptr VMEMBaseDiff;	// Global offset between a Atari VideoRAM address and /dev/fb0 mmap

// From newcpu.cpp
extern int quit_program;

#if defined(ENABLE_EXCLUSIVE_SPCFLAGS) && !defined(HAVE_HARDWARE_LOCKS)
SDL_mutex *spcflags_lock;
#endif

SDL_mutex *stopCondLock;
SDL_cond  *stopCondition;


/*
 *  Initialize 680x0 emulation
 */

bool InitMEM() {
#if REAL_ADDRESSING
	// Atari address space = host address space
	RAMBase = (uint32)RAMBaseHost;
	ROMBase = (uint32)ROMBaseHost;
#else
	InitMEMBaseDiff(RAMBaseHost, RAMBase);
#endif
	InitVMEMBaseDiff(VideoRAMBaseHost, VideoRAMBase);
	return true;
}

bool Init680x0(void)
{
	init_m68k();

#if (ENABLE_EXCLUSIVE_SPCFLAGS && !(HAVE_HARDWARE_LOCKS))
    if ((spcflags_lock = SDL_CreateMutex()) ==  NULL) {
		panicbug("Error by SDL_CreateMutex()");
		exit(EXIT_FAILURE);
    }
#endif

    if ((stopCondLock = SDL_CreateMutex()) ==  NULL) {
		panicbug("Error by SDL_CreateMutex()");
		exit(EXIT_FAILURE);
    }
    if ((stopCondition = SDL_CreateCond()) ==  NULL) {
		panicbug("Error by SDL_CreateCond()");
		exit(EXIT_FAILURE);
    }


#ifdef USE_JIT
	if (bx_options.jit.jit) compiler_init();
#endif
	return true;
}

/*
 * Instr. RESET
 */

void AtariReset(void)
{
	// reset Atari hardware here
}

/*
 * Reset CPU
 */

void Reset680x0(void)
{
	m68k_reset();
}

/*
 *  Deinitialize 680x0 emulation
 */

void Exit680x0(void)
{
#ifdef USE_JIT
	if (bx_options.jit.jit) compiler_exit();
#endif
	exit_m68k();
}


/*
 *  Reset and start 680x0 emulation
 */

void Start680x0(void)
{
	m68k_reset();
#ifdef USE_JIT
	if (bx_options.jit.jit)
		m68k_compile_execute();
	else
#endif
		m68k_execute();
}

/*
 * Restart running 680x0 emulation
 */
void Restart680x0(void)
{
	quit_program = 2;
	TriggerNMI();
}

/*
 * Quit 680x0 emulation
 */
void Quit680x0(void)
{
	quit_program = 1;
	TriggerNMI();
}


/*
 *  Trigger interrupts
 */
void TriggerInternalIRQ(void)
{
	SPCFLAGS_SET( SPCFLAG_INTERNAL_IRQ );
	AwakeFromSleep();
}

void TriggerInt3(void)
{
	SPCFLAGS_SET( SPCFLAG_INT3 );
	AwakeFromSleep();
}

void TriggerVBL(void)
{
	SPCFLAGS_SET( SPCFLAG_VBL );
	AwakeFromSleep();
}

void TriggerInt5(void)
{
	SPCFLAGS_SET( SPCFLAG_INT5 );
	AwakeFromSleep();
}

void TriggerMFP(bool enable)
{
	if (enable)
		SPCFLAGS_SET( SPCFLAG_MFP );
	else
		SPCFLAGS_CLEAR( SPCFLAG_MFP );

	AwakeFromSleep();
}

void TriggerNMI(void)
{
	SPCFLAGS_SET( SPCFLAG_NMI );
	AwakeFromSleep();
}

void SleepAndWait(void)
{
	SDL_mutexP(stopCondLock);
	SDL_CondWait(stopCondition, stopCondLock);
	SDL_mutexV(stopCondLock);
}

void AwakeFromSleep(void)
{
	SDL_mutexP(stopCondLock);
	SDL_CondSignal(stopCondition);
	SDL_mutexV(stopCondLock);
}
