/* 2001 MJ */

 /*
  * UAE - The Un*x Amiga Emulator
  *
  * MC68000 emulation
  *
  * Copyright 1995 Bernd Schmidt
  */

#ifndef NEWCPU_H
#define NEWCPU_H

#include "registers.h"

#define SPCFLAG_STOP 2
#define SPCFLAG_DISK 4
#define SPCFLAG_INT  8
#define SPCFLAG_BRK  16
#define SPCFLAG_EXTRA_CYCLES 32
#define SPCFLAG_TRACE 64
#define SPCFLAG_DOTRACE 128
#define SPCFLAG_DOINT 256
#define SPCFLAG_VBL 512
#define SPCFLAG_MFP 1024
#define SPCFLAG_MODE_CHANGE 8192

extern int areg_byteinc[];
extern int imm8_table[];

extern int movem_index1[256];
extern int movem_index2[256];
extern int movem_next[256];

extern int fpp_movem_index1[256];
extern int fpp_movem_index2[256];
extern int fpp_movem_next[256];

extern int broken_in;

typedef void REGPARAM2 cpuop_func (uae_u32) REGPARAM;

struct cputbl {
    cpuop_func *handler;
    int specific;
    uae_u16 opcode;
};

extern void REGPARAM2 op_illg (uae_u32) REGPARAM;

static __inline__ void set_special (uae_u32 x)
{
    regs.spcflags |= x;
}

static __inline__ void unset_special (uae_u32 x)
{
    regs.spcflags &= ~x;
}

#define m68k_dreg(r,num) ((r).regs[(num)])
#define m68k_areg(r,num) (((r).regs + 8)[(num)])

#define get_ibyte(o) do_get_mem_byte((uae_u8 *)(do_get_real_address(regs.pcp, false, false) + (o) + 1))
#define get_iword(o) do_get_mem_word((uae_u16 *)(do_get_real_address(regs.pcp, false, false) + (o)))
#define get_ilong(o) do_get_mem_long((uae_u32 *)(do_get_real_address(regs.pcp, false, false) + (o)))

#ifdef HAVE_GET_WORD_UNSWAPPED
#define GET_OPCODE (do_get_mem_word_unswapped (do_get_real_address(regs.pcp, false, false)))
#else
#define GET_OPCODE (get_iword (0))
#endif

static __inline__ uae_u32 get_ibyte_prefetch (uae_s32 o)
{
    if (o > 3 || o < 0)
	return do_get_mem_byte((uae_u8 *)(do_get_real_address(regs.pcp, false, false) + o + 1));

    return do_get_mem_byte((uae_u8 *)(((uae_u8 *)&regs.prefetch) + o + 1));
}
static __inline__ uae_u32 get_iword_prefetch (uae_s32 o)
{
    if (o > 3 || o < 0)
	return do_get_mem_word((uae_u16 *)(do_get_real_address(regs.pcp, false, false) + o));

    return do_get_mem_word((uae_u16 *)(((uae_u8 *)&regs.prefetch) + o));
}
static __inline__ uae_u32 get_ilong_prefetch (uae_s32 o)
{
    if (o > 3 || o < 0)
	return do_get_mem_long((uae_u32 *)(do_get_real_address(regs.pcp, false, false) + o));
    if (o == 0)
	return do_get_mem_long(&regs.prefetch);
    return (do_get_mem_word (((uae_u16 *)&regs.prefetch) + 1) << 16) | do_get_mem_word ((uae_u16 *)(do_get_real_address(regs.pcp, false, false) + 4));
}

#define m68k_incpc(o) (regs.pcp += (o))

static __inline__ void fill_prefetch_0 (void)
{
#if USE_PREFETCH_BUFFER
    uae_u32 r;
#ifdef UNALIGNED_PROFITABLE
    r = *(uae_u32 *)do_get_real_address(regs.pcp, false, false);
    regs.prefetch = r;
#else
    r = do_get_mem_long ((uae_u32 *)do_get_real_address(regs.pcp, false, false));
    do_put_mem_long (&regs.prefetch, r);
#endif
#endif
}

#if 0
static __inline__ void fill_prefetch_2 (void)
{
    uae_u32 r = do_get_mem_long (&regs.prefetch) << 16;
    uae_u32 r2 = do_get_mem_word (((uae_u16 *)do_get_real_address(regs.pcp, false, false)) + 1);
    r |= r2;
    do_put_mem_long (&regs.prefetch, r);
}
#else
#define fill_prefetch_2 fill_prefetch_0
#endif

