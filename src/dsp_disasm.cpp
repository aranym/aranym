/*
 *	DSP56K emulation
 *	disassembler
 *
 *	Patrice Mandin
 */

#include "sysdeps.h"
#include "hardware.h"
#include "cpu_emulation.h"
#include "memory.h"
#include "dsp.h"
#include "dsp_cpu.h"
#include "dsp_disasm.h"

#define DEBUG 0
#include "debug.h"

/* More disasm infos, if wanted */
#define DSP_DISASM_REG_PC 0

/**********************************
 *	Defines
 **********************************/

#define BITMASK(x)	((1<<(x))-1)

/**********************************
 *	Variables
 **********************************/

extern DSP dsp;

/* Current instruction */
static uint32 cur_inst;

/**********************************
 *	Register change
 **********************************/

static uint32 registers_save[64];
#if DSP_DISASM_REG_PC
static uint32 pc_save;
#endif

static char *registers_name[64]={
	"","","","",
	"x0","x1","y0","y1",
	"a0","b0","a2","b2",
	"a1","b1","a","b",
	
	"r0","r1","r2","r3",
	"r4","r5","r6","r7",
	"n0","n1","n2","n3",
	"n4","n5","n6","n7",

	"m0","m1","m2","m3",
	"m4","m5","m6","m7",
	"","","","",
	"","","","",

	"","","","",
	"","","","",
	"","sr","omr","sp",
	"ssh","ssl","la","lc"
};

void dsp56k_disasm_reg_read(void)
{
	memcpy(registers_save, dsp.registers , sizeof(registers_save));
#if DSP_DISASM_REG_PC
	pc_save = dsp.pc;
#endif
}

void dsp56k_disasm_reg_compare(void)
{
	int i;
	
	for (i=0;i<64;i++) {
		if (registers_save[i]==dsp.registers[i]) {
			continue;
		}

		switch(i) {
			case REG_X0:
			case REG_X1:
			case REG_Y0:
			case REG_Y1:
			case REG_A0:
			case REG_A1:
			case REG_B0:
			case REG_B1:
				fprintf(stderr,"Dsp: Reg: %s:0x%06x -> 0x%06x\n", registers_name[i], registers_save[i] & BITMASK(24), dsp.registers[i]  & BITMASK(24));
				break;
			case REG_R0:
			case REG_R1:
			case REG_R2:
			case REG_R3:
			case REG_R4:
			case REG_R5:
			case REG_R6:
			case REG_R7:
			case REG_M0:
			case REG_M1:
			case REG_M2:
			case REG_M3:
			case REG_M4:
			case REG_M5:
			case REG_M6:
			case REG_M7:
			case REG_N0:
			case REG_N1:
			case REG_N2:
			case REG_N3:
			case REG_N4:
			case REG_N5:
			case REG_N6:
			case REG_N7:
			case REG_SR:
			case REG_LA:
			case REG_LC:
				fprintf(stderr,"Dsp: Reg: %s:0x%04x -> 0x%04x\n", registers_name[i], registers_save[i] & BITMASK(16), dsp.registers[i]  & BITMASK(16));
				break;
			case REG_A2:
			case REG_B2:
			case REG_OMR:
			case REG_SP:
			case REG_SSH:
			case REG_SSL:
				fprintf(stderr,"Dsp: Reg: %s:0x%02x -> 0x%02x\n", registers_name[i], registers_save[i] & BITMASK(8), dsp.registers[i]  & BITMASK(8));
				break;
		}
	}
#if DSP_DISASM_REG_PC
	if (pc_save != dsp.pc) {
		fprintf(stderr,"Dsp: Reg: pc:0x%04x -> 0x%04x\n", pc_save, dsp.pc);
	}
#endif
}

/**********************************
 *	Opcode disassembler
 **********************************/

typedef void (*dsp_emul_t)(void);

static void opcode8h_0(void);
static void opcode8h_1(void);
static void opcode8h_4(void);
static void opcode8h_6(void);
static void opcode8h_8(void);
static void opcode8h_a(void);
static void opcode8h_b(void);

static int dsp_calc_ea(uint32 ea_mode, char *dest);
static void dsp_calc_cc(uint32 cc_mode, char *dest);
static void dsp_undefined(void);

/* Instructions without parallel moves */
static void dsp_andi(void);		/* done */
static void dsp_bchg(void);		/* done */
static void dsp_bclr(void);		/* done */
static void dsp_bset(void);		/* done */
static void dsp_btst(void);		/* done */
static void dsp_div(void);		/* done */
static void dsp_do(void);		/* done */
static void dsp_enddo(void);	/* done */
static void dsp_illegal(void);	/* done */
static void dsp_jcc(void);		/* done */
static void dsp_jclr(void);		/* done */
static void dsp_jmp(void);		/* done */
static void dsp_jscc(void);		/* done */
static void dsp_jsclr(void);	/* done */
static void dsp_jset(void);		/* done */
static void dsp_jsr(void);		/* done */
static void dsp_jsset(void);	/* done */
static void dsp_lua(void);		/* done */
static void dsp_movec(void);	/* done */
static void dsp_movem(void);	/* done */
static void dsp_movep(void);	/* done */
static void dsp_nop(void);		/* done */
static void dsp_norm(void);		/* done */
static void dsp_ori(void);		/* done */
static void dsp_rep(void);		/* done */
static void dsp_reset(void);	/* done */
static void dsp_rti(void);		/* done */
static void dsp_rts(void);		/* done */
static void dsp_stop(void);		/* done */
static void dsp_swi(void);		/* done */
static void dsp_tcc(void);		/* done */
static void dsp_wait(void);		/* done */

static void dsp_do_0(void);		/* done */
static void dsp_do_2(void);		/* done */
static void dsp_do_4(void);		/* done */
static void dsp_do_c(void);		/* done */
static void dsp_movec_7(void);	/* done */
static void dsp_movec_9(void);	/* done */
static void dsp_movec_b(void);	/* done */
static void dsp_movec_d(void);	/* done */
static void dsp_movep_0(void);	/* done */
static void dsp_movep_1(void);	/* done */
static void dsp_movep_2(void);	/* done */
static void dsp_rep_1(void);	/* done */
static void dsp_rep_3(void);	/* done */
static void dsp_rep_5(void);	/* done */
static void dsp_rep_d(void);	/* done */

/* Parallel moves */
static void dsp_pm(void);		/* done */
static void dsp_pm_0(void);		/* done */
static void dsp_pm_1(void);		/* done */
static void dsp_pm_2(void);		/* done */
static void dsp_pm_4(void);		/* done */
static void dsp_pm_8(void);		/* done */

/* Instructions with parallel moves */
static void dsp_abs(void);		/* done */
static void dsp_adc(void);		/* done */
static void dsp_add(void);		/* done */
static void dsp_addl(void);		/* done */
static void dsp_addr(void);		/* done */
static void dsp_and(void);		/* done */
static void dsp_asl(void);		/* done */
static void dsp_asr(void);		/* done */
static void dsp_clr(void);		/* done */
static void dsp_cmp(void);		/* done */
static void dsp_cmpm(void);		/* done */
static void dsp_eor(void);		/* done */
static void dsp_lsl(void);		/* done */
static void dsp_lsr(void);		/* done */
static void dsp_mac(void);		/* done */
static void dsp_macr(void);		/* done */
static void dsp_move(void);		/* done */
static void dsp_move_nopm(void);/* done */
static void dsp_mpy(void);		/* done */
static void dsp_mpyr(void);		/* done */
static void dsp_neg(void);		/* done */
static void dsp_not(void);		/* done */
static void dsp_or(void);		/* done */
static void dsp_rnd(void);		/* done */
static void dsp_rol(void);		/* done */
static void dsp_ror(void);		/* done */
static void dsp_sbc(void);		/* done */
static void dsp_sub(void);		/* done */
static void dsp_subl(void);		/* done */
static void dsp_subr(void);		/* done */
static void dsp_tfr(void);		/* done */
static void dsp_tst(void);		/* done */

static dsp_emul_t opcodes8h[16]={
	opcode8h_0,
	opcode8h_1,
	dsp_tcc,
	dsp_tcc,
	opcode8h_4,
	dsp_movec,
	opcode8h_6,
	dsp_movem,
	opcode8h_8,
	opcode8h_8,
	opcode8h_a,
	opcode8h_b,
	dsp_jmp,
	dsp_jsr,
	dsp_jcc,
	dsp_jscc
};

static dsp_emul_t opcodes_0809[16]={
	dsp_move_nopm,
	dsp_move_nopm,
	dsp_move_nopm,
	dsp_move_nopm,

	dsp_movep,
	dsp_movep,
	dsp_movep,
	dsp_movep,

	dsp_move_nopm,
	dsp_move_nopm,
	dsp_move_nopm,
	dsp_move_nopm,

	dsp_movep,
	dsp_movep,
	dsp_movep,
	dsp_movep
};

static dsp_emul_t opcodes_0a[32]={
	dsp_bclr,
	dsp_bset,
	dsp_bclr,
	dsp_bset,
	dsp_jclr,
	dsp_jset,
	dsp_jclr,
	dsp_jset,

	dsp_bclr,
	dsp_bset,
	dsp_bclr,
	dsp_bset,
	dsp_jclr,
	dsp_jset,
	dsp_jclr,
	dsp_jset,

	dsp_bclr,
	dsp_bset,
	dsp_bclr,
	dsp_bset,
	dsp_jclr,
	dsp_jset,
	dsp_jclr,
	dsp_jset,

	dsp_jclr,
	dsp_jset,
	dsp_bclr,
	dsp_bset,
	dsp_jmp,
	dsp_jcc,
	dsp_undefined,
	dsp_undefined
};

