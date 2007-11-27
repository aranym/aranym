/*
	Falcon VIDEL emulation

	(C) 2001-2007 ARAnyM developer team

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

#include "sysdeps.h"
#include "hardware.h"
#include "cpu_emulation.h"
#include "memory.h"
#include "hostscreen.h"
#include "host.h"
#include "icio.h"
#include "host_surface.h"
#include "videl.h"
#include "hardware.h"
#include "parameters.h"

#define DEBUG 0
#include "debug.h"

/*--- Defines ---*/

#define HW	getHWoffset()
#define VIDEL_COLOR_REGS_BEGIN	0xff9800UL
#define VIDEL_COLOR_REGS_END	(VIDEL_COLOR_REGS_BEGIN+256*4)

#define F_COLORS(i) handleRead(VIDEL_COLOR_REGS_BEGIN + (i))
#define STE_COLORS(i)	handleReadW(0xff8240 + (i<<1))

/*--- Constructor/destructor ---*/

VIDEL::VIDEL(memptr addr, uint32 size)
	: BASE_IO(addr, size), surface(NULL),
	prevVidelWidth(-1), prevVidelHeight(-1), prevVidelBpp(-1)
{
	reset();
}

VIDEL::~VIDEL(void)
{
	if (surface) {
		delete surface;
	}
}

void VIDEL::reset(void)
{
	updatePalette = false;
}

bool VIDEL::isMyHWRegister(memptr addr)
{
	// FALCON VIDEL COLOR REGISTERS
	if (addr >= VIDEL_COLOR_REGS_BEGIN && addr < VIDEL_COLOR_REGS_END)
		return true;

	return BASE_IO::isMyHWRegister(addr);
}

/*--- BASE_IO functions ---*/

void VIDEL::handleWrite(uint32 addr, uint8 value)
{
	BASE_IO::handleWrite(addr, value);

	if ((addr >= VIDEL_COLOR_REGS_BEGIN && addr < VIDEL_COLOR_REGS_END) ||
		(addr >= 0xff8240 && addr < 0xff8260)) {
		if (surface) {
			surface->dirty_flags |= HostSurface::DIRTY_PALETTE;
		}
		return;
	}

	if ((addr & ~1) == HW+0x60) {
		D(bug("VIDEL st_shift: %06x = %d ($%02x)", addr, value, value));
		// writing to st_shift changed scn_width and vid_mode
		// BASE_IO::handleWrite(HW+0x10, 0x0028);
		// BASE_IO::handleWrite(HW+0xc2, 0x0008);
	}
	else if ((addr & ~1) == HW+0x66) {
		D(bug("VIDEL f_shift: %06x = %d ($%02x)", addr, value, value));
		// IMPORTANT:
		// set st_shift to 0, so we can tell the screen-depth if f_shift==0.
		// Writing 0 to f_shift enables 4 plane Falcon mode but
		// doesn't set st_shift. st_shift!=0 (!=4planes) is impossible
		// with Falcon palette so we set st_shift to 0 manually.
		if (handleReadW(HW+0x66) == 0) {
			BASE_IO::handleWrite(HW+0x60, 0);
		}
	}
}

/*--- Private functions ---*/

bool VIDEL::useStPalette(void)
{
	bool useStPal = false;

	int st_shift = handleReadW(HW + 0x60);
	if (st_shift == 0) {
		// bpp == 4
		int hreg = handleReadW(HW + 0x82); // Too lame!
		// Better way how to make out ST LOW mode wanted
		if (hreg == 0x10 | hreg == 0x17 | hreg == 0x3e) {
			useStPal = true;
		}
	} else if (st_shift == 0x100) {
		// bpp == 2
		useStPal = true;
	} else {
		// bpp == 1	// if (st_shift == 0x200)
		useStPal = true;
	}

	return useStPal;
}

/*--- Protected functions ---*/

Uint32 VIDEL::getVramAddress(void)
{
	return (handleRead(HW + 1) << 16) | (handleRead(HW + 3) << 8) | handleRead(HW + 0x0d);
}

int VIDEL::getWidth(void)
{
	return handleReadW(HW + 0x10) * 16 / getBpp();
}

int VIDEL::getHeight(void)
{
	int vdb = handleReadW(HW + 0xa8);
	int vde = handleReadW(HW + 0xaa);
	int vmode = handleReadW(HW + 0xc2);

	/* visible y resolution:
	 * Graphics display starts at line VDB and ends at line
	 * VDE. If interlace mode off unit of VC-registers is
	 * half lines, else lines.
	 */
	int yres = vde - vdb;
	if (!(vmode & 0x02))		// interlace
		yres >>= 1;
	if (vmode & 0x01)		// double
		yres >>= 1;

	return yres;
}

