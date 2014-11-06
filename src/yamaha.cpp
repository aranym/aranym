/*
 * yamaha.cpp - Atari YM2149 emulation code
 *
 * Copyright (c) 2001-2004 Petr Stehlik of ARAnyM dev team (see AUTHORS)
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
#include "yamaha.h"
#include "parameters.h"
#include "parallel.h"
#include "parallel_file.h"
#ifdef ENABLE_PARALLELX86
#include "parallel_x86.h"
#endif
#ifdef ENABLE_PARALLELPARPORT
#include "parallel_parport.h"
#endif

#include "dsp.h"

#define DEBUG 0
#include "debug.h"

YAMAHA::YAMAHA(memptr addr, uint32 size) : BASE_IO(addr, size) {
#ifdef ENABLE_PARALLELX86
	if (strcmp("x86", bx_options.parallel.type)==0)
		parallel = new ParallelX86;
	else
#endif
#ifdef ENABLE_PARALLELPARPORT
	if (strcmp("parport", bx_options.parallel.type)==0)
		parallel = new ParallelParport;
	else
#endif
		parallel = new ParallelFile;

	reset();
}

YAMAHA::~YAMAHA()
{
	delete parallel;
}

void YAMAHA::reset()
{
	active_reg = 0;
	for(unsigned int i=0; i<sizeof(yamaha_regs)/sizeof(yamaha_regs[0]); i++)
		yamaha_regs[i] = 0;

	parallel->reset();
}

uint8 YAMAHA::handleRead(memptr addr) {
	uint8 value=0;

	addr -= getHWoffset();
	if (addr == 0) {
		switch(active_reg) {
			case 15:
				value=parallel->getData();
				break;
			default:
				value=yamaha_regs[active_reg];
				break;
		}
	}

/*	D(bug("HWget_b(0x%08x)=0x%02x at 0x%08x", addr+HW, value, showPC()));*/
	return value;
}

void YAMAHA::handleWrite(memptr addr, uint8 value) {
	addr -= getHWoffset();
	switch(addr) {
		case 0:
			active_reg = value & 0x0f;
			break;

		case 2:
			yamaha_regs[active_reg] = value;
			switch(active_reg) {
				case 7:
					parallel->setDirection(value >> 7);
					break;
				case 14:
					parallel->setStrobe((value >> 5) & 0x01);
					if (value & (1<<4)) {
						getDSP()->reset();
					}
					break;
				case 15:
					parallel->setData(value);
					break;
			}
			break;
	}

/*	D(bug("HWput_b(0x%08x,0x%02x) at 0x%08x", addr+HW, value, showPC()));*/
}

int YAMAHA::getFloppyStat() { return yamaha_regs[14] & 7; }
