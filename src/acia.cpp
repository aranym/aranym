/* Joy 2001 */

#include "sysdeps.h"
#include "hardware.h"
#include "cpu_emulation.h"
#include "memory.h"
#include "acia.h"

int HW = 0xfffc00;

ACIA::ACIA(bool value) {
	midi = value;

	status = 0x00;
	mode = 0x00;
	rxdata = 0x00;
	txdata = 0x00;
}

uae_u8 ACIA::getStatus() {
	return status;
}

void ACIA::setMode(uae_u8 value) {
}

uae_u8 ACIA::getData() {
	return 0xa2;	/* taken from Stonx:ikbd.c */
}

void ACIA::setData(uae_u8 value) {
	/* send data */
	status |= 2;
}
