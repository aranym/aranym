#include <SDL.h>
#include <stdlib.h>
#include <string.h>

#include "sysdeps.h"
#include "cpu_emulation.h"
#include "nfaudio.h"
#include "audio.h"

#define DEBUG 0
#include "debug.h"

extern AUDIO *audio;

extern "C" {
	static SDL_audiostatus playing;
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

			/* Current buffer too small ? */
			if (cvt_buf_len<par->len) {
				if (cvt.buf) {
					free(cvt.buf);
					cvt.buf=NULL;
				}
			}

			/* Allocate needed buffer */
			if (cvt.buf==NULL) {
				cvt.buf=(Uint8 *)malloc(par->len);
				cvt_buf_len = par->len;
			}

			memcpy(cvt.buf, buffer, par->len);
			cvt.len = par->len;
			SDL_ConvertAudio(&cvt);
			SDL_MixAudio(stream, cvt.buf, len, par->volume);
		} else {
			par->len = len;
			SDL_MixAudio(stream, buffer, len, par->volume);
		}

		TriggerInt5();		// Audio is at interrupt level 5
	}
};

AUDIODriver::AUDIODriver()
{
	cvt.buf = NULL;
	cvt_buf_len = 0;
	reset();
	audio->AddCallback(audio_callback, &AudioParameters);
}

AUDIODriver::~AUDIODriver()
{
	audio->RemoveCallback(audio_callback);
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
}

char *AUDIODriver::name()
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
		case 1:					// OpenAudio:
			D(bug("Audio: OpenAudio: 0x%08x", getParameter(4)));
			SDL_BuildAudioCVT(&cvt,
				(uint16)getParameter(1), (uint8)getParameter(2), getParameter(0),
				audio->obtained.format, audio->obtained.channels, audio->obtained.freq
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
			break;

		case 7:					// UnlockAudio    
			D(bug("Audio: UnlockAudio"));
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
