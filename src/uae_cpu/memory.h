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
#include "sysdeps.h"
#include "m68k.h"

#define ATCSIZE	256

#define REGS
typedef char flagtype;

extern struct regstruct         // MJ tuhle ptakovinu uz musim konecne odstranit
{
    uae_u32 regs[16];
    uaecptr  usp,isp,msp;
    uae_u16 sr;
    flagtype t1;
    flagtype t0;
    flagtype s;
    flagtype m;
    flagtype x;
    flagtype stopped;
    int intmask;

    uae_u32 pc;
    uae_u32 pcp;	//    uae_u8 *pc_p;
    uae_u32 pcoldp;	//    uae_u8 *pc_oldp;

    uae_u32 vbr,sfc,dfc;

    double fp[8];
    uae_u32 fpcr,fpsr,fpiar;

    uae_u32 spcflags;
    uae_u32 kick_mask;

    /* Fellow sources say this is 4 longwords. That's impossible. It needs
     * to be at least a longword. The HRM has some cryptic comment about two
     * instructions being on the same longword boundary.
     * The way this is implemented now seems like a good compromise.
     */
    uae_u32 prefetch;

    /* MMU reg*/
    uae_u32 urp,srp;
    flagtype tce;
    flagtype tcp;
    uae_u32 dtt0,dtt1,itt0,itt1,mmusr;

    flagtype atcvali[ATCSIZE];
    flagtype atcvald[ATCSIZE];
    flagtype atcu0d[ATCSIZE];
    flagtype atcu0i[ATCSIZE];
    flagtype atcu1d[ATCSIZE];
    flagtype atcu1i[ATCSIZE];
    flagtype atcsuperd[ATCSIZE];
    flagtype atcsuperi[ATCSIZE];
    int atccmd[ATCSIZE];
    int atccmi[ATCSIZE];
    flagtype atcmodifd[ATCSIZE];
    flagtype atcmodifi[ATCSIZE];
    flagtype atcwritepd[ATCSIZE];
    flagtype atcwritepi[ATCSIZE];
    flagtype atcresidd[ATCSIZE];
    flagtype atcresidi[ATCSIZE];
    flagtype atcglobald[ATCSIZE];
    flagtype atcglobali[ATCSIZE];
    flagtype atcfc2d[ATCSIZE];
    flagtype atcfc2i[ATCSIZE];
    uaecptr atcind[ATCSIZE];
    uaecptr atcini[ATCSIZE];
    uaecptr atcoutd[ATCSIZE];
    uaecptr atcouti[ATCSIZE];

    /* Cache reg*/
    int cacr,caar;
} regs, lastint_regs;


extern uintptr MEMBaseDiff;
extern uintptr VMEMBaseDiff;
#define ARANYMVRAMSTART 0xf0000000

#ifdef MMU
# define do_get_real_address(a,b,c)	do_get_real_address_mmu(a,b,c)
# define get_long(a,b)			get_long_mmu(a,b)
# define get_word(a,b)			get_word_mmu(a,b)
# define get_byte(a,b)			get_byte_mmu(a,b)
# define put_long(a,b)			put_long_mmu(a,b)
# define put_word(a,b)			put_word_mmu(a,b)
# define put_byte(a,b)			put_byte_mmu(a,b)
# define get_real_address(a,b,c)	get_real_address_mmu(a,b,c)
#else
# define do_get_real_address(a,b,c)		(((a) < 0xff000000) ? (((a) < ARANYMVRAMSTART) ? ((uae_u8 *)(a) + MEMBaseDiff) : ((uae_u8*)(a) + VMEMBaseDiff)) : ((uae_u8 *)(a & 0x00ffffff) + MEMBaseDiff))
# define get_long(a,b)			get_long_direct(a)
# define get_word(a,b)			get_word_direct(a)
# define get_byte(a,b)			get_byte_direct(a)
# define put_long(a,b)			put_long_direct(a,b)
# define put_word(a,b)			put_word_direct(a,b)
# define put_byte(a,b)			put_byte_direct(a,b)
# define get_real_address(a,b,c)	get_real_address_direct(a)
#endif

//extern uintptr MEMBaseDiff;
//extern uintptr VMEMBaseDiff;

//#define ARANYMVRAMSTART	0xf0000000
#define do_get_real_address_direct(a)		(((a) < 0xff000000) ? (((a) < ARANYMVRAMSTART) ? ((uae_u8 *)(a) + MEMBaseDiff) : ((uae_u8 *)(a) + VMEMBaseDiff)) : ((uae_u8 *)(a & 0x00ffffff) + MEMBaseDiff))

