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
#include "exceptions.h"
#include "sysdeps.h"
#include "m68k.h"
#include "registers.h"

#define CHECK_RAM_END	0
#if CHECK_RAM_END
#define READ_RAM_END		0x1000000
#define WRITE_RAM_END		0x0e00000
#define BUS_ERROR	longjmp(excep_env, 2)
#endif

#define ARANYMVRAMSTART 0xf0000000

#if !DIRECT_ADDRESSING && !REAL_ADDRESSING

/* Enabling this adds one additional native memory reference per 68k memory
 * access, but saves one shift (on the x86). Enabling this is probably
 * better for the cache. My favourite benchmark (PP2) doesn't show a
 * difference, so I leave this enabled. */

#if 1 || defined SAVE_MEMORY
#define SAVE_MEMORY_BANKS
#endif

typedef uae_u32 (REGPARAM2 *mem_get_func)(uaecptr) REGPARAM;
typedef void (REGPARAM2 *mem_put_func)(uaecptr, uae_u32) REGPARAM;
typedef uae_u8 *(REGPARAM2 *xlate_func)(uaecptr) REGPARAM;
typedef int (REGPARAM2 *check_func)(uaecptr, uae_u32) REGPARAM;

#undef DIRECT_MEMFUNCS_SUCCESSFUL

#ifndef CAN_MAP_MEMORY
#undef USE_COMPILER
#endif

#if defined(USE_COMPILER) && !defined(USE_MAPPED_MEMORY)
#define USE_MAPPED_MEMORY
#endif

typedef struct {
    /* These ones should be self-explanatory... */
    mem_get_func lget, wget, bget;
    mem_put_func lput, wput, bput;
    /* Use xlateaddr to translate an Amiga address to a uae_u8 * that can
     * be used to address memory without calling the wget/wput functions.
     * This doesn't work for all memory banks, so this function may call
     * abort(). */
    xlate_func xlateaddr;
    /* To prevent calls to abort(), use check before calling xlateaddr.
     * It checks not only that the memory bank can do xlateaddr, but also
     * that the pointer points to an area of at least the specified size.
     * This is used for example to translate bitplane pointers in custom.c */
    check_func check;
} addrbank;

extern uae_u8 filesysory[65536];

extern addrbank ram_bank;	// Mac RAM
extern addrbank rom_bank;	// Mac ROM
extern addrbank frame_bank;	// Frame buffer

/* Default memory access functions */

extern int REGPARAM2 default_check(uaecptr addr, uae_u32 size) REGPARAM;
extern uae_u8 *REGPARAM2 default_xlate(uaecptr addr) REGPARAM;

#define bankindex(addr) (((uaecptr)(addr)) >> 16)

#ifdef SAVE_MEMORY_BANKS
extern addrbank *mem_banks[65536];
#define get_mem_bank(addr) (*mem_banks[bankindex(addr)])
#define put_mem_bank(addr, b) (mem_banks[bankindex(addr)] = (b))
#else
extern addrbank mem_banks[65536];
#define get_mem_bank(addr) (mem_banks[bankindex(addr)])
#define put_mem_bank(addr, b) (mem_banks[bankindex(addr)] = *(b))
#endif

extern void memory_init(void);
extern void map_banks(addrbank *bank, int first, int count);

#ifndef NO_INLINE_MEMORY_ACCESS

#define longget(addr) (call_mem_get_func(get_mem_bank(addr).lget, addr))
#define wordget(addr) (call_mem_get_func(get_mem_bank(addr).wget, addr))
#define byteget(addr) (call_mem_get_func(get_mem_bank(addr).bget, addr))
#define longput(addr,l) (call_mem_put_func(get_mem_bank(addr).lput, addr, l))
#define wordput(addr,w) (call_mem_put_func(get_mem_bank(addr).wput, addr, w))
#define byteput(addr,b) (call_mem_put_func(get_mem_bank(addr).bput, addr, b))

#else

