/*
 * $Header$
 *
 * STanda 2001
 */

#ifndef _HOSTSCREEN_H
#define _HOSTSCREEN_H


#include <SDL/SDL.h>


class HostScreen {
protected:
	SDL_Surface *surf;
	int sdl_videoparams;
	int width, height;
	uint32 sdl_colors[256];

public:
	HostScreen() {
	}

	bool renderBegin();
	void renderEnd();

	// the w, h should be width & height (but C++ complains -> 'if's in the implementation)
	void update( int x = 0, int y = 0, int w = -1, int h = -1 );

	int  getBpp();
	int  getWidth();
	int  getHeight();
	long getVideoramAddress();

	void   setPaletteColor( int index, int red, int green, int blue );
	void   mapPaletteColor( int destIndex, int sourceIndex );
	uint32 getPaletteColor( int index );
	uint32 getColor( int red, int green, int blue );


	void setWindowSize( int width, int height );
};


// inline functions

inline int HostScreen::getBpp() {
	return surf->format->BytesPerPixel;
}

inline int HostScreen::getWidth() {
	return width;
}

inline int HostScreen::getHeight() {
	return height;
}

inline long HostScreen::getVideoramAddress() {
	return (long)surf->pixels;
}


inline void HostScreen::setPaletteColor( int index, int red, int green, int blue ) {
	// no palette size bound cross
	assert( index >= 0 && index <= 255 );

	sdl_colors[index] = SDL_MapRGB( surf->format, red, green, blue );
}

inline void HostScreen::mapPaletteColor( int destIndex, int sourceIndex ) {
	sdl_colors[destIndex] = sdl_colors[sourceIndex];
}

inline uint32 HostScreen::getPaletteColor( int index ) {
	return sdl_colors[index];
}

inline uint32 HostScreen::getColor( int red, int green, int blue ) {
	return SDL_MapRGB( surf->format, red, green, blue );
}


inline bool HostScreen::renderBegin() {
	if (SDL_MUSTLOCK(surf))
		if (SDL_LockSurface(surf) < 0) {
			printf("Couldn't lock surface to refresh!\n");
			return false;
		}

	return true;
}

inline void HostScreen::renderEnd() {
	if (SDL_MUSTLOCK(surf))
		SDL_UnlockSurface(surf);
}

inline void HostScreen::update( int x, int y, int w, int h )
{
	// the object variable could not be the default one for the method's w value
	if ( w == -1 )
		w = width;
	if ( h == -1 )
		h = height;

	//	SDL_UpdateRect(SDL_GetVideoSurface(), 0, 0, width, height);
	SDL_UpdateRect(surf, x, y, w, h);
}


#endif


/*
 * $Log$
 *
 */
