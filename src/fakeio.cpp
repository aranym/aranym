/* Joy 2001 */

#include "sysdeps.h"
#include "hardware.h"
#include "cpu_emulation.h"
#include "memory.h"
#include "icio.h"
#include "parameters.h"
#include "debug.h"

uae_u8 BASE_IO::handleRead(uaecptr addr) {
	D(bug("HWget_b %x <- %s at %08x", addr, debug_print_IO(addr), showPC()));
	return *((uae_u8 *)(addr) + MEMBaseDiff);	// fetch from underlying RAM
}

uae_u16 BASE_IO::handleReadW(uaecptr addr) {
	D(bug("HWget_w %x <- %s at %08x", addr, debug_print_IO(addr), showPC()));
	return *((uae_u8 *)(addr) + MEMBaseDiff) << 8 | *((uae_u8 *)(addr+1) + MEMBaseDiff);
}

uae_u32 BASE_IO::handleReadL(uaecptr addr) {
	D(bug("HWget_l %x <- %s at %08x", addr, debug_print_IO(addr), showPC()));
	return *((uae_u8 *)(addr) + MEMBaseDiff) << 24 | *((uae_u8 *)(addr+1) + MEMBaseDiff) << 16 |
		 *((uae_u8 *)(addr+2) + MEMBaseDiff) << 8 | *((uae_u8 *)(addr+3) + MEMBaseDiff);
}

void BASE_IO::handleWrite(uaecptr addr, uae_u8 value) {
	D(bug("HWput_b %x = %d ($%x) <- %s at %08x\n", addr, value, value, debug_print_IO(addr), showPC()));
	*((uae_u8 *)(addr) + MEMBaseDiff) = value;	// store to underlying RAM
}

void BASE_IO::handleWriteW(uaecptr addr, uae_u16 value) {
	D(bug("HWput_w %x = %d ($%x) <- %s at %08x\n", addr, value, value, debug_print_IO(addr), showPC()));
	*((uae_u8 *)(addr) + MEMBaseDiff) = value >> 8;	// store to underlying RAM
	*((uae_u8 *)(addr+1) + MEMBaseDiff) = value;	// store to underlying RAM
}

void BASE_IO::handleWriteL(uaecptr addr, uae_u32 value) {
	D(bug("HWput_l %x = %d ($%x) <- %s at %08x\n", addr, value, value, debug_print_IO(addr), showPC()));
	*((uae_u8 *)(addr) + MEMBaseDiff) = value >> 24;	// store to underlying RAM
	*((uae_u8 *)(addr+1) + MEMBaseDiff) = value >> 16;	// store to underlying RAM
	*((uae_u8 *)(addr+2) + MEMBaseDiff) = value >> 8;	// store to underlying RAM
	*((uae_u8 *)(addr+3) + MEMBaseDiff) = value;	// store to underlying RAM
}
