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

#include "dsp.h"

#define DEBUG 0
#include "debug.h"


SCC::SCC(memptr addr, uint32 size) : BASE_IO(addr, size)
{
#ifdef ENABLE_SERIALUNIX
	serial = new Serialport;
#else
	serial = new Serial;
#endif
	reset();
}

SCC::~SCC()
{
	delete serial;
}

void SCC::reset()
{
	active_reg = 0;
	for(unsigned int i=0; i<sizeof(scc_regs)/sizeof(scc_regs[0]); i++)
		scc_regs[i] = 0;
	channelAreset();
	channelBreset();
	RR3=0;
	RR3M=0;
	charcount=0;
	serial->reset();
}

void SCC::channelAreset()
{
	scc_regs[15]=0xF8;
	scc_regs[14]=0xA0;
	scc_regs[11]=0x08;
	scc_regs[9]=0;
	RR3&=~0x38;RR3M&=~0x38;
	scc_regs[0]=1<<TBE;//RR0A
}

void SCC::channelBreset()
{
	scc_regs[15+16]=0xF8;
	scc_regs[14+16]=0xA0;
	scc_regs[11+16]=0x08;
	scc_regs[9]=0;//single WR9
	RR3&=~7;RR3M&=~7;
	scc_regs[16]=1<<TBE;//RR0B
}

uint8 SCC::handleRead(memptr addr)
{
	uint8 value=0;
	uint16 temp;
	int set2;
	addr -= getHWoffset();
	set2=(addr>=4)?16:0;//16=channel B
	switch(addr) {
	  case 0: //channel A
	  case 4: //channel B
		switch(active_reg){
		 case 0:
			if(set2){ // RR0B
				temp=serial->getStatus();// only for channel B
				scc_regs[16]=temp&0xFF;// define CTS(5), TBE(2) and RBF=RCA(0)
				RR3=RR3M&(temp>>8);// define RxIP(2), TxIP(1) and ExtIP(0)
			}
			else{scc_regs[0]=4;if(scc_regs[9]==0x20)RR3|=0x8;}

		 	value=scc_regs[set2];// not yet defined for channel A
		 break;
		 case 2:// not really useful (RR2 seems unaccessed...)
		  value=scc_regs[2];
		  if(set2==0)break;// vector base only for RR2A
		  if((scc_regs[9]&1)==0)break;// no status bit added
		  // status bit added to vector
		  if (scc_regs[9]&0x10){// modify high bits
			if(RR3==0){value|=0x60;break;}
			if(RR3&32){value|=0x30;break;}// A RxIP
			if(RR3&16){value|=0x10;break;}// A TxIP
			if(RR3&8){value|=0x50;break;}// A Ext IP
			if(RR3&4){value|=0x20;break;}// B RBF
			if(RR3&2)break;// B TBE
			if(RR3&1)value|=0x40;// B Ext Status
		  }
		  else{//modify low bits
			if(RR3==0){value|=6;break;}// no one
			if(RR3&32){value|=0xC;break;}// A RxIP
			if(RR3&16){value|=0x8;break;}// A TxIP
			if(RR3&8){value|=0xA;break;}// A Ext IP
			if(RR3&4){value|=4;break;}// B RBF
			if(RR3&2)break;// B TBE
			if(RR3&1)value|=2;// B Ext Status (CTS)
		  }
		 break;
		 case 3:
		   value=(set2)?0:RR3; //access on A channel only
		 break;
		 case 4:// RR0
			value=scc_regs[set2];
		 break;
		 case 8:// DATA reg
			if(set2){ // only channel B processed
				scc_regs[8+set2]=serial->getData();
			}
		  value=scc_regs[8+set2];
		 break;
		 case 9://WR13
		  value=scc_regs[13+set2];
		 break;
		 case 11:// WR15
		 case 15:// EXT/STATUS IT Ctrl
		  value=scc_regs[15+set2]&=0xFA;// mask out D2 and D0
		 break;
		 case 12:// BRG LSB
		 case 13:// BRG MSB
		  value=scc_regs[active_reg+set2];
		 break;

		 default:// RR5,RR6,RR7,RR10,RR14 not processed
		  D(bug("scc : unprocessed read address=$%x *********\n",active_reg));
		  value=0;
		 break;
		}
	  break;
	  case 2:// channel A
		  value=scc_regs[8]; // TBD (LAN)
	  break;
	  case 6:// channel B
		  scc_regs[8+16]=serial->getData();
		  value=scc_regs[8+16];
	  break;
	  default:
		 D(bug("scc : illegal read address=$%x\n",addr));
	  break;
	}
	active_reg=0;// next access for RR0 or WR0
/*	D(bug("HWget_b(0x%08x)=0x%02x at 0x%08x", addr+HW, value, showPC()));*/
	return value;
}

