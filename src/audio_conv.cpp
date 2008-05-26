/*
	Audio format conversions

	ARAnyM (C) 2008 Patrice Mandin

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

#include "audio_conv.h"

#define DEBUG 0
#include "debug.h"

AudioConv::AudioConv(void)
	: tmpBuf(NULL), srcRate(0), srcChan(0), srcOffset(0), srcSkip(0), dstRate(0), tmpBufLen(0)
{
}

AudioConv::~AudioConv()
{
	if (!tmpBuf) {
		free(tmpBuf);
		tmpBuf=NULL;
	}
}

/*
	SDL 1.2 won't properly convert between different rates
	so we use it only to convert formats
*/
void AudioConv::setConversion(Uint16 src_fmt, Uint8 src_chan, int src_rate, int src_offset, int src_skip,
	Uint16 dst_fmt, Uint8 dst_chan, int dst_rate)
{
	SDL_BuildAudioCVT(&cvt,
		src_fmt, src_chan, dst_rate,
		dst_fmt, dst_chan, dst_rate);

	srcRate = src_rate;
	srcOffset = src_offset;
	srcSkip = src_skip;
	srcChan = src_chan;
	dstRate = dst_rate;

	D(bug("audio_conv: 0x%04x, %d chans, %d Hz to 0x%04x, %d chans, %d Hz",
		src_fmt, src_chan, src_rate,
		dst_fmt, dst_chan, dst_rate));
	D(bug("audio_conv: offset %d bytes, skip %d bytes in source", src_offset, src_skip));
}

int AudioConv::rescaleFreq8(Uint8 *source, int *src_len, Uint8 *dest, int dst_len)
{
	int srcSamplesRead=0, srcBytesRead=0;
	int dstSamplesWritten=0, dstBytesWritten=0;

	while ((srcBytesRead<*src_len) && (dstBytesWritten<dst_len)) {
		switch(srcChan) {
			case 2:
				dest[dstBytesWritten++] = source[srcBytesRead++];
				/* break; */
			case 1:
				dest[dstBytesWritten++] = source[srcBytesRead];
				break;
		}

		srcSamplesRead = (dstSamplesWritten++ * srcRate) / dstRate;
		srcBytesRead = srcSamplesRead*srcSkip+srcOffset;
	}

	*src_len = srcBytesRead;
	return dstBytesWritten;
}

int AudioConv::rescaleFreq16(Uint16 *source, int *src_len, Uint16 *dest, int dst_len)
{
	int srcSamplesRead=0, srcWordsRead=0;
	int dstSamplesWritten=0, dstWordsWritten=0;
	int curSkip = srcSkip>>1;
	int curOffset = srcOffset>>1;
	int srcLen = (*src_len)>>1;

	dst_len >>= 1;
	while ((srcWordsRead<srcLen) && (dstWordsWritten<dst_len)) {
		switch(srcChan) {
			case 2:
				dest[dstWordsWritten++] = source[srcWordsRead++];
				/* break; */
			case 1:
				dest[dstWordsWritten++] = source[srcWordsRead];
				break;
		}

		srcSamplesRead = (dstSamplesWritten++ * srcRate) / dstRate;
		srcWordsRead = srcSamplesRead*curSkip+curOffset;
	}

	*src_len = srcWordsRead<<1;
	return dstWordsWritten<<1;
}

/*
	Convert a block to host audio format
	Set source and dest lengths to length really read and written
*/
void AudioConv::doConversion(Uint8 *source, int *src_len, Uint8 *dest, int *dst_len)
{
	if ((srcRate==0) || (dstRate==0)) {
		return;
	}

	D(bug("audioconv: 0x%08x, %d -> 0x%08x, %d", source, *src_len, dest, *dst_len));

	/* Calc needed buffer size */
	if (tmpBufLen< *dst_len) {
		tmpBuf = (Uint8 *) realloc(tmpBuf, *dst_len);
		tmpBufLen = *dst_len;
		D(bug("audioconv: realloc tmpbuf, len: %d", *dst_len));
	}

	/* First convert according to freq rates in a temp buffer */
	int dstConvertedLen = 0, neededBufSize = (int) (*dst_len / cvt.len_ratio);
	if (neededBufSize > tmpBufLen) {
		neededBufSize = tmpBufLen;
	}

	switch(cvt.src_format & 0xff) {
		case 8:
			dstConvertedLen = rescaleFreq8(source, src_len, tmpBuf, neededBufSize);
			break;
		case 16:
			dstConvertedLen = rescaleFreq16((Uint16 *) source, src_len, (Uint16 *) tmpBuf, neededBufSize);
			break;
	}

	/* Then convert to final format */
	cvt.buf = tmpBuf;
	cvt.len = (int) (dstConvertedLen / cvt.len_ratio);
	SDL_ConvertAudio(&cvt);

	SDL_MixAudio(dest, cvt.buf, cvt.len_cvt, SDL_MIX_MAXVOLUME);

	/* Set converted length */
	*dst_len = cvt.len_cvt;
}
