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
#include "logo.h"
#include "hostscreen.h"
#include "host_surface.h"
#include "parameters.h"
#ifdef SDL_GUI
# include "gui-sdl/sdlgui.h"
#endif
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

#define MIN_WIDTH 640
#define MIN_HEIGHT 480

#ifdef ENABLE_VBL_UPDATES
#define MAXIMUM_UPDATE_CACHE 1000
	static SDL_Rect updateRects[MAXIMUM_UPDATE_CACHE];
	static int sdl_rectcount = 0;
#endif

HostScreen::HostScreen(void)
	: DirtyRects(), logo(NULL), logo_present(true), refreshCounter(0)
{
	// the counter init
	snapCounter = 0;

	mainSurface=NULL;

#ifdef ENABLE_OPENGL
	// OpenGL stuff
	SdlGlSurface=NULL;
	SdlGlTexture=NULL;
	dirty_rects=NULL;
	dirty_w=dirty_h=0;
	npot_texture=rect_texture=SDL_FALSE;
	rect_target=GL_TEXTURE_2D;

	if (dyngl_load(bx_options.opengl.library)==0) {
		bx_options.opengl.enabled = false;
	}
#else
	bx_options.opengl.enabled = false;
#endif /* !ENABLE_OPENGL */

	renderVidelSurface = true;
	lastVidelWidth = lastVidelHeight = lastVidelBpp = -1;
	DisableOpenGLVdi();

	setWindowSize(MIN_WIDTH,MIN_HEIGHT,8);
}

HostScreen::~HostScreen(void)
{
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
	if (logo) {
		delete logo;
	}
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

void HostScreen::makeSnapshot()
{
	char filename[15];
	sprintf( filename, "snap%03d.bmp", snapCounter++ );

	SDL_SaveBMP( mainSurface, filename );
}


void HostScreen::toggleFullScreen()
{
	bx_options.video.fullscreen = !bx_options.video.fullscreen;

	setWindowSize( width, height, bpp );
}

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

	if (width<MIN_WIDTH) {
		width=MIN_WIDTH;
	}
	if (height<MIN_HEIGHT) {
		height=MIN_HEIGHT;
	}

	// Select a correct video mode
	searchVideoMode(&width, &height, &bpp);	

	this->width	 = width;
	this->height = height;
	this->bpp = bpp;

	// SelectVideoMode();
	sdl_videoparams = SDL_RESIZABLE /*SDL_HWSURFACE | SDL_HWPALETTE*/;
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
		bx_options.video.fullscreen = ((SdlGlSurface->flags & SDL_FULLSCREEN) == SDL_FULLSCREEN);

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
		if (mainSurface) {
			bx_options.video.fullscreen = ((mainSurface->flags & SDL_FULLSCREEN) == SDL_FULLSCREEN);
		}
	}

	resizeDirty(width, height);
	forceRefreshNfvdi();

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
		setWindowSize(MIN_WIDTH,MIN_HEIGHT,8);
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

#ifdef SDL_GUI
	if (!SDLGui_isClosed()) {
		refreshGui();
	}
#endif

#ifdef ENABLE_VBL_UPDATES
	if (sdl_rectcount > 0) {
		SDL_UpdateRects(mainSurface, sdl_rectcount, updateRects);
		sdl_rectcount = 0;
	}
#endif

	refreshScreen();
}

void HostScreen::setVidelRendering(bool videlRender)
{
	renderVidelSurface = videlRender;
}

void HostScreen::refreshVidel(void)
{
	HostSurface *videl_hsurf = getVIDEL()->getSurface();
	if (!videl_hsurf) {
		return;
	}

	SDL_Surface *videl_surf = videl_hsurf->getSdlSurface();

	/* Display logo if videl not ready */
	bool displayLogo = true;
	if (videl_surf) {
		if ((videl_surf->w > 64) && (videl_surf->h > 64)) {
			displayLogo = false;
		}
	}
	if (displayLogo) {
		refreshLogo();
		return;
	}

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

	drawSurfaceToScreen(videl_hsurf);
}

void HostScreen::refreshLogo(void)
{
	if (!logo_present) {
		return;
	}
	if (!logo) {
		logo = new Logo(bx_options.logo_path);
		if (!logo) {
			return;
		}
	}

	HostSurface *logo_hsurf = logo->getSurface();
	if (!logo_hsurf) {
		logo->load(bx_options.logo_path);
		logo_hsurf = logo->getSurface();
		if (!logo_hsurf) {
			fprintf(stderr, "Can not load logo from %s file\n",
				bx_options.logo_path); 
			logo_present = false;
			return;
		}
	}

	SDL_Surface *logo_surf = logo_hsurf->getSdlSurface();
	if (!logo_surf) {
		return;
	}

	int logo_width = logo_hsurf->getWidth();
	int logo_height = logo_hsurf->getHeight();

	int w = (logo_width < 320) ? 320 : logo_width;
	int h = (logo_height < 200) ? 200 : logo_height;
	int bpp = logo_hsurf->getBpp();
	if ((w!=lastVidelWidth) || (h!=lastVidelHeight) || (bpp!=lastVidelBpp)) {
		setWindowSize(w, h, bpp);
		lastVidelWidth = w;
		lastVidelHeight = h;
		lastVidelBpp = bpp;
	}

	/* Set palette from surface */
	if ((bpp==8) && (mainSurface->format->BitsPerPixel == 8)) {
		SDL_Color palette[256];
		for (int i=0; i<256; i++) {
			palette[i].r = logo_surf->format->palette->colors[i].r;
			palette[i].g = logo_surf->format->palette->colors[i].g;
			palette[i].b = logo_surf->format->palette->colors[i].b;
		}
		SDL_SetPalette(mainSurface, SDL_LOGPAL|SDL_PHYSPAL, palette, 0,256);
	}

	drawSurfaceToScreen(logo_hsurf);
}

