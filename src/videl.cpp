/* Joy 2001 */

#include "sysdeps.h"
#include "hardware.h"
#include "cpu_emulation.h"
#include "memory.h"
#include "videl.h"

static const int HW = 0xff8200;

static bool dP = false;

extern uae_u32 vram_addr;

VIDEL::VIDEL() {
	syncmode = videoctrl = videomode = 0xff;
	videl = 0xfffe;
}

uae_u8 VIDEL::handleRead(uaecptr addr) {
	addr -= HW;
	if (addr < 0 || addr > 0x400)
		return 0;

	if (dP && (addr < 0x40 || (addr > 0x60 && addr < 0x400)))
		fprintf(stderr, "VIDEL reads %06x at $%06x\n", 0xff8200+addr, showPC());

	switch(addr) {
		case 0x01: return vram_addr >> 16;
		case 0x03: return vram_addr >> 8;
		case 0x0d: return vram_addr;
		case 0x0a: return syncmode;
		case 0x60: return shifter;
		case 0x66: return videl >> 8;
		case 0x67: return videl & 0x00ff;
		case 0xc0: return videoctrl >> 8;
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
		case 0x01: vram_addr = (vram_addr & 0x0000ffff) | (value << 16); break;
		case 0x03: vram_addr = (vram_addr & 0x00ff00ff) | (value << 8); break;
		case 0x0d: vram_addr = (vram_addr & 0x00ffff00) | value; break;
		case 0x0a: syncmode = value; break;
		case 0x60: shifter = value; break;
		case 0x66: videl = (videl & 0x00ff) | (value << 8); break;
		case 0x67: videl = (videl & 0xff00) | value; break;
		case 0xc0: videoctrl = (videoctrl & 0x00ff) | (value << 8); break;
		case 0xc1: videoctrl = (videoctrl & 0xff00) | value; break;
		case 0xc3: videomode = value; break;
		default: break;
	}
}

// mode 60    66   c0   c2
// 1     0  0400 0082 0008
// STHig 0  0400 0186 0008

// 2     1  0000 0186 0008
// STMid 1  0000 0186 0009 doublescan (protoze 640x200)

// 4     1  0000 0082 0008 864x640 s BlowUP
// 4     0  0000 0186 0008
// STLow 0  0000 0186 0005 doublescan (protoze 320x200)

// 8     1  0010 0186 0008

// 16    1  0100 0182 0004

// c2
// 04: pixel clock = 2
// 05: pixel clock = 2 + double scan
// 08: pixel clock = 1
// 09: pixel clock = 1 + double scan

int VIDEL::getVideoMode() {
	if (videl & 0x0100)
		return 16;
	else if (videl & 0x0010)
		return 8;
	else if (videl & 0x0400)
		return 1;

	if (shifter == 1)
		return 2;
	else
		return 4;
}
