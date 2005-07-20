/*
 * hardware.h - Atari hardware emulation - definitions
 *
 * Copyright (c) 2001-2004 Petr Stehlik of ARAnyM dev team (see AUTHORS)
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
#include "ide.h"
#include "audio_dma.h"

extern Parallel *parallel;

extern uae_u32 vram_addr;

void HWInit(void);
void HWExit(void);
void HWReset(void);

ARADATA *getARADATA(void);
MFP *getMFP(void);
IKBD *getIKBD(void);
MIDI *getMIDI(void);
YAMAHA *getYAMAHA(void);
VIDEL *getVIDEL(void);
DSP *getDSP(void);
ACSIFDC *getFDC(void);
IDE *getIDE(void);
AUDIODMA *getAUDIODMA(void);

extern uae_u32 HWget_l(uaecptr addr);
extern uae_u16 HWget_w(uaecptr addr);
extern uae_u8 HWget_b(uaecptr addr);

extern void HWput_l(uaecptr addr, uae_u32 l);
extern void HWput_w(uaecptr addr, uae_u16 w);
extern void HWput_b(uaecptr addr, uae_u8 b);

extern char* debug_print_IO(uaecptr addr);	// for debugging only

#endif 
