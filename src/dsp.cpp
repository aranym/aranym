/* Joy 2001 */

#include "sysdeps.h"
#include "hardware.h"
#include "cpu_emulation.h"
#include "memory.h"
#include "dsp.h"
#include "parameters.h"

DSP::DSP() {
}

static const int HW = 0xffa200;

uae_u8 DSP::handleRead(uaecptr addr) {
	addr -= HW;
	switch(addr) {
		case 2:	// interrupt status register
			return 0xff;
		default:
			return 0xff;
			// return BASE_IO::handleRead(addr);
	}
}
