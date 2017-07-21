/*
	Rendering surface

	(C) 2007 ARAnyM developer team

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "sysdeps.h"
#include "SDL_compat.h"

#include "dirty_rects.h"
#include "host_surface.h"

#define DEBUG 0
#include "debug.h"

/*--- Defines ---*/

/*--- Constructor/destructor ---*/

HostSurface::HostSurface(int width, int height, int bpp)
	: DirtyRects(width, height)
{
	surface = SDL_CreateRGBSurface(SDL_SWSURFACE, width, height, bpp,
		0,0,0,0);
	clip_w = surface ? surface->w : 0;
	clip_h = surface ? surface->h : 0;

	setParam(SURF_DRAW, DRAW_CROP_AND_CENTER);
	setParam(SURF_USE_ALPHA, 0);
	setParam(SURF_ALPHA, 100);
}

HostSurface::HostSurface(int width, int height, SDL_PixelFormat *pixelFormat)
	: DirtyRects(width, height)
{
	surface = SDL_CreateRGBSurface(SDL_SWSURFACE, width, height,
		pixelFormat->BitsPerPixel,
		pixelFormat->Rmask, pixelFormat->Gmask,
		pixelFormat->Bmask, pixelFormat->Amask
	);
	clip_w = surface ? surface->w : 0;
	clip_h = surface ? surface->h : 0;

	setParam(SURF_DRAW, DRAW_CROP_AND_CENTER);
	setParam(SURF_USE_ALPHA, 0);
	setParam(SURF_ALPHA, 100);
}

HostSurface::~HostSurface(void)
{
	if (surface) {
		SDL_FreeSurface(surface);
		surface = NULL;
	}
}

/*--- Protected functions ---*/

SDL_Surface *HostSurface::createSdlSurface(int width, int height,
	SDL_PixelFormat *pixelFormat)
{
	return SDL_CreateRGBSurface(SDL_SWSURFACE,
		width,height, pixelFormat->BitsPerPixel,
		pixelFormat->Rmask, pixelFormat->Gmask,
		pixelFormat->Bmask, pixelFormat->Amask
	);
}

/*--- Public functions ---*/

SDL_Surface *HostSurface::getSdlSurface(void)
{
	return surface;
}

int HostSurface::getWidth(void)
{
	return clip_w;
}

int HostSurface::getHeight(void)
{
	return clip_h;
}

int HostSurface::getBpp(void)
{
	return (surface ? surface->format->BitsPerPixel : 0);
}

void HostSurface::update(void)
{
}

void HostSurface::resize(int new_width, int new_height, bool force_recreate)
{
	resize(new_width,new_height, new_width,new_height, force_recreate);
}

void HostSurface::resize(int new_width, int new_height,
	int surf_width, int surf_height, bool force_recreate)
{
	SDL_PixelFormat pixelFormat;
	SDL_Color palette[256];
	bool restore_palette = false;
	bool recreateSurface = force_recreate;

	if (surf_width & 15) {
		surf_width = (surf_width|15)+1;
	}
	if (surf_height & 15) {
		surf_height = (surf_height|15)+1;
	}

	if (!surface) {
		recreateSurface = true;
	} else {
		/* Recreate surface if too small */
		if ((surf_width>surface->w) || (surf_height>surface->h)) {
			recreateSurface = true;
		}
		clip_w = new_width;
		clip_h = new_height;
	}

	if (recreateSurface) {
		memset(&pixelFormat, 0, sizeof(SDL_PixelFormat));

		if (surface) {
			/* Save pixel format */
			memcpy(&pixelFormat, surface->format, sizeof(SDL_PixelFormat));

			/* Save palette ? */
			if ((surface->format->BitsPerPixel==8) && surface->format->palette) {
				int i;

				for (i=0; i<surface->format->palette->ncolors; i++) {
					palette[i].r = surface->format->palette->colors[i].r;
					palette[i].g = surface->format->palette->colors[i].g;
					palette[i].b = surface->format->palette->colors[i].b;
#if SDL_VERSION_ATLEAST(2, 0, 0)
					palette[i].a = surface->format->palette->colors[i].a;
#endif
				}
				restore_palette = SDL_TRUE;
			}

			SDL_FreeSurface(surface);
		} else {
			pixelFormat.BitsPerPixel = 8;
		}

		surface = createSdlSurface(surf_width,surf_height, &pixelFormat);

		if (restore_palette) {
			setPalette(palette, 0, 256);
		}
	}

	resizeDirty(clip_w, clip_h);
}

void HostSurface::setPalette(SDL_Color *palette, int first, int count)
{
	if (!surface || !palette) {
		return;
	}

#if SDL_VERSION_ATLEAST(2, 0, 0)
	SDL_SetPaletteColors(surface->format->palette, palette, first, count);
#else
	SDL_SetPalette(surface, SDL_LOGPAL, palette, first, count);
#endif
}

int HostSurface::getParam(int num_param)
{
	switch (num_param) {
		case SURF_DRAW:
			return draw_mode;
		case SURF_ALPHA:
			return alpha_coef;
		case SURF_USE_ALPHA:
			return use_alpha;
	}

	return 0;
}

void HostSurface::setParam(int num_param, int value)
{
	switch (num_param) {
		case SURF_DRAW:
			draw_mode = value;
			break;
		case SURF_ALPHA:
			alpha_coef = value;
			break;
		case SURF_USE_ALPHA:
			use_alpha = value;
			break;
	}
}
