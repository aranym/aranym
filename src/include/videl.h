/* Joy 2001 */
#ifndef _VIDEL_H
#define _VIDEL_H

#include "icio.h"

class VIDEL : public ICio {
private:
	uae_u8 shifter;
	uae_u16 videl;
	uae_u16 videoctrl;
	uae_u8 syncmode;
	uae_u8 videomode;

public:
	VIDEL();
	virtual uae_u8 handleRead(uaecptr);
	virtual void handleWrite(uaecptr, uae_u8);
	int getVideoMode();
};

#endif
