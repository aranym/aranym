/*
 * DSP56K emulation
 * host port
 *
 * Joy 2001
 * Patrice Mandin
 */

#ifdef HAVE_NEW_HEADERS
# include <cmath>
#else
# include <math.h>
#endif

#include <SDL.h>
#include <SDL_thread.h>

#include "sysdeps.h"
#include "hardware.h"
#include "cpu_emulation.h"
#include "memory.h"
#include "dsp.h"
#include "dsp_cpu.h"

#define DEBUG 0
#include "debug.h"

#ifndef M_PI
#define M_PI	3.141592653589793238462643383279502
#endif

DSP::DSP(memptr address, uint32 size) : BASE_IO(address, size)
{
#if DSP_EMULATION
	init();
#endif
}

#if DSP_EMULATION

/* More disasm infos, if wanted */
#define DSP_DISASM_HOSTREAD 0	/* Dsp->Host transfer */
#define DSP_DISASM_HOSTWRITE 0	/* Host->Dsp transfer */
#define DSP_DISASM_STATE 0		/* State changes */

/* Execute DSP instructions till the DSP waits for a read/write */
#define DSP_HOST_FORCEEXEC 0

/* Constructor and  destructor for DSP class */
void DSP::init(void)
{
	int i;

	memset(ram, 0,sizeof(ram));

	/* Initialize Y:rom[0x0100-0x01ff] with a sin table */
	{
		float src;
		int32 dest;

		for (i=0;i<256;i++) {
			src = (((float) i)*M_PI)/128.0;
			dest = (int32) (sin(src) * 8388608.0); /* 1<<23 */
			if (dest>8388607) {
				dest = 8388607;
			} else if (dest<-8388608) {
				dest = -8388608;
			}
			rom[SPACE_Y][0x100+i]=dest & 0x00ffffff;
		}
	}

	/* Initialize X:rom[0x0100-0x017f] with a mu-law table */
	{
		const uint16 mulaw_base[8]={
			0x7d7c, 0x3e7c, 0x1efc, 0x0f3c, 0x075c, 0x036c, 0x0174, 0x0078
		};

		uint32 value, offset, position;
		int j;

		position = 0x0100;
		offset = 0x040000;
		for(i=0;i<8;i++) {
			value = mulaw_base[i]<<8;

			for (j=0;j<16;j++) {
				rom[SPACE_X][position++]=value;
				value -= offset;
			}

			offset >>= 1;
		}
	}

	/* Initialize X:rom[0x0180-0x01ff] with a a-law table */
	{
		const int32 multiply_base[8]={
			0x1580, 0x0ac0, 0x5600, 0x2b00,
			0x1580, 0x0058, 0x0560, 0x02b0
		};
		const int32 multiply_col[4]={0x10, 0x01, 0x04, 0x02};
		const int32 multiply_line[4]={0x40, 0x04, 0x10, 0x08};
		const int32 base_values[4]={0, -1, 2, 1};
		uint32 pos=0x0180;
		
		for (i=0;i<8;i++) {
			int32 alawbase, j;

			alawbase = multiply_base[i]<<8;
			for (j=0;j<4;j++) {
				int32 alawbase1, k;
				
				alawbase1 = alawbase + ((base_values[j]*multiply_line[i & 3])<<12);

				for (k=0;k<4;k++) {
					int32 alawbase2;

					alawbase2 = alawbase1 + ((base_values[k]*multiply_col[i & 3])<<12);

					rom[SPACE_X][pos++]=alawbase2;
				}
			}
		}
	}
	
	D(bug("Dsp: power-on done"));

	dsp56k_thread = NULL;
	dsp56k_sem = NULL;

	state = DSP_HALT;
#if DSP_DISASM_STATE
	D(bug("Dsp: state = HALT"));
#endif
}

DSP::~DSP(void)
{
	shutdown();
}

