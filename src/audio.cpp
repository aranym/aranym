
#include "sysdeps.h"
#include "cpu_emulation.h"
#include "main.h"
#include "audio.h"
#include "hardware.h"

#define DEBUG 1
#include "debug.h"

static void audio_callback(void *userdata, Uint8 * stream, int len);

void AudioDriver::dispatch(uint32 fncode, M68kRegisters * r)
{
	static SDL_AudioSpec spec_desired;
	static SDL_AudioSpec spec_obtained;
	static AUDIOPAR AudioParameters = { 0, 0, SDL_MIX_MAXVOLUME };
	// fix the stack (the fncode was pushed onto the stack)
	r->a[7] += 4;

	switch (fncode) {
	// NEEDED functions

	case 1:					// OpenAudio:
		spec_desired.freq = ReadInt32(r->a[7]);
		spec_desired.format = ReadInt16(r->a[7] + 4);
		spec_desired.channels = (uint8) ReadInt16(r->a[7] + 6);
		spec_desired.samples = ReadInt16(r->a[7] + 8);
		spec_desired.userdata = (void *) &AudioParameters;
		AudioParameters.buffer = (void *) ReadInt32(r->a[7] + 10);
		AudioParameters.len = 0;
		D(bug
		  ("Audio: OpenAudio %x Fr:%d Ch:%d S:%d", spec_desired.format,
		   spec_desired.freq, spec_desired.channels,
		   spec_desired.samples));
		spec_desired.callback = audio_callback;
		r->d[0] = SDL_OpenAudio(&spec_desired, &spec_obtained);
		if (r->d[0] < 0) {
			D(bug("Audio: OpenAudio error: %s", SDL_GetError()));
		}
		else {
			D(bug
			  ("Audio: OpenAudio return %x Fr:%d Ch:%d S:%d",
			   spec_obtained.format, spec_obtained.freq,
			   spec_obtained.channels, spec_obtained.samples));
			if (spec_desired.userdata && spec_obtained.size) {
				AUDIOPAR *par = (AUDIOPAR *)(spec_desired.userdata);
				void *buffer = do_get_real_address(par->buffer);
				memset(buffer, 0, spec_obtained.size);
			}
			r->d[0] = (uint32) spec_obtained.format;
		}
		break;

	case 2:					// CloseAudio
		D(bug("Audio: CloseAudio"));
		SDL_CloseAudio();
		break;

	case 3:					// PauseAudio
		D(bug("Audio: PauseAudio %d", ReadInt16(r->a[7])));
		SDL_PauseAudio(ReadInt16(r->a[7]));
		break;

	case 4:					// AudioStatus
		r->d[0] = (uint32) SDL_GetAudioStatus();
//		D(bug("Audio: AudioStatus %d", r->d[0]));
		break;

	case 5:					// AudioVolume
		AudioParameters.volume = ReadInt16(r->a[7]);
		D(bug("Audio: AudioVolume %d", AudioParameters.volume));
		break;

	case 6:					// LockAudio         
//		D(bug("Audio: LockAudio"));
		SDL_LockAudio();
		break;

	case 7:					// UnlockAudio    
//		D(bug("Audio: UnlockAudio"));
		SDL_UnlockAudio();
		break;

	case 8:					// GetAudioFreq
		D(bug("Audio: GetAudioFreq %d", spec_obtained.freq));
		r->d[0] = spec_obtained.freq;
		break;

	case 9:					// GetAudioLen
//		D(bug("Audio: GetAudioLen %d", AudioParameters.len));
		r->d[0] = AudioParameters.len;
		break;

	// not implemented functions
	default:
		D(bug("Audio: Unknown %d", fncode));
		r->d[0] = 1;
	}
}

static void audio_callback(void *userdata, uint8 * stream, int len)
{
	AUDIOPAR *par = (AUDIOPAR *)userdata;
	if (userdata) {
		uint8 *buffer = do_get_real_address(par->buffer);
		SDL_MixAudio(stream, buffer, len, par->volume);
		par->len = len;
		TriggerInterrupt();		// Interrupt level 5
	}
}
