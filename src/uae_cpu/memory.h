/* 2001 MJ */

 /*
  * UAE - The Un*x Amiga Emulator
  *
  * memory management
  *
  * Copyright 1995 Bernd Schmidt
  */

#ifndef UAE_MEMORY_H
#define UAE_MEMORY_H

#include "hardware.h"
#include "parameters.h"
#include "exceptions.h"
#include "sysdeps.h"
#include "m68k.h"
#include "registers.h"

#define BUS_ERROR	longjmp(excep_env, 2)
#define STRAM_END	0x0e00000UL	// should be replaced by global ROMBase as soon as ROMBase will be a constant
#define ROM_END		0x0e80000UL	// should be replaced by ROMBase + RealROMSize if we are going to work with larger TOS ROMs than 512 kilobytes
#define FastRAM_BEGIN	0x1000000UL	// should be replaced by global FastRAMBase as soon as FastRAMBase will be a constant
extern uint32 FastRAMSize;

#define ARANYMVRAMSTART 0xf0000000UL
#define ARANYMVRAMSIZE	0x00100000	// should be a variable to protect VGA card offscreen memory

extern uintptr VMEMBaseDiff;

#if REAL_ADDRESSING
const uintptr MEMBaseDiff = 0;

# ifdef FULLMMU
#  define do_get_real_address(a,b,c)	do_get_real_address_mmu(a,b,c)
#  define get_long(a,b)			get_long_mmu(a,b)
#  define get_word(a,b)			get_word_mmu(a,b)
#  define get_byte(a,b)			get_byte_mmu(a,b)
#  define put_long(a,b)			put_long_mmu(a,b)
#  define put_word(a,b)			put_word_mmu(a,b)
#  define put_byte(a,b)			put_byte_mmu(a,b)
#  define get_real_address(a,b,c)	get_real_address_mmu(a,b,c)
# else
#  define do_get_real_address(a,b,c)		(((a) < ARANYMVRAMSTART) ? ((uae_u8 *)(a)) : ((uae_u8*)(a) + VMEMBaseDiff))
#  define get_long(a,b)			get_long_direct(a)
#  define get_word(a,b)			get_word_direct(a)
#  define get_byte(a,b)			get_byte_direct(a)
#  define put_long(a,b)			put_long_direct(a,b)
#  define put_word(a,b)			put_word_direct(a,b)
#  define put_byte(a,b)			put_byte_direct(a,b)
#  define get_real_address(a,b,c)	get_real_address_direct(a)
# endif /* FULLMMU */

# define do_get_real_address_direct(a)		(((a) < ARANYMVRAMSTART) ? ((uae_u8 *)(a)) : ((uae_u8 *)(a) + VMEMBaseDiff))

#else
extern uintptr MEMBaseDiff;

# ifdef FULLMMU
#  define do_get_real_address(a,b,c)	do_get_real_address_mmu(a,b,c)
#  define get_long(a,b)			get_long_mmu(a,b)
#  define get_word(a,b)			get_word_mmu(a,b)
#  define get_byte(a,b)			get_byte_mmu(a,b)
#  define put_long(a,b)			put_long_mmu(a,b)
#  define put_word(a,b)			put_word_mmu(a,b)
#  define put_byte(a,b)			put_byte_mmu(a,b)
#  define get_real_address(a,b,c)	get_real_address_mmu(a,b,c)
# else
#  define do_get_real_address(a,b,c)		(((a) < ARANYMVRAMSTART) ? ((uae_u8 *)(a) + MEMBaseDiff) : ((uae_u8*)(a) + VMEMBaseDiff))
#  define get_long(a,b)			get_long_direct(a)
#  define get_word(a,b)			get_word_direct(a)
#  define get_byte(a,b)			get_byte_direct(a)
#  define put_long(a,b)			put_long_direct(a,b)
#  define put_word(a,b)			put_word_direct(a,b)
#  define put_byte(a,b)			put_byte_direct(a,b)
#  define get_real_address(a,b,c)	get_real_address_direct(a)
# endif /* FULLMMU */

# define do_get_real_address_direct(a)		(((a) < ARANYMVRAMSTART) ? ((uae_u8 *)(a) + MEMBaseDiff) : ((uae_u8 *)(a) + VMEMBaseDiff))

# define InitMEMBaseDiff(va, ra)		(MEMBaseDiff = (uintptr)(va) - (uintptr)(ra))

#endif /* REAL_ADDRESSING */

#define InitVMEMBaseDiff(va, ra)	(VMEMBaseDiff = (uintptr)(va) - (uintptr)(ra))

#ifdef CHECK_BOUNDARY_BY_ARRAY

extern bool isRMemory[];
extern bool isWMemory[];

static __inline__ uae_u32 get_long_direct(uaecptr addr)
{
	int index = addr >> 20;
	if (isRMemory[index]) {
    	uae_u32 * const m = (uae_u32 *)do_get_real_address_direct(addr);
    	return do_get_mem_long(m);
	}
	else if (index == 15 || index == 4095)
		return HWget_l(addr & 0x00ffffff);
	else
		BUS_ERROR;
}

static __inline__ uae_u32 get_word_direct(uaecptr addr)
{
	int index = addr >> 20;
	if (isRMemory[index]) {
    	uae_u16 * const m = (uae_u16 *)do_get_real_address_direct(addr);
    	return do_get_mem_word(m);
    }
	else if (index == 15 || index == 4095)
		return HWget_w(addr & 0x00ffffff);
	else
		BUS_ERROR;
}

