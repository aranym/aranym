/*
 * $Header$
 *
 */

#include "sysdeps.h"
#include "hardware.h"
#include "cpu_emulation.h"
#include "memory.h"
#include "icio.h"
#include "mfp.h"
#include "acia.h"
#include "acsifdc.h"
#include "rtc.h"
#include "blitter.h"
#include "ide.h"
#include "videl.h"
#include "yamaha.h"
#include "dsp.h"
#include "mmu.h"
#include "hostscreen.h"
#include "exceptions.h"
#include "uae_cpu/newcpu.h"	// for regs.pc

#define DEBUG 0
#include "debug.h"

// host OS dependent objects
HostScreen hostScreen;

// chipset IOs
BASE_IO fake_io;
MMU mmu;
MFP mfp;
IKBD ikbd;
MIDI midi;
ACSIFDC fdc;
RTC rtc;
IDE ide;
DSP dsp;
BLITTER blitter;
VIDEL videl;
YAMAHA yamaha;

#define BUS_ERROR	longjmp(excep_env, 2)

void renderScreen() { videl.renderScreen(); }
int getFloppyStats() { return yamaha.getFloppyStat(); }
bool isIkbdBufEmpty() { return ikbd.isBufferEmpty(); }
void MakeMFPIRQ(int no) { mfp.IRQ(no); }
void ikbd_send(int code) { ikbd.ikbd_send(code); }

void HWInit (void) {
	rtc.init();
	videl.init();
	ide.init();
}

/* obsolete */
static const uint HW_IDE 	= 0xf00000;
static const uint HW_ROM	= 0xfa0000;
static const uint HW_MMU 	= 0xff8000;
static const uint HW_VIDEO 	= 0xff8200;
static const uint HW_FDC	= 0xff8600;
static const uint HW_SCSI	= 0xff8700;
static const uint HW_YAMAHA	= 0xff8800;
static const uint HW_SOUND	= 0xff8900;
static const uint HW_DSP	= 0xff8930;
static const uint HW_RTC	= 0xff8960;
static const uint HW_BLITT	= 0xff89a0;
static const uint HW_SCC	= 0xff8c00;
static const uint HW_SCU	= 0xff8e00;
static const uint HW_PADDLE	= 0xff9200;
static const uint HW_VIDEL	= 0xff9800;
static const uint HW_DSPH	= 0xffa200;
static const uint HW_STMFP	= 0xfffa00;
static const uint HW_FPU	= 0xfffa40;
static const uint HW_TTMFP	= 0xfffa80;
static const uint HW_IKBD	= 0xfffc00;
static const uint HW_MIDI	= 0xfffc04;
/* end of obsolete */

struct HARDWARE {
	char name[32];
	uint	begin;
	uint len;	// TODO replace len with end to save some CPU cycles in runtime
	ICio *handle;
};

HARDWARE ICs[] = {
	{"IDE", 0xf00000, 0x3a, &ide},
	{"Aranym", 0xf90000, 0x04, &fake_io},
	{"Cartridge", 0xfa0000, 0x20000, &fake_io},
	{"Memory Management", 0xff8000, 8, &mmu},
	{"VIDEL", 0xff8200, 0xc4, &videl},
	{"DMA/FDC", 0xff8600, 0x10, &fdc},
//	{"DMA/SCSI", 0xff8700, 0x16, &fake_io},
// 	{"SCSI", 0xff8780, 0x10, &fake_io},
	{"Yamaha", 0xff8800, 4, &yamaha},
	{"Sound", 0xff8900, 0x22, &fake_io},
//	{"MicroWire", 0xff8922, 0x4},
	{"DMA/DSP", 0xff8930, 0x14, &fake_io},
	{"TT RTC", 0xff8960, 4, &rtc},
	{"BLiTTER", 0xff8A00, 0x3e, &blitter},
//	{"DMA/SCC", 0xff8C00, 0x16},
	{"SCC", 0xff8C80, 0x16, &fake_io},
//	{"VME", 0xff8e00, 0x0c},
	{"Paddle", 0xff9200, 0x24, &fake_io},
	{"VIDEL Pallete", 0xff9800, 0x400, &videl},
	{"DSP", 0xffa200, 8, &dsp},
	{"STMFP", 0xfffa00, 0x30, &mfp},
//	{"STFPC", 0xfffa40, 8},
	{"IKBD", 0xfffc00, 4, &ikbd},
	{"MIDI", 0xfffc04, 4, &midi}
//	{"RTC", 0xfffc20, 0x20}
};

/*static*/char* debug_print_IO(uaecptr addr) {
	int len = sizeof(ICs) / sizeof(ICs[0]);
	for(int i=0; i<len; i++)
		if (addr >= ICs[i].begin && addr < (ICs[i].begin + ICs[i].len))
			return ICs[i].name;
	return "Unknown";
}

uaecptr showPC() {
	return regs.pcp;
}

