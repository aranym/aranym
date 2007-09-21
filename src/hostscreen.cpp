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

#ifdef NFVDI_SUPPORT
# include "nf_objs.h"
# include "nfvdi.h"
#endif

#define DEBUG 0
#include "debug.h"

#ifdef ENABLE_OPENGL
#include <SDL_opengl.h>
#include "dyngl.h"
#endif

#ifdef OS_cygwin
# define WIN32 1
# include <SDL_syswm.h>
# include <windows.h>
# undef WIN32
#else
# include <SDL_syswm.h>
#endif

#ifdef SDL_GUI
#include "sdlgui.h"
extern char *displayKeysym(SDL_keysym keysym, char *buffer);
#endif

#ifdef ENABLE_VBL_UPDATES
#define MAXIMUM_UPDATE_CACHE 1000
	static SDL_Rect updateRects[MAXIMUM_UPDATE_CACHE];
	static int sdl_rectcount = 0;
	static SDL_mutex  *updateLock;
#endif

HostScreen::HostScreen(void)
	: DirtyRects(), refreshCounter(0)
{
	// the counter init
	snapCounter = 0;

	screenLock = SDL_CreateMutex();
#ifdef ENABLE_VBL_UPDATES
	updateLock = SDL_CreateMutex();
#endif

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

	renderVidelSurface = true;
	lastVidelWidth = lastVidelHeight = lastVidelBpp = -1;
	DisableOpenGLVdi();
}

HostScreen::~HostScreen(void) {
	SDL_DestroyMutex(screenLock);
#ifdef ENABLE_VBL_UPDATES
	SDL_DestroyMutex(updateLock);
#endif

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

		/* TODO: gl pointer not valid anymore at this time */
// 		if (SdlGlTexture) {
// 			gl.DeleteTextures(1, &SdlGlTexObj);
// 			SdlGlTexture=NULL;
// 		}
	}
#endif
}

uint32 HostScreen::getBpp(void)
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
	return mainSurface->format->BytesPerPixel;
}

uint32 HostScreen::getBitsPerPixel(void)
{
	return (mainSurface ? mainSurface->format->BitsPerPixel : 0);
}

uint32 HostScreen::getWidth(void)
{
	return width;
}

uint32 HostScreen::getHeight(void)
{
	return height;
}

void HostScreen::lock(void) {
	while (SDL_mutexP(screenLock)==-1) {
		SDL_Delay(20);
		fprintf(stderr, "Couldn't lock mutex\n");
	}
}

void HostScreen::unlock(void) {
	while (SDL_mutexV(screenLock)==-1) {
		SDL_Delay(20);
		fprintf(stderr, "Couldn't unlock mutex\n");
	}
}


void HostScreen::makeSnapshot()
{
	char filename[15];
	sprintf( filename, "snap%03d.bmp", snapCounter++ );

	SDL_SaveBMP( mainSurface, filename );
}


void HostScreen::toggleFullScreen()
{
	bx_options.video.fullscreen = !bx_options.video.fullscreen;
	sdl_videoparams ^= SDL_FULLSCREEN;

	setWindowSize( width, height, bpp );

	forceRefreshNfvdi();
}

#ifdef SDL_GUI
void HostScreen::openGUI()
{
	GUIopened = true;
}

