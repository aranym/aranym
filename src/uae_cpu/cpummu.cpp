/*
 * cpummu.cpp -  MMU emulation
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

#include "sysdeps.h"

#include "cpummu.h"
#include "memory.h"
#include "newcpu.h"
#define DEBUG 0
#include "debug.h"

#define DBG_MMU_VERBOSE	1
#define DBG_MMU_SANITY	1

#ifdef SMALL_ATC
#define DISABLE_ATC
struct mmu_atc_line super_read_qatc;
bool super_read_qatc_valid = false;
struct mmu_atc_line super_write_qatc;
bool super_write_qatc_valid = false;
struct mmu_atc_line super_instr_qatc;
bool super_instr_qatc_valid = false;
struct mmu_atc_line user_read_qatc;
bool user_read_qatc_valid = false;
struct mmu_atc_line user_write_qatc;
bool user_write_qatc_valid = false;
struct mmu_atc_line user_instr_qatc;
bool user_instr_qatc_valid = false;

#ifdef SMALL_ATC_STATS
static unsigned int qatc_hits;
#endif

static inline unsigned int mmu_check_qatc(uaecptr page_frame, int supervisor, int write, int datamode)
{
	if (supervisor) {
		if (datamode) {
			if (write) {
				if (super_write_qatc_valid) {
					if (page_frame == super_write_qatc.log) {
#ifdef SMALL_ATC_STATS
						qatc_hits++;
#endif
						return 2;
					}
				} else return 0;
			} else {
				if (super_read_qatc_valid) {
					if (page_frame == super_read_qatc.log) {
#ifdef SMALL_ATC_STATS
						qatc_hits++;
#endif
						return 1;
					}
				} else return 0;
			}
		} else {
				if (super_instr_qatc_valid) {
					if (page_frame == super_instr_qatc.log) {
#ifdef SMALL_ATC_STATS
						qatc_hits++;
#endif
						return 3;
					}
				}
		}
	} else {
		if (datamode) {
			if (write) {
				if (user_write_qatc_valid) {
					if (page_frame == user_write_qatc.log) {
#ifdef SMALL_ATC_STATS
						qatc_hits++;
#endif
						return 5;
					}
				} else return 0;
			} else {
				if (user_read_qatc_valid) {
					if (page_frame == user_read_qatc.log) {
#ifdef SMALL_ATC_STATS
						qatc_hits++;
#endif
						return 4;
					}
				} else return 0;
			}
		} else {
				if (user_instr_qatc_valid) {
					if (page_frame == user_instr_qatc.log) {
#ifdef SMALL_ATC_STATS
						qatc_hits++;
#endif
						return 6;
					}
				} else return 0;
		}
	}
	return 0;
}

static inline void mmu_flush_qatc()
{
	super_read_qatc_valid = false;
	super_write_qatc_valid = false;
	super_instr_qatc_valid = false;
	user_read_qatc_valid = false;
	user_write_qatc_valid = false;
	user_instr_qatc_valid = false;
}
#endif

static void mmu_dump_ttr(const char * label, uae_u32 ttr)
{
	DUNUSED(label);
	uae_u32 from_addr, to_addr;

	from_addr = ttr & MMU_TTR_LOGICAL_BASE;
	to_addr = (ttr & MMU_TTR_LOGICAL_MASK) << 8;

	D(bug("%s: [%08lx] %08lx - %08lx enabled=%d supervisor=%d wp=%d cm=%02d",
			label, ttr,
			from_addr, to_addr,
			ttr & MMU_TTR_BIT_ENABLED ? 1 : 0,
			(ttr & (MMU_TTR_BIT_SFIELD_ENABLED | MMU_TTR_BIT_SFIELD_SUPER)) >> MMU_TTR_SFIELD_SHIFT,
			ttr & MMU_TTR_BIT_WRITE_PROTECT ? 1 : 0,
			(ttr & MMU_TTR_CACHE_MASK) >> MMU_TTR_CACHE_SHIFT
		  ));
}

void mmu_make_transparent_region(uaecptr baseaddr, uae_u32 size, int datamode)
{
	uae_u32 * ttr;
	uae_u32 * ttr0 = datamode ? &regs.dtt0 : &regs.itt0;
	uae_u32 * ttr1 = datamode ? &regs.dtt1 : &regs.itt1;

	if ((*ttr1 & MMU_TTR_BIT_ENABLED) == 0)
		ttr = ttr1;
	else if ((*ttr0 & MMU_TTR_BIT_ENABLED) == 0)
		ttr = ttr0;
	else
		return;

	*ttr = baseaddr & MMU_TTR_LOGICAL_BASE;
	*ttr |= ((baseaddr + size - 1) & MMU_TTR_LOGICAL_BASE) >> 8;
	*ttr |= MMU_TTR_BIT_ENABLED;

	D(bug("MMU: map transparent mapping of %08x", *ttr));
}

/* check if an address matches a ttr */
static int mmu_do_match_ttr(uae_u32 ttr, uaecptr addr, int super, int test)
{
	if (ttr & MMU_TTR_BIT_ENABLED)	{	/* TTR enabled */
		uae_u8 msb, mask;

		msb = ((addr ^ ttr) & MMU_TTR_LOGICAL_BASE) >> 24;
		mask = (ttr & MMU_TTR_LOGICAL_MASK) >> 16;

		if (!(msb & ~mask)) {

			if ((ttr & MMU_TTR_BIT_SFIELD_ENABLED) == 0) {
				if (((ttr & MMU_TTR_BIT_SFIELD_SUPER) == 0) != (super == 0)) {
					return TTR_NO_MATCH;
				}
			}

			if (test) {
				regs.mmusr = MMU_MMUSR_T | MMU_MMUSR_R;
			}

			return (ttr & MMU_TTR_BIT_WRITE_PROTECT) ? TTR_NO_WRITE : TTR_OK_MATCH;
		}
	}
	return TTR_NO_MATCH;
}

