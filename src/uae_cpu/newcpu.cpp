 /*
  * UAE - The Un*x Amiga Emulator
  *
  * MC68000 emulation
  *
  * (c) 1995 Bernd Schmidt
  */

#include "sysdeps.h"

#include "cpu_emulation.h"
#include "main.h"
#include "emul_op.h"
#include "ndebug.h"
#include "m68k.h"
#include "memory.h"
#include "readcpu.h"
#include "newcpu.h"
#include "exceptions.h"
#define DEBUG 1
#include "debug.h"

int quit_program = 0;

struct flag_struct regflags;

/* LongJump buffer */
jmp_buf excep_env;
/* Opcode of faulting instruction */
uae_u16 last_op_for_exception_3;
/* PC at fault time */
uaecptr last_addr_for_exception_3;
/* Address that generated the exception */
uaecptr last_fault_for_exception_3;

int areg_byteinc[] = { 1,1,1,1,1,1,1,2 };
int imm8_table[] = { 8,1,2,3,4,5,6,7 };

int movem_index1[256];
int movem_index2[256];
int movem_next[256];

int fpp_movem_index1[256];
int fpp_movem_index2[256];
int fpp_movem_next[256];

cpuop_func *cpufunctbl[65536];

#define COUNT_INSTRS 0

#if COUNT_INSTRS
static unsigned long int instrcount[65536];
static uae_u16 opcodenums[65536];

static int compfn (const void *el1, const void *el2)
{
    return instrcount[*(const uae_u16 *)el1] < instrcount[*(const uae_u16 *)el2];
}

static char *icountfilename (void)
{
    char *name = getenv ("INSNCOUNT");
    if (name)
	return name;
    return COUNT_INSTRS == 2 ? "frequent.68k" : "insncount";
}

void dump_counts (void)
{
    FILE *f = fopen (icountfilename (), "w");
    unsigned long int total;
    int i;

    D(bug("Writing instruction count file..."));
    for (i = 0; i < 65536; i++) {
	opcodenums[i] = i;
	total += instrcount[i];
    }
    qsort (opcodenums, 65536, sizeof(uae_u16), compfn);

    fprintf (f, "Total: %lu\n", total);
    for (i=0; i < 65536; i++) {
	unsigned long int cnt = instrcount[opcodenums[i]];
	struct instr *dp;
	struct mnemolookup *lookup;
	if (!cnt)
	    break;
	dp = table68k + opcodenums[i];
	for (lookup = lookuptab;lookup->mnemo != dp->mnemo; lookup++)
	    ;
	fprintf (f, "%04x: %lu %s\n", opcodenums[i], cnt, lookup->name);
    }
    fclose (f);
}
#else
void dump_counts (void)
{
}
#endif

int broken_in;

static __inline__ unsigned int cft_map (unsigned int f)
{
#ifndef HAVE_GET_WORD_UNSWAPPED
    return f;
#else
    return ((f >> 8) & 255) | ((f & 255) << 8);
#endif
}

static void REGPARAM2 op_illg_1 (uae_u32 opcode) REGPARAM;

static void REGPARAM2 op_illg_1 (uae_u32 opcode)
{
    op_illg (cft_map (opcode));
}

static void build_cpufunctbl (void)
{
    int i;
    unsigned long opcode;
	int cpu_level = 0;		// 68000 (default)
	if (CPUType == 4)
		cpu_level = 4;		// 68040 with FPU
	else {
		if (FPUType)
			cpu_level = 3;	// 68020 with FPU
		else if (CPUType >= 2)
			cpu_level = 2;	// 68020
		else if (CPUType == 1)
			cpu_level = 1;
	}
    struct cputbl *tbl = op_smalltbl_0;

    for (opcode = 0; opcode < 65536; opcode++)
	cpufunctbl[cft_map (opcode)] = op_illg_1;
    for (i = 0; tbl[i].handler != NULL; i++) {
	if (! tbl[i].specific)
	    cpufunctbl[cft_map (tbl[i].opcode)] = tbl[i].handler;
    }
    for (opcode = 0; opcode < 65536; opcode++) {
	cpuop_func *f;

	if (table68k[opcode].mnemo == i_ILLG || (unsigned)table68k[opcode].clev > (unsigned)cpu_level)
	    continue;

	if (table68k[opcode].handler != -1) {
	    f = cpufunctbl[cft_map (table68k[opcode].handler)];
	    if (f == op_illg_1)
		abort();
	    cpufunctbl[cft_map (opcode)] = f;
	}
    }
    for (i = 0; tbl[i].handler != NULL; i++) {
	if (tbl[i].specific)
	    cpufunctbl[cft_map (tbl[i].opcode)] = tbl[i].handler;
    }
}

void init_m68k (void)
{
    int i;

    for (i = 0 ; i < 256 ; i++) {
	int j;
	for (j = 0 ; j < 8 ; j++) {
		if (i & (1 << j)) break;
	}
	movem_index1[i] = j;
	movem_index2[i] = 7-j;
	movem_next[i] = i & (~(1 << j));
    }
    for (i = 0 ; i < 256 ; i++) {
	int j;
	for (j = 7 ; j >= 0 ; j--) {
		if (i & (1 << j)) break;
	}
	fpp_movem_index1[i] = 7-j;
	fpp_movem_index2[i] = j;
	fpp_movem_next[i] = i & (~(1 << j));
    }
#if COUNT_INSTRS
    {
	FILE *f = fopen (icountfilename (), "r");
	memset (instrcount, 0, sizeof instrcount);
	if (f) {
	    uae_u32 opcode, count, total;
	    char name[20];
	    D(bug("Reading instruction count file..."));
	    fscanf (f, "Total: %lu\n", &total);
	    while (fscanf (f, "%lx: %lu %s\n", &opcode, &count, name) == 3) {
		instrcount[opcode] = count;
	    }
	    fclose(f);
	}
    }
#endif
    read_table68k ();
    do_merges ();

    build_cpufunctbl ();
    
    fpu_init ();
    fpu_set_integral_fpu (CPUType == 4);
}

void exit_m68k (void)
{
	fpu_exit ();
}

struct regstruct regs, lastint_regs;
// MJ static struct regstruct regs_backup[16];
// MJ static int backup_pointer = 0;
static long int m68kpc_offset;
int lastint_no;

#define get_ibyte_1(o) get_byte(regs.pcp + (o) + 1)
#define get_iword_1(o) get_word(regs.pcp + (o))
#define get_ilong_1(o) get_long(regs.pcp + (o))

uae_s32 ShowEA (int reg, amodes mode, wordsizes size, char *buf)
{
    uae_u16 dp;
    uae_s8 disp8;
    uae_s16 disp16;
    int r;
    uae_u32 dispreg;
    uaecptr addr;
    uae_s32 offset = 0;
    char buffer[80];

    strcpy(buffer, "");

    switch (mode){
     case Dreg:
	sprintf (buffer,"D%d", reg);
	break;
     case Areg:
	sprintf (buffer,"A%d", reg);
	break;
     case Aind:
	sprintf (buffer,"(A%d)", reg);
	break;
     case Aipi:
	sprintf (buffer,"(A%d)+", reg);
	break;
     case Apdi:
	sprintf (buffer,"-(A%d)", reg);
	break;
     case Ad16:
	disp16 = get_iword_1 (m68kpc_offset); m68kpc_offset += 2;
	addr = m68k_areg(regs,reg) + (uae_s16)disp16;
	sprintf (buffer,"(A%d,$%04x) == $%08lx", reg, disp16 & 0xffff,
					(unsigned long)addr);
	break;
     case Ad8r:
	dp = get_iword_1 (m68kpc_offset); m68kpc_offset += 2;
	disp8 = dp & 0xFF;
	r = (dp & 0x7000) >> 12;
	dispreg = dp & 0x8000 ? m68k_areg(regs,r) : m68k_dreg(regs,r);
	if (!(dp & 0x800)) dispreg = (uae_s32)(uae_s16)(dispreg);
	dispreg <<= (dp >> 9) & 3;

	if (dp & 0x100) {
	    uae_s32 outer = 0, disp = 0;
	    uae_s32 base = m68k_areg(regs,reg);
	    char name[10];
	    sprintf (name,"A%d, ",reg);
	    if (dp & 0x80) { base = 0; name[0] = 0; }
	    if (dp & 0x40) dispreg = 0;
	    if ((dp & 0x30) == 0x20) { disp = (uae_s32)(uae_s16)get_iword_1 (m68kpc_offset); m68kpc_offset += 2; }
	    if ((dp & 0x30) == 0x30) { disp = get_ilong_1 (m68kpc_offset); m68kpc_offset += 4; }
	    base += disp;

	    if ((dp & 0x3) == 0x2) { outer = (uae_s32)(uae_s16)get_iword_1 (m68kpc_offset); m68kpc_offset += 2; }
	    if ((dp & 0x3) == 0x3) { outer = get_ilong_1 (m68kpc_offset); m68kpc_offset += 4; }

	    if (!(dp & 4)) base += dispreg;
	    if (dp & 3) base = get_long (base);
	    if (dp & 4) base += dispreg;

	    addr = base + outer;
	    sprintf (buffer,"(%s%c%d.%c*%d+%ld)+%ld == $%08lx", name,
		    dp & 0x8000 ? 'A' : 'D', (int)r, dp & 0x800 ? 'L' : 'W',
		    1 << ((dp >> 9) & 3),
		    (unsigned long)disp, (unsigned long)outer,
		    (unsigned long)addr);
	} else {
	  addr = m68k_areg(regs,reg) + (uae_s32)((uae_s8)disp8) + dispreg;
	  sprintf (buffer,"(A%d, %c%d.%c*%d, $%02x) == $%08lx", reg,
	       dp & 0x8000 ? 'A' : 'D', (int)r, dp & 0x800 ? 'L' : 'W',
	       1 << ((dp >> 9) & 3), disp8,
	       (unsigned long)addr);
	}
	break;
     case PC16:
	addr = m68k_getpc () + m68kpc_offset;
	disp16 = get_iword_1 (m68kpc_offset); m68kpc_offset += 2;
	addr += (uae_s16)disp16;
	sprintf (buffer,"(PC,$%04x) == $%08lx", disp16 & 0xffff,(unsigned long)addr);
	break;
     case PC8r:
	addr = m68k_getpc () + m68kpc_offset;
	dp = get_iword_1 (m68kpc_offset); m68kpc_offset += 2;
	disp8 = dp & 0xFF;
	r = (dp & 0x7000) >> 12;
	dispreg = dp & 0x8000 ? m68k_areg(regs,r) : m68k_dreg(regs,r);
	if (!(dp & 0x800)) dispreg = (uae_s32)(uae_s16)(dispreg);
	dispreg <<= (dp >> 9) & 3;

	if (dp & 0x100) {
	    uae_s32 outer = 0,disp = 0;
	    uae_s32 base = addr;
	    char name[10];
	    sprintf (name,"PC, ");
	    if (dp & 0x80) { base = 0; name[0] = 0; }
	    if (dp & 0x40) dispreg = 0;
	    if ((dp & 0x30) == 0x20) { disp = (uae_s32)(uae_s16)get_iword_1 (m68kpc_offset); m68kpc_offset += 2; }
	    if ((dp & 0x30) == 0x30) { disp = get_ilong_1 (m68kpc_offset); m68kpc_offset += 4; }
	    base += disp;

	    if ((dp & 0x3) == 0x2) { outer = (uae_s32)(uae_s16)get_iword_1 (m68kpc_offset); m68kpc_offset += 2; }
	    if ((dp & 0x3) == 0x3) { outer = get_ilong_1 (m68kpc_offset); m68kpc_offset += 4; }

	    if (!(dp & 4)) base += dispreg;
	    if (dp & 3) base = get_long (base);
	    if (dp & 4) base += dispreg;

	    addr = base + outer;
	    sprintf (buffer,"(%s%c%d.%c*%d+%ld)+%ld == $%08lx", name,
		    dp & 0x8000 ? 'A' : 'D', (int)r, dp & 0x800 ? 'L' : 'W',
		    1 << ((dp >> 9) & 3),
		    (unsigned long)disp, (unsigned long)outer,
		    (unsigned long)addr);
	} else {
	  addr += (uae_s32)((uae_s8)disp8) + dispreg;
	  sprintf (buffer,"(PC, %c%d.%c*%d, $%02x) == $%08lx", dp & 0x8000 ? 'A' : 'D',
		(int)r, dp & 0x800 ? 'L' : 'W',  1 << ((dp >> 9) & 3),
		disp8, (unsigned long)addr);
	}
	break;
     case absw:
	sprintf (buffer,"$%08lx", (unsigned long)(uae_s32)(uae_s16)get_iword_1 (m68kpc_offset));
	m68kpc_offset += 2;
	break;
     case absl:
	sprintf (buffer,"$%08lx", (unsigned long)get_ilong_1 (m68kpc_offset));
	m68kpc_offset += 4;
	break;
     case imm:
	switch (size){
	 case sz_byte:
	    sprintf (buffer,"#$%02x", (unsigned int)(get_iword_1 (m68kpc_offset) & 0xff));
	    m68kpc_offset += 2;
	    break;
	 case sz_word:
	    sprintf (buffer,"#$%04x", (unsigned int)(get_iword_1 (m68kpc_offset) & 0xffff));
	    m68kpc_offset += 2;
	    break;
	 case sz_long:
	    sprintf (buffer,"#$%08lx", (unsigned long)(get_ilong_1 (m68kpc_offset)));
	    m68kpc_offset += 4;
	    break;
	 default:
	    break;
	}
	break;
     case imm0:
	offset = (uae_s32)(uae_s8)get_iword_1 (m68kpc_offset);
	m68kpc_offset += 2;
	sprintf (buffer,"#$%02x", (unsigned int)(offset & 0xff));
	break;
     case imm1:
	offset = (uae_s32)(uae_s16)get_iword_1 (m68kpc_offset);
	m68kpc_offset += 2;
	sprintf (buffer,"#$%04x", (unsigned int)(offset & 0xffff));
	break;
     case imm2:
	offset = (uae_s32)get_ilong_1 (m68kpc_offset);
	m68kpc_offset += 4;
	sprintf (buffer,"#$%08lx", (unsigned long)offset);
	break;
     case immi:
	offset = (uae_s32)(uae_s8)(reg & 0xff);
	sprintf (buffer,"#$%08lx", (unsigned long)offset);
	break;
     default:
	break;
    }
    if (buf == 0)
	printf ("%s", buffer);
    else
	strcat (buf, buffer);
    return offset;
}

/* The plan is that this will take over the job of exception 3 handling -
 * the CPU emulation functions will just do a longjmp to m68k_go whenever
 * they hit an odd address. */
static int verify_ea (int reg, amodes mode, wordsizes size, uae_u32 *val)
{
    uae_u16 dp;
    uae_s8 disp8;
    uae_s16 disp16;
    int r;
    uae_u32 dispreg;
    uaecptr addr;
// MJ    uae_s32 offset = 0;

    switch (mode){
     case Dreg:
	*val = m68k_dreg (regs, reg);
	return 1;
     case Areg:
	*val = m68k_areg (regs, reg);
	return 1;

     case Aind:
     case Aipi:
	addr = m68k_areg (regs, reg);
	break;
     case Apdi:
	addr = m68k_areg (regs, reg);
	break;
     case Ad16:
	disp16 = get_iword_1 (m68kpc_offset); m68kpc_offset += 2;
	addr = m68k_areg(regs,reg) + (uae_s16)disp16;
	break;
     case Ad8r:
	addr = m68k_areg (regs, reg);
     d8r_common:
	dp = get_iword_1 (m68kpc_offset); m68kpc_offset += 2;
	disp8 = dp & 0xFF;
	r = (dp & 0x7000) >> 12;
	dispreg = dp & 0x8000 ? m68k_areg(regs,r) : m68k_dreg(regs,r);
	if (!(dp & 0x800)) dispreg = (uae_s32)(uae_s16)(dispreg);
	dispreg <<= (dp >> 9) & 3;

	if (dp & 0x100) {
	    uae_s32 outer = 0, disp = 0;
	    uae_s32 base = addr;
	    if (dp & 0x80) base = 0;
	    if (dp & 0x40) dispreg = 0;
	    if ((dp & 0x30) == 0x20) { disp = (uae_s32)(uae_s16)get_iword_1 (m68kpc_offset); m68kpc_offset += 2; }
	    if ((dp & 0x30) == 0x30) { disp = get_ilong_1 (m68kpc_offset); m68kpc_offset += 4; }
	    base += disp;

	    if ((dp & 0x3) == 0x2) { outer = (uae_s32)(uae_s16)get_iword_1 (m68kpc_offset); m68kpc_offset += 2; }
	    if ((dp & 0x3) == 0x3) { outer = get_ilong_1 (m68kpc_offset); m68kpc_offset += 4; }

	    if (!(dp & 4)) base += dispreg;
	    if (dp & 3) base = get_long (base);
	    if (dp & 4) base += dispreg;

	    addr = base + outer;
	} else {
	  addr += (uae_s32)((uae_s8)disp8) + dispreg;
	}
	break;
     case PC16:
	addr = m68k_getpc () + m68kpc_offset;
	disp16 = get_iword_1 (m68kpc_offset); m68kpc_offset += 2;
	addr += (uae_s16)disp16;
	break;
     case PC8r:
	addr = m68k_getpc () + m68kpc_offset;
	goto d8r_common;
     case absw:
	addr = (uae_s32)(uae_s16)get_iword_1 (m68kpc_offset);
	m68kpc_offset += 2;
	break;
     case absl:
	addr = get_ilong_1 (m68kpc_offset);
	m68kpc_offset += 4;
	break;
     case imm:
	switch (size){
	 case sz_byte:
	    *val = get_iword_1 (m68kpc_offset) & 0xff;
	    m68kpc_offset += 2;
	    break;
	 case sz_word:
	    *val = get_iword_1 (m68kpc_offset) & 0xffff;
	    m68kpc_offset += 2;
	    break;
	 case sz_long:
	    *val = get_ilong_1 (m68kpc_offset);
	    m68kpc_offset += 4;
	    break;
	 default:
	    break;
	}
	return 1;
     case imm0:
	*val = (uae_s32)(uae_s8)get_iword_1 (m68kpc_offset);
	m68kpc_offset += 2;
	return 1;
     case imm1:
	*val = (uae_s32)(uae_s16)get_iword_1 (m68kpc_offset);
	m68kpc_offset += 2;
	return 1;
     case imm2:
	*val = get_ilong_1 (m68kpc_offset);
	m68kpc_offset += 4;
	return 1;
     case immi:
	*val = (uae_s32)(uae_s8)(reg & 0xff);
	return 1;
     default:
	addr = 0;
	break;
    }
    if ((addr & 1) == 0)
	return 1;

    last_addr_for_exception_3 = m68k_getpc () + m68kpc_offset;
    last_fault_for_exception_3 = addr;
    return 0;
}

