/*
 *	DSP56K emulation
 *
 *	Patrice Mandin
 */

#include "sysdeps.h"
#include "hardware.h"
#include "cpu_emulation.h"
#include "memory.h"
#include "dsp.h"
#include "dsp_cpu.h"

#define DEBUG 1
#include "debug.h"

/**********************************
 *	Defines
 **********************************/

#define BITMASK(x)	((1<<(x))-1)

/**********************************
 *	Variables
 **********************************/

extern DSP dsp;

/* Length of current instruction */
static uint32 cur_inst_len;	/* =0:jump, >0:increment */

/* Current instruction */
static uint32 cur_inst;		

/* Parallel move temp data */
static uint32 tmp_parmove_src[2][3];	/* What to read */
static uint32 *tmp_parmove_dest[2][3];	/* Where to write */
static uint32 tmp_parmove_start[2];		/* From where to read/write */
static uint32 tmp_parmove_len[2];		/* How many to read/write */
static uint32 tmp_parmove_type[2];		/* 0=register, 1=memory */
static uint32 tmp_parmove_space[2];		/* Memory space to write to */

/* PC on Rep instruction ? */
static uint32 pc_on_rep;

/**********************************
 *	Functions
 **********************************/

typedef void (*dsp_emul_t)(void);

static void dsp_execute_instruction(void);
static uint32 read_memory(int space, uint16 address);
static void write_memory(int space, uint16 address, uint32 value);

static void dsp_stack_push(uint32 curpc, uint32 cursr);
static void dsp_stack_pop(uint32 *curpc, uint32 *cursr);

static void opcode8h_0(void);
static void opcode8h_1(void);
/*static void opcode8h_2(void);*/
/*static void opcode8h_3(void);*/
static void opcode8h_4(void);
/*static void opcode8h_5(void);*/
static void opcode8h_6(void);
/*static void opcode8h_7(void);*/
static void opcode8h_8(void);
static void opcode8h_a(void);
static void opcode8h_b(void);
/*static void opcode8h_c(void);*/
/*static void opcode8h_d(void);*/
/*static void opcode8h_e(void);*/
/*static void opcode8h_f(void);*/

static void dsp_update_rn(uint32 numreg, int16 modifier);
static void dsp_update_rn_bitreverse(uint32 numreg);
static void dsp_update_rn_modulo(uint32 numreg, int16 modifier);
static int dsp_calc_ea(uint32 ea_mode, uint32 *dst_addr);
static int dsp_calc_cc(uint32 cc_code);

/* Parallel move analyzer */
static void dsp_parmove_read(void);
static void dsp_parmove_write(void);
static void dsp_pm_0(void);		/* done */
static void dsp_pm_1(void);		/* done */
static void dsp_pm_1x(int immediat, uint16 xy_addr);
static void dsp_pm_2(void);		/* done */
static void dsp_pm_2_2(void);	/* done */
static void dsp_pm_3(void);		/* done */
static void dsp_pm_4(void);		/* done */
static void dsp_pm_4x(int immediat, uint16 l_addr);
static void dsp_pm_5(void);		/* done */
static void dsp_pm_5x(int immediat, uint16 xy_addr);
static void dsp_pm_8(void);		/* done */

static void dsp_undefined(void);

/* Instructions without parallel moves */
static void dsp_andi(void);		/* done */
static void dsp_bchg(void);		/* done */
static void dsp_bclr(void);		/* done */
static void dsp_bset(void);		/* done */
static void dsp_btst(void);		/* done */
static void dsp_div(void);
static void dsp_do(void);
static void dsp_enddo(void);
static void dsp_illegal(void);
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
static void dsp_norm(void);
static void dsp_ori(void);		/* done */
static void dsp_rep(void);		/* done */
static void dsp_reset(void);	/* done */
static void dsp_rti(void);		/* done */
static void dsp_rts(void);		/* done */
static void dsp_stop(void);		/* done */
static void dsp_swi(void);
static void dsp_tcc(void);		/* done */
static void dsp_wait(void);		/* done */

static void dsp_rep_1(void);	/* done */
static void dsp_rep_3(void);	/* done */
static void dsp_rep_5(void);	/* done */
static void dsp_rep_d(void);	/* done */

static void dsp_movec_7(void);	/* done */
static void dsp_movec_9(void);	/* done */
static void dsp_movec_b(void);	/* done */
static void dsp_movec_d(void);	/* done */

static void dsp_movep_0(void);	/* done */
static void dsp_movep_1(void);	/* done */
static void dsp_movep_2(void);	/* done */

/* Instructions with parallel moves */
static void dsp_abs(void);
static void dsp_adc(void);
static void dsp_add(void);
static void dsp_addl(void);
static void dsp_addr(void);
static void dsp_and(void);	/* done */	/* FIXME ccr */
static void dsp_asl(void);	/* done */	/* FIXME ccr */
static void dsp_asr(void);	/* done */	/* FIXME ccr */
static void dsp_clr(void);	/* done */
static void dsp_cmp(void);
static void dsp_cmpm(void);
static void dsp_eor(void);	/* done */	/* FIXME ccr */
static void dsp_lsl(void);	/* done */	/* FIXME ccr */
static void dsp_lsr(void);	/* done */	/* FIXME ccr */
static void dsp_mac(void);
static void dsp_macr(void);
static void dsp_move(void);	/* done */
static void dsp_mpy(void);
static void dsp_mpyr(void);
static void dsp_neg(void);
static void dsp_not(void);	/* done */	/* FIXME ccr */
static void dsp_or(void);	/* done */	/* FIXME ccr */
static void dsp_rnd(void);
static void dsp_rol(void);	/* done */	/* FIXME ccr */
static void dsp_ror(void);	/* done */	/* FIXME ccr */
static void dsp_sbc(void);
static void dsp_sub(void);
static void dsp_subl(void);
static void dsp_subr(void);
static void dsp_tfr(void);
static void dsp_tst(void);

static dsp_emul_t opcodes8h[16]={
	opcode8h_0,
	opcode8h_1,
	/*opcode8h_2,*/	dsp_tcc,
	/*opcode8h_3,*/	dsp_tcc,
	opcode8h_4,
	/*opcode8h_5,*/	dsp_movec,
	opcode8h_6,
	/*opcode8h_7,*/	dsp_movem,
	opcode8h_8,
	opcode8h_8,
	opcode8h_a,
	opcode8h_b,
	/*opcode8h_c,*/ dsp_jmp,
	/*opcode8h_d,*/	dsp_jsr,
	/*opcode8h_e,*/	dsp_jcc,
	/*opcode8h_f,*/	dsp_jscc
};

static dsp_emul_t opcodes_0809[16]={
	dsp_move,
	dsp_move,
	dsp_move,
	dsp_move,

	dsp_movep,
	dsp_movep,
	dsp_movep,
	dsp_movep,

	dsp_move,
	dsp_move,
	dsp_move,
	dsp_move,

	dsp_movep,
	dsp_movep,
	dsp_movep,
	dsp_movep
};

static dsp_emul_t opcodes_0a[32]={
	/* 00 000 */	dsp_bclr,
	/* 00 001 */	dsp_bset,
	/* 00 010 */	dsp_bclr,
	/* 00 011 */	dsp_bset,
	/* 00 100 */	dsp_jclr,
	/* 00 101 */	dsp_jset,
	/* 00 110 */	dsp_jclr,
	/* 00 111 */	dsp_jset,

	/* 01 000 */	dsp_bclr,
	/* 01 001 */	dsp_bset,
	/* 01 010 */	dsp_bclr,
	/* 01 011 */	dsp_bset,
	/* 01 100 */	dsp_jclr,
	/* 01 101 */	dsp_jset,
	/* 01 110 */	dsp_jclr,
	/* 01 111 */	dsp_jset,

	/* 10 000 */	dsp_bclr,
	/* 10 001 */	dsp_bset,
	/* 10 010 */	dsp_bclr,
	/* 10 011 */	dsp_bset,
	/* 10 100 */	dsp_jclr,
	/* 10 101 */	dsp_jset,
	/* 10 110 */	dsp_jclr,
	/* 10 111 */	dsp_jset,

	/* 11 000 */	dsp_jclr,
	/* 11 001 */	dsp_jset,
	/* 11 010 */	dsp_bclr,
	/* 11 011 */	dsp_bset,
	/* 11 100 */	dsp_jmp,
	/* 11 101 */	dsp_jcc,
	/* 11 110 */	dsp_undefined,
	/* 11 111 */	dsp_undefined
};

static dsp_emul_t opcodes_0b[32]={
	/* 00 000 */	dsp_bchg,
	/* 00 001 */	dsp_btst,
	/* 00 010 */	dsp_bchg,
	/* 00 011 */	dsp_btst,
	/* 00 100 */	dsp_jsclr,
	/* 00 101 */	dsp_jsset,
	/* 00 110 */	dsp_jsclr,
	/* 00 111 */	dsp_jsset,

	/* 01 000 */	dsp_bchg,
	/* 01 001 */	dsp_btst,
	/* 01 010 */	dsp_bchg,
	/* 01 011 */	dsp_btst,
	/* 01 100 */	dsp_jsclr,
	/* 01 101 */	dsp_jsset,
	/* 01 110 */	dsp_jsclr,
	/* 01 111 */	dsp_jsset,

	/* 10 000 */	dsp_bchg,
	/* 10 001 */	dsp_btst,
	/* 10 010 */	dsp_bchg,
	/* 10 011 */	dsp_btst,
	/* 10 100 */	dsp_jsclr,
	/* 10 101 */	dsp_jsset,
	/* 10 110 */	dsp_jsclr,
	/* 10 111 */	dsp_jsset,

	/* 11 000 */	dsp_jsclr,
	/* 11 001 */	dsp_jsclr,
	/* 11 010 */	dsp_bchg,
	/* 11 011 */	dsp_btst,
	/* 11 100 */	dsp_jsr,
	/* 11 101 */	dsp_jscc,
	/* 11 110 */	dsp_undefined,
	/* 11 111 */	dsp_undefined
};

