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
#include "cpummu.h"
#include "readcpu.h"

#include <csetjmp>

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
extern uintptr pc_offset, read_offset, write_offset;
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
static inline void check_ram_boundary(uaecptr addr, int size, bool write)
{
#ifndef NOCHECKBOUNDARY
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

	// D(bug("BUS ERROR %s at $%x\n", (write ? "writting" : "reading"), addr));
	regs.mmu_fault_addr = addr;
	longjmp(excep_env, 2);
#endif
}

#ifdef FIXED_VIDEORAM
# define do_get_real_address(a)		((uae_u8 *)(((uaecptr)(a) < ARANYMVRAMSTART) ? ((uaecptr)(a) + MEMBaseDiff) : ((uaecptr)(a) + VMEMBaseDiff)))
#else
# define do_get_real_address(a)		((uae_u8 *)((uintptr)(a) + MEMBaseDiff))
#endif

static inline uae_u8 *phys_get_real_address(uaecptr addr)
{
    return do_get_real_address(addr);
}

static inline bool phys_valid_address(uaecptr addr, bool write, uaecptr pc, int sz)
{
    jmp_buf excep_env_old;
    memcpy(excep_env_old, excep_env, sizeof(jmp_buf));
    int prb = setjmp(excep_env);
    if (prb != 0) {
        memcpy(excep_env, excep_env_old, sizeof(jmp_buf));
        return false;
    }
    check_ram_boundary(addr, sz, write);
    memcpy(excep_env, excep_env_old, sizeof(jmp_buf));
    return true;
}


static inline uae_u32 phys_get_long(uaecptr addr)
{
#if ARAM_PAGE_CHECK
    if (((addr ^ read_page) <= ARAM_PAGE_MASK))
        return do_get_mem_long((uae_u32*)(addr + read_offset));
#endif
#ifndef HW_SIGSEGV
    addr = addr < 0xff000000 ? addr : addr & 0x00ffffff;
    if ((addr & 0xfff00000) == 0x00f00000) return HWget_l(addr);
#endif
    check_ram_boundary(addr, 4, false);
    uae_u32 * const m = (uae_u32 *)phys_get_real_address(addr);
#if ARAM_PAGE_CHECK
    read_page = addr;
    read_offset = (uintptr)m - (uintptr)addr;
#endif
    return do_get_mem_long(m);
}

static inline uae_u32 phys_get_word(uaecptr addr)
{
#if ARAM_PAGE_CHECK
    if (((addr ^ read_page) <= ARAM_PAGE_MASK))
        return do_get_mem_word((uae_u16*)(addr + read_offset));
#endif
#ifndef HW_SIGSEGV
    addr = addr < 0xff000000 ? addr : addr & 0x00ffffff;
    if ((addr & 0xfff00000) == 0x00f00000) return HWget_w(addr);
#endif
    check_ram_boundary(addr, 2, false);
    uae_u16 * const m = (uae_u16 *)phys_get_real_address(addr);
#if ARAM_PAGE_CHECK
    read_page = addr;
    read_offset = (uintptr)m - (uintptr)addr;
#endif
    return do_get_mem_word(m);
}

static inline uae_u32 phys_get_byte(uaecptr addr)
{
#if ARAM_PAGE_CHECK
    if (((addr ^ read_page) <= ARAM_PAGE_MASK))
        return do_get_mem_byte((uae_u8*)(addr + read_offset));
#endif
#ifndef HW_SIGSEGV
    addr = addr < 0xff000000 ? addr : addr & 0x00ffffff;
    if ((addr & 0xfff00000) == 0x00f00000) return HWget_b(addr);
#endif
    check_ram_boundary(addr, 1, false);
    uae_u8 * const m = (uae_u8 *)phys_get_real_address(addr);
#if ARAM_PAGE_CHECK
    read_page = addr;
    read_offset = (uintptr)m - (uintptr)addr;
#endif
    return do_get_mem_byte(m);
}

static inline void phys_put_long(uaecptr addr, uae_u32 l)
{
#if ARAM_PAGE_CHECK
    if (((addr ^ write_page) <= ARAM_PAGE_MASK)) {
        do_put_mem_long((uae_u32*)(addr + write_offset), l);
        return;
    }
#endif
#ifndef HW_SIGSEGV
    addr = addr < 0xff000000 ? addr : addr & 0x00ffffff;
    if ((addr & 0xfff00000) == 0x00f00000) {
        HWput_l(addr, l);
        return;
    } 
#endif
    check_ram_boundary(addr, 4, true);
    uae_u32 * const m = (uae_u32 *)phys_get_real_address(addr);
#if ARAM_PAGE_CHECK
    write_page = addr;
    write_offset = (uintptr)m - (uintptr)addr;
#endif
    do_put_mem_long(m, l);
}