void HostScreen::closeGUI()
{
	forceRefreshNfvdi();
	GUIopened = false;
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

		gl.Viewport(0, 0, width, height);

		/* Projection matrix */
		gl.MatrixMode(GL_PROJECTION);
		gl.LoadIdentity();
		gl.Ortho(0.0, width, height, 0.0, -1.0, 1.0);

		/* Texture matrix */
		gl.MatrixMode(GL_TEXTURE);
		gl.LoadIdentity();

		/* Model view matrix */
		gl.MatrixMode(GL_MODELVIEW);
		gl.LoadIdentity();
		gl.Translatef(0.375, 0.375, 0.0);

		/* Setup texturing mode */
		gl.TexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

		/* Delete previous stuff */
		if (mainSurface) {
			SDL_FreeSurface(mainSurface);
			mainSurface=NULL;
		}

		if (SdlGlTexture) {
			gl.DeleteTextures(1, &SdlGlTexObj);
		}

		/* Full screen OpenGL rendering ? */
		if (!OpenGLVdi) {
			GLint MaxTextureSize;
			char *extensions;

			rect_target=GL_TEXTURE_2D;
			npot_texture = rect_texture = SDL_FALSE;
			gl.GetIntegerv(GL_MAX_TEXTURE_SIZE, &MaxTextureSize);

			extensions = (char *)gl.GetString(GL_EXTENSIONS);

			/* Check texture rectangle extensions */

#if defined(GL_NV_texture_rectangle)
			if (strstr(extensions, "GL_NV_texture_rectangle")) {
				rect_texture = SDL_TRUE;
				rect_target=GL_TEXTURE_RECTANGLE_NV;
				gl.GetIntegerv(GL_MAX_RECTANGLE_TEXTURE_SIZE_NV, &MaxTextureSize);
			}
#endif
#if defined(GL_EXT_texture_rectangle)
			if (strstr(extensions, "GL_EXT_texture_rectangle")) {
				rect_texture = SDL_TRUE;
				rect_target=GL_TEXTURE_RECTANGLE_EXT;
				gl.GetIntegerv(GL_MAX_RECTANGLE_TEXTURE_SIZE_EXT, &MaxTextureSize);
			}
#endif
#if defined(GL_ARB_texture_rectangle)
			if (strstr(extensions, "GL_ARB_texture_rectangle")) {
				rect_texture = SDL_TRUE;
				rect_target=GL_TEXTURE_RECTANGLE_ARB;
				gl.GetIntegerv(GL_MAX_RECTANGLE_TEXTURE_SIZE_ARB, &MaxTextureSize);
			}
#endif

			/* Check non power of two texture extension */
			npot_texture = rect_texture;
			if (strstr(extensions, "GL_ARB_texture_non_power_of_two")) {
				npot_texture=SDL_TRUE;
				rect_texture=SDL_FALSE;
				rect_target=GL_TEXTURE_2D;
				gl.GetIntegerv(GL_MAX_TEXTURE_SIZE, &MaxTextureSize);
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
			gl.GenTextures(1, &SdlGlTexObj);
			gl.BindTexture(GL_TEXTURE_2D, SdlGlTexObj);

			filtering = GL_NEAREST;		
			if (bx_options.opengl.filtered) {
				filtering = GL_LINEAR;
			}
			gl.TexParameteri(rect_target, GL_TEXTURE_MAG_FILTER, filtering); // scale when image bigger than texture
			gl.TexParameteri(rect_target, GL_TEXTURE_MIN_FILTER, filtering); // scale when image smaller than texture

			gl.TexImage2D(rect_target, 0, GL_RGBA, SdlGlTextureWidth, SdlGlTextureHeight, 0, GL_BGRA, GL_UNSIGNED_BYTE, SdlGlTexture);

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
	if (GUIopened) {
		// force SDL GUI to redraw the dialog because resolution has changed
		SDL_Event event;
		event.type = SDL_USEREVENT;
		event.user.code = SDL_USEREVENT; // misused this code for signalizing the resolution change. Did that because I knew the code was unique (needed something distinguishable from keyboard and mouse codes that are sent by the same event name from the input checking thread)
		SDL_PeepEvents(&event, 1, SDL_ADDEVENT, SDL_EVENTMASK(SDL_USEREVENT));
	}
#endif /* SDL_GUI */

	resizeDirty(width, height);

	char buf[sizeof(VERSION_STRING)+128];
#ifdef SDL_GUI
	char key[80];
	displayKeysym(bx_options.hotkeys.setup, key);
	snprintf(buf, sizeof(buf), "%s (press %s key for SETUP)", VERSION_STRING, key);
#else
	snprintf(buf, sizeof(buf), "%s", VERSION_STRING);
#endif /* SDL_GUI */
	SDL_WM_SetCaption(buf, "ARAnyM");

	D(bug("Surface Pitch = %d, width = %d, height = %d", surf->pitch, surf->w, surf->h));
	D(bug("Must Lock? %s", SDL_MUSTLOCK(surf) ? "YES" : "NO"));

	VideoRAMBaseHost = (uint8 *) mainSurface->pixels;
	InitVMEMBaseDiff(VideoRAMBaseHost, VideoRAMBase);
	D(bug("VideoRAM starts at %p (%08x)", VideoRAMBaseHost, VideoRAMBase));
	D(bug("surf->pixels = %x, getVideoSurface() = %x",
			VideoRAMBaseHost, SDL_GetVideoSurface()->pixels));

	D(bug("Pixel format:bitspp=%d, tmasks r=%04x g=%04x b=%04x"
			", tshifts r=%d g=%d b=%d"
			", tlosses r=%d g=%d b=%d",
			mainSurface->format->BitsPerPixel,
			mainSurface->format->Rmask, mainSurface->format->Gmask, mainSurface->format->Bmask,
			mainSurface->format->Rshift, mainSurface->format->Gshift, mainSurface->format->Bshift,
			mainSurface->format->Rloss, mainSurface->format->Gloss, mainSurface->format->Bloss));
}

void HostScreen::OpenGLUpdate(void)
{
#ifdef ENABLE_OPENGL
	GLfloat tex_width, tex_height;

	if (OpenGLVdi) {
		return;
	}

	gl.Enable(rect_target);
	gl.BindTexture(rect_target, SdlGlTexObj);

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
				gl.TexSubImage2D(rect_target, 0,
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

	gl.Begin(GL_QUADS);
		gl.TexCoord2f( 0.0, 0.0 );
		gl.Vertex2i( 0, 0);

		gl.TexCoord2f( tex_width, 0.0 );
		gl.Vertex2i( SdlGlWidth, 0);

		gl.TexCoord2f( tex_width, tex_height);
		gl.Vertex2i( SdlGlWidth, SdlGlHeight);

		gl.TexCoord2f( 0.0, tex_height);
		gl.Vertex2i( 0, SdlGlHeight);
	gl.End();

	gl.Disable(rect_target);
#endif
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
 * this is called in VBL, i.e. 50 times per second
 */
void HostScreen::refresh(void)
{
	if (++refreshCounter < bx_options.video.refresh) {
		return;
	}

	refreshCounter = 0;

	/* Create dummy main surface? */
	if (!mainSurface) {
		setWindowSize(320,200,8);
	}

	if (!mainSurface) {
		return;
	}
	
	/* Render videl surface ? */
	if (renderVidelSurface) {
		refreshVidel();
	} else {
		refreshNfvdi();
	}

	if (GUIopened) {
		refreshGui();
	}

#ifdef ENABLE_VBL_UPDATES
	SDL_mutexP(updateLock);
	if (sdl_rectcount > 0) {
		SDL_UpdateRects(mainSurface, sdl_rectcount, updateRects);
		sdl_rectcount = 0;
	}
	SDL_mutexV(updateLock);
#endif

	refreshScreen();
}

void HostScreen::setVidelRendering(bool videlRender)
{
	renderVidelSurface = videlRender;
}

void HostScreen::refreshVidel(void)
{
	SDL_Surface *videl_surf = getVIDEL()->getSurface();
	if (!videl_surf) {
		return;
	}

	int w = (videl_surf->w < 320) ? 320 : videl_surf->w;
	int h = (videl_surf->h < 200) ? 200 : videl_surf->h;
	int bpp = videl_surf->format->BitsPerPixel;
	if ((w!=lastVidelWidth) || (h!=lastVidelHeight) || (bpp!=lastVidelBpp)) {
		setWindowSize(w, h, bpp);
		lastVidelWidth = w;
		lastVidelHeight = h;
		lastVidelBpp = bpp;
	}

	/* Set palette from videl surface if needed */
	if ((bpp==8) && (mainSurface->format->BitsPerPixel == 8)) {
		SDL_Color palette[256];
		for (int i=0; i<256; i++) {
			palette[i].r = videl_surf->format->palette->colors[i].r;
			palette[i].g = videl_surf->format->palette->colors[i].g;
			palette[i].b = videl_surf->format->palette->colors[i].b;
		}
		SDL_SetPalette(mainSurface, SDL_LOGPAL|SDL_PHYSPAL, palette, 0,256);
	}

	SDL_Rect dst_rect;
	dst_rect.x = (mainSurface->w - videl_surf->w) >> 1;
	dst_rect.y = (mainSurface->h - videl_surf->h) >> 1;
	dst_rect.w = videl_surf->w;
	dst_rect.h = videl_surf->h;

	SDL_BlitSurface(videl_surf, NULL, mainSurface, &dst_rect);

	setDirtyRect(dst_rect.x,dst_rect.y,dst_rect.w,dst_rect.h);
}

void HostScreen::forceRefreshNfvdi(void)
{
#ifdef NFVDI_SUPPORT
	/* Force nfvdi surface refresh */
	NF_Base* fvdi = NFGetDriver("fVDI");
	if (fvdi) {
		SDL_Surface *nfvdi_surf = ((VdiDriver *) fvdi)->getSurface();
		if (nfvdi_surf) {
			((VdiDriver *) fvdi)->setDirtyRect(
				0,0, nfvdi_surf->w, nfvdi_surf->h);
		}
	}
#endif
}

void HostScreen::refreshNfvdi(void)
{
#ifdef NFVDI_SUPPORT
	NF_Base* fvdi = NFGetDriver("fVDI");
	if (!fvdi) {
		return;
	}

	SDL_Surface *nfvdi_surf = ((VdiDriver *) fvdi)->getSurface();
	if (!nfvdi_surf) {
		return;
	}

	int w = (nfvdi_surf->w < 320) ? 320 : nfvdi_surf->w;
	int h = (nfvdi_surf->h < 200) ? 200 : nfvdi_surf->h;
	int bpp = nfvdi_surf->format->BitsPerPixel;
	if ((w!=lastVidelWidth) || (h!=lastVidelHeight) || (bpp!=lastVidelBpp)) {
		setWindowSize(w, h, bpp);
		lastVidelWidth = w;
		lastVidelHeight = h;
		lastVidelBpp = bpp;
	}

	/* Set palette from videl surface if needed */
	if ((bpp==8) && (mainSurface->format->BitsPerPixel == 8)) {
		SDL_Color palette[256];
		for (int i=0; i<256; i++) {
			palette[i].r = nfvdi_surf->format->palette->colors[i].r;
			palette[i].g = nfvdi_surf->format->palette->colors[i].g;
			palette[i].b = nfvdi_surf->format->palette->colors[i].b;
		}
		SDL_SetPalette(mainSurface, SDL_LOGPAL|SDL_PHYSPAL, palette, 0,256);
	}

	Uint8 *dirtyRects = ((VdiDriver *) fvdi)->getDirtyRects();
	if (!dirtyRects) {
		/* No dirty rects, refresh whole surface */
		SDL_Rect dst_rect;
		dst_rect.x = (mainSurface->w - nfvdi_surf->w) >> 1;
		dst_rect.y = (mainSurface->h - nfvdi_surf->h) >> 1;
		dst_rect.w = nfvdi_surf->w;
		dst_rect.h = nfvdi_surf->h;

		SDL_BlitSurface(nfvdi_surf, NULL, mainSurface, &dst_rect);

		setDirtyRect(dst_rect.x,dst_rect.y,dst_rect.w,dst_rect.h);
		return;
	}

	int dirty_w = ((VdiDriver *) fvdi)->getDirtyWidth();
	int dirty_h = ((VdiDriver *) fvdi)->getDirtyHeight();
	for (int y=0; y<dirty_h; y++) {
		for (int x=0; x<dirty_w; x++) {
			if (dirtyRects[y * dirty_w + x]) {
				SDL_Rect src, dst;

				src.x = dst.x = x<<4;
				src.y = dst.y = y<<4;
				src.w = dst.w = 1<<4;
				src.h = dst.h = 1<<4;

				SDL_BlitSurface(nfvdi_surf, &src, mainSurface, &dst);

				setDirtyRect(dst.x,dst.y,dst.w,dst.h);
			}
		}
	}

	((VdiDriver *) fvdi)->clearDirtyRects();
#endif
}

void HostScreen::refreshGui(void)
{
#ifdef SDL_GUI
	SDL_Surface *gui_surf = SDLGui_getSurface();
	if (!gui_surf) {
		return;
	}

	int gui_x, gui_y;

	/* Blit gui on screen */
	SDL_Rect dst_rect;
	dst_rect.x = gui_x = (mainSurface->w - gui_surf->w) >> 1;
	dst_rect.y = gui_y = (mainSurface->h - gui_surf->h) >> 1;
	dst_rect.w = gui_surf->w /* < dest->w ? gui_surf->w : dest->w*/;
	dst_rect.h = gui_surf->h /*< dest->h ? gui_surf->h : dest->h*/;

	SDL_BlitSurface(gui_surf, NULL, mainSurface, &dst_rect);

	setDirtyRect(dst_rect.x,dst_rect.y,dst_rect.w,dst_rect.h);

	SDLGui_setGuiPos(gui_x, gui_y);
#endif /* SDL_GUI */
}

void HostScreen::refreshScreen(void)
{
#ifdef ENABLE_OPENGL
	if (bx_options.opengl.enabled) {
		OpenGLUpdate();

		SDL_GL_SwapBuffers();
		return;
	}
#endif	/* ENABLE_OPENGL */

	if ((mainSurface->flags & SDL_DOUBLEBUF)==SDL_DOUBLEBUF) {
		SDL_Flip(mainSurface);
		return;
	}

	if (!dirtyMarker) {
		return;
	}

	/* Only update dirtied rects */
	SDL_Rect update_rects[dirtyW*dirtyH];
	int i = 0;
	for (int y=0; y<dirtyH; y++) {
		for (int x=0; x<dirtyW; x++) {
			if (dirtyMarker[y * dirtyW + x]) {
				int maxw = 1<<4, maxh = 1<<4;
				if (mainSurface->w - (x<<4) < (1<<4)) {
					maxw = mainSurface->w - (x<<4);
				}
				if (mainSurface->h - (y<<4) < (1<<4)) {
					maxh = mainSurface->h - (y<<4);
				}

				update_rects[i].x = x<<4;
				update_rects[i].y = y<<4;
				update_rects[i].w = maxw;
				update_rects[i].h = maxh;

				i++;
			}
		}
	}

	SDL_UpdateRects(mainSurface,i,update_rects);

	clearDirtyRects();
}


/*
vim:ts=4:sw=4:
*/
