/*
 * main_unix.cpp - Startup code for Unix
 *
 * Copyright (c) 2000-2005 ARAnyM developer team (see AUTHORS)
 *
 * Authors:
 *  MJ		Milan Jurik
 *  Joy		Petr Stehlik
 * 
 * Originally derived from Basilisk II (C) 1997-2000 Christian Bauer
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
#include "vm_alloc.h"
#include "hardware.h"
#include "parameters.h"
#include "newcpu.h"

#define DEBUG 0
#include "debug.h"

#ifdef ENABLE_MON
# include "mon.h"
#endif

#ifdef HAVE_NEW_HEADERS
# include <cerrno>
# include <csignal>
# include <cstdlib>
#else
# include <errno.h>
# include <signal.h>
# include <stdlib.h>
#endif

#ifndef HAVE_STRDUP
extern "C" char *strdup(const char *s)
{
	char *n = (char *)malloc(strlen(s) + 1);
	strcpy(n, s);
	return n;
}
#endif

#if defined(ENABLE_MON) || defined(NEWDEBUG)
static struct sigaction sigint_sa;
# ifdef ENABLE_MON
static void sigint_handler(...);
# endif
#endif

extern void showBackTrace(int, bool=true);

#ifdef OS_irix
void segmentationfault()
#else
void segmentationfault(int)
#endif
{
	grabMouse(false);
	panicbug("Gotcha! Illegal memory access. Atari PC = $%x", (unsigned)showPC());
#ifdef FULL_HISTORY
	showBackTrace(20, false);
	m68k_dumpstate (NULL);
#else
	panicbug("If the Full History was enabled you would see the last 20 instructions here.");
#endif
	exit(0);
}

bool ThirtyThreeBitAddressing = false;

static void allocate_all_memory()
{
#if REAL_ADDRESSING || DIRECT_ADDRESSING || FIXED_ADDRESSING
	// Initialize VM system
	vm_init();

#if FIXED_ADDRESSING
	if (vm_acquire_fixed((void *)FMEMORY, RAMSize + ROMSize + HWSize + FastRAMSize + RAMEnd) == false) {
		panicbug("Not enough free memory.");
		QuitEmulator();
	}
	RAMBaseHost = (uint8 *)FMEMORY;
	ROMBaseHost = RAMBaseHost + ROMBase;
	HWBaseHost = RAMBaseHost + HWBase;
	FastRAMBaseHost = RAMBaseHost + FastRAMBase;
# ifdef EXTENDED_SIGSEGV
	if (vm_acquire_fixed((void *)(FMEMORY + 0xff000000), RAMSize + ROMSize + HWSize) == false) {
		panicbug("Not enough free memory.");
		QuitEmulator();
	}

#  ifdef HW_SIGSEGV

	if ((FakeIOBaseHost = (uint8 *)vm_acquire(0x00100000)) == VM_MAP_FAILED) {
		panicbug("Not enough free memory.");
		QuitEmulator();
	}

#  endif /* HW_SISEGV */
# endif /* EXTENDED_SIGSEGV */
#else
# if REAL_ADDRESSING
	// Flag: RAM and ROM are contigously allocated from address 0
	bool memory_mapped_from_zero = false;
	
	// Probably all OSes have problems
	// when trying to map a too big chunk of memory starting at address 0
	
	// Try to allocate all memory from 0x0000, if it is not known to crash
	if (vm_acquire_fixed(0, RAMSize + ROMSize + HWSize + FastRAMSize + RAMEnd) == true) {
		D(bug("Could allocate RAM and ROM from 0x0000"));
		memory_mapped_from_zero = true;
	}

	if (memory_mapped_from_zero) {
		RAMBaseHost = (uint8 *)0;
		ROMBaseHost = RAMBaseHost + ROMBase;
		HWBaseHost = RAMBaseHost + HWBase;
		FastRAMBaseHost = RAMBaseHost + FastRAMBase;
	}
	else
# endif
	{
#ifdef USE_33BIT_ADDRESSING
		// Speculatively enables 33-bit addressing
		RAMBaseHost = (uint8*)vm_acquire(RAMSize + ROMSize + HWSize + FastRAMSize + RAMEnd, VM_MAP_DEFAULT | VM_MAP_33BIT);
		if (RAMBaseHost != VM_MAP_FAILED)
			ThirtyThreeBitAddressing = true;
		else
#endif
		RAMBaseHost = (uint8*)vm_acquire(RAMSize + ROMSize + HWSize + FastRAMSize + RAMEnd);
		if (RAMBaseHost == VM_MAP_FAILED) {
			panicbug("Not enough free memory.");
			QuitEmulator();
		}

		ROMBaseHost = RAMBaseHost + ROMBase;
		HWBaseHost = RAMBaseHost + HWBase;
		FastRAMBaseHost = RAMBaseHost + FastRAMBase;
	}
#endif
	D(bug("ST-RAM starts at %p (%08x)", RAMBaseHost, RAMBase));
	D(bug("TOS ROM starts at %p (%08x)", ROMBaseHost, ROMBase));
	D(bug("HW space starts at %p (%08x)", HWBaseHost, HWBase));
	D(bug("TT-RAM starts at %p (%08x)", FastRAMBaseHost, FastRAMBase));
# ifdef EXTENDED_SIGSEGV
#  ifdef HW_SIGSEGV
	D(panicbug("FakeIOspace %p", FakeIOBaseHost));
#  endif
#  ifdef RAMENDNEEDED
	D(bug("RAMEnd needed"));
