/*
 * scc.cpp - SCC 85C30 emulation code (preliminary)
 *
 * Copyright (c) 2001-2004 Petr Stehlik of ARAnyM dev team (see AUTHORS)
 *				 2010 Jean Conter
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

#include "sysdeps.h"
#include "hardware.h"
#include "cpu_emulation.h"
#include "memory-uae.h"
#include "scc.h"
#include "parameters.h"

#include "serial.h"
#ifdef ENABLE_SERIALUNIX
#include "serial_port.h"
#endif

#define DEBUG 0
#include "debug.h"


SCC::SCC(memptr addr, uint32 size):BASE_IO(addr, size)
{
	if (bx_options.serial.enabled && bx_options.serial.serport[0])
	{
#ifdef ENABLE_SERIALUNIX
		serial = new Serialport(bx_options.serial.serport);
#else
		serial = new Serial;
#endif
	} else
	{
		serial = new Serial;
	}
	reset();
}

SCC::~SCC()
{
	delete serial;
}

void SCC::reset()
{
	RR3 = 0;
	RR3M = 0;
	A.reset(0);
	B.reset(1);
	serial->reset();
}

void SCC::SCCchannel::reset(int num)
{
	channel = num;
	active_reg = 0;
	charcount = 0;
	for (unsigned int i = 0; i < sizeof(regs) / sizeof(regs[0]); i++)
		regs[i] = 0;
	regs[15] = 0xF8;
	regs[14] = 0xA0;
	regs[11] = 0x08;
	regs[9] = 0;
	regs[0] = SCC_TBE;				//RR0A
}


uint8 SCC::handleRead(memptr addr)
{
	uint8 value = 0;
	uint16 temp;

	addr -= getHWoffset();
	SCCchannel &channel = addr >= 4 ? B : A;
	switch (addr)
	{
	case 0:							// channel A
	case 4:							// channel B
		switch (channel.active_reg)
		{
		case 0:
			if (channel.channel)
			{							// RR0B
				temp = serial->getStatus(channel.charcount);	// only for channel B
				channel.regs[0] = temp & 0xFF;	// define CTS(5), SCC_TBE(2) and RBF=RCA(0)
				RR3 |= RR3M & (temp >> 8);	// define RxIP(2), TxIP(1) and ExtIP(0)
				value = channel.regs[0];
			} else
			{
				channel.regs[0] = SCC_TBE;
				if (channel.regs[9] == SCC_INTACK)
					RR3 |= RR3M & SCCA_CTS_INT;
				value = channel.regs[0];		// not yet defined for channel A
			}
			break;

		case 2:
			/*
			 * RR2 contains the interrupt vector written into WR2. When
			 * the register is accessed in Channel A, the vector returned
			 * is the vector actually stored in WR2. When this register is
			 * accessed in Channel B, the vector returned includes
			 * status information in bits 1, 2 and 3 or in bits 6, 5 and 4,
			 * depending on the state of the Status High/Status Low bit
			 * in WR9 and independent of the state of the VIS bit in
			 * WR9. The vector is modified according to Table 5-6
			 * shown in the explanation of the VIS bit in WR9 (Section
			 * 5.2.11). If no interrupts are pending, the status is
			 * V3,V2,V1 -011, or V6,V5,V4-110. Figure 5-21 shows the
			 * bit positions for RR2.
			 */

			value = channel.regs[2];
			if (channel.channel == 0)
				break;					// vector base only for RR2A
			if ((A.regs[9] & SCC_VIS) == 0)
				break;					// no status bit added
			// status bit added to vector
			if (A.regs[9] & SCC_STATUS)
			{							// modify high bits 4-6
				if (RR3 == 0)
				{
					value |= 0x60;
					break;
				}
				if (RR3 & SCCA_RCV_INT)
				{
					value |= 0x30;
					break;
				}
				if (RR3 & SCCA_TXR_INT)
				{
					value |= 0x10;
					break;
				}
				if (RR3 & SCCA_CTS_INT)
				{
					value |= 0x50;
					break;
				}
				if (RR3 & SCCB_RCV_INT)
				{
					value |= 0x20;
					break;
				}
				if (RR3 & SCCB_TXR_INT)
					break;
				if (RR3 & SCCB_CTS_INT)
					value |= 0x40;
			} else
			{							// modify low bits 1-3
				if (RR3 == 0)
				{
					value |= 6;
					break;
				}
				if (RR3 & SCCA_RCV_INT)
				{
					value |= 0xC;
					break;
				}
				if (RR3 & SCCA_TXR_INT)
				{
					value |= 0x8;
					break;
				}
				if (RR3 & SCCA_CTS_INT)
				{
					value |= 0xA;
					break;
				}
				if (RR3 & SCCB_RCV_INT)
				{
					value |= SCCB_TXR_INT;
					break;
				}
				if (RR3 & SCCB_CTS_INT)
					break;
				if (RR3 & SCCB_CTS_INT)
					value |= 2;
			}
			break;

		case 3:
			value = channel.channel ? 0 : RR3;	//access on A channel only
			break;

		case 4:
			/*
			 * On the ESCC, Read Register 4 reflects the contents of
			 * Write Register 4 provided the Extended Read option is en-
			 * abled. Otherwise, this register returns an image of RR0.
			 * On the NMOS/CMOS version, a read to this location re-
			 * turns an image of RR0.
			 */
			value = channel.regs[0];
			break;

		case 8:
			/* RR8 is the Receive Data register */
			if (channel.channel)
			{							// only channel B processed
				channel.regs[8] = serial->getData();
				value = channel.regs[8];
			} else
			{
				value = channel.regs[8];
			}
			break;

		case 9:
			/*
			 * On the ESCC, Read Register 9 reflects the contents of
			 * Write Register 3 provided the Extended Read option has
			 * been enabled.
			 * On the NMOS/CMOS version, a read to this location re-
			 * turns an image of RR13.
			 */
			value = channel.regs[13];
			break;

		case 11:
			/*
			 * On the ESCC, Read Register 11 reflects the contents of
			 * Write Register 10 provided the Extended Read option has
			 * been enabled. Otherwise, this register returns an image of
			 * RR15.
			 * On the NMOS/CMOS version, a read to this location re-
			 * turns an image of RR15.
			 */
		case 15:						// EXT/STATUS IT Ctrl
			/*
			 * RR15 reflects the value stored in WR15, the
			 * External/Status IE bits. The two unused bits are always
			 * returned as Os.
			 */
			value = channel.regs[15] &= 0xFA;	// mask out D2 and D0
			break;

		case 12:
			/*
			 * RR12 returns the value stored in WR12, the lower byte of
			 * the time constant, for the BRG.
			 */
		case 13:
			/*
			 * RR13 returns the value stored in WR13, the upper byte of
			 * the time constant for the BRG.
			 */
			value = channel.regs[channel.active_reg];
			break;

		case 1:
			/*
			 * RR1 contains the Special Receive Condition status bits
			 * and the residue codes for the l-field in SDLC mode.
			 */
		case 5:
			/*
			 * On the ESCC, Read Register 5 reflects the contents of
			 * Write Register 5 provided the Extended Read option is en-
			 * abled. Otherwise, this register returns an image of RR1.
			 * On the NMOS/CMOS version, a read to this register re-
			 * turns an image of RR1.
			 */
		case 6:
			/*
			 * On the CMOS and ESCC, Read Register 6 contains the
			 * least significant byte of the frame byte count that is currently
			 * at the top of the Status FIFO. This register is readable only
			 * if the FIFO is enabled. Otherwise, this register is an image of RR2.
			 * On the NMOS version, a read to this register location re-
			 * turns an image of RR2.
			 */
		case 7:
			/*
			 * On the CMOS and ESCC, Read Register 7 contains the
			 * most significant six bits of the frame byte count that is
			 * currently at the top of the Status FIFO. Bit D7 is the FIFO
			 * Overflow Status and bit D6 is the FIFO Data Available
			 * Status. This register is readable only if the FIFO
			 * is enabled. Otherwise this register is an image of RR3. Note,
			 * for proper operation of the FIFO and byte count logic, the
			 * registers should be read in the following order: RR7, RR6,
			 * RR1.
			 */
		case 10:
			/*
			 * RR10 contains some miscellaneous status bits. Unused
			 * bits are always 0.
			 */
			D(bug("scc : unprocessed register %d read *********", channel.active_reg));
			value = 0;
			break;
		default:
			panicbug("scc : illegal register %d read *********", channel.active_reg);
			value = 0;
			break;
		}
		break;
	case 2:							// channel A
		value = A.regs[8];			// TBD (LAN)
		break;
	case 6:							// channel B
		B.regs[8] = serial->getData();
		value = B.regs[8];
		break;
	default:
		D(bug("scc : illegal read address=$%x", addr + getHWoffset()));
		break;
	}
	channel.active_reg = 0;						// next access for RR0 or WR0
