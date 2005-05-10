/*
 * hostscreen.cpp - host video routines
 *
 * Copyright (c) 2001-2005 STanda of ARAnyM developer team (see AUTHORS)
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


#include "sysdeps.h"
#include "hardware.h"
#include "cpu_emulation.h"
#include "memory.h"
#include "hostscreen.h"
#include "parameters.h"
#include "gui-sdl/sdlgui.h"

#define DEBUG 0
#include "debug.h"

#ifdef ENABLE_OPENGL
#include <SDL_opengl.h>
#endif

#define RGB_BLACK     0x00000000
#define RGB_BLUE      0x000000ff
#define RGB_GREEN     0x00ff0000
#define RGB_CYAN      0x00ff00ff
#define RGB_RED       0xff000000
#define RGB_MAGENTA   0xff0000ff
#define RGB_LTGRAY    0xbbbb00bb
#define RGB_GRAY      0x88880088
#define RGB_LTBLUE    0x000000aa
#define RGB_LTGREEN   0x00aa0000
#define RGB_LTCYAN    0x00aa00aa
#define RGB_LTRED     0xaa000000
#define RGB_LTMAGENTA 0xaa0000aa
#define RGB_YELLOW    0xffff0000
#define RGB_LTYELLOW  0xaaaa0000
#define RGB_WHITE     0xffff00ff

static const unsigned long default_palette[] = {
    RGB_WHITE, RGB_RED, RGB_GREEN, RGB_YELLOW,
    RGB_BLUE, RGB_MAGENTA, RGB_CYAN, RGB_LTGRAY,
    RGB_GRAY, RGB_LTRED, RGB_LTGREEN, RGB_LTYELLOW,
    RGB_LTBLUE, RGB_LTMAGENTA, RGB_LTCYAN, RGB_BLACK
};

HostScreen::HostScreen(void) {
	for(int i=0; i<256; i++) {
		unsigned long color = default_palette[i%16];
		palette.standard[i].r = color >> 24;
		palette.standard[i].g = (color >> 16) & 0xff;
		palette.standard[i].b = color & 0xff;
	}

	// the counter init
	snapCounter = 0;

	screenLock = SDL_CreateMutex();

	backgroundSurf = NULL;
	GUIopened = false;

	mainSurface=NULL;

#ifdef ENABLE_OPENGL
	// OpenGL stuff
	SdlGlSurface=NULL;
	SdlGlTexture=NULL;
#endif /* ENABLE_OPENGL */
}

HostScreen::~HostScreen(void) {
	SDL_DestroyMutex(screenLock);

	if (backgroundSurf) {
		SDL_FreeSurface(backgroundSurf);
		backgroundSurf=NULL;
	}

	// OpenGL stuff
#ifdef ENABLE_OPENGL
	if (bx_options.opengl.enabled) {
		if (mainSurface) {
			SDL_FreeSurface(mainSurface);
			mainSurface=NULL;
		}

		if (SdlGlTexture) {
			glDeleteTextures(1, &SdlGlTexObj);
			SdlGlTexture=NULL;
		}
	}
#endif
}

void HostScreen::makeSnapshot()
{
	char filename[15];
	sprintf( filename, "snap%03d.bmp", snapCounter++ );

	SDL_SaveBMP( surf, filename );
}


void HostScreen::toggleFullScreen()
{
	bx_options.video.fullscreen = !bx_options.video.fullscreen;
	sdl_videoparams ^= SDL_FULLSCREEN;
	if(SDL_WM_ToggleFullScreen(mainSurface) == 0) {
		// SDL_WM_ToggleFullScreen() did not work.
		// We have to change video mode "by hand".
		SDL_Surface *temp = SDL_ConvertSurface(mainSurface, mainSurface->format,
		                                       mainSurface->flags);
		if (temp == NULL)
			bug("toggleFullScreen: Unable to save screen content.");

		mainSurface = SDL_SetVideoMode(width, height, bpp, sdl_videoparams);
		if (mainSurface == NULL)
			bug("toggleFullScreen: Unable to set new video mode.");
		if (mainSurface->format->BitsPerPixel <= 8)
			SDL_SetColors(mainSurface, temp->format->palette->colors, 0,
			              temp->format->palette->ncolors);

		if (SDL_BlitSurface(temp, NULL, mainSurface, NULL) != 0)
			bug("toggleFullScreen: Unable to restore screen content.");
		SDL_FreeSurface(temp);

		if (isGUIopen() == 0)
			surf = mainSurface;
	}
}

#ifdef SDL_GUI
void HostScreen::allocateBackgroundSurf()
{
	// allocate new background video surface
	if (backgroundSurf != NULL)
		panicbug("Memory leak? The background video surface should not be allocated.");

	backgroundSurf = SDL_ConvertSurface(mainSurface, mainSurface->format, mainSurface->flags);

	D(bug("Allocating background video surface"));
}

void HostScreen::freeBackgroundSurf()
{
	// free background video surface
	if (backgroundSurf != NULL) {
		D(bug("Freeing background video surface"));
		SDL_FreeSurface(backgroundSurf);
		backgroundSurf = NULL;
	}
}

void HostScreen::openGUI()
{
	D(bug("open GUI"));
	if (isGUIopen()) {
		D(bug("GUI is already open!"));
		return;
	}
	allocateBackgroundSurf();
	GUIopened = true;
}

void HostScreen::closeGUI()
{
	D(bug("close GUI"));
	// update the main surface and then redirect VDI to it
	restoreBackground();
	surf = mainSurface;			// redirect VDI to main surface
	D(bug("VDI redirected back to main video surface"));
	freeBackgroundSurf();
	GUIopened = false;
}

void HostScreen::saveBackground()
{
	if (backgroundSurf != NULL) {
		SDL_BlitSurface(mainSurface, NULL, backgroundSurf, NULL);
		surf = backgroundSurf;	// redirect VDI to background surface
		D(bug("video surface saved to background, VDI redirected"));
	}
}

void HostScreen::restoreBackground()
{
	if (backgroundSurf != NULL) {
		SDL_BlitSurface(backgroundSurf, NULL, mainSurface, NULL);
		update(true);
		D(bug("video surface restored"));
	}
}
void HostScreen::blendBackgrounds()
{
	if (backgroundSurf != NULL) {
		SDL_Rect *Rect;

		Rect = SDLGui_GetFirstBackgroundRect();
		while (Rect != NULL) {
			SDL_BlitSurface(backgroundSurf, Rect, mainSurface, Rect);
			Rect = SDLGui_GetNextBackgroundRect();
		}
		update(true);
	}
}
#endif /* SDL_GUI */

