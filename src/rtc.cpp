/* Joy 2001 */

#include "sysdeps.h"
#include "hardware.h"
#include "cpu_emulation.h"
#include "memory.h"
#include "rtc.h"
#include "parameters.h"

uae_u8 cmos[64]={48,255,21,255,23,255,1,25,3,33,42,14,112,128,
		0,0,0,0,0,0,0,1,17,46,32,1,255,0,0,56,135,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,224,31};

#if 0
int byte15th = (colors & 7) | (80:40) << 3 | (VGA : TV) << 4 | (PAL : NTSC) << 5| overscan << 6 | STcompatible << 7);
int byte14th = VGA:TV ? line doubling : half screen;
#endif

RTC::RTC() {
	addr = 0;
	cmos[24] = 32;	// boot delay
	switch(boot_color_depth) {
		case 2: cmos[29] = 57; break;
		case 4: cmos[29] = 58; break;
		case 8: cmos[29] = 59; break;
		case 16: cmos[29] = 60; break;
		default: cmos[29] = 56; break;
	}
//	cmos[29] = 56;	// 2 colors with 80 columns on PAL VGA
//	cmos[29] = 59;	// 256 colors with 80 columns on PAL VGA
//	cmos[29] = 60;	// TC colors on PAL VGA
//	cmos[29] = 44;	// TC colors on PAL TV
	setChecksum();	// in case somebody edited the cmos array manually
}

static const int HW = 0xff8960;

uae_u8 RTC::handleRead(uaecptr addr) {
	addr -= HW;
	if (addr < 0 || addr > 3)
		return 0;

	switch(addr) {
		case 1: return addr;
		case 3: return getData();
	}

	return 0;
}

void RTC::handleWrite(uaecptr addr, uae_u8 value) {
	addr -= HW;
	if (addr < 0 || addr > 3)
		return;

	switch(addr) {
		case 1: setAddr(value); break;
		case 3: setData(value); break;
	}
}

void RTC::setAddr(uae_u8 value) {
	if (addr < 64)
		addr = value;
}

uae_u8 RTC::getData() {
//	fprintf(stderr, "Reading NVRAM data at %d = %d ($%02x) at %06x\n", addr, cmos[addr], cmos[addr], showPC());
	return cmos[addr];
}

void RTC::setData(uae_u8 value) {
//	fprintf(stderr, "Writting NVRAM data at %d = %d ($%02x) at %06x\n", addr, value, value, showPC());
	cmos[addr] = value;
}

/* the checksum is over all bytes except the checksum bytes
 * themselves; these are at the very end */
#define CKS_RANGE_START	14
#define CKS_RANGE_END	(14+47)
#define CKS_LOC			(14+48)
void RTC::setChecksum()
{
	int i;
	unsigned char sum = 0;
	
	for(i = CKS_RANGE_START; i <= CKS_RANGE_END; ++i)
		sum += cmos[i];
	cmos[CKS_LOC] = ~sum;
	cmos[CKS_LOC+1] = sum;
}