static inline int mmu_match_ttr(uaecptr addr, int datamode, int super, int test)
{
	int res;

	if (datamode) {
		res = mmu_do_match_ttr(regs.dtt0, addr, super, test);
		if (res != TTR_NO_MATCH)
			res = mmu_do_match_ttr(regs.dtt1, addr, super, test);
	} else {
		res = mmu_do_match_ttr(regs.itt0, addr, super, test);
		if (res != TTR_NO_MATCH)
			res = mmu_do_match_ttr(regs.itt1, addr, super, test);
	}
	return res;
}

struct mmu_atc_line atc[2][ATC_SIZE];

# ifdef ATC_STATS
static unsigned int mmu_atc_hits[ATC_SIZE];
# endif

#if DEBUG
/* {{{ mmu_dump_table */
static void mmu_dump_table(const char * label, uaecptr root_ptr)
{
	DUNUSED(label);
	const int ROOT_TABLE_SIZE = 128,
		PTR_TABLE_SIZE = 128,
		PAGE_TABLE_SIZE = 64,
		ROOT_INDEX_SHIFT = 25,
		PTR_INDEX_SHIFT = 18;
	// const int PAGE_INDEX_SHIFT = 12;
	int root_idx, ptr_idx, page_idx;
	uae_u32 root_des, ptr_des, page_des;
	uaecptr ptr_des_addr, page_addr,
		root_log, ptr_log, page_log;

	D(bug("%s: root=%lx", label, root_ptr));

	for (root_idx = 0; root_idx < ROOT_TABLE_SIZE; root_idx++) {
		root_des = phys_get_long(root_ptr + root_idx);

		if ((root_des & 2) == 0)
			continue;	/* invalid */

		D(bug("ROOT: %03d U=%d W=%d UDT=%02d", root_idx,
				root_des & 8 ? 1 : 0,
				root_des & 4 ? 1 : 0,
				root_des & 3
			  ));

		root_log = root_idx << ROOT_INDEX_SHIFT;

		ptr_des_addr = root_des & MMU_ROOT_PTR_ADDR_MASK;

		for (ptr_idx = 0; ptr_idx < PTR_TABLE_SIZE; ptr_idx++) {
			struct {
				uaecptr	log, phys;
				int start_idx, n_pages;	/* number of pages covered by this entry */
				uae_u32 match;
			} page_info[PAGE_TABLE_SIZE];
			int n_pages_used;

			ptr_des = phys_get_long(ptr_des_addr + ptr_idx);
			ptr_log = root_log | (ptr_idx << PTR_INDEX_SHIFT);

			if ((ptr_des & 2) == 0)
				continue; /* invalid */

			page_addr = ptr_des & (regs.mmu_pagesize ? MMU_PTR_PAGE_ADDR_MASK_8 : MMU_PTR_PAGE_ADDR_MASK_4);

			n_pages_used = -1;
			for (page_idx = 0; page_idx < PAGE_TABLE_SIZE; page_idx++) {

				page_des = phys_get_long(page_addr + page_idx);
				page_log = ptr_log | (page_idx << 2);		// ??? PAGE_INDEX_SHIFT

				switch (page_des & 3) {
					case 0: /* invalid */
						continue;
					case 1: case 3: /* resident */
					case 2: /* indirect */
						if (n_pages_used == -1 || page_info[n_pages_used].match != page_des) {
							/* use the next entry */
							n_pages_used++;

							page_info[n_pages_used].match = page_des;
							page_info[n_pages_used].n_pages = 1;
							page_info[n_pages_used].start_idx = page_idx;
							page_info[n_pages_used].log = page_log;
						} else {
							page_info[n_pages_used].n_pages++;
						}
						break;
				}
			}

			if (n_pages_used == -1)
				continue;

			D(bug(" PTR: %03d U=%d W=%d UDT=%02d", ptr_idx,
				ptr_des & 8 ? 1 : 0,
				ptr_des & 4 ? 1 : 0,
				ptr_des & 3
			  ));


			for (page_idx = 0; page_idx <= n_pages_used; page_idx++) {
				page_des = page_info[page_idx].match;

				if ((page_des & MMU_PDT_MASK) == 2) {
					D(bug("  PAGE: %03d-%03d log=%08lx INDIRECT --> addr=%08lx",
							page_info[page_idx].start_idx,
							page_info[page_idx].start_idx + page_info[page_idx].n_pages - 1,
							page_info[page_idx].log,
							page_des & MMU_PAGE_INDIRECT_MASK
						  ));

				} else {
					D(bug("  PAGE: %03d-%03d log=%08lx addr=%08lx UR=%02d G=%d U1/0=%d S=%d CM=%d M=%d U=%d W=%d",
							page_info[page_idx].start_idx,
							page_info[page_idx].start_idx + page_info[page_idx].n_pages - 1,
							page_info[page_idx].log,
							page_des & (regs.mmu_pagesize ? MMU_PAGE_ADDR_MASK_8 : MMU_PAGE_ADDR_MASK_4),
							(page_des & (regs.mmu_pagesize ? MMU_PAGE_UR_MASK_8 : MMU_PAGE_UR_MASK_4)) >> MMU_PAGE_UR_SHIFT,
							page_des & MMU_DES_GLOBAL ? 1 : 0,
							(page_des & MMU_TTR_UX_MASK) >> MMU_TTR_UX_SHIFT,
							page_des & MMU_DES_SUPER ? 1 : 0,
							(page_des & MMU_TTR_CACHE_MASK) >> MMU_TTR_CACHE_SHIFT,
							page_des & MMU_DES_MODIFIED ? 1 : 0,
							page_des & MMU_DES_USED ? 1 : 0,
							page_des & MMU_DES_WP ? 1 : 0
						  ));
				}
			}
		}

	}
}
/* }}} */
#endif