int HostScreen::selectVideoMode(SDL_Rect **modes, uint32 *width, uint32 *height)
{
	int i, bestw, besth;

	/* Search the smallest nearest mode */
	bestw = modes[0]->w;
	besth = modes[0]->h;
	for (i=0;modes[i]; ++i) {
		if ((modes[i]->w >= *width) && (modes[i]->h >= *height)) {
			if ((modes[i]->w < bestw) || (modes[i]->h < besth)) {
				bestw = modes[i]->w;
				besth = modes[i]->h;
			}			
		}
	}

	*width = bestw;
	*height = besth;
	D(bug("hostscreen: video mode found: %dx%d",*width,*height));

	return 1;
}

void HostScreen::searchVideoMode( uint32 *width, uint32 *height, uint32 *bpp )
{
	SDL_Rect **modes;
	SDL_PixelFormat pixelformat;
	int modeflags;

	/* Search in available modes the best suited */
	D(bug("hostscreen: video mode asked: %dx%dx%d",*width,*height,*bpp));

	if ((*width == 0) || (*height == 0)) {
		*width = 640;
		*height = 480;
	}

	/* Read available video modes */
	modeflags = SDL_HWSURFACE | SDL_HWPALETTE;
	if (bx_options.video.fullscreen)
		modeflags |= SDL_FULLSCREEN;

	/*--- Search a video mode with asked bpp ---*/
	if (*bpp != 0) {
		pixelformat.BitsPerPixel = *bpp;
		modes = SDL_ListModes(&pixelformat, modeflags);
		if ((modes != (SDL_Rect **) 0) && (modes != (SDL_Rect **) -1)) {
			D(bug("hostscreen: searching a good video mode (any bpp)"));
			if (selectVideoMode(modes,width,height)) {
				D(bug("hostscreen: video mode selected: %dx%dx%d",*width,*height,*bpp));
				return;
			}
		}
	}

	/*--- Search a video mode with any bpp ---*/
	modes = SDL_ListModes(NULL, modeflags);
	if ((modes != (SDL_Rect **) 0) && (modes != (SDL_Rect **) -1)) {
		D(bug("hostscreen: searching a good video mode"));
		if (selectVideoMode(modes,width,height)) {
			D(bug("hostscreen: video mode selected: %dx%dx%d",*width,*height,*bpp));
			return;
		}
	}

	if (modes == (SDL_Rect **) 0) {
		D(bug("hostscreen: No modes available"));
	}

	if (modes == (SDL_Rect **) -1) {
		/* Any mode available */
		D(bug("hostscreen: Any modes available"));
	}

	D(bug("hostscreen: video mode selected: %dx%dx%d",*width,*height,*bpp));
}

void HostScreen::setWindowSize( uint32 width, uint32 height, uint32 bpp )
{
	if (bx_options.autozoom.fixedsize) {
		width = bx_options.autozoom.width;
		height = bx_options.autozoom.height;
	}

	// Select a correct video mode
	searchVideoMode(&width, &height, &bpp);	

	this->width	 = width;
	this->height = height;
	this->bpp = bpp;

	// SelectVideoMode();
	sdl_videoparams = SDL_HWSURFACE | SDL_HWPALETTE;
	if (bx_options.video.fullscreen)
		sdl_videoparams |= SDL_FULLSCREEN;

#ifdef ENABLE_OPENGL
	if (bx_options.opengl.enabled) {
		GLint MaxTextureSize;
		int filtering;		

		/* Set video mode if not already done */
		if (SdlGlSurface==NULL) {
			sdl_videoparams |= SDL_OPENGL;

/*			SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, bx_options.opengl.bpp);*/
			SdlGlSurface = SDL_SetVideoMode(bx_options.opengl.width, bx_options.opengl.height, 0 /*bx_options.opengl.bpp*/, sdl_videoparams);

			glViewport(0, 0, bx_options.opengl.width, bx_options.opengl.height);

			/* Projection matrix */
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			gluOrtho2D(0.0, bx_options.opengl.width, bx_options.opengl.height, 0.0);

			/* Texture matrix */
			glMatrixMode(GL_TEXTURE);
			glLoadIdentity();

			/* Model view matrix */
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();

			/* Enable texturing */
			glEnable(GL_TEXTURE_2D);
			{
				glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
			}
			D(bug("gl: video mode set"));
		}

		/* Create a surface for Aranym */
		if (mainSurface) {
			SDL_FreeSurface(mainSurface);
		}
		
		this->bpp = bpp = 32;

		/* Create a texture */
		if (SdlGlTexture) {
			glDeleteTextures(1, &SdlGlTexObj);
		}

		glGetIntegerv(GL_MAX_TEXTURE_SIZE, &MaxTextureSize);

		D(bug("gl: need at least a %dx%d texture",width,height));
		D(bug("gl: texture is at most a %dx%d texture",MaxTextureSize,MaxTextureSize));

		/* Calculate the smallest needed texture */
		SdlGlTextureWidth = SdlGlTextureHeight = MaxTextureSize;
		if (width<SdlGlTextureWidth) {
			while (((SdlGlTextureWidth>>1)>width) && (SdlGlTextureWidth>64)) {
				SdlGlTextureWidth>>=1;
			}
		}
		if (height<SdlGlTextureHeight) {
			while (((SdlGlTextureHeight>>1)>height) && (SdlGlTextureHeight>64)) {
				SdlGlTextureHeight>>=1;
			}
		}

		D(bug("gl: texture will be %dx%d texture",SdlGlTextureWidth,SdlGlTextureHeight));

		mainSurface = SDL_CreateRGBSurface(SDL_SWSURFACE, SdlGlTextureWidth,SdlGlTextureHeight,bpp, 255<<16,255<<8,255,255<<24);
		SdlGlTexture = (uint8 *) (mainSurface->pixels);

		glGenTextures(1, &SdlGlTexObj);
		glBindTexture(GL_TEXTURE_2D, SdlGlTexObj);

		filtering = GL_NEAREST;		
		if (bx_options.opengl.filtered) {
			filtering = GL_LINEAR;
		}
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filtering); // scale when image bigger than texture
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filtering); // scale when image smaller than texture

		glTexImage2D(GL_TEXTURE_2D, 0, 4, SdlGlTextureWidth, SdlGlTextureHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, SdlGlTexture);

		D(bug("gl: texture created"));

		/* Activate autozoom if texture smaller than screen */
		if ((width>SdlGlTextureWidth) || (height>SdlGlTextureHeight)) {
			this->width = width = SdlGlTextureWidth;
			this->height = height = SdlGlTextureHeight;
			bx_options.autozoom.enabled = true;
			bx_options.autozoom.integercoefs = false;

			D(bug("gl: autozoom enabled"));
		} else {
			bx_options.autozoom.enabled = false;
			bx_options.autozoom.integercoefs = false;
			D(bug("gl: autozoom disabled"));
		}
	}
	else
