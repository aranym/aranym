/* Joy 2001 */

#include "sysdeps.h"
#include "hardware.h"
#include "cpu_emulation.h"
#include "memory.h"
#include "videl.h"

static const int HW = 0xff8200;

static bool dP = false;

VIDEL::VIDEL() {
	syncmode = shifter = videoctrl = videoctrl2 = videomode = 0xff;
	videl = 0xfffe;
}

uae_u8 VIDEL::handleRead(uaecptr addr) {
	addr -= HW;
	if (addr < 0 || addr > 0x400)
		return 0;

	if (dP && (addr < 0x40 || (addr > 0x60 && addr < 0x400)))
		fprintf(stderr, "VIDEL reads %06x at $%06x\n", 0xff8200+addr, showPC());

	switch(addr) {
		case 0x0a: return syncmode;
		case 0x60: return shifter;
		case 0x66: return videl >> 8;
		case 0x67: return videl & 0x00ff;
		case 0xc0: return videoctrl2;
		case 0xc1: return videoctrl;
		case 0xc3: return videomode;
		default: return 0;
	}
}

void VIDEL::handleWrite(uaecptr addr, uae_u8 value) {
	addr -= HW;
	if (addr < 0 || addr > 0x400)
		return;

	if (dP && (addr < 0x40 || (addr > 0x60 && addr < 0x400)))
		fprintf(stderr, "VIDEL writes %06x = %d ($%02x) at $%06x\n", 0xff8200+addr, value, value, showPC());

	switch(addr) {
		case 0x0a: syncmode = value; break;
		case 0x60: shifter = value; break;
		case 0x66: videl = (videl & 0x00ff) | (value << 8); break;
		case 0x67: videl = (videl & 0xff00) | value; break;
		case 0xc0: videoctrl2 = value; break;
		case 0xc1: videoctrl = value; break;
		case 0xc3: videomode = value; break;
		default: break;
	}
}
