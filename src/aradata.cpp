/* Joy 2001 */

#include "sysdeps.h"
#include "hardware.h"
#include "cpu_emulation.h"
#include "memory.h"
#include "aradata.h"
#include "parameters.h"
#include "extfs.h"
extern ExtFs extFS;

ARADATA::ARADATA() {
	mouse_x = 640/2;	// center of screen
	mouse_y = 480/2;	// center of screen
}

static const int HW = 0xf90000;

uae_u8 ARADATA::handleRead(uaecptr addr) {
	addr -= HW;
	switch(addr) {
		case 0: return '_';
		case 1: return 'A';
		case 2: return 'R';
		case 3: return 'A';
		case 4: return VERSION_MAJOR;
		case 5: return VERSION_MINOR;
		case 6: return FastRAMSize >> 24;
		case 7: return FastRAMSize >> 16;
		case 8: return FastRAMSize >> 8;
		case 9: return FastRAMSize;
		case 10: return extFS.getDrvBits() >> 24;
		case 11: return extFS.getDrvBits() >> 16;
		case 12: return extFS.getDrvBits() >> 8;
		case 13: return extFS.getDrvBits();
	}

	return 0;
}

void ARADATA::handleWrite(uaecptr addr, uae_u8 value) {
	addr -= HW;
	switch(addr) {
		case 14: mouse_x = (mouse_x & 0xff) | (value << 8); break;
		case 15: mouse_x = (mouse_x & 0xff00) | value; break;
		case 16: mouse_y = (mouse_y & 0xff) | (value << 8); break;
		case 17: mouse_y = (mouse_y & 0xff00) | value; break;
	}
}
