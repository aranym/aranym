/*
 *	DSP56K emulation
 *	kernel
 *
 *	Patrice Mandin
 */

#include <SDL.h>
#include <SDL_thread.h>

#include "sysdeps.h"
#include "hardware.h"
#include "cpu_emulation.h"
#include "memory.h"
#include "dsp.h"
#include "dsp_cpu.h"
#ifdef DSP_DISASM
#include "dsp_disasm.h"
#endif

#define DEBUG 0
#include "debug.h"

/* More disasm infos, if wanted */
#define DSP_DISASM_INST 0		/* Instructions */
#define DSP_DISASM_REG 0		/* Registers changes */
#define DSP_DISASM_MEM 0		/* Memory changes */
#define DSP_DISASM_STATE 0		/* State changes */
#define DSP_DISASM_HOSTREAD 0	/* Host port read */
#define DSP_DISASM_HOSTWRITE 0	/* Host port write */
#define DSP_DISASM_INTER 0		/* Interrupts */

/* Prevent DSP from accessing non-present memory */
#define DSP_CHECK_MEM_ACCESS 1

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
static void dsp_postexecute_update_pc(void);
static void dsp_postexecute_interrupts(void);
/*
static void dsp_host2dsp(void);
static void dsp_dsp2host(void);
*/
static void dsp_ccr_extension(uint32 *reg0, uint32 *reg1, uint32 *reg2);
static void dsp_ccr_unnormalized(uint32 *reg0, uint32 *reg1, uint32 *reg2);
static void dsp_ccr_negative(uint32 *reg0, uint32 *reg1, uint32 *reg2);
static void dsp_ccr_zero(uint32 *reg0, uint32 *reg1, uint32 *reg2);

#if DSP_DISASM_MEM
static uint32 read_memory_disasm(int space, uint16 address);
#endif
static uint32 read_memory(int space, uint16 address);
static void write_memory(int space, uint32 address, uint32 value);

static void dsp_stack_push(uint32 curpc, uint32 cursr);
static void dsp_stack_pop(uint32 *curpc, uint32 *cursr);

static void opcode8h_0(void);
static void opcode8h_1(void);
static void opcode8h_4(void);
static void opcode8h_6(void);
static void opcode8h_8(void);
static void opcode8h_a(void);
static void opcode8h_b(void);

static void dsp_update_rn(uint32 numreg, int16 modifier);
static void dsp_update_rn_bitreverse(uint32 numreg);
static void dsp_update_rn_modulo(uint32 numreg, int16 modifier);
static int dsp_calc_ea(uint32 ea_mode, uint32 *dst_addr);
static int dsp_calc_cc(uint32 cc_code);

static void dsp_undefined(void);

/* Instructions without parallel moves */
static void dsp_andi(void);
static void dsp_bchg(void);
static void dsp_bclr(void);
static void dsp_bset(void);
static void dsp_btst(void);
static void dsp_div(void);
static void dsp_do(void);
static void dsp_enddo(void);
static void dsp_illegal(void);
static void dsp_jcc(void);
static void dsp_jclr(void);
static void dsp_jmp(void);
static void dsp_jscc(void);
static void dsp_jsclr(void);
static void dsp_jset(void);
static void dsp_jsr(void);
static void dsp_jsset(void);
static void dsp_lua(void);
static void dsp_movec(void);
static void dsp_movem(void);
static void dsp_movep(void);
static void dsp_nop(void);
static void dsp_norm(void);
static void dsp_ori(void);
static void dsp_rep(void);
static void dsp_reset(void);
static void dsp_rti(void);
static void dsp_rts(void);
static void dsp_stop(void);
static void dsp_swi(void);
static void dsp_tcc(void);
static void dsp_wait(void);

static void dsp_do_0(void);
static void dsp_do_2(void);
static void dsp_do_4(void);
static void dsp_do_c(void);
static void dsp_rep_1(void);
static void dsp_rep_3(void);
static void dsp_rep_5(void);
static void dsp_rep_d(void);
static void dsp_movec_7(void);
static void dsp_movec_9(void);
static void dsp_movec_b(void);
static void dsp_movec_d(void);
static void dsp_movep_0(void);
static void dsp_movep_1(void);
static void dsp_movep_2(void);

/* Parallel move analyzer */
static void dsp_parmove_read(void);
static void dsp_parmove_write(void);

static void dsp_pm_read_accu24(int numreg, uint32 *dest);
static void dsp_pm_writereg(int numreg, int position);

static void dsp_pm_0(void);
static void dsp_pm_1(void);
static void dsp_pm_2(void);
static void dsp_pm_2_2(void);
static void dsp_pm_3(void);
static void dsp_pm_4(void);
static void dsp_pm_4x(int immediat, uint32 l_addr);
static void dsp_pm_5(void);
static void dsp_pm_8(void);

/* 56bits arithmetic */
static uint16 dsp_abs56(uint32 *dest);
static uint16 dsp_asl56(uint32 *dest);
static uint16 dsp_asr56(uint32 *dest);
static uint16 dsp_add56(uint32 *source, uint32 *dest);
static uint16 dsp_sub56(uint32 *source, uint32 *dest);
static void dsp_mul56(uint32 source1, uint32 source2, uint32 *dest);
static void dsp_rnd56(uint32 *dest);

/* Instructions with parallel moves */
static void dsp_abs(void);
static void dsp_adc(void);
static void dsp_add(void);
static void dsp_addl(void);
static void dsp_addr(void);
static void dsp_and(void);
static void dsp_asl(void);
static void dsp_asr(void);
static void dsp_clr(void);
static void dsp_cmp(void);
static void dsp_cmpm(void);
static void dsp_eor(void);
static void dsp_lsl(void);
static void dsp_lsr(void);
static void dsp_mac(void);
static void dsp_macr(void);
static void dsp_move(void);
static void dsp_mpy(void);
static void dsp_mpyr(void);
static void dsp_neg(void);
static void dsp_not(void);
static void dsp_or(void);
static void dsp_rnd(void);
static void dsp_rol(void);
static void dsp_ror(void);
static void dsp_sbc(void);
static void dsp_sub(void);
static void dsp_subl(void);
static void dsp_subr(void);
static void dsp_tfr(void);
static void dsp_tst(void);

static void dsp_move_pm(void);

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
	dsp_move_pm,
	dsp_move_pm,
	dsp_move_pm,
	dsp_move_pm,

	dsp_movep,
	dsp_movep,
	dsp_movep,
	dsp_movep,

	dsp_move_pm,
	dsp_move_pm,
	dsp_move_pm,
	dsp_move_pm,

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
	{REG_A,REG_A},		/* A */
	{REG_B,REG_B},		/* B */
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

static int registers_mpy[8][2]={
	{REG_X0,REG_X0},
	{REG_Y0,REG_Y0},
	{REG_X1,REG_X0},
	{REG_Y1,REG_Y0},

	{REG_X0,REG_Y1},
	{REG_Y0,REG_X0},
	{REG_X1,REG_Y0},
	{REG_Y1,REG_X1}
};

static int registers_mask[64]={
	0, 0, 0, 0,
	24, 24, 24, 24,
	24, 24, 8, 8,
	24, 24, 24, 24,
	
	16, 16, 16, 16,
	16, 16, 16, 16,
	16, 16, 16, 16,
	16, 16, 16, 16,
	
	16, 16, 16, 16,
	16, 16, 16, 16,
	0, 0, 0, 0,
	0, 0, 0, 0,

	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 16, 8, 6,
	6, 6, 16, 16
};

/**********************************
 *	Emulator kernel
 **********************************/

int dsp56k_do_execute(void *)
{
	/* Wait upload of bootstrap code */
	SDL_SemWait(dsp.dsp56k_sem);

	for(;;) {
		dsp_execute_instruction();
	}

	return 0;
}

static void dsp_execute_instruction(void)
{
	uint32 value;

#ifdef DSP_DISASM
#if DSP_DISASM_REG
	dsp56k_disasm_reg_read();
#endif
#if DSP_DISASM_INST
	dsp56k_disasm();
#endif
#endif

	/* Decode and execute current instruction */
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

	/* Don't update the PC if we are halted */
	if (dsp.state == DSP_RUNNING) {
		dsp_postexecute_update_pc();
	}

	/* Interrupts ? */
	dsp_postexecute_interrupts();

#ifdef DSP_DISASM
#if DSP_DISASM_REG
	dsp56k_disasm_reg_compare();
#endif
#endif
}

/**********************************
 *	Update the PC
**********************************/