/* {{{ mmu_dump_atc */
void mmu_dump_atc(void)
{
	int i, j;
	for (i = 0; i < 2; i++) {
		for (j = 0; j < ATC_SIZE; j++) {
			if (atc[i][j].tag == (uae_u16)~0)
				continue;
			D(bug("ATC[%02d] G=%d TT=%d M=%d WP=%d VD=%d VI=%d tag=%08x --> phys=%08x",
				j, atc[i][j].global, atc[i][j].tt, atc[i][j].modified,
				atc[i][j].write_protect, atc[i][j].valid_data, atc[i][j].valid_inst,
				atc[i][j].tag, atc[i][j].phys));
		}
	}
}
/* }}} */

/* {{{ mmu_dump_tables */
void mmu_dump_tables(void)
{
	D(bug("URP: %08x   SRP: %08x  MMUSR: %x  TC: %x", regs.urp, regs.srp, regs.mmusr, regs.tc));
	mmu_dump_ttr("DTT0", regs.dtt0);
	mmu_dump_ttr("DTT1", regs.dtt1);
	mmu_dump_ttr("ITT0", regs.itt0);
	mmu_dump_ttr("ITT1", regs.itt1);
	mmu_dump_atc();
	//mmu_dump_table("SRP", regs.srp);
}
/* }}} */

static void phys_dump_mem (uaecptr addr, int lines)
{
	for (;lines--;) {
		int i;
		D(bug("%08lx:", addr));
		for (i = 0; i < 16; i++) {
			D(bug("%04x", phys_get_word(addr))); addr += 2;
		}
	}
}

static uaecptr REGPARAM2 mmu_do_translate(uaecptr theaddr, int fc, int write, int size, int test, int datamode, int supervisor);

