/*
 * hostscreen.cpp - host video routines
 *
 * Copyright (c) 2001-2003 STanda of ARAnyM developer team (see AUTHORS)
 *
 * This file is part of the ARAnyM project which builds a new and powerful
 * TOS/FreeMiNT compatible virtual machine running on almost any hardware.
 *
 * ARAnyM is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * ARAnyM is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ARAnyM; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _HOSTSCREEN_H
#define _HOSTSCREEN_H

# include <cstdlib>
# include <cstring>

#include <SDL.h>
#include <SDL_thread.h>

#include "parameters.h"

/**
 * This macro handles the endianity for 24 bit per item data
 **/
#if SDL_BYTEORDER == SDL_BIG_ENDIAN

#define putBpp24Pixel( address, color ) \
{ \
        ((Uint8*)(address))[0] = ((color) >> 16) & 0xff; \
        ((Uint8*)(address))[1] = ((color) >> 8) & 0xff; \
        ((Uint8*)(address))[2] = (color) & 0xff; \
}

#define getBpp24Pixel( address ) \
    ( ((uint32)(address)[0] << 16) | ((uint32)(address)[1] << 8) | (uint32)(address)[2] )

#else

#define putBpp24Pixel( address, color ) \
{ \
    ((Uint8*)(address))[0] = (color) & 0xff; \
        ((Uint8*)(address))[1] = ((color) >> 8) & 0xff; \
        ((Uint8*)(address))[2] = ((color) >> 16) & 0xff; \
}

#define getBpp24Pixel( address ) \
    ( ((uint32)(address)[2] << 16) | ((uint32)(address)[1] << 8) | (uint32)(address)[0] )

#endif


class HostScreen {
  private:
 	SDL_Surface *mainSurface;		// The main window surface
 	SDL_Surface *backgroundSurf;	// Background window surface
	SDL_Surface *surf;			// pointer to actual surface for VDI
  	bool GUIopened;
  	void allocateBackgroundSurf();
  	void freeBackgroundSurf();

	int selectVideoMode(SDL_Rect **modes, uint32 *width, uint32 *height);
	void searchVideoMode( uint32 *width, uint32 *height, uint32 *bpp );

	// OpenGL stuff
	SDL_Surface *SdlGlSurface;
	unsigned int SdlGlTexObj;
	unsigned int SdlGlTextureWidth;
	unsigned int SdlGlTextureHeight;
	uint8 *SdlGlTexture;
	int SdlGlWidth, SdlGlHeight;
	SDL_bool npot_texture;		/* Use non power of two texture ? */
	SDL_bool rect_texture;		/* Use rectangle texture ? */
	int rect_target;
	SDL_bool OpenGLVdi;			/* Using NF OpenGL VDI renderer ? */
	SDL_bool *dirty_rects;		/* Dirty rectangles list */
	int dirty_w,dirty_h;

  public:
	SDL_mutex   *screenLock;
	uint32 sdl_videoparams;
	uint32 width, height, bpp;
	bool   doUpdate; // the HW surface is available -> the SDL need not to update the surface after ->pixel access

	struct { // TOS palette (bpp < 16) to SDL color mapping
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
	                     uint16 pattern, uint32 fgColor, uint32 bgColor, uint16 logOp, bool last_pixel = true );
	void   gfxBoxColorPattern( int16 x1, int16 y1, int16 x2, int16 y2,
                               uint16 *areaPattern, uint32 fgColor, uint32 bgColor, uint16 logOp );

  public:
	HostScreen(void);
	~HostScreen(void);

	inline void lock();
	inline void unlock();

	inline bool	  renderBegin();
	inline void	  renderEnd();

	// the w, h should be width & height (but C++ complains -> 'if's in the implementation)
	void update( int32 x, int32 y, int32 w, int32 h, bool forced = false );
	void update( bool forced );
	void update();

	void OpenGLUpdate(void);	/* Full screen update with NF software VDI */
	void EnableOpenGLVdi(void);
	void DisableOpenGLVdi(void);

#ifdef SDL_GUI
	// GUI
	void openGUI();
	void closeGUI();
	bool isGUIopen()	{ return GUIopened; }
	// save and restore background under GUI
	void saveBackground();
	void restoreBackground();
	void blendBackgrounds();
	SDL_Surface *getPhysicalSurface() { return mainSurface; }
#endif

	uint32 getBpp();	/* Bytes per pixel */
	uint32 getBitsPerPixel(void);
	uint32 getPitch();
	uint32 getWidth();
	uint32 getHeight();
	uintptr getVideoramAddress();

	void   setPaletteColor( uint8 index, uint32 red, uint32 green, uint32 blue );
	uint32 getPaletteColor( uint8 index );
	void   updatePalette( uint16 colorCount );
	uint32 getColor( uint32 red, uint32 green, uint32 blue );

