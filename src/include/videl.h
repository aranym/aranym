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
	SDL_Surface *surf;
	int sdl_videoparams;
	int width, height, od_posledni_zmeny;
	uint32 sdl_colors[256];
	bool sdl_colors_uptodate;

public:
	VIDEL();
	void init();
	virtual void handleWrite(uaecptr addr, uint8 value);
	void renderScreen();

	bool lockScreen();
	void unlockScreen();

	// the w, h should be width & height (but C++ complains -> 'if's in the implementation)
	void updateScreen( int x = 0, int y = 0, int w = -1, int h = -1 );

	void updateColors();

	int getVideoMode();
	int getScreenWidth();
	int getScreenHeight();

	long getVideoramAddress();
	long getHostVideoramAddress();

protected:
	void setHostWindow();
};

#endif


/*
 * $Log$
 * Revision 1.8  2001/06/15 14:16:06  joy
 * VIDEL palette registers are now processed by the VIDEL object.
 *
 * Revision 1.7  2001/06/13 07:12:39  standa
 * Various methods renamed to conform the sementics.
 * Added videl fuctions needed for VDI driver.
 *
 *
 */