#endif /* ENABLE_OPENGL */
	{
		mainSurface = SDL_SetVideoMode(width, height, bpp, sdl_videoparams);
	}

#ifdef SDL_GUI
	if (isGUIopen()) {
		freeBackgroundSurf();
		allocateBackgroundSurf();
		saveBackground();
		// force SDL GUI to redraw the dialog because resolution has changed
		SDL_Event event;
		event.type = SDL_USEREVENT;
		event.user.code = SDL_USEREVENT; // misused this code for signalizing the resolution change. Did that because I knew the code was unique (needed something distinguishable from keyboard and mouse codes that are sent by the same event name from the input checking thread)
		SDL_PeepEvents(&event, 1, SDL_ADDEVENT, SDL_EVENTMASK(SDL_USEREVENT));
	}
	else
#endif /* SDL_GUI */
	{
		surf = mainSurface;
	}

	SDL_WM_SetCaption(VERSION_STRING, "ARAnyM");

	// update the surface's palette
	updatePalette( 256 );

	D(bug("Surface Pitch = %d, width = %d, height = %d", surf->pitch, surf->w, surf->h));
	D(bug("Must Lock? %s", SDL_MUSTLOCK(surf) ? "YES" : "NO"));

	// is the SDL_update needed?
	doUpdate = ( surf->flags & SDL_HWSURFACE ) == 0;

	renderBegin();

	VideoRAMBaseHost = (uint8 *) surf->pixels;
	InitVMEMBaseDiff(VideoRAMBaseHost, VideoRAMBase);
	D(bug("VideoRAM starts at %p (%08x)", VideoRAMBaseHost, VideoRAMBase));
	D(bug("surf->pixels = %x, getVideoSurface() = %x",
			VideoRAMBaseHost, SDL_GetVideoSurface()->pixels));

	renderEnd();

	D(bug("Pixel format:bitspp=%d, tmasks r=%04x g=%04x b=%04x"
			", tshifts r=%d g=%d b=%d"
			", tlosses r=%d g=%d b=%d",
			surf->format->BitsPerPixel,
			surf->format->Rmask, surf->format->Gmask, surf->format->Bmask,
			surf->format->Rshift, surf->format->Gshift, surf->format->Bshift,
			surf->format->Rloss, surf->format->Gloss, surf->format->Bloss));
}

/*
extern "C" {
	static void getBinary( uint16 data, char *buffer ) {
		for( uint16 i=0; i<=15; i++ ) {
			buffer[i] = (data & 1)?'1':' ';
			data>>=1;
		}
		buffer[16]='\0';
	}
}
*/


inline void HostScreen::putPixel( int16 x, int16 y, uint16 pattern, uint32 fgColor, uint32 bgColor, uint16 logOp )
{
	switch (logOp) {
		case 1:
			gfxFastPixelColorNolock( x, y, pattern ? fgColor : bgColor );
			break;
		case 2:
			if ( pattern )
				gfxFastPixelColorNolock( x, y, fgColor );
			break;
		case 3:
			if ( pattern )
				gfxFastPixelColorNolock( x, y, ~ gfxGetPixel( x, y ) );
			break;
		case 4:
			if ( ! pattern )
				gfxFastPixelColorNolock( x, y, fgColor );
			break;
	}
}


void HostScreen::gfxHLineColor ( int16 x1, int16 x2, int16 y, uint16 pattern, uint32 fgColor, uint32 bgColor, uint16 logOp )
{
	uint8 *pixel,*pixellast;
	int dx;
	int pixx, pixy;
	int16 w;
	int16 xtmp;
	uint8 ppos;

	/* Swap x1, x2 if required */
	if (x1>x2) {
		xtmp=x1; x1=x2; x2=xtmp;
	}

	/* Calculate width */
	w=x2-x1;

	/* Sanity check on width */
	if (w<0)
		return;

	/* More variable setup */
	dx=w+1;
	pixx = surf->format->BytesPerPixel;
	pixy = surf->pitch;
	pixel = ((uint8*)surf->pixels) + pixx * (int)x1 + pixy * (int)y;
	ppos = 0;

	D(bug("HLn %3d,%3d,%3d", x1, x2, y));

	/* Draw */
	switch(surf->format->BytesPerPixel) {
		case 1:
			pixellast = pixel + dx;
			switch (logOp) {
				case 1:
					for (; pixel<pixellast; pixel += pixx)
						*(uint8*)pixel = (( ( pattern & ( 1 << ( (ppos++) & 0xf ) )) != 0 ) ? fgColor : bgColor);
					break;
				case 2:
					for (; pixel<pixellast; pixel += pixx)
						if ( ( pattern & ( 1 << ( (ppos++) & 0xf ) )) != 0 )
							*(uint8*)pixel = fgColor;
					break;
				case 3:
					for (; pixel<pixellast; pixel += pixx)
						if ( ( pattern & ( 1 << ( (ppos++) & 0xf ) )) != 0 )
							*(uint8*)pixel = ~(*(uint8*)pixel);
					break;
				case 4:
					for (; pixel<pixellast; pixel += pixx)
						if ( ( pattern & ( 1 << ( (ppos++) & 0xf ) )) == 0 )
							*(uint8*)pixel = fgColor;
					break;
			}
			break;
		case 2:
			pixellast = pixel + dx + dx;
			switch (logOp) {
				case 1:
					for (; pixel<pixellast; pixel += pixx)
						*(uint16*)pixel = (( ( pattern & ( 1 << ( (ppos++) & 0xf ) )) != 0 ) ? fgColor : bgColor);
					break;
				case 2:
					for (; pixel<pixellast; pixel += pixx)
						if ( ( pattern & ( 1 << ( (ppos++) & 0xf ) )) != 0 )
							*(uint16*)pixel = fgColor;
					break;
				case 3:
					for (; pixel<pixellast; pixel += pixx)
						if ( ( pattern & ( 1 << ( (ppos++) & 0xf ) )) != 0 )
							*(uint16*)pixel = ~(*(uint16*)pixel);
					break;
				case 4:
					for (; pixel<pixellast; pixel += pixx)
						if ( ( pattern & ( 1 << ( (ppos++) & 0xf ) )) == 0 )
							*(uint16*)pixel = fgColor;
					break;
			}
			break;
		case 3:
			pixellast = pixel + dx + dx + dx;
			switch (logOp) {
				case 1:
					for (; pixel<pixellast; pixel += pixx)
						putBpp24Pixel( pixel, (( ( pattern & ( 1 << ( (ppos++) & 0xf ) )) != 0 ) ? fgColor : bgColor ) );
					break;
				case 2:
					for (; pixel<pixellast; pixel += pixx)
						if ( ( pattern & ( 1 << ( (ppos++) & 0xf ) )) != 0 )
							putBpp24Pixel( pixel, fgColor );
					break;
				case 3:
					for (; pixel<pixellast; pixel += pixx)
						if ( ( pattern & ( 1 << ( (ppos++) & 0xf ) )) != 0 )
							putBpp24Pixel( pixel, ~ getBpp24Pixel( pixel ) );
					break;
				case 4:
					for (; pixel<pixellast; pixel += pixx)
						if ( ( pattern & ( 1 << ( (ppos++) & 0xf ) )) == 0 )
							putBpp24Pixel( pixel, fgColor );
					break;
			}
			break;
		default: /* case 4*/
			dx = dx + dx;
			pixellast = pixel + dx + dx;
			switch (logOp) {
				case 1:
					for (; pixel<pixellast; pixel += pixx)
						*(uint32*)pixel = (( ( pattern & ( 1 << ( (ppos++) & 0xf ) )) != 0 ) ? fgColor : bgColor);
					break;
				case 2:
					for (; pixel<pixellast; pixel += pixx)
						if ( ( pattern & ( 1 << ( (ppos++) & 0xf ) )) != 0 )
							*(uint32*)pixel = fgColor;
					break;
				case 3:
					for (; pixel<pixellast; pixel += pixx)
						if ( ( pattern & ( 1 << ( (ppos++) & 0xf ) )) != 0 )
							*(uint32*)pixel = ~(*(uint32*)pixel);
					break;
				case 4:
					for (; pixel<pixellast; pixel += pixx)
						if ( ( pattern & ( 1 << ( (ppos++) & 0xf ) )) == 0 )
							*(uint32*)pixel = fgColor;
					break;
			}
			break;
	}
}


