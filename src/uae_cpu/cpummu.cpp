#include "sysdeps.h"

#include "cpummu.h"
#include "memory.h"
#include "newcpu.h"
#define DEBUG 1
#include "debug.h"

#define DBG_MMU_VERBOSE	1
#define DBG_MMU_SANITY	0

static void mmu_dump_ttr(const char * label, uae_u32 ttr)
{
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

extern void mmu_make_transparent_region(uaecptr baseaddr, uae_u32 size, int datamode)
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
static __inline__ int mmu_match_ttr(uae_u32 ttr, uaecptr addr, int write, int test)
{
	if (ttr & MMU_TTR_BIT_ENABLED)	{	/* TTR enabled */
		uae_u8 msb, match, mask;
		
		msb = (addr & MMU_TTR_LOGICAL_BASE) >> 24;
		match = (ttr & MMU_TTR_LOGICAL_BASE) >> 24;
		mask = (ttr & MMU_TTR_LOGICAL_MASK) >> 16;
		
		if ((msb & ~mask) == match) {

			if ((ttr & MMU_TTR_BIT_SFIELD_ENABLED) == 0)	{
				if ((ttr & MMU_TTR_BIT_SFIELD_SUPER) && !regs.s)	{
					return TTR_NO_MATCH;
				}
				if ((ttr & MMU_TTR_BIT_SFIELD_SUPER) == 0 && regs.s)	{
					return TTR_NO_MATCH;
				}
			}

			if (test)	{
				regs.mmusr = MMU_MMUSR_T | MMU_MMUSR_R;
			}

			if ((ttr & MMU_TTR_BIT_WRITE_PROTECT) && write)
				return TTR_NO_WRITE;
			return TTR_OK_MATCH;
		}
	}
	return TTR_NO_MATCH;
}

struct mmu_atc_line atc[64];
static int atc_rand = 0;
static int atc_last_hit = -1;

/* {{{ mmu_dump_table */
static void mmu_dump_table(const char * label, uaecptr root_ptr)
{
	const int ROOT_TABLE_SIZE = 128,
		PTR_TABLE_SIZE = 128,
		PAGE_TABLE_SIZE = 64,
		ROOT_INDEX_SHIFT = 25,
		PTR_INDEX_SHIFT = 18,
		PAGE_INDEX_SHIFT = 12;
	int root_idx, ptr_idx, page_idx;
	uae_u32 root_des, ptr_des, page_des;
	uaecptr ptr_des_addr, page_addr,
		root_log, ptr_log, page_log;
		
	D(bug("%s: root=%lx", label, root_ptr));
	
	for (root_idx = 0; root_idx < ROOT_TABLE_SIZE; root_idx++)	{
		root_des = phys_get_long(root_ptr + root_idx);

		if ((root_des & 2) == 0)
			continue;	/* invalid */
		
		D(bug("ROOT: %03d U=%d W=%d UDT=%02d", root_idx,
				root_des & 8 ? 1 : 0,
				root_des & 4 ? 1 : 0,
				root_des & 3
			  ));

		root_log = root_idx << 25;
		
		ptr_des_addr = root_des & MMU_ROOT_PTR_ADDR_MASK;
		
		for (ptr_idx = 0; ptr_idx < PTR_TABLE_SIZE; ptr_idx++)	{
			struct {
				uaecptr	log, phys;
				int start_idx, n_pages;	/* number of pages covered by this entry */
				uae_u32 match;
			} page_info[PAGE_TABLE_SIZE];
			int n_pages_used;

			ptr_des = phys_get_long(ptr_des_addr + ptr_idx);
			ptr_log = root_log | (ptr_idx << 18);

			if ((ptr_des & 2) == 0)
				continue; /* invalid */

			page_addr = ptr_des & (regs.mmu_pagesize ? MMU_PTR_PAGE_ADDR_MASK_8 : MMU_PTR_PAGE_ADDR_MASK_4);

			n_pages_used = -1;
			for (page_idx = 0; page_idx < PAGE_TABLE_SIZE; page_idx++)	{
				
				page_des = phys_get_long(page_addr + page_idx);
				page_log = ptr_log | (page_idx << 2);

				switch (page_des & 3)	{
					case 0: /* invalid */
						continue;
					case 1: case 3: /* resident */
					case 2: /* indirect */
						if (n_pages_used == -1 || page_info[n_pages_used].match != page_des)	{
							/* use the next entry */
							n_pages_used++;

							page_info[n_pages_used].match = page_des;
							page_info[n_pages_used].n_pages = 1;
							page_info[n_pages_used].start_idx = page_idx;
							page_info[n_pages_used].log = page_log;
						}
						else	{
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


			for (page_idx = 0; page_idx <= n_pages_used; page_idx++)	{
				page_des = page_info[page_idx].match;

				if ((page_des & MMU_PDT_MASK) == 2)	{
					D(bug("  PAGE: %03d-%03d log=%08lx INDIRECT --> addr=%08lx",
							page_info[page_idx].start_idx,
							page_info[page_idx].start_idx + page_info[page_idx].n_pages - 1,
							page_info[page_idx].log,
							page_des & MMU_PAGE_INDIRECT_MASK
						  ));

				}
				else	{
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

/* {{{ mmu_dump_atc */
void mmu_dump_atc(void)
{
	int i;
	for (i = 0; i < 64; i++)	{
		if (!atc[i].v)
			continue;
		D(bug("ATC[%02d] G=%d S=%d CM=%d M=%d W=%d R=%d FC2=%d log=%08x --> phys=%08x",
				i, atc[i].g ? 1 : 0, atc[i].s, atc[i].cm, atc[i].m, atc[i].w, atc[i].r,
				atc[i].fc2 ? 1 : 0,
				atc[i].log,
				atc[i].phys
				));
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

uaecptr REGPARAM2 mmu_translate(uaecptr theaddr, int fc, int write, uaecptr pc, int size, int test)
{
	uae_u32 
		atc_hit_addr = 0,
		root_ptr,
		root_des, root_des_addr,
		ptr_des = 0, ptr_des_addr = 0,
		page_des = 0, page_des_addr = 0,
		phys_addr = 0,
		fslw = 0;
	uae_u8	ri, pi, pgi, wp = 0;
	uae_u16	ssw = 0;
	uae_u32 page_frame;
	int supervisor, datamode =0;
	int i, atc_sel, atc_index = -1, n_table_searches = 0;

//	if (theaddr == 0x40000000) test |= MMU_TEST_VERBOSE;
	
	supervisor = fc & 4;
	
	switch(fc)	{
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
			panicbug("FC=%d should not happen", datamode);
			abort();
	}
	
	root_ptr = supervisor ? regs.srp : regs.urp;
	
	/* check ttr0 */

	/* TTR operate independently from the enable bit, so we can just ignore it if the MMU
	 * is not enabled to get better performance.
	 * But AmigaOS depends on PTEST to operate when the MMU is disabled;
	 * it uses the result in the ssw to detect a working MMU and then enables the MMU */
	if (regs.mmu_enabled || test)	{
		switch(mmu_match_ttr(datamode ? regs.dtt0 : regs.itt0, theaddr, write, test))	{
			case TTR_NO_WRITE:
				D(bug("MMU: write protected (via ttr) %lx", theaddr));
				goto bus_err;
			case TTR_OK_MATCH:
				return theaddr;
		}
		/* check ttr1 */
		switch(mmu_match_ttr(datamode ? regs.dtt1 : regs.itt1, theaddr, write, test))	{
			case TTR_NO_WRITE:
				D(bug("MMU: write protected (via ttr) %lx", theaddr));
				goto bus_err;
			case TTR_OK_MATCH:
				return theaddr;
		}
	}

	if (!regs.mmu_enabled)
		return theaddr;


	ri = (theaddr & 0xfe000000) >> 25;
	pi = (theaddr & 0x01fc0000) >> 18;
	if (regs.mmu_pagesize == MMU_PAGE_8KB)	{
		pgi = (theaddr & 0x3e000) >> 13;
		page_frame = theaddr & 0xffffe000;
		atc_sel = ((theaddr & 0x1e000) >> 13) & 0xf;
	}
	else	{
		pgi = (theaddr & 0x3f000) >> 12;
		page_frame = theaddr & 0xfffff000;
		atc_sel = ((theaddr & 0xf000) >> 12) & 0xf;
	}
check_atc:
	
	atc_rand++;	/* for random replacement */
	
	if (test & MMU_TEST_FORCE_TABLE_SEARCH)
		goto table_search;
	
	for (i = 0; i < 4; i++)	{
		atc_index = atc_sel + (4 * i);

#if DBG_MMU_VERBOSE
	if (test & MMU_TEST_VERBOSE)
		D(bug("MMU: %lx checking atc %d v=%d log=%lx s=%d PHYS=%lx (frame=%lx s=%d)",
				theaddr, atc_index,
				atc[atc_index].v, atc[atc_index].log, atc[atc_index].s,
				atc[atc_index].phys,
				page_frame, regs.s));
#endif


		if (atc[atc_index].v && (atc[atc_index].log == page_frame) && atc[atc_index].fc2 == (fc & 4))
			break;
		atc_index = -1;
	}

	if (atc_index != -1)	{
atc_matched:

		/* it's a hit! */

		if (!atc[atc_index].r)	{
#if DBG_MMU_VERBOSE
			D(bug("MMU: non-resident page!"));
#endif
			goto bus_err;
		}


		wp = atc[atc_index].w;

		atc_hit_addr = atc[atc_index].phys | ((regs.mmu_pagesize == MMU_PAGE_8KB) 
				? (theaddr & 0x1fff)
				: (theaddr & 0x0fff));

		if (test)	{
			if (atc[atc_index].g)
				regs.mmusr |= MMU_MMUSR_G;
			if (atc[atc_index].s)
				regs.mmusr |= MMU_MMUSR_S;
			if (atc[atc_index].m)
				regs.mmusr |= MMU_MMUSR_M;
			if (atc[atc_index].w)
				regs.mmusr |= MMU_MMUSR_W;
			if (atc[atc_index].r)
				regs.mmusr |= MMU_MMUSR_R;

			regs.mmusr |= atc[atc_index].phys & MMU_MMUSR_ADDR_MASK;
		}

#if DBG_MMU_VERBOSE
		if (test & MMU_TEST_VERBOSE)
			if (atc_last_hit != atc_index)	{
				atc_last_hit = atc_index;
				D(bug("MMU: ATC %d HIT! %lx --> %lx", atc_index, theaddr, atc_hit_addr));
				D(bug("MMU: ATC v=%d log=%lx s=%d PHYS=%lx (frame=%lx s=%d)",
						atc[atc_index].v, atc[atc_index].log, atc[atc_index].s,
						atc[atc_index].phys,
						page_frame, regs.s));

			}
#endif

		if (atc[atc_index].s && !supervisor)	{
			D(bug("MMU: Supervisor only"));
			fslw |= (1 << 8);
			goto bus_err;
		}
		if (wp && write)	{
			D(bug("MMU: write protected!"));
			fslw |= (1 << 7);
			goto bus_err;
		}

		if (!atc[atc_index].m && write)	{
			/* we need to update the M bit of the final descriptor */
			goto table_search;
		}
#if 0
		goto table_search;
#endif
		return atc_hit_addr;
	}
	atc_index = -1;
	
table_search:

	if (n_table_searches++ > 3)	{
		panicbug("MMU: apparently looping during table search.");
		abort();
	}
	
	if (atc_index == -1)	{
		//write_log("MMU: replace atc: ");
		for (i = 0; i < 4; i++)	{
			if (!atc[atc_sel + (4 * i)].v)	{
				atc_index = atc_sel + (4 * i);
				break;
			}
		}
		/* random choice */
		if (atc_index == -1)	{
			atc_index = atc_sel + (4 * (atc_rand & 2));
		}
	}

	fslw |= (1 << 6); /* TWE: flag as being in table search */

#if DBG_MMU_VERBOSE
	if (test & MMU_TEST_VERBOSE)
	D(bug("MMU: table search for logical=%08x ri=%02x pi=%02x pgi=%03x page_frame=%08x root_ptr=%08x",
			theaddr, ri, pi, pgi, page_frame, root_ptr));
#endif

	/* root descriptor */
	root_des_addr = (root_ptr & MMU_ROOT_PTR_ADDR_MASK) | (ri << 2);

#if DBG_MMU_SANITY
	if (!phys_valid_address(root_des_addr, sz_long))
		goto bus_err;
#endif
	
	root_des = phys_get_long(root_des_addr);

#if DBG_MMU_VERBOSE
	if (test & MMU_TEST_VERBOSE)	{
	D(bug("MMU: root_des_addr = %lx  val=%08x", root_des_addr, root_des));
	//phys_dump_mem(root_ptr, 128 / 16);
	}
#endif
	
	switch(root_des & MMU_UDT_MASK)	{
		case 0x0:
		case 0x1:
			D(bug("MMU: invalid root descriptor for %lx", theaddr));
			fslw |= (1 << 12); /* PTA */
			goto make_non_resident_atc;
	}
	
	wp |= root_des & MMU_DES_WP;
	/* touch the page */
	if (!wp && (root_des & MMU_DES_USED) == 0)	{
		root_des |= MMU_DES_USED;
		phys_put_long(root_des_addr, root_des);
	}

	
	ptr_des_addr = (root_des & MMU_ROOT_PTR_ADDR_MASK) | (pi << 2);
#if DBG_MMU_SANITY
	if (!phys_valid_address(ptr_des_addr, sz_long))
		goto bus_err;
#endif
	
	ptr_des = phys_get_long(ptr_des_addr);
#if DBG_MMU_VERBOSE
	if (test & MMU_TEST_VERBOSE)	
	D(bug("MMU: ptr_des_addr = %lx  val=%08x", ptr_des_addr, ptr_des));
	//phys_dump_mem(ptr_des_addr, 128 / 16);
#endif
	
	switch(ptr_des & MMU_UDT_MASK)	{
		case 0x0:
		case 0x1:
			D(bug("MMU: invalid ptr descriptor for %lx", theaddr));
			fslw |= (1 << 11); /* PTB */
			goto make_non_resident_atc;
	}
	wp |= ptr_des & MMU_DES_WP;
	/* touch */
	if (!wp && (ptr_des & MMU_DES_USED) == 0)	{
		ptr_des |= MMU_DES_USED;
		phys_put_long(ptr_des_addr, ptr_des);
	}

	if (regs.mmu_pagesize == MMU_PAGE_8KB)
		page_des_addr = (ptr_des & MMU_PTR_PAGE_ADDR_MASK_8) | (pgi << 2);
	else
		page_des_addr = (ptr_des & MMU_PTR_PAGE_ADDR_MASK_4) | (pgi << 2);
	
get_page_descriptor:
#if DBG_MMU_SANITY
	if (!phys_valid_address(page_des_addr, sz_long))
		goto bus_err;
#endif
	
	page_des = phys_get_long(page_des_addr);
#if DBG_MMU_VERBOSE
	if (test & MMU_TEST_VERBOSE)	{
		D(bug("MMU: page_des_addr = %lx  val=%08x", page_des_addr, page_des));
		phys_dump_mem(page_des_addr, 64 / 16);
	}
#endif

	switch(page_des & MMU_PDT_MASK)	{
		case 0x0:
			D(bug("MMU: invalid page descriptor log=%08lx page_des=%08lx @%08lx", theaddr, page_des, page_des_addr));
			fslw |= (1 << 9); /* PF */
			goto make_non_resident_atc;
		case 0x1:
		case 0x3:
			/* resident page */
			break;
		case 0x2:
		default:
			/* indirect */
			if (fslw & (1 << 10))	{
				D(bug("MMU: double indirect descriptor log=%lx descriptor @ %lx", theaddr, page_des_addr));
				goto make_non_resident_atc;
			}
			page_des_addr = page_des & MMU_PAGE_INDIRECT_MASK;
			fslw |= (1 << 10); /* IL - in case a fault occurs later, tag it as indirect */
			goto get_page_descriptor;
	}

	wp |= page_des & MMU_DES_WP;
	if (!wp)	{
		int modify = 0;
		if ((page_des & MMU_DES_USED) == 0)	{
			page_des |= MMU_DES_USED;
			modify = 1;
		}
		/* set the modified bit */
		if (write && (page_des & MMU_DES_MODIFIED) == 0)	{
			page_des |= MMU_DES_MODIFIED;
			modify = 1;
		}
		if (modify)
			phys_put_long(page_des_addr, page_des);
	}
	
	atc[atc_index].log = page_frame;
	atc[atc_index].v = 1;
	atc[atc_index].r = 1;
	atc[atc_index].s = page_des & MMU_DES_SUPER;	/* supervisor */
	atc[atc_index].w = wp;
	atc[atc_index].fc2 = fc & 4;
	atc[atc_index].g = page_des & MMU_DES_GLOBAL;
	atc[atc_index].phys = page_des & (regs.mmu_pagesize ? MMU_PAGE_ADDR_MASK_8 : MMU_PAGE_ADDR_MASK_4);

	atc[atc_index].m = (page_des & MMU_DES_MODIFIED) ? 1 : 0;

	
#if 0
	if (atc_hit_addr != 0 && atc_hit_addr != phys_addr)	{
		write_log("MMU: ERROR! ATC hit does not match table search! for %lx --> %lx (atc gave %lx)\n",
				theaddr, phys_addr, atc_hit_addr);
		activate_debugger();
	}
#endif
	/* re-use the end of the atc code */
	goto atc_matched;
	
bus_err:

	ssw |= (1 << 10);	/* ATC */
	if (!write)
		ssw |= (1 << 8);

	fslw |= (1 << (write ? 23 : 24));
	if (!datamode)	{
		fslw |= (1 << 15); /* IO */

		if (supervisor)
			ssw |= 0x6;
		else
			ssw |= 0x2;
	}
#if 0
	if (regs.t0)
		fslw |= (1 << 19);
	if (regs.t1)
		fslw |= (1 << 20);
#endif

	ssw |= fc & 7; /* Copy TM */
	
	regs.mmu_fault_addr = theaddr;
	regs.mmu_fslw = fslw;
	regs.mmu_ssw = ssw;

	if (test)
		regs.mmusr |= MMU_MMUSR_B;
	
	D(bug("BUS ERROR: fc=%d w=%d log=%08x ssw=%04x fslw=%08x", fc, write, theaddr, ssw, fslw));

	if ((test & MMU_TEST_NO_BUSERR) == 0)	{
		Exception(2, pc);
		longjmp(excep_env, 2);
	}
	return 0;

make_non_resident_atc:
#if DBG_MMU_VERBOSE
	D(bug("MMU: table search for logical=%08x FC=%d ri=%02x pi=%02x pgi=%03x page_frame=%08x root_ptr=%08x",
			theaddr, fc, ri, pi, pgi, page_frame, root_ptr));
	D(bug("MMU: root_des_addr = %lx  val=%08x", root_des_addr, root_des));
	D(bug("MMU: ptr_des_addr = %lx  val=%08x", ptr_des_addr, ptr_des));
	D(bug("MMU: page_des_addr = %lx  val=%08x", page_des_addr, page_des));
	mmu_dump_ttr("DTT0", regs.dtt0);	
	mmu_dump_ttr("DTT1", regs.dtt1);	
	mmu_dump_ttr("ITT0", regs.itt0);	
	mmu_dump_ttr("ITT1", regs.itt1);	
#endif

	atc[atc_index].log = page_frame;
	atc[atc_index].phys = 0;
	atc[atc_index].v = 0;
	atc[atc_index].r = 0;
	goto bus_err;
}

void mmu_op(uae_u32 opcode, uae_u16 extra)
{
	if ((opcode & 0xFE0) == 0x0500) {
		int i, regno, didflush = 0;
		/* PFLUSH */
		mmu_set_mmusr(0);

		regno = opcode & 7;
		
		switch((opcode & 24) >> 3)	{
			case 0:
				/* PFLUSHN (An) flush page entry if not global */
				D(bug("PFLUSHN (A%d) %08x DFC=%d", regno, m68k_areg(regs, regno), regs.dfc));
				for (i = 0; i < 64; i++)	{
					if (atc[i].v && !atc[i].g && (atc[i].log == m68k_areg(regs, regno))
							&& (int)(regs.dfc & 4) == atc[i].fc2)
					{
						atc[i].v = 0;
						didflush++;
					}
				}
				break;
			case 1:
				/* PFLUSH (An) flush page entry */
				D(bug("PFLUSH (A%d) %08x DFC=%d", regno, m68k_areg(regs, regno), regs.dfc));
				for (i = 0; i < 64; i++)	{
					if (atc[i].v && (atc[i].log == m68k_areg(regs, regno))
							&& (int)(regs.dfc & 4) == atc[i].fc2)
					{
						atc[i].v = 0;
						didflush++;
					}
				}

				break;

			case 2:
				/* PFLUSHAN flush all except global */
				D(bug("PFLUSHAN"));
				for (i = 0; i < 64; i++)	{
					if (atc[i].v && !atc[i].g && (atc[i].log == m68k_areg(regs, regno)))
						atc[i].v = 0;
				}
				break;

			case 3:
				/* PFLUSHA flush all entries */
				D(bug("PFLUSHA"));
				for (i = 0; i < 64; i++)	{
					if (atc[i].v)
						didflush++;
					atc[i].v = 0;
				}
				atc_last_hit = -1;
				break;
		}
		if (didflush)
			D(bug("  -> flushed %d matching entries", didflush));
		
	} else if ((opcode & 0x0FD8) == 0x548) {
		int write, regno;
		regno = opcode & 7;
		write = opcode & 32;
		D(bug("PTEST%c (A%d) %08x DFC=%d", write ? 'W' : 'R', regno, m68k_areg(regs, regno), regs.dfc));
		mmu_set_mmusr(0);
		mmu_translate(m68k_areg(regs, regno), regs.dfc, write, m68k_getpc(), sz_byte, 1); 
		D(bug("PTEST result: mmusr %08x", regs.mmusr));
	} else
		op_illg (opcode);
}