#  endif
# endif /* EXTENDED_SIGSEGV */
#endif /* REAL_ADDRESSING || DIRECT_ADDRESSING || FIXED_ADDRESSING */
}

#ifdef EXTENDED_SIGSEGV
extern void install_sigsegv();
#else
static void install_sigsegv() {
	signal(SIGSEGV, segmentationfault);
}
#endif

static void install_signal_handler()
{
	install_sigsegv();
	D(bug("Sigsegv handler installed"));

#ifdef ENABLE_MON
	sigemptyset(&sigint_sa.sa_mask);
	sigint_sa.sa_handler = (void (*)(int))sigint_handler;
	sigint_sa.sa_flags = 0;
	sigaction(SIGINT, &sigint_sa, NULL);
#else
# ifdef NEWDEBUG
	if (bx_options.startup.debugger) {
		sigemptyset(&sigint_sa.sa_mask);
		sigint_sa.sa_handler = (void (*)(int))setactvdebug;
		sigint_sa.sa_flags = 0;
		sigaction(SIGINT, &sigint_sa, NULL);
	}
# endif
#endif

#ifdef EXTENDED_SIGSEGV
	if (vm_protect(ROMBaseHost, ROMSize, VM_PAGE_READ)) {
		panicbug("Couldn't protect ROM");
		exit(-1);
	}

	D(panicbug("Protected ROM (%08lx - %08lx)", ROMBaseHost, ROMBaseHost + ROMSize));

# ifdef RAMENDNEEDED
	if (vm_protect(ROMBaseHost + ROMSize + HWSize + FastRAMSize, RAMEnd, VM_PAGE_NOACCESS)) {
		panicbug("Couldn't protect RAMEnd");
		exit(-1);
	}
	D(panicbug("Protected RAMEnd (%08lx - %08lx)", ROMBaseHost + ROMSize + HWSize + FastRAMSize, ROMBaseHost + ROMSize + HWSize + FastRAMSize + RAMEnd));
# endif

# ifdef HW_SIGSEGV
	if (vm_protect(HWBaseHost, HWSize, VM_PAGE_NOACCESS)) {
		panicbug("Couldn't set HW address space");
		exit(-1);
	}

	D(panicbug("Protected HW space (%08lx - %08lx)", HWBaseHost, HWBaseHost + HWSize));

	if (vm_protect(RAMBaseHost + 0xff000000, 0x1000000, VM_PAGE_NOACCESS)) {
		panicbug("Couldn't set mirror address space");
		QuitEmulator();
	}

	D(panicbug("Protected mirror space (%08lx - %08lx)", RAMBaseHost + 0xff000000, RAMBaseHost + 0xff000000 + RAMSize + ROMSize + HWSize));
# endif /* HW_SIGSEGV */
#endif /* EXTENDED_SIGSEGV */
}

/*
 *  Main program
 */
int main(int argc, char **argv)
{
	// Initialize variables
	RAMBaseHost = NULL;
	ROMBaseHost = NULL;
	HWBaseHost = NULL;
	FastRAMBaseHost = NULL;

	// display version string on console (help when users provide debug info)
	infoprint("%s", VERSION_STRING);

	// remember program name
	program_name = argv[0];

	// parse command line switches
	if (!decode_switches(stderr, argc, argv))
		exit(-1);

#ifdef NEWDEBUG
	if (bx_options.startup.debugger) ndebug::init();
	signal(SIGINT, setactvdebug);
#endif

	allocate_all_memory();

	// Initialize everything
	D(bug("Initializing All Modules..."));
	if (!InitAll())
		QuitEmulator();
	D(bug("Initialization complete"));

	install_signal_handler();

	// Start 68k and jump to ROM boot routine
	D(bug("Starting emulation..."));
	Start680x0();

	// returning from emulation after the NMI

	QuitEmulator();

	return 0;
}


/*
 *  Quit emulator
 */

void QuitEmulator(void)
{
	D(bug("QuitEmulator"));

#if EMULATED_68K
	// Exit 680x0 emulation
	Exit680x0();
#endif

	ExitAll();

	// Free ROM/RAM areas
#if REAL_ADDRESSING || DIRECT_ADDRESSING || FIXED_ADDRESSING
	if (RAMBaseHost != VM_MAP_FAILED) {
#ifdef RAMENDNEEDED
		vm_release(RAMBaseHost + RAMSize + ROMSize + HWSize + FastRAMSize, RAMEnd);
#endif
		vm_release(RAMBaseHost, RAMSize);
		RAMBaseHost = NULL;
	}
	if (ROMBaseHost != VM_MAP_FAILED) {
		vm_release(ROMBaseHost, ROMSize);
		ROMBaseHost = NULL;
	}
	if (HWBaseHost != VM_MAP_FAILED) {
		vm_release(HWBaseHost, HWSize);
		HWBaseHost = NULL;
	}
	if (FastRAMBaseHost !=VM_MAP_FAILED) {
		vm_release(FastRAMBaseHost, FastRAMSize);
		FastRAMBaseHost = NULL;
	}
#else
	free(RAMBaseHost);
#endif

	// Exit VM wrappers
	vm_exit();

	exit(0);
}

#ifdef ENABLE_MON
static void sigint_handler(...)
{
#if EMULATED_68K
	uaecptr nextpc;
	extern void m68k_dumpstate(uaecptr *nextpc);
	m68k_dumpstate(&nextpc);
#endif
	char *arg[4] = {"mon", "-m", "-r", NULL};
	mon(3, arg);
	QuitEmulator();
}
#endif
