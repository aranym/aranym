/* Joy 2001 */

#include "icio.h"

class MMU : public ICio {
private:
	uae_u8 addr;

public:
	MMU();
	virtual uae_u8 handleRead(uaecptr addr);
	virtual void handleWrite(uaecptr addr, uae_u8 value);
};
