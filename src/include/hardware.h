/* MJ 2001 */

#ifndef HARDWARE_H
#define HARDWARE_H

#define MEM_VALID_1_A	0x00000420
#define MEM_VALID_1	0x752019f3	// 0x00000420 - memvalid
#define MEM_VALID_2_A	0x0000043a
#define MEM_VALID_2	0x237698aa	// 0x0000043a - memvalid2
#define MEM_VALID_3_A	0x0000051a
#define MEM_VALID_3	0x5555aaaa	// 0x0000051a - memvalid3

#define SEEK_RATE_A	0x00000440
#define SEEK_RATE	0x0003		// 0x00000440 - seek rate

#define A_MEM_VALID_A	0x000005a8
#define A_MEM_VALID	0x1357bd13	// 0x000005a8 - if 0x000005a4 == TT-RAM

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
