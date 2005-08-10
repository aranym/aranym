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
 
#include <SDL.h>

#include "sysdeps.h"
#include "hardware.h"
#include "cpu_emulation.h"
#include "memory.h"
#include "audio.h"
#include "audio_dma.h"

#define DEBUG 0
#include "debug.h"

extern	AUDIO	*audio;

/*--- Defines ---*/

#define CTRL_TIMERA_RECORD_END		(1<<11)
#define CTRL_TIMERA_PLAYBACK_END	(1<<10)
#define CTRL_MFPI7_RECORD_END		(1<<9)
#define CTRL_MFPI7_PLAYBACK_END		(1<<8)
#define CTRL_RECORD_SELECT			(1<<7)
#define CTRL_PLAYBACK_SELECT		(0<<7)
#define CTRL_RECORD_REPEAT			(1<<5)
#define CTRL_RECORD_ENABLE			(1<<4)
#define CTRL_PLAYBACK_REPEAT		(1<<1)
#define CTRL_PLAYBACK_ENABLE		(1<<0)

#define MODE_MONITOR_TRACK			12
#define MODE_MONITOR_TRACK_MASK		3
#define MODE_MONITOR_TRACK0			(0<<MODE_MONITOR_TRACK)
#define MODE_MONITOR_TRACK1			(1<<MODE_MONITOR_TRACK)
#define MODE_MONITOR_TRACK2			(2<<MODE_MONITOR_TRACK)
#define MODE_MONITOR_TRACK3			(3<<MODE_MONITOR_TRACK)

#define MODE_PLAY_TRACK				8
#define MODE_PLAY_TRACK_MASK		3
#define MODE_PLAY_1TRACKS			(0<<MODE_PLAY_TRACK)
#define MODE_PLAY_2TRACKS			(1<<MODE_PLAY_TRACK)
#define MODE_PLAY_3TRACKS			(2<<MODE_PLAY_TRACK)
#define MODE_PLAY_4TRACKS			(3<<MODE_PLAY_TRACK)

#define MODE_FORMAT					6
#define MODE_FORMAT_MASK			3
#define MODE_FORMAT_8STEREO			(0<<MODE_FORMAT)
#define MODE_FORMAT_16STEREO		(1<<MODE_FORMAT)
#define MODE_FORMAT_8MONO			(2<<MODE_FORMAT)

#define MODE_FREQ					0
#define MODE_FREQ_MASK				3
#define MODE_FREQ0					(0<<MODE_FREQ)
#define MODE_FREQ1					(1<<MODE_FREQ)
#define MODE_FREQ2					(2<<MODE_FREQ)
#define MODE_FREQ3					(3<<MODE_FREQ)

/*--- DMA callback ---*/

