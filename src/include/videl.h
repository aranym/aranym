/* Joy 2001 */
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
	void update_screen();

protected:
	void setHostWindow();
	long getVideoramAddress();
	int getVideoMode();
	int getScreenWidth();
	int getScreenHeight();
};

#endif
