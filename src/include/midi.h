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

#define HW_MIDI 0xfffc04

class MIDI: public ACIA {
	private:
		int output_to_file;
		FILE *output_handle;

	public:
		MIDI(void);
		~MIDI(void);
		void reset(void);

		uae_u8 ReadStatus();
		void WriteControl(uae_u8 value);
		uae_u8 ReadData();
		void WriteData(uae_u8);
};

#endif /* _MIDI_H */
