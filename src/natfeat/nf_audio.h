#ifndef _NF_AUDIO_H
#define _NF_AUDIO_H
#include "nf_base.h"

typedef struct nf_audio_parameters
{
	memptr buffer;
	uint32 len;
	uint32 volume;
} AUDIOPAR;

class AUDIODriver : public NF_Base
{
public:
	char *name() { return "AUDIO"; }
	bool isSuperOnly() { return true; }
	int32 dispatch(uint32 fncode);
};

#endif /* _NF_AUDIO_H */
