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

#ifndef _MIDI_H
#define _MIDI_H

#include "acia.h"

class MIDI: public ACIA
{
	protected:
		int fd;
		virtual void close(void);

	public:
		MIDI(memptr, uint32);
		virtual ~MIDI();
		virtual void reset(void);
		virtual void enable(bool bEnable) = 0;
		virtual const char *type() = 0;
};

#endif /* _MIDI_H */
