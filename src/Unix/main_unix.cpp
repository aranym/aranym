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
#include <signal.h>
#include <errno.h>

#include <pthread.h>

#include <linux/fb.h>
#include <sys/mman.h>

#include "cpu_emulation.h"
#include "timer.h"
#include "version.h"
#include "main.h"

#define DEBUG 1
#include "debug.h"


// Constants
const char ROM_FILE_NAME[] = "ROM";
const int SIG_STACK_SIZE = SIGSTKSZ;	// Size of signal stack
const int SCRATCH_MEM_SIZE = 0x10000;	// Size of scratch memory area

// CPU and FPU type, addressing mode
int CPUType;
int FPUType;

static int zero_fd = -1;				// FD of /dev/zero

static pthread_t emul_thread;				// Handle of MacOS emulation thread (main thread)

static bool tick_thread_active = false;			// Flag: 60Hz thread installed
static volatile bool tick_thread_cancel = false;	// Flag: Cancel 60Hz thread
static pthread_t tick_thread;						// 60Hz thread
static pthread_attr_t tick_thread_attr;			// 60Hz thread attributes

static pthread_mutex_t intflag_lock = PTHREAD_MUTEX_INITIALIZER;	// Mutex to protect InterruptFlags

uint8 *ScratchMem = NULL;			// Scratch memory for Mac ROM writes

static struct sigaction timer_sa;	// sigaction used for timer

static uint32 mapped_ram_rom_size;		// Total size of mmap()ed RAM/ROM area

// Prototypes
static void *tick_func(void *arg);
static void one_tick(...);

void MakeIRQ();	// hardware.cpp
void init_fdc();	// fdc.cpp


/*
 *  Ersatz functions
 */


/*
 *  Main program
 */
#define MAXDRIVES	32
int drive_fd[MAXDRIVES];

