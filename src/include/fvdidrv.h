/*
 * $Header$
 *
 * STanda 2001
 */

#ifndef _FVDIDRV_H
#define _FVDIDRV_H


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

  public:
	FVDIDriver()
	{
		// This is the default drv (shouldn't be used)
	}
	~FVDIDriver() {
	}

	void dispatch( uint32 fncode, M68kRegisters *r );

	void restoreMouseBackground();
	void saveMouseBackground( int16 x, int16 y, int16 width, int16 height );
	void setColor( uint32 paletteIndex, uint32 red, uint32 green, uint32 blue );
	void setResolution( int32 width, int32 height, int32 depth, int32 freq );

	MFDB* FVDIDriver::fetchMFDB( MFDB* mfdb, uint32 pmfdb );

	uint32 putPixel( void *vwk, MFDB *dst, int32 x, int32 y, uint32 colour );
	uint32 getPixel( void *vwk, MFDB *src, int32 x, int32 y );
	uint32 drawMouse( void *wrk, int16 x, int16 y, uint32 mode );

	uint32 fillArea(void *vwk, int32 x1, int32 y1, int32 x2, int32 y2,
					uint16 *pattern, uint32 fgColor, uint32 bgColor);
	uint32 drawLine(void *vwk, int32 x1, int32 y1, int32 x2, int32 y2,
					uint16 pattern, uint32 fgColor, uint32 bgColor, uint32 logOp);
	uint32 expandArea(void *vwk, MFDB *src, MFDB *dest, int32 sx, int32 sy, int32 dx, int32 dy, int32 w, int32 h,
					  uint32 fgColor, uint32 bgColor, uint32 logOp);
	uint32 blitArea(void *vwk, MFDB *src, MFDB *dest, int32 sx, int32 sy, int32 dx, int32 dy, int32 w, int32 h,	uint32 logOp);
};

#endif


/*
 * $Log$
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