static inline void phys_put_word(uaecptr addr, uae_u32 w)
{
#if ARAM_PAGE_CHECK
    if (((addr ^ write_page) <= ARAM_PAGE_MASK)) {
        do_put_mem_word((uae_u16*)(addr + write_offset), w);
        return;
    }
#endif
#ifndef HW_SIGSEGV
    addr = addr < 0xff000000 ? addr : addr & 0x00ffffff;
    if ((addr & 0xfff00000) == 0x00f00000) {
        HWput_w(addr, w);
        return;
    }
#endif
    check_ram_boundary(addr, 2, true);
    uae_u16 * const m = (uae_u16 *)phys_get_real_address(addr);
#if ARAM_PAGE_CHECK
    write_page = addr;
    write_offset = (uintptr)m - (uintptr)addr;
#endif
    do_put_mem_word(m, w);
}

static inline void phys_put_byte(uaecptr addr, uae_u32 b)
{
#if ARAM_PAGE_CHECK
    if (((addr ^ write_page) <= ARAM_PAGE_MASK)) {
        do_put_mem_byte((uae_u8*)(addr + write_offset), b);
        return;
    }
#endif
#ifndef HW_SIGSEGV
    addr = addr < 0xff000000 ? addr : addr & 0x00ffffff;
    if ((addr & 0xfff00000) == 0x00f00000) {
        HWput_b(addr, b);
        return;
    }
#endif
    check_ram_boundary(addr, 1, true);
    uae_u8 * const m = (uae_u8 *)phys_get_real_address(addr);
#if ARAM_PAGE_CHECK
    write_page = addr;
    write_offset = (uintptr)m - (uintptr)addr;
#endif
    do_put_mem_byte(m, b);
}

#ifdef FULLMMU
static inline uae_u32 get_long(uaecptr addr)
{
    jmp_buf excep_env_old;
    excep_env_old = excep_env;
    uae_u32 l;
    int prb = setjmp(excep_env);
    if (prb != 0) {
	excep_env = excep_env_old;
	regs.mmu_fault_addr = addr;
	longjmp(excep_env, prb);
    }
    l = phys_get_long(mmu_translate(addr, FC_DATA, 0, m68k_getpc(), sz_long, 0));
    excep_env = excep_env_old;
    return l;
}

static inline uae_u16 get_word(uaecptr addr)
{
    jmp_buf excep_env_old;
    excep_env_old = excep_env;
    uae_u16 w;
    int prb = setjmp(excep_env);
    if (prb != 0) {
	excep_env = excep_env_old;
	regs.mmu_fault_addr = addr;
	longjmp(excep_env, prb);
    }
    w = phys_get_word(mmu_translate(addr, FC_DATA, 0, m68k_getpc(), sz_word, 0));
    excep_env = excep_env_old;
    return w;
}

static inline uae_u8 get_byte(uaecptr addr)
{
    jmp_buf excep_env_old;
    excep_env_old = excep_env;
    uae_u8 b;
    int prb = setjmp(excep_env);
    if (prb != 0) {
	excep_env = excep_env_old;
	regs.mmu_fault_addr = addr;
	longjmp(excep_env, prb);
    }
    b = phys_get_byte(mmu_translate(addr, FC_DATA, 0, m68k_getpc(), sz_byte, 0));
    excep_env = excep_env_old;
    return b;
}

static inline void put_long(uaecptr addr, uae_u32 l)
{
    jmp_buf excep_env_old;
    excep_env_old = excep_env;
    int prb = setjmp(excep_env);
    if (prb != 0) {
	excep_env = excep_env_old;
	regs.mmu_fault_addr = addr;
	longjmp(excep_env, prb);
    }
    phys_put_long(mmu_translate(addr, FC_DATA, 1, m68k_getpc(), sz_long, 0),l);
    excep_env = excep_env_old;
}

static inline void put_word(uaecptr addr, uae_u16 w)
{
    jmp_buf excep_env_old;
    excep_env_old = excep_env;
    int prb = setjmp(excep_env);
    if (prb != 0) {
	excep_env = excep_env_old;
	regs.mmu_fault_addr = addr;
	longjmp(excep_env, prb);
    }
    phys_put_word(mmu_translate(addr, FC_DATA, 1, m68k_getpc(), sz_word, 0),w);
    excep_env = excep_env_old;
}

static inline void put_byte(uaecptr addr, uae_u16 b)
{
    jmp_buf excep_env_old;
    excep_env_old = excep_env;
    int prb = setjmp(excep_env);
    if (prb != 0) {
	excep_env = excep_env_old;
	regs.mmu_fault_addr = addr;
	longjmp(excep_env, prb);
    }
    phys_put_byte(mmu_translate(addr, FC_DATA, 1, m68k_getpc(), sz_byte, 0),b);
    excep_env = excep_env_old;
}

