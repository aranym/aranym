/* Joy 2001 */

#ifndef _ARADATA
#define _ARADATA
#include "icio.h"

class ARADATA : public BASE_IO {
private:
	bool mouseDriver;
	int mouse_x, mouse_y;

public:
	ARADATA(memptr, uint32);
	virtual uae_u8 handleRead(uaecptr addr);
	virtual void handleWrite(uaecptr addr, uae_u8 value);
	bool isAtariMouseDriver()	{ return mouseDriver; }
	int getAtariMouseX()	{ return mouse_x; }
	int getAtariMouseY()	{ return mouse_y; }
};
#endif /* _ARADATA */
