/*
	Falcon VIDEL emulation, with zoom

	(C) 2006-2007 ARAnyM developer team

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
#include <SDL_endian.h>

#include "sysdeps.h"
#include "parameters.h"
#include "hostscreen.h"
#include "host.h"
#include "icio.h"
#include "host_surface.h"
#include "videl.h"
#include "videl_zoom.h"

#define DEBUG 0
#include "debug.h"

#define HW	getHWoffset()

VidelZoom::VidelZoom(memptr addr, uint32 size) :
	VIDEL(addr, size), surface(NULL),
	zoomWidth(0), zoomHeight(0),
	prevWidth(0), prevHeight(0), prevBpp(0),
	xtable(NULL), ytable(NULL)
{
	reset();
}

VidelZoom::~VidelZoom(void)
{
	if (surface) {
		host->video->destroySurface(surface);
	}
	if (xtable) {
		delete xtable;
	}
	if (ytable) {
		delete ytable;
	}
}

void VidelZoom::reset(void)
{
	VIDEL::reset();
}

HostSurface *VidelZoom::getSurface(void)
{
	HostSurface *videl_hsurf = VIDEL::getSurface();
	if (!videl_hsurf) {
		return NULL;
	}

	int videlWidth = videl_hsurf->getWidth();
	int videlHeight = videl_hsurf->getHeight();
	int videlBpp = videl_hsurf->getBpp();

	int hostWidth = host->video->getWidth();
	int hostHeight = host->video->getHeight();
	int autozoom_enabled = bx_options.autozoom.enabled;

	zoomWidth = hostWidth;
	zoomHeight = hostHeight;
	if ((hostWidth>=videlWidth) && (hostHeight>=videlHeight)) {
		if (bx_options.autozoom.integercoefs) {
			int coefx = hostWidth / videlWidth;
			int coefy = hostHeight / videlHeight;
			zoomWidth = videlWidth * coefx;
			zoomHeight = videlHeight * coefy;
		} else if ((aspect_x>1) || (aspect_y>1)) {
			zoomWidth = videlWidth * aspect_x;
			zoomHeight = videlHeight * aspect_y;
			autozoom_enabled = 1;
		}
	}

	/* Return non zoomed surface if correct size, or zoom not suitable */
	if (bx_options.opengl.enabled || !autozoom_enabled ||
		((zoomWidth==videlWidth) && (zoomHeight==videlHeight))) {
		return videl_hsurf;
	}

	/* Recalc zoom table if videl or host screen size changes */
	bool updateZoomTable = (!xtable || !ytable);
	if (prevVidelBpp!=videlBpp) {
		updateZoomTable=true;
		prevVidelBpp=videlBpp;
	}
	if (prevVidelWidth!=videlWidth) {
		updateZoomTable=true;
		prevVidelWidth=videlWidth;
	}
	if (prevVidelHeight!=videlHeight) {
		updateZoomTable=true;
		prevVidelHeight=videlHeight;
	}

	/* Recreate surface if needed */
	if (surface) {
		if (prevBpp == videlBpp) {
			if ((prevWidth!=zoomWidth) || (prevHeight!=zoomHeight)) {
				surface->resize(zoomWidth, zoomHeight);
				updateZoomTable = true;
			}
		} else {
			delete surface;
			surface = NULL;
		}
	}
	if (surface==NULL) {
		surface = host->video->createSurface(zoomWidth,zoomHeight,videlBpp);
		updateZoomTable = true;
	}

	prevWidth = zoomWidth;
	prevHeight = zoomHeight;
	prevBpp = videlBpp;

	/* Update zoom tables if needed */
	if (updateZoomTable) {
		int i;

		if (xtable) {
			delete xtable;
		}
		xtable = new int[zoomWidth];
		for (i=0; i<zoomWidth; i++) {
			xtable[i] = (i*videlWidth)/zoomWidth;
		}

		if (ytable) {
			delete ytable;
		}
		ytable = new int[zoomHeight];
		for (i=0; i<zoomHeight; i++) {
			ytable[i] = (i*videlHeight)/zoomHeight;
		}
	}

	/* Refresh dirty parts of non zoomed surface to zoomed surface */
	refreshScreen();

	return surface;
}