int VIDEL::getBpp(void)
{
	int f_shift = handleReadW(HW + 0x66);
	int st_shift = handleRead(HW + 0x60);
	/* to get bpp, we must examine f_shift and st_shift.
	 * f_shift is valid if any of bits no. 10, 8 or 4
	 * is set. Priority in f_shift is: 10 ">" 8 ">" 4, i.e.
	 * if bit 10 set then bit 8 and bit 4 don't care...
	 * If all these bits are 0 get display depth from st_shift
	 * (as for ST and STE)
	 */
	int bits_per_pixel = 1;
	if (f_shift & 0x400)		/* 2 colors */
		bits_per_pixel = 1;
	else if (f_shift & 0x100)	/* hicolor */
		bits_per_pixel = 16;
	else if (f_shift & 0x010)	/* 8 bitplanes */
		bits_per_pixel = 8;
	else if (st_shift == 0)
		bits_per_pixel = 4;
	else if (st_shift == 0x01)
		bits_per_pixel = 2;
	else /* if (st_shift == 0x02) */
		bits_per_pixel = 1;

	// D(bug("Videl works in %d bpp, f_shift=%04x, st_shift=%d", bits_per_pixel, f_shift, st_shift));

	return bits_per_pixel;
}

HostSurface *VIDEL::getSurface(void)
{
	int videlBpp = getBpp();
	int bpp = (videlBpp<=8) ? 8 : 16;
	int width = getWidth();
	int height = getHeight();

	if (width<64) {
		width = 64;
	}
	if (height<64) {
		height = 64;
	}

	if (surface) {
		if (prevVidelBpp == bpp) {
			if ((prevVidelWidth!=width) || (prevVidelHeight!=height)) {
				surface->resize(width, height);
			}
		} else {
			delete surface;
			surface = NULL;
		}
	}
	if (surface==NULL) {
		surface = host->hostScreen.createSurface(width,height,bpp);
	}

	prevVidelWidth = width;
	prevVidelHeight = height;
	prevVidelBpp = bpp;

	refreshScreen();

	return surface;
}

void VIDEL::refreshPalette(void)
{
	SDL_Color palette[256];
	int i, c, numColors = useStPalette() ? 16 : 256;

	if (numColors == 16) {
		/* STe palette registers, 4 bits/component */
		for (i=0; i<16; i++) {
			int color = STE_COLORS(i);

			c = (color>>8) & 0x0f;
			c = ((c & 7)<<1)|((c & 1)>>3);
			palette[i].r = (c<<4)|c;
			c = (color>>4) & 0x0f;
			c = ((c & 7)<<1)|((c & 1)>>3);
			palette[i].g = (c<<4)|c;
			c = color & 0x0f;
			c = ((c & 7)<<1)|((c & 1)>>3);
			palette[i].b = (c<<4)|c;
		}
	} else {
		/* Falcon palette registers, 6 bits/component */
		for (i=0; i<256; i++) {
			int offset = i<<2;

			c = F_COLORS(offset) & 0xfc;
			c |= (c>>6) & 3;
			palette[i].r = c;
			c = F_COLORS(offset+1) & 0xfc;
			c |= (c>>6) & 3;
			palette[i].g = c;
			c = F_COLORS(offset+3) & 0xfc;
			c |= (c>>6) & 3;
			palette[i].b = c;
		}
	}

	SDL_SetPalette(surface->getSdlSurface(), SDL_LOGPAL, palette, 0,
		numColors);

	surface->dirty_flags &= ~HostSurface::DIRTY_PALETTE;
}

void VIDEL::refreshScreen(void)
{
	SDL_Surface *sdl_surf;

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

	int videlBpp = getBpp();

	int lineoffset = handleReadW(HW + 0x0e) & 0x01ff; // 9 bits
	int linewidth = handleReadW(HW + 0x10) & 0x03ff; // 10 bits

	Uint16 *src = (uint16 *) Atari2HostAddr(getVramAddress());
	int src_pitch = linewidth + lineoffset;
	int dst_pitch = sdl_surf->pitch;
	int x,y;

	if (videlBpp==16) {
		Uint16 *dst = (Uint16 *) sdl_surf->pixels;
		for (y=0; y<surface->getHeight() ;y++) {
			Uint16 *src_line = src;
			Uint16 *dst_line = dst; 
			for (x=0; x<surface->getWidth(); x++) {
				Uint16 pixel = *src_line++;
				*dst_line++ = SDL_SwapBE16(pixel);
			}
			src += src_pitch;
			dst += dst_pitch>>1;
		}
	} else {
		Uint8 *dst = (Uint8 *) sdl_surf->pixels;
		for (y=0; y<surface->getHeight();y++) {
			convertLineBitplaneToChunky(src, dst, surface->getWidth(), videlBpp);

			src += src_pitch;
			dst += dst_pitch;
		}
	}

	surface->setDirtyRect(0,0,surface->getWidth(),surface->getHeight());
}

void VIDEL::convertLineBitplaneToChunky(Uint16 *source, Uint8 *dest, int width, int bpp)
{
	Uint8 pixels[16];

	for (int x=0; x<width>>4; x++) {
		/* Clear block of pixels */
		memset(pixels, 0, 16);

		for (int plane=0; plane<bpp; plane++) {
			Uint16 planeValue = SDL_SwapBE16(source[plane]);

			for (int pixel=0; pixel<16; pixel++) {
				pixels[pixel] |= (planeValue>>(15-pixel)&1)<<plane;
			}
		}

		/* Copy final values in surface */
		memcpy(dest, pixels, 16);

		source += bpp;
		dest += 16;
	}
}