static __inline__ uae_u32 get_byte_direct(uaecptr addr)
{
	int index = addr >> 20;
	if (isRMemory[index]) {
	    uae_u8 * const m = (uae_u8 *)do_get_real_address_direct(addr);
    	return do_get_mem_byte(m);
    }
	else if (index == 15 || index == 4095)
		return HWget_b(addr & 0x00ffffff);
	else
		BUS_ERROR;
}

static __inline__ void put_long_direct(uaecptr addr, uae_u32 l)
{
	int index = addr >> 20;
	if (isWMemory[index]) {
    	uae_u32 * const m = (uae_u32 *)do_get_real_address_direct(addr);
    	do_put_mem_long(m, l);
	}
	else if (index == 15 || index == 4095)
        HWput_l(addr & 0x00ffffff, l);
	else
		BUS_ERROR;
}

static __inline__ void put_word_direct(uaecptr addr, uae_u32 w)
{
	int index = addr >> 20;
	if (isWMemory[index]) {
    	uae_u16 * const m = (uae_u16 *)do_get_real_address_direct(addr);
    	do_put_mem_word(m, w);
	}
	else if (index == 15 || index == 4095)
        HWput_w(addr & 0x00ffffff, w);
	else
		BUS_ERROR;
}

static __inline__ void put_byte_direct(uaecptr addr, uae_u32 b)
{
	int index = addr >> 20;
	if (isWMemory[index]) {
    	uae_u8 * const m = (uae_u8 *)do_get_real_address_direct(addr);
    	do_put_mem_byte(m, b);
	}
	else if (index == 15 || index == 4095)
        HWput_b(addr & 0x00ffffff, b);
	else
		BUS_ERROR;
}

#else

static __inline__ void check_ram_boundary(uaecptr addr, bool write = false)
{
	if (addr < (write ? STRAM_END : ROM_END))		// ST-RAM or ROM
		return;
	if (addr >= FastRAM_BEGIN && addr < (FastRAM_BEGIN+FastRAMSize))	// FastRAM
		return;
#ifdef DIRECT_TRUECOLOR
	if (bx_options.video.direct_truecolor) {		// VideoRAM
		if (addr >= ARANYMVRAMSTART && addr < (ARANYMVRAMSTART + ARANYMVRAMSIZE))
			return;
	}
#endif
	// printf("BUS ERROR %s at $%x\n", (write ? "writting" : "reading"), addr);
	BUS_ERROR;
}

static __inline__ uae_u32 get_long_direct(uaecptr addr)
{
    addr = addr < 0xff000000 ? addr : addr & 0x00ffffff;
    if ((addr & 0xfff00000) == 0x00f00000) return HWget_l(addr);
    check_ram_boundary(addr);
    uae_u32 * const m = (uae_u32 *)do_get_real_address_direct(addr);
    return do_get_mem_long(m);
}

static __inline__ uae_u32 get_word_direct(uaecptr addr)
{
    addr = addr < 0xff000000 ? addr : addr & 0x00ffffff;
    if ((addr & 0xfff00000) == 0x00f00000) return HWget_w(addr);
    check_ram_boundary(addr);
    uae_u16 * const m = (uae_u16 *)do_get_real_address_direct(addr);
    return do_get_mem_word(m);
}

static __inline__ uae_u32 get_byte_direct(uaecptr addr)
{
    addr = addr < 0xff000000 ? addr : addr & 0x00ffffff;
    if ((addr & 0xfff00000) == 0x00f00000) return HWget_b(addr);
    check_ram_boundary(addr);
    uae_u8 * const m = (uae_u8 *)do_get_real_address_direct(addr);
    return do_get_mem_byte(m);
}

static __inline__ void put_long_direct(uaecptr addr, uae_u32 l)
{
    addr = addr < 0xff000000 ? addr : addr & 0x00ffffff;
    if ((addr & 0xfff00000) == 0x00f00000) {
        HWput_l(addr, l);
        return;
    } 
    check_ram_boundary(addr, true);
    uae_u32 * const m = (uae_u32 *)do_get_real_address_direct(addr);
    do_put_mem_long(m, l);
}

static __inline__ void put_word_direct(uaecptr addr, uae_u32 w)
{
    addr = addr < 0xff000000 ? addr : addr & 0x00ffffff;
    if ((addr & 0xfff00000) == 0x00f00000) {
        HWput_w(addr, w);
        return;
    }
    check_ram_boundary(addr, true);
    uae_u16 * const m = (uae_u16 *)do_get_real_address_direct(addr);
    do_put_mem_word(m, w);
}

static __inline__ void put_byte_direct(uaecptr addr, uae_u32 b)
{
    addr = addr < 0xff000000 ? addr : addr & 0x00ffffff;
    if ((addr & 0xfff00000) == 0x00f00000) {
        HWput_b(addr, b);
        return;
    }
    check_ram_boundary(addr, true);
    uae_u8 * const m = (uae_u8 *)do_get_real_address_direct(addr);
    do_put_mem_byte(m, b);
}

#endif	/* CHECK_BOUNDARY_BY_ARRAY */

static __inline__ int valid_address(uaecptr addr, uae_u32 size)
{
    return 1;
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

#endif /* MEMORY_H */