	void   setWindowSize( uint32 width, uint32 height, uint32 bpp );
	void   setRendering( bool render );

	// gfx Primitives draw functions
	uint32 getPixel( int16 x, int16 y );
	void   putPixel( int16 x, int16 y, uint32 color );
	void   putPixel( int16 x, int16 y, uint16 pattern, uint32 fgColor, uint32 bgColor, uint16 logOp );
	void   drawLine( int16 x1, int16 y1, int16 x2, int16 y2,
	                 uint16 pattern, uint32 fgColor, uint32 bgColor, uint16 logOp, bool last_pixel /*= true*/);
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

	// Toggle Window/FullScreen mode
	void   toggleFullScreen();

	int	refreshCounter;
	void	refresh(void);
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
	return 0;	// should never happen
}

inline uint32 HostScreen::getBpp()
{
#ifdef ENABLE_OPENGL
	if (bx_options.opengl.enabled) {
		switch(bpp) {
			case 15:
			case 16:
				return 2;
			case 24:
				return 3;
			case 32:
				return 4;
		}
		return 1;
	}
#endif
	return surf->format->BytesPerPixel;
}

inline uint32 HostScreen::getPitch() {
	return surf->pitch;
}

inline uint32 HostScreen::getWidth() {
	return width;
}

inline uint32 HostScreen::getHeight() {
	return height;
}

inline uintptr HostScreen::getVideoramAddress() {
	return (uintptr)surf->pixels;	/* FIXME maybe this should be mainSurface? */
}

inline void HostScreen::setPaletteColor( uint8 index, uint32 red, uint32 green, uint32 blue ) {
	SDL_Color& color = palette.standard[index];
	color.r = red; color.g = green; color.b = blue; // set the SDL standard RGB palette settings
	palette.native[index] = SDL_MapRGB( surf->format, red, green, blue ); // convert the color to native
}

inline uint32 HostScreen::getPaletteColor( uint8 index ) {
	return palette.native[index];
}

inline void HostScreen::updatePalette( uint16 colorCount ) {
	SDL_SetColors( surf, palette.standard, 0, colorCount );
}

inline uint32 HostScreen::getColor( uint32 red, uint32 green, uint32 blue ) {
	return SDL_MapRGB( surf->format, red, green, blue );
}


inline void HostScreen::lock() {
	while (SDL_mutexP(screenLock)==-1) {
		SDL_Delay(20);
		fprintf(stderr, "Couldn't lock mutex\n");
	}
}