void HostScreen::gfxVLineColor( int16 x, int16 y1, int16 y2,
								uint16 pattern, uint32 fgColor, uint32 bgColor, uint16 logOp )
{
	uint8 *pixel, *pixellast;
	int dy;
	int pixx, pixy;
	int16 h;
	int16 ytmp;
	uint8 ppos;

	/* Swap y1, y2 if required */
	if (y1>y2) {
		ytmp=y1; y1=y2; y2=ytmp;
	}

	/* Calculate height */
	h=y2-y1;

	/* Sanity check on height */
	if (h<0)
		return;

	ppos = 0;

	D(bug("VLn %3d,%3d,%3d", x, y1, y2));

	/* More variable setup */
	dy=h+1;
	pixx = surf->format->BytesPerPixel;
	pixy = surf->pitch;
	pixel = ((uint8*)surf->pixels) + pixx * (int)x + pixy * (int)y1;
	pixellast = pixel + pixy*dy;

	/* Draw */
	switch(surf->format->BytesPerPixel) {
		case 1:
			switch (logOp) {
				case 1:
					for (; pixel<pixellast; pixel += pixy)
						*(uint8*)pixel = (( ( pattern & ( 1 << ( (ppos++) & 0xf ) )) != 0 ) ? fgColor : bgColor);
					break;
				case 2:
					for (; pixel<pixellast; pixel += pixy)
						if ( ( pattern & ( 1 << ( (ppos++) & 0xf ) )) != 0 )
							*(uint8*)pixel = fgColor;
					break;
				case 3:
					for (; pixel<pixellast; pixel += pixy)
						if ( ( pattern & ( 1 << ( (ppos++) & 0xf ) )) != 0 )
							*(uint8*)pixel = ~(*(uint8*)pixel);
					break;
				case 4:
					for (; pixel<pixellast; pixel += pixy)
						if ( ( pattern & ( 1 << ( (ppos++) & 0xf ) )) == 0 )
							*(uint8*)pixel = fgColor;
					break;
			}
			break;
		case 2:
			switch (logOp) {
				case 1:
					for (; pixel<pixellast; pixel += pixy)
						*(uint16*)pixel = (( ( pattern & ( 1 << ( (ppos++) & 0xf ) )) != 0 ) ? fgColor : bgColor);
					break;
				case 2:
					for (; pixel<pixellast; pixel += pixy)
						if ( ( pattern & ( 1 << ( (ppos++) & 0xf ) )) != 0 )
							*(uint16*)pixel = fgColor;
					break;
				case 3:
					for (; pixel<pixellast; pixel += pixy)
						if ( ( pattern & ( 1 << ( (ppos++) & 0xf ) )) != 0 )
							*(uint16*)pixel = ~(*(uint16*)pixel);
					break;
				case 4:
					for (; pixel<pixellast; pixel += pixy)
						if ( ( pattern & ( 1 << ( (ppos++) & 0xf ) )) == 0 )
							*(uint16*)pixel = fgColor;
					break;
			}
			break;
		case 3:
			switch (logOp) {
				case 1:
					for (; pixel<pixellast; pixel += pixy)
						putBpp24Pixel( pixel, (( ( pattern & ( 1 << ( (ppos++) & 0xf ) )) != 0 ) ? fgColor : bgColor ) );
					break;
				case 2:
					for (; pixel<pixellast; pixel += pixy)
						if ( ( pattern & ( 1 << ( (ppos++) & 0xf ) )) != 0 )
							putBpp24Pixel( pixel, fgColor );
					break;
				case 3:
					for (; pixel<pixellast; pixel += pixy)
						if ( ( pattern & ( 1 << ( (ppos++) & 0xf ) )) != 0 )
							putBpp24Pixel( pixel, ~ getBpp24Pixel( pixel ) );
					break;
				case 4:
					for (; pixel<pixellast; pixel += pixy)
						if ( ( pattern & ( 1 << ( (ppos++) & 0xf ) )) == 0 )
							putBpp24Pixel( pixel, fgColor );
					break;
			}
			break;
		default: /* case 4*/
			switch (logOp) {
				case 1:
					for (; pixel<pixellast; pixel += pixy)
						*(uint32*)pixel = (( ( pattern & ( 1 << ( (ppos++) & 0xf ) )) != 0 ) ? fgColor : bgColor);
					break;
				case 2:
					for (; pixel<pixellast; pixel += pixy)
						if ( ( pattern & ( 1 << ( (ppos++) & 0xf ) )) != 0 )
							*(uint32*)pixel = fgColor;
					break;
				case 3:
					for (; pixel<pixellast; pixel += pixy)
						if ( ( pattern & ( 1 << ( (ppos++) & 0xf ) )) != 0 )
							*(uint32*)pixel = ~(*(uint32*)pixel);
					break;
				case 4:
					for (; pixel<pixellast; pixel += pixy)
						if ( ( pattern & ( 1 << ( (ppos++) & 0xf ) )) == 0 )
							*(uint32*)pixel = fgColor;
					break;
			}
			break;
	}
}


