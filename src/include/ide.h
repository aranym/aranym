/* Joy 2001 */

#include "icio.h"

class IDE : public BASE_IO {
public:
	IDE(memptr, uint32);
	virtual uae_u8 handleRead(uaecptr addr);
	virtual void handleWrite(uaecptr addr, uae_u8 value);
	virtual uae_u16 handleReadW(uaecptr addr);
	virtual void handleWriteW(uaecptr addr, uae_u16 value);
	virtual uae_u32 handleReadL(uaecptr addr);
	virtual void handleWriteL(uaecptr addr, uae_u32 value);
};