/* Other functions to init/shutdown dsp emulation */
void DSP::reset(void)
{
	int i;

	/* Kill existing thread and semaphore */
	shutdown();

	/* Pause thread */
	state = DSP_BOOTING;
#if DSP_DISASM_STATE
	D(bug("Dsp: state = BOOTING"));
#endif

	/* Memory */
	memset(periph, 0,sizeof(periph));
	memset(stack, 0,sizeof(stack));
	memset(registers, 0,sizeof(registers));

	bootstrap_pos = bootstrap_accum = 0;
	
	/* Registers */
	pc = 0x0000;
	registers[REG_OMR]=0x02;
	for (i=0;i<8;i++) {
		registers[REG_M0+i]=0x00ffff;
	}

	/* host port init, dsp side */
	periph[SPACE_X][DSP_HOST_HSR]=(1<<DSP_HOST_HSR_HTDE);

	/* host port init, cpu side */
	hostport[CPU_HOST_CVR]=0x12;
	hostport[CPU_HOST_ISR]=(1<<CPU_HOST_ISR_TRDY)|(1<<CPU_HOST_ISR_TXDE);
	hostport[CPU_HOST_IVR]=0x0f;

	/* Other hardware registers */
	periph[SPACE_X][DSP_IPR]=0;
	periph[SPACE_X][DSP_BCR]=0xffff;

	/* Misc */
	loop_rep = 0;
	last_loop_inst = 0;
	first_host_write = 1;

	D(bug("Dsp: reset done"));

	/* Create thread and semaphore if needed */
	if (dsp56k_sem == NULL) {
		dsp56k_sem = SDL_CreateSemaphore(0);
	}

	if (dsp56k_thread == NULL) {
		dsp56k_thread = SDL_CreateThread(dsp56k_do_execute, NULL);
	}
}

void DSP::shutdown(void)
{
	if (dsp56k_thread != NULL) {

		/* Stop thread */
		state = DSP_STOPTHREAD;
#if DSP_DISASM_STATE
		D(bug("Dsp: state = STOPTHREAD"));
#endif

		/* Release semaphore, if thread waiting for it */
		if (SDL_SemValue(dsp56k_sem)==0) {
			SDL_SemPost(dsp56k_sem);
		}

		/* Wait for the thread to finish */
		while (state != DSP_STOPPEDTHREAD) {
			SDL_Delay(1);
		}

		dsp56k_thread = NULL;
	}

	/* Destroy the semaphore */
	if (dsp56k_sem != NULL) {
		SDL_DestroySemaphore(dsp56k_sem);
		dsp56k_sem = NULL;
	}
}

/**********************************
 *	Force execution of DSP, till something
 *  to read from/write to host port
 **********************************/

inline void DSP::force_exec(void)
{
#if DSP_HOST_FORCEEXEC
	while (state == DSP_RUNNING) {
	}
#endif
}

#endif /* DSP_EMULATION */

/**********************************
 *	Hardware address read/write by CPU
 **********************************/

uae_u8 DSP::handleRead(memptr addr)
{
#if DSP_EMULATION
	uae_u8 value=0;

	addr -= getHWoffset();

/*	D(bug("HWget_b(0x%08x)=0x%02x at 0x%08x", addr+HW_DSP, value, showPC()));*/

	/* Whenever the host want to read something on host port, we test if a
	   transfer is needed */
	dsp2host();

	switch(addr) {
		case CPU_HOST_ICR:
			value = hostport[CPU_HOST_ICR];
			break;
		case CPU_HOST_CVR:
			value = hostport[CPU_HOST_CVR];
			break;
		case CPU_HOST_ISR:
			value = hostport[CPU_HOST_ISR];
			break;
		case CPU_HOST_IVR:
			value = hostport[CPU_HOST_IVR];
			break;
		case CPU_HOST_RX0:
			force_exec();
			value = 0;
			break;
		case CPU_HOST_RXH:
			force_exec();
			value = hostport[CPU_HOST_RXH];
			break;
		case CPU_HOST_RXM:
			force_exec();
			value = hostport[CPU_HOST_RXM];
			break;
		case CPU_HOST_RXL:
			force_exec();
			value = hostport[CPU_HOST_RXL];

			if (state!=DSP_BOOTING) {
				/* Clear RXDF bit to say that CPU has read */
				hostport[CPU_HOST_ISR] &= 0xff-(1<<CPU_HOST_ISR_RXDF);
#if DSP_DISASM_HOSTWRITE
				D(bug("Dsp: (D->H): Host RXDF cleared"));
#endif
			}

			/* Wake up DSP if it was waiting our read */
			if (state==DSP_WAITHOSTREAD) {
#if DSP_DISASM_STATE
				D(bug("Dsp: state = DSP_RUNNING"));
#endif
				state = DSP_RUNNING;

				if (SDL_SemValue(dsp56k_sem)==0) {
					SDL_SemPost(dsp56k_sem);
				}
			}

			break;
	}

	return value;
#else
	return 0xff;	// this value prevents TOS from hanging in the DSP init code */
#endif	/* DSP_EMULATION */
}