/* ----- Line */

/* Non-alpha line drawing code adapted from routine			 */
/* by Pete Shinners, pete@shinners.org						 */
/* Originally from pygame, http://pygame.seul.org			 */

void HostScreen::gfxLineColor( int16 x1, int16 y1, int16 x2, int16 y2,
                               uint16 pattern, uint32 fgColor, uint32 bgColor, uint16 logOp,
                               bool last_pixel )
{
	int16 pixx, pixy;
	int16 x, y;
	int16 dx, dy;
	int16 sx, sy;
	int16 swaptmp;
	uint8 *pixel;
	uint8 ppos;

	/* Test for special cases of straight lines or single point */
	if (x1 == x2) {
		if (y1 < y2) {
			gfxVLineColor(x1, y1, y2 - !last_pixel, pattern, fgColor, bgColor, logOp);
			return;
		} else if (y1 > y2) {
			gfxVLineColor(x1, y2 + !last_pixel, y1, pattern, fgColor, bgColor, logOp);
			return;
		} else if (last_pixel) {
			putPixel( x1, y1, pattern & 0x8000, fgColor, bgColor, logOp );
		}
	}
	if (y1 == y2) {
		if (x1 < x2) {
			gfxHLineColor(x1, x2 - !last_pixel, y1, pattern, fgColor, bgColor, logOp);
			return;
		} else if (x1 > x2) {
			gfxHLineColor(x2 + !last_pixel, x1, y1, pattern, fgColor, bgColor, logOp);
			return;
		}
	}

	D(bug("CLn %3d,%3d,%3d,%3d", x1, x2, y1, y2));

	/* Variable setup */
	dx = x2 - x1;
	dy = y2 - y1;
	sx = (dx >= 0) ? 1 : -1;
	sy = (dy >= 0) ? 1 : -1;
	ppos = 0;

	/* More variable setup */
	dx = sx * dx + 1;
	dy = sy * dy + 1;
	pixx = surf->format->BytesPerPixel;
	pixy = surf->pitch;
	pixel = ((uint8*)surf->pixels) + pixx * (uint32)x1 + pixy * (uint32)y1;
	pixx *= sx;
	pixy *= sy;
	if (dx < dy) {
		swaptmp = dx; dx = dy; dy = swaptmp;
		swaptmp = pixx; pixx = pixy; pixy = swaptmp;
	}

	//	D2(bug("ln pix pixx, pixy: %d,%d : %d,%d : %x, %d", sx, sy, dx, dy, pixx, pixy));

	/* Draw */
	x = !last_pixel;	// 0 if last pixel should be drawn, else 1
	y = 0;
	switch(surf->format->BytesPerPixel) {
		case 1:
			switch (logOp) {
				case 1:
					for (; x < dx; x++, pixel += pixx) {
						*(uint8*)pixel = (( ( pattern & ( 1 << ( (ppos++) & 0xf ) )) != 0 ) ? fgColor : bgColor);

						y += dy;
						if (y >= dx) {
							y -= dx;
							pixel += pixy;
						}
					}
					break;
				case 2:
					for (; x < dx; x++, pixel += pixx) {
						if ( ( pattern & ( 1 << ( (ppos++) & 0xf ) )) != 0 )
							*(uint8*)pixel = fgColor;

						y += dy;
						if (y >= dx) {
							y -= dx;
							pixel += pixy;
						}
					}
					break;
				case 3:
					for (; x < dx; x++, pixel += pixx) {
						if ( ( pattern & ( 1 << ( (ppos++) & 0xf ) )) != 0 )
							*(uint8*)pixel = ~(*(uint8*)pixel);

						y += dy;
						if (y >= dx) {
							y -= dx;
							pixel += pixy;
						}
					}
					break;
				case 4:
					for (; x < dx; x++, pixel += pixx) {
						if ( ( pattern & ( 1 << ( (ppos++) & 0xf ) )) == 0 )
							*(uint8*)pixel = fgColor;

						y += dy;
						if (y >= dx) {
							y -= dx;
							pixel += pixy;
						}
					}
					break;
			}
			break;
		case 2:
			switch (logOp) {
				case 1:
					for (; x < dx; x++, pixel += pixx) {
						*(uint16*)pixel = (( ( pattern & ( 1 << ( (ppos++) & 0xf ) )) != 0 ) ? fgColor : bgColor);

						y += dy;
						if (y >= dx) {
							y -= dx;
							pixel += pixy;
						}
					}
					break;
				case 2:
					for (; x < dx; x++, pixel += pixx) {
						if ( ( pattern & ( 1 << ( (ppos++) & 0xf ) )) != 0 )
							*(uint16*)pixel = fgColor;

						y += dy;
						if (y >= dx) {
							y -= dx;
							pixel += pixy;
						}
					}
					break;
				case 3:
					for (; x < dx; x++, pixel += pixx) {
						if ( ( pattern & ( 1 << ( (ppos++) & 0xf ) )) != 0 )
							*(uint16*)pixel = ~(*(uint16*)pixel);

						y += dy;
						if (y >= dx) {
							y -= dx;
							pixel += pixy;
						}
					}
					break;
				case 4:
					for (; x < dx; x++, pixel += pixx) {
						if ( ( pattern & ( 1 << ( (ppos++) & 0xf ) )) == 0 )
							*(uint16*)pixel = fgColor;

						y += dy;
						if (y >= dx) {
							y -= dx;
							pixel += pixy;
						}
					}
					break;
			}
			break;
		case 3:
			switch (logOp) {
				case 1:
					for (; x < dx; x++, pixel += pixx) {
						putBpp24Pixel( pixel, (( ( pattern & ( 1 << ( (ppos++) & 0xf ) )) != 0 ) ? fgColor : bgColor) );

						y += dy;
						if (y >= dx) {
							y -= dx;
							pixel += pixy;
						}
					}
					break;
				case 2:
					for (; x < dx; x++, pixel += pixx) {
						if ( ( pattern & ( 1 << ( (ppos++) & 0xf ) )) != 0 )
							putBpp24Pixel( pixel, fgColor );

						y += dy;
						if (y >= dx) {
							y -= dx;
							pixel += pixy;
						}
					}
					break;
				case 3:
					for (; x < dx; x++, pixel += pixx) {
						if ( ( pattern & ( 1 << ( (ppos++) & 0xf ) )) != 0 )
							putBpp24Pixel( pixel, ~ getBpp24Pixel( pixel ) );

						y += dy;
						if (y >= dx) {
							y -= dx;
							pixel += pixy;
						}
					}
					break;
				case 4:
					for (; x < dx; x++, pixel += pixx) {
						if ( ( pattern & ( 1 << ( (ppos++) & 0xf ) )) == 0 )
							putBpp24Pixel( pixel, fgColor );

						y += dy;
						if (y >= dx) {
							y -= dx;
							pixel += pixy;
						}
					}
					break;
			}
			break;
		default: /* case 4 */
			switch (logOp) {
				case 1:
					for (; x < dx; x++, pixel += pixx) {
						*(uint32*)pixel = (( ( pattern & ( 1 << ( (ppos++) & 0xf ) )) != 0 ) ? fgColor : bgColor);

						y += dy;
						if (y >= dx) {
							y -= dx;
							pixel += pixy;
						}
					}
					break;
				case 2:
					for (; x < dx; x++, pixel += pixx) {
						if ( ( pattern & ( 1 << ( (ppos++) & 0xf ) )) != 0 )
							*(uint32*)pixel = fgColor;

						y += dy;
						if (y >= dx) {
							y -= dx;
							pixel += pixy;
						}
					}
					break;
				case 3:
					for (; x < dx; x++, pixel += pixx) {
						if ( ( pattern & ( 1 << ( (ppos++) & 0xf ) )) != 0 )
							*(uint32*)pixel = ~(*(uint32*)pixel);

						y += dy;
						if (y >= dx) {
							y -= dx;
							pixel += pixy;
						}
					}
					break;
				case 4:
					for (; x < dx; x++, pixel += pixx) {
						if ( ( pattern & ( 1 << ( (ppos++) & 0xf ) )) == 0 )
							*(uint32*)pixel = fgColor;

						y += dy;
						if (y >= dx) {
							y -= dx;
							pixel += pixy;
						}
					}
					break;
			}
			break;
	}
}




