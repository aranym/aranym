#include "sysdeps.h"
#include "cpu_emulation.h"
#include "audio.h"

#define DEBUG 0
#include "debug.h"

static void audio_callback(void *userdata, Uint8 * stream, int len);

int32 AUDIODriver::dispatch(uint32 fncode)
{
	static SDL_AudioSpec spec_desired;
	static SDL_AudioSpec spec_obtained;
	static AUDIOPAR AudioParameters = { 0, 0, SDL_MIX_MAXVOLUME };
        int32 ret=1;

	switch (fncode) {
	// NEEDED functions
        
        case  0: 				// version
		D(bug("Audio: Version"));
                ret = 0x0061;
		break;

	case 1:					// OpenAudio:
		spec_desired.freq = getParameter(0);
		spec_desired.format = (uint16)getParameter(1);
		spec_desired.channels = (uint8)getParameter(2);
		spec_desired.samples = (uint16)getParameter(3);
		spec_desired.userdata = (void *) &AudioParameters;
		AudioParameters.buffer = getParameter(4);
		AudioParameters.len = 0;
		D(bug
		  ("Audio: OpenAudio %x Fr:%d Ch:%d S:%d", spec_desired.format,
		   spec_desired.freq, spec_desired.channels,
		   spec_desired.samples));
		spec_desired.callback = audio_callback;
		ret = SDL_OpenAudio(&spec_desired, &spec_obtained);
		if (ret < 0) {
			D(bug("Audio: OpenAudio error: %s", SDL_GetError()));
		}
		else {
			D(bug
			  ("Audio: OpenAudio return %x Fr:%d Ch:%d S:%d",
			   spec_obtained.format, spec_obtained.freq,
			   spec_obtained.channels, spec_obtained.samples));
			if (spec_desired.userdata && spec_obtained.size) {
				AUDIOPAR *par = (AUDIOPAR *)(spec_desired.userdata);
				uint8 *buffer = Atari2HostAddr(par->buffer);
				memset(buffer, 0, spec_obtained.size);
			}
			ret = (uint32) spec_obtained.format;
		}
		break;

	case 2:					// CloseAudio
		D(bug("Audio: CloseAudio"));
		SDL_CloseAudio();
		break;

	case 3:					// PauseAudio
		D(bug("Audio: PauseAudio %d", getParameter(0)));
		SDL_PauseAudio(getParameter(0));
		break;

	case 4:					// AudioStatus
		ret = (uint32) SDL_GetAudioStatus();
		D(bug("Audio: AudioStatus %d", ret));
		break;

	case 5:					// AudioVolume
		AudioParameters.volume = (uint16)getParameter(0);
		D(bug("Audio: AudioVolume %d", AudioParameters.volume));
		break;

	case 6:					// LockAudio         
		D(bug("Audio: LockAudio"));
		SDL_LockAudio();
		break;

	case 7:					// UnlockAudio    
		D(bug("Audio: UnlockAudio"));
		SDL_UnlockAudio();
		break;

	case 8:					// GetAudioFreq
		D(bug("Audio: GetAudioFreq %d", spec_obtained.freq));
		ret = spec_obtained.freq;
		break;

	case 9:					// GetAudioLen
		D(bug("Audio: GetAudioLen %d", AudioParameters.len));
		ret = AudioParameters.len;
		break;

	// not implemented functions
	default:
		D(bug("Audio: Unknown %d", fncode));
	}
        return(ret);
}

static void audio_callback(void *userdata, uint8 * stream, int len)
{
	AUDIOPAR *par = (AUDIOPAR *)userdata;
	if (userdata) {
		uint8 *buffer = Atari2HostAddr(par->buffer);
		SDL_MixAudio(stream, buffer, len, par->volume);
		par->len = len;
		TriggerInt5();		// Audio is at interrupt level 5
	}
}
