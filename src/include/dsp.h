/* Joy 2001 */

#include "icio.h"

#define HW_DSP 0xffa200
#define DSP_RAMSIZE 32768

/* Dsp State */
#define DSP_BOOTING 0
#define DSP_RUNNING 1
#define DSP_WAITHOST 2
#define DSP_WAITINTERRUPT 3

/* Host port, CPU side */		/* Mapped to these reserved space */
#define CPU_HOST_ICR	0x00	/* x:ffc0 */
#define CPU_HOST_CVR	0x01	/* x:ffc1 */
#define CPU_HOST_ISR	0x02	/* x:ffc2 */
#define CPU_HOST_IVR	0x03	/* x:ffc3 */
#define CPU_HOST_RX0	0x04
#define CPU_HOST_RXH	0x05
#define CPU_HOST_RXM	0x06
#define CPU_HOST_RXL	0x07
#define CPU_HOST_TX0	0x04
#define CPU_HOST_TXH	0x05
#define CPU_HOST_TXM	0x06
#define CPU_HOST_TXL	0x07

#define HOST_ICR_RREQ	0x00
#define HOST_ICR_TREQ	0x01
#define HOST_ICR_HF0	0x03
#define HOST_ICR_HF1	0x04
#define HOST_ICR_HM0	0x05
#define HOST_ICR_HM1	0x06
#define HOST_ICR_INIT	0x07

#define HOST_CVR_HC		0x07

#define HOST_ISR_RXDF	0x00
#define HOST_ISR_TXDE	0x01
#define HOST_ISR_TRDY	0x02
#define HOST_ISR_HF2	0x03
#define HOST_ISR_HF3	0x04
#define HOST_ISR_DMA	0x06
#define HOST_ISR_HREQ	0x07

/* Host port, DSP side */
#define PERIPH_HOST_HCR 0x28	/* x:ffe8 */
#define PERIPH_HOST_HSR 0x29	/* x:ffe9 */
#define PERIPH_HOST_HRX 0x2b	/* x:ffeb */
#define PERIPH_HOST_HTX 0x2b	/* x:ffeb */

#define HOST_HCR_HRIE	0x00
#define HOST_HCR_HTIE	0x01
#define HOST_HCR_HCIE	0x02
#define HOST_HCR_HF2	0x03
#define HOST_HCR_HF3	0x04

#define HOST_HSR_HRDF	0x00
#define HOST_HSR_HTDE	0x01
#define HOST_HSR_HCP	0x02
#define HOST_HSR_HF0	0x03
#define HOST_HSR_HF1	0x04
#define HOST_HSR_DMA	0x07

class DSP : public BASE_IO {
	
	public:
		void init(void);
		virtual uae_u8 handleRead(uaecptr addr);
		virtual void handleWrite(uaecptr, uae_u8);

		/* DSP state */
		uint8	state;

		/* Registers */
		uint16	pc;
		uint32	registers[64];

		/* stack[0=ssh], stack[1=ssl] */
		uint16	stack[2][15];

		/* ram[0] is x:, ram[1] is y:, ram[2] is p: */
		uint32	ram[3][DSP_RAMSIZE];

		/* rom[0] is x:, rom[1] is y: */
		uint32	rom[2][512];

		/* peripheral space, [x|y]:0xffc0-0xffff */
		uint32	periph[2][64];

		/* Misc */
		uint32 loop_rep;	/* executing rep ? */

	private:
		/* For bootstrap routine */
		uint16	bootstrap_pos;
		uint32	bootstrap_accum;
};

