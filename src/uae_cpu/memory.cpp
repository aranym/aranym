/* 2001 MJ */

 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Memory management
  *
  * (c) 1995 Bernd Schmidt
  */

#include <stdio.h>
#include <stdlib.h>

#include "sysdeps.h"

#include "cpu_emulation.h"
#include "m68k.h"
#include "newcpu.h"
#include "memory.h"
#include "readcpu.h"
#include "newcpu.h"
#include "main.h"
#include "exceptions.h"

#if !REAL_ADDRESSING && !DIRECT_ADDRESSING
// This part need rewrite for ARAnyM !!

static bool illegal_mem = false;

#ifdef SAVE_MEMORY_BANKS
addrbank *mem_banks[65536];
#else
addrbank mem_banks[65536];
#endif

#ifdef WORDS_BIGENDIAN
# define swap_words(X) (X)
#else
# define swap_words(X) (((X) >> 16) | ((X) << 16))
#endif

#ifdef NO_INLINE_MEMORY_ACCESS
__inline__ uae_u32 longget (uaecptr addr)
{
    return call_mem_get_func (get_mem_bank (addr).lget, addr);
}
__inline__ uae_u32 wordget (uaecptr addr)
{
    return call_mem_get_func (get_mem_bank (addr).wget, addr);
}
__inline__ uae_u32 byteget (uaecptr addr)
{
    return call_mem_get_func (get_mem_bank (addr).bget, addr);
}
__inline__ void longput (uaecptr addr, uae_u32 l)
{
    call_mem_put_func (get_mem_bank (addr).lput, addr, l);
}
__inline__ void wordput (uaecptr addr, uae_u32 w)
{
    call_mem_put_func (get_mem_bank (addr).wput, addr, w);
}
__inline__ void byteput (uaecptr addr, uae_u32 b)
{
    call_mem_put_func (get_mem_bank (addr).bput, addr, b);
}
#endif

/* A dummy bank that only contains zeros */

static uae_u32 REGPARAM2 dummy_lget (uaecptr) REGPARAM;
static uae_u32 REGPARAM2 dummy_wget (uaecptr) REGPARAM;
static uae_u32 REGPARAM2 dummy_bget (uaecptr) REGPARAM;
static void REGPARAM2 dummy_lput (uaecptr, uae_u32) REGPARAM;
static void REGPARAM2 dummy_wput (uaecptr, uae_u32) REGPARAM;
static void REGPARAM2 dummy_bput (uaecptr, uae_u32) REGPARAM;
static int REGPARAM2 dummy_check (uaecptr addr, uae_u32 size) REGPARAM;

uae_u32 REGPARAM2 dummy_lget (uaecptr addr)
{
    if (illegal_mem)
	write_log ("Illegal lget at %08lx\n", addr);

    return 0;
}

uae_u32 REGPARAM2 dummy_wget (uaecptr addr)
{
    if (illegal_mem)
	write_log ("Illegal wget at %08lx\n", addr);

    return 0;
}

uae_u32 REGPARAM2 dummy_bget (uaecptr addr)
{
    if (illegal_mem)
	write_log ("Illegal bget at %08lx\n", addr);

    return 0;
}

void REGPARAM2 dummy_lput (uaecptr addr, uae_u32 l)
{
    if (illegal_mem)
	write_log ("Illegal lput at %08lx\n", addr);
}
void REGPARAM2 dummy_wput (uaecptr addr, uae_u32 w)
{
    if (illegal_mem)
	write_log ("Illegal wput at %08lx\n", addr);
}
void REGPARAM2 dummy_bput (uaecptr addr, uae_u32 b)
{
    if (illegal_mem)
	write_log ("Illegal bput at %08lx\n", addr);
}

int REGPARAM2 dummy_check (uaecptr addr, uae_u32 size)
{
    if (illegal_mem)
	write_log ("Illegal check at %08lx\n", addr);

    return 0;
}

/* Mac RAM (32 bit addressing) */

static uae_u32 REGPARAM2 ram_lget(uaecptr) REGPARAM;
static uae_u32 REGPARAM2 ram_wget(uaecptr) REGPARAM;
static uae_u32 REGPARAM2 ram_bget(uaecptr) REGPARAM;
static void REGPARAM2 ram_lput(uaecptr, uae_u32) REGPARAM;
static void REGPARAM2 ram_wput(uaecptr, uae_u32) REGPARAM;
static void REGPARAM2 ram_bput(uaecptr, uae_u32) REGPARAM;
static int REGPARAM2 ram_check(uaecptr addr, uae_u32 size) REGPARAM;
static uae_u8 *REGPARAM2 ram_xlate(uaecptr addr) REGPARAM;

static uae_u32 RAMBaseDiff;	// RAMBaseHost - RAMBaseMac

uae_u32 REGPARAM2 ram_lget(uaecptr addr)
{
    uae_u32 *m;
    m = (uae_u32 *)(RAMBaseDiff + addr);
    return do_get_mem_long(m);
}

uae_u32 REGPARAM2 ram_wget(uaecptr addr)
{
    uae_u16 *m;
    m = (uae_u16 *)(RAMBaseDiff + addr);
    return do_get_mem_word(m);
}

uae_u32 REGPARAM2 ram_bget(uaecptr addr)
{
    return (uae_u32)*(uae_u8 *)(RAMBaseDiff + addr);
}

void REGPARAM2 ram_lput(uaecptr addr, uae_u32 l)
{
    uae_u32 *m;
    m = (uae_u32 *)(RAMBaseDiff + addr);
    do_put_mem_long(m, l);
}

void REGPARAM2 ram_wput(uaecptr addr, uae_u32 w)
{
    uae_u16 *m;
    m = (uae_u16 *)(RAMBaseDiff + addr);
    do_put_mem_word(m, w);
}

void REGPARAM2 ram_bput(uaecptr addr, uae_u32 b)
{
	*(uae_u8 *)(RAMBaseDiff + addr) = b;
}

int REGPARAM2 ram_check(uaecptr addr, uae_u32 size)
{
    return (addr - RAMBaseMac + size) < RAMSize;
}

