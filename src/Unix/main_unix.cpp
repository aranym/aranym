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
#include "cpu_emulation.h"
#include "main.h"
#include "input.h"
#include "vm_alloc.h"
#include "hardware.h"
#include "parameters.h"
#include "newcpu.h"

#define DEBUG 1
#include "debug.h"

#ifdef ENABLE_MON
# include "mon.h"
#endif

#include <cerrno>
#include <csignal>
#include <cstdlib>

#ifndef HAVE_STRDUP
extern "C" char *strdup(const char *s)
{
	char *n = (char *)malloc(strlen(s) + 1);
	strcpy(n, s);
	return n;
}
#endif

#ifdef ENABLE_MON
static struct sigaction sigint_sa;
static void sigint_handler(...);
#endif

extern void showBackTrace(int, bool=true);

#ifdef OS_irix
void segmentationfault()
#else
void segmentationfault(int x)
#endif
{
	grabMouse(false);
	printf("Gotcha! Illegal memory access. Atari PC = $%x\n", (unsigned)showPC());
#ifdef FULL_HISTORY
	showBackTrace(20, false);
	m68k_dumpstate (NULL);
#else
	printf("If the Full History was enabled you would see the last 20 instructions here.\n");
#endif
	exit(0);
}

#ifdef EXTENDED_SIGSEGV
extern void install_sigsegv();
#else
static void install_sigsegv() {
	signal(SIGSEGV, segmentationfault);
}
#endif

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

	program_name = argv[0];
	(void)decode_switches(stderr, argc, argv);

#ifdef NEWDEBUG
	if (bx_options.startup.debugger) ndebug::init();
#endif

#if REAL_ADDRESSING || DIRECT_ADDRESSING || FIXED_ADDRESSING
	// Initialize VM system
	vm_init();

