/* Joy 2001 */

#include "icio.h"

class FAKEIO : public ICio {
private:
	bool use_RAM;
public:
	FAKEIO(bool bRAM = false);
	virtual uae_u8 handleRead(uaecptr addr);
	virtual void handleWrite(uaecptr addr, uae_u8 value);
};