uae_u32 handleRead(uaecptr addr) {
	int len = sizeof(ICs) / sizeof(ICs[0]);
	for(int i=0; i<len; i++) {
		if (addr >= ICs[i].begin && addr < (ICs[i].begin + ICs[i].len)) {
			ICio *ptr = ICs[i].handle;
			return ptr->handleRead(addr);
		}
	}
	D(bug("HWget_b %x <- %s at %08x", addr, debug_print_IO(addr), showPC()));
	BUS_ERROR;
}

void handleWrite(uaecptr addr, uae_u8 value) {
	int len = sizeof(ICs) / sizeof(ICs[0]);
	for(int i=0; i<len; i++) {
		if (addr >= ICs[i].begin && addr < (ICs[i].begin + ICs[i].len)) {
			ICio *ptr = ICs[i].handle;
			ptr->handleWrite(addr, value);
			return;
		}
	}
	D(bug("HWput_b %x = %d ($%x) <- %s at %08x", addr, value, value, debug_print_IO(addr), showPC()));
	BUS_ERROR;
}

uae_u32 HWget_l (uaecptr addr) {
//	uae_u32 * const m = (uae_u32 *)do_get_real_address(addr);
//	return do_get_mem_long(m);
	D(bug("HWget_l %x <- %s at %08x", addr, debug_print_IO(addr), showPC()));
	// return (handleRead(addr) << 24) | (handleRead(addr+1) << 16) | (handleRead(addr+2) << 8) | handleRead(addr+3);
/*
	if (addr >= 0xf00000 && addr < 0xf0003a)
		return ide.read_handler(&ide, addr, 4);
*/
	if (addr == HW_IDE) {
		uae_u16 x = HWget_w(addr);
		uae_u16 y = HWget_w(addr);
		return (x << 16)| y;
	}
	else
		return (HWget_w(addr) << 16) | HWget_w(addr+2);
}

uae_u32 HWget_w (uaecptr addr) {
//	uae_u16 * const m = (uae_u16 *)do_get_real_address(addr);
//	return do_get_mem_word(m);
	D(bug("HWget_w %x <- %s at %08x", addr, debug_print_IO(addr), showPC()));
	if (addr == HW_IDE)
		return ide.handleReadW(addr);
	else
		return (handleRead(addr) << 8) | handleRead(addr+1);
}

uae_u32 HWget_b (uaecptr addr) {
//	uae_u8 * const m = (uae_u8 *)do_get_real_address(addr);
//	return do_get_mem_byte(m);
	D(bug("HWget_b %x <- %s at %08x", addr, debug_print_IO(addr), showPC()));
	return handleRead(addr);
}

void HWput_l (uaecptr addr, uae_u32 l) {
//	uae_u32 * const m = (uae_u32 *)do_get_real_address(addr);
//	do_put_mem_long(m, l);
	D(bug("HWput_l %x,%d ($%08x) -> %s at %08x", addr, l, l, debug_print_IO(addr), showPC()));
	if (addr == HW_IDE) {
		HWput_w(addr, l >> 16);
		HWput_w(addr, l & 0x0000ffff);
	}
	else {
		handleWrite(addr, l >> 24);
		handleWrite(addr+1, l >> 16);
		handleWrite(addr+2, l >> 8);
		handleWrite(addr+3, l);
	}
}

void HWput_w (uaecptr addr, uae_u32 w) {
//	uae_u16 * const m = (uae_u16 *)do_get_real_address(addr);
//	do_put_mem_word(m, w);
	D(bug("HWput_w %x,%d ($%04x) -> %s at %08x", addr, w, w, debug_print_IO(addr), showPC()));
	if (addr == HW_IDE)
		ide.handleWriteW(addr, w);
	else {
		handleWrite(addr, w >> 8);
		handleWrite(addr+1, w);
	}
}

void HWput_b (uaecptr addr, uae_u32 b) {
//	uae_u8 * const m = (uae_u8 *)do_get_real_address(addr);
//	do_put_mem_byte(m, b);
	D(bug("HWput_b %x,%u ($%02x) -> %s at %08x", addr, b & 0xff, b & 0xff, debug_print_IO(addr), showPC()));
	handleWrite(addr, b);
}


/*
 * $Log$
 * Revision 1.27  2001/08/21 18:19:16  milan
 * CPU update, disk's geometry autodetection - the 1st step
 *
 * Revision 1.26  2001/08/13 22:29:06  milan
 * IDE's params from aranymrc file etc.
 *
 * Revision 1.25  2001/07/24 13:54:11  joy
 * #define DEBUG before #include "debug"
 *
 * Revision 1.24  2001/07/24 06:40:33  joy
 * SCSI disabled
 * D(bug()) replaces printf
 *
 * Revision 1.23  2001/07/12 22:10:05  standa
 * updateHostScreen() function added to let the direct_fullscreen mode work again.
 *
 * Revision 1.22  2001/06/18 13:21:55  standa
 * Several template.cpp like comments were added.
 * HostScreen SDL encapsulation class.
 *
 *
 */
