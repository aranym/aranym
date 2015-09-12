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

#define DEBUG 0
#include "debug.h"

#include <cstdlib>
#include <cstring>
#include "SDL_compat.h"

#define ZMAGXSND_BUFSIZE 4096

extern "C" {
	static SDL_AudioStatus playing;
	static SDL_AudioCVT	cvt;
	static uint32 cvt_buf_len;

	static void audio_callback(void *userdata, uint8 * stream, int len)
	{
		if ((playing!=SDL_AUDIO_PLAYING) || (!userdata))
			return;

		AUDIOPAR *par = (AUDIOPAR *)userdata;

		if (!par->buffer)
			return;

		uint8 *buffer = Atari2HostAddr(par->buffer);

		if (cvt.needed) {
			/* Convert Atari audio to host audio */
			par->len = (uint32 ) (len / cvt.len_ratio);
			if (par->len > ZMAGXSND_BUFSIZE) {
				par->len = ZMAGXSND_BUFSIZE;
			}
			uint32 bufSize = par->len * cvt.len_mult;
			
			/* Current buffer too small ? */
			if (cvt_buf_len<bufSize) {
				if (cvt.buf) {
					free(cvt.buf);
					cvt.buf=NULL;
				}
			}

			/* Allocate needed buffer */
			if (cvt.buf==NULL) {
				cvt.buf=(uint8 *)malloc(bufSize);
				cvt_buf_len = bufSize;
			}

			memcpy(cvt.buf, buffer, par->len);
			cvt.len = par->len;
			SDL_ConvertAudio(&cvt);
			SDL_MixAudio(stream, cvt.buf, cvt.len_cvt, par->volume);
		} else {
			par->len = len;
			if (par->len > ZMAGXSND_BUFSIZE) {
				par->len = ZMAGXSND_BUFSIZE;
			}

			SDL_MixAudio(stream, buffer, par->len, par->volume);
		}

		TriggerInt5();		// Audio is at interrupt level 5
	}
};

AUDIODriver::AUDIODriver()
{
	cvt.buf = NULL;
	cvt_buf_len = 0;
	reset();
	host->audio.AddCallback(audio_callback, &AudioParameters);
}

AUDIODriver::~AUDIODriver()
{
	host->audio.RemoveCallback(audio_callback);
	reset();
}

void AUDIODriver::reset()
{
	playing = SDL_AUDIO_STOPPED;
	AudioParameters.buffer = 0;
	AudioParameters.len = 0;
	AudioParameters.volume = SDL_MIX_MAXVOLUME;
	if (cvt.buf) {
		free(cvt.buf);
		cvt.buf = NULL;
	}
	locked = false;
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
		case 1:					// OpenAudio:
			D(bug("Audio: OpenAudio: 0x%08x", getParameter(4)));
			SDL_BuildAudioCVT(&cvt,
				(uint16)getParameter(1),
				(uint8)getParameter(2),
				getParameter(0),
				host->audio.obtained.format,
				host->audio.obtained.channels,
				host->audio.obtained.freq
			);
			AudioParameters.buffer = getParameter(4);
			AudioParameters.freq = getParameter(0);
			AudioParameters.len = 0;
			playing = SDL_AUDIO_PAUSED;
			ret = (uint16)getParameter(1);
			break;

		case 2:					// CloseAudio
			D(bug("Audio: CloseAudio"));
			playing = SDL_AUDIO_STOPPED;
			break;

		case 3:					// PauseAudio
			D(bug("Audio: PauseAudio: %d", getParameter(0)));
			if (getParameter(0)==0) {
				playing = SDL_AUDIO_PLAYING;
			} else {
				playing = SDL_AUDIO_PAUSED;
			}
			break;

		case 4:					// AudioStatus
			D(bug("Audio: AudioStatus: %d", playing));
			ret = playing;
			break;

		case 5:					// AudioVolume
			AudioParameters.volume = (uint16)getParameter(0);
			D(bug("Audio: AudioVolume: %d", AudioParameters.volume));
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