void SCC::handleWrite(memptr addr, uint8 value)
{
	int i,set2;
	uint32 BaudRate;
	addr -= getHWoffset();
	set2=(addr>=4)?16:0;// channel B
	switch(addr) {
		case 0:
		case 4:
			switch(active_reg) {
				case 0:
				 if(value<=15)active_reg = value & 0x0f;
				 else{
				  if((value&0x38)==0x38){// Reset Highest IUS (last operation in IT service routine)
					for(i=0x20;i;i>>=1){
						if(RR3&i)break;
					}
#define UGLY
#ifdef UGLY
					// tricky & ugly speed improvement for input
					if(i==4){// RxIP
						charcount--;
						if(charcount<=0) RR3&=~4;// optimize input; don't reset RxIP when chars are buffered
					}
					else RR3&=~i;
#else
					RR3&=~i;
#endif
				  }
				  else if((value&0x38)==0x28){// Reset Tx int pending
					if(set2)RR3&=~2;// channel B
					else RR3&=~0x10;// channel A
				  }
				  else if((value&0x38)==0x10){// Reset Ext/Status ints
					if(set2)RR3&=~1;// channel B
					else RR3&=~8;// channel A
				  }
				  TriggerSCC((RR3&RR3M)&&((0xB&scc_regs[9])==9)); // clear SCC flag if no pending IT
					// or no properly configured WR9
					// must be done here to avoid scc_do_Interrupt call without pending IT
				 }
				break;
				default: // all but 0
				 scc_regs[active_reg+set2] = value;
			 	 if(active_reg==2)scc_regs[active_reg] = value;// single WR2 on SCC
				 else if(active_reg==8){
						if(set2)serial->setData(value);// channel B only
						// channel A to be done if necessary
				 }
				 else if(active_reg==1){// Tx/Rx interrupt enable
					switch(set2){
						case 0:// channel A
							if(value&1)RR3M|=8;else RR3&=~8;// no IP(RR3) if not enabled(RR3M)
							if(value&2)RR3M|=16;else RR3&=~16;
							if(value&0x18)RR3M|=32;else RR3&=~32;
						break;
						default:// channel B
							if(value&1)RR3M|=1;else RR3&=~1;
							if(value&2)RR3M|=2;else RR3&=~2;
							if(value&0x18)RR3M|=4;else RR3&=~4;
							//  set or clear SCC flag if necessary (see later)
						break;
					}
				 }
				 else if(active_reg==5){// Transmit parameter and control
					serial->setRTS(value&2);
					serial->setDTR(value&128);
					// Tx character format & Tx CRC would be selected also here (8 bits/char and no CRC assumed)
				 }
				 else if(active_reg==9){// Master interrupt control (common for both channels)
					scc_regs[9] = value;// single WR9 (accessible by both channels)
					if(value&0x40){channelBreset();}
					if(value&0x80){channelAreset();}
					//  set or clear SCC flag accordingly (see later)
				 }
				 else if(active_reg==13){// set baud rate according to WR13 and WR12
					// Normally we have to set the baud rate according
					// to clock source (WR11) and clock mode (WR4)
					// In fact, we choose the baud rate from the value stored in WR12 & WR13
					// Note: we assume that WR13 is always written last (after WR12)
					// we tried to be more or less compatible with HSMODEM (see below)
					// 75 and 50 bauds are preserved because 153600 and 76800 were not available
					// 3600 and 2000 were also unavailable and are remapped to 57600 and 38400 respectively
					BaudRate=0;
					switch (value){
						case 0:
							switch (scc_regs[12+set2]){
								case 0://HSMODEM for 200 mapped to 230400
									BaudRate=230400;
								break;
								case 2://HSMODEM for 150 mapped to 115200
									BaudRate=115200;
								break;
								case 6://HSMODEM for 134 mapped to 57600
								case 0x7e://HSMODEM for 3600 remapped to 57600
								case 0x44://normal for 3600 remapped to 57600
									BaudRate=57600;
								break;
								case 0xa://HSMODEM for 110 mapped to 38400
								case 0xe4://HSMODEM for 2000 remapped to 38400
								case 0x7c://normal for 2000 remapped to 38400
									BaudRate=38400;
								break;
								case 0x16://HSMODEM for 19200
								case 0xb:// normal for 19200
									BaudRate=19200;
								break;
								case 0x2e://HSMODEM for 9600
								case 0x18://normal for 9600
									BaudRate=9600;
								break;
								case 0x5e://HSMODEM for 4800
								case 0x32://normal for 4800
									BaudRate=4800;
								break;
								case 0xbe://HSMODEM for 2400
								case 0x67://normal
									BaudRate=2400;
								break;
								case 0xfe://HSMODEM for 1800
								case 0x8a://normal for 1800
									BaudRate=1800;
								break;
								case 0xd0://normal for 1200
									BaudRate=1200;
								break;
								case 1://HSMODEM for 75 kept to 75
									BaudRate=75;
								break;
								case 4://HSMODEM for 50 kept to 50
									BaudRate=50;
								break;
								default:
									D(bug("unexpected LSB constant for baud rate"));
								break;
							}
						break;
						case 1:
							switch(scc_regs[12+set2]){
								case 0xa1://normal for 600
									BaudRate=600;
								break;
								case 0x7e://HSMODEM for 1200
									BaudRate=1200;
								break;
							}
						break;
						case 2:
							if(scc_regs[12+set2]==0xfe)BaudRate=600;//HSMODEM
						break;
						case 3:
							if(scc_regs[12+set2]==0x45)BaudRate=300;//normal
						break;
						case 4:
							if(scc_regs[12+set2]==0xe8)BaudRate=200;//normal
						break;
						case 5:
							if(scc_regs[12+set2]==0xfe)BaudRate=300;//HSMODEM
						break;
						case 6:
							if(scc_regs[12+set2]==0x8c)BaudRate=150;//normal
						break;
						case 7:
							if(scc_regs[12+set2]==0x4d)BaudRate=134;//normal
						break;
						case 8:
							if(scc_regs[12+set2]==0xee)BaudRate=110;//normal
						break;
						case 0xd:
							if(scc_regs[12+set2]==0x1a)BaudRate=75;//normal
						break;
						case 0x13:
							if(scc_regs[12+set2]==0xa8)BaudRate=50;//normal
						break;
						case 0xff://HSMODEM dummy value->silently ignored
						break;
						default:
							D(bug("unexpected MSB constant for baud rate"));
						break;
					}
					if(BaudRate)serial->setBaud(BaudRate);// set only if defined

/* summary of baud rates:

Rsconf		Falcon		Falcon(+HSMODEM)	    Aranym	  Aranym(+HSMODEM)
0		   19200		 19200				   19200	   19200
1			9600		  9600					9600		9600
2			4800		  4800					4800		4800
3			3600		  3600				   57600	   57600
4			2400		  2400					2400		2400
5			2000		  2000				   38400	   38400
6			1800		  1800					1800		1800
7			1200		  1200					1200	    1200
8			 600		   600					 600		 600
9			 300		   300					 300		 300

10			 200		230400					 200	  230400
11			 150		115200					 150	  115200
12			 134		 57600					 134	   57600
13			 110		 38400					 110	   38400
14			  75		153600					  75		  75
15			  50		 76800					  50		  50

*/


				 }
				 else if(active_reg==15){// external status int control
					if(value&1) {
						D(bug("SCC WR7 prime not yet processed\n"));
					}
				 }

				 if( (active_reg==1)||(active_reg==2)||(active_reg==9))
				 	TriggerSCC((RR3&RR3M) && ((0xB&scc_regs[9])==9)); // set or clear SCC flag accordingly. Yes it's ugly but avoids unnecessary useless calls
				 active_reg=0;// next access for RR0 or WR0
				break;
			}

		break;
		case 2: // channel A to be done if necessary
		break;
		case 6: // channel B
			serial->setData(value);
		break;
		default:
	                D(bug( "scc : illegal write address =$%x\n",addr));
		break;
	}

/*	D(bug("HWput_b(0x%08x,0x%02x) at 0x%08x", addr+HW, value, showPC()));*/
}