static dsp_emul_t opcodes_0b[32]={
	dsp_bchg,
	dsp_btst,
	dsp_bchg,
	dsp_btst,
	dsp_jsclr,
	dsp_jsset,
	dsp_jsclr,
	dsp_jsset,

	dsp_bchg,
	dsp_btst,
	dsp_bchg,
	dsp_btst,
	dsp_jsclr,
	dsp_jsset,
	dsp_jsclr,
	dsp_jsset,

	dsp_bchg,
	dsp_btst,
	dsp_bchg,
	dsp_btst,
	dsp_jsclr,
	dsp_jsset,
	dsp_jsclr,
	dsp_jsset,

	dsp_jsclr,
	dsp_jsclr,
	dsp_bchg,
	dsp_btst,
	dsp_jsr,
	dsp_jscc,
	dsp_undefined,
	dsp_undefined
};

static dsp_emul_t opcodes_alu003f[64]={
	/* 0x00 - 0x0f */
	dsp_move,
	dsp_tfr,
	dsp_addr,
	dsp_tst,
	dsp_undefined,
	dsp_cmp,
	dsp_subr,
	dsp_cmpm,
	dsp_undefined,
	dsp_tfr,
	dsp_addr,
	dsp_tst,
	dsp_undefined,
	dsp_cmp,
	dsp_subr,
	dsp_cmpm,

	/* 0x10 - 0x1f */
	dsp_add,
	dsp_rnd,
	dsp_addl,
	dsp_clr,
	dsp_sub,
	dsp_undefined,
	dsp_subl,
	dsp_not,
	dsp_add,
	dsp_rnd,
	dsp_addl,
	dsp_clr,
	dsp_sub,
	dsp_undefined,
	dsp_subl,
	dsp_not,

	/* 0x20 - 0x2f */
	dsp_add,
	dsp_adc,
	dsp_asr,
	dsp_lsr,
	dsp_sub,
	dsp_sbc,
	dsp_abs,
	dsp_ror,
	dsp_add,
	dsp_adc,
	dsp_asr,
	dsp_lsr,
	dsp_sub,
	dsp_sbc,
	dsp_abs,
	dsp_ror,

	/* 0x30 - 0x3f */
	dsp_add,
	dsp_adc,
	dsp_asl,
	dsp_lsl,
	dsp_sub,
	dsp_sbc,
	dsp_neg,
	dsp_rol,
	dsp_add,
	dsp_adc,
	dsp_asl,
	dsp_lsl,
	dsp_sub,
	dsp_sbc,
	dsp_neg,
	dsp_rol
};

static dsp_emul_t opcodes_alu407f[16]={
	dsp_add,
	dsp_tfr,
	dsp_or,
	dsp_eor,
	dsp_sub,
	dsp_cmp,
	dsp_and,
	dsp_cmpm,
	dsp_add,
	dsp_tfr,
	dsp_or,
	dsp_eor,
	dsp_sub,
	dsp_cmp,
	dsp_and,
	dsp_cmpm
};

static dsp_emul_t opcodes_alu80ff[4]={
	dsp_mpy,
	dsp_mpyr,
	dsp_mac,
	dsp_macr
};

static dsp_emul_t opcodes_do[16]={
	dsp_do_0,
	dsp_undefined,
	dsp_do_2,
	dsp_undefined,

	dsp_do_4,
	dsp_undefined,
	dsp_do_2,
	dsp_undefined,

	dsp_undefined,
	dsp_undefined,
	dsp_do_2,
	dsp_undefined,

	dsp_do_c,
	dsp_undefined,
	dsp_do_2,
	dsp_undefined
};

static dsp_emul_t opcodes_movec[16]={
	dsp_undefined,
	dsp_undefined,
	dsp_undefined,
	dsp_undefined,

	dsp_undefined,
	dsp_undefined,
	dsp_undefined,
	dsp_movec_7,

	dsp_undefined,
	dsp_movec_9,
	dsp_undefined,
	dsp_movec_b,

	dsp_undefined,
	dsp_movec_d,
	dsp_undefined,
	dsp_movec_b
};

static dsp_emul_t opcodes_movep[4]={
	dsp_movep_0,
	dsp_movep_1,
	dsp_movep_2,
	dsp_movep_2
};

static dsp_emul_t opcodes_rep[16]={
	dsp_undefined,
	dsp_rep_1,
	dsp_undefined,
	dsp_rep_3,

	dsp_undefined,
	dsp_rep_5,
	dsp_undefined,
	dsp_rep_3,

	dsp_undefined,
	dsp_undefined,
	dsp_undefined,
	dsp_rep_3,

	dsp_undefined,
	dsp_rep_d,
	dsp_undefined,
	dsp_rep_3
};

static dsp_emul_t opcodes_parmove[16]={
	dsp_pm_0,
	dsp_pm_1,
	dsp_pm_2,
	dsp_pm_2,
	dsp_pm_4,
	dsp_pm_4,
	dsp_pm_4,
	dsp_pm_4,

	dsp_pm_8,
	dsp_pm_8,
	dsp_pm_8,
	dsp_pm_8,
	dsp_pm_8,
	dsp_pm_8,
	dsp_pm_8,
	dsp_pm_8
};

static int registers_tcc[16][2]={
	{REG_B,REG_A},
	{REG_A,REG_B},
	{REG_NULL,REG_NULL},
	{REG_NULL,REG_NULL},

	{REG_NULL,REG_NULL},
	{REG_NULL,REG_NULL},
	{REG_NULL,REG_NULL},
	{REG_NULL,REG_NULL},

	{REG_X0,REG_A},
	{REG_X0,REG_B},
	{REG_X1,REG_A},
	{REG_X1,REG_B},

	{REG_Y0,REG_A},
	{REG_Y0,REG_B},
	{REG_Y1,REG_A},
	{REG_Y1,REG_B}
};

static char *registers_lmove[8]={
	"a10",
	"b10",
	"x",
	"y",
	"a",
	"b",
	"ab",
	"ba"
};

static char *ea_names[9]={
	"(r%d)-n%d",	/* 000xxx */
	"(r%d)+n%d",	/* 001xxx */
	"(r%d)-",		/* 010xxx */
	"(r%d)+",		/* 011xxx */
	"(r%d)",		/* 100xxx */
	"(r%d+n%d)",	/* 101xxx */
	"0x%04x",		/* 110000 */
	"-(r%d)",		/* 111xxx */
	"0x%06x"		/* 110100 */
};

static char *cc_name[16]={
	"cc",
	"ge",
	"ne",
	"pl",
	"nn",
	"ec",
	"lc",
	"gt",
	
	"cs",
	"lt",
	"eq",
	"mi",
	"nr",
	"es",
	"ls",
	"le"
};

static char parallelmove_name[64];

void dsp56k_disasm(void)
{
	uint32 value;

	cur_inst = dsp.ram[SPACE_P][dsp.pc];

	strcpy(parallelmove_name, "");

	value = (cur_inst >> 16) & BITMASK(8);
	if (value< 0x10) {
		opcodes8h[value]();
	} else {
		dsp_pm();
		value = cur_inst & BITMASK(8);
		if (value < 0x40) {
			opcodes_alu003f[value]();
		} else if (value < 0x80) {
			value &= BITMASK(4);
			opcodes_alu407f[value]();
		} else {
			value &= BITMASK(2);
			opcodes_alu80ff[value]();
		}
	}
}

/**********************************
 *	Conditions code calculation
 **********************************/

static void dsp_calc_cc(uint32 cc_mode, char *dest)
{
	strcpy(dest, cc_name[cc_mode & BITMASK(4)]);
}

/**********************************
 *	Effective address calculation
 **********************************/

static int dsp_calc_ea(uint32 ea_mode, char *dest)
{
	int value, retour;

	value = (ea_mode >> 3) & BITMASK(3);
	retour = 0;
	switch (value) {
		case 0:
			/* (Rx)-Nx */
		case 1:
			/* (Rx)+Nx */
		case 5:
			/* (Rx+Nx) */
			sprintf(dest, ea_names[value], ea_mode & BITMASK(3), ea_mode & BITMASK(3));
			break;
		case 2:
			/* (Rx)- */
		case 3:
			/* (Rx)+ */
		case 4:
			/* (Rx) */
		case 7:
			/* -(Rx) */
			sprintf(dest, ea_names[value], ea_mode & BITMASK(3));
			break;
		case 6:
			switch ((ea_mode >> 2) & 1) {
				case 0:
					/* Absolute address */
					sprintf(dest, ea_names[value], dsp.ram[SPACE_P][dsp.pc+1]);
					break;
				case 1:
					/* Immediate value */
					sprintf(dest, ea_names[8], dsp.ram[SPACE_P][dsp.pc+1]);
					retour = 1;
					break;
			}
			break;
	}
	return retour;
}

static void opcode8h_0(void)
{
	uint32 value;

	if (cur_inst <= 0x00008c) {
		switch(cur_inst) {
			case 0x000000:
				dsp_nop();
				break;
			case 0x000004:
				dsp_rti();
				break;
			case 0x000005:
				dsp_illegal();
				break;
			case 0x000006:
				dsp_swi();
				break;
			case 0x00000c:
				dsp_rts();
				break;
			case 0x000084:
				dsp_reset();
				break;
			case 0x000086:
				dsp_wait();
				break;
			case 0x000087:
				dsp_stop();
				break;
			case 0x00008c:
				dsp_enddo();
				break;
		}
	} else {
		value = cur_inst & 0xf8;
		switch (value) {
			case 0x0000b8:
				dsp_andi();
				break;
			case 0x0000f8:
				dsp_ori();
				break;
		}
	}
}

static void opcode8h_1(void)
{
	switch(cur_inst & 0xfff8c7) {
		case 0x018040:
			dsp_div();
			break;
		case 0x01c805:
			dsp_norm();
			break;
	}
}