/*	D(bug("HWget_b(0x%08x)=0x%02x at 0x%08x", addr + getHWoffset(), value, showPC()));*/
	return value;
}

void SCC::handleWrite(memptr addr, uint8 value)
{
	int i;
	uint32 BaudRate;

	addr -= getHWoffset();
	SCCchannel &channel = addr >= 4 ? B : A;
	SCCchannel &other_channel = addr >= 4 ? A : B;
	switch (addr)
	{
	case 0:
	case 4:
		switch (channel.active_reg)
		{
		case 0:
			/*
			 * Theoretically, only  bits 2-0 are used to change the register point.
			 * However, the point-high-command written to bits 5-3 that is used
			 * to select the remaining 8 registers is 001, so we can just use
			 * bits 3-0 as register select
			 */
			if (value <= 15)
			{
				channel.active_reg = value & 0x0f;
			} else
			{
				if ((value & 0x38) == 0x38)
				{						// Reset Highest IUS (last operation in IT service routine)
					for (i = SCCA_RCV_INT; i; i >>= 1)
					{
						if (RR3 & i)
							break;
					}
#define UGLY
#ifdef UGLY
					// tricky & ugly speed improvement for input
					if (i == SCCB_RCV_INT)
					{					// RxIP
						B.charcount--;
						if (B.charcount <= 0)
							RR3 &= ~SCCB_RCV_INT;	// optimize input; don't reset RxIP when chars are buffered
					} else
#endif
					{
						RR3 &= ~i;
					}
				} else if ((value & 0x38) == 0x28)
				{						// Reset Tx int pending
					if (channel.channel)
						RR3 &= ~SCCB_TXR_INT;		// channel B
					else
						RR3 &= ~SCCA_TXR_INT;		// channel A
				} else if ((value & 0x38) == 0x10)
				{									// Reset Ext/Status ints
					if (channel.channel)
						RR3 &= ~SCCB_CTS_INT;		// channel B
					else
						RR3 &= ~SCCA_CTS_INT;		// channel A
				}
				TriggerSCC((RR3 & RR3M) && (((SCC_MIE|SCC_NV|SCC_VIS) & A.regs[9]) == (SCC_MIE|SCC_VIS)));	// clear SCC flag if no pending IT
				// or no properly configured WR9
				// must be done here to avoid scc_do_Interrupt call without pending IT
			}
			break;

		case 2:
			channel.regs[channel.active_reg] = value;
			// single WR2 on SCC
			other_channel.regs[channel.active_reg] = value;
			channel.active_reg = 0;				// next access for RR0 or WR0
			break;

		case 8:
			channel.regs[channel.active_reg] = value;
			if (channel.channel)
				serial->setData(value);	// channel B only
			// channel A to be done if necessary
			channel.active_reg = 0;				// next access for RR0 or WR0
			break;

		case 1:
			// Tx/Rx interrupt enable
			channel.regs[channel.active_reg] = value;
			if (channel.channel)
			{
				// channel B
				if (value & 1) /* ext int enable */
					RR3M |= SCCB_CTS_INT;
				else
					RR3 &= ~SCCB_CTS_INT;
				if (value & 2) /* Tx int enable */
					RR3M |= SCCB_TXR_INT;
				else
					RR3 &= ~SCCB_TXR_INT;
				if (value & 0x18) /* Rx int enable */
					RR3M |= SCCB_RCV_INT;
				else
					RR3 &= ~SCCB_RCV_INT;
				//  set or clear SCC flag if necessary (see later)
			} else
			{
				// channel A
				if (value & 1) /* ext int enable */
					RR3M |= SCCA_CTS_INT;
				else
					RR3 &= ~SCCA_CTS_INT;		// no IP(RR3) if not enabled(RR3M)
				if (value & 2) /* Tx int enable */
					RR3M |= SCCA_TXR_INT;
				else
					RR3 &= ~SCCA_TXR_INT;
				if (value & 0x18) /* Rx int enable */
					RR3M |= SCCA_RCV_INT;
				else
					RR3 &= ~SCCA_RCV_INT;
			}
			channel.active_reg = 0;				// next access for RR0 or WR0
			break;

		case 5:
			// Transmit parameter and control
			channel.regs[channel.active_reg] = value;
			if (channel.channel)
			{
				serial->setRTS(value & 2);
				serial->setDTR(value & 128);
			}
			// Tx character format & Tx CRC would be selected also here (8 bits/char and no CRC assumed)

			channel.active_reg = 0;				// next access for RR0 or WR0
			break;

		case 9:
			// Master interrupt control
			channel.regs[channel.active_reg] = value & ~(SCC_RESETA | SCC_RESETB);
			// single WR9 (accessible by both channels)
			other_channel.regs[channel.active_reg] = value & ~(SCC_RESETA | SCC_RESETB);
			if (value & SCC_RESETB)
			{
				B.reset(1);
				RR3 &= ~(SCCB_CTS_INT | SCCB_TXR_INT | SCCB_RCV_INT);
				RR3M &= ~(SCCB_CTS_INT | SCCB_TXR_INT | SCCB_RCV_INT);
			}
			if (value & SCC_RESETA)
			{
				A.reset(0);
				RR3 &= ~(SCCA_CTS_INT | SCCA_TXR_INT | SCCA_RCV_INT);
				RR3M &= ~(SCCA_CTS_INT | SCCA_TXR_INT | SCCA_RCV_INT);
			}
			//  set or clear SCC flag accordingly (see later)

			channel.active_reg = 0;				// next access for RR0 or WR0
			break;

		case 12:
			/*
			 * WR12 contains the lower byte of the time constant for the
			 * baud rate generator.
			 */
			channel.regs[channel.active_reg] = value;
			channel.active_reg = 0;				// next access for RR0 or WR0
			break;

		case 13:
			// set baud rate according to WR13 and WR12
			channel.regs[channel.active_reg] = value;
			// Normally we have to set the baud rate according
			// to clock source (WR11) and clock mode (WR4)
			// In fact, we choose the baud rate from the value stored in WR12 & WR13
			// Note: we assume that WR13 is always written last (after WR12)
			// we tried to be more or less compatible with HSMODEM (see below)
			// 75 and 50 bauds are preserved because 153600 and 76800 were not available
			// 3600 and 2000 were also unavailable and are remapped to 57600 and 38400 respectively
			BaudRate = 0;
			switch (value)
			{
			case 0:
				switch (channel.regs[12])
				{
				case 0:			// HSMODEM for 200 mapped to 230400
					BaudRate = 230400;
					break;
				case 2:			// HSMODEM for 150 mapped to 115200
					BaudRate = 115200;
					break;
				case 6:			// HSMODEM for 134 mapped to 57600
				case 0x7e:		// HSMODEM for 3600 remapped to 57600
				case 0x44:		// normal for 3600 remapped to 57600
					BaudRate = 57600;
					break;
				case 0xa:		// HSMODEM for 110 mapped to 38400
				case 0xe4:		// HSMODEM for 2000 remapped to 38400
				case 0x7c:		// normal for 2000 remapped to 38400
					BaudRate = 38400;
					break;
				case 0x16:		// HSMODEM for 19200
				case 0xb:		// normal for 19200
					BaudRate = 19200;
					break;
				case 0x2e:		// HSMODEM for 9600
				case 0x18:		// normal for 9600
					BaudRate = 9600;
					break;
				case 0x5e:		// HSMODEM for 4800
				case 0x32:		// normal for 4800
					BaudRate = 4800;
					break;
				case 0xbe:		// HSMODEM for 2400
				case 0x67:		// normal
					BaudRate = 2400;
					break;
				case 0xfe:		// HSMODEM for 1800
				case 0x8a:		// normal for 1800
					BaudRate = 1800;
					break;
				case 0xd0:		// normal for 1200
					BaudRate = 1200;
					break;
				case 1:			// HSMODEM for 75 kept to 75
					BaudRate = 75;
					break;
				case 4:			// HSMODEM for 50 kept to 50
					BaudRate = 50;
					break;
				default:
					D(bug("unexpected LSB constant for baud rate"));
					break;
				}
				break;
			case 1:
				switch (channel.regs[12])
				{
				case 0xa1:		// normal for 600
					BaudRate = 600;
					break;
				case 0x7e:		// HSMODEM for 1200
					BaudRate = 1200;
					break;
				}
				break;
			case 2:
				if (channel.regs[12] == 0xfe)
					BaudRate = 600;	// HSMODEM
				break;
			case 3:
				if (channel.regs[12] == 0x45)
					BaudRate = 300;	// normal
				break;
			case 4:
				if (channel.regs[12] == 0xe8)
					BaudRate = 200;	// normal
				break;
			case 5:
				if (channel.regs[12] == 0xfe)
					BaudRate = 300;	// HSMODEM
				break;
			case 6:
				if (channel.regs[12] == 0x8c)
					BaudRate = 150;	// normal
				break;
			case 7:
				if (channel.regs[12] == 0x4d)
					BaudRate = 134;	// normal
				break;
			case 8:
				if (channel.regs[12] == 0xee)
					BaudRate = 110;	// normal
				break;
			case 0xd:
				if (channel.regs[12] == 0x1a)
					BaudRate = 75;	// normal
				break;
			case 0x13:
				if (channel.regs[12] == 0xa8)
					BaudRate = 50;	// normal
				break;
			case 0xff:				// HSMODEM dummy value->silently ignored
				break;
			default:
				D(bug("unexpected MSB constant for baud rate"));
				break;
			}
			if (BaudRate)
				serial->setBaud(BaudRate);	// set only if defined

/* summary of baud rates:

Rsconf    Falcon    Falcon(+HSMODEM)          Aranym      Aranym(+HSMODEM)
0          19200         19200                 19200       19200
1           9600          9600                  9600        9600
2           4800          4800                  4800        4800
3           3600          3600                 57600       57600
4           2400          2400                  2400        2400
5           2000          2000                 38400       38400
6           1800          1800                  1800        1800
7           1200          1200                  1200        1200
8            600           600                   600         600
9            300           300                   300         300
10           200        230400                   200      230400
11           150        115200                   150      115200
12           134         57600                   134       57600
13           110         38400                   110       38400
14            75        153600                    75          75
15            50         76800                    50          50

*/
			channel.active_reg = 0;				// next access for RR0 or WR0
			break;

		case 15:
			// external status int control
			channel.regs[channel.active_reg] = value;
			if (value & 1)
			{
				D(bug("SCC WR7 prime not yet processed"));
			}
			channel.active_reg = 0;				// next access for RR0 or WR0
			break;

			if (channel.active_reg == 1 || channel.active_reg == 2 || channel.active_reg == 9)
				TriggerSCC((RR3 & RR3M) && (((SCC_MIE|SCC_NV|SCC_VIS) & A.regs[9]) == (SCC_MIE|SCC_VIS)));	// set or clear SCC flag accordingly. Yes it's ugly but avoids unnecessary useless calls
			channel.active_reg = 0;				// next access for RR0 or WR0
			break;

		case 3:
		case 4:
		case 6:
		case 7:
		case 10:
		case 11:
			channel.regs[channel.active_reg] = value;
			D(bug("scc : unprocessed register %d read *********", channel.active_reg));
			channel.active_reg = 0;				// next access for RR0 or WR0
			break;

		default:
			panicbug("scc : illegal register %d written *********", channel.active_reg);
			channel.active_reg = 0;				// next access for RR0 or WR0
			break;
		}
		break;

	case 2:							// channel A to be done if necessary
		break;

	case 6:							// channel B
		serial->setData(value);
		break;

	default:
		D(bug("scc : illegal write address =$%x", addr + getHWoffset()));
		break;
	}

/*	D(bug("HWput_b(0x%08x,0x%02x) at 0x%08x", addr+HW, value, showPC()));*/
}

