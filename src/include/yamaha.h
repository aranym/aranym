/* Joy 2001 */

#include "icio.h"

class YAMAHA : public ICio {
private:
	int active_reg;
	int yamaha_regs[16];

public:
	YAMAHA();
	virtual uae_u8 handleRead(uaecptr addr);
	virtual void handleWrite(uaecptr addr, uae_u8 value);
	int getFloppyStat();
};
