/*
 * $Header$
 *
 */

#include "sysdeps.h"
#include "hardware.h"
#include "cpu_emulation.h"
#include "memory.h"
#include "icio.h"
#include "acsifdc.h"
#include "rtc.h"
#include "blitter.h"
#include "ide.h"
#include "dsp.h"
#include "mmu.h"
#include "hostscreen.h"
#include "parallel.h"

#define DEBUG 0
#include "debug.h"

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
ARADATA aradata;

#ifdef USE_JIT
extern int in_handler;
# define BUS_ERROR(a)	{ regs.mmu_fault_addr=(a); in_handler = 0; longjmp(excep_env, 2); }
#else
# define BUS_ERROR(a)	{ regs.mmu_fault_addr=(a); longjmp(excep_env, 2); }
#endif

Parallel parallel;

void HWInit (void) {
	rtc.init();
	videl.init();
	ide.init();
#if DSP_EMULATION
	dsp.init();
#endif
}

struct HARDWARE {
	char name[32];
	uint32	begin;
	uint32 len;	// TODO replace len with end to save some CPU cycles in runtime
	ICio *handle;
};

HARDWARE ICs[] = {
	{"IDE", 0xf00000, 0x3a, &ide},
	{"Aranym", 0xf90000, 0xffff, &aradata},
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
	{"DSP", HW_DSP, 8, &dsp},
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

uae_u32 handleRead(uaecptr addr) {
	int len = sizeof(ICs) / sizeof(ICs[0]);
	for(int i=0; i<len; i++) {
		if (addr >= ICs[i].begin && addr < (ICs[i].begin + ICs[i].len)) {
			ICio *ptr = ICs[i].handle;
			return ptr->handleRead(addr);
		}
	}
	D(bug("HWget_b %x <- %s at %08x", addr, debug_print_IO(addr), showPC()));
	BUS_ERROR(addr);
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
	BUS_ERROR(addr);
}

#define HW_IDE	0xf00000

uae_u32 HWget_l (uaecptr addr) {
	D(bug("HWget_l %x <- %s at %08x", addr, debug_print_IO(addr), showPC()));
	// return (handleRead(addr) << 24) | (handleRead(addr+1) << 16) | (handleRead(addr+2) << 8) | handleRead(addr+3);
/*
	if (addr >= 0xf00000 && addr < 0xf0003a)
		return ide.read_handler(&ide, addr, 4);
*/
	if (addr == HW_IDE)
		return ide.handleReadL(addr);
	else
		return (HWget_w(addr) << 16) | HWget_w(addr+2);
}

uae_u16 HWget_w (uaecptr addr) {
	D(bug("HWget_w %x <- %s at %08x", addr, debug_print_IO(addr), showPC()));
	if (addr == HW_IDE)
		return ide.handleReadW(addr);
	else
		return (handleRead(addr) << 8) | handleRead(addr+1);
}

uae_u8 HWget_b (uaecptr addr) {
	D(bug("HWget_b %x <- %s at %08x", addr, debug_print_IO(addr), showPC()));
	uint8 result = handleRead(addr);
	D(bug("Above HWget_b returned %d ($%x)", result, result));
	return result;
}

void HWput_l (uaecptr addr, uae_u32 l) {
	D(bug("HWput_l %x,%d ($%08x) -> %s at %08x", addr, l, l, debug_print_IO(addr), showPC()));
	if (addr == HW_IDE)
		ide.handleWriteL(addr, l);
	else {
		handleWrite(addr, l >> 24);
		handleWrite(addr+1, l >> 16);
		handleWrite(addr+2, l >> 8);
		handleWrite(addr+3, l);
	}
}

void HWput_w (uaecptr addr, uae_u16 w) {
	D(bug("HWput_w %x,%d ($%04x) -> %s at %08x", addr, w, w, debug_print_IO(addr), showPC()));
	if (addr == HW_IDE)
		ide.handleWriteW(addr, w);
	else {
		handleWrite(addr, w >> 8);
		handleWrite(addr+1, w);
	}
}

void HWput_b (uaecptr addr, uae_u8 b) {
	D(bug("HWput_b %x,%u ($%02x) -> %s at %08x", addr, b & 0xff, b & 0xff, debug_print_IO(addr), showPC()));
	handleWrite(addr, b);
}


/*
 * $Log$
 * Revision 1.42  2002/07/24 18:34:18  joy
 * parameter size fixed
 *
 * Revision 1.41  2002/07/21 15:22:57  milan
 * mon readded
 *
 * Revision 1.40  2002/07/18 20:52:43  joy
 * DSP host port simplified if DSP_EMULATION is not defined
 *
 * Revision 1.39  2002/07/08 22:03:49  joy
 * DSP support by Patrice
 *
 * Revision 1.38  2002/06/24 20:38:23  joy
 * do_get_real_addr is memory.h internal and should not be used outside of CPU. fixed.
 *
 * Revision 1.37  2002/03/21 10:26:31  joy
 * serious bug fixed - we forgot to set the faulty address in hardware register accesses!
 *
 * Revision 1.36  2002/02/26 21:08:13  milan
 * address validation in CPU interface
 *
 * Revision 1.35  2002/02/21 17:48:45  joy
 * prepared for 32-bit access to IDE interface
 *
 * Revision 1.34  2001/11/19 17:45:56  joy
 * parallel port emulation
 *
 * Revision 1.33  2001/11/15 13:46:01  joy
 * don't use uint, it's undefined on Solaris
 *
 * Revision 1.32  2001/10/25 19:56:01  standa
 * The Log and Header CVS tags in the Log removed. Was recursing.
 *
 * Revision 1.31  2001/10/08 21:46:05  standa
 * The Header and Log CVS tags added.
 *
 * Revision 1.30  2001/09/21 14:11:48  joy
 * little helper functions removed. Modules call each other via the public functions directly now.
 *
 * Revision 1.29  2001/09/18 12:35:44  joy
 * ARADATA placed at $f90000-$f9ffff
 *
 * Revision 1.28  2001/09/05 15:06:09  joy
 * using D(bug())
 *
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
