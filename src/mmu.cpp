/* Joy 2001 */

#include "sysdeps.h"
#include "hardware.h"
#include "cpu_emulation.h"
#include "memory.h"
#include "mmu.h"
#include "parameters.h"

MMU::MMU() {
}

static const int HW = 0xff8000;

uae_u8 MMU::handleRead(uaecptr addr) {
	addr -= HW;
	switch(addr) {
		case 1: return addr;
		case 6: return monitor == 0 ? 0xa6 : 0xe6;	// a6 = 14MB, 96 = 4MB on VGA
		case 7: return 0x61;
	}

	return 0;
}

void MMU::handleWrite(uaecptr addr, uae_u8 value) {
	addr -= HW;
	switch(addr) {
		case 1: break;
		case 6: break;
		case 7: break;
	}
}