static dsp_emul_t opcodes_parmove[16]={
	dsp_pm_0,
	dsp_pm_1,
	dsp_pm_2,
	dsp_pm_3,
	dsp_pm_4,
	dsp_pm_5,
	dsp_pm_5,
	dsp_pm_5,

	dsp_pm_8,
	dsp_pm_8,
	dsp_pm_8,
	dsp_pm_8,
	dsp_pm_8,
	dsp_pm_8,
	dsp_pm_8,
	dsp_pm_8
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

static int registers_lmove[8][2]={
	{REG_A1,REG_A0},	/* A10 */
	{REG_B1,REG_B0},	/* B10 */
	{REG_X1,REG_X0},	/* X */
	{REG_Y1,REG_Y0},	/* Y */
	{REG_A1,REG_A0},	/* A */
	{REG_B1,REG_B0},	/* B */
	{REG_A,REG_B},		/* AB */
	{REG_B,REG_A}		/* BA */
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

/**********************************
 *	Emulator kernel
 **********************************/

void dsp56k_do_execute(void)
{
	int i;

	/* Execute some instructions, how many ? */
	for (i=0;i<2;i++) {
		if (dsp.state != DSP_RUNNING) {
			break;
		}

		dsp_execute_instruction();
	}
}

static void dsp_execute_instruction(void)
{
	uint32 value;

	/* First decode */
	cur_inst = read_memory(SPACE_P,dsp.pc);
	cur_inst_len = 1;

	value = (cur_inst >> 16) & BITMASK(8);
	if (value< 0x10) {
		opcodes8h[value]();
	} else {
		dsp_parmove_read();
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
		dsp_parmove_write();
	}

	/* Are we running a rep ? */
	if (dsp.loop_rep) {
		/* Is PC on the instruction to repeat ? */		
		if (pc_on_rep==0) {
			--dsp.registers[REG_LC];

			if (dsp.registers[REG_LC] > 0) {
				cur_inst_len=0;	/* Stay on this instruction */
			} else {
				dsp.loop_rep = 0;
				dsp.registers[REG_LC] = dsp.registers[REG_LCSAVE];
			}
		} else {
			/* Init LC at right value */
			if (dsp.registers[REG_LC] == 0) {
				dsp.registers[REG_LC] = 0x010000;
			}
			pc_on_rep = 0;
		}
	}

	if (cur_inst_len>0) {
		dsp.pc += cur_inst_len;
	}
}

/**********************************
 *	Read/Write memory functions
 **********************************/

static uint32 read_memory(int space, uint16 address)
{
	address &= BITMASK(16);

	switch(space) {
		case SPACE_X:
			if (address < 0x200) {
				if (dsp.registers[REG_OMR] & (1<<OMR_DE)) {
					return dsp.rom[space][address] & BITMASK(24);
				} else {
					return dsp.ram[space][address] & BITMASK(24);
				}
			} else if (address >= 0xffc0) {
				return dsp.periph[address-0xffc0] & BITMASK(24);
			} else {
				return dsp.ram[space][address] & BITMASK(24);
			}
			break;
		case SPACE_Y:
			if (address < 0x200) {
				if (dsp.registers[REG_OMR] & (1<<OMR_DE)) {
					return dsp.rom[space][address] & BITMASK(24);
				} else {
					return dsp.ram[space][address] & BITMASK(24);
				}
			} else {
				return dsp.ram[space][address] & BITMASK(24);
			}
			break;
		case SPACE_P:
			return dsp.ram[space][address] & BITMASK(24);
			break;
	}

	return 0x00dead;	/* avoid gcc warning */
}

static void write_memory(int space, uint16 address, uint32 value)
{
	address &= BITMASK(16);
	value &= BITMASK(24);

	switch(space) {
		case SPACE_X:
			if ((address >= 0x100) && (address <= 0x200)) {
				if (dsp.registers[REG_OMR] & (1<<OMR_DE)) {
					/* Can not write to rom */
					return;
				}
			}
			if ((address >= 0xffc0) && (address <= 0xffff)) {
				dsp.periph[address-0xffc0] = value;
			} else {
				dsp.ram[space][address] = value;
			}
			/* x:0x0000-0x01ff is internal ram/rom */
			/* x:0x0200-0x3fff = p:0x4200-0x7fff */
			if ((address >= 0x200) && (address <= 0x3fff)) {
				dsp.ram[SPACE_P][address+0x4000] = value;
			}
			break;
		case SPACE_Y:
			if ((address >= 0x100) && (address <= 0x200)) {
				if (dsp.registers[REG_OMR] & (1<<OMR_DE)) {
					/* Can not write to rom */
					return;
				}
			}
			dsp.ram[space][address] = value;
			/* y:0x0000-0x01ff is internal ram/rom */
			/* y:0x0200-0x3fff = p:0x0200-0x3fff */
			if ((address >= 0x200) && (address <= 0x3fff)) {
				dsp.ram[SPACE_P][address] = value;
			}
			break;
		case SPACE_P:
			dsp.ram[space][address] = value;
			/* p:0x0000-0x01ff is internal ram */
			/* p:0x0200-0x3fff = y:0x200-0x3fff */
			/* p:0x4200-0x7fff = x:0x200-0x3fff */
			if ((address >= 0x200) && (address <= 0x3fff)) {
				dsp.ram[SPACE_Y][address] = value;
			} else if ((address >= 0x4200) && (address <= 0x7fff)) {
				dsp.ram[SPACE_X][address-0x4000] = value;
			}
			break;
	}
}

/**********************************
 *	Stack push/pop
 **********************************/

static void dsp_stack_push(uint32 curpc, uint32 cursr)
{
	dsp.registers[REG_SP]++;
	dsp.registers[REG_SSH]++;
	dsp.registers[REG_SSL]++;

	dsp.stack[0][dsp.registers[REG_SSH]]=curpc;
	dsp.stack[1][dsp.registers[REG_SSL]]=cursr;
}

static void dsp_stack_pop(uint32 *newpc, uint32 *newsr)
{
	*newpc = dsp.stack[0][dsp.registers[REG_SSH]];
	*newsr = dsp.stack[1][dsp.registers[REG_SSL]];

	--dsp.registers[REG_SP];
	--dsp.registers[REG_SSH];
	--dsp.registers[REG_SSL];
}

/**********************************
 *	Effective address calculation
 **********************************/

static void dsp_update_rn(uint32 numreg, int16 modifier)
{
	int16 value;
	uint16 m_reg;

	m_reg = (uint16) dsp.registers[REG_M0+numreg];
	if (m_reg == 0) {
		/* Bit reversed carry update */
		dsp_update_rn_bitreverse(numreg);
	} else if ((m_reg>=1) && (m_reg<=32767)) {
		/* Modulo update */
		dsp_update_rn_modulo(numreg, modifier);
	} else if (m_reg == 65535) {
		/* Linear addressing mode */
		value = (int16) dsp.registers[REG_R0+numreg];
		value += modifier;
		dsp.registers[REG_R0+numreg] = ((uint32) value) & BITMASK(16);
	} else {
		/* Undefined */
	}
}

static void dsp_update_rn_bitreverse(uint32 numreg)
{
	int revbits, i;
	uint32 value, r_reg;

	/* Check how many bits to reverse */
	value = dsp.registers[REG_N0+numreg];
	for (revbits=15;revbits>=0;--revbits) {
		if (value & (1<<revbits)) {
			break;
		}
	}	
		
	/* Reverse Rn bits */
	r_reg = dsp.registers[REG_R0+numreg];
	value = r_reg & (BITMASK(16)-BITMASK(revbits));
	for (i=0;i<revbits;i++) {
		if (r_reg & (1<<i)) {
			value |= 1<<(revbits-i);
		}
	}
		
	/* Increment */
	value++;
	value &= BITMASK(revbits);

	/* Reverse Rn bits */
	r_reg &= (BITMASK(16)-BITMASK(revbits));
	r_reg |= value;

	value = r_reg & (BITMASK(16)-BITMASK(revbits));
	for (i=0;i<revbits;i++) {
		if (r_reg & (1<<i)) {
			value |= 1<<(revbits-i);
		}
	}

	dsp.registers[REG_R0+numreg] = value;
}

static void dsp_update_rn_modulo(uint32 numreg, int16 modifier)
{
	uint16 bufsize, modulo, lobound, hibound, value;
	int16 r_reg;

	modulo = (dsp.registers[REG_M0+numreg]+1) & BITMASK(16);
	bufsize = 1;
	while (bufsize < modulo) {
		bufsize <<= 1;
	}
	
	/* lobound is the highest multiple of bufsize<= Rn */
	value = dsp.registers[REG_R0+numreg] & BITMASK(16);
	value /= bufsize;
	lobound = value*bufsize;

	hibound = lobound + modulo-1;

	r_reg = (int16) (dsp.registers[REG_R0+numreg] & BITMASK(16));
	r_reg += modifier;
	if (r_reg>hibound) {
		r_reg -= hibound;
		r_reg += lobound;
	} else if (r_reg<lobound) {
		r_reg -= lobound;
		r_reg += hibound;
	}

	dsp.registers[REG_R0+numreg] = ((uint32) r_reg) & BITMASK(16);
}

static int dsp_calc_ea(uint32 ea_mode, uint32 *dst_addr)
{
	uint32 value, numreg;

	value = (ea_mode >> 3) & BITMASK(3);
	numreg = ea_mode & BITMASK(3);
	switch (value) {
		case 0:
			/* (Rx)-Nx */
			*dst_addr = dsp.registers[REG_R0+numreg];
			dsp_update_rn(numreg, -dsp.registers[REG_N0+numreg]);
			break;
		case 1:
			/* (Rx)+Nx */
			*dst_addr = dsp.registers[REG_R0+numreg];
			dsp_update_rn(numreg, dsp.registers[REG_N0+numreg]);
			break;
		case 2:
			/* (Rx)- */
			*dst_addr = dsp.registers[REG_R0+numreg];
			dsp_update_rn(numreg, -1);
			break;
		case 3:
			/* (Rx)+ */
			*dst_addr = dsp.registers[REG_R0+numreg];
			dsp_update_rn(numreg, +1);
			break;
		case 4:
			/* (Rx) */
			*dst_addr = dsp.registers[REG_R0+numreg];
			break;
		case 5:
			/* (Rx+Nx) */
			dsp_update_rn(numreg, dsp.registers[REG_N0+numreg]);
			*dst_addr = dsp.registers[REG_R0+numreg];
			break;
		case 6:
			/* aa */
			*dst_addr = read_memory(SPACE_P,dsp.pc+1);
			cur_inst_len++;
			if (numreg != 0) {
				return 1; /* immediate value */
			}
			break;
		case 7:
			/* -(Rx) */
			dsp_update_rn(numreg, -1);
			*dst_addr = dsp.registers[REG_R0+numreg];
			break;
	}
	/* address */
	return 0;
}

/**********************************
 *	Condition code test
 **********************************/

#define SR_NV	8	/* N xor V */
#define SR_ZUE	9	/* Z or ((not U) and (not E)) */
#define SR_ZNV	10	/* Z or (N xor V) */

static int cc_code_map[8]={
	SR_C,
	SR_NV,
	SR_Z,
	SR_N,
	SR_ZUE,
	SR_E,
	SR_L,
	SR_ZNV
};

static int dsp_calc_cc(uint32 cc_code)
{
	uint8 ccr_bits[8+3], value;	

	value = dsp.registers[REG_SR] & BITMASK(8);
	ccr_bits[SR_C] = (value >> SR_C) & 1;
	ccr_bits[SR_V] = (value >> SR_V) & 1;
	ccr_bits[SR_Z] = (value >> SR_Z) & 1;
	ccr_bits[SR_N] = (value >> SR_N) & 1;
	ccr_bits[SR_U] = (value >> SR_U) & 1;
	ccr_bits[SR_E] = (value >> SR_E) & 1;
	ccr_bits[SR_L] = (value >> SR_L) & 1;
	ccr_bits[SR_NV] = ccr_bits[SR_N] ^ ccr_bits[SR_V];
	ccr_bits[SR_ZUE] = ccr_bits[SR_Z] | ((~ccr_bits[SR_U]) & (~ccr_bits[SR_E]));
	ccr_bits[SR_ZNV] = ccr_bits[SR_Z] | ccr_bits[SR_NV];

	return (ccr_bits[cc_code_map[cc_code & BITMASK(3)]]==((cc_code>>3) & 1));
}

/**********************************
 *	Highbyte opcodes dispatchers
 **********************************/

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
			default:
				D(bug("Dsp: 0x%04x: 0x%06x",dsp.pc, cur_inst));
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
			default:
				D(bug("Dsp: 0x%04x: 0x%06x",dsp.pc, cur_inst));
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
		default:
			D(bug("Dsp: 0x%04x: 0x%06x",dsp.pc, cur_inst));
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
		default:
			D(bug("Dsp: 0x%04x: 0x%06x",dsp.pc, cur_inst));
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
	cur_inst_len = 0;
	D(bug("Dsp: 0x%04x: 0x%06x unknown instruction",dsp.pc, cur_inst));
}

static void dsp_andi(void)
{
	uint32 regnum, value;

	value = (cur_inst >> 8) & BITMASK(8);
	regnum = cur_inst & BITMASK(2);
	switch(regnum) {
		case 0:
			/* mr */
			dsp.registers[REG_SR] &= (value<<8)|BITMASK(8);
			break;
		case 1:
			/* ccr */
			dsp.registers[REG_SR] &= (BITMASK(8)<<8)|value;
			break;
		case 2:
			/* omr */
			dsp.registers[REG_OMR] &= value;
			break;
	}
}

static void dsp_bchg(void)
{
	uint32 memspace, addr, value, numreg, newcarry, numbit;
	
	memspace = (cur_inst>>6) & 1;
	value = (cur_inst>>8) & BITMASK(6);
	numbit = cur_inst & BITMASK(5);
	newcarry = 0;

	switch((cur_inst>>14) & BITMASK(2)) {
		case 0:
			/* bchg #n,x:aa */
			/* bchg #n,y:aa */
			addr = value;
			value = read_memory(memspace, addr);
			newcarry = (value & (1<<numbit));
			if (newcarry) {
				value -= (1<<numbit);
			} else {
				value += (1<<numbit);
			}
			write_memory(memspace, addr, value);
			break;
		case 1:
			/* bchg #n,x:ea */
			/* bchg #n,y:ea */
			dsp_calc_ea(value, &addr);
			value = read_memory(memspace, addr);
			newcarry = (value & (1<<numbit));
			if (newcarry) {
				value -= (1<<numbit);
			} else {
				value += (1<<numbit);
			}
			write_memory(memspace, addr, value);
			break;
		case 2:
			/* bchg #n,x:pp */
			/* bchg #n,y:pp */
			addr = 0xffc0 + value;
			value = read_memory(memspace, addr);
			newcarry = (value & (1<<numbit));
			if (newcarry) {
				value -= (1<<numbit);
			} else {
				value += (1<<numbit);
			}
			write_memory(memspace, addr, value);
			break;
		case 3:
			/* bchg #n,R */
			numreg = value;
			value = dsp.registers[numreg];
			newcarry = (value & (1<<numbit));
			if (newcarry) {
				value -= (1<<numbit);
			} else {
				value += (1<<numbit);
			}
			dsp.registers[numreg] = value;
			break;
	}

	/* Set carry */
	dsp.registers[REG_SR] &= 0xfffe;
	dsp.registers[REG_SR] |= newcarry;
}

static void dsp_bclr(void)
{
	uint32 memspace, addr, value, numreg, newcarry, numbit;
	
	memspace = (cur_inst>>6) & 1;
	value = (cur_inst>>8) & BITMASK(6);
	numbit = cur_inst & BITMASK(5);
	newcarry = 0;

	switch((cur_inst>>14) & BITMASK(2)) {
		case 0:
			/* bclr #n,x:aa */
			/* bclr #n,y:aa */
			addr = value;
			value = read_memory(memspace, addr);
			newcarry = (value & (1<<numbit));
			value &= 0xffffffff-(1<<numbit);
			write_memory(memspace, addr, value);
			break;
		case 1:
			/* bclr #n,x:ea */
			/* bclr #n,y:ea */
			dsp_calc_ea(value, &addr);
			value = read_memory(memspace, addr);
			newcarry = (value & (1<<numbit));
			value &= 0xffffffff-(1<<numbit);
			write_memory(memspace, addr, value);
			break;
		case 2:
			/* bclr #n,x:pp */
			/* bclr #n,y:pp */
			addr = 0xffc0 + value;
			value = read_memory(memspace, addr);
			newcarry = (value & (1<<numbit));
			value &= 0xffffffff-(1<<numbit);
			write_memory(memspace, addr, value);
			break;
		case 3:
			/* bclr #n,R */
			numreg = value;
			value = dsp.registers[numreg];
			newcarry = (value & (1<<numbit));
			value &= 0xffffffff-(1<<numbit);
			dsp.registers[numreg] = value;
			break;
	}

	/* Set carry */
	dsp.registers[REG_SR] &= 0xfffe;
	dsp.registers[REG_SR] |= (newcarry != 0);
}

static void dsp_bset(void)
{
	uint32 memspace, addr, value, numreg, newcarry, numbit;
	
	memspace = (cur_inst>>6) & 1;
	value = (cur_inst>>8) & BITMASK(6);
	numbit = cur_inst & BITMASK(5);
	newcarry = 0;

	switch((cur_inst>>14) & BITMASK(2)) {
		case 0:
			/* bset #n,x:aa */
			/* bset #n,y:aa */
			addr = value;
			value = read_memory(memspace, addr);
			newcarry = (value & (1<<numbit));
			value |= (1<<numbit);
			write_memory(memspace, addr, value);
			break;
		case 1:
			/* bset #n,x:ea */
			/* bset #n,y:ea */
			dsp_calc_ea(value, &addr);
			value = read_memory(memspace, addr);
			newcarry = (value & (1<<numbit));
			value |= (1<<numbit);
			write_memory(memspace, addr, value);
			break;
		case 2:
			/* bset #n,x:pp */
			/* bset #n,y:pp */
			addr = 0xffc0 + value;
			value = read_memory(memspace, addr);
			newcarry = (value & (1<<numbit));
			value |= (1<<numbit);
			write_memory(memspace, addr, value);
			break;
		case 3:
			/* bset #n,R */
			numreg = value;
			value = dsp.registers[numreg];
			newcarry = (value & (1<<numbit));
			value |= (1<<numbit);
			dsp.registers[numreg] = value;
			break;
	}

	/* Set carry */
	dsp.registers[REG_SR] &= 0xfffe;
	dsp.registers[REG_SR] |= (newcarry != 0);
}

static void dsp_btst(void)
{
	uint32 memspace, addr, value, numreg, newcarry, numbit;
	
	memspace = (cur_inst>>6) & 1;
	value = (cur_inst>>8) & BITMASK(6);
	numbit = cur_inst & BITMASK(5);
	newcarry = 0;

	switch((cur_inst>>14) & BITMASK(2)) {
		case 0:
			/* btst #n,x:aa */
			/* btst #n,y:aa */
			addr = value;
			value = read_memory(memspace, addr);
			newcarry = (value & (1<<numbit));
			break;
		case 1:
			/* btst #n,x:ea */
			/* btst #n,y:ea */
			dsp_calc_ea(value, &addr);
			value = read_memory(memspace, addr);
			newcarry = (value & (1<<numbit));
			break;
		case 2:
			/* btst #n,x:pp */
			/* btst #n,y:pp */
			addr = 0xffc0 + value;
			value = read_memory(memspace, addr);
			newcarry = (value & (1<<numbit));
			break;
		case 3:
			/* btst #n,R */
			numreg = value;
			value = dsp.registers[numreg];
			newcarry = (value & (1<<numbit));
			break;
	}

	/* Set carry */
	dsp.registers[REG_SR] &= 0xfffe;
	dsp.registers[REG_SR] |= (newcarry != 0);
}

static void dsp_div(void)
{
	D(bug("Dsp: 0x%04x: div",dsp.pc));
	cur_inst_len = 0;
}

static void dsp_do(void)
{
	D(bug("Dsp: 0x%04x: do",dsp.pc));
	cur_inst_len = 0;
}

static void dsp_enddo(void)
{
	D(bug("Dsp: 0x%04x: enddo",dsp.pc));
	cur_inst_len = 0;
}

static void dsp_illegal(void)
{
	D(bug("Dsp: 0x%04x: illegal",dsp.pc));
	cur_inst_len = 0;
}

static void dsp_jcc(void)
{
	uint32 newpc, cc_code;

	cc_code = 0;
	switch((cur_inst >> 16) & BITMASK(8)) {
		case 0x0a:
			dsp_calc_ea((cur_inst >>8) & BITMASK(6), &newpc);
			cc_code=cur_inst & BITMASK(4);
			break;
		case 0x0c:
			newpc = cur_inst & BITMASK(12);
			cc_code=(cur_inst>>12) & BITMASK(4);
			break;
	}

	if (dsp_calc_cc(cc_code)) {
		dsp.pc = newpc;
		cur_inst_len = 0;
	}
}

static void dsp_jclr(void)
{
	uint32 memspace, addr, value, numreg, newpc, numbit;
	
	memspace = (cur_inst>>6) & 1;
	value = (cur_inst>>8) & BITMASK(6);
	numbit = cur_inst & BITMASK(5);

	switch((cur_inst>>14) & BITMASK(2)) {
		case 0:
			/* jclr #n,x:aa,p:xx */
			/* jclr #n,y:aa,p:xx */
			addr = value;
			value = read_memory(memspace, addr);
			break;
		case 1:
			/* jclr #n,x:ea,p:xx */
			/* jclr #n,y:ea,p:xx */
			dsp_calc_ea(value, &addr);
			value = read_memory(memspace, addr);
			break;
		case 2:
			/* jclr #n,x:pp,p:xx */
			/* jclr #n,y:pp,p:xx */
			addr = 0xffc0 + value;
			value = read_memory(memspace, addr);
			break;
		case 3:
			/* jclr #n,R,p:xx */
			numreg = value;
			value = dsp.registers[numreg];
			break;
	}

	cur_inst_len++;
	if ((value & (1<<numbit))==0) {
		newpc = read_memory(SPACE_P, dsp.pc+1);
		dsp.pc = newpc;
		cur_inst_len = 0;
	} 
}

static void dsp_jmp(void)
{
	uint32 newpc;

	switch((cur_inst >> 16) & BITMASK(8)) {
		case 0x0a:
			dsp_calc_ea((cur_inst >>8) & BITMASK(6), &newpc);
			break;
		case 0x0c:
			newpc = cur_inst & BITMASK(12);
			break;
	}

	cur_inst_len = 0;

	if (newpc == dsp.pc) {
		/* Infinite loop */
		dsp.state = DSP_WAITINTERRUPT;
		return;
	}

	dsp.pc = newpc;
}

static void dsp_jscc(void)
{
	uint32 newpc, cc_code;

	cc_code = 0;
	switch((cur_inst >> 16) & BITMASK(8)) {
		case 0x0b:
			dsp_calc_ea((cur_inst >>8) & BITMASK(6), &newpc);
			cc_code=cur_inst & BITMASK(4);
			break;
		case 0x0f:
			newpc = cur_inst & BITMASK(12);
			cc_code=(cur_inst>>12) & BITMASK(4);
			break;
	}

	if (dsp_calc_cc(cc_code)) {
		dsp_stack_push(dsp.pc, dsp.registers[REG_SR]);

		dsp.pc = newpc;
		cur_inst_len = 0;
	} 
}

static void dsp_jsclr(void)
{
	uint32 memspace, addr, value, numreg, newpc, numbit;
	
	memspace = (cur_inst>>6) & 1;
	value = (cur_inst>>8) & BITMASK(6);
	numbit = cur_inst & BITMASK(5);

	switch((cur_inst>>14) & BITMASK(2)) {
		case 0:
			/* jsclr #n,x:aa,p:xx */
			/* jsclr #n,y:aa,p:xx */
			addr = value;
			value = read_memory(memspace, addr);
			break;
		case 1:
			/* jsclr #n,x:ea,p:xx */
			/* jsclr #n,y:ea,p:xx */
			dsp_calc_ea(value, &addr);
			value = read_memory(memspace, addr);
			break;
		case 2:
			/* jsclr #n,x:pp,p:xx */
			/* jsclr #n,y:pp,p:xx */
			addr = 0xffc0 + value;
			value = read_memory(memspace, addr);
			break;
		case 3:
			/* jsclr #n,R,p:xx */
			numreg = value;
			value = dsp.registers[numreg];
			break;
	}

	cur_inst_len++;
	if ((value & (1<<numbit))==0) {
		dsp_stack_push(dsp.pc, dsp.registers[REG_SR]);

		newpc = read_memory(SPACE_P, dsp.pc+1);
		dsp.pc = newpc;
		cur_inst_len = 0;
	} 
}

static void dsp_jset(void)
{
	uint32 memspace, addr, value, numreg, numbit;
	uint32 newpc;
	
	memspace = (cur_inst>>6) & 1;
	value = (cur_inst>>8) & BITMASK(6);
	numbit = cur_inst & BITMASK(5);

	switch((cur_inst>>14) & BITMASK(2)) {
		case 0:
			/* jset #n,x:aa,p:xx */
			/* jset #n,y:aa,p:xx */
			addr = value;
			value = read_memory(memspace, addr);
			break;
		case 1:
			/* jset #n,x:ea,p:xx */
			/* jset #n,y:ea,p:xx */
			dsp_calc_ea(value, &addr);
			value = read_memory(memspace, addr);
			break;
		case 2:
			/* jset #n,x:pp,p:xx */
			/* jset #n,y:pp,p:xx */
			addr = 0xffc0 + value;
			value = read_memory(memspace, addr);
			break;
		case 3:
			/* jset #n,R,p:xx */
			numreg = value;
			value = dsp.registers[numreg];
			break;
	}

	cur_inst_len++;
	if (value & (1<<numbit)) {
		newpc = read_memory(SPACE_P, dsp.pc+1);
		dsp.pc = newpc;
		cur_inst_len=0;
	} 
}

static void dsp_jsr(void)
{
	uint32 newpc;

	dsp_stack_push(dsp.pc, dsp.registers[REG_SR]);

	if (((cur_inst>>12) & BITMASK(4))==0) {
		newpc = cur_inst & BITMASK(12);
	} else {
		dsp_calc_ea((cur_inst>>8) & BITMASK(6),&newpc);
	}

	dsp.pc = newpc;
	cur_inst_len = 0;
}

static void dsp_jsset(void)
{
	uint32 memspace, addr, value, numreg, newpc, numbit;
	
	memspace = (cur_inst>>6) & 1;
	value = (cur_inst>>8) & BITMASK(6);
	numbit = cur_inst & BITMASK(5);

	switch((cur_inst>>14) & BITMASK(2)) {
		case 0:
			/* jsset #n,x:aa,p:xx */
			/* jsset #n,y:aa,p:xx */
			addr = value;
			value = read_memory(memspace, addr);
			break;
		case 1:
			/* jsset #n,x:ea,p:xx */
			/* jsset #n,y:ea,p:xx */
			dsp_calc_ea(value, &addr);
			value = read_memory(memspace, addr);
			break;
		case 2:
			/* jsset #n,x:pp,p:xx */
			/* jsset #n,y:pp,p:xx */
			addr = 0xffc0 + value;
			value = read_memory(memspace, addr);
			break;
		case 3:
			/* jsset #n,R,p:xx */
			numreg = value;
			value = dsp.registers[numreg];
			break;
	}

	cur_inst_len++;
	if (value & (1<<numbit)) {
		dsp_stack_push(dsp.pc, dsp.registers[REG_SR]);

		newpc = read_memory(SPACE_P, dsp.pc+1);
		dsp.pc = newpc;
		cur_inst_len = 0;
	} 
}

static void dsp_lua(void)
{
	uint32 value, numreg;
	
	dsp_calc_ea((cur_inst>>8) & BITMASK(5), &value);

	numreg = cur_inst & BITMASK(3);
	if (cur_inst & (1<<3)) {
		dsp.registers[REG_N0+numreg]=value;
	} else {
		dsp.registers[REG_R0+numreg]=value;
	}
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
	/* S1,D2 */
	/* S2,D1 */

	uint32 numreg1, numreg2, value;

	numreg2 = (cur_inst>>8) & BITMASK(6);
	numreg1 = (cur_inst & BITMASK(5))|0x20;

	if (cur_inst & (1<<15)) {
		/* Write D1 */

		if ((numreg2 == REG_A) || (numreg2 == REG_B)) {
			dsp.registers[numreg1] = dsp.registers[REG_A1+(numreg2 & 1)];
		} else {
			dsp.registers[numreg1] = dsp.registers[numreg2];
		}
	} else {
		/* Read S1 */

		if ((numreg2 == REG_A) || (numreg2 == REG_B)) {
			value = dsp.registers[numreg1];
			dsp.registers[REG_A2+(numreg2 & 1)] = 0;
			if (value & (1<<23)) {
				dsp.registers[REG_A2+(numreg2 & 1)] = 0xff;
			}
			dsp.registers[REG_A1+(numreg2 & 1)] = value;
			dsp.registers[REG_A0+(numreg2 & 1)] = 0;
		} else {
			dsp.registers[numreg2] = dsp.registers[numreg1];
		}
	}
}

static void dsp_movec_9(void)
{
	/* x:aa,D1 */
	/* S1,x:aa */
	/* y:aa,D1 */
	/* S1,y:aa */

	uint32 numreg, addr, memspace;

	numreg = (cur_inst & BITMASK(5))|0x20;
	addr = (cur_inst>>8) & BITMASK(6);
	memspace = (cur_inst>>6) & 1;

	if (cur_inst & (1<<15)) {
		/* Write D1 */

		dsp.registers[numreg] = read_memory(memspace, addr);
	} else {
		/* Read S1 */

		write_memory(memspace, addr, dsp.registers[numreg]);
	}
}

static void dsp_movec_b(void)
{
	/* x:ea,D1 */
	/* S1,x:ea */
	/* y:ea,D1 */
	/* S1,y:ea */
	/* #xxxx,D1 */

	uint32 numreg, addr, memspace, ea_mode;
	int retour;

	numreg = (cur_inst & BITMASK(5))|0x20;
	ea_mode = (cur_inst>>8) & BITMASK(6);
	memspace = (cur_inst>>6) & 1;

	if (cur_inst & (1<<15)) {
		/* Write D1 */

		retour = dsp_calc_ea(ea_mode, &addr);
		if (retour) {
			dsp.registers[numreg] = addr;
		} else {
			dsp.registers[numreg] = read_memory(memspace, addr);
		}
	} else {
		/* Read S1 */

		retour = dsp_calc_ea(ea_mode, &addr);

		write_memory(memspace, addr, dsp.registers[numreg]);
	}
}

static void dsp_movec_d(void)
{
	/* #xx,D1 */

	uint32 numreg;

	numreg = (cur_inst & BITMASK(5))|0x20;
	dsp.registers[numreg] = (cur_inst>>8) & BITMASK(8);
}

static void dsp_movem(void)
{
	uint32 numreg, addr, ea_mode, value;

	numreg = cur_inst & BITMASK(6);

	if (cur_inst & (1<<14)) {
		/* S,p:ea */
		/* p:ea,D */

		ea_mode = (cur_inst>>8) & BITMASK(6);
		dsp_calc_ea(ea_mode, &addr);
	} else {
		/* S,p:aa */
		/* p:aa,D */

		addr = (cur_inst>>8) & BITMASK(6);
	}

	if  (cur_inst & (1<<15)) {
		/* Write D */

		if ((numreg == REG_A) || (numreg == REG_B)) {
			value = read_memory(SPACE_P, addr);
			dsp.registers[REG_A2+(numreg & 1)] = 0;
			if (value & (1<<23)) {
				dsp.registers[REG_A2+(numreg & 1)] = 0xff;
			}
			dsp.registers[REG_A1+(numreg & 1)] = value;
			dsp.registers[REG_A0+(numreg & 1)] = 0;
		} else {
			dsp.registers[numreg] = read_memory(SPACE_P, addr);
		}
	} else {
		/* Read S */

		if ((numreg == REG_A) || (numreg == REG_B)) {
			write_memory(SPACE_P, addr, dsp.registers[REG_A1+(numreg & 1)]);
		} else {
			write_memory(SPACE_P, addr, dsp.registers[numreg]);
		}
	}
}

static void dsp_movep(void)
{
	uint32 value;
	
	value = (cur_inst>>6) & BITMASK(2);

	opcodes_movep[value]();
}

static void dsp_movep_0(void)
{
	/* S,x:pp */
	/* x:pp,D */
	/* S,y:pp */
	/* y:pp,D */
	
	uint32 addr, memspace, numreg, value;

	addr = 0xffc0 + (cur_inst & BITMASK(6));
	memspace = (cur_inst>>16) & 1;
	numreg = (cur_inst>>8) & BITMASK(6);

	if  (cur_inst & (1<<15)) {
		/* Write D */

		if ((numreg == REG_A) || (numreg == REG_B)) {
			value = read_memory(memspace, addr);
			dsp.registers[REG_A2+(numreg & 1)] = 0;
			if (value & (1<<23)) {
				dsp.registers[REG_A2+(numreg & 1)] = 0xff;
			}
			dsp.registers[REG_A1+(numreg & 1)] = value;
			dsp.registers[REG_A0+(numreg & 1)] = 0;
		} else {
			dsp.registers[numreg] = read_memory(memspace, addr);
		}
	} else {
		/* Read S */

		if ((numreg == REG_A) || (numreg == REG_B)) {
			write_memory(memspace, addr, dsp.registers[REG_A1+(numreg & 1)]);
		} else {
			write_memory(memspace, addr, dsp.registers[numreg]);
		}
	}
}

static void dsp_movep_1(void)
{
	/* p:ea,x:pp */
	/* x:pp,p:ea */
	/* p:ea,y:pp */
	/* y:pp,p:ea */

	uint32 xyaddr, memspace, paddr;

	xyaddr = 0xffc0 + (cur_inst & BITMASK(6));
	dsp_calc_ea((cur_inst>>8) & BITMASK(6), &paddr);
	memspace = (cur_inst>>16) & 1;

	if (cur_inst & (1<<15)) {
		/* Write */
		write_memory(memspace, xyaddr, read_memory(SPACE_P, paddr));
	} else {
		/* Read */
		write_memory(SPACE_P, paddr, read_memory(memspace, xyaddr));
	}
}

static void dsp_movep_2(void)
{
	/* x:ea,x:pp */
	/* y:ea,x:pp */
	/* #xxxxxx,x:pp */
	/* x:pp,x:ea */
	/* x:pp,y:pp */
	/* x:ea,y:pp */
	/* y:ea,y:pp */
	/* #xxxxxx,y:pp */
	/* y:pp,x:ea */
	/* y:pp,y:pp */

	uint32 addr, peraddr, easpace, perspace, ea_mode;
	int retour;

	peraddr = 0xffc0 + (cur_inst & BITMASK(6));
	perspace = (cur_inst>>16) & 1;
	
	ea_mode = (cur_inst>>8) & BITMASK(6);
	easpace = (cur_inst>>6) & 1;
	retour = dsp_calc_ea(ea_mode, &addr);

	if (cur_inst & (1<<15)) {
		/* Write per */

		if (retour) {
			write_memory(perspace, peraddr, addr);
		} else {
			write_memory(perspace, peraddr, read_memory(easpace, addr));
		}
	} else {
		/* Read per */

		write_memory(easpace, addr, read_memory(perspace, peraddr));
	}
}

static void dsp_norm(void)
{
	D(bug("Dsp: 0x%04x: norm",dsp.pc));
	cur_inst_len = 0;
}

static void dsp_ori(void)
{
	uint32 regnum, value;

	value = (cur_inst >> 8) & BITMASK(8);
	regnum = cur_inst & BITMASK(2);
	switch(regnum) {
		case 0:
			/* mr */
			dsp.registers[REG_SR] |= value<<8;
			break;
		case 1:
			/* ccr */
			dsp.registers[REG_SR] |= value;
			break;
		case 2:
			/* omr */
			dsp.registers[REG_OMR] |= value;
			break;
	}
}

static void dsp_rep(void)
{
	uint32 value;

	dsp.registers[REG_LCSAVE] = dsp.registers[REG_LC];

	value = (cur_inst>>12) & (BITMASK(2)<<2);
	value |= (cur_inst>>6) & (1<<1);
	value |= (cur_inst>>5) & 1;

	opcodes_rep[value]();
	pc_on_rep = 1;		/* Not decrement LC at first time */
	dsp.loop_rep = 1;	/* We are now running rep */
}

static void dsp_rep_1(void)
{
	/* x:aa */
	/* y:aa */

	dsp.registers[REG_LC]=read_memory((cur_inst>>6) & 1,(cur_inst>>8) & BITMASK(6));
}

static void dsp_rep_3(void)
{
	/* #xxx */

	dsp.registers[REG_LC]= (cur_inst>>8) & BITMASK(8);
}

static void dsp_rep_5(void)
{
	uint32 value;

	/* x:ea */
	/* y:ea */

	dsp_calc_ea((cur_inst>>8) & BITMASK(6),&value);
	dsp.registers[REG_LC]= value;
}

static void dsp_rep_d(void)
{
	uint32 numreg;

	/* R */

	numreg = (cur_inst>>8) & BITMASK(6);
	if ((numreg == REG_A) || (numreg == REG_B)) {
		numreg = REG_A1+(numreg & 1);
	}
	dsp.registers[REG_LC] = dsp.registers[numreg];
}

static void dsp_reset(void)
{
}

static void dsp_rti(void)
{
	uint32 newpc, newsr;

	dsp_stack_pop(&newpc, &newsr);

	dsp.pc = newpc;
	dsp.registers[REG_SR] = newsr;

	cur_inst_len = 0;
}

static void dsp_rts(void)
{
	uint32 newpc, newsr;

	dsp_stack_pop(&newpc, &newsr);

	dsp.pc = newpc;
	cur_inst_len = 0;
}

static void dsp_stop(void)
{
	dsp.state = DSP_WAITINTERRUPT;
}

static void dsp_swi(void)
{
	D(bug("Dsp: 0x%04x: swi",dsp.pc));
	cur_inst_len = 0;
}

static void dsp_tcc(void)
{
	uint32 cc_code, regsrc1, regdest1, value;
	uint32 regsrc2, regdest2;

	cc_code = (cur_inst>>12) & BITMASK(4);

	if (dsp_calc_cc(cc_code)) {
		regsrc1 = registers_tcc[(cur_inst>>3) & BITMASK(4)][0];
		regdest1 = registers_tcc[(cur_inst>>3) & BITMASK(4)][0];

		/* Read S1 */
		if ((regsrc1 == REG_A) || (regsrc1 == REG_B)) {
			tmp_parmove_src[0][0]=dsp.registers[REG_A2+(regsrc1 & 1)];
			tmp_parmove_src[0][1]=dsp.registers[REG_A1+(regsrc1 & 1)];
			tmp_parmove_src[0][2]=dsp.registers[REG_A0+(regsrc1 & 1)];
		} else {
			value = dsp.registers[regsrc1];
			tmp_parmove_src[0][0]=0;
			if (value & (1<<23)) {
				tmp_parmove_src[0][0]=0x0000ff;
			}
			tmp_parmove_src[0][1]=value;
			tmp_parmove_src[0][2]=0;
		}
		
		/* Write D1 */
		dsp.registers[REG_A2+(regdest1 & 1)]=tmp_parmove_src[0][0];
		dsp.registers[REG_A1+(regdest1 & 1)]=tmp_parmove_src[0][1];
		dsp.registers[REG_A0+(regdest1 & 1)]=tmp_parmove_src[0][2];

		/* S2,D2 transfer */
		if (cur_inst & (1<<16)) {
			regsrc2 = REG_R0+(cur_inst & BITMASK(3));
			regdest2 = REG_R0+((cur_inst>>8) & BITMASK(3));

			dsp.registers[regdest2] = dsp.registers[regsrc2];
		}
	}
}

static void dsp_wait(void)
{
	dsp.state = DSP_WAITINTERRUPT;
}

/**********************************
 *	Parallel moves instructions dispatcher
 **********************************/

static void dsp_parmove_read(void)
{
	uint32 value;

	tmp_parmove_len[0] = tmp_parmove_len[1] = 0;

	/* Calculate needed parallel moves */
	value = (cur_inst >> 20) & BITMASK(4);

	/* Do parallel move read */
	opcodes_parmove[value]();
}

static void dsp_parmove_write(void)
{
	uint32 i,j;
	uint32 *dest;
	
	for(i=0;i<2;i++) {
		if (tmp_parmove_len[i]==0) {
			continue;
		}

		/* Do parallel move write */
		for (
			j=tmp_parmove_start[i];
			j<tmp_parmove_start[i]+tmp_parmove_len[i];
			j++
		) {
			dest=tmp_parmove_dest[i][j];
			if (tmp_parmove_type[i]) {
				/* Write to memory */
				write_memory(tmp_parmove_space[i], (uint16) dest, tmp_parmove_src[i][j]);
			} else {
				/* Write to register */
				*dest = tmp_parmove_src[i][j];
			}
		}
	}
}

static void dsp_pm_0(void)
{
	uint32 memspace, dummy, numreg, value;
/*
	0000 100d 00mm mrrr S,x:ea	x0,D
	0000 100d 10mm mrrr S,y:ea	y0,D
*/
	memspace = (cur_inst>>15) & BITMASK(1);
	numreg = (cur_inst>>16) & BITMASK(1);
	dsp_calc_ea((cur_inst>>8) & BITMASK(6), &dummy);

	/* [A|B] to [x|y]:ea */	
/*	tmp_parmove_src[0][0]=dsp.registers[REG_A2+numreg];*/
	tmp_parmove_src[0][1]=dsp.registers[REG_A1+numreg];
/*	tmp_parmove_src[0][2]=dsp.registers[REG_A0+numreg];*/
	tmp_parmove_dest[0][1]=(uint32 *)dummy;

	tmp_parmove_start[0] = 1;
	tmp_parmove_len[0] = 1;

	tmp_parmove_type[0]=1;
	tmp_parmove_space[0]=memspace;

	/* [x|y]0 to [A|B] */
	value = dsp.registers[REG_X0+(memspace<<1)];
	if (value & (1<<23)) {
		tmp_parmove_src[1][0]=0x0000ff;
	} else {
		tmp_parmove_src[1][0]=0x000000;
	}
	tmp_parmove_src[1][1]=value;
	tmp_parmove_src[1][2]=0x000000;
	tmp_parmove_dest[1][0]=&dsp.registers[REG_A2+numreg];
	tmp_parmove_dest[1][1]=&dsp.registers[REG_A1+numreg];
	tmp_parmove_dest[1][2]=&dsp.registers[REG_A0+numreg];

	tmp_parmove_start[1] = 0;
	tmp_parmove_len[1] = 3;

	tmp_parmove_type[0]=0;
}

static void dsp_pm_1(void)
{
	uint32 value, xy_addr, retour;
/*
	0001 ffdf w0mm mrrr x:ea,D1		S2,D2
						S1,x:ea		S2,D2
						#xxxxxx,D1	S2,D2
	0001 deff w1mm mrrr S1,D1		y:ea,D2
						S1,D1		S2,y:ea
						S1,D1		#xxxxxx,D2
*/
	value = (cur_inst>>8) & BITMASK(6);

	retour = dsp_calc_ea(value, &xy_addr);	

	dsp_pm_1x(retour, xy_addr);
}

static void dsp_pm_1x(int immediat, uint16 xy_addr)
{
	uint32 memspace, numreg, value;
/*
	0001 ffdf w0mm mrrr x:ea,D1		S2,D2
						S1,x:ea		S2,D2
						#xxxxxx,D1	S2,D2
	0001 deff w1mm mrrr y:ea,D1		S2,D2		
						S1,y:ea		S2,D2		
						#xxxxxx,D1	S2,D2		
*/
	memspace = (cur_inst>>14) & 1;
	numreg = REG_NULL;

	if (memspace) {
		/* Y: */
		switch((cur_inst>>16) & BITMASK(2)) {
			case 0:
				numreg = REG_Y0;
				break;
			case 1:
				numreg = REG_Y1;
				break;
			case 2:
				numreg = REG_A;
				break;
			case 3:
				numreg = REG_B;
				break;
		}
	} else {
		/* X: */
		switch((cur_inst>>18) & BITMASK(2)) {
			case 0:
				numreg = REG_X0;
				break;
			case 1:
				numreg = REG_X1;
				break;
			case 2:
				numreg = REG_A;
				break;
			case 3:
				numreg = REG_B;
				break;
		}
	}

	if (cur_inst & (1<<15)) {
		/* Write D1 */

		if (immediat) {
			value = xy_addr;
		} else {
			value = read_memory(memspace, xy_addr);
		}
		tmp_parmove_src[0][0]= 0x000000;
		if (value & (1<<23)) {
			tmp_parmove_src[0][0]= 0x0000ff;
		}
		tmp_parmove_src[0][1]= value;
		tmp_parmove_src[0][2]= 0x000000;

		if ((numreg == REG_A) || (numreg == REG_B)) {
			tmp_parmove_dest[0][0]=&dsp.registers[REG_A2+(numreg & 1)];
			tmp_parmove_dest[0][1]=&dsp.registers[REG_A1+(numreg & 1)];
			tmp_parmove_dest[0][2]=&dsp.registers[REG_A0+(numreg & 1)];

			tmp_parmove_start[0]=0;
			tmp_parmove_len[0]=3;
		} else {
			tmp_parmove_dest[0][1]=&dsp.registers[numreg];

			tmp_parmove_start[0]=1;
			tmp_parmove_len[0]=1;
		}
		tmp_parmove_type[0]=0;
	} else {
		/* Read S1 */

		tmp_parmove_src[0][1]=dsp.registers[numreg];

		tmp_parmove_dest[0][1]=(uint32 *)xy_addr;

		tmp_parmove_start[0]=1;
		tmp_parmove_len[0]=1;

		tmp_parmove_type[0]=1;
		tmp_parmove_space[0]=memspace;
	}

	/* S2 */
	if (memspace) {
		/* Y: */
		numreg = REG_A + ((cur_inst>>19) & 1);
	} else {
		/* X: */
		numreg = REG_A + ((cur_inst>>17) & 1);
	}	
	tmp_parmove_src[1][1]=dsp.registers[numreg];
	
	/* D2 */
	if (memspace) {
		/* Y: */
		numreg = REG_Y0 + ((cur_inst>>18) & 1);
	} else {
		/* X: */
		numreg = REG_X0 + ((cur_inst>>16) & 1);
	}	
	tmp_parmove_dest[1][1]=&dsp.registers[numreg];

	tmp_parmove_start[0]=1;
	tmp_parmove_len[0]=1;

	tmp_parmove_type[0]=0;
}

static void dsp_pm_2(void)
{
	uint32 dummy;
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
		dsp_calc_ea(cur_inst & BITMASK(5), &dummy);
		return;
	}

	if (((cur_inst >> 8) & 0xfc00) == 0x2000) {
		dsp_pm_2_2();
		return;
	}

	dsp_pm_3();
}

