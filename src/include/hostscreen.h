/*
 * $Header$
 *
 * STanda 2001
 */

#ifndef _HOSTSCREEN_H
#define _HOSTSCREEN_H


#include <SDL/SDL.h>
#include "gfxprimitives.h"


class HostScreen {
protected:
	SDL_Surface *surf;  // The main window surface
	uint32 sdl_videoparams;
	uint32 width, height;
	bool   doUpdate; // the HW surface is available -> the SDL need not to update the surface after ->pixel access
	uint32 sdl_colors[256]; // TOS palette (bpp < 16) to SDL color mapping

	/**
	 * This is the SDL_gfxPrimitives derived functions.
	 **/
	inline void gfxFastPixelColorNolock(int16 x, int16 y, uint32 color);
	       void gfxBoxColorPattern(int16 x1, int16 y1, int16 x2, int16 y2, uint16 *areaPattern, uint32 fgColor, uint32 bgColor);
	       void gfxBoxColorPatternBgTrans(int16 x1, int16 y1, int16 x2, int16 y2, uint16 *areaPattern, uint32 color);

public:

	HostScreen() {
	}

	inline bool   renderBegin();
	inline void   renderEnd();

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
	uint32 getPixel( int32 x, int32 y );
	void   putPixel( int32 x, int32 y, uint32 color );
	void   drawLine( int32 x1, int32 y1, int32 x2, int32 y2, uint16 pattern, uint32 color );
	// transparent background
	void   fillArea( int32 x1, int32 y1, int32 x2, int32 y2, uint16 *pattern, uint32 color );
	// VDI required function to fill areas
	void   fillArea( int32 x1, int32 y1, int32 x2, int32 y2, uint16 *pattern, uint32 fgColor, uint32 bgColor );
};


// inline functions
inline void HostScreen::gfxFastPixelColorNolock(int16 x, int16 y, uint32 color)
{
	int bpp;
	uint8 *p;

	/* Get destination format */
	bpp = surf->format->BytesPerPixel;
	p = (uint8 *)surf->pixels + y * surf->pitch + x * bpp;
	switch(bpp) {
		case 1:
			*p = color;
			break;
		case 2:
			*(uint16 *)p = color;
			break;
		case 3:
			if(SDL_BYTEORDER == SDL_BIG_ENDIAN) {
				p[0] = (color >> 16) & 0xff;
				p[1] = (color >> 8) & 0xff;
				p[2] = color & 0xff;
			} else {
				p[0] = color & 0xff;
				p[1] = (color >> 8) & 0xff;
				p[2] = (color >> 16) & 0xff;
			}
			break;
		case 4:
			*(uint32 *)p = color;
			break;
	} /* switch */
}



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

inline uint32 HostScreen::getPixel( int32 x, int32 y ) {
	if ( x < 0 || x >= (int32)width || y < 0 || y >= (int32)height )
		return 0;

	// HACK for bpp == 2 FIXME!
	return ((uint16*)surf->pixels)[((uint32)y*width)+(uint32)x];
}

inline void HostScreen::putPixel( int32 x, int32 y, uint32 color ) {
	if ( x < 0 || x >= (int32)width || y < 0 || y >= (int32)height )
		return;

	gfxFastPixelColorNolock( x, y, color );
}

inline void HostScreen::fillArea( int32 x1, int32 y1, int32 x2, int32 y2, uint16 *pattern, uint32 color )
{
	gfxBoxColorPatternBgTrans( (int16)x1, (int16)y1, (int16)x2, (int16)y2, pattern, color );
}

inline void HostScreen::fillArea( int32 x1, int32 y1, int32 x2, int32 y2, uint16 *pattern, uint32 fgColor, uint32 bgColor )
{
	gfxBoxColorPattern( (int16)x1, (int16)y1, (int16)x2, (int16)y2, pattern, fgColor, bgColor );
}

#endif


/*
 * $Log$
 * Revision 1.5  2001/09/19 22:52:43  standa
 * The putPixel is now much faster because it doesn't either update or surface locking.
 *
 * Revision 1.4  2001/09/17 10:36:08  joy
 * color fixed by Standa at AHody 2001.
 *
 * Revision 1.3  2001/08/30 14:04:59  standa
 * The fVDI driver. mouse_draw implemented. Partial pattern fill support.
 * Still buggy.
 *
 * Revision 1.2  2001/08/28 23:26:09  standa
 * The fVDI driver update.
 * VIDEL got the doRender flag with setter setRendering().
 *       The host_colors_uptodate variable name was changed to hostColorsSync.
 * HostScreen got the doUpdate flag cleared upon initialization if the HWSURFACE
 *       was created.
 * fVDIDriver got first version of drawLine and fillArea (thanks to SDL_gfxPrimitives).
 *
 * Revision 1.1  2001/06/18 13:21:55  standa
 * Several template.cpp like comments were added.
 * HostScreen SDL encapsulation class.
 *
 *
 */
