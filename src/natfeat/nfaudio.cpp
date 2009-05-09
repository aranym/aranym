/*
 * nfaudio.cpp - NatFeat Audio driver
 *
 * Copyright (c) 2002-2006 ARAnyM dev team (see AUTHORS)
 * 
 * This file is part of the ARAnyM project which builds a new and powerful
 * TOS/FreeMiNT compatible virtual machine running on almost any hardware.
 *
 * ARAnyM is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * ARAnyM is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ARAnyM; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "sysdeps.h"
#include "cpu_emulation.h"
#include "nfaudio.h"
#include "host.h"
#include "host_audio.h"
#include "audio_conv.h"

#define DEBUG 0
#include "debug.h"

#include <cstdlib>
#include <cstring>
#include <SDL.h>

/* Zmagxsnd buffer size on Atari side */
#define ATARI_BUF_SIZE 4096

extern "C" {
	static SDL_audiostatus playing;
	static AudioConv *audioConv;

	static void audio_callback(void *userdata, uint8 * stream, int len)
	{
		if (playing!=SDL_AUDIO_PLAYING)
			return;

		AUDIOPAR *par = (AUDIOPAR *)userdata;
		if (!par->buffer) {
			return;
		}

		int src_len = ATARI_BUF_SIZE;
		int dst_len = len;

		audioConv->doConversion(Atari2HostAddr(par->buffer), &src_len, stream, &dst_len);
		par->len = src_len;

		TriggerInt5();		// Audio is at interrupt level 5
	}
};

AUDIODriver::AUDIODriver()
{
	SDL_LockAudio();
	audioConv = new AudioConv();
	SDL_UnlockAudio();

	reset();

	host->audio.AddCallback(audio_callback, &AudioParameters);
}

AUDIODriver::~AUDIODriver()
{
	host->audio.RemoveCallback(audio_callback);

	SDL_LockAudio();
	delete audioConv;
	audioConv = NULL;
	SDL_UnlockAudio();

	reset();
}

void AUDIODriver::reset()
{
	playing = SDL_AUDIO_STOPPED;
	AudioParameters.buffer = NULL;
	AudioParameters.len = 0;
	locked = false;
}

const char *AUDIODriver::name()
{
	return "AUDIO";
}

bool AUDIODriver::isSuperOnly()
{
	return true;
}

int32 AUDIODriver::dispatch(uint32 fncode)
{
	int32 ret = 1;

	switch (fncode) {
		// NEEDED functions
        
		case  0: 				// version
			D(bug("Audio: Version"));
			ret = 0x0061;
			break;
		case 1:		// OpenAudio(int freq, int format, int channels, int length, void *buffer)
			{
				Uint16 src_fmt = getParameter(1);
				int src_chans = getParameter(2);
				int src_freq = getParameter(0);
				int skip = ((src_fmt & 0xff)>>3)*src_chans;

				D(bug("Audio: OpenAudio: 0x%08x, format 0x%04x, chans %d, freq %d Hz",
					getParameter(4), src_fmt, src_chans, src_freq
				));

				SDL_LockAudio();
				audioConv->setConversion(src_fmt, src_chans, src_freq, 0, skip,
					host->audio.obtained.format, host->audio.obtained.channels,
					host->audio.obtained.freq);
				AudioParameters.buffer = getParameter(4);
				AudioParameters.freq = src_freq;
				AudioParameters.len = 0;
				playing = SDL_AUDIO_PAUSED;
				SDL_UnlockAudio();
				ret = (uint16)getParameter(1);
			}
			break;

		case 2:					// CloseAudio
			D(bug("Audio: CloseAudio"));
			playing = SDL_AUDIO_STOPPED;
			break;

		case 3:					// PauseAudio
			D(bug("Audio: PauseAudio: %d", getParameter(0)));
			playing = ((getParameter(0)==0) ? SDL_AUDIO_PLAYING : SDL_AUDIO_PAUSED);
			break;

		case 4:					// AudioStatus
			D(bug("Audio: AudioStatus: %d", playing));
			ret = playing;
			break;

		case 5:					// AudioVolume
			D(bug("Audio: AudioVolume: %d", (uint16)getParameter(0)));
			SDL_LockAudio();
			audioConv->setVolume((uint16)getParameter(0));
			SDL_UnlockAudio();
			break;

		case 6:					// LockAudio         
			D(bug("Audio: LockAudio"));
			if (! locked) {
				locked = true;
				ret = 1;	// lock acquired OK
			}
			else
				ret = -129;	// was already locked
			break;

		case 7:					// UnlockAudio    
			D(bug("Audio: UnlockAudio"));
			if (locked) {
				locked = false;
				ret = 0;	// unlocked OK
			}
			else
				ret = -128;	// wasn't locked at all
			break;

		case 8:					// GetAudioFreq
			D(bug("Audio: GetAudioFreq: %d", AudioParameters.freq));
			ret = AudioParameters.freq;
			break;

		case 9:					// GetAudioLen
			D(bug("Audio: GetAudioLen: %d", AudioParameters.len));
			ret = AudioParameters.len;
			break;
		// not implemented functions
		default:
			D(bug("Audio: Unknown %d", fncode));
	}
	return ret;
}

/*
vim:ts=4:sw=4:
*/
