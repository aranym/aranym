/* Joy 2001 */

#include "sysdeps.h"
#include "hardware.h"
#include "cpu_emulation.h"
#include "memory.h"
#include "videl.h"

static const int HW = 0xff8200;
#if 0
/*
static bool dP = true;

extern uae_u32 vram_addr;

VIDEL::VIDEL() {
	videoctrl = videomode = 0xff;
	syncmode = 2;	// PAL mode, internal sync
	videl = 0xfffe;
}

#define LO(a) ((a) & 0x00ff)
#define HI(a) ((a) >> 8)

#define STORELO(a, b) (((a) & 0xff00) | (b))
#define STOREHI(a, b) (((a) & 0x00ff) | ((b) << 8))

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
		case 0x10: return HI(linewide);
		case 0x11: return LO(linewide);
		case 0x60: return shifter;
		case 0x66: return HI(videl);
		case 0x67: return LO(videl);
		case 0x88: return HI(hdb);
		case 0x89: return LO(hdb);
		case 0x8a: return HI(hde);
		case 0x8b: return LO(hde);
		case 0xa8: return HI(vdb);
		case 0xa9: return LO(vdb);
		case 0xaa: return HI(vde);
		case 0xab: return LO(vde);
		case 0xc0: return HI(videoctrl);
		case 0xc1: return LO(videoctrl);	// monitor == TV ? videoctrl | 0x03
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
		case 0x10: linewide = (value << 8) | (linewide & 0x00ff); break;
		case 0x11: linewide = (linewide & 0xff00) | value; break;
		case 0x88: hdb = STOREHI(hdb, value); break;
		case 0x89: hdb = STORELO(hdb, value); fprintf(stderr, "hdb = %d\n", hdb); break;
		case 0x8a: hde = STOREHI(hde, value); break;
		case 0x8b: hde = STORELO(hde, value); fprintf(stderr, "hde = %d\n", hde); break;
		case 0xa8: vdb = STOREHI(vdb, value); break;
		case 0xa9: vdb = STORELO(vdb, value); fprintf(stderr, "vdb = %d\n", vdb); break;
		case 0xaa: vde = STOREHI(vde, value); break;
		case 0xab: vde = STORELO(vde, value); fprintf(stderr, "vde = %d\n", vde); break;
		case 0x60: shifter = value; break;
		case 0x66: videl = (videl & 0x00ff) | (value << 8); break;
		case 0x67: videl = (videl & 0xff00) | value; break;
		case 0xc0: videoctrl = (videoctrl & 0x00ff) | (value << 8); break;
		case 0xc1: videoctrl = (videoctrl & 0xff00) | value; break;
		case 0xc3: videomode = value; break;
		default: break;
	}
}
*/
#endif

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

uaecptr VIDEL::getVideoramAddress() {
	return (handleRead(HW+1) << 16) | (handleRead(HW+3) << 8) | handleRead(HW+0x0d);
}

int VIDEL::getVideoMode() {
	int videl = handleReadW(HW+0x66);
	int shifter = handleRead(HW+0x60);
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

int VIDEL::getScreenWidth() {
	return handleReadW(HW+0x10) * 16 / getVideoMode();
}

int VIDEL::getScreenHeight() {
	int vdb = handleReadW(HW+0xa8);
	int vde = handleReadW(HW+0xaa);
	if (vdb != 0 && vde - vdb > 0)
		return (vde - vdb) >> 1;
	else
		return 480;
}