int main(int argc, char **argv)
{
	char str[256];

	drive_fd[0] = drive_fd[1] = drive_fd[2] = -1;

	drive_fd[0] = open("/dev/fd0", O_RDONLY);
	if (drive_fd[0] >= 0)
    	init_fdc();

	// Initialize variables
	RAMBaseHost = NULL;
	ROMBaseHost = NULL;
	srand(time(NULL));
	tzset();

	// Open /dev/zero
	zero_fd = open("/dev/zero", O_RDWR);
	if (zero_fd < 0) {
		sprintf(str, "Couldn't open /dev/zero\n", strerror(errno));
		ErrorAlert(str);
		QuitEmulator();
	}

	// Read RAM size
	RAMSize = 14 * 1024 * 1024;	// Round down to 1MB boundary
	if (RAMSize < 1024*1024) {
		RAMSize = 1024*1024;
	}

	const uint32 page_size = getpagesize();
	const uint32 page_mask = page_size - 1;
	const uint32 aligned_ram_size = (RAMSize + page_mask) & ~page_mask;
	mapped_ram_rom_size = aligned_ram_size + 0x100000 + 0x100000;

	// Allocate scratch memory
	ScratchMem = (uint8 *)malloc(SCRATCH_MEM_SIZE);
	if (ScratchMem == NULL) {
		ErrorAlert("Not enough memory\n");
		QuitEmulator();
	}
	ScratchMem += SCRATCH_MEM_SIZE/2;	// ScratchMem points to middle of block

	// Create areas for Mac RAM and ROM
	// gb-- Overkill, needs to be cleaned up. Probably explode it for either
	// real or direct addressing mode.
	RAMBaseHost = (uint8 *)mmap(0, mapped_ram_rom_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, zero_fd, 0);
	if (RAMBaseHost == (uint8 *)MAP_FAILED) {
		ErrorAlert("Not enough memory\n");
		QuitEmulator();
	}
	ROMBaseHost = RAMBaseHost + aligned_ram_size;

	// Initialize MEMBaseDiff now so that Host2MacAddr in the Video module
	// will return correct results
	RAMBase = 0;
	ROMBase = RAMBase + aligned_ram_size;

/*
	char *fbname;
	fbname = getenv("FRAMEBUFFER");
	if (!fbname) fbname = "/dev/fb0";
	int fb;
	if ((fb = open(fbname,O_RDONLY | O_NONBLOCK)) == -1) {
		ErrorAlert("Open failed on FB\n");
		QuitEmulator();
	}
	// Nacist info
	(void)close(fb);
	if ((fb = open(fbname, O_RDWR)) == -1) {
		ErrorAlert("Open failed on FB\n");
		QuitEmulator();
	}
	if ((VideoRAMBaseHost = (uint8 *)mmap((void *)0,VideoRAMSize, PROT_READ | PROT_WRITE, MAP_SHARED, fb, 0)) == (void *)-1) {
		ErrorAlert("mmap failed\n");
	}
*/
	InitMEMBaseDiff(RAMBaseHost, RAMBase);
	InitVMEMBaseDiff(VideoRAMBaseHost, VideoRAMBase);
	D(bug("ST-RAM starts at %p (%08x)\n", RAMBaseHost, RAMBase));
	D(bug("TOS ROM starts at %p (%08x)\n", ROMBaseHost, ROMBase));
	D(bug("VideoRAM starts at %p (%08x)\n", VideoRAMBaseHost, VideoRAMBase));
	
	// Get rom file path from preferences
	const char *rom_path = "ROM";

	// Load Mac ROM
	int rom_fd = open(rom_path ? rom_path : ROM_FILE_NAME, O_RDONLY);
	if (rom_fd < 0) {
		ErrorAlert("ROM file not found\n");
		QuitEmulator();
	}
	printf("Reading ROM file...\n");
	ROMSize = lseek(rom_fd, 0, SEEK_END);
	if (ROMSize != 64*1024 && ROMSize != 128*1024 && ROMSize != 256*1024 && ROMSize != 512*1024 && ROMSize != 1024*1024) {
		ErrorAlert("Invalid ROM size\n");
		close(rom_fd);
		QuitEmulator();
	}
	lseek(rom_fd, 0, SEEK_SET);
	if (read(rom_fd, ROMBaseHost, ROMSize) != (ssize_t)ROMSize) {
		ErrorAlert("ROM file reading error\n");
		close(rom_fd);
		QuitEmulator();
	}

	// Initialize everything
	if (!InitAll())
		QuitEmulator();
	D(bug("Initialization complete\n"));

	// Get handle of main thread
	emul_thread = pthread_self();

	// POSIX threads available, start 60Hz thread
	pthread_attr_init(&tick_thread_attr);
#if defined(_POSIX_THREAD_PRIORITY_SCHEDULING)
	if (geteuid() == 0) {
		pthread_attr_setinheritsched(&tick_thread_attr, PTHREAD_EXPLICIT_SCHED);
		pthread_attr_setschedpolicy(&tick_thread_attr, SCHED_FIFO);
		struct sched_param fifo_param;
		fifo_param.sched_priority = (sched_get_priority_min(SCHED_FIFO) + sched_get_priority_max(SCHED_FIFO)) / 2;
		pthread_attr_setschedparam(&tick_thread_attr, &fifo_param);
	}
#endif
	tick_thread_active = (pthread_create(&tick_thread, &tick_thread_attr, tick_func, NULL) == 0);
	if (!tick_thread_active) {
		sprintf(str, "Tick thread error", strerror(errno));
		ErrorAlert(str);
		QuitEmulator();
	}
	D(bug("60Hz thread started\n"));

	// Start 68k and jump to ROM boot routine
	D(bug("Starting emulation...\n"));
	Start680x0();

	QuitEmulator();
	return 0;
}


/*
 *  Quit emulator
 */