static void dsp_pm_2_2(void)
{
/*
	0010 00ee eeed dddd S,D
*/
	uint32 source, dest, srcvalue;
	
	source = (cur_inst >> 13) & BITMASK(5);
	dest = (cur_inst >> 8) & BITMASK(5);

	srcvalue = dsp.registers[source];
	tmp_parmove_src[0][0]=0x000000;
	if ((dest == REG_A) || (dest == REG_B)) {
		if (srcvalue & (1<<23)) {
			tmp_parmove_src[0][0]=0x0000ff;
		}
	}
	tmp_parmove_src[0][1]=srcvalue;
	tmp_parmove_src[0][2]=0x000000;

	if ((dest == REG_A) || (dest == REG_B)) {
		tmp_parmove_dest[0][0]=&dsp.registers[REG_A2+(dest & 1)];
		tmp_parmove_dest[0][1]=&dsp.registers[REG_A1+(dest & 1)];
		tmp_parmove_dest[0][2]=&dsp.registers[REG_A0+(dest & 1)];
		tmp_parmove_start[0] = 0;
		tmp_parmove_len[0] = 3;
	} else {
		tmp_parmove_dest[0][1]=&dsp.registers[dest];
		tmp_parmove_start[0] = 1;
		tmp_parmove_len[0] = 1;
	}
	
	tmp_parmove_type[0]=0;
}

