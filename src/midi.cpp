/*
	Atari MIDI emulation

	ARAnyM (C) 2005-2006 Patrice Mandin

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
#include "midi.h"
#include "parameters.h"

#define DEBUG 0
#include "debug.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

MIDI::MIDI(memptr addr, uint32 size) : ACIA(addr, size)
{
	D(bug("midi: interface created at 0x%06x", getHWoffset()));

	reset();
}

MIDI::~MIDI(void)
{
	D(bug("midi: interface destroyed at 0x%06x", getHWoffset()));
}

void MIDI::reset(void)
{
	D(bug("midi: reset"));

	ACIA::reset();
}


void MIDI::close(void)
{
	if (fd >= 0)
	{
		int i, j;

		for (j=0;j<128;j++) {
			for (i=0;i<16;i++) {
				WriteData(0x80 + i);
				WriteData(j);
				WriteData(0);
			}
		}
	
		::close(fd);
		fd = -1;
	}
}
