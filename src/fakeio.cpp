/* Joy 2001 */

#include "sysdeps.h"
#include "hardware.h"
#include "cpu_emulation.h"
#include "memory.h"
#include "icio.h"
#include "parameters.h"

#ifdef HW_SIGSEGV
# define FakeIOPlace (addr - HWBase + FakeIOBaseHost)
#else
# define FakeIOPlace Atari2HostAddr(addr)
#endif

uae_u8 BASE_IO::handleRead(uaecptr addr) {
	return do_get_mem_byte((uae_u8 *)FakeIOPlace); // fetch from underlying RAM
}

uae_u16 BASE_IO::handleReadW(uaecptr addr) {
	return (handleRead(addr) << 8) | handleRead(addr+1);
}

uae_u32 BASE_IO::handleReadL(uaecptr addr) {
	return (handleReadW(addr) << 16) | handleReadW(addr+2);
}

void BASE_IO::handleWrite(uaecptr addr, uae_u8 value) {
	do_put_mem_byte((uae_u8 *)FakeIOPlace, value); // store to underlying RAM
}

void BASE_IO::handleWriteW(uaecptr addr, uae_u16 value) {
	handleWrite(addr, value >> 8);
	handleWrite(addr+1, value);
}

void BASE_IO::handleWriteL(uaecptr addr, uae_u32 value) {
	handleWriteW(addr, value >> 16);
	handleWriteW(addr+2, value);
}
