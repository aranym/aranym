/*
 *	ACIA 6850 emulation
 *
 *	Joy 2001
 *	Patrice Mandin
 */

#ifndef _ACIA_H
#define _ACIA_H

#include "icio.h"

/*--- ACIA CR register ---*/

/* Clock predivisor, bits 0-1 */
#define ACIA_CR_PREDIV_MASK		0x03

#define ACIA_CR_PREDIV1		0x00	/* Nominal clock = 500 Khz */
#define ACIA_CR_PREDIV16	0x01	/* Clock/16 = 31250 Hz */
#define ACIA_CR_PREDIV64	0x02	/* Clock/64 = 7812.5 Hz */
#define ACIA_CR_RESET		0x03	/* Master reset */

/* Format, bits 2-4 */
#define ACIA_CR_PARITY		0x02	/* 0=parity, 1=no parity */
#define ACIA_CR_STOPBITS	0x03	/* 0=2 stop bits, 1=1 stop bits */
#define ACIA_CR_DATABITS	0x04	/* 0=7 data bits, 1=8 data bits */

/* Emission control, bits 5-6 */
#define ACIA_CR_EMIT_RTS	0x05	/* 0=RTS low, 1=RTS high */
#define ACIA_CR_EMIT_INTER	0x06	/* 0=interrupt disabled, 1=interrupt enabled */

/* Reception control, bit 7 */
#define ACIA_CR_REC_INTER	0x07	/* 0=interrupt disabled, 1=interrupt enabled */

/*--- ACIA SR register ---*/
#define ACIA_SR_RXFULL		0x00	/* Receive full */
#define ACIA_SR_TXEMPTY		0x01	/* Transmit empty */
#define ACIA_SR_CD			0x02	/* Carrier detect */
#define ACIA_SR_CTS			0x03	/* Clear to send */
#define ACIA_SR_FRAMEERR	0x04	/* Frame error */
#define ACIA_SR_OVERRUN		0x05	/* Overrun */
#define ACIA_SR_PARITYERR	0x06	/* Parity error */
#define ACIA_SR_INTERRUPT	0x07	/* Interrupt source */

/*--- ACIA class ---*/
class ACIA : public BASE_IO {
	protected:
		uae_u8 sr;		/* Status register */
		uae_u8 cr;		/* Control register */
		uae_u8 rxdr;	/* Reception data register */
		uae_u8 txdr;	/* Transmit data register */

	public:
		ACIA(memptr, uint32);
		~ACIA();
		virtual void reset(void);

		virtual uae_u8 handleRead(uaecptr addr);
		virtual void handleWrite(uaecptr addr, uae_u8 value);

		virtual uae_u8 ReadStatus();
		virtual void WriteControl(uae_u8 value);
		virtual uae_u8 ReadData();
		virtual void WriteData(uae_u8);

		void PrintControlRegister(char *devicename, uae_u8 value);
};

#endif /* _ACIA_H */
