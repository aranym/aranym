
#include "sysdeps.h"
#include "memory.h"

#include <csignal>

#define DEBUG 1
#include "debug.h"

#if (__i386__)

static void segfault_vec(int x, struct sigcontext sc) {
	memptr addr = sc.cr2;
	memptr ainstr = sc.eip;
	int instr = (int)*(int *)ainstr;

	D(panicbug("fault address is %08x at %08x", addr, ainstr));
	D(panicbug("instruction is %08x", instr));
#if 0
	if (addr <= (FastRAM_BEGIN + FastRAM_SIZE - size)) {
		if (!write)
			return;
		else {
			// first two longwords of ST-RAM are ROM
			if ((addr >= FastRAM_BEGIN) || (addr >= 8 && addr <= (STRAM_END - size)))
				return;
		}
	}
#ifdef FIXED_VIDEORAM
	if (addr >= ARANYMVRAMSTART && addr <= (ARANYMVRAMSTART + ARANYMVRAMSIZE - size))
#else
	if (addr >= VideoRAMBase && addr <= (VideoRAMBase + ARANYMVRAMSIZE - size))
#endif
		return;

	// D(bug("BUS ERROR %s at $%x\n", (write ? "writting" : "reading"), addr));
#endif
	regs.mmu_fault_addr = addr;
	longjmp(excep_env, 2);
//	exit(0);
}

#endif

void install_sigsegv() {
	signal(SIGSEGV, (sighandler_t)segfault_vec);
}
