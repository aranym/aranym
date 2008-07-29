/*
	DSP M56001 emulation
	Dummy emulation, Aranym glue

	(C) 2001-2008 ARAnyM developer team

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

#ifndef DSP_H
#define DSP_H

#include "icio.h"

#if DSP_EMULATION
# include "dsp_core.h"
#endif

class DSP : public BASE_IO
{
	public:
		DSP(memptr, uint32);
		~DSP();

		virtual void reset(void);
		virtual uint8 handleRead(memptr addr);
		virtual void handleWrite(memptr, uint8);

#if DSP_EMULATION
	private:
		dsp_core_t dsp_core;
#endif
};

#endif /* DSP_H */