static uaecptr mmu_bus_error(uae_u16 ssw, uaecptr theaddr, int fc, int write, int size, int test, int datamode, int supervisor)
{
	ssw |= (1 << 10);	/* ATC */
	if (!write)
		ssw |= (1 << 8);

	if (!datamode)  {
		if (supervisor)
			ssw |= 0x6;
		else
			ssw |= 0x2;
	}

	ssw |= fc & 7; /* Copy TM */
	switch (size) {
	case sz_byte:
		ssw |= 1 << 5;
		break;
	case sz_word:
		ssw |= 2 << 5;
		break;
	}

	regs.mmu_fault_addr = theaddr;
	regs.mmu_ssw = ssw;

	D(bug("BUS ERROR: fc=%d w=%d log=%08x ssw=%04x", fc, write, theaddr, ssw));

	if ((test & MMU_TEST_NO_BUSERR) == 0) {
		THROW(2);
	}
	return 0;
}

uaecptr REGPARAM2 mmu_translate(uaecptr theaddr, int fc, int write, int size, int test)
{
	struct mmu_atc_line *l;
	int supervisor, datamode;
	int idx, res;
	uae_u32 desc;
	uae_u16 tag;

	supervisor = (fc & 4) != 0;

#if 0
	switch (fc) {
		case 0: /* data cache push */
		case 1:
		case 3:
		case 5:
			datamode = 1;
			break;
		case 2:
		case 4:
		case 6:
			datamode = 0;
			break;
		case 7:
		default:
			panicbug("FC=%d should not happen", fc);
			abort();
	}
#else
	datamode = (fc == 0) ? 1 : (fc % 2);
#endif
	idx = ((theaddr >> 12) ^ (theaddr >> (32 - ATC_SIZE_LOG))) % ATC_SIZE;
	tag = theaddr >> (ATC_SIZE_LOG + 12);
	l = &atc[supervisor][idx];
	if (l->tag == tag) {
		if (datamode ? l->valid_data : l->valid_inst) {
			if (write) {
				if (l->write_protect) {
					D(bug("MMU: write protected (via atc) %lx", theaddr));
					//bug("MMU: write protected (via atc) %lx", theaddr);
					return mmu_bus_error(0, theaddr, fc, write,
							     size, test, datamode, supervisor);
				}
				if (!l->modified)
					goto lookup;
			}
			return l->phys | (theaddr & 0xfff);
		} else {
			if (!l->tt) {
#if DBG_MMU_VERBOSE
				D(bug("MMU: non-resident page!"));
#endif
				//bug("MMU: non-resident page! %lx", theaddr);
				return mmu_bus_error(0, theaddr, fc, write, size,
						     test, datamode, supervisor);
			}
		}
	}
lookup:
	l->tag = tag;

//	root_ptr = supervisor ? regs.srp : regs.urp;

	/* check ttr0 */

	/* TTR operate independently from the enable bit, so we can just ignore it if the MMU
	 * is not enabled to get better performance.
	 * But AmigaOS depends on PTEST to operate when the MMU is disabled;
	 * it uses the result in the ssw to detect a working MMU and then enables the MMU */
	res = mmu_match_ttr(theaddr, datamode, supervisor, test);
	if (res != TTR_NO_MATCH) {
		l->phys = theaddr & ~0xfff;
		l->tt = 1;
		l->modified = 1;
		if (datamode) {
			l->valid_data = 1;
			l->valid_inst = mmu_match_ttr(theaddr, 0, supervisor, test) != TTR_NO_MATCH;
		} else {
			l->valid_inst = 1;
			l->valid_data = mmu_match_ttr(theaddr, 1, supervisor, test) != TTR_NO_MATCH;
		}
		l->write_protect = (res == TTR_NO_WRITE);
		if (l->write_protect && write) {
			D(bug("MMU: write protected (via ttr) %lx", theaddr));
			return mmu_bus_error(0, theaddr, fc, write,
					     size, test, datamode, supervisor);
		}
		return theaddr;
	}

	if (!regs.mmu_enabled) {
		l->phys = theaddr & ~0xfff;
		l->tt = 0;
		l->modified = 1;
		l->valid_data = 1;
		l->valid_inst = 1;
		l->write_protect = 0;

		return theaddr;
	}

	desc = mmu_do_translate(theaddr, fc, write, size, test, datamode, supervisor);
	//bug("translate: %x,%u,%u -> %x", theaddr, fc, write, desc);
	if (!desc || (!supervisor && desc & MMU_MMUSR_S)) {
		l->tt = 0;
		l->valid_data = 0;
		l->valid_inst = 0;
		//bug("MMU: page fault %lx,%u,%u", desc, fc, write);
		return mmu_bus_error(0, theaddr, fc, write,
				     size, test, datamode, supervisor);
	}
	l->phys = desc & ~0xfff;
	l->tt = 0;
	l->modified = (desc & MMU_MMUSR_M) != 0;
	l->valid_data = 1;
	l->valid_inst = 1;
	l->write_protect = (desc & MMU_MMUSR_W) != 0;

	return l->phys | (theaddr & 0xfff);
}

