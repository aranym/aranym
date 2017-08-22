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

#include "SDL_compat.h"

#include "dirty_rects.h"
#include "host_surface.h"
#include "logo.h"
#include "hostscreen.h"
#include "host.h"

#define DEBUG 0
#include "debug.h"

 
/*--- Constructor/destructor ---*/

Logo::Logo(const char *filename)
	: logo_surf(NULL), surface(NULL), opacity(100)
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

	logo_surf = mySDL_LoadBMP_RW(SDL_RWFromFile(filename, "rb"), 1);
	if (!logo_surf) {
		panicbug("Can not load logo from file %s", filename); 
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

void Logo::alphaBlend(bool init)
{
	if (init) {
		opacity = 100;
	}
	else {
		if (bx_options.opengl.enabled && opacity > 0) {
			if (surface != NULL) {
				surface->setParam(HostSurface::SURF_ALPHA, opacity);
				surface->setDirtyRect(0, 0, surface->getWidth(), surface->getHeight());
				host->video->drawSurfaceToScreen(surface);
			}

			opacity -= 5 * bx_options.video.refresh;
			if (opacity <= 0 && getVIDEL() != NULL && getVIDEL()->getSurface() != NULL && surface != NULL) {
				getVIDEL()->getSurface()->setDirtyRect(0, 0, surface->getWidth(), surface->getHeight());
			}
		}
	}
}

/*
vim:ts=4:sw=4:
*/
