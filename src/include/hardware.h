/*
 * $Header$
 *
 * MJ 2001
 */

#ifndef HARDWARE_H
#define HARDWARE_H

extern uae_u32 vram_addr;

extern void HWInit(void);

extern uae_u32 HWget_l(uaecptr addr);
extern uae_u32 HWget_w(uaecptr addr);
extern uae_u32 HWget_b(uaecptr addr);

extern void HWput_l(uaecptr addr, uae_u32 l);
extern void HWput_w(uaecptr addr, uae_u32 w);
extern void HWput_b(uaecptr addr, uae_u32 b);

extern uaecptr showPC(void);	// for debugging only
extern char* debug_print_IO(uaecptr addr);	// for debugging only

extern void MakeMFPIRQ(int);
extern void ikbd_send(int);

extern void renderScreen();
extern void updateHostScreen();

extern int getFloppyStats();
extern bool isIkbdBufEmpty();
#endif 


/*
 * $Log$
 * Revision 1.13  2001/07/12 22:12:15  standa
 * updateHostScreen() added.
 *
 * Revision 1.12  2001/06/18 13:21:55  standa
 * Several template.cpp like comments were added.
 * HostScreen SDL encapsulation class.
 *
 *
 */
