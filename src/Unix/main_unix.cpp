/*
 * main_unix.cpp - Startup code for Unix
 *
 * Copyright (c) 2000-2006 ARAnyM developer team (see AUTHORS)
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

#if defined _WIN32 || defined(OS_cygwin)
# define SDL_MAIN_HANDLED
#endif

#include "sysdeps.h"
#include "cpu_emulation.h"
#include "main.h"
#include "input.h"
#include "vm_alloc.h"
#include "hardware.h"
#include "parameters.h"
#include "newcpu.h"
#if defined _WIN32 || defined(OS_cygwin)
#include "win32_supp.h"
#endif
#ifdef SDL_GUI
#include "sdlgui.h"
#include "dlgAlert.h"
#endif

#define USE_VALGRIND 0
#if USE_VALGRIND
#include <valgrind/memcheck.h>
#endif
#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif

#define DEBUG 0
#include "debug.h"

# include <cerrno>
# include <csignal>
# include <signal.h>
# include <cstdlib>

#include "version_date.h"
#include "version.h"

#ifdef VERSION_DATE
#define CVS_DATE	"git " VERSION_DATE
#else
#define CVS_DATE	"git"
#endif

#define str(x)		_stringify (x)
#define _stringify(x)	#x

#define VERSION_STRING	NAME_STRING " " str (VER_MAJOR) "." str (VER_MINOR) "." str (VER_MICRO) VER_STATUS 


char const name_string[] = NAME_STRING;
char const version_string[] = VERSION_STRING;



#ifndef HAVE_SIGHANDLER_T
typedef void (*sighandler_t)(int);
#endif

#ifdef OS_darwin
	extern void refreshMenuKeys();
#endif

#ifndef HAVE_STRDUP
extern "C" char *strdup(const char *s)
{
	char *n = (char *)malloc(strlen(s) + 1);
	strcpy(n, s);
	return n;
}
#endif

#ifdef OS_mingw
# ifndef HAVE_GETTIMEOFDAY
extern "C" void gettimeofday(struct timeval *p, void *tz /*IGNORED*/)
{
        union {
                long long ns100;
                FILETIME ft;
        } _now;

        GetSystemTimeAsFileTime(&(_now.ft));
        p->tv_usec=(long)((_now.ns100/10LL)%1000000LL);
        p->tv_sec=(long)((_now.ns100-(116444736000000000LL))/10000000LL);
}
# endif
#endif

void real_segmentationfault(void)
{
	grabMouse(SDL_FALSE);
	panicbug("Gotcha! Illegal memory access. Atari PC = $%x", (unsigned)showPC());
#ifdef FULL_HISTORY
	ndebug::showHistory(20, false);
	m68k_dumpstate (stderr, NULL);
#else
	panicbug("If the Full History was enabled you would see the last 20 instructions here.");
#endif
	exit(EXIT_FAILURE);
}

#ifdef OS_irix
void segmentationfault()
#else
void segmentationfault(int)
#endif
{
	real_segmentationfault();
}

