/*
 * rtc.cpp - Atari NVRAM emulation code
 *
 * Copyright (c) 2001-2006 Petr Stehlik of ARAnyM dev team (see AUTHORS)
 * 
 * This file is part of the ARAnyM project which builds a new and powerful
 * TOS/FreeMiNT compatible virtual machine running on almost any hardware.
 *
 * ARAnyM is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * ARAnyM is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ARAnyM; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "sysdeps.h"
#include "hardware.h"
#include "cpu_emulation.h"
#include "memory-uae.h"
#include "rtc.h"
#include "parameters.h"

#define DEBUG 0
#include "debug.h"

#define CKS_RANGE_START	14
#define CKS_RANGE_END	(14+47)
#define CKS_RANGE_LEN	(CKS_RANGE_END-CKS_RANGE_START+1)
#define CKS_LOC			(14+48)

uint8 nvram[64]={48,255,21,255,23,255,1,25,3,33,42,14,112,128,
		0,0,0,0,0,0,0,0,17,46,32,1,255,0,0,56,135,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,224,31};

#define NVRAM_SYSTEM_LANGUAGE	20
#define NVRAM_KEYBOARD_LAYOUT	21

/*
int byte15th = (colors & 7) | (80:40) << 3 | (VGA : TV) << 4 | (PAL : NTSC) << 5| overscan << 6 | STcompatible << 7);
int byte14th = VGA:TV ? line doubling : half screen;
*/

RTC::RTC(memptr addr, uint32 size) : BASE_IO(addr, size)
{
	init();
	reset();
}

void RTC::reset()
{
	patch();
	index = 0;
}

RTC::~RTC()
{
	save();		// save NVRAM file upon exit automatically (should be conditionalized)
}

void RTC::init()
{
	// set up the nvram filename
	getConfFilename(ARANYMNVRAM, nvram_filename, sizeof(nvram_filename));

	load();		// load NVRAM file automatically

}

void RTC::patch()
{
	if (bx_options.video.monitor != -1) {
		if (bx_options.video.monitor == 0)	// VGA
			nvram[29] |= 0x10;		// the monitor type should be set on a working copy only
		else
			nvram[29] &= ~0x10;
	}
	if (bx_options.video.boot_color_depth != -1) {
		int res = nvram[29] & 0x07;
		switch(bx_options.video.boot_color_depth) {
			case 1: res = 0; break;
			case 2: res = 1; break;
			case 4: res = 2; break;
			case 8: res = 3; break;
			case 16: res = 4; break;
		}
		nvram[29] &= ~0x07;
		nvram[29] |= res;		// the booting resolution should be set on a working copy only
	}

	if (bx_options.tos.cookie_akp != -1) {
		nvram[NVRAM_SYSTEM_LANGUAGE] = bx_options.tos.cookie_akp & 0xff;
		nvram[NVRAM_KEYBOARD_LAYOUT] = (bx_options.tos.cookie_akp >> 8) & 0xff;
	}

	setChecksum();
}

bool RTC::load()
{
	bool ret = false;
	FILE *f = fopen(nvram_filename, "rb");
	if (f != NULL) {
		uint8 fnvram[CKS_RANGE_LEN];
		if (fread(fnvram, 1, CKS_RANGE_LEN, f) == CKS_RANGE_LEN) {
			memcpy(nvram+CKS_RANGE_START, fnvram, CKS_RANGE_LEN);
			ret = true;
		}
		fclose(f);
		if (false /*verbose*/) infoprint("NVRAM loaded from '%s'", nvram_filename);
	}
	else {
		panicbug("NVRAM not found at '%s'", nvram_filename);
	}

	return ret;
}

bool RTC::save()
{
	bool ret = false;
	FILE *f = fopen(nvram_filename, "wb");
	if (f != NULL) {
		if (fwrite(nvram+CKS_RANGE_START, 1, CKS_RANGE_LEN, f) == CKS_RANGE_LEN) {
			ret = true;
		}
		fclose(f);
	}
	else {
		panicbug("ERROR: cannot store NVRAM to '%s'", nvram_filename);
	}

	return ret;
}