void QuitEmulator(void)
{
	D(bug("QuitEmulator\n"));

	// Exit 680x0 emulation
	Exit680x0();

	// Stop 60Hz thread
	if (tick_thread_active) {
		tick_thread_cancel = true;
		pthread_cancel(tick_thread);
		pthread_join(tick_thread, NULL);
	}

	// Deinitialize everything
	ExitAll();

	// Free ROM/RAM areas
	if (RAMBaseHost != (uint8 *)MAP_FAILED) {
		munmap((caddr_t)RAMBaseHost, mapped_ram_rom_size);
		RAMBaseHost = NULL;
	}

	// Delete scratch memory area
	if (ScratchMem) {
		free((void *)(ScratchMem - SCRATCH_MEM_SIZE/2));
		ScratchMem = NULL;
	}

	// Close /dev/zero
	if (zero_fd > 0)
		close(zero_fd);

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
 *  Interrupt flags (must be handled atomically!)
 */

uint32 InterruptFlags = 0;

void SetInterruptFlag(uint32 flag)
{
	pthread_mutex_lock(&intflag_lock);
	InterruptFlags |= flag;
	pthread_mutex_unlock(&intflag_lock);
}

void ClearInterruptFlag(uint32 flag)
{
	pthread_mutex_lock(&intflag_lock);
	InterruptFlags &= ~flag;
	pthread_mutex_unlock(&intflag_lock);
}

/*
 *  200Hz thread
 */

const int TICKSPERSEC = 200U;
const int MICROSECSPERTICK = (1000000UL / TICKSPERSEC);

static void one_second(void)
{
/*
	// Pseudo Mac 1Hz interrupt, update local time
	WriteMacInt32(0x20c, TimerDateTime());

	SetInterruptFlag(INTFLAG_1HZ);
	TriggerInterrupt();
*/
//	fprintf(stderr, "one second: ($4BA) = %ld\n", ReadMacInt32(0x4ba));
	// SetInterruptFlag(INTFLAG_1HZ);
	// TriggerVBL();
}

static void one_tick(...)
{
	static int tick_counter = 0;
	static int VBL_counter = 0;
	if (++tick_counter > TICKSPERSEC) {
		tick_counter = 0;
		one_second();
	}
	if (++VBL_counter > 4) {
		VBL_counter = 0;
		TriggerVBL();
	}

	// Trigger 200Hz interrupt
	// SetInterruptFlag(INTFLAG_200HZ);
	MakeIRQ();
}

static void *tick_func(void *arg)
{
	uint64 next = GetTicks_usec();
	while (!tick_thread_cancel) {
		one_tick();
		next += MICROSECSPERTICK;
		int64 delay = next - GetTicks_usec();
		if (delay > 0)
			Delay_usec(delay);
		else if (delay < -MICROSECSPERTICK)
			next = GetTicks_usec();
	}
	return NULL;
}


/*
 *  Get current value of microsecond timer
 */

uint64 GetTicks_usec(void)
{
	struct timeval t;
	gettimeofday(&t, NULL);
	return (uint64)t.tv_sec * 1000000 + t.tv_usec;
}


/*
 *  Delay by specified number of microseconds (<1 second)
 *  (adapted from SDL_Delay() source; this function is designed to provide
 *  the highest accuracy possible)
 */

#if defined(linux)
// Linux select() changes its timeout parameter upon return to contain
// the remaining time. Most other unixen leave it unchanged or undefined.
#define SELECT_SETS_REMAINING
#elif defined(__FreeBSD__) || defined(__sun__) || defined(sgi)
#define USE_NANOSLEEP
#endif

void Delay_usec(uint32 usec)
{
	int was_error;

#ifdef USE_NANOSLEEP
	struct timespec elapsed, tv;
#else
	struct timeval tv;
#ifndef SELECT_SETS_REMAINING
	uint64 then, now, elapsed;
#endif
#endif

	// Set the timeout interval - Linux only needs to do this once
#ifdef SELECT_SETS_REMAINING
    tv.tv_sec = 0;
    tv.tv_usec = usec;
#elif defined(USE_NANOSLEEP)
    elapsed.tv_sec = 0;
    elapsed.tv_nsec = usec * 1000;
#else
    then = GetTicks_usec();
#endif

	do {
		errno = 0;
#ifdef USE_NANOSLEEP
		tv.tv_sec = elapsed.tv_sec;
		tv.tv_nsec = elapsed.tv_nsec;
		was_error = nanosleep(&tv, &elapsed);
#else
#ifndef SELECT_SETS_REMAINING
		// Calculate the time interval left (in case of interrupt)
		now = GetTicks_usec();
		elapsed = now - then;
		then = now;
		if (elapsed >= usec)
			break;
		usec -= elapsed;
		tv.tv_sec = 0;
		tv.tv_usec = usec;
#endif
		was_error = select(0, NULL, NULL, NULL, &tv);
#endif
	} while (was_error && (errno == EINTR));
}

/*
 *  Display error alert
 */

void ErrorAlert(const char *text)
{
	printf(text);
}


/*
 *  Display warning alert
 */

void WarningAlert(const char *text)
{
	printf(text);
}


/*
 *  Display choice alert
 */

bool ChoiceAlert(const char *text, const char *pos, const char *neg)
{
	printf(text);
	return false;	//!!
}
