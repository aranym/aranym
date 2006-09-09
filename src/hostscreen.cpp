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
#include "main.h"

#define DEBUG 0
#include "debug.h"

#ifdef ENABLE_OPENGL
#include <SDL_opengl.h>
#endif

#ifdef OS_cygwin
# define WIN32 1
# include <SDL_syswm.h>
# include <windows.h>
# undef WIN32
#else
# include <SDL_syswm.h>
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
	dirty_rects=NULL;
	dirty_w=dirty_h=0;
	npot_texture=rect_texture=SDL_FALSE;
	rect_target=GL_TEXTURE_2D;
#endif /* ENABLE_OPENGL */

	DisableOpenGLVdi();
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
		if (dirty_rects) {
			delete dirty_rects;
			dirty_rects=NULL;
		}

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
		D2(bug("toggleFullScreen: SDL_WM_ToggleFullScreen() not supported -> using SDL_SetVideoMode()"));

		// SDL_WM_ToggleFullScreen() did not work.
		// We have to change video mode "by hand".
		SDL_Surface *temp = SDL_ConvertSurface(mainSurface, mainSurface->format,
		                                       mainSurface->flags);
		if (temp == NULL)
			bug("toggleFullScreen: Unable to save screen content.");

#if 1
		setWindowSize( width, height, bpp );
#else
		mainSurface = SDL_SetVideoMode(width, height, bpp, sdl_videoparams);
		if (mainSurface == NULL)
			bug("toggleFullScreen: Unable to set new video mode.");
		if (mainSurface->format->BitsPerPixel <= 8)
			SDL_SetColors(mainSurface, temp->format->palette->colors, 0,
			              temp->format->palette->ncolors);
#endif

		if (SDL_BlitSurface(temp, NULL, mainSurface, NULL) != 0)
			bug("toggleFullScreen: Unable to restore screen content.");
		SDL_FreeSurface(temp);
#ifdef SDL_GUI
		if (isGUIopen() == 0)
			surf = mainSurface;
#endif /* SDL_GUI */

		/* refresh the screen */
		update( true);
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
	modeflags = 0 /*SDL_HWSURFACE | SDL_HWPALETTE*/;
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
	sdl_videoparams = 0 /*SDL_HWSURFACE | SDL_HWPALETTE*/;
	if (bx_options.video.fullscreen)
		sdl_videoparams |= SDL_FULLSCREEN;

