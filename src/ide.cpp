/* Joy 2001 */

#include "sysdeps.h"
#include "hardware.h"
#include "cpu_emulation.h"
#include "memory.h"
#include "mmu.h"
#include "parameters.h"
#include "ide.h"
#include "ata.h"

IDE::IDE(memptr addr, uint32 size) : BASE_IO(addr, size) {
	bx_hard_drive.init();
}

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
	return bx_hard_drive.read_handler(&bx_hard_drive, addr, 4);
}

void IDE::handleWriteL(uaecptr addr, uae_u32 value) {
	bx_hard_drive.write_handler(&bx_hard_drive, addr, value, 4);
}