static void opcode8h_4(void)
{
	switch((cur_inst>>5) & BITMASK(3)) {
		case 0:
			dsp_lua();
			break;
		case 5:
			dsp_movec();
			break;
	}
}

static void opcode8h_6(void)
{
	if (cur_inst & (1<<5)) {
		dsp_rep();
	} else {
		dsp_do();
	}
}

static void opcode8h_8(void)
{
	uint32 value;

	value = (cur_inst >> 12) & BITMASK(4);
	opcodes_0809[value]();
}

static void opcode8h_a(void)
{
	uint32 value;
	
	value = (cur_inst >> 11) & (BITMASK(2)<<3);
	value |= (cur_inst >> 5) & BITMASK(3);

	opcodes_0a[value]();
}

static void opcode8h_b(void)
{
	uint32 value;
	
	value = (cur_inst >> 11) & (BITMASK(2)<<3);
	value |= (cur_inst >> 5) & BITMASK(3);

	opcodes_0b[value]();
}

/**********************************
 *	Non-parallel moves instructions
 **********************************/

static void dsp_undefined(void)
{
	fprintf(stderr,"Dsp: 0x%04x: 0x%06x unknown instruction\n",dsp.pc, cur_inst);
}

static void dsp_andi(void)
{
	char *regname;

	switch(cur_inst & BITMASK(2)) {
		case 0:	regname="mr";	break;
		case 1:	regname="ccr";	break;
		case 2:	regname="omr";	break;
		default: regname="";	break;
	}

	fprintf(stderr,"Dsp: 0x%04x: andi #0x%02x,%s\n",
		dsp.pc,
		(cur_inst>>8) & BITMASK(8),
		regname
	);
}

static void dsp_bchg(void)
{
	char name[16], addr_name[16];
	uint32 memspace, value, numbit;
	
	memspace = (cur_inst>>6) & 1;
	value = (cur_inst>>8) & BITMASK(6);
	numbit = cur_inst & BITMASK(5);

	switch((cur_inst>>14) & BITMASK(2)) {
		case 0:
			/* bchg #n,x:aa */
			/* bchg #n,y:aa */
			if (memspace) {
				sprintf(name,"y:0x%04x",value);
			} else {
				sprintf(name,"x:0x%04x",value);
			}
			break;
		case 1:
			/* bchg #n,x:ea */
			/* bchg #n,y:ea */
			dsp_calc_ea(value, addr_name);
			if (memspace) {
				sprintf(name,"y:%s",addr_name);
			} else {
				sprintf(name,"x:%s",addr_name);
			}
			break;
		case 2:
			/* bchg #n,x:pp */
			/* bchg #n,y:pp */
			if (memspace) {
				sprintf(name,"y:0x%04x",value+0xffc0);
			} else {
				sprintf(name,"x:0x%04x",value+0xffc0);
			}
			break;
		case 3:
			/* bchg #n,R */
			sprintf(name,"%s",registers_name[value]);
			break;
	}

	fprintf(stderr,"Dsp: 0x%04x: bchg #%d,%s\n",dsp.pc, numbit, name);
}

static void dsp_bclr(void)
{
	char name[16], addr_name[16];
	uint32 memspace, value, numbit;
	
	memspace = (cur_inst>>6) & 1;
	value = (cur_inst>>8) & BITMASK(6);
	numbit = cur_inst & BITMASK(5);

	switch((cur_inst>>14) & BITMASK(2)) {
		case 0:
			/* bclr #n,x:aa */
			/* bclr #n,y:aa */
			if (memspace) {
				sprintf(name,"y:0x%04x",value);
			} else {
				sprintf(name,"x:0x%04x",value);
			}
			break;
		case 1:
			/* bclr #n,x:ea */
			/* bclr #n,y:ea */
			dsp_calc_ea(value, addr_name);
			if (memspace) {
				sprintf(name,"y:%s",addr_name);
			} else {
				sprintf(name,"x:%s",addr_name);
			}
			break;
		case 2:
			/* bclr #n,x:pp */
			/* bclr #n,y:pp */
			if (memspace) {
				sprintf(name,"y:0x%04x",value+0xffc0);
			} else {
				sprintf(name,"x:0x%04x",value+0xffc0);
			}
			break;
		case 3:
			/* bclr #n,R */
			sprintf(name,"%s",registers_name[value]);
			break;
	}

	fprintf(stderr,"Dsp: 0x%04x: bclr #%d,%s\n",dsp.pc, numbit, name);
}

static void dsp_bset(void)
{
	char name[16], addr_name[16];
	uint32 memspace, value, numbit;
	
	memspace = (cur_inst>>6) & 1;
	value = (cur_inst>>8) & BITMASK(6);
	numbit = cur_inst & BITMASK(5);

	switch((cur_inst>>14) & BITMASK(2)) {
		case 0:
			/* bset #n,x:aa */
			/* bset #n,y:aa */
			if (memspace) {
				sprintf(name,"y:0x%04x",value);
			} else {
				sprintf(name,"x:0x%04x",value);
			}
			break;
		case 1:
			/* bset #n,x:ea */
			/* bset #n,y:ea */
			dsp_calc_ea(value, addr_name);
			if (memspace) {
				sprintf(name,"y:%s",addr_name);
			} else {
				sprintf(name,"x:%s",addr_name);
			}
			break;
		case 2:
			/* bset #n,x:pp */
			/* bset #n,y:pp */
			if (memspace) {
				sprintf(name,"y:0x%04x",value+0xffc0);
			} else {
				sprintf(name,"x:0x%04x",value+0xffc0);
			}
			break;
		case 3:
			/* bset #n,R */
			sprintf(name,"%s",registers_name[value]);
			break;
	}

	fprintf(stderr,"Dsp: 0x%04x: bset #%d,%s\n",dsp.pc, numbit, name);
}

static void dsp_btst(void)
{
	char name[16], addr_name[16];
	uint32 memspace, value, numbit;
	
	memspace = (cur_inst>>6) & 1;
	value = (cur_inst>>8) & BITMASK(6);
	numbit = cur_inst & BITMASK(5);

	switch((cur_inst>>14) & BITMASK(2)) {
		case 0:
			/* btst #n,x:aa */
			/* btst #n,y:aa */
			if (memspace) {
				sprintf(name,"y:0x%04x",value);
			} else {
				sprintf(name,"x:0x%04x",value);
			}
			break;
		case 1:
			/* btst #n,x:ea */
			/* btst #n,y:ea */
			dsp_calc_ea(value, addr_name);
			if (memspace) {
				sprintf(name,"y:%s",addr_name);
			} else {
				sprintf(name,"x:%s",addr_name);
			}
			break;
		case 2:
			/* btst #n,x:pp */
			/* btst #n,y:pp */
			if (memspace) {
				sprintf(name,"y:0x%04x",value+0xffc0);
			} else {
				sprintf(name,"x:0x%04x",value+0xffc0);
			}
			break;
		case 3:
			/* btst #n,R */
			sprintf(name,"%s",registers_name[value]);
			break;
	}

	fprintf(stderr,"Dsp: 0x%04x: btst #%d,%s\n",dsp.pc, numbit, name);
}

static void dsp_div(void)
{
	uint32 srcreg=REG_NULL, destreg;
	
	switch((cur_inst>>4) & BITMASK(2)) {
		case 0:
			srcreg = REG_X0;
				break;
		case 1:
			srcreg = REG_Y0;
				break;
		case 2:
			srcreg = REG_X1;
				break;
		case 3:
			srcreg = REG_Y1;
				break;
	}
	destreg = REG_A+((cur_inst>>3) & 1);

	fprintf(stderr,"Dsp: 0x%04x: div %s,%s\n",dsp.pc, registers_name[srcreg],registers_name[destreg]);
}

static void dsp_do(void)
{
	uint32 value;

	value = (cur_inst>>12) & (BITMASK(2)<<2);
	value |= (cur_inst>>6) & 1<<1;
	value |= (cur_inst>>5) & 1;

	opcodes_do[value]();
}

static void dsp_do_0(void)
{
	char name[16];

	if (cur_inst & (1<<6)) {
		sprintf(name, "y:0x%04x", (cur_inst>>8) & BITMASK(6));
	} else {
		sprintf(name, "x:0x%04x", (cur_inst>>8) & BITMASK(6));
	}

	fprintf(stderr,"Dsp: 0x%04x: do %s,p:0x%04x\n",
		dsp.pc,
		name,
		dsp.ram[SPACE_P][dsp.pc+1] & BITMASK(16)
	);
}

static void dsp_do_2(void)
{
	fprintf(stderr,"Dsp: 0x%04x: do #0x%04x,p:0x%04x\n",
		dsp.pc,
		((cur_inst>>8) & BITMASK(8))|((cur_inst & BITMASK(4))<<8),
		dsp.ram[SPACE_P][dsp.pc+1] & BITMASK(16)
	);
}

static void dsp_do_4(void)
{
	char addr_name[16], name[16];
	uint32 ea_mode;
	
	ea_mode = (cur_inst>>8) & BITMASK(6);
	dsp_calc_ea(ea_mode, addr_name);

	if (cur_inst & (1<<6)) {
		sprintf(name, "y:%s", addr_name);
	} else {
		sprintf(name, "x:%s", addr_name);
	}

	fprintf(stderr,"Dsp: 0x%04x: do %s,p:0x%04x\n", 
		dsp.pc,
		name,
		dsp.ram[SPACE_P][dsp.pc+1] & BITMASK(16)
	);
}

static void dsp_do_c(void)
{
	fprintf(stderr,"Dsp: 0x%04x: do %s,p:0x%04x\n",
		dsp.pc,
		registers_name[(cur_inst>>8) & BITMASK(6)],
		dsp.ram[SPACE_P][dsp.pc+1] & BITMASK(16)
	);
}

static void dsp_enddo(void)
{
	fprintf(stderr,"Dsp: 0x%04x: enddo\n",dsp.pc);
}