extern "C" {
	static SDL_audiostatus playing;
	static SDL_AudioCVT	cvt;

	static Uint16	format;
	static int		channels, freq, offset, skip;
	static uint32	start_replay, current_replay, end_replay;

	static void	*tmp_buf;
	static int	tmp_buf_len;

	static int resize_8bit(Sint8 *dest, int dst_len)
	{
		uint32	src;
		int j,k;

//		D(bug("audiodma: resize 8bit"));		
		src = current_replay + offset*channels;
		j = k = 0;
		while (((current_replay+j*skip)<end_replay) && (dst_len>0)) {
			j = (k * freq) / audio->obtained.freq;
			for (int i=0;i<channels;i++) {
				*dest++ = ReadInt8(src + i + j*skip);
				dst_len--;
			}
			k++;
		}
		current_replay += j*skip;
		return k*channels;
	}
	
	static int resize_16bit(Sint16 *dest, int dst_len)
	{
		uint32	src;
		int j,k;
		
//		D(bug("audiodma: resize 16bit"));		
		src = current_replay + offset*channels;
		j = k = 0;
		while (((current_replay+j*skip)<end_replay) && (dst_len>0)) {
			j = (k * freq) / audio->obtained.freq;
			for (int i=0;i<channels;i++) {
				*dest++ = ReadInt16(src + (i<<1) + j*skip);
				dst_len -= 2;
			}
			k++;
		}
		current_replay += j*skip;
		return (k*channels)<<1;
	}
	
	static void audio_callback(void *userdata, uint8 * stream, int len)
	{
		Uint8 *dest;
		int dest_len, trigger_interrupt;

		if ((playing!=SDL_AUDIO_PLAYING) || (format==0) || (channels==0)
			|| (freq==0) || (start_replay==0) || (end_replay==0))
			return;

//		D(bug("audiodma: %d to fill, from %d Hz to %d Hz", len, freq, audio->obtained.freq));

		/* Allocate needed temp buffer */
		if (tmp_buf_len<len) {
			if (tmp_buf) {
				free(tmp_buf);
			}
			tmp_buf_len = len*cvt.len_mult;
			tmp_buf = malloc(tmp_buf_len);
//			D(bug("audiodma: %d allocated for temp buffer", tmp_buf_len));
		}

		trigger_interrupt = 0;
		dest = (Uint8 *)tmp_buf;
		dest_len = (int) (len / cvt.len_ratio);
		while (dest_len>0) {
			int converted_len;

//			D(bug("audiodma: replay from 0x%08x to 0x%08x via 0x%08x", start_replay, end_replay, current_replay));
//			D(bug("audiodma: buffer 0x%08x, len %d", dest, dest_len));

			/* Resize Atari buffer using offset, skip and freq */
			switch(format & 0xff) {
				case 8:
					converted_len = resize_8bit((Sint8 *)dest, dest_len);
					break;
				case 16:
					converted_len = resize_16bit((Sint16 *)dest, dest_len);
					break;
				default:
					return;
			}

//			D(bug("audiodma: %d converted from %d", converted_len, dest_len));

			/* Go to next part */
			dest_len -= converted_len;
			dest += converted_len;

			/* End of audio frame ? */
			if (current_replay<end_replay)
				continue;

//			D(bug("audiodma: end of frame"));
				
			if (getAUDIODMA()->control & CTRL_PLAYBACK_ENABLE) {
				if (getAUDIODMA()->control & CTRL_PLAYBACK_REPEAT) {
					start_replay = current_replay = getAUDIODMA()->start;
					end_replay = getAUDIODMA()->end;			
					getAUDIODMA()->start_tic = SDL_GetTicks();
					D(bug("audiodma: playback loop: 0x%08x to 0x%08x", start_replay, end_replay));
				} else {
					getAUDIODMA()->control &= ~CTRL_PLAYBACK_ENABLE;
					playing = SDL_AUDIO_STOPPED;
					D(bug("audiodma: playback stop"));
					memset(dest, 0x80, dest_len);
					break;
				}
			}

			/* Trigger MFP interrupt if needed */
			if (getAUDIODMA()->control & CTRL_TIMERA_PLAYBACK_END) {
				trigger_interrupt = 1;
				D(bug("audiodma: MFP Timer A interrupt to trigger"));
			} else if (getAUDIODMA()->control & CTRL_MFPI7_PLAYBACK_END) {
				trigger_interrupt = 1;
				D(bug("audiodma: MFP I7 interrupt to trigger"));
			}
		}

		if (trigger_interrupt) {
			/* Generate MFP interrupt if needed */
			if (getAUDIODMA()->control & CTRL_TIMERA_PLAYBACK_END) {
				getMFP()->IRQ(13, 1);
				D(bug("audiodma: MFP Timer A interrupt triggered"));
			} else if (getAUDIODMA()->control & CTRL_MFPI7_PLAYBACK_END) {
				getMFP()->IRQ(15, 1);
				D(bug("audiodma: MFP I7 interrupt triggered"));
			}
		}
		
		/* Convert Atari buffer to host format */
		cvt.buf = (Uint8 *)tmp_buf;
		cvt.len = (int) (len / cvt.len_ratio);
		SDL_ConvertAudio(&cvt);

		SDL_MixAudio(stream, cvt.buf, len, SDL_MIX_MAXVOLUME);
	}
};

/*--- Constructor/destructor of class ---*/

AUDIODMA::AUDIODMA(memptr addr, uint32 size) : BASE_IO(addr, size)
{
	D(bug("audiodma: interface created at 0x%06x", getHWoffset()));

	freqs[MODE_FREQ0>>MODE_FREQ] = 6146;	/* 25.175MHz / (256 * 16) */
	freqs[MODE_FREQ1>>MODE_FREQ] = 12292;	/* 25.175MHz / (256 * 8) */
	freqs[MODE_FREQ2>>MODE_FREQ] = 24585;	/* 25.175MHz / (256 * 4) */
	freqs[MODE_FREQ3>>MODE_FREQ] = 49170;	/* 25.175MHz / (256 * 2) */
	tmp_buf=NULL;
	reset();

	audio->AddCallback(audio_callback, NULL);
}

AUDIODMA::~AUDIODMA()
{
	D(bug("audiodma: interface destroyed at 0x%06x", getHWoffset()));
	audio->RemoveCallback(audio_callback);
	reset();
}

/*--- Public functions ---*/

void AUDIODMA::reset()
{
	start_tic = SDL_GetTicks();
	start = current = end = control = mode = 0;
	playing = SDL_AUDIO_STOPPED;
	format = channels = freq = 0;
	start_replay = current_replay = end_replay = 0;
	offset = skip = 0;
	tmp_buf_len = 0;
	if (tmp_buf) {
		free(tmp_buf);
		tmp_buf=NULL;
	}
	D(bug("audiodma: reset"));
}