static void dsp_pm_3(void)
{
	uint32 dest, srcvalue;
/*
	001d dddd iiii iiii #xx,R
*/
	dest = (cur_inst >> 16) & BITMASK(5);
	srcvalue = (cur_inst >> 8) & BITMASK(8);

	switch(dest) {
		case REG_X0:
		case REG_X1:
		case REG_Y0:
		case REG_Y1:
		case REG_A:
		case REG_B:
			if (srcvalue & (1<<7)) {
				srcvalue |= 0xffff00;
			}
			break;
	}

	tmp_parmove_src[0][0]=0x000000;
	if ((dest == REG_A) || (dest == REG_B)) {
		if (srcvalue & (1<<23)) {
			tmp_parmove_src[0][0]=0x0000ff;
		}
	}
	tmp_parmove_src[0][1]=srcvalue;
	tmp_parmove_src[0][2]=0x000000;

	if ((dest == REG_A) || (dest == REG_B)) {
		tmp_parmove_dest[0][0]=&dsp.registers[REG_A2+(dest & 1)];
		tmp_parmove_dest[0][1]=&dsp.registers[REG_A1+(dest & 1)];
		tmp_parmove_dest[0][2]=&dsp.registers[REG_A0+(dest & 1)];
		tmp_parmove_start[0] = 0;
		tmp_parmove_len[0] = 3;
	} else {
		tmp_parmove_dest[0][1]=&dsp.registers[dest];
		tmp_parmove_start[0] = 1;
		tmp_parmove_len[0] = 1;
	}

	tmp_parmove_type[0]=0;
}

