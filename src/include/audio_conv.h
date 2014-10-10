/*
	Audio format conversions

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

#ifndef AUDIOCONV_H
#define AUDIOCONV_H

#include "SDL_compat.h"

class AudioConv {
	private:
		SDL_AudioCVT	cvt;
		Uint8 *tmpBuf;
		int srcRate, srcChan, srcOffset, srcSkip;
		int dstRate, tmpBufLen;
		int volume;

		int rescaleFreq8(Uint8 *source, int *src_len, Uint8 *dest, int dst_len);
		int rescaleFreq16(Uint16 *source, int *src_len, Uint16 *dest, int dst_len);

	public:
		AudioConv(void);
		~AudioConv();

		void setConversion(Uint16 src_fmt, Uint8 src_chan, int src_rate,
			int src_offset, int src_skip,
			Uint16 dst_fmt, Uint8 dst_chan, int dst_rate);
		void doConversion(Uint8 *source, int *src_len, Uint8 *dest, int *dst_len);
		void setVolume(int newVolume);
};

#endif /* AUDIOCONV_H */