void SCC::IRQ(void)
{
	uint16 temp;
	temp=serial->getStatus();
	if(scc_regs[9]==0x20)
		temp|=0x800; // fake ExtStatusChange for HSMODEM install
	scc_regs[16]=temp&0xFF;// RR0B
	RR3=RR3M&(temp>>8);
	if((RR3)&&((scc_regs[9]&0xB)==9))
		TriggerSCC(true);
}


// return : vector number, or zero if no interrupt
int SCC::doInterrupt()
{
	int vector;
	uint8 i;
	for(i = 0x20 ; i ; i>>=1) { // highest priority first
		if(RR3 & i & RR3M)
			break ;
	}
	vector = scc_regs[2];//WR2 = base of vectored interrupts for SCC
	if((scc_regs[9]&3)==0)
		return vector;// no status included in vector
	if((scc_regs[9]&0x32)!=0) { //shouldn't happen with TOS, (to be completed if needed)
        	D(bug( "unexpected WR9 contents \n"));
		// no Soft IACK, Status Low control bit expected, no NV
		return 0;
	}
	switch(i) {
		case 0: /* this shouldn't happen :-) */
			D(bug( "scc_do_interrupt called with no pending interrupt\n"));
			vector=0;// cancel
			break;
		case 1:
			vector|=2;// Ch B Ext/status change
			break;
		case 2:
			break;// Ch B Transmit buffer Empty
		case 4:
			vector|=4;// Ch B Receive Char available
			break;
		case 8:
			vector|=0xA;// Ch A Ext/status change
			break;
		case 16:
			vector|=8;// Ch A Transmit Buffer Empty
			break;
		case 32:
			vector|=0xC;// Ch A Receive Char available
		break;
		// special receive condition not yet processed
	}
#if 0
        D(bug( "SCC::doInterrupt : vector %d\n", vector));
#endif
        return vector ;
}

/*
vim:ts=4:sw=4:
*/
