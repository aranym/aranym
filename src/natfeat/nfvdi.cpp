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

	if (fncode & 0x80000000)
	{
	    if (!events)
		return 1;

	    fncode &= 0x7fffffff;
	    if (fncode & 0x8000)
		{
			mouse_x = fncode >> 16;
			mouse_y = fncode & 0x7fff;
			new_event |= 1;
	    } else
		{
			switch (fncode >> 28)
			{
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

	if (fncode == FVDI_EVENT)
	{
	    memptr array = getParameter(0);
	    switch((int)array)
		{
	    case 0:
			return bx_options.nfvdi.use_host_mouse_cursor;
	    case 1:
			new_event = buttons = wheel = vblank = 0;
			events = 1;
			break;
	    default:
			int n = 0;
			if (new_event & 1)
			{
			    WriteInt32(array + n++ * 4, 2);
			    WriteInt32(array + n++ * 4,
			               (mouse_x << 16) | (mouse_y & 0xffff));
			}
			if (new_event & 2)
			{
			    WriteInt32(array + n++ * 4, 3);
			    WriteInt32(array + n++ * 4, buttons);
			}
			if (new_event & 4)
			{
			    WriteInt32(array + n++ * 4, 4);
			    WriteInt32(array + n++ * 4, 0x00000000 | (wheel & 0xffff));
			}
			if (new_event & 8)
			{
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

	switch(fncode)
	{
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
				if ( mask > 7 )
				{
					ret = drawMouse((memptr)getParameter(0),	// wk
						getParameter(1), getParameter(2),		// x, y
						mask,                               	// mask*
						getParameter(4),			            // data*
						getParameter(5), getParameter(6),		// hot_x, hot_y
						getParameter(7),						// fgColor
						getParameter(8),						// bgColor
						getParameter(9));			        	// type
				} else
				{
					ret = drawMouse((memptr)getParameter(0),	// wk
						getParameter(1), getParameter(2),		// x, y
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

	if (cursor)
	{
		SDL_FreeCursor(cursor);
		cursor = NULL;
	}

	if (surface)
	{
		host->video->destroySurface(surface);
	}
}

void VdiDriver::reset()
{
	host->video->setVidelRendering(true);
}

/*--- Protected functions ---*/

uint32 VdiDriver::applyBlitLogOperation(int logicalOperation,
	uint32 destinationData, uint32 sourceData)
{
	switch(logicalOperation)
	{
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

void VdiDriver::setResolution(int32 width, int32 height, int32 depth)
{
	if (bx_options.autozoom.fixedsize)
	{
		width = bx_options.autozoom.width;
		height = bx_options.autozoom.height;
		if ((bx_options.autozoom.width == 0)
		  && (bx_options.autozoom.height == 0))
		{
			width = host->video->getWidth();
			height = host->video->getHeight();
		}
	}

	if (width < 64)
	{
		width = 64;
	}
	if (height < 64)
	{
		height = 64;
	}
	if (depth < 8)
	{
		depth = 8;
	}

	/* Recreate surface if needed */
	if (surface)
	{
		if (surface->getBpp() == depth)
		{
			if ((surface->getWidth() != width) || (surface->getHeight() != height))
			{
				surface->resize(width, height);
			}
		} else
		{
			delete surface;
			surface = NULL;
		}
	}
	if (surface==NULL)
	{
		surface = host->video->createSurface(width,height,depth);
	}

	/* TODO: restore palette ? */
}

int32 VdiDriver::getWidth(void)
{
	if (surface == NULL)
	{
		bug("VdiDriver::getWidth: no surface");
		return 0;
	}
	return surface->getWidth();
}

int32 VdiDriver::getHeight(void)
{
	if (surface == NULL)
	{
		bug("VdiDriver::getHeight: no surface");
		return 0;
	}
	return surface->getHeight();
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
	if (surface == NULL)
	{
		bug("VdiDriver::getBpp: no surface");
		return 0;
	}
	SDL_Surface *sdl_surf = surface->getSdlSurface();
	return (sdl_surf ? sdl_surf->format->BitsPerPixel : 0);
}

// The polygon code needs some arrays of unknown size
// These routines and members are used so that no unnecessary allocations are done
bool VdiDriver::AllocIndices(int n)
{
	if (n > index_count)
	{
		D2(bug("More indices %d->%d\n", index_count, n));
		int count = n * 2;		// Take a few extra right away
		int16* tmp = new(std::nothrow) int16[count];
		if (!tmp)
		{
			count = n;
			tmp = new(std::nothrow) int16[count];
		}
		if (tmp)
		{
			delete[] alloc_index;
			alloc_index = tmp;
			index_count = count;
		}
	}

	return index_count >= n;
}

bool VdiDriver::AllocCrossings(int n)
{
	if (n > crossing_count)
	{
		D2(bug("More crossings %d->%d\n", crossing_count, n));
		int count = n * 2;		// Take a few extra right away
		int16* tmp = new(std::nothrow) int16[count];
		if (!tmp)
		{
			count = (n * 3) / 2;	// Try not so many extra
			tmp = new(std::nothrow) int16[count];
		}
		if (!tmp)
		{
			count = n;		// This is going to be slow if it goes on...
			tmp = new(std::nothrow) int16[count];
		}
		if (tmp)
		{
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
	if (n > point_count)
	{
		D2(bug("More points %d->%d", point_count, n));
		int count = n * 2;		// Take a few extra right away
		int16* tmp = new(std::nothrow) int16[count * 2];
		if (!tmp)
		{
			count = n;
			tmp = new(std::nothrow) int16[count * 2];
		}
		if (tmp)
		{
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
 * Since an MFDB is passed, the source is not necessarily the screen.
 **/

int32 VdiDriver::getPixel(memptr vwk, memptr src, int32 x, int32 y)
{
	DUNUSED(vwk);
	uint32 color = 0;

	set_mfdb(src);
	if (!src || !mem_mfdb.dest)
	{
		D(bug("VdiDriver::getPixel(): source is NULL"));
		return color;
	}
	return hsGetPixel(x, y);
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
 * As usual, only the first one is necessary, and a return with d0 <= 0
 * signifies that a special mode should be broken down to the basic one.
 *
 * Since an MFDB is passed, the destination is not necessarily the screen.
 **/

int32 VdiDriver::putPixel(memptr vwk, memptr dst, int32 x, int32 y, uint32 color)
{
	set_mfdb(dst);
	if (!dst || !mem_mfdb.dest)
	{
		D(bug("VdiDriver::putPixel(): destination is NULL"));
		return 1;
	}

	if (vwk & 1)
		return 0;

	hsPutPixel(x, y, color);
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

	switch (mode)
	{
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

	if (!dest)
	{
		D(bug("VdiDriver::expandArea(): destination is NULL"));
		return 1;
	}

	if (ReadInt16( src + MFDB_STAND ) & 0x100)
	{
		if ( ReadInt16( src + MFDB_NPLANES ) != 8 )
		{
			bug("fVDI: Only 8bit chunky expand is supported so far.");
			return 1;
		}
		bug("fVDI: FIXME: NYI: 8bit chunky expand");
		return 1;
	}
	
	uint16 pitch = ReadInt16(src + MFDB_WDWIDTH) * 2; // the byte width (always monochrom);
	memptr data  = ReadInt32(src + MFDB_ADDRESS) + sy * pitch; // MFDB *src->address;

	set_mfdb(dest);
	uint32 destPitch   = mem_mfdb.pitch; // MFDB *dest->pitch
	uint32 destAddress = mem_mfdb.dest;

	D(bug("fVDI: %s %x %d,%d:%d,%d:%d,%d (%lx, %lx)", "expandArea", logOp, sx, sy, dx, dy, w, h, fgColor, bgColor ));
	D2(bug("fVDI: %s %x,%x : %x,%x", "expandArea - MFDB addresses", src, dest, ReadInt32(src + MFDB_ADDRESS),ReadInt32(dest + MFDB_ADDRESS)));
	D2(bug("fVDI: %s %x, %d, %d", "expandArea - src: data address, MFDB wdwidth << 1, bitplanes", data, pitch, ReadInt16( src + MFDB_NPLANES )));
	D2(bug("fVDI: %s %x, %d, %d", "expandArea - dst: data address, MFDB wdwidth << 1, bitplanes", destAdress, destPitch, mem_mfdb.planes));

	switch(mem_mfdb.planes)
	{
	case 16:
		for(uint16 j = 0; j < h; j++)
		{
			D2(fprintf(stderr, "fVDI: bmp:"));

			uint16 theWord = ReadInt16(data + j * pitch + ((sx >> 3) & 0xfffe));
			for(uint16 i = sx; i < sx + w; i++)
			{
				uint32 offset = (dx + i - sx) * 2 + (dy + j) * destPitch;
				if (i % 16 == 0)
					theWord = ReadInt16(data + j * pitch + ((i >> 3) & 0xfffe));

				D2(fprintf(stderr, "%s", ((theWord >> (15 - (i & 0xf))) & 1) ? "1" : " "));
				switch(logOp)
				{
				case MD_REPLACE:
					WriteInt16(destAddress + offset, ((theWord >> (15 - (i & 0xf))) & 1) ? fgColor : bgColor);
					break;
				case MD_TRANS:
					if ((theWord >> (15 - (i & 0xf))) & 1)
						WriteInt16(destAddress + offset, fgColor);
					break;
				case MD_XOR:
					if ((theWord >> (15 - (i & 0xf))) & 1)
						WriteInt16(destAddress + offset, ~ReadInt16(destAddress + offset));
					break;
				case MD_ERASE:
					if (!((theWord >> (15 - (i & 0xf))) & 1))
						WriteInt16(destAddress + offset, bgColor);
					break;
				}
			}
			D2(bug("")); //newline
		}
		break;
	case 24:
		for(uint16 j = 0; j < h; j++)
		{
			D2(fprintf(stderr, "fVDI: bmp:"));

			uint16 theWord = ReadInt16(data + j * pitch + ((sx >> 3) & 0xfffe));
			for(uint16 i = sx; i < sx + w; i++)
			{
				uint32 offset = (dx + i - sx) * 3 + (dy + j) * destPitch;
				if (i % 16 == 0)
					theWord = ReadInt16(data + j * pitch + ((i >> 3) & 0xfffe));

				D2(fprintf(stderr, "%s", ((theWord >> (15 - (i & 0xf))) & 1) ? "1" : " "));
				switch(logOp)
				{
				case MD_REPLACE:
					put_dtriplet(destAddress + offset, ((theWord >> (15 - (i & 0xf))) & 1) ? fgColor : bgColor);
					break;
				case MD_TRANS:
					if ((theWord >> (15 - (i & 0xf))) & 1)
						put_dtriplet(destAddress + offset, fgColor);
					break;
				case MD_XOR:
					if ((theWord >> (15-(i&0xf))) & 1)
						put_dtriplet(destAddress + offset, ~get_dtriplet(destAddress + offset));
					break;
				case MD_ERASE:
					if (!((theWord >> (15 - (i & 0xf))) & 1))
						put_dtriplet(destAddress + offset, bgColor);
					break;
				}
			}
			D2(bug("")); //newline
		}
		break;
	case 32:
		for(uint16 j = 0; j < h; j++)
		{
			D2(fprintf(stderr, "fVDI: bmp:"));

			uint16 theWord = ReadInt16(data + j * pitch + ((sx >> 3) & 0xfffe));
			for(uint16 i = sx; i < sx + w; i++)
			{
				uint32 offset = (dx + i - sx) * 4 + (dy + j) * destPitch;
				if (i % 16 == 0)
					theWord = ReadInt16(data + j * pitch + ((i >> 3) & 0xfffe));

				D2(fprintf(stderr, "%s", ((theWord >> (15 - (i & 0xf))) & 1) ? "1" : " "));
				switch(logOp) {
				case MD_REPLACE:
					WriteInt32(destAddress + offset, ((theWord >> (15 - (i & 0xf))) & 1) ? fgColor : bgColor);
					break;
				case MD_TRANS:
					if ((theWord >> (15-(i&0xf))) & 1)
						WriteInt32(destAddress + offset, fgColor);
					break;
				case MD_XOR:
					if ((theWord >> (15 - (i & 0xf))) & 1)
						WriteInt32(destAddress + offset, ~ReadInt32(destAddress + offset));
					break;
				case MD_ERASE:
					if (!((theWord >> (15 - (i & 0xf))) & 1))
						WriteInt32(destAddress + offset, bgColor);
					break;
				}
			}
			D2(bug("")); //newline
		}
		break;
	
	case 8:
	default:
		{ // do the mangling for bitplanes. TOS<->VDI color conversions implemented.
			uint8 color[16];
			uint16 bitplanePixels[8] = { 0, 0, 0, 0, 0, 0, 0, 0 }; // initialized just to quiet GCC warning

			for(uint16 j = 0; j < h; j++)
			{
				D2(fprintf(stderr, "fVDI: bmp:"));

				uint32 address = destAddress + ((((dx >> 4) * mem_mfdb.planes) << 1) + (dy + j) * destPitch);
				HostScreen::bitplaneToChunky((uint16*)Atari2HostAddr(address), mem_mfdb.planes, color);

				uint16 theWord = ReadInt16(data + j * pitch + ((sx >> 3) & 0xfffe));
				for(uint16 i = sx; i < sx + w; i++)
				{
					if (i % 16 == 0)
					{
						uint32 wordIndex = ((dx + i - sx) >> 4) * mem_mfdb.planes;

						// convert the 16pixels (VDI->TOS colors - within the chunkyToBitplane
						// function) into the bitplane and write it to the destination
						//
						// note: we can't do the conversion directly
						//       because it needs the little->bigendian conversion
						HostScreen::chunkyToBitplane(color, mem_mfdb.planes, bitplanePixels);
						for(int d = 0; d < mem_mfdb.planes; d++)
							WriteInt16(address + (d<<1), bitplanePixels[d]);

						// convert next 16pixels to chunky
						address = destAddress + ((wordIndex << 1) + (dy + j) * destPitch);
						HostScreen::bitplaneToChunky((uint16*)Atari2HostAddr(address), mem_mfdb.planes, color);
						theWord = ReadInt16(data + j * pitch + ((i >> 3) & 0xfffe));
					}

					D2(fprintf(stderr, "%s", ((theWord >> (15 - (i & 0xf))) & 1) ? "1" : " "));
					switch(logOp)
					{
					case MD_REPLACE:
						color[i&0xf] = ((theWord >> (15 - (i & 0xf))) & 1) ? fgColor : bgColor;
						break;
					case MD_TRANS:
						if ((theWord >> (15-(i&0xf))) & 1)
							color[i&0xf] = fgColor;
						break;
					case MD_XOR:
						if ((theWord >> (15 - (i & 0xf))) & 1)
							color[i&0xf] = ~color[i&0xf];
						break;
					case MD_ERASE:
						if (!((theWord >> (15 - (i & 0xf))) & 1))
							color[i&0xf] = bgColor;
						break;
					}
				}
				HostScreen::chunkyToBitplane(color, mem_mfdb.planes, bitplanePixels);
				for(int32 d = 0; d < mem_mfdb.planes; d++)
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
	DUNUSED(interior_style);
	memptr wk = ReadInt32((vwk & -2) + VWK_REAL_ADDRESS); /* vwk->real_address */

	set_mfdb(wk + WK_SCREEN_MFDB);
	if (!mem_mfdb.dest)
	{
		D(bug("VdiDriver::fillArea(): destination is NULL"));
		return 1;
	}

	uint16 pattern[16];
	for(int i = 0; i < 16; ++i)
		pattern[i] = ReadInt16(pattern_addr + i * 2);

	memptr table = 0;

	int x = x_;
	int y = y_;

	if (vwk & 1)
	{
		if ((y_ & 0xffff) != 0)
			return -1;		// Don't know about this kind of table operation
		table = (memptr)x_;
		h = (y_ >> 16) & 0xffff;
		vwk -= 1;
	}

	D(bug("fVDI: %s %d %d,%d:%d,%d : %d,%d p:%x, (fgc:%lx : bgc:%lx)", "fillArea",
	      logOp, x, y, w, h, x + w - 1, x + h - 1, *pattern,
	      fgColor, bgColor));

	/* Perform rectangle fill. */
	if (!table)
	{
		hsFillArea(x, y, w, h, pattern, fgColor, bgColor, logOp);
	} else
	{
		for(h = h - 1; h >= 0; h--)
		{
			y = (int16)ReadInt16(table); table+=2;
			x = (int16)ReadInt16(table); table+=2;
			w = (int16)ReadInt16(table) - x + 1; table+=2;
			hsFillArea(x, y, w, 1, pattern, fgColor, bgColor, logOp);
		}
	}

	return 1;
}

#define putBpp24Pixel( address, color ) \
{ \
        ((address))[0] = ((color) >> 16) & 0xff; \
        ((address))[1] = ((color) >> 8) & 0xff; \
        ((address))[2] = (color) & 0xff; \
}

#define getBpp24Pixel( address ) \
    ( ((uint32)(address)[0] << 16) | ((uint32)(address)[1] << 8) | (uint32)(address)[2] )

void VdiDriver::hsFillArea(uint32 x, uint32 y, uint32 w, uint32 h,
                             uint16* areaPattern, uint32 fgColor, uint32 bgColor,
                             uint32 logOp)
{
	int pitch;
	int16 i;
	int16 dx=w;
	int16 dy=h;
	uint8_t *pixels;

	/* More variable setup */
	pitch = mem_mfdb.pitch;
	pixels = mem_mfdb.pixels;

	// STanda // FIXME here the pattern should be checked out of the loops for performance
			  // but for now it is good enough (if there is no pattern -> another switch?)

	/* Draw */
	switch(mem_mfdb.planes)
	{
	case 8:
		{
		uint8 *pixel, *pixellast;

		pixel = pixels + x + pitch * y;
		pixellast = pixel + pitch * dy;
		pitch -= dx;
		switch (logOp)
		{
		case MD_REPLACE:
			for (; pixel<pixellast; pixel += pitch)
			{
				uint16 pattern = areaPattern[ y++ & 0xf ];

				for (i=0; i<dx; i++)
				{
					*pixel = (( ( pattern & ( 1 << ( (x+i) & 0xf ) )) != 0 ) ? fgColor : bgColor);
					pixel += 1;
				}
			}
			break;
		case MD_TRANS:
			for (; pixel<pixellast; pixel += pitch)
			{
				uint16 pattern = areaPattern[ y++ & 0xf ];

				for (i=0; i<dx; i++)
				{
					if ( ( pattern & ( 1 << ( (x+i) & 0xf ) )) != 0 )
						*pixel = fgColor;
					pixel += 1;
				}
			}
			break;
		case MD_XOR:
			for (; pixel<pixellast; pixel += pitch)
			{
				uint16 pattern = areaPattern[ y++ & 0xf ];

				for (i=0; i<dx; i++)
				{
					if ( ( pattern & ( 1 << ( (x+i) & 0xf ) )) != 0 )
						*pixel = ~(*(uint8*)pixel);
					pixel += 1;
				}
			}
			break;
		case MD_ERASE:
			for (; pixel<pixellast; pixel += pitch)
			{
				uint16 pattern = areaPattern[ y++ & 0xf ];

				for (i=0; i<dx; i++)
				{
					if ( ( pattern & ( 1 << ( (x+i) & 0xf ) )) == 0 )
						*pixel = fgColor;
					pixel += 1;
				}
			}
			break;
		}
		}
		break;
	case 15:
	case 16:
		{
		uint16_t *pixel16, *pixellast;

		pitch >>= 1;
		pixel16 = (uint16_t *)pixels + x + pitch * y;
		pixellast = pixel16 + pitch * dy;
		pitch -= dx;
		fgColor = SDL_SwapBE16(fgColor);
		bgColor = SDL_SwapBE16(bgColor);
		//				D2(bug("bix pix: %d, %x, %d", y, pixel, pitch));

		switch (logOp)
		{
		case MD_REPLACE:
			for (; pixel16<pixellast; pixel16 += pitch)
			{
				uint16 pattern = areaPattern[ y++ & 0xf ];

				for (i=0; i<dx; i++)
				{
					*pixel16 = ( ( pattern & ( 1 << ( (x+i) & 0xf ) )) != 0 ) ? fgColor : bgColor;
					pixel16 += 1;
				}
			}
			break;
		case MD_TRANS:
			for (; pixel16<pixellast; pixel16 += pitch)
			{
				uint16 pattern = areaPattern[ y++ & 0xf ];

				for (i=0; i<dx; i++)
				{
					if ( ( pattern & ( 1 << ( (x+i) & 0xf ) )) != 0 )
						*pixel16 = fgColor;
					pixel16 += 1;
				}
			}
			break;
		case MD_XOR:
			for (; pixel16<pixellast; pixel16 += pitch)
			{
				uint16 pattern = areaPattern[ y++ & 0xf ];

				for (i=0; i<dx; i++)
				{
					if ( ( pattern & ( 1 << ( (x+i) & 0xf ) )) != 0 )
						*pixel16 = ~(*pixel16);
					pixel16 += 1;
				}
			}
			break;
		case MD_ERASE:
			for (; pixel16<pixellast; pixel16 += pitch)
			{
				uint16 pattern = areaPattern[ y++ & 0xf ];

				for (i=0; i<dx; i++)
				{
					if ( ( pattern & ( 1 << ( (x+i) & 0xf ) )) == 0 )
						*pixel16 = fgColor;
					pixel16 += 1;
				}
			}
			break;
		}
		}
		break;
	case 24:
		{
		uint8 *pixel, *pixellast;

		pixel = pixels + 3 * x + pitch * y;
		pixellast = pixel + pitch * dy;
		pitch -= 3 * dx;
		switch (logOp)
		{
		case MD_REPLACE:
			for (; pixel<pixellast; pixel += pitch)
			{
				uint16 pattern = areaPattern[ y++ & 0xf ];

				for (i=0; i<dx; i++)
				{
					putBpp24Pixel( pixel, (( ( pattern & ( 1 << ( (x+i) & 0xf ) )) != 0 ) ? fgColor : bgColor) );
					pixel += 3;
				}
			}
			break;
		case MD_TRANS:
			for (; pixel<pixellast; pixel += pitch)
			{
				uint16 pattern = areaPattern[ y++ & 0xf ];

				for (i=0; i<dx; i++)
				{
					if ( ( pattern & ( 1 << ( (x+i) & 0xf ) )) != 0 )
						putBpp24Pixel( pixel, fgColor );
					pixel += 3;
				}
			}
			break;
		case MD_XOR:
			for (; pixel<pixellast; pixel += pitch)
			{
				uint16 pattern = areaPattern[ y++ & 0xf ];

				for (i=0; i<dx; i++)
				{
					if ( ( pattern & ( 1 << ( (x+i) & 0xf ) )) != 0 )
						putBpp24Pixel( pixel, ~ getBpp24Pixel( pixel ) );
					pixel += 3;
				}
			}
			break;
		case MD_ERASE:
			for (; pixel<pixellast; pixel += pitch)
			{
				uint16 pattern = areaPattern[ y++ & 0xf ];

				for (i=0; i<dx; i++)
				{
					if ( ( pattern & ( 1 << ( (x+i) & 0xf ) )) == 0 )
						putBpp24Pixel( pixel, fgColor );
					pixel += 3;
				}
			}
			break;
		}
		}
		break;
	case 32:
		{
		uint32 *pixel32, *pixellast;

		pitch >>= 2;
		pixel32 = (uint32_t *)pixels + x + pitch * y;
		pixellast = pixel32 + pitch * dy;
		fgColor = SDL_SwapBE32(fgColor);
		bgColor = SDL_SwapBE32(bgColor);
		pitch -= dx;
		switch (logOp)
		{
		case MD_REPLACE:
			for (; pixel32<pixellast; pixel32 += pitch)
			{
				uint16 pattern = areaPattern[ y++ & 0xf ];

				for (i=0; i<dx; i++)
				{
					*pixel32 = (( ( pattern & ( 1 << ( (x+i) & 0xf ) )) != 0 ) ? fgColor : bgColor);
					pixel32 += 1;
				}
			}
			break;
		case MD_TRANS:
			for (; pixel32<pixellast; pixel32 += pitch)
			{
				uint16 pattern = areaPattern[ y++ & 0xf ];

				for (i=0; i<dx; i++)
				{
					if ( ( pattern & ( 1 << ( (x+i) & 0xf ) )) != 0 )
						*pixel32 = fgColor;
					pixel32 += 1;
				}
			}
			break;
		case MD_XOR:
			for (; pixel32<pixellast; pixel32 += pitch)
			{
				uint16 pattern = areaPattern[ y++ & 0xf ];

				for (i=0; i<dx; i++)
				{
					if ( ( pattern & ( 1 << ( (x+i) & 0xf ) )) != 0 )
						*pixel32 = ~(*pixel32);
					pixel32 += 1;
				}
			}
			break;
		case MD_ERASE:
			for (; pixel32<pixellast; pixel32 += pitch)
			{
				uint16 pattern = areaPattern[ y++ & 0xf ];

				for (i=0; i<dx; i++)
				{
					if ( ( pattern & ( 1 << ( (x+i) & 0xf ) )) == 0 )
						*pixel32 = fgColor;
					pixel32 += 1;
				}
			}
			break;
		}
		}
		break;
	}
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
 * Since MFDBs are passed, the screen is not necessarily involved.
 *
 * A return with 0 gives a fallback (normally pixel by pixel drawing by the
 * fVDI engine).
 **/

int32 VdiDriver::blitArea(memptr vwk, memptr src, int32 sx, int32 sy,
	memptr dest, int32 dx, int32 dy, int32 w, int32 h, uint32 logOp)
{
	int32 ret;

	if (src)
	{
		if (dest)
		{
			ret=blitArea_M2M(vwk, src, sx, sy, dest, dx, dy, w, h, logOp);
		} else
		{
			/* can only blit to screen if MFDB has screen format */
			if (ReadInt16(src + MFDB_NPLANES) != surface->getBpp())
				ret = 1;
			else
				ret=blitArea_M2S(vwk, src, sx, sy, dest, dx, dy, w, h, logOp);
		}
	} else
	{
		if (dest)
		{
			/* can only blit to memory if MFDB has screen format */
			if (ReadInt16(dest + MFDB_NPLANES) != surface->getBpp())
				ret = 1;
			else
				ret=blitArea_S2M(vwk, src, sx, sy, dest, dx, dy, w, h, logOp);
		} else
		{
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
	if (planes == 15)
		planes = 16;
	uint32 pitch  = ReadInt16(src + MFDB_WDWIDTH) * planes * 2;	// MFDB *src->pitch
	memptr data   = ReadInt32(src + MFDB_ADDRESS) + sy * pitch;			// MFDB *src->address host OS address

	// the destPlanes is always the same?
	set_mfdb(dest);
	uint32 destPitch = mem_mfdb.pitch;	// MFDB *dest->pitch
	memptr destAddress = mem_mfdb.dest;

	D(bug("fVDI: blitArea M->M"));

	uint32 srcData;
	uint32 destData;

	switch(planes)
	{
	case 16:
		for(int32 j = 0; j < h; j++)
			for(int32 i = sx; i < sx + w; i++)
			{
				uint32 offset = (dx + i - sx) * 2 + (dy + j) * destPitch;
				srcData = ReadInt16(data + j * pitch + i * 2);
				destData = ReadInt16(destAddress + offset);
				destData = applyBlitLogOperation(logOp, destData, srcData);
				WriteInt16(destAddress + offset, destData);
			}
		break;
	case 24:
		for(int32 j = 0; j < h; j++)
			for(int32 i = sx; i < sx + w; i++)
			{
				uint32 offset = (dx + i - sx) * 3 + (dy + j) * destPitch;
				srcData = get_dtriplet(data + j * pitch + i * 3);
				destData = get_dtriplet(destAddress + offset);
				destData = applyBlitLogOperation(logOp, destData, srcData);
				put_dtriplet(destAddress + offset, destData);
			}
		break;
	case 32:
		for(int32 j = 0; j < h; j++)
			for(int32 i = sx; i < sx + w; i++)
			{
				uint32 offset = (dx + i - sx) * 4 + (dy + j) * destPitch;
				srcData = ReadInt32(data + j * pitch + i * 4);
				destData = ReadInt32(destAddress + offset);
				destData = applyBlitLogOperation(logOp, destData, srcData);
				WriteInt32(destAddress + offset, destData);
			}
		break;
	case 8:
		if ( ((uint32)ReadInt16( src + MFDB_STAND ) & 0x100))
		{
			// chunky
			D(bug("fVDI: blitArea M->S: chunky8bit"));
			for(int32 j = 0; j < h; j++)
			{
				for(int32 i = sx; i < sx + w; i++)
				{
					uint32 offset = (dx + i - sx) + (dy + j) * destPitch;
					srcData = ReadInt8(data + j * pitch + i);
					destData = ReadInt8(destAddress + offset);
					destData = applyBlitLogOperation(logOp, destData, srcData);
					WriteInt8(destAddress + offset, destData);
				}
			}
			return 1;
		}
		/* fall through */
	default:
		D(bug("fVDI: blitArea M->M: NOT TESTED bitplaneToChunky conversion"));
		break;
	}

	return 1;
}


void VdiDriver::set_mfdb(memptr mfdb)
{
	mem_mfdb.dest = ReadInt32(mfdb + MFDB_ADDRESS);
	mem_mfdb.planes = ReadInt16(mfdb + MFDB_NPLANES);
	if (mem_mfdb.planes == 15)
		mem_mfdb.planes = 16;
	mem_mfdb.width = ReadInt16(mfdb + MFDB_WIDTH);
	mem_mfdb.height = ReadInt16(mfdb + MFDB_HEIGHT);
	mem_mfdb.pitch = ReadInt16(mfdb + MFDB_WDWIDTH) * 2 * mem_mfdb.planes;
	mem_mfdb.pixels = Atari2HostAddr(mem_mfdb.dest);
	mem_mfdb.standard = ReadInt16(mfdb + MFDB_STAND);
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
	memptr wk = ReadInt32((vwk & -2) + VWK_REAL_ADDRESS); /* vwk->real_address */
	set_mfdb(wk + WK_SCREEN_MFDB);

	if (!mem_mfdb.dest)
	{
		D(bug("VdiDriver::drawLine(): destination is NULL"));
		return 1;
	}

	memptr table = 0;
	memptr index = 0;
	int length = 0;
	int moves = 0;

	int16 x1 = x1_;
	int16 y1 = y1_;
	int16 x2 = x2_;
	int16 y2 = y2_;

	if (vwk & 1)
	{
		if ((unsigned)(y1 & 0xffff) > 1)
			return -1;		/* Don't know about this kind of table operation */
		table = (memptr)x1_;
		length = (y1_ >> 16) & 0xffff;
		if ((y1_ & 0xffff) == 1)
		{
			index = (memptr)y2_;
			moves = x2_ & 0xffff;
		}
		x1 = (int16)ReadInt16(table);
		y1 = (int16)ReadInt16(table + 2);
		x2 = (int16)ReadInt16(table + 4);
		y2 = (int16)ReadInt16(table + 6);
	}

	int cliparray[4];
	if (clip)				// Clipping is not off
	{
		cliparray[0] = (int16)ReadInt32(clip);
		cliparray[1] = (int16)ReadInt32(clip + 4);
		cliparray[2] = (int16)ReadInt32(clip + 8);
		cliparray[3] = (int16)ReadInt32(clip + 12);
		D2(bug("fVDI: %s %d,%d:%d,%d", "clipLineTO", cliparray[0], cliparray[1],
		       cliparray[2], cliparray[3]));
	} else
	{
		cliparray[0] = 0;
		cliparray[1] = 0;
		cliparray[2] = ReadInt16(wk + WK_SCREEN_MFDB + MFDB_WIDTH) - 1;
		cliparray[3] = ReadInt16(wk + WK_SCREEN_MFDB + MFDB_HEIGHT) - 1;
	}

	if (table)
	{
		if (moves)
		{
			drawMoveLine(table, length, index, moves, pattern, fgColor, bgColor,
			             logOp, cliparray);
		} else
		{
			switch (length)
			{
			case 0:
				break;
			case 1:
				drawSingleLine(x1, y1, x2, y2, pattern, fgColor, bgColor,
				               logOp, cliparray);
				break;
			default:
				drawTableLine(table, length, pattern, fgColor, bgColor,
				              logOp, cliparray);
 				break;
			}
 		}
	} else
	{
		drawSingleLine(x1, y1, x2, y2, pattern, fgColor, bgColor,
		               logOp, cliparray);
	}

	return 1;
}

bool VdiDriver::clipLine(int x1, int y1, int x2, int y2, int cliprect[])
{
	// Get clipping boundary
	int left, top, right, bottom;
	int swaptmp;
	
	left   = cliprect[0];
	top    = cliprect[1];
	right  = cliprect[2];
	bottom = cliprect[3];
	if (right < left) { swaptmp = right; right = left; left = swaptmp; }
	if (bottom < top) { swaptmp = bottom; bottom = top; top = swaptmp; }
	cliprect[0] = left;
	cliprect[1] = top;
	cliprect[2] = right;
	cliprect[3] = bottom;
	if (x2 < x1) { swaptmp = x1; x1 = x2; x2 = swaptmp; }
	if (y2 < y1) { swaptmp = y1; y1 = y2; y2 = swaptmp; }
	if (x2 < left || x1 > right)
		return false;	
	if (y2 < top || y1 > bottom)
		return false;
	return true;
}


// Don't forget rotation of pattern!
int VdiDriver::drawSingleLine(int x1, int y1, int x2, int y2, uint16 pattern,
                               uint32 fgColor, uint32 bgColor, int logOp,
                               int cliprect[])
{
	if (clipLine(x1, y1, x2, y2, cliprect))	// Do not draw the line when it is completely out
	{
		D(bug("fVDI: %s %d,%d:%d,%d (%lx,%lx)", "drawSingleLine", x1, y1, x2, y2, fgColor, bgColor));
		hsDrawLine(x1, y1, x2, y2, pattern, fgColor, bgColor, logOp, cliprect);
	}
	return 1;
}


// Don't forget rotation of pattern!
int VdiDriver::drawTableLine(memptr table, int length, uint16 pattern,
                              uint32 fgColor, uint32 bgColor, int logOp,
                              int cliprect[])
{
	int x1 = (int16)ReadInt16(table); table+=2;
	int y1 = (int16)ReadInt16(table); table+=2;
	for(--length; length > 0; length--)
	{
		int x2 = (int16)ReadInt16(table); table+=2;
		int y2 = (int16)ReadInt16(table); table+=2;

		drawSingleLine(x1, y1, x2, y2, pattern, fgColor, bgColor,
		               logOp, cliprect);
		x1 = x2;
		y1 = y2;
	}

	return 1;
}


// Don't forget rotation of pattern!
int VdiDriver::drawMoveLine(memptr table, int length, memptr index, int moves, uint16 pattern,
                             uint32 fgColor, uint32 bgColor, int logOp,
                             int cliprect[])
{
	int x1 = (int16)ReadInt16(table); table+=2;
	int y1 = (int16)ReadInt16(table); table+=2;
	moves *= 2;
	moves-=2;
	if ((int16)ReadInt16(index + moves) == -4)
		moves-=2;
	if ((int16)ReadInt16(index + moves) == -2)
		moves-=2;
	int movepnt = -1;
	if (moves >= 0)
		movepnt = ((int16)ReadInt16(index + moves) + 4) / 2;
	for(int n = 1; n < length; n++)
	{
		int x2 = (int16)ReadInt16(table); table+=2;
		int y2 = (int16)ReadInt16(table); table+=2;
		if (n == movepnt)
		{
			moves-=2;
			if (moves >= 0)
				movepnt = ((int16)ReadInt16(index + moves) + 4) / 2;
			else
				movepnt = -1;		/* Never again equal to n */
			x1 = x2;
			y1 = y2;
			continue;
		}

		drawSingleLine(x1, y1, x2, y2, pattern, fgColor, bgColor,
		               logOp, cliprect);

		x1 = x2;
		y1 = y2;
	}

	return 1;
}


#define lineloop(putfg, putbg, BytesPerPixel) \
	switch (vec) \
	{ \
	case 0: \
		/* drawing from bottom left to top right */ \
		if (pattern == 0xffff && (logOp == MD_REPLACE || logOp == MD_TRANS)) \
		{ \
			for (;;) \
			{ \
				if (!clipped(x, y, cliprect)) \
				{ \
					putfg; \
				} \
				if (--diff == 0) \
					break; \
				if (x1 >= 0) \
				{ \
					++x, pixel += BytesPerPixel; \
					x1 += dy; \
				} \
				if (x1 < 0) \
				{ \
					--y, pixel -= pitch; \
					x1 += dx; \
				} \
			} \
		} else \
		{ \
			for (;; ppos++) \
			{ \
				if (!clipped(x, y, cliprect)) \
				{ \
					if ( ( pattern & ( 1 << ( (ppos) & 0xf ) )) != 0 ) \
					{ \
						putfg; \
					} else \
					{ \
						putbg; \
					} \
				} \
				if (--diff == 0) \
					break; \
				if (x1 >= 0) \
				{ \
					++x, pixel += BytesPerPixel; \
					x1 += dy; \
				} \
				if (x1 < 0) \
				{ \
					--y, pixel -= pitch; \
					x1 += dx; \
				} \
			} \
		} \
		break; \
	case 1: \
		/* drawing from bottom right to top left */ \
		if (pattern == 0xffff && (logOp == MD_REPLACE || logOp == MD_TRANS)) \
		{ \
			for (;;) \
			{ \
				if (!clipped(x, y, cliprect)) \
				{ \
					putfg; \
				} \
				if (--diff == 0) \
					break; \
				if (x1 >= 0) \
				{ \
					--x, pixel -= BytesPerPixel; \
					x1 += dy; \
				} \
				if (x1 < 0) \
				{ \
					--y, pixel -= pitch; \
					x1 += dx; \
				} \
			} \
		} else \
		{ \
			for (;; ppos++) \
			{ \
				if (!clipped(x, y, cliprect)) \
				{ \
					if ( ( pattern & ( 1 << ( (ppos) & 0xf ) )) != 0 ) \
					{ \
						putfg; \
					} else \
					{ \
						putbg; \
					} \
				} \
				if (--diff == 0) \
					break; \
				if (x1 >= 0) \
				{ \
					--x, pixel -= BytesPerPixel; \
					x1 += dy; \
				} \
				if (x1 < 0) \
				{ \
					--y, pixel -= pitch; \
					x1 += dx; \
				} \
			} \
		} \
		break; \
	case 2: \
		/* drawing from top left to bottom right */ \
		if (pattern == 0xffff && (logOp == MD_REPLACE || logOp == MD_TRANS)) \
		{ \
			for (;;) \
			{ \
				if (!clipped(x, y, cliprect)) \
				{ \
					putfg; \
				} \
				if (--diff == 0) \
					break; \
				if (x1 >= 0) \
				{ \
					++x, pixel += BytesPerPixel; \
					x1 += dy; \
				} \
				if (x1 < 0) \
				{ \
					++y, pixel += pitch; \
					x1 += dx; \
				} \
			} \
		} else \
		{ \
			for (;; ppos++) \
			{ \
				if (!clipped(x, y, cliprect)) \
				{ \
					if ( ( pattern & ( 1 << ( (ppos) & 0xf ) )) != 0 ) \
					{ \
						putfg; \
					} else \
					{ \
						putbg; \
					} \
				} \
				if (--diff == 0) \
					break; \
				if (x1 >= 0) \
				{ \
					++x, pixel += BytesPerPixel; \
					x1 += dy; \
				} \
				if (x1 < 0) \
				{ \
					++y, pixel += pitch; \
					x1 += dx; \
				} \
			} \
		} \
		break; \
	case 3: \
		/* drawing from top right to bottom left */ \
		if (pattern == 0xffff && (logOp == MD_REPLACE || logOp == MD_TRANS)) \
		{ \
			for (;;) \
			{ \
				if (!clipped(x, y, cliprect)) \
				{ \
					putfg; \
				} \
				if (--diff == 0) \
					break; \
				if (x1 >= 0) \
				{ \
					--x, pixel -= BytesPerPixel; \
					x1 += dy; \
				} \
				if (x1 < 0) \
				{ \
					++y, pixel += pitch; \
					x1 += dx; \
				} \
			} \
		} else \
		{ \
			for (;; ppos++) \
			{ \
				if (!clipped(x, y, cliprect)) \
				{ \
					if ( ( pattern & ( 1 << ( (ppos) & 0xf ) )) != 0 ) \
					{ \
						putfg; \
					} else \
					{ \
						putbg; \
					} \
				} \
				if (--diff == 0) \
					break; \
				if (x1 >= 0) \
				{ \
					--x, pixel -= BytesPerPixel; \
					x1 += dy; \
				} \
				if (x1 < 0) \
				{ \
					++y, pixel += pitch; \
					x1 += dx; \
				} \
			} \
		} \
		break; \
	}

void VdiDriver::hsPutPixel(int x, int y, uint32 color)
{
	memptr row_address = mem_mfdb.dest + mem_mfdb.pitch * y;

	switch (mem_mfdb.planes)
	{
	case 1:
		{
		uint8 byte;
		row_address += (x >> 3);
		byte = ReadInt8(row_address);
		x = 7 - (x & 7);
		byte &= ~(1 << x);
		byte |= (color & 1) << x;
		WriteInt8(row_address, byte);
		}
		break;
	case 8:
		row_address += x;
		WriteInt8(row_address, color);
		break;
	case 16:
		row_address += x * 2;
		WriteInt16(row_address, color);
		break;
	case 24:
		row_address += x * 3;
		WriteInt8(row_address, (color >> 16) & 0xff);
		WriteInt8(row_address + 1, (color >> 8) & 0xff);
		WriteInt8(row_address + 2, color & 0xff);
		break;
	case 32:
		row_address += x * 4;
		WriteInt32(row_address, color);
		break;
	}
}

uint32 VdiDriver::hsGetPixel(int x, int y)
{
	memptr row_address;
	uint32 color = 0;

	row_address = mem_mfdb.dest + mem_mfdb.pitch * y;

	switch (mem_mfdb.planes)
	{
	case 1:
		row_address += (x >> 3);
		color = ReadInt8(row_address);
		color >>= (7 - (x & 7));
		color &= 1;
		break;
	case 8:
		row_address += x;
		color = ReadInt8(row_address);
		break;
	case 16:
		row_address += x * 2;
		color = ReadInt16(row_address);
		break;
	case 24:
		row_address += x * 3;
		color = ((uint32)ReadInt8(row_address) << 16) +
			((uint32)ReadInt8(row_address + 1) << 8) +
			(uint32)ReadInt8(row_address + 2);
		break;
	case 32:
		row_address += x * 4;
		color = ReadInt32(row_address);
		break;
	}

	return color;
}

void VdiDriver::hsDrawLine(int x1, int y1, int x2, int y2,
	uint16 pattern, uint32 fgColor, uint32 bgColor, uint16 logOp, int cliprect[])
{
	uint8_t *pixel;
	int pitch;
	int16 x, y;
	int16 dx, dy;
	uint8 ppos;
	int diff;
	int vec;

	/* Test for special cases of straight lines or single point */
	if (x1 == x2)
	{
		if (y1 < y2)
		{
			gfxVLineColor(x1, y1, y2, pattern, fgColor, bgColor, logOp, cliprect);
		} else if (y1 > y2)
		{
			gfxVLineColor(x1, y2, y1, pattern, fgColor, bgColor, logOp, cliprect);
		} else
		{
			if (!clipped(x1, y1, cliprect))
			{
				switch (logOp)
				{
				case MD_REPLACE:
					hsPutPixel(x1, y1, pattern ? fgColor : bgColor );
					break;
				case MD_TRANS:
					if ( pattern )
						hsPutPixel(x1, y1, fgColor );
					break;
				case MD_XOR:
					if ( pattern )
						hsPutPixel(x1, y1, ~hsGetPixel(x1, y1));
					break;
				case MD_ERASE:
					if ( ! pattern )
						hsPutPixel(x1, y1, fgColor);
					break;
				}
			}
		}
		return;
	}
	if (y1 == y2)
	{
		if (x1 < x2)
		{
			gfxHLineColor(x1, x2, y1, pattern, fgColor, bgColor, logOp, cliprect);
		} else if (x1 > x2)
		{
			gfxHLineColor(x2, x1, y1, pattern, fgColor, bgColor, logOp, cliprect);
		} else
		{
			/* x1 == x2; already catched above */
		}
		return;
	}

	D2(bug("CLn %3d,%3d,%3d,%3d", x1, x2, y1, y2));

	/* Variable setup */
	dx = x2 - x1;
	dy = y2 - y1;
	vec = 0;
	if (dx < 0)
	{
		dx = -dx;
		vec |= 1;
	}
	if (dy >= 0)
	{
		vec |= 2;
		dy = -dy;
	}
	diff = -dy;
	if (dx > diff)
		diff = dx;
	diff++;
	ppos = 0;

	/* More variable setup */
	pitch = mem_mfdb.pitch;
	pixel = mem_mfdb.pixels + mem_mfdb.pitch * y1;

	/* Draw */
	x = x1;
	y = y1;
	x1 = (dx + dy) / 2;
	switch (mem_mfdb.planes)
	{
	case 8:
		pixel += x;

		switch (logOp)
		{
		case MD_REPLACE:
			lineloop(*pixel = fgColor, *pixel = bgColor, 1);
			break;
		case MD_TRANS:
			lineloop(*pixel = fgColor, , 1);
			break;
		case MD_XOR:
			lineloop(*pixel = ~(*pixel), , 1);
			break;
		case MD_ERASE:
			lineloop(, *pixel = fgColor, 1);
			break;
		}
		break;
	case 15:
	case 16:
		pixel += 2 * x;

		fgColor = SDL_SwapBE16(fgColor);
		bgColor = SDL_SwapBE16(bgColor);
		switch (logOp)
		{
		case MD_REPLACE:
			lineloop(*(uint16*)pixel = fgColor, *(uint16*)pixel = bgColor, 2);
			break;
		case MD_TRANS:
			lineloop(*(uint16*)pixel = fgColor, , 2);
			break;
		case MD_XOR:
			lineloop(*(uint16*)pixel = ~(*(uint16*)pixel), , 2);
			break;
		case MD_ERASE:
			lineloop(, *(uint16*)pixel = fgColor, 2);
			break;
		}
		break;
	case 24:
		pixel += 3 * x;

		switch (logOp)
		{
		case MD_REPLACE:
			lineloop(putBpp24Pixel(pixel, fgColor), putBpp24Pixel(pixel, bgColor), 3);
			break;
		case MD_TRANS:
			lineloop(putBpp24Pixel(pixel, fgColor), , 3);
			break;
		case MD_XOR:
			lineloop(putBpp24Pixel(pixel, getBpp24Pixel( pixel )), , 3);
			break;
		case MD_ERASE:
			lineloop(, putBpp24Pixel(pixel, fgColor), 3);
			break;
		}
		break;
	case 32:
		pixel += 4 * x;

		fgColor = SDL_SwapBE32(fgColor);
		bgColor = SDL_SwapBE32(bgColor);
		switch (logOp)
		{
		case MD_REPLACE:
			lineloop(*(uint32*)pixel = fgColor, *(uint32*)pixel = bgColor, 4);
			break;
		case MD_TRANS:
			lineloop(*(uint32*)pixel = fgColor, , 4);
			break;
		case MD_XOR:
			lineloop(*(uint32*)pixel = ~(*(uint32*)pixel), , 4);
			break;
		case MD_ERASE:
			lineloop(, *(uint32*)pixel = fgColor, 4);
			break;
		}
		break;
	}
}



void VdiDriver::gfxHLineColor(int x1, int x2, int y, uint16 pattern,
	uint32 fgColor, uint32 bgColor, uint16 logOp, int cliprect[] )
{
	uint8 *pixel;
	uint8 *pixellast;
	int pitch;
	int w;
	uint8 ppos;

	/* Calculate width */
	if (x1 < cliprect[0]) x1 = cliprect[0];
	if (y < cliprect[1]) return;
	if (x2 > cliprect[2]) x2 = cliprect[2];
	if (y > cliprect[3]) return;
	w = x2 - x1 + 1;

	/* Sanity check on width */
	if (w<=0)
		return;

	if (pattern == 0xffff && (logOp == MD_REPLACE || logOp == MD_TRANS))
	{
		/* TODO: optimize solid lines */
	}
	
	/* More variable setup */
	pitch = mem_mfdb.pitch;
	pixel = mem_mfdb.pixels + pitch * y;
	ppos = 0;

	D2(bug("HLn %3d,%3d,%3d", x1, x2, y));

	/* Draw */
	switch(mem_mfdb.planes)
	{
	case 8:
		pixel += x1;
		pixellast = pixel + w;
		switch (logOp)
		{
		case MD_REPLACE:
			for (; pixel<pixellast; pixel += 1, ppos++)
				if (!clipped(x1 + ppos, y, cliprect))
					*(uint8*)pixel = (( ( pattern & ( 1 << ( (ppos) & 0xf ) )) != 0 ) ? fgColor : bgColor);
			break;
		case MD_TRANS:
			for (; pixel<pixellast; pixel += 1, ppos++)
				if (!clipped(x1 + ppos, y, cliprect))
					if ( ( pattern & ( 1 << ( (ppos) & 0xf ) )) != 0 )
						*(uint8*)pixel = fgColor;
			break;
		case MD_XOR:
			for (; pixel<pixellast; pixel += 1, ppos++)
				if (!clipped(x1 + ppos, y, cliprect))
					if ( ( pattern & ( 1 << ( (ppos) & 0xf ) )) != 0 )
						*(uint8*)pixel = ~(*(uint8*)pixel);
			break;
		case MD_ERASE:
			for (; pixel<pixellast; pixel += 1, ppos++)
				if (!clipped(x1 + ppos, y, cliprect))
					if ( ( pattern & ( 1 << ( (ppos) & 0xf ) )) == 0 )
						*(uint8*)pixel = fgColor;
			break;
		}
		break;
	case 15:
	case 16:
		pixel += 2 * x1;
		pixellast = pixel + 2 * w;
		fgColor = SDL_SwapBE16(fgColor);
		bgColor = SDL_SwapBE16(bgColor);
		switch (logOp)
		{
		case MD_REPLACE:
			for (; pixel<pixellast; pixel += 2, ppos++)
				if (!clipped(x1 + ppos, y, cliprect))
					*(uint16*)pixel = (( ( pattern & ( 1 << ( (ppos) & 0xf ) )) != 0 ) ? fgColor : bgColor);
			break;
		case MD_TRANS:
			for (; pixel<pixellast; pixel += 2, ppos++)
				if (!clipped(x1 + ppos, y, cliprect))
					if ( ( pattern & ( 1 << ( (ppos) & 0xf ) )) != 0 )
						*(uint16*)pixel = fgColor;
			break;
		case MD_XOR:
			for (; pixel<pixellast; pixel += 2, ppos++)
				if (!clipped(x1 + ppos, y, cliprect))
					if ( ( pattern & ( 1 << ( (ppos) & 0xf ) )) != 0 )
						*(uint16*)pixel = ~(*(uint16*)pixel);
			break;
		case MD_ERASE:
			for (; pixel<pixellast; pixel += 2, ppos++)
				if (!clipped(x1 + ppos, y, cliprect))
					if ( ( pattern & ( 1 << ( (ppos) & 0xf ) )) == 0 )
						*(uint16*)pixel = fgColor;
			break;
		}
		break;
	case 24:
		pixel += 3 * x1;
		pixellast = pixel + 3 * w;
		switch (logOp)
		{
		case MD_REPLACE:
			for (; pixel<pixellast; pixel += 3, ppos++)
				if (!clipped(x1 + ppos, y, cliprect))
					putBpp24Pixel( pixel, (( ( pattern & ( 1 << ( (ppos) & 0xf ) )) != 0 ) ? fgColor : bgColor ) );
			break;
		case MD_TRANS:
			for (; pixel<pixellast; pixel += 3, ppos++)
				if (!clipped(x1 + ppos, y, cliprect))
					if ( ( pattern & ( 1 << ( (ppos) & 0xf ) )) != 0 )
						putBpp24Pixel( pixel, fgColor );
			break;
		case MD_XOR:
			for (; pixel<pixellast; pixel += 3, ppos++)
				if (!clipped(x1 + ppos, y, cliprect))
					if ( ( pattern & ( 1 << ( (ppos) & 0xf ) )) != 0 )
						putBpp24Pixel( pixel, ~ getBpp24Pixel( pixel ) );
			break;
		case MD_ERASE:
			for (; pixel<pixellast; pixel += 3, ppos++)
				if (!clipped(x1 + ppos, y, cliprect))
					if ( ( pattern & ( 1 << ( (ppos) & 0xf ) )) == 0 )
						putBpp24Pixel( pixel, fgColor );
			break;
		}
		break;
	case 32:
		pixel += 4 * x1;
		pixellast = pixel + 4 * w;
		fgColor = SDL_SwapBE32(fgColor);
		bgColor = SDL_SwapBE32(bgColor);
		switch (logOp)
		{
		case MD_REPLACE:
			for (; pixel<pixellast; pixel += 4, ppos++)
				if (!clipped(x1 + ppos, y, cliprect))
					*(uint32*)pixel = (( ( pattern & ( 1 << ( (ppos) & 0xf ) )) != 0 ) ? fgColor : bgColor);
			break;
		case MD_TRANS:
			for (; pixel<pixellast; pixel += 4, ppos++)
				if (!clipped(x1 + ppos, y, cliprect))
					if ( ( pattern & ( 1 << ( (ppos) & 0xf ) )) != 0 )
						*(uint32*)pixel = fgColor;
			break;
		case MD_XOR:
			for (; pixel<pixellast; pixel += 4, ppos++)
				if (!clipped(x1 + ppos, y, cliprect))
					if ( ( pattern & ( 1 << ( (ppos) & 0xf ) )) != 0 )
						*(uint32*)pixel = ~(*(uint32*)pixel);
			break;
		case MD_ERASE:
			for (; pixel<pixellast; pixel += 4, ppos++)
				if (!clipped(x1 + ppos, y, cliprect))
					if ( ( pattern & ( 1 << ( (ppos) & 0xf ) )) == 0 )
						*(uint32*)pixel = fgColor;
			break;
		}
		break;
	}
}


void VdiDriver::gfxVLineColor(int x, int y1, int y2,
	uint16 pattern, uint32 fgColor, uint32 bgColor, uint16 logOp, int cliprect[] )
{
	uint8 *pixel;
	uint8 *pixellast;
	int pitch;
	int h;
	uint8 ppos;

	/* Calculate height */
	if (x < cliprect[0]) return;
	if (y1 < cliprect[1]) y1 = cliprect[1];
	if (x > cliprect[2]) return;
	if (y2 > cliprect[3]) y2 = cliprect[3];
	h = y2 - y1 + 1;

	/* Sanity check on height */
	if (h <=0 )
		return;

	if (pattern == 0xffff && (logOp == MD_REPLACE || logOp == MD_TRANS))
	{
		/* TODO: optimize solid lines */
	}
	
	D2(bug("VLn %3d,%3d,%3d", x, y1, y2));

	/* More variable setup */
	pitch = mem_mfdb.pitch;
	pixel = mem_mfdb.pixels + pitch * y1;
	ppos = 0;

	/* Draw */
	switch(mem_mfdb.planes)
	{
	case 8:
		pixel += x;
		pixellast = pixel + pitch * h;
		switch (logOp)
		{
		case MD_REPLACE:
			for (; pixel<pixellast; pixel += pitch, ppos++)
				if (!clipped(x, y1 + ppos, cliprect))
					*pixel = (( ( pattern & ( 1 << ( (ppos) & 0xf ) )) != 0 ) ? fgColor : bgColor);
			break;
		case MD_TRANS:
			for (; pixel<pixellast; pixel += pitch, ppos++)
				if (!clipped(x, y1 + ppos, cliprect))
					if ( ( pattern & ( 1 << ( (ppos) & 0xf ) )) != 0 )
						*pixel = fgColor;
			break;
		case MD_XOR:
			for (; pixel<pixellast; pixel += pitch, ppos++)
				if (!clipped(x, y1 + ppos, cliprect))
					if ( ( pattern & ( 1 << ( (ppos) & 0xf ) )) != 0 )
						*pixel = ~(*pixel);
			break;
		case MD_ERASE:
			for (; pixel<pixellast; pixel += pitch, ppos++)
				if (!clipped(x, y1 + ppos, cliprect))
					if ( ( pattern & ( 1 << ( (ppos) & 0xf ) )) == 0 )
						*(uint8*)pixel = fgColor;
			break;
		}
		break;
	case 15:
	case 16:
		pixel += 2 * x;
		pixellast = pixel + pitch * h;
		fgColor = SDL_SwapBE16(fgColor);
		bgColor = SDL_SwapBE16(bgColor);
		switch (logOp)
		{
		case MD_REPLACE:
			for (; pixel<pixellast; pixel += pitch, ppos++)
				if (!clipped(x, y1 + ppos, cliprect))
					*(uint16*)pixel = (( ( pattern & ( 1 << ( (ppos) & 0xf ) )) != 0 ) ? fgColor : bgColor);
			break;
		case MD_TRANS:
			for (; pixel<pixellast; pixel += pitch, ppos++)
				if (!clipped(x, y1 + ppos, cliprect))
					if ( ( pattern & ( 1 << ( (ppos) & 0xf ) )) != 0 )
						*(uint16*)pixel = fgColor;
			break;
		case MD_XOR:
			for (; pixel<pixellast; pixel += pitch, ppos++)
				if (!clipped(x, y1 + ppos, cliprect))
					if ( ( pattern & ( 1 << ( (ppos) & 0xf ) )) != 0 )
						*(uint16*)pixel = ~(*(uint16*)pixel);
			break;
		case MD_ERASE:
			for (; pixel<pixellast; pixel += pitch, ppos++)
				if (!clipped(x, y1 + ppos, cliprect))
					if ( ( pattern & ( 1 << ( (ppos) & 0xf ) )) == 0 )
						*(uint16*)pixel = fgColor;
			break;
		}
		break;
	case 24:
		pixel += 3 * x;
		pixellast = pixel + pitch * h;
		switch (logOp)
		{
		case MD_REPLACE:
			for (; pixel<pixellast; pixel += pitch, ppos++)
				if (!clipped(x, y1 + ppos, cliprect))
					putBpp24Pixel( pixel, (( ( pattern & ( 1 << ( (ppos) & 0xf ) )) != 0 ) ? fgColor : bgColor ) );
			break;
		case MD_TRANS:
			for (; pixel<pixellast; pixel += pitch, ppos++)
				if (!clipped(x, y1 + ppos, cliprect))
					if ( ( pattern & ( 1 << ( (ppos) & 0xf ) )) != 0 )
						putBpp24Pixel( pixel, fgColor );
			break;
		case MD_XOR:
			for (; pixel<pixellast; pixel += pitch, ppos++)
				if (!clipped(x, y1 + ppos, cliprect))
					if ( ( pattern & ( 1 << ( (ppos) & 0xf ) )) != 0 )
						putBpp24Pixel( pixel, ~ getBpp24Pixel( pixel ) );
			break;
		case MD_ERASE:
			for (; pixel<pixellast; pixel += pitch, ppos++)
				if (!clipped(x, y1 + ppos, cliprect))
					if ( ( pattern & ( 1 << ( (ppos) & 0xf ) )) == 0 )
						putBpp24Pixel( pixel, fgColor );
			break;
		}
		break;
	case 32:
		pixel += 4 * x;
		pixellast = pixel + pitch * h;
		fgColor = SDL_SwapBE32(fgColor);
		bgColor = SDL_SwapBE32(bgColor);
		switch (logOp)
		{
		case MD_REPLACE:
			for (; pixel<pixellast; pixel += pitch, ppos++)
				if (!clipped(x, y1 + ppos, cliprect))
					*(uint32*)pixel = (( ( pattern & ( 1 << ( (ppos) & 0xf ) )) != 0 ) ? fgColor : bgColor);
			break;
		case MD_TRANS:
			for (; pixel<pixellast; pixel += pitch, ppos++)
				if (!clipped(x, y1 + ppos, cliprect))
					if ( ( pattern & ( 1 << ( (ppos) & 0xf ) )) != 0 )
						*(uint32*)pixel = fgColor;
			break;
		case MD_XOR:
			for (; pixel<pixellast; pixel += pitch, ppos++)
				if (!clipped(x, y1 + ppos, cliprect))
					if ( ( pattern & ( 1 << ( (ppos) & 0xf ) )) != 0 )
						*(uint32*)pixel = ~(*(uint32*)pixel);
			break;
		case MD_ERASE:
			for (; pixel<pixellast; pixel += pitch, ppos++)
				if (!clipped(x, y1 + ppos, cliprect))
					if ( ( pattern & ( 1 << ( (ppos) & 0xf ) )) == 0 )
						*(uint32*)pixel = fgColor;
			break;
		}
		break;
	}
}


int32 VdiDriver::fillPoly(memptr vwk, memptr points_addr, int n,
	memptr index_addr, int moves, memptr pattern_addr, uint32 fgColor,
	uint32 bgColor, uint32 logOp, uint32 interior_style, memptr clip)
{
	memptr wk = ReadInt32((vwk & -2) + VWK_REAL_ADDRESS); /* vwk->real_address */

	set_mfdb(wk + WK_SCREEN_MFDB);
	if (!mem_mfdb.dest)
	{
		D(bug("VdiDriver::fillPoly(): destination is NULL"));
		return 1;
	}

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
	if (clip)     // Clipping is not off
	{
		cliparray[0] = (int16)ReadInt32(clip);
		cliparray[1] = (int16)ReadInt32(clip + 4);
		cliparray[2] = (int16)ReadInt32(clip + 8);
		cliparray[3] = (int16)ReadInt32(clip + 12);
		D2(bug("fVDI: %s %d,%d:%d,%d", "clipFillTO", cliparray[0], cliparray[1],
		       cliparray[2], cliparray[3]));
	} else
	{
		cliparray[0] = 0;
		cliparray[1] = 0;
		cliparray[2] = ReadInt16(wk + WK_SCREEN_MFDB + MFDB_WIDTH) - 1;
		cliparray[3] = ReadInt16(wk + WK_SCREEN_MFDB + MFDB_HEIGHT) - 1;
	}

	Points p(alloc_point);
	int16* index = alloc_index;
	int16* crossing = alloc_crossing;

	for(int i = 0; i < n; ++i)
	{
		p[i][0] = (int16)ReadInt16(points_addr + i * 4);
		p[i][1] = (int16)ReadInt16(points_addr + i * 4 + 2);
	}
	bool indices = moves;
	for(int i = 0; i < moves; ++i)
		index[i] = (int16)ReadInt16(index_addr + i * 2);

	if (!n)
		return 1;

	if (!indices)
	{
		if ((p[0][0] == p[n - 1][0]) && (p[0][1] == p[n - 1][1]))
			n--;
	} else
	{
		moves--;
		if (index[moves] == -4)
			moves--;
		if (index[moves] == -2)
			moves--;
	}

	int miny = p[0][1];
	int maxy = miny;
	for(int i = 1; i < n; ++i)
	{
		int16 y = p[i][1];
		if (y < miny)
		{
			miny = y;
		}
		if (y > maxy)
		{
			maxy = y;
		}
	}
	if (miny < cliparray[1])
		miny = cliparray[1];
	if (maxy > cliparray[3])
		maxy = cliparray[3];

	for(int16 y = miny; y <= maxy; ++y)
	{
		int ints = 0;
		int16 x1 = 0;   // Make the compiler happy with some initializations
		int16 y1 = 0;
		int16 x2 = 0;
		int16 y2 = 0;
		int move_n = 0;
		int movepnt = 0;
		if (indices)
		{
			move_n = moves;
			movepnt = (index[move_n] + 4) / 2;
			x2 = p[0][0];
			y2 = p[0][1];
		} else
		{
			x1 = p[n - 1][0];
			y1 = p[n - 1][1];
		}

		for(int i = indices; i < n; ++i)
		{
			if (EnoughCrossings(ints + 1) || AllocCrossings(ints + 1))
				crossing = alloc_crossing;
			else
				break;          // At least something will get drawn

			if (indices)
			{
				x1 = x2;
				y1 = y2;
			}
			x2 = p[i][0];
			y2 = p[i][1];
			if (indices)
			{
				if (i == movepnt)
				{
					if (--move_n >= 0)
						movepnt = (index[move_n] + 4) / 2;
					else
						movepnt = -1;           // Never again equal to n
					continue;
				}
			}

			if (y1 < y2)
			{
				if ((y >= y1) && (y < y2))
				{
					crossing[ints++] = SMUL_DIV((y - y1), (x2 - x1), (y2 - y1)) + x1;
				}
			} else if (y1 > y2)
			{
				if ((y >= y2) && (y < y1))
				{
					crossing[ints++] = SMUL_DIV((y - y2), (x1 - x2), (y1 - y2)) + x2;
				}
			}
			if (!indices)
			{
				x1 = x2;
				y1 = y2;
			}
		}

		for(int i = 0; i < ints - 1; ++i)
		{
			for(int j = i + 1; j < ints; ++j)
			{
				if (crossing[i] > crossing[j])
				{
					int16 tmp = crossing[i];
					crossing[i] = crossing[j];
					crossing[j] = tmp;
				}
			}
		}

		x1 = cliparray[0];
		x2 = cliparray[2];
		for(int i = 0; i < ints - 1; i += 2)
		{
			y1 = crossing[i];       // Really x-values, but...
			y2 = crossing[i + 1];
			if (y1 < x1)
				y1 = x1;
			if (y2 > x2)
				y2 = x2;
			if (y1 <= y2)
			{
				hsFillArea(y1, y, y2 - y1 + 1, 1, pattern,
				         fgColor, bgColor, logOp);
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