uae_u8 AUDIODMA::handleRead(uaecptr addr)
{
	uae_u8 value=0;

	switch(addr-getHWoffset()) {
		case 0x00:
			value = (control>>8) & 0xff;
			break;
		case 0x01:
			value = control & 0xff;
			break;
		case 0x03:
			value = (start>>16) & 0xff;
			break;
		case 0x05:
			value = (start>>8) & 0xff;
			break;
		case 0x07:
			value = start & 0xff;
			break;
		case 0x09:
			updateCurrent();
			value = (current>>16) & 0xff;
			break;
		case 0x0b:
			updateCurrent();
			value = (current>>8) & 0xff;
			break;
		case 0x0d:
			updateCurrent();
			value = current & 0xff;
			break;
		case 0x0f:
			value = (end>>16) & 0xff;
			break;
		case 0x11:
			value = (end>>8) & 0xff;
			break;
		case 0x13:
			value = end & 0xff;
			break;
		case 0x20:
			value = (mode>>8) & 0xff;
			break;
		case 0x21:
			value = mode & 0xff;
			break;
	}

#if 0 /*DEBUG*/
	switch(addr-getHWoffset()) {
		case 0x00:
		case 0x01:
			D(bug("audiodma: control=0x%04x", control));
			break;
		case 0x03:
		case 0x05:
		case 0x07:
			D(bug("audiodma: start=0x%06x", start));
			break;
		case 0x09:
		case 0x0b:
		case 0x0d:
			D(bug("audiodma: current=0x%06x", current));
			break;
		case 0x0f:
		case 0x11:
		case 0x13:
			D(bug("audiodma: end=0x%06x", end));
			break;
		case 0x20:
		case 0x21:
			D(bug("audiodma: mode=0x%04x", mode));
			break;
	}
#endif

	return value;
}

void AUDIODMA::handleWrite(uaecptr addr, uae_u8 value)
{
	switch(addr-getHWoffset()) {
		case 0x00:
			control &= 0x00ff;
			control |= value<<8;
			SDL_LockAudio();
			updateControl();
			SDL_UnlockAudio();
			break;
		case 0x01:
			control &= 0xff00;
			control |= value;
			SDL_LockAudio();
			updateControl();
			SDL_UnlockAudio();
			break;
		case 0x03:
			if ((control & CTRL_RECORD_SELECT)==CTRL_PLAYBACK_SELECT) {
				start &= 0x0000ffff;
				start |= value<<16;
			}
			break;
		case 0x05:
			if ((control & CTRL_RECORD_SELECT)==CTRL_PLAYBACK_SELECT) {
				start &= 0x00ff00ff;
				start |= value<<8;
			}
			break;
		case 0x07:
			if ((control & CTRL_RECORD_SELECT)==CTRL_PLAYBACK_SELECT) {
				start &= 0x00ffff00;
				start |= value;
			}
			break;
		case 0x0f:
			if ((control & CTRL_RECORD_SELECT)==CTRL_PLAYBACK_SELECT) {
				end &= 0x0000ffff;
				end |= value<<16;
			}
			break;
		case 0x11:
			if ((control & CTRL_RECORD_SELECT)==CTRL_PLAYBACK_SELECT) {
				end &= 0x00ff00ff;
				end |= value<<8;
			}
			break;
		case 0x13:
			if ((control & CTRL_RECORD_SELECT)==CTRL_PLAYBACK_SELECT) {
				end &= 0x00ffff00;
				end |= value;
			}
			break;
		case 0x20:
			mode &= 0x00ff;
			mode |= value<<8;
			SDL_LockAudio();
			updateMode();
			SDL_UnlockAudio();
			break;
		case 0x21:
			mode &= 0xff00;
			mode |= value;
			SDL_LockAudio();
			updateMode();
			SDL_UnlockAudio();
			break;
	}

#if DEBUG
	switch(addr-getHWoffset()) {
		case 0x00:
		case 0x01:
			D(bug("audiodma: control=0x%04x", control));
			if (control & CTRL_TIMERA_RECORD_END) {
				D(bug("audiodma:  Timer A interrupt at end of record"));
			}
			if (control & CTRL_TIMERA_PLAYBACK_END) {
				D(bug("audiodma:  Timer A interrupt at end of playback"));
			}
			if (control & CTRL_MFPI7_RECORD_END) {
				D(bug("audiodma:  MFP I7 interrupt at end of record"));
			}
			if (control & CTRL_MFPI7_PLAYBACK_END) {
				D(bug("audiodma:  MFP I7 interrupt at end of playback"));
			}
			if ((control & CTRL_RECORD_SELECT)==CTRL_RECORD_SELECT) {
				D(bug("audiodma:  Select record registers"));
			} else {
				D(bug("audiodma:  Select playback registers"));
			}
			if (control & CTRL_RECORD_REPEAT) {
				D(bug("audiodma:  Record repeat mode"));
			}
			if (control & CTRL_RECORD_ENABLE) {
				D(bug("audiodma:  Record mode enabled"));
			}
			if (control & CTRL_PLAYBACK_REPEAT) {
				D(bug("audiodma:  Playback repeat mode"));
			}
			if (control & CTRL_PLAYBACK_ENABLE) {
				D(bug("audiodma:  Playback mode enabled"));
			}
			break;
		case 0x03:
		case 0x05:
		case 0x07:
			D(bug("audiodma: start=0x%06x", start));
			break;
		case 0x0f:
		case 0x11:
		case 0x13:
			D(bug("audiodma: end=0x%06x", end));
			break;
		case 0x20:
		case 0x21:
			D(bug("audiodma: mode=0x%04x", mode));
			D(bug("audiodma:  Monitor track %d", (mode>>MODE_MONITOR_TRACK) & MODE_MONITOR_TRACK_MASK));
			D(bug("audiodma:  Play %d tracks", ((mode>>MODE_PLAY_TRACK) & MODE_PLAY_TRACK_MASK)+1));
			switch (mode & (MODE_FORMAT_MASK<<MODE_FORMAT)) {
				case MODE_FORMAT_8STEREO:
					D(bug("audiodma:  8 bits stereo"));
					break;
				case MODE_FORMAT_16STEREO:
					D(bug("audiodma:  16 bits stereo"));
					break;
				case MODE_FORMAT_8MONO:
					D(bug("audiodma:  8 bits mono"));
					break;
				default:
					D(bug("audiodma:  Unknown format: %d", (mode>>MODE_FORMAT)&MODE_FORMAT_MASK));
					break;
			}
			D(bug("audiodma:  %d Hz", freqs[(mode>>MODE_FREQ) & MODE_FREQ_MASK]));
			break;
	}
#endif
}

