/* MJ 2001 */

#include "sysdeps.h"
#include "hardware.h"
#include "cpu_emulation.h"
#include "memory.h"
#include "mfp.h"
#include "acia.h"
#include "acsifdc.h"
#include "rtc.h"
#include "blitter.h"
#include "videl.h"
#include "ide.h"
#include "uae_cpu/newcpu.h"	// for regs.pc

MFP mfp;
IKBD ikbd;
MIDI midi;
ACSIFDC fdc;
RTC rtc;
IDE ide;
BLITTER blitter;
VIDEL videl;

int snd_reg;
extern int snd_porta;

bool dP = false;

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

uaecptr showPC() {
	return do_get_virtual_address(regs.pc_p);
}

uae_u32 handleRead(uaecptr addr) {
	if (addr >= HW_STMFP && addr < HW_FPU)
		return mfp.handleRead(addr);
	else if (addr >= HW_FDC && addr < HW_SCSI)
		return fdc.handleRead(addr);
	else if (addr >= HW_BLITT && addr < HW_SCC)
		return blitter.handleRead(addr);
	else if (addr >= HW_VIDEO && addr < HW_FDC)
		return videl.handleRead(addr);
	else if (addr >= HW_RTC && addr < (HW_RTC+4))
		return rtc.handleRead(addr);
	else if (addr >= HW_IDE && addr < (HW_IDE+0x3a))
		return ide.handleRead(addr);
	else if (addr >= HW_IKBD && addr < HW_MIDI)
		return ikbd.handleRead(addr);
	else if (addr >= HW_MIDI && addr < HW_MIDI + 3)
		return midi.handleRead(addr);

	else if (addr == 0xff8006)
		return 0x96;
	else if (addr == 0xff8007)
		return 0x61;
	else if (addr == 0xffa202)
		return 0xff;	// DSP interrupt
	else if (addr == HW_YAMAHA && snd_reg == 14)
		return snd_porta;
	else {
		fprintf(stderr, "HWget_b %x <- %s at %08x\n", addr, debug_print_IO(addr), showPC());
		return 0;
	}
}

void handleWrite(uaecptr addr, uae_u8 value) {
	if (addr >= HW_STMFP && addr < HW_FPU)
		mfp.handleWrite(addr, value);
	else if (addr >= HW_FDC && addr < HW_SCSI)
		fdc.handleWrite(addr, value);
	else if (addr > HW_BLITT && addr < HW_SCC)
		blitter.handleWrite(addr, value);
	else if (addr >= HW_VIDEO && addr < HW_FDC)
		videl.handleWrite(addr, value);
	else if (addr >= HW_RTC && addr < (HW_RTC+4))
		rtc.handleWrite(addr, value);
	else if (addr >= HW_IDE && addr < (HW_IDE+0x3a))
		ide.handleWrite(addr, value);
	else if (addr >= HW_IKBD && addr < HW_MIDI)
		ikbd.handleWrite(addr, value);
	else if (addr >= HW_MIDI && addr < HW_MIDI + 3)
		midi.handleWrite(addr, value);

	else if (addr == HW_YAMAHA)
		snd_reg = value;
	else if (addr == (HW_YAMAHA+2) && snd_reg == 14)
		snd_porta = value;
	else
		fprintf(stderr, "HWput_b %x,%u ($%02x) -> %s at %08x\n", addr, value, value, debug_print_IO(addr), showPC());
}

void MakeMFPIRQ(int no) {
	mfp.IRQ(no);
}

void ikbd_send(int code) {
	ikbd.ikbd_send(code);
}

uae_u32 HWget_l (uaecptr addr) {
//	uae_u32 * const m = (uae_u32 *)do_get_real_address(addr);
//	return do_get_mem_long(m);
	if (dP)
		fprintf(stderr, "HWget_l %x <- %s at %08x\n", addr, debug_print_IO(addr), showPC());
	return (handleRead(addr) << 24) | (handleRead(addr+1) << 16) | (handleRead(addr+2) << 8) | handleRead(addr+3);
}

uae_u32 HWget_w (uaecptr addr) {
//	uae_u16 * const m = (uae_u16 *)do_get_real_address(addr);
//	return do_get_mem_word(m);
	if (dP)
		fprintf(stderr, "HWget_w %x <- %s at %08x\n", addr, debug_print_IO(addr), showPC());
	return (handleRead(addr) << 8) | handleRead(addr+1);
}

uae_u32 HWget_b (uaecptr addr) {
//	uae_u8 * const m = (uae_u8 *)do_get_real_address(addr);
//	return do_get_mem_byte(m);
	if (dP)
		fprintf(stderr, "HWget_b %x <- %s at %08x\n", addr, debug_print_IO(addr), showPC());
	return handleRead(addr);
}

void HWput_l (uaecptr addr, uae_u32 l) {
//	uae_u32 * const m = (uae_u32 *)do_get_real_address(addr);
//	do_put_mem_long(m, l);
	if (dP)
		fprintf(stderr, "HWput_l %x,%d ($%08x) -> %s at %08x\n", addr, l, l, debug_print_IO(addr), showPC());
	handleWrite(addr, l >> 24);
	handleWrite(addr+1, l >> 16);
	handleWrite(addr+2, l >> 8);
	handleWrite(addr+3, l);
}

void HWput_w (uaecptr addr, uae_u32 w) {
//	uae_u16 * const m = (uae_u16 *)do_get_real_address(addr);
//	do_put_mem_word(m, w);
	if (dP)
		fprintf(stderr, "HWput_w %x,%d ($%04x) -> %s at %08x\n", addr, w, w, debug_print_IO(addr), showPC());
	handleWrite(addr, w >> 8);
	handleWrite(addr+1, w);
}

void HWput_b (uaecptr addr, uae_u32 b) {
//	uae_u8 * const m = (uae_u8 *)do_get_real_address(addr);
//	do_put_mem_byte(m, b);
	unsigned int bb = b & 0x000000ff;
	if (dP)
		fprintf(stderr, "HWput_b %x,%u ($%02x) -> %s at %08x\n", addr, bb, bb, debug_print_IO(addr), showPC());
	handleWrite(addr, b);
}
