/*
 * $Header$
 *
 * STanda 2001
 */

#ifndef _FVDIDRV_H
#define _FVDIDRV_H

#include "cpu_emulation.h"

class FVDIDriver {
  private:

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

	// fillPoly helpers
	bool AllocIndices(int n);
	bool AllocCrossings(int n);
	bool AllocPoints(int n);

	int16* alloc_index;
	int index_count;
	int16* alloc_crossing;
	int crossing_count;
	int16* alloc_point;
	int point_count;


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

	int putPixel( memptr vwk, memptr dst, int32 x, int32 y, uint32 colour );
	uint32 getPixel( memptr vwk, memptr src, int32 x, int32 y );
	int drawMouse( memptr wrk, int16 x, int16 y, uint32 mode );

	int fillArea(memptr vwk, uint32 x_, uint32 y_, int w, int h,
	             memptr pattern_address, int32 colors);
	int drawLine(memptr vwk, uint32 x1_, uint32 y1_, uint32 x2_, uint32 y2_,
	             uint16 pattern, int32 colors, int logOp);
	int fillPoly(memptr vwk, int32 points_addr, int n, memptr index_addr, int moves,
	             memptr pattern_addr, int32 colors);
	int expandArea(memptr vwk, memptr src, memptr dest, int32 sx, int32 sy, int32 dx, int32 dy,
	               int32 w, int32 h, uint32 fgColor, uint32 bgColor, uint32 logOp);
	int blitArea(memptr vwk, memptr src, memptr dest, int32 sx, int32 sy, int32 dx, int32 dy,
	             int32 w, int32 h, uint32 logOp);
};

#endif


/*
 * $Log$
 * Revision 1.13  2001/12/11 21:03:57  standa
 * Johan's patch caused DEBUG directive to fail e.g. in main.cpp.
 * The inline functions were put into the .cpp file.
 *
 * Revision 1.12  2001/11/29 23:51:56  standa
 * Johan Klockars <rand@cd.chalmers.se> fVDI driver changes.
 *
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
