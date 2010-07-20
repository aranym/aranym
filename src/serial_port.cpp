/*
	Serial port emulation, Linux driver

	ARAnyM (C) 2005-2008 Patrice Mandin
			2010 Jean Conter

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

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>

#include "serial.h"
#include "serial_port.h"
#include <errno.h>


#define DEBUG 0
#include "debug.h"


Serialport::Serialport(void)
{
	D(bug("Serialport: interface created"));
	handle = open(bx_options.serial.serport,O_RDWR|O_NDELAY|O_NONBLOCK);/* Raw mode */
	// /dev/ttyS0 by default or /dev/ttyUSB0 : see Serport in ./aranym/config [SERIAL]
	if (handle<0) {
		panicbug("Serialport: Can not open device %s", bx_options.serial.serport);
		return;
	}
}

Serialport::~Serialport(void)
{
	D(bug("Serialport: interface destroyed"));
	if (handle>=0) {
		fcntl(handle,F_SETFL,0);// back to (almost...) normal
		close(handle);
		handle=-1;
	}
}

void Serialport::reset(void)
{
	D(bug("Serialport: reset"));
}

uint8 Serialport::getData()
{
	uint8 value=0;
	int nb;

	D(bug("Serialport: getData"));
	if (handle>=0) {
		nb=read(handle,&value,1);
		if(nb<0){
			panicbug("Serialport: impossible to get data");
		}
	}
	return value;
}

void Serialport::setData(uint8 value)
{
	int nb;
	D(bug("Serialport: setData"));
	if (handle>=0) {
		//while((getTBE()&4)==0); nb=write(handle,&value,1); if(nb<0)panicbug("Serialport: setdata problem");
		// NB: wait for TBE before single write cancelled (not available with USB)
		// and replaced by the following do while (add a timeout if necessary)
		do{
		  nb=write(handle,&value,1);// nb<0 if device not ready
		}while(nb<0);
	}
}

void Serialport::setBaud(uint8 value)
{
	D(bug("Serialport: setBaud"));
	if (handle>=0) {
		struct termios options;
		tcgetattr(handle,&options);

		switch(value){
			case 0xd0: // 1200
			cfsetspeed(&options,B1200);
			break;
			case 0xfe:// HSMODEM
			panicbug("ambiguous: 1800 could be also 600 or 300 baud");
			case 0x8a: // 1800
			cfsetspeed(&options,B1800);
			break;
			case 0xe4:// HSMODEM
			case 0x7c: // remap 2000 into 38400 bauds
			cfsetspeed(&options,B38400);
			break;
			case 0xbe: // HSMODEM
			case 0x67: // 2400
			cfsetspeed(&options,B2400);
			break;
			case 0x7e:
			panicbug("ambiguous: 57600 could be also 1200 baud");
			case 0x44: // remap 3600 into 57600
			cfsetspeed(&options,B57600);
			break;
			case 0x5e: // HSMODEM
			case 0x32: // 4800
			cfsetspeed(&options,B4800);
			break;
			case 0x2e: //HSMODEM
			case 0x18: // 9600
			cfsetspeed(&options,B9600);
			break;
			case 0x16: // HSMODEM
			case 0xb: // 19200
			cfsetspeed(&options,B19200);
			break;
			case 0xa1: // 600
			cfsetspeed(&options,B600);
			break;
			case 0x45: // 300
			cfsetspeed(&options,B300);
			break;
			case 0x8c: // 150
			cfsetspeed(&options,B150);
			break;
			case 0x4d: // remap 134 into 115200
			cfsetspeed(&options,B115200);
			break;
			case 0xee: // 110
			cfsetspeed(&options,B110);
			break;
			case 1: // HSMODEM 153600 not done
			case 0x1a: // 75
			cfsetspeed(&options,B75);
			break;
			case 4: // HSMODEM 76800 not done
			case 0xa8: // 50
			cfsetspeed(&options,B50);
			break;
			case 0:// remap 200 into 230400 (HSMODEM)
			cfsetspeed(&options,B230400);
			break;
			case 2:// remap 150 into 115200 (HSMODEM)
			cfsetspeed(&options,B115200);
			break;
			case 6:// remap 134 into 57600 (HSMODEM)
			cfsetspeed(&options,B57600);
			break;
			case 0xa:// remap 110 into 38400 (HSMODEM)
			cfsetspeed(&options,B38400);
			break;
			default:
			panicbug("unregistred baud value $%x\n",value);
			break;
		}
		options.c_cflag|=(CLOCAL | CREAD);//
		options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);// raw input
		options.c_iflag &= ~(ICRNL);// CR is not CR+LF

		tcsetattr(handle,TCSANOW,&options);
	}
}

void Serialport::setRTSDTR(uint8 value)
{
	int status=0;

	if (handle>=0) {
		if(ioctl(handle,TIOCMGET,&status)<0){
			panicbug("Serialport: Can't get status");
		}
		if(value&2)status|=TIOCM_RTS;
		else status&=~TIOCM_RTS;
		if(value&128)status|=TIOCM_DTR;
		else status&=~TIOCM_DTR;
		ioctl(handle,TIOCMSET,&status);
	}
}

uint16 Serialport::getStatus()
{
	uint16 value;
	uint16 diff;
	int status=0;
	int nbchar=0;
	value=0;
	D(bug("Serialport: getBusy"));
	if (handle>=0) {
		if (ioctl(handle,FIONREAD,&nbchar)<0){
			panicbug("Serialport: Can't get input fifo count");
		}
		getSCC()->charcount=nbchar;// to optimize input (see UGLY in scc.cpp)
		if(nbchar>0) value=0x0401;// RxIC+RBF
		value|=getTBE();// TxIC
		value|=(1<<TBE);// fake TBE to optimize output (for ttyS0)
		if(ioctl(handle,TIOCMGET,&status)<0){
			panicbug("Serialport: Can't get status");
		}
		if(status&TIOCM_CTS)value|=(1<<CTS);
	}
	diff=oldStatus^value;
	if(diff&(1<<CTS))value|=0x100;// ext status IC on CTS change

	oldStatus=value;
	return value;
}


uint16 Serialport::getTBE() // not suited to serial USB
{
	uint16 value=0;
	int status=0;
		if (ioctl(handle,TIOCSERGETLSR,&status)<0){// OK with ttyS0, not OK with ttyUSB0
			//panicbug("Serialport: Can't get LSR");
			value|=(1<<TBE);// only for serial USB
		}
		else if(status&TIOCSER_TEMT) {value=(1<<TBE);// this is a real TBE for ttyS0
			if((oldTBE&(1<<TBE))==0){value|=0x200;}// TBE rise=>TxIP (based on real TBE)
		}
		oldTBE=value;
		return value;

}