static void dsp_postexecute_update_pc(void)
{
	/* When running a REP, PC must stay on the current instruction */
	if (dsp.loop_rep) {
		/* Is PC on the instruction to repeat ? */		
		if (pc_on_rep==0) {
			--dsp.registers[REG_LC];
			dsp.registers[REG_LC] &= BITMASK(16);

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

	/* Normal execution, go to next instruction */
	if (cur_inst_len>0) {
		dsp.pc += cur_inst_len;
	}

	/* When running a DO loop, we test the end of loop with the */
	/* updated PC, pointing to last instruction of the loop */
	if (dsp.registers[REG_SR] & (1<<SR_LF)) {

		/* Did we execute the last instruction in loop ? */
		if (dsp.last_loop_inst) {		
			dsp.last_loop_inst = 0;

			--dsp.registers[REG_LC];
			dsp.registers[REG_LC] &= BITMASK(16);

			if (dsp.registers[REG_LC]==0) {
				/* end of loop */
				uint32 newpc;
				
				dsp_stack_pop(&newpc, &dsp.registers[REG_SR]);
				dsp_stack_pop(&dsp.registers[REG_LA], &dsp.registers[REG_LC]);
			} else {
				/* Loop one more time */
				dsp.pc = dsp.stack[0][dsp.registers[REG_SSH]];
			}
		}

		if (dsp.pc == dsp.registers[REG_LA]) {
			/* We are executing the last loop instruction */
			dsp.last_loop_inst = 1;
		}
	}
}

/**********************************
 *	Interrupts
**********************************/

static void dsp_postexecute_interrupts(void)
{
	uint32 ipl, ipl_hi;
	
	ipl = (dsp.registers[REG_SR]>>SR_I0) & BITMASK(2);
	ipl_hi = (dsp.periph[SPACE_X][DSP_IPR]>>10) & BITMASK(2);

	/* Trace, level 3 */
	if (dsp.registers[REG_SR] & (1<<SR_T)) {
		/* Raise interrupt p:0x0004 */
#if DSP_DISASM_INTER
		D(bug("Dsp: Interrupt: Trace"));
#endif
	}

	/* Host interface interrupts */
	if ((ipl_hi>0) && ((ipl_hi-1)>=ipl)) {

		/* Host transmit */
		if (
			(dsp.periph[SPACE_X][DSP_HOST_HCR] & (1<<DSP_HOST_HCR_HTIE)) &&
			((dsp.periph[SPACE_X][DSP_HOST_HSR] & (1<<DSP_HOST_HSR_HTDE))==0)
			) {
			/* Raise interrupt p:0x0022 */
#if DSP_DISASM_INTER
			D(bug("Dsp: Interrupt: Host transmit"));
#endif
		}

		/* Host receive */
		if (
			(dsp.periph[SPACE_X][DSP_HOST_HCR] & (1<<DSP_HOST_HCR_HRIE)) &&
			((dsp.periph[SPACE_X][DSP_HOST_HSR] & (1<<DSP_HOST_HSR_HRDF)))
			) {
			/* Raise interrupt p:0x0020 */
#if DSP_DISASM_INTER
			D(bug("Dsp: Interrupt: Host receive"));
#endif
		}

		/* Host command */
		if (
			(dsp.periph[SPACE_X][DSP_HOST_HCR] & (1<<DSP_HOST_HCR_HCIE)) &&
			(dsp.periph[SPACE_X][DSP_HOST_HSR] & (1<<DSP_HOST_HSR_HCP))
			) {
			/* Raise interrupt p:((hostport[CPU_HOST_CVR] & 31)<<1) */
#if DSP_DISASM_INTER
			D(bug("Dsp: Interrupt: Host command"));
#endif
		}
	}
}

/**********************************
 *	Set/clear ccr bits
 **********************************/

/* reg0 has bits 55..48 */
/* reg1 has bits 47..24 */
/* reg2 has bits 23..0 */

static void dsp_ccr_extension(uint32 *reg0, uint32 *reg1, uint32 *reg2)
{
	uint32 scaling, value, numbits;

	scaling = (dsp.registers[REG_SR]>>SR_S0) & BITMASK(2);
	value = (*reg0) & 0xff;
	numbits = 8;
	switch(scaling) {
		case 0:
			value <<=1;
			value |= ((*reg1)>>23) & 1;
			numbits=9;
			break;
		case 1:
			break;
		case 2:
			value <<=2;
			value |= ((*reg1)>>22) & 3;
			numbits=10;
			break;
		default:
			return;
			break;
	}

	dsp.registers[REG_SR] &= BITMASK(16)-(1<<SR_E);
	dsp.registers[REG_SR] |= ((value==0) || (value==(uint32)(BITMASK(numbits))))<<SR_E;
}

static void dsp_ccr_unnormalized(uint32 *reg0, uint32 *reg1, uint32 *reg2)
{
	uint32 scaling, value;

	scaling = (dsp.registers[REG_SR]>>SR_S0) & BITMASK(2);

	switch(scaling) {
		case 0:
			value = ((*reg1)>>22) & 3;
			break;
		case 1:
			value = ((*reg0)<<1) & 2;
			value |= ((*reg1)>>23) & 1;
			break;
		case 2:
			value = ((*reg1)>>21) & 3;
			break;
		default:
			return;
			break;
	}

	dsp.registers[REG_SR] &= BITMASK(16)-(1<<SR_U);
	dsp.registers[REG_SR] |= ((value==0) || (value==BITMASK(2)))<<SR_U;
}

static void dsp_ccr_negative(uint32 *reg0, uint32 *reg1, uint32 *reg2)
{
	dsp.registers[REG_SR] &= BITMASK(16)-(1<<SR_N);
	dsp.registers[REG_SR] |= (((*reg0)>>7) & 1)<<SR_N;
}

static void dsp_ccr_zero(uint32 *reg0, uint32 *reg1, uint32 *reg2)
{
	uint32 zeroed;

	zeroed=1;
	if (((*reg2) & BITMASK(24))!=0) {
		zeroed=0;
	} else if (((*reg1) & BITMASK(24))!=0) {
		zeroed=0;
	} else if (((*reg0) & BITMASK(8))!=0) {
		zeroed=0;
	}

	dsp.registers[REG_SR] &= BITMASK(16)-(1<<SR_Z);
	dsp.registers[REG_SR] |= zeroed<<SR_Z;
}

/**********************************
 *	Read/Write memory functions
 **********************************/

#if DSP_DISASM_MEM
static uint32 read_memory_disasm(int space, uint16 address)
{
	address &= BITMASK(16);

	switch(space) {
		case SPACE_X:
		case SPACE_Y:
			/* Internal RAM or ROM ? */
			if ((dsp.registers[REG_OMR] & (1<<OMR_DE)) &&
				(address>=0x100) && (address<0x200)) {
				return dsp.rom[space][address] & BITMASK(24);
			}
			
			/* Peripheral address ? */
			if (address >= 0xffc0) {
				return dsp.periph[space][address-0xffc0] & BITMASK(24);
			}

			/* Now continue with common code, no break here */
		case SPACE_P:
			if (address<=0x8000) {
				return dsp.ram[space][address] & BITMASK(24);
			}
			break;
	}

	return 0xdead;
}
#endif

static uint32 read_memory(int space, uint16 address)
{
	address &= BITMASK(16);

	switch(space) {
		case SPACE_X:
		case SPACE_Y:
			/* Internal RAM or ROM ? */
			if ((dsp.registers[REG_OMR] & (1<<OMR_DE)) &&
				(address>=0x100) && (address<0x200)) {
				return dsp.rom[space][address] & BITMASK(24);
			}
			
			/* Peripheral address ? */
			if (address >= 0xffc0) {

				if ((space==SPACE_X) && (address==0xffc0+DSP_HOST_HRX)) {
					int trdy;

					/* Read available data from host for the DSP */
					dsp.host2dsp();

					/* Clear HRDF bit to say that DSP has read */
					dsp.periph[SPACE_X][DSP_HOST_HSR] &= BITMASK(8)-(1<<DSP_HOST_HSR_HRDF);
#if DSP_DISASM_HOSTREAD
					D(bug("Dsp: (H->D): Dsp HRDF cleared"));
#endif
					/* Clear/set TRDY bit */
					dsp.hostport[CPU_HOST_ISR] &= 0xff-(1<<CPU_HOST_ISR_TRDY);
					trdy = (dsp.hostport[CPU_HOST_ISR]>>CPU_HOST_ISR_TXDE) & 1;
					trdy &= !((dsp.periph[SPACE_X][DSP_HOST_HSR]>>DSP_HOST_HSR_HRDF) & 1);
					dsp.hostport[CPU_HOST_ISR] |= (trdy & 1)<< CPU_HOST_ISR_TRDY;
				}

				return dsp.periph[space][address-0xffc0] & BITMASK(24);
			}

			/* Now continue with common code, no break here */
		case SPACE_P:
#if DSP_CHECK_MEM_ACCESS
			if (address<0x8000) {
#endif
				return dsp.ram[space][address] & BITMASK(24);
#if DSP_CHECK_MEM_ACCESS
			} else {
				D(bug("Dsp: Read at 0x%04x without mapped memory",address));
#if DSP_DISASM_STATE
				D(bug("Dsp: state = DSP_HALT"));
#endif
				dsp.state = DSP_HALT;

				SDL_SemWait(dsp.dsp56k_sem);

				return 0xdead;
#endif /* DSP_CHECK_MEM_ACCESS */
			}
			break;
	}

	return 0xdead;
}

static void write_memory(int space, uint32 address, uint32 value)
{
#ifdef DSP_DISASM
#if DSP_DISASM_MEM
	uint32 curvalue;
#endif
#endif

	address &= BITMASK(16);
	value &= BITMASK(24);

#ifdef DSP_DISASM
#if DSP_DISASM_MEM
	curvalue = read_memory_disasm(space, address);
#endif
#endif

	switch(space) {
		case SPACE_X:
			/* Internal RAM or ROM ? */
			if ((address >= 0x100) && (address <= 0x200)) {
				if (dsp.registers[REG_OMR] & (1<<OMR_DE)) {
					/* Can not write to rom */
					return;
				}
			}

			/* Peripheral space ? */
			if ((address >= 0xffc0) && (address <= 0xffff)) {
				switch(address-0xffc0) {
					case DSP_HOST_HTX:
						dsp.periph[space][DSP_HOST_HTX] = value;
						/* Clear HTDE bit to say that DSP has written */
						dsp.periph[SPACE_X][DSP_HOST_HSR] &= BITMASK(8)-(1<<DSP_HOST_HSR_HTDE);
#if DSP_DISASM_HOSTWRITE
						D(bug("Dsp: (D->H): Dsp HTDE cleared"));
#endif
						/* Write available data from DSP for the host */
						dsp.dsp2host();
						break;
					case DSP_HOST_HCR:
						dsp.periph[space][DSP_HOST_HCR] = value;
						/* Set HF3 and HF2 accordingly on the host side */
						dsp.hostport[CPU_HOST_ISR] &=
							BITMASK(8)-((1<<CPU_HOST_ISR_HF3)|(1<<CPU_HOST_ISR_HF2));
						dsp.hostport[CPU_HOST_ISR] |=
							dsp.periph[SPACE_X][DSP_HOST_HCR] & ((1<<CPU_HOST_ISR_HF3)|(1<<CPU_HOST_ISR_HF2));
						break;
					case DSP_HOST_HSR:
						/* Read only */
						break;
					default:
						dsp.periph[space][address-0xffc0] = value;
						break;
				}
			}

#if DSP_CHECK_MEM_ACCESS
			if ((address<0x8000) || (address>=0xffc0)) {
#endif
				dsp.ram[space][address] = value;
#if DSP_CHECK_MEM_ACCESS
			} else {
				D(bug("Dsp: Write at 0x%04x without mapped memory",address));
#if DSP_DISASM_STATE
				D(bug("Dsp: state = DSP_HALT"));
#endif
				dsp.state = DSP_HALT;
				SDL_SemWait(dsp.dsp56k_sem);
				return;
#endif
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

			if ((address >= 0xffc0) && (address <= 0xffff)) {
				dsp.periph[space][address-0xffc0] = value;
			}

#if DSP_CHECK_MEM_ACCESS
			if ((address<0x8000) || (address>=0xffc0)) {
#endif
				dsp.ram[space][address] = value;
#if DSP_CHECK_MEM_ACCESS
			} else {
				D(bug("Dsp: Write at 0x%04x without mapped memory",address));
#if DSP_DISASM_STATE
				D(bug("Dsp: state = DSP_HALT"));
#endif
				dsp.state = DSP_HALT;
				SDL_SemWait(dsp.dsp56k_sem);
				return;
#endif
			}
			/* y:0x0000-0x01ff is internal ram/rom */
			/* y:0x0200-0x3fff = p:0x0200-0x3fff */
			if ((address >= 0x200) && (address <= 0x3fff)) {
				dsp.ram[SPACE_P][address] = value;
			}
			break;
		case SPACE_P:
			dsp.ram[space][address] = value;
#if DSP_CHECK_MEM_ACCESS
			if (address>=0x8000) {
				D(bug("Dsp: Write at 0x%04x without mapped memory",address));
#if DSP_DISASM_STATE
				D(bug("Dsp: state = DSP_HALT"));
#endif
				dsp.state = DSP_HALT;
				SDL_SemWait(dsp.dsp56k_sem);
				return;
			}
#endif
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

#ifdef DSP_DISASM
#if DSP_DISASM_MEM
	switch(space) {
		case SPACE_P:
			fprintf(stderr,"Dsp: Mem: p:0x%04x:0x%06x -> 0x%06x\n", address, curvalue, read_memory_disasm(space, address));
			break;
		case SPACE_X:
			fprintf(stderr,"Dsp: Mem: x:0x%04x:0x%06x -> 0x%06x\n", address, curvalue, read_memory_disasm(space, address));
			break;
		case SPACE_Y:
			fprintf(stderr,"Dsp: Mem: y:0x%04x:0x%06x -> 0x%06x\n", address, curvalue, read_memory_disasm(space, address));
			break;
	}
#endif
#endif
}

/**********************************
 *	Stack push/pop
 **********************************/

static void dsp_stack_push(uint32 curpc, uint32 cursr)
{
	if (dsp.registers[REG_SP]==0x0f) {
		/* Stack full, raise interrupt */
		D(bug("Dsp: Interrupt: Stack error (overflow)"));
#if DSP_DISASM_STATE
		D(bug("Dsp: state = DSP_HALT"));
#endif
		dsp.state = DSP_HALT;
		SDL_SemWait(dsp.dsp56k_sem);
		return;
	}

	dsp.registers[REG_SP]++;
	dsp.registers[REG_SSH]++;
	dsp.registers[REG_SSL]++;

	dsp.stack[0][dsp.registers[REG_SSH]]=curpc;
	dsp.stack[1][dsp.registers[REG_SSL]]=cursr;
}

static void dsp_stack_pop(uint32 *newpc, uint32 *newsr)
{
	if (dsp.registers[REG_SP]==0x00) {
		/* Stack empty, raise interrupt */
		D(bug("Dsp: Interrupt: Stack error (underflow)"));
#if DSP_DISASM_STATE
		D(bug("Dsp: state = DSP_HALT"));
#endif
		dsp.state = DSP_HALT;
		SDL_SemWait(dsp.dsp56k_sem);
		return;
	}

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
	for (revbits=0;revbits<16;revbits++) {
		if (value & (1<<revbits)) {
			break;
		}
	}	
	revbits++;
		
	/* Reverse Rn bits */
	r_reg = dsp.registers[REG_R0+numreg];
	value = r_reg & (BITMASK(16)-BITMASK(revbits));
	for (i=0;i<revbits;i++) {
		if (r_reg & (1<<i)) {
			value |= 1<<(revbits-i-1);
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
			value |= 1<<(revbits-i-1);
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
	uint32 value, numreg, curreg;

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
			curreg = dsp.registers[REG_R0+numreg];
			dsp_update_rn(numreg, dsp.registers[REG_N0+numreg]);
			*dst_addr = dsp.registers[REG_R0+numreg];
			dsp.registers[REG_R0+numreg] = curreg;
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

#define CCR_BIT(x,y) \
	(((x) >> (y)) & 1)	

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
	uint16 value;	

	value = dsp.registers[REG_SR] & BITMASK(8);
	value |= (CCR_BIT(value,SR_N) ^ CCR_BIT(value, SR_V))<<SR_NV;
	value |= (CCR_BIT(value,SR_Z) | ((~CCR_BIT(value,SR_U)) & (~CCR_BIT(value,SR_E))))<<SR_ZUE;
	value |= (CCR_BIT(value,SR_Z) | CCR_BIT(value, SR_NV))<<SR_ZNV;
	
	return (uint32)(CCR_BIT(value,cc_code_map[cc_code & BITMASK(3)]))==((cc_code>>3) & 1);
}

/**********************************
 *	Highbyte opcodes dispatchers
 **********************************/

static void opcode8h_0(void)
{
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
		switch (cur_inst & 0xf8) {
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
			newcarry = (value>>numbit) & 1;
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
			newcarry = (value>>numbit) & 1;
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
			newcarry = (value>>numbit) & 1;
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
			if ((numreg==REG_A) || (numreg==REG_B)) {
				numreg = REG_A1+(numreg & 1);
			}
			value = dsp.registers[numreg];
			newcarry = (value>>numbit) & 1;
			if (newcarry) {
				value -= (1<<numbit);
			} else {
				value += (1<<numbit);
			}
			dsp.registers[numreg] = value;
			if (((numreg==REG_A) || (numreg==REG_B)) && (numbit==23)) {
				if (newcarry) {
					dsp.registers[REG_A2+(numreg & 1)]=0x00;
				} else  {
					dsp.registers[REG_A2+(numreg & 1)]=0xff;
				}
			}
			break;
	}

	/* Set carry */
	dsp.registers[REG_SR] &= BITMASK(16)-(1<<SR_C);
	dsp.registers[REG_SR] |= newcarry<<SR_C;
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
			newcarry = (value>>numbit) & 1;
			value &= 0xffffffff-(1<<numbit);
			write_memory(memspace, addr, value);
			break;
		case 1:
			/* bclr #n,x:ea */
			/* bclr #n,y:ea */
			dsp_calc_ea(value, &addr);
			value = read_memory(memspace, addr);
			newcarry = (value>>numbit) & 1;
			value &= 0xffffffff-(1<<numbit);
			write_memory(memspace, addr, value);
			break;
		case 2:
			/* bclr #n,x:pp */
			/* bclr #n,y:pp */
			addr = 0xffc0 + value;
			value = read_memory(memspace, addr);
			newcarry = (value>>numbit) & 1;
			value &= 0xffffffff-(1<<numbit);
			write_memory(memspace, addr, value);
			break;
		case 3:
			/* bclr #n,R */
			numreg = value;
			if ((numreg==REG_A) || (numreg==REG_B)) {
				numreg = REG_A1+(numreg & 1);
			}
			value = dsp.registers[numreg];
			newcarry = (value>>numbit) & 1;
			value &= 0xffffffff-(1<<numbit);
			dsp.registers[numreg] = value;
			if (((numreg==REG_A) || (numreg==REG_B)) && (numbit==23)) {
				dsp.registers[REG_A2+(numreg & 1)]=0x00;
			}
			break;
	}

	/* Set carry */
	dsp.registers[REG_SR] &= BITMASK(16)-(1<<SR_C);
	dsp.registers[REG_SR] |= newcarry<<SR_C;
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
			newcarry = (value>>numbit) & 1;
			value |= (1<<numbit);
			write_memory(memspace, addr, value);
			break;
		case 1:
			/* bset #n,x:ea */
			/* bset #n,y:ea */
			dsp_calc_ea(value, &addr);
			value = read_memory(memspace, addr);
			newcarry = (value>>numbit) & 1;
			value |= (1<<numbit);
			write_memory(memspace, addr, value);
			break;
		case 2:
			/* bset #n,x:pp */
			/* bset #n,y:pp */
			addr = 0xffc0 + value;
			value = read_memory(memspace, addr);
			newcarry = (value>>numbit) & 1;
			value |= (1<<numbit);
			write_memory(memspace, addr, value);
			break;
		case 3:
			/* bset #n,R */
			numreg = value;
			if ((numreg==REG_A) || (numreg==REG_B)) {
				numreg = REG_A1+(numreg & 1);
			}
			value = dsp.registers[numreg];
			newcarry = (value>>numbit) & 1;
			value |= (1<<numbit);
			dsp.registers[numreg] = value;
			if (((numreg==REG_A) || (numreg==REG_B)) && (numbit==23)) {
				dsp.registers[REG_A2+(numreg & 1)]=0xff;
			}
			break;
	}

	/* Set carry */
	dsp.registers[REG_SR] &= BITMASK(16)-(1<<SR_C);
	dsp.registers[REG_SR] |= newcarry<<SR_C;
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
			newcarry = (value>>numbit) & 1;
			break;
		case 1:
			/* btst #n,x:ea */
			/* btst #n,y:ea */
			dsp_calc_ea(value, &addr);
			value = read_memory(memspace, addr);
			newcarry = (value>>numbit) & 1;
			break;
		case 2:
			/* btst #n,x:pp */
			/* btst #n,y:pp */
			addr = 0xffc0 + value;
			value = read_memory(memspace, addr);
			newcarry = (value>>numbit) & 1;
			break;
		case 3:
			/* btst #n,R */
			numreg = value;
			if ((numreg==REG_A) || (numreg==REG_B)) {
				numreg = REG_A1+(numreg & 1);
			}
			value = dsp.registers[numreg];
			newcarry = (value>>numbit) & 1;
			break;
	}

	/* Set carry */
	dsp.registers[REG_SR] &= BITMASK(16)-(1<<SR_C);
	dsp.registers[REG_SR] |= newcarry<<SR_C;
}

static void dsp_div(void)
{
	uint32 srcreg, destreg, source, dest[3], newcarry, cursign;
	uint16 newsr;

	srcreg = REG_NULL;
	switch((cur_inst>>4) & BITMASK(2)) {
		case 0:	srcreg = REG_X0;	break;
		case 1:	srcreg = REG_Y0;	break;
		case 2:	srcreg = REG_X1;	break;
		case 3:	srcreg = REG_Y1;	break;
	}
	destreg = REG_A+((cur_inst>>3) & 1);

	source = dsp.registers[srcreg];
	dest[0] = dsp.registers[REG_A2+(destreg & 1)];
	dest[1] = dsp.registers[REG_A1+(destreg & 1)];
	dest[2] = dsp.registers[REG_A0+(destreg & 1)];
	newcarry = 0;

	newsr = dsp_asl56(dest);
	dest[2] |= (dsp.registers[REG_SR]>>SR_C) & 1;

	if (((dest[0]>>7) & 1) ^ ((source>>23) & 1)) {
		/* D += S */
		dest[1] += source;
		if ((dest[1]>>24) & BITMASK(8)) {
			cursign = (dest[0]>>7) & 1;

			++dest[0];
			dest[1] &= BITMASK(24);

			newcarry =(cursign != ((dest[0]>>7) & 1)) && (cursign==1);
		}
	} else {
		/* D -= S */
		dest[1] -= source;
		if ((dest[1]>>24) & BITMASK(8)) {
			cursign = (dest[0]>>7) & 1;

			--dest[0];
			dest[1] &= BITMASK(24);

			newcarry =(cursign != ((dest[0]>>7) & 1)) && (cursign==0);
		}
	}

	dsp.registers[REG_A2+(destreg & 1)] = dest[0];
	dsp.registers[REG_A1+(destreg & 1)] = dest[1];
	dsp.registers[REG_A0+(destreg & 1)] = dest[2];

	dsp.registers[REG_SR] &= BITMASK(16)-((1<<SR_C)|(1<<SR_V));
	dsp.registers[REG_SR] |= (newcarry<<SR_C);
	dsp.registers[REG_SR] |= newsr & (1<<SR_L);
	dsp.registers[REG_SR] |= newsr & (1<<SR_V);
}

static void dsp_do(void)
{
	uint32 value;

	dsp_stack_push(dsp.registers[REG_LA], dsp.registers[REG_LC]);

	dsp.registers[REG_LA] = read_memory(SPACE_P, dsp.pc+1) & BITMASK(16);
	cur_inst_len++;

	dsp_stack_push(dsp.pc+cur_inst_len, dsp.registers[REG_SR]);

	dsp.registers[REG_SR] |= (1<<SR_LF);

	value = (cur_inst>>12) & (BITMASK(2)<<2);
	value |= (cur_inst>>6) & 1<<1;
	value |= (cur_inst>>5) & 1;

	opcodes_do[value]();
}

static void dsp_do_0(void)
{
	uint32 memspace, addr;

	/* x:aa */
	/* y:aa */

	memspace = (cur_inst>>6) & 1;
	addr = (cur_inst>>8) & BITMASK(6);
	dsp.registers[REG_LC] = read_memory(memspace, addr) & BITMASK(16);
}

static void dsp_do_2(void)
{
	/* #xx */
	dsp.registers[REG_LC] = (cur_inst>>8) & BITMASK(8);
	dsp.registers[REG_LC] |= (cur_inst & BITMASK(4))<<8;
}

static void dsp_do_4(void)
{
	uint32 memspace, ea_mode, addr;

	/* x:ea */
	/* y:ea */

	memspace = (cur_inst>>6) & 1;
	ea_mode = (cur_inst>>8) & BITMASK(6);
	dsp_calc_ea(ea_mode, &addr);
	dsp.registers[REG_LC] = read_memory(memspace, addr) & BITMASK(16);
}

static void dsp_do_c(void)
{
	uint32 numreg;

	/* S */

	numreg = (cur_inst>>8) & BITMASK(6);
	if ((numreg == REG_A) || (numreg == REG_B)) {
		dsp_pm_read_accu24(numreg, &dsp.registers[REG_LC]); 
	} else {
		dsp.registers[REG_LC] = dsp.registers[numreg];
	}
	dsp.registers[REG_LC] &= BITMASK(16);
}

static void dsp_enddo(void)
{
	uint32 newpc;

	dsp_stack_pop(&newpc, &dsp.registers[REG_SR]);
	dsp.pc = dsp.registers[REG_LA];
	cur_inst_len = 0;

	dsp_stack_pop(&dsp.registers[REG_LA], &dsp.registers[REG_LC]);
}

static void dsp_illegal(void)
{
	/* Raise interrupt p:0x003e */
#if DSP_DISASM_INTER
	D(bug("Dsp: Interrupt: Illegal"));
#endif
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
		case 0x0e:
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
			addr = 0xffff0000; /* invalid address */
			memspace = 0xffff0000; /* invalid memspace */
			break;
	}

	cur_inst_len++;

	if ((value & (1<<numbit))==0) {
		newpc = read_memory(SPACE_P, dsp.pc+1);

		/* Polling loop ? */
		if (newpc == dsp.pc) {

			/* Are we waiting for host port ? */
			if ((memspace==SPACE_X) && (addr==0xffc0+DSP_HOST_HSR)) {
				/* Wait for host to write */
				if (numbit==DSP_HOST_HSR_HRDF) {
#if DSP_DISASM_STATE
					D(bug("Dsp: state = DSP_WAITHOSTWRITE"));
#endif
					dsp.state = DSP_WAITHOSTWRITE;
					SDL_SemWait(dsp.dsp56k_sem);
				}

				/* Wait for host to read */
				if (numbit==DSP_HOST_HSR_HTDE) {
#if DSP_DISASM_STATE
					D(bug("Dsp: state = DSP_WAITHOSTREAD"));
#endif
					dsp.state = DSP_WAITHOSTREAD;
					SDL_SemWait(dsp.dsp56k_sem);
				}
			}
		}

		dsp.pc = newpc;
		cur_inst_len = 0;
		return;
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

	/* Infinite loop ? */
	if (newpc == dsp.pc) {
#if DSP_DISASM_STATE
		D(bug("Dsp: state = DSP_HALT"));
#endif
		dsp.state = DSP_HALT;
		SDL_SemWait(dsp.dsp56k_sem);
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
		dsp_stack_push(dsp.pc+cur_inst_len, dsp.registers[REG_SR]);

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
		dsp_stack_push(dsp.pc+cur_inst_len+1, dsp.registers[REG_SR]);

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

	if (((cur_inst>>12) & BITMASK(4))==0) {
		newpc = cur_inst & BITMASK(12);
	} else {
		dsp_calc_ea((cur_inst>>8) & BITMASK(6),&newpc);
	}

	dsp_stack_push(dsp.pc+cur_inst_len, dsp.registers[REG_SR]);

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
		dsp_stack_push(dsp.pc+cur_inst_len+1, dsp.registers[REG_SR]);

		newpc = read_memory(SPACE_P, dsp.pc+1);
		dsp.pc = newpc;
		cur_inst_len = 0;
	} 
}

static void dsp_lua(void)
{
	uint32 value, srcreg, dstreg;
	
	dsp_calc_ea((cur_inst>>8) & BITMASK(5), &value);
	srcreg = (cur_inst>>8) & BITMASK(3);
	dstreg = cur_inst & BITMASK(3);
	
	if (cur_inst & (1<<3)) {
		dsp.registers[REG_N0+dstreg] = dsp.registers[REG_N0+srcreg];
	} else {
		dsp.registers[REG_R0+dstreg] = dsp.registers[REG_R0+srcreg];
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
	uint32 numreg1, numreg2, value;

	/* S1,D2 */
	/* S2,D1 */

	numreg2 = (cur_inst>>8) & BITMASK(6);
	numreg1 = (cur_inst & BITMASK(5))|0x20;

	if (cur_inst & (1<<15)) {
		/* Write D1 */

		if ((numreg2 == REG_A) || (numreg2 == REG_B)) {
			dsp_pm_read_accu24(numreg2, &dsp.registers[numreg1]); 
		} else {
			dsp.registers[numreg1] = dsp.registers[numreg2];
		}
		dsp.registers[numreg1] &= BITMASK(registers_mask[numreg1]);
	} else {
		/* Read S1 */

		if ((numreg2 == REG_A) || (numreg2 == REG_B)) {
			value = dsp.registers[numreg1];
			dsp.registers[REG_A2+(numreg2 & 1)] = 0;
			if (value & (1<<23)) {
				dsp.registers[REG_A2+(numreg2 & 1)] = 0xff;
			}
			dsp.registers[REG_A1+(numreg2 & 1)] = value & BITMASK(24);
			dsp.registers[REG_A0+(numreg2 & 1)] = 0;
		} else {
			dsp.registers[numreg2] = dsp.registers[numreg1];
			dsp.registers[numreg2] &= BITMASK(registers_mask[numreg2]);
		}
	}
}

static void dsp_movec_9(void)
{
	uint32 numreg, addr, memspace;

	/* x:aa,D1 */
	/* S1,x:aa */
	/* y:aa,D1 */
	/* S1,y:aa */

	numreg = (cur_inst & BITMASK(5))|0x20;
	addr = (cur_inst>>8) & BITMASK(6);
	memspace = (cur_inst>>6) & 1;

	if (cur_inst & (1<<15)) {
		/* Write D1 */

		dsp.registers[numreg] = read_memory(memspace, addr);
		dsp.registers[numreg] &= BITMASK(registers_mask[numreg]);
	} else {
		/* Read S1 */

		write_memory(memspace, addr, dsp.registers[numreg]);
	}
}

static void dsp_movec_b(void)
{
	uint32 numreg;

	/* #xx,D1 */

	numreg = (cur_inst & BITMASK(5))|0x20;
	dsp.registers[numreg] = (cur_inst>>8) & BITMASK(8);
}

static void dsp_movec_d(void)
{
	uint32 numreg, addr, memspace, ea_mode;
	int retour;

	/* x:ea,D1 */
	/* S1,x:ea */
	/* y:ea,D1 */
	/* S1,y:ea */
	/* #xxxx,D1 */

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
		dsp.registers[numreg] &= BITMASK(registers_mask[numreg]);
	} else {
		/* Read S1 */

		retour = dsp_calc_ea(ea_mode, &addr);

		write_memory(memspace, addr, dsp.registers[numreg]);
	}
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
			dsp.registers[REG_A1+(numreg & 1)] = value & BITMASK(24);
			dsp.registers[REG_A0+(numreg & 1)] = 0;
		} else {
			dsp.registers[numreg] = read_memory(SPACE_P, addr);
			dsp.registers[numreg] &= BITMASK(registers_mask[numreg]);
		}
	} else {
		/* Read S */

		if ((numreg == REG_A) || (numreg == REG_B)) {
			dsp_pm_read_accu24(numreg, &value); 
		} else {
			value = dsp.registers[numreg];
		}
		write_memory(SPACE_P, addr, value);
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
		/* Write pp */

		if ((numreg == REG_A) || (numreg == REG_B)) {
			dsp_pm_read_accu24(numreg, &value); 
		} else {
			value = dsp.registers[numreg];
		}
		write_memory(memspace, addr, value);
	} else {
		/* Read pp */

		value = read_memory(memspace, addr);

		if ((numreg == REG_A) || (numreg == REG_B)) {
			dsp.registers[REG_A2+(numreg & 1)] = 0;
			if (value & (1<<23)) {
				dsp.registers[REG_A2+(numreg & 1)] = 0xff;
			}
			dsp.registers[REG_A1+(numreg & 1)] = value & BITMASK(24);
			dsp.registers[REG_A0+(numreg & 1)] = 0;
		} else {
			dsp.registers[numreg] = value;
			dsp.registers[numreg] &= BITMASK(registers_mask[numreg]);
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
		/* Write pp */
		write_memory(memspace, xyaddr, read_memory(SPACE_P, paddr));
	} else {
		/* Read pp */
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
	/* y:pp,y:ea */
	/* y:pp,x:ea */

	uint32 addr, peraddr, easpace, perspace, ea_mode;
	int retour;

	peraddr = 0xffc0 + (cur_inst & BITMASK(6));
	perspace = (cur_inst>>16) & 1;
	
	ea_mode = (cur_inst>>8) & BITMASK(6);
	easpace = (cur_inst>>6) & 1;
	retour = dsp_calc_ea(ea_mode, &addr);

	if (cur_inst & (1<<15)) {
		/* Write pp */
		
		if (retour) {
			write_memory(perspace, peraddr, addr);
		} else {
			write_memory(perspace, peraddr, read_memory(easpace, addr));
		}
	} else {
		/* Read pp */

		write_memory(easpace, addr, read_memory(perspace, peraddr));
	}
}

static void dsp_norm(void)
{
	uint32 cursr,cur_e, cur_euz, dest[3], numreg, rreg;
	uint16 newsr;

	cursr = dsp.registers[REG_SR];
	cur_e = (cursr>>SR_E) & 1;	/* E */
	cur_euz = ~cur_e;			/* (not E) and U and (not Z) */
	cur_euz &= (cursr>>SR_U) & 1;
	cur_euz &= ~((cursr>>SR_Z) & 1);
	cur_euz &= 1;

	numreg = (cur_inst>>3) & 1;
	dest[0] = dsp.registers[REG_A2+numreg];
	dest[1] = dsp.registers[REG_A1+numreg];
	dest[2] = dsp.registers[REG_A0+numreg];
	rreg = REG_R0+((cur_inst>>8) & BITMASK(3));

	if (cur_euz) {
		newsr = dsp_asl56(dest);
		--dsp.registers[rreg];
		dsp.registers[rreg] &= BITMASK(16);
	} else if (cur_e) {
		newsr = dsp_asr56(dest);
		++dsp.registers[rreg];
		dsp.registers[rreg] &= BITMASK(16);
	} else {
		newsr = 0;
	}

	dsp.registers[REG_A2+numreg] = dest[0];
	dsp.registers[REG_A1+numreg] = dest[1];
	dsp.registers[REG_A0+numreg] = dest[2];

	dsp_ccr_extension(&dest[0], &dest[1], &dest[2]);
	dsp_ccr_unnormalized(&dest[0], &dest[1], &dest[2]);
	dsp_ccr_negative(&dest[0], &dest[1], &dest[2]);
	dsp_ccr_zero(&dest[0], &dest[1], &dest[2]);

	dsp.registers[REG_SR] &= BITMASK(16)-((1<<SR_V)|(1<<SR_C));
	dsp.registers[REG_SR] |= newsr;
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
		dsp_pm_read_accu24(numreg, &dsp.registers[REG_LC]); 
	} else {
		dsp.registers[REG_LC] = dsp.registers[numreg];
	}
	dsp.registers[REG_LC] &= BITMASK(16);
}

static void dsp_reset(void)
{
	/* Reset external peripherals */
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
	dsp.state = DSP_HALT;
	SDL_SemWait(dsp.dsp56k_sem);
}

static void dsp_swi(void)
{
	/* Raise interrupt p:0x0006 */
#if DSP_DISASM_INTER
	D(bug("Dsp: Interrupt: Swi"));
#endif
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
	dsp.state = DSP_HALT;
	SDL_SemWait(dsp.dsp56k_sem);
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
				write_memory(tmp_parmove_space[i], (uint32) dest, tmp_parmove_src[i][j]);
			} else {
				/* Write to register */
				*dest = tmp_parmove_src[i][j];
			}
		}
	}
}

