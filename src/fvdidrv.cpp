/*
 * $Header$
 *
 * STanda 2001
 */

#include "sysdeps.h"
#include "cpu_emulation.h"
#include "main.h"

#include "videl.h"
#include "hostscreen.h"

#include "fvdidrv.h"

#define DEBUG 0
#include "debug.h"


extern HostScreen hostScreen;

extern VIDEL videl;
// extern bool updateScreen;


void FVDIDriver::dispatch( uint32 fncode, M68kRegisters *r )
{
    MFDB dst;
    MFDB src;
    MFDB *pSrc = NULL;
    MFDB *pDst = NULL;

    // fix the stack (the fncode was pushed onto the stack)
    r->a[7] += 4;

	videl.setRendering( false );

    switch (fncode) {
		// NEEDED functions
		case 1: // getPixel:
			D(fprintf(stderr, "fVDI: %s %d,%d\n", "readPixel", (uint16)r->d[1], (uint16)r->d[2] ));
			r->d[0] = getPixel( (void*)get_long( (uint32)r->a[1], true ),
								fetchMFDB( &src, get_long( (uint32)r->a[1] + 4, true ) ),
								r->d[1] /* x */, r->d[2] /* y */);
			break;
		case 2: // putPixel:
			D(fprintf(stderr, "fVDI: %s %d,%d (%x)\n", "writePixel", (uint16)r->d[1], (uint16)r->d[2], (uint32)r->d[0] ));
			r->d[0] = putPixel( (void*)get_long( (uint32)r->a[1], true ),
								fetchMFDB( &dst, get_long( (uint32)r->a[1] + 4, true ) ),
								r->d[1] /* x */, r->d[2] /* y */,
								r->d[0] /* color */);
			break;

		// SPEEDUP functions
		case 4: // expandArea:
			D(fprintf(stderr, "fVDI: expandArea %d NOT IMPLEMENTED\n", fncode));
			r->d[0] = 1;
			break;
		case 5: // fillArea
			D(fprintf(stderr, "fVDI: %s %d,%d:%d,%d p:%x, (%d)\n", "fillArea", r->d[1], r->d[2], r->d[0] & 0xffff, r->d[0] >> 16, (uint32)r->d[3], (uint32)r->d[4] ));

			if ( ( r->d[2] >> 16 ) != 0 )
				r->d[0] = (uint32)-1; // we can do only the basic single mode
			else
				r->d[0] = fillArea( (void*)get_long( (uint32)r->a[1], true ),
									r->d[1] /* x */, r->d[2] /* y */,
									r->d[0] & 0xffff /* w */, r->d[0] >> 16 /* h */,
									r->d[3] /* pattern */, r->d[4] /* color */);
			break;
		case 6: // blitArea
			D(fprintf(stderr, "fVDI: blitArea %d NOT IMPLEMENTED\n", fncode));
			r->d[0] = 1;
			break;
		case 7: // drawLine:
			D(fprintf(stderr, "fVDI: %s %x %d,%d:%d,%d p:%x, (%d)\n", "drawLine", r->d[0], r->d[1], r->d[2], (uint32)r->d[3], (uint32)r->d[4], (uint32)r->d[5], (uint32)r->d[6] ));

			if ( ( r->d[2] >> 16 ) != 0 )
				r->d[0] = (uint32)-1; // we can do only one line at once
			else
				r->d[0] = drawLine( (void*)get_long( (uint32)r->a[1], true ),
									r->d[1] /* x1 */, r->d[2] /* y1 */,
									r->d[3] /* x2 */, r->d[4] /* y2 */,
									r->d[5] /* pattern */, r->d[0] >> 16 /* color */);
			break;

		// not implemented functions
		default:
			D(fprintf(stderr, "fVDI: Unknown %d\n", fncode));
			r->d[0] = 1;
    }
}


FVDIDriver::MFDB* FVDIDriver::fetchMFDB( FVDIDriver::MFDB* mfdb, uint32 pmfdb )
{
    if ( pmfdb != 0 ) {
		mfdb->address = (uint16*)get_long( pmfdb, true );
		return mfdb;
    }

    return NULL;
}


