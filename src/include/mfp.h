/* Joy 2001 */

#ifndef _MFP_H
#define _MFP_H

#include "icio.h"

class MFP_Timer {
private:
	uae_u8 control;
	uae_u8 start_data, current_data;
	bool state;
	char name;

protected:
	bool MFP_Timer::isRunning();

public:
	MFP_Timer(int);
	void setControl(uae_u8);
	uae_u8 getControl();
	void setData(uae_u8);
	uae_u8 getData();
	void reset();
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
	uae_u8 synchar;
	uae_u8 control;
	uae_u8 rxstat;
	uae_u8 txstat;
	uae_u8 data;
};

/*****************************************************************/

class MFP : public ICio {
private:
	uae_u8 GPIP_data;
	uae_u8 active_edge;
	uae_u8 data_direction;
	uae_u16 irq_enable;
	uae_u16 irq_pending;
	uae_u16 irq_inservice;
	uae_u16 irq_mask;
	bool automaticServiceEnd;
	MFP_TimerA A;
	MFP_TimerB B;
	MFP_TimerC C;
	MFP_TimerD D;
	USART usart;
	int flags;
	int timerCounter;
	enum FLAGS {F_ACIA=(1<<6),F_TIMERC=(1<<5)};

public:
	MFP();
	virtual uae_u8 handleRead(uaecptr);
	virtual void handleWrite(uaecptr, uae_u8);
	void IRQ(int, int count);
	int doInterrupt(void);
};

#endif /* _MFP_H */