static void dsp_illegal(void)
{
	fprintf(stderr,"Dsp: 0x%04x: illegal\n",dsp.pc);
}

static void dsp_jcc(void)
{
	char cond_name[16], addr_name[16];
	uint32 cc_code=0;
	
	switch((cur_inst >> 16) & BITMASK(8)) {
		case 0x0a:
			dsp_calc_ea((cur_inst >>8) & BITMASK(6), addr_name);
			cc_code=cur_inst & BITMASK(4);
			break;
		case 0x0e:
			sprintf(addr_name, "0x%04x", cur_inst & BITMASK(12));
			cc_code=(cur_inst>>12) & BITMASK(4);
			break;
	}
	dsp_calc_cc(cc_code, cond_name);	

	fprintf(stderr,"Dsp: 0x%04x: j%s p:%s\n",dsp.pc, cond_name, addr_name);
}

static void dsp_jclr(void)
{
	char srcname[16], addr_name[16];
	uint32 memspace, value, numbit;
	
	memspace = (cur_inst>>6) & 1;
	value = (cur_inst>>8) & BITMASK(6);
	numbit = cur_inst & BITMASK(5);

	switch((cur_inst>>14) & BITMASK(2)) {
		case 0:
			/* jclr #n,x:aa,p:xx */
			/* jclr #n,y:aa,p:xx */
			if (memspace) {
				sprintf(srcname, "y:0x%04x", value);
			} else {
				sprintf(srcname, "x:0x%04x", value);
			}
			break;
		case 1:
			/* jclr #n,x:ea,p:xx */
			/* jclr #n,y:ea,p:xx */
			dsp_calc_ea(value, addr_name);
			if (memspace) {
				sprintf(srcname, "y:%s", addr_name);
			} else {
				sprintf(srcname, "x:%s", addr_name);
			}
			break;
		case 2:
			/* jclr #n,x:pp,p:xx */
			/* jclr #n,y:pp,p:xx */
			value += 0xffc0;
			if (memspace) {
				sprintf(srcname, "y:0x%04x", value);
			} else {
				sprintf(srcname, "x:0x%04x", value);
			}
			break;
		case 3:
			/* jclr #n,R,p:xx */
			sprintf(srcname, registers_name[value]);
			break;
	}

	fprintf(stderr,"Dsp: 0x%04x: jclr #%d,%s,p:0x%04x\n",
		dsp.pc,
		numbit,
		srcname,
		dsp.ram[SPACE_P][dsp.pc+1]
	);
}

static void dsp_jmp(void)
{
	char dstname[16];

	switch((cur_inst >> 16) & BITMASK(8)) {
		case 0x0a:
			dsp_calc_ea((cur_inst >>8) & BITMASK(6), dstname);
			break;
		case 0x0c:
			sprintf(dstname, "0x%04x", cur_inst & BITMASK(12));
			break;
	}

	fprintf(stderr,"Dsp: 0x%04x: jmp p:%s\n",dsp.pc, dstname);
}

static void dsp_jscc(void)
{
	char cond_name[16], addr_name[16];
	uint32 cc_code=0;
	
	switch((cur_inst >> 16) & BITMASK(8)) {
		case 0x0b:
			dsp_calc_ea((cur_inst >>8) & BITMASK(6), addr_name);
			cc_code=cur_inst & BITMASK(4);
			break;
		case 0x0f:
			sprintf(addr_name, "0x%04x", cur_inst & BITMASK(12));
			cc_code=(cur_inst>>12) & BITMASK(4);
			break;
	}
	dsp_calc_cc(cc_code, cond_name);	

	fprintf(stderr,"Dsp: 0x%04x: js%s p:%s\n",dsp.pc, cond_name, addr_name);
}
	
static void dsp_jsclr(void)
{
	char srcname[16], addr_name[16];
	uint32 memspace, value, numbit;
	
	memspace = (cur_inst>>6) & 1;
	value = (cur_inst>>8) & BITMASK(6);
	numbit = cur_inst & BITMASK(5);

	switch((cur_inst>>14) & BITMASK(2)) {
		case 0:
			/* jsclr #n,x:aa,p:xx */
			/* jsclr #n,y:aa,p:xx */
			if (memspace) {
				sprintf(srcname, "y:0x%04x", value);
			} else {
				sprintf(srcname, "x:0x%04x", value);
			}
			break;
		case 1:
			/* jsclr #n,x:ea,p:xx */
			/* jsclr #n,y:ea,p:xx */
			dsp_calc_ea(value, addr_name);
			if (memspace) {
				sprintf(srcname, "y:%s", addr_name);
			} else {
				sprintf(srcname, "x:%s", addr_name);
			}
			break;
		case 2:
			/* jsclr #n,x:pp,p:xx */
			/* jsclr #n,y:pp,p:xx */
			value += 0xffc0;
			if (memspace) {
				sprintf(srcname, "y:0x%04x", value);
			} else {
				sprintf(srcname, "x:0x%04x", value);
			}
			break;
		case 3:
			/* jsclr #n,R,p:xx */
			sprintf(srcname, registers_name[value]);
			break;
	}

	fprintf(stderr,"Dsp: 0x%04x: jsclr #%d,%s,p:0x%04x\n",
		dsp.pc,
		numbit,
		srcname,
		dsp.ram[SPACE_P][dsp.pc+1]
	);
}

static void dsp_jset(void)
{
	char srcname[16], addr_name[16];
	uint32 memspace, value, numbit;
	
	memspace = (cur_inst>>6) & 1;
	value = (cur_inst>>8) & BITMASK(6);
	numbit = cur_inst & BITMASK(5);

	switch((cur_inst>>14) & BITMASK(2)) {
		case 0:
			/* jset #n,x:aa,p:xx */
			/* jset #n,y:aa,p:xx */
			if (memspace) {
				sprintf(srcname, "y:0x%04x", value);
			} else {
				sprintf(srcname, "x:0x%04x", value);
			}
			break;
		case 1:
			/* jset #n,x:ea,p:xx */
			/* jset #n,y:ea,p:xx */
			dsp_calc_ea(value, addr_name);
			if (memspace) {
				sprintf(srcname, "y:%s", addr_name);
			} else {
				sprintf(srcname, "x:%s", addr_name);
			}
			break;
		case 2:
			/* jset #n,x:pp,p:xx */
			/* jset #n,y:pp,p:xx */
			value += 0xffc0;
			if (memspace) {
				sprintf(srcname, "y:0x%04x", value);
			} else {
				sprintf(srcname, "x:0x%04x", value);
			}
			break;
		case 3:
			/* jset #n,R,p:xx */
			sprintf(srcname, registers_name[value]);
			break;
	}

	fprintf(stderr,"Dsp: 0x%04x: jset #%d,%s,p:0x%04x\n",
		dsp.pc,
		numbit,
		srcname,
		dsp.ram[SPACE_P][dsp.pc+1]
	);
}

static void dsp_jsr(void)
{
	char dstname[16];

	if (((cur_inst>>12) & BITMASK(4))==0) {
		sprintf(dstname, "0x%04x", cur_inst & BITMASK(12));
	} else {
		dsp_calc_ea((cur_inst>>8) & BITMASK(6),dstname);
	}

	fprintf(stderr,"Dsp: 0x%04x: jsr p:%s\n",dsp.pc, dstname);
}

static void dsp_jsset(void)
{
	char srcname[16], addr_name[16];
	uint32 memspace, value, numbit;
	
	memspace = (cur_inst>>6) & 1;
	value = (cur_inst>>8) & BITMASK(6);
	numbit = cur_inst & BITMASK(5);

	switch((cur_inst>>14) & BITMASK(2)) {
		case 0:
			/* jsset #n,x:aa,p:xx */
			/* jsset #n,y:aa,p:xx */
			if (memspace) {
				sprintf(srcname, "y:0x%04x", value);
			} else {
				sprintf(srcname, "x:0x%04x", value);
			}
			break;
		case 1:
			/* jsset #n,x:ea,p:xx */
			/* jsset #n,y:ea,p:xx */
			dsp_calc_ea(value, addr_name);
			if (memspace) {
				sprintf(srcname, "y:%s", addr_name);
			} else {
				sprintf(srcname, "x:%s", addr_name);
			}
			break;
		case 2:
			/* jsset #n,x:pp,p:xx */
			/* jsset #n,y:pp,p:xx */
			value += 0xffc0;
			if (memspace) {
				sprintf(srcname, "y:0x%04x", value);
			} else {
				sprintf(srcname, "x:0x%04x", value);
			}
			break;
		case 3:
			/* jsset #n,R,p:xx */
			sprintf(srcname, registers_name[value]);
			break;
	}

	fprintf(stderr,"Dsp: 0x%04x: jsset #%d,%s,p:0x%04x\n",
		dsp.pc,
		numbit,
		srcname,
		dsp.ram[SPACE_P][dsp.pc+1]
	);
}

static void dsp_lua(void)
{
	char addr_name[16];

	dsp_calc_ea((cur_inst>>8) & BITMASK(5), addr_name);

	fprintf(stderr,"Dsp: 0x%04x: lua %s,r%d\n",dsp.pc, addr_name, cur_inst & BITMASK(3));
}

static void dsp_movec(void)
{
	uint32 value;
	
	value = (cur_inst>>13) & (1<<3);
	value |= (cur_inst>>12) & (1<<2);
	value |= (cur_inst>>6) & (1<<1);
	value |= (cur_inst>>5) & 1;

	opcodes_movec[value]();
}

