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

		case 3: // drawMouse:
			D(fprintf(stderr, "fVDI: drawMouse\n"));
			r->d[0] = drawMouse( (void*)get_long( (uint32)r->a[1], true ),
								 r->d[0] /* x */, r->d[1] /* y */, r->d[2] /* mode/Mouse* */ );
			break;

		// SPEEDUP functions

		case 4: // expandArea:
			D(fprintf(stderr, "fVDI: %s %x %d,%d:%d,%d (%d,%d)\n", "expandArea", r->d[7], r->d[1], r->d[2], r->d[3], r->d[4], r->d[6] & 0xffff, r->d[6] >> 16 ));
			r->d[0] = 1;
			break;

		case 5: // fillArea
			if ( ( r->d[2] >> 16 ) != 0 )
				r->d[0] = (uint32)-1; // we can do only the basic single mode
			else {
				uint16 pattern[16];
				uint32 fPatterAddress = r->d[3];
				for( uint16 i=0; i<=30; i+=2 )
					pattern[i >> 1] = get_word( fPatterAddress + i, true );

				r->d[0] = fillArea( (void*)get_long( (uint32)r->a[1], true ),
									r->d[1] & 0xffff /* x */, r->d[2] & 0xffff /* y */,
									r->d[0] & 0xffff /* w */, r->d[0] >> 16 /* h */,
									pattern /* pattern */, r->d[4] /* color */ );
			}
			break;

		case 6: // blitArea
			D(fprintf(stderr, "fVDI: blitArea %d NOT IMPLEMENTED\n", fncode));
			r->d[0] = 0;
			break;

		case 7: // drawLine:
			if ( ( r->d[2] >> 16 ) != 0 )
				r->d[0] = (uint32)-1; // we can do only one line at once
			else {
				r->d[0] = drawLine( (void*)get_long( (uint32)r->a[1], true ),
									r->d[1] /* x1 */, r->d[2] /* y1 */,
									r->d[3] /* x2 */, r->d[4] /* y2 */,
									r->d[5] & 0xffff /* pattern */, r->d[6] /* color */,
									r->d[0] & 0xffff /* logical operation */ );
			}
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
 * Set a coloured pixel.

 * c_write_pixel(Virtual *vwk, MFDB *mfdb, long x, long y, long colour)
 * write_pixel
 * In:	a1	VDI struct, destination MFDB
 *	d0	colour
 *	d1	x or table address
 *	d2	y or table length (high) and type (low)
 * XXX:	?
 *
 * This function has two modes:
 *  - single pixel
 *  - table based multi pixel (special mode 0 (low word of 'y'))
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
uint32 FVDIDriver::putPixel(void *vwk, MFDB *dst, int32 x, int32 y, uint32 color)
{
    uint32 offset;

    if ((uint32)vwk & 1)
		return 0;

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

/**
 * Get a coloured pixel.
 *
 * c_read_pixel(Virtual *vwk, MFDB *mfdb, long x, long y)
 * read_pixel
 * In:	a1	VDI struct, source MFDB
 *	d1	x
 *	d2	y
 *
 * Only one mode here.
 *
 * Note that a1 does not point to the VDI struct, but to a place in memory
 * where the VDI struct pointer can be found. Four bytes beyond that address
 * is a pointer to the destination MFDB.
 *
 * Since an MFDB is passed, the source is not necessarily the screen.
 **/
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


/**
 * Set palette colours
 *
 * c_set_colours(Virtual *vwk, long start, long entries, short requested[3][], Colour palette[])
 * set_colours
 * In:	a0	VDI struct
 *	d0	number of entries, start entry
 *	a1	requested colour values (3 word/entry)
 *	a2	colour palette
 **/

///// FIXME!!! TODO!!! now in m68 code driver part.



/*************************************************************************************************/
/**************************************************************************************************
 * 'Wanted' functions
 * ------------------
 * While fVDI can make do without these, it will be very slow.
 * Note that the fallback mechanism can be used for uncommon operations
 * without sacrificing speed for common ones. At least during development,
 * it is also possible to simply ignore some things, such as a request for
 * colour, patterns, or a specific effect.
**************************************************************************************************/
/*************************************************************************************************/

/**
 * Draw the mouse
 *
 * Draw a coloured line between two points.
 *
 * c_mouse_draw(Workstation *wk, long x, long y, Mouse *mouse)
 * mouse_draw
 * In:	a1	Pointer to Workstation struct
 *	d0/d1	x,y
 *  d2	0 - move shown  1 - move hidden  2 - hide  3 - show  >3 - change shape (pointer to mouse struct)
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
 *   short type;
 *   short hide;
 *   struct position_ {
 *	   short x;
 *	   short y;
 *   } position;
 *   struct hotspot_ {
 *	   short x;
 *	   short y;
 *   } hotspot;
 *   Fgbg colour;
 *   short mask[16];
 *   short data[16];
 *   void *extra_info;
 * } Mouse;
 **/
extern "C" {
	static void getBinary( uint16 data, char *buffer ) {
		for( uint16 i=0; i<=15; i++ ) {
			buffer[i] = (data & 1)?'1':' ';
			data>>=1;
		}
		buffer[16]='\0';
	}
	static uint16 reverse_bits( uint16 data ) {
		uint16 res = 0;
		for( uint16 i=0; i<=15; i++ )
			res |= (( data >> i ) & 1 ) << ( 15 - i );
		return res;
	}
}

void FVDIDriver::saveMouseBackground( int32 x, int32 y, bool save )
{
	D(fprintf(stderr, "fVDI: saveMouseBackground: %d,%d,%d\n", x, y, save ));

	if ( save ) {
		for( uint16 i=0; i<=15; i++ )
			for( uint16 j=0; j<=15; j++ ) {
				mouseBackground[i][j] =	hostScreen.getPixel(x + j, y + i);
			}
	} else {
		for( uint16 i=0; i<=15; i++ )
			for( uint16 j=0; j<=15; j++ )
				hostScreen.putPixel(x + j, y + i, mouseBackground[i][j] );
	}
}

uint32 FVDIDriver::drawMouse( void *wrk, int32 x, int32 y, uint32 mode ) {
	//	uint8 r,g,b,a; SDL_GetRGBA( color, surf->format, &r, &g, &b, &a);

	D(fprintf(stderr, "fVDI: mouse mode: %x\n", mode ));

	switch ( mode ) {
		case 0:  // move shown
			saveMouseBackground( oldMouseX, oldMouseY, false ); // restore
			saveMouseBackground( oldMouseX = x, oldMouseY = y, true ); // save
			break;
		case 1:  // move hidden
			return 1;
		case 2:  // hide
			saveMouseBackground( oldMouseX, oldMouseY, false ); // restore
			return 1;
		case 3:  // show
			saveMouseBackground( oldMouseX = x, oldMouseY = y, true ); // save
			break;

		default: // change pointer shape
			uint32 fPatterAddress = mode + sizeof( uint16 ) * 8;
			for( uint16 i=0; i<=30; i+=2 )
				mouseMask[i >> 1] = reverse_bits( get_word( fPatterAddress + i, true ) );
			fPatterAddress += 30;
			for( uint16 i=0; i<=30; i+=2 )
				mouseData[i >> 1] = reverse_bits( get_word( fPatterAddress + i, true ) );

			char buffer[30];
			for( uint16 i=0; i<=15; i++ ) {
				getBinary( mouseMask[i], buffer );
				D(fprintf(stderr, "fVDI: apm:%s\n", buffer ));
			}
			for( uint16 i=0; i<=15; i++ ) {
				getBinary( mouseData[i], buffer );
				D(fprintf(stderr, "fVDI: apd:%s\n", buffer ));
			}

			return 1;
	}

	uint16 mm[16];
	uint16 md[16];
	uint16 shift = x & 0xf;
	for( uint16 i=0; i<=15; i++ ) {
		mm[(i + (y&0xf)) & 0xf] = ( mouseMask[i] << shift ) | ((mouseMask[i] >> (16-shift)) & ((1<<shift)-1) );
		md[(i + (y&0xf)) & 0xf] = ( mouseData[i] << shift ) | ((mouseData[i] >> (16-shift)) & ((1<<shift)-1) );
	}

	hostScreen.fillArea( x, y, x + 15, y + 15, mm, hostScreen.getColor( 0xfe, 0xfe, 0xfe ) /* white */ );
	hostScreen.fillArea( x, y, x + 15, y + 15, md, hostScreen.getColor( 0, 0, 0 ) /* black */ );
	hostScreen.update( true ); // x, y, x + 15, y + 15, true );

	return 1;
}


/**
 * Expand a monochrome area to a coloured one.
 *
 * c_expand_area(Virtual *vwk, MFDB *src, long src_x, long src_y,
 *                             MFDB *dst, long dst_x, long dst_y,
 *                             long w, long h, long operation, long colour)
 * expand_area
 * In:	a1	VDI struct, destination MFDB, VDI struct, source MFDB
 *	d0	height and width to move (high and low word)
 *	d1-d2	source coordinates
 *	d3-d4	destination coordinates
 *	d6	background and foreground colour
 *	d7	logic operation
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
 **/

///// FIXME!!! TODO!!!



/**
 * Fill a coloured area using a monochrome pattern.
 *
 * c_fill_area(Virtual *vwk, long x, long y, long w, long h,
 *                           short *pattern, long colour)
 * fill_area
 * In:	a1	VDI struct
 *	d0	height and width to fill (high and low word)
 *	d1	x or table address
 *	d2	y or table length (high) and type (low)
 *	d3	pattern address
 *	d4	colour
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
uint32 FVDIDriver::fillArea(void *vwk, int32 x, int32 y, int32 w, int32 h, uint16 *pattern, uint32 color)
{
	D(fprintf(stderr, "fVDI: %s %d,%d:%d,%d p:%x, (%x)\n", "fillArea", x, y, w, h, *pattern, color ));

	hostScreen.fillArea( x, y, x + w - 1, y + h - 1, pattern, color );
	hostScreen.update( x, y, w, h, true );

	return 1;
}


/**
 * Blit an area
 *
 * c_blit_area(Virtual *vwk, MFDB *src, long src_x, long src_y,
 *                        MFDB *dst, long dst_x, long dst_y,
 *                        long w, long h, long operation)
 * blit_area
 * In:	a1	VDI struct, destination MFDB, VDI struct, source MFDB
 *	d0	height and width to move (high and low word)
 *	d1-d2	source coordinates
 *	d3-d4	destination coordinates
 *	d5	logic operation
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

///// FIXME!!! TODO!!!



/**
 * Draw a coloured line between two points
 *
 * c_draw_line(Virtual *vwk, long x1, long y1, long x2, long y2,
 *                          long pattern, long colour)
 * draw_line
 * In:	a1	VDI struct
 *	d0	logic operation
 *	d1	x1 or table address
 *	d2	y1 or table length (high) and type (low)
 *	d3	x2 or move point count
 *	d4	y2 or move index address
 *	d5	pattern
 *	d6	colour
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
uint32 FVDIDriver::drawLine(void *vwk, int32 x1, int32 y1, int32 x2, int32 y2, uint16 pattern, uint32 color, uint32 logop )
{
	D(fprintf(stderr, "fVDI: %s %x %d,%d:%d,%d p:%x, (%d)\n", "drawLine", logop, x1, y1, x2, y2, pattern, color ));

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


/*
 * $Log$
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