void DSP::handleWrite(memptr addr, uae_u8 value)
{
#if DSP_EMULATION
	addr -= getHWoffset();

/*	D(bug("HWput_b(0x%08x,0x%02x) at 0x%08x", addr+HW_DSP, value, showPC()));*/

	switch(addr) {
		case CPU_HOST_ICR:
			hostport[CPU_HOST_ICR]=value & 0xfb;
			/* Set HF1 and HF0 accordingly on the host side */
			periph[SPACE_X][DSP_HOST_HSR] &=
					0xff-((1<<DSP_HOST_HSR_HF1)|(1<<DSP_HOST_HSR_HF0));
			periph[SPACE_X][DSP_HOST_HSR] |=
					hostport[CPU_HOST_ICR] & ((1<<DSP_HOST_HSR_HF1)|(1<<DSP_HOST_HSR_HF0));
			break;
		case CPU_HOST_CVR:
			hostport[CPU_HOST_CVR]=value & 0x9f;
			/* if bit 7=1, host command */
			if (value & (1<<7)) {
				periph[SPACE_X][DSP_HOST_HSR] |= 1<<DSP_HOST_HSR_HCP;
			}
			break;
		case CPU_HOST_ISR:
			/* Read only */
			break;
		case CPU_HOST_IVR:
			hostport[CPU_HOST_IVR]=value;
			break;
		case CPU_HOST_TX0:
			force_exec();

			if (first_host_write) {
				first_host_write = 0;
				bootstrap_accum = 0;
			}
			break;
		case CPU_HOST_TXH:
			force_exec();

			if (first_host_write) {
				first_host_write = 0;
				bootstrap_accum = 0;
			}
			hostport[CPU_HOST_TXH]=value;
			bootstrap_accum |= value<<16;
			break;
		case CPU_HOST_TXM:
			force_exec();

			if (first_host_write) {
				first_host_write = 0;
				hostport[CPU_HOST_TXH]=value;	/* FIXME: is it correct ? */
				bootstrap_accum = 0;
			}
			hostport[CPU_HOST_TXM]=value;
			bootstrap_accum |= value<<8;
			break;
		case CPU_HOST_TXL:
			force_exec();

			if (first_host_write) {
				first_host_write = 0;
				hostport[CPU_HOST_TXH]=value;	/* FIXME: is it correct ? */
				hostport[CPU_HOST_TXM]=value;	/* FIXME: is it correct ? */
				bootstrap_accum = 0;
			}
			hostport[CPU_HOST_TXL]=value;
			bootstrap_accum |= value;

			first_host_write = 1;

			if (state!=DSP_BOOTING) {
				/* Clear TXDE to say that host has written */
				hostport[CPU_HOST_ISR] &= 0xff-(1<<CPU_HOST_ISR_TXDE);
#if DSP_DISASM_HOSTREAD
				D(bug("Dsp: (H->D): Host TXDE cleared"));
#endif

				host2dsp();
			}

			switch(state) {
				case DSP_BOOTING:
					ram[SPACE_P][bootstrap_pos] = bootstrap_accum;
/*					D(bug("Dsp: bootstrap: p:0x%04x: 0x%06x written", bootstrap_pos, bootstrap_accum));*/
					bootstrap_pos++;
					if (bootstrap_pos == 0x200) {
#if DSP_DISASM_STATE
						D(bug("Dsp: bootstrap done"));
#endif
						state = DSP_RUNNING;

						SDL_SemPost(dsp56k_sem);
					}		
					bootstrap_accum = 0;
					break;
				case DSP_WAITHOSTWRITE:
					/* Wake up DSP if it was waiting our write */
#if DSP_DISASM_STATE
					D(bug("Dsp: state = DSP_RUNNING"));
#endif
					state = DSP_RUNNING;

					if (SDL_SemValue(dsp56k_sem)==0) {
						SDL_SemPost(dsp56k_sem);
					}
					break;
			}

			break;
	}
#endif	/* DSP_EMULATION */
}

