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

#include "sysdeps.h"
#include "registers.h"
#include "spcflags.h"
#include "m68k.h"

extern int areg_byteinc[];
extern int imm8_table[];

extern int movem_index1[256];
extern int movem_index2[256];
extern int movem_next[256];

extern int broken_in;

/* Control flow information */
#define CFLOW_NORMAL		0
#define CFLOW_BRANCH		1
#define CFLOW_JUMP			2
#define CFLOW_TRAP			CFLOW_JUMP
#define CFLOW_RETURN		3
#define CFLOW_SPCFLAGS		32	/* some spcflags are set */
#define CFLOW_EXEC_RETURN	64	/* must exit from the execution loop */

#define cpuop_rettype		void
#define cpuop_return(v)		do { (v); return; } while (0)

#ifdef X86_ASSEMBLY
/* This hack seems to force all register saves (pushl %reg) to be moved to the
   begining of the function, thus making it possible to cpuopti to remove them
   since m68k_run_1 will save those registers before calling the instruction
   handler */
# define cpuop_tag(tag)		ASM_VOLATILE ( "#cpuop_" tag )
#else
# define cpuop_tag(tag)		;
#endif

#define cpuop_begin()		do { cpuop_tag("begin"); } while (0)
#define cpuop_end(cflow)	do { cpuop_tag("end"); cpuop_return(cflow); } while (0)

typedef cpuop_rettype REGPARAM2 cpuop_func (uae_u32) REGPARAM;

struct cputbl {
    cpuop_func *handler;
    uae_u16 specific;
    uae_u16 opcode;
};

extern cpuop_func *cpufunctbl[65536] ASM_SYM_FOR_FUNC ("cpufunctbl");

#if USE_JIT
typedef void compop_func (uae_u32) REGPARAM;

struct comptbl {
    compop_func *handler;
	uae_u32		specific;
	uae_u32		opcode;
};
#endif

extern cpuop_rettype REGPARAM2 op_illg (uae_u32) REGPARAM;

#define m68k_dreg(r,num) ((r).regs[(num)])
#define m68k_areg(r,num) (((r).regs + 8)[(num)])

#ifdef FULLMMU
static __inline__ uae_u8 get_ibyte(uae_u32 o)
{
    uaecptr addr = m68k_getpc() + o + 1;
    return phys_get_byte(mmu_translate(addr, FC_INST, 0, addr, sz_byte, 0));
}
static __inline__ uae_u16 get_iword(uae_u32 o)
{
    uaecptr addr = m68k_getpc() + o;
    return phys_get_word(mmu_translate(addr, FC_INST, 0, addr, sz_word, 0));
}
static __inline__ uae_u32 get_ilong(uae_u32 o)
{
    uaecptr addr = m68k_getpc() + o;
    return phys_get_long(mmu_translate(addr, FC_INST, 0, addr, sz_long, 0));
}

static __inline__ uae_u8 get_ibyte_1(uae_u32 o)
{
    uaecptr addr = m68k_getpc() + o + 1;
    return phys_get_byte(mmu_translate(addr, FC_INST, 0, addr, sz_byte, 0));
}
static __inline__ uae_u16 get_iword_1(uae_u32 o)
{
    uaecptr addr = m68k_getpc() + o;
    return phys_get_word(mmu_translate(addr, FC_INST, 0, addr, sz_word, 0));
}

static __inline__ uae_u32 get_ilong_1(uae_u32 o)
{
    uaecptr addr = m68k_getpc() + o;
    return phys_get_long(mmu_translate(addr, FC_INST, 0, addr, sz_long, 0));
}
#else
#define get_ibyte(o) do_get_mem_byte((uae_u8 *)(get_real_address(m68k_getpc(), 0, sz_byte) + (o) + 1))
#define get_iword(o) do_get_mem_word((uae_u16 *)(get_real_address(m68k_getpc(), 0, sz_word) + (o)))
#define get_ilong(o) do_get_mem_long((uae_u32 *)(get_real_address(m68k_getpc(), 0, sz_long) + (o)))
#endif

#ifdef ARAM_PAGE_CHECK
# ifdef HAVE_GET_WORD_UNSWAPPED
#define GET_OPCODE (do_get_mem_word_unswapped((uae_u16*)(pc + pc_offset)));
# else
#define GET_OPCODE (do_get_mem_word((uae_u16*)(pc + pc_offset)));
# endif
#else
#ifdef HAVE_GET_WORD_UNSWAPPED
#define GET_OPCODE (do_get_mem_word_unswapped (get_real_address(m68k_getpc(), 0, sz_word)))
#else
#define GET_OPCODE (get_iword (0))
#endif
#endif

#if 0
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
#endif

#define m68k_incpc(o) (regs.pc_p += (o))

static __inline__ void fill_prefetch_0 (void)
{
#if USE_PREFETCH_BUFFER
    uae_u32 r;
#ifdef UNALIGNED_PROFITABLE
    r = *(uae_u32 *)do_get_real_address(m68k_getpc(), false, false);
    regs.prefetch = r;
#else
    r = do_get_mem_long ((uae_u32 *)do_get_real_address(m68k_getpc(), false, false));
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
    regs.pc_p = regs.pc_oldp = get_real_address(newpc, 0, sz_word);
    regs.pc = newpc;
}

#define m68k_setpc_fast m68k_setpc
#define m68k_setpc_bcc  m68k_setpc
#define m68k_setpc_rte  m68k_setpc

static __inline__ void m68k_do_rts(void)
{
    m68k_setpc(get_long(m68k_areg(regs, 7)));
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
	SPCFLAGS_SET( SPCFLAG_STOP );
}

extern uae_u32 get_disp_ea_020 (uae_u32 base, uae_u32 dp);
extern uae_u32 get_disp_ea_000 (uae_u32 base, uae_u32 dp);

extern void MakeSR (void);
extern void MakeFromSR (void);
extern void Exception (int, uaecptr);
extern void dump_counts (void);
extern int m68k_move2c (int, uae_u32 *);
extern int m68k_movec2 (int, uae_u32 *);
extern void m68k_divl (uae_u32, uae_u32, uae_u16, uaecptr);
extern void m68k_mull (uae_u32, uae_u32, uae_u16);
extern void m68k_emulop (uae_u32);
extern void m68k_emulop_return (void);
extern void m68k_natfea (uae_u32);
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

/* Opcode of faulting instruction */
extern uae_u16 last_op_for_exception_3;
/* PC at fault time */
extern uaecptr last_addr_for_exception_3;
/* Address that generated the exception */
extern uaecptr last_fault_for_exception_3;

#define CPU_OP_NAME(a) op ## a

/* 68040+ 68881 */
extern struct cputbl op_smalltbl_0[];

#endif /* NEWCPU_H */