static void dsp_movec_7(void)
{
	uint32 numreg1, numreg2;

	/* S1,D2 */
	/* S2,D1 */

	numreg2 = (cur_inst>>8) & BITMASK(6);
	numreg1 = (cur_inst & BITMASK(5))|0x20;

	if (cur_inst & (1<<15)) {
		/* Write D1 */
		fprintf(stderr,"Dsp: 0x%04x: movec %s,%s\n",dsp.pc, registers_name[numreg2], registers_name[numreg1]);
	} else {
		/* Read S1 */
		fprintf(stderr,"Dsp: 0x%04x: movec %s,%s\n",dsp.pc, registers_name[numreg1], registers_name[numreg2]);
	}
}

static void dsp_movec_9(void)
{
	char *spacename,srcname[16],dstname[16];
	uint32 numreg, addr;

	/* x:aa,D1 */
	/* S1,x:aa */
	/* y:aa,D1 */
	/* S1,y:aa */

	numreg = (cur_inst & BITMASK(5))|0x20;
	addr = (cur_inst>>8) & BITMASK(6);

	if (cur_inst & (1<<6)) {
		spacename="y";
	} else {
		spacename="x";
	}

	if (cur_inst & (1<<15)) {
		/* Write D1 */
		sprintf(srcname, "%s:0x%04x", spacename, addr);
		strcpy(dstname, registers_name[numreg]);
	} else {
		/* Read S1 */
		strcpy(srcname, registers_name[numreg]);
		sprintf(dstname, "%s:0x%04x", spacename, addr);
	}

	fprintf(stderr,"Dsp: 0x%04x: movec %s,%s\n",dsp.pc, srcname, dstname);
}

static void dsp_movec_b(void)
{
	uint32 numreg;

	/* #xx,D1 */

	numreg = (cur_inst & BITMASK(5))|0x20;

	fprintf(stderr,"Dsp: 0x%04x: movec #0x%02x,%s\n",dsp.pc, (cur_inst>>8) & BITMASK(8), registers_name[numreg]);
}

static void dsp_movec_d(void)
{
	char *spacename, srcname[16], dstname[16], addr_name[16];
	uint32 numreg, ea_mode;
	int retour;

	/* x:ea,D1 */
	/* S1,x:ea */
	/* y:ea,D1 */
	/* S1,y:ea */
	/* #xxxx,D1 */

	numreg = (cur_inst & BITMASK(5))|0x20;
	ea_mode = (cur_inst>>8) & BITMASK(6);
	retour = dsp_calc_ea(ea_mode, addr_name);

	if (cur_inst & (1<<6)) {
		spacename="y";
	} else {
		spacename="x";
	}

	if (cur_inst & (1<<15)) {
		/* Write D1 */
		if (retour) {
			sprintf(srcname, "#%s", addr_name);
		} else {
			sprintf(srcname, "%s:%s", spacename, addr_name);
		}
		strcpy(dstname, registers_name[numreg]);
	} else {
		/* Read S1 */
		strcpy(srcname, registers_name[numreg]);
		sprintf(dstname, "%s:%s", spacename, addr_name);
	}

	fprintf(stderr,"Dsp: 0x%04x: movec %s,%s\n",dsp.pc, srcname, dstname);
}

static void dsp_movem(void)
{
	char addr_name[16], srcname[16], dstname[16];
	uint32 ea_mode, numreg;

	if (cur_inst & (1<<14)) {
		/* S,p:ea */
		/* p:ea,D */

		ea_mode = (cur_inst>>8) & BITMASK(6);
		dsp_calc_ea(ea_mode, addr_name);
	} else {
		/* S,p:aa */
		/* p:aa,D */

		sprintf(addr_name, "0x%04x",(cur_inst>>8) & BITMASK(6));
	}

	numreg = cur_inst & BITMASK(6);
	if  (cur_inst & (1<<15)) {
		/* Write D */
		sprintf(srcname, "p:%s", addr_name);
		strcpy(dstname, registers_name[numreg]);
	} else {
		/* Read S */
		strcpy(srcname, registers_name[numreg]);
		sprintf(dstname, "p:%s", addr_name);
	}

	fprintf(stderr,"Dsp: 0x%04x: movem %s,%s\n",dsp.pc, srcname, dstname);
}

static void dsp_movep(void)
{
	uint32 value;
	
	value = (cur_inst>>6) & BITMASK(2);

	opcodes_movep[value]();
}

static void dsp_movep_0(void)
{
	char srcname[16]="",dstname[16]="";
	uint32 addr, memspace, numreg;

	/* S,x:pp */
	/* x:pp,D */
	/* S,y:pp */
	/* y:pp,D */

	addr = 0xffc0 + (cur_inst & BITMASK(6));
	memspace = (cur_inst>>16) & 1;
	numreg = (cur_inst>>8) & BITMASK(6);

	if (cur_inst & (1<<15)) {
		/* Write pp */

		strcpy(srcname, registers_name[numreg]);

		if (memspace) {
			sprintf(dstname, "y:0x%04x", addr);
		} else {
			sprintf(dstname, "x:0x%04x", addr);
		}
	} else {
		/* Read pp */

		if (memspace) {
			sprintf(srcname, "y:0x%04x", addr);
		} else {
			sprintf(srcname, "x:0x%04x", addr);
		}

		strcpy(dstname, registers_name[numreg]);
	}

	fprintf(stderr,"Dsp: 0x%04x: movep %s,%s\n",dsp.pc, srcname, dstname);
}

static void dsp_movep_1(void)
{
	char srcname[16]="",dstname[16]="",name[16]="";
	uint32 addr, memspace; 

	/* p:ea,x:pp */
	/* x:pp,p:ea */
	/* p:ea,y:pp */
	/* y:pp,p:ea */

	addr = 0xffc0 + (cur_inst & BITMASK(6));
	dsp_calc_ea((cur_inst>>8) & BITMASK(6), name);
	memspace = (cur_inst>>16) & 1;

	if (cur_inst & (1<<15)) {
		/* Write pp */

		sprintf(srcname, "p:%s", name);

		if (memspace) {
			sprintf(dstname, "y:0x%04x", addr);
		} else {
			sprintf(dstname, "x:0x%04x", addr);
		}
	} else {
		/* Read pp */

		if (memspace) {
			sprintf(srcname, "y:0x%04x", addr);
		} else {
			sprintf(srcname, "x:0x%04x", addr);
		}

		sprintf(dstname, "p:%s", name);
	}

	fprintf(stderr,"Dsp: 0x%04x: movep %s,%s\n",dsp.pc, srcname, dstname);
}

static void dsp_movep_2(void)
{
	char srcname[16]="",dstname[16]="",name[16]="";
	uint32 addr, memspace, easpace, retour; 

	/* x:ea,x:pp */
	/* y:ea,x:pp */
	/* #xxxxxx,x:pp */
	/* x:pp,x:ea */
	/* x:pp,y:ea */

	/* x:ea,y:pp */
	/* y:ea,y:pp */
	/* #xxxxxx,y:pp */
	/* y:pp,y:ea */
	/* y:pp,x:ea */

	addr = 0xffc0 + (cur_inst & BITMASK(6));
	retour = dsp_calc_ea((cur_inst>>8) & BITMASK(6), name);
	memspace = (cur_inst>>16) & 1;
	easpace = (cur_inst>>6) & 1;

	if (cur_inst & (1<<15)) {
		/* Write pp */

		if (retour) {
			sprintf(srcname, "#%s", name);
		} else {
			if (easpace) {
				sprintf(srcname, "y:%s", name);
			} else {
				sprintf(srcname, "x:%s", name);
			}
		}

		if (memspace) {
			sprintf(dstname, "y:0x%04x", addr);
		} else {
			sprintf(dstname, "x:0x%04x", addr);
		}
	} else {
		/* Read pp */

		if (memspace) {
			sprintf(srcname, "y:0x%04x", addr);
		} else {
			sprintf(srcname, "x:0x%04x", addr);
		}

		if (easpace) {
			sprintf(dstname, "y:%s", name);
		} else {
			sprintf(dstname, "x:%s", name);
		}
	}

	fprintf(stderr,"Dsp: 0x%04x: movep %s,%s\n",dsp.pc, srcname, dstname);
}

static void dsp_nop(void)
{
	fprintf(stderr,"Dsp: 0x%04x: nop\n",dsp.pc);
}

static void dsp_norm(void)
{
	uint32 srcreg, destreg;

	srcreg = REG_R0+((cur_inst>>8) & BITMASK(3));
	destreg = REG_A+((cur_inst>>3) & 1);

	fprintf(stderr,"Dsp: 0x%04x: norm %s,%s\n",dsp.pc, registers_name[srcreg], registers_name[destreg]);
}

static void dsp_ori(void)
{
	char *regname;

	switch(cur_inst & BITMASK(2)) {
		case 0:	regname="mr";	break;
		case 1:	regname="ccr";	break;
		case 2:	regname="omr";	break;
		default: regname="";	break;
	}

	fprintf(stderr,"Dsp: 0x%04x: ori #0x%02x,%s\n",
		dsp.pc,
		(cur_inst>>8) & BITMASK(8),
		regname
	);
}

static void dsp_rep(void)
{
	uint32 value;

	value = (cur_inst>>12) & (BITMASK(2)<<2);
	value |= (cur_inst>>6) & (1<<1);
	value |= (cur_inst>>5) & 1;

	opcodes_rep[value]();
}

static void dsp_rep_1(void)
{
	char name[16];

	/* x:aa */
	/* y:aa */

	if (cur_inst & (1<<6)) {
		sprintf(name, "y:0x%04x",(cur_inst>>8) & BITMASK(6));
	} else {
		sprintf(name, "x:0x%04x",(cur_inst>>8) & BITMASK(6));
	}

	fprintf(stderr,"Dsp: 0x%04x: rep %s\n",dsp.pc, name);
}

static void dsp_rep_3(void)
{
	/* #xxx */
	fprintf(stderr,"Dsp: 0x%04x: rep #0x%02x\n",dsp.pc, (cur_inst>>8) & BITMASK(8));
}