uae_u8 *REGPARAM2 ram_xlate(uaecptr addr)
{
    return (uae_u8 *)(RAMBaseDiff + addr);
}

/* Mac RAM (24 bit addressing) */

static uae_u32 REGPARAM2 ram24_lget(uaecptr) REGPARAM;
static uae_u32 REGPARAM2 ram24_wget(uaecptr) REGPARAM;
static uae_u32 REGPARAM2 ram24_bget(uaecptr) REGPARAM;
static void REGPARAM2 ram24_lput(uaecptr, uae_u32) REGPARAM;
static void REGPARAM2 ram24_wput(uaecptr, uae_u32) REGPARAM;
static void REGPARAM2 ram24_bput(uaecptr, uae_u32) REGPARAM;
static int REGPARAM2 ram24_check(uaecptr addr, uae_u32 size) REGPARAM;
static uae_u8 *REGPARAM2 ram24_xlate(uaecptr addr) REGPARAM;

uae_u32 REGPARAM2 ram24_lget(uaecptr addr)
{
    uae_u32 *m;
    m = (uae_u32 *)(RAMBaseDiff + (addr & 0xffffff));
    return do_get_mem_long(m);
}

uae_u32 REGPARAM2 ram24_wget(uaecptr addr)
{
    uae_u16 *m;
    m = (uae_u16 *)(RAMBaseDiff + (addr & 0xffffff));
    return do_get_mem_word(m);
}

uae_u32 REGPARAM2 ram24_bget(uaecptr addr)
{
    return (uae_u32)*(uae_u8 *)(RAMBaseDiff + (addr & 0xffffff));
}

void REGPARAM2 ram24_lput(uaecptr addr, uae_u32 l)
{
    uae_u32 *m;
    m = (uae_u32 *)(RAMBaseDiff + (addr & 0xffffff));
    do_put_mem_long(m, l);
}

void REGPARAM2 ram24_wput(uaecptr addr, uae_u32 w)
{
    uae_u16 *m;
    m = (uae_u16 *)(RAMBaseDiff + (addr & 0xffffff));
    do_put_mem_word(m, w);
}

void REGPARAM2 ram24_bput(uaecptr addr, uae_u32 b)
{
	*(uae_u8 *)(RAMBaseDiff + (addr & 0xffffff)) = b;
}

int REGPARAM2 ram24_check(uaecptr addr, uae_u32 size)
{
    return ((addr & 0xffffff) - RAMBaseMac + size) < RAMSize;
}

uae_u8 *REGPARAM2 ram24_xlate(uaecptr addr)
{
    return (uae_u8 *)(RAMBaseDiff + (addr & 0xffffff));
}

/* Mac ROM (32 bit addressing) */

static uae_u32 REGPARAM2 rom_lget(uaecptr) REGPARAM;
static uae_u32 REGPARAM2 rom_wget(uaecptr) REGPARAM;
static uae_u32 REGPARAM2 rom_bget(uaecptr) REGPARAM;
static void REGPARAM2 rom_lput(uaecptr, uae_u32) REGPARAM;
static void REGPARAM2 rom_wput(uaecptr, uae_u32) REGPARAM;
static void REGPARAM2 rom_bput(uaecptr, uae_u32) REGPARAM;
static int REGPARAM2 rom_check(uaecptr addr, uae_u32 size) REGPARAM;
static uae_u8 *REGPARAM2 rom_xlate(uaecptr addr) REGPARAM;

static uae_u32 ROMBaseDiff;	// ROMBaseHost - ROMBaseMac

uae_u32 REGPARAM2 rom_lget(uaecptr addr)
{
    uae_u32 *m;
    m = (uae_u32 *)(ROMBaseDiff + addr);
    return do_get_mem_long(m);
}

uae_u32 REGPARAM2 rom_wget(uaecptr addr)
{
    uae_u16 *m;
    m = (uae_u16 *)(ROMBaseDiff + addr);
    return do_get_mem_word(m);
}

uae_u32 REGPARAM2 rom_bget(uaecptr addr)
{
    return (uae_u32)*(uae_u8 *)(ROMBaseDiff + addr);
}

void REGPARAM2 rom_lput(uaecptr addr, uae_u32 b)
{
    if (illegal_mem)
	write_log ("Illegal ROM lput at %08lx\n", addr);
}

void REGPARAM2 rom_wput(uaecptr addr, uae_u32 b)
{
    if (illegal_mem)
	write_log ("Illegal ROM wput at %08lx\n", addr);
}

void REGPARAM2 rom_bput(uaecptr addr, uae_u32 b)
{
    if (illegal_mem)
	write_log ("Illegal ROM bput at %08lx\n", addr);
}

int REGPARAM2 rom_check(uaecptr addr, uae_u32 size)
{
    return (addr - ROMBaseMac + size) < ROMSize;
}

uae_u8 *REGPARAM2 rom_xlate(uaecptr addr)
{
    return (uae_u8 *)(ROMBaseDiff + addr);
}

/* Mac ROM (24 bit addressing) */

static uae_u32 REGPARAM2 rom24_lget(uaecptr) REGPARAM;
static uae_u32 REGPARAM2 rom24_wget(uaecptr) REGPARAM;
static uae_u32 REGPARAM2 rom24_bget(uaecptr) REGPARAM;
static int REGPARAM2 rom24_check(uaecptr addr, uae_u32 size) REGPARAM;
static uae_u8 *REGPARAM2 rom24_xlate(uaecptr addr) REGPARAM;

uae_u32 REGPARAM2 rom24_lget(uaecptr addr)
{
    uae_u32 *m;
    m = (uae_u32 *)(ROMBaseDiff + (addr & 0xffffff));
    return do_get_mem_long(m);
}

uae_u32 REGPARAM2 rom24_wget(uaecptr addr)
{
    uae_u16 *m;
    m = (uae_u16 *)(ROMBaseDiff + (addr & 0xffffff));
    return do_get_mem_word(m);
}

uae_u32 REGPARAM2 rom24_bget(uaecptr addr)
{
    return (uae_u32)*(uae_u8 *)(ROMBaseDiff + (addr & 0xffffff));
}