uae_u32 get_disp_ea_020 (uae_u32 base, uae_u32 dp)
{
    int reg = (dp >> 12) & 15;
    uae_s32 regd = regs.regs[reg];
    if ((dp & 0x800) == 0)
	regd = (uae_s32)(uae_s16)regd;
    regd <<= (dp >> 9) & 3;
    if (dp & 0x100) {
	uae_s32 outer = 0;
	if (dp & 0x80) base = 0;
	if (dp & 0x40) regd = 0;

	if ((dp & 0x30) == 0x20) base += (uae_s32)(uae_s16)next_iword();
	if ((dp & 0x30) == 0x30) base += next_ilong();

	if ((dp & 0x3) == 0x2) outer = (uae_s32)(uae_s16)next_iword();
	if ((dp & 0x3) == 0x3) outer = next_ilong();

	if ((dp & 0x4) == 0) base += regd;
	if (dp & 0x3) base = get_long (base);
	if (dp & 0x4) base += regd;

	return base + outer;
    } else {
	return base + (uae_s32)((uae_s8)dp) + regd;
    }
}

uae_u32 get_disp_ea_000 (uae_u32 base, uae_u32 dp)
{
    int reg = (dp >> 12) & 15;
    uae_s32 regd = regs.regs[reg];
#if 1
    if ((dp & 0x800) == 0)
	regd = (uae_s32)(uae_s16)regd;
    return base + (uae_s8)dp + regd;
#else
    /* Branch-free code... benchmark this again now that
     * things are no longer inline.  */
    uae_s32 regd16;
    uae_u32 mask;
    mask = ((dp & 0x800) >> 11) - 1;
    regd16 = (uae_s32)(uae_s16)regd;
    regd16 &= mask;
    mask = ~mask;
    base += (uae_s8)dp;
    regd &= mask;
    regd |= regd16;
    return base + regd;
#endif
}

void MakeSR (void)
{
#if 0
    assert((regs.t1 & 1) == regs.t1);
    assert((regs.t0 & 1) == regs.t0);
    assert((regs.s & 1) == regs.s);
    assert((regs.m & 1) == regs.m);
    assert((XFLG & 1) == XFLG);
    assert((NFLG & 1) == NFLG);
    assert((ZFLG & 1) == ZFLG);
    assert((VFLG & 1) == VFLG);
    assert((CFLG & 1) == CFLG);
#endif
    regs.sr = ((regs.t1 << 15) | (regs.t0 << 14)
	       | (regs.s << 13) | (regs.m << 12) | (regs.intmask << 8)
	       | (GET_XFLG << 4) | (GET_NFLG << 3) | (GET_ZFLG << 2) | (GET_VFLG << 1)
	       | GET_CFLG);
}

void MakeFromSR (void)
{
    int oldm = regs.m;
    int olds = regs.s;

    regs.t1 = (regs.sr >> 15) & 1;
    regs.t0 = (regs.sr >> 14) & 1;
    regs.s = (regs.sr >> 13) & 1;
    regs.m = (regs.sr >> 12) & 1;
    regs.intmask = (regs.sr >> 8) & 7;
    SET_XFLG ((regs.sr >> 4) & 1);
    SET_NFLG ((regs.sr >> 3) & 1);
    SET_ZFLG ((regs.sr >> 2) & 1);
    SET_VFLG ((regs.sr >> 1) & 1);
    SET_CFLG (regs.sr & 1);
	if (olds != regs.s) {
	    if (olds) {
		if (oldm)
		    regs.msp = m68k_areg(regs, 7);
		else
		    regs.isp = m68k_areg(regs, 7);
		m68k_areg(regs, 7) = regs.usp;
	    } else {
		regs.usp = m68k_areg(regs, 7);
		m68k_areg(regs, 7) = regs.m ? regs.msp : regs.isp;
	    }
	} else if (olds && oldm != regs.m) {
	    if (oldm) {
		regs.msp = m68k_areg(regs, 7);
		m68k_areg(regs, 7) = regs.isp;
	    } else {
		regs.isp = m68k_areg(regs, 7);
		m68k_areg(regs, 7) = regs.msp;
	    }
	}

    regs.spcflags |= SPCFLAG_INT;
    if (regs.t1 || regs.t0)
	regs.spcflags |= SPCFLAG_TRACE;
    else
	regs.spcflags &= ~(SPCFLAG_TRACE/* | SPCFLAG_DOTRACE*/);
}

void Exception(int nr, uaecptr oldpc)
{
   uae_u32 currpc = m68k_getpc ();
   MakeSR();
    if (!regs.s) {
	regs.usp = m68k_areg(regs, 7);
	    m68k_areg(regs, 7) = regs.m ? regs.msp : regs.isp;
	regs.s = 1;
    }
	if (nr == 2 || nr == 3) {	// 16 words on stack
	   // internal register
		m68k_areg(regs, 7) -= 4;
		put_long (m68k_areg(regs, 7), 0);

		// data to write
		m68k_areg(regs, 7) -= 4;
		put_long (m68k_areg(regs, 7), 0);

	    	// internal register
		m68k_areg(regs, 7) -= 4;
		put_long (m68k_areg(regs, 7), 0);

		// data to write
		m68k_areg(regs, 7) -= 4;
		put_long (m68k_areg(regs, 7), last_fault_for_exception_3);

	    	// instruction B prefetch
		m68k_areg(regs, 7) -= 2;
		put_word (m68k_areg(regs, 7), get_word(regs.pc+2));

	    	// instruction C prefetch
		m68k_areg(regs, 7) -= 2;
		put_word (m68k_areg(regs, 7), get_word(regs.pc+4));

	    	// special status register ssw
		m68k_areg(regs, 7) -= 2;
		put_word (m68k_areg(regs, 7), 0x0100);

	    	// internal register
		m68k_areg(regs, 7) -= 2;
		put_word (m68k_areg(regs, 7), 0);

		// vector offset
	    m68k_areg(regs, 7) -= 2;
	    put_word (m68k_areg(regs, 7), 0xa000 + nr * 4);

		// PC
    	    m68k_areg(regs, 7) -= 4;
    	    put_long (m68k_areg(regs, 7), currpc);
	    goto kludge_me_do;
	} else if (nr ==5 || nr == 6 || nr == 7 || nr == 9) {
	    m68k_areg(regs, 7) -= 4;
	    put_long (m68k_areg(regs, 7), oldpc);
	    m68k_areg(regs, 7) -= 2;
	    put_word (m68k_areg(regs, 7), 0x2000 + nr * 4);
	} else if (regs.m && nr >= 24 && nr < 32) {
	    m68k_areg(regs, 7) -= 2;
	    put_word (m68k_areg(regs, 7), nr * 4);
	    m68k_areg(regs, 7) -= 4;
	    put_long (m68k_areg(regs, 7), currpc);
	    m68k_areg(regs, 7) -= 2;
	    put_word (m68k_areg(regs, 7), regs.sr);
	    regs.sr |= (1 << 13);
	    regs.msp = m68k_areg(regs, 7);
	    m68k_areg(regs, 7) = regs.isp;
	    m68k_areg(regs, 7) -= 2;
	    put_word (m68k_areg(regs, 7), 0x1000 + nr * 4);
	} else {
	    m68k_areg(regs, 7) -= 2;
	    put_word (m68k_areg(regs, 7), nr * 4);
	}
    m68k_areg(regs, 7) -= 4;
    put_long (m68k_areg(regs, 7), currpc);
kludge_me_do:
    m68k_areg(regs, 7) -= 2;
    put_word (m68k_areg(regs, 7), regs.sr);
    m68k_setpc (get_long (regs.vbr + 4*nr));
    fill_prefetch_0 ();
    regs.t1 = regs.t0 = regs.m = 0;
    regs.spcflags &= ~(SPCFLAG_TRACE | SPCFLAG_DOTRACE);
}

static void Interrupt(int nr)
{
    assert(nr < 8 && nr >= 0);
    lastint_regs = regs;
    lastint_no = nr;
    Exception(nr+24, 0);

    regs.intmask = nr;
    // why the hell the SPCFLAG_INT is to be set??? (joy)
    // regs.spcflags |= SPCFLAG_INT; (disabled by joy)
}

static void MFPInterrupt(int nr)
{
    // fprintf(stderr, "CPU: in MFPInterrupt\n");
    assert(nr < 16 && nr >= 0);
    lastint_regs = regs;
    lastint_no = 6;
    Exception(nr+64, 0);

    regs.intmask = 6;
}

uae_u32 m68k_move2c (int regno, uae_u32 *regp)
{
/* MJ   if ((CPUType == 1 && (regno & 0x7FF) > 1)
	|| (CPUType < 4 && (regno & 0x7FF) > 2)
	|| (CPUType == 4 && regno == 0x802))
    {
	op_illg (0x4E7B);
	return 0;
    } else {*/
	switch (regno) {
	 case 0: regs.sfc = *regp & 7; break;
	 case 1: regs.dfc = *regp & 7; break;
	 case 2: regs.cacr = *regp & 0x80008000; break;
	 case 3: regs.tce = (flagtype)(((*regp) & 0x8000) ? 1 : 0);
	         regs.tcp = (flagtype)(((*regp) & 0x4000) ? 1 : 0);
		 break;
	 case 4: regs.itt0 = *regp & 0xffffe364; break;
	 case 5: regs.itt1 = *regp & 0xffffe364; break;
	 case 6: regs.dtt0 = *regp & 0xffffe364; break;
	 case 7: regs.dtt1 = *regp & 0xffffe364; break;
	 case 0x800: regs.usp = *regp; break;
	 case 0x801: regs.vbr = *regp; break;
	 case 0x802: regs.caar = *regp & 0xfc; break;
	 case 0x803: regs.msp = *regp; if (regs.m == 1) m68k_areg(regs, 7) = regs.msp; break;
	 case 0x804: regs.isp = *regp; if (regs.m == 0) m68k_areg(regs, 7) = regs.isp; break;
	 case 0x805: regs.mmusr = *regp; break;
	 case 0x806: regs.urp = *regp & 0xffffff00; break;
	 case 0x807: regs.srp = *regp & 0xffffff00; break;
	 default:
	    op_illg (0x4E7B);
	    return 0;
	}
// MJ    }
    return 1;
}

uae_u32 m68k_movec2 (int regno, uae_u32 *regp)
{
/* MJ    if ((CPUType == 1 && (regno & 0x7FF) > 1)
	|| (CPUType < 4 && (regno & 0x7FF) > 2)
	|| (CPUType == 4 && regno == 0x802))
    {
	op_illg (0x4E7A);
	return 0;
    } else {*/
	switch (regno) {
	 case 0: *regp = regs.sfc; break;
	 case 1: *regp = regs.dfc; break;
	 case 2: *regp = regs.cacr; break;
	 case 3: *regp = ((((int)regs.tce) << 15) | (((int)regs.tcp) << 14)); break;
	 case 4: *regp = regs.itt0; break;
	 case 5: *regp = regs.itt1; break;
	 case 6: *regp = regs.dtt0; break;
	 case 7: *regp = regs.dtt1; break;
	 case 0x800: *regp = regs.usp; break;
	 case 0x801: *regp = regs.vbr; break;
	 case 0x802: *regp = regs.caar; break;
	 case 0x803: *regp = regs.m == 1 ? m68k_areg(regs, 7) : regs.msp; break;
	 case 0x804: *regp = regs.m == 0 ? m68k_areg(regs, 7) : regs.isp; break;
	 case 0x805: *regp = regs.mmusr; break;
	 case 0x806: *regp = regs.urp; break;
	 case 0x807: *regp = regs.srp; break;
	 default:
	    op_illg (0x4E7A);
	    return 0;
	}
// MJ    }
    return 1;
}

static __inline__ int
div_unsigned(uae_u32 src_hi, uae_u32 src_lo, uae_u32 div, uae_u32 *quot, uae_u32 *rem)
{
	uae_u32 q = 0, cbit = 0;
	int i;

	if (div <= src_hi) {
	    return 1;
	}
	for (i = 0 ; i < 32 ; i++) {
		cbit = src_hi & 0x80000000ul;
		src_hi <<= 1;
		if (src_lo & 0x80000000ul) src_hi++;
		src_lo <<= 1;
		q = q << 1;
		if (cbit || div <= src_hi) {
			q |= 1;
			src_hi -= div;
		}
	}
	*quot = q;
	*rem = src_hi;
	return 0;
}

void m68k_divl (uae_u32 opcode, uae_u32 src, uae_u16 extra, uaecptr oldpc)
{
#if defined(uae_s64)
    if (src == 0) {
	Exception (5, oldpc);
	return;
    }
    if (extra & 0x800) {
	/* signed variant */
	uae_s64 a = (uae_s64)(uae_s32)m68k_dreg(regs, (extra >> 12) & 7);
	uae_s64 quot, rem;

	if (extra & 0x400) {
	    a &= 0xffffffffu;
	    a |= (uae_s64)m68k_dreg(regs, extra & 7) << 32;
	}
	rem = a % (uae_s64)(uae_s32)src;
	quot = a / (uae_s64)(uae_s32)src;
	if ((quot & UVAL64(0xffffffff80000000)) != 0
	    && (quot & UVAL64(0xffffffff80000000)) != UVAL64(0xffffffff80000000))
	{
	    SET_VFLG (1);
	    SET_NFLG (1);
	    SET_CFLG (0);
	} else {
	    if (((uae_s32)rem < 0) != ((uae_s64)a < 0)) rem = -rem;
	    SET_VFLG (0);
	    SET_CFLG (0);
	    SET_ZFLG (((uae_s32)quot) == 0);
	    SET_NFLG (((uae_s32)quot) < 0);
	    m68k_dreg(regs, extra & 7) = rem;
	    m68k_dreg(regs, (extra >> 12) & 7) = quot;
	}
    } else {
	/* unsigned */
	uae_u64 a = (uae_u64)(uae_u32)m68k_dreg(regs, (extra >> 12) & 7);
	uae_u64 quot, rem;

	if (extra & 0x400) {
	    a &= 0xffffffffu;
	    a |= (uae_u64)m68k_dreg(regs, extra & 7) << 32;
	}
	rem = a % (uae_u64)src;
	quot = a / (uae_u64)src;
	if (quot > 0xffffffffu) {
	    SET_VFLG (1);
	    SET_NFLG (1);
	    SET_CFLG (0);
	} else {
	    SET_VFLG (0);
	    SET_CFLG (0);
	    SET_ZFLG (((uae_s32)quot) == 0);
	    SET_NFLG (((uae_s32)quot) < 0);
	    m68k_dreg(regs, extra & 7) = rem;
	    m68k_dreg(regs, (extra >> 12) & 7) = quot;
	}
    }
#else
    if (src == 0) {
	Exception (5, oldpc);
	return;
    }
    if (extra & 0x800) {
	/* signed variant */
	uae_s32 lo = (uae_s32)m68k_dreg(regs, (extra >> 12) & 7);
	uae_s32 hi = lo < 0 ? -1 : 0;
	uae_s32 save_high;
	uae_u32 quot, rem;
	uae_u32 sign;

	if (extra & 0x400) {
	    hi = (uae_s32)m68k_dreg(regs, extra & 7);
	}
	save_high = hi;
	sign = (hi ^ src);
	if (hi < 0) {
	    hi = ~hi;
	    lo = -lo;
	    if (lo == 0) hi++;
	}
	if ((uae_s32)src < 0) src = -src;
	if (div_unsigned(hi, lo, src, &quot, &rem) ||
	    (sign & 0x80000000) ? quot > 0x80000000 : quot > 0x7fffffff) {
	    SET_VFLG (1);
	    SET_NFLG (1);
	    SET_CFLG (0);
	} else {
	    if (sign & 0x80000000) quot = -quot;
	    if (((uae_s32)rem < 0) != (save_high < 0)) rem = -rem;
	    SET_VFLG (0);
	    SET_CFLG (0);
	    SET_ZFLG (((uae_s32)quot) == 0);
	    SET_NFLG (((uae_s32)quot) < 0);
	    m68k_dreg(regs, extra & 7) = rem;
	    m68k_dreg(regs, (extra >> 12) & 7) = quot;
	}
    } else {
	/* unsigned */
	uae_u32 lo = (uae_u32)m68k_dreg(regs, (extra >> 12) & 7);
	uae_u32 hi = 0;
	uae_u32 quot, rem;

	if (extra & 0x400) {
	    hi = (uae_u32)m68k_dreg(regs, extra & 7);
	}
	if (div_unsigned(hi, lo, src, &quot, &rem)) {
	    SET_VFLG (1);
	    SET_NFLG (1);
	    SET_CFLG (0);
	} else {
	    SET_VFLG (0);
	    SET_CFLG (0);
	    SET_ZFLG (((uae_s32)quot) == 0);
	    SET_NFLG (((uae_s32)quot) < 0);
	    m68k_dreg(regs, extra & 7) = rem;
	    m68k_dreg(regs, (extra >> 12) & 7) = quot;
	}
    }
#endif
}