#if FIXED_ADDRESSING
bool allocate_all_memory(uintptr fmemory, bool quiet)
#else
bool allocate_all_memory(bool quiet)
#endif
{
	UNUSED(quiet);
#if DIRECT_ADDRESSING || FIXED_ADDRESSING
#if FIXED_ADDRESSING
	if (vm_acquire_fixed((void *)fmemory, RAMSize + ROMSize + HWSize + FastRAMSize + RAMEnd) == false) {
		if (!quiet)
			panicbug("Not enough free memory (ST-RAM 0x%08x + TT-RAM 0x%08x).", RAMSize, FastRAMSize);
		return false;
	}
	RAMBaseHost = (uint8 *)fmemory;
	ROMBaseHost = RAMBaseHost + ROMBase;
	HWBaseHost = RAMBaseHost + HWBase;
	FastRAMBaseHost = RAMBaseHost + FastRAMBase;
# ifdef EXTENDED_SIGSEGV
	if (vm_acquire_fixed((void *)(fmemory + ~0xffffffL), RAMSize + ROMSize + HWSize) == false) {
		if (!quiet)
			panicbug("Not enough free memory (protected mirror RAM 0x%08x-0x%08x).", (unsigned int)(fmemory + ~0xffffffL), (unsigned int)(fmemory + ~0xffffffL + RAMSize + ROMSize + HWSize));
		return false;
	}

#  ifdef HW_SIGSEGV

	if ((FakeIOBaseHost = (uint8 *)vm_acquire(0x00100000)) == VM_MAP_FAILED) {
		if (!quiet)
			panicbug("Not enough free memory (Shadow IO).");
		return false;
	}

#  endif /* HW_SISEGV */
# endif /* EXTENDED_SIGSEGV */
#else
	RAMBaseHost = (uint8*)vm_acquire(RAMSize + ROMSize + HWSize + FastRAMSize + RAMEnd);
	if (RAMBaseHost == VM_MAP_FAILED) {
		if (!quiet)
			panicbug("Not enough free memory (ST-RAM 0x%08x + TT-RAM 0x%08x).", RAMSize, FastRAMSize);
		return false;
	}

	ROMBaseHost = RAMBaseHost + ROMBase;
	HWBaseHost = RAMBaseHost + HWBase;
	FastRAMBaseHost = RAMBaseHost + FastRAMBase;
#endif
	InitMEM();
	D(bug("ST-RAM     at %p - %p (0x%08x - 0x%08x)", RAMBaseHost, RAMBaseHost + RAMSize, RAMBase, RAMBase + RAMSize));
	D(bug("TOS ROM    at %p - %p (0x%08x - 0x%08x)", ROMBaseHost, ROMBaseHost + ROMSize, ROMBase, ROMBase + ROMSize));
	D(bug("HW space   at %p - %p (0x%08x - 0x%08x)", HWBaseHost, HWBaseHost + HWSize, HWBase, HWBase + HWSize));
	D(bug("TT-RAM     at %p - %p (0x%08x - 0x%08x)", FastRAMBaseHost, FastRAMBaseHost + FastRAMSize, FastRAMBase, FastRAMBase + FastRAMSize));
	if (VideoRAMBaseHost) {
	D(bug("Video-RAM  at %p - %p (0x%08x - 0x%08x)", VideoRAMBaseHost, VideoRAMBaseHost + ARANYMVRAMSIZE, VideoRAMBase, VideoRAMBase + ARANYMVRAMSIZE));
	}
# ifdef EXTENDED_SIGSEGV
#  ifdef HW_SIGSEGV
	D(bug("FakeIOspace %p", FakeIOBaseHost));
#  endif
#  ifdef RAMENDNEEDED
	D(bug("RAMEnd needed"));
#  endif
# endif /* EXTENDED_SIGSEGV */
#endif /* DIRECT_ADDRESSING || FIXED_ADDRESSING */
	
	return true;
}

void release_all_memory(void)
{
#if DIRECT_ADDRESSING || FIXED_ADDRESSING
	if (RAMBaseHost != VM_MAP_FAILED && RAMBaseHost != NULL) {
#ifdef RAMENDNEEDED
		vm_release(RAMBaseHost + RAMSize + ROMSize + HWSize + FastRAMSize, RAMEnd);
#endif
		vm_release(RAMBaseHost, RAMSize);
#if FIXED_ADDRESSING && defined(EXTENDED_SIGSEGV)
		void *mirror = (void *)(RAMBaseHost + ~0xffffffL);
		vm_release(mirror, RAMSize + ROMSize + HWSize);
#ifdef HW_SIGSEGV
		vm_release((void *)FakeIOBaseHost, 0x00100000);
#endif
#endif
	}
	RAMBaseHost = NULL;
#ifdef HW_SIGSEGV
	FakeIOBaseHost = NULL;
#endif
	if (ROMBaseHost != VM_MAP_FAILED && ROMBaseHost != NULL) {
		vm_release(ROMBaseHost, ROMSize);
	}
	ROMBaseHost = NULL;
	if (HWBaseHost != VM_MAP_FAILED && HWBaseHost != NULL) {
		vm_release(HWBaseHost, HWSize);
	}
	HWBaseHost = NULL;
	if (FastRAMBaseHost != VM_MAP_FAILED && FastRAMBaseHost != NULL) {
		vm_release(FastRAMBaseHost, FastRAMSize);
	}
	FastRAMBaseHost = NULL;
#else
	free(RAMBaseHost);
	RAMBaseHost = NULL;
#endif
}