#define InitMEMBaseDiff(va, ra)		(MEMBaseDiff = (uintptr)(va) - (uintptr)(ra))
#define InitVMEMBaseDiff(va, ra)	(VMEMBaseDiff = (uintptr)(va) - (uintptr)(ra))

static __inline__ uae_u32 get_long_direct(uaecptr addr)
{
    if (((addr & 0xfff00000) == 0x00f00000) || ((addr & 0xfff00000) == 0xfff00000)) return HWget_l(addr & 0x00ffffff);
    uae_u32 * const m = (uae_u32 *)do_get_real_address_direct(addr);
    return do_get_mem_long(m);
}
static __inline__ uae_u32 get_word_direct(uaecptr addr)
{
    if (((addr & 0xfff00000) == 0x00f00000) || ((addr & 0xfff00000) == 0xfff00000)) return HWget_w(addr & 0x00ffffff);
    uae_u16 * const m = (uae_u16 *)do_get_real_address_direct(addr);
    return do_get_mem_word(m);
}
static __inline__ uae_u32 get_byte_direct(uaecptr addr)
{
    if (((addr & 0xfff00000) == 0x00f00000) || ((addr & 0xfff00000) == 0xfff00000)) return HWget_b(addr & 0x00ffffff);
    uae_u8 * const m = (uae_u8 *)do_get_real_address_direct(addr);
    return do_get_mem_byte(m);
}
static __inline__ void put_long_direct(uaecptr addr, uae_u32 l)
{
    if (((addr & 0xfff00000) == 0x00f00000) || ((addr & 0xfff00000) == 0xfff00000)) {
        HWput_l(addr & 0x00ffffff, l);
        return;
    } 
    uae_u32 * const m = (uae_u32 *)do_get_real_address_direct(addr);
    do_put_mem_long(m, l);
}
static __inline__ void put_word_direct(uaecptr addr, uae_u32 w)
{
    if (((addr & 0xfff00000) == 0x00f00000) || ((addr & 0xfff00000) == 0xfff00000)) {
        HWput_w(addr & 0x00ffffff, w);
        return;
    }
    uae_u16 * const m = (uae_u16 *)do_get_real_address_direct(addr);
    do_put_mem_word(m, w);
}
static __inline__ void put_byte_direct(uaecptr addr, uae_u32 b)
{
    if (((addr & 0xfff00000) == 0x00f00000) || ((addr & 0xfff00000) == 0xfff00000)) {
        HWput_b(addr & 0x00ffffff, b);
        return;
    }
    uae_u8 * const m = (uae_u8 *)do_get_real_address_direct(addr);
    do_put_mem_byte(m, b);
}
static __inline__ uae_u8 *get_real_address_direct(uaecptr addr)
{
	return do_get_real_address_direct(addr);
}

uaecptr mmu_decode_addr(uaecptr addr, bool data, bool write);

inline uae_u8 *do_get_real_address_mmu(uaecptr addr, bool data, bool write)
{
	return do_get_real_address_direct(regs.tce ? mmu_decode_addr(addr, data, write) : addr);
}

static __inline__ uae_u32 get_long_mmu(uaecptr addr, bool data)
{
    return get_long_direct(regs.tce ? mmu_decode_addr(addr, data, false) : addr);
}
static __inline__ uae_u32 get_word_mmu(uaecptr addr, bool data)
{
    return get_word_direct(regs.tce ? mmu_decode_addr(addr, data, false) : addr);
}
static __inline__ uae_u32 get_byte_mmu(uaecptr addr, bool data)
{
    return get_byte_direct(regs.tce ? mmu_decode_addr(addr, data, false) : addr);
}
static __inline__ void put_long_mmu(uaecptr addr, uae_u32 l)
{
    put_long_direct((regs.tce ? mmu_decode_addr(addr, true, true) : addr), l);
}
static __inline__ void put_word_mmu(uaecptr addr, uae_u32 w)
{
    put_word_direct((regs.tce ? mmu_decode_addr(addr, true, true) : addr), w);
}
static __inline__ void put_byte_mmu(uaecptr addr, uae_u32 b)
{
    put_byte_direct((regs.tce ? mmu_decode_addr(addr, true, true) : addr), b);
}
static __inline__ uae_u8 *get_real_address_mmu(uaecptr addr, bool data, bool write)
{
	return do_get_real_address_mmu(addr, data, write);
}

static __inline__ int valid_address(uaecptr addr, uae_u32 size)
{
    return 1;
}

#endif /* MEMORY_H */