static __inline__ void
mul_unsigned(uae_u32 src1, uae_u32 src2, uae_u32 *dst_hi, uae_u32 *dst_lo)
{
	uae_u32 r0 = (src1 & 0xffff) * (src2 & 0xffff);
	uae_u32 r1 = ((src1 >> 16) & 0xffff) * (src2 & 0xffff);
	uae_u32 r2 = (src1 & 0xffff) * ((src2 >> 16) & 0xffff);
	uae_u32 r3 = ((src1 >> 16) & 0xffff) * ((src2 >> 16) & 0xffff);
	uae_u32 lo;

	lo = r0 + ((r1 << 16) & 0xffff0000ul);
	if (lo < r0) r3++;
	r0 = lo;
	lo = r0 + ((r2 << 16) & 0xffff0000ul);
	if (lo < r0) r3++;
	r3 += ((r1 >> 16) & 0xffff) + ((r2 >> 16) & 0xffff);
	*dst_lo = lo;
	*dst_hi = r3;
}

void m68k_mull (uae_u32 opcode, uae_u32 src, uae_u16 extra)
{
#if defined(uae_s64)
    if (extra & 0x800) {
	/* signed variant */
	uae_s64 a = (uae_s64)(uae_s32)m68k_dreg(regs, (extra >> 12) & 7);

	a *= (uae_s64)(uae_s32)src;
	SET_VFLG (0);
	SET_CFLG (0);
	SET_ZFLG (a == 0);
	SET_NFLG (a < 0);
	if (extra & 0x400)
	    m68k_dreg(regs, extra & 7) = a >> 32;
	else if ((a & UVAL64(0xffffffff80000000)) != 0
		 && (a & UVAL64(0xffffffff80000000)) != UVAL64(0xffffffff80000000))
	{
	    SET_VFLG (1);
	}
	m68k_dreg(regs, (extra >> 12) & 7) = (uae_u32)a;
    } else {
	/* unsigned */
	uae_u64 a = (uae_u64)(uae_u32)m68k_dreg(regs, (extra >> 12) & 7);

	a *= (uae_u64)src;
	SET_VFLG (0);
	SET_CFLG (0);
	SET_ZFLG (a == 0);
	SET_NFLG (((uae_s64)a) < 0);
	if (extra & 0x400)
	    m68k_dreg(regs, extra & 7) = a >> 32;
	else if ((a & UVAL64(0xffffffff00000000)) != 0) {
	    SET_VFLG (1);
	}
	m68k_dreg(regs, (extra >> 12) & 7) = (uae_u32)a;
    }
#else
    if (extra & 0x800) {
	/* signed variant */
	uae_s32 src1,src2;
	uae_u32 dst_lo,dst_hi;
	uae_u32 sign;

	src1 = (uae_s32)src;
	src2 = (uae_s32)m68k_dreg(regs, (extra >> 12) & 7);
	sign = (src1 ^ src2);
	if (src1 < 0) src1 = -src1;
	if (src2 < 0) src2 = -src2;
	mul_unsigned((uae_u32)src1,(uae_u32)src2,&dst_hi,&dst_lo);
	if (sign & 0x80000000) {
		dst_hi = ~dst_hi;
		dst_lo = -dst_lo;
		if (dst_lo == 0) dst_hi++;
	}
	SET_VFLG (0);
	SET_CFLG (0);
	SET_ZFLG (dst_hi == 0 && dst_lo == 0);
	SET_NFLG (((uae_s32)dst_hi) < 0);
	if (extra & 0x400)
	    m68k_dreg(regs, extra & 7) = dst_hi;
	else if ((dst_hi != 0 || (dst_lo & 0x80000000) != 0)
		 && ((dst_hi & 0xffffffff) != 0xffffffff
		     || (dst_lo & 0x80000000) != 0x80000000))
	{
	    SET_VFLG (1);
	}
	m68k_dreg(regs, (extra >> 12) & 7) = dst_lo;
    } else {
	/* unsigned */
	uae_u32 dst_lo,dst_hi;

	mul_unsigned(src,(uae_u32)m68k_dreg(regs, (extra >> 12) & 7),&dst_hi,&dst_lo);

	SET_VFLG (0);
	SET_CFLG (0);
	SET_ZFLG (dst_hi == 0 && dst_lo == 0);
	SET_NFLG (((uae_s32)dst_hi) < 0);
	if (extra & 0x400)
	    m68k_dreg(regs, extra & 7) = dst_hi;
	else if (dst_hi != 0) {
	    SET_VFLG (1);
	}
	m68k_dreg(regs, (extra >> 12) & 7) = dst_lo;
    }
#endif
}
static char* ccnames[] =
{ "T ","F ","HI","LS","CC","CS","NE","EQ",
  "VC","VS","PL","MI","GE","LT","GT","LE" };

void m68k_reset (void)
{
    m68k_areg (regs, 7) = phys_get_long(0x00000000);
    m68k_setpc (phys_get_long(0x00000004));
    fill_prefetch_0 ();
    regs.kick_mask = 0xF80000;
    regs.s = 1;
    regs.m = 0;
    regs.stopped = 0;
    regs.t1 = 0;
    regs.t0 = 0;
    SET_ZFLG (0);
    SET_XFLG (0);
    SET_CFLG (0);
    SET_VFLG (0);
    SET_NFLG (0);
    regs.spcflags = 0;
    regs.intmask = 7;
    regs.vbr = regs.sfc = regs.dfc = 0;
    /* gb-- moved into {fpp,fpu_x86}.cpp::fpu_init()
    regs.fpcr = regs.fpsr = regs.fpiar = 0; */
    fpu_reset();
    // MMU
    regs.urp = 0;
    regs.srp = 0;
    regs.tce = 0;
//    regs.tcp = 0;    !!! Reset doesn't affect this bit
    regs.dtt0 = 0;
    regs.dtt1 = 0;
    regs.itt0 = 0;
    regs.itt1 = 0;
    regs.mmusr = 0;
    // Cache
    regs.cacr = 0;
    regs.caar = 0;
}

void REGPARAM2 op_illg (uae_u32 opcode)
{
#if DEBUG
    uaecptr pc = m68k_getpc ();
#endif

	if ((opcode & 0xFF00) == 0x7100) {
		struct M68kRegisters r;
		int i;

		// Return from Execute68k()?
		if (opcode == M68K_EXEC_RETURN) {
			regs.spcflags |= SPCFLAG_BRK;
			quit_program = 1;
			return;
		}

		// Call EMUL_OP opcode
		for (i=0; i<8; i++) {
			r.d[i] = m68k_dreg(regs, i);
			r.a[i] = m68k_areg(regs, i);
		}
		MakeSR();
		r.sr = regs.sr;
		EmulOp(opcode, &r);
		for (i=0; i<8; i++) {
			m68k_dreg(regs, i) = r.d[i];
			m68k_areg(regs, i) = r.a[i];
		}
		regs.sr = r.sr;
		MakeFromSR();
		m68k_incpc(2);
		fill_prefetch_0 ();
		return;
	}

    if ((opcode & 0xF000) == 0xA000) {
	Exception(0xA,0);
	return;
    }

//    D(bug("Illegal instruction: %04x at %08lx", opcode, pc));

    if ((opcode & 0xF000) == 0xF000) {
	Exception(0xB,0);
	return;
    }

    D(bug("Illegal instruction: %04x at %08lx", opcode, (unsigned long)pc));

    Exception (4,0);
}

