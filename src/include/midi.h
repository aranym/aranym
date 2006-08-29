/*
 * midi.h - Atari MIDI emulation code - declaration
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

#ifndef _MIDI_H
#define _MIDI_H

#include <stdio.h>

#include "acia.h"

class MIDI: public ACIA {
	private:
		int output_to_file;
		int bufpos;
		FILE *output_handle;

	public:
		MIDI(memptr, uint32);
		~MIDI();
		void reset(void);

		uint8 ReadStatus();
		void WriteControl(uint8 value);
		uint8 ReadData();
		void WriteData(uint8);
};

#endif /* _MIDI_H */
