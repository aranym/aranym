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

#include "sysdeps.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#ifdef __CYGWIN__
#include <sys/socket.h> /* for FIONREAD */
#endif

#include "serial.h"
#include "serial_port.h"
#include <errno.h>


#define DEBUG 0
#include "debug.h"


Serialport::Serialport(void)
{
	D(bug("Serialport: interface created"));
	oldTBE = 0;
	oldStatus = 0;
	handle = open(bx_options.serial.serport,O_RDWR|O_NDELAY|O_NONBLOCK);/* Raw mode */
	// /dev/ttyS0 by default or /dev/ttyUSB0 : see Serport in ./aranym/config [SERIAL]
	if (handle<0) {
		D(bug("Serialport: Can not open device %s", bx_options.serial.serport));
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
			D(bug("Serialport: impossible to get data"));
		}
	}
	return value;
}

void Serialport::setData(uint8 value)
{
	int nb;
	D(bug("Serialport: setData"));
	if (handle>=0) {
		//while((getTBE()&4)==0); nb=write(handle,&value,1); if(nb<0) D(bug("Serialport: setdata problem"));
		// NB: wait for TBE before single write cancelled (not available with USB)
		// and replaced by the following do while (add a timeout if necessary)
		do{
		  nb=write(handle,&value,1);// nb<0 if device not ready
		}while(nb<0);
	}
}

void Serialport::setBaud(uint32 value)
{
	D(bug("Serialport: setBaud"));

	if (handle<0)
		return;

	speed_t new_speed = B0;

	switch(value){
		case 230400:	new_speed = B230400;	break;
		case 115200:	new_speed = B115200;	break;
		case 57600:	new_speed = B57600;	break;
		case 38400:	new_speed = B38400;	break;
		case 19200:	new_speed = B19200;	break;
		case 9600:	new_speed = B9600;	break;
		case 4800:	new_speed = B4800;	break;
		case 2400:	new_speed = B2400;	break;
		case 1800:	new_speed = B1800;	break;
		case 1200:	new_speed = B1200;	break;
		case 600:	new_speed = B600;	break;
		case 300:	new_speed = B300;	break;
		case 200:	new_speed = B200;	break;
		case 150:	new_speed = B150;	break;
		case 134:	new_speed = B134;	break;
		case 110:	new_speed = B110;	break;
		case 75:	new_speed = B75;	break;
		case 50:	new_speed = B50;	break;
		default:	D(bug("unregistered baud value $%x\n",value));	break;
	}

	if (new_speed == B0)
		return;

	struct termios options;
	tcgetattr(handle,&options);

	cfsetispeed(&options, new_speed);
	cfsetospeed(&options, new_speed);

	options.c_cflag|=(CLOCAL | CREAD);//
	options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);// raw input
	options.c_iflag &= ~(ICRNL);// CR is not CR+LF

	tcsetattr(handle,TCSANOW,&options);
}

void Serialport::setRTS(bool value)
{
	int status=0;

	if (handle>=0) {
		if(ioctl(handle,TIOCMGET,&status)<0){
			D(bug("Serialport: Can't get status"));
		}
		if(value)status|=TIOCM_RTS;
		else status&=~TIOCM_RTS;
		ioctl(handle,TIOCMSET,&status);
	}
}

void Serialport::setDTR(bool value)
{
	int status=0;

	if (handle>=0) {
		if(ioctl(handle,TIOCMGET,&status)<0){
			D(bug("Serialport: Can't get status"));
		}
		if(value)status|=TIOCM_DTR;
		else status&=~TIOCM_DTR;
		ioctl(handle,TIOCMSET,&status);
	}
}

/*
void Serialport::setRTSDTR(uint8 value)
{
	int status=0;

	if (handle>=0) {
		if(ioctl(handle,TIOCMGET,&status)<0){
			D(bug("Serialport: Can't get status"));
		}
		if(value&2)status|=TIOCM_RTS;
		else status&=~TIOCM_RTS;
		if(value&128)status|=TIOCM_DTR;
		else status&=~TIOCM_DTR;
		ioctl(handle,TIOCMSET,&status);
	}
}
*/

uint16 Serialport::getStatus()
{
	uint16 value;
	uint16 diff;
	int status=0;
	int nbchar=0;
	value=0;
	D(bug("Serialport: getStatus"));
	if (handle>=0) {
		if (ioctl(handle,FIONREAD,&nbchar)<0){
			D(bug("Serialport: Can't get input fifo count"));
		}
		getSCC()->charcount=nbchar;// to optimize input (see UGLY in scc.cpp)
		if(nbchar>0) value=0x0401;// RxIC+RBF
		value|=getTBE();// TxIC
		value|=(1<<TBE);// fake TBE to optimize output (for ttyS0)
		if(ioctl(handle,TIOCMGET,&status)<0){
			D(bug("Serialport: Can't get status"));
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
	
#if defined(TIOCSERGETLSR) && defined(TIOCSER_TEMT)
	int status=0;
	if (ioctl(handle,TIOCSERGETLSR,&status)<0){// OK with ttyS0, not OK with ttyUSB0
		//D(bug("Serialport: Can't get LSR"));
		value|=(1<<TBE);// only for serial USB
	} else if(status&TIOCSER_TEMT) {
		value=(1<<TBE);// this is a real TBE for ttyS0
		if ((oldTBE&(1<<TBE))==0) {
			value|=0x200;
		}// TBE rise=>TxIP (based on real TBE)
	}
#endif

	oldTBE=value;
	return value;
}
