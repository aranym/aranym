/*
	NatFeat VDI driver

	ARAnyM (C) 2005,2006 Patrice Mandin

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
#include "cpu_emulation.h"
#include "parameters.h"
#include "dirty_rects.h"
#include "host_surface.h"
#include "nfvdi.h"
#include "fvdidrv_nfapi.h"	// keep in sync with same file in fVDI CVS
#include "hostscreen.h"
#include "host.h"

#include "cpu_emulation.h"

#define DEBUG 0
#include "debug.h"

#include <new>

/*--- Defines ---*/

#define EINVFN -32

/*--- Types ---*/

/*--- Variables ---*/

/*--- Public functions ---*/

int32 VdiDriver::dispatch(uint32 fncode)
{
	int32 ret = EINVFN;

	D(bug("nfvdi: dispatch(%u)", fncode));

	if (fncode & 0x80000000) {
	    if (!events)
		return 1;

	    fncode &= 0x7fffffff;
	    if (fncode & 0x8000) {
		mouse_x = fncode >> 16;
		mouse_y = fncode & 0x7fff;
		new_event |= 1;
	    } else {
		switch (fncode >> 28) {
		case 3:
		    buttons = ((fncode & 1) << 1) | ((fncode & 2) >> 1);
		    new_event |= 2;
		    break;
	        case 4:
		    wheel += (signed char)(fncode & 0xff);
		    new_event |= 4;
		    break;
	        case 5:
		    vblank++;
		    new_event |= 8;
		    break;
		}
	    }
	    TriggerInt3();
	    
	    return 0;
	}

	if (fncode == FVDI_EVENT) {
	    memptr array = getParameter(0);
	    switch((int)array) {
	    case 0:
		return bx_options.nfvdi.use_host_mouse_cursor;
	    case 1:
		new_event = buttons = wheel = vblank = 0;
		events = 1;
		break;
	    default:
		int n = 0;
		if (new_event & 1) {
		    WriteInt32(array + n++ * 4, 2);
		    WriteInt32(array + n++ * 4,
		               (mouse_x << 16) | (mouse_y & 0xffff));
		}
		if (new_event & 2) {
		    WriteInt32(array + n++ * 4, 3);
		    WriteInt32(array + n++ * 4, buttons);
		}
		if (new_event & 4) {
		    WriteInt32(array + n++ * 4, 4);
		    WriteInt32(array + n++ * 4, 0x00000000 | (wheel & 0xffff));
		}
		if (new_event & 8) {
		    WriteInt32(array + n++ * 4, 5);
		    WriteInt32(array + n++ * 4, vblank);
		}
		if (n < 8)
		    WriteInt32(array + n * 4, 0);
		new_event = wheel = vblank = 0;
		break;
	    }
	    return 1;
	}

	switch(fncode) {
		case FVDI_GET_VERSION:
    		ret = FVDIDRV_NFAPI_VERSION;
			break;
		case FVDI_GET_PIXEL:
			ret = getPixel((memptr)getParameter(0),(memptr)getParameter(1),getParameter(2),getParameter(3));
			break;
		case FVDI_PUT_PIXEL:
			ret = putPixel((memptr)getParameter(0),(memptr)getParameter(1),getParameter(2),getParameter(3),getParameter(4));
			break;
		case FVDI_MOUSE:
			{
				// mode (0/1 - move, 2 - hide, 3 - show)
				// These are only valid when not mode
				uint32 mask = getParameter(3);
				if ( mask > 7 ) {
					ret = drawMouse((memptr)getParameter(0),	// wk
						getParameter(1), getParameter(2),	// x, y
						mask,                                // mask*
						getParameter(4),			            // data*
						getParameter(5), getParameter(6),	// hot_x, hot_y
						getParameter(7),			// fgColor
						getParameter(8),			// bgColor
						getParameter(9));			        // type
				} else {
					ret = drawMouse((memptr)getParameter(0),		        // wk
						getParameter(1), getParameter(2),	// x, y
						mask,
						0, 0, 0, 0, 0, 0); // dummy
				}
			}
			break;
		case FVDI_EXPAND_AREA:
			ret = expandArea((memptr)getParameter(0),		// vwk
			                    (memptr)getParameter(1),		// src MFDB*
			                    getParameter(2), getParameter(3),	// sx, sy
			                    (memptr)getParameter(4),		// dest MFDB*
			                    getParameter(5), getParameter(6),	// dx, dy
			                    getParameter(7),			// width
			                    getParameter(8),			// height
			                    getParameter(9),			// logical operation
			                  getParameter(10),			// fgColor
			                  getParameter(11));			// bgColor
			break;
		case FVDI_FILL_AREA:
			ret = fillArea((memptr)getParameter(0),		// vwk
			                  getParameter(1), getParameter(2),	// x, y
			                                               		// table*, table length/type (0 - y/x1/x2 spans)
			                  getParameter(3), getParameter(4),	// width, height
			                  (memptr)getParameter(5),		// pattern*
			                  getParameter(6),			// fgColor
			                  getParameter(7),			// bgColor
			                  getParameter(8),			// mode
			                  getParameter(9));			// interior style
			break;
		case FVDI_BLIT_AREA:
			ret = blitArea((memptr)getParameter(0),		// vwk
			                  (memptr)getParameter(1),		// src MFDB*
			                  getParameter(2), getParameter(3),	// sx, sy
			                  (memptr)getParameter(4),		// dest MFDB*
			                  getParameter(5), getParameter(6),	// dx, dy
			                  getParameter(7),			// width
			                  getParameter(8),			// height
			                  getParameter(9));			// logical operation
			break;
		case FVDI_LINE:
			ret = drawLine((memptr)getParameter(0),		// vwk
			                  getParameter(1), getParameter(2),	// x1, y1
			                                                	// table*, table length/type (0 - coordinate pairs, 1 - pairs + moves)
			                  getParameter(3), getParameter(4),	// x2, y2
			                                                	// move point count, move index*
			                  getParameter(5) & 0xffff,		// pattern
			                  getParameter(6),			// fgColor
			                  getParameter(7),			// bgColor
			                  getParameter(8) & 0xffff,		// logical operation
			                  (memptr)getParameter(9));		// clip rectangle* (0 or long[4])
			break;
		case FVDI_FILL_POLYGON:
			ret = fillPoly((memptr)getParameter(0),		// vwk
			                  (memptr)getParameter(1),		// points*
			                  getParameter(2),			// point count
			                  (memptr)getParameter(3),		// index*
			                  getParameter(4),			// index count
			                  (memptr)getParameter(5),		// pattern*
			                  getParameter(6),			// fgColor
			                  getParameter(7),			// bgColor
			                  getParameter(8),			// logic operation
			                  getParameter(9),			// interior style
			                  (memptr)getParameter(10));		// clip rectangle
			break;
		case FVDI_TEXT_AREA:
			ret = drawText((memptr)getParameter(0),		// vwk
			                  (memptr)getParameter(1),		// text*
			                  getParameter(2),			// length
			                  getParameter(3),			// dst_x
			                  getParameter(4),			// dst_y
			                  (memptr)getParameter(5),		// font*
			                  getParameter(6),			// width
			                  getParameter(7),			// height
			                  getParameter(8),			// fgColor
			                  getParameter(9),			// bgColor
			                  getParameter(10),			// logic operation
			                  (memptr)getParameter(11));		// clip rectangle
			break;
		case FVDI_GET_HWCOLOR:
			getHwColor(getParameter(0), getParameter(1), getParameter(2), getParameter(3), getParameter(4));
			break;
		case FVDI_SET_COLOR:
			setColor(getParameter(4), getParameter(0), getParameter(1), getParameter(2), getParameter(3));
			break;
		case FVDI_GET_FBADDR:
			ret = getFbAddr();
			break;
		case FVDI_SET_RESOLUTION:
			setResolution(getParameter(0),getParameter(1),getParameter(2));
			ret = 0;
			break;
		case FVDI_GET_WIDTH:
			ret = getWidth();
			break;
		case FVDI_GET_HEIGHT:
			ret = getHeight();
			break;
		case FVDI_OPENWK:
			ret = openWorkstation();
			break;
		case FVDI_CLOSEWK:
			ret = closeWorkstation();
			break;
		case FVDI_GETBPP:
			ret = getBpp();
			break;
		default:
			D(bug("nfvdi: unimplemented function #%d", fncode));
			break;
	}

	D(bug("nfvdi: function returning with 0x%08x", ret));
	return ret;
}

