/* MJ 2001 */

#ifndef HARDWARE_H
#define HARDWARE_H

#define MEM_CTL_A	0xffff8000
#define MEM_CTL		0x0a		// 0xffff8000 - 2MB banks
#define SYS_CTL_A	0xffff8006
#define SYS_CTL		0x8001		// 0xffff8006 - VGA monitor, CPU speed full

extern void HWInit(void);

extern uae_u32 HWget_l(uaecptr addr);
extern uae_u32 HWget_w(uaecptr addr);
extern uae_u32 HWget_b(uaecptr addr);

extern void HWput_l(uaecptr addr, uae_u32 l);
extern void HWput_w(uaecptr addr, uae_u32 w);
extern void HWput_b(uaecptr addr, uae_u32 b);

#endif 
