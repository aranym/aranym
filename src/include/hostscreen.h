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
	SDL_Surface *surf;	// The main window surface
	uint32 sdl_videoparams;
	uint32 width, height, bpp;
	bool   doUpdate; // the HW surface is available -> the SDL need not to update the surface after ->pixel access

	struct { // TOS palette (bpp < 16) to SDL color mapping
		SDL_Palette sdl;
		SDL_Color	standard[256];
		uint32		native[256];
	} palette;

	uint16 snapCounter; // ALT+PrintScreen to make a snap?

	/**
	 * This is the SDL_gfxPrimitives derived functions.
	 **/
	inline void	  gfxFastPixelColorNolock( int16 x, int16 y, uint32 color );
	inline uint32 gfxGetPixel( int16 x, int16 y );

	void   gfxHLineColor ( int16 x1, int16 x2, int16 y,
						   uint16 pattern, uint32 fgColor, uint32 bgColor, uint16 logOp );
	void   gfxVLineColor( int16 x, int16 y1, int16 y2,
						  uint16 pattern, uint32 fgColor, uint32 bgColor, uint16 logOp );
	void   gfxLineColor( int16 x1, int16 y1, int16 x2, int16 y2,
						 uint16 pattern, uint32 fgColor, uint32 bgColor, uint16 logOp );
	void   gfxBoxColorPattern( int16 x1, int16 y1, int16 x2, int16 y2,
                               uint16 *areaPattern, uint32 fgColor, uint32 bgColor, uint16 logOp );

  public:

	HostScreen() {
		// setting up the static palette settings
		palette.sdl.ncolors = 256;
		palette.sdl.colors = palette.standard;
	}

	inline bool	  renderBegin();
	inline void	  renderEnd();

	// the w, h should be width & height (but C++ complains -> 'if's in the implementation)
	inline void	  update( int32 x, int32 y, int32 w, int32 h, bool forced = false );
	inline void	  update( bool forced );
	inline void	  update();

	uint32 getBpp();
	uint32 getWidth();
	uint32 getHeight();
	uint32 getVideoramAddress();

	void   setPaletteColor( uint32 index, uint32 red, uint32 green, uint32 blue );
	uint32 getPaletteColor( uint32 index );
	void   updatePalette( uint32 colorCount );
	uint32 getColor( uint32 red, uint32 green, uint32 blue );

	void   setWindowSize( uint32 width, uint32 height, uint32 bpp );
	void   setRendering( bool render );

	// gfx Primitives draw functions
	uint32 getPixel( int16 x, int16 y );
	void   putPixel( int16 x, int16 y, uint32 color );
	void   putPixel( int16 x, int16 y, uint16 pattern, uint32 fgColor, uint32 bgColor, uint16 logOp );
	void   drawLine( int16 x1, int16 y1, int16 x2, int16 y2,
					 uint16 pattern, uint32 fgColor, uint32 bgColor, uint16 logOp );
	// transparent background
	void   fillArea( int16 x1, int16 y1, int16 x2, int16 y2, uint16 *pattern, uint32 color );
	// VDI required function to fill areas
	void   fillArea( int16 x1, int16 y1, int16 x2, int16 y2, uint16 *pattern, uint32 fgColor, uint32 bgColor, uint16 logOp );
	void   blitArea( int16 sx, int16 sy, int16 dx, int16 dy, int16 w, int16 h );

	/**
	 * Atari bitplane to chunky conversion helper.
	 **/
	void   bitplaneToChunky( uint16 *atariBitplaneData, uint16 bpp, uint8 colorValues[16] );

	// Create a BMP file with a snapshot of the screen surface
	void   makeSnapshot();
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
			putBpp24Pixel( p, color );
			break;
		case 4:
			*(uint32 *)p = color;
			break;
	} /* switch */
}

