/* MJ 2001 */

#include "sysdeps.h"
#include "hardware.h"
#include "cpu_emulation.h"
#include "memory.h"
#include "mfp.h"

MFP mfp;

void HWInit (void) {
	/*
	put_long(MEM_VALID_1_A,MEM_VALID_1);
	put_long(MEM_VALID_2_A,MEM_VALID_2);
	put_long(MEM_VALID_3_A,MEM_VALID_3);
	put_byte(MEM_CTL_A,MEM_CTL);
	put_word(SYS_CTL_A,SYS_CTL);
	*/
}

static const int HW_IDE 	= 0xf00000;
static const int HW_ROM	= 0xfa0000;
static const int HW_MMU 	= 0xff8000;
static const int HW_VIDEO 	= 0xff8200;
static const int HW_FDC	= 0xff8600;
static const int HW_SCSI	= 0xff8700;
static const int HW_YAMAHA	= 0xff8800;
static const int HW_SOUND	= 0xff8900;
static const int HW_DSP	= 0xff8930;
static const int HW_RTC	= 0xff8960;
static const int HW_BLITT	= 0xff89a0;
static const int HW_SCC	= 0xff8c00;
static const int HW_SCU	= 0xff8e00;
static const int HW_PADDLE	= 0xff9200;
static const int HW_VIDEL	= 0xff9800;
static const int HW_DSPH	= 0xffa200;
static const int HW_STMFP	= 0xfffa00;
static const int HW_FPU	= 0xfffa40;
static const int HW_TTMFP	= 0xfffa80;
static const int HW_IKBD	= 0xfffc00;
static const int HW_MIDI	= 0xfffc04;

static char* debug_print_IO(uaecptr addr) {
	if (addr < HW_ROM)
		return "IDE";
	if (addr < HW_MMU)
		return "Cartridge";
	else if (addr < HW_VIDEO)
		return "MMU";
	else if (addr < HW_FDC)
		return "Video";
	else if (addr < HW_SCSI)
		return "Floppy";
	else if (addr < HW_YAMAHA)
		return "SCSI";
	else if (addr < HW_SOUND)
		return "Yamaha";
	else if (addr < HW_DSP)
		return "DMA Sound";
	else if (addr < HW_RTC)
		return "DSP DMA";
	else if (addr < HW_BLITT)
		return "RTC";
	else if (addr < HW_SCC)
		return "Blitter";
	else if (addr < HW_SCU)
		return "SCC";
	else if (addr < HW_PADDLE)
		return "SCU";
	else if (addr < HW_VIDEL)
		return "Paddle";
	else if (addr < HW_DSPH)
		return "Videl";
	else if (addr < HW_STMFP)
		return "DSP Host";
	else if (addr < HW_FPU)
		return "MFP";
	else if (addr < HW_TTMFP)
		return "FPU";
	else if (addr < HW_IKBD)
		return "TT MFP";
	else if (addr < HW_MIDI)
		return "IKBD ACIA";
	else
		return "MIDI ACIA";
}

uae_u32 HWget_l (uaecptr addr) {
//	uae_u32 * const m = (uae_u32 *)do_get_real_address(addr);
//	return do_get_mem_long(m);
	fprintf(stderr, "HWget_l %x <- %s\n", addr, debug_print_IO(addr));
	return mfp.handleRead(addr);
}

uae_u32 HWget_w (uaecptr addr) {
//	uae_u16 * const m = (uae_u16 *)do_get_real_address(addr);
//	return do_get_mem_word(m);
	fprintf(stderr, "HWget_w %x <- %s\n", addr, debug_print_IO(addr));
	return mfp.handleRead(addr);
}

uae_u32 HWget_b (uaecptr addr) {
//	uae_u8 * const m = (uae_u8 *)do_get_real_address(addr);
//	return do_get_mem_byte(m);
	fprintf(stderr, "HWget_b %x <- %s\n", addr, debug_print_IO(addr));
	return mfp.handleRead(addr);
}

void HWput_l (uaecptr addr, uae_u32 l) {
//	uae_u32 * const m = (uae_u32 *)do_get_real_address(addr);
//	do_put_mem_long(m, l);
	fprintf(stderr, "HWput_l %x,%d ($%08x) -> %s\n", addr, l, l, debug_print_IO(addr));
	mfp.handleWrite(addr, l >> 24);
	mfp.handleWrite(addr+1, l >> 16);
	mfp.handleWrite(addr+2, l >> 8);
	mfp.handleWrite(addr+3, l);
}

void HWput_w (uaecptr addr, uae_u32 w) {
//	uae_u16 * const m = (uae_u16 *)do_get_real_address(addr);
//	do_put_mem_word(m, w);
	fprintf(stderr, "HWput_w %x,%d ($%04x) -> %s\n", addr, w, w, debug_print_IO(addr));
	mfp.handleWrite(addr, w >> 8);
	mfp.handleWrite(addr+1, w);
}

void HWput_b (uaecptr addr, uae_u32 b) {
//	uae_u8 * const m = (uae_u8 *)do_get_real_address(addr);
//	do_put_mem_byte(m, b);
	unsigned int bb = b & 0x000000ff;
	fprintf(stderr, "HWput_b %x,%u ($%02x) -> %s\n", addr, bb, bb, debug_print_IO(addr));
	mfp.handleWrite(addr, b);
}
