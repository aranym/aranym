/* Joy 2001 */

#include "sysdeps.h"
#include "hardware.h"
#include "cpu_emulation.h"
#include "memory.h"
#include "dsp.h"

static const int HW = 0xffa200;

uae_u8 DSP::handleRead(uaecptr addr) {
	return 0xff;	// this is to prevent the TOS to hang in DSP initialization routine
}