static void dsp_pm_read_accu24(int numreg, uint32 *dest)
{
	uint32 scaling, value, numbits;

	/* Read an accumulator, stores it limited */

	scaling = (dsp.registers[REG_SR]>>SR_S0) & BITMASK(2);
	numreg &= 1;

	/* scaling==1 */
	value = dsp.registers[REG_A2+numreg] & 0xff;
	numbits = 8;

	switch(scaling) {
		case 0:
			value <<=1;
			value |= (dsp.registers[REG_A1+numreg]>>23) & 1;
			numbits=9;
			break;
		case 2:
			value <<=2;
			value |= (dsp.registers[REG_A1+numreg]>>22) & 3;
			numbits=10;
			break;
	}

	if ((value==0) || (value==(uint32)(BITMASK(numbits)))) {
		/* No limiting */
		*dest=dsp.registers[REG_A1+numreg];
	} else if (dsp.registers[REG_A2+numreg] & (1<<7)) {
		/* Limited to maximum negative value */
		*dest=0x00800000;
		dsp.registers[REG_SR] |= (1<<SR_L);
	} else {
		/* Limited to maximal positive value */
		*dest=0x007fffff;
		dsp.registers[REG_SR] |= (1<<SR_L);
	}	
}

static void dsp_pm_writereg(int numreg, int position)
{
	if ((numreg == REG_A) || (numreg == REG_B)) {
		tmp_parmove_dest[position][0]=&dsp.registers[REG_A2+(numreg & 1)];
		tmp_parmove_dest[position][1]=&dsp.registers[REG_A1+(numreg & 1)];
		tmp_parmove_dest[position][2]=&dsp.registers[REG_A0+(numreg & 1)];

		tmp_parmove_start[position]=0;
		tmp_parmove_len[position]=3;
	} else {
		tmp_parmove_dest[position][1]=&dsp.registers[numreg];

		tmp_parmove_start[position]=1;
		tmp_parmove_len[position]=1;
	}
}

