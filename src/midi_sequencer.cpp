/*
	Atari MIDI emulation, output to sequencer device

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

#include <linux/soundcard.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

#include "sysdeps.h"
#include "hardware.h"
#include "cpu_emulation.h"
#include "memory.h"
#include "midi.h"
#include "midi_sequencer.h"
#include "parameters.h"

#define DEBUG 0
#include "debug.h"

MidiSequencer::MidiSequencer(memptr addr, uint32 size) : MIDI(addr, size)
{
	D(bug("midi_sequencer: interface created at 0x%06x", getHWoffset()));

	packet[0] = SEQ_MIDIPUTC;
	packet[1] = 0;	/* data */
	packet[2] = 0;	/* device number */
	packet[3] = 0;	/* NULL */

	fd = open(bx_options.midi.sequencer, O_WRONLY, 0664);
	if (fd<0) {
		fprintf(stderr, "midi_sequencer: Can not open %s\n", bx_options.midi.sequencer); 
	}
}

MidiSequencer::~MidiSequencer(void)
{
	D(bug("midi_sequencer: interface destroyed at 0x%06x", getHWoffset()));

	if (fd>=0) {
		close(fd);
	}
}

void MidiSequencer::WriteData(uae_u8 value)
{
	D(bug("midi_sequencer: WriteData(0x%02x)",value));

	if (fd>=0) {
		packet[1] = value;	
		write(fd, packet, sizeof(packet));
	}
}
