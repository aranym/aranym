/* Joy 2001 */

#include "sysdeps.h"
#include "hardware.h"
#include "cpu_emulation.h"
#include "memory.h"
#include "yamaha.h"
#include "parameters.h"

#ifdef DSP_EMULATION
#include "dsp.h"

extern DSP dsp;
#endif

#define DEBUG 1
#include "debug.h"

YAMAHA::YAMAHA() {
	active_reg = 0;
}

static const int HW = 0xff8800;

uae_u8 YAMAHA::handleRead(uaecptr addr) {
	uae_u8 value=0;

	addr -= HW;
	if (addr == 0) {
		switch(active_reg) {
			case 15:
				value=parallel.getData();
				break;
			default:
				value=yamaha_regs[active_reg];
				break;
		}
	}

/*	D(bug("HWget_b(0x%08x)=0x%02x at 0x%08x", addr+HW, value, showPC()));*/
	return value;
}

void YAMAHA::handleWrite(uaecptr addr, uae_u8 value) {
	addr -= HW;
	switch(addr) {
		case 0:
			active_reg = value & 0x0f;
			break;

		case 2:
			yamaha_regs[active_reg] = value;
			switch(active_reg) {
				case 7:
					parallel.setDirection(value >> 7);
					break;
				case 14:
					parallel.setStrobe((value >> 5) & 0x01);
#ifdef DSP_EMULATION
					if (value & (1<<4)) {
						dsp.reset();
					}
#endif
					break;
				case 15:
					parallel.setData(value);
					break;
			}
			break;
	}

/*	D(bug("HWput_b(0x%08x,0x%02x) at 0x%08x", addr+HW, value, showPC()));*/
}

int YAMAHA::getFloppyStat() { return yamaha_regs[14] & 7; }
