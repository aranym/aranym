#ifndef _AUDIODRV_H
#define _AUDIODRV_H

#include "cpu_emulation.h"

typedef struct audio_parameters
{
	memptr buffer;
	uint32 len;
	uint32 volume;
} AUDIOPAR;

class AudioDriver
{
public:
	void dispatch(uint32 fncode, M68kRegisters * r);
};

#endif
