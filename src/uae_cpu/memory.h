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
#ifdef FixedSizeFastRAM
#define FastRAM_SIZE	(FixedSizeFastRAM * 1024 * 1024)
#else
#define FastRAM_SIZE	FastRAMSize
#endif

#define ARANYMVRAMSTART 0xf0000000UL
#define ARANYMVRAMSIZE	0x00100000	// should be a variable to protect VGA card offscreen memory

extern uintptr VMEMBaseDiff;

#if ARAM_PAGE_CHECK
extern uaecptr pc_page, read_page, write_page;
extern uae_u32 pc_offset, read_offset, write_offset;
# ifdef FULLMMU
#  define ARAM_PAGE_MASK 0xfff
# else
#  define ARAM_PAGE_MASK 0xfffff
# endif
#endif

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

/*
 * "size" is the size of the memory access (byte = 1, word = 2, long = 4)
 */
static __inline__ void check_ram_boundary(uaecptr addr, int size, bool write)
{
	if (addr <= (FastRAM_BEGIN + FastRAM_SIZE - size)) {
		if (!write)
			return;
		else {
			// first two longwords of ST-RAM are ROM
			if ((addr >= FastRAM_BEGIN) || (addr >= 8 && addr <= (STRAM_END - size)))
				return;
		}
	}

	if (addr >= ARANYMVRAMSTART && addr <= (ARANYMVRAMSTART + ARANYMVRAMSIZE - size))
		return;

	// printf("BUS ERROR %s at $%x\n", (write ? "writting" : "reading"), addr);
	BUS_ERROR;
}

static __inline__ uae_u32 get_long_direct(uaecptr addr)
{
#if ARAM_PAGE_CHECK
    if (((addr ^ read_page) <= ARAM_PAGE_MASK))
        return do_get_mem_long((uae_u32*)((uae_u8*)addr + read_offset));
#endif
    addr = addr < 0xff000000 ? addr : addr & 0x00ffffff;
    if ((addr & 0xfff00000) == 0x00f00000) return HWget_l(addr);
    check_ram_boundary(addr, 4, false);
    uae_u32 * const m = (uae_u32 *)do_get_real_address_direct(addr);
#if ARAM_PAGE_CHECK
    read_page = addr;
    read_offset = (uintptr)m - (uintptr)addr;
#endif
    return do_get_mem_long(m);
}

static __inline__ uae_u32 get_word_direct(uaecptr addr)
{
#if ARAM_PAGE_CHECK
    if (((addr ^ read_page) <= ARAM_PAGE_MASK))
        return do_get_mem_word((uae_u16*)((uae_u8*)addr + read_offset));
#endif
    addr = addr < 0xff000000 ? addr : addr & 0x00ffffff;
    if ((addr & 0xfff00000) == 0x00f00000) return HWget_w(addr);
    check_ram_boundary(addr, 2, false);
    uae_u16 * const m = (uae_u16 *)do_get_real_address_direct(addr);
#if ARAM_PAGE_CHECK
    read_page = addr;
    read_offset = (uintptr)m - (uintptr)addr;
#endif
    return do_get_mem_word(m);
}

static __inline__ uae_u32 get_byte_direct(uaecptr addr)
{
#if ARAM_PAGE_CHECK
    if (((addr ^ read_page) <= ARAM_PAGE_MASK))
        return do_get_mem_byte((uae_u32*)((uae_u8*)addr + read_offset));
#endif
    addr = addr < 0xff000000 ? addr : addr & 0x00ffffff;
    if ((addr & 0xfff00000) == 0x00f00000) return HWget_b(addr);
    check_ram_boundary(addr, 1, false);
    uae_u8 * const m = (uae_u8 *)do_get_real_address_direct(addr);
#if ARAM_PAGE_CHECK
    read_page = addr;
    read_offset = (uintptr)m - (uintptr)addr;
#endif
    return do_get_mem_byte(m);
}

static __inline__ void put_long_direct(uaecptr addr, uae_u32 l)
{
#if ARAM_PAGE_CHECK
    if (((addr ^ write_page) <= ARAM_PAGE_MASK)) {
        do_put_mem_long((uae_u32*)((uae_u8*)addr + write_offset), l);
        return;
    }
#endif
    addr = addr < 0xff000000 ? addr : addr & 0x00ffffff;
    if ((addr & 0xfff00000) == 0x00f00000) {
        HWput_l(addr, l);
        return;
    } 
    check_ram_boundary(addr, 4, true);
    uae_u32 * const m = (uae_u32 *)do_get_real_address_direct(addr);
#if ARAM_PAGE_CHECK
    write_page = addr;
    write_offset = (uintptr)m - (uintptr)addr;
#endif
    do_put_mem_long(m, l);
}

static __inline__ void put_word_direct(uaecptr addr, uae_u32 w)
{
#if ARAM_PAGE_CHECK
    if (((addr ^ write_page) <= ARAM_PAGE_MASK)) {
        do_put_mem_word((uae_u16*)((uae_u8*)addr + write_offset), w);
        return;
    }
#endif
    addr = addr < 0xff000000 ? addr : addr & 0x00ffffff;
    if ((addr & 0xfff00000) == 0x00f00000) {
        HWput_w(addr, w);
        return;
    }
    check_ram_boundary(addr, 2, true);
    uae_u16 * const m = (uae_u16 *)do_get_real_address_direct(addr);
#if ARAM_PAGE_CHECK
    write_page = addr;
    write_offset = (uintptr)m - (uintptr)addr;
#endif
    do_put_mem_word(m, w);
}

static __inline__ void put_byte_direct(uaecptr addr, uae_u32 b)
{
#if ARAM_PAGE_CHECK
    if (((addr ^ write_page) <= ARAM_PAGE_MASK)) {
        do_put_mem_byte((uae_u8*)((uae_u8*)addr + write_offset), b);
        return;
    }
#endif
    addr = addr < 0xff000000 ? addr : addr & 0x00ffffff;
    if ((addr & 0xfff00000) == 0x00f00000) {
        HWput_b(addr, b);
        return;
    }
    check_ram_boundary(addr, 1, true);
    uae_u8 * const m = (uae_u8 *)do_get_real_address_direct(addr);
#if ARAM_PAGE_CHECK
    write_page = addr;
    write_offset = (uintptr)m - (uintptr)addr;
#endif
    do_put_mem_byte(m, b);
}

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