void SCC::IRQ(void)
{
	uint16 temp;

	temp = serial->getStatus(B.charcount);
	if (A.regs[9] == SCC_INTACK)
		temp |= SCCA_CTS_INT << 8;		// fake ExtStatusChange for HSMODEM install
	B.regs[0] = temp & 0xFF;			// RR0B
	RR3 = RR3M & (temp >> 8);
	if (RR3 && ((A.regs[9] & (SCC_MIE|SCC_NV|SCC_VIS)) == (SCC_MIE|SCC_VIS)))
		TriggerSCC(true);
}


// return : vector number, or zero if no interrupt
int SCC::doInterrupt()
{
	int vector;
	uint8 i;

	for (i = SCCA_RCV_INT; i; i >>= 1)
	{									// highest priority first
		if (RR3 & i & RR3M)
			break;
	}
	vector = A.regs[2];					//WR2 = base of vectored interrupts for SCC
	if ((A.regs[9] & (SCC_NV|SCC_VIS)) == 0)
		return vector;					// no status included in vector
	if ((A.regs[9] & (SCC_NV|SCC_STATUS|SCC_INTACK)) != 0)
	{									//shouldn't happen with TOS, (to be completed if needed)
		D(bug("unexpected WR9 contents"));
		// no Soft IACK, Status Low control bit expected, no NV
		return 0;
	}
	switch (i)
	{
	case 0:							/* this shouldn't happen :-) */
		D(bug("scc_do_interrupt called with no pending interrupt"));
		vector = 0;						// cancel
		break;
	case SCCB_CTS_INT:
		vector |= 2;					// Ch B Ext/status change
		break;
	case SCCB_TXR_INT:
		break;							// Ch B Transmit buffer Empty
	case SCCB_RCV_INT:
		vector |= 4;					// Ch B Receive Char available
		break;
	case SCCA_CTS_INT:
		vector |= 0xA;					// Ch A Ext/status change
		break;
	case SCCA_TXR_INT:
		vector |= 8;					// Ch A Transmit Buffer Empty
		break;
	case SCCA_RCV_INT:
		vector |= 0xC;					// Ch A Receive Char available
		break;
		// special receive condition not yet processed
	}
#if 0
	D(bug("SCC::doInterrupt : vector %d", vector));
#endif
	return vector;
}
