/*
 *	DSP56K emulation
 *	kernel
 *
 *	Patrice Mandin
 */

#ifndef _DSP_CPU_H_
#define _DSP_CPU_H_

/* Defines */
#define OMR_MA	0x00
#define OMR_MB	0x01
#define OMR_DE	0x02
#define OMR_SD	0x06
#define OMR_EA	0x07

#define SR_C	0x00
#define SR_V	0x01
#define SR_Z	0x02
#define SR_N	0x03
#define SR_U	0x04
#define SR_E	0x05
#define SR_L	0x06

#define SR_I0	0x08
#define SR_I1	0x09
#define SR_S0	0x0a
#define SR_S1	0x0b
#define SR_T	0x0d
#define SR_LF	0x0f

/* Registers numbers in dsp.registers[] */
#define REG_X0	0x04
#define REG_X1	0x05
#define REG_Y0	0x06
#define REG_Y1	0x07
#define REG_A0	0x08
#define REG_B0	0x09
#define REG_A2	0x0a
#define REG_B2	0x0b
#define REG_A1	0x0c
#define REG_B1	0x0d
#define REG_A	0x0e
#define REG_B	0x0f

#define REG_R0	0x10
#define REG_R1	0x11
#define REG_R2	0x12
#define REG_R3	0x13
#define REG_R4	0x14
#define REG_R5	0x15
#define REG_R6	0x16
#define REG_R7	0x17

#define REG_N0	0x18
#define REG_N1	0x19
#define REG_N2	0x1a
#define REG_N3	0x1b
#define REG_N4	0x1c
#define REG_N5	0x1d
#define REG_N6	0x1e
#define REG_N7	0x1f

#define REG_M0	0x20
#define REG_M1	0x21
#define REG_M2	0x22
#define REG_M3	0x23
#define REG_M4	0x24
#define REG_M5	0x25
#define REG_M6	0x26
#define REG_M7	0x27

#define REG_SR	0x39
#define REG_OMR	0x3a
#define REG_SP	0x3b
#define REG_SSH	0x3c
#define REG_SSL	0x3d
#define REG_LA	0x3e
#define REG_LC	0x3f

#define REG_NULL	0x00
#define REG_LCSAVE	0x30

/* Memory spaces for dsp.ram[], dsp.rom[] */
#define SPACE_X	0x00
#define SPACE_Y	0x01
#define SPACE_P	0x02

/* Functions */
void dsp56k_do_execute(void);

#endif
