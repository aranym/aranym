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

#include "parameters.h"

#include <SDL.h>
#include <SDL_thread.h>
#ifdef ENABLE_OPENGL
#include <SDL_opengl.h>
#endif

#include "dirty_rects.h"

class HostSurface;
class Logo;

class HostScreen: public DirtyRects
{
  private:
	int selectVideoMode(SDL_Rect **modes, uint32 *width, uint32 *height);
	void searchVideoMode( uint32 *width, uint32 *height, uint32 *bpp );

	void refreshVidel(void);
	void refreshLogo(void);
	void refreshNfvdi(void);
	void forceRefreshNfvdi(void);
	void refreshGui(void);

#ifdef ENABLE_OPENGL
	// OpenGL stuff
	SDL_Surface *SdlGlSurface;
	GLuint SdlGlTexObj;
	unsigned int SdlGlTextureWidth;
	unsigned int SdlGlTextureHeight;
	uint8 *SdlGlTexture;
	int SdlGlWidth, SdlGlHeight;
#endif
	SDL_bool npot_texture;		/* Use non power of two texture ? */
	SDL_bool rect_texture;		/* Use rectangle texture ? */
	int rect_target;
	SDL_bool OpenGLVdi;			/* Using NF OpenGL VDI renderer ? */
	SDL_bool *dirty_rects;		/* Dirty rectangles list */
	int dirty_w,dirty_h;

	Logo *logo;
	bool logo_present;
	bool clear_screen;

  protected:
 	SDL_Surface *mainSurface;		// The main window surface

	virtual void refreshScreen(void);
	virtual void drawSurfaceToScreen(HostSurface *hsurf, int *dst_x = NULL,
		int *dst_y = NULL);

  public:
	uint32 sdl_videoparams;
	uint32 width, height, bpp;

	uint16 snapCounter; // ALT+PrintScreen to make a snap?

  public:
	HostScreen(void);
	virtual ~HostScreen(void);

	void reset(void);

	void OpenGLUpdate(void);	/* Full screen update with NF software VDI */
	void EnableOpenGLVdi(void);
	void DisableOpenGLVdi(void);

	uint32 getBpp();	/* Bytes per pixel */
	uint32 getBitsPerPixel(void);
	uint32 getWidth(void);
	uint32 getHeight(void);

	void   setWindowSize( uint32 width, uint32 height, uint32 bpp );
	void   setRendering( bool render );

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
	void	forceRefreshScreen(void);

	bool	renderVidelSurface;
	void	setVidelRendering(bool videlRender);
	int	lastVidelWidth, lastVidelHeight, lastVidelBpp;

	/* Surface creation */
	virtual HostSurface *createSurface(int width, int height, int bpp);
	virtual HostSurface *createSurface(SDL_Surface *sdl_surf);
};

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
