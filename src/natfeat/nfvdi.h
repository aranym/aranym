/*
	NatFeat VDI driver

	ARAnyM (C) 2005 Patrice Mandin

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

#ifndef NFVDI_H
#define NFVDI_H

/*--- Includes ---*/

#include "SDL_compat.h"

#include "nf_base.h"
#include "parameters.h"

#include "../include/dirty_rects.h"

/*--- Defines ---*/

// The Atari structures offsets
#define MFDB_ADDRESS                0
#define MFDB_WIDTH                  4
#define MFDB_HEIGHT                 6
#define MFDB_WDWIDTH                8
#define MFDB_STAND                 10
#define MFDB_NPLANES               12

/* gsx modes */

#define MD_REPLACE      1
#define MD_TRANS        2
#define MD_XOR          3
#define MD_ERASE        4

/* bit blt rules */

#define ALL_WHITE        0
#define S_AND_D          1
#define S_AND_NOTD       2
#define S_ONLY           3
#define NOTS_AND_D       4
#define D_ONLY           5
#define S_XOR_D          6
#define S_OR_D           7
#define NOT_SORD         8
#define NOT_SXORD        9
#define D_INVERT        10
#define NOT_D			D_INVERT
#define S_OR_NOTD		11
#define NOT_S			12
#define NOTS_OR_D       13
#define NOT_SANDD       14
#define ALL_BLACK       15

// This is supposed to be a fast 16x16/16 with 32 bit intermediate result
#define SMUL_DIV(x,y,z)	((short)(((x)*(long)(y))/(z)))
// Some other possible variants are
//#define SMUL_DIV(x,y,z)	((long)(y - y1) * (x2 - x1) / (y2 - y1))
//#define SMUL_DIV(x,y,z)	((short)(((short)(x)*(long)((short)(y)))/(short)(z)))

#if SDL_BYTEORDER == SDL_BIG_ENDIAN

#define put_dtriplet(address, data) \
{ \
	WriteInt8((address), ((data) >> 16) & 0xff); \
	WriteInt8((address) + 1, ((data) >> 8) & 0xff); \
	WriteInt8((address) + 2, (data) & 0xff); \
}

#define get_dtriplet(address) \
	((ReadInt8((address)) << 16) | (ReadInt8((address) + 1) << 8) | ReadInt8((address) + 2))

#else

#define put_dtriplet(address, data) \
{ \
	WriteInt8((address), (data) & 0xff); \
	WriteInt8((address) + 1, ((data) >> 8) & 0xff); \
	WriteInt8((address) + 2, ((data) >> 16) & 0xff); \
}

#define get_dtriplet(address) \
	((ReadInt8((address) + 2) << 16) | (ReadInt8((address) + 1) << 8) | ReadInt8((address)))

#endif // SDL_BYTEORDER == SDL_BIG_ENDIAN

/*--- Types ---*/

/*--- Class ---*/

class HostSurface;

class VdiDriver : public NF_Base
{
public:
	const char *name() { return "fVDI"; }
	bool isSuperOnly() { return false; }
	int32 dispatch(uint32 fncode);
	void reset();

	VdiDriver();
	virtual ~VdiDriver();

	HostSurface *getSurface(void);

protected:
	HostSurface *surface;

	struct _Mouse {
		struct {
			uint16 x, y;
			uint16 width, height;
			uint32 background[16][16]; // The mouse background backup surface for FVDIDriver
			struct {
				uint32 foreground; // The mouse shape color for FVDIDriver
				uint32 background; // The mouse mask color for FVDIDriver
			} color;
		} storage;
		struct hotspot_ {
			int16  x, y;
		} hotspot;
		struct colors_ {
			int16  fgColorIndex, bgColorIndex;
		} colors;

		uint16 mask[16];
		uint16 shape[16];
	} Mouse;

	virtual void restoreMouseBackground(void);
	virtual void saveMouseBackground(int16 x, int16 y, int16 width, int16 height);