void VidelZoom::forceRefresh(void)
{
	VIDEL::forceRefresh();

	if (!surface) {
		return;
	}

	surface->setDirtyRect(0,0,
		surface->getWidth(), surface->getHeight());
}

void VidelZoom::refreshScreen(void)
{
	HostSurface *videl_hsurf = VIDEL::getSurface();
	if (!videl_hsurf) {
		return;
	}
	SDL_Surface *videl_surf = videl_hsurf->getSdlSurface();
	if (!videl_surf) {
		return;
	}

	if (!surface) {
		return;
	}
	SDL_Surface *sdl_surf = surface->getSdlSurface();
	if (!sdl_surf) {
		return;
	}

	/* Update palette from non zoomed surface */
	if ((videl_hsurf->getBpp()==8) && (surface->getBpp()==8)) {
		int i;
		SDL_Color palette[256];
		for (i=0;i<256;i++) {
			palette[i].r = videl_surf->format->palette->colors[i].r;
			palette[i].g = videl_surf->format->palette->colors[i].g;
			palette[i].b = videl_surf->format->palette->colors[i].b;
		}
		surface->setPalette(palette, 0, 256);
	}

	int videlWidth = videl_hsurf->getWidth();
	int videlHeight = videl_hsurf->getHeight();
	int videlBpp = videl_hsurf->getBpp();

	Uint8 *dirtyRects = videl_hsurf->getDirtyRects();
	if (!dirtyRects) {
		return;
	}

	int dirty_w = videl_hsurf->getDirtyWidth();
	int dirty_h = videl_hsurf->getDirtyHeight();

	int x,y;
	for (y=0;y<dirty_h;y++) {
		/* Atari screen may not have a multiple of 16 lines */
		int num_lines = videl_hsurf->getHeight() - (y<<4);
		if (num_lines>16) {
			num_lines=16;
		}
		for (x=0;x<dirty_w;x++) {
			if (!dirtyRects[y * dirty_w + x]) {
				continue;
			}

			/* Zoom 16x16 block */
			int dst_x1 = ((x<<4) * zoomWidth) / videlWidth;
			int dst_x2 = (((x+1)<<4) * zoomWidth) / videlWidth;
			int dst_y1 = ((y<<4) * zoomHeight) / videlHeight;
			int dst_y2 = (((y<<4)+num_lines) * zoomHeight) / videlHeight;

			int i,j;

			Uint8 *dst = (Uint8 *) sdl_surf->pixels;
			dst += dst_y1 * sdl_surf->pitch;
			dst += dst_x1 * (videlBpp>>3);

			if (videlBpp==16) {
				/* True color, 16 bits surface */
				Uint16 *dst_line = (Uint16 *) dst;
				for(j=dst_y1;j<dst_y2;j++) {
					Uint16 *src_col = (Uint16 *) videl_surf->pixels;
					src_col += ytable[j] * (videl_surf->pitch>>1); 
					Uint16 *dst_col = dst_line;
					for(i=dst_x1;i<dst_x2;i++) {
						*dst_col++ = src_col[xtable[i]];
					}
					dst_line += sdl_surf->pitch >> 1;
				}
			} else {
				/* Bitplanes, 8 bits surface */
				Uint8 *dst_line = (Uint8 *) dst;
				for(j=dst_y1;j<dst_y2;j++) {
					Uint8 *src_col = (Uint8 *) videl_surf->pixels;
					src_col += ytable[j] * videl_surf->pitch; 
					Uint8 *dst_col = dst_line;
					for(i=dst_x1;i<dst_x2;i++) {
						*dst_col++ = src_col[xtable[i]];
					}
					dst_line += sdl_surf->pitch;
				}
			}

			surface->setDirtyRect(dst_x1,dst_y1,
				dst_x2-dst_x1,dst_y2-dst_y1);
		}
	}

	/* Mark original surface as updated */
	videl_hsurf->clearDirtyRects();
}
