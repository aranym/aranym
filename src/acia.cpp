/* Joy 2001 */

#include "sysdeps.h"
#include "hardware.h"
#include "cpu_emulation.h"
#include "memory.h"
#include "acia.h"

static const int HW = 0xfffc00;

static bool dP = false;

ACIA::ACIA(uaecptr addr) {
	baseaddr = addr;
}

uae_u8 ACIA::handleRead(uaecptr addr) {
	addr -= baseaddr;
	if (addr == 0)
		return getStatus();
	else if (addr == 2)
		return getData();
	else
		return 0;
}

void ACIA::handleWrite(uaecptr addr, uae_u8 value) {
	addr -= baseaddr;
	if (addr == 0)
		setMode(value);
	else if (addr == 2)
		setData(value);
}

/*******************************/
IKBD::IKBD() : ACIA(0xfffc00) {
	status = 0x0e;
	ikbd_inbuf = ikbd_bufpos = 0;
};

uae_u8 IKBD::getStatus() {
	return status | 0x02;
}

void IKBD::setMode(uae_u8 value) {
}

uae_u8 IKBD::getData() {
	int pos = (ikbd_bufpos - ikbd_inbuf)&MAXBUF;
	if (ikbd_inbuf-- > 0)
	{
		if (ikbd_inbuf ==0) {
			/* Clear GPIP/I4 */
			status = 0;
			uae_u8 x = get_byte_direct(0xfffa01);
			x |= 0x10;
			put_byte_direct(0xfffa01,x);
		}
		if (dP)
			fprintf(stderr, "IKBD read code %2x (%d left)\n", buffer[pos], ikbd_inbuf);
		return buffer[pos];
	}
	else {
		ikbd_inbuf = 0;
		return 0xa2;
	}
}

void IKBD::ikbd_send(int value)
{
	int pos;
	uae_u8 x;
	value &= 0xff;
	if (ikbd_inbuf <= MAXBUF)
	{
		buffer[ikbd_bufpos] = value;
		ikbd_bufpos++;
		ikbd_bufpos &= MAXBUF;
		ikbd_inbuf++;
	}
//	if ((LM_UB(MEM(0xfffa09)) & 0x40) == 0) return;
	if (dP)
		fprintf(stderr, "IKBD sends %2x (->buffer pos %d)\n", value, ikbd_bufpos-1);
	/* set Interrupt Request */
	status |= 0x81;
	/* signal ACIA interrupt */
	MakeMFPIRQ(6);
#if 0
	flags |= F_ACIA;
	/* IPRB/I4 is not set at all */
	/* GPIP/I4 */
	x = LM_UB(MEM(0xfffa01));
	x &= ~0x10;
	SM_UB(MEM(0xfffa01),x);
	/* ISRB/I4 */
	x = LM_UB(MEM(0xfffa11));
	x |= 0x40;
	SM_UB(MEM(0xfffa11),x);
#endif
}

void IKBD::setData(uae_u8 value) {
	/* send data */
	if (dP)
		fprintf(stderr, "IKBD data = %02x\n", value);
}