int REGPARAM2 rom24_check(uaecptr addr, uae_u32 size)
{
    return ((addr & 0xffffff) - ROMBaseMac + size) < ROMSize;
}

uae_u8 *REGPARAM2 rom24_xlate(uaecptr addr)
{
    return (uae_u8 *)(ROMBaseDiff + (addr & 0xffffff));
}

/* Frame buffer */

static uae_u32 REGPARAM2 frame_direct_lget(uaecptr) REGPARAM;
static uae_u32 REGPARAM2 frame_direct_wget(uaecptr) REGPARAM;
static uae_u32 REGPARAM2 frame_direct_bget(uaecptr) REGPARAM;
static void REGPARAM2 frame_direct_lput(uaecptr, uae_u32) REGPARAM;
static void REGPARAM2 frame_direct_wput(uaecptr, uae_u32) REGPARAM;
static void REGPARAM2 frame_direct_bput(uaecptr, uae_u32) REGPARAM;

static uae_u32 REGPARAM2 frame_host_555_lget(uaecptr) REGPARAM;
static uae_u32 REGPARAM2 frame_host_555_wget(uaecptr) REGPARAM;
static void REGPARAM2 frame_host_555_lput(uaecptr, uae_u32) REGPARAM;
static void REGPARAM2 frame_host_555_wput(uaecptr, uae_u32) REGPARAM;

static uae_u32 REGPARAM2 frame_host_565_lget(uaecptr) REGPARAM;
static uae_u32 REGPARAM2 frame_host_565_wget(uaecptr) REGPARAM;
static void REGPARAM2 frame_host_565_lput(uaecptr, uae_u32) REGPARAM;
static void REGPARAM2 frame_host_565_wput(uaecptr, uae_u32) REGPARAM;

static uae_u32 REGPARAM2 frame_host_888_lget(uaecptr) REGPARAM;
static void REGPARAM2 frame_host_888_lput(uaecptr, uae_u32) REGPARAM;

static int REGPARAM2 frame_check(uaecptr addr, uae_u32 size) REGPARAM;
static uae_u8 *REGPARAM2 frame_xlate(uaecptr addr) REGPARAM;

static uae_u32 FrameBaseDiff;	// MacFrameBaseHost - MacFrameBaseMac

uae_u32 REGPARAM2 frame_direct_lget(uaecptr addr)
{
    uae_u32 *m;
    m = (uae_u32 *)(FrameBaseDiff + addr);
    return do_get_mem_long(m);
}

uae_u32 REGPARAM2 frame_direct_wget(uaecptr addr)
{
    uae_u16 *m;
    m = (uae_u16 *)(FrameBaseDiff + addr);
    return do_get_mem_word(m);
}

uae_u32 REGPARAM2 frame_direct_bget(uaecptr addr)
{
    return (uae_u32)*(uae_u8 *)(FrameBaseDiff + addr);
}

void REGPARAM2 frame_direct_lput(uaecptr addr, uae_u32 l)
{
    uae_u32 *m;
    m = (uae_u32 *)(FrameBaseDiff + addr);
    do_put_mem_long(m, l);
}

void REGPARAM2 frame_direct_wput(uaecptr addr, uae_u32 w)
{
    uae_u16 *m;
    m = (uae_u16 *)(FrameBaseDiff + addr);
    do_put_mem_word(m, w);
}

void REGPARAM2 frame_direct_bput(uaecptr addr, uae_u32 b)
{
    *(uae_u8 *)(FrameBaseDiff + addr) = b;
}

uae_u32 REGPARAM2 frame_host_555_lget(uaecptr addr)
{
    uae_u32 *m, l;
    m = (uae_u32 *)(FrameBaseDiff + addr);
    l = *m;
	return swap_words(l);
}

uae_u32 REGPARAM2 frame_host_555_wget(uaecptr addr)
{
    uae_u16 *m;
    m = (uae_u16 *)(FrameBaseDiff + addr);
    return *m;
}

void REGPARAM2 frame_host_555_lput(uaecptr addr, uae_u32 l)
{
    uae_u32 *m;
    m = (uae_u32 *)(FrameBaseDiff + addr);
    *m = swap_words(l);
}

void REGPARAM2 frame_host_555_wput(uaecptr addr, uae_u32 w)
{
    uae_u16 *m;
    m = (uae_u16 *)(FrameBaseDiff + addr);
    *m = w;
}

uae_u32 REGPARAM2 frame_host_565_lget(uaecptr addr)
{
    uae_u32 *m, l;
    m = (uae_u32 *)(FrameBaseDiff + addr);
    l = *m;
    l = (l & 0x001f001f) | ((l >> 1) & 0x7fe07fe0);
    return swap_words(l);
}

uae_u32 REGPARAM2 frame_host_565_wget(uaecptr addr)
{
    uae_u16 *m, w;
    m = (uae_u16 *)(FrameBaseDiff + addr);
    w = *m;
    return (w & 0x1f) | ((w >> 1) & 0x7fe0);
}

void REGPARAM2 frame_host_565_lput(uaecptr addr, uae_u32 l)
{
    uae_u32 *m;
    m = (uae_u32 *)(FrameBaseDiff + addr);
    l = (l & 0x001f001f) | ((l << 1) & 0xffc0ffc0);
    *m = swap_words(l);
}

void REGPARAM2 frame_host_565_wput(uaecptr addr, uae_u32 w)
{
    uae_u16 *m;
    m = (uae_u16 *)(FrameBaseDiff + addr);
    *m = (w & 0x1f) | ((w << 1) & 0xffc0);
}

uae_u32 REGPARAM2 frame_host_888_lget(uaecptr addr)
{
    uae_u32 *m, l;
    m = (uae_u32 *)(FrameBaseDiff + addr);
    return *m;
}

void REGPARAM2 frame_host_888_lput(uaecptr addr, uae_u32 l)
{
    uae_u32 *m;
    m = (uae_u32 *)(MacFrameBaseHost + addr - MacFrameBaseMac);
    *m = l;
}

