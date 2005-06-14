/*
	NatFeat VDI driver, software

	ARAnyM (C) 2001 Standa

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

#include <new>
#include <cstring>

#include "sysdeps.h"
#include "cpu_emulation.h"
#include "parameters.h"
#include "hostscreen.h"
#include "nfvdi.h"
#include "nfvdi_soft.h"

#define DEBUG 0
#include "debug.h"

/*--- Defines ---*/

//#define USE_HOST_MOUSE_CURSOR 1

#define EINVFN -32

static const uint8 vdi_colours[] = { 0,2,3,6,4,7,5,8,9,10,11,14,12,15,13,255 };
static const uint8 tos_colours[] = { 0,255,1,2,4,6,3,5,7,8,9,10,12,14,11,13 };
#define toTosColors( color ) \
    ( (color)<(sizeof(tos_colours)/sizeof(*tos_colours)) ? tos_colours[color] : ((color) == 255 ? 15 : (color)) )
#define toVdiColors( color ) \
    ( (color)<(sizeof(vdi_colours)/sizeof(*vdi_colours)) ? vdi_colours[color] : color)

/*--- Types ---*/

/*--- Variables ---*/

extern HostScreen hostScreen;

/*--- Public functions ---*/

SoftVdiDriver::SoftVdiDriver()
{
	index_count = crossing_count = point_count = 0;
	alloc_index = alloc_crossing = alloc_point = NULL;
}

SoftVdiDriver::~SoftVdiDriver()
{
	delete[] alloc_index;
	delete[] alloc_crossing;
	delete[] alloc_point;
}

void SoftVdiDriver::reset(void)
{
	VdiDriver::reset();
}

/*--- Private functions ---*/

int32 SoftVdiDriver::openWorkstation(void)
{
	return VdiDriver::openWorkstation();
}

int32 SoftVdiDriver::closeWorkstation(void)
{
	return VdiDriver::closeWorkstation();
}

// The polygon code needs some arrays of unknown size
// These routines and members are used so that no unnecessary allocations are done
inline bool SoftVdiDriver::AllocIndices(int n)
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

inline bool SoftVdiDriver::AllocCrossings(int n)
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
			std::memcpy(tmp, alloc_crossing, crossing_count * sizeof(*alloc_crossing));
			delete[] alloc_crossing;
			alloc_crossing = tmp;
			crossing_count = count;
		}
	}

	return crossing_count >= n;
}

inline bool SoftVdiDriver::AllocPoints(int n)
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

// A helper class to make it possible to access
// points in a nicer way in fillPoly.
class Points {
  public:
	explicit Points(int16* vector_) : vector(vector_) { }
	~Points() { }
	int16* operator[](int n) { return &vector[n * 2]; }
  private:
	int16* vector;
};

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