static void dsp_rep_5(void)
{
	char name[16],addr_name[16];

	/* x:ea */
	/* y:ea */

	dsp_calc_ea((cur_inst>>8) & BITMASK(6), addr_name);
	if (cur_inst & (1<<6)) {
		sprintf(name, "y:%s",addr_name);
	} else {
		sprintf(name, "x:%s",addr_name);
	}

	fprintf(stderr,"Dsp: 0x%04x: rep %s\n",dsp.pc, name);
}

static void dsp_rep_d(void)
{
	/* R */

	fprintf(stderr,"Dsp: 0x%04x: rep %s\n",dsp.pc, registers_name[(cur_inst>>8) & BITMASK(6)]);
}

static void dsp_reset(void)
{
	fprintf(stderr,"Dsp: 0x%04x: reset\n",dsp.pc);
}

static void dsp_rti(void)
{
	fprintf(stderr,"Dsp: 0x%04x: rti\n",dsp.pc);
}

static void dsp_rts(void)
{
	fprintf(stderr,"Dsp: 0x%04x: rts\n",dsp.pc);
}

static void dsp_stop(void)
{
	fprintf(stderr,"Dsp: 0x%04x: stop\n",dsp.pc);
}
	
static void dsp_swi(void)
{
	fprintf(stderr,"Dsp: 0x%04x: swi\n",dsp.pc);
}

static void dsp_tcc(void)
{
	char ccname[16];
	uint32 src1reg, dst1reg, src2reg, dst2reg;

	dsp_calc_cc((cur_inst>>12) & BITMASK(4), ccname);
	src1reg = registers_tcc[(cur_inst>>3) & BITMASK(4)][0];
	dst1reg = registers_tcc[(cur_inst>>3) & BITMASK(4)][0];

	if (cur_inst & (1<<16)) {
		src2reg = REG_R0+(cur_inst & BITMASK(3));
		dst2reg = REG_R0+((cur_inst>>8) & BITMASK(3));

		fprintf(stderr,"Dsp: 0x%04x: t%s %s,%s %s,%s\n",
			dsp.pc,
			ccname,
			registers_name[src1reg],
			registers_name[dst1reg],
			registers_name[src2reg],
			registers_name[dst2reg]
		);
	} else {
		fprintf(stderr,"Dsp: 0x%04x: t%s %s,%s\n",
			dsp.pc,
			ccname,
			registers_name[src1reg],
			registers_name[dst1reg]
		);
	}
}

static void dsp_wait(void)
{
	fprintf(stderr,"Dsp: 0x%04x: wait\n",dsp.pc);
}

/**********************************
 *	Parallel moves
 **********************************/

static void dsp_pm(void)
{
	uint32 value;

	value = (cur_inst >> 20) & BITMASK(4);

	opcodes_parmove[value]();
}

static void dsp_pm_0(void)
{
	char space_name[16], addr_name[16];
	uint32 memspace, numreg1, numreg2;
/*
	0000 100d 00mm mrrr S,x:ea	x0,D
	0000 100d 10mm mrrr S,y:ea	y0,D
*/
	memspace = (cur_inst>>15) & 1;
	numreg1 = REG_A+((cur_inst>>16) & 1);
	dsp_calc_ea((cur_inst>>8) & BITMASK(6), addr_name);

	if (memspace) {
		strcpy(space_name,"y");
		numreg2 = REG_Y0;
	} else {
		strcpy(space_name,"x");
		numreg2 = REG_X0;
	}

	sprintf(parallelmove_name,
		"%s,%s:%s %s,%s",
		registers_name[numreg1],
		space_name,
		addr_name,
		registers_name[numreg2],
		registers_name[numreg1]
	);
}

static void dsp_pm_1(void)
{
/*
	0001 ffdf w0mm mrrr x:ea,D1		S2,D2
						S1,x:ea		S2,D2
						#xxxxxx,D1	S2,D2
	0001 deff w1mm mrrr S1,D1		y:ea,D2
						S1,D1		S2,y:ea
						S1,D1		#xxxxxx,D2
*/

	char addr_name[16];
	uint32 memspace, write_flag, retour, s1reg, s2reg, d1reg, d2reg;

	memspace = (cur_inst>>14) & 1;
	write_flag = (cur_inst>>15) & 1;
	retour = dsp_calc_ea((cur_inst>>8) & BITMASK(6), addr_name);

	if (memspace==SPACE_Y) {
		s2reg = d2reg = REG_Y0;
		switch((cur_inst>>16) & BITMASK(2)) {
			case 0:	s2reg = d2reg = REG_Y0;	break;
			case 1:	s2reg = d2reg = REG_Y1;	break;
			case 2:	s2reg = d2reg = REG_A;	break;
			case 3:	s2reg = d2reg = REG_B;	break;
		}

		s1reg = REG_A+((cur_inst>>19) & 1);
		d1reg = REG_X0+((cur_inst>>18) & 1);

		if (write_flag) {
			/* Write D2 */

			if (retour) {
				sprintf(parallelmove_name,"%s,%s #%s,%s",
					registers_name[s1reg],
					registers_name[d1reg],
					addr_name,
					registers_name[d2reg]
				);
			} else {
				sprintf(parallelmove_name,"%s,%s y:%s,%s",
					registers_name[s1reg],
					registers_name[d1reg],
					addr_name,
					registers_name[d2reg]
				);
			}
		} else {
			/* Read S2 */
			sprintf(parallelmove_name,"%s,%s %s,y:%s",
				registers_name[s1reg],
				registers_name[d1reg],
				registers_name[s2reg],
				addr_name
			);
		}		

	} else {
		s1reg = d1reg = REG_X0;
		switch((cur_inst>>18) & BITMASK(2)) {
			case 0:	s1reg = d1reg = REG_X0;	break;
			case 1:	s1reg = d1reg = REG_X1;	break;
			case 2:	s1reg = d1reg = REG_A;	break;
			case 3:	s1reg = d1reg = REG_B;	break;
		}

		s2reg = REG_A+((cur_inst>>17) & 1);
		d2reg = REG_Y0+((cur_inst>>16) & 1);

		if (write_flag) {
			/* Write D1 */

			if (retour) {
				sprintf(parallelmove_name,"#%s,%s %s,%s",
					addr_name,
					registers_name[d1reg],
					registers_name[s2reg],
					registers_name[d2reg]
				);
			} else {
				sprintf(parallelmove_name,"x:%s,%s %s,%s",
					addr_name,
					registers_name[d1reg],
					registers_name[s2reg],
					registers_name[d2reg]
				);
			}
		} else {
			/* Read S1 */
			sprintf(parallelmove_name,"%s,x:%s %s,%s",
				registers_name[s1reg],
				addr_name,
				registers_name[s2reg],
				registers_name[d2reg]
			);
		}		
	
	}
}

static void dsp_pm_2(void)
{
	char addr_name[16];
	uint32 numreg1, numreg2;
/*
	0010 0000 0000 0000 nop
	0010 0000 010m mrrr R update
	0010 00ee eeed dddd S,D
	001d dddd iiii iiii #xx,D
*/
	if (((cur_inst >> 8) & 0xffff) == 0x2000) {
		return;
	}

	if (((cur_inst >> 8) & 0xffe0) == 0x2040) {
		dsp_calc_ea(cur_inst & BITMASK(5), addr_name);
		sprintf(parallelmove_name, "%s,r%d",addr_name, (cur_inst>>8) & BITMASK(3));
		return;
	}

	if (((cur_inst >> 8) & 0xfc00) == 0x2000) {
		numreg1 = (cur_inst>>13) & BITMASK(5);
		numreg2 = (cur_inst>>8) & BITMASK(5);
		sprintf(parallelmove_name, "%s,%s", registers_name[numreg1], registers_name[numreg2]); 
		return;
	}

	numreg1 = (cur_inst>>16) & BITMASK(5);
	sprintf(parallelmove_name, "#0x%02x,%s", (cur_inst >> 8) & BITMASK(8), registers_name[numreg1]);
}

static void dsp_pm_4(void)
{
	char addr_name[16];
	uint32 value, retour, ea_mode, memspace;
/*
	0100 l0ll w0aa aaaa l:aa,D
						S,l:aa
	0100 l0ll w1mm mrrr l:ea,D
						S,l:ea
	01dd 0ddd w0aa aaaa x:aa,D
						S,x:aa
	01dd 0ddd w1mm mrrr x:ea,D
						S,x:ea
						#xxxxxx,D
	01dd 1ddd w0aa aaaa y:aa,D
						S,y:aa
	01dd 1ddd w1mm mrrr y:ea,D
						S,y:ea
						#xxxxxx,D
*/
	value = (cur_inst>>16) & BITMASK(3);
	value |= (cur_inst>>17) & (BITMASK(2)<<3);

	ea_mode = (cur_inst>>8) & BITMASK(6);

	if ((value>>2)==0) {
		/* L: memory move */
		if (cur_inst & (1<<14)) {
			retour = dsp_calc_ea(ea_mode, addr_name);	
		} else {
			sprintf(addr_name,"0x%04x", value);
			retour = 0;
		}

		value = (cur_inst>>16) & BITMASK(2);
		value |= (cur_inst>>17) & (1<<2);

		if (cur_inst & (1<<15)) {
			/* Write D */

			if (retour) {
				sprintf(parallelmove_name, "#%s,%s", addr_name, registers_lmove[value]);
			} else {
				sprintf(parallelmove_name, "l:%s,%s", addr_name, registers_lmove[value]);
			}
		} else {
			/* Read S */
			sprintf(parallelmove_name, "%s,l:%s", registers_lmove[value], addr_name);
		}

		return;
	}

	memspace = (cur_inst>>19) & 1;
	if (cur_inst & (1<<14)) {
		retour = dsp_calc_ea(ea_mode, addr_name);	
	} else {
		sprintf(addr_name,"0x%04x", ea_mode);
		retour = 0;
	}

	if (memspace) {
		/* Y: */

		if (cur_inst & (1<<15)) {
			/* Write D */

			if (retour) {
				sprintf(parallelmove_name, "#%s,%s", addr_name, registers_name[value]);
			} else {
				sprintf(parallelmove_name, "y:%s,%s", addr_name, registers_name[value]);
			}

		} else {
			/* Read S */
			sprintf(parallelmove_name, "%s,y:%s", registers_name[value], addr_name);
		}
	} else {
		/* X: */

		if (cur_inst & (1<<15)) {
			/* Write D */

			if (retour) {
				sprintf(parallelmove_name, "#%s,%s", addr_name, registers_name[value]);
			} else {
				sprintf(parallelmove_name, "x:%s,%s", addr_name, registers_name[value]);
			}
		} else {
			/* Read S */
			sprintf(parallelmove_name, "%s,x:%s", registers_name[value], addr_name);
		}
	}
}