VdiDriver::VdiDriver()
	: surface(NULL)
{
	index_count = crossing_count = point_count = 0;
	alloc_index = alloc_crossing = alloc_point = NULL;
	cursor = NULL;
	Mouse.storage.x = Mouse.storage.y = Mouse.storage.width = Mouse.storage.height = 0;
	events = 0;
}

VdiDriver::~VdiDriver()
{
	delete[] alloc_index;
	delete[] alloc_crossing;
	delete[] alloc_point;

	if (cursor) {
		SDL_FreeCursor(cursor);
		cursor = NULL;
	}

	if (surface) {
		host->video->destroySurface(surface);
	}
}

void VdiDriver::reset()
{
	host->video->setVidelRendering(true);
}

/*--- Protected functions ---*/

void VdiDriver::chunkyToBitplane(uint8 *sdlPixelData, uint16 bpp,
	uint16 bitplaneWords[8])
{
	DUNUSED(bpp);
	for (int l=0; l<16; l++) {
		uint8 data = sdlPixelData[l]; // note: this is about 2000 dryhstones speedup (the local variable)

		bitplaneWords[0] <<= 1; bitplaneWords[0] |= (data >> 0) & 1;
		bitplaneWords[1] <<= 1; bitplaneWords[1] |= (data >> 1) & 1;
		bitplaneWords[2] <<= 1; bitplaneWords[2] |= (data >> 2) & 1;
		bitplaneWords[3] <<= 1; bitplaneWords[3] |= (data >> 3) & 1;
		bitplaneWords[4] <<= 1; bitplaneWords[4] |= (data >> 4) & 1;
		bitplaneWords[5] <<= 1; bitplaneWords[5] |= (data >> 5) & 1;
		bitplaneWords[6] <<= 1; bitplaneWords[6] |= (data >> 6) & 1;
		bitplaneWords[7] <<= 1; bitplaneWords[7] |= (data >> 7) & 1;
	}
}

uint32 VdiDriver::applyBlitLogOperation(int logicalOperation,
	uint32 destinationData, uint32 sourceData)
{
	switch(logicalOperation) {
		case ALL_WHITE:
			destinationData = 0;
			break;
		case S_AND_D:
			destinationData = sourceData & destinationData;  
			break;
		case S_AND_NOTD:
			destinationData = sourceData & ~destinationData;
			break;
		case S_ONLY:
			destinationData = sourceData;
			break;
		case NOTS_AND_D:
			destinationData = ~sourceData & destinationData;
			break;
/*
		case D_ONLY:
			destinationData = destinationData;
			break;
*/
		case S_XOR_D:
			destinationData = sourceData ^ destinationData;
			break;
		case S_OR_D:
			destinationData = sourceData | destinationData;
			break;
		case NOT_SORD:
			destinationData = ~(sourceData | destinationData);
			break;
		case NOT_SXORD:
			destinationData = ~(sourceData ^ destinationData);
			break;
		case D_INVERT:
			destinationData = ~destinationData;
			break;
		case S_OR_NOTD:
			destinationData = sourceData | ~destinationData;
			break;
		case NOT_S:
			destinationData = ~sourceData;
			break;
		case NOTS_OR_D:
			destinationData = ~sourceData | destinationData;
			break;
		case NOT_SANDD:
			destinationData = ~(sourceData & destinationData);
			break;
		case ALL_BLACK:
			destinationData = 0xffffffffUL;
			break;
	}

	return destinationData;
}