#if REAL_ADDRESSING
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
#endif
#ifdef FIXED_ADDRESSING
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
	{
		RAMBaseHost = (uint8 *)vm_acquire(RAMSize);
		ROMBaseHost = (uint8 *)vm_acquire(ROMSize);
		HWBaseHost = (uint8 *)vm_acquire(HWSize);
		if (FastRAMSize) FastRAMBaseHost = (uint8 *)vm_acquire(FastRAMSize); else FastRAMBaseHost = RAMBaseHost + 0x1000000;
		if (RAMBaseHost == VM_MAP_FAILED || ROMBaseHost == VM_MAP_FAILED || HWBaseHost == VM_MAP_FAILED || FastRAMBaseHost == VM_MAP_FAILED) {
			panicbug("Not enough free memory.");
			QuitEmulator();
		}
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

	// Initialize everything
	D(bug("Initializing All Modules..."));
	if (!InitAll())
		QuitEmulator();
	D(bug("Initialization complete"));

	install_sigsegv();
	D(bug("Sigsegv handler installed"));

#ifdef ENABLE_MON
	sigemptyset(&sigint_sa.sa_mask);
	sigint_sa.sa_handler = (void (*)(int))sigint_handler;
	sigint_sa.sa_flags = 0;
	sigaction(SIGINT, &sigint_sa, NULL);
#else
# ifdef NEWDEBUG
	if (bx_options.startup.debugger) signal(SIGINT, setactvdebug);
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

/*
 * $Log$
 * Revision 1.71  2002/07/15 19:04:06  milan
 * sigsegv.h never commited
 *
 * Revision 1.70  2002/07/15 18:24:15  milan
 * extended sigsegv handler upgraded
 *
 * Revision 1.69  2002/07/03 19:27:15  milan
 * some minor JIT compiler corrections
 * extended sigsegv handler for x86/linux - no check of memory boundary
 *
 * Revision 1.68  2002/06/24 19:14:14  joy
 * Call ExitAll() to safely stop the timer thread before releasing Atari memory()
 *
 * Revision 1.67  2002/05/14 19:09:50  milan
 * Better structure of vm_alloc, not own OS dependent includes
 * Some debug outputs added for JIT compiler
 * A small reform in memory allocation, one stupid bug (of course, I'm the author) found in QuitEmulator (vm_release(nil))
 *
 * Revision 1.66  2002/04/29 11:45:18  joy
 * ErrorAlert() -> panicbug()
 *
 * Revision 1.65  2002/04/22 21:20:32  milan
 * distributed dispatcher rewroted
 * JIT compiler integrated
 *
 * Revision 1.64  2002/04/22 18:30:50  milan
 * header files reform
 *
 * Revision 1.63  2002/04/22 08:55:08  milan
 * better segfault handling with fullhistory
 *
 * Revision 1.62  2002/04/10 21:05:33  joy
 * just comment added
 *
 * Revision 1.61  2002/03/15 12:22:41  milan
 * Exception() is called only one time
 * JIT c. uses own SEGV handler
 *
 * Revision 1.60  2002/02/23 13:44:52  joy
 * input related code separated from main.cpp
 *
 * Revision 1.59  2002/01/17 14:59:19  milan
 * cleaning in HW <-> memory communication
 * support for JIT CPU
 *
 * Revision 1.58  2002/01/10 00:09:11  milan
 * vm_acquire_fixed corrected
 *
 * Revision 1.57  2002/01/09 19:14:12  milan
 * Preliminary support for SGI/Irix
 *
 * Revision 1.56  2002/01/08 16:21:45  joy
 * config variables moved from global ones to bx_options struct.
 *
 * Revision 1.55  2001/11/21 13:29:51  milan
 * cleanning & portability
 *
 * Revision 1.54  2001/11/08 16:46:37  milan
 * another correction of SDL's includes
 *
 * Revision 1.53  2001/10/29 08:15:45  milan
 * some changes around debuggers
 *
 * Revision 1.52  2001/10/25 12:57:59  joy
 * if segmentation fault occures then release the mouse and keyboard and display last 20 instructions before the sigsegv.
 *
 * Revision 1.51  2001/10/16 19:38:44  milan
 * Integration of BasiliskII' cxmon, FastRAM in aranymrc etc.
 *
 * Revision 1.50  2001/10/09 19:25:19  milan
 * MemAlloc's rewriting
 *
 * Revision 1.49  2001/10/02 19:13:28  milan
 * ndebug, malloc
 *
 * Revision 1.48  2001/10/02 12:13:50  joy
 * ROM file reading moved to main.cpp
 * SIGSEGV handler not installed if you start aranym with internal debugger enabled. This is to allow gdb to catch any problem. So under gdb start aranym with -D.
 *
 * Revision 1.47  2001/09/25 10:00:06  milan
 * cleaning of MM, static version of ARAnyM
 *
 * Revision 1.46  2001/09/25 00:04:17  milan
 * cleaning of memory managment
 *
 * Revision 1.45  2001/09/21 14:24:10  joy
 * little things just to make it compilable
 *
 * Revision 1.44  2001/09/11 11:45:27  joy
 * one more debug line never hurts.
 *
 * Revision 1.43  2001/09/11 11:31:30  joy
 * detect segmentation fault and quit cleanly. Print the offending PC address.
 *
 * Revision 1.42  2001/09/11 10:12:41  joy
 * define SRACKA if you want to get rid of the complicated vm management. Use ./configure --enable-addressing=direct at the same time, otherwise it's not compilable.
 *
 * Revision 1.41  2001/09/10 15:27:30  joy
 * new bogomips-like test. Need to find out if every 100 instructions test in cpu emulation is not too late for slow machines.
 *
 * Revision 1.40  2001/09/08 23:42:26  joy
 * all the shitty memory management disabled for now.
 *
 * Revision 1.39  2001/08/29 18:36:25  milan
 * Integration of TV conf. GUI, small patches of MMU and debugger
 *
 * Revision 1.38  2001/08/21 18:19:16  milan
 * CPU update, disk's geometry autodetection - the 1st step
 *
 * Revision 1.37  2001/08/10 18:41:24  milan
 * Some patches, see ChangeLog (CPU API etc.), debianized
 *
 * Revision 1.35  2001/07/24 07:10:46  joy
 * cleaned up a bit. Portable things moved to main.cpp.
 *
 * Revision 1.34  2001/07/21 18:13:29  milan
 * sclerosis, sorry, ndebug added
 *
 * Revision 1.33  2001/07/20 22:48:19  milan
 * mmu_op use only set/longjmp now, first step for ndebug integration, signals
 * and pthread checks removed, cleaning etc.
 *
 * Revision 1.32  2001/07/12 22:11:22  standa
 * The updateHostScreen() call when in direct_truecolor mode.
 * Commmented STonX mouse code removed.
 *
 * Revision 1.31  2001/07/09 19:41:13  standa
 * grab_mouse is compulsory to get it functioning in fullscreen
 *
 * Revision 1.30  2001/07/03 23:02:41  milan
 * cleaning
 *
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