static void dsp_pm_8(void)
{
	char addr1_name[16], addr2_name[16];
	uint32 ea_mode1, ea_mode2, numreg1, numreg2;
/*
	1wmm eeff WrrM MRRR x:ea,D1		y:ea,D2	
						x:ea,D1		S2,y:ea
						S1,x:ea		y:ea,D2
						S1,x:ea		S2,y:ea
*/
	numreg1 = REG_X0;
	switch((cur_inst>>18) & BITMASK(2)) {
		case 0:	numreg1 = REG_X0;	break;
		case 1:	numreg1 = REG_X1;	break;
		case 2:	numreg1 = REG_A;	break;
		case 3:	numreg1 = REG_B;	break;
	}

	numreg2 = REG_Y0;
	switch((cur_inst>>16) & BITMASK(2)) {
		case 0:	numreg2 = REG_Y0;	break;
		case 1:	numreg2 = REG_Y1;	break;
		case 2:	numreg2 = REG_A;	break;
		case 3:	numreg2 = REG_B;	break;
	}

	ea_mode1 = (cur_inst>>8) & BITMASK(5);
	if ((ea_mode1>>3) == 0) {
		ea_mode1 |= (1<<5);
	}
	ea_mode2 = (cur_inst>>13) & BITMASK(2);
	ea_mode2 |= ((cur_inst>>20) & BITMASK(2))<<3;
	if ((ea_mode1 & (1<<2))==0) {
		ea_mode2 |= 1<<2;
	}
	if ((ea_mode2>>3) == 0) {
		ea_mode2 |= (1<<5);
	}

	dsp_calc_ea(ea_mode1, addr1_name);
	dsp_calc_ea(ea_mode2, addr2_name);
	
	if (cur_inst & (1<<15)) {
		if (cur_inst & (1<<22)) {
			sprintf(parallelmove_name, "x:%s,%s y:%s,%s",
				addr1_name,
				registers_name[numreg1],
				addr2_name,
				registers_name[numreg2]
			);
		} else {
			sprintf(parallelmove_name, "x:%s,%s %s,y:%s",
				addr1_name,
				registers_name[numreg1],
				registers_name[numreg2],
				addr2_name
			);
		}
	} else {
		if (cur_inst & (1<<22)) {
			sprintf(parallelmove_name, "%s,x:%s y:%s,%s",
				registers_name[numreg1],
				addr1_name,
				addr2_name,
				registers_name[numreg2]
			);
		} else {
			sprintf(parallelmove_name, "x:%s,%s %s,y:%s",
				registers_name[numreg1],
				addr1_name,
				registers_name[numreg2],
				addr2_name
			);
		}
	}	
}


/**********************************
 *	Parallel moves ALU instructions
 **********************************/

static void dsp_abs(void)
{
	fprintf(stderr,"Dsp: 0x%04x: abs %s %s\n",
		dsp.pc,
		registers_name[REG_A+((cur_inst>>3) & 1)],
		parallelmove_name
	);
}

static void dsp_adc(void)
{
	char *srcname;

	if (cur_inst & (1<<4)) {
		srcname="y";
	} else {
		srcname="x";
	}

	fprintf(stderr,"Dsp: 0x%04x: adc %s,%s %s\n",
		dsp.pc,
		srcname,
		registers_name[REG_A+((cur_inst>>3) & 1)],
		parallelmove_name
	);
}

static void dsp_add(void)
{
	char *srcname;
	uint32 srcreg, dstreg;
	
	srcreg = (cur_inst>>4) & BITMASK(3);
	dstreg = (cur_inst>>3) & 1;

	switch(srcreg) {
		case 1:
			srcreg = dstreg ^ 1;
			srcname = registers_name[REG_A+srcreg];
			break;
		case 2:
			srcname="x";
			break;
		case 3:
			srcname="y";
			break;
		case REG_X0:
		case REG_X1:
		case REG_Y0:
		case REG_Y1:
			srcname=registers_name[srcreg];
			break;
		default:
			srcname="";
			break;
	}

	fprintf(stderr,"Dsp: 0x%04x: add %s,%s %s\n",
		dsp.pc,
		srcname,
		registers_name[REG_A+dstreg],
		parallelmove_name
	);
}

static void dsp_addl(void)
{
	uint32 numreg;

	numreg = (cur_inst>>3) & 1;

	fprintf(stderr,"Dsp: 0x%04x: addl %s,%s %s\n",
		dsp.pc,
		registers_name[REG_A+(numreg ^ 1)],
		registers_name[REG_A+numreg],
		parallelmove_name
	);
}

static void dsp_addr(void)
{
	uint32 numreg;

	numreg = (cur_inst>>3) & 1;

	fprintf(stderr,"Dsp: 0x%04x: addr %s,%s %s\n",
		dsp.pc,
		registers_name[REG_A+(numreg ^ 1)],
		registers_name[REG_A+numreg],
		parallelmove_name
	);
}

static void dsp_and(void)
{
	fprintf(stderr,"Dsp: 0x%04x: and %s,%s %s\n",
		dsp.pc,
		registers_name[REG_X0+((cur_inst>>4) & BITMASK(2))],
		registers_name[REG_A+((cur_inst>>3) & 1)],
		parallelmove_name
	);
}

static void dsp_asl(void)
{
	fprintf(stderr,"Dsp: 0x%04x: asl %s %s\n",
		dsp.pc,
		registers_name[REG_A+((cur_inst>>3) & 1)],
		parallelmove_name
	);
}

static void dsp_asr(void)
{
	fprintf(stderr,"Dsp: 0x%04x: asr %s %s\n",
		dsp.pc,
		registers_name[REG_A+((cur_inst>>3) & 1)],
		parallelmove_name
	);
}

static void dsp_clr(void)
{
	fprintf(stderr,"Dsp: 0x%04x: clr %s %s\n",
		dsp.pc,
		registers_name[REG_A+((cur_inst>>3) & 1)],
		parallelmove_name
	);
}

static void dsp_cmp(void)
{
	uint32 srcreg, dstreg;
	
	srcreg = (cur_inst>>4) & BITMASK(3);
	dstreg = (cur_inst>>3) & 1;

	switch(srcreg) {
		case 0:
			srcreg = REG_A+(dstreg ^ 1);
			break;
		case 4:
			srcreg = REG_X0;
			break;
		case 5:
			srcreg = REG_Y0;
			break;
		case 6:
			srcreg = REG_X1;
			break;
		case 7:
			srcreg = REG_Y1;
			break;
	}

	fprintf(stderr,"Dsp: 0x%04x: cmp %s,%s %s\n",
		dsp.pc,
		registers_name[srcreg],
		registers_name[REG_A+dstreg],
		parallelmove_name
	);
}

static void dsp_cmpm(void)
{
	uint32 srcreg, dstreg;
	
	srcreg = (cur_inst>>4) & BITMASK(3);
	dstreg = (cur_inst>>3) & 1;

	switch(srcreg) {
		case 0:
			srcreg = REG_A+(dstreg ^ 1);
			break;
		case 4:
			srcreg = REG_X0;
			break;
		case 5:
			srcreg = REG_Y0;
			break;
		case 6:
			srcreg = REG_X1;
			break;
		case 7:
			srcreg = REG_Y1;
			break;
	}

	fprintf(stderr,"Dsp: 0x%04x: cmpm %s,%s %s\n",
		dsp.pc,
		registers_name[srcreg],
		registers_name[REG_A+dstreg],
		parallelmove_name
	);
}

static void dsp_eor(void)
{
	fprintf(stderr,"Dsp: 0x%04x: eor %s,%s %s\n",
		dsp.pc,
		registers_name[REG_X0+((cur_inst>>4) & BITMASK(2))],
		registers_name[REG_A+((cur_inst>>3) & 1)],
		parallelmove_name
	);
}

static void dsp_lsl(void)
{
	fprintf(stderr,"Dsp: 0x%04x: lsl %s %s\n",
		dsp.pc,
		registers_name[REG_A+((cur_inst>>3) & 1)],
		parallelmove_name
	);
}

static void dsp_lsr(void)
{
	fprintf(stderr,"Dsp: 0x%04x: lsr %s %s\n",
		dsp.pc,
		registers_name[REG_A+((cur_inst>>3) & 1)],
		parallelmove_name
	);
}