static inline uae_u8 *get_real_address(uaecptr addr, int write, int sz)
{
    wordsizes i = sz_long;
    switch (sz) {
        case 1: i = sz_byte; break;
        case 2: i = sz_word; break;
    }
    return phys_get_real_address(mmu_translate(addr, FC_DATA, write, m68k_getpc(), i, 0));
}

static inline uae_u32 sfc_get_long(uaecptr addr)
{
    jmp_buf excep_env_old;
    excep_env_old = excep_env;
    uae_u32 l;
    int prb = setjmp(excep_env);
    if (prb != 0) {
	excep_env = excep_env_old;
	regs.mmu_fault_addr = addr;
	longjmp(excep_env, prb);
    }
    l = phys_get_long(mmu_translate(addr, regs.sfc, 0, m68k_getpc(), sz_long, 0));
    excep_env = excep_env_old;
    return l;
}
static inline uae_u16 sfc_get_word(uaecptr addr)
{
    jmp_buf excep_env_old;
    excep_env_old = excep_env;
    uae_u16 w;
    int prb = setjmp(excep_env);
    if (prb != 0) {
	excep_env = excep_env_old;
	regs.mmu_fault_addr = addr;
	longjmp(excep_env, prb);
    }
    w = phys_get_word(mmu_translate(addr, regs.sfc, 0, m68k_getpc(), sz_word, 0));
    excep_env = excep_env_old;
    return w;
}
static inline uae_u8 sfc_get_byte(uaecptr addr)
{
    jmp_buf excep_env_old;
    excep_env_old = excep_env;
    uae_u8 b;
    int prb = setjmp(excep_env);
    if (prb != 0) {
	excep_env = excep_env_old;
	regs.mmu_fault_addr = addr;
	longjmp(excep_env, prb);
    }
    b = phys_get_byte(mmu_translate(addr, regs.sfc, 0, m68k_getpc(), sz_byte, 0));
    excep_env = excep_env_old;
    return b;
}

static inline void dfc_put_long(uaecptr addr, uae_u32 l)
{
    jmp_buf excep_env_old;
    excep_env_old = excep_env;
    int prb = setjmp(excep_env);
    if (prb != 0) {
	excep_env = excep_env_old;
	regs.mmu_fault_addr = addr;
	longjmp(excep_env, prb);
    }
    phys_put_long(mmu_translate(addr, regs.dfc, 1, m68k_getpc(), sz_long, 0), l);
    excep_env = excep_env_old;
}
static inline void dfc_put_word(uaecptr addr, uae_u16 w)
{
    jmp_buf excep_env_old;
    excep_env_old = excep_env;
    int prb = setjmp(excep_env);
    if (prb != 0) {
	excep_env = excep_env_old;
	regs.mmu_fault_addr = addr;
	longjmp(excep_env, prb);
    }
    phys_put_word(mmu_translate(addr, regs.dfc, 1, m68k_getpc(), sz_word, 0), w);
    excep_env = excep_env_old;
}
static inline void dfc_put_byte(uaecptr addr, uae_u16 b)
{
    jmp_buf excep_env_old;
    excep_env_old = excep_env;
    int prb = setjmp(excep_env);
    if (prb != 0) {
	excep_env = excep_env_old;
	regs.mmu_fault_addr = addr;
	longjmp(excep_env, prb);
    }
    phys_put_byte(mmu_translate(addr, regs.dfc, 1, m68k_getpc(), sz_byte, 0), b);
    excep_env = excep_env_old;
}

static inline bool valid_address(uaecptr addr, bool write, uaecptr pc, int sz)
{
    jmp_buf excep_env_old;
    excep_env_old = excep_env;
    int prb = setjmp(excep_env);
    wordsizes i = sz_long;
    switch (sz) {
        case 1: i = sz_byte; break;
        case 2: i = sz_word; break;
    }
    if (prb != 0) {
	excep_env = excep_env_old;
	return false;
    } 
    check_ram_boundary(mmu_translate(addr, FC_DATA, (write ? 1 : 0), pc, i, 0), sz, write);
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
#  define get_real_address(a,w,s)	phys_get_real_address(a)

#define valid_address(a,w,p,s)		phys_valid_address(a,w,p,s)
#endif

static void inline flush_internals() {
#if ARAM_PAGE_CHECK
    pc_page = 0xeeeeeeee;
    read_page = 0xeeeeeeee;
    write_page = 0xeeeeeeee;
#endif
}

#endif /* MEMORY_H */
