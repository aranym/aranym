/*
	Atari MIDI emulation, output to file

	ARAnyM (C) 2005-2009 Patrice Mandin

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
#include "midi_file.h"
#include "parameters.h"

#define DEBUG 0
#include "debug.h"

MidiFile::MidiFile(memptr addr, uint32 size) : MIDI(addr, size)
{
	D(bug("midi_file: interface created at 0x%06x", getHWoffset()));
	fd = -1;
	if (bx_options.midi.enabled)
		enable(true);
}


void MidiFile::enable(bool bEnable)
{
	bx_midi_options_t *m = &bx_options.midi;
	if (!bEnable)
		close();
	if (strcmp(type(), m->type)==0 && strlen(m->file) > 0) {
		if (bEnable && fd < 0)
		{
			fd = ::open(m->file, O_WRONLY|O_CREAT, 0664);
			if (fd < 0) {
				panicbug("midi_file: Can not open %s", m->file);
			}
		}
	}
	m->enabled = bEnable;
}

MidiFile::~MidiFile(void)
{
	D(bug("midi_file: interface destroyed at 0x%06x", getHWoffset()));

	close();
}

void MidiFile::WriteData(uae_u8 value)
{
	D(bug("midi_file: WriteData(0x%02x)",value));

	if (fd>=0) {
		if (write(fd, &value, 1) != 1) {
			panicbug("midi_file: error writing");
		}
	}
}

// don't remove this modeline with intended formatting for vim:ts=4:sw=4:
