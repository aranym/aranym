/*
 * $Header$
 *
 */

#ifndef HARDWARE_H
#define HARDWARE_H

#include "aradata.h"
#include "mfp.h"
#include "acia.h"
#include "yamaha.h"
#include "videl.h"
#include "parallel.h"

extern ARADATA aradata;
extern MFP mfp;
extern IKBD ikbd;
extern YAMAHA yamaha;
extern VIDEL videl;

extern Parallel parallel;

extern uae_u32 vram_addr;

extern void HWInit(void);

extern uae_u32 HWget_l(uaecptr addr);
extern uae_u16 HWget_w(uaecptr addr);
extern uae_u8 HWget_b(uaecptr addr);

extern void HWput_l(uaecptr addr, uae_u32 l);
extern void HWput_w(uaecptr addr, uae_u16 w);
extern void HWput_b(uaecptr addr, uae_u8 b);

extern char* debug_print_IO(uaecptr addr);	// for debugging only

#endif 


/*
 * $Log$
 * Revision 1.18  2001/11/19 17:48:58  joy
 * parallel port emulation
 *
 * Revision 1.17  2001/09/21 14:23:33  joy
 * little things just to make it compilable
 *
 * Revision 1.16  2001/09/18 12:38:26  joy
 * extern ARADATA until all I/O HW modules are placed in one Aranym object.
 *
 * Revision 1.15  2001/08/21 18:19:16  milan
 * CPU update, disk's geometry autodetection - the 1st step
 *
 * Revision 1.14  2001/08/13 22:29:06  milan
 * IDE's params from aranymrc file etc.
 *
 * Revision 1.13  2001/07/12 22:12:15  standa
 * updateHostScreen() added.
 *
 * Revision 1.12  2001/06/18 13:21:55  standa
 * Several template.cpp like comments were added.
 * HostScreen SDL encapsulation class.
 *
 *
 */