#ifndef EXTENDED_SIGSEGV
void install_sigsegv() {
	signal(SIGSEGV, segmentationfault);
}

void uninstall_sigsegv() {
	signal(SIGSEGV, SIG_DFL);
}
#endif

static bool install_signal_handler(bool quiet)
{
	if (!quiet)
	{
#ifdef HAVE_SIGACTION
		{
			struct sigaction sa;
			memset(&sa, 0, sizeof(sa));
			sigemptyset(&sa.sa_mask);
			sa.sa_handler = (void (*)(int))setactvdebug;
			sa.sa_flags = 0;
			sigaction(SIGINT, &sa, NULL);
		}
#else
		signal(SIGINT, (void (*)(int))setactvdebug);
#endif
	}
	
#ifdef EXTENDED_SIGSEGV
	if (vm_protect(ROMBaseHost, ROMSize, VM_PAGE_READ)) {
		if (!quiet)
			panicbug("Couldn't protect ROM");
		return false;
	}

	D(bug("Protected ROM          at %p - %p (0x%08x - 0x%08x)", ROMBaseHost, ROMBaseHost + ROMSize, ROMBase, ROMBase + ROMSize));
#if USE_VALGRIND
	VALGRIND_MAKE_MEM_DEFINED(ROMBaseHost, ROMSize);
#endif

# ifdef RAMENDNEEDED
	if (vm_protect(ROMBaseHost + ROMSize + HWSize + FastRAMSize, RAMEnd, VM_PAGE_NOACCESS)) {
		if (!quiet)
			panicbug("Couldn't protect RAMEnd");
		return false;
	}
	D(bug("Protected RAMEnd       at %p - %p (0x%08x - 0x%08x)",
		ROMBaseHost + ROMSize + HWSize + FastRAMSize, ROMBaseHost + ROMSize + HWSize + FastRAMSize + RAMEnd,
		ROMBase + ROMSize + HWSize + FastRAMSize, ROMBase + ROMSize + HWSize + FastRAMSize + RAMEnd));
#if USE_VALGRIND
	VALGRIND_MAKE_MEM_DEFINED(ROMBaseHost + ROMSize + HWSize + FastRAMSize, RAMEnd);
#endif
# endif

# ifdef HW_SIGSEGV
	if (vm_protect(HWBaseHost, HWSize, VM_PAGE_NOACCESS)) {
		if (!quiet)
			panicbug("Couldn't set HW address space");
		return false;
	}

	D(bug("Protected HW space     at %p - %p (0x%08x - 0x%08x)", HWBaseHost, HWBaseHost + HWSize, HWBase, HWBase + HWSize));

	if (vm_protect(RAMBaseHost + ~0xffffffUL, 0x1000000, VM_PAGE_NOACCESS)) {
		if (!quiet)
			panicbug("Couldn't set mirror address space");
		return false;
	}

	D(bug("Protected mirror space at %p - %p (0x%08x - 0x%08x)",
		RAMBaseHost + ~0xffffffUL, RAMBaseHost + ~0xffffffUL + RAMSize + ROMSize + HWSize,
		RAMBase + ~0xffffffU, RAMBase + ~0xffffffU + RAMSize + ROMSize + HWSize));
#if USE_VALGRIND
	VALGRIND_MAKE_MEM_DEFINED(HWBaseHost, HWSize);
	VALGRIND_MAKE_MEM_DEFINED(RAMBaseHost + ~0xffffffUL, 0x1000000);
#endif
# endif /* HW_SIGSEGV */

#ifdef HAVE_SBRK
	D(bug("Program break           %p", sbrk(0)));
#endif

#endif /* EXTENDED_SIGSEGV */

	return true;
}

static void remove_signal_handler()
{
	uninstall_sigsegv();
	D(bug("Sigsegv handler removed"));
}


#if FIXED_ADDRESSING

#include <setjmp.h>

static volatile int try_gotsig;
static sigjmp_buf seg_jmpbuf;

#ifdef OS_irix
static void try_segfault()
#else
static void try_segfault(int)
#endif
{
	try_gotsig = 1;
	siglongjmp(seg_jmpbuf, 1);
}


