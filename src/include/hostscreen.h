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
	SDL_Surface *surf; // The main window surface
	uint32 sdl_videoparams;
	uint32 width, height;
	bool   doUpdate; // the HW surface is available -> the SDL need not to update the surface after ->pixel access
	uint32 sdl_colors[256]; // TOS palette (bpp < 16) to SDL color mapping

public:
	HostScreen() {
	}

	bool   renderBegin();
	void   renderEnd();

	// the w, h should be width & height (but C++ complains -> 'if's in the implementation)
	inline void   update( uint32 x = 0, uint32 y = 0, int32 w = -1, int32 h = -1, bool forced = false );
	inline void   update( bool forced );

	uint32 getBpp();
	uint32 getWidth();
	uint32 getHeight();
	uint32 getVideoramAddress();

	void   setPaletteColor( uint32 index, uint32 red, uint32 green, uint32 blue );
	void   mapPaletteColor( uint32 destIndex, uint32 sourceIndex );
	uint32 getPaletteColor( uint32 index );
	uint32 getColor( uint32 red, uint32 green, uint32 blue );

	void   setWindowSize( uint32 width, uint32 height );
	void   setRendering( bool render );

	// gfx Primitives draw functions
	void   drawLine( int32 x1, int32 y1, int32 x2, int32 y2, uint32 pattern, uint32 color );
	void   fillArea( int32 x1, int32 y1, int32 x2, int32 y2, uint32 pattern, uint32 color );
};


// inline functions

inline uint32 HostScreen::getBpp() {
	return surf->format->BytesPerPixel;
}

inline uint32 HostScreen::getWidth() {
	return width;
}

inline uint32 HostScreen::getHeight() {
	return height;
}

inline uint32 HostScreen::getVideoramAddress() {
	return (uint32)surf->pixels;
}

inline void HostScreen::setPaletteColor( uint32 index, uint32 red, uint32 green, uint32 blue ) {
	// no palette size bound cross
	assert( index >= 0 && index <= 255 );

	sdl_colors[index] = SDL_MapRGB( surf->format, red, green, blue );
}

inline void HostScreen::mapPaletteColor( uint32 destIndex, uint32 sourceIndex ) {
	sdl_colors[destIndex] = sdl_colors[sourceIndex];
}

inline uint32 HostScreen::getPaletteColor( uint32 index ) {
	return sdl_colors[index];
}

inline uint32 HostScreen::getColor( uint32 red, uint32 green, uint32 blue ) {
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


inline void HostScreen::update( bool forced )
{
	update( 0, 0, width, height, forced );
}


inline void HostScreen::update( uint32 x, uint32 y, int32 w, int32 h, bool forced )
{
	if ( !forced && !doUpdate ) // the HW surface is available
		return;

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
 * Revision 1.1  2001/06/18 13:21:55  standa
 * Several template.cpp like comments were added.
 * HostScreen SDL encapsulation class.
 *
 *
 */