static uaecptr REGPARAM2 mmu_do_translate(uaecptr theaddr, int fc, int write, int size, int test, int datamode, int supervisor)
{
	uae_u32
		atc_hit_addr = 0,
		root_ptr,
		root_des, root_des_addr,
		ptr_des = 0, ptr_des_addr = 0,
		page_des = 0, page_des_addr = 0,
		fslw = 0;
	uae_u8	ri, pi, pgi, wp = 0;
	uae_u16	ssw = 0;
	uae_u32 page_frame;
	int n_table_searches = 0;

//	if (theaddr == 0x40000000) test |= MMU_TEST_VERBOSE;

	root_ptr = supervisor ? regs.srp : regs.urp;

	ri = (theaddr & 0xfe000000) >> 25;
	pi = (theaddr & 0x01fc0000) >> 18;
	if (regs.mmu_pagesize == MMU_PAGE_8KB) {
		pgi = (theaddr & 0x3e000) >> 13;
		page_frame = theaddr & 0xffffe000;
#ifdef SMALL_ATC
		if (!test) {
			switch (mmu_check_qatc(page_frame, supervisor, write, datamode)) {
				case 0: break;
				case 1: return super_read_qatc.phys | (theaddr & 0x1fff);
				case 2: return super_write_qatc.phys | (theaddr & 0x1fff);
				case 3: return super_instr_qatc.phys | (theaddr & 0x1fff);
				case 4: return user_read_qatc.phys | (theaddr & 0x1fff);
				case 5: return user_write_qatc.phys | (theaddr & 0x1fff);
				case 6: return user_instr_qatc.phys | (theaddr & 0x1fff);
			}
		}
#endif
	} else {
		pgi = (theaddr & 0x3f000) >> 12;
		page_frame = theaddr & 0xfffff000;
#ifdef SMALL_ATC
		if (!test) {
			switch (mmu_check_qatc(page_frame, supervisor, write, datamode)) {
				case 0: break;
				case 1: return super_read_qatc.phys | (theaddr & 0x0fff);
				case 2: return super_write_qatc.phys | (theaddr & 0x0fff);
				case 3: return super_instr_qatc.phys | (theaddr & 0x0fff);
				case 4: return user_read_qatc.phys | (theaddr & 0x0fff);
				case 5: return user_write_qatc.phys | (theaddr & 0x0fff);
				case 6: return user_instr_qatc.phys | (theaddr & 0x0fff);
			}
		}
#endif
	}

	if (test & MMU_TEST_FORCE_TABLE_SEARCH)
		goto table_search;
table_search:

	if (n_table_searches++ > 3) {
		panicbug("MMU: apparently looping during table search.");
		abort();
	}

#if DBG_MMU_VERBOSE
	if (test & MMU_TEST_VERBOSE)
	D(bug("MMU: table search for logical=%08x ri=%02x pi=%02x pgi=%03x page_frame=%08x root_ptr=%08x",
			theaddr, ri, pi, pgi, page_frame, root_ptr));
#endif

	/* root descriptor */
	root_des_addr = (root_ptr & MMU_ROOT_PTR_ADDR_MASK) | (ri << 2);

#if DBG_MMU_SANITY
	if (!phys_valid_address(root_des_addr, false, 4)) {
		regs.mmusr = MMU_MMUSR_B;
		return mmu_bus_error(ssw, theaddr, fc, write, size, test, datamode, supervisor);
	}
#endif

	root_des = phys_get_long(root_des_addr);

#if DBG_MMU_VERBOSE
	if (test & MMU_TEST_VERBOSE) {
	D(bug("MMU: root_des_addr = %lx  val=%08x", root_des_addr, root_des));
	//phys_dump_mem(root_ptr, 128 / 16);
	}
#endif

	switch (root_des & MMU_UDT_MASK) {
		case 0x0:
		case 0x1:
			D(bug("MMU: invalid root descriptor for %lx", theaddr));
			goto make_non_resident_atc;
	}

	wp |= root_des;
	/* touch the page */
	if ((root_des & MMU_DES_USED) == 0) {
		root_des |= MMU_DES_USED;
		phys_put_long(root_des_addr, root_des);
	}


	ptr_des_addr = (root_des & MMU_ROOT_PTR_ADDR_MASK) | (pi << 2);
#if DBG_MMU_SANITY
	if (!phys_valid_address(ptr_des_addr, false, 4)) {
		regs.mmusr = MMU_MMUSR_B;
		return mmu_bus_error(ssw, theaddr, fc, write, size, test, datamode, supervisor);
	}
#endif

	ptr_des = phys_get_long(ptr_des_addr);
#if DBG_MMU_VERBOSE
	if (test & MMU_TEST_VERBOSE)
	D(bug("MMU: ptr_des_addr = %lx  val=%08x", ptr_des_addr, ptr_des));
	//phys_dump_mem(ptr_des_addr, 128 / 16);
#endif

	switch (ptr_des & MMU_UDT_MASK) {
		case 0x0:
		case 0x1:
			D(bug("MMU: invalid ptr descriptor for %lx", theaddr));
			goto make_non_resident_atc;
	}
	wp |= ptr_des;
	/* touch */
	if ((ptr_des & MMU_DES_USED) == 0) {
		ptr_des |= MMU_DES_USED;
		phys_put_long(ptr_des_addr, ptr_des);
	}

	if (regs.mmu_pagesize == MMU_PAGE_8KB)
		page_des_addr = (ptr_des & MMU_PTR_PAGE_ADDR_MASK_8) | (pgi << 2);
	else
		page_des_addr = (ptr_des & MMU_PTR_PAGE_ADDR_MASK_4) | (pgi << 2);

get_page_descriptor:
#if DBG_MMU_SANITY
	if (!phys_valid_address(page_des_addr, false, 4)) {
		regs.mmusr = MMU_MMUSR_B;
		return mmu_bus_error(ssw, theaddr, fc, write, size, test, datamode, supervisor);
	}
#endif

	page_des = phys_get_long(page_des_addr);
#if DBG_MMU_VERBOSE
	if (test & MMU_TEST_VERBOSE) {
		D(bug("MMU: page_des_addr = %lx  val=%08x", page_des_addr, page_des));
		phys_dump_mem(page_des_addr, 64 / 16);
	}
#endif

	switch (page_des & MMU_PDT_MASK) {
		case 0x0:
			D(bug("MMU: invalid page descriptor log=%08lx page_des=%08lx @%08lx", theaddr, page_des, page_des_addr));
			goto make_non_resident_atc;
		case 0x1:
		case 0x3:
			/* resident page */
			break;
		case 0x2:
		default:
			/* indirect */
			if (fslw & (1 << 10)) {
				D(bug("MMU: double indirect descriptor log=%lx descriptor @ %lx", theaddr, page_des_addr));
				goto make_non_resident_atc;
			}
			wp |= page_des;
			if ((page_des & MMU_DES_USED) == 0) {
				page_des |= MMU_DES_USED;
				phys_put_long(page_des_addr, page_des);
			}
			page_des_addr = page_des & MMU_PAGE_INDIRECT_MASK;
			fslw |= (1 << 10); /* IL - in case a fault occurs later, tag it as indirect */
			goto get_page_descriptor;
	}

	page_des |= wp & MMU_DES_WP;
	if (write) {
		if (page_des & MMU_DES_WP) {
			if ((page_des & MMU_DES_USED) == 0) {
				page_des |= MMU_DES_USED;
				phys_put_long(page_des_addr, page_des);
			}
			return mmu_bus_error(ssw, theaddr, fc, write, size, test, datamode, supervisor);
		}
		if ((page_des & (MMU_DES_USED|MMU_DES_MODIFIED)) !=
		    (MMU_DES_USED|MMU_DES_MODIFIED)) {
			page_des |= MMU_DES_USED|MMU_DES_MODIFIED;
			phys_put_long(page_des_addr, page_des);
		}
	} else {
		if ((page_des & MMU_DES_USED) == 0) {
			page_des |= MMU_DES_USED;
			phys_put_long(page_des_addr, page_des);
		}
	}
	if (test) {
		regs.mmusr |= page_des & (~0xfff|MMU_MMUSR_G|MMU_MMUSR_Ux|MMU_MMUSR_S|
					  MMU_MMUSR_CM|MMU_MMUSR_M|MMU_MMUSR_W);
		regs.mmusr |= MMU_MMUSR_R;
	}
#ifdef SMALL_ATC
	if (supervisor) {
		if (datamode) {
			if (write) {
				super_write_qatc_valid = true;
				super_write_qatc.log = page_frame;
				super_write_qatc.phys = page_des & (regs.mmu_pagesize ? MMU_PAGE_ADDR_MASK_8 : MMU_PAGE_ADDR_MASK_4);
			} else {
				super_read_qatc_valid = true;
				super_read_qatc.log = page_frame;
				super_read_qatc.phys = page_des & (regs.mmu_pagesize ? MMU_PAGE_ADDR_MASK_8 : MMU_PAGE_ADDR_MASK_4);
			}
		} else {
				super_instr_qatc_valid = true;
				super_instr_qatc.log = page_frame;
				super_instr_qatc.phys = page_des & (regs.mmu_pagesize ? MMU_PAGE_ADDR_MASK_8 : MMU_PAGE_ADDR_MASK_4);
		}
	} else {
		if (datamode) {
			if (write) {
				user_write_qatc_valid = true;
				user_write_qatc.log = page_frame;
				user_write_qatc.phys = page_des & (regs.mmu_pagesize ? MMU_PAGE_ADDR_MASK_8 : MMU_PAGE_ADDR_MASK_4);
			} else {
				user_read_qatc_valid = true;
				user_read_qatc.log = page_frame;
				user_read_qatc.phys = page_des & (regs.mmu_pagesize ? MMU_PAGE_ADDR_MASK_8 : MMU_PAGE_ADDR_MASK_4);
			}
		} else {
				user_instr_qatc_valid = true;
				user_instr_qatc.log = page_frame;
				user_instr_qatc.phys = page_des & (regs.mmu_pagesize ? MMU_PAGE_ADDR_MASK_8 : MMU_PAGE_ADDR_MASK_4);
		}
	}
#endif

#if 0
	if (atc_hit_addr != 0 && atc_hit_addr != phys_addr) {
		write_log("MMU: ERROR! ATC hit does not match table search! for %lx --> %lx (atc gave %lx)\n",
				theaddr, phys_addr, atc_hit_addr);
		activate_debugger();
	}
#endif
	return page_des;

make_non_resident_atc:
	if (test && (wp & MMU_MMUSR_W))
		regs.mmusr |= MMU_MMUSR_W;
	return 0;
}

