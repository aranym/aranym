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
		uint16 *address;
		uint16 width;
		uint16 height;
		uint16 wdwidth;
		uint16 standard;
		uint16 bitplanes;
		uint16 reserved[3];
	};

	uint16 mouseMask[16];
	uint16 mouseData[16];
	uint16 oldMouseX, oldMouseY;
	uint32 mouseBackground[16][16]; // The mouse background backup surface for FVDIDriver

  public:
	FVDIDriver()
	{
		// This is the default drv (shouldn't be used)
	}
	~FVDIDriver() {
	}

	void dispatch( uint32 fncode, M68kRegisters *r );

	MFDB* FVDIDriver::fetchMFDB( MFDB* mfdb, uint32 pmfdb );
	//  MFDB*  fetchMFDB( MFDB* mfdb, uint32 pmfdb );
	uint32 putPixel( void *vwk, MFDB *dst, int32 x, int32 y, uint32 colour );
	uint32 getPixel( void *vwk, MFDB *src, int32 x, int32 y );
	uint32 drawMouse( void *wrk, int32 x, int32 y, uint32 mode );

	void FVDIDriver::saveMouseBackground( int32 x, int32 y, bool save );


	uint32 fillArea(void *vwk, int32 x1, int32 y1, int32 x2, int32 y2, uint16 *pattern, uint32 color);
	uint32 drawLine(void *vwk, int32 x1, int32 y1, int32 x2, int32 y2, uint16 pattern, uint32 color, uint32 logop);
};

#endif


/*
 * $Log$
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