int REGPARAM2 frame_check(uaecptr addr, uae_u32 size)
{
    return (addr - MacFrameBaseMac + size) < MacFrameSize;
}

uae_u8 *REGPARAM2 frame_xlate(uaecptr addr)
{
    return (uae_u8 *)(FrameBaseDiff + addr);
}

/* Default memory access functions */

int REGPARAM2 default_check (uaecptr a, uae_u32 b)
{
    return 0;
}

uae_u8 *REGPARAM2 default_xlate (uaecptr a)
{
    write_log("Your Mac program just did something terribly stupid\n");
    return NULL;
}

/* Address banks */

addrbank dummy_bank = {
    dummy_lget, dummy_wget, dummy_bget,
    dummy_lput, dummy_wput, dummy_bput,
    default_xlate, dummy_check
};

addrbank ram_bank = {
    ram_lget, ram_wget, ram_bget,
    ram_lput, ram_wput, ram_bput,
    ram_xlate, ram_check
};

addrbank ram24_bank = {
    ram24_lget, ram24_wget, ram24_bget,
    ram24_lput, ram24_wput, ram24_bput,
    ram24_xlate, ram24_check
};

addrbank rom_bank = {
    rom_lget, rom_wget, rom_bget,
    rom_lput, rom_wput, rom_bput,
    rom_xlate, rom_check
};

addrbank rom24_bank = {
    rom24_lget, rom24_wget, rom24_bget,
    rom_lput, rom_wput, rom_bput,
    rom24_xlate, rom24_check
};

addrbank frame_direct_bank = {
    frame_direct_lget, frame_direct_wget, frame_direct_bget,
    frame_direct_lput, frame_direct_wput, frame_direct_bput,
    frame_xlate, frame_check
};

addrbank frame_host_555_bank = {
    frame_host_555_lget, frame_host_555_wget, frame_direct_bget,
    frame_host_555_lput, frame_host_555_wput, frame_direct_bput,
    frame_xlate, frame_check
};

addrbank frame_host_565_bank = {
    frame_host_565_lget, frame_host_565_wget, frame_direct_bget,
    frame_host_565_lput, frame_host_565_wput, frame_direct_bput,
    frame_xlate, frame_check
};

addrbank frame_host_888_bank = {
    frame_host_888_lget, frame_direct_wget, frame_direct_bget,
    frame_host_888_lput, frame_direct_wput, frame_direct_bput,
    frame_xlate, frame_check
};

void memory_init(void)
{
	for(uint32 i=0; i<65536; i++)
		put_mem_bank(i<<16, &dummy_bank);

	// Limit RAM size to not overlap ROM
	uint32 ram_size = RAMSize > ROMBaseMac ? ROMBaseMac : RAMSize;

	RAMBaseDiff = (uae_u32)RAMBaseHost - (uae_u32)RAMBaseMac;
	ROMBaseDiff = (uae_u32)ROMBaseHost - (uae_u32)ROMBaseMac;
	FrameBaseDiff = (uae_u32)MacFrameBaseHost - (uae_u32)MacFrameBaseMac;

	// Map RAM and ROM
	if (TwentyFourBitAddressing) {
		map_banks(&ram24_bank, RAMBaseMac >> 16, ram_size >> 16);
		map_banks(&rom24_bank, ROMBaseMac >> 16, ROMSize >> 16);
	} else {
		map_banks(&ram_bank, RAMBaseMac >> 16, ram_size >> 16);
		map_banks(&rom_bank, ROMBaseMac >> 16, ROMSize >> 16);
	}

	// Map frame buffer
	switch (MacFrameLayout) {
		case FLAYOUT_DIRECT:
			map_banks(&frame_direct_bank, MacFrameBaseMac >> 16, (MacFrameSize >> 16) + 1);
			break;
		case FLAYOUT_HOST_555:
			map_banks(&frame_host_555_bank, MacFrameBaseMac >> 16, (MacFrameSize >> 16) + 1);
			break;
		case FLAYOUT_HOST_565:
			map_banks(&frame_host_565_bank, MacFrameBaseMac >> 16, (MacFrameSize >> 16) + 1);
			break;
		case FLAYOUT_HOST_888:
			map_banks(&frame_host_888_bank, MacFrameBaseMac >> 16, (MacFrameSize >> 16) + 1);
			break;
	}
}

void map_banks(addrbank *bank, int start, int size)
{
    int bnr;
    unsigned long int hioffs = 0, endhioffs = 0x100;

    if (start >= 0x100) {
	for (bnr = start; bnr < start + size; bnr++)
	    put_mem_bank (bnr << 16, bank);
	return;
    }
    if (TwentyFourBitAddressing) endhioffs = 0x10000;
    for (hioffs = 0; hioffs < endhioffs; hioffs += 0x100)
	for (bnr = start; bnr < start+size; bnr++)
	    put_mem_bank((bnr + hioffs) << 16, bank);
}

#endif /* !REAL_ADDRESSING && !DIRECT_ADDRESSING */

#ifdef FULL_MMU

