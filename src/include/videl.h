/* Joy 2001 */
#ifndef _VIDEL_H
#define _VIDEL_H

class VIDEL {
private:
	uae_u8 shifter;
	uae_u16 videl;
	uae_u16 videoctrl;
	uae_u8 syncmode;
	uae_u8 videomode;

public:
	VIDEL();
	uae_u8 handleRead(uaecptr);
	void handleWrite(uaecptr, uae_u8);
	int getVideoMode();
};

#endif