int32 SoftVdiDriver::getPixel(memptr vwk, memptr src, int32 x, int32 y)
{
	DUNUSED(vwk);
	uint32 color = 0;

	if (src)
		return VdiDriver::getPixel(vwk, src, x, y);

	if (!hostScreen.renderBegin())
		return 0;
	color = hostScreen.getPixel((int16)x, (int16)y);
	hostScreen.renderEnd();

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
 
int32 SoftVdiDriver::putPixel(memptr vwk, memptr dst, int32 x, int32 y,
	uint32 color)
{
	if (vwk & 1)
		return 0;

	if (dst)
		return VdiDriver::putPixel(vwk, dst, x, y, color);

	// To screen
	if (!hostScreen.renderBegin())
		return 1;
	hostScreen.putPixel((int16)x, (int16)y, color);
	hostScreen.renderEnd();
	hostScreen.update((int16)x, (int16)y, 1, 1, true);

	D(bug("softvdi: putpixel(x,y,0x%08x)", x,y,color));
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
 *  d2  0 - move shown  1 - move hidden  2 - hide  3 - show  >7 - change shape (pointer to mouse struct)
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

#ifndef USE_HOST_MOUSE_CURSOR
extern "C" {
	static uint16 reverse_bits(uint16 data)
	{
		uint16 res = 0;
		for(uint16 i = 0; i <= 15; i++)
			res |= ((data >> i) & 1) << (15 - i);
		return res;
	}
}

void SoftVdiDriver::restoreMouseBackground(void)
{
	int16 x = Mouse.storage.x;
	int16 y = Mouse.storage.y;

	if (!hostScreen.renderBegin())
		return;

	for(uint16 i = 0; i < Mouse.storage.height; i++)
		for(uint16 j = 0; j < Mouse.storage.width; j++)
			hostScreen.putPixel(x + j, y + i, Mouse.storage.background[i][j]);

	hostScreen.renderEnd();
	hostScreen.update(x, y, Mouse.storage.width, Mouse.storage.height, true);
}

void SoftVdiDriver::saveMouseBackground(int16 x, int16 y, int16 width,
	int16 height)
{
	D2(bug("fVDI: saveMouseBackground: %d,%d,%d,%d", x, y, width, height));

	for(uint16 i = 0; i < height; i++)
		for(uint16 j = 0; j < width; j++) {
			Mouse.storage.background[i][j] = hostScreen.getPixel(x + j, y + i);
		}

	Mouse.storage.x = x;
	Mouse.storage.y = y;
	Mouse.storage.height = height;
	Mouse.storage.width = width;
}
#endif

int SoftVdiDriver::drawMouse(memptr wk, int32 x, int32 y, uint32 mode,
	uint32 data, uint32 hot_x, uint32 hot_y, uint32 fgColor, uint32 bgColor,
	uint32 mouse_type)
{
#ifndef USE_HOST_MOUSE_CURSOR
	DUNUSED(wk);
	DUNUSED(mouse_type);
	D2(bug("fVDI: mouse mode: %x", mode));

	switch (mode) {
	case 4:
	case 0:  // move shown
		restoreMouseBackground();
		break;
	case 5:
	case 1:  // move hidden
		return 1;
	case 2:  // hide
		restoreMouseBackground();
		return 1;
	case 3:  // show
		break;
	case 6:
	case 7:
		break;

	default: // change pointer shape
		D(bug("fVDI: mouse : %08x, %08x, (%08x,%08x)", hot_x, hot_y, fgColor, bgColor));
		memptr fPatterAddress = (memptr)mode;
		for(uint16 i = 0; i < 32; i += 2)
			Mouse.mask[i >> 1] = reverse_bits(ReadInt16(fPatterAddress + i));
		fPatterAddress  = (memptr)data;
		for(uint16 i = 0; i < 32; i += 2)
			Mouse.shape[i >> 1] = reverse_bits(ReadInt16(fPatterAddress + i));

		Mouse.hotspot.x = hot_x & 0xf;
		Mouse.hotspot.y = hot_y & 0xf;
		Mouse.storage.color.foreground = fgColor;
		Mouse.storage.color.background = bgColor;

		return 1;
	}

	// handle the mouse hotspot point
	x -= Mouse.hotspot.x;
	y -= Mouse.hotspot.y;

	// roll the pattern properly
	uint16 mm[16];
	uint16 md[16];
	uint16 shift = x & 0xf;
	for(uint16 i = 0; i <= 15; i++) {
		mm[(i + (y & 0xf)) & 0xf] = (Mouse.mask[i]  << shift) | ((Mouse.mask[i]  >> (16 - shift)) & ((1 << shift) - 1));
		md[(i + (y & 0xf)) & 0xf] = (Mouse.shape[i] << shift) | ((Mouse.shape[i] >> (16 - shift)) & ((1 << shift) - 1));
	}

	// beware of the edges of the screen
	int16 w, h;

	if (x < 0) {
		w = 16 + x;
		x = 0;
	} else {
		w = 16;
		if ((int16)x + 16 >= (int32)hostScreen.getWidth())
			w = hostScreen.getWidth() - (int16)x;
	}
	if (y < 0) {
		h = 16 + y;
		y = 0;
	} else {
		h = 16;
		if ((int16)y + 16 >= (int32)hostScreen.getHeight())
			h = hostScreen.getHeight() - (int16)y;
	}

	D2(bug("fVDI: mouse x,y: %d,%d,%d,%d (%x,%x)", x, y, w, h,
	   Mouse.storage.color.background, Mouse.storage.color.foreground));

	// draw the mouse
	if (!hostScreen.renderBegin())
		return 1;

	saveMouseBackground(x, y, w, h);

	hostScreen.fillArea(x, y, w, h, mm, Mouse.storage.color.background);
	hostScreen.fillArea(x, y, w, h, md, Mouse.storage.color.foreground);

	hostScreen.renderEnd();
	hostScreen.update((uint16)x, (uint16)y, w, h, true);

	return 1;
#else
	return VdiDriver::drawMouse(wk,x,y,mode,data,hot_x,hot_y,fgColor,bgColor,mouse_type);
#endif
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

int32 SoftVdiDriver::expandArea(memptr vwk, memptr src, int32 sx, int32 sy,
	memptr dest, int32 dx, int32 dy, int32 w, int32 h, uint32 logOp,
	uint32 fgColor, uint32 bgColor)
{
	DUNUSED(vwk);
	if (hostScreen.getBpp() <= 1) {
		fgColor &= 0xff;
		bgColor &= 0xff;
	}

	uint16 pitch = ReadInt16(src + MFDB_WDWIDTH) * 2; // the byte width (always monochrom);
	memptr data  = ReadInt32(src + MFDB_ADDRESS) + sy * pitch; // MFDB *src->address;

	D(bug("fVDI: %s %x %d,%d:%d,%d:%d,%d (%lx, %lx)", "expandArea", logOp, sx, sy, dx, dy, w, h, fgColor, bgColor ));
	D2(bug("fVDI: %s %x,%x : %x,%x", "expandArea - MFDB addresses", src, dest, ReadInt32( src ),ReadInt32( dest )));
	D2(bug("fVDI: %s %x, %d, %d", "expandArea - src: data address, MFDB wdwidth << 1, bitplanes", data, pitch, ReadInt16( src + MFDB_NPLANES )));
	D2(bug("fVDI: %s %x, %d, %d", "expandArea - dst: data address, MFDB wdwidth << 1, bitplanes", ReadInt32(dest), ReadInt16(dest + MFDB_WDWIDTH) * (ReadInt16(dest + MFDB_NPLANES) >> 2), ReadInt16(dest + MFDB_NPLANES)));

	if (dest) {
		return VdiDriver::expandArea(vwk, src, sx, sy, dest, dx, dy, w, h, logOp, fgColor, bgColor);
	}

	if (!hostScreen.renderBegin())
		return 1;

	D(bug("fVDI: expandArea M->S"));
	for(uint16 j = 0; j < h; j++) {
		D2(fprintf(stderr, "fVDI: bmp:"));

		uint16 theWord = ReadInt16(data + j * pitch + ((sx >> 3) & 0xfffe));
		for(uint16 i = sx; i < sx + w; i++) {
			if (i % 16 == 0)
				theWord = ReadInt16(data + j * pitch + ((i >> 3) & 0xfffe));

			D2(fprintf(stderr, "%s", ((theWord >> (15 - (i & 0xf))) & 1) ? "1" : " "));
			switch(logOp) {
				case 1:
					hostScreen.putPixel(dx + i - sx, dy + j, ((theWord >> (15 - (i & 0xf))) & 1) ? fgColor : bgColor);
					break;
				case 2:
					if ((theWord >> (15 - (i & 0xf))) & 1)
						hostScreen.putPixel(dx + i - sx, dy + j, fgColor);
					break;
				case 3:
					if ((theWord >> (15 - (i & 0xf))) & 1)
						hostScreen.putPixel(dx + i - sx, dy + j, ~hostScreen.getPixel(dx + i - sx, dy + j));
					break;
				case 4:
					if (!((theWord >> (15 - (i & 0xf))) & 1))
						hostScreen.putPixel(dx + i - sx, dy + j, bgColor);
					break;
			}
		}
		D2(bug("")); //newline
	}

	hostScreen.renderEnd();
	hostScreen.update(dx, dy, w, h, true);

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

int32 SoftVdiDriver::fillArea(memptr vwk, uint32 x_, uint32 y_, int32 w,
	int32 h, memptr pattern_addr, uint32 fgColor, uint32 bgColor,
	uint32 logOp, uint32 interior_style)
{
	DUNUSED(interior_style);
	if (hostScreen.getBpp() <= 1) {
		fgColor &= 0xff;
		bgColor &= 0xff;
	}

	uint16 pattern[16];
	for(int i = 0; i < 16; ++i)
		pattern[i] = ReadInt16(pattern_addr + i * 2);

	memptr table = 0;

	int x = x_;
	int y = y_;

	if ((long)vwk & 1) {
		if ((y_ & 0xffff) != 0)
			return -1;		// Don't know about this kind of table operation
		table = (memptr)x_;
		h = (y_ >> 16) & 0xffff;
		vwk -= 1;
	}

	D(bug("fVDI: %s %d %d,%d:%d,%d : %d,%d p:%x, (fgc:%lx : bgc:%lx)", "fillArea",
	      logOp, x, y, w, h, x + w - 1, x + h - 1, *pattern,
	      fgColor, bgColor));

	if (!hostScreen.renderBegin())
		return 1;

	int minx = 1000000;
	int miny = 1000000;
	int maxx = -1000000;
	int maxy = -1000000;

	/* Perform rectangle fill. */
	if (!table) {
		hostScreen.fillArea(x, y, w, h, pattern, fgColor, bgColor, logOp);
		minx = x;
		miny = y;
		maxx = x + w - 1;
		maxy = y + h - 1;
	} else {
		for(h = h - 1; h >= 0; h--) {
			y = (int16)ReadInt16(table); table+=2;
			x = (int16)ReadInt16(table); table+=2;
			w = (int16)ReadInt16(table) - x + 1; table+=2;
			hostScreen.fillArea(x, y, w, 1, pattern, fgColor, bgColor, logOp);
			if (x < minx)
				minx = x;
			if (y < miny)
				miny = y;
			if (x + w - 1 > maxx)
				maxx = x + w - 1;
			if (y > maxy)
				maxy = y;
		}
	}

	hostScreen.renderEnd();
	hostScreen.update(minx, miny, maxx - minx + 1, maxy - miny + 1, true);

	return 1;
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

int32 SoftVdiDriver::blitArea_M2S(memptr vwk, memptr src, int32 sx, int32 sy,
	memptr dest, int32 dx, int32 dy, int32 w, int32 h, uint32 logOp)
{
	DUNUSED(vwk);

	if (!hostScreen.renderBegin())
		return 1;

	uint32 planes = ReadInt16(src + MFDB_NPLANES);			// MFDB *src->bitplanes
	uint32 pitch  = ReadInt16(src + MFDB_WDWIDTH) * planes * 2;	// MFDB *src->pitch
	memptr data   = ReadInt32(src) + sy * pitch;			// MFDB *src->address host OS address

	uint32 srcData;
	uint32 destData;

	switch(planes) {
		case 16:
			if (logOp != 3) {
				for(int32 j = 0; j < h; j++) {
					for(int32 i = sx; i < sx + w; i++) {
						srcData = ReadInt16(data + j * pitch + i * 2);
						destData = hostScreen.getPixel(dx + i - sx, dy + j);
						destData = applyBlitLogOperation(logOp, destData, srcData);
						hostScreen.putPixel(dx + i - sx, dy + j, destData);
					}
				}
			} else {
				uint16* daddr_base = (uint16*)((long)hostScreen.getVideoramAddress() +
				                          dy * hostScreen.getPitch() + dx * 2);
				memptr saddr_base = data + sx * 2;
				for(int32 j = 0; j < h; j++) {
					uint16* daddr = daddr_base;
					memptr saddr = saddr_base;
					daddr_base += hostScreen.getPitch() / 2;
					saddr_base += pitch;
					for(int32 i = 0; i < w; i++) {
						destData = ReadInt16(saddr);
						saddr += 2;
						*daddr++ = destData;
					}
				}
			}
			break;
		case 24:
			for(int32 j = 0; j < h; j++)
				for(int32 i = sx; i < sx + w; i++) {
					srcData = get_dtriplet(data + j * pitch + i * 3);
					destData = hostScreen.getPixel(dx + i - sx, dy + j);
					destData = applyBlitLogOperation(logOp, destData, srcData);
					hostScreen.putPixel(dx + i - sx, dy + j, destData);
				}
			break;
		case 32:
			if (logOp != 3) {
				for(int32 j = 0; j < h; j++)
					for(int32 i = sx; i < sx + w; i++) {
						srcData = ReadInt32(data + j * pitch + i * 4);
						destData = hostScreen.getPixel(dx + i - sx, dy + j);
						destData = applyBlitLogOperation(logOp, destData, srcData);
						hostScreen.putPixel(dx + i - sx, dy + j, destData);
					}
			} else {
				uint32* daddr_base = (uint32*)((long)hostScreen.getVideoramAddress() +
				                          dy * hostScreen.getPitch() + dx * 4);
				memptr saddr_base = data + sx * 4;
				for(int32 j = 0; j < h; j++) {
					uint32* daddr = daddr_base;
					memptr saddr = saddr_base;
					daddr_base += hostScreen.getPitch() / 4;
					saddr_base += pitch;
					for(int32 i = 0; i < w; i++) {
						destData = ReadInt32(saddr);
						saddr += 4;
						*daddr++ = destData;
					}
				}
			}
			break;

		default: // bitplane modes...
			if (planes < 16) {
				uint8 color[16];

				D(bug("fVDI: blitArea M->S: bitplaneToCunky conversion"));
				uint16 *dataHost = (uint16*)Atari2HostAddr(data);
				// FIXME: Hack! Should use the get_X() methods above

				for(int32 j = 0; j < h; j++) {
					uint32 wordIndex = (j * pitch >> 1) + (sx >> 4) * planes;
					hostScreen.bitplaneToChunky(&dataHost[wordIndex], planes, color);

					for(int32 i = sx; i < sx + w; i++) {
						uint8 bitNo = i & 0xf;
						if (bitNo == 0) {
							uint32 wordIndex = (j * pitch >> 1) + (i >> 4) * planes;
							hostScreen.bitplaneToChunky(&dataHost[wordIndex], planes, color);
						}

						destData = hostScreen.getPixel(dx + i - sx, dy + j);
						destData = applyBlitLogOperation(logOp, destData, color[bitNo]);
						hostScreen.putPixel(dx + i - sx, dy + j, destData);
					}
				}
			}
	}

	hostScreen.renderEnd();
	hostScreen.update(dx, dy, w, h, true);

	return 1;
}

int32 SoftVdiDriver::blitArea_S2M(memptr vwk, memptr src, int32 sx, int32 sy,
	memptr dest, int32 dx, int32 dy, int32 w, int32 h, uint32 logOp)
{
	DUNUSED(vwk);

	if (!hostScreen.renderBegin())
		return 1;

	uint32 planes = ReadInt16(dest + MFDB_NPLANES);			// MFDB *dest->bitplanes
	uint32 destPitch = ReadInt16(dest + MFDB_WDWIDTH) * planes * 2;	// MFDB *dest->pitch
	memptr destAddress = ReadInt32(dest);

	uint32 srcData;
	uint32 destData;

	switch(planes) {
		case 16:
			for(int32 j = 0; j < h; j++)
				for(int32 i = sx; i < sx + w; i++) {
					uint32 offset = (dx + i - sx) * 2 + (dy + j) * destPitch;
					srcData = hostScreen.getPixel(i, sy + j);
					destData = ReadInt16(destAddress + offset);
					destData = applyBlitLogOperation(logOp, destData, srcData);
					WriteInt16(destAddress + offset, destData);
				}
			break;
		case 24:
			for(int32 j = 0; j < h; j++)
				for(int32 i = sx; i < sx + w; i++) {
					uint32 offset = (dx + i - sx) * 3 + (dy + j) * destPitch;
					srcData = hostScreen.getPixel( i, sy + j );
					destData = get_dtriplet(destAddress + offset);
					destData = applyBlitLogOperation(logOp, destData, srcData);
					put_dtriplet(destAddress + offset, destData);
				}
			break;
		case 32:
			for(int32 j = 0; j < h; j++)
				for(int32 i = sx; i < sx + w; i++) {
					uint32 offset = (dx + i - sx) * 4 + (dy + j) * destPitch;
					srcData = hostScreen.getPixel(i, sy + j);
					destData = ReadInt32(destAddress + offset);
					destData = applyBlitLogOperation(logOp, destData, srcData);
					WriteInt32(destAddress + offset, destData);
				}
			break;

		default:
			if (planes < 16) {
				D(bug("fVDI: blitArea S->M: bitplane conversion"));

				uint16 bitplanePixels[8];
				uint32 pitch = hostScreen.getPitch();
				uint8* dataHost = (uint8*)hostScreen.getVideoramAddress() + sy * pitch;

				for(int32 j = 0; j < h; j++) {
					uint32 pixelPosition = j * pitch + sx & ~0xf; // div 16
					chunkyToBitplane(dataHost + pixelPosition, planes, bitplanePixels);
					for(uint32 d = 0; d < planes; d++)
						WriteInt16(destAddress + (((dx >> 4) * planes) + d) * 2 + (dy + j) * destPitch, bitplanePixels[d]);

					for(int32 i = sx; i < sx + w; i++) {
						uint8 bitNo = i & 0xf;
						if (bitNo == 0) {
							uint32 wordIndex = ((dx + i - sx) >> 4) * planes;
							uint32 pixelPosition = j * pitch + i & ~0xf; // div 16
							chunkyToBitplane(dataHost + pixelPosition, planes, bitplanePixels);
							for(uint32 d = 0; d < planes; d++)
								WriteInt16(destAddress + (wordIndex + d) * 2 + (dy + j) * destPitch, bitplanePixels[d]);
						}
					}
				}
			}
	}

	hostScreen.renderEnd();
	return 1;
}

int32 SoftVdiDriver::blitArea_S2S(memptr vwk, memptr src, int32 sx, int32 sy,
	memptr dest, int32 dx, int32 dy, int32 w, int32 h, uint32 logOp)
{
	DUNUSED(vwk);

	if (logOp == 3) {
	        // for S->S blits... -> SDL does the whole thing at once

		// if (!hostScreen.renderBegin()) // the surface must _not_ be locked for blitArea (SDL_BlitSurface)
		hostScreen.blitArea(sx, sy, dx, dy, w, h);
	} else {
		if (!hostScreen.renderBegin())
			return 1;

		uint32 srcData;
		uint32 destData;

		for(int32 j = 0; j < h; j++)
			for(int32 i = 0; i < w; i++) {
				srcData = hostScreen.getPixel(i + sx, sy + j);
				destData = hostScreen.getPixel(i + dx, dy + j);
				destData = applyBlitLogOperation(logOp, destData, srcData);
				hostScreen.putPixel(dx + i, dy + j, destData);
			}

		hostScreen.renderEnd();
	}

	hostScreen.update(sx, sy, w, h, true);
	hostScreen.update(dx, dy, w, h, true);

	return 1;
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


/* --------- Clipping routines for box/line */

/* Clipping based heavily on code from                       */
/* http://www.ncsa.uiuc.edu/Vis/Graphics/src/clipCohSuth.c   */

#define CLIP_LEFT_EDGE   0x1
#define CLIP_RIGHT_EDGE  0x2
#define CLIP_BOTTOM_EDGE 0x4
#define CLIP_TOP_EDGE    0x8
#define CLIP_INSIDE(a)   (!a)
#define CLIP_REJECT(a,b) (a&b)
#define CLIP_ACCEPT(a,b) (!(a|b))


static inline int clipEncode (int x, int y, int left, int top, int right, int bottom)
{
	int code = 0;
	if (x < left) {
		code |= CLIP_LEFT_EDGE;
	} else if (x > right) {
		code |= CLIP_RIGHT_EDGE;
	}
	if (y < top) {
		code |= CLIP_TOP_EDGE;
	} else if (y > bottom) {
		code |= CLIP_BOTTOM_EDGE;
	}
	return code;
}


static inline bool clipLine(int& x1, int& y1, int& x2, int& y2, int cliprect[])
{
#if OLD_CODE // check what is bad!
	if (!cliprect) {
		return true; // Clipping is off
	// Get clipping boundary
 	int left   = cliprect[0];
 	int top    = cliprect[1];
 	int right  = cliprect[2];
 	int bottom = cliprect[3];
#else
	// Get clipping boundary
	int left, top, right, bottom;

	if (!cliprect) {
		left   = 0;
		top    = 0;
		right  = hostScreen.getWidth() -1;
		bottom = hostScreen.getHeight()-1;
	} else {
		left   = cliprect[0];
		top    = cliprect[1];
		right  = cliprect[2];
		bottom = cliprect[3];
	}
#endif

	bool draw = false;
	while (1) {
		int code1 = clipEncode(x1, y1, left, top, right, bottom);
		int code2 = clipEncode(x2, y2, left, top, right, bottom);
		if (CLIP_ACCEPT(code1, code2)) {
			draw = true;
			break;
		} else if (CLIP_REJECT(code1, code2)) {
			break;
		} else {
			if (CLIP_INSIDE(code1)) {
				int swaptmp = x2; x2 = x1; x1 = swaptmp;
				swaptmp = y2; y2 = y1; y1 = swaptmp;
				swaptmp = code2; code2 = code1; code1 = swaptmp;
			}
			float m = 1.0f;
			if (x2 != x1) {
				m = (y2 - y1) / (float)(x2 - x1);
			}
			if (code1 & CLIP_LEFT_EDGE) {
				y1 += (int)((left - x1) * m);
				x1 = left;
			} else if (code1 & CLIP_RIGHT_EDGE) {
				y1 += (int)((right - x1) * m);
				x1 = right;
			} else if (code1 & CLIP_BOTTOM_EDGE) {
				if (x2 != x1) {
					x1 += (int)((bottom - y1) / m);
				}
				y1 = bottom;
			} else if (code1 & CLIP_TOP_EDGE) {
				if (x2 != x1) {
					x1 += (int)((top - y1) / m);
				}
				y1 = top;
			}
		}
	}

	D2(bug("fVDI: %s %d,%d:%d,%d", "clipLineEND", x1, y1, x2, y2));

	return draw;
}


// Don't forget rotation of pattern!
int SoftVdiDriver::drawSingleLine(int x1, int y1, int x2, int y2, uint16 pattern,
                               uint32 fgColor, uint32 bgColor, int logOp, bool last_pixel,
                               int cliprect[], int minmax[])
{
	if (clipLine(x1, y1, x2, y2, cliprect)) {	// Do not draw the line when it is out
		D(bug("fVDI: %s %d,%d:%d,%d (%lx,%lx)", "drawSingleLine", x1, y1, x2, y2, fgColor, bgColor));
		hostScreen.drawLine(x1, y1, x2, y2, pattern, fgColor, bgColor, logOp, last_pixel);
		if (x1 < x2) {
			if (x1 < minmax[0])
				minmax[0] = x1;
			if (x2 > minmax[2])
				minmax[2] = x2;
		} else {
			if (x2 < minmax[0])
				minmax[0] = x2;
			if (x1 > minmax[2])
				minmax[2] = x1;
		}
		if (y1 < y2) {
			if (y1 < minmax[1])
				minmax[1] = y1;
			if (y2 > minmax[3])
				minmax[3] = y2;
		} else {
			if (y2 < minmax[1])
				minmax[1] = y2;
			if (y1 > minmax[3])
				minmax[3] = y1;
		}
	}
	return 1;
}


// Don't forget rotation of pattern!
int SoftVdiDriver::drawTableLine(memptr table, int length, uint16 pattern,
                              uint32 fgColor, uint32 bgColor, int logOp,
                              int cliprect[], int minmax[])
{
	int x1 = (int16)ReadInt16(table); table+=2;
	int y1 = (int16)ReadInt16(table); table+=2;
	for(--length; length > 0; length--) {
		int x2 = (int16)ReadInt16(table); table+=2;
		int y2 = (int16)ReadInt16(table); table+=2;

		drawSingleLine(x1, y1, x2, y2, pattern, fgColor, bgColor,
		               logOp, length == 1, cliprect, minmax);
		x1 = x2;
		y1 = y2;
	}

	return 1;
}


// Don't forget rotation of pattern!
int SoftVdiDriver::drawMoveLine(memptr table, int length, memptr index, int moves, uint16 pattern,
                             uint32 fgColor, uint32 bgColor, int logOp,
                             int cliprect[], int minmax[])
{
	int x1 = (int16)ReadInt16(table); table+=2;
	int y1 = (int16)ReadInt16(table); table+=2;
	moves-=2;
	if ((int16)ReadInt16(index + moves) == -4)
		moves-=2;
	if ((int16)ReadInt16(index + moves) == -2)
		moves-=2;
	int movepnt = -1;
	if (moves >= 0)
		movepnt = ((int16)ReadInt16(index + moves) + 4) / 2;
	for(int n = 1; n < length; n++) {
		int x2 = (int16)ReadInt16(table); table+=2;
		int y2 = (int16)ReadInt16(table); table+=2;
		if (n == movepnt) {
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
		               logOp, length == 1, cliprect, minmax);

		x1 = x2;
		y1 = y2;
	}

	return 1;
}

int32 SoftVdiDriver::drawLine(memptr vwk, uint32 x1_, uint32 y1_, uint32 x2_,
	uint32 y2_, uint32 pattern, uint32 fgColor, uint32 bgColor,
	uint32 logOp, memptr clip)
{
	if (hostScreen.getBpp() <= 1) {
		fgColor &= 0xff;
		bgColor &= 0xff;
	}

	memptr table = 0;
	memptr index = 0;
	int length = 0;
	int moves = 0;

	int16 x1 = x1_;
	int16 y1 = y1_;
	int16 x2 = x2_;
	int16 y2 = y2_;

	if (vwk & 1) {
		if ((unsigned)(y1 & 0xffff) > 1)
			return -1;		/* Don't know about this kind of table operation */
		table = (memptr)x1_;
		length = (y1_ >> 16) & 0xffff;
		if ((y1_ & 0xffff) == 1) {
			index = (memptr)y2_;
			moves = x2_ & 0xffff;
		}
		vwk -= 1;
		x1 = (int16)ReadInt16(table);
		y1 = (int16)ReadInt16(table + 2);
		x2 = (int16)ReadInt16(table + 4);
		y2 = (int16)ReadInt16(table + 6);
	}

	int cliparray[4];
	int* cliprect = 0;
	if (clip) {				// Clipping is not off
		cliprect = cliparray;
		cliprect[0] = (int16)ReadInt32(clip);
		cliprect[1] = (int16)ReadInt32(clip + 4);
		cliprect[2] = (int16)ReadInt32(clip + 8);
		cliprect[3] = (int16)ReadInt32(clip + 12);
		D2(bug("fVDI: %s %d,%d:%d,%d", "clipLineTO", cliprect[0], cliprect[1],
		       cliprect[2], cliprect[3]));
	}

	int minmax[4] = {1000000, 1000000, -1000000, -1000000};

#if TEST_STRAIGHT	// Not yet working
	int eq_coord = (x1 == x2) + 2 * (y1 == y2);
#endif

	if (table) {
		if (moves)
			drawMoveLine(table, length, index, moves, pattern, fgColor, bgColor,
			             logOp, cliprect, minmax);
		else {
#if TEST_STRAIGHT	// Not yet working
			if (eq_coord && ((pattern & 0xffff) == 0xffff) && (logOp < 3)) {
				table += 8;
				for(--length; length > 0; length--) {
					if (eq_coord & 1) {
						if (y1 < y2)
							vertical(x1, y1, 1, y2 - y1 + 1);
						else
							vertical(x2, y2, 1, y1 - y2 + 1);
					} else if (eq_coord & 2) {
						if (x1 < x2)
							horizontal(x1, y1, x2 - x1 + 1, 1);
						else
							horizontal(x2, y2, x1 - x2 + 1, 1);
					} else {
						length++;
						table -= 8;
						break;
					}
					x1 = x2;
					y1 = y2;
					x2 = (int16)ReadInt16(table); table+=2;
					y2 = (int16)ReadInt16(table); table+=2;
					eq_coord = (x1 == x2) + 2 * (y1 == y2);
				}
			}
#endif
			switch (length) {
			case 0:
				break;
			case 1:
				drawSingleLine(x1, y1, x2, y2, pattern, fgColor, bgColor,
				               logOp, true, cliprect, minmax);
				break;
			default:
				drawTableLine(table, length, pattern, fgColor, bgColor,
				              logOp, cliprect, minmax);
 				break;
			}
 		}
#if TEST_STRAIGHT	// Not yet working
	} else if (eq_coord && ((pattern & 0xffff) == 0xffff)) {
		if (eq_coord & 1) {
			if (y1 < y2)
				vertical(x1, y1, 1, y2 - y1 + 1);
			else
				vertical(x2, y2, 1, y1 - y2 + 1);
		} else {
			if (x1 < x2)
				horizontal(x1, y1, x2 - x1 + 1, 1);
			else
				horizontal(x2, y2, x1 - x2 + 1, 1);
		}
#endif
	} else
		drawSingleLine(x1, y1, x2, y2, pattern, fgColor, bgColor,
		               logOp, true, cliprect, minmax);

	if (minmax[0] != 1000000) {
		D(bug("fVDI: %s %d,%d:%d,%d", "drawLineUp",
		       minmax[0], minmax[1], minmax[2], minmax[3]));
		hostScreen.update(minmax[0], minmax[1],
		                  minmax[2] - minmax[0] + 1, minmax[3] - minmax[1] + 1, true);
	} else {
		D2(bug("fVDI: drawLineUp nothing to redraw"));
	}

	return 1;
}

// This is supposed to be a fast 16x16/16 with 32 bit intermediate result
#define SMUL_DIV(x,y,z)	((short)(((x)*(long)(y))/(z)))
// Some other possible variants are
//#define SMUL_DIV(x,y,z)	((long)(y - y1) * (x2 - x1) / (y2 - y1))
//#define SMUL_DIV(x,y,z)	((short)(((short)(x)*(long)((short)(y)))/(short)(z)))

int32 SoftVdiDriver::fillPoly(memptr vwk, memptr points_addr, int n,
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

	if (!hostScreen.renderBegin())
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
			if (AllocCrossings(ints + 1))
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
				hostScreen.fillArea(y1, y, y2 - y1 + 1, 1, pattern,
				                    fgColor, bgColor, logOp);
				if (y1 < minx)
					minx = y1;
				if (y2 > maxx)
					maxx = y2;
			}
		}
	}

	hostScreen.renderEnd();
	if (minx != 1000000)
		hostScreen.update(minx, miny, maxx - minx + 1, maxy - miny + 1, true);

	return 1;
}

void SoftVdiDriver::getHwColor(uint16 index, uint32 red, uint32 green,
	uint32 blue, memptr hw_value)
{
	if (hostScreen.getBpp() != 2)
		// <16bit, 24 and 32bit graphics
		WriteInt32( hw_value,
			    hostScreen.getColor((red * ((1L << 8) - 1) + 500L) / 1000,
		                                (green * ((1L << 8) - 1) + 500L) / 1000,
		                                (blue * ((1L << 8) - 1) + 500L) / 1000) );
	else
		// 5+6+5 rounding needed
		WriteInt32( hw_value,
			    hostScreen.getColor(((red * ((1L << 5) - 1) + 500L) / 1000) << 3,
		                       		((green * ((1L << 6) - 1) + 500L) / 1000) << 2,
		                           	((blue * ((1L << 5) - 1) + 500L) / 1000) << 3) );

	if (hostScreen.getBpp() <= 1) {
		WriteInt32( hw_value, index );
	}

	D(bug("fVDI: getHwColor (%03d) %04d,%04d,%04d - %lx", index, red, green, blue, ReadInt32( hw_value )));
}

/**
 * Set palette colour (hooked into the c_set_colors driver function by STanda)
 *
 * set_color_hook
 *  4(a7)   paletteIndex
 *  8(a7)   red component byte value
 *  10(a7)  green component byte value
 *  12(a7)  blue component byte value
 **/
 
void SoftVdiDriver::setColor(memptr vwk, uint32 paletteIndex, uint32 red,
	uint32 green, uint32 blue)
{
	DUNUSED(vwk);
	D(bug("fVDI: setColor [NULL]: %03d (%03d) - %04d,%04d,%04d - %02x,%02x,%02x", paletteIndex, toTosColors(paletteIndex) & 0xff, red, green, blue, (uint8)((red * ((1L << 8) - 1) + 500L) / 1000), (uint8)((green * ((1L << 8) - 1) + 500L) / 1000), (uint8)((blue * ((1L << 8) - 1) + 500L) / 1000) ));

	if (hostScreen.getBpp() != 2)
		// <16bit, 24 and 32bit graphics
		hostScreen.setPaletteColor(toTosColors(paletteIndex),
		                           (red * ((1L << 8) - 1) + 500L) / 1000,
		                           (green * ((1L << 8) - 1) + 500L) / 1000,
		                           (blue * ((1L << 8) - 1) + 500L) / 1000);
	else
		// 5+6+5 rounding needed
		hostScreen.setPaletteColor(toTosColors(paletteIndex),
		                           ((red * ((1L << 5) - 1) + 500L) / 1000) << 3,
		                           ((green * ((1L << 6) - 1) + 500L) / 1000) << 2,
		                           ((blue * ((1L << 5) - 1) + 500L) / 1000) << 3);

	hostScreen.updatePalette( 256 );
}

int32 SoftVdiDriver::getFbAddr(void)
{
	int32 result;

#ifdef FIXED_VIDEORAM
	result = ARANYMVRAMSTART;
#else
	result = VideoRAMBase;
#endif
	D(bug("fVDI: getVideoramAddress: %#lx", result));
	return result;
}

/*
 * $Log$
 * Revision 1.9  2005/06/07 21:35:12  johan
 * support for forced mouse positioning
 *
 * Revision 1.8  2005/06/05 21:47:46  pmandin
 * OpenGL mouse rendering
 *
 * Revision 1.7  2005/05/27 11:57:03  pmandin
 * Revert back to software mouse driver for nfvdi
 *
 * Revision 1.6  2005/05/16 09:50:37  pmandin
 * Enable OpenGL rendering when opening workstation
 *
 * Revision 1.4  2005/05/12 17:11:06  pmandin
 * Split blitArea function in 4 parts
 *
 * Revision 1.3  2005/05/12 11:30:03  pmandin
 * Bugfixes
 *
 * Revision 1.2  2005/05/09 12:21:10  pmandin
 * Move driver independent functions to nfvdi.cpp
 *
 * Revision 1.1  2005/05/07 09:36:23  pmandin
 * Split NF vdi driver in 2 parts
 *
 * Revision 1.63  2005/01/24 18:22:53  standa
 * openwk() and closewk() added to the NF api -> boot bug fixed.
 *
 * Large cleanup in the m68k part.
 *
 * Revision 1.62  2005/01/12 14:35:16  joy
 * more unused variables hidden
 *
 * Revision 1.61  2005/01/12 10:56:02  joy
 * compiler warnings about unused variables fixed
 *
 * Revision 1.60  2005/01/11 20:32:13  standa
 * One more bugfix regarding the pointers and 64bit clean.
 *
 * Revision 1.59  2005/01/11 19:02:52  standa
 * 64bit cleanup revisited.
 *
 * Revision 1.58  2005/01/11 15:28:14  standa
 * 64bit cleanup
 *
 * Revision 1.57  2004/10/31 23:17:09  pmandin
 * Forgot some break instructions
 *
 * Revision 1.56  2004/09/29 20:45:57  xavier
 * Fixed typo.
 *
 * Revision 1.55  2004/09/24 13:50:14  pmandin
 * FVDI: add functions to read screen size really set by the host
 *
 * Revision 1.54  2004/06/07 08:39:23  standa
 * Code beautyfying. Some remarks removed. Some debugs added.
 *
 * Revision 1.53  2004/01/05 10:05:19  standa
 * Palette handling reworked. Old non-NF dispatch removed.
 *
 * Revision 1.52  2003/11/25 22:56:49  joy
 * part of a major hardware dispatcher rewrite
 *
 * Revision 1.51  2003/04/22 21:22:14  johan
 * Reverse transparent mode for vrt_cpyfm corrected.
 *
 * Revision 1.50  2003/02/19 20:02:35  standa
 * Small bugfix in nonbitplane modes.
 *
 * Revision 1.49  2003/02/19 19:39:38  standa
 * SDL surface is now in TOS colors internally for bitplane modes. This
 * allows much simpler blits and expands.
 *
 * Revision 1.48  2003/02/19 09:02:56  standa
 * The bitplane modes expandArea fix.
 *
 * Revision 1.47  2003/02/18 22:05:07  standa
 * The bitplane fVDI modes improved.
 *
 * Revision 1.46  2003/01/15 08:35:08  standa
 * The drawMouse hotspot coordinates are not stripped to 4bits (0..15).
 *
 * Revision 1.45  2003/01/14 20:49:16  standa
 * Natfeat dispatch for fVDI::drawMouse() fixed.
 *
 * Revision 1.44  2002/12/14 04:58:03  johan
 * Fast 32 bit vro_cpyfm D=S mode.
 * Screen->screen vro_cpyfm modes.
 *
 * Revision 1.43  2002/10/21 22:50:08  johan
 * NatFeat support added.
 *
 * Revision 1.42  2002/10/15 21:26:52  milan
 * non-cheaders support (for MipsPro C/C++ compiler)
 *
 * Revision 1.41  2002/09/28 00:07:37  johan
 * Special handling of vro_cpyfm D=S mode. 16 bit only.
 *
 * Revision 1.40  2002/09/15 15:17:15  joy
 * CPU to separate thread
 *
 * Revision 1.39  2002/08/03 12:36:42  johan
 * Updated to work with new API (dependencies on internal fVDI structures
 * have been removed and all parameters are passed on the stack).
 * Some internal fixes and cleanups.
 *
 * Revision 1.38  2002/06/24 17:08:48  standa
 * The pointer arithmetics fixed. The memptr usage introduced in my code.
 *
 * Revision 1.37  2002/04/22 18:30:50  milan
 * header files reform
 *
 * Revision 1.36  2002/03/27 20:01:47  standa
 * fVDI work also without the --fixedvideoram option. There is a new
 * getVideoramAddress native function. This might be reqested to be removed
 * in the future due to its missuse. The solution would be to make a
 * very special fVDI structure aware function that would set the address
 * to the required places.
 *
 * Revision 1.35  2002/02/19 20:04:05  milan
 * src/ <-> CPU interaction cleaned
 * memory access cleaned
 *
 * Revision 1.1.1.1  2002/02/10 23:32:50  jurikm
 * Unofficial ARAnyM
 *
 * Revision 1.34  2002/01/17 14:59:19  milan
 * cleaning in HW <-> memory communication
 * support for JIT CPU
 *
 * Revision 1.33  2002/01/13 23:08:49  standa
 * The fVDI driver expandArea 24 and 32bit patch. 1, 2 and 4bit depth driver
 * configuration available.
 *
 * Revision 1.32  2002/01/09 19:37:33  standa
 * The fVDI driver patched to not to pollute the HostScreen class getPaletteColor().
 *
 * Revision 1.31  2002/01/08 22:40:00  standa
 * The palette fix and a little 8bit driver update.
 *
 * Revision 1.30  2002/01/08 21:20:57  standa
 * fVDI driver palette store on res change implemented.
 *
 * Revision 1.29  2001/12/29 17:03:23  joy
 * Johan: new(nothrow) => new(std:nothrow)
 *
 * Revision 1.28  2001/12/17 08:33:00  standa
 * Thread synchronization added. The check_event and fvdidriver actions are
 * synchronized each to other.
 *
 * Revision 1.27  2001/12/12 02:17:36  standa
 * Some line clipping bug fixed (the clip is on every time now -> should fix).
 *
 * Revision 1.26  2001/12/11 21:03:57  standa
 * Johan's patch caused DEBUG directive to fail e.g. in main.cpp.
 * The inline functions were put into the .cpp file.
 *
 * Revision 1.25  2001/11/29 23:51:56  standa
 * Johan Klockars <rand@cd.chalmers.se> fVDI driver changes.
 *
 * Revision 1.24  2001/11/26 16:07:57  standa
 * Olivier Landemarre found bug fixed.
 *
 * Revision 1.23  2001/11/19 01:39:10  standa
 * The first bitplane mode version. The blit and expand M->M needs to be
 * implemented. There is some pixel shift in blitting that I can't find.
 *
 * Revision 1.22  2001/10/31 23:17:38  standa
 * fVDI driver update The 16,24 and 32bit mode should work.
 *
 * Revision 1.21  2001/10/30 22:59:34  standa
 * The resolution change is now possible through the fVDI driver.
 *
 * Revision 1.20  2001/10/29 23:15:26  standa
 * The blitArea method rewitten to use macros. More readable code.
 *
 * Revision 1.19  2001/10/24 17:55:01  standa
 * The fVDI driver fixes. Finishing the functionality tuning.
 *
 * Revision 1.18  2001/10/23 21:28:49  standa
 * Several changes, fixes and clean up. Shouldn't crash on high resolutions.
 * hostscreen/gfx... methods have fixed the loop upper boundary. The interface
 * types have changed quite havily.
 *
 * Revision 1.17  2001/10/19 11:58:46  standa
 * The line clipping added (has bug, when neither one point is within the rect).
 * expandArea not handles the spacial mode (fallbacks it).
 * working with fVDI beta.
 *
 * Revision 1.16  2001/10/16 09:07:36  standa
 * The fVDI expandArea M->M implemented. The 16bit driver should work now.
 *
 * Revision 1.15  2001/10/14 16:11:54  standa
 * Syntax fix. STanda's calls to videl.renderNoFlag commented out.
 *
 * Revision 1.14  2001/10/12 08:25:38  standa
 * The fVDI blitting fixed and extended. Now only the expandToMemory and
 * blit mem2mem is to be done.
 *
 * Revision 1.13  2001/10/03 06:37:41  standa
 * General cleanup. Some constants added. Better "to screen" operation
 * recognition (the videoram address is checked too - instead of only the
 * MFDB == NULL || MFDB->address == NULL)
 *
 * Revision 1.12  2001/10/01 22:22:41  standa
 * bitplaneToChunky conversion moved into HostScreen (inline - should be no performance penalty).
 * fvdidrv/blitArea form memory works in TC.
 *
 * Revision 1.11  2001/09/30 23:09:23  standa
 * The line logical operation added.
 * The first version of blitArea (screen to screen only).
 *
 * Revision 1.10  2001/09/24 23:16:28  standa
 * Another minor changes. some logical operation now works.
 * fvdidrv/fillArea and fvdidrv/expandArea got the first logOp handling.
 *
 * Revision 1.9  2001/09/21 13:57:32  standa
 * D2(x) used - see include/debug.h
 *
 * Revision 1.8  2001/09/21 06:53:26  standa
 * The blitting is now said to be working, although it is not implemented.
 * expand to memory (not to SDL surface) is not either processed or dispalyed.
 *
 * Revision 1.7  2001/09/20 18:12:09  standa
 * Off by one bug fixed in fillArea.
 * Separate functions for transparent and opaque background.
 * gfxPrimitives methods moved to the HostScreen
 *
 * Revision 1.6  2001/09/19 23:03:46  standa
 * The fVDI driver update. Basic expandArea was added to display texts.
 * Still heavy buggy code!
 *
 * Revision 1.5  2001/08/30 14:04:59  standa
 * The fVDI driver. mouse_draw implemented. Partial pattern fill support.
 * Still buggy.
 *
 * Revision 1.4  2001/08/28 23:26:09  standa
 * The fVDI driver update.
 * VIDEL got the doRender flag with setter setRendering().
 *       The host_colors_uptodate variable name was changed to hostColorsSync.
 * HostScreen got the doUpdate flag cleared upon initialization if the HWSURFACE
 *       was created.
 * fVDIDriver got first version of drawLine and fillArea (thanks to SDL_gfxPrimitives).
 *
 * Revision 1.3  2001/08/09 12:35:43  standa
 * Forced commit to sync the CVS. ChangeLog should contain all details.
 *
 * Revision 1.2  2001/07/24 06:41:25  joy
 * updateScreen removed.
 * Videl reference should be removed as well.
 *
 * Revision 1.1  2001/06/18 15:48:42  standa
 * fVDI driver object.
 *
 *
 */
