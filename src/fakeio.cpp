/* Joy 2001 */

#include "sysdeps.h"
#include "hardware.h"
#include "cpu_emulation.h"
#include "memory.h"
#include "icio.h"
#include "parameters.h"
#include "debug.h"

#ifdef USE_JIT
# define FakeIOPlace (addr - HWBase + FakeIOBaseHost)
#else
# define FakeIOPlace Atari2HostAddr(addr)
#endif

uae_u8 BASE_IO::handleRead(uaecptr addr) {
	D(bug("HWget_b %x <- %s at %08x", addr, debug_print_IO(addr), showPC()));
	return do_get_mem_byte((uae_u8 *)FakeIOPlace); // fetch from underlying RAM
}

uae_u16 BASE_IO::handleReadW(uaecptr addr) {
	D(bug("HWget_w %x <- %s at %08x", addr, debug_print_IO(addr), showPC()));
	return do_get_mem_word((uae_u16 *)FakeIOPlace);
}

uae_u32 BASE_IO::handleReadL(uaecptr addr) {
	D(bug("HWget_l %x <- %s at %08x", addr, debug_print_IO(addr), showPC()));
	return do_get_mem_long((uae_u32 *)FakeIOPlace);
}

void BASE_IO::handleWrite(uaecptr addr, uae_u8 value) {
	D(bug("HWput_b %x = %d ($%x) <- %s at %08x\n", addr, value, value, debug_print_IO(addr), showPC()));
	do_put_mem_byte((uae_u8 *)FakeIOPlace, value); // store to underlying RAM
}

void BASE_IO::handleWriteW(uaecptr addr, uae_u16 value) {
	D(bug("HWput_w %x = %d ($%x) <- %s at %08x\n", addr, value, value, debug_print_IO(addr), showPC()));
	do_put_mem_word((uae_u16 *)FakeIOPlace, value);
}

void BASE_IO::handleWriteL(uaecptr addr, uae_u32 value) {
	D(bug("HWput_l %x = %d ($%x) <- %s at %08x\n", addr, value, value, debug_print_IO(addr), showPC()));
	do_put_mem_long((uae_u32 *)FakeIOPlace, value);
}
