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
 
#include "sysdeps.h"
#include "hardware.h"
#include "cpu_emulation.h"
#include "memory-uae.h"
#include "host.h"
#include "audio_dma.h"
#include "audio_conv.h"

#define DEBUG 0
#include "debug.h"

#include "SDL_compat.h"


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
	static SDL_AudioStatus playing;
	static AudioConv *audioConv;

	static uint32	start_replay, current_replay, end_replay;
	
	static void audio_callback(void * /*userdata*/, uint8 * stream, int len)
	{
		Uint8 *dest;
		int dest_len, trigger_interrupt = 0;

		if ((playing!=SDL_AUDIO_PLAYING) || (start_replay==0) || (end_replay==0))
			return;

		dest = stream;
		dest_len = len;
		while (dest_len>0) {
			int src_len = end_replay-current_replay;
			int dst_len = dest_len;

			audioConv->doConversion(Atari2HostAddr(current_replay), &src_len, dest, &dst_len);

			dest_len -= dst_len;
			dest += dst_len;
			current_replay += src_len;

			/* End of audio frame ? */
			if (current_replay<end_replay)
				continue;
				
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
					if (dest_len>0) {
						memset(dest, 0x80, dest_len);
					}
					break;
				}
			}

			/* Need to trigger MFP interrupt ? */
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

	SDL_LockAudio();
	audioConv = new AudioConv();
	SDL_UnlockAudio();

	reset();

	host->audio.AddCallback(audio_callback, NULL);
}

AUDIODMA::~AUDIODMA()
{
	D(bug("audiodma: interface destroyed at 0x%06x", getHWoffset()));
	host->audio.RemoveCallback(audio_callback);

	SDL_LockAudio();
	delete audioConv;
	audioConv = NULL;
	SDL_UnlockAudio();

	reset();
}

/*--- Public functions ---*/

void AUDIODMA::reset()
{
	start_tic = SDL_GetTicks();
	start = current = end = control = mode = 0;
	playing = SDL_AUDIO_STOPPED;
	start_replay = current_replay = end_replay = 0;
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
			updateControl();
			break;
		case 0x01:
			control &= 0xff00;
			control |= value;
			updateControl();
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
			updateMode();
			break;
		case 0x21:
			mode &= 0xff00;
			mode |= value;
			updateMode();
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
	int time_elapsed, total_samples, total_duration, sample_size_shift;

	current = start_replay;

	if (playing == SDL_AUDIO_PLAYING) {
		time_elapsed = SDL_GetTicks() - start_tic;

		switch (mode & (MODE_FORMAT_MASK<<MODE_FORMAT)) {
			case MODE_FORMAT_8STEREO:
				sample_size_shift = 1;
				break;
			case MODE_FORMAT_8MONO:
				sample_size_shift = 0;
				break;
			case MODE_FORMAT_16STEREO:
			default:
				sample_size_shift = 2;
				break;
		}

		total_samples = (end_replay-start_replay)>>sample_size_shift;
		total_duration = (total_samples * 1000) / freq;
		if (total_duration<0) {
			total_duration = 1;
		}
		
		current += ((time_elapsed * total_samples)/ total_duration)<<sample_size_shift;

		if (current<start_replay) current=start_replay;
		if (current>end_replay) current=end_replay;
	}
}

void AUDIODMA::updateControl(void)
{
	SDL_LockAudio();

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

	SDL_UnlockAudio();
}

void AUDIODMA::updateMode(void)
{
	int channels, offset, skip, prediv;
	Uint16	format;

	SDL_LockAudio();

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

	prediv = getCROSSBAR()->getIntPrediv();
	if (prediv == 0) {
		freq = freqs[(mode>>MODE_FREQ) & MODE_FREQ_MASK];
	} else {
		freq = getCROSSBAR()->getIntFreq() / (256 * (prediv+1));
	}

	D(bug("audiodma: mode: format %s, %d channels, offset %d, skip %d, %d freq",
		HostAudio::FormatName(format), channels, offset, skip, freq));

	audioConv->setConversion(format, channels, freq, offset, skip,
		host->audio.obtained.format, host->audio.obtained.channels,
		host->audio.obtained.freq);

	SDL_UnlockAudio();
}
