/* Joy 2001 */

#include "sysdeps.h"
#include "hardware.h"
#include "cpu_emulation.h"
#include "memory.h"
#include "ide.h"

IDE::IDE() {
	data = error = status = altstatus = 0;
}

static const int HW = 0xf00000;

uae_u8 IDE::handleRead(uaecptr addr) {
	addr -= HW;
	if (addr < 0 || addr > 0x39)
		return 0;

	switch(addr) {
		case 0:
		case 1:
		case 2:
		case 3: return (data >> (8*(3-addr)));
		case 5: return error;
		case 0x1d: return status;
		case 0x39:	return altstatus;
		default: fprintf(stderr, "Unknown IDE register read: %06x\n", HW+addr);
	}

	return 0;
}

void IDE::handleWrite(uaecptr addr, uae_u8 value) {
	addr -= HW;
	if (addr < 0 || addr > 0x39)
		return;

	switch(addr) {
		case 0:
		case 1:
		case 2:
		case 3: data = (data & (~(0xff000000 >> (8*addr)))) | (value << (8*(3-addr))); break;
		case 9: scount = value; break;
		case 0xd: snumber = value; break;
		case 0x11: clow = value; break;
		case 0x15: chigh = value; break;
		case 0x19: dhead = value; break;
		case 0x1d: cmd = value; break;
		case 0x39: dataout = value; break;
		default: fprintf(stderr, "Unknown IDE register write: %06x = %d ($%02x)\n", HW+addr, value, value);
	}
}
