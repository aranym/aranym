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
	VIDEL(addr, size),
	xtable(NULL), ytable(NULL)
{
	D(bug("VidelZoom::VidelZoom()"));
	reset();
}

VidelZoom::~VidelZoom(void)
{
	D(bug("VidelZoom::~VidelZoom()"));
	if (xtable) {
		delete xtable;
		xtable = NULL;
	}
	if (ytable) {
		delete ytable;
		ytable = NULL;
	}
}

void VidelZoom::reset(void)
{
	D(bug("VidelZoom::reset()"));
	VIDEL::reset();

	videlWidth = videlHeight = 0;
	zoomWidth = zoomHeight = 0;
	prevWidth = prevHeight = 0;
}

HostSurface *VidelZoom::getSurface(void)
{
	if (!bx_options.autozoom.enabled) {
		return VIDEL::getSurface();
	}

	int videlBpp = getBpp();
	int bpp = (videlBpp<=8) ? 8 : 16;
	videlWidth = getWidth();
	videlHeight = getHeight();
	if (videlWidth<64) {
		videlWidth = 64;
	}
	if (videlHeight<64) {
		videlHeight = 64;
	}

	int hostWidth = host->hostScreen.getWidth();
	int hostHeight = host->hostScreen.getHeight();

	zoomWidth = hostWidth;
	zoomHeight = hostHeight;
	if ((hostWidth>=videlWidth) && (hostHeight>=videlHeight)) {
		if (bx_options.autozoom.integercoefs) {
			int coefx = hostWidth / videlWidth;
			int coefy = hostHeight / videlHeight;
			zoomWidth = videlWidth * coefx;
			zoomHeight = videlHeight * coefy;
		}
	}

	/* Recreate surface if needed */
	if (surface) {
		if (prevVidelBpp == bpp) {
			if ((prevVidelWidth!=zoomWidth) || (prevVidelHeight!=zoomHeight)) {
				surface->resize(zoomWidth, zoomHeight);
			}
		} else {
			delete surface;
			surface = NULL;
		}
	}
	if (surface==NULL) {
		surface = host->hostScreen.createSurface(zoomWidth,zoomHeight,bpp);
	}

	prevVidelWidth = zoomWidth;
	prevVidelHeight = zoomHeight;
	prevVidelBpp = bpp;

	refreshScreen();

	return surface;
}

void VidelZoom::refreshScreen(void)
{
	SDL_Surface *sdl_surf;

	D(bug("VidelZoom::renderScreen()"));
	
	if ((zoomWidth==videlWidth) && (zoomHeight==videlHeight)) {
		VIDEL::refreshScreen();
		return;
	}

	if (!surface) {
		return;
	}
	sdl_surf = surface->getSdlSurface();
	if (!sdl_surf) {
		return;
	}

	if (surface->dirty_flags & HostSurface::DIRTY_PALETTE) {
		refreshPalette();
	}

	/* Recalc zoom tables if needed */
	if (prevWidth!=zoomWidth) {
		if (xtable) {
			delete xtable;
		}
		xtable = new int[zoomWidth];
		for (int i=0; i<zoomWidth; i++) {
			xtable[i] = (i*videlWidth)/zoomWidth;
		}
		prevWidth = zoomWidth;
	}

	if (prevHeight!=zoomHeight) {
		if (ytable) {
			delete ytable;
		}
		ytable = new int[zoomHeight];
		for (int i=0; i<zoomHeight; i++) {
			ytable[i] = (i*videlHeight)/zoomHeight;
		}
		prevWidth = zoomHeight;
	}

	int videlBpp = getBpp();

	int lineoffset = handleReadW(HW + 0x0e) & 0x01ff; // 9 bits
	int linewidth = handleReadW(HW + 0x10) & 0x03ff; // 10 bits

	Uint16 *src = (uint16 *) Atari2HostAddr(getVramAddress());
	int src_pitch = linewidth + lineoffset;
	int dst_pitch = sdl_surf->pitch;
	int x,y;
	int srcLine, prevLine = -1;

	if (videlBpp==16) {
		Uint16 *dst = (Uint16 *) sdl_surf->pixels;
		for (y=0; y<zoomHeight ;y++) {
			srcLine = ytable[y];

			Uint16 *src_line = &src[srcLine*src_pitch];
			Uint16 *dst_line = dst; 

			if (prevLine == srcLine) {
				memcpy(dst_line, dst_line-(dst_pitch>>1), zoomWidth<<1);
			} else {
				for (x=0; x<zoomWidth; x++) {
					Uint16 pixel = src_line[xtable[x]];
					*dst_line++ = SDL_SwapBE16(pixel);
				}
			}
			prevLine = srcLine;
			dst += dst_pitch>>1;
		}
	} else {
		Uint8 chunky[videlWidth];

		Uint8 *dst = (Uint8 *) sdl_surf->pixels;
		for (y=0; y<zoomHeight ;y++) {
			srcLine = ytable[y];

			Uint16 *src_line = &src[srcLine*src_pitch];
			Uint8 *dst_line = dst; 

			if (prevLine == srcLine) {
				memcpy(dst_line, dst_line-dst_pitch, zoomWidth);
			} else {
				convertLineBitplaneToChunky(src_line, chunky, videlWidth, videlBpp);

				for (int x=0; x<zoomWidth; x++) {
					*dst_line++ = chunky[xtable[x]];
				}
			}
			prevLine = srcLine;
			dst += dst_pitch;
		}
	}

	surface->setDirtyRect(0,0,surface->getWidth(),surface->getHeight());
}
