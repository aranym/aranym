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

#include "sysdeps.h"
#include "hardware.h"
#include "parameters.h"
#include "registers.h"
//#include "cpummu.h"
#include "readcpu.h"

// newcpu.h
extern void Exception (int, uaecptr);
extern jmp_buf excep_env;
extern int in_exception_2;

#define STRAM_END	0x0e00000UL	// should be replaced by global ROMBase as soon as ROMBase will be a constant
#define ROM_END		0x0e80000UL	// should be replaced by ROMBase + RealROMSize if we are going to work with larger TOS ROMs than 512 kilobytes
#define FastRAM_BEGIN	0x1000000UL	// should be replaced by global FastRAMBase as soon as FastRAMBase will be a constant
#ifdef FixedSizeFastRAM
#define FastRAM_SIZE	(FixedSizeFastRAM * 1024 * 1024)
#else
#define FastRAM_SIZE	FastRAMSize
#endif

#ifdef FIXED_VIDEORAM
#define ARANYMVRAMSTART 0xf0000000UL
#endif

#define ARANYMVRAMSIZE	0x00100000	// should be a variable to protect VGA card offscreen memory

#ifdef FIXED_VIDEORAM
extern uintptr VMEMBaseDiff;
#else
extern uae_u32 VideoRAMBase;
#endif

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
#else
extern uintptr MEMBaseDiff;
# define InitMEMBaseDiff(va, ra)	(MEMBaseDiff = (uintptr)(va) - (uintptr)(ra))
#endif /* REAL_ADDRESSING */

#ifdef FIXED_VIDEORAM
#define InitVMEMBaseDiff(va, ra)	(VMEMBaseDiff = (uintptr)(va) - (uintptr)(ra))
#else
#define InitVMEMBaseDiff(va, ra)        (ra = (uintptr)(va) + MEMBaseDiff)
#endif

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
#ifdef FIXED_VIDEORAM
	if (addr >= ARANYMVRAMSTART && addr <= (ARANYMVRAMSTART + ARANYMVRAMSIZE - size))
#else
	if (addr >= VideoRAMBase && addr <= (VideoRAMBase + ARANYMVRAMSIZE - size))
#endif
		return;

	// printf("BUS ERROR %s at $%x\n", (write ? "writting" : "reading"), addr);
//	regs.mmu_fault_addr = addr;
//	Exception(2, regs.pcp);
	longjmp(excep_env, 2);
}

#ifdef FIXED_VIDEORAM
# define do_get_real_address(a)		(((a) < ARANYMVRAMSTART) ? ((uae_u8 *)(a) + MEMBaseDiff) : ((uae_u8 *)(a) + VMEMBaseDiff))
#else
# define do_get_real_address(a)          ((uae_u8 *)(a))
#endif

static __inline__ uae_u8 *phys_get_real_address(uaecptr addr)
{
    return do_get_real_address(addr);
}

static __inline__ uae_u32 phys_get_long(uaecptr addr)
{
#if ARAM_PAGE_CHECK
    if (((addr ^ read_page) <= ARAM_PAGE_MASK))
        return do_get_mem_long((uae_u32*)((uae_u8*)addr + read_offset));
#endif
    addr = addr < 0xff000000 ? addr : addr & 0x00ffffff;
    if ((addr & 0xfff00000) == 0x00f00000) return HWget_l(addr);
    check_ram_boundary(addr, 4, false);
    uae_u32 * const m = (uae_u32 *)phys_get_real_address(addr);
#if ARAM_PAGE_CHECK
    read_page = addr;
    read_offset = (uintptr)m - (uintptr)addr;
#endif
    return do_get_mem_long(m);
}

static __inline__ uae_u32 phys_get_word(uaecptr addr)
{
#if ARAM_PAGE_CHECK
    if (((addr ^ read_page) <= ARAM_PAGE_MASK))
        return do_get_mem_word((uae_u16*)((uae_u8*)addr + read_offset));
#endif
    addr = addr < 0xff000000 ? addr : addr & 0x00ffffff;
    if ((addr & 0xfff00000) == 0x00f00000) return HWget_w(addr);
    check_ram_boundary(addr, 2, false);
    uae_u16 * const m = (uae_u16 *)phys_get_real_address(addr);
#if ARAM_PAGE_CHECK
    read_page = addr;
    read_offset = (uintptr)m - (uintptr)addr;
#endif
    return do_get_mem_word(m);
}

static __inline__ uae_u32 phys_get_byte(uaecptr addr)
{
#if ARAM_PAGE_CHECK
    if (((addr ^ read_page) <= ARAM_PAGE_MASK))
        return do_get_mem_byte((uae_u32*)((uae_u8*)addr + read_offset));
#endif
    addr = addr < 0xff000000 ? addr : addr & 0x00ffffff;
    if ((addr & 0xfff00000) == 0x00f00000) return HWget_b(addr);
    check_ram_boundary(addr, 1, false);
    uae_u8 * const m = (uae_u8 *)phys_get_real_address(addr);
#if ARAM_PAGE_CHECK
    read_page = addr;
    read_offset = (uintptr)m - (uintptr)addr;
#endif
    return do_get_mem_byte(m);
}

