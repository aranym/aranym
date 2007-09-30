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
		SDL_FreeSurface(surface);
	}
}

/*--- Public functions ---*/

void Logo::load(const char *filename)
{
	if (surface) {
		SDL_FreeSurface(surface);
		surface = NULL;
	}

	SDL_RWops *rwops = SDL_RWFromFile(filename, "rb");
	if (!rwops) {
		return;
	}

#ifdef HAVE_SDL_IMAGE
	surface = IMG_Load_RW(rwops, 0);
#else
	surface = SDL_LoadBMP_RW(rwops, 0);
#endif
	SDL_FreeRW(rwops);

	if (!surface) {
		return;
	}

	/* FIXME: set key for other bpp ? */
	if (surface->format->BitsPerPixel == 1) {
		/* Set transparency from first pixel */
		SDL_SetColorKey(surface, SDL_SRCCOLORKEY|SDL_RLEACCEL, ((Uint8 *)surface->pixels)[0]);
	}
}

SDL_Surface *Logo::getSurface(void)
{
	return surface;
}
