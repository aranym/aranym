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

public:
	VIDEL();
	void renderScreen();

	bool lockScreen();
	void unlockScreen();

	// the w, h should be width & height (but C++ complains -> 'if's in the implementation)
	void updateScreen( int x = 0, int y = 0, int w = -1, int h = -1 );

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
 *
 */