static __inline__ void phys_put_long(uaecptr addr, uae_u32 l)
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
    uae_u32 * const m = (uae_u32 *)phys_get_real_address(addr);
#if ARAM_PAGE_CHECK
    write_page = addr;
    write_offset = (uintptr)m - (uintptr)addr;
#endif
    do_put_mem_long(m, l);
}

static __inline__ void phys_put_word(uaecptr addr, uae_u32 w)
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
    uae_u16 * const m = (uae_u16 *)phys_get_real_address(addr);
#if ARAM_PAGE_CHECK
    write_page = addr;
    write_offset = (uintptr)m - (uintptr)addr;
#endif
    do_put_mem_word(m, w);
}

static __inline__ void phys_put_byte(uaecptr addr, uae_u32 b)
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
    uae_u8 * const m = (uae_u8 *)phys_get_real_address(addr);
#if ARAM_PAGE_CHECK
    write_page = addr;
    write_offset = (uintptr)m - (uintptr)addr;
#endif
    do_put_mem_byte(m, b);
}

#ifdef FULLMMU
static __inline__ uae_u32 get_long(uaecptr addr)
{
    return phys_get_long(mmu_translate(addr, FC_DATA, 0, m68k_getpc(), sz_long, 0));
}

static __inline__ uae_u16 get_word(uaecptr addr)
{
    return phys_get_word(mmu_translate(addr, FC_DATA, 0, m68k_getpc(), sz_word, 0));
}

static __inline__ uae_u8 get_byte(uaecptr addr)
{
    return phys_get_byte(mmu_translate(addr, FC_DATA, 0, m68k_getpc(), sz_byte, 0));
}

static __inline__ void put_long(uaecptr addr, uae_u32 l)
{
    phys_put_long(mmu_translate(addr, FC_DATA, 1, m68k_getpc(), sz_long, 0),l);
}

static __inline__ void put_word(uaecptr addr, uae_u16 w)
{
    phys_put_word(mmu_translate(addr, FC_DATA, 1, m68k_getpc(), sz_word, 0),w);
}

static __inline__ void put_byte(uaecptr addr, uae_u16 b)
{
    phys_put_byte(mmu_translate(addr, FC_DATA, 1, m68k_getpc(), sz_byte, 0),b);
}

static __inline__ uae_u8 *get_real_address(uaecptr addr, int write, uaecptr pc, wordsizes sz)
{
    return phys_get_real_address(mmu_translate(addr, FC_DATA, write, pc, sz, 0));
}

static __inline__ uae_u32 sfc_get_long(uaecptr addr)
{
    return phys_get_long(mmu_translate(addr, regs.sfc, 0, m68k_getpc(), sz_long, 0));
}
static __inline__ uae_u16 sfc_get_word(uaecptr addr)
{
    return phys_get_word(mmu_translate(addr, regs.sfc, 0, m68k_getpc(), sz_word, 0));
}
static __inline__ uae_u8 sfc_get_byte(uaecptr addr)
{
    return phys_get_byte(mmu_translate(addr, regs.sfc, 0, m68k_getpc(), sz_byte, 0));
}

static __inline__ void dfc_put_long(uaecptr addr, uae_u32 l)
{
    phys_put_long(mmu_translate(addr, regs.dfc, 1, m68k_getpc(), sz_long, 0), l);
}
static __inline__ void dfc_put_word(uaecptr addr, uae_u16 w)
{
    phys_put_word(mmu_translate(addr, regs.dfc, 1, m68k_getpc(), sz_word, 0), w);
}
static __inline__ void dfc_put_byte(uaecptr addr, uae_u16 b)
{
    phys_put_byte(mmu_translate(addr, regs.dfc, 1, m68k_getpc(), sz_byte, 0), b);
}

static __inline__ bool valid_address(uaecptr addr, int write, uaecptr pc, wordsizes sz)
{
    jmp_buf excep_env_old;
    excep_env_old = excep_env;
    int prb = setjmp(excep_env);
    if (prb != 0) {
	excep_env = excep_env_old;
	return false;
    } 
    check_ram_boundary(mmu_translate(addr, FC_DATA, write, pc, sz, 0), ((sz == sz_byte) ?  1 : ((sz == sz_word) ? 2 : 4)), (write == 0 ? false : true));
    excep_env = excep_env_old;
    return true;
}

#else

#  define get_long(a)			phys_get_long(a)
#  define get_word(a)			phys_get_word(a)
#  define get_byte(a)			phys_get_byte(a)
#  define put_long(a,b)			phys_put_long(a,b)
#  define put_word(a,b)			phys_put_word(a,b)
#  define put_byte(a,b)			phys_put_byte(a,b)
#  define get_real_address(a,w,p,s)	phys_get_real_address(a)

static __inline__ bool valid_address(uaecptr addr, int write, uaecptr pc, wordsizes sz)
{
    jmp_buf excep_env_old;
    excep_env_old = excep_env;
    int prb = setjmp(excep_env);
    if (prb != 0) {
        excep_env = excep_env_old;
        return false;
    }
    check_ram_boundary(addr, ((sz == sz_byte) ?  1 : ((sz == sz_word) ? 2 : 4)), (write == 0 ? false : true));
    excep_env = excep_env_old;
    return true;
}

#endif

#endif /* MEMORY_H */