void HostScreen::forceRefreshNfvdi(void)
{
#ifdef NFVDI_SUPPORT
	/* Force nfvdi surface refresh */
	NF_Base* fvdi = NFGetDriver("fVDI");
	if (!fvdi) {
		return;
	}

	HostSurface *nfvdi_hsurf = ((VdiDriver *) fvdi)->getSurface();
	if (!nfvdi_hsurf) {
		return;
	}

	nfvdi_hsurf->setDirtyRect(0,0,
		nfvdi_hsurf->getWidth(), nfvdi_hsurf->getHeight());
#endif
}

void HostScreen::refreshNfvdi(void)
{
#ifdef NFVDI_SUPPORT
	NF_Base* fvdi = NFGetDriver("fVDI");
	if (!fvdi) {
		return;
	}

	HostSurface *nfvdi_hsurf = ((VdiDriver *) fvdi)->getSurface();
	if (!nfvdi_hsurf) {
		return;
	}
	SDL_Surface *nfvdi_surf = nfvdi_hsurf->getSdlSurface();
	if (!nfvdi_surf) {
		return;
	}

	int vdi_width = nfvdi_hsurf->getWidth();
	int vdi_height = nfvdi_hsurf->getHeight();

	int w = (vdi_width < 320) ? 320 : vdi_width;
	int h = (vdi_height < 200) ? 200 : vdi_height;
	int bpp = nfvdi_hsurf->getBpp();
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

	drawSurfaceToScreen(nfvdi_hsurf);
#endif
}

void HostScreen::refreshGui(void)
{
#ifdef SDL_GUI
	int gui_x, gui_y;

	drawSurfaceToScreen(SDLGui_getSurface(), &gui_x, &gui_y);

	SDLGui_setGuiPos(gui_x, gui_y);
#endif /* SDL_GUI */
}

void HostScreen::drawSurfaceToScreen(HostSurface *hsurf, int *dst_x, int *dst_y)
{
	if (!hsurf) {
		return;
	}
	SDL_Surface *sdl_surf = hsurf->getSdlSurface();
	if (!sdl_surf) {
		return;
	}

	int width = hsurf->getWidth();
	int height = hsurf->getHeight();

	SDL_Rect src_rect = {0,0, width, height};
	SDL_Rect dst_rect = {0,0, mainSurface->w, mainSurface->h};
	if (mainSurface->w > width) {
		dst_rect.x = (mainSurface->w - width) >> 1;
		dst_rect.w = width;
	} else {
		src_rect.w = mainSurface->w;
	}
	if (mainSurface->h > height) {
		dst_rect.y = (mainSurface->h - height) >> 1;
		dst_rect.h = height;
	} else {
		src_rect.h = mainSurface->h;
	}

	Uint8 *dirtyRects = hsurf->getDirtyRects();
	if (!dirtyRects) {
		SDL_BlitSurface(sdl_surf, &src_rect, mainSurface, &dst_rect);

		setDirtyRect(dst_rect.x,dst_rect.y,dst_rect.w,dst_rect.h);
	} else {
		int dirty_w = hsurf->getDirtyWidth();
		int dirty_h = hsurf->getDirtyHeight();
		for (int y=0; y<dirty_h; y++) {
			for (int x=0; x<dirty_w; x++) {
				if (dirtyRects[y * dirty_w + x]) {
					SDL_Rect src, dst;

					src.x = src_rect.x + (x<<4);
					src.y = src_rect.y + (y<<4);
					src.w = (1<<4);
					src.h = (1<<4);

					dst.x = dst_rect.x + (x<<4);
					dst.y = dst_rect.y + (y<<4);
					dst.w = (1<<4);
					dst.h = (1<<4);

					SDL_BlitSurface(sdl_surf, &src, mainSurface, &dst);

					setDirtyRect(dst.x,dst.y,dst.w,dst.h);
				}
			}
		}

		hsurf->clearDirtyRects();
	}

	/* GUI need to know where it is */
	if (dst_x) {
		*dst_x = dst_rect.x;
	}
	if (dst_y) {
		*dst_y = dst_rect.y;
	}
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

void HostScreen::forceRefreshScreen(void)
{
	forceRefreshNfvdi();
	if (mainSurface) {
		setDirtyRect(0,0, mainSurface->w, mainSurface->h);
	}
}

HostSurface *HostScreen::createSurface(int width, int height, int bpp)
{
	return new HostSurface(width, height, bpp);
}

HostSurface *HostScreen::createSurface(SDL_Surface *sdl_surf)
{
	return new HostSurface(sdl_surf);
}

/*
vim:ts=4:sw=4:
*/