HostSurface *VdiDriver::getSurface(void)
{
	return surface;
}

void VdiDriver::setResolution(int32 width, int32 height, int32 depth)
{
	if (bx_options.autozoom.fixedsize) {
		width = bx_options.autozoom.width;
		height = bx_options.autozoom.height;
		if ((bx_options.autozoom.width == 0)
		  && (bx_options.autozoom.height == 0))
		{
			width = host->video->getWidth();
			height = host->video->getHeight();
		}
	}

	if (width<64) {
		width = 64;
	}
	if (height<64) {
		height = 64;
	}
	if (depth<8) {
		depth = 8;
	}

	/* Recreate surface if needed */
	if (surface) {
		if (surface->getBpp() == depth) {
			if ((surface->getWidth() != width) || (surface->getHeight() != height)) {
				surface->resize(width, height);
			}
		} else {
			delete surface;
			surface = NULL;
		}
	}
	if (surface==NULL) {
		surface = host->video->createSurface(width,height,depth);
	}

	/* TODO: restore palette ? */
}

int32 VdiDriver::getWidth(void)
{
	return (surface ? surface->getWidth() : 0);
}

int32 VdiDriver::getHeight(void)
{
	return (surface ? surface->getHeight() : 0);
}

int32 VdiDriver::openWorkstation(void)
{
	host->video->setVidelRendering(false);
	return 1;
}

int32 VdiDriver::closeWorkstation(void)
{
	host->video->setVidelRendering(true);
	return 1;
}

int32 VdiDriver::getBpp(void)
{
	if (!surface) {
		return 0;
	}
	SDL_Surface *sdl_surf = surface->getSdlSurface();
	return (sdl_surf ? sdl_surf->format->BitsPerPixel : 0);
}

// The polygon code needs some arrays of unknown size
// These routines and members are used so that no unnecessary allocations are done
bool VdiDriver::AllocIndices(int n)
{
	if (n > index_count) {
		D2(bug("More indices %d->%d\n", index_count, n));
		int count = n * 2;		// Take a few extra right away
		int16* tmp = new(std::nothrow) int16[count];
		if (!tmp) {
			count = n;
			tmp = new(std::nothrow) int16[count];
		}
		if (tmp) {
			delete[] alloc_index;
			alloc_index = tmp;
			index_count = count;
		}
	}

	return index_count >= n;
}

bool VdiDriver::AllocCrossings(int n)
{
	if (n > crossing_count) {
		D2(bug("More crossings %d->%d\n", crossing_count, n));
		int count = n * 2;		// Take a few extra right away
		int16* tmp = new(std::nothrow) int16[count];
		if (!tmp) {
			count = (n * 3) / 2;	// Try not so many extra
			tmp = new(std::nothrow) int16[count];
		}
		if (!tmp) {
			count = n;		// This is going to be slow if it goes on...
			tmp = new(std::nothrow) int16[count];
		}
		if (tmp) {
			memcpy(tmp, alloc_crossing, crossing_count * sizeof(*alloc_crossing));
			delete[] alloc_crossing;
			alloc_crossing = tmp;
			crossing_count = count;
		}
	}

	return crossing_count >= n;
}

bool VdiDriver::AllocPoints(int n)
{
	if (n > point_count) {
		D2(bug("More points %d->%d", point_count, n));
		int count = n * 2;		// Take a few extra right away
		int16* tmp = new(std::nothrow) int16[count * 2];
		if (!tmp) {
			count = n;
			tmp = new(std::nothrow) int16[count * 2];
		}
		if (tmp) {
			delete[] alloc_point;
			alloc_point = tmp;
			point_count = count;
		}
	}

	return point_count >= n;
}


/*--- Virtual functions ---*/

/**
 * Get a coloured pixel.
 *
 * c_read_pixel(Virtual *vwk, MFDB *mfdb, long x, long y)
 * read_pixel
 * In:  a1  VDI struct, source MFDB
 *  d1  x
 *  d2  y
 *
 * Only one mode here.
 *
 * Note that a1 does not point to the VDI struct, but to a place in memory
 * where the VDI struct pointer can be found. Four bytes beyond that address
 * is a pointer to the destination MFDB.
 *
 * Since an MFDB is passed, the source is not necessarily the screen.
 **/

int32 VdiDriver::getPixel(memptr vwk, memptr src, int32 x, int32 y)
{
	DUNUSED(vwk);
	uint32 row_address, color = 0;
	uint16 planes;

	if (!src) {
		D(bug("VdiDriver::getPixel(): source is NULL"));
		return color;
	}

	planes = ReadInt16(src + MFDB_NPLANES);
	row_address = ReadInt32(src + MFDB_ADDRESS) +
		ReadInt16(src + MFDB_WDWIDTH) * 2 * planes * y;

	switch (planes) {
		case 8:
			color = ReadInt8(row_address + x);
			break;
		case 16:
			color = ReadInt16(row_address + x * 2);
			break;
		case 24:
			color = ((uint32)ReadInt8(row_address + x * 3) << 16) +
				((uint32)ReadInt8(row_address + x * 3 + 1) << 8) +
				(uint32)ReadInt8(row_address + x * 3 + 2);
			break;
		case 32:
			color = ReadInt32(row_address + x * 4);
			break;
	}

	return color;
}

/**
 * Set a coloured pixel.
 *
 * c_write_pixel(Virtual *vwk, MFDB *mfdb, long x, long y, long colour)
 * write_pixel
 * In:   a1  VDI struct, destination MFDB
 *   d0  colour
 *   d1  x or table address
 *   d2  y or table length (high) and type (low)
 * XXX: ?
 *
 * This function has two modes:
 *   - single pixel
 *   - table based multi pixel (special mode 0 (low word of 'y'))
 *
 * Note that a1 does not point to the VDI struct, but to a place in memory
 * where the VDI struct pointer can be found. Four bytes beyond that address
 * is a pointer to the destination MFDB.
 *
 * As usual, only the first one is necessary, and a return with d0 = -1
 * signifies that a special mode should be broken down to the basic one.
 *
 * Since an MFDB is passed, the destination is not necessarily the screen.
 **/

