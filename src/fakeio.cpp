/* Joy 2001 */

#include "sysdeps.h"
#include "hardware.h"
#include "cpu_emulation.h"
#include "memory.h"
#include "fakeio.h"
#include "parameters.h"

extern bool dP;

FAKEIO::FAKEIO(bool bRAM) {
	use_RAM = bRAM;
}

uae_u8 FAKEIO::handleRead(uaecptr addr) {
	if (dP)
		fprintf(stderr, "HWget_b %x <- %s at %08x\n", addr, debug_print_IO(addr), showPC());
	if (use_RAM)
		return *((uae_u8 *)(addr) + MEMBaseDiff);	// fetch from underlying RAM
	else
		return 0xff;
}

void FAKEIO::handleWrite(uaecptr addr, uae_u8 value) {
	if (dP)
		fprintf(stderr, "HWput_b %x = %d ($%x) <- %s at %08x\n", addr, value, value, debug_print_IO(addr), showPC());
	if (use_RAM)
		*((uae_u8 *)(addr) + MEMBaseDiff) = value;	// store to underlying RAM
}
