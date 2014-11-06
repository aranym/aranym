/*
	DSP M56001 emulation
	Dummy emulation, Aranym glue

	(C) 2001-2008 ARAnyM developer team

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "sysdeps.h"
#include "hardware.h"
#include "cpu_emulation.h"
#include "memory-uae.h"
#include "dsp.h"
#if DSP_EMULATION
# include "dsp_cpu.h"
#endif

#define DEBUG 0
#include "debug.h"

DSP::DSP(memptr address, uint32 size) : BASE_IO(address, size)
{
#if DSP_EMULATION
	dsp_core_init(&dsp_core, 1 /* use_thread */);
#endif
}

DSP::~DSP(void)
{
#if DSP_EMULATION
	dsp_core_shutdown(&dsp_core);
#endif
}

/* Other functions to init/shutdown dsp emulation */
void DSP::reset(void)
{
#if DSP_EMULATION
	dsp_core_reset(&dsp_core);
#endif
}

/* Exec some DSP instructions */
void DSP::exec_insts(int num_inst)
{
#if DSP_EMULATION
	while (dsp_core.running && (num_inst-->0)) {
		dsp56k_execute_instruction();
	}
#endif
}

/**********************************
 *	Hardware address read/write by CPU
 **********************************/

uint8 DSP::handleRead(memptr addr)
{
	uint8 value;
	static uint8 prev_value = 0;
	static memptr prev_addr=0;
#if DSP_EMULATION
	value = dsp_core_read_host(&dsp_core, addr-getHWoffset());
#else
	/* this value prevents TOS from hanging in the DSP init code */
	value = 0xff;
#endif

	if ((value!=prev_value) || (addr!=prev_addr)) {
		D(bug("HWget_b(0x%08x)=0x%02x at 0x%08x", addr, value, showPC()));
		prev_value = value;
		prev_addr = addr;
	}
	return value;
}

void DSP::handleWrite(memptr addr, uint8 value)
{
	D(bug("HWput_b(0x%08x,0x%02x) at 0x%08x", addr, value, showPC()));
#if DSP_EMULATION
	dsp_core_write_host(&dsp_core, addr-getHWoffset(), value);
#endif
}

/*
vim:ts=4:sw=4:
*/
