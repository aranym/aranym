/*
 * $Header$
 *
 */

#ifndef HARDWARE_H
#define HARDWARE_H

#include "aradata.h"
#include "mfp.h"
#include "ikbd.h"
#include "midi.h"
#include "parallel.h"
#include "yamaha.h"
#include "videl.h"
#include "dsp.h"
#include "acsifdc.h"

extern Parallel parallel;

extern uae_u32 vram_addr;

void HWInit(void);
void HWExit(void);

ARADATA *getARADATA(void);
MFP *getMFP(void);
IKBD *getIKBD(void);
MIDI *getMIDI(void);
YAMAHA *getYAMAHA(void);
VIDEL *getVIDEL(void);
DSP *getDSP(void);
ACSIFDC *getFDC(void);

extern uae_u32 HWget_l(uaecptr addr);
extern uae_u16 HWget_w(uaecptr addr);
extern uae_u8 HWget_b(uaecptr addr);

extern void HWput_l(uaecptr addr, uae_u32 l);
extern void HWput_w(uaecptr addr, uae_u16 w);
extern void HWput_b(uaecptr addr, uae_u8 b);

extern char* debug_print_IO(uaecptr addr);	// for debugging only

#endif 