static void dsp_pm_4(void)
{
	uint32 l_addr, value;
	int retour;
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

	if ((value>>2)==0) {
		value = (cur_inst>>8) & BITMASK(6);
		if (cur_inst & (1<<14)) {
			retour = dsp_calc_ea(value, &l_addr);	
		} else {
			l_addr = value;
			retour = 0;
		}
		dsp_pm_4x(retour, l_addr);
		return;
	}

	dsp_pm_5();
}

static void dsp_pm_4x(int immediat, uint16 l_addr)
{
	uint32 value, numreg, numreg2;
/*
	0100 l0ll w0aa aaaa l:aa,D
						S,l:aa
	0100 l0ll w1mm mrrr l:ea,D
						S,l:ea
*/
	numreg = (cur_inst>>16) & BITMASK(2);
	numreg |= (cur_inst>>17) & (1<<2);

	if (cur_inst & (1<<15)) {
		/* Read S */

		/* S1 */
/*		tmp_parmove_src[0][0]=0x000000;*/
		numreg2 = registers_lmove[numreg][0];
		if ((numreg2 == REG_A) || (numreg2 == REG_B)) {
			tmp_parmove_src[0][1]=dsp.registers[REG_A2+(numreg2 & 1)];
		} else {
			tmp_parmove_src[0][1]=dsp.registers[numreg2];
		}
/*		tmp_parmove_src[0][2]=0x000000;*/
		
		/* S2 */
/*		tmp_parmove_src[1][0]=0x000000;*/
		numreg2 = registers_lmove[numreg][1];
		if ((numreg2 == REG_A) || (numreg2 == REG_B)) {
			tmp_parmove_src[1][1]=dsp.registers[REG_A2+(numreg2 & 1)];
		} else {
			tmp_parmove_src[1][1]=dsp.registers[numreg2];
		}
/*		tmp_parmove_src[1][2]=0x000000;*/
		
		/* D1 */
/*		tmp_parmove_dest[0][0]=NULL;*/
		tmp_parmove_dest[0][1]=(uint32 *)l_addr;
/*		tmp_parmove_dest[0][2]=NULL;*/

		tmp_parmove_start[0]=1;
		tmp_parmove_len[0]=1;
		
		tmp_parmove_type[0]=1;
		tmp_parmove_space[0]=SPACE_X;

		/* D2 */
/*		tmp_parmove_dest[1][0]=NULL;*/
		tmp_parmove_dest[1][1]=(uint32 *)l_addr;
/*		tmp_parmove_dest[1][2]=NULL;*/

		tmp_parmove_start[0]=1;
		tmp_parmove_len[0]=1;

		tmp_parmove_type[0]=1;
		tmp_parmove_space[0]=SPACE_Y;
	} else {
		/* Write D */

		/* S1 */
		value = read_memory(SPACE_X,l_addr);
		tmp_parmove_src[0][0] = 0x000000;
		if (value & (1<<23)) {
			tmp_parmove_src[0][0] = 0x0000ff;
		}
		tmp_parmove_src[0][1] = value;
		tmp_parmove_src[0][2] = 0x000000;

		/* S2 */
		value = read_memory(SPACE_Y,l_addr);
		tmp_parmove_src[1][0] = 0x000000;
		if (value & (1<<23)) {
			tmp_parmove_src[1][0] = 0x0000ff;
		}
		tmp_parmove_src[1][1] = value;
		tmp_parmove_src[1][2] = 0x000000;

		/* D1 */
		tmp_parmove_dest[0][0] = NULL;
		tmp_parmove_start[0]=1;
		tmp_parmove_len[0]=1;
		if (numreg > 4) {
			tmp_parmove_dest[0][0] = &dsp.registers[REG_A2+(numreg & 1)];
			tmp_parmove_start[0]=0;
			tmp_parmove_len[0]=2;
		}
		numreg2 = registers_lmove[numreg][0];
		if ((numreg2 == REG_A) || (numreg2 == REG_B)) {
			tmp_parmove_dest[0][1] = &dsp.registers[REG_A1+(numreg2 & 1)];
		} else {
			tmp_parmove_dest[0][1] = &dsp.registers[numreg2];
		}
		if (numreg > 6) {
			tmp_parmove_dest[0][2] = &dsp.registers[REG_A0+(numreg & 1)];
			tmp_parmove_start[0]=0;
			tmp_parmove_len[0]=3;
		}

		tmp_parmove_type[0]=0;

		/* D2 */
		tmp_parmove_dest[1][0] = NULL;
		tmp_parmove_start[1]=1;
		tmp_parmove_len[1]=1;
		if (numreg > 4) {
			tmp_parmove_dest[1][0] = &dsp.registers[REG_A2+(numreg & 1)];
			tmp_parmove_start[1]=0;
			tmp_parmove_len[1]=2;
		}
		numreg2 = registers_lmove[numreg][1];
		if ((numreg2 == REG_A) || (numreg2 == REG_B)) {
			tmp_parmove_dest[1][1] = &dsp.registers[REG_A1+(numreg2 & 1)];
		} else {
			tmp_parmove_dest[1][1] = &dsp.registers[numreg2];
		}
		if (numreg > 6) {
			tmp_parmove_dest[1][2] = &dsp.registers[REG_A0+(numreg & 1)];
			tmp_parmove_start[1]=0;
			tmp_parmove_len[1]=3;
		}

		tmp_parmove_type[1]=0;
	}
}

