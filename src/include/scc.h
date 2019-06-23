/*
 * scc.h - 85C30 emulation code - declaration
 *
 * Copyright (c) 2001-2004 Petr Stehlik of ARAnyM dev team (see AUTHORS)
 * 
 * This file is part of the ARAnyM project which builds a new and powerful
 * TOS/FreeMiNT compatible virtual machine running on almost any hardware.
 *
 * ARAnyM is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * ARAnyM is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ARAnyM; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef SCC_H
#define SCC_H

#include "icio.h"

#include "serial.h"
#include "serial_port.h"

/*
 * bits in RR0 (transmit/receive/extstatus)
 */
#define SCC_RCA   (1 << 0)	/* Rx character available */
#define SCC_ZERO  (1 << 1)	/* zero count */
#define SCC_TBE   (1 << 2)	/* Tx buffer empty */
#define SCC_DCD   (1 << 3)	/* data carrier detect */
#define SCC_SYNC  (1 << 4)	/* sync/hunt */
#define SCC_CTS   (1 << 5)	/* clear to send */
#define SCC_EOM   (1 << 6)	/* Tx underrun/EOM */
#define SCC_BREAK (1 << 7)	/* Break/Abort */

/*
 * bits in RR3 (interrupt pending register)
 * also used as interrupt mask
 */
#define SCCB_CTS_INT (1 << 0) /* channel B status change */
#define SCCB_TXR_INT (1 << 1) /* channel B transmitter int */
#define SCCB_RCV_INT (1 << 2) /* channel B receiver int */
#define SCCA_CTS_INT (1 << 3) /* channel A status change */
#define SCCA_TXR_INT (1 << 4) /* channel A transmitter int */
#define SCCA_RCV_INT (1 << 5) /* channel A receiver int */

/*
 * bits in WR9 (Master Interrupt control)
 */
#define SCC_RESETA  (1 << 7) /* Reset channel A */
#define SCC_RESETB  (1 << 6) /* Reset channel B */
#define SCC_INTACK  (1 << 5) /* Software INTACK Enable */
#define SCC_STATUS  (1 << 4) /* Status High/Status Low */
#define SCC_MIE     (1 << 3) /* Master Interrupt Enable */
#define SCC_DLC     (1 << 2) /* Disable Lower chain */
#define SCC_NV      (1 << 1) /* No Vector select */
#define SCC_VIS     (1 << 0) /* Vector Includes Status */


class SCC : public BASE_IO {
private:
	class SCCchannel {
	public:
		int channel;
		int active_reg;
		/*
		 * only contents of write registers are kept,
		 * read registers are emulated
		 */
		uint8 regs[16];
		int charcount;

		void reset(int num);
	};
	SCCchannel A, B;
	uint8 RR3, RR3M;     //jc common to channel A & B

public:
	SCC(memptr, uint32);
	~SCC();
	void reset();
	virtual uint8 handleRead(memptr addr);
	virtual void handleWrite(memptr addr, uint8 value);
	void IRQ(void);
	int doInterrupt(void);

	Serial *serial;
};

#endif /* SCC_H */