void mmu_op(uae_u32 opcode, uae_u16 extra)
{
#ifdef FULLMMU
    uae_u16 i;
    uaecptr addr = m68k_areg(regs, extra);
    uint16 excep_nono;
    jmp_buf excep_env_old;
    excep_env_old = excep_env;
    uaecptr mask;
    uaecptr atcindex;
    uaecptr rootp;
    uae_u32 root;
    uaecptr apdt, apd;
    uae_u32 pdt, pd;
    flagtype wr;
//    set_special(SPCFLAG_BRK);
#endif
    if ((opcode & 0xFF8) == 0x0500) { /* PFLUSHN instruction (An) */
#ifdef FULLMMU
        for (i = 0; i < ATCSIZE; i++) {
            if (!regs.atcglobald[i]
                && (addr == regs.atcind[i]))
                    regs.atcvald[i] = 0;
            if (!regs.atcglobali[i]
                && (addr == regs.atcini[i]))
                    regs.atcvali[i] = 0;
        }
        regs.mmusr = 0;
#endif
    } else if ((opcode & 0xFF8) == 0x0508) { /* PFLUSH instruction (An) */
#ifdef FULLMMU
        switch (regs.dfc) {
	    case 1:
	    case 2:
                    for (i = 0; i < ATCSIZE; i++) {
                        if ((!regs.atcsuperd[i]) && (addr == regs.atcind[i]))
                            regs.atcvald[i] = 0;
                        if ((!regs.atcsuperi[i]) && (addr == regs.atcini[i]))
                            regs.atcvali[i] = 0;
                    }
	            break;
	    case 5:
	    case 6:
                    for (i = 0; i < ATCSIZE; i++) {
                        if ((regs.atcsuperd[i]) && (addr == regs.atcind[i]))
                            regs.atcvald[i] = 0;
                        if ((regs.atcsuperi[i]) && (addr == regs.atcini[i]))
                            regs.atcvali[i] = 0;
                    }
	            break;
	    default: break;
	}
        regs.mmusr = 0;
#endif
    } else if ((opcode & 0xFF8) == 0x0510) { /* PFLUSHAN instruction */
#ifdef FULLMMU
        for (i = 0; i < ATCSIZE; i++) {
            if (!regs.atcglobald[i]) regs.atcvald[i] = 0;
            if (!regs.atcglobali[i]) regs.atcvali[i] = 0;
        }
        regs.mmusr = 0;
#endif
    } else if ((opcode & 0xFF8) == 0x0518) { /* PFLUSHA instruction */
#ifdef FULLMMU
        for (i = 0; i < ATCSIZE; i++) {
            regs.atcvald[i] = 0;
            regs.atcvali[i] = 0;
        }
        regs.mmusr = 0;
#endif
    } else if ((opcode & 0xFF8) == 0x548) { /* PTESTW instruction */
#ifdef FULLMMU
/* DFC = 1 - data / user
         2 - instr / user
	 5 - data / super
	 6 - instr / super*/
        switch (regs.dfc) {
          case 1:
                  if ((regs.dtt0 & 0x8000)
                      && (!(regs.dtt0 & 0x60) || (regs.dtt0 & 0x40))) {
                    mask = ((~regs.dtt0) & 0xff0000) << 8;
                    if ((addr & mask) == (regs.dtt0 & mask)) {
                      if ((regs.dtt0 & 0x4) != 0) {} // WP??
                      regs.mmusr = 3;
                      return;
                    }
                  }
                  if ((regs.dtt1 & 0x8000)
                      && (!(regs.dtt1 & 0x60) || (regs.dtt1 & 0x40))) {
                    mask = ((~regs.dtt1) & 0xff0000) << 8;
                    if ((addr & mask) == (regs.dtt1 & mask)) {
                      if ((regs.dtt1 & 0x4) != 0) {} // WP??
                      regs.mmusr = 3;
                      return;
                    }
                  }
                  if ((excep_nono = setjmp(excep_env)) == 0) {
                    if (regs.tcp) {
                      atcindex = ((addr << 11) >> 24);
                      rootp = regs.urp | ((addr >> 25) << 2);
                      root = get_long_direct(rootp);
                      if ((root & 0x3) > 1) {
                        wr = (root & 0x4) >> 2;
                        put_long_direct(rootp, root | 0x8);
                        apdt = (root & 0xfffffe00) | ((addr & 0x01fc0000) >> 16);
                        pdt = get_long_direct(apdt);
                        if ((pdt & 0x3) > 1) {
                          wr += (pdt & 0x4) >> 2;
                          put_long_direct(apdt, pdt | 0x8);
                          apd = (pdt & 0xffffff80) | ((addr & 0x0003e000) >> 11);
                          pd = get_long_direct(apd);
                          switch (pd & 0x3) {
                            case 0:  regs.mmusr = 0x800;
                                     excep_env = excep_env_old;
                                     return;
                            case 2:  apd = pd & 0xfffffffc;
                                     pd = get_long_direct(apd);
                                     if (((pd & 0x3) % 2) == 0) {
                                       regs.mmusr = 0x800;
                                       excep_env = excep_env_old;
                                       return;
                                     }
                            default: wr += (pd & 0x4) >> 2;
                                     if (wr > 1) {} // WP??
                                     put_long_direct(apd, pd | 0x18);
                                     regs.atcind[atcindex] = addr & 0xffffe000;
               	                     regs.atcoutd[atcindex] = pd & 0xffffe000;
                                     regs.atcu0d[atcindex] = (pd & 0x00000100) >> 8;
                                     regs.atcu1d[atcindex] = (pd & 0x00000200) >> 9;
                                     regs.atcsuperd[atcindex] = (pd & 0x00000080) >> 7;
                                     regs.atccmd[atcindex] = (pd & 0x00000060) >> 5;
                                     regs.atcmodifd[atcindex] = 1;
                                     regs.atcwritepd[atcindex] = wr;
                                     regs.atcresidd[atcindex] = 1;
                                     regs.atcglobald[atcindex] = (pd & 0x00000400) >> 10;
                                     regs.atcfc2d[atcindex] = regs.s; // ??

                                     regs.mmusr = (pd & 0xffffe000) | (addr & 0x00001000) | 0x11;
                                     regs.mmusr |= pd & 0x000007e0;
                                     regs.mmusr |= wr ? 0x4 : 0;
                          }
                        } else {
                          regs.mmusr = 0x800;
                          excep_env = excep_env_old;
                          return;
                        }
                      } else {
                        regs.mmusr = 0x800;
                        excep_env = excep_env_old;
                        return;
                      }
                    } else {
                      atcindex = ((addr << 12) >> 24);
                      rootp = regs.urp | ((addr >> 25) << 2);
                      root = get_long_direct(rootp);
                      if ((root & 0x3) > 1) {
                        wr = (root & 0x4) >> 2;
                        put_long_direct(rootp, root | 0x8);
                        apdt = (root & 0xfffffe00) | ((addr & 0x01fc0000) >> 16);
                        pdt = get_long_direct(apdt);
                        if ((pdt & 0x3) > 1) {
                          wr += (pdt & 0x4) >> 2;
                          put_long_direct(apdt, pdt | 0x8);
                          apd = (pdt & 0xffffff00) | ((addr & 0x0003f000) >> 10);
                          pd = get_long_direct(apd);
                          switch (pd & 0x3) {
                            case 0:  regs.mmusr = 0x800;
                                     excep_env = excep_env_old;
                                     return;
                            case 2:  apd = pd & 0xfffffffc;
                                     pd = get_long_direct(apd);
                                     if (((pd & 0x3) % 2) == 0) {
                                       regs.mmusr = 0x800;
                                       excep_env = excep_env_old;
                                       return;
                                     }
                            default: wr += (pd & 0x4) >> 2;
                                     if (wr > 1) {} // WP??
                                     put_long_direct(apd, pd | 0x18);
                                     regs.atcind[atcindex] = addr & 0xfffff000;
                                     regs.atcoutd[atcindex] = pd & 0xfffff000;
                                     regs.atcu0d[atcindex] = (pd & 0x00000100) >> 8;
                                     regs.atcu1d[atcindex] = (pd & 0x00000200) >> 9;
                                     regs.atcsuperd[atcindex] = (pd & 0x00000080) >> 7;
                                     regs.atccmd[atcindex] = (pd & 0x00000060) >> 5;
                                     regs.atcmodifd[atcindex] = 1;
                                     regs.atcwritepd[atcindex] = wr;
                                     regs.atcresidd[atcindex] = 1;
                                     regs.atcglobald[atcindex] = (pd & 0x00000400) >> 10;
                                     regs.atcfc2d[atcindex] = regs.s; // ??
                                     regs.mmusr = (pd & 0xfffff000) | 0x11;
                                     regs.mmusr |= pd & 0x000007e0;
                                     regs.mmusr |= wr ? 0x4 : 0;
                          }
                        } else {
                          regs.mmusr = 0x800;
                          excep_env = excep_env_old;
                          return;
                        }
                      } else {
                        regs.mmusr = 0x800;
                        excep_env = excep_env_old;
                        return;
                      }
		    }
                  } else {
                    switch (excep_nono) {
                          case 2: regs.mmusr = 0x800;
                                  excep_env = excep_env_old;
                                  return;
                          default:
                                  excep_env = excep_env_old;
                                  longjmp(excep_env, excep_nono);
                    }
                  }
                  break;
	  case 2:
                  if ((regs.itt0 & 0x8000)
                          && (!(regs.itt0 & 0x60) || (regs.itt0 & 0x40))) {
                      mask = ((~regs.itt0) & 0xff0000) << 8;
                      if ((addr & mask) == (regs.itt0 & mask)) {
                          if ((regs.itt0 & 0x4) != 0) {} // WP??
                              regs.mmusr = 3;
	                      return;
                      }
	          }
                  if ((regs.itt1 & 0x8000)
                          && (!(regs.itt1 & 0x60) || (regs.itt1 & 0x40))) {
                      mask = ((~regs.itt1) & 0xff0000) << 8;
                      if ((addr & mask) == (regs.itt1 & mask)) {
                          if ((regs.itt1 & 0x4) != 0) {} // WP??
                          regs.mmusr = 3;
                          return;
                      }
                  }
                  if ((excep_nono = setjmp(excep_env)) == 0) {
                    if (regs.tcp) {
                      atcindex = ((addr << 11) >> 24);
                      rootp = regs.urp | ((addr >> 25) << 2);
                      root = get_long_direct(rootp);
                      if ((root & 0x3) > 1) {
                        wr = (root & 0x4) >> 2;
                        put_long_direct(rootp, root | 0x8);
                        apdt = (root & 0xfffffe00) | ((addr & 0x01fc0000) >> 16);
                        pdt = get_long_direct(apdt);
                        if ((pdt & 0x3) > 1) {
                          wr += (pdt & 0x4) >> 2;
                          put_long_direct(apdt, pdt | 0x8);
                          apd = (pdt & 0xffffff80) | ((addr & 0x0003e000) >> 11);
                          pd = get_long_direct(apd);
                          switch (pd & 0x3) {
                            case 0:  regs.mmusr = 0x800;
                                     excep_env = excep_env_old;
                                     return;
                            case 2:  apd = pd & 0xfffffffc;
                                     pd = get_long_direct(apd);
                                     if (((pd & 0x3) % 2) == 0) {
                                       regs.mmusr = 0x800;
                                       excep_env = excep_env_old;
                                       return;
                                     }
                            default: wr += (pd & 0x4) >> 2;
                                     if (wr > 1) {} // WP??
                                     put_long_direct(apd, pd | 0x18);
                                     regs.atcini[atcindex] = addr & 0xffffe000;
               	                     regs.atcouti[atcindex] = pd & 0xffffe000;
                                     regs.atcu0i[atcindex] = (pd & 0x00000100) >> 8;
                                     regs.atcu1i[atcindex] = (pd & 0x00000200) >> 9;
                                     regs.atcsuperi[atcindex] = (pd & 0x00000080) >> 7;
                                     regs.atccmi[atcindex] = (pd & 0x00000060) >> 5;
                                     regs.atcmodifi[atcindex] = 1;
                                     regs.atcwritepi[atcindex] = wr;
                                     regs.atcresidi[atcindex] = 1;
                                     regs.atcglobali[atcindex] = (pd & 0x00000400) >> 10;
                                     regs.atcfc2i[atcindex] = regs.s; // ??

                                     regs.mmusr = (pd & 0xffffe000) | (addr & 0x00001000) | 0x11;
                                     regs.mmusr |= pd & 0x000007e0;
                                     regs.mmusr |= wr ? 0x4 : 0;
                          }
                        } else {
                          regs.mmusr = 0x800;
                          excep_env = excep_env_old;
                          return;
                        }
                      } else {
                        excep_env = excep_env_old;
                        regs.mmusr = 0x800;
                        return;
                      }
                    } else {
                      atcindex = ((addr << 12) >> 24);
                      rootp = regs.urp | ((addr >> 25) << 2);
                      root = get_long_direct(rootp);
                      if ((root & 0x3) > 1) {
                        wr = (root & 0x4) >> 2;
                        put_long_direct(rootp, root | 0x8);
                        apdt = (root & 0xfffffe00) | ((addr & 0x01fc0000) >> 16);
                        pdt = get_long_direct(apdt);
                        if ((pdt & 0x3) > 1) {
                          wr += (pdt & 0x4) >> 2;
                          put_long_direct(apdt, pdt | 0x8);
                          apd = (pdt & 0xffffff00) | ((addr & 0x0003f000) >> 10);
                          pd = get_long_direct(apd);
                          switch (pd & 0x3) {
                            case 0:  regs.mmusr = 0x800;
                                     excep_env = excep_env_old;
                                     return;
                            case 2:  apd = pd & 0xfffffffc;
                                     pd = get_long_direct(apd);
                                     if (((pd & 0x3) % 2) == 0) {
                                       regs.mmusr = 0x800;
                                       excep_env = excep_env_old;
                                       return;
                                     }
                            default: wr += (pd & 0x4) >> 2;
                                     if (wr > 1) {} // WP??
                                     put_long_direct(apd, pd | 0x18);
                                     regs.atcini[atcindex] = addr & 0xfffff000;
                                     regs.atcouti[atcindex] = pd & 0xfffff000;
                                     regs.atcu0i[atcindex] = (pd & 0x00000100) >> 8;
                                     regs.atcu1i[atcindex] = (pd & 0x00000200) >> 9;
                                     regs.atcsuperi[atcindex] = (pd & 0x00000080) >> 7;
                                     regs.atccmi[atcindex] = (pd & 0x00000060) >> 5;
                                     regs.atcmodifi[atcindex] = 1;
                                     regs.atcwritepi[atcindex] = wr;
                                     regs.atcresidi[atcindex] = 1;
                                     regs.atcglobali[atcindex] = (pd & 0x00000400) >> 10;
                                     regs.atcfc2i[atcindex] = regs.s; // ??
                                     regs.mmusr = (pd & 0xfffff000) | 0x11;
                                     regs.mmusr |= pd & 0x000007e0;
                                     regs.mmusr |= wr ? 0x4 : 0;
                          }
                          } else {
                            regs.mmusr = 0x800;
                            excep_env = excep_env_old;
                            return;
                          }
                        } else {
                          regs.mmusr = 0x800;
                          excep_env = excep_env_old;
                          return;
                        }
		      }
                    } else {
                      switch (excep_nono) {
                            case 2: regs.mmusr = 0x800;
                                    excep_env = excep_env_old;
                                    return;
                            default:
                                    excep_env = excep_env_old;
                                    longjmp(excep_env, excep_nono);
                      }
                    }
	            break;
          case 5:
                  if ((regs.dtt0 & 0x8000)
		            && (!(regs.dtt0 & 0x20) || (regs.dtt0 & 0x40))) {
                        mask = ((~regs.dtt0) & 0xff0000) << 8;
                        if ((addr & mask) == (regs.dtt0 & mask)) {
                            if ((regs.dtt0 & 0x4) != 0) {} // WP??
                	    regs.mmusr = 3;
	                    return;
               		}
	            }
                    if ((regs.dtt1 & 0x8000)
		            && (!(regs.dtt1 & 0x20) || (regs.dtt1 & 0x40))) {
                        mask = ((~regs.dtt1) & 0xff0000) << 8;
                        if ((addr & mask) == (regs.dtt1 & mask)) {
                            if ((regs.dtt1 & 0x4) != 0) {} // WP??
                            regs.mmusr = 3;
                            return;
                        }
                    }
                  if ((excep_nono = setjmp(excep_env)) == 0) {
                    if (regs.tcp) {
                      atcindex = ((addr << 11) >> 24);
                      rootp = regs.srp | ((addr >> 25) << 2);
                      root = get_long_direct(rootp);
                      if ((root & 0x3) > 1) {
                        wr = (root & 0x4) >> 2;
                        put_long_direct(rootp, root | 0x8);
                        apdt = (root & 0xfffffe00) | ((addr & 0x01fc0000) >> 16);
                        pdt = get_long_direct(apdt);
                        if ((pdt & 0x3) > 1) {
                          wr += (pdt & 0x4) >> 2;
                          put_long_direct(apdt, pdt | 0x8);
                          apd = (pdt & 0xffffff80) | ((addr & 0x0003e000) >> 11);
                          pd = get_long_direct(apd);
                          switch (pd & 0x3) {
                            case 0:  regs.mmusr = 0x800;
                                     excep_env = excep_env_old;
                                     return;
                            case 2:  apd = pd & 0xfffffffc;
                                     pd = get_long_direct(apd);
                                     if (((pd & 0x3) % 2) == 0) {
                                       regs.mmusr = 0x800;
                                       excep_env = excep_env_old;
                                       return;
                                     }
                            default: wr += (pd & 0x4) >> 2;
                                     if (wr > 1) {} // WP??
                                     put_long_direct(apd, pd | 0x18);
                                     regs.atcind[atcindex] = addr & 0xffffe000;
               	                     regs.atcoutd[atcindex] = pd & 0xffffe000;
                                     regs.atcu0d[atcindex] = (pd & 0x00000100) >> 8;
                                     regs.atcu1d[atcindex] = (pd & 0x00000200) >> 9;
                                     regs.atcsuperd[atcindex] = (pd & 0x00000080) >> 7;
                                     regs.atccmd[atcindex] = (pd & 0x00000060) >> 5;
                                     regs.atcmodifd[atcindex] = 1;
                                     regs.atcwritepd[atcindex] = wr;
                                     regs.atcresidd[atcindex] = 1;
                                     regs.atcglobald[atcindex] = (pd & 0x00000400) >> 10;
                                     regs.atcfc2d[atcindex] = regs.s; // ??

                                     regs.mmusr = (pd & 0xffffe000) | (addr & 0x00001000) | 0x11;
                                     regs.mmusr |= pd & 0x000007e0;
                                     regs.mmusr |= wr ? 0x4 : 0;
                          }
                        } else {
                          regs.mmusr = 0x800;
                          excep_env = excep_env_old;
                          return;
                        }
                      } else {
                        regs.mmusr = 0x800;
                        excep_env = excep_env_old;
                        return;
                      }
                    } else {
                      atcindex = ((addr << 12) >> 24);
                      rootp = regs.srp | ((addr >> 25) << 2);
                      root = get_long_direct(rootp);
                      if ((root & 0x3) > 1) {
                        wr = (root & 0x4) >> 2;
                        put_long_direct(rootp, root | 0x8);
                        apdt = (root & 0xfffffe00) | ((addr & 0x01fc0000) >> 16);
                        pdt = get_long_direct(apdt);
                        if ((pdt & 0x3) > 1) {
                          wr += (pdt & 0x4) >> 2;
                          put_long_direct(apdt, pdt | 0x8);
                          apd = (pdt & 0xffffff00) | ((addr & 0x0003f000) >> 10);
                          pd = get_long_direct(apd);
                          switch (pd & 0x3) {
                            case 0:  regs.mmusr = 0x800;
                                     excep_env = excep_env_old;
                                     return;
                            case 2:  apd = pd & 0xfffffffc;
                                     pd = get_long_direct(apd);
                                     if (((pd & 0x3) % 2) == 0) {
                                       regs.mmusr = 0x800;
                                       excep_env = excep_env_old;
                                       return;
                                     }
                            default: wr += (pd & 0x4) >> 2;
                                     if (wr > 1) {} // WP??
                                     put_long_direct(apd, pd | 0x18);
                                     regs.atcind[atcindex] = addr & 0xfffff000;
                                     regs.atcoutd[atcindex] = pd & 0xfffff000;
                                     regs.atcu0d[atcindex] = (pd & 0x00000100) >> 8;
                                     regs.atcu1d[atcindex] = (pd & 0x00000200) >> 9;
                                     regs.atcsuperd[atcindex] = (pd & 0x00000080) >> 7;
                                     regs.atccmd[atcindex] = (pd & 0x00000060) >> 5;
                                     regs.atcmodifd[atcindex] = 1;
                                     regs.atcwritepd[atcindex] = wr;
                                     regs.atcresidd[atcindex] = 1;
                                     regs.atcglobald[atcindex] = (pd & 0x00000400) >> 10;
                                     regs.atcfc2d[atcindex] = regs.s; // ??
                                     regs.mmusr = (pd & 0xfffff000) | 0x11;
                                     regs.mmusr |= pd & 0x000007e0;
                                     regs.mmusr |= wr ? 0x4 : 0;
                            }
                          } else {
                            regs.mmusr = 0x800;
                            excep_env = excep_env_old;
                            return;
                          }
                        } else {
                          regs.mmusr = 0x800;
                          excep_env = excep_env_old;
                          return;
                        }
                      }
                    } else {
                      switch (excep_nono) {
                            case 2: regs.mmusr = 0x800;
                                    excep_env = excep_env_old;
                                    return;
                            default:
                                    excep_env = excep_env_old;
                                    longjmp(excep_env, excep_nono);
                      }
                    }
                    break;
	    case 6:
                    if ((regs.itt0 & 0x8000)
		            && (!(regs.itt0 & 0x20) || (regs.itt0 & 0x40))) {
                        mask = ((~regs.itt0) & 0xff0000) << 8;
                        if ((addr & mask) == (regs.itt0 & mask)) {
                            if ((regs.itt0 & 0x4) != 0) {} // WP??
                	    regs.mmusr = 3;
	                    return;
               		}
	            }
                    if ((regs.itt1 & 0x8000)
		            && (!(regs.itt1 & 0x20) || (regs.itt1 & 0x40))) {
                        mask = ((~regs.itt1) & 0xff0000) << 8;
                        if ((addr & mask) == (regs.itt1 & mask)) {
                            if ((regs.itt1 & 0x4) != 0) {} // WP??
                            regs.mmusr = 3;
                            return;
                        }
                    }
                  if ((excep_nono = setjmp(excep_env)) == 0) {
                    if (regs.tcp) {
                      atcindex = ((addr << 11) >> 24);
                      rootp = regs.srp | ((addr >> 25) << 2);
                      root = get_long_direct(rootp);
                      if ((root & 0x3) > 1) {
                        wr = (root & 0x4) >> 2;
                        put_long_direct(rootp, root | 0x8);
                        apdt = (root & 0xfffffe00) | ((addr & 0x01fc0000) >> 16);
                        pdt = get_long_direct(apdt);
                        if ((pdt & 0x3) > 1) {
                          wr += (pdt & 0x4) >> 2;
                          put_long_direct(apdt, pdt | 0x8);
                          apd = (pdt & 0xffffff80) | ((addr & 0x0003e000) >> 11);
                          pd = get_long_direct(apd);
                          switch (pd & 0x3) {
                            case 0:  regs.mmusr = 0x800;
                                     excep_env = excep_env_old;
                                     return;
                            case 2:  apd = pd & 0xfffffffc;
                                     pd = get_long_direct(apd);
                                     if (((pd & 0x3) % 2) == 0) {
                                       regs.mmusr = 0x800;
                                       excep_env = excep_env_old;
                                       return;
                                     }
                            default: wr += (pd & 0x4) >> 2;
                                     if (wr > 1) {} // WP??
                                     put_long_direct(apd, pd | 0x18);
                                     regs.atcini[atcindex] = addr & 0xffffe000;
               	                     regs.atcouti[atcindex] = pd & 0xffffe000;
                                     regs.atcu0i[atcindex] = (pd & 0x00000100) >> 8;
                                     regs.atcu1i[atcindex] = (pd & 0x00000200) >> 9;
                                     regs.atcsuperi[atcindex] = (pd & 0x00000080) >> 7;
                                     regs.atccmi[atcindex] = (pd & 0x00000060) >> 5;
                                     regs.atcmodifi[atcindex] = 1;
                                     regs.atcwritepi[atcindex] = wr;
                                     regs.atcresidi[atcindex] = 1;
                                     regs.atcglobali[atcindex] = (pd & 0x00000400) >> 10;
                                     regs.atcfc2i[atcindex] = regs.s; // ??

                                     regs.mmusr = (pd & 0xffffe000) | (addr & 0x00001000) | 0x11;
                                     regs.mmusr |= pd & 0x000007e0;
                                     regs.mmusr |= wr ? 0x4 : 0;
                          }
                        } else {
                          regs.mmusr = 0x800;
                          excep_env = excep_env_old;
                          return;
                        }
                      } else {
                        regs.mmusr = 0x800;
                        excep_env = excep_env_old;
                        return;
                      }
                    } else {
                      atcindex = ((addr << 12) >> 24);
                      rootp = regs.srp | ((addr >> 25) << 2);
                      root = get_long_direct(rootp);
                      if ((root & 0x3) > 1) {
                        wr = (root & 0x4) >> 2;
                        put_long_direct(rootp, root | 0x8);
                        apdt = (root & 0xfffffe00) | ((addr & 0x01fc0000) >> 16);
                        pdt = get_long_direct(apdt);
                        if ((pdt & 0x3) > 1) {
                          wr += (pdt & 0x4) >> 2;
                          put_long_direct(apdt, pdt | 0x8);
                          apd = (pdt & 0xffffff00) | ((addr & 0x0003f000) >> 10);
                          pd = get_long_direct(apd);
                          switch (pd & 0x3) {
                            case 0:  regs.mmusr = 0x800;
                                     excep_env = excep_env_old;
                                     return;
                            case 2:  apd = pd & 0xfffffffc;
                                     pd = get_long_direct(apd);
                                     if (((pd & 0x3) % 2) == 0) {
                                       regs.mmusr = 0x800;
                                       excep_env = excep_env_old;
                                       return;
                                     }
                            default: wr += (pd & 0x4) >> 2;
                                     if (wr > 1) {} // WP??
                                     put_long_direct(apd, pd | 0x18);
                                     regs.atcini[atcindex] = addr & 0xfffff000;
                                     regs.atcouti[atcindex] = pd & 0xfffff000;
                                     regs.atcu0i[atcindex] = (pd & 0x00000100) >> 8;
                                     regs.atcu1i[atcindex] = (pd & 0x00000200) >> 9;
                                     regs.atcsuperi[atcindex] = (pd & 0x00000080) >> 7;
                                     regs.atccmi[atcindex] = (pd & 0x00000060) >> 5;
                                     regs.atcmodifi[atcindex] = 1;
                                     regs.atcwritepi[atcindex] = wr;
                                     regs.atcresidi[atcindex] = 1;
                                     regs.atcglobali[atcindex] = (pd & 0x00000400) >> 10;
                                     regs.atcfc2i[atcindex] = regs.s; // ??
                                     regs.mmusr = (pd & 0xfffff000) | 0x11;
                                     regs.mmusr |= pd & 0x000007e0;
                                     regs.mmusr |= wr ? 0x4 : 0;
                          }
                        } else {
                          regs.mmusr = 0x800;
                          excep_env = excep_env_old;
                          return;
                        }
                      } else {
                        regs.mmusr = 0x800;
                        excep_env = excep_env_old;
                        return;
                      }
		      }
                    } else {
                      switch (excep_nono) {
                            case 2: regs.mmusr = 0x800;
                                    excep_env = excep_env_old;
                                    return;
                            default:
                                    excep_env = excep_env_old;
                                    longjmp(excep_env, excep_nono);
                      }
                    }

                    break;
	    default: break;
	}
/*
	if (regs.tcp) {
            atcindex = ((addr << 11) >> 24);
	    if ((excep_nono = setjmp(excep_env)) == 0) {
	        rootp = regs.srp | ((addr >> 25) << 2);
	    } else {
		switch (excep_nono) {
		    case 2: regs.mmusr = 0x800;
		            excep_env = excep_env_old;
		            return;
		    default:
		            excep_env = excep_env_old;
			    longjmp(excep_env, excep_nono);
		}
	    }
	    root = get_long_direct(rootp);
	    if ((root & 0x3) > 1) {
	        wr = (root & 0x4) >> 2;
	        put_long_direct(rootp, root | 0x8);
		if ((excep_nono = setjmp(excep_env)) == 0) {
		    apdt = (root & 0xfffffe00) | ((addr & 0x01fc0000) >> 16);
		} else {
		    switch (excep_nono) {
		        case 2: regs.mmusr = 0x800;
			        excep_env = excep_env_old;
		                return;
			default:
		        	excep_env = excep_env_old;
				longjmp(excep_env, excep_nono);
		    }
		}
	        pdt = get_long_direct(apdt);
	        if ((pdt & 0x3) > 1) {
	            wr += (pdt & 0x4) >> 2;
		    put_long_direct(apdt, pdt | 0x8);
		    if ((excep_nono = setjmp(excep_env)) == 0) {
		       	apd = (pdt & 0xffffff80) | ((addr & 0x0003e000) >> 11);
		    } else {
			switch (excep_nono) {
			    case 2: regs.mmusr = 0x800;
			            excep_env = excep_env_old;
				    return;
			    default:
			            excep_env = excep_env_old;
				    longjmp(excep_env, excep_nono);
			}
		    }
                    pd = get_long_direct(apd);
		    switch (pd & 0x3) {
		       	case 0:  regs.mmusr = 0x800;
			         return;
		       	case 2:  if ((excep_nono = setjmp(excep_env)) == 0) {
				     apd = pd & 0xfffffffc;
				 } else {
				     switch (excep_nono) {
				         case 2: regs.mmusr = 0x800;
					         excep_env = excep_env_old;
						 return;
					default:
				        	excep_env = excep_env_old;
						longjmp(excep_env, excep_nono);
				     }
				 }
		        	 pd = get_long_direct(apd);
			         if (((pd & 0x3) % 2) == 0) {
				     regs.mmusr = 0x800;
				     return;
				 }
                    	default: wr += (pd & 0x4) >> 2;
				 if (!wr) throw access_error(addr);
                        	 put_long_direct(apd, pd | 0x18);
			         regs.atcind[atcindex] = addr & 0xffffe000;
			         regs.atcini[atcindex] = addr & 0xffffe000;
				 regs.atcouti[atcindex] = pd & 0xffffe000;
               			 regs.atcoutd[atcindex] = pd & 0xffffe000;
				 regs.atcu0i[atcindex] = (pd & 0x00000100) >> 8;
                         	 regs.atcu0d[atcindex] = (pd & 0x00000100) >> 8;
				 regs.atcu1i[atcindex] = (pd & 0x00000200) >> 9;
                         	 regs.atcu1d[atcindex] = (pd & 0x00000200) >> 9;
				 regs.atcsuperi[atcindex] = (pd & 0x00000080) >> 7;
                         	 regs.atcsuperd[atcindex] = (pd & 0x00000080) >> 7;
				 regs.atccmi[atcindex] = (pd & 0x00000060) >> 5;
                         	 regs.atccmd[atcindex] = (pd & 0x00000060) >> 5;
				 regs.atcmodifi[atcindex] = 1;
                         	 regs.atcmodifd[atcindex] = 1;
				 regs.atcwritepi[atcindex] = wr;
                         	 regs.atcwritepd[atcindex] = wr;
				 regs.atcresidi[atcindex] = 1;
                         	 regs.atcresidd[atcindex] = 1;
				 regs.atcglobali[atcindex] = (pd & 0x00000400) >> 10;
                         	 regs.atcglobald[atcindex] = (pd & 0x00000400) >> 10;
				 regs.atcfc2i[atcindex] = regs.s; // ??
                         	 regs.atcfc2d[atcindex] = regs.s; // ??

				 regs.mmusr = (pd & 0xffffe000) | (addr & 0x00001000) | 0x11;
				 regs.mmusr |= pd & 0x000007e0;
				 regs.mmusr |= wr ? 0x4 : 0;
		    }
	        } else {
		    regs.mmusr = 0x800;
		    return;
		}
	    } else {
		regs.mmusr = 0x800;
		return;
	    }
        } else {
            atcindex = ((addr << 12) >> 24);
	    if ((excep_nono = setjmp(excep_env)) == 0) {
		rootp = regs.srp | ((addr >> 25) << 2);
	    } else {
		regs.mmusr = 0x800;
                excep_env = excep_env_old;
		return;
	    }
	    root = get_long_direct(rootp);
	    if ((root & 0x3) > 1) {
	       	wr = (root & 0x4) >> 2;
	       	put_long_direct(rootp, root | 0x8);
		if ((excep_nono = setjmp(excep_env)) == 0) {
	            apdt = (root & 0xfffffe00) | ((addr & 0x01fc0000) >> 16);
		} else {
		    switch (excep_nono) {
		        case 2: regs.mmusr = 0x800;
			        excep_env = excep_env_old;
		                return;
			default:
		        	excep_env = excep_env_old;
				longjmp(excep_env, excep_nono);
		    }
		}
	        pdt = get_long_direct(apdt);
	        if ((pdt & 0x3) > 1) {
	            wr += (pdt & 0x4) >> 2;
		    put_long_direct(apdt, pdt | 0x8);
		    if ((excep_nono = setjmp(excep_env)) == 0) {
		    	apd = (pdt & 0xffffff00) | ((addr & 0x0003f000) >> 10);
		    } else {
		        switch (excep_nono) {
		            case 2: regs.mmusr = 0x800;
			            excep_env = excep_env_old;
		                    return;
			    default:
		        	    excep_env = excep_env_old;
				    longjmp(excep_env, excep_nono);
			}
		    }
                    pd = get_long_direct(apd);
		    switch (pd & 0x3) {
		        case 0:  regs.mmusr = 0x800;
			         return;
		        case 2:  if ((excep_nono = setjmp(excep_env)) == 0) {
			             apd = pd & 0xfffffffc;
				 } else {
				     switch (excep_nono) {
				        case 2: regs.mmusr = 0x800;
					         excep_env = excep_env_old;
						 return;
					default:
			        		excep_env = excep_env_old;
						longjmp(excep_env, excep_nono);
				     }
				 }
		                 pd = get_long_direct(apd);
			         if (((pd & 0x3) % 2) == 0) {
				     regs.mmusr = 0x800;
				     return;
				 }
                        default: wr += (pd & 0x4) >> 2;
				 if (!wr) throw access_error(addr);
                                 put_long_direct(apd, pd | 0x18);
			         regs.atcind[atcindex] = addr & 0xfffff000;
			         regs.atcini[atcindex] = addr & 0xfffff000;
				 regs.atcouti[atcindex] = pd & 0xfffff000;
				 regs.atcoutd[atcindex] = pd & 0xfffff000;
				 regs.atcu0i[atcindex] = (pd & 0x00000100) >> 8;
                         	 regs.atcu0d[atcindex] = (pd & 0x00000100) >> 8;
				 regs.atcu1i[atcindex] = (pd & 0x00000200) >> 9;
                         	 regs.atcu1d[atcindex] = (pd & 0x00000200) >> 9;
				 regs.atcsuperi[atcindex] = (pd & 0x00000080) >> 7;
                         	 regs.atcsuperd[atcindex] = (pd & 0x00000080) >> 7;
				 regs.atccmi[atcindex] = (pd & 0x00000060) >> 5;
                         	 regs.atccmd[atcindex] = (pd & 0x00000060) >> 5;
				 regs.atcmodifi[atcindex] = 1;
                         	 regs.atcmodifd[atcindex] = 1;
				 regs.atcwritepi[atcindex] = wr;
                         	 regs.atcwritepd[atcindex] = wr;
				 regs.atcresidi[atcindex] = 1;
                         	 regs.atcresidd[atcindex] = 1;
				 regs.atcglobali[atcindex] = (pd & 0x00000400) >> 10;
                         	 regs.atcglobald[atcindex] = (pd & 0x00000400) >> 10;
				 regs.atcfc2i[atcindex] = regs.s; // ??
                         	 regs.atcfc2d[atcindex] = regs.s; // ??
				 				                      	
				 regs.mmusr = (pd & 0xfffff000) | 0x11;
				 regs.mmusr |= pd & 0x000007e0;
				 regs.mmusr |= wr ? 0x4 : 0;
		    }
	        } else {
		    regs.mmusr = 0x800;
		    return;
		}
	    } else {
		regs.mmusr = 0x800;
		return;
	    }
	}*/
#endif /* FULLMMU */
    } else if ((opcode & 0xFF8) == 0x568) { /* PTESTR instruction */
#ifdef FULLMMU
        switch (regs.dfc) {
	    case 1:
                    if ((regs.dtt0 & 0x8000)
		            && (!(regs.dtt0 & 0x60) || (regs.dtt0 & 0x40))) {
                        mask = ((~regs.dtt0) & 0xff0000) << 8;
                        if ((addr & mask) == (regs.dtt0 & mask)) {
                	    regs.mmusr = 3;
	                    return;
               		}
	            }
                    if ((regs.dtt1 & 0x8000)
		            && (!(regs.dtt1 & 0x60) || (regs.dtt1 & 0x40))) {
                        mask = ((~regs.dtt1) & 0xff0000) << 8;
                        if ((addr & mask) == (regs.dtt1 & mask)) {
                            regs.mmusr = 3;
                            return;
                        }
                    }
                    if ((excep_nono = setjmp(excep_env)) == 0) {
                      if (regs.tcp) {
                        atcindex = ((addr << 11) >> 24);
                        rootp = regs.urp | ((addr >> 25) << 2);
                        root = get_long_direct(rootp);
                        if ((root & 0x3) > 1) {
                          wr = (root & 0x4) >> 2;
                          put_long_direct(rootp, root | 0x8);
                          apdt = (root & 0xfffffe00) | ((addr & 0x01fc0000) >> 16);
                          pdt = get_long_direct(apdt);
                          if ((pdt & 0x3) > 1) {
                            wr += (pdt & 0x4) >> 2;
                            put_long_direct(apdt, pdt | 0x8);
                            apd = (pdt & 0xffffff80) | ((addr & 0x0003e000) >> 11);
                            pd = get_long_direct(apd);
                            switch (pd & 0x3) {
                              case 0:  regs.mmusr = 0x800;
                                       excep_env = excep_env_old;
                                       return;
                              case 2:  apd = pd & 0xfffffffc;
                                       pd = get_long_direct(apd);
                                       if (((pd & 0x3) % 2) == 0) {
                                         regs.mmusr = 0x800;
                                         excep_env = excep_env_old;
                                         return;
                                       }
                              default: wr += (pd & 0x4) >> 2;
                                       put_long_direct(apd, pd | 0x18);
                                       regs.atcind[atcindex] = addr & 0xffffe000;
                                       regs.atcoutd[atcindex] = pd & 0xffffe000;
                                       regs.atcu0d[atcindex] = (pd & 0x00000100) >> 8;
                                       regs.atcu1d[atcindex] = (pd & 0x00000200) >> 9;
                                       regs.atcsuperd[atcindex] = (pd & 0x00000080) >> 7;
                                       regs.atccmd[atcindex] = (pd & 0x00000060) >> 5;
                                       regs.atcmodifd[atcindex] = 1;
                                       regs.atcwritepd[atcindex] = wr;
                                       regs.atcresidd[atcindex] = 1;
                                       regs.atcglobald[atcindex] = (pd & 0x00000400) >> 10;
                                       regs.atcfc2d[atcindex] = regs.s; // ??
                                       regs.mmusr = (pd & 0xffffe000) | (addr & 0x00001000) | 0x11;
                                       regs.mmusr |= pd & 0x000007e0;
                                       regs.mmusr |= wr ? 0x4 : 0;
                            }
                          } else {
                            regs.mmusr = 0x800;
                            excep_env = excep_env_old;
                            return;
                          }
                        } else {
                          regs.mmusr = 0x800;
                          excep_env = excep_env_old;
                          return;
                        }
                      } else {
                        atcindex = ((addr << 12) >> 24);
                        rootp = regs.urp | ((addr >> 25) << 2);
                        root = get_long_direct(rootp);
                        if ((root & 0x3) > 1) {
                          wr = (root & 0x4) >> 2;
                          put_long_direct(rootp, root | 0x8);
                          apdt = (root & 0xfffffe00) | ((addr & 0x01fc0000) >> 16);
                          pdt = get_long_direct(apdt);
                          if ((pdt & 0x3) > 1) {
                            wr += (pdt & 0x4) >> 2;
                            put_long_direct(apdt, pdt | 0x8);
                            apd = (pdt & 0xffffff00) | ((addr & 0x0003f000) >> 10);
                            pd = get_long_direct(apd);
                            switch (pd & 0x3) {
                              case 0:  regs.mmusr = 0x800;
                                       excep_env = excep_env_old;
                                       return;
                              case 2:  apd = pd & 0xfffffffc;
                                       pd = get_long_direct(apd);
                                       if (((pd & 0x3) % 2) == 0) {
                                         regs.mmusr = 0x800;
                                         excep_env = excep_env_old;
                                         return;
                                       }
                              default: wr += (pd & 0x4) >> 2;
                                       put_long_direct(apd, pd | 0x18);
                                       regs.atcind[atcindex] = addr & 0xfffff000;
                                       regs.atcoutd[atcindex] = pd & 0xfffff000;
                                       regs.atcu0d[atcindex] = (pd & 0x00000100) >> 8;
                                       regs.atcu1d[atcindex] = (pd & 0x00000200) >> 9;
                                       regs.atcsuperd[atcindex] = (pd & 0x00000080) >> 7;
                                       regs.atccmd[atcindex] = (pd & 0x00000060) >> 5;
                                       regs.atcmodifd[atcindex] = 1;
                                       regs.atcwritepd[atcindex] = wr;
                                       regs.atcresidd[atcindex] = 1;
                                       regs.atcglobald[atcindex] = (pd & 0x00000400) >> 10;
                                       regs.atcfc2d[atcindex] = regs.s; // ??
                                       regs.mmusr = (pd & 0xfffff000) | 0x11;
                                       regs.mmusr |= pd & 0x000007e0;
                                       regs.mmusr |= wr ? 0x4 : 0;
                            }
                          } else {
                            regs.mmusr = 0x800;
                            excep_env = excep_env_old;
                            return;
                          }
                        } else {
                          regs.mmusr = 0x800;
                          excep_env = excep_env_old;
                          return;
                        }
                      }
                    } else {
                      switch (excep_nono) {
                        case 2: regs.mmusr = 0x800;
                                excep_env = excep_env_old;
                                return;
                        default:
                                excep_env = excep_env_old;
                                longjmp(excep_env, excep_nono);
                      }
                    }
                    break;
	    case 2:
                    if ((regs.itt0 & 0x8000)
		            && (!(regs.itt0 & 0x60) || (regs.itt0 & 0x40))) {
                        mask = ((~regs.itt0) & 0xff0000) << 8;
                        if ((addr & mask) == (regs.itt0 & mask)) {
                	    regs.mmusr = 3;
	                    return;
               		}
	            }
                    if ((regs.itt1 & 0x8000)
		            && (!(regs.itt1 & 0x60) || (regs.itt1 & 0x40))) {
                        mask = ((~regs.itt1) & 0xff0000) << 8;
                        if ((addr & mask) == (regs.itt1 & mask)) {
                            regs.mmusr = 3;
                            return;
                        }
                    }
                    if ((excep_nono = setjmp(excep_env)) == 0) {
                      if (regs.tcp) {
                        atcindex = ((addr << 11) >> 24);
                        rootp = regs.urp | ((addr >> 25) << 2);
                        root = get_long_direct(rootp);
                        if ((root & 0x3) > 1) {
                          wr = (root & 0x4) >> 2;
                          put_long_direct(rootp, root | 0x8);
                          apdt = (root & 0xfffffe00) | ((addr & 0x01fc0000) >> 16);
                          pdt = get_long_direct(apdt);
                          if ((pdt & 0x3) > 1) {
                            wr += (pdt & 0x4) >> 2;
                            put_long_direct(apdt, pdt | 0x8);
                            apd = (pdt & 0xffffff80) | ((addr & 0x0003e000) >> 11);
                            pd = get_long_direct(apd);
                            switch (pd & 0x3) {
                              case 0:  regs.mmusr = 0x800;
                                       excep_env = excep_env_old;
                                       return;
                              case 2:  apd = pd & 0xfffffffc;
                                       pd = get_long_direct(apd);
                                       if (((pd & 0x3) % 2) == 0) {
                                         regs.mmusr = 0x800;
                                         excep_env = excep_env_old;
                                         return;
                                       }
                              default: wr += (pd & 0x4) >> 2;
                                       put_long_direct(apd, pd | 0x18);
                                       regs.atcini[atcindex] = addr & 0xffffe000;
                                       regs.atcouti[atcindex] = pd & 0xffffe000;
                                       regs.atcu0i[atcindex] = (pd & 0x00000100) >> 8;
                                       regs.atcu1i[atcindex] = (pd & 0x00000200) >> 9;
                                       regs.atcsuperi[atcindex] = (pd & 0x00000080) >> 7;
                                       regs.atccmi[atcindex] = (pd & 0x00000060) >> 5;
                                       regs.atcmodifi[atcindex] = 1;
                                       regs.atcwritepi[atcindex] = wr;
                                       regs.atcresidi[atcindex] = 1;
                                       regs.atcglobali[atcindex] = (pd & 0x00000400) >> 10;
                                       regs.atcfc2i[atcindex] = regs.s; // ??
                                       regs.mmusr = (pd & 0xffffe000) | (addr & 0x00001000) | 0x11;
                                       regs.mmusr |= pd & 0x000007e0;
                                       regs.mmusr |= wr ? 0x4 : 0;
                            }
                          } else {
                            regs.mmusr = 0x800;
                            excep_env = excep_env_old;
                            return;
                          }
                        } else {
                          regs.mmusr = 0x800;
                          excep_env = excep_env_old;
                          return;
                        }
                      } else {
                        atcindex = ((addr << 12) >> 24);
                        rootp = regs.urp | ((addr >> 25) << 2);
                        root = get_long_direct(rootp);
                        if ((root & 0x3) > 1) {
                          wr = (root & 0x4) >> 2;
                          put_long_direct(rootp, root | 0x8);
                          apdt = (root & 0xfffffe00) | ((addr & 0x01fc0000) >> 16);
                          pdt = get_long_direct(apdt);
                          if ((pdt & 0x3) > 1) {
                            wr += (pdt & 0x4) >> 2;
                            put_long_direct(apdt, pdt | 0x8);
                            apd = (pdt & 0xffffff00) | ((addr & 0x0003f000) >> 10);
                            pd = get_long_direct(apd);
                            switch (pd & 0x3) {
                              case 0:  regs.mmusr = 0x800;
                                       excep_env = excep_env_old;
                                       return;
                              case 2:  apd = pd & 0xfffffffc;
                                       pd = get_long_direct(apd);
                                       if (((pd & 0x3) % 2) == 0) {
                                         regs.mmusr = 0x800;
                                         excep_env = excep_env_old;
                                         return;
                                       }
                              default: wr += (pd & 0x4) >> 2;
                                       put_long_direct(apd, pd | 0x18);
                                       regs.atcini[atcindex] = addr & 0xfffff000;
                                       regs.atcouti[atcindex] = pd & 0xfffff000;
                                       regs.atcu0i[atcindex] = (pd & 0x00000100) >> 8;
                                       regs.atcu1i[atcindex] = (pd & 0x00000200) >> 9;
                                       regs.atcsuperi[atcindex] = (pd & 0x00000080) >> 7;
                                       regs.atccmi[atcindex] = (pd & 0x00000060) >> 5;
                                       regs.atcmodifi[atcindex] = 1;
                                       regs.atcwritepi[atcindex] = wr;
                                       regs.atcresidi[atcindex] = 1;
                                       regs.atcglobali[atcindex] = (pd & 0x00000400) >> 10;
                                       regs.atcfc2i[atcindex] = regs.s; // ??
                                       regs.mmusr = (pd & 0xfffff000) | 0x11;
                                       regs.mmusr |= pd & 0x000007e0;
                                       regs.mmusr |= wr ? 0x4 : 0;
                            }
                          } else {
                            regs.mmusr = 0x800;
                            excep_env = excep_env_old;
                            return;
                          }
                        } else {
                          regs.mmusr = 0x800;
                          excep_env = excep_env_old;
                          return;
                        }
                      }
                    } else {
                      switch (excep_nono) {
                        case 2: regs.mmusr = 0x800;
                                excep_env = excep_env_old;
                                return;
                        default:
                                excep_env = excep_env_old;
                                longjmp(excep_env, excep_nono);
                      }
                    }

	            break;
	    case 5:
                    if ((regs.dtt0 & 0x8000)
		            && (!(regs.dtt0 & 0x20) || (regs.dtt0 & 0x40))) {
                        mask = ((~regs.dtt0) & 0xff0000) << 8;
                        if ((addr & mask) == (regs.dtt0 & mask)) {
                	    regs.mmusr = 3;
	                    return;
               		}
	            }
                    if ((regs.dtt1 & 0x8000)
		            && (!(regs.dtt1 & 0x20) || (regs.dtt1 & 0x40))) {
                        mask = ((~regs.dtt1) & 0xff0000) << 8;
                        if ((addr & mask) == (regs.dtt1 & mask)) {
                            regs.mmusr = 3;
                            return;
                        }
                    }
                    if ((excep_nono = setjmp(excep_env)) == 0) {
                      if (regs.tcp) {
                        atcindex = ((addr << 11) >> 24);
                        rootp = regs.srp | ((addr >> 25) << 2);
                        root = get_long_direct(rootp);
                        if ((root & 0x3) > 1) {
                          wr = (root & 0x4) >> 2;
                          put_long_direct(rootp, root | 0x8);
                          apdt = (root & 0xfffffe00) | ((addr & 0x01fc0000) >> 16);
                          pdt = get_long_direct(apdt);
                          if ((pdt & 0x3) > 1) {
                            wr += (pdt & 0x4) >> 2;
                            put_long_direct(apdt, pdt | 0x8);
                            apd = (pdt & 0xffffff80) | ((addr & 0x0003e000) >> 11);
                            pd = get_long_direct(apd);
                            switch (pd & 0x3) {
                              case 0:  regs.mmusr = 0x800;
                                       excep_env = excep_env_old;
                                       return;
                              case 2:  apd = pd & 0xfffffffc;
                                       pd = get_long_direct(apd);
                                       if (((pd & 0x3) % 2) == 0) {
                                         regs.mmusr = 0x800;
                                         excep_env = excep_env_old;
                                         return;
                                       }
                              default: wr += (pd & 0x4) >> 2;
                                       put_long_direct(apd, pd | 0x18);
                                       regs.atcind[atcindex] = addr & 0xffffe000;
                                       regs.atcoutd[atcindex] = pd & 0xffffe000;
                                       regs.atcu0d[atcindex] = (pd & 0x00000100) >> 8;
                                       regs.atcu1d[atcindex] = (pd & 0x00000200) >> 9;
                                       regs.atcsuperd[atcindex] = (pd & 0x00000080) >> 7;
                                       regs.atccmd[atcindex] = (pd & 0x00000060) >> 5;
                                       regs.atcmodifd[atcindex] = 1;
                                       regs.atcwritepd[atcindex] = wr;
                                       regs.atcresidd[atcindex] = 1;
                                       regs.atcglobald[atcindex] = (pd & 0x00000400) >> 10;
                                       regs.atcfc2d[atcindex] = regs.s; // ??
                                       regs.mmusr = (pd & 0xffffe000) | (addr & 0x00001000) | 0x11;
                                       regs.mmusr |= pd & 0x000007e0;
                                       regs.mmusr |= wr ? 0x4 : 0;
                            }
                          } else {
                            regs.mmusr = 0x800;
                            excep_env = excep_env_old;
                            return;
                          }
                        } else {
                          regs.mmusr = 0x800;
                          excep_env = excep_env_old;
                          return;
                        }
                      } else {
                        atcindex = ((addr << 12) >> 24);
                        rootp = regs.srp | ((addr >> 25) << 2);
                        root = get_long_direct(rootp);
                        if ((root & 0x3) > 1) {
                          wr = (root & 0x4) >> 2;
                          put_long_direct(rootp, root | 0x8);
                          apdt = (root & 0xfffffe00) | ((addr & 0x01fc0000) >> 16);
                          pdt = get_long_direct(apdt);
                          if ((pdt & 0x3) > 1) {
                            wr += (pdt & 0x4) >> 2;
                            put_long_direct(apdt, pdt | 0x8);
                            apd = (pdt & 0xffffff00) | ((addr & 0x0003f000) >> 10);
                            pd = get_long_direct(apd);
                            switch (pd & 0x3) {
                              case 0:  regs.mmusr = 0x800;
                                       excep_env = excep_env_old;
                                       return;
                              case 2:  apd = pd & 0xfffffffc;
                                       pd = get_long_direct(apd);
                                       if (((pd & 0x3) % 2) == 0) {
                                         regs.mmusr = 0x800;
                                         excep_env = excep_env_old;
                                         return;
                                       }
                              default: wr += (pd & 0x4) >> 2;
                                       put_long_direct(apd, pd | 0x18);
                                       regs.atcind[atcindex] = addr & 0xfffff000;
                                       regs.atcoutd[atcindex] = pd & 0xfffff000;
                                       regs.atcu0d[atcindex] = (pd & 0x00000100) >> 8;
                                       regs.atcu1d[atcindex] = (pd & 0x00000200) >> 9;
                                       regs.atcsuperd[atcindex] = (pd & 0x00000080) >> 7;
                                       regs.atccmd[atcindex] = (pd & 0x00000060) >> 5;
                                       regs.atcmodifd[atcindex] = 1;
                                       regs.atcwritepd[atcindex] = wr;
                                       regs.atcresidd[atcindex] = 1;
                                       regs.atcglobald[atcindex] = (pd & 0x00000400) >> 10;
                                       regs.atcfc2d[atcindex] = regs.s; // ??
                                       regs.mmusr = (pd & 0xfffff000) | 0x11;
                                       regs.mmusr |= pd & 0x000007e0;
                                       regs.mmusr |= wr ? 0x4 : 0;
                            }
                          } else {
                            regs.mmusr = 0x800;
                            excep_env = excep_env_old;
                            return;
                          }
                        } else {
                          regs.mmusr = 0x800;
                          excep_env = excep_env_old;
                          return;
                        }
                      }
                    } else {
                      switch (excep_nono) {
                        case 2: regs.mmusr = 0x800;
                                excep_env = excep_env_old;
                                return;
                        default:
                                excep_env = excep_env_old;
                                longjmp(excep_env, excep_nono);
                      }
                    }

                    break;
	    case 6:
                    if ((regs.itt0 & 0x8000)
		            && (!(regs.itt0 & 0x20) || (regs.itt0 & 0x40))) {
                        mask = ((~regs.itt0) & 0xff0000) << 8;
                        if ((addr & mask) == (regs.itt0 & mask)) {
                	    regs.mmusr = 3;
	                    return;
               		}
	            }
                    if ((regs.itt1 & 0x8000)
		            && (!(regs.itt1 & 0x20) || (regs.itt1 & 0x40))) {
                        mask = ((~regs.itt1) & 0xff0000) << 8;
                        if ((addr & mask) == (regs.itt1 & mask)) {
                            regs.mmusr = 3;
                            return;
                        }
                    }
                    if ((excep_nono = setjmp(excep_env)) == 0) {
                      if (regs.tcp) {
                        atcindex = ((addr << 11) >> 24);
                        rootp = regs.srp | ((addr >> 25) << 2);
                        root = get_long_direct(rootp);
                        if ((root & 0x3) > 1) {
                          wr = (root & 0x4) >> 2;
                          put_long_direct(rootp, root | 0x8);
                          apdt = (root & 0xfffffe00) | ((addr & 0x01fc0000) >> 16);
                          pdt = get_long_direct(apdt);
                          if ((pdt & 0x3) > 1) {
                            wr += (pdt & 0x4) >> 2;
                            put_long_direct(apdt, pdt | 0x8);
                            apd = (pdt & 0xffffff80) | ((addr & 0x0003e000) >> 11);
                            pd = get_long_direct(apd);
                            switch (pd & 0x3) {
                              case 0:  regs.mmusr = 0x800;
                                       excep_env = excep_env_old;
                                       return;
                              case 2:  apd = pd & 0xfffffffc;
                                       pd = get_long_direct(apd);
                                       if (((pd & 0x3) % 2) == 0) {
                                         regs.mmusr = 0x800;
                                         excep_env = excep_env_old;
                                         return;
                                       }
                              default: wr += (pd & 0x4) >> 2;
                                       put_long_direct(apd, pd | 0x18);
                                       regs.atcini[atcindex] = addr & 0xffffe000;
                                       regs.atcouti[atcindex] = pd & 0xffffe000;
                                       regs.atcu0i[atcindex] = (pd & 0x00000100) >> 8;
                                       regs.atcu1i[atcindex] = (pd & 0x00000200) >> 9;
                                       regs.atcsuperi[atcindex] = (pd & 0x00000080) >> 7;
                                       regs.atccmi[atcindex] = (pd & 0x00000060) >> 5;
                                       regs.atcmodifi[atcindex] = 1;
                                       regs.atcwritepi[atcindex] = wr;
                                       regs.atcresidi[atcindex] = 1;
                                       regs.atcglobali[atcindex] = (pd & 0x00000400) >> 10;
                                       regs.atcfc2i[atcindex] = regs.s; // ??
                                       regs.mmusr = (pd & 0xffffe000) | (addr & 0x00001000) | 0x11;
                                       regs.mmusr |= pd & 0x000007e0;
                                       regs.mmusr |= wr ? 0x4 : 0;
                            }
                          } else {
                            regs.mmusr = 0x800;
                            excep_env = excep_env_old;
                            return;
                          }
                        } else {
                          regs.mmusr = 0x800;
                          excep_env = excep_env_old;
                          return;
                        }
                      } else {
                        atcindex = ((addr << 12) >> 24);
                        rootp = regs.srp | ((addr >> 25) << 2);
                        root = get_long_direct(rootp);
                        if ((root & 0x3) > 1) {
                          wr = (root & 0x4) >> 2;
                          put_long_direct(rootp, root | 0x8);
                          apdt = (root & 0xfffffe00) | ((addr & 0x01fc0000) >> 16);
                          pdt = get_long_direct(apdt);
                          if ((pdt & 0x3) > 1) {
                            wr += (pdt & 0x4) >> 2;
                            put_long_direct(apdt, pdt | 0x8);
                            apd = (pdt & 0xffffff00) | ((addr & 0x0003f000) >> 10);
                            pd = get_long_direct(apd);
                            switch (pd & 0x3) {
                              case 0:  regs.mmusr = 0x800;
                                       excep_env = excep_env_old;
                                       return;
                              case 2:  apd = pd & 0xfffffffc;
                                       pd = get_long_direct(apd);
                                       if (((pd & 0x3) % 2) == 0) {
                                         regs.mmusr = 0x800;
                                         excep_env = excep_env_old;
                                         return;
                                       }
                              default: wr += (pd & 0x4) >> 2;
                                       put_long_direct(apd, pd | 0x18);
                                       regs.atcini[atcindex] = addr & 0xfffff000;
                                       regs.atcouti[atcindex] = pd & 0xfffff000;
                                       regs.atcu0i[atcindex] = (pd & 0x00000100) >> 8;
                                       regs.atcu1i[atcindex] = (pd & 0x00000200) >> 9;
                                       regs.atcsuperi[atcindex] = (pd & 0x00000080) >> 7;
                                       regs.atccmi[atcindex] = (pd & 0x00000060) >> 5;
                                       regs.atcmodifi[atcindex] = 1;
                                       regs.atcwritepi[atcindex] = wr;
                                       regs.atcresidi[atcindex] = 1;
                                       regs.atcglobali[atcindex] = (pd & 0x00000400) >> 10;
                                       regs.atcfc2i[atcindex] = regs.s; // ??
                                       regs.mmusr = (pd & 0xfffff000) | 0x11;
                                       regs.mmusr |= pd & 0x000007e0;
                                       regs.mmusr |= wr ? 0x4 : 0;
                            }
                          } else {
                            regs.mmusr = 0x800;
                            excep_env = excep_env_old;
                            return;
                          }
                        } else {
                          regs.mmusr = 0x800;
                          excep_env = excep_env_old;
                          return;
                        }
                      }
                    } else {
                      switch (excep_nono) {
                        case 2: regs.mmusr = 0x800;
                                excep_env = excep_env_old;
                                return;
                        default:
                                excep_env = excep_env_old;
                                longjmp(excep_env, excep_nono);
                      }
                    }
                    break;
	    default: break;
	}

/*
	if (regs.tcp) {
            atcindex = ((addr << 11) >> 24);
	    if ((excep_nono = setjmp(excep_env)) == 0) {
	        rootp = regs.srp | ((addr >> 25) << 2);
	    } else {
		switch (excep_nono) {
		    case 2: regs.mmusr = 0x800;
			    excep_env = excep_env_old;
			    return;
		    default:
			    excep_env = excep_env_old;
			    longjmp(excep_env, excep_nono);
		}
	    }
	    root = get_long_direct(rootp);
	    if ((root & 0x3) > 1) {
	        wr = (root & 0x4) >> 2;
	        put_long_direct(rootp, root | 0x8);
		if ((excep_nono = setjmp(excep_env)) == 0) {
		    apdt = (root & 0xfffffe00) | ((addr & 0x01fc0000) >> 16);
		} else {
		    switch (excep_nono) {
			case 2: regs.mmusr = 0x800;
				excep_env = excep_env_old;
				return;
			default:
				excep_env = excep_env_old;
				longjmp(excep_env, excep_nono);
		    }
		}
	        pdt = get_long_direct(apdt);
	        if ((pdt & 0x3) > 1) {
	            wr += (pdt & 0x4) >> 2;
		    put_long_direct(apdt, pdt | 0x8);
		    if ((excep_nono = setjmp(excep_env)) == 0) {
		       	apd = (pdt & 0xffffff80) | ((addr & 0x0003e000) >> 11);
		    } else {
			switch (excep_nono) {
			    case 2: regs.mmusr = 0x800;
				    excep_env = excep_env_old;
				    return;
			    default:
				    excep_env = excep_env_old;
				    longjmp(excep_env, excep_nono);
			}
		    }
                    pd = get_long_direct(apd);
		    switch (pd & 0x3) {
		       	case 0:  regs.mmusr = 0x800;
			         return;
		       	case 2:  if ((excep_nono = setjmp(excep_env)) == 0) {
				     apd = pd & 0xfffffffc;
				 } else {
				     switch (excep_nono) {
				         case 2: regs.mmusr = 0x800;
					         excep_env = excep_env_old;
						 return;
					 default:
						 excep_env = excep_env_old;
						 longjmp(excep_env, excep_nono);
				     }
				 }
		        	 pd = get_long_direct(apd);
			         if (((pd & 0x3) % 2) == 0) {
				     regs.mmusr = 0x800;
				     return;
				 }
                    	default: wr += (pd & 0x4) >> 2;
                        	 put_long_direct(apd, pd | 0x18);
			         regs.atcind[atcindex] = addr & 0xffffe000;
			         regs.atcini[atcindex] = addr & 0xffffe000;
				 regs.atcouti[atcindex] = pd & 0xffffe000;
               			 regs.atcoutd[atcindex] = pd & 0xffffe000;
				 regs.atcu0i[atcindex] = (pd & 0x00000100) >> 8;
                         	 regs.atcu0d[atcindex] = (pd & 0x00000100) >> 8;
				 regs.atcu1i[atcindex] = (pd & 0x00000200) >> 9;
                         	 regs.atcu1d[atcindex] = (pd & 0x00000200) >> 9;
				 regs.atcsuperi[atcindex] = (pd & 0x00000080) >> 7;
                         	 regs.atcsuperd[atcindex] = (pd & 0x00000080) >> 7;
				 regs.atccmi[atcindex] = (pd & 0x00000060) >> 5;
                         	 regs.atccmd[atcindex] = (pd & 0x00000060) >> 5;
				 regs.atcmodifi[atcindex] = 1;
                         	 regs.atcmodifd[atcindex] = 1;
				 regs.atcwritepi[atcindex] = wr;
                         	 regs.atcwritepd[atcindex] = wr;
				 regs.atcresidi[atcindex] = 1;
                         	 regs.atcresidd[atcindex] = 1;
				 regs.atcglobali[atcindex] = (pd & 0x00000400) >> 10;
                         	 regs.atcglobald[atcindex] = (pd & 0x00000400) >> 10;
				 regs.atcfc2i[atcindex] = regs.s; // ??
                         	 regs.atcfc2d[atcindex] = regs.s; // ??

				 regs.mmusr = (pd & 0xffffe000) | (addr & 0x00001000) | 0x11;
				 regs.mmusr |= pd & 0x000007e0;
				 regs.mmusr |= wr ? 0x4 : 0;
		    }
	        } else {
		    regs.mmusr = 0x800;
		    return;
		}
	    } else {
		regs.mmusr = 0x800;
		return;
	    }
        } else {
            atcindex = ((addr << 12) >> 24);
	    if ((excep_nono = setjmp(excep_env)) == 0) {
		rootp = regs.srp | ((addr >> 25) << 2);
	    } else {
		regs.mmusr = 0x800;
		excep_env = excep_env_old;
		return;
	    }
	    root = get_long_direct(rootp);
	    if ((root & 0x3) > 1) {
	       	wr = (root & 0x4) >> 2;
	       	put_long_direct(rootp, root | 0x8);
		if ((excep_nono = setjmp(excep_env)) == 0) {
	            apdt = (root & 0xfffffe00) | ((addr & 0x01fc0000) >> 16);
		} else {
		     switch (excep_nono) {
		        case 2: regs.mmusr = 0x800;
			        excep_env = excep_env_old;
				return;
			default:
				excep_env = excep_env_old;
				longjmp(excep_env, excep_nono);
		    }
		}
	        pdt = get_long_direct(apdt);
	        if ((pdt & 0x3) > 1) {
	            wr += (pdt & 0x4) >> 2;
		    put_long_direct(apdt, pdt | 0x8);
		    if ((excep_nono = setjmp(excep_env)) == 0) {
		    	apd = (pdt & 0xffffff00) | ((addr & 0x0003f000) >> 10);
		    } else {
			switch (excep_nono) {
			    case 2: regs.mmusr = 0x800;
				    excep_env = excep_env_old;
				    return;
			    default:
				    excep_env = excep_env_old;
				    longjmp(excep_env, excep_nono);
		        }
		    }
                    pd = get_long_direct(apd);
		    switch (pd & 0x3) {
		        case 0:  regs.mmusr = 0x800;
			         return;
		        case 2:  if ((excep_nono = setjmp(excep_env)) == 0) {
			             apd = pd & 0xfffffffc;
				 } else {
				     switch (excep_nono) {
				         case 2: regs.mmusr = 0x800;
					         excep_env = excep_env_old;
						 return;
					 default:
						 excep_env = excep_env_old;
						 longjmp(excep_env, excep_nono);
				     }
				 }
		                 pd = *apd;
			         if (((pd & 0x3) % 2) == 0) {
				     regs.mmusr = 0x800;
				     return;
				 }
                        default: wr += (pd & 0x4) >> 2;
                                 put_long_direct(apd, pd | 0x18);
			         regs.atcind[atcindex] = addr & 0xfffff000;
			         regs.atcini[atcindex] = addr & 0xfffff000;
				 regs.atcouti[atcindex] = pd & 0xfffff000;
				 regs.atcoutd[atcindex] = pd & 0xfffff000;
				 regs.atcu0i[atcindex] = (pd & 0x00000100) >> 8;
                         	 regs.atcu0d[atcindex] = (pd & 0x00000100) >> 8;
				 regs.atcu1i[atcindex] = (pd & 0x00000200) >> 9;
                         	 regs.atcu1d[atcindex] = (pd & 0x00000200) >> 9;
				 regs.atcsuperi[atcindex] = (pd & 0x00000080) >> 7;
                         	 regs.atcsuperd[atcindex] = (pd & 0x00000080) >> 7;
				 regs.atccmi[atcindex] = (pd & 0x00000060) >> 5;
                         	 regs.atccmd[atcindex] = (pd & 0x00000060) >> 5;
				 regs.atcmodifi[atcindex] = 1;
                         	 regs.atcmodifd[atcindex] = 1;
				 regs.atcwritepi[atcindex] = wr;
                         	 regs.atcwritepd[atcindex] = wr;
				 regs.atcresidi[atcindex] = 1;
                         	 regs.atcresidd[atcindex] = 1;
				 regs.atcglobali[atcindex] = (pd & 0x00000400) >> 10;
                         	 regs.atcglobald[atcindex] = (pd & 0x00000400) >> 10;
				 regs.atcfc2i[atcindex] = regs.s; // ??
                         	 regs.atcfc2d[atcindex] = regs.s; // ??
				 				                      	
				 regs.mmusr = (pd & 0xfffff000) | 0x11;
				 regs.mmusr |= pd & 0x000007e0;
				 regs.mmusr |= wr ? 0x4 : 0;
		    }
	        } else {
		    regs.mmusr = 0x800;
		    return;
		}
	    } else {
		regs.mmusr = 0x800;
		return;
	    }
	}*/
#endif /* FULLMMU */
    } else op_illg(opcode);
#ifdef FULLMMU
    excep_env = excep_env_old;
#endif /* FULLMMU */
}

