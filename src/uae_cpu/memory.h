/* 2001 MJ */

 /*
  * 
  * UAE - The Un*x Amiga Emulator
  *
  * memory management
  *
  * Copyright 1995 Bernd Schmidt
  */

#ifndef UAE_MEMORY_H
#define UAE_MEMORY_H

#include "hardware.h"

extern uintptr MEMBaseDiff;
extern uintptr VMEMBaseDiff;

//#define FALCVRAMSIZE	0x96100		/* TrueColor */
// #define FALCVRAMSIZE	0x08000		/* Mono */
// #define FALCVRAMSIZE	0x48000		/* 256 colors */

#if 0
#define FALCSTRAMEND	0x00400000
#define FALCVRAMSTART	(FALCSTRAMEND - FALCVRAMSIZE)
#else
//#define FALCVRAMSTART	0x00800000
//#define FALCVRAMSTART	0xf0000000
#endif
//#define FALCVRAMEND		(FALCVRAMSTART + FALCVRAMSIZE)

#if 1
#define ARANYMVRAMSTART	0xf0000000
#define do_get_real_address(a)		(((a) < 0xff000000) ? (((a) < ARANYMVRAMSTART) ? ((uae_u8 *)(a) + MEMBaseDiff) : ((uae_u8 *)(a) + VMEMBaseDiff)) : ((uae_u8 *)(a & 0x00ffffff) + MEMBaseDiff))
#else
#define ARANYMVRAMSTART	FALCVRAMSTART
#define do_get_real_address(a)		((((a) >= FALCVRAMSTART) && ((a) < FALCVRAMEND)) ? ((uae_u8 *)(a) + VMEMBaseDiff) : (((a) < 0xff000000) ? ((uae_u8 *)(a) + MEMBaseDiff) : ((uae_u8 *)(a & 0x00ffffff) + MEMBaseDiff)))
#endif

#define do_get_virtual_address(a)	((uae_u32)(a) - MEMBaseDiff)	// only for xRAM, not for VideoRAM!
#define InitMEMBaseDiff(va, ra)		(MEMBaseDiff = (uintptr)(va) - (uintptr)(ra))
#define InitVMEMBaseDiff(va, ra)	(VMEMBaseDiff = (uintptr)(va) - (uintptr)(ra))

static __inline__ uae_u32 get_long(uaecptr addr)
{
    if (((addr & 0xfff00000) == 0x00f00000) || ((addr & 0xfff00000) == 0xfff00000)) return HWget_l(addr & 0x00ffffff);
    uae_u32 * const m = (uae_u32 *)do_get_real_address(addr);
    return do_get_mem_long(m);
}
static __inline__ uae_u32 get_word(uaecptr addr)
{
    if (((addr & 0xfff00000) == 0x00f00000) || ((addr & 0xfff00000) == 0xfff00000)) return HWget_w(addr & 0x00ffffff);
    uae_u16 * const m = (uae_u16 *)do_get_real_address(addr);
    return do_get_mem_word(m);
}
static __inline__ uae_u32 get_byte(uaecptr addr)
{
    if (((addr & 0xfff00000) == 0x00f00000) || ((addr & 0xfff00000) == 0xfff00000)) return HWget_b(addr & 0x00ffffff);
    uae_u8 * const m = (uae_u8 *)do_get_real_address(addr);
    return do_get_mem_byte(m);
}
static __inline__ void put_long(uaecptr addr, uae_u32 l)
{
    if (((addr & 0xfff00000) == 0x00f00000) || ((addr & 0xfff00000) == 0xfff00000)) {
        HWput_l(addr & 0x00ffffff, l);
        return;
    } 
    uae_u32 * const m = (uae_u32 *)do_get_real_address(addr);
    do_put_mem_long(m, l);
}
static __inline__ void put_word(uaecptr addr, uae_u32 w)
{
    if (((addr & 0xfff00000) == 0x00f00000) || ((addr & 0xfff00000) == 0xfff00000)) {
        HWput_w(addr & 0x00ffffff, w);
        return;
    }
    uae_u16 * const m = (uae_u16 *)do_get_real_address(addr);
    do_put_mem_word(m, w);
}
static __inline__ void put_byte(uaecptr addr, uae_u32 b)
{
    if (((addr & 0xfff00000) == 0x00f00000) || ((addr & 0xfff00000) == 0xfff00000)) {
        HWput_b(addr & 0x00ffffff, b);
        return;
    }
    uae_u8 * const m = (uae_u8 *)do_get_real_address(addr);
    do_put_mem_byte(m, b);
}
static __inline__ uae_u8 *get_real_address(uaecptr addr)
{
	return do_get_real_address(addr);
}
static __inline__ uae_u32 get_virtual_address(uae_u8 *addr)
{
	return do_get_virtual_address(addr);
}
static __inline__ int valid_address(uaecptr addr, uae_u32 size)
{
    return 1;
}
#endif /* MEMORY_H */

