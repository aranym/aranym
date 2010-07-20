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
#include "memory.h"
#include "scc.h"
#include "parameters.h"

#include "serial.h"
#include "serial_port.h"

#include "dsp.h"

#define DEBUG 0
#include "debug.h"


SCC::SCC(memptr addr, uint32 size) : BASE_IO(addr, size)
{
	serial = new Serialport;
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
		  panicbug("scc : unprocessed read address=$%x *********\n",active_reg);
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
		 panicbug("scc : illegal read address=$%x\n",addr);
	  break;
	}
	active_reg=0;// next access for RR0 or WR0
/*	D(bug("HWget_b(0x%08x)=0x%02x at 0x%08x", addr+HW, value, showPC()));*/
	return value;
}

void SCC::handleWrite(memptr addr, uint8 value)
{
	int i,set2;
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
					serial->setRTSDTR(value);
					// Tx character format & Tx CRC would be selected also here (8 bits/char and no CRC assumed)
				 }
				 else if(active_reg==9){// Master interrupt control (common for both channels)
					scc_regs[9] = value;// single WR9 (accessible by both channels)
					if(value&0x40){channelBreset();}
					if(value&0x80){channelAreset();}
					//  set or clear SCC flag accordingly (see later)
				 }
				 else if(active_reg==12){// new baud rate
					serial->setBaud(value);// set baud rate according only to LSB byte (ignore MSB (WR13))
					// NB : to be recoded correctly : normally, we have to pass a 16 bit value (WR13<<8+WR12) but
					// WR13 is not yet defined (LSB written first...). We also have to set the baud rate according
					// to clock source (WR11) and clock mode (WR4)
					// We choose a more simple solution, replacing some baud rates by more useful values.
					// we tried to be more or less compatible with HSMODEM (see below)
/*
Falcon		Falcon(+HSMODEM)	    Aranym	  Aranym(+HSMODEM)
19200		 19200					19200	   19200
9600		  9600					9600		9600
4800		  4800					4800		4800
3600		  3600					57600	   57600
2400		  2400					2400		2400
2000		  2000					38400	   38400
1800		  1800					1800		1800
1200		  1200					1200	   57600
600			   600					600			1800
300			   300					300			1800
200			230400					230400	  230400
150			115200					150		   115200
134			 57600					115200	   57600
110			 38400					110		   38400
75			153600					75		      75
50			 76800					50		      50

*/
				 }
				 else if(active_reg==15){// external status int control
					if(value&1)panicbug("SCC WR7 prime not yet processed\n");
				 }

				 if( (active_reg==1)||(active_reg==2)||(active_reg==9))TriggerSCC((RR3&RR3M) && ((0xB&scc_regs[9])==9)); // set or clear SCC flag accordingly. Yes it's ugly but avoids unnecessary useless calls
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
                panicbug( "scc : illegal write address =$%x\n",addr) ;
		break;
	}

/*	D(bug("HWput_b(0x%08x,0x%02x) at 0x%08x", addr+HW, value, showPC()));*/
}

void SCC::IRQ(void)
{
	uint16 temp;
	temp=serial->getStatus();
	if(scc_regs[9]==0x20)temp|=0x800; // fake ExtStatusChange for HSMODEM install
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
		if(RR3 & i & RR3M)break ;
	}
	vector = scc_regs[2];//WR2 = base of vectored interrupts for SCC
	if((scc_regs[9]&3)==0)return vector;// no status included in vector
	if((scc_regs[9]&0x32)!=0){//shouldn't happen with TOS, (to be completed if needed)
        panicbug( "unexpected WR9 contents \n");
		// no Soft IACK, Status Low control bit expected, no NV
		return 0;
	}
	switch(i){
	 case 0: /* this shouldn't happen :-) */
        panicbug( "scc_do_interrupt called with no pending interrupt\n") ;
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
        panicbug( "SCC::doInterrupt : vector %d\n", vector) ;
#endif
        return vector ;
}

/*
vim:ts=4:sw=4:
*/
