/*
 * mfp.h - MFP emulation - declaration
 *
 * Copyright (c) 2001-2005 Petr Stehlik of ARAnyM dev team (see AUTHORS)
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

#ifndef _MFP_H
#define _MFP_H

#include "icio.h"

class MFP_Timer {
private:
	uint8 control;
	uint8 start_data, current_data;
	bool state;
	char name;

protected:
	bool isRunning();

public:
	MFP_Timer(int);
	void reset();
	void setControl(uint8);
	uint8 getControl();
	void setData(uint8);
	uint8 getData();
	void resetCounter();
	int compute_timer_freq();
};

class MFP_TimerA:public MFP_Timer {
public:
	MFP_TimerA() : MFP_Timer(0) {};
};

class MFP_TimerB:public MFP_Timer {
public:
	MFP_TimerB() : MFP_Timer(1) {};
};

class MFP_TimerC:public MFP_Timer {
public:
	MFP_TimerC() : MFP_Timer(2) {};
};

class MFP_TimerD:public MFP_Timer {
public:
	MFP_TimerD() : MFP_Timer(3) {};
};

class USART {
public:
	uint8 synchar;
	uint8 control;
	uint8 rxstat;
	uint8 txstat;
	uint8 data;
};

/*****************************************************************/

class MFP : public BASE_IO {
private:
	uint8 input;
	uint8 GPIP_data;
	uint8 active_edge;
	uint8 data_direction;
	uint16 irq_enable;
	uint16 irq_pending;
	uint16 irq_inservice;
	uint16 irq_mask;
	MFP_TimerA A;
	MFP_TimerB B;
	MFP_TimerC C;
	MFP_TimerD D;
	// USART usart;
	int timerCounter;
	int vr;

public:
	MFP(memptr addr, uint32 size);
	void reset();
	virtual uint8 handleRead(memptr);
	virtual void handleWrite(memptr, uint8);
	void IRQ(int, int count);
	void setGPIPbit(int mask, int value);
	int doInterrupt(void);
	int timerC_ms_ticks();

private:
	void set_active_edge(uint8 value);
};

#endif /* _MFP_H */
