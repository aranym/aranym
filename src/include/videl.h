/*
 * $Header$
 *
 * Joy 2001
 */

#ifndef _VIDEL_H
#define _VIDEL_H

#include "icio.h"
#include <SDL/SDL.h>

class VIDEL : public BASE_IO {
protected:
	int width, height, od_posledni_zmeny;
	bool host_colors_uptodate;

public:
	VIDEL();
	void init();

	virtual void handleWrite(uaecptr addr, uint8 value);

	void renderScreen();
	void updateColors();

	int getScreenBpp();
	int getScreenWidth();
	int getScreenHeight();

	long getVideoramAddress();
};

#endif


/*
 * $Log$
 * Revision 1.9  2001/06/17 21:56:32  joy
 * updated to reflect changes in .cpp counterpart
 *
 * Revision 1.8  2001/06/15 14:16:06  joy
 * VIDEL palette registers are now processed by the VIDEL object.
 *
 * Revision 1.7  2001/06/13 07:12:39  standa
 * Various methods renamed to conform the sementics.
 * Added videl fuctions needed for VDI driver.
 *
 *
 */

