/*
 * cpummu.h - MMU emulation
 *
 * Copyright (c) 2001-2004 Milan Jurik of ARAnyM dev team (see AUTHORS)
 * 
 * Inspired by UAE MMU patch
 *
 * This file is part of the ARAnyM project which builds a new and powerful
 * TOS/FreeMiNT compatible virtual machine running on almost any hardware.
 *
 * ARAnyM is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * ARAnyM is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ARAnyM; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef CPUMMU_H
#define CPUMMU_H

#include "registers.h"

# include <cstdlib>

#define MMU_TEST_PTEST		1
#define MMU_TEST_VERBOSE	2
#define MMU_TEST_FORCE_TABLE_SEARCH	4
#define MMU_TEST_NO_BUSERR	8

extern void mmu_dump_tables(void);

#define MMU_PAGE_8KB	1
#define MMU_PAGE_4KB	0

#define MMU_TTR_LOGICAL_BASE				0xff000000
#define MMU_TTR_LOGICAL_MASK				0x00ff0000
#define MMU_TTR_BIT_ENABLED					(1 << 15)
#define MMU_TTR_BIT_SFIELD_ENABLED			(1 << 14)
#define MMU_TTR_BIT_SFIELD_SUPER			(1 << 13)
#define MMU_TTR_SFIELD_SHIFT				13
#define MMU_TTR_UX_MASK						((1 << 9) | (1 << 8))
#define MMU_TTR_UX_SHIFT					8
#define MMU_TTR_CACHE_MASK				((1 << 6) | (1 << 5))
#define MMU_TTR_CACHE_SHIFT						5
#define MMU_TTR_BIT_WRITE_PROTECT				(1 << 2)

#define MMU_UDT_MASK	3
#define MMU_PDT_MASK	3

#define MMU_DES_WP			4
#define MMU_DES_USED		8

/* page descriptors only */
#define MMU_DES_MODIFIED	16
#define MMU_DES_SUPER		(1 << 7)
#define MMU_DES_GLOBAL		(1 << 10)

#define MMU_ROOT_PTR_ADDR_MASK				0xfffffe00
#define MMU_PTR_PAGE_ADDR_MASK_8			0xffffff80
#define MMU_PTR_PAGE_ADDR_MASK_4			0xffffff00

#define MMU_PAGE_INDIRECT_MASK				0xfffffffc
#define MMU_PAGE_ADDR_MASK_8				0xffffe000
#define MMU_PAGE_ADDR_MASK_4				0xfffff000
#define MMU_PAGE_UR_MASK_8					((1 << 12) | (1 << 11))
#define MMU_PAGE_UR_MASK_4					(1 << 11)
#define MMU_PAGE_UR_SHIFT					11

#define MMU_MMUSR_ADDR_MASK	0xfffff000
#define MMU_MMUSR_B			(1 << 11)
#define MMU_MMUSR_G			(1 << 10)
#define MMU_MMUSR_U1		(1 << 9)
#define MMU_MMUSR_U0		(1 << 8)
#define MMU_MMUSR_S			(1 << 7)
#define MMU_MMUSR_CM		(1 << 6) | ( 1 << 5)
#define MMU_MMUSR_M			(1 << 4)
#define MMU_MMUSR_W			(1 << 2)
#define MMU_MMUSR_T			(1 << 1)
#define MMU_MMUSR_R			(1 << 0)

struct mmu_atc_line	{
	int	v, umode, g, s, cm, m, w, r, fc2;
	uaecptr phys, log;
};

#define ATC_SIZE	128

extern struct mmu_atc_line atc[ATC_SIZE];

#define TTR_I0	4
#define TTR_I1	5
#define TTR_D0	6
#define TTR_D1	7

#define TTR_NO_MATCH	0
#define TTR_NO_WRITE	1
#define TTR_OK_MATCH	2

static inline void mmu_set_tc(uae_u16 tc)
{
	extern void activate_debugger (void);
	regs.tc = tc;

#if 0
		if (tc & 0x8000)
		{
			uaecptr nextpc;
			m68k_disasm(stdout, m68k_getpc(), &nextpc, 10);
		}
#endif

	regs.mmu_enabled = tc & 0x8000 ? 1 : 0;
	regs.mmu_pagesize = tc & 0x4000 ? MMU_PAGE_8KB : MMU_PAGE_4KB;

//		D(bug("MMU: enabled=%d page=%d\n", regs.mmu_enabled, regs.mmu_pagesize));
}

extern void mmu_make_transparent_region(uaecptr baseaddr, uae_u32 size, int datamode);

static inline void mmu_set_ttr(int regno, uae_u32 val)
{
	uae_u32 * ttr;
	switch(regno)	{
		case TTR_I0:	ttr = &regs.itt0;	break;
		case TTR_I1:	ttr = &regs.itt1;	break;
		case TTR_D0:	ttr = &regs.dtt0;	break;
		case TTR_D1:	ttr = &regs.dtt1;	break;
		default: abort();
	}
	*ttr = val;
}

static inline void mmu_set_mmusr(uae_u32 val)
{
	regs.mmusr = val;
}

static inline void mmu_set_root_pointer(int regno, uae_u32 val)
{
	uae_u32 * rp;
	switch(regno)	{
		case 0x806: rp = &regs.urp; break;
		case 0x807: rp = &regs.srp; break;
		default: abort();
	}
	*rp = val;
}

#define FC_DATA regs.s ? 5 : 1
#define FC_INST regs.s ? 6 : 2

extern uaecptr mmu_translate(uaecptr addr,
		int fc,
		int write,
		int size,
		int test
		) REGPARAM;

extern uae_u32 mmu_get_unaligned(uaecptr addr, int fc, int size);
extern void mmu_put_unaligned(uaecptr addr, uae_u32 data, int fc, int size);

#endif /* CPUMMU_H */
