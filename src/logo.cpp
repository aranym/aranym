/*
	Logo

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

#include <SDL.h>
#ifdef HAVE_SDL_IMAGE
#include <SDL_image.h>
#endif

#include "dirty_rects.h"
#include "host_surface.h"
#include "logo.h"
#include "hostscreen.h"
#include "host.h"
 
/*--- Constructor/destructor ---*/

Logo::Logo(const char *filename)
	: logo_surf(NULL), surface(NULL)
{
	load(filename);
}

Logo::~Logo()
{
	if (logo_surf) {
		SDL_FreeSurface(logo_surf);
	}
	if (surface) {
		host->video->destroySurface(surface);
	}
}

/*--- Public functions ---*/

void Logo::load(const char *filename)
{
	if (surface) {
		delete surface;
		surface = NULL;
	}

	SDL_RWops *rwops = SDL_RWFromFile(filename, "rb");
	if (!rwops) {
		return;
	}

#ifdef HAVE_SDL_IMAGE
	logo_surf = IMG_Load_RW(rwops, 0);
#else
	logo_surf = SDL_LoadBMP_RW(rwops, 0);
#endif
	SDL_FreeRW(rwops);

	if (!logo_surf) {
		return;
	}

	/* FIXME: set key for other bpp ? */
	if (logo_surf->format->BitsPerPixel == 8) {
		/* Set transparency from first pixel */
		SDL_SetColorKey(logo_surf, SDL_SRCCOLORKEY|SDL_RLEACCEL, ((Uint8 *)logo_surf->pixels)[0]);
	}

	surface = host->video->createSurface(logo_surf->w, logo_surf->h, logo_surf->format);

	if (surface != NULL) {
		surface->setParam(HostSurface::SURF_USE_ALPHA, 1);
	}
}

HostSurface *Logo::getSurface(void)
{
	if (!surface) {
		return NULL;
	}

	SDL_Surface *sdl_surf = surface->getSdlSurface();
	if (!sdl_surf) {
		return NULL;
	}

	Uint8 *dirtyRects = surface->getDirtyRects();

	/* Refresh surface from loaded file if needed */
	int dirty_w = surface->getDirtyWidth();
	int dirty_h = surface->getDirtyHeight();
	for (int y=0; y<dirty_h; y++) {
		for (int x=0; x<dirty_w; x++) {
			if (dirtyRects[y * dirty_w + x]) {
				SDL_Rect src, dst;

				src.x = dst.x = (x<<4);
				src.y = dst.y = (y<<4);
				src.w = dst.w = (1<<4);
				src.h = dst.h = (1<<4);

				SDL_FillRect(surface->getSdlSurface(), &dst, sdl_surf->format->Amask);
				SDL_BlitSurface(logo_surf, &src, surface->getSdlSurface(), &dst);
			}
		}
	}

	return surface;
}

/*
vim:ts=4:sw=4:
*/
