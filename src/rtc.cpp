/* Joy 2001 */

#include "sysdeps.h"
#include "hardware.h"
#include "cpu_emulation.h"
#include "memory.h"
#include "rtc.h"

static const int HW = 0xff8960;

uae_u8 cmos[64]={48,255,21,255,23,255,1,25,3,33,42,14,112,128,0,
        0,0,0,0,0,0,1,17,46,32,1,255,0,0,56,135,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,224,31};

RTC::RTC() {
	addr = data = 0;
}

void RTC::setAddr(uae_u8 value) {
	if (addr < 64)
		addr = value;
}

uae_u8 RTC::getData() {
	return cmos[addr];
}

void RTC::setData(uae_u8 value) {
}