static void dsp_pm_5(void)
{
	uint32 xy_addr;
	uint32 value;
	int retour;
/*
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

	value = (cur_inst>>8) & BITMASK(6);

	if (cur_inst & (1<<14)) {
		retour = dsp_calc_ea(value, &xy_addr);	
	} else {
		xy_addr = value;
		retour = 0;
	}

	dsp_pm_5x(retour, xy_addr);
}

static void dsp_pm_5x(int immediat, uint16 xy_addr)
{
	uint32 memspace, numreg, value;
/*
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
	memspace = (cur_inst>>19) & 1;
	numreg = (cur_inst>>16) & BITMASK(3);
	numreg |= (cur_inst>>17) & (BITMASK(2)<<3);

	if (cur_inst & (1<<14)) {
		/* Write D */

		if (immediat) {
			value = xy_addr;
		} else {
			value = read_memory(memspace, xy_addr);
		}
		tmp_parmove_src[0][0]= 0x000000;
		if (value & (1<<23)) {
			tmp_parmove_src[0][0]= 0x0000ff;
		}
		tmp_parmove_src[0][1]= value;
		tmp_parmove_src[0][2]= 0x000000;

		if ((numreg == REG_A) || (numreg == REG_B)) {
			tmp_parmove_dest[0][0]=&dsp.registers[REG_A2+(numreg & 1)];
			tmp_parmove_dest[0][1]=&dsp.registers[REG_A1+(numreg & 1)];
			tmp_parmove_dest[0][2]=&dsp.registers[REG_A0+(numreg & 1)];

			tmp_parmove_start[0]=0;
			tmp_parmove_len[0]=3;
		} else {
			tmp_parmove_dest[0][1]=&dsp.registers[numreg];

			tmp_parmove_start[0]=1;
			tmp_parmove_len[0]=1;
		}
		tmp_parmove_type[0]=0;
	} else {
		/* Read S */

		tmp_parmove_src[0][1]=dsp.registers[numreg];

		tmp_parmove_dest[0][1]=(uint32 *)xy_addr;

		tmp_parmove_start[0]=1;
		tmp_parmove_len[0]=1;

		tmp_parmove_type[0]=1;
		tmp_parmove_space[0]=memspace;
	}
}