extern uae_u32 alongget(uaecptr addr);
extern uae_u32 awordget(uaecptr addr);
extern uae_u32 longget(uaecptr addr);
extern uae_u32 wordget(uaecptr addr);
extern uae_u32 byteget(uaecptr addr);
extern void longput(uaecptr addr, uae_u32 l);
extern void wordput(uaecptr addr, uae_u32 w);
extern void byteput(uaecptr addr, uae_u32 b);

#endif

#ifndef MD_HAVE_MEM_1_FUNCS

#define longget_1 longget
#define wordget_1 wordget
#define byteget_1 byteget
#define longput_1 longput
#define wordput_1 wordput
#define byteput_1 byteput

#endif

#endif /* !DIRECT_ADDRESSING && !REAL_ADDRESSING */

#if REAL_ADDRESSING
const uintptr MEMBaseDiff = 0;
#endif

#if DIRECT_ADDRESSING
extern uintptr MEMBaseDiff;
#endif

#if REAL_ADDRESSING || DIRECT_ADDRESSING
extern uintptr VMEMBaseDiff;
#ifdef FULLMMU
# define do_get_real_address(a,b,c)	do_get_real_address_mmu(a,b,c)
# define get_long(a,b)			get_long_mmu(a,b)
# define get_word(a,b)			get_word_mmu(a,b)
# define get_byte(a,b)			get_byte_mmu(a,b)
# define put_long(a,b)			put_long_mmu(a,b)
# define put_word(a,b)			put_word_mmu(a,b)
# define put_byte(a,b)			put_byte_mmu(a,b)
# define get_real_address(a,b,c)	get_real_address_mmu(a,b,c)
#else
# define do_get_real_address(a,b,c)		(((a) < ARANYMVRAMSTART) ? ((uae_u8 *)(a) + MEMBaseDiff) : ((uae_u8*)(a) + VMEMBaseDiff))
# define get_long(a,b)			get_long_direct(a)
# define get_word(a,b)			get_word_direct(a)
# define get_byte(a,b)			get_byte_direct(a)
# define put_long(a,b)			put_long_direct(a,b)
# define put_word(a,b)			put_word_direct(a,b)
# define put_byte(a,b)			put_byte_direct(a,b)
# define get_real_address(a,b,c)	get_real_address_direct(a)
#endif /* FULLMMU */

#define do_get_real_address_direct(a)		(((a) < ARANYMVRAMSTART) ? ((uae_u8 *)(a) + MEMBaseDiff) : ((uae_u8 *)(a) + VMEMBaseDiff))

#define InitMEMBaseDiff(va, ra)		(MEMBaseDiff = (uintptr)(va) - (uintptr)(ra))
#define InitVMEMBaseDiff(va, ra)	(VMEMBaseDiff = (uintptr)(va) - (uintptr)(ra))

static __inline__ uae_u32 get_long_direct(uaecptr addr)
{
    addr = addr < 0xff000000 ? addr : addr & 0x00ffffff;
    if ((addr & 0xfff00000) == 0x00f00000) return HWget_l(addr);
#if CHECK_RAM_END
    if (addr >= READ_RAM_END)
    	BUS_ERROR;
#endif
    uae_u32 * const m = (uae_u32 *)do_get_real_address_direct(addr);
    return do_get_mem_long(m);
}
static __inline__ uae_u32 get_word_direct(uaecptr addr)
{
    addr = addr < 0xff000000 ? addr : addr & 0x00ffffff;
    if ((addr & 0xfff00000) == 0x00f00000) return HWget_w(addr);
#if CHECK_RAM_END
    if (addr >= READ_RAM_END)
    	BUS_ERROR;
#endif
    uae_u16 * const m = (uae_u16 *)do_get_real_address_direct(addr);
    return do_get_mem_word(m);
}
static __inline__ uae_u32 get_byte_direct(uaecptr addr)
{
    addr = addr < 0xff000000 ? addr : addr & 0x00ffffff;
    if ((addr & 0xfff00000) == 0x00f00000) return HWget_b(addr);
#if CHECK_RAM_END
    if (addr >= READ_RAM_END)
    	BUS_ERROR;
#endif
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
#if CHECK_RAM_END
    if (addr >= WRITE_RAM_END)
    	BUS_ERROR;
#endif
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
#if CHECK_RAM_END
    if (addr >= WRITE_RAM_END)
    	BUS_ERROR;
#endif
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
#if CHECK_RAM_END
    if (addr >= WRITE_RAM_END)
    	BUS_ERROR;
#endif
    uae_u8 * const m = (uae_u8 *)do_get_real_address_direct(addr);
    do_put_mem_byte(m, b);
}
static __inline__ int valid_address(uaecptr addr, uae_u32 size)
{
    return 1;
}