// MJ static int n_insns = 0, n_spcinsns = 0;

static uaecptr last_trace_ad = 0;

static void do_trace (void)
{
    if (regs.t0) {
       uae_u16 opcode;
       /* should also include TRAP, CHK, SR modification FPcc */
       /* probably never used so why bother */
       /* We can afford this to be inefficient... */
       m68k_setpc (m68k_getpc ());
       fill_prefetch_0 ();
       opcode = get_word (regs.pc);
       if (opcode == 0x4e72            /* RTE */
           || opcode == 0x4e74                 /* RTD */
           || opcode == 0x4e75                 /* RTS */
           || opcode == 0x4e77                 /* RTR */
           || opcode == 0x4e76                 /* TRAPV */
           || (opcode & 0xffc0) == 0x4e80      /* JSR */
           || (opcode & 0xffc0) == 0x4ec0      /* JMP */
           || (opcode & 0xff00) == 0x6100  /* BSR */
           || ((opcode & 0xf000) == 0x6000     /* Bcc */
               && cctrue((opcode >> 8) & 0xf))
           || ((opcode & 0xf0f0) == 0x5050 /* DBcc */
               && !cctrue((opcode >> 8) & 0xf)
               && (uae_s16)m68k_dreg(regs, opcode & 7) != 0))
      {
 	    last_trace_ad = m68k_getpc ();
	    regs.spcflags &= ~SPCFLAG_TRACE;
	    regs.spcflags |= SPCFLAG_DOTRACE;
	}
    } else if (regs.t1) {
       last_trace_ad = m68k_getpc ();
       regs.spcflags &= ~SPCFLAG_TRACE;
       regs.spcflags |= SPCFLAG_DOTRACE;
    }
}