/* These are only used by the 68020/68881 code, and therefore don't
 * need to handle prefetch.  */
static __inline__ uae_u32 next_ibyte (void)
{
    uae_u32 r = get_ibyte (0);
    m68k_incpc (2);
    return r;
}

static __inline__ uae_u32 next_iword (void)
{
    uae_u32 r = get_iword (0);
    m68k_incpc (2);
    return r;
}

static __inline__ uae_u32 next_ilong (void)
{
    uae_u32 r = get_ilong (0);
    m68k_incpc (4);
    return r;
}

static __inline__ void m68k_setpc (uaecptr newpc)
{
	regs.pcp = newpc;
}

static __inline__ uaecptr m68k_getpc (void)
{
    return regs.pcp;
}

#define m68k_setpc_fast m68k_setpc
#define m68k_setpc_bcc  m68k_setpc
#define m68k_setpc_rte  m68k_setpc

static __inline__ void m68k_do_rts(void)
{
	    m68k_setpc(get_long(m68k_areg(regs, 7), true));
	        m68k_areg(regs, 7) += 4;
}
 
static __inline__ void m68k_do_bsr(uaecptr oldpc, uae_s32 offset)
{
	    m68k_areg(regs, 7) -= 4;
	        put_long(m68k_areg(regs, 7), oldpc);
		    m68k_incpc(offset);
}
 
static __inline__ void m68k_do_jsr(uaecptr oldpc, uaecptr dest)
{
	    m68k_areg(regs, 7) -= 4;
	        put_long(m68k_areg(regs, 7), oldpc);
		    m68k_setpc(dest);
}

static __inline__ void m68k_setstopped (int stop)
{
    regs.stopped = stop;
    if (stop)
	regs.spcflags |= SPCFLAG_STOP;
}

extern uae_u32 get_disp_ea_020 (uae_u32 base, uae_u32 dp);
extern uae_u32 get_disp_ea_000 (uae_u32 base, uae_u32 dp);

extern void MakeSR (void);
extern void MakeFromSR (void);
extern void Exception (int, uaecptr);
extern void dump_counts (void);
extern uae_u32 m68k_move2c (int, uae_u32 *);
extern uae_u32 m68k_movec2 (int, uae_u32 *);
extern void m68k_divl (uae_u32, uae_u32, uae_u16, uaecptr);
extern void m68k_mull (uae_u32, uae_u32, uae_u16);
extern void init_m68k (void);
extern void exit_m68k (void);
extern void m68k_go (int);
extern void m68k_dumpstate (uaecptr *);
extern void m68k_disasm (uaecptr, uaecptr *, int);
extern void newm68k_disasm(FILE *, uaecptr, uaecptr *, unsigned int);
extern void showDisasm(uaecptr);
extern void m68k_reset (void);
extern void m68k_enter_debugger(void);

extern void mmu_op (uae_u32, uae_u16);

extern void fpp_opp (uae_u32, uae_u16);
extern void fdbcc_opp (uae_u32, uae_u16);
extern void fscc_opp (uae_u32, uae_u16);
extern void ftrapcc_opp (uae_u32,uaecptr);
extern void fbcc_opp (uae_u32, uaecptr, uae_u32);
extern void fsave_opp (uae_u32);
extern void frestore_opp (uae_u32);

extern void fpu_set_integral_fpu (bool is_integral);
extern void fpu_init (void);
extern void fpu_exit (void);
extern void fpu_reset (void);

/* Opcode of faulting instruction */
extern uae_u16 last_op_for_exception_3;
/* PC at fault time */
extern uaecptr last_addr_for_exception_3;
/* Address that generated the exception */
extern uaecptr last_fault_for_exception_3;

#define CPU_OP_NAME(a) op ## a

/* 68020 + 68881 */
extern struct cputbl op_smalltbl_0[];
/* 68020 */
extern struct cputbl op_smalltbl_1[];
/* 68010 */
extern struct cputbl op_smalltbl_2[];
/* 68000 */
extern struct cputbl op_smalltbl_3[];
/* 68000 slow but compatible.  */
extern struct cputbl op_smalltbl_4[];

extern cpuop_func *cpufunctbl[65536] ASM_SYM_FOR_FUNC ("cpufunctbl");

#endif /* NEWCPU_H */
