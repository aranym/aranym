/*
 * $Header$
 *
 * STanda 2001
 */

#ifndef _FVDIDRV_H
#define _FVDIDRV_H

#include "cpu_emulation.h"
#include "debug.h"
#include <new>
#include <cstring>

class FVDIDriver {
  private:

	struct MFDB {
		uint32 address; // uint16* before
		uint16 width;
		uint16 height;
		uint16 wdwidth;
		uint16 standard;
		uint16 bitplanes;
		uint16 reserved[3];
	};

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

	int drawSingleLine(int x1, int y1, int x2, int y2, uint16 pattern,
	                   uint32 fgColor, uint32 bgColor, int logOp, bool last_pixel,
	                   int cliprect[], int minmax[]);
	int drawTableLine(int16 table[], int length, uint16 pattern,
	                  uint32 fgColor, uint32 bgColor, int logOp, int cliprect[], int minmax[]);
	int drawMoveLine(int16 table[], int length, uint16 index[], int moves, uint16 pattern,
	                 uint32 fgColor, uint32 bgColor, int logOp, int cliprect[], int minmax[]);

	// The polygon code needs some arrays of unknown size
	// These routines and members are used so that no unnecessary allocations are done
	bool AllocIndices(int n)
	{
		if (n > index_count) {
			D2(bug("More indices %d->%d\n", index_count, n));
			int count = n * 2;	// Take a few extra right away
			int16* tmp = new(nothrow) int16[count];
			if (!tmp) {
				count = n;
				tmp = new(nothrow) int16[count];
			}
			if (tmp) {
				delete[] alloc_index;
				alloc_index = tmp;
				index_count = count;
			}
		}

		return index_count >= n;
	}

	bool AllocCrossings(int n)
	{
		if (n > crossing_count) {
			D2(bug("More crossings %d->%d\n", crossing_count, n));
			int count = n * 2;		// Take a few extra right away
			int16* tmp = new(nothrow) int16[count];
			if (!tmp) {
				count = (n * 3) / 2;	// Try not so many extra
				tmp = new(nothrow) int16[count];
			}
			if (!tmp) {
				count = n;		// This is going to be slow if it goes on...
				tmp = new(nothrow) int16[count];
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

	bool AllocPoints(int n)
	{
		if (n > point_count) {
			D2(bug("More points %d->%d", point_count, n));
			int count = n * 2;	// Take a few extra right away
			int16* tmp = new(nothrow) int16[count * 2];
			if (!tmp) {
				count = n;
				tmp = new(nothrow) int16[count * 2];
			}
			if (tmp) {
				delete[] alloc_point;
				alloc_point = tmp;
				point_count = count;
			}
		}

		return point_count >= n;
	}

	int16* alloc_index;
	int index_count;
	int16* alloc_crossing;
	int crossing_count;
	int16* alloc_point;
	int point_count;

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

  public:
	FVDIDriver()
	 : alloc_index(0), index_count(0),
	   alloc_crossing(0), crossing_count(0),
	   alloc_point(0), point_count(0)
	{
		// This is the default drv (shouldn't be used)
	}
	~FVDIDriver() {
		delete[] alloc_index;
		delete[] alloc_crossing;
		delete[] alloc_point;
	}

	void dispatch( uint32 fncode, M68kRegisters *r );

	void restoreMouseBackground();
	void saveMouseBackground( int16 x, int16 y, int16 width, int16 height );
	void setColor( uint32 paletteIndex, uint32 red, uint32 green, uint32 blue );
	void setResolution( int32 width, int32 height, int32 depth, int32 freq );

	MFDB* FVDIDriver::fetchMFDB( MFDB* mfdb, uint32 pmfdb );

	int putPixel( void *vwk, MFDB *dst, int32 x, int32 y, uint32 colour );
	uint32 getPixel( void *vwk, MFDB *src, int32 x, int32 y );
	int drawMouse( void *wrk, int16 x, int16 y, uint32 mode );

	int fillArea(uint32 vwk, uint32 x_, uint32 y_, int w, int h,
	             uint32 pattern_address, int32 colors);
	int drawLine(uint32 vwk, uint32 x1_, uint32 y1_, uint32 x2_, uint32 y2_,
	             uint16 pattern, int32 colors, int logOp);
	int fillPoly(uint32 vwk, int32 points_addr, int n, uint32 index_addr, int moves,
	             uint32 pattern_addr, int32 colors);
	int expandArea(void *vwk, MFDB *src, MFDB *dest, int32 sx, int32 sy, int32 dx, int32 dy,
	               int32 w, int32 h, uint32 fgColor, uint32 bgColor, uint32 logOp);
	int blitArea(void *vwk, MFDB *src, MFDB *dest, int32 sx, int32 sy, int32 dx, int32 dy,
	             int32 w, int32 h, uint32 logOp);
};

#endif


/*
 * $Log$
 * Revision 1.11  2001/11/21 13:29:51  milan
 * cleanning & portability
 *
 * Revision 1.10  2001/10/31 23:17:38  standa
 * fVDI driver update The 16,24 and 32bit mode should work.
 *
 * Revision 1.9  2001/10/30 22:59:34  standa
 * The resolution change is now possible through the fVDI driver.
 *
 * Revision 1.8  2001/10/23 21:28:49  standa
 * Several changes, fixes and clean up. Shouldn't crash on high resolutions.
 * hostscreen/gfx... methods have fixed the loop upper boundary. The interface
 * types have changed quite havily.
 *
 * Revision 1.7  2001/10/03 06:37:41  standa
 * General cleanup. Some constants added. Better "to screen" operation
 * recognition (the videoram address is checked too - instead of only the
 * MFDB == NULL || MFDB->address == NULL)
 *
 * Revision 1.6  2001/09/30 23:09:23  standa
 * The line logical operation added.
 * The first version of blitArea (screen to screen only).
 *
 * Revision 1.5  2001/09/20 18:12:09  standa
 * Off by one bug fixed in fillArea.
 * Separate functions for transparent and opaque background.
 * gfxPrimitives methods moved to the HostScreen
 *
 * Revision 1.4  2001/09/19 23:03:46  standa
 * The fVDI driver update. Basic expandArea was added to display texts.
 * Still heavy buggy code!
 *
 * Revision 1.3  2001/08/30 14:04:59  standa
 * The fVDI driver. mouse_draw implemented. Partial pattern fill support.
 * Still buggy.
 *
 * Revision 1.2  2001/08/28 23:26:09  standa
 * The fVDI driver update.
 * VIDEL got the doRender flag with setter setRendering().
 *       The host_colors_uptodate variable name was changed to hostColorsSync.
 * HostScreen got the doUpdate flag cleared upon initialization if the HWSURFACE
 *       was created.
 * fVDIDriver got first version of drawLine and fillArea (thanks to SDL_gfxPrimitives).
 *
 * Revision 1.1  2001/06/18 15:48:42  standa
 * fVDI driver object.
 *
 *
 */