static void dsp_pm_8(void)
{
	uint32 ea1, ea2;
	uint32 numreg1, numreg2;
	uint32 value, dummy1, dummy2;
/*
	1wmm eeff WrrM MRRR x:ea,D1		y:ea,D2	
						x:ea,D1		S2,y:ea
						S1,x:ea		y:ea,D2
						S1,x:ea		S2,y:ea
*/
	numreg1 = numreg2 = REG_NULL;

	ea1 = (cur_inst>>8) & BITMASK(5);
	if ((ea1>>3) == 0) {
		ea1 |= (1<<5);
	}
	ea2 = (cur_inst>>13) & BITMASK(2);
	ea2 |= (cur_inst>>17) & (BITMASK(2)<<3);
	ea2 |= ~(ea1 & (1<<2));
	if ((ea2>>3) == 0) {
		ea2 |= (1<<5);
	}

	dsp_calc_ea(ea1, &dummy1);
	dsp_calc_ea(ea2, &dummy2);

	switch((cur_inst>>18) & BITMASK(2)) {
		case 0:
			numreg1=REG_X0;
			break;
		case 1:
			numreg1=REG_X1;
			break;
		case 2:
			numreg1=REG_A;
			break;
		case 3:
			numreg1=REG_B;
			break;
	}
	switch((cur_inst>>16) & BITMASK(2)) {
		case 0:
			numreg2=REG_Y0;
			break;
		case 1:
			numreg2=REG_Y1;
			break;
		case 2:
			numreg2=REG_A;
			break;
		case 3:
			numreg2=REG_B;
			break;
	}
	
	if (cur_inst & (1<<15)) {
		/* Write D1 */

		value = read_memory(SPACE_X, dummy1);
		tmp_parmove_src[0][0]= 0x000000;
		if (value & (1<<23)) {
			tmp_parmove_src[0][0]= 0x0000ff;
		}
		tmp_parmove_src[0][1]= value;
		tmp_parmove_src[0][2]= 0x000000;

		if ((numreg1 == REG_A) || (numreg1 == REG_B)) {
			tmp_parmove_dest[0][0]=&dsp.registers[REG_A2+(numreg1 & 1)];
			tmp_parmove_dest[0][1]=&dsp.registers[REG_A1+(numreg1 & 1)];
			tmp_parmove_dest[0][2]=&dsp.registers[REG_A0+(numreg1 & 1)];

			tmp_parmove_start[0]=0;
			tmp_parmove_len[0]=3;
		} else {
			tmp_parmove_dest[0][1]=&dsp.registers[numreg1];

			tmp_parmove_start[0]=1;
			tmp_parmove_len[0]=1;
		}
		tmp_parmove_type[0]=0;
	} else {
		/* Read S1 */

		tmp_parmove_src[0][1]=dsp.registers[numreg1];

		tmp_parmove_dest[0][1]=(uint32 *)dummy1;

		tmp_parmove_start[0]=1;
		tmp_parmove_len[0]=1;

		tmp_parmove_type[0]=1;
		tmp_parmove_space[0]=SPACE_X;
	}

	if (cur_inst & (1<<22)) {
		/* Write D2 */

		value = read_memory(SPACE_Y, dummy2);
		tmp_parmove_src[1][0]= 0x000000;
		if (value & (1<<23)) {
			tmp_parmove_src[1][0]= 0x0000ff;
		}
		tmp_parmove_src[1][1]= value;
		tmp_parmove_src[1][2]= 0x000000;

		if ((numreg2 == REG_A) || (numreg2 == REG_B)) {
			tmp_parmove_dest[1][0]=&dsp.registers[REG_A2+(numreg2 & 1)];
			tmp_parmove_dest[1][1]=&dsp.registers[REG_A1+(numreg2 & 1)];
			tmp_parmove_dest[1][2]=&dsp.registers[REG_A0+(numreg2 & 1)];

			tmp_parmove_start[1]=0;
			tmp_parmove_len[1]=3;
		} else {
			tmp_parmove_dest[1][1]=&dsp.registers[numreg2];

			tmp_parmove_start[1]=1;
			tmp_parmove_len[1]=1;
		}
		tmp_parmove_type[1]=0;
	} else {
		/* Read S2 */
		tmp_parmove_src[1][1]=dsp.registers[numreg2];

		tmp_parmove_dest[1][1]=(uint32 *)dummy2;

		tmp_parmove_start[1]=1;
		tmp_parmove_len[1]=1;

		tmp_parmove_type[1]=1;
		tmp_parmove_space[1]=SPACE_Y;
	}
}

