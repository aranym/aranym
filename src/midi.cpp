/*
 *	MIDI port emulation
 *
 *	Patrice Mandin
 */

#include "sysdeps.h"
#include "hardware.h"
#include "cpu_emulation.h"
#include "memory.h"
#include "midi.h"

#define DEBUG 0
#include "debug.h"

MIDI::MIDI(void) : ACIA(HW_MIDI)
{
	D(bug("midi: interface created at 0x%06x",HW_MIDI));

	reset();
}

MIDI::~MIDI(void)
{
	D(bug("midi: interface destroyed at 0x%06x",baseaddr));
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
}