/**
 * Derived from the SDL_gfxPrimitives::boxColor().
 * The colors are in the destination surface format here.
 * The trivial cases optimalization removed.
 *
 * @author STanda
 **/
void HostScreen::gfxBoxColorPattern (int16 x, int16 y, int16 w, int16 h,
									 uint16 *areaPattern, uint32 fgColor, uint32 bgColor, uint16 logOp)
{
	uint8 *pixel, *pixellast;
	int16 pixx, pixy;
	int16 i;
	int16 dx=w;
	int16 dy=h;

	/* More variable setup */
	pixx = surf->format->BytesPerPixel;
	pixy = surf->pitch;
	pixel = ((uint8*)surf->pixels) + pixx * (int32)x + pixy * (int32)y;
	pixellast = pixel + pixy*dy;

	// STanda // FIXME here the pattern should be checked out of the loops for performance
			  // but for now it is good enough (if there is no pattern -> another switch?)

	/* Draw */
	switch(surf->format->BytesPerPixel) {
		case 1:
			pixy -= (pixx*dx);
			switch (logOp) {
				case 1:
					for (; pixel<pixellast; pixel += pixy) {
						uint16 pattern = areaPattern ? areaPattern[ y++ & 0xf ] : 0xffff;

						for (i=0; i<dx; i++) {
							*(uint8*)pixel = (( ( pattern & ( 1 << ( (x+i) & 0xf ) )) != 0 ) ? fgColor : bgColor);
							pixel += pixx;
						}
					}
					break;
				case 2:
					for (; pixel<pixellast; pixel += pixy) {
						uint16 pattern = areaPattern ? areaPattern[ y++ & 0xf ] : 0xffff;

						for (i=0; i<dx; i++) {
							if ( ( pattern & ( 1 << ( (x+i) & 0xf ) )) != 0 )
								*(uint8*)pixel = fgColor;
							pixel += pixx;
						}
					}
					break;
				case 3:
					for (; pixel<pixellast; pixel += pixy) {
						uint16 pattern = areaPattern ? areaPattern[ y++ & 0xf ] : 0xffff;

						for (i=0; i<dx; i++) {
							if ( ( pattern & ( 1 << ( (x+i) & 0xf ) )) != 0 )
								*(uint8*)pixel = ~(*(uint8*)pixel);
							pixel += pixx;
						};
					}
					break;
				case 4:
					for (; pixel<pixellast; pixel += pixy) {
						uint16 pattern = areaPattern ? areaPattern[ y++ & 0xf ] : 0xffff;

						for (i=0; i<dx; i++) {
							if ( ( pattern & ( 1 << ( (x+i) & 0xf ) )) == 0 )
								*(uint8*)pixel = fgColor;
							pixel += pixx;
						}
					}
					break;
			}
			break;
		case 2:
			pixy -= (pixx*dx);
			//				D2(bug("bix pix: %d, %x, %d", y, pixel, pixy));

			switch (logOp) {
				case 1:
					for (; pixel<pixellast; pixel += pixy) {
						uint16 pattern = areaPattern ? areaPattern[ y++ & 0xf ] : 0xffff;

						for (i=0; i<dx; i++) {
							*(uint16*)pixel = (( ( pattern & ( 1 << ( (x+i) & 0xf ) )) != 0 ) ? fgColor : bgColor);
							pixel += pixx;
						}
					}
					break;
				case 2:
					for (; pixel<pixellast; pixel += pixy) {
						uint16 pattern = areaPattern ? areaPattern[ y++ & 0xf ] : 0xffff;

						for (i=0; i<dx; i++) {
							if ( ( pattern & ( 1 << ( (x+i) & 0xf ) )) != 0 )
								*(uint16*)pixel = fgColor;
							pixel += pixx;
						}
					}
					break;
				case 3:
					for (; pixel<pixellast; pixel += pixy) {
						uint16 pattern = areaPattern ? areaPattern[ y++ & 0xf ] : 0xffff;

						for (i=0; i<dx; i++) {
							if ( ( pattern & ( 1 << ( (x+i) & 0xf ) )) != 0 )
								*(uint16*)pixel = ~(*(uint16*)pixel);
							pixel += pixx;
						};
					}
					break;
				case 4:
					for (; pixel<pixellast; pixel += pixy) {
						uint16 pattern = areaPattern ? areaPattern[ y++ & 0xf ] : 0xffff;

						for (i=0; i<dx; i++) {
							if ( ( pattern & ( 1 << ( (x+i) & 0xf ) )) == 0 )
								*(uint16*)pixel = fgColor;
							pixel += pixx;
						}
					}
					break;
			}
			break;
		case 3:
			pixy -= (pixx*dx);
			switch (logOp) {
				case 1:
					for (; pixel<pixellast; pixel += pixy) {
						uint16 pattern = areaPattern ? areaPattern[ y++ & 0xf ] : 0xffff;

						for (i=0; i<dx; i++) {
							putBpp24Pixel( pixel, (( ( pattern & ( 1 << ( (x+i) & 0xf ) )) != 0 ) ? fgColor : bgColor) );
							pixel += pixx;
						}
					}
					break;
				case 2:
					for (; pixel<pixellast; pixel += pixy) {
						uint16 pattern = areaPattern ? areaPattern[ y++ & 0xf ] : 0xffff;

						for (i=0; i<dx; i++) {
							if ( ( pattern & ( 1 << ( (x+i) & 0xf ) )) != 0 )
								putBpp24Pixel( pixel, fgColor );
							pixel += pixx;
						}
					}
					break;
				case 3:
					for (; pixel<pixellast; pixel += pixy) {
						uint16 pattern = areaPattern ? areaPattern[ y++ & 0xf ] : 0xffff;

						for (i=0; i<dx; i++) {
							if ( ( pattern & ( 1 << ( (x+i) & 0xf ) )) != 0 )
								putBpp24Pixel( pixel, ~ getBpp24Pixel( pixel ) );
							pixel += pixx;
						};
					}
					break;
				case 4:
					for (; pixel<pixellast; pixel += pixy) {
						uint16 pattern = areaPattern ? areaPattern[ y++ & 0xf ] : 0xffff;

						for (i=0; i<dx; i++) {
							if ( ( pattern & ( 1 << ( (x+i) & 0xf ) )) == 0 )
								putBpp24Pixel( pixel, fgColor );
							pixel += pixx;
						}
					}
					break;
			}
			break;
		default: /* case 4*/
			pixy -= (pixx*dx);
			switch (logOp) {
				case 1:
					for (; pixel<pixellast; pixel += pixy) {
						uint16 pattern = areaPattern ? areaPattern[ y++ & 0xf ] : 0xffff;

						for (i=0; i<dx; i++) {
							*(uint32*)pixel = (( ( pattern & ( 1 << ( (x+i) & 0xf ) )) != 0 ) ? fgColor : bgColor);
							pixel += pixx;
						}
					}
					break;
				case 2:
					for (; pixel<pixellast; pixel += pixy) {
						uint16 pattern = areaPattern ? areaPattern[ y++ & 0xf ] : 0xffff;

						for (i=0; i<dx; i++) {
							if ( ( pattern & ( 1 << ( (x+i) & 0xf ) )) != 0 )
								*(uint32*)pixel = fgColor;
							pixel += pixx;
						}
					}
					break;
				case 3:
					for (; pixel<pixellast; pixel += pixy) {
						uint16 pattern = areaPattern ? areaPattern[ y++ & 0xf ] : 0xffff;

						for (i=0; i<dx; i++) {
							if ( ( pattern & ( 1 << ( (x+i) & 0xf ) )) != 0 )
								*(uint32*)pixel = ~(*(uint32*)pixel);
							pixel += pixx;
						};
					}
					break;
				case 4:
					for (; pixel<pixellast; pixel += pixy) {
						uint16 pattern = areaPattern ? areaPattern[ y++ & 0xf ] : 0xffff;

						for (i=0; i<dx; i++) {
							if ( ( pattern & ( 1 << ( (x+i) & 0xf ) )) == 0 )
								*(uint32*)pixel = fgColor;
							pixel += pixx;
						}
					}
					break;
			}
			break;
	}  // switch
}

