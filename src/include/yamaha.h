/* Joy 2001 */

#ifndef _YAMAHA_H
#define _YAMAHA_H

#include "icio.h"

class YAMAHA : public BASE_IO {
private:
	int active_reg;
	int yamaha_regs[16];

public:
	YAMAHA(memptr, uint32);
	virtual uae_u8 handleRead(uaecptr addr);
	virtual void handleWrite(uaecptr addr, uae_u8 value);
	int getFloppyStat();
};

#endif /* _YAMAHA_H */