int32 VdiDriver::putPixel(memptr vwk, memptr dst, int32 x, int32 y,
	uint32 color)
{
	if (vwk & 1)
		return 0;

	if (!dst) {
		D(bug("VdiDriver::putPixel(): destination is NULL"));
		return color;
	}

	uint16 planes = ReadInt16(dst + MFDB_NPLANES);
	uint32 row_address = ReadInt32(dst + MFDB_ADDRESS) +
		ReadInt16(dst + MFDB_WDWIDTH) * 2 * planes * y;

	switch (planes) {
		case 8:
			WriteInt8(row_address + x, color);
			break;
		case 16:
			WriteInt16(row_address + x * 2, color);
			break;
		case 24:
			WriteInt8(row_address + x * 3, (color >> 16) & 0xff);
			WriteInt8(row_address + x * 3 + 1, (color >> 8) & 0xff);
			WriteInt8(row_address + x * 3 + 2, color & 0xff);
			break;
		case 32:
			WriteInt32(row_address + x * 4, color);
			break;
		}

	return 1;
}

/**
 * Draw the mouse
 *
 * Draw a coloured line between two points.
 *
 * c_mouse_draw(Workstation *wk, long x, long y, Mouse *mouse)
 * mouse_draw
 * In:  a1  Pointer to Workstation struct
 *  d0/d1   x,y
 *  d2  0 - move shown  1 - move hidden  2 - hide  3 - show  >3 - change shape (pointer to mouse struct)
 *
 * Unlike all the other functions, this does not receive a pointer to a VDI
 * struct, but rather one to the screen's workstation struct. This is
 * because the mouse handling concerns the screen as a whole (and the
 * routine is also called from inside interrupt routines).
 *
 * The Mouse structure pointer doubles as a mode variable. If it is a small
 * number, the mouse's state is supposed to change somehow, while a large
 * number is a pointer to a new mouse shape.
 *
 * This is currently not a required function, but it probably should be.
 * The fallback handling is not done in the usual way, and to make it
 * at least somewhat usable, the mouse pointer is reduced to 4x4 pixels.
 *
 * typedef struct Fgbg_ {
 *   short background;
 *   short foreground;
 * } Fgbg;
 *
 * typedef struct Mouse_ {
 * 0  short type;
 * 2   short hide;
 *   struct position_ {
 * 4   short x;
 * 6   short y;
 *   } position;
 *   struct hotspot_ {
 * 8   short x;
 * 10   short y;
 *   } hotspot;
 * 12  Fgbg colour;
 * 16  short mask[16];
 * 48  short data[16];
 * 80  void *extra_info;
 * } Mouse;
 **/

void VdiDriver::restoreMouseBackground(void)
{
}

void VdiDriver::saveMouseBackground(int16 /*x*/, int16 /*y*/, int16 /*width*/,
	int16 /*height*/)
{
}

int32 VdiDriver::drawMouse(memptr wk, int32 x, int32 y, uint32 mode,
	uint32 data, uint32 hot_x, uint32 hot_y, uint32 fgColor, uint32 bgColor,
	uint32 mouse_type)
{
	DUNUSED(wk);
	DUNUSED(fgColor);
	DUNUSED(bgColor);
	DUNUSED(mouse_type);

	D(bug("VdiDriver::drawMouse(): x,y = [%ld,%ld] mode=%ld", x, y, mode));

	switch (mode) {
		case 0:
		case 1:
			break;
		case 2:
#if 0
			SDL_ShowCursor(SDL_DISABLE);
#endif
			break;
		case 3:
			SDL_ShowCursor(SDL_ENABLE);
			break;
		case 4:
		case 5:
			host->video->WarpMouse(x, y);
			break;
		case 6:
		case 7:
			break;
		default:
			{
				if (cursor)
					SDL_FreeCursor(cursor);

				cursor = SDL_CreateCursor(
					(Uint8 *)Atari2HostAddr(data),
					(Uint8 *)Atari2HostAddr(mode),
					16, 16, hot_x, hot_y
				);

				SDL_SetCursor(cursor);
			}
			break;
	}

	return 1;
}

/**
 * Expand a monochrome area to a coloured one.
 *
 * c_expand_area(Virtual *vwk, MFDB *src, long src_x, long src_y,
 *                             MFDB *dst, long dst_x, long dst_y,
 *                             long w, long h, long operation, long colour)
 * expand_area
 * In:  a1  VDI struct, destination MFDB, VDI struct, source MFDB
 *  d0  height and width to move (high and low word)
 *  d1-d2   source coordinates
 *  d3-d4   destination coordinates
 *  d6  background and foreground colour
 *  d7  logic operation
 *
 * Only one mode here.
 *
 * Note that a1 does not point to the VDI struct, but to a place in memory
 * where the VDI struct pointer can be found. Four bytes beyond that address
 * is a pointer to the destination MFDB, and then comes a VDI struct
 * pointer again (the same) and a pointer to the source MFDB.
 *
 * Since MFDBs are passed, the screen is not necessarily involved.
 *
 * A return with 0 gives a fallback (normally pixel by pixel drawing by
 * the fVDI engine).
 *
 * typedef struct MFDB_ {
 *   short *address;
 *   short width;
 *   short height;
 *   short wdwidth;
 *   short standard;
 *   short bitplanes;
 *   short reserved[3];
 * } MFDB;
 **/

