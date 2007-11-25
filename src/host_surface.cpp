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
	dirty_flags = 0;
}

HostSurface::HostSurface(int width, int height, int bpp)
	: DirtyRects(width, height)
{
	surface = SDL_CreateRGBSurface(SDL_SWSURFACE, width, height, bpp,
		0,0,0,0);
	clip_w = surface ? surface->w : 0;
	clip_h = surface ? surface->h : 0;
	dirty_flags = DIRTY_PALETTE|DIRTY_SURFACE|DIRTY_TEXTURE;
}

HostSurface::HostSurface(SDL_Surface *surf, int clip_width, int clip_height)
	: DirtyRects(clip_width, clip_height)
{
	surface = surf;
	clip_w = clip_width;
	clip_h = clip_height;
	dirty_flags = 0;
}

HostSurface::~HostSurface(void)
{
	if (surface) {
		SDL_FreeSurface(surface);
		delete surface;
	}
}

/*--- Public functions ---*/

SDL_Surface *HostSurface::getSurface(void)
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

void HostSurface::resize(int new_width, int new_height)
{
	SDL_PixelFormat pixelFormat;
	bool recreateSurface = false;

	if (!surface) {
		recreateSurface = true;
	} else {
		/* Only change dimensions if we want a smaller surface */
		if (surface->w >= new_width) {
			clip_w = new_width;
		} else {
			recreateSurface = true;
		}
		if (surface->h >= new_height) {
			clip_h = new_height;
		} else {
			recreateSurface = true;
		}
	}

	if (!recreateSurface) {
		return;
	}

	memset(&pixelFormat, 0, sizeof(SDL_PixelFormat));

	if (surface) {
		/* Save pixel format */
		memcpy(&pixelFormat, surface->format, sizeof(SDL_PixelFormat));

		SDL_FreeSurface(surface);
	} else {
		pixelFormat.BitsPerPixel = 8;
	}

	dirty_flags = DIRTY_PALETTE|DIRTY_SURFACE|DIRTY_TEXTURE;

	surface = SDL_CreateRGBSurface(SDL_SWSURFACE,
		clip_w,clip_h, pixelFormat.BitsPerPixel,
		pixelFormat.Rmask, pixelFormat.Gmask,
		pixelFormat.Bmask, pixelFormat.Amask
	);

	resizeDirty(clip_w, clip_h);
}
