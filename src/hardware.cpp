/* MJ 2001 */

#include "sysdeps.h"
#include "hardware.h"
#include "cpu_emulation.h"
#include "memory.h"

void HWInit (void) {
	put_byte(MEM_CTL_A,MEM_CTL);
	put_word(SYS_CTL_A,SYS_CTL);
}

uae_u32 HWget_l (uaecptr addr) {
	uae_u32 * const m = (uae_u32 *)do_get_real_address(addr);
	return do_get_mem_long(m);
}

uae_u32 HWget_w (uaecptr addr) {
	uae_u16 * const m = (uae_u16 *)do_get_real_address(addr);
	return do_get_mem_word(m);
}

uae_u32 HWget_b (uaecptr addr) {
	uae_u8 * const m = (uae_u8 *)do_get_real_address(addr);
	return do_get_mem_byte(m);
}

void HWput_l (uaecptr addr, uae_u32 l) {
	uae_u32 * const m = (uae_u32 *)do_get_real_address(addr);
	do_put_mem_long(m, l);
}

void HWput_w (uaecptr addr, uae_u32 w) {
	uae_u16 * const m = (uae_u16 *)do_get_real_address(addr);
	do_put_mem_word(m, w);
}

void HWput_b (uaecptr addr, uae_u32 b) {
	uae_u8 * const m = (uae_u8 *)do_get_real_address(addr);
	do_put_mem_byte(m, b);
}