#define SERVE_VBL_MFP(resetStop)							\
{															\
	if (regs.spcflags & (SPCFLAG_VBL|SPCFLAG_MFP)) {		\
		if (regs.spcflags & SPCFLAG_VBL) {					\
			if (4 > regs.intmask) {							\
				Interrupt(4);								\
				regs.stopped = 0;							\
				regs.spcflags &= ~SPCFLAG_VBL;				\
				if (resetStop)								\
					regs.spcflags &= ~SPCFLAG_STOP;			\
			}												\
		}													\
		if (regs.spcflags & SPCFLAG_MFP) {					\
			if (6 > regs.intmask) {							\
				int vector_number = mfp.doInterrupt();		\
				if (vector_number) {						\
					MFPInterrupt(vector_number);			\
					regs.stopped = 0;						\
					if (resetStop)							\
						regs.spcflags &= ~SPCFLAG_STOP;		\
				}											\
				else										\
					regs.spcflags &= ~SPCFLAG_MFP;			\
			}												\
		}													\
	}														\
}

static int do_specialties(void)
{
	/*n_spcinsns++;*/
	if (regs.spcflags & SPCFLAG_DOTRACE) {
		Exception (9,last_trace_ad);
	}
	while (regs.spcflags & SPCFLAG_STOP) {
		usleep(1000);	// give unused time slices back to OS
		SERVE_VBL_MFP(true);
	}
	if (regs.spcflags & SPCFLAG_TRACE)
		do_trace ();

	SERVE_VBL_MFP(false);

/*  
// do not understand the INT vs DOINT stuff so I disabled it (joy)
	if (regs.spcflags & SPCFLAG_INT) {
		regs.spcflags &= ~SPCFLAG_INT;
		regs.spcflags |= SPCFLAG_DOINT;
	}
*/
	if (regs.spcflags & (SPCFLAG_BRK | SPCFLAG_MODE_CHANGE)) {
		regs.spcflags &= ~(SPCFLAG_BRK | SPCFLAG_MODE_CHANGE);
		return 1;
	}

	return 0;
}

