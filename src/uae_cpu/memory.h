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

extern uintptr MEMBaseDiff;
//#define do_get_real_address(a)		((uae_u8 *)(a) + MEMBaseDiff)
#define do_get_real_address(a)		(((a) < 0xff000000) ? ((uae_u8 *)(a) + MEMBaseDiff) : ((uae_u8 *)(a & 0x00ffffff) + MEMBaseDiff))
#define do_get_virtual_address(a)	((uae_u32)(a) - MEMBaseDiff)
#define InitMEMBaseDiff(va, ra)		(MEMBaseDiff = (uintptr)(va) - (uintptr)(ra))

static __inline__ uae_u32 get_long(uaecptr addr)
{
    uae_u32 * const m = (uae_u32 *)do_get_real_address(addr);
    return do_get_mem_long(m);
}
static __inline__ uae_u32 get_word(uaecptr addr)
{
    uae_u16 * const m = (uae_u16 *)do_get_real_address(addr);
    return do_get_mem_word(m);
}
static __inline__ uae_u32 get_byte(uaecptr addr)
{
    uae_u8 * const m = (uae_u8 *)do_get_real_address(addr);
    return do_get_mem_byte(m);
}
static __inline__ void put_long(uaecptr addr, uae_u32 l)
{
    uae_u32 * const m = (uae_u32 *)do_get_real_address(addr);
    do_put_mem_long(m, l);
}
static __inline__ void put_word(uaecptr addr, uae_u32 w)
{
    uae_u16 * const m = (uae_u16 *)do_get_real_address(addr);
    do_put_mem_word(m, w);
}
static __inline__ void put_byte(uaecptr addr, uae_u32 b)
{
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

