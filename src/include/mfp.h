/* Joy 2001 */

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
	bool MFP_Timer::isRunning();

public:
	MFP_Timer(int);
	void setControl(uint8);
	uint8 getControl();
	void setData(uint8);
	uint8 getData();
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
	uint8 synchar;
	uint8 control;
	uint8 rxstat;
	uint8 txstat;
	uint8 data;
};

/*****************************************************************/

class MFP : public ICio {
private:
	uint8 GPIP_data;
	uint8 active_edge;
	uint8 data_direction;
	uint8 irq_enable;
	uint8 irq_pending;
	uint16 irq_inservice;
	uint16 irq_mask;
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
	virtual uint8 handleRead(uaecptr);
	virtual void handleWrite(uaecptr, uint8);
	void IRQ(int, int count);
	int doInterrupt(void);
};

#endif /* _MFP_H */
