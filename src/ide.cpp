/* Joy 2001 */

#include "sysdeps.h"
#include "hardware.h"
#include "cpu_emulation.h"
#include "memory.h"
#include "mmu.h"
#include "parameters.h"
#include "ide.h"
#include "ata.h"

void IDE::init() {
	bx_hard_drive.init();
}

static const int HW = 0xf00000;

uae_u8 IDE::handleRead(uaecptr addr) {
	return bx_hard_drive.read_handler(&bx_hard_drive, addr, 1);
}

void IDE::handleWrite(uaecptr addr, uae_u8 value) {
	bx_hard_drive.write_handler(&bx_hard_drive, addr, value, 1);
}

uae_u16 IDE::handleReadW(uaecptr addr) {
	return bx_hard_drive.read_handler(&bx_hard_drive, addr, 2);
}

void IDE::handleWriteW(uaecptr addr, uae_u16 value) {
	bx_hard_drive.write_handler(&bx_hard_drive, addr, value, 2);
}

uae_u32 IDE::handleReadL(uaecptr addr) {
#if 0
	uint32 a = 0;
	a = bx_hard_drive.read_handler(&bx_hard_drive, addr, 2) << 16;
	a |= bx_hard_drive.read_handler(&bx_hard_drive, addr, 2);
	return a;
#else
	return bx_hard_drive.read_handler(&bx_hard_drive, addr, 4);
#endif
}

void IDE::handleWriteL(uaecptr addr, uae_u32 value) {
#if 0
    uint16 a = value >> 16;
    uint16 b = value;
	bx_hard_drive.write_handler(&bx_hard_drive, addr, a, 2);
	bx_hard_drive.write_handler(&bx_hard_drive, addr, b, 2);
#else
	bx_hard_drive.write_handler(&bx_hard_drive, addr, value, 4);
#endif
}