inline uint32 HostScreen::gfxGetPixel( int16 x, int16 y )
{
	int bpp;
	uint8 *p;

	/* Get destination format */
	bpp = surf->format->BytesPerPixel;
	p = (uint8 *)surf->pixels + y * surf->pitch + x * bpp;
	switch(bpp) {
		case 1:
			return (uint32)(*(uint8 *)p);
		case 2:
			return (uint32)(*(uint16 *)p);
		case 3:
			// FIXME maybe some & problems? and endian
			return getBpp24Pixel( p );
		case 4:
			return *(uint32 *)p;
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

	SDL_Color& color = palette.standard[index];
	color.r = red; color.g = green; color.b = blue; // set the SDL standard RGB palette settings
	palette.native[index] = SDL_MapRGB( surf->format, red, green, blue ); // convert the color to native
}

inline uint32 HostScreen::getPaletteColor( uint32 index ) {
	return palette.native[index];
}

inline void HostScreen::updatePalette( uint32 colorCount ) {
	SDL_SetColors( surf, palette.standard, 0, colorCount );
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


inline void HostScreen::update()
{
	update( 0, 0, width, height, false );
}


inline void HostScreen::update( int32 x, int32 y, int32 w, int32 h, bool forced )
{
	if ( !forced && !doUpdate ) // the HW surface is available
		return;

	//	SDL_UpdateRect(SDL_GetVideoSurface(), 0, 0, width, height);
	SDL_UpdateRect(surf, x, y, w, h);
}

inline uint32 HostScreen::getPixel( int16 x, int16 y ) {
	if ( x < 0 || x >= (int32)width || y < 0 || y >= (int32)height )
		return 0;

	return gfxGetPixel( x, y );
}

inline void HostScreen::putPixel( int16 x, int16 y, uint32 color ) {
	if ( x < 0 || x >= (int32)width || y < 0 || y >= (int32)height )
		return;

	gfxFastPixelColorNolock( x, y, color );
}

inline void HostScreen::drawLine( int16 x1, int16 y1, int16 x2, int16 y2, uint16 pattern, uint32 fgColor, uint32 bgColor, uint16 logOp )
{
	gfxLineColor( x1, y1, x2, y2, pattern, fgColor, bgColor, logOp ); // SDL_gfxPrimitives
}


inline void HostScreen::fillArea( int16 x, int16 y, int16 w, int16 h, uint16 *pattern, uint32 color )
{
	gfxBoxColorPattern( x, y, w, h, pattern, color, color, 2 );
}

inline void HostScreen::fillArea( int16 x, int16 y, int16 w, int16 h,
								  uint16 *pattern, uint32 fgColor, uint32 bgColor, uint16 logOp )
{
	gfxBoxColorPattern( x, y, w, h, pattern, fgColor, bgColor, logOp );
}

inline void HostScreen::blitArea( int16 sx, int16 sy, int16 dx, int16 dy, int16 w, int16 h )
{
	SDL_Rect srcrect;
	SDL_Rect dstrect;

	srcrect.x = sx;
	srcrect.y = sy;
	dstrect.x = dx;
	dstrect.y = dy;
	srcrect.w = dstrect.w = w;
	srcrect.h = dstrect.h = h;

	SDL_BlitSurface(surf, &srcrect, surf, &dstrect);
}


inline void HostScreen::bitplaneToChunky( uint16 *atariBitplaneData, uint16 bpp, uint8 colorValues[16] )
{
	memset( colorValues, 0, 16 ); // clear the color values for the 16 pixels (word length)

	for (int l = bpp - 1; l >= 0; l--) {
		uint16 data = atariBitplaneData[l]; // note: this is about 2000 dryhstones sppedup (the local variable)

		colorValues[ 0] <<= 1;	colorValues[ 0] |= (data >>	 7) & 1;
		colorValues[ 1] <<= 1;	colorValues[ 1] |= (data >>	 6) & 1;
		colorValues[ 2] <<= 1;	colorValues[ 2] |= (data >>	 5) & 1;
		colorValues[ 3] <<= 1;	colorValues[ 3] |= (data >>	 4) & 1;
		colorValues[ 4] <<= 1;	colorValues[ 4] |= (data >>	 3) & 1;
		colorValues[ 5] <<= 1;	colorValues[ 5] |= (data >>	 2) & 1;
		colorValues[ 6] <<= 1;	colorValues[ 6] |= (data >>	 1) & 1;
		colorValues[ 7] <<= 1;	colorValues[ 7] |= (data >>	 0) & 1;

		colorValues[ 8] <<= 1;	colorValues[ 8] |= (data >> 15) & 1;
		colorValues[ 9] <<= 1;	colorValues[ 9] |= (data >> 14) & 1;
		colorValues[10] <<= 1;	colorValues[10] |= (data >> 13) & 1;
		colorValues[11] <<= 1;	colorValues[11] |= (data >> 12) & 1;
		colorValues[12] <<= 1;	colorValues[12] |= (data >> 11) & 1;
		colorValues[13] <<= 1;	colorValues[13] |= (data >> 10) & 1;
		colorValues[14] <<= 1;	colorValues[14] |= (data >>	 9) & 1;
		colorValues[15] <<= 1;	colorValues[15] |= (data >>	 8) & 1;
	}
}



#endif


/*
 * $Log$
 * Revision 1.15  2001/10/30 22:59:34  standa
 * The resolution change is now possible through the fVDI driver.
 *
 * Revision 1.14  2001/10/29 23:14:17  standa
 * The HostScreen support for arbitrary destination BPP (8,16,24,32bit).
 *
 * Revision 1.13  2001/10/25 19:56:01  standa
 * The Log and Header CVS tags in the Log removed. Was recursing.
 *
 * Revision 1.12  2001/10/24 17:55:01  standa
 * The fVDI driver fixes. Finishing the functionality tuning.
 *
 * Revision 1.11  2001/10/23 21:28:49  standa
 * Several changes, fixes and clean up. Shouldn't crash on high resolutions.
 * hostscreen/gfx... methods have fixed the loop upper boundary. The interface
 * types have changed quite havily.
 *
 * Revision 1.10  2001/10/08 21:46:05  standa
 * The Header and Log CVS tags added.
 *
 * Revision 1.9  2001/10/01 22:22:41  standa
 * bitplaneToChunky conversion moved into HostScreen (inline - should be no performance penalty).
 * fvdidrv/blitArea form memory works in TC.
 *
 * Revision 1.8  2001/09/30 23:09:23  standa
 * The line logical operation added.
 * The first version of blitArea (screen to screen only).
 *
 * Revision 1.7  2001/09/24 23:16:28  standa
 * Another minor changes. some logical operation now works.
 * fvdidrv/fillArea and fvdidrv/expandArea got the first logOp handling.
 *
 * Revision 1.6  2001/09/20 18:12:09  standa
 * Off by one bug fixed in fillArea.
 * Separate functions for transparent and opaque background.
 * gfxPrimitives methods moved to the HostScreen
 *
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
