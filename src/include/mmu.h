/* Joy 2001 */

#include "icio.h"

class MMU : public BASE_IO {
private:
	uae_u8 addr;

public:
	MMU(memptr, uint32);
	virtual uae_u8 handleRead(uaecptr addr);
	virtual void handleWrite(uaecptr addr, uae_u8 value);
};
