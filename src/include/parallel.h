/*
 * Parallel port emulation
 */

#ifndef _PARALLEL_H
#define _PARALLEL_H

#include "sysdeps.h"

class Parallel {
private:
	int port;
	int gcontrol;
	int old_strobe, old_busy;

	void set_ctrl(int);
	void clr_ctrl(int);

public:
	Parallel();
	void setDirection(bool out);
	uint8 getData();
	void setData(uint8 value);
	uint8 getBusy();
	void setStrobe(bool high);
};

#endif /* _PARALLEL_H */