static void dsp_mac(void)
{
	char *sign_name;
	uint32 src1reg=REG_NULL, src2reg=REG_NULL, dstreg;

	if (cur_inst & (1<<2)) {
		sign_name="-";
	} else {
		sign_name="";
	}
	
	switch((cur_inst>>4) & BITMASK(3)) {
		case 0:
			src1reg = REG_X0;
			src2reg = REG_X0;
			break;
		case 1:
			src1reg = REG_Y0;
			src2reg = REG_Y0;
			break;
		case 2:
			src1reg = REG_X1;
			src2reg = REG_X0;
			break;
		case 3:
			src1reg = REG_Y1;
			src2reg = REG_Y0;
			break;
		case 4:
			src1reg = REG_X0;
			src2reg = REG_Y1;
			break;
		case 5:
			src1reg = REG_Y0;
			src2reg = REG_X0;
			break;
		case 6:
			src1reg = REG_X1;
			src2reg = REG_Y0;
			break;
		case 7:
			src1reg = REG_Y1;
			src2reg = REG_X1;
			break;
	}
	dstreg = (cur_inst>>3) & 1;

	fprintf(stderr,"Dsp: 0x%04x: mac %s%s,%s,%s %s\n",
		dsp.pc,
		sign_name,
		registers_name[src1reg],
		registers_name[src2reg],
		registers_name[REG_A+dstreg],
		parallelmove_name
	);
}

static void dsp_macr(void)
{
	char *sign_name;
	uint32 src1reg=REG_NULL, src2reg=REG_NULL, dstreg;

	if (cur_inst & (1<<2)) {
		sign_name="-";
	} else {
		sign_name="";
	}
	
	switch((cur_inst>>4) & BITMASK(3)) {
		case 0:
			src1reg = REG_X0;
			src2reg = REG_X0;
			break;
		case 1:
			src1reg = REG_Y0;
			src2reg = REG_Y0;
			break;
		case 2:
			src1reg = REG_X1;
			src2reg = REG_X0;
			break;
		case 3:
			src1reg = REG_Y1;
			src2reg = REG_Y0;
			break;
		case 4:
			src1reg = REG_X0;
			src2reg = REG_Y1;
			break;
		case 5:
			src1reg = REG_Y0;
			src2reg = REG_X0;
			break;
		case 6:
			src1reg = REG_X1;
			src2reg = REG_Y0;
			break;
		case 7:
			src1reg = REG_Y1;
			src2reg = REG_X1;
			break;
	}
	dstreg = (cur_inst>>3) & 1;

	fprintf(stderr,"Dsp: 0x%04x: macr %s%s,%s,%s %s\n",
		dsp.pc,
		sign_name,
		registers_name[src1reg],
		registers_name[src2reg],
		registers_name[REG_A+dstreg],
		parallelmove_name
	);
}

static void dsp_move(void)
{
	fprintf(stderr,"Dsp: 0x%04x: move %s\n",dsp.pc, parallelmove_name);
}

static void dsp_move_nopm(void)
{
	dsp_pm();
	fprintf(stderr,"Dsp: 0x%04x: move %s\n",dsp.pc, parallelmove_name);
}

static void dsp_mpy(void)
{
	char *sign_name;
	uint32 src1reg=REG_NULL, src2reg=REG_NULL, dstreg;

	if (cur_inst & (1<<2)) {
		sign_name="-";
	} else {
		sign_name="";
	}
	
	switch((cur_inst>>4) & BITMASK(3)) {
		case 0:
			src1reg = REG_X0;
			src2reg = REG_X0;
			break;
		case 1:
			src1reg = REG_Y0;
			src2reg = REG_Y0;
			break;
		case 2:
			src1reg = REG_X1;
			src2reg = REG_X0;
			break;
		case 3:
			src1reg = REG_Y1;
			src2reg = REG_Y0;
			break;
		case 4:
			src1reg = REG_X0;
			src2reg = REG_Y1;
			break;
		case 5:
			src1reg = REG_Y0;
			src2reg = REG_X0;
			break;
		case 6:
			src1reg = REG_X1;
			src2reg = REG_Y0;
			break;
		case 7:
			src1reg = REG_Y1;
			src2reg = REG_X1;
			break;
	}
	dstreg = (cur_inst>>3) & 1;

	fprintf(stderr,"Dsp: 0x%04x: mpy %s%s,%s,%s %s\n",
		dsp.pc,
		sign_name,
		registers_name[src1reg],
		registers_name[src2reg],
		registers_name[REG_A+dstreg],
		parallelmove_name
	);
}

static void dsp_mpyr(void)
{
	char *sign_name;
	uint32 src1reg=REG_NULL, src2reg=REG_NULL, dstreg;

	if (cur_inst & (1<<2)) {
		sign_name="-";
	} else {
		sign_name="";
	}
	
	switch((cur_inst>>4) & BITMASK(3)) {
		case 0:
			src1reg = REG_X0;
			src2reg = REG_X0;
			break;
		case 1:
			src1reg = REG_Y0;
			src2reg = REG_Y0;
			break;
		case 2:
			src1reg = REG_X1;
			src2reg = REG_X0;
			break;
		case 3:
			src1reg = REG_Y1;
			src2reg = REG_Y0;
			break;
		case 4:
			src1reg = REG_X0;
			src2reg = REG_Y1;
			break;
		case 5:
			src1reg = REG_Y0;
			src2reg = REG_X0;
			break;
		case 6:
			src1reg = REG_X1;
			src2reg = REG_Y0;
			break;
		case 7:
			src1reg = REG_Y1;
			src2reg = REG_X1;
			break;
	}
	dstreg = (cur_inst>>3) & 1;

	fprintf(stderr,"Dsp: 0x%04x: mpyr %s%s,%s,%s %s\n",
		dsp.pc,
		sign_name,
		registers_name[src1reg],
		registers_name[src2reg],
		registers_name[REG_A+dstreg],
		parallelmove_name
	);
}

static void dsp_neg(void)
{
	fprintf(stderr,"Dsp: 0x%04x: neg %s %s\n",
		dsp.pc,
		registers_name[REG_A+((cur_inst>>3) & 1)],
		parallelmove_name
	);
}

static void dsp_not(void)
{
	fprintf(stderr,"Dsp: 0x%04x: not %s %s\n",
		dsp.pc,
		registers_name[REG_A+((cur_inst>>3) & 1)],
		parallelmove_name
	);
}

static void dsp_or(void)
{
	fprintf(stderr,"Dsp: 0x%04x: or %s,%s %s\n",
		dsp.pc,
		registers_name[REG_X0+((cur_inst>>4) & BITMASK(2))],
		registers_name[REG_A+((cur_inst>>3) & 1)],
		parallelmove_name
	);
}

static void dsp_rnd(void)
{
	fprintf(stderr,"Dsp: 0x%04x: rnd %s %s\n",
		dsp.pc,
		registers_name[REG_A+((cur_inst>>3) & 1)],
		parallelmove_name
	);
}

static void dsp_rol(void)
{
	fprintf(stderr,"Dsp: 0x%04x: rol %s %s\n",
		dsp.pc,
		registers_name[REG_A+((cur_inst>>3) & 1)],
		parallelmove_name
	);
}

static void dsp_ror(void)
{
	fprintf(stderr,"Dsp: 0x%04x: ror %s %s\n",
		dsp.pc,
		registers_name[REG_A+((cur_inst>>3) & 1)],
		parallelmove_name
	);
}

static void dsp_sbc(void)
{
	char *srcname;

	if (cur_inst & (1<<4)) {
		srcname="y";
	} else {
		srcname="x";
	}

	fprintf(stderr,"Dsp: 0x%04x: sbc %s,%s %s\n",
		dsp.pc,
		srcname,
		registers_name[REG_A+((cur_inst>>3) & 1)],
		parallelmove_name
	);
}

static void dsp_sub(void)
{
	char *srcname;
	uint32 srcreg, dstreg;
	
	srcreg = (cur_inst>>4) & BITMASK(3);
	dstreg = (cur_inst>>3) & 1;

	switch(srcreg) {
		case 1:
			srcreg = dstreg ^ 1;
			srcname = registers_name[REG_A+srcreg];
			break;
		case 2:
			srcname="x";
			break;
		case 3:
			srcname="y";
			break;
		case REG_X0:
		case REG_X1:
		case REG_Y0:
		case REG_Y1:
			srcname=registers_name[srcreg];
			break;
		default:
			srcname="";
			break;
	}

	fprintf(stderr,"Dsp: 0x%04x: sub %s,%s %s\n",
		dsp.pc,
		srcname,
		registers_name[REG_A+dstreg],
		parallelmove_name
	);
}

static void dsp_subl(void)
{
	uint32 numreg;

	numreg = (cur_inst>>3) & 1;

	fprintf(stderr,"Dsp: 0x%04x: subl %s,%s %s\n",
		dsp.pc,
		registers_name[REG_A+(numreg ^ 1)],
		registers_name[REG_A+numreg],
		parallelmove_name
	);
}

static void dsp_subr(void)
{
	uint32 numreg;

	numreg = (cur_inst>>3) & 1;

	fprintf(stderr,"Dsp: 0x%04x: subr %s,%s %s\n",
		dsp.pc,
		registers_name[REG_A+(numreg ^ 1)],
		registers_name[REG_A+numreg],
		parallelmove_name
	);
}

static void dsp_tfr(void)
{
	uint32 srcreg, dstreg;
	
	srcreg = (cur_inst>>4) & BITMASK(3);
	dstreg = (cur_inst>>3) & 1;

	if (srcreg==0) {
		srcreg = REG_A+(dstreg ^ 1);
	}

	fprintf(stderr,"Dsp: 0x%04x: tfr %s,%s %s\n",
		dsp.pc,
		registers_name[srcreg],
		registers_name[REG_A+dstreg],
		parallelmove_name
	);
}

static void dsp_tst(void)
{
	fprintf(stderr,"Dsp: 0x%04x: tst %s %s\n",
		dsp.pc,
		registers_name[REG_A+((cur_inst>>3) & 1)],
		parallelmove_name
	);
}

/*
	2002-07-26:PM	BUG:added missing '\n' to disasm output
	2002-07-22:PM	FIX:disasm output changed from D(bug()) to fprintf()
	2002-07-19:PM	BUG:movec_b and movec_d operations permuted
					BUG:pm_5: bad calc of address in [x|y]:aa addressing
*/