uae_u32 mmu_get_unaligned (uaecptr addr, int fc, int size)
{
	uaecptr physaddr;
	int size1 = size - (addr & (size - 1));
	volatile uae_u32 result;

	SAVE_EXCEPTION;
	TRY(prb) {
		physaddr = mmu_translate(addr, fc, 0, size1 == 1 ? sz_byte : sz_word, 0);
		switch (size1) {
		case 1:
			result = phys_get_byte(physaddr);
			break;
		case 2:
			result = phys_get_word (physaddr);
			break;
		default:
			result = (uae_u32)phys_get_byte (physaddr) << 16;
			result |= phys_get_word (physaddr + 1);
			break;
		}
	}
	CATCH(prb) {
		RESTORE_EXCEPTION;
		regs.mmu_fault_addr = addr;
		regs.mmu_ssw = (regs.mmu_ssw & ~(3 << 5)) | ((size & 3) << 5);
		THROW_AGAIN(prb);
	}
	TRY(prb2) {
		physaddr = mmu_translate(addr + size1, fc, 0, (size - size1) == 1 ? sz_byte : sz_word, 0);
		result <<= (size - size1) * 8;
		switch (size - size1) {
		case 1:
			result |= phys_get_byte(physaddr);
			break;
		case 2:
			result |= phys_get_word (physaddr);
			break;
		case 3:
			result |= (uae_u32)phys_get_byte (physaddr) << 16;
			result |= phys_get_word (physaddr + 1);
			break;
		}
	}
	CATCH(prb2) {
		RESTORE_EXCEPTION;
		regs.mmu_fault_addr = addr;
		regs.mmu_ssw = (regs.mmu_ssw & ~(3 << 5)) | ((size & 3) << 5);
		regs.mmu_ssw |= (1 << 11);
		THROW_AGAIN(prb2);
	}
	RESTORE_EXCEPTION;
	return result;
}