/**
 *
 * destination MFDB (odd address marks table operation)
 * x or table address
 * y or table length (high) and type (0 - coordinates)
 */
uint32 FVDIDriver::putPixel(void *vwk, MFDB *dst, int32 x, int32 y, uint32 color)
{
    uint32 offset;

    if ((uint32)vwk & 1)
		return 0;

    // updateScreen = false;

    if (!dst || !dst->address) {
		//  Workstation *wk;
		//wk = vwk->real_address;
		//|| (dst->address == wk->screen.mfdb.address)) {

		//D(fprintf(stderr, "\nfVDI: %s %d,%d (%d %08x)\n", "write_pixel", (uint16)x, (uint16)y, (uint32)color, (uint32)color));

		if (!hostScreen.renderBegin())
			return 0;

		((uint16*)hostScreen.getVideoramAddress())[((uint16)y)*videl.getScreenWidth()+(uint16)x] = (uint16)color;
		//((uint16*)surf->pixels)[(uint16)y*640+(uint16)x] = (uint16)sdl_colors[(uint16)color];

		hostScreen.renderEnd();
		// FIXME!!!		hostScreen.putPixel( (int16)x, (int16)y, color );
		hostScreen.update( (int16)x, (int16)y, 1, 1, true );
		//    if ( (uint16)y > 479 || (uint16)x > 639 )

    } else {
		D(fprintf(stderr, "fVDI: %s %d,%d (%d)\n", "write_pixel no screen", (uint16)x, (uint16)y, (uint32)color));

		offset = (dst->wdwidth * 2 * dst->bitplanes) * y + x * sizeof(uint16);
		put_word( (uint32)dst->address + offset, color );
    }

    return 1;
}


uint32 FVDIDriver::getPixel(void *vwk, MFDB *src, int32 x, int32 y)
{
    uint32 offset;
    uint32 color = 0;

    if (!src || !src->address) {
		//|| (src->address == wk->screen.mfdb.address)) {

		if (!hostScreen.renderBegin())
			return 0;
		color = ((uint16*)hostScreen.getVideoramAddress())[((uint16)y)*videl.getScreenWidth()+(uint16)x];

		// FIXME!!!		color = hostScreen.getPixel( (int16)x, (int16)y );

		//    if ( (uint16)y > 479 || (uint16)x > 639 )
		//D(fprintf(stderr, "fVDI: %s %d,%d %04x\n", "read_pixel", (uint16)x, (uint16)y, (uint16)color ));

		hostScreen.renderEnd();
		//    color = (color >> 8) | ((color & 0xff) << 8);  // byteswap
    } else {
		D(fprintf(stderr, "fVDI: %s %d,%d (%d)\n", "read_pixel no screen", (uint16)x, (uint16)y, (uint32)color));

		offset = (src->wdwidth * 2 * src->bitplanes) * y + x * sizeof(uint16);
		color = get_word((uint32)src->address + offset, true);
    }

    return color;
}


uint32 FVDIDriver::drawLine(void *vwk, int32 x1, int32 y1, int32 x2, int32 y2, uint32 pattern, uint32 color)
{
	hostScreen.drawLine( x1, y1, x2, y2, pattern, color );

	int32 dx, dy, lx, ly;
	if ( x1 >= x2 ) {
		lx = x2;
		dx = x1 - x2 + 1;
	} else {
		lx = x1;
		dx = x2 - x1 + 1;
	}
	if ( y1 >= y2 ) {
		ly = y2;
		dy = y1 - y2 + 1;
	} else {
		ly = y1;
		dy = y2 - y1 + 1;
	}

	hostScreen.update( lx, ly, dx, dy, true );

	return 1;
}


uint32 FVDIDriver::fillArea(void *vwk, int32 x, int32 y, int32 w, int32 h, uint32 pattern, uint32 color)
{
	hostScreen.fillArea( x, y, x + w - 1, y + h - 1, pattern, color );
	hostScreen.update( x, y, w, h, true );

	return 1;
}



/*
 * $Log$
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
