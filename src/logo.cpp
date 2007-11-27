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
 
/*--- Constructor/destructor ---*/

Logo::Logo(const char *filename)
	: surface(NULL)
{
	load(filename);
}

Logo::~Logo()
{
	if (surface) {
		delete surface;
		surface = NULL;
	}
}

/*--- Public functions ---*/

void Logo::load(const char *filename)
{
	SDL_Surface *sdl_surf;

	if (surface) {
		delete surface;
		surface = NULL;
	}

	SDL_RWops *rwops = SDL_RWFromFile(filename, "rb");
	if (!rwops) {
		return;
	}

#ifdef HAVE_SDL_IMAGE
	sdl_surf = IMG_Load_RW(rwops, 0);
#else
	sdl_surf = SDL_LoadBMP_RW(rwops, 0);
#endif
	SDL_FreeRW(rwops);

	if (!sdl_surf) {
		return;
	}

	surface = new HostSurface(sdl_surf);
	if (!surface) {
		return;
	}

	/* Set color key */
	sdl_surf = surface->getSdlSurface();

	/* FIXME: set key for other bpp ? */
	if (sdl_surf->format->BitsPerPixel == 1) {
		/* Set transparency from first pixel */
		SDL_SetColorKey(sdl_surf, SDL_SRCCOLORKEY|SDL_RLEACCEL, ((Uint8 *)sdl_surf->pixels)[0]);
	}
}

HostSurface *Logo::getSurface(void)
{
	return surface;
}