/*--- Private  functions ---*/

void AUDIODMA::updateCurrent(void)
{
	current = start;

	if (playing == SDL_AUDIO_PLAYING) {
		current += (end_replay-start_replay)>>1;
	}
}

void AUDIODMA::updateControl(void)
{
	if ((control & CTRL_PLAYBACK_ENABLE)==CTRL_PLAYBACK_ENABLE) {
		/* Start replay ? */
		if (playing == SDL_AUDIO_STOPPED) {
			start_replay = current_replay = start;
			end_replay = end;			
			start_tic = SDL_GetTicks();
			playing = SDL_AUDIO_PLAYING;
		}
	} else {
		/* Stop replay ? */
		if (playing == SDL_AUDIO_PLAYING) {
			playing = SDL_AUDIO_STOPPED;
		}
	}
}

void AUDIODMA::updateMode(void)
{
	switch (mode & (MODE_FORMAT_MASK<<MODE_FORMAT)) {
		case MODE_FORMAT_8STEREO:
			format = AUDIO_S8;
			channels = 2;
			break;
		case MODE_FORMAT_8MONO:
			format = AUDIO_S8;
			channels = 1;
			break;
		case MODE_FORMAT_16STEREO:
		default:
			format = AUDIO_S16MSB;
			channels = 2;
			break;
	}

	offset = (mode>>MODE_MONITOR_TRACK) & MODE_MONITOR_TRACK_MASK;
	offset *= ((format & 0xff)>>3)*channels;
	skip = ((mode>>MODE_PLAY_TRACK) & MODE_PLAY_TRACK_MASK)+1;
	skip *= ((format & 0xff)>>3)*channels;

	updateFreq();

	D(bug("audiodma: mode: format 0x%04x, %d channels, offset %d, skip %d, %d freq",
		format, channels, offset, skip, freq));

	SDL_BuildAudioCVT(&cvt,
		format, channels, audio->obtained.freq,
		audio->obtained.format, audio->obtained.channels, audio->obtained.freq
	);
}

void AUDIODMA::updateFreq(void)
{
	int prediv;

	prediv = getCROSSBAR()->getIntPrediv();
	if (prediv == 0) {
		freq = freqs[(mode>>MODE_FREQ) & MODE_FREQ_MASK];
	} else {
		freq = getCROSSBAR()->getIntFreq() / (256 * (prediv+1));
	}
	D(bug("audiodma:  freq %d Hz", freq));
}