void HostScreen::update( int32 x, int32 y, int32 w, int32 h, bool forced )
{
	if ( !forced && !doUpdate ) // the HW surface is available
		return;

	//	SDL_UpdateRect(SDL_GetVideoSurface(), 0, 0, width, height);
	// SDL_UpdateRect(surf, x, y, w, h);
	SDL_UpdateRect(mainSurface, x, y, w, h);
}

void HostScreen::update( bool forced )
{
	update( 0, 0, width, height, forced );
}

void HostScreen::update()
{
	update( 0, 0, width, height, false );
}

void HostScreen::OpenGLUpdate(void)
{
#ifdef ENABLE_OPENGL
	/* Update the texture */
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, SdlGlTextureWidth, SdlGlTextureHeight, GL_RGBA, GL_UNSIGNED_BYTE, SdlGlTexture);

	/* Render the textured quad */
	glBegin(GL_QUADS);
		glTexCoord2f( 0.0, 0.0 );
		glVertex2i( 0, 0);

		glTexCoord2f( (GLfloat)(((GLfloat)width)/((GLfloat)SdlGlTextureWidth)), 0.0 );
		glVertex2i( bx_options.opengl.width, 0);

		glTexCoord2f( (GLfloat)(((GLfloat)width)/((GLfloat)SdlGlTextureWidth)), (GLfloat)(((GLfloat)height)/((GLfloat)SdlGlTextureHeight)) );
		glVertex2i( bx_options.opengl.width, bx_options.opengl.height);

		glTexCoord2f( 0.0, (GLfloat)(((GLfloat)height)/((GLfloat)SdlGlTextureHeight)) );
		glVertex2i( 0, bx_options.opengl.height);
	glEnd();
#endif
}