int32 VdiDriver::expandArea(memptr vwk, memptr src, int32 sx, int32 sy,
	memptr dest, int32 dx, int32 dy, int32 w, int32 h, uint32 logOp,
	uint32 fgColor, uint32 bgColor)
{
	DUNUSED(vwk);

	if (!dest) {
		D(bug("VdiDriver::expandArea(): destination is NULL"));
		return 1;
	}

	if (getBpp() == 8) {
		fgColor &= 0xff;
		bgColor &= 0xff;
	}

	uint16 pitch = ReadInt16(src + MFDB_WDWIDTH) * 2; // the byte width (always monochrom);
	memptr data  = ReadInt32(src + MFDB_ADDRESS) + sy * pitch; // MFDB *src->address;

	D(bug("fVDI: %s %x %d,%d:%d,%d:%d,%d (%lx, %lx)", "expandArea", logOp, sx, sy, dx, dy, w, h, fgColor, bgColor ));
	D2(bug("fVDI: %s %x,%x : %x,%x", "expandArea - MFDB addresses", src, dest, ReadInt32( src ),ReadInt32( dest )));
	D2(bug("fVDI: %s %x, %d, %d", "expandArea - src: data address, MFDB wdwidth << 1, bitplanes", data, pitch, ReadInt16( src + MFDB_NPLANES )));
	D2(bug("fVDI: %s %x, %d, %d", "expandArea - dst: data address, MFDB wdwidth << 1, bitplanes", ReadInt32(dest), ReadInt16(dest + MFDB_WDWIDTH) * (ReadInt16(dest + MFDB_NPLANES) >> 2), ReadInt16(dest + MFDB_NPLANES)));

	uint32 destPlanes  = (uint32)ReadInt16( dest + MFDB_NPLANES );
	uint32 destPitch   = ReadInt16(dest + MFDB_WDWIDTH) * destPlanes << 1; // MFDB *dest->pitch
	uint32 destAddress = ReadInt32(dest);

	switch(destPlanes) {
		case 16:
			for(uint16 j = 0; j < h; j++) {
				D2(fprintf(stderr, "fVDI: bmp:"));

				uint16 theWord = ReadInt16(data + j * pitch + ((sx >> 3) & 0xfffe));
				for(uint16 i = sx; i < sx + w; i++) {
					uint32 offset = (dx + i - sx) * 2 + (dy + j) * destPitch;
					if (i % 16 == 0)
						theWord = ReadInt16(data + j * pitch + ((i >> 3) & 0xfffe));

					D2(fprintf(stderr, "%s", ((theWord >> (15 - (i & 0xf))) & 1) ? "1" : " "));
					switch(logOp) {
					case 1:
						WriteInt16(destAddress + offset, ((theWord >> (15 - (i & 0xf))) & 1) ? fgColor : bgColor);
						break;
					case 2:
						if ((theWord >> (15 - (i & 0xf))) & 1)
							WriteInt16(destAddress + offset, fgColor);
						break;
					case 3:
						if ((theWord >> (15 - (i & 0xf))) & 1)
							WriteInt16(destAddress + offset, ~ReadInt16(destAddress + offset));
						break;
					case 4:
						if (!((theWord >> (15 - (i & 0xf))) & 1))
							WriteInt16(destAddress + offset, bgColor);
						break;
					}
				}
				D2(bug("")); //newline
			}
			break;
		case 24:
			for(uint16 j = 0; j < h; j++) {
				D2(fprintf(stderr, "fVDI: bmp:"));

				uint16 theWord = ReadInt16(data + j * pitch + ((sx >> 3) & 0xfffe));
				for(uint16 i = sx; i < sx + w; i++) {
					uint32 offset = (dx + i - sx) * 3 + (dy + j) * destPitch;
					if (i % 16 == 0)
						theWord = ReadInt16(data + j * pitch + ((i >> 3) & 0xfffe));

					D2(fprintf(stderr, "%s", ((theWord >> (15 - (i & 0xf))) & 1) ? "1" : " "));
					switch(logOp) {
					case 1:
						put_dtriplet(destAddress + offset, ((theWord >> (15 - (i & 0xf))) & 1) ? fgColor : bgColor);
						break;
					case 2:
						if ((theWord >> (15 - (i & 0xf))) & 1)
							put_dtriplet(destAddress + offset, fgColor);
						break;
					case 3:
						if ((theWord >> (15-(i&0xf))) & 1)
							put_dtriplet(destAddress + offset, ~get_dtriplet(destAddress + offset));
						break;
					case 4:
						if (!((theWord >> (15 - (i & 0xf))) & 1))
							put_dtriplet(destAddress + offset, bgColor);
						break;
					}
				}
				D2(bug("")); //newline
			}
			break;
		case 32:
			for(uint16 j = 0; j < h; j++) {
				D2(fprintf(stderr, "fVDI: bmp:"));

				uint16 theWord = ReadInt16(data + j * pitch + ((sx >> 3) & 0xfffe));
				for(uint16 i = sx; i < sx + w; i++) {
					uint32 offset = (dx + i - sx) * 4 + (dy + j) * destPitch;
					if (i % 16 == 0)
						theWord = ReadInt16(data + j * pitch + ((i >> 3) & 0xfffe));

					D2(fprintf(stderr, "%s", ((theWord >> (15 - (i & 0xf))) & 1) ? "1" : " "));
					switch(logOp) {
					case 1:
						WriteInt32(destAddress + offset, ((theWord >> (15 - (i & 0xf))) & 1) ? fgColor : bgColor);
						break;
					case 2:
						if ((theWord >> (15-(i&0xf))) & 1)
							WriteInt32(destAddress + offset, fgColor);
						break;
					case 3:
						if ((theWord >> (15 - (i & 0xf))) & 1)
							WriteInt32(destAddress + offset, ~ReadInt32(destAddress + offset));
						break;
					case 4:
						if (!((theWord >> (15 - (i & 0xf))) & 1))
							WriteInt32(destAddress + offset, bgColor);
						break;
					}
				}
				D2(bug("")); //newline
			}
			break;
		default:
			{ // do the mangling for bitplanes. TOS<->VDI color conversions implemented.
				uint8 color[16];
				uint16 bitplanePixels[8] = {0, 0, 0, 0, 0, 0, 0, 0}; // initialized just to quiet GCC warning

				for(uint16 j = 0; j < h; j++) {
					D2(fprintf(stderr, "fVDI: bmp:"));

					uint32 address = destAddress + ((((dx >> 4) * destPlanes) << 1) + (dy + j) * destPitch);
					HostScreen::bitplaneToChunky((uint16*)Atari2HostAddr(address), destPlanes, color);

					uint16 theWord = ReadInt16(data + j * pitch + ((sx >> 3) & 0xfffe));
					for(uint16 i = sx; i < sx + w; i++) {
						if (i % 16 == 0) {
							uint32 wordIndex = ((dx + i - sx) >> 4) * destPlanes;

							// convert the 16pixels (VDI->TOS colors - within the chunkyToBitplane
							// function) into the bitplane and write it to the destination
							//
							// note: we can't do the conversion directly
							//       because it needs the little->bigendian conversion
							chunkyToBitplane(color, destPlanes, bitplanePixels);
							for(uint32 d = 0; d < destPlanes; d++)
								WriteInt16(address + (d<<1), bitplanePixels[d]);

							// convert next 16pixels to chunky
							address = destAddress + ((wordIndex << 1) + (dy + j) * destPitch);
							HostScreen::bitplaneToChunky((uint16*)Atari2HostAddr(address), destPlanes, color);
							theWord = ReadInt16(data + j * pitch + ((i >> 3) & 0xfffe));
						}

						D2(fprintf(stderr, "%s", ((theWord >> (15 - (i & 0xf))) & 1) ? "1" : " "));
						switch(logOp) {
							case 1:
								color[i&0xf] = ((theWord >> (15 - (i & 0xf))) & 1) ? fgColor : bgColor;
								break;
							case 2:
								if ((theWord >> (15-(i&0xf))) & 1)
									color[i&0xf] = fgColor;
								break;
							case 3:
								if ((theWord >> (15 - (i & 0xf))) & 1)
									color[i&0xf] = ~color[i&0xf];
								break;
							case 4:
								if (!((theWord >> (15 - (i & 0xf))) & 1))
									color[i&0xf] = bgColor;
								break;
						}
					}
					chunkyToBitplane(color, destPlanes, bitplanePixels);
					for(uint32 d = 0; d < destPlanes; d++)
						WriteInt16(address + (d<<1), bitplanePixels[d]);

					D2(bug("")); //newline
				}
			}
			break;
	}

	return 1;
}

