/* Joy 2001 */

#include "sysdeps.h"
#include "hardware.h"
#include "cpu_emulation.h"
#include "memory.h"
#include "ide.h"

static bool dP = true;

IDE::IDE() {
	data = error = 0;
	status = altstatus = 0x50;
}

static const int HW = 0xf00000;

static uae_u16 DriveInformation[256];

uae_u8 IDE::handleRead(uaecptr addr) {
	addr -= HW;
	if (addr < 0 || addr > 0x39)
		return 0;

	switch(addr) {
		case 0:
		case 1:
		case 2:
		case 3: return (data >> (8*(3-addr)));
		case 5: if (dP)
					fprintf(stderr, "read error register at %x\n", showPC());
				return error;
		case 0x1d: if (dP)
					fprintf(stderr, "read status register at %x\n", showPC());
				return status;
		case 0x39: if (dP)
					fprintf(stderr, "read ALTstatus register at %x\n", showPC());
				return altstatus;
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
		case 3: data = (data & (~(0xff000000 >> (8*addr)))) | (value << (8*(3-addr)));
				if (dP)
					fprintf(stderr, "Written data %08x at %x\n", data, showPC());
				break;
		case 9: scount = value; break;
		case 0xd: snumber = value; break;
		case 0x11: clow = value; break;
		case 0x15: chigh = value; break;
		case 0x19: dhead = value;
					if (dP)
						fprintf(stderr, "Written drive head %d ($%x) at %x\n", value, value, showPC());
					break;
		case 0x1d: cmd = value;
					switch(cmd) {
						case 0xec:	/* Identify Drive */
							/* set BSY, store info in sector buffer, set DRQ and generate interrupt */
							status = 0x08;	// DRQ
							break;

					}
					if (dP)
						fprintf(stderr, "Written command %d ($%x) at %x\n", value, value, showPC());
					break;
		case 0x39: dataout = value;
					if (dP)
						fprintf(stderr, "Written data out %d ($%x) at %x\n", value, value, showPC());
					break;
		default: fprintf(stderr, "Unknown IDE register write: %06x = %d ($%02x)\n", HW+addr, value, value);
	}
}