/*
 * $Log$
 * Revision 1.54  2005/05/10 10:37:44  pmandin
 * Faster OpenGL rendering
 *
 * Revision 1.53  2005/04/22 21:57:16  xavier
 * Remove debug code left by mistake.
 *
 * Revision 1.52  2005/04/22 21:52:36  xavier
 * Switch between fullscreen and windowed mode on Microsoft Windows
 *
 * Revision 1.51  2005/01/22 16:39:48  joy
 * --disable-gui now disables all SDL GUI related code
 *
 * Revision 1.50  2005/01/12 14:35:16  joy
 * more unused variables hidden
 *
 * Revision 1.49  2004/12/11 09:55:52  pmandin
 * Move update function from .h to .cpp, mandatory for NFOSMesa
 *
 * Revision 1.48  2004/10/31 23:15:56  pmandin
 * Do not need to enable autozoom to force screen size
 *
 * Revision 1.47  2004/09/10 17:07:25  pmandin
 * Autozoom: now you can choose a specific host screen size that will remain constant accross Atari screen size changes
 *
 * Revision 1.46  2004/04/26 07:24:50  standa
 * Just comment adjustments.
 *
 * Revision 1.45  2004/02/07 13:20:42  joy
 * explanation for some unclear trick
 *
 * Revision 1.44  2004/01/25 15:13:04  xavier
 * More work on SDL GUI
 *
 * Revision 1.43  2004/01/06 22:36:22  xavier
 * Improved SDL Gui maintainability and "look'n'feel".
 *
 * Revision 1.42  2004/01/05 10:05:20  standa
 * Palette handling reworked. Old non-NF dispatch removed.
 *
 * Revision 1.41  2003/12/28 19:34:04  joy
 * IPC fix - SDL user events
 *
 * Revision 1.40  2003/12/27 23:26:56  joy
 * color palette initialized to default TOS colors
 *
 * Revision 1.39  2003/12/25 22:51:05  joy
 * dirty hack for redrawing SDL GUI if screen size changed
 *
 * Revision 1.38  2003/12/24 23:31:15  joy
 * when GUI is open the background is being updated with running ARAnyM
 *
 * Revision 1.37  2003/06/01 08:35:39  milan
 * MacOS X support updated and <SDL/> removed from includes, path to SDL headers must be fully defined
 *
 * Revision 1.36  2003/04/16 19:35:49  pmandin
 * Correct inclusion of SDL headers
 *
 * Revision 1.35  2003/03/29 08:45:38  milan
 * capabilities output
 * manpage
 * infoprint
 *
 * Revision 1.34  2003/02/09 14:51:04  pmandin
 * Filtered parameter for OpenGL rendering
 *
 * Revision 1.33  2002/12/02 16:54:38  milan
 * non-OpenGL support
 *
 * Revision 1.32  2002/12/01 10:28:29  pmandin
 * OpenGL rendering
 *
 * Revision 1.31  2002/09/23 09:21:37  pmandin
 * Select best video mode
 *
 * Revision 1.30  2002/07/20 12:44:17  joy
 * GUI can survive even video mode change now. Just the dialog is not redrawn yet
 *
 * Revision 1.29  2002/07/20 08:10:55  joy
 * GUI background saving/restoring fixed
 *
 * Revision 1.28  2002/07/19 12:27:40  joy
 * main and background video surfaces. 'surf' is just pointer to either of them depending on whether SDL GUI is active and has stored the background or not.
 *
 * Revision 1.27  2002/06/09 20:08:31  joy
 * save_bkg/restore_bkg added (used in SDL GUI)
 * the save to mem worked for a while and then it started failing for no apparent reason so I had to write saving to file. It's a hack. Proper solution would be to create a complete surface by cloning the current one and blit the screen there, I think.
 *
 * Revision 1.26  2002/06/07 20:56:56  joy
 * added window/fullscreen mode switch
 *
 * Revision 1.25  2002/01/08 21:20:57  standa
 * fVDI driver palette store on res change implemented.
 *
 * Revision 1.24  2001/12/22 18:13:24  joy
 * most video related parameters moved to bx_options.video struct.
 * --refresh <x> added
 *
 * Revision 1.23  2001/12/03 20:56:07  standa
 * The gfsprimitives library files removed. All the staff was moved and
 * adjusted directly into the HostScreen class.
 *
 * Revision 1.22  2001/11/29 23:51:56  standa
 * Johan Klockars <rand@cd.chalmers.se> fVDI driver changes.
 *
 * Revision 1.21  2001/11/21 13:29:51  milan
 * cleanning & portability
 *
 * Revision 1.20  2001/11/19 01:37:35  standa
 * PaletteInversIndex search. Bugfix in fillArea in 8bit depth.
 *
 * Revision 1.19  2001/11/04 23:17:08  standa
 * 8bit destination surface support in VIDEL. Blit routine optimalization.
 * Bugfix in compatibility modes palette copying.
 *
 * Revision 1.18  2001/10/30 22:59:34  standa
 * The resolution change is now possible through the fVDI driver.
 *
 * Revision 1.17  2001/10/29 23:14:17  standa
 * The HostScreen support for arbitrary destination BPP (8,16,24,32bit).
 *
 * Revision 1.16  2001/10/25 19:56:01  standa
 * The Log and Header CVS tags in the Log removed. Was recursing.
 *
 * Revision 1.15  2001/10/24 17:55:01  standa
 * The fVDI driver fixes. Finishing the functionality tuning.
 *
 * Revision 1.14  2001/10/23 21:28:49  standa
 * Several changes, fixes and clean up. Shouldn't crash on high resolutions.
 * hostscreen/gfx... methods have fixed the loop upper boundary. The interface
 * types have changed quite havily.
 *
 * Revision 1.13  2001/10/16 19:06:55  standa
 * The uint32 changed to int16 to make the gfxLineColor work.
 * Now it seems not to segfault anywhere.
 *
 * Revision 1.12  2001/10/08 21:46:05  standa
 * The Header and Log CVS tags added.
 *
 * Revision 1.11  2001/10/03 06:37:41  standa
 * General cleanup. Some constants added. Better "to screen" operation
 * recognition (the videoram address is checked too - instead of only the
 * MFDB == NULL || MFDB->address == NULL)
 *
 * Revision 1.10  2001/09/30 23:09:23  standa
 * The line logical operation added.
 * The first version of blitArea (screen to screen only).
 *
 * Revision 1.9  2001/09/24 23:16:28  standa
 * Another minor changes. some logical operation now works.
 * fvdidrv/fillArea and fvdidrv/expandArea got the first logOp handling.
 *
 * Revision 1.8  2001/09/20 18:12:09  standa
 * Off by one bug fixed in fillArea.
 * Separate functions for transparent and opaque background.
 * gfxPrimitives methods moved to the HostScreen
 *
 * Revision 1.7  2001/09/05 15:06:41  joy
 * SelectVideoMode() commented out.
 *
 * Revision 1.6  2001/09/04 13:51:45  joy
 * debug disabled
 *
 * Revision 1.5  2001/08/30 14:04:59  standa
 * The fVDI driver. mouse_draw implemented. Partial pattern fill support.
 * Still buggy.
 *
 * Revision 1.4  2001/08/28 23:26:09  standa
 * The fVDI driver update.
 * VIDEL got the doRender flag with setter setRendering().
 *       The host_colors_uptodate variable name was changed to hostColorsSync.
 * HostScreen got the doUpdate flag cleared upon initialization if the HWSURFACE
 *       was created.
 * fVDIDriver got first version of drawLine and fillArea (thanks to SDL_gfxPrimitives).
 *
 * Revision 1.3  2001/08/13 22:29:06  milan
 * IDE's params from aranymrc file etc.
 *
 * Revision 1.2  2001/07/24 09:36:51  joy
 * D(bug) macro replaces fprintf
 *
 * Revision 1.1  2001/06/18 13:21:55  standa
 * Several template.cpp like comments were added.
 * HostScreen SDL encapsulation class.
 *
 *
 */

/*
vim:ts=4:sw=4:
*/