/**
 * Fill a coloured area using a monochrome pattern.
 *
 * c_fill_area(Virtual *vwk, long x, long y, long w, long h,
 *                           short *pattern, long colour, long mode, long interior_style)
 * fill_area
 * In:  a1  VDI struct
 *  d0  height and width to fill (high and low word)
 *  d1  x or table address
 *  d2  y or table length (high) and type (low)
 *  d3  pattern address
 *  d4  colour
 *
 * This function has two modes:
 * - single block to fill
 * - table based y/x1/x2 spans to fill (special mode 0 (low word of 'y'))
 *
 * As usual, only the first one is necessary, and a return with d0 = -1
 * signifies that a special mode should be broken down to the basic one.
 *
 * An immediate return with 0 gives a fallback (normally line based drawing
 * by the fVDI engine for solid fills, otherwise pixel by pixel).
 * A negative return will break down the special mode into separate calls,
 * with no more fallback possible.
 **/

int32 VdiDriver::fillArea(memptr vwk, uint32 x_, uint32 y_, int32 w,
	int32 h, memptr pattern_addr, uint32 fgColor, uint32 bgColor,
	uint32 logOp, uint32 interior_style)
{
	DUNUSED(vwk);
	DUNUSED(x_);
	DUNUSED(y_);
	DUNUSED(w);
	DUNUSED(h);
	DUNUSED(pattern_addr);
	DUNUSED(fgColor);
	DUNUSED(bgColor);
	DUNUSED(logOp);
	DUNUSED(interior_style);

	return -1;
}

/**
 * Blit an area
 *
 * c_blit_area(Virtual *vwk, MFDB *src, long src_x, long src_y,
 *                        MFDB *dst, long dst_x, long dst_y,
 *                        long w, long h, long operation)
 * blit_area
 * In:  a1  VDI struct, destination MFDB, VDI struct, source MFDB
 *  d0  height and width to move (high and low word)
 *  d1-d2   source coordinates
 *  d3-d4   destination coordinates
 *  d7  logic operation
 *
 * Only one mode here.
 *
 * Note that a1 does not point to the VDI struct, but to a place in memory
 * where the VDI struct pointer can be found. Four bytes beyond that address
 * is a pointer to the destination MFDB, and then comes a VDI struct
 * pointer again (the same) and a pointer to the source MFDB.
 *
 * Since MFDBs are passed, the screen is not necessarily involved.
 *
 * A return with 0 gives a fallback (normally pixel by pixel drawing by the
 * fVDI engine).
 **/

int32 VdiDriver::blitArea(memptr vwk, memptr src, int32 sx, int32 sy,
	memptr dest, int32 dx, int32 dy, int32 w, int32 h, uint32 logOp)
{
	int32 ret;

	if (src) {
		if (dest) {
			ret=blitArea_M2M(vwk, src, sx, sy, dest, dx, dy, w, h, logOp);
		} else {
			ret=blitArea_M2S(vwk, src, sx, sy, dest, dx, dy, w, h, logOp);
		}
	} else {
		if (dest) {
			ret=blitArea_S2M(vwk, src, sx, sy, dest, dx, dy, w, h, logOp);
		} else {
			ret=blitArea_S2S(vwk, src, sx, sy, dest, dx, dy, w, h, logOp);
		}
	}

	return ret;
}