#else

static __inline__ uae_u32 get_long_direct(uaecptr addr)
{
    addr = addr < 0xff000000 ? addr : addr & 0x00ffffff;
    if ((addr & 0xfff00000) == 0x00f00000) return HWget_l(addr);
#if CHECK_RAM_END
    if (addr >= READ_RAM_END)
    	BUS_ERROR;
#endif
    uae_u32 * const m = (uae_u32 *)do_get_real_address_direct(addr);
    return longget_1(addr);
}
static __inline__ uae_u32 get_word_direct(uaecptr addr)
{
    addr = addr < 0xff000000 ? addr : addr & 0x00ffffff;
    if ((addr & 0xfff00000) == 0x00f00000) return HWget_w(addr);
#if CHECK_RAM_END
    if (addr >= READ_RAM_END)
    	BUS_ERROR;
#endif
    uae_u16 * const m = (uae_u16 *)do_get_real_address_direct(addr);
    return wordget_1(addr);
}
static __inline__ uae_u32 get_byte_direct(uaecptr addr)
{
    addr = addr < 0xff000000 ? addr : addr & 0x00ffffff;
    if ((addr & 0xfff00000) == 0x00f00000) return HWget_b(addr);
#if CHECK_RAM_END
    if (addr >= READ_RAM_END)
    	BUS_ERROR;
#endif
    uae_u8 * const m = (uae_u8 *)do_get_real_address_direct(addr);
    return byteget_1(addr);
}
static __inline__ void put_long_direct(uaecptr addr, uae_u32 l)
{
    addr = addr < 0xff000000 ? addr : addr & 0x00ffffff;
    if ((addr & 0xfff00000) == 0x00f00000) {
        HWput_l(addr, l);
        return;
    } 
#if CHECK_RAM_END
    if (addr >= WRITE_RAM_END)
    	BUS_ERROR;
#endif
    uae_u32 * const m = (uae_u32 *)do_get_real_address_direct(addr);
    longput_1(addr, l);
}
static __inline__ void put_word_direct(uaecptr addr, uae_u32 w)
{
    addr = addr < 0xff000000 ? addr : addr & 0x00ffffff;
    if ((addr & 0xfff00000) == 0x00f00000) {
        HWput_w(addr, w);
        return;
    }
#if CHECK_RAM_END
    if (addr >= WRITE_RAM_END)
    	BUS_ERROR;
#endif
    uae_u16 * const m = (uae_u16 *)do_get_real_address_direct(addr);
    wordput_1(addr, w);
}
static __inline__ void put_byte_direct(uaecptr addr, uae_u32 b)
{
    addr = addr < 0xff000000 ? addr : addr & 0x00ffffff;
    if ((addr & 0xfff00000) == 0x00f00000) {
        HWput_b(addr, b);
        return;
    }
#if CHECK_RAM_END
    if (addr >= WRITE_RAM_END)
    	BUS_ERROR;
#endif
    uae_u8 * const m = (uae_u8 *)do_get_real_address_direct(addr);
    byteput_1(addr, b);
}
static __inline__ int valid_address(uaecptr addr, uae_u32 size)
{
    return get_mem_bank(addr).check(addr, size);
}

#endif /* DIRECT_ADDRESSING || REAL_ADDRESSING */

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

