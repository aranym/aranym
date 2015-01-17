/*
	Audio DMA emulation

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

#ifndef AUDIODMA_H
#define AUDIODMA_H

#include "icio.h"

/*--- AUDIODMA class ---*/

class AUDIODMA : public BASE_IO {
	private:
		int freqs[4];
		uint32	current;
		uint16	mode;

		int freq;

		void updateCurrent();
		void updateControl();

	public:
		AUDIODMA(memptr, uint32);
		~AUDIODMA();
		void reset();

		virtual uae_u8 handleRead(uaecptr addr);
		virtual void handleWrite(uaecptr addr, uae_u8 value);

		void updateMode(void);

		/* Only for audio callback */
		uint16	control;
		uint32	start, end;
		int	start_tic;
};

#endif /* AUDIODMA_H */