void mmu_put_unaligned (uaecptr addr, uae_u32 data, int fc, int size)
{
	uaecptr physaddr;
	int size1 = size - (addr & (size - 1));
	uae_u32 data1 = data >> 8 * (size - size1);

	SAVE_EXCEPTION;
	TRY(prb) {
		physaddr = mmu_translate(addr, fc, 1, size1 == 1 ? sz_byte : sz_word, 0);
		switch (size1) {
		case 1:
			phys_put_byte(physaddr, data1);
			break;
		case 2:
			phys_put_word (physaddr, data1);
			break;
		case 3:
			phys_put_byte (physaddr, data1 >> 16);
			phys_put_word (physaddr + 1, data1);
			break;
		}
	}
	CATCH(prb) {
		RESTORE_EXCEPTION;
		regs.mmu_fault_addr = addr;
		regs.mmu_ssw = (regs.mmu_ssw & ~(3 << 5)) | ((size & 3) << 5);
		regs.wb3_data = data;
		regs.wb3_status = (regs.mmu_ssw & 0x7f) | 0x80;
		THROW_AGAIN(prb);
	}
	TRY(prb2) {
		physaddr = mmu_translate(addr + size1, fc, 1, (size - size1) == 1 ? sz_byte : sz_word, 0);
		switch (size - size1) {
		case 1:
			phys_put_byte(physaddr, data);
			break;
		case 2:
			phys_put_word (physaddr, data);
			break;
		case 3:
			phys_put_byte (physaddr, data >> 16);
			phys_put_word (physaddr + 1, data);
			break;
		}
	}
	CATCH(prb2) {
		RESTORE_EXCEPTION;
		regs.mmu_fault_addr = addr;
		regs.mmu_ssw = (regs.mmu_ssw & ~(3 << 5)) | ((size & 3) << 5);
		regs.mmu_ssw |= (1 << 11);
		regs.wb3_data = data;
		regs.wb3_status = (regs.mmu_ssw & 0x7f) | 0x80;
		THROW_AGAIN(prb2);
	}
	RESTORE_EXCEPTION;
}