	void setResolution(int32 width, int32 height, int32 depth);
	int32 getWidth(void);
	int32 getHeight(void);
	int32 getBpp(void);
	int32 blitArea(memptr vwk, memptr src, int32 sx, int32 sy, memptr dest,
			int32 dx, int32 dy, int32 w, int32 h, uint32 logOp);

	virtual int32 openWorkstation(void);
	virtual int32 closeWorkstation(void);
	virtual int32 getPixel(memptr vwk, memptr src, int32 x, int32 y);
	virtual int32 putPixel(memptr vwk, memptr dst, int32 x, int32 y, uint32 color);
	virtual int32 drawMouse(memptr wk, int32 x, int32 y, uint32 mode,
		uint32 data, uint32 hot_x, uint32 hot_y, uint32 fgColor,
		uint32 bgColor, uint32 mouse_type);
	virtual int32 expandArea(memptr vwk, memptr src, int32 sx, int32 sy,
		memptr dest, int32 dx, int32 dy, int32 w, int32 h, uint32 logOp,
		uint32 fgColor, uint32 bgColor);
	virtual int32 fillArea(memptr vwk, uint32 x_, uint32 y_, int32 w,
		int32 h, memptr pattern_address, uint32 fgColor, uint32 bgColor,
		uint32 logOp, uint32 interior_style);
	virtual	void fillArea(uint32 x, uint32 y, uint32 w, uint32 h,
	                      uint16* pattern, uint32 fgColor, uint32 bgColor,
	                      uint32 logOp) = 0;
	virtual int32 drawLine(memptr vwk, uint32 x1_, uint32 y1_, uint32 x2_,
		uint32 y2_, uint32 pattern, uint32 fgColor, uint32 bgColor,
		uint32 logOp, memptr clip);
	virtual int32 fillPoly(memptr vwk, memptr points_addr, int n,
		memptr index_addr, int moves, memptr pattern_addr, uint32 fgColor,
		uint32 bgColor, uint32 logOp, uint32 interior_style, memptr clip);
	virtual int32 drawText(memptr vwk, memptr text, uint32 length,
		int32 dst_x, int32 dst_y, memptr font,
		uint32 w, uint32 h, uint32 fgColor, uint32 bgColor,
		uint32 logOp, memptr clip);
	virtual void getHwColor(uint16 index, uint32 red, uint32 green,
		uint32 blue, memptr hw_value);
	virtual void setColor(memptr vwk, uint32 paletteIndex, uint32 red,
		uint32 green, uint32 blue);
	virtual int32 getFbAddr(void);

	/* Blit memory to screen */
	virtual int32 blitArea_M2S(memptr vwk, memptr src, int32 sx, int32 sy,
			memptr dest, int32 dx, int32 dy, int32 w, int32 h, uint32 logOp);
	/* Blit screen to screen */
	virtual int32 blitArea_S2S(memptr vwk, memptr src, int32 sx, int32 sy,
			memptr dest, int32 dx, int32 dy, int32 w, int32 h, uint32 logOp);
	/* Blit screen to memory */
	virtual int32 blitArea_S2M(memptr vwk, memptr src, int32 sx, int32 sy,
			memptr dest, int32 dx, int32 dy, int32 w, int32 h, uint32 logOp);

	/* Inlinable functions */
	void chunkyToBitplane(uint8 *sdlPixelData, uint16 bpp, uint16 bitplaneWords[8]);
	uint32 applyBlitLogOperation(int logicalOperation, uint32 destinationData, uint32 sourceData);

	// fillPoly helpers
	bool AllocIndices(int n);
	bool AllocCrossings(int n);
	bool EnoughCrossings(int n) { return n <= crossing_count; }
	bool AllocPoints(int n);

	int16* alloc_index;
	int16* alloc_crossing;
	int16* alloc_point;

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

private:
	SDL_Cursor *cursor;

	int events, new_event, mouse_x, mouse_y, buttons, wheel, vblank;
 
	/* Blit memory to memory */
	int32 blitArea_M2M(memptr vwk, memptr src, int32 sx, int32 sy,
			memptr dest, int32 dx, int32 dy, int32 w, int32 h, uint32 logOp);

	int index_count;
	int crossing_count;
	int point_count;
};

#endif /* NFVDI_H */
