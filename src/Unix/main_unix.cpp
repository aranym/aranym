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

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#if REAL_ADDRESSING || DIRECT_ADDRESSING
# include <sys/mman.h>
#endif


#include "cpu_emulation.h"
#include "main.h"
#include "vm_alloc.h"
#include "hardware.h"
#include "parameters.h"
#include <SDL/SDL_timer.h>
#include <signal.h>

#define DEBUG 1
#include "debug.h"

// Constants
const char ROM_FILE_NAME[] = DATADIR "/ROM";
const int SCRATCH_MEM_SIZE = 0x10000;	// Size of scratch memory area

#if USE_SCRATCHMEM_SUBTERFUGE
uint8 *ScratchMem = NULL;			// Scratch memory for Mac ROM writes
#endif

#if REAL_ADDRESSING
static bool lm_area_mapped = false;	// Flag: Low Memory area mmap()ped
#endif

#ifndef HAVE_STRDUP
extern "C" char *strdup(const char *s)
{
	char *n = (char *)malloc(strlen(s) + 1);
	strcpy(n, s);
	return n;
}
#endif

void segmentationfault(int x)
{
	printf("Dostali te na PC = $%x!\n", (unsigned)showPC());
	exit(0);
}


/*
 *  Main program
 */
int main(int argc, char **argv)
{
/*
	long i = 0;
	long x = SDL_GetTicks();
	while(x == SDL_GetTicks())
		;
	x = SDL_GetTicks();
	while(x == SDL_GetTicks())
		i++;
	printf("SDL_GetTicks = 1 / %d\n", i);
*/
	// Initialize variables
	RAMBaseHost = NULL;
	ROMBaseHost = NULL;
	TTRAMBaseHost = NULL;
	TTRAMSize = 32*1024*1024;

	program_name = argv[0];
	(void)decode_switches(stderr, argc, argv);

#ifdef NEWDEBUG
	if (start_debug) ndebug::init();
#endif

#ifndef SRACKA
#if REAL_ADDRESSING || DIRECT_ADDRESSING
	TTRAMSize = TTRAMSize & -getpagesize();					// Round down to page boundary
#endif

	// Initialize VM system
	vm_init();

#if REAL_ADDRESSING
	// Flag: RAM and ROM are contigously allocated from address 0
	bool memory_mapped_from_zero = false;
	
	// Under Solaris/SPARC and NetBSD/m68k, Basilisk II is known to crash
	// when trying to map a too big chunk of memory starting at address 0
#if defined(OS_solaris) || defined(OS_netbsd)
	const bool can_map_all_memory = false;
#else
	const bool can_map_all_memory = true;
#endif
	
	// Try to allocate all memory from 0x0000, if it is not known to crash
	if (can_map_all_memory && (vm_acquire_fixed(0, RAMSize + ROMSize + TTRAMSize) == 0)) {
		D(bug("Could allocate RAM and ROM from 0x0000"));
		memory_mapped_from_zero = true;
	}
	
	// Otherwise, just create the Low Memory area (0x0000..0x2000)
	else if (vm_acquire_fixed(0, 0x2000) == 0) {
		D(bug("Could allocate the Low Memory globals"));
		lm_area_mapped = true;
	}
	
	// Exit on failure
	else {
		ErrorAlert("Cannot map Low Memory Globals.\n");
		QuitEmulator();
	}
#endif

	// Create areas for Mac RAM and ROM
#if REAL_ADDRESSING
	if (memory_mapped_from_zero) {
		RAMBaseHost = (uint8 *)0;
		ROMBaseHost = RAMBaseHost + RAMSize;
		TTRAMBaseHost = RAMBaseHost + 0x1000000;
	}
	else
#endif
	{
		RAMBaseHost = (uint8 *)vm_acquire(RAMSize);
		ROMBaseHost = (uint8 *)vm_acquire(ROMSize);
		TTRAMBaseHost = (uint8 *)vm_acquire(TTRAMSize);
		if (RAMBaseHost == VM_MAP_FAILED || ROMBaseHost == VM_MAP_FAILED || TTRAMBaseHost == VM_MAP_FAILED) {
			ErrorAlert("Not enough free memory.\n");
			QuitEmulator();
		}
	}

#if USE_SCRATCHMEM_SUBTERFUGE
	// Allocate scratch memory
	ScratchMem = (uint8 *)vm_acquire(SCRATCH_MEM_SIZE);
	if (ScratchMem == VM_MAP_FAILED) {
		ErrorAlert("Not enough free memory.\n");
		QuitEmulator();
	}
	ScratchMem += SCRATCH_MEM_SIZE/2;	// ScratchMem points to middle of block
#endif

#if DIRECT_ADDRESSING
	// RAMBase shall always be zero
	MEMBaseDiff = (uintptr)RAMBaseHost;
	RAMBase = 0;
	ROMBase = 0xe00000;
	TTRAMBase = 0x1000000;
#endif
#if REAL_ADDRESSING
	RAMBase = (uint32)RAMBaseHost;
	ROMBase = (uint32)ROMBaseHost;
	TTRAMBase = (uint32)TTRAMBaseHost;
#endif
#else /* SRACKA */
	RAMBaseHost = (uint8 *)malloc((14+2)*1024*1024 + TTRAMSize);
	MEMBaseDiff = (uintptr)RAMBaseHost;
	RAMBase = 0;
	ROMBase = 0xe00000;
	ROMBaseHost = ROMBase + RAMBaseHost;
	TTRAMBase = 0x1000000;
	TTRAMBaseHost = TTRAMBase + RAMBaseHost;
#endif /* SRACKA */

	D(bug("ST-RAM starts at %p (%08x)", RAMBaseHost, RAMBase));
	D(bug("TOS ROM starts at %p (%08x)", ROMBaseHost, ROMBase));
	D(bug("TT-RAM starts at %p (%08x)", TTRAMBaseHost, TTRAMBase));

	// Load TOS ROM
	int rom_fd = open(rom_path ? rom_path : ROM_FILE_NAME, O_RDONLY);
	if (rom_fd < 0) {
		ErrorAlert("ROM file not found\n");
		QuitEmulator();
	}
	D(bug("Reading ROM file..."));
	RealROMSize = lseek(rom_fd, 0, SEEK_END);
	if (RealROMSize != 512 * 1024) {
		ErrorAlert("Invalid ROM size\n");
		close(rom_fd);
		QuitEmulator();
	}
	lseek(rom_fd, 0, SEEK_SET);
	if (read(rom_fd, ROMBaseHost, RealROMSize) != (ssize_t) RealROMSize) {
		ErrorAlert("ROM file reading error\n");
		close(rom_fd);
		QuitEmulator();
	}

	// Initialize everything
	D(bug("Initializing All Modules..."));
	if (!InitAll())
		QuitEmulator();
	D(bug("Initialization complete"));

	signal(SIGSEGV, segmentationfault);

	// Start 68k and jump to ROM boot routine
	D(bug("Starting emulation..."));
	Start680x0();

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

	// Free ROM/RAM areas
#ifndef SRACKA
	if (RAMBaseHost != VM_MAP_FAILED) {
		vm_release(RAMBaseHost, RAMSize);
		RAMBaseHost = NULL;
	}
	if (ROMBaseHost != VM_MAP_FAILED) {
		vm_release(ROMBaseHost, ROMSize);
		ROMBaseHost = NULL;
	}
	if (TTRAMBaseHost !=VM_MAP_FAILED) {
		vm_release(ROMBaseHost, TTRAMSize);
		ROMBaseHost = NULL;
	}

#if USE_SCRATCHMEM_SUBTERFUGE
	// Delete scratch memory area
	if (ScratchMem != (uint8 *)VM_MAP_FAILED) {
		vm_release((void *)(ScratchMem - SCRATCH_MEM_SIZE/2), SCRATCH_MEM_SIZE);
		ScratchMem = NULL;
	}
#endif

#if REAL_ADDRESSING
	// Delete Low Memory area
	if (lm_area_mapped)
		vm_release(0, 0x2000);
#endif
	
	// Exit VM wrappers
	vm_exit();
#else /* SRACKA */
	free(RAMBaseHost);
#endif /* SRACKA */

	exit(0);
}


/*
 *  Code was patched, flush caches if neccessary (i.e. when using a real 680x0
 *  or a dynamically recompiling emulator)
 */

void FlushCodeCache(void *start, uint32 size)
{
}


/*
 * $Log$
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
