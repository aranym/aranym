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

#include <SDL.h>

#include "dirty_rects.h"
#include "host_surface.h"

#define DEBUG 0
#include "debug.h"

/*--- Defines ---*/

/*--- Constructor/destructor ---*/

HostSurface::HostSurface(SDL_Surface *surf)
	: DirtyRects(surf->w, surf->h)
{
	surface = surf;
	clip_w = surf->w;
	clip_h = surf->h;
}

HostSurface::HostSurface(int width, int height, int bpp)
	: DirtyRects(width, height)
{
	surface = SDL_CreateRGBSurface(SDL_SWSURFACE, width, height, bpp,
		0,0,0,0);
	clip_w = surface ? surface->w : 0;
	clip_h = surface ? surface->h : 0;
}

HostSurface::~HostSurface(void)
{
	if (surface) {
		SDL_FreeSurface(surface);
		surface = NULL;
	}
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

void HostSurface::resize(int new_width, int new_height)
{
	resize(new_width,new_height, new_width,new_height);
}

void HostSurface::resize(int new_width, int new_height,
	int surf_width, int surf_height)
{
	SDL_PixelFormat pixelFormat;
	SDL_Color palette[256];
	bool restore_palette = false;
	bool recreateSurface = false;

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

	if (!recreateSurface) {
		return;
	}

	memset(&pixelFormat, 0, sizeof(SDL_PixelFormat));

	if (surface) {
		/* Save pixel format */
		memcpy(&pixelFormat, surface->format, sizeof(SDL_PixelFormat));

		/* Save palette ? */
		if ((surface->format->BitsPerPixel==8) && surface->format->palette) {
			int i;

			for (i=0; i<256; i++) {
				memcpy(&palette[i], &(surface->format->palette[i]), sizeof(SDL_Color));
			}
			restore_palette = SDL_TRUE;
		}

		SDL_FreeSurface(surface);
	} else {
		pixelFormat.BitsPerPixel = 8;
	}

	surface = SDL_CreateRGBSurface(SDL_SWSURFACE,
		surf_width,surf_height, pixelFormat.BitsPerPixel,
		pixelFormat.Rmask, pixelFormat.Gmask,
		pixelFormat.Bmask, pixelFormat.Amask
	);

	if (restore_palette) {
		setPalette(palette, 0, 256);
	}

	resizeDirty(clip_w, clip_h);
}

void HostSurface::setPalette(SDL_Color *palette, int first, int count)
{
	if (!surface || !palette) {
		return;
	}

	SDL_SetPalette(surface, SDL_LOGPAL, palette, first, count);
}
