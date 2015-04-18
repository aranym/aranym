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

#ifndef _MIDI_SEQUENCER_H
#define _MIDI_SEQUENCER_H

#include "midi.h"

class MidiSequencer: public MIDI
{
	private:
		unsigned char packet[4];

	public:
		MidiSequencer(memptr, uint32);
		~MidiSequencer();

		void WriteData(uint8);
		virtual void enable(bool bEnable);
		virtual const char *type() { return "sequencer"; }
};

#endif /* _MIDI_SEQUENCER_H */