static void dsp_pm_0(void)
{
	uint32 memspace, dummy, numreg, value;
/*
	0000 100d 00mm mrrr S,x:ea	x0,D
	0000 100d 10mm mrrr S,y:ea	y0,D
*/
	memspace = (cur_inst>>15) & 1;
	numreg = (cur_inst>>16) & 1;
	dsp_calc_ea((cur_inst>>8) & BITMASK(6), &dummy);

	/* [A|B] to [x|y]:ea */	
	dsp_pm_read_accu24(numreg, &tmp_parmove_src[0][1]);
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
	uint32 memspace, numreg, value, xy_addr, retour;
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

	memspace = (cur_inst>>14) & 1;
	numreg = REG_NULL;

	if (memspace) {
		/* Y: */
		switch((cur_inst>>16) & BITMASK(2)) {
			case 0:	numreg = REG_Y0;	break;
			case 1:	numreg = REG_Y1;	break;
			case 2:	numreg = REG_A;		break;
			case 3:	numreg = REG_B;		break;
		}
	} else {
		/* X: */
		switch((cur_inst>>18) & BITMASK(2)) {
			case 0:	numreg = REG_X0;	break;
			case 1:	numreg = REG_X1;	break;
			case 2:	numreg = REG_A;		break;
			case 3:	numreg = REG_B;		break;
		}
	}

	if (cur_inst & (1<<15)) {
		/* Write D1 */

		if (retour) {
			value = xy_addr;
		} else {
			value = read_memory(memspace, xy_addr);
		}
		tmp_parmove_src[0][0]= 0x000000;
		if (value & (1<<23)) {
			tmp_parmove_src[0][0]= 0x0000ff;
		}
		tmp_parmove_src[0][1]= value & BITMASK(registers_mask[numreg]);
		tmp_parmove_src[0][2]= 0x000000;

		dsp_pm_writereg(numreg, 0);
		tmp_parmove_type[0]=0;
	} else {
		/* Read S1 */

		if ((numreg==REG_A) || (numreg==REG_B)) {
			dsp_pm_read_accu24(numreg, &tmp_parmove_src[0][1]);
		} else {
			tmp_parmove_src[0][1]=dsp.registers[numreg];
		}

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
	dsp_pm_read_accu24(numreg, &tmp_parmove_src[1][1]);
	
	/* D2 */
	if (memspace) {
		/* Y: */
		numreg = REG_Y0 + ((cur_inst>>18) & 1);
	} else {
		/* X: */
		numreg = REG_X0 + ((cur_inst>>16) & 1);
	}	
	tmp_parmove_src[1][1] &= BITMASK(registers_mask[numreg]);
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
		dsp_calc_ea((cur_inst>>8) & BITMASK(5), &dummy);
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
	uint32 srcreg, dstreg;
	
	srcreg = (cur_inst >> 13) & BITMASK(5);
	dstreg = (cur_inst >> 8) & BITMASK(5);

	tmp_parmove_src[0][0]=
		tmp_parmove_src[0][1]=
		tmp_parmove_src[0][2]= 0x000000;

	if ((srcreg == REG_A) || (srcreg == REG_B)) {
		if ((dstreg == REG_A) || (dstreg == REG_B)) {
			/* Accu to accu: full 56 bits */
			tmp_parmove_src[0][0]=dsp.registers[REG_A2+(srcreg & 1)] & BITMASK(8);
			tmp_parmove_src[0][1]=dsp.registers[REG_A1+(srcreg & 1)] & BITMASK(24);
			tmp_parmove_src[0][2]=dsp.registers[REG_A0+(srcreg & 1)] & BITMASK(24);
		} else {
			/* Accu to register: limited 24 bits */
			dsp_pm_read_accu24(srcreg, &tmp_parmove_src[0][1]); 
			if (tmp_parmove_src[0][1] & (1<<23)) {
				tmp_parmove_src[0][0]=0x0000ff;
			}
		}
	} else {
		if ((dstreg == REG_A) || (dstreg == REG_B)) {
			/* Register to accu: sign extended to 56 bits */
			tmp_parmove_src[0][1]=dsp.registers[srcreg] & BITMASK(registers_mask[srcreg]);
			if (tmp_parmove_src[0][1] & (1<<23)) {
				tmp_parmove_src[0][0]=0x0000ff;
			}
		} else {
			/* Register to register: n bits */
			tmp_parmove_src[0][1]=dsp.registers[srcreg] & BITMASK(registers_mask[srcreg]);
		}
	}

	dsp_pm_writereg(dstreg, 0);
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
			srcvalue <<= 16;
			break;
	}

	tmp_parmove_src[0][0]=0x000000;
	if ((dest == REG_A) || (dest == REG_B)) {
		if (srcvalue & (1<<23)) {
			tmp_parmove_src[0][0]=0x0000ff;
		}
	}
	tmp_parmove_src[0][1]=srcvalue & BITMASK(registers_mask[dest]);
	tmp_parmove_src[0][2]=0x000000;

	dsp_pm_writereg(dest, 0);
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

static void dsp_pm_4x(int immediat, uint32 l_addr)
{
	uint32 value, numreg, numreg2;
/*
	0100 l0ll w0aa aaaa l:aa,D
						S,l:aa
	0100 l0ll w1mm mrrr l:ea,D
						S,l:ea
*/
	l_addr &= BITMASK(16);
	numreg = (cur_inst>>16) & BITMASK(2);
	numreg |= (cur_inst>>17) & (1<<2);

	if (cur_inst & (1<<15)) {
		/* Write D */

		/* S1 */
		value = read_memory(SPACE_X,l_addr);
		tmp_parmove_src[0][0] = 0x000000;
		if (value & (1<<23)) {
			tmp_parmove_src[0][0] = 0x0000ff;
		}
		tmp_parmove_src[0][1] = value & BITMASK(registers_mask[registers_lmove[numreg][0]]);
		tmp_parmove_src[0][2] = 0x000000;

		/* S2 */
		value = read_memory(SPACE_Y,l_addr);
		tmp_parmove_src[1][0] = 0x000000;
		if (value & (1<<23)) {
			tmp_parmove_src[1][0] = 0x0000ff;
		}
		tmp_parmove_src[1][1] = value & BITMASK(registers_mask[registers_lmove[numreg][1]]);
		tmp_parmove_src[1][2] = 0x000000;

		/* D1 */
		tmp_parmove_dest[0][0] = NULL;
		tmp_parmove_start[0]=1;
		tmp_parmove_len[0]=1;
		if (numreg >= 4) {
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
		if (numreg >= 6) {
			tmp_parmove_dest[0][2] = &dsp.registers[REG_A0+(numreg & 1)];
			tmp_parmove_start[0]=0;
			tmp_parmove_len[0]=3;
		}

		tmp_parmove_type[0]=0;

		/* D2 */
		tmp_parmove_dest[1][0] = NULL;
		tmp_parmove_start[1]=1;
		tmp_parmove_len[1]=1;
		if (numreg >= 4) {
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
		if (numreg >= 6) {
			tmp_parmove_dest[1][2] = &dsp.registers[REG_A0+(numreg & 1)];
			tmp_parmove_start[1]=0;
			tmp_parmove_len[1]=3;
		}
		tmp_parmove_len[0]=1;

		tmp_parmove_type[1]=0;
	} else {
		/* Read S */

		/* S1 */
		numreg2 = registers_lmove[numreg][0];
		if (numreg>=4) {
			/* A, B, AB, BA */
			dsp_pm_read_accu24(numreg2, &tmp_parmove_src[0][1]); 
		} else {
			tmp_parmove_src[0][1] = dsp.registers[numreg2];
		}
		
		/* S2 */
		numreg2 = registers_lmove[numreg][1];
		if (numreg>=4) {
			/* A, B, AB, BA */
			dsp_pm_read_accu24(numreg2, &tmp_parmove_src[1][1]); 
		} else {
			tmp_parmove_src[1][1] = dsp.registers[numreg2];
		}
		
		/* D1 */
		tmp_parmove_dest[0][1]=(uint32 *)l_addr;

		tmp_parmove_start[0]=1;
		tmp_parmove_len[0]=1;
		
		tmp_parmove_type[0]=1;
		tmp_parmove_space[0]=SPACE_X;

		/* D2 */
		tmp_parmove_dest[1][1]=(uint32 *)l_addr;

		tmp_parmove_start[1]=1;
		tmp_parmove_len[1]=1;

		tmp_parmove_type[1]=1;
		tmp_parmove_space[1]=SPACE_Y;
	}
}

static void dsp_pm_5(void)
{
	uint32 memspace, numreg, value, xy_addr, retour;
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

	memspace = (cur_inst>>19) & 1;
	numreg = (cur_inst>>16) & BITMASK(3);
	numreg |= (cur_inst>>17) & (BITMASK(2)<<3);

	if (cur_inst & (1<<15)) {
		/* Write D */

		if (retour) {
			value = xy_addr;
		} else {
			value = read_memory(memspace, xy_addr);
		}
		tmp_parmove_src[0][0]= 0x000000;
		if (value & (1<<23)) {
			tmp_parmove_src[0][0]= 0x0000ff;
		}
		tmp_parmove_src[0][1]= value & BITMASK(registers_mask[numreg]);
		tmp_parmove_src[0][2]= 0x000000;

		dsp_pm_writereg(numreg, 0);
		tmp_parmove_type[0]=0;
	} else {
		/* Read S */

		if ((numreg==REG_A) || (numreg==REG_B)) {
			dsp_pm_read_accu24(numreg, &tmp_parmove_src[0][1]);
		} else {
			tmp_parmove_src[0][1]=dsp.registers[numreg];
		}

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
	if ((ea1 & (1<<2))==0) {
		ea2 |= 1<<2;
	}
	if ((ea2>>3) == 0) {
		ea2 |= (1<<5);
	}

	dsp_calc_ea(ea1, &dummy1);
	dsp_calc_ea(ea2, &dummy2);

	switch((cur_inst>>18) & BITMASK(2)) {
		case 0:	numreg1=REG_X0;	break;
		case 1:	numreg1=REG_X1;	break;
		case 2:	numreg1=REG_A;	break;
		case 3:	numreg1=REG_B;	break;
	}
	switch((cur_inst>>16) & BITMASK(2)) {
		case 0:	numreg2=REG_Y0;	break;
		case 1:	numreg2=REG_Y1;	break;
		case 2:	numreg2=REG_A;	break;
		case 3:	numreg2=REG_B;	break;
	}
	
	if (cur_inst & (1<<15)) {
		/* Write D1 */

		value = read_memory(SPACE_X, dummy1);
		tmp_parmove_src[0][0]= 0x000000;
		if (value & (1<<23)) {
			tmp_parmove_src[0][0]= 0x0000ff;
		}
		tmp_parmove_src[0][1]= value & BITMASK(registers_mask[numreg1]);
		tmp_parmove_src[0][2]= 0x000000;

		dsp_pm_writereg(numreg1, 0);
		tmp_parmove_type[0]=0;
	} else {
		/* Read S1 */

		if ((numreg1==REG_A) || (numreg1==REG_B)) {
			dsp_pm_read_accu24(numreg1, &tmp_parmove_src[0][1]);
		} else {
			tmp_parmove_src[0][1]=dsp.registers[numreg1];
		}

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
		tmp_parmove_src[1][1]= value & BITMASK(registers_mask[numreg2]);
		tmp_parmove_src[1][2]= 0x000000;

		dsp_pm_writereg(numreg2, 1);
		tmp_parmove_type[1]=0;
	} else {
		/* Read S2 */
		if ((numreg1==REG_A) || (numreg1==REG_B)) {
			dsp_pm_read_accu24(numreg1, &tmp_parmove_src[1][1]);
		} else {
			tmp_parmove_src[1][1]=dsp.registers[numreg1];
		}

		tmp_parmove_dest[1][1]=(uint32 *)dummy2;

		tmp_parmove_start[1]=1;
		tmp_parmove_len[1]=1;

		tmp_parmove_type[1]=1;
		tmp_parmove_space[1]=SPACE_Y;
	}
}

/**********************************
 *	56bit arithmetic
 **********************************/

/* source,dest[0] is 55:48 */
/* source,dest[1] is 47:24 */
/* source,dest[2] is 23:00 */

static uint16 dsp_abs56(uint32 *dest)
{
	uint32 zerodest[3];
	uint16 newsr;

	/* D=|D| */

	if (dest[0] & (1<<7)) {
		zerodest[0] = zerodest[1] = zerodest[2] = 0;

		newsr = dsp_sub56(dest, zerodest);

		dest[0] = zerodest[0];
		dest[1] = zerodest[1];
		dest[2] = zerodest[2];
	} else {
		newsr = 0;
	}

	return newsr;
}

static uint16 dsp_asl56(uint32 *dest)
{
	uint16 overflow, carry;

	/* Shift left dest 1 bit: D<<=1 */

	carry = (dest[0]>>7) & 1;

	dest[0] <<= 1;
	dest[0] |= (dest[1]>>23) & 1;
	dest[0] &= BITMASK(8);

	dest[1] <<= 1;
	dest[1] |= (dest[2]>>23) & 1;
	dest[1] &= BITMASK(24);
	
	dest[2] <<= 1;
	dest[2] &= BITMASK(24);

	overflow = (carry != ((dest[0]>>7) & 1));

	return (overflow<<SR_L)|(overflow<<SR_V)|(carry<<SR_C);
}

static uint16 dsp_asr56(uint32 *dest)
{
	uint16 carry;

	/* Shift right dest 1 bit: D>>=1 */

	carry = dest[2] & 1;

	dest[2] >>= 1;
	dest[2] &= BITMASK(23);
	dest[2] |= (dest[1] & 1)<<23;

	dest[1] >>= 1;
	dest[1] &= BITMASK(23);
	dest[1] |= (dest[0] & 1)<<23;

	dest[0] >>= 1;
	dest[0] &= BITMASK(7);
	dest[0] |= (dest[0] & (1<<6))<<1;

	return (carry<<SR_C);
}

static uint16 dsp_add56(uint32 *source, uint32 *dest)
{
	uint16 overflow, carry;
	uint32 src;

	/* Add source to dest: D = D+S */
	dest[2] &= BITMASK(24);
	dest[1] &= BITMASK(24);
	dest[0] &= BITMASK(8);

	carry = (dest[0]>>7) & 1;

	if (dest[0] & (1<<7)) {
		dest[0] |= 0xffffff00;
	}

	dest[2] += source[2] & BITMASK(24);

	dest[1] += source[1] & BITMASK(24);
	if ((dest[2]>>24) & BITMASK(8)) {
		dest[1]++;
	}

	src = source[0] & BITMASK(8);
	if (src & (1<<7)) {
		src |= 0xffffff00;
	}
	dest[0] += src;
	if ((dest[1]>>24) & BITMASK(8)) {
		dest[0]++;
	}

	/* overflow if we go below -256.0 or above +256.0 */
	overflow = (((dest[0] & 0xffffff00)!=0) && ((dest[0] & 0xffffff00)!=0xffffff00));

	if (overflow) {
		carry = 1;
	} else {
		if (carry) {
			/* Carry set if we go from negative to positive value */
			carry = ( ((dest[0]>>7) & 1)==0);
		} else {
			/* Carry set if we go from positive to negative value */
			carry = ( ((dest[0]>>7) & 1)==1);
		}
	}

	dest[2] &= BITMASK(24);
	dest[1] &= BITMASK(24);
	dest[0] &= BITMASK(8);

	return (overflow<<SR_L)|(overflow<<SR_V)|(carry<<SR_C);
}

static uint16 dsp_sub56(uint32 *source, uint32 *dest)
{
	uint16 overflow, carry;
	uint32 src;

	/* Substract source from dest: D = D-S */

	dest[2] &= BITMASK(24);
	dest[1] &= BITMASK(24);
	dest[0] &= BITMASK(8);

	carry = (dest[0]>>7) & 1;

	if (dest[0] & (1<<7)) {
		dest[0] |= 0xffffff00;
	}

	dest[2] -= source[2] & BITMASK(24);

	dest[1] -= source[1] & BITMASK(24);
	if ((dest[2]>>24) & BITMASK(8)) {
		dest[1]--;
	}

	src = source[0] & BITMASK(8);
	if (src & (1<<7)) {
		src |= 0xffffff00;
	}
	dest[0] -= src;
	if ((dest[1]>>24) & BITMASK(8)) {
		dest[0]--;
	}

	/* overflow if we go below -256.0 or above +256.0 */
	overflow = (((dest[0] & 0xffffff00)!=0) && ((dest[0] & 0xffffff00)!=0xffffff00));

	if (overflow) {
		carry = 1;
	} else {
		if (carry) {
			/* Carry set if we go from negative to positive value */
			carry = ( ((dest[0]>>7) & 1)==0);
		} else {
			/* Carry set if we go from positive to negative value */
			carry = ( ((dest[0]>>7) & 1)==1);
		}
	}

	dest[2] &= BITMASK(24);
	dest[1] &= BITMASK(24);
	dest[0] &= BITMASK(8);

	return (overflow<<SR_L)|(overflow<<SR_V)|(carry<<SR_C);
}

static void dsp_mul56(uint32 source1, uint32 source2, uint32 *dest)
{
	uint32 negresult;	/* Negate the result ? */
	uint32 part[4], zerodest[3], value;

	/* Multiply: D = S1*S2 */
	negresult = 0;
	if (source1 & (1<<23)) {
		negresult ^= 1;
		source1 = (1<<24) - (source1 & BITMASK(24));
	}
	if (source2 & (1<<23)) {
		negresult ^= 1;
		source2 = (1<<24) - (source2 & BITMASK(24));
	}

	/* bits 0-11 * bits 0-11 */
	part[0]=(source1 & BITMASK(12))*(source2 & BITMASK(12));
	/* bits 12-23 * bits 0-11 */
	part[1]=((source1>>12) & BITMASK(12))*(source2 & BITMASK(12));
	/* bits 0-11 * bits 12-23 */
	part[2]=(source1 & BITMASK(12))*((source2>>12)  & BITMASK(12));
	/* bits 12-23 * bits 12-23 */
	part[3]=((source1>>12) & BITMASK(12))*((source2>>12) & BITMASK(12));

	/* Calc dest 2 */
	dest[2] = part[0];
	dest[2] += (part[1] & BITMASK(12)) << 12;
	dest[2] += (part[2] & BITMASK(12)) << 12;

	/* Calc dest 1 */
	dest[1] = (part[1]>>12) & BITMASK(12);
	dest[1] += (part[2]>>12) & BITMASK(12);
	dest[1] += part[3];

	/* Calc dest 0 */
	dest[0] = 0;

	/* Add carries */
	value = (dest[2]>>24) & BITMASK(8);
	if (value) {
		dest[1] += value;
		dest[2] &= BITMASK(24);
	}
	value = (dest[1]>>24) & BITMASK(8);
	if (value) {
		dest[0] += value;
		dest[1] &= BITMASK(24);
	}

	/* Get rid of extra sign bit */
	dsp_asl56(dest);

	if (negresult) {
		zerodest[0] = zerodest[1] = zerodest[2] = 0;

		dsp_sub56(dest, zerodest);

		dest[0] = zerodest[0];
		dest[1] = zerodest[1];
		dest[2] = zerodest[2];
	}
}

static void dsp_rnd56(uint32 *dest)
{
	uint32 value;

	/* Round D */

	value = dest[2] & BITMASK(24);
	if (value==0x800000) {
		if (dest[1] & 1) {
			++dest[1];
			if ((dest[1]>>24) & BITMASK(8)) {
				++dest[0];
				dest[0] &= BITMASK(8);
				dest[1] &= BITMASK(24);
			}
		}
	} else if (value>0x800000) {
		++dest[1];
		if ((dest[1]>>24) & BITMASK(8)) {
			++dest[0];
			dest[0] &= BITMASK(8);
			dest[1] &= BITMASK(24);
		}
	}

	dest[2]=0;
}

/**********************************
 *	Parallel moves instructions
 **********************************/

static void dsp_abs(void)
{
	uint32 numreg, dest[3], overflowed;

	numreg = (cur_inst>>3) & 1;

	dest[0] = dsp.registers[REG_A2+numreg];
	dest[1] = dsp.registers[REG_A1+numreg];
	dest[2] = dsp.registers[REG_A0+numreg];

	overflowed = ((dest[2]==0) && (dest[1]==0) && (dest[0]==0x80));

	dsp_abs56(dest);

	dsp.registers[REG_A2+numreg] = dest[0];
	dsp.registers[REG_A1+numreg] = dest[1];
	dsp.registers[REG_A0+numreg] = dest[2];

	dsp.registers[REG_SR] &= BITMASK(16)-(1<<SR_V);
	dsp.registers[REG_SR] |= (overflowed<<SR_L)|(overflowed<<SR_V);

	dsp_ccr_extension(&dest[0], &dest[1], &dest[2]);
	dsp_ccr_unnormalized(&dest[0], &dest[1], &dest[2]);
	dsp_ccr_negative(&dest[0], &dest[1], &dest[2]);
	dsp_ccr_zero(&dest[0], &dest[1], &dest[2]);
}

static void dsp_adc(void)
{
	uint32 srcreg, destreg, source[3], dest[3], curcarry;
	uint16 newsr;

	curcarry = (dsp.registers[REG_SR]>>SR_C) & 1;

	destreg = (cur_inst>>3) & 1;
	dest[0] = dsp.registers[REG_A2+destreg];
	dest[1] = dsp.registers[REG_A1+destreg];
	dest[2] = dsp.registers[REG_A0+destreg];

	srcreg = (cur_inst>>4) & 1;
	switch(srcreg) {
		case 0:	/* X */
			source[1] = dsp.registers[REG_X1];
			source[2] = dsp.registers[REG_X0];
			source[0] = 0;
			if (source[1] & (1<<23)) {
				source[0] = 0x0000ff;
			}
			break;
		case 1:	/* Y */
			source[1] = dsp.registers[REG_Y1];
			source[2] = dsp.registers[REG_Y0];
			source[0] = 0;
			if (source[1] & (1<<23)) {
				source[0] = 0x0000ff;
			}
			break;
	}

	newsr = dsp_add56(source, dest);
	
	if (curcarry) {
		source[0]=0;
		source[1]=0;
		source[2]=1;
		newsr |= dsp_add56(source, dest);
	}

	dsp.registers[REG_A2+destreg] = dest[0];
	dsp.registers[REG_A1+destreg] = dest[1];
	dsp.registers[REG_A0+destreg] = dest[2];

	dsp_ccr_extension(&dest[0], &dest[1], &dest[2]);
	dsp_ccr_unnormalized(&dest[0], &dest[1], &dest[2]);
	dsp_ccr_negative(&dest[0], &dest[1], &dest[2]);
	dsp_ccr_zero(&dest[0], &dest[1], &dest[2]);

	dsp.registers[REG_SR] &= BITMASK(16)-((1<<SR_V)|(1<<SR_C));
	dsp.registers[REG_SR] |= newsr;
}

static void dsp_add(void)
{
	uint32 srcreg, destreg, source[3], dest[3];
	uint16 newsr;

	destreg = (cur_inst>>3) & 1;
	dest[0] = dsp.registers[REG_A2+destreg];
	dest[1] = dsp.registers[REG_A1+destreg];
	dest[2] = dsp.registers[REG_A0+destreg];

	srcreg = (cur_inst>>4) & BITMASK(3);
	switch(srcreg) {
		case 1:	/* A or B */
			srcreg = destreg ^ 1;
			source[0] = dsp.registers[REG_A2+srcreg];
			source[1] = dsp.registers[REG_A1+srcreg];
			source[2] = dsp.registers[REG_A0+srcreg];
			break;
		case 2:	/* X */
			source[1] = dsp.registers[REG_X1];
			source[2] = dsp.registers[REG_X0];
			source[0] = 0;
			if (source[1] & (1<<23)) {
				source[0] = 0x0000ff;
			}
			break;
		case 3:	/* Y */
			source[1] = dsp.registers[REG_Y1];
			source[2] = dsp.registers[REG_Y0];
			source[0] = 0;
			if (source[1] & (1<<23)) {
				source[0] = 0x0000ff;
			}
			break;
		case 4:	/* X0 */
			source[2] = 0;
			source[1] = dsp.registers[REG_X0];
			source[0] = 0;
			if (source[1] & (1<<23)) {
				source[0] = 0x0000ff;
			}
			break;
		case 5:	/* Y0 */
			source[2] = 0;
			source[1] = dsp.registers[REG_Y0];
			source[0] = 0;
			if (source[1] & (1<<23)) {
				source[0] = 0x0000ff;
			}
			break;
		case 6:	/* X1 */
			source[2] = 0;
			source[1] = dsp.registers[REG_X1];
			source[0] = 0;
			if (source[1] & (1<<23)) {
				source[0] = 0x0000ff;
			}
			break;
		case 7:	/* Y1 */
			source[2] = 0;
			source[1] = dsp.registers[REG_Y1];
			source[0] = 0;
			if (source[1] & (1<<23)) {
				source[0] = 0x0000ff;
			}
			break;
	}

	newsr = dsp_add56(source, dest);

	dsp.registers[REG_A2+destreg] = dest[0];
	dsp.registers[REG_A1+destreg] = dest[1];
	dsp.registers[REG_A0+destreg] = dest[2];

	dsp_ccr_extension(&dest[0], &dest[1], &dest[2]);
	dsp_ccr_unnormalized(&dest[0], &dest[1], &dest[2]);
	dsp_ccr_negative(&dest[0], &dest[1], &dest[2]);
	dsp_ccr_zero(&dest[0], &dest[1], &dest[2]);

	dsp.registers[REG_SR] &= BITMASK(16)-((1<<SR_V)|(1<<SR_C));
	dsp.registers[REG_SR] |= newsr;
}

static void dsp_addl(void)
{
	uint32 numreg, source[3], dest[3];
	uint16 newsr;

	numreg = (cur_inst>>3) & 1;

	dest[0] = dsp.registers[REG_A2+numreg];
	dest[1] = dsp.registers[REG_A1+numreg];
	dest[2] = dsp.registers[REG_A0+numreg];
	newsr = dsp_asl56(dest);

	source[0] = dsp.registers[REG_A2+(numreg ^ 1)];
	source[1] = dsp.registers[REG_A1+(numreg ^ 1)];
	source[2] = dsp.registers[REG_A0+(numreg ^ 1)];
	newsr |= dsp_add56(source, dest);

	dsp.registers[REG_A2+numreg] = dest[0];
	dsp.registers[REG_A1+numreg] = dest[1];
	dsp.registers[REG_A0+numreg] = dest[2];

	dsp_ccr_extension(&dest[0], &dest[1], &dest[2]);
	dsp_ccr_unnormalized(&dest[0], &dest[1], &dest[2]);
	dsp_ccr_negative(&dest[0], &dest[1], &dest[2]);
	dsp_ccr_zero(&dest[0], &dest[1], &dest[2]);

	dsp.registers[REG_SR] &= BITMASK(16)-((1<<SR_V)|(1<<SR_C));
	dsp.registers[REG_SR] |= newsr;
}

static void dsp_addr(void)
{
	uint32 numreg, source[3], dest[3];
	uint16 newsr;

	numreg = (cur_inst>>3) & 1;

	dest[0] = dsp.registers[REG_A2+numreg];
	dest[1] = dsp.registers[REG_A1+numreg];
	dest[2] = dsp.registers[REG_A0+numreg];
	newsr = dsp_asr56(dest);

	source[0] = dsp.registers[REG_A2+(numreg ^ 1)];
	source[1] = dsp.registers[REG_A1+(numreg ^ 1)];
	source[2] = dsp.registers[REG_A0+(numreg ^ 1)];
	newsr |= dsp_add56(source, dest);

	dsp.registers[REG_A2+numreg] = dest[0];
	dsp.registers[REG_A1+numreg] = dest[1];
	dsp.registers[REG_A0+numreg] = dest[2];

	dsp_ccr_extension(&dest[0], &dest[1], &dest[2]);
	dsp_ccr_unnormalized(&dest[0], &dest[1], &dest[2]);
	dsp_ccr_negative(&dest[0], &dest[1], &dest[2]);
	dsp_ccr_zero(&dest[0], &dest[1], &dest[2]);

	dsp.registers[REG_SR] &= BITMASK(16)-((1<<SR_V)|(1<<SR_C));
	dsp.registers[REG_SR] |= newsr;
}

static void dsp_and(void)
{
	uint32 srcreg, dstreg;

	srcreg = REG_X0+((cur_inst>>4) & BITMASK(2));
	dstreg = REG_A1+((cur_inst>>3) & 1);

	dsp.registers[dstreg] &= dsp.registers[srcreg];
	dsp.registers[dstreg] &= BITMASK(24); /* FIXME: useless ? */

	dsp.registers[REG_SR] &= BITMASK(16)-((1<<SR_N)|(1<<SR_Z)|(1<<SR_V));
	dsp.registers[REG_SR] |= ((dsp.registers[dstreg]>>23) & 1)<<SR_N;
	dsp.registers[REG_SR] |= (dsp.registers[dstreg]==0)<<SR_Z;
}

static void dsp_asl(void)
{
	uint32 numreg, dest[3];
	uint16 newsr;

	numreg = (cur_inst>>3) & 1;

	dest[0] = dsp.registers[REG_A2+numreg];
	dest[1] = dsp.registers[REG_A1+numreg];
	dest[2] = dsp.registers[REG_A0+numreg];

	newsr = dsp_asl56(dest);

	dsp.registers[REG_A2+numreg] = dest[0];
	dsp.registers[REG_A1+numreg] = dest[1];
	dsp.registers[REG_A0+numreg] = dest[2];

	dsp.registers[REG_SR] &= BITMASK(16)-((1<<SR_C)|(1<<SR_V));
	dsp.registers[REG_SR] |= newsr;

	dsp_ccr_extension(&dest[0], &dest[1], &dest[2]);
	dsp_ccr_unnormalized(&dest[0], &dest[1], &dest[2]);
	dsp_ccr_negative(&dest[0], &dest[1], &dest[2]);
	dsp_ccr_zero(&dest[0], &dest[1], &dest[2]);
}

static void dsp_asr(void)
{
	uint32 numreg, newsr, dest[3];

	numreg = (cur_inst>>3) & 1;

	dest[0] = dsp.registers[REG_A2+numreg];
	dest[1] = dsp.registers[REG_A1+numreg];
	dest[2] = dsp.registers[REG_A0+numreg];

	newsr = dsp_asr56(dest);

	dsp.registers[REG_A2+numreg] = dest[0];
	dsp.registers[REG_A1+numreg] = dest[1];
	dsp.registers[REG_A0+numreg] = dest[2];

	dsp.registers[REG_SR] &= BITMASK(16)-((1<<SR_C)|(1<<SR_V));
	dsp.registers[REG_SR] |= newsr;

	dsp_ccr_extension(&dest[0], &dest[1], &dest[2]);
	dsp_ccr_unnormalized(&dest[0], &dest[1], &dest[2]);
	dsp_ccr_negative(&dest[0], &dest[1], &dest[2]);
	dsp_ccr_zero(&dest[0], &dest[1], &dest[2]);
}

static void dsp_clr(void)
{
	uint32 numreg;

	numreg = (cur_inst>>3) & 1;

	dsp.registers[REG_A2+numreg]=0;
	dsp.registers[REG_A1+numreg]=0;
	dsp.registers[REG_A0+numreg]=0;

	dsp.registers[REG_SR] &= BITMASK(16)-((1<<SR_E)|(1<<SR_N)|(1<<SR_V));
	dsp.registers[REG_SR] |= (1<<SR_U)|(1<<SR_Z);
}

static void dsp_cmp(void)
{
	uint32 srcreg, destreg, source[3], dest[3];
	uint16 newsr;

	destreg = (cur_inst>>3) & 1;
	dest[0] = dsp.registers[REG_A2+destreg];
	dest[1] = dsp.registers[REG_A1+destreg];
	dest[2] = dsp.registers[REG_A0+destreg];

	srcreg = (cur_inst>>4) & BITMASK(3);
	switch(srcreg) {
		case 0:	/* A or B */
			srcreg = destreg ^ 1;
			source[0] = dsp.registers[REG_A2+srcreg];
			source[1] = dsp.registers[REG_A1+srcreg];
			source[2] = dsp.registers[REG_A0+srcreg];
			break;
		case 4:	/* X0 */
			source[2] = 0;
			source[1] = dsp.registers[REG_X0];
			source[0] = 0;
			if (source[1] & (1<<23)) {
				source[0] = 0x0000ff;
			}
			break;
		case 5:	/* Y0 */
			source[2] = 0;
			source[1] = dsp.registers[REG_Y0];
			source[0] = 0;
			if (source[1] & (1<<23)) {
				source[0] = 0x0000ff;
			}
			break;
		case 6:	/* X1 */
			source[2] = 0;
			source[1] = dsp.registers[REG_X1];
			source[0] = 0;
			if (source[1] & (1<<23)) {
				source[0] = 0x0000ff;
			}
			break;
		case 7:	/* Y1 */
			source[2] = 0;
			source[1] = dsp.registers[REG_Y1];
			source[0] = 0;
			if (source[1] & (1<<23)) {
				source[0] = 0x0000ff;
			}
			break;
	}

	newsr = dsp_sub56(source, dest);

	dsp_ccr_extension(&dest[0], &dest[1], &dest[2]);
	dsp_ccr_unnormalized(&dest[0], &dest[1], &dest[2]);
	dsp_ccr_negative(&dest[0], &dest[1], &dest[2]);
	dsp_ccr_zero(&dest[0], &dest[1], &dest[2]);

	dsp.registers[REG_SR] &= BITMASK(16)-((1<<SR_V)|(1<<SR_C));
	dsp.registers[REG_SR] |= newsr;
}

static void dsp_cmpm(void)
{
	uint32 srcreg, destreg, source[3], dest[3];
	uint16 newsr;

	destreg = (cur_inst>>3) & 1;
	dest[0] = dsp.registers[REG_A2+destreg];
	dest[1] = dsp.registers[REG_A1+destreg];
	dest[2] = dsp.registers[REG_A0+destreg];
	dsp_abs56(dest);

	srcreg = (cur_inst>>4) & BITMASK(3);
	switch(srcreg) {
		case 0:	/* A or B */
			srcreg = destreg ^ 1;
			source[0] = dsp.registers[REG_A2+srcreg];
			source[1] = dsp.registers[REG_A1+srcreg];
			source[2] = dsp.registers[REG_A0+srcreg];
			break;
		case 4:	/* X0 */
			source[2] = 0;
			source[1] = dsp.registers[REG_X0];
			source[0] = 0;
			if (source[1] & (1<<23)) {
				source[0] = 0x0000ff;
			}
			break;
		case 5:	/* Y0 */
			source[2] = 0;
			source[1] = dsp.registers[REG_Y0];
			source[0] = 0;
			if (source[1] & (1<<23)) {
				source[0] = 0x0000ff;
			}
			break;
		case 6:	/* X1 */
			source[2] = 0;
			source[1] = dsp.registers[REG_X1];
			source[0] = 0;
			if (source[1] & (1<<23)) {
				source[0] = 0x0000ff;
			}
			break;
		case 7:	/* Y1 */
			source[2] = 0;
			source[1] = dsp.registers[REG_Y1];
			source[0] = 0;
			if (source[1] & (1<<23)) {
				source[0] = 0x0000ff;
			}
			break;
	}

	dsp_abs56(source);
	newsr = dsp_sub56(source, dest);

	dsp_ccr_extension(&dest[0], &dest[1], &dest[2]);
	dsp_ccr_unnormalized(&dest[0], &dest[1], &dest[2]);
	dsp_ccr_negative(&dest[0], &dest[1], &dest[2]);
	dsp_ccr_zero(&dest[0], &dest[1], &dest[2]);

	dsp.registers[REG_SR] &= BITMASK(16)-((1<<SR_V)|(1<<SR_C));
	dsp.registers[REG_SR] |= newsr;
}

static void dsp_eor(void)
{
	uint32 srcreg, dstreg;

	srcreg = REG_X0+((cur_inst>>4) & BITMASK(2));
	dstreg = REG_A1+((cur_inst>>3) & 1);

	dsp.registers[dstreg] ^= dsp.registers[srcreg];
	dsp.registers[dstreg] &= BITMASK(24); /* FIXME: useless ? */

	dsp.registers[REG_SR] &= BITMASK(16)-((1<<SR_N)|(1<<SR_Z)|(1<<SR_V));
	dsp.registers[REG_SR] |= ((dsp.registers[dstreg]>>23) & 1)<<SR_N;
	dsp.registers[REG_SR] |= (dsp.registers[dstreg]==0)<<SR_Z;
}

static void dsp_lsl(void)
{
	uint32 numreg, newcarry;

	numreg = (cur_inst>>3) & 1;

	newcarry = (dsp.registers[REG_A1+numreg]>>23) & 1;

	dsp.registers[REG_A1+numreg] &= BITMASK(24);
	dsp.registers[REG_A1+numreg] <<= 1;

	dsp.registers[REG_SR] &= BITMASK(16)-((1<<SR_C)|(1<<SR_N)|(1<<SR_Z)|(1<<SR_V));
	dsp.registers[REG_SR] |= newcarry;
	dsp.registers[REG_SR] |= ((dsp.registers[numreg]>>23) & 1)<<SR_N;
	dsp.registers[REG_SR] |= (dsp.registers[numreg]==0)<<SR_Z;
}

static void dsp_lsr(void)
{
	uint32 numreg, newcarry;

	numreg = (cur_inst>>3) & 1;

	newcarry = dsp.registers[REG_A1+numreg] & 1;

	dsp.registers[REG_A1+numreg] &= BITMASK(24);
	dsp.registers[REG_A1+numreg] >>= 1;

	dsp.registers[REG_SR] &= BITMASK(16)-((1<<SR_C)|(1<<SR_N)|(1<<SR_Z)|(1<<SR_V));
	dsp.registers[REG_SR] |= newcarry;
	dsp.registers[REG_SR] |= (dsp.registers[numreg]==0)<<SR_Z;
}

static void dsp_mac(void)
{
	uint32 srcreg1, srcreg2, destreg, value, source[3], dest[3];
	uint16 newsr;

	value = (cur_inst>>4) & BITMASK(3);
	srcreg1 = registers_mpy[value][0];
	srcreg2 = registers_mpy[value][1];

	dsp_mul56(dsp.registers[srcreg1], dsp.registers[srcreg2], source);

	if (cur_inst & (1<<2)) {
		dest[0] = dest[1] = dest[2] = 0;

		dsp_sub56(source, dest);

		source[0] = dest[0];
		source[1] = dest[1];
		source[2] = dest[2];
	}

	destreg = (cur_inst>>3) & 1;
	dest[0] = dsp.registers[REG_A2+destreg];
	dest[1] = dsp.registers[REG_A1+destreg];
	dest[2] = dsp.registers[REG_A0+destreg];
	newsr = dsp_add56(source, dest);

	dsp.registers[REG_A2+destreg] = dest[0];
	dsp.registers[REG_A1+destreg] = dest[1];
	dsp.registers[REG_A0+destreg] = dest[2];

	dsp_ccr_extension(&dest[0], &dest[1], &dest[2]);
	dsp_ccr_unnormalized(&dest[0], &dest[1], &dest[2]);
	dsp_ccr_negative(&dest[0], &dest[1], &dest[2]);
	dsp_ccr_zero(&dest[0], &dest[1], &dest[2]);

	dsp.registers[REG_SR] &= BITMASK(16)-((1<<SR_V)|(1<<SR_C));
	dsp.registers[REG_SR] |= newsr;
}

static void dsp_macr(void)
{
	uint32 srcreg1, srcreg2, destreg, value, source[3], dest[3];
	uint16 newsr;

	value = (cur_inst>>4) & BITMASK(3);
	srcreg1 = registers_mpy[value][0];
	srcreg2 = registers_mpy[value][1];

	dsp_mul56(dsp.registers[srcreg1], dsp.registers[srcreg2], source);

	if (cur_inst & (1<<2)) {
		dest[0] = dest[1] = dest[2] = 0;

		dsp_sub56(source, dest);

		source[0] = dest[0];
		source[1] = dest[1];
		source[2] = dest[2];
	}

	destreg = (cur_inst>>3) & 1;
	dest[0] = dsp.registers[REG_A2+destreg];
	dest[1] = dsp.registers[REG_A1+destreg];
	dest[2] = dsp.registers[REG_A0+destreg];
	newsr = dsp_add56(source, dest);

	dsp_rnd56(dest);

	dsp.registers[REG_A2+destreg] = dest[0];
	dsp.registers[REG_A1+destreg] = dest[1];
	dsp.registers[REG_A0+destreg] = dest[2];

	dsp_ccr_extension(&dest[0], &dest[1], &dest[2]);
	dsp_ccr_unnormalized(&dest[0], &dest[1], &dest[2]);
	dsp_ccr_negative(&dest[0], &dest[1], &dest[2]);
	dsp_ccr_zero(&dest[0], &dest[1], &dest[2]);

	dsp.registers[REG_SR] &= BITMASK(16)-((1<<SR_V)|(1<<SR_C));
	dsp.registers[REG_SR] |= newsr;
}

static void dsp_move(void)
{
	/*	move instruction inside alu opcodes
		taken care of by parallel move dispatcher */
}

static void dsp_move_pm(void)
{
	/* move instruction outside alu opcodes */
	dsp_parmove_read();
	dsp_parmove_write();
}

static void dsp_mpy(void)
{
	uint32 srcreg1, srcreg2, destreg, value, dest[3], source[3];

	value = (cur_inst>>4) & BITMASK(3);
	srcreg1 = registers_mpy[value][0];
	srcreg2 = registers_mpy[value][1];

	dsp_mul56(dsp.registers[srcreg1], dsp.registers[srcreg2], source);

	destreg = (cur_inst>>3) & 1;
	if (cur_inst & (1<<2)) {
		dest[0] = dest[1] = dest[2] = 0;

		dsp_sub56(source, dest);

		dsp.registers[REG_A2+destreg] = dest[0];
		dsp.registers[REG_A1+destreg] = dest[1];
		dsp.registers[REG_A0+destreg] = dest[2];
	} else {
		dsp.registers[REG_A2+destreg] = source[0];
		dsp.registers[REG_A1+destreg] = source[1];
		dsp.registers[REG_A0+destreg] = source[2];
	}

	dsp_ccr_extension(&dsp.registers[REG_A2+destreg], &dsp.registers[REG_A1+destreg], &dsp.registers[REG_A0+destreg]);
	dsp_ccr_unnormalized(&dsp.registers[REG_A2+destreg], &dsp.registers[REG_A1+destreg], &dsp.registers[REG_A0+destreg]);
	dsp_ccr_negative(&dsp.registers[REG_A2+destreg], &dsp.registers[REG_A1+destreg], &dsp.registers[REG_A0+destreg]);
	dsp_ccr_zero(&dsp.registers[REG_A2+destreg], &dsp.registers[REG_A1+destreg], &dsp.registers[REG_A0+destreg]);

	dsp.registers[REG_SR] &= BITMASK(16)-(1<<SR_V);
}

static void dsp_mpyr(void)
{
	uint32 srcreg1, srcreg2, destreg, value, dest[3], source[3];

	value = (cur_inst>>4) & BITMASK(3);
	srcreg1 = registers_mpy[value][0];
	srcreg2 = registers_mpy[value][1];

	dsp_mul56(dsp.registers[srcreg1], dsp.registers[srcreg2], source);

	destreg = (cur_inst>>3) & 1;
	if (cur_inst & (1<<2)) {
		dest[0] = dest[1] = dest[2] = 0;

		dsp_sub56(source, dest);
	} else {
		dest[0] = source[0];
		dest[1] = source[1];
		dest[2] = source[2];
	}

	dsp_rnd56(dest);

	dsp.registers[REG_A2+destreg] = dest[0];
	dsp.registers[REG_A1+destreg] = dest[1];
	dsp.registers[REG_A0+destreg] = dest[2];

	dsp_ccr_extension(&dest[0], &dest[1], &dest[2]);
	dsp_ccr_unnormalized(&dest[0], &dest[1], &dest[2]);
	dsp_ccr_negative(&dest[0], &dest[1], &dest[2]);
	dsp_ccr_zero(&dest[0], &dest[1], &dest[2]);

	dsp.registers[REG_SR] &= BITMASK(16)-(1<<SR_V);
}

static void dsp_neg(void)
{
	uint32 srcreg, source[3], dest[3], overflowed;

	srcreg = (cur_inst>>3) & 1;
	source[0] = dsp.registers[REG_A2+srcreg];
	source[1] = dsp.registers[REG_A1+srcreg];
	source[2] = dsp.registers[REG_A0+srcreg];

	overflowed = ((source[2]==0) && (source[1]==0) && (source[0]==0x80));

	dest[0] = dest[1] = dest[2] = 0;

	dsp_sub56(source, dest);

	dsp.registers[REG_A2+srcreg] = dest[0];
	dsp.registers[REG_A1+srcreg] = dest[1];
	dsp.registers[REG_A0+srcreg] = dest[2];

	dsp.registers[REG_SR] &= BITMASK(16)-(1<<SR_V);
	dsp.registers[REG_SR] |= (overflowed<<SR_L)|(overflowed<<SR_V);

	dsp_ccr_extension(&dest[0], &dest[1], &dest[2]);
	dsp_ccr_unnormalized(&dest[0], &dest[1], &dest[2]);
	dsp_ccr_negative(&dest[0], &dest[1], &dest[2]);
	dsp_ccr_zero(&dest[0], &dest[1], &dest[2]);
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

	dsp.registers[REG_SR] &= BITMASK(16)-((1<<SR_N)|(1<<SR_Z)|(1<<SR_V));
	dsp.registers[REG_SR] |= ((dsp.registers[dstreg]>>23) & 1)<<SR_N;
	dsp.registers[REG_SR] |= (dsp.registers[dstreg]==0)<<SR_Z;
}

static void dsp_or(void)
{
	uint32 srcreg, dstreg;

	srcreg = REG_X0+((cur_inst>>4) & BITMASK(2));
	dstreg = REG_A1+((cur_inst>>3) & 1);

	dsp.registers[dstreg] |= dsp.registers[srcreg];
	dsp.registers[dstreg] &= BITMASK(24); /* FIXME: useless ? */

	dsp.registers[REG_SR] &= BITMASK(16)-((1<<SR_N)|(1<<SR_Z)|(1<<SR_V));
	dsp.registers[REG_SR] |= ((dsp.registers[dstreg]>>23) & 1)<<SR_N;
	dsp.registers[REG_SR] |= (dsp.registers[dstreg]==0)<<SR_Z;
}

static void dsp_rnd(void)
{
	uint32 numreg, dest[3];

	numreg = (cur_inst>>3) & 1;

	dest[0] = dsp.registers[REG_A2+numreg];
	dest[1] = dsp.registers[REG_A1+numreg];
	dest[2] = dsp.registers[REG_A0+numreg];

	dsp_rnd56(dest);

	dsp.registers[REG_A2+numreg] = dest[0];
	dsp.registers[REG_A1+numreg] = dest[1];
	dsp.registers[REG_A0+numreg] = dest[2];

	dsp.registers[REG_SR] &= BITMASK(16)-(1<<SR_V);

	dsp_ccr_extension(&dest[0], &dest[1], &dest[2]);
	dsp_ccr_unnormalized(&dest[0], &dest[1], &dest[2]);
	dsp_ccr_negative(&dest[0], &dest[1], &dest[2]);
	dsp_ccr_zero(&dest[0], &dest[1], &dest[2]);
}

static void dsp_rol(void)
{
	uint32 dstreg, newcarry;

	dstreg = REG_A1+((cur_inst>>3) & 1);

	newcarry = (dsp.registers[dstreg]>>23) & 1;

	dsp.registers[dstreg] <<= 1;
	dsp.registers[dstreg] |= newcarry;
	dsp.registers[dstreg] &= BITMASK(24);

	dsp.registers[REG_SR] &= BITMASK(16)-((1<<SR_C)|(1<<SR_N)|(1<<SR_Z)|(1<<SR_V));
	dsp.registers[REG_SR] |= newcarry;
	dsp.registers[REG_SR] |= ((dsp.registers[dstreg]>>23) & 1)<<SR_N;
	dsp.registers[REG_SR] |= (dsp.registers[dstreg]==0)<<SR_Z;
}

static void dsp_ror(void)
{
	uint32 dstreg, newcarry;

	dstreg = REG_A1+((cur_inst>>3) & 1);

	newcarry = dsp.registers[dstreg] & 1;

	dsp.registers[dstreg] >>= 1;
	dsp.registers[dstreg] |= newcarry<<23;
	dsp.registers[dstreg] &= BITMASK(24);

	dsp.registers[REG_SR] &= BITMASK(16)-((1<<SR_C)|(1<<SR_N)|(1<<SR_Z)|(1<<SR_V));
	dsp.registers[REG_SR] |= newcarry;
	dsp.registers[REG_SR] |= ((dsp.registers[dstreg]>>23) & 1)<<SR_N;
	dsp.registers[REG_SR] |= (dsp.registers[dstreg]==0)<<SR_Z;
}

static void dsp_sbc(void)
{
	uint32 srcreg, destreg, source[3], dest[3], curcarry;
	uint16 newsr;

	curcarry = (dsp.registers[REG_SR]>>(SR_C)) & 1;

	destreg = (cur_inst>>3) & 1;
	dest[0] = dsp.registers[REG_A2+destreg];
	dest[1] = dsp.registers[REG_A1+destreg];
	dest[2] = dsp.registers[REG_A0+destreg];

	srcreg = (cur_inst>>4) & 1;
	switch(srcreg) {
		case 0:	/* X */
			source[1] = dsp.registers[REG_X1];
			source[2] = dsp.registers[REG_X0];
			source[0] = 0;
			if (source[1] & (1<<23)) {
				source[0] = 0x0000ff;
			}
			break;
		case 1:	/* Y */
			source[1] = dsp.registers[REG_Y1];
			source[2] = dsp.registers[REG_Y0];
			source[0] = 0;
			if (source[1] & (1<<23)) {
				source[0] = 0x0000ff;
			}
			break;
	}

	newsr = dsp_sub56(source, dest);
	
	if (curcarry) {
		source[0]=0;
		source[1]=0;
		source[2]=1;
		newsr |= dsp_sub56(source, dest);
	}

	dsp.registers[REG_A2+destreg] = dest[0];
	dsp.registers[REG_A1+destreg] = dest[1];
	dsp.registers[REG_A0+destreg] = dest[2];

	dsp_ccr_extension(&dest[0], &dest[1], &dest[2]);
	dsp_ccr_unnormalized(&dest[0], &dest[1], &dest[2]);
	dsp_ccr_negative(&dest[0], &dest[1], &dest[2]);
	dsp_ccr_zero(&dest[0], &dest[1], &dest[2]);

	dsp.registers[REG_SR] &= BITMASK(16)-((1<<SR_V)|(1<<SR_C));
	dsp.registers[REG_SR] |= newsr;
}

static void dsp_sub(void)
{
	uint32 srcreg, destreg, source[3], dest[3];
	uint16 newsr;

	destreg = (cur_inst>>3) & 1;
	dest[0] = dsp.registers[REG_A2+destreg];
	dest[1] = dsp.registers[REG_A1+destreg];
	dest[2] = dsp.registers[REG_A0+destreg];

	srcreg = (cur_inst>>4) & BITMASK(3);
	switch(srcreg) {
		case 1:	/* A or B */
			srcreg = destreg ^ 1;
			source[0] = dsp.registers[REG_A2+srcreg];
			source[1] = dsp.registers[REG_A1+srcreg];
			source[2] = dsp.registers[REG_A0+srcreg];
			break;
		case 2:	/* X */
			source[1] = dsp.registers[REG_X1];
			source[2] = dsp.registers[REG_X0];
			source[0] = 0;
			if (source[1] & (1<<23)) {
				source[0] = 0x0000ff;
			}
			break;
		case 3:	/* Y */
			source[1] = dsp.registers[REG_Y1];
			source[2] = dsp.registers[REG_Y0];
			source[0] = 0;
			if (source[1] & (1<<23)) {
				source[0] = 0x0000ff;
			}
			break;
		case 4:	/* X0 */
			source[2] = 0;
			source[1] = dsp.registers[REG_X0];
			source[0] = 0;
			if (source[1] & (1<<23)) {
				source[0] = 0x0000ff;
			}
			break;
		case 5:	/* Y0 */
			source[2] = 0;
			source[1] = dsp.registers[REG_Y0];
			source[0] = 0;
			if (source[1] & (1<<23)) {
				source[0] = 0x0000ff;
			}
			break;
		case 6:	/* X1 */
			source[2] = 0;
			source[1] = dsp.registers[REG_X1];
			source[0] = 0;
			if (source[1] & (1<<23)) {
				source[0] = 0x0000ff;
			}
			break;
		case 7:	/* Y1 */
			source[2] = 0;
			source[1] = dsp.registers[REG_Y1];
			source[0] = 0;
			if (source[1] & (1<<23)) {
				source[0] = 0x0000ff;
			}
			break;
	}

	newsr = dsp_sub56(source, dest);

	dsp.registers[REG_A2+destreg] = dest[0];
	dsp.registers[REG_A1+destreg] = dest[1];
	dsp.registers[REG_A0+destreg] = dest[2];

	dsp_ccr_extension(&dest[0], &dest[1], &dest[2]);
	dsp_ccr_unnormalized(&dest[0], &dest[1], &dest[2]);
	dsp_ccr_negative(&dest[0], &dest[1], &dest[2]);
	dsp_ccr_zero(&dest[0], &dest[1], &dest[2]);

	dsp.registers[REG_SR] &= BITMASK(16)-((1<<SR_V)|(1<<SR_C));
	dsp.registers[REG_SR] |= newsr;
}

static void dsp_subl(void)
{
	uint32 numreg, source[3], dest[3];
	uint16 newsr;

	numreg = (cur_inst>>3) & 1;

	dest[0] = dsp.registers[REG_A2+numreg];
	dest[1] = dsp.registers[REG_A1+numreg];
	dest[2] = dsp.registers[REG_A0+numreg];
	newsr = dsp_asl56(dest);

	source[0] = dsp.registers[REG_A2+(numreg ^ 1)];
	source[1] = dsp.registers[REG_A1+(numreg ^ 1)];
	source[2] = dsp.registers[REG_A0+(numreg ^ 1)];
	newsr |= dsp_sub56(source, dest);

	dsp.registers[REG_A2+numreg] = dest[0];
	dsp.registers[REG_A1+numreg] = dest[1];
	dsp.registers[REG_A0+numreg] = dest[2];

	dsp_ccr_extension(&dest[0], &dest[1], &dest[2]);
	dsp_ccr_unnormalized(&dest[0], &dest[1], &dest[2]);
	dsp_ccr_negative(&dest[0], &dest[1], &dest[2]);
	dsp_ccr_zero(&dest[0], &dest[1], &dest[2]);

	dsp.registers[REG_SR] &= BITMASK(16)-((1<<SR_V)|(1<<SR_C));
	dsp.registers[REG_SR] |= newsr;
}

static void dsp_subr(void)
{
	uint32 numreg, source[3], dest[3];
	uint16 newsr;

	numreg = (cur_inst>>3) & 1;

	dest[0] = dsp.registers[REG_A2+numreg];
	dest[1] = dsp.registers[REG_A1+numreg];
	dest[2] = dsp.registers[REG_A0+numreg];
	newsr = dsp_asr56(dest);

	source[0] = dsp.registers[REG_A2+(numreg ^ 1)];
	source[1] = dsp.registers[REG_A1+(numreg ^ 1)];
	source[2] = dsp.registers[REG_A0+(numreg ^ 1)];
	newsr |= dsp_sub56(source, dest);

	dsp.registers[REG_A2+numreg] = dest[0];
	dsp.registers[REG_A1+numreg] = dest[1];
	dsp.registers[REG_A0+numreg] = dest[2];

	dsp_ccr_extension(&dest[0], &dest[1], &dest[2]);
	dsp_ccr_unnormalized(&dest[0], &dest[1], &dest[2]);
	dsp_ccr_negative(&dest[0], &dest[1], &dest[2]);
	dsp_ccr_zero(&dest[0], &dest[1], &dest[2]);

	dsp.registers[REG_SR] &= BITMASK(16)-((1<<SR_V)|(1<<SR_C));
	dsp.registers[REG_SR] |= newsr;
}

static void dsp_tfr(void)
{
	uint32 srcreg, destreg, source[3];

	destreg = (cur_inst>>3) & 1;

	srcreg = (cur_inst>>4) & BITMASK(3);
	switch(srcreg) {
		case 0:	/* A or B */
			srcreg = destreg ^ 1;
			source[0] = dsp.registers[REG_A2+srcreg];
			source[1] = dsp.registers[REG_A1+srcreg];
			source[2] = dsp.registers[REG_A0+srcreg];
			break;
		case 4:	/* X0 */
			source[2] = 0;
			source[1] = dsp.registers[REG_X0];
			source[0] = 0;
			if (source[1] & (1<<23)) {
				source[0] = 0x0000ff;
			}
			break;
		case 5:	/* Y0 */
			source[2] = 0;
			source[1] = dsp.registers[REG_Y0];
			source[0] = 0;
			if (source[1] & (1<<23)) {
				source[0] = 0x0000ff;
			}
			break;
		case 6:	/* X1 */
			source[2] = 0;
			source[1] = dsp.registers[REG_X1];
			source[0] = 0;
			if (source[1] & (1<<23)) {
				source[0] = 0x0000ff;
			}
			break;
		case 7:	/* Y1 */
			source[2] = 0;
			source[1] = dsp.registers[REG_Y1];
			source[0] = 0;
			if (source[1] & (1<<23)) {
				source[0] = 0x0000ff;
			}
			break;
	}

	dsp.registers[REG_A2+destreg] = source[0];
	dsp.registers[REG_A1+destreg] = source[1];
	dsp.registers[REG_A0+destreg] = source[2];
}

static void dsp_tst(void)
{
	uint32 destreg;
	
	destreg = (cur_inst>>3) & 1;

	dsp_ccr_extension(&dsp.registers[REG_A2+destreg], &dsp.registers[REG_A1+destreg], &dsp.registers[REG_A0+destreg]);
	dsp_ccr_unnormalized(&dsp.registers[REG_A2+destreg], &dsp.registers[REG_A1+destreg], &dsp.registers[REG_A0+destreg]);
	dsp_ccr_negative(&dsp.registers[REG_A2+destreg], &dsp.registers[REG_A1+destreg], &dsp.registers[REG_A0+destreg]);
	dsp_ccr_zero(&dsp.registers[REG_A2+destreg], &dsp.registers[REG_A1+destreg], &dsp.registers[REG_A0+destreg]);

	dsp.registers[REG_SR] &= BITMASK(16)-(1<<SR_V);
}

/*
	2002-09-20:PM
		BUG: Bit-reversed addressing not working
		BUG: host port transfer not done when necessary when using thread
	2002-08-27:PM
		BUG: Fixes in thread support
		Host port transfer routines moved in dsp.cpp
	2002-08-26:PM
		Added thread support
	2002-08-08:PM
		Added some casts to avoid warnings
		BUG:host receive interrupt not raised
		BUG:mul56 operation fixed
	2002-08-01:PM
		FIX:Finished ccr bits stuff
	2002-07-31:PM
		FIX:Nearly finished ccr bits stuff
		FIX:host port TRDY bit now correctly set
		BUG:host port transfer routines not called when DSP not running
		BUG:when disassembly memory access, host port flags were cbanged
		BUG:pm_2 (register update) wrong calc of eamode
	2002-07-30:PM
		BUG:added stack check when pushing/popping values
		BUG:move(parallel,c,m) to register were not masked
	2002-07-29:PM
		BUG:corrected div routine
	2002-07-26:PM
		BUG:bad detection of rom space in read_memory
		BUG:lua updated register with previous value, not the new one
		BUG:added missing '\n' in disasm output
	2002-07-25:PM
		FIX:replaced D(bug()) by fprintf() for disasm
	2002-07-22:PM
		FIX:removed sub56 operation in tst()
	2002-07-19:PM
		BUG:movec_b and movec_d operations permuted
		BUG:pm_5: wrong bit number used for write flag
		BUG:div: did not save the result
	2002-07-18:PM
		FIX:modified ccr stuff to test arbitrary accumulator, not only A and B
*/
