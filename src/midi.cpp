/*
 * midi.cpp - Atari MIDI emulation code
 *
 * Copyright (c) 2001-2004 Patrice Mandin of ARAnyM dev team (see AUTHORS)
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sysdeps.h"
#include "hardware.h"
#include "cpu_emulation.h"
#include "memory.h"
#include "midi.h"
#include "parameters.h"

#define DEBUG 0
#include "debug.h"

MIDI::MIDI(memptr addr, uint32 size) : ACIA(addr, size)
{
	output_to_file=0;

	D(bug("midi: interface created at 0x%06x", getHWoffset()));

	reset();
}

MIDI::~MIDI(void)
{
	D(bug("midi: interface destroyed at 0x%06x", getHWoffset()));

	if (output_to_file) {
		fclose(output_handle);
	}
}

void MIDI::reset(void)
{
	D(bug("midi: reset"));
}

uae_u8 MIDI::ReadStatus()
{
	D(bug("midi: ReadStatus()=0x%02x",sr));
	return sr;
}

void MIDI::WriteControl(uae_u8 value)
{
	cr = value;
	D(bug("midi: WriteControl(0x%02x)",cr));

#if DEBUG
	PrintControlRegister("midi",cr);
#endif
}

uae_u8 MIDI::ReadData()
{
	D(bug("midi: ReadData()=0x%02x",rxdr));
	return rxdr;
}

void MIDI::WriteData(uae_u8 value)
{
	txdr = value;
	D(bug("midi: WriteData(0x%02x)",txdr));

	if (!output_to_file) {
		if (bx_options.midi.enabled) {
			output_handle = fopen(bx_options.midi.output,"w");
			if (output_handle!=NULL) {
				output_to_file=1;
			}
		}
	}

	if (output_to_file) {
		fprintf(output_handle,
			(((value>=32) && (value<=127)) || (value==13) || (value==10)) ? "%c" : "<0x%02x>",
			value);
		fflush(output_handle);
	}
}