/**********************************
 *	Parallel moves instructions
 **********************************/

static void dsp_abs(void)
{
	D(bug("Dsp: 0x%04x: abs",dsp.pc));
	cur_inst_len = 0;
}

static void dsp_adc(void)
{
	D(bug("Dsp: 0x%04x: adc",dsp.pc));
	cur_inst_len = 0;
}

static void dsp_add(void)
{
	D(bug("Dsp: 0x%04x: add",dsp.pc));
	cur_inst_len = 0;
}

static void dsp_addl(void)
{
	D(bug("Dsp: 0x%04x: addl",dsp.pc));
	cur_inst_len = 0;
}

static void dsp_addr(void)
{
	D(bug("Dsp: 0x%04x: addr",dsp.pc));
	cur_inst_len = 0;
}

static void dsp_and(void)
{
	uint32 srcreg, dstreg;

	srcreg = REG_X0+((cur_inst>>4) & BITMASK(2));
	dstreg = REG_A1+((cur_inst>>3) & 1);

	dsp.registers[dstreg] &= dsp.registers[srcreg];
	dsp.registers[dstreg] &= BITMASK(24); /* FIXME: useless ? */
}

static void dsp_asl(void)
{
	uint32 numreg, newcarry;

	numreg = (cur_inst>>3) & 1;

	newcarry = (dsp.registers[REG_A2+numreg]>>7) & 1;

	dsp.registers[REG_A2+numreg] <<= 1;
	dsp.registers[REG_A2+numreg] |= (dsp.registers[REG_A1+numreg]>>23) & 1;
	dsp.registers[REG_A2+numreg] &= BITMASK(8);

	dsp.registers[REG_A1+numreg] <<= 1;
	dsp.registers[REG_A1+numreg] |= (dsp.registers[REG_A0+numreg]>>23) & 1;
	dsp.registers[REG_A1+numreg] &= BITMASK(24);
	
	dsp.registers[REG_A0+numreg] <<= 1;
	dsp.registers[REG_A0+numreg] &= BITMASK(24);

	dsp.registers[REG_SR] &= 0xfffe;
	dsp.registers[REG_SR] |= newcarry;
}

static void dsp_asr(void)
{
	uint32 numreg, newcarry;

	numreg = (cur_inst>>3) & 1;

	newcarry = dsp.registers[REG_A0+numreg] & 1;

	dsp.registers[REG_A0+numreg] >>= 1;
	dsp.registers[REG_A0+numreg] &= BITMASK(23);
	dsp.registers[REG_A0+numreg] |= (dsp.registers[REG_A1+numreg] & 1)<<23;

	dsp.registers[REG_A1+numreg] >>= 1;
	dsp.registers[REG_A1+numreg] &= BITMASK(23);
	dsp.registers[REG_A1+numreg] |= (dsp.registers[REG_A2+numreg] & 1)<<23;

	dsp.registers[REG_A2+numreg] >>= 1;
	dsp.registers[REG_A2+numreg] &= BITMASK(7);
	dsp.registers[REG_A2+numreg] |= (dsp.registers[REG_A2+numreg] & (1<<6))<<1;

	dsp.registers[REG_SR] &= 0xfffe;
	dsp.registers[REG_SR] |= newcarry;
}

static void dsp_clr(void)
{
	uint32 numreg, value;

	numreg = (cur_inst>>3) & 1;

	dsp.registers[REG_A2+numreg]=0;
	dsp.registers[REG_A1+numreg]=0;
	dsp.registers[REG_A0+numreg]=0;

	value = dsp.registers[REG_SR];
	value &= BITMASK(16)-((1<<SR_E)|(1<<SR_N)|(1<<SR_V));
	value |= (1<<SR_U)|(1<<SR_Z);
	dsp.registers[REG_SR] = value;
}

static void dsp_cmp(void)
{
	D(bug("Dsp: 0x%04x: cmp",dsp.pc));
	cur_inst_len = 0;
}

static void dsp_cmpm(void)
{
	D(bug("Dsp: 0x%04x: cmpm",dsp.pc));
	cur_inst_len = 0;
}

static void dsp_eor(void)
{
	uint32 srcreg, dstreg;

	srcreg = REG_X0+((cur_inst>>4) & BITMASK(2));
	dstreg = REG_A1+((cur_inst>>3) & 1);

	dsp.registers[dstreg] ^= dsp.registers[srcreg];
	dsp.registers[dstreg] &= BITMASK(24); /* FIXME: useless ? */
}

static void dsp_lsl(void)
{
	uint32 numreg, newcarry;

	numreg = (cur_inst>>3) & 1;

	newcarry = (dsp.registers[REG_A1+numreg]>>23) & 1;

	dsp.registers[REG_A1+numreg] &= BITMASK(24);
	dsp.registers[REG_A1+numreg] <<= 1;

	dsp.registers[REG_SR] &= 0xfffe;
	dsp.registers[REG_SR] |= newcarry;
}

static void dsp_lsr(void)
{
	uint32 numreg, newcarry;

	numreg = (cur_inst>>3) & 1;

	newcarry = dsp.registers[REG_A1+numreg] & 1;

	dsp.registers[REG_A1+numreg] &= BITMASK(24);
	dsp.registers[REG_A1+numreg] >>= 1;

	dsp.registers[REG_SR] &= 0xfffe;
	dsp.registers[REG_SR] |= newcarry;
}

static void dsp_mac(void)
{
	D(bug("Dsp: 0x%04x: mac",dsp.pc));
	cur_inst_len = 0;
}

static void dsp_macr(void)
{
	D(bug("Dsp: 0x%04x: macr",dsp.pc));
	cur_inst_len = 0;
}

static void dsp_move(void)
{
}

static void dsp_mpy(void)
{
	D(bug("Dsp: 0x%04x: mpy",dsp.pc));
	cur_inst_len = 0;
}

static void dsp_mpyr(void)
{
	D(bug("Dsp: 0x%04x: mpyr",dsp.pc));
	cur_inst_len = 0;
}

static void dsp_neg(void)
{
	D(bug("Dsp: 0x%04x: neg",dsp.pc));
	cur_inst_len = 0;
}

static void dsp_nop(void)
{
}

static void dsp_not(void)
{
	uint32 dstreg;

	dstreg = REG_A1+((cur_inst>>3) & 1);

	dsp.registers[dstreg] = ~dsp.registers[dstreg];
	dsp.registers[dstreg] &= BITMASK(24); /* FIXME: useless ? */
}

static void dsp_or(void)
{
	uint32 srcreg, dstreg;

	srcreg = REG_X0+((cur_inst>>4) & BITMASK(2));
	dstreg = REG_A1+((cur_inst>>3) & 1);

	dsp.registers[dstreg] |= dsp.registers[srcreg];
	dsp.registers[dstreg] &= BITMASK(24); /* FIXME: useless ? */
}

static void dsp_rnd(void)
{
	D(bug("Dsp: 0x%04x: rnd",dsp.pc));
	cur_inst_len = 0;
}

static void dsp_rol(void)
{
	uint32 dstreg, newcarry;

	dstreg = REG_A1+((cur_inst>>3) & 1);

	newcarry = (dsp.registers[dstreg]>>23) & 1;

	dsp.registers[dstreg] <<= 1;
	dsp.registers[dstreg] |= newcarry;
	dsp.registers[dstreg] &= BITMASK(24);

	dsp.registers[REG_SR] &= 0xfffe;
	dsp.registers[REG_SR] |= newcarry;
}

static void dsp_ror(void)
{
	uint32 dstreg, newcarry;

	dstreg = REG_A1+((cur_inst>>3) & 1);

	newcarry = dsp.registers[dstreg] & 1;

	dsp.registers[dstreg] >>= 1;
	dsp.registers[dstreg] |= newcarry<<23;
	dsp.registers[dstreg] &= BITMASK(24);

	dsp.registers[REG_SR] &= 0xfffe;
	dsp.registers[REG_SR] |= newcarry;
}

static void dsp_sbc(void)
{
	D(bug("Dsp: 0x%04x: sbc",dsp.pc));
	cur_inst_len = 0;
}

static void dsp_sub(void)
{
	D(bug("Dsp: 0x%04x: sub",dsp.pc));
	cur_inst_len = 0;
}

static void dsp_subl(void)
{
	D(bug("Dsp: 0x%04x: subl",dsp.pc));
	cur_inst_len = 0;
}

static void dsp_subr(void)
{
	D(bug("Dsp: 0x%04x: subr",dsp.pc));
	cur_inst_len = 0;
}

static void dsp_tfr(void)
{
	D(bug("Dsp: 0x%04x: tfr",dsp.pc));
	cur_inst_len = 0;
}

static void dsp_tst(void)
{
	D(bug("Dsp: 0x%04x: tst",dsp.pc));
	cur_inst_len = 0;
}