#ifdef ENABLE_OPENGL
	if (bx_options.opengl.enabled) {
		int filtering, i, gl_bpp[4]={0,16,24,32};

		sdl_videoparams |= SDL_OPENGL;

		/* Setup at least 15 bits true colour OpenGL context */
		SDL_GL_SetAttribute(SDL_GL_RED_SIZE,5);
		SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE,5);
		SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,5);
		SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE,15);

		for (i=0;i<4;i++) {
			SdlGlSurface = SDL_SetVideoMode(width, height, gl_bpp[i], sdl_videoparams);
			if (SdlGlSurface) {
				break;
			}
		}
		if (!SdlGlSurface) {
			fprintf(stderr,"Can not setup %dx%d OpenGL video mode\n",width,height);
			QuitEmulator();
		}
		this->width = width = SdlGlSurface->w;
		this->height = height = SdlGlSurface->h;
		this->bpp = bpp = 32;	/* bpp of texture that will be used */

		glViewport(0, 0, width, height);

		/* Projection matrix */
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0.0, width, height, 0.0, -1.0, 1.0);

		/* Texture matrix */
		glMatrixMode(GL_TEXTURE);
		glLoadIdentity();

		/* Model view matrix */
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glTranslatef(0.375, 0.375, 0.0);

		/* Setup texturing mode */
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

		/* Delete previous stuff */
		if (mainSurface) {
			SDL_FreeSurface(mainSurface);
			mainSurface=NULL;
		}

		if (SdlGlTexture) {
			glDeleteTextures(1, &SdlGlTexObj);
		}

		/* Full screen OpenGL rendering ? */
		if (!OpenGLVdi) {
			GLint MaxTextureSize;
			char *extensions;

			rect_target=GL_TEXTURE_2D;
			npot_texture = rect_texture = SDL_FALSE;
			glGetIntegerv(GL_MAX_TEXTURE_SIZE, &MaxTextureSize);

			extensions = (char *)glGetString(GL_EXTENSIONS);

			/* Check texture rectangle extensions */

#if defined(GL_NV_texture_rectangle)
			if (strstr(extensions, "GL_NV_texture_rectangle")) {
				rect_texture = SDL_TRUE;
				rect_target=GL_TEXTURE_RECTANGLE_NV;
				glGetIntegerv(GL_MAX_RECTANGLE_TEXTURE_SIZE_NV, &MaxTextureSize);
			}
#endif
#if defined(GL_EXT_texture_rectangle)
			if (strstr(extensions, "GL_EXT_texture_rectangle")) {
				rect_texture = SDL_TRUE;
				rect_target=GL_TEXTURE_RECTANGLE_EXT;
				glGetIntegerv(GL_MAX_RECTANGLE_TEXTURE_SIZE_EXT, &MaxTextureSize);
			}
#endif
#if defined(GL_ARB_texture_rectangle)
			if (strstr(extensions, "GL_ARB_texture_rectangle")) {
				rect_texture = SDL_TRUE;
				rect_target=GL_TEXTURE_RECTANGLE_ARB;
				glGetIntegerv(GL_MAX_RECTANGLE_TEXTURE_SIZE_ARB, &MaxTextureSize);
			}
#endif

			/* Check non power of two texture extension */
			npot_texture = rect_texture;
			if (strstr(extensions, "GL_ARB_texture_non_power_of_two")) {
				npot_texture=SDL_TRUE;
				rect_texture=SDL_FALSE;
				rect_target=GL_TEXTURE_2D;
				glGetIntegerv(GL_MAX_TEXTURE_SIZE, &MaxTextureSize);
			}

			SdlGlTextureWidth = SdlGlTextureHeight = MaxTextureSize;
			D(bug("gl: need at least a %dx%d texture",width,height));
			D(bug("gl: texture is at most a %dx%d texture",MaxTextureSize,MaxTextureSize));

			if (!npot_texture) {
				/* Calculate the smallest needed texture */
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
			} else {
				if (width<SdlGlTextureWidth) {
					if (width>64) {
						SdlGlTextureWidth=width;
					} else {
						SdlGlTextureWidth=64;
					}
				}
				if (height<SdlGlTextureHeight) {
					if (height>64) {
						SdlGlTextureHeight=height;
					} else {
						SdlGlTextureHeight=64;
					}
				}
			}
			D(bug("gl: texture will be %dx%d texture",SdlGlTextureWidth,SdlGlTextureHeight));
		}

		mainSurface = SDL_CreateRGBSurface(SDL_SWSURFACE,
			SdlGlTextureWidth,SdlGlTextureHeight,32,
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
			255<<16,255<<8,255,255<<24	/* GL_BGRA little endian */
#else
			255<<8,255<<16,255<<24,255	/* GL_BGRA big endian */
#endif
		);
		if (!mainSurface) {
			fprintf(stderr,"Can not create %dx%dx%d texture\n",SdlGlTextureWidth,SdlGlTextureHeight,bpp);
			QuitEmulator();
		}
		SdlGlTexture = (uint8 *) (mainSurface->pixels);

		if (!OpenGLVdi) {
			glGenTextures(1, &SdlGlTexObj);
			glBindTexture(GL_TEXTURE_2D, SdlGlTexObj);

			filtering = GL_NEAREST;		
			if (bx_options.opengl.filtered) {
				filtering = GL_LINEAR;
			}
			glTexParameteri(rect_target, GL_TEXTURE_MAG_FILTER, filtering); // scale when image bigger than texture
			glTexParameteri(rect_target, GL_TEXTURE_MIN_FILTER, filtering); // scale when image smaller than texture

			glTexImage2D(rect_target, 0, GL_RGBA, SdlGlTextureWidth, SdlGlTextureHeight, 0, GL_BGRA, GL_UNSIGNED_BYTE, SdlGlTexture);

			D(bug("gl: texture created"));

			/* Activate autozoom if texture smaller than screen */
			SdlGlWidth = width;
			SdlGlHeight = height;
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

			/* Create dirty rectangles list */
			if (dirty_rects)
				delete dirty_rects;
		
			dirty_w=((width|15)+1)>>4;
			dirty_h=((height|15)+1)>>4;
			dirty_rects=new SDL_bool[dirty_w*dirty_h];
			memset(dirty_rects,SDL_FALSE,sizeof(SDL_bool)*dirty_w*dirty_h);
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

	D2(bug("HLn %3d,%3d,%3d", x1, x2, y));

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

	D2(bug("VLn %3d,%3d,%3d", x, y1, y2));

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

	D2(bug("CLn %3d,%3d,%3d,%3d", x1, x2, y1, y2));

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

#ifdef ENABLE_OPENGL
	if (!OpenGLVdi && bx_options.opengl.enabled && dirty_rects) {
		/* Mark dirty rectangles */
		int x1,y1,x2,y2, i,j;
		
		x1=x>>4;
		y1=y>>4;
		x2=x+w;
		if (x2&15) x2=(x2|15)+1;
		x2>>=4;
		y2=y+h;
		if (y2&15) y2=(y2|15)+1;
		y2>>=4;

		for (j=y1;j<y2;j++) {
			for (i=x1;i<x2;i++) {
				dirty_rects[i+(j*dirty_w)]=SDL_TRUE;
			}
		}
	}
#endif
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
	GLfloat tex_width, tex_height;

	if (OpenGLVdi) {
		return;
	}

	glEnable(rect_target);
	glBindTexture(rect_target, SdlGlTexObj);

	/* Update the texture */
	{
		int x,y;

		for (y=0;y<dirty_h;y++) {
			SDL_bool update_line=SDL_FALSE;
			for (x=0;x<dirty_w;x++) {
				if (dirty_rects[y*dirty_w+x]==SDL_TRUE) {
					update_line=SDL_TRUE;
					break;
				}
			}
			if (update_line) {
				glTexSubImage2D(rect_target, 0,
					 0, y<<4,
					 SdlGlTextureWidth, 16,
					 GL_BGRA, GL_UNSIGNED_BYTE,
					 SdlGlTexture + (y<<4)*SdlGlTextureWidth*4
				);
			}
		}
	}
	memset(dirty_rects,SDL_FALSE,sizeof(SDL_bool)*dirty_w*dirty_h);

	/* Render the textured quad */
	tex_width = ((GLfloat)width)/((GLfloat)SdlGlTextureWidth);
	tex_height = ((GLfloat)height)/((GLfloat)SdlGlTextureHeight);
	if (rect_target!=GL_TEXTURE_2D) {
		tex_width = (GLfloat)width;
		tex_height = (GLfloat)height;
	}

	glBegin(GL_QUADS);
		glTexCoord2f( 0.0, 0.0 );
		glVertex2i( 0, 0);

		glTexCoord2f( tex_width, 0.0 );
		glVertex2i( SdlGlWidth, 0);

		glTexCoord2f( tex_width, tex_height);
		glVertex2i( SdlGlWidth, SdlGlHeight);

		glTexCoord2f( 0.0, tex_height);
		glVertex2i( 0, SdlGlHeight);
	glEnd();

	glDisable(rect_target);
#endif
}

uint32 HostScreen::getBitsPerPixel(void)
{
	return surf->format->BitsPerPixel;
}

void HostScreen::EnableOpenGLVdi(void)
{
	OpenGLVdi = SDL_TRUE;
}

void HostScreen::DisableOpenGLVdi(void)
{
	OpenGLVdi = SDL_FALSE;
}

/*
vim:ts=4:sw=4:
*/
