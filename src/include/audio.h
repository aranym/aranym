
#ifndef _AUDIODRV_H
#define _AUDIODRV_H

#include "cpu_emulation.h"

typedef struct audio_parameters
{
    void *buffer;
    int len;
    int volume;
} AUDIOPAR;

class AudioDriver {
  private:

  public:
	void dispatch( uint32 fncode, M68kRegisters *r );
};

#endif