#if DSP_EMULATION

/**********************************
 *	Host transfer
 **********************************/

void DSP::host2dsp(void)
{
	int trdy;

	/* Host port transfer ? (host->dsp) */
	if (
		((hostport[CPU_HOST_ISR] & (1<<CPU_HOST_ISR_TXDE))==0) &&
		((periph[SPACE_X][DSP_HOST_HSR] & (1<<DSP_HOST_HSR_HRDF))==0)
		) {

		periph[SPACE_X][DSP_HOST_HRX] = hostport[CPU_HOST_TXL];
		periph[SPACE_X][DSP_HOST_HRX] |= hostport[CPU_HOST_TXM]<<8;
		periph[SPACE_X][DSP_HOST_HRX] |= hostport[CPU_HOST_TXH]<<16;

#if DSP_DISASM_HOSTREAD
		D(bug("Dsp: (H->D): Transfer 0x%06x",periph[SPACE_X][DSP_HOST_HRX]));
#endif

		/* Set HRDF bit to say that DSP can read */
		periph[SPACE_X][DSP_HOST_HSR] |= 1<<DSP_HOST_HSR_HRDF;
#if DSP_DISASM_HOSTREAD
		D(bug("Dsp: (H->D): Dsp HRDF set"));
#endif

		/* Set TXDE bit to say that host can write */
		hostport[CPU_HOST_ISR] |= 1<<CPU_HOST_ISR_TXDE;
#if DSP_DISASM_HOSTREAD
		D(bug("Dsp: (H->D): Host TXDE set"));
#endif

		/* Clear/set TRDY bit */
		hostport[CPU_HOST_ISR] &= 0xff-(1<<CPU_HOST_ISR_TRDY);
		trdy = (hostport[CPU_HOST_ISR]>>CPU_HOST_ISR_TXDE) & 1;
		trdy &= !((periph[SPACE_X][DSP_HOST_HSR]>>DSP_HOST_HSR_HRDF) & 1);
		hostport[CPU_HOST_ISR] |= (trdy & 1)<< CPU_HOST_ISR_TRDY;
	}
}

void DSP::dsp2host(void)
{
	/* Host port transfer ? (dsp->host) */
	if (
		((hostport[CPU_HOST_ISR] & (1<<CPU_HOST_ISR_RXDF))==0) &&
		((periph[SPACE_X][DSP_HOST_HSR] & (1<<DSP_HOST_HSR_HTDE))==0)
		) {

		hostport[CPU_HOST_RXL] = periph[SPACE_X][DSP_HOST_HTX];
		hostport[CPU_HOST_RXM] = periph[SPACE_X][DSP_HOST_HTX]>>8;
		hostport[CPU_HOST_RXH] = periph[SPACE_X][DSP_HOST_HTX]>>16;

#if DSP_DISASM_HOSTWRITE
		D(bug("Dsp: (D->H): Transfer 0x%06x",periph[SPACE_X][DSP_HOST_HTX]));
#endif

		/* Set HTDE bit to say that DSP can write */
		periph[SPACE_X][DSP_HOST_HSR] |= 1<<DSP_HOST_HSR_HTDE;
#if DSP_DISASM_HOSTWRITE
		D(bug("Dsp: (D->H): Dsp HTDE set"));
#endif

		/* Set RXDF bit to say that host can read */
		hostport[CPU_HOST_ISR] |= 1<<CPU_HOST_ISR_RXDF;
#if DSP_DISASM_HOSTWRITE
		D(bug("Dsp: (D->H): Host RXDF set"));
#endif
	}
}

#endif	/* DSP_EMULATION */

/*
	2002-09-20:PM
		Constructor/destructor for DSP C++ class
	2002-08-28:PM
		BUG:host port read not working with thread
	2002-08-27:PM
		BUG: Fixes in thread support
		Host port transfer routines moved in dsp.cpp
	2002-08-26:PM
		Added thread support
	2002-07-31:PM
		FIX:host port TRDY bit now correctly set
	2002-07-22:PM
		FIX:mu-law generator added, periph init also moved to reset function
	2002-07-19:PM
		FIX:stack and registers init moved to reset function
*/
