/*
 *	MIDI port emulation
 *
 *	Joy 2001
 *	Patrice Mandin
 */

#ifndef _MIDI_H
#define _MIDI_H

#include <stdio.h>

#include "acia.h"

class MIDI: public ACIA {
	private:
		int output_to_file;
		FILE *output_handle;

	public:
		MIDI(memptr, uint32);
		~MIDI();
		void reset(void);

		uae_u8 ReadStatus();
		void WriteControl(uae_u8 value);
		uae_u8 ReadData();
		void WriteData(uae_u8);
};

#endif /* _MIDI_H */
