/*
	DSP M56001 emulation
	Core of DSP emulation

	(C) 2003-2008 ARAnyM developer team

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef DSP_CORE_H
#define DSP_CORE_H

#include <SDL.h>
#include <SDL_thread.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DSP_RAMSIZE 32768

/* Host port, CPU side */
#define CPU_HOST_ICR	0x00
#define CPU_HOST_CVR	0x01
#define CPU_HOST_ISR	0x02
#define CPU_HOST_IVR	0x03
#define CPU_HOST_RX0	0x04
#define CPU_HOST_RXH	0x05
#define CPU_HOST_RXM	0x06
#define CPU_HOST_RXL	0x07
#define CPU_HOST_TX0	0x04
#define CPU_HOST_TXH	0x05
#define CPU_HOST_TXM	0x06
#define CPU_HOST_TXL	0x07

#define CPU_HOST_ICR_RREQ	0x00
#define CPU_HOST_ICR_TREQ	0x01
#define CPU_HOST_ICR_HF0	0x03
#define CPU_HOST_ICR_HF1	0x04
#define CPU_HOST_ICR_HM0	0x05
#define CPU_HOST_ICR_HM1	0x06
#define CPU_HOST_ICR_INIT	0x07

#define CPU_HOST_CVR_HC		0x07

#define CPU_HOST_ISR_RXDF	0x00
#define CPU_HOST_ISR_TXDE	0x01
#define CPU_HOST_ISR_TRDY	0x02
#define CPU_HOST_ISR_HF2	0x03
#define CPU_HOST_ISR_HF3	0x04
#define CPU_HOST_ISR_DMA	0x06
#define CPU_HOST_ISR_HREQ	0x07

/* Host port, DSP side, DSP addresses are 0xffc0+value */
#define DSP_PBC			0x20	/* Port B control register */
#define DSP_PCC			0x21	/* Port C control register */
#define DSP_PBDDR		0x22	/* Port B data direction register */
#define DSP_PCDDR		0x23	/* Port C data direction register */
#define DSP_PBD			0x24	/* Port B data register */
#define DSP_PCD			0x25	/* Port C data register */
#define DSP_HOST_HCR	0x28	/* Host control register */
#define DSP_HOST_HSR	0x29	/* Host status register */
#define DSP_HOST_HRX	0x2b	/* Host receive register */
#define DSP_HOST_HTX	0x2b	/* Host transmit register */
#define DSP_SSI_CRA		0x2c	/* Ssi control register A */
#define DSP_SSI_CRB		0x2d	/* Ssi control register B */
#define DSP_SSI_SR		0x2e	/* Ssi status register */
#define DSP_SSI_TSR		0x2e	/* Ssi time slot register */
#define DSP_SSI_RX		0x2f	/* Ssi receive register */
#define DSP_SSI_TX		0x2f	/* Ssi transmit register */
#define DSP_BCR			0x3e	/* Port A bus control register */
#define DSP_IPR			0x3f	/* Interrupt priority register */

#define DSP_HOST_HCR_HRIE	0x00
#define DSP_HOST_HCR_HTIE	0x01
#define DSP_HOST_HCR_HCIE	0x02
#define DSP_HOST_HCR_HF2	0x03
#define DSP_HOST_HCR_HF3	0x04

#define DSP_HOST_HSR_HRDF	0x00
#define DSP_HOST_HSR_HTDE	0x01
#define DSP_HOST_HSR_HCP	0x02
#define DSP_HOST_HSR_HF0	0x03
#define DSP_HOST_HSR_HF1	0x04
#define DSP_HOST_HSR_DMA	0x07

typedef struct {
	SDL_Thread	*thread;	/* Thread in which DSP emulation runs */
	SDL_sem		*semaphore;	/* Semaphore used to pause/unpause thread */
	SDL_mutex	*mutex;		/* Mutex for read/writes through host port */

	/* DSP executing instructions ? */
	volatile int running;

	/* Registers */
	Uint16	pc;
	Uint32	registers[64];

	/* stack[0=ssh], stack[1=ssl] */
	Uint16	stack[2][15];

	/* External ram[0] is x:, ram[1] is y:, ram[2] is p: */
	Uint32	ram[3][DSP_RAMSIZE];

	/* rom[0] is x:, rom[1] is y: */
	Uint32	rom[2][512];

	/* Internal ram[0] is x:, ram[1] is y:, ram[2] is p: */
	Uint32	ramint[3][512];

	/* peripheral space, [x|y]:0xffc0-0xffff */
	volatile Uint32	periph[2][64];

	/* host port, CPU side */
	volatile Uint8 hostport[8];

	/* Misc */
	Uint32 loop_rep;		/* executing rep ? */

	/* For bootstrap routine */
	Uint16	bootstrap_pos;
} dsp_core_t;

/* Emulator call these to init/stop/reset DSP emulation */
void dsp_core_init(dsp_core_t *dsp_core);
void dsp_core_shutdown(dsp_core_t *dsp_core);
void dsp_core_reset(dsp_core_t *dsp_core);

/* host port read/write by emulator, addr is 0-7, not 0xffa200-0xffa207 */
Uint8 dsp_core_read_host(dsp_core_t *dsp_core, int addr);
void dsp_core_write_host(dsp_core_t *dsp_core, int addr, Uint8 value);

/* dsp_cpu call these to signal state change */
void dsp_core_set_state(dsp_core_t *dsp_core, int new_state);
void dsp_core_set_state_sem(dsp_core_t *dsp_core, int new_state, int use_semaphore);

/* dsp_cpu call these to read/write host port */
void dsp_core_hostport_dspread(dsp_core_t *dsp_core);
void dsp_core_hostport_dspwrite(dsp_core_t *dsp_core);

#ifdef __cplusplus
}
#endif

#endif /* DSP_CORE_H */