int32 VdiDriver::blitArea_M2M(memptr vwk, memptr src, int32 sx, int32 sy,
	memptr dest, int32 dx, int32 dy, int32 w, int32 h, uint32 logOp)
{
	DUNUSED(vwk);

	uint32 planes = ReadInt16(src + MFDB_NPLANES);			// MFDB *src->bitplanes
	uint32 pitch  = ReadInt16(src + MFDB_WDWIDTH) * planes * 2;	// MFDB *src->pitch
	memptr data   = ReadInt32(src) + sy * pitch;			// MFDB *src->address host OS address

	// the destPlanes is always the same?
	planes = ReadInt16(dest + MFDB_NPLANES);		// MFDB *dest->bitplanes
	uint32 destPitch = ReadInt16(dest + MFDB_WDWIDTH) * planes * 2;	// MFDB *dest->pitch
	memptr destAddress = (memptr)ReadInt32(dest);

	D(bug("fVDI: blitArea M->M"));

	uint32 srcData;
	uint32 destData;

	switch(planes) {
		case 16:
			for(int32 j = 0; j < h; j++)
				for(int32 i = sx; i < sx + w; i++) {
					uint32 offset = (dx + i - sx) * 2 + (dy + j) * destPitch;
					srcData = ReadInt16(data + j * pitch + i * 2);
					destData = ReadInt16(destAddress + offset);
					destData = applyBlitLogOperation(logOp, destData, srcData);
					WriteInt16(destAddress + offset, destData);
				}
			break;
		case 24:
			for(int32 j = 0; j < h; j++)
				for(int32 i = sx; i < sx + w; i++) {
					uint32 offset = (dx + i - sx) * 3 + (dy + j) * destPitch;
					srcData = get_dtriplet(data + j * pitch + i * 3);
					destData = get_dtriplet(destAddress + offset);
					destData = applyBlitLogOperation(logOp, destData, srcData);
					put_dtriplet(destAddress + offset, destData);
				}
			break;
		case 32:
			for(int32 j = 0; j < h; j++)
				for(int32 i = sx; i < sx + w; i++) {
					uint32 offset = (dx + i - sx) * 4 + (dy + j) * destPitch;
					srcData = ReadInt32(data + j * pitch + i * 4);
					destData = ReadInt32(destAddress + offset);
					destData = applyBlitLogOperation(logOp, destData, srcData);
					WriteInt32(destAddress + offset, destData);
				}
			break;
		default:
			if (planes < 16) {
				D(bug("fVDI: blitArea M->M: NOT TESTED bitplaneToCunky conversion"));
			}
	}

	return 1;
}

int32 VdiDriver::blitArea_M2S(memptr vwk, memptr src, int32 sx, int32 sy,
	memptr dest, int32 dx, int32 dy, int32 w, int32 h, uint32 logOp)
{
	DUNUSED(vwk);
	DUNUSED(src);
	DUNUSED(sx);
	DUNUSED(sy);
	DUNUSED(dest);
	DUNUSED(dx);
	DUNUSED(dy);
	DUNUSED(w);
	DUNUSED(h);
	DUNUSED(logOp);
	return -1;
}

int32 VdiDriver::blitArea_S2M(memptr vwk, memptr src, int32 sx, int32 sy,
	memptr dest, int32 dx, int32 dy, int32 w, int32 h, uint32 logOp)
{
	DUNUSED(vwk);
	DUNUSED(src);
	DUNUSED(sx);
	DUNUSED(sy);
	DUNUSED(dest);
	DUNUSED(dx);
	DUNUSED(dy);
	DUNUSED(w);
	DUNUSED(h);
	DUNUSED(logOp);
	return -1;
}

int32 VdiDriver::blitArea_S2S(memptr vwk, memptr src, int32 sx, int32 sy,
	memptr dest, int32 dx, int32 dy, int32 w, int32 h, uint32 logOp)
{
	DUNUSED(vwk);
	DUNUSED(src);
	DUNUSED(sx);
	DUNUSED(sy);
	DUNUSED(dest);
	DUNUSED(dx);
	DUNUSED(dy);
	DUNUSED(w);
	DUNUSED(h);
	DUNUSED(logOp);
	return -1;
}

/**
 * Draw a coloured line between two points
 *
 * c_draw_line(Virtual *vwk, long x1, long y1, long x2, long y2,
 *                          long pattern, long colour)
 * draw_line
 * In:  a1  VDI struct
 *  d0  logic operation
 *  d1  x1 or table address
 *  d2  y1 or table length (high) and type (low)
 *  d3  x2 or move point count
 *  d4  y2 or move index address
 *  d5  pattern
 *  d6  colour
 *
 * This function has three modes:
 * - single line
 * - table based coordinate pairs (special mode 0 (low word of 'y1'))
 * - table based coordinate pairs+moves (special mode 1)
 *
 * As usual, only the first one is necessary, and a return with d0 = -1
 * signifies that a special mode should be broken down to the basic one.
 *
 * An immediate return with 0 gives a fallback (normally pixel by pixel
 * drawing by the fVDI engine).
 * A negative return will break down the special modes into separate calls,
 * with no more fallback possible.
 **/

int32 VdiDriver::drawLine(memptr vwk, uint32 x1_, uint32 y1_, uint32 x2_,
	uint32 y2_, uint32 pattern, uint32 fgColor, uint32 bgColor,
	uint32 logOp, memptr clip)
{
	DUNUSED(vwk);
	DUNUSED(x1_);
	DUNUSED(y1_);
	DUNUSED(x2_);
	DUNUSED(y2_);
	DUNUSED(pattern);
	DUNUSED(fgColor);
	DUNUSED(bgColor);
	DUNUSED(logOp);
	DUNUSED(clip);

	return -1;
}