uaecptr mmu_decode_addr(uaecptr addr, bool data, bool write)
{
    uae_u32 rp;
    uaecptr mask;
    if (regs.s) rp = regs.srp; else rp = regs.urp;
    if (data) {
      if (regs.dtt0 & 0x8000) {
        if (((regs.dtt0 & 0x6000) > 0x3000)
              || (!regs.s || !(regs.dtt0 & 0x6000))
              || (regs.s && ((regs.dtt0 & 0x6000) == 0x2000))) {
          if ((write) && ((regs.dtt0 & 0x4) != 0)) {
	    last_fault_for_exception_3 = addr;
	    longjmp(excep_env, 3);
	  }
          mask = ((~regs.dtt0) & 0xff0000) << 8;
          if ((addr & mask) == (regs.dtt0 & mask)) return addr;
        }
      }
      if (regs.dtt1 & 0x8000) {
        if (((regs.dtt1 & 0x6000) > 0x3000)
              || (!regs.s || !(regs.dtt1 & 0x6000))
              || (regs.s && ((regs.dtt1 & 0x6000) == 0x2000))) {
          if ((write) && ((regs.dtt1 & 0x4) != 0)) {
	    last_fault_for_exception_3 = addr;
	    longjmp(excep_env, 3);
	  }
          mask = ((~regs.dtt1) & 0xff0000) << 8;
          if ((addr & mask) == (regs.dtt1 & mask)) return addr;
        }
      }
      if (regs.tcp) {
        uaecptr atcindex = ((addr << 11) >> 24);
        if (regs.atcvald[atcindex] && (regs.atcind[atcindex] = addr)) {
          if (!regs.s || !regs.atcsuperd[atcindex]) {
            if (!write || !regs.atcwritepd[atcindex]) {
              // Resident?
              if (write && !regs.atcmodifd[atcindex]) {
                regs.atcmodifd[atcindex] = 1;
                // Najit v pameti a nastavit modif
                uint16 *rootp = (uint16 *)do_get_real_address_direct(rp & ((addr >> 25) << 2));
                uint16 root = *rootp;
                uint8 *apdt, *apd;
                uint16 pdt, pd;
                flagtype wr;
                if ((root & 0x3) > 1) {
                  wr = (root & 0x4) >> 2;         // write
                  *rootp = root | 0x8;            // used
                  apdt = do_get_real_address_direct((root & 0xfffffe00) | ((addr & 0x01fc0000) >> 16));
                  pdt = *apdt;
                  if ((pdt & 0x3) > 1) {
                    wr += (pdt & 0x4) >> 2;
                    *apdt = pdt | 0x8;
                    apd = do_get_real_address_direct((pdt & 0xffffff80) | ((addr & 0x0003e000) >> 11));
                    pd = *apd;
                    switch (pd & 0x3) {
                      case 0: 
		               last_fault_for_exception_3 = pd & 0xfffffffc;
			       longjmp(excep_env, 2);
                      case 2:  apd = do_get_real_address_direct(pd & 0xfffffffc);
                               pd = *apd;
                               if (((pd & 0x3) % 2) == 0) {
			         last_fault_for_exception_3 = pd & 0xfffffffc;
				 longjmp(excep_env, 2);
			       }
                      default: wr += (pd & 0x4) >> 2;
                               if (!(wr > 1)) {
				 last_fault_for_exception_3 = pd & 0xfffffffc;
				 longjmp(excep_env, 3);
			       }
                               if (!regs.s && ((pd & 0x80) != 0)) {
				 last_fault_for_exception_3 = pd & 0xfffffffc;
				 longjmp(excep_env, 3);
			       }
                               *apd = pd | 0x18;
                    }
                  } else {
		    last_fault_for_exception_3 = pdt & 0xfffffffc;
		    longjmp(excep_env, 3);
		  }
                } else {
		  last_fault_for_exception_3 = root & 0xfffffffc;
		  longjmp(excep_env, 3);
		}
              }
              return regs.atcoutd[atcindex];
            } else {
	      last_fault_for_exception_3 = addr;
	      longjmp(excep_env, 3);
	    }
          } else {
	    last_fault_for_exception_3 = addr;
	    longjmp(excep_env, 3);
	  }
        } else {
          uint16 *rootp = (uint16 *)do_get_real_address_direct(rp & ((addr >> 25) << 2));
          uint16 root = *rootp;
          uint8 *apdt, *apd;
          uint16 pdt, pd;
          flagtype wr;
          if ((root & 0x3) > 1) {
            wr = (root & 0x4) >> 2;         // write
            *rootp = root | 0x8;            // used
            apdt = do_get_real_address_direct((root & 0xfffffe00) | ((addr & 0x01fc0000) >> 16));
            pdt = *apdt;
            if ((pdt & 0x3) > 1) {
              wr += (pdt & 0x4) >> 2;
              *apdt = pdt | 0x8;
              apd = do_get_real_address_direct((pdt & 0xffffff80) | ((addr & 0x0003e000) >> 11));
              pd = *apd;
              switch (pd & 0x3) {
                case 0: 
		         last_fault_for_exception_3 = pd & 0xfffffffc;
			 longjmp(excep_env, 2);
                case 2:  apd = do_get_real_address_direct(pd & 0xfffffffc);
                         pd = *apd;
                         if (((pd & 0x3) % 2) == 0) {
			   last_fault_for_exception_3 = pd & 0xfffffffc;
			   longjmp(excep_env, 2);
			 }
                default: wr += (pd & 0x4) >> 2;
                         if (write && !(wr > 1)) {
			   last_fault_for_exception_3 = pd & 0xfffffffc;
			   longjmp(excep_env, 3);
			 }
                         if (!regs.s && ((pd & 0x80) != 0)) {
			   last_fault_for_exception_3 = pd & 0xfffffffc;
			   longjmp(excep_env, 3);
			 }
                         *apd = pd | 0x8;
                         if (write) *apd = pd | 0x10;
                         regs.atcind[atcindex] = addr & 0xffffe000;
                         regs.atcoutd[atcindex] = pd & 0xffffe000;
                         regs.atcu0d[atcindex] = (pd & 0x00000100) >> 8;
                         regs.atcu1d[atcindex] = (pd & 0x00000200) >> 9;
                         regs.atcsuperd[atcindex] = (pd & 0x00000080) >> 7;
                         regs.atccmd[atcindex] = (pd & 0x00000060) >> 5;
                         regs.atcmodifd[atcindex] = write;
                         regs.atcwritepd[atcindex] = wr ? 1 : 0;
                         regs.atcresidd[atcindex] = 1;
                         regs.atcglobald[atcindex] = (pd & 0x00000400) >> 10;
                         regs.atcfc2d[atcindex] = regs.s; // ??
                         addr = (pd & 0xffffe000) | (addr & 0x00001fff);
              }
            } else {
	      last_fault_for_exception_3 = pdt & 0xfffffffc;
	      longjmp(excep_env, 2);
	    }
          } else {
	    last_fault_for_exception_3 = root & 0xfffffffc;
	    longjmp(excep_env, 2);
          }
        }
      } else {
        uaecptr atcindex = ((addr << 12) >> 24);
        if (regs.atcvald[atcindex] && (regs.atcind[atcindex] = addr)) {
          if (!regs.s || !regs.atcsuperd[atcindex]) {
            if (!write || !regs.atcwritepd[atcindex]) {
              // Resident?
              if (write && !regs.atcmodifd[atcindex]) {
                regs.atcmodifd[atcindex] = 1;
                // Najit v pameti a nastavit modif
                uint16 *rootp = (uint16 *)do_get_real_address_direct(rp & ((addr >> 25) << 2));
                uint16 root = *rootp;
                uint8 *apdt, *apd;
                uint16 pdt, pd;
                flagtype wr;
                if ((root & 0x3) > 1) {
                  wr = (root & 0x4) >> 2;         // write
                  *rootp = root | 0x8;            // used
                  apdt = do_get_real_address_direct((root & 0xfffffe00) | ((addr & 0x01fc0000) >> 16));
                  pdt = *apdt;
                  if ((pdt & 0x3) > 1) {
                    wr += (pdt & 0x4) >> 2;
                    *apdt = pdt | 0x8;
                    apd = do_get_real_address_direct((pdt & 0xffffff00) | ((addr & 0x0003f000) >> 10));
                    pd = *apd;
                    switch (pd & 0x3) {
                      case 0: 
		               last_fault_for_exception_3 = pd & 0xfffffffc;
			       longjmp(excep_env, 2);
                      case 2:  apd = do_get_real_address_direct(pd & 0xfffffffc);
                               pd = *apd;
                               if (((pd & 0x3) % 2) == 0) {
				 last_fault_for_exception_3 = pd & 0xfffffffc;
				 longjmp(excep_env, 2);
			       }
                      default: wr += (pd & 0x4) >> 2;
                               if (write && !(wr > 1)) {
				 last_fault_for_exception_3 = pd & 0xfffffffc;
				 longjmp(excep_env, 3);
			       }
                               if (!regs.s && ((pd & 0x80) != 0)) {
				 last_fault_for_exception_3 = pd & 0xfffffffc;
				 longjmp(excep_env, 3);
			       }
                               *apd = pd | 0x18;
                    }
                  } else {
		    last_fault_for_exception_3 = pdt & 0xfffffffc;
		    longjmp(excep_env, 2);
		  }
                } else {
		  last_fault_for_exception_3 = root & 0xfffffffc;
		  longjmp(excep_env, 2);
		}
              }
              return regs.atcoutd[atcindex];
            } else {
	      last_fault_for_exception_3 = addr;
	      longjmp(excep_env, 3);
	    }
          } else {
	    last_fault_for_exception_3 = addr;
	    longjmp(excep_env, 3);
	  }
        } else {
          uint16 *rootp = (uint16 *)do_get_real_address_direct(rp & ((addr >> 25) << 2));
          uint16 root = *rootp;
          uint8 *apdt, *apd;
          uint16 pdt, pd;
          flagtype wr;
          if ((root & 0x3) > 1) {
            wr = (root & 0x4) >> 2;         // write
            *rootp = root | 0x8;            // used
            apdt = do_get_real_address_direct((root & 0xfffffe00) | ((addr & 0x01fc0000) >> 16));
            pdt = *apdt;
            if ((pdt & 0x3) > 1) {
              wr += (pdt & 0x4) >> 2;
              *apdt = pdt | 0x8;
              apd = do_get_real_address_direct((pdt & 0xffffff00) | ((addr & 0x0003f000) >> 10));
              pd = *apd;
              switch (pd & 0x3) {
                case 0: 
		         last_fault_for_exception_3 = pd & 0xfffffffc;
			 longjmp(excep_env, 2);
                case 2:  apd = do_get_real_address_direct(pd & 0xfffffffc);
                         pd = *apd;
                         if (((pd & 0x3) % 2) == 0) {
			   last_fault_for_exception_3 = pd & 0xfffffffc;
			   longjmp(excep_env, 2);
			 }
                default: wr += (pd & 0x4) >> 2;
                         if (write && !(wr > 1)) {
			   last_fault_for_exception_3 = pd & 0xfffffffc;
			   longjmp(excep_env, 3);
			 }
                         if (!regs.s && ((pd & 0x80) != 0)) {
			   last_fault_for_exception_3 = pd & 0xfffffffc;
			   longjmp(excep_env, 3);
			 }
                         *apd = pd | 0x8;
                         if (write) *apd = pd | 0x10;
                         regs.atcind[atcindex] = addr & 0xfffff000;
                         regs.atcoutd[atcindex] = pd & 0xfffff000;
                         regs.atcu0d[atcindex] = (pd & 0x00000100) >> 8;
                         regs.atcu1d[atcindex] = (pd & 0x00000200) >> 9;
                         regs.atcsuperd[atcindex] = (pd & 0x00000080) >> 7;
                         regs.atccmd[atcindex] = (pd & 0x00000060) >> 5;
                         regs.atcmodifd[atcindex] = write;
                         regs.atcwritepd[atcindex] = wr ? 1 : 0;
                         regs.atcresidd[atcindex] = 1;
                         regs.atcglobald[atcindex] = (pd & 0x00000400) >> 10;
                         regs.atcfc2d[atcindex] = regs.s; // ??
                         addr = (pd & 0xfffff000) | (addr & 0x00000fff);
              }
            } else {
	      last_fault_for_exception_3 = pdt & 0xfffffffc;
	      longjmp(excep_env, 2);
	    }
          } else {
	    last_fault_for_exception_3 = root & 0xfffffffc;
	    longjmp(excep_env, 2);
	  }
        }    
      }
    } else {
      if (regs.itt0 & 0x8000) {
        if (((regs.itt0 & 0x6000) > 0x3000)
              || (!regs.s || !(regs.itt0 & 0x6000))
              || (regs.s && ((regs.itt0 & 0x6000) == 0x2000))) {
          if ((write) && ((regs.itt0 & 0x4) != 0)) {
	    last_fault_for_exception_3 = addr;
	    longjmp(excep_env, 3);
	  }
          mask = ((~regs.itt0) & 0xff0000) << 8;
          if ((addr & mask) == (regs.itt0 & mask)) return addr;
        }
      }
      if (regs.itt1 & 0x8000) {
        if (((regs.itt1 & 0x6000) > 0x3000)
              || (!regs.s || !(regs.itt1 & 0x6000))
              || (regs.s && ((regs.itt1 & 0x6000) == 0x2000))) {
          if ((write) && ((regs.itt1 & 0x4) != 0)) {
	    last_fault_for_exception_3 = addr;
	    longjmp(excep_env, 3);
	  }
          mask = ((~regs.itt1) & 0xff0000) << 8;
          if ((addr & mask) == (regs.itt1 & mask)) return addr;
        }
      }
      if (regs.tcp) {
        uaecptr atcindex = ((addr << 11) >> 24);
        if (regs.atcvali[atcindex] && (regs.atcini[atcindex] = addr)) {
          if (!regs.s || !regs.atcsuperi[atcindex]) {
            if (!write || !regs.atcwritepi[atcindex]) {
              // Resident?
              if (write && !regs.atcmodifi[atcindex]) {
                regs.atcmodifi[atcindex] = 1;
                // Najit v pameti a nastavit modif
                uint16 *rootp = (uint16 *)do_get_real_address_direct(rp & ((addr >> 25) << 2));
                uint16 root = *rootp;
                uint8 *apdt, *apd;
                uint16 pdt, pd;
                flagtype wr;
                if ((root & 0x3) > 1) {
                  wr = (root & 0x4) >> 2;         // write
                  *rootp = root | 0x8;            // used
                  apdt = do_get_real_address_direct((root & 0xfffffe00) | ((addr & 0x01fc0000) >> 16));
                  pdt = *apdt;
                  if ((pdt & 0x3) > 1) {
                    wr += (pdt & 0x4) >> 2;
                    *apdt = pdt | 0x8;
                    apd = do_get_real_address_direct((pdt & 0xffffff80) | ((addr & 0x0003e000) >> 11));
                    pd = *apd;
                    switch (pd & 0x3) {
                      case 0: 
		               last_fault_for_exception_3 = pd & 0xfffffffc;
			       longjmp(excep_env, 2);
                      case 2:  apd = do_get_real_address_direct(pd & 0xfffffffc);
                               pd = *apd;
                               if (((pd & 0x3) % 2) == 0) {
				 last_fault_for_exception_3 = pd & 0xfffffffc;
				 longjmp(excep_env, 2);
			       }
                      default: wr += (pd & 0x4) >> 2;
                               if (!(wr > 1)) {
				 last_fault_for_exception_3 = pd & 0xfffffffc;
				 longjmp(excep_env, 3);
			       }
                               if (!regs.s && ((pd & 0x80) != 0)) {
				 last_fault_for_exception_3 = pd & 0xfffffffc;
				 longjmp(excep_env, 3);
			       }
                               *apd = pd | 0x18;
                    }
                  } else {
		    last_fault_for_exception_3 = pdt & 0xfffffffc;
		    longjmp(excep_env, 2);
		  }
                } else {
		  last_fault_for_exception_3 = root & 0xfffffffc;
		  longjmp(excep_env, 2);
		}
              }
              return regs.atcouti[atcindex];
            } else {
	      last_fault_for_exception_3 = addr;
	      longjmp(excep_env, 3);
	    }
          } else {
	    last_fault_for_exception_3 = addr;
	    longjmp(excep_env, 3);
	  }
        } else {
          uint16 *rootp = (uint16 *)do_get_real_address_direct(rp & ((addr >> 25) << 2));
          uint16 root = *rootp;
          uint8 *apdt, *apd;
          uint16 pdt, pd;
          flagtype wr;
          if ((root & 0x3) > 1) {
            wr = (root & 0x4) >> 2;         // write
            *rootp = root | 0x8;            // used
            apdt = do_get_real_address_direct((root & 0xfffffe00) | ((addr & 0x01fc0000) >> 16));
            pdt = *apdt;
            if ((pdt & 0x3) > 1) {
              wr += (pdt & 0x4) >> 2;
              *apdt = pdt | 0x8;
              apd = do_get_real_address_direct((pdt & 0xffffff80) | ((addr & 0x0003e000) >> 11));
              pd = *apd;
              switch (pd & 0x3) {
                case 0: 
		         last_fault_for_exception_3 = pd & 0xfffffffc;
			 longjmp(excep_env, 2);
                case 2:  apd = do_get_real_address_direct(pd & 0xfffffffc);
                         pd = *apd;
                         if (((pd & 0x3) % 2) == 0) {
			   last_fault_for_exception_3 = pd & 0xfffffffc;
			   longjmp(excep_env, 2);
			 }
                default: wr += (pd & 0x4) >> 2;
                         if (write && !(wr > 1)) {
			   last_fault_for_exception_3 = pd & 0xfffffffc;
			   longjmp(excep_env, 3);
			 }
                         if (!regs.s && ((pd & 0x80) != 0)) {
			   last_fault_for_exception_3 = pd & 0xfffffffc;
			   longjmp(excep_env, 3);
			 }
                         *apd = pd | 0x8;
                         if (write) *apd = pd | 0x10;
                         regs.atcini[atcindex] = addr & 0xffffe000;
                         regs.atcouti[atcindex] = pd & 0xffffe000;
                         regs.atcu0i[atcindex] = (pd & 0x00000100) >> 8;
                         regs.atcu1i[atcindex] = (pd & 0x00000200) >> 9;
                         regs.atcsuperi[atcindex] = (pd & 0x00000080) >> 7;
                         regs.atccmi[atcindex] = (pd & 0x00000060) >> 5;
                         regs.atcmodifi[atcindex] = write;
                         regs.atcwritepi[atcindex] = wr ? 1 : 0;
                         regs.atcresidi[atcindex] = 1;
                         regs.atcglobali[atcindex] = (pd & 0x00000400) >> 10;
                         regs.atcfc2i[atcindex] = regs.s; // ??
                         addr = (pd & 0xffffe000) | (addr & 0x00001fff);
              }
            } else {
	      last_fault_for_exception_3 = pdt & 0xfffffffc;
	      longjmp(excep_env, 2);
	    }
          } else {
	    last_fault_for_exception_3 = root & 0xfffffffc;
	    longjmp(excep_env, 2);
	  } 
        }
      } else {
        uaecptr atcindex = ((addr << 12) >> 24);
        if (regs.atcvali[atcindex] && (regs.atcini[atcindex] = addr)) {
          if (!regs.s || !regs.atcsuperi[atcindex]) {
            if (!write || !regs.atcwritepi[atcindex]) {
              // Resident?
              if (write && !regs.atcmodifi[atcindex]) {
                regs.atcmodifi[atcindex] = 1;
                // Najit v pameti a nastavit modif
                uint16 *rootp = (uint16 *)do_get_real_address_direct(rp & ((addr >> 25) << 2));
                uint16 root = *rootp;
                uint8 *apdt, *apd;
                uint16 pdt, pd;
                flagtype wr;
                if ((root & 0x3) > 1) {
                  wr = (root & 0x4) >> 2;         // write
                  *rootp = root | 0x8;            // used
                  apdt = do_get_real_address_direct((root & 0xfffffe00) | ((addr & 0x01fc0000) >> 16));
                  pdt = *apdt;
                  if ((pdt & 0x3) > 1) {
                    wr += (pdt & 0x4) >> 2;
                    *apdt = pdt | 0x8;
                    apd = do_get_real_address_direct((pdt & 0xffffff00) | ((addr & 0x0003f000) >> 10));
                    pd = *apd;
                    switch (pd & 0x3) {
                      case 0: 
		               last_fault_for_exception_3 = pd & 0xfffffffc;
			       longjmp(excep_env, 2);
                      case 2:  apd = do_get_real_address_direct(pd & 0xfffffffc);
                               pd = *apd;
                               if (((pd & 0x3) % 2) == 0) {
				 last_fault_for_exception_3 = pd & 0xfffffffc;
				 longjmp(excep_env, 2);
			       }
                      default: wr += (pd & 0x4) >> 2;
                               if (write && !(wr > 1)) {
				 last_fault_for_exception_3 = pd & 0xfffffffc;
				 longjmp(excep_env, 3);
			       }
                               if (!regs.s && ((pd & 0x80) != 0)) {
				 last_fault_for_exception_3 = pd & 0xfffffffc;
				 longjmp(excep_env, 3);
			       }
                               *apd = pd | 0x18;
                    }
                  } else {
		    last_fault_for_exception_3 = pdt & 0xfffffffc;
		    longjmp(excep_env, 2);
		  }
                } else {
		  last_fault_for_exception_3 = root & 0xfffffffc;
		  longjmp(excep_env, 2);
		}
              }
              return regs.atcouti[atcindex];
            } else {
	      last_fault_for_exception_3 = addr;
	      longjmp(excep_env, 3);
	    }
          } else {
	    last_fault_for_exception_3 = addr;
	    longjmp(excep_env, 3);
	  }
        } else {
          uint16 *rootp = (uint16 *)do_get_real_address_direct(rp & ((addr >> 25) << 2));
          uint16 root = *rootp;
          uint8 *apdt, *apd;
          uint16 pdt, pd;
          flagtype wr;
          if ((root & 0x3) > 1) {
            wr = (root & 0x4) >> 2;         // write
            *rootp = root | 0x8;            // used
            apdt = do_get_real_address_direct((root & 0xfffffe00) | ((addr & 0x01fc0000) >> 16));
            pdt = *apdt;
            if ((pdt & 0x3) > 1) {
              wr += (pdt & 0x4) >> 2;
              *apdt = pdt | 0x8;
              apd = do_get_real_address_direct((pdt & 0xffffff00) | ((addr & 0x0003f000) >> 10));
              pd = *apd;
              switch (pd & 0x3) {
                case 0: 
		         last_fault_for_exception_3 = pd & 0xfffffffc;
			 longjmp(excep_env, 2);
                case 2:  apd = do_get_real_address_direct(pd & 0xfffffffc);
                         pd = *apd;
                         if (((pd & 0x3) % 2) == 0) {
			   last_fault_for_exception_3 = pd & 0xfffffffc;
			   longjmp(excep_env, 2);
			 }
                default: wr += (pd & 0x4) >> 2;
                         if (write && !(wr > 1)) {
			   last_fault_for_exception_3 = pd & 0xfffffffc;
			   longjmp(excep_env, 3);
			 }
                         if (!regs.s && ((pd & 0x80) != 0)) {
			   last_fault_for_exception_3 = pd & 0xfffffffc;
			   longjmp(excep_env, 3);
			 }
                         *apd = pd | 0x8;
                         if (write) *apd = pd | 0x10;
                         regs.atcini[atcindex] = addr & 0xfffff000;
                         regs.atcouti[atcindex] = pd & 0xfffff000;
                         regs.atcu0i[atcindex] = (pd & 0x00000100) >> 8;
                         regs.atcu1i[atcindex] = (pd & 0x00000200) >> 9;
                         regs.atcsuperi[atcindex] = (pd & 0x00000080) >> 7;
                         regs.atccmi[atcindex] = (pd & 0x00000060) >> 5;
                         regs.atcmodifi[atcindex] = write;
                         regs.atcwritepi[atcindex] = wr ? 1 : 0;
                         regs.atcresidi[atcindex] = 1;
                         regs.atcglobali[atcindex] = (pd & 0x00000400) >> 10;
                         regs.atcfc2i[atcindex] = regs.s; // ??
                         addr = (pd & 0xfffff000) | (addr & 0x00000fff);
              }
            } else {
	      last_fault_for_exception_3 = pdt & 0xfffffffc;
	      longjmp(excep_env, 2);
	    }
          } else {
	    last_fault_for_exception_3 = root & 0xfffffffc;
	    longjmp(excep_env, 2);
	  }
        }    
      }
    }
  return addr;
}

#endif /* FULL_MMU */