uint8 RTC::handleRead(memptr addr)
{
	addr -= getHWoffset();
	if (addr > getHWsize())
		return 0;

	switch(addr) {
		case 1: return index;
		case 3: return getData();
	}

	return 0;
}

void RTC::handleWrite(memptr addr, uint8 value)
{
	addr -= getHWoffset();
	if (addr > getHWsize())
		return;

	switch(addr) {
		case 1: setAddr(value); break;
		case 3: setData(value); break;
	}
}

void RTC::setAddr(uint8 value)
{
	if (value < sizeof(nvram)) {
		index = value;
	}
	else {
		D(bug("NVRAM: trying to set out-of-bound position (%d)", value));
	}
}

void RTC::freezeTime(void)
{
	time_t tim = time(NULL);
	frozen_time = *(bx_options.gmtime ? gmtime(&tim) : localtime(&tim));
}

struct tm RTC::getFrozenTime(void)
{
	if (!(nvram[11] & 0x80))
		freezeTime();
	return frozen_time;
}

uint8 RTC::getData()
{
	uint8 value;

#define BIN_TO_BCD() \
	if (!(nvram[11] & 0x04)) \
		value = ((value / 10) << 4) | (value % 10)
	
	switch(index) {
	case 0:
		value = getFrozenTime().tm_sec;
		BIN_TO_BCD();
		break;
	case 2:
		value = getFrozenTime().tm_min;
		BIN_TO_BCD();
		break;
	case 4:
		value = getFrozenTime().tm_hour;
		if (!(nvram[11] & 0x02))
		{
			uint8 pmflag = (value == 0 || value >= 13) ? 0x80 : 0;
			value = value % 12;
			if (value == 0)
				value = 12;
			BIN_TO_BCD();
			value |= pmflag;
		} else
		{
			BIN_TO_BCD();
		}
		break;
	case 6:
		value = getFrozenTime().tm_wday + 1;
		BIN_TO_BCD();
		break;
	case 7:
		value = getFrozenTime().tm_mday;
		BIN_TO_BCD();
		break;
	case 8:
		value = getFrozenTime().tm_mon+1;
		BIN_TO_BCD();
		break;
	case 9:
		value = getFrozenTime().tm_year - 68;
		BIN_TO_BCD();
		break;
	case 1: /* alarm seconds */
	case 3:	/* alarm minutes */
	case 5: /* alarm hour */
		value = nvram[index];
		BIN_TO_BCD();
		break;
	case 10:
		nvram[index] ^= 0x80; // toggle UIP bit
		value = nvram[index];
		break;
	default:
		value = nvram[index];
		break;
	}
	D(bug("Reading NVRAM data at %d = %d ($%02x) at %06x", index, value, value, showPC()));
	return value;
}

void RTC::setData(uint8 value)
{
	D(bug("Writing NVRAM data at %d = %d ($%02x) at %06x", index, value, value, showPC()));
	switch (index)
	{
	case 11:
		if (value & 0x80)
			freezeTime();
		break;
	}
	nvram[index] = value;
}

nvram_t RTC::getNvramKeyboard()
{
	/* Return keyboard language setting */
	return (nvram_t) nvram[NVRAM_KEYBOARD_LAYOUT];
}

/* the checksum is over all bytes except the checksum bytes
 * themselves; these are at the very end */
void RTC::setChecksum()
{
	int i;
	unsigned char sum = 0;
	
	for(i = CKS_RANGE_START; i <= CKS_RANGE_END; ++i)
		sum += nvram[i];
	nvram[CKS_LOC] = ~sum;
	nvram[CKS_LOC+1] = sum;
}

/*
vim:ts=4:sw=4:
*/
