/* Joy 2001 */

#include "sysdeps.h"
#include "hardware.h"
#include "cpu_emulation.h"
#include "memory.h"
#include "mmu.h"
#include "parameters.h"
#include "ide.h"
#include "ata.h"

bx_hard_drive_c ata;

void IDE::init() {
	ata.init();
}

static const int HW = 0xf00000;

uae_u8 IDE::handleRead(uaecptr addr) {
	return ata.read_handler(&ata, addr, 1);
}

void IDE::handleWrite(uaecptr addr, uae_u8 value) {
	ata.write_handler(&ata, addr, value, 1);
}

uae_u16 IDE::handleReadW(uaecptr addr) {
	return ata.read_handler(&ata, addr, 2);
}

void IDE::handleWriteW(uaecptr addr, uae_u16 value) {
	ata.write_handler(&ata, addr, value, 2);
}

uae_u32 IDE::handleReadL(uaecptr addr) {
#if 0
	uint32 a = 0;
	a = ata.read_handler(&ata, addr, 2) << 16;
	a |= ata.read_handler(&ata, addr, 2);
	return a;
#else
	return ata.read_handler(&ata, addr, 4);
#endif
}

void IDE::handleWriteL(uaecptr addr, uae_u32 value) {
#if 0
    uint16 a = value >> 16;
    uint16 b = value;
	ata.write_handler(&ata, addr, a, 2);
	ata.write_handler(&ata, addr, b, 2);
#else
	ata.write_handler(&ata, addr, value, 4);
#endif
}