int32 VdiDriver::fillPoly(memptr vwk, memptr points_addr, int n,
	memptr index_addr, int moves, memptr pattern_addr, uint32 fgColor,
	uint32 bgColor, uint32 logOp, uint32 interior_style, memptr clip)
{
	DUNUSED(interior_style);
	if (vwk & 1)
		return -1;      // Don't know about any special fills

	// Allocate arrays for data
	if (!AllocPoints(n) || !AllocIndices(moves) || !AllocCrossings(200))
		return -1;

	uint16 pattern[16];
	for(int i = 0; i < 16; ++i)
		pattern[i] = ReadInt16(pattern_addr + i * 2);

	int cliparray[4];
	int* cliprect = 0;
	if (clip) {	// Clipping is not off
		cliprect = cliparray;
		cliprect[0] = (int16)ReadInt32(clip);
		cliprect[1] = (int16)ReadInt32(clip + 4);
		cliprect[2] = (int16)ReadInt32(clip + 8);
		cliprect[3] = (int16)ReadInt32(clip + 12);
		D2(bug("fVDI: %s %d,%d:%d,%d", "clipLineTO", cliprect[0], cliprect[1],
		       cliprect[2], cliprect[3]));
	}

	Points p(alloc_point);
	int16* index = alloc_index;
	int16* crossing = alloc_crossing;

	for(int i = 0; i < n; ++i) {
		p[i][0] = (int16)ReadInt16(points_addr + i * 4);
		p[i][1] = (int16)ReadInt16(points_addr + i * 4 + 2);
	}
	bool indices = moves;
	for(int i = 0; i < moves; ++i)
		index[i] = (int16)ReadInt16(index_addr + i * 2);


	if (!n)
		return 1;

	if (!indices) {
		if ((p[0][0] == p[n - 1][0]) && (p[0][1] == p[n - 1][1]))
			n--;
	} else {
		moves--;
		if (index[moves] == -4)
			moves--;
		if (index[moves] == -2)
			moves--;
	}

	int miny = p[0][1];
	int maxy = miny;
	for(int i = 1; i < n; ++i) {
		int16 y = p[i][1];
		if (y < miny) {
			miny = y;
		}
		if (y > maxy) {
			maxy = y;
		}
	}
	if (cliprect) {
		if (miny < cliprect[1])
			miny = cliprect[1];
		if (maxy > cliprect[3])
			maxy = cliprect[3];
	}

	int minx = 1000000;
	int maxx = -1000000;

	for(int16 y = miny; y <= maxy; ++y) {
		int ints = 0;
		int16 x1 = 0;	// Make the compiler happy with some initializations
		int16 y1 = 0;
		int16 x2 = 0;
		int16 y2 = 0;
		int move_n = 0;
		int movepnt = 0;
		if (indices) {
			move_n = moves;
			movepnt = (index[move_n] + 4) / 2;
			x2 = p[0][0];
			y2 = p[0][1];
		} else {
			x1 = p[n - 1][0];
			y1 = p[n - 1][1];
		}

		for(int i = indices; i < n; ++i) {
			if (EnoughCrossings(ints + 1) || AllocCrossings(ints + 1))
				crossing = alloc_crossing;
			else
				break;		// At least something will get drawn

			if (indices) {
				x1 = x2;
				y1 = y2;
			}
			x2 = p[i][0];
			y2 = p[i][1];
			if (indices) {
				if (i == movepnt) {
					if (--move_n >= 0)
						movepnt = (index[move_n] + 4) / 2;
					else
						movepnt = -1;		// Never again equal to n
					continue;
				}
			}

			if (y1 < y2) {
				if ((y >= y1) && (y < y2)) {
					crossing[ints++] = SMUL_DIV((y - y1), (x2 - x1), (y2 - y1)) + x1;
				}
			} else if (y1 > y2) {
				if ((y >= y2) && (y < y1)) {
					crossing[ints++] = SMUL_DIV((y - y2), (x1 - x2), (y1 - y2)) + x2;
				}
			}
			if (!indices) {
				x1 = x2;
				y1 = y2;
			}
		}

		for(int i = 0; i < ints - 1; ++i) {
			for(int j = i + 1; j < ints; ++j) {
				if (crossing[i] > crossing[j]) {
					int16 tmp = crossing[i];
					crossing[i] = crossing[j];
					crossing[j] = tmp;
				}
			}
		}

		x1 = cliprect[0];
		x2 = cliprect[2];
		for(int i = 0; i < ints - 1; i += 2) {
			y1 = crossing[i];	// Really x-values, but...
			y2 = crossing[i + 1];
			if (y1 < x1)
				y1 = x1;
			if (y2 > x2)
				y2 = x2;
			if (y1 <= y2) {
				fillArea(y1, y, y2 - y1 + 1, 1, pattern,
				         fgColor, bgColor, logOp);
				if (y1 < minx)
					minx = y1;
				if (y2 > maxx)
					maxx = y2;
			}
		}
	}

	return 1;
}

int32 VdiDriver::drawText(memptr vwk, memptr text, uint32 length, int32 dst_x, int32 dst_y,
			  memptr font, uint32 w, uint32 h, uint32 fgColor, uint32 bgColor,
			  uint32 logOp, memptr clip)
{
	DUNUSED(vwk);
	DUNUSED(text);
	DUNUSED(length);
	DUNUSED(dst_x);
	DUNUSED(dst_y);
	DUNUSED(font);
	DUNUSED(w);
	DUNUSED(h);
	DUNUSED(fgColor);
	DUNUSED(bgColor);
	DUNUSED(logOp);
	DUNUSED(clip);

	return -1;
}

void VdiDriver::getHwColor(uint16 index, uint32 red, uint32 green,
	uint32 blue, memptr hw_value)
{
	DUNUSED(index);
	DUNUSED(red);
	DUNUSED(green);
	DUNUSED(blue);
	DUNUSED(hw_value);
}

void VdiDriver::setColor(memptr vwk, uint32 paletteIndex, uint32 red,
	uint32 green, uint32 blue)
{
	DUNUSED(vwk);
	DUNUSED(paletteIndex);
	DUNUSED(red);
	DUNUSED(green);
	DUNUSED(blue);
}

int32 VdiDriver::getFbAddr(void)
{
	return 0;
}