static void try_release(uintptr addr, size_t size)
{
	if (RAMBaseHost != VM_MAP_FAILED)
	{
# ifdef EXTENDED_SIGSEGV
		/* release mirror RAM */
		void *mirror = (void *)(addr + ~0xffffffL);
		if (mirror != 0 && mirror != VM_MAP_FAILED)
			vm_release(mirror, RAMSize + ROMSize + HWSize);
#  ifdef HW_SIGSEGV
		vm_release((void *)FakeIOBaseHost, 0x00100000);
#  endif
#endif
		vm_release((void *)addr, size);
	}
}


static bool try_acquire(uintptr addr, size_t ttram_size)
{
	size_t const stram_size = RAMSize + ROMSize + HWSize;
	size_t try_size = stram_size + ttram_size + RAMEnd;
	bool const quiet = true;
	
	FastRAMSize = ttram_size;
#ifdef HW_SIGSEGV
	FakeIOBaseHost = (uint8 *)VM_MAP_FAILED;
#endif
	RAMBaseHost = (uint8 *)VM_MAP_FAILED;
	try_gotsig = 0;
	if (sigsetjmp(seg_jmpbuf, 1) != 0)
	{
		printf("got segfault\n");
		fflush(stdout);
		try_release(addr, try_size);
		return false;
	}
	signal(SIGSEGV, (void (*)(int))try_segfault);
#ifdef SIGBUS
	signal(SIGBUS, (void (*)(int))try_segfault);
#endif
	if (allocate_all_memory(addr, quiet) == false)
	{
		try_release(addr, try_size);
		return false;
	}
	
	if (install_signal_handler(quiet) == false)
	{
		try_release(addr, try_size);
		return false;
	}
	
	try_release(addr, try_size);
	if (try_gotsig)
		return false;
	
	return true;
}


void vm_probe_fixed(void)
{	
	// This might need tweaking
	// 0x01000000 gives SIGSEGV without being catched by handler on linux
	uintptr_t const probestart = 0x04000000;
	uintptr_t const probeend   = 0xfff00000;
	size_t    const step       = 0x00100000;
	size_t    const mapsize    = (probeend - probestart) / step;
	size_t    const reserved   = RAMSize + ROMSize + HWSize + RAMEnd;
	
	size_t ttram_size = 0;
	size_t best_size = 0;
	uintptr best_addr = 0;
	uintptr addr;
	signed char *maptab;
	size_t i, j;
	
	maptab = new signed char[mapsize];
	memset(maptab, 0, mapsize);
	
	vm_init();
	/* no install_sigsegv() here; catch segfaults seperately */
	
	printf("probing available memory ranges (this may take a while)\n");
	/*
	 * try to blacklist ranges of already existing mappings
	 * that might overlap. Attempting to use them
	 * might result in un-catchable segmentation faults
	 */
	{
		FILE *fp = fopen("/proc/self/maps", "r");
		if (fp != NULL)
		{
			char buf[1024];
			uintptr_t lower, upper;
			
			while (fgets(buf, sizeof(buf), fp) != NULL)
			{
				if (sscanf(buf, "%" SCNxPTR "-%" SCNxPTR, &lower, &upper) == 2)
				{
					lower = lower & ~(step - 1);
					upper = (upper + step - 1) & ~(step - 1);
					if (lower < probeend)
					{
						if (lower >= reserved)
							addr = lower - reserved;
						else
							addr = lower;
						for (; addr < (upper + reserved) && addr < probeend; addr += step)
						{
							if (addr >= probestart)
							{
								i = (addr - probestart) / step;
								if (i < mapsize)
								{
									// printf("will skip 0x%08" PRIxPTR "-0x%08" PRIxPTR "\n", addr, addr + step);
									maptab[i] = -1;
								}
							}
						}
					}
				}
			}
			fclose(fp);
		}
	}
	fflush(stdout);
	for (addr = probestart; addr < probeend; addr += step)
	{
		i = (addr - probestart) / step;;
		if (maptab[i] >= 0 && try_acquire(addr, ttram_size))
			maptab[i] = 1;
	}
	for (i = 0; i < mapsize; i++)
	{
		if (maptab[i] > 0)
		{
			for (j = i + 1; j < mapsize; j++)
				if (maptab[j] <= 0)
					break;
			size_t size = (j - i) * step;
			addr = probestart + i * step;
			size_t ttram_size;
			if (size > reserved)
				ttram_size = size - reserved;
			else
				ttram_size = 0;
			if (ttram_size > best_size)
			{
				best_addr = addr;
				best_size = ttram_size;
			}
			printf("available: 0x%08x - 0x%08x (%uMB TT-RAM)\n", (unsigned int)addr, (unsigned int)(addr + size), (unsigned int)(ttram_size / 0x00100000));
			i = j - 1;
		}
	}
	
	if (best_addr)
		guialert("suggested --fixedmem setting: 0x%08x (%uMB TT-RAM)", (unsigned int)best_addr, (unsigned int)(best_size / 0x00100000));
	else
		guialert("no suitable address to allow TT-RAM found!");
	
	delete [] maptab;
}
#endif