#ifndef USE_TIMERS
long maxInnerCounter = 10000;	// default value good for 1GHz Athlon machines
static long innerCounter = 1;
extern void ivoke200HzInterrupt(void);	// in main.cpp
#endif /* !USE_TIMERS */

static void m68k_run_1 (void)
{
    uae_u32 opcode;
    for (;;) {
#ifdef FULL_HISTORY
#ifdef NEED_TO_DEBUG_BADLY
	history[lasthist] = regs;
	historyf[lasthist] =  regflags;
#else
	history[lasthist] = m68k_getpc();
#endif
	if (++lasthist == MAX_HIST) lasthist = 0;
	if (lasthist == firsthist) {
	    if (++firsthist == MAX_HIST) firsthist = 0;
	}
#endif

#if ARAM_PAGE_CHECK
	if (((regs.pcp ^ pc_page) > ARAM_PAGE_MASK)) {
	    check_ram_boundary(regs.pcp, 2, false);
//	    opcode = GET_OPCODE;
	    uae_u16* addr = (uae_u16*)get_real_address(regs.pcp, 0, regs.pcp, sz_word);
	    pc_page = regs.pcp;
	    pc_offset = (uae_u32)addr - regs.pcp;
	}
# ifdef HAVE_GET_WORD_UNSWAPPED
	opcode = do_get_mem_word_unswapped((uae_u16*)(regs.pcp + pc_offset));
# else
	opcode = do_get_mem_word((uae_u16*)(regs.pcp + pc_offset));
# endif
#else
	check_ram_boundary(regs.pcp, 2, false);
	opcode = GET_OPCODE;
#endif

// Seems to be faster without the assembly...
//#ifdef X86_ASSEMBLY
#if 0
        __asm__ __volatile__ ("\tpushl %%ebp\n\tcall *%%ebx\n\tpopl %%ebp" /* FIXME */
                     : : "b" (cpufunctbl[opcode]), "a" (opcode)
                     : "%edx", "%ecx", "%esi", "%edi",  "%ebp", "memory", "cc");
#else
	(*cpufunctbl[opcode])(opcode);
#endif

	if (regs.spcflags) {
	    if (do_specialties())
		return;
	}
#ifndef USE_TIMERS
	{
	    if (--innerCounter == 0) {
		innerCounter = maxInnerCounter;
		invoke200HzInterrupt();
	    }
	}
#endif
    }
}