inline void HostScreen::unlock() {
	while (SDL_mutexV(screenLock)==-1) {
		SDL_Delay(20);
		fprintf(stderr, "Couldn't unlock mutex\n");
	}
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

inline void HostScreen::drawLine( int16 x1, int16 y1, int16 x2, int16 y2, uint16 pattern, uint32 fgColor, uint32 bgColor, uint16 logOp, bool last_pixel = true )
{
	gfxLineColor( x1, y1, x2, y2, pattern, fgColor, bgColor, logOp, last_pixel ); // SDL_gfxPrimitives
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


/**
 * Performs conversion from the TOS's bitplane word order (big endian) data
 * into the native chunky color index.
 */
inline void HostScreen::bitplaneToChunky( uint16 *atariBitplaneData, uint16 bpp, uint8 colorValues[16] )
{
	uint32 a, b, c, d, x;

	/* Obviously the different cases can be broken out in various
	 * ways to lessen the amount of work needed for <8 bit modes.
	 * It's doubtful if the usage of those modes warrants it, though.
	 * The branches below should be ~100% correctly predicted and
	 * thus be more or less for free.
	 * Getting the palette values inline does not seem to help
	 * enough to worry about. The palette lookup is much slower than
	 * this code, though, so it would be nice to do something about it.
	 */
	if (bpp >= 4) {
		d = *(uint32 *)&atariBitplaneData[0];
		c = *(uint32 *)&atariBitplaneData[2];
		if (bpp == 4) {
			a = b = 0;
		} else {
			b = *(uint32 *)&atariBitplaneData[4];
			a = *(uint32 *)&atariBitplaneData[6];
		}
	} else {
		a = b = c = 0;
		if (bpp == 2) {
			d = *(uint32 *)&atariBitplaneData[0];
		} else {
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
			d = atariBitplaneData[0]<<16;
#else
			d = atariBitplaneData[0];
#endif
		}
	}

	x = a;
	a =  (a & 0xf0f0f0f0)       | ((c & 0xf0f0f0f0) >> 4);
	c = ((x & 0x0f0f0f0f) << 4) |  (c & 0x0f0f0f0f);
	x = b;
	b =  (b & 0xf0f0f0f0)       | ((d & 0xf0f0f0f0) >> 4);
	d = ((x & 0x0f0f0f0f) << 4) |  (d & 0x0f0f0f0f);

	x = a;
	a =  (a & 0xcccccccc)       | ((b & 0xcccccccc) >> 2);
	b = ((x & 0x33333333) << 2) |  (b & 0x33333333);
	x = c;
	c =  (c & 0xcccccccc)       | ((d & 0xcccccccc) >> 2);
	d = ((x & 0x33333333) << 2) |  (d & 0x33333333);

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	a = (a & 0x5555aaaa) | ((a & 0x00005555) << 17) | ((a & 0xaaaa0000) >> 17);
	b = (b & 0x5555aaaa) | ((b & 0x00005555) << 17) | ((b & 0xaaaa0000) >> 17);
	c = (c & 0x5555aaaa) | ((c & 0x00005555) << 17) | ((c & 0xaaaa0000) >> 17);
	d = (d & 0x5555aaaa) | ((d & 0x00005555) << 17) | ((d & 0xaaaa0000) >> 17);
	
	colorValues[ 8] = a;
	a >>= 8;
	colorValues[ 0] = a;
	a >>= 8;
	colorValues[ 9] = a;
	a >>= 8;
	colorValues[ 1] = a;
	
	colorValues[10] = b;
	b >>= 8;
	colorValues[ 2] = b;
	b >>= 8;
	colorValues[11] = b;
	b >>= 8;
	colorValues[ 3] = b;
	
	colorValues[12] = c;
	c >>= 8;
	colorValues[ 4] = c;
	c >>= 8;
	colorValues[13] = c;
	c >>= 8;
	colorValues[ 5] = c;
	
	colorValues[14] = d;
	d >>= 8;
	colorValues[ 6] = d;
	d >>= 8;
	colorValues[15] = d;
	d >>= 8;
	colorValues[ 7] = d;
#else
	a = (a & 0xaaaa5555) | ((a & 0x0000aaaa) << 15) | ((a & 0x55550000) >> 15);
	b = (b & 0xaaaa5555) | ((b & 0x0000aaaa) << 15) | ((b & 0x55550000) >> 15);
	c = (c & 0xaaaa5555) | ((c & 0x0000aaaa) << 15) | ((c & 0x55550000) >> 15);
	d = (d & 0xaaaa5555) | ((d & 0x0000aaaa) << 15) | ((d & 0x55550000) >> 15);

	colorValues[ 1] = a;
	a >>= 8;
	colorValues[ 9] = a;
	a >>= 8;
	colorValues[ 0] = a;
	a >>= 8;
	colorValues[ 8] = a;

	colorValues[ 3] = b;
	b >>= 8;
	colorValues[11] = b;
	b >>= 8;
	colorValues[ 2] = b;
	b >>= 8;
	colorValues[10] = b;

	colorValues[ 5] = c;
	c >>= 8;
	colorValues[13] = c;
	c >>= 8;
	colorValues[ 4] = c;
	c >>= 8;
	colorValues[12] = c;

	colorValues[ 7] = d;
	d >>= 8;
	colorValues[15] = d;
	d >>= 8;
	colorValues[ 6] = d;
	d >>= 8;
	colorValues[14] = d;
#endif
}

#endif

/*
 * $Log$
 * Revision 1.67  2006/08/25 18:34:22  philipp
 * Fixed p2c routine for big endian, colors are now correct.
 *
 * Revision 1.66  2006/08/25 13:16:10  pmandin
 * big endian patch from philipp donze
 *
 * Revision 1.65  2006/05/31 22:06:18  johan
 * "Real" planar to chunky code for little endian CPUs.
 *
 * Revision 1.64  2005/06/14 16:45:04  pmandin
 * Use rectangle texture when available
 *
 * Revision 1.63  2005/06/14 15:11:43  pmandin
 * Use non power of two texture when available
 *
 * Revision 1.62  2005/06/13 12:58:05  pmandin
 * Correctly setup shadow texture when smaller than asked
 *
 * Revision 1.61  2005/06/11 20:13:14  pmandin
 * Remove getComponent stuff
 *
 * Revision 1.60  2005/06/09 19:50:16  pmandin
 * Fixing getComponent function
 *
 * Revision 1.59  2005/06/08 19:33:02  pmandin
 * Add getComponent() function
 *
 * Revision 1.58  2005/06/08 18:14:59  pmandin
 * Update texture using a dirty rectangles list
 *
 * Revision 1.57  2005/05/12 07:55:08  pmandin
 * Keep OpenGL related stuff in hostscreen and nfvdi classes
 *
 * Revision 1.56  2005/05/11 20:19:26  pmandin
 * Add NF vdi OpenGL renderer
 *
 * Revision 1.55  2005/05/11 16:16:24  pmandin
 * Confused by bpp (bits per pixel) and bpp (bytes per pixel)
 *
 * Revision 1.53  2005/05/10 21:03:40  pmandin
 * Added getBpp function to FVDI driver
 *
 * Revision 1.52  2005/05/10 10:37:44  pmandin
 * Faster OpenGL rendering
 *
 * Revision 1.51  2005/05/05 22:47:02  milan
 * support of old headers removed
 *
 * Revision 1.50  2005/01/22 16:39:48  joy
 * --disable-gui now disables all SDL GUI related code
 *
 * Revision 1.49  2004/12/11 09:55:52  pmandin
 * Move update function from .h to .cpp, mandatory for NFOSMesa
 *
 * Revision 1.48  2004/04/26 07:25:59  standa
 * GPL header & comment adjustments.
 *
 * Revision 1.47  2004/01/08 21:44:34  pmandin
 * Missing inclusion of config.h for ENABLE_OPENGL
 *
 * Revision 1.46  2004/01/05 10:05:20  standa
 * Palette handling reworked. Old non-NF dispatch removed.
 *
 * Revision 1.45  2003/12/24 23:30:54  joy
 * when GUI is open the background is being updated with running ARAnyM
 *
 * Revision 1.44  2003/06/01 08:35:39  milan
 * MacOS X support updated and <SDL/> removed from includes, path to SDL headers must be fully defined
 *
 * Revision 1.43  2003/04/16 19:35:49  pmandin
 * Correct inclusion of SDL headers
 *
 * Revision 1.42  2003/02/19 09:02:56  standa
 * The bitplane modes expandArea fix.
 *
 * Revision 1.41  2002/12/02 16:54:38  milan
 * non-OpenGL support
 *
 * Revision 1.40  2002/12/01 10:28:29  pmandin
 * OpenGL rendering
 *
 * Revision 1.39  2002/10/15 21:26:53  milan
 * non-cheaders support (for MipsPro C/C++ compiler)
 *
 * Revision 1.38  2002/09/23 09:21:37  pmandin
 * Select best video mode
 *
 * Revision 1.37  2002/07/25 13:44:29  pmandin
 * gcc-3.1 does not like a parameter to be set both in the definition and the method (drawline)
 *
 * Revision 1.36  2002/07/20 12:41:19  joy
 * always update() the physical video surface
 *
 * Revision 1.35  2002/07/20 08:10:55  joy
 * GUI background saving/restoring fixed
 *
 * Revision 1.34  2002/07/19 12:25:00  joy
 * main and background video surfaces
 *
 * Revision 1.33  2002/06/26 21:06:27  joy
 * bool GUIopened flag added
 *
 * Revision 1.32  2002/06/09 20:06:07  joy
 * save_bkg/restore_bkg added (used in SDL GUI)
 *
 * Revision 1.31  2002/06/07 20:55:35  joy
 * added toggle window/fullscreen mode switch
 *
 * Revision 1.30  2002/04/22 18:30:50  milan
 * header files reform
 *
 * Revision 1.29  2002/02/27 12:08:01  milan
 * uae_u32 -> uintptr where it is necessary
 *
 * Revision 1.28  2002/01/09 19:37:33  standa
 * The fVDI driver patched to not to pollute the HostScreen class getPaletteColor().
 *
 * Revision 1.27  2002/01/08 22:40:00  standa
 * The palette fix and a little 8bit driver update.
 *
 * Revision 1.26  2002/01/08 21:20:57  standa
 * fVDI driver palette store on res change implemented.
 *
 * Revision 1.25  2001/12/17 09:38:09  standa
 * The inline update() function order change to not to cause warnings.
 *
 * Revision 1.24  2001/12/17 08:33:00  standa
 * Thread synchronization added. The check_event and fvdidriver actions are
 * synchronized each to other.
 *
 * Revision 1.23  2001/12/03 20:56:07  standa
 * The gfsprimitives library files removed. All the staff was moved and
 * adjusted directly into the HostScreen class.
 *
 * Revision 1.22  2001/11/29 23:51:56  standa
 * Johan Klockars <rand@cd.chalmers.se> fVDI driver changes.
 *
 * Revision 1.21  2001/11/21 17:26:26  standa
 * The BIGENDIAN bitplaneToChunky corrected.
 *
 * Revision 1.20  2001/11/19 01:37:35  standa
 * PaletteInversIndex search. Bugfix in fillArea in 8bit depth.
 *
 * Revision 1.19  2001/11/18 21:04:59  standa
 * The BIG endiam chunky to bitplane conversion fix.
 *
 * Revision 1.18  2001/11/11 22:09:17  joy
 * gcc warning fix
 *
 * Revision 1.17  2001/11/07 21:18:25  milan
 * SDL_CFLAGS in CXXFLAGS now.
 *
 * Revision 1.16  2001/11/04 23:17:08  standa
 * 8bit destination surface support in VIDEL. Blit routine optimalization.
 * Bugfix in compatibility modes palette copying.
 *
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