void mmu_op(uae_u32 opcode, uae_u16 extra)
{
	int super = (regs.dfc & 4) != 0;
	DUNUSED(extra);
	if ((opcode & 0xFE0) == 0x0500) {
		struct mmu_atc_line *l;
		int i, j, regno, glob;
		D(didflush = 0);
		uae_u32 addr;
		/* PFLUSH */
		regno = opcode & 7;
		glob = opcode & 8;

		if (opcode & 16) {
			//bug("pflusha(%u,%u,%x)", glob, regs.dfc, regs.pc);
			for (i = 0; i < 2; i++) {
				l = atc[i];
				for (j = 0; j < ATC_SIZE; l++, j++) {
					if (glob || !l->global)
						l->tag = ~0;
				}
			}
		} else {
			l = atc[super];
			addr = m68k_areg(regs, regno);
			//bug("pflush(%u,%u,%x)", glob, regs.dfc, addr);
			i = ((addr >> 12) ^ (addr >> (32 - ATC_SIZE_LOG))) % ATC_SIZE;
			if (glob || !l[i].global)
				l[i].tag = ~0;
			if (regs.mmu_pagesize != MMU_PAGE_4KB) {
				i ^= 1;
				if (glob || !l[i].global)
					l[i].tag = ~0;
			}
		}
		flush_internals();
#ifdef USE_JIT
		flush_icache(0);
#endif
#ifdef SMALL_ATC
		mmu_flush_qatc();
# ifdef SMALL_ATC_STATS
		panicbug("QATC hits: %d", qatc_hits);
		qatc_hits = 0;
# endif
#endif
	} else if ((opcode & 0x0FD8) == 0x548) {
		struct mmu_atc_line *l;
		int write, regno, i;
		uae_u32 addr;

		l = atc[super];
		regno = opcode & 7;
		write = (opcode & 32) == 0;
		addr = m68k_areg(regs, regno);
		//bug("ptest(%u,%u,%x)", write, regs.dfc, addr);
		D(bug("PTEST%c (A%d) %08x DFC=%d", write ? 'W' : 'R', regno, addr, regs.dfc));
		i = ((addr >> 12) ^ (addr >> (32 - ATC_SIZE_LOG))) % ATC_SIZE;
		l[i].tag = ~0;
		if (regs.mmu_pagesize != MMU_PAGE_4KB)
			l[i ^ 1].tag = ~0;
#ifdef SMALL_ATC
		mmu_flush_qatc();
# ifdef SMALL_ATC_STATS
		panicbug("QATC hits: %d", qatc_hits);
		qatc_hits = 0;
# endif
#endif
		mmu_set_mmusr(0);
		mmu_do_translate(addr, regs.dfc, write, sz_byte, MMU_TEST_FORCE_TABLE_SEARCH | MMU_TEST_NO_BUSERR,
				 1, super);
		D(bug("PTEST result: mmusr %08x", regs.mmusr));
	} else
		op_illg (opcode);
}

void mmu_reset(void)
{
	int s, i;

	for (s = 0; s < 2; s++) {
		for (i = 0; i < ATC_SIZE; i++) {
			atc[s][i].tag = ~0;
		}
	}
	regs.urp = regs.srp = 0;
	regs.itt0 = regs.itt0 = 0;
	regs.dtt0 = regs.dtt0 = 0;
	regs.mmusr = 0;
}