#define m68k_run1 m68k_run_1

int in_m68k_go = 0;

void m68k_go (int may_quit)
{
// m68k_go() must be reentrant for Execute68k() and Execute68kTrap() to work
/*
    if (in_m68k_go || !may_quit) {
	fprintf(stderr, "Bug! m68k_go is not reentrant."));
	abort();
    }
*/
    in_m68k_go++;
setjmpagain:
    int prb = setjmp(excep_env);
    if (prb != 0) {
        Exception(prb, 0);
    	goto setjmpagain;
    }
    for (;;) {
	if (quit_program > 0) {
	    if (quit_program == 1)
		break;
	    quit_program = 0;
	    m68k_reset ();
	}
#ifdef DEBUGGER
	if (debugging) debug();
#endif
	m68k_run1();
    }
    in_m68k_go--;
}

static void m68k_verify (uaecptr addr, uaecptr *nextpc)
{
    uae_u32 opcode, val;
    struct instr *dp;

    opcode = get_iword_1(0);
    last_op_for_exception_3 = opcode;
    m68kpc_offset = 2;

    if (cpufunctbl[cft_map (opcode)] == op_illg_1) {
	opcode = 0x4AFC;
    }
    dp = table68k + opcode;

    if (dp->suse) {
	if (!verify_ea (dp->sreg, (amodes)dp->smode, (wordsizes)dp->size, &val)) {
	    Exception (3, 0);
	    return;
	}
    }
    if (dp->duse) {
	if (!verify_ea (dp->dreg, (amodes)dp->dmode, (wordsizes)dp->size, &val)) {
	    Exception (3, 0);
	    return;
	}
    }
}

void m68k_disasm (uaecptr addr, uaecptr *nextpc, int cnt)
{
    uaecptr newpc = 0;
    m68kpc_offset = addr - m68k_getpc ();
    while (cnt-- > 0) {
	char instrname[20],*ccpt;
	int opwords;
	uae_u32 opcode;
	struct mnemolookup *lookup;
	struct instr *dp;
	printf ("%08lx: ", m68k_getpc () + m68kpc_offset);
	for (opwords = 0; opwords < 5; opwords++){
	    printf ("%04x ", get_iword_1 (m68kpc_offset + opwords*2));
	}
	opcode = get_iword_1 (m68kpc_offset);
	m68kpc_offset += 2;
	if (cpufunctbl[cft_map (opcode)] == op_illg_1) {
	    opcode = 0x4AFC;
	}
	dp = table68k + opcode;
	for (lookup = lookuptab;(unsigned)lookup->mnemo != (unsigned)dp->mnemo; lookup++)
	    ;

	strcpy (instrname, lookup->name);
	ccpt = strstr (instrname, "cc");
	if (ccpt != 0) {
	    strncpy (ccpt, ccnames[dp->cc], 2);
	}
	printf ("%s", instrname);
	switch (dp->size){
	 case sz_byte: printf (".B "); break;
	 case sz_word: printf (".W "); break;
	 case sz_long: printf (".L "); break;
	 default: printf ("   "); break;
	}

	if (dp->suse) {
	    newpc = m68k_getpc () + m68kpc_offset;
	    newpc += ShowEA (dp->sreg, (amodes)dp->smode, (wordsizes)dp->size, 0);
	}
	if (dp->suse && dp->duse)
	    printf (",");
	if (dp->duse) {
	    newpc = m68k_getpc () + m68kpc_offset;
	    newpc += ShowEA (dp->dreg, (amodes)dp->dmode, (wordsizes)dp->size, 0);
	}
	if (ccpt != 0) {
	    if (cctrue(dp->cc))
		printf (" == %08lx (TRUE)", (unsigned long)newpc);
	    else
		printf (" == %08lx (FALSE)", (unsigned long)newpc);
	} else if ((opcode & 0xff00) == 0x6100) /* BSR */
	    printf (" == %08lx", (unsigned long)newpc);
	printf ("\n");
    }
    if (nextpc)
	*nextpc = m68k_getpc () + m68kpc_offset;
}

#ifdef NEWDEBUG
void newm68k_disasm(FILE *f, uaecptr addr, uaecptr *nextpc, volatile unsigned int cnt)
{
    char *buffer = (char *)malloc(80 * sizeof(char));
    jmp_buf excep_env_old;
    excep_env_old = excep_env;
    strcpy(buffer,"");
    volatile uaecptr newpc = 0;
    m68kpc_offset = addr - m68k_getpc ();
    if (cnt == 0) {
        int prb = setjmp(excep_env);
        if (prb != 0) {
            goto setjmpagainx;
        }
        char instrname[20],*ccpt;
        int opwords;
        uae_u32 opcode;
        struct mnemolookup *lookup;
        struct instr *dp;
        for (opwords = 0; opwords < 5; opwords++) {
            get_iword_1 (m68kpc_offset + opwords*2);
        }
        opcode = get_iword_1 (m68kpc_offset);
        m68kpc_offset += 2;
        if (cpufunctbl[cft_map (opcode)] == op_illg_1) {
            opcode = 0x4AFC;
        }
        dp = table68k + opcode;
        for (lookup = lookuptab;(unsigned)lookup->mnemo != (unsigned)dp->mnemo; lookup++)
            ;
        strcpy (instrname, lookup->name);
        ccpt = strstr (instrname, "cc");
        if (ccpt != 0) {
            strncpy (ccpt, ccnames[dp->cc], 2);
        }
        if (dp->suse) {
            newpc = m68k_getpc () + m68kpc_offset;
            newpc += ShowEA (dp->sreg, (amodes)dp->smode, (wordsizes)dp->size, buffer);
            strcpy(buffer,"");
        }
        if (dp->duse) {
            newpc = m68k_getpc () + m68kpc_offset;
            newpc += ShowEA (dp->dreg, (amodes)dp->dmode, (wordsizes)dp->size, buffer);
            strcpy(buffer,"");
        }
    } else {
setjmpagain:
        int prb = setjmp(excep_env);
        if (prb != 0) {
		fprintf (f, " unknown address\n");
                goto setjmpagain;
        }
	while (cnt-- > 0) {
		char instrname[20],*ccpt;
		int opwords;
		uae_u32 opcode;
		struct mnemolookup *lookup;
		struct instr *dp;
		fprintf (f, "%08lx: ", m68k_getpc () + m68kpc_offset);
		for (opwords = 0; opwords < 5; opwords++) {
		    fprintf (f, "%04x ", get_iword_1 (m68kpc_offset + opwords*2));
		}
		opcode = get_iword_1 (m68kpc_offset);
		m68kpc_offset += 2;
		if (cpufunctbl[cft_map (opcode)] == op_illg_1) {
			opcode = 0x4AFC;
		}
		dp = table68k + opcode;
		for (lookup = lookuptab;(unsigned)lookup->mnemo != (unsigned)dp->mnemo; lookup++)
		    ;
		strcpy (instrname, lookup->name);
		ccpt = strstr (instrname, "cc");
		if (ccpt != 0) {
		    strncpy (ccpt, ccnames[dp->cc], 2);
		}
		fprintf (f, "%s", instrname);
		switch (dp->size){
		 case sz_byte: fprintf (f, ".B "); break;
		 case sz_word: fprintf (f, ".W "); break;
		 case sz_long: fprintf (f, ".L "); break;
		 default: fprintf (f, "   "); break;
		}

		if (dp->suse) {
		    newpc = m68k_getpc () + m68kpc_offset;
		    newpc += ShowEA (dp->sreg, (amodes)dp->smode, (wordsizes)dp->size, buffer);
		    fprintf(f, "%s", buffer);
		    strcpy(buffer,"");
		}
		if (dp->suse && dp->duse)
		    fprintf (f, ",");
		if (dp->duse) {
		    newpc = m68k_getpc () + m68kpc_offset;
		    newpc += ShowEA (dp->dreg, (amodes)dp->dmode, (wordsizes)dp->size, buffer);
		    fprintf(f, "%s", buffer);
		    strcpy(buffer,"");
		}
		if (ccpt != 0) {
		    if (cctrue(dp->cc))
			fprintf (f, " == %08lx (TRUE)", (unsigned long)newpc);
		    else
			fprintf (f, " == %08lx (FALSE)", (unsigned long)newpc);
		} else if ((opcode & 0xff00) == 0x6100) /* BSR */
		    fprintf (f, " == %08lx", (unsigned long)newpc);
		fprintf (f, "\n");
	    }
    }
setjmpagainx:
    if (nextpc)
	*nextpc = m68k_getpc () + m68kpc_offset;
    free(buffer);
    excep_env = excep_env_old;
}

#ifdef FULL_HISTORY
void showDisasm(uaecptr addr) {
	char *buffer = (char *)malloc(80 * sizeof(char));
	strcpy(buffer, "");
	char *sbuffer[7];
	for (int i = 0; i < 7; i++) {
		sbuffer[i] = (char *)malloc(80 * sizeof(char));
		strcpy(sbuffer[i], "");
	}
	char *buff[5];
	for (int i = 0; i < 5; i++) {
		buff[i] = (char *)malloc(80 * sizeof(char));
		strcpy(buff[i],"");
	}
	jmp_buf excep_env_old;
	excep_env_old = excep_env;
	uaecptr newpc = 0;
	m68kpc_offset = addr - m68k_getpc ();
	int prb = setjmp(excep_env);
	if (prb != 0) {
		bug("%s%s%s%s%s%s%s%s%s%s%s%s unknown address", sbuffer[0], buff[0],  buff[1],  buff[2], buff[3], buff[4], sbuffer[1], sbuffer[2], sbuffer[3], sbuffer[4], sbuffer[5], sbuffer[6]);
		free(buffer);
		for (int i = 0; i < 7; i++) free(sbuffer[i]);
		for (int i = 0; i < 5; i++) free(buff[i]);
		return;
	}
	char instrname[20],*ccpt;
	int opwords;
	uae_u32 opcode;
	struct mnemolookup *lookup;
	struct instr *dp;
	sprintf(sbuffer[0], "%08lx: ", m68k_getpc () + m68kpc_offset);
	for (opwords = 0; opwords < 5; opwords++) {
		sprintf (buff[opwords], "%04x ", get_iword_1 (m68kpc_offset + opwords*2));
	}
	opcode = get_iword_1 (m68kpc_offset);
	m68kpc_offset += 2;
	if (cpufunctbl[cft_map (opcode)] == op_illg_1) {
		opcode = 0x4AFC;
	}
	dp = table68k + opcode;
	for (lookup = lookuptab;(unsigned)lookup->mnemo != (unsigned)dp->mnemo; lookup++)
		    ;
	strcpy (instrname, lookup->name);
	ccpt = strstr (instrname, "cc");
	if (ccpt != 0) {
		strncpy (ccpt, ccnames[dp->cc], 2);
	}
	sprintf (sbuffer[1], "%s", instrname);
	switch (dp->size){
		 case sz_byte: sprintf (sbuffer[2], ".B "); break;
		 case sz_word: sprintf (sbuffer[2], ".W "); break;
		 case sz_long: sprintf (sbuffer[2], ".L "); break;
		 default: sprintf (sbuffer[2], "   "); break;
	}

	if (dp->suse) {
		newpc = m68k_getpc () + m68kpc_offset;
		newpc += ShowEA (dp->sreg, (amodes)dp->smode, (wordsizes)dp->size, buffer);
		sprintf(sbuffer[3], "%s", buffer);
		strcpy(buffer,"");
	}
	if (dp->suse && dp->duse) sprintf (sbuffer[4], ",");
	if (dp->duse) {
		newpc = m68k_getpc () + m68kpc_offset;
		newpc += ShowEA (dp->dreg, (amodes)dp->dmode, (wordsizes)dp->size, buffer);
		sprintf(sbuffer[5], "%s", buffer);
		strcpy(buffer,"");
	}
	if (ccpt != 0) {
		if (cctrue(dp->cc)) sprintf (sbuffer[6], " == %08lx (TRUE)", (unsigned long)newpc);
			else sprintf (sbuffer[6], " == %08lx (FALSE)", (unsigned long)newpc);
	} else if ((opcode & 0xff00) == 0x6100) /* BSR */
		sprintf (sbuffer[6], " == %08lx", (unsigned long)newpc);
	
	bug("%s%s%s%s%s%s%s%s%s%s%s%s", sbuffer[0], buff[0], buff[1], buff[2], buff[3],  buff[4], sbuffer[1], sbuffer[2], sbuffer[3], sbuffer[4], sbuffer[5], sbuffer[6]);
	free(buffer);
	for (int i = 0; i < 7; i++) free(sbuffer[i]);
	for (int i = 0; i < 5; i++) free(buff[i]);
	excep_env = excep_env_old;
}
#endif
#endif

void m68k_dumpstate (uaecptr *nextpc)
{
    int i;
    for (i = 0; i < 8; i++){
	printf ("D%d: %08lx ", i, (unsigned long)m68k_dreg(regs, i));
	if ((i & 3) == 3) printf ("\n");
    }
    for (i = 0; i < 8; i++){
	printf ("A%d: %08lx ", i, (unsigned long)m68k_areg(regs, i));
	if ((i & 3) == 3) printf ("\n");
    }
    if (regs.s == 0) regs.usp = m68k_areg(regs, 7);
    if (regs.s && regs.m) regs.msp = m68k_areg(regs, 7);
    if (regs.s && regs.m == 0) regs.isp = m68k_areg(regs, 7);
    printf ("USP=%08lx ISP=%08lx MSP=%08lx VBR=%08lx\n",
	    (unsigned long)regs.usp, (unsigned long)regs.isp,
	    (unsigned long)regs.msp, (unsigned long)regs.vbr);
    printf ("T=%d%d S=%d M=%d X=%d N=%d Z=%d V=%d C=%d IMASK=%d TCE=%d TCP=%d\n",
	    regs.t1, regs.t0, regs.s, regs.m,
	    GET_XFLG, GET_NFLG, GET_ZFLG, GET_VFLG, GET_CFLG, regs.intmask,
	    regs.tce, regs.tcp);
    printf ("CACR=%08lx CAAR=%08lx  URP=%08lx  SRP=%08lx\n",
            (unsigned long)regs.cacr,
	    (unsigned long)regs.caar,
	    (unsigned long)regs.urp,
	    (unsigned long)regs.srp);
    printf ("DTT0=%08lx DTT1=%08lx ITT0=%08lx ITT1=%08lx\n",
            (unsigned long)regs.dtt0,
	    (unsigned long)regs.dtt1,
	    (unsigned long)regs.itt0,
	    (unsigned long)regs.itt1);
    for (i = 0; i < 8; i++){
	printf ("FP%d: %g ", i, regs.fp[i]);
	if ((i & 3) == 3) printf ("\n");
    }
    printf ("N=%d Z=%d I=%d NAN=%d\n",
		(regs.fpsr & 0x8000000) != 0,
		(regs.fpsr & 0x4000000) != 0,
		(regs.fpsr & 0x2000000) != 0,
		(regs.fpsr & 0x1000000) != 0);

    m68k_disasm(m68k_getpc (), nextpc, 1);
    if (nextpc)
	printf ("next PC: %08lx\n", (unsigned long)*nextpc);
}
