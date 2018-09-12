/*
	Audio core

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

#ifndef HOSTAUDIO_H
#define HOSTAUDIO_H

#include "SDL_compat.h"

/*--- Defines ---*/

#define	MAX_AUDIO_CALLBACKS	8

/*--- Types ---*/

typedef void (*audio_callback_f)(void *unused, Uint8 *stream, int len);

/*--- Class ---*/

class HostAudio {
	private:
		SDL_AudioSpec	desired;
		bool recording;
		bool enabled;
		
	public:
		HostAudio();
		~HostAudio();
		void reset(void);

		/* private data but needed by SDL audio callback */
		audio_callback_f	callbacks[MAX_AUDIO_CALLBACKS];
		void	*userdatas[MAX_AUDIO_CALLBACKS];

		/* Register your external audio callback with this */
		void AddCallback(audio_callback_f callback, void *userdata);
		void RemoveCallback(audio_callback_f callback);

		/* Usable by external audio callbacks, consider read-only */
		SDL_AudioSpec	obtained;
		
		bool Recording(void) { return recording; }
		void StartRecording() { recording = true && enabled; }
		void StopRecording() { recording = false; }
		
		bool Enabled(void) { return enabled; }
		void Enable(bool enable);
		void ToggleAudio() { Enable(!enabled); }
		static const char *FormatName(Uint16 format);
};

#endif /* HOSTAUDIO_H */
