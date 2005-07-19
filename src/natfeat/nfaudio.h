#ifndef _AUDIO_H
#define _AUDIO_H

#include "nf_base.h"

typedef struct nf_audio_parameters
{
	memptr buffer;
	uint32 freq;
	uint32 len;
	uint32 volume;
} AUDIOPAR;

class AUDIODriver : public NF_Base
{
	private:
		AUDIOPAR AudioParameters;

	public:
		AUDIODriver();
		~AUDIODriver();

		char *name();
		bool isSuperOnly();
		void reset();
		int32 dispatch(uint32 fncode);
};

#endif /* _AUDIO_H */
