/*
	Audio crossbar emulation

	ARAnyM (C) 2005 Patrice Mandin

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

#ifndef CROSSBAR_H
#define CROSSBAR_H

#include "icio.h"

/*--- CROSSBAR class ---*/

class CROSSBAR : public BASE_IO {
	private:
		uint16	input, output;
		uint8	extfreqdiv, intfreqdiv, rec_tracks;
		uint8	in_source, adc_input;
		uint8	gain, atten;
		uint8	gpio_dir, gpio_data;

	public:
		CROSSBAR(memptr, uint32);
		~CROSSBAR();
		void reset();

		virtual uae_u8 handleRead(uaecptr addr);
		virtual void handleWrite(uaecptr addr, uae_u8 value);

		int getIntFreq(void);
		int getIntPrediv(void);
};

#endif /* CROSSBAR_H */