void vm_probe_fixed_hint(void)
{
#if FIXED_ADDRESSING
	/*
	 * give a hint for the command line option if memory allocation fails
	 * (with FIXED_ADDRESSING, this is usually not caused by missing memory,
	 * but overlapping of virtual addresses due to the --fixedmem setting)
	 */
	guialert("failed to acquire virtual memory at 0x%08x\n"
			 "try running 'aranym --probe-fixed'",
			 (unsigned int) fixed_memory_offset);
#endif
}


#if !defined(OS_darwin) && !defined(_WIN32) && !defined(__CYGWIN__)
void guialert(const char *fmt, ...)
{
	va_list args;
	
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	fputs("\n", stderr);
	va_end(args);
#ifdef SDL_GUI
	char *buf = NULL;
	va_start(args, fmt);
	int ret = vasprintf(&buf, fmt, args);
	va_end(args);
	if (ret >= 0)
	{
		startupGUI = true;
		startupAlert = buf;
	}
#else
	// FIXME: assuming some unix; use external tool to display alert
#endif
}
#endif


/* we don't link to SDL_main */
#undef main
#if defined(__MACOS__) || defined(__MACOSX__)
/* on macOS, the systems entry point is in SDLMain.M */
#define main SDL_main
#endif


/*
 *  Main program
 */
int main(int argc, char **argv)
{
#if defined _WIN32 || defined(OS_cygwin)
	SDL_SetMainReady();
#endif

	// Initialize variables
	RAMBaseHost = NULL;
	ROMBaseHost = NULL;
	HWBaseHost = NULL;
	FastRAMBaseHost = NULL;

	// remember program name
	program_name = argv[0];

#ifdef DEBUGGER
	ndebug::init();
#endif

	// display version string on console (help when users provide debug info)
	infoprint("%s", version_string);

	// parse command line switches
	if (!decode_switches(argc, argv))
		exit(EXIT_FAILURE);

#if DIRECT_ADDRESSING || FIXED_ADDRESSING
	// Initialize VM system
	vm_init();
#endif

#if FIXED_ADDRESSING
	if (!allocate_all_memory(fixed_memory_offset, false))
#else
	if (!allocate_all_memory(false))
#endif
	{
		vm_probe_fixed_hint();
		QuitEmulator();
	}
	
	// Initialize everything
	D(bug("Initializing All Modules..."));
	if (!InitAll())
		QuitEmulator();
	D(bug("Initialization complete"));

	install_sigsegv();
	D(bug("Sigsegv handler installed"));
	if (!install_signal_handler(false))
	{
		vm_probe_fixed_hint();
		QuitEmulator();
	}

#ifdef OS_darwin
	refreshMenuKeys();
#endif

	// Start 68k and jump to ROM boot routine
	D(bug("Starting emulation..."));
	Start680x0();

	// returning from emulation after the NMI
	remove_signal_handler();

	QuitEmulator(exit_val);

	return exit_val;
}


/*
 *  Quit emulator
 */
void QuitEmulator(int exitcode)
{
	D(bug("QuitEmulator"));

#if EMULATED_68K
	// Exit 680x0 emulation
	Exit680x0();
#endif

	ExitAll();

	// Free ROM/RAM areas
	release_all_memory();

	// Exit VM wrappers
	vm_exit();

	exit(exitcode); // the Quit is real
}
