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

// this serves for debugging the color palette code
#undef DEBUG_DRAW_PALETTE

extern HostScreen hostScreen;
extern VIDEL videl;

/**
    typedef struct vwk_ {
0		struct wk_ *real_address;
4		short standard_handle;
		struct text_ {
6			short font;
8			Fgbg colour;
12			short rotation;
			struct alignment_ {
14				short horizontal;
16				short vertical;
			} alignment;
			struct character_ {
18				short width;
20				short height;
			} character;
			struct cell_ {
22				short width;
24				short height;
			} cell;
26			short effects;
28			Fontheader *current_font;	// Not in standard VDI
		} text;
		struct line_ {
32			short type;
34			Fgbg colour;
			struct ends_ {
38				short beginning;
40				short end;
			} ends;
42			short width;
44			short user_mask;
		} line;
		struct bezier_ {
46			short on;		// Should these really be per vwk?
48			short depth_scale;
		} bezier;
		struct marker_ {
50			short type;
52			Fgbg colour;
			struct size_ {
56				short width;
58				short height;
			} size;
		} marker;
		struct fill_ {
60			short interior;
62			Fgbg colour;
66			short style;
68			short perimeter;
			struct user_ {
				struct pattern_ {
70					short *in_use;
74					short *extra;
				} pattern;
78				short multiplane;
			} user;
		} fill;
		struct clip_ {
80			short on;
			struct rectangle_ {
82				short x1;
84				short y1;
86				short x2;
88				short y2;
			} rectangle;
		} clip;
90		short mode;
92		Colour *palette;		// Odd when only negative (fg/bg)
	} Virtual;
**/


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
			/*
			 * read_pixel
			 * In:	a1	VDI struct, source MFDB
			 *	d1	x
			 *	d2	y
			 */
			D(bug("fVDI: %s %d,%d", "readPixel", (uint16)r->d[1], (uint16)r->d[2] ));
			r->d[0] = getPixel( (void*)r->a[1],
								fetchMFDB( &src, get_long( (uint32)r->a[1] + 4, true ) ),
								r->d[1] /* x */, r->d[2] /* y */);
			break;

		case 2: // putPixel:
			/*
			 * write_pixel
			 * In:	a1	VDI struct, destination MFDB
			 *	d0	colour
			 *	d1	x or table address
			 *	d2	y or table length (high) and type (low)
			 * XXX:	?
			 */
			D(bug("fVDI: %s %d,%d (%x)", "writePixel", (uint16)r->d[1], (uint16)r->d[2], (uint32)r->d[0] ));
			r->d[0] = putPixel( (void*)r->a[1],
								fetchMFDB( &dst, get_long( (uint32)r->a[1] + 4, true ) ),
								r->d[1] /* x */, r->d[2] /* y */,
								r->d[0] /* color */);
			break;

		case 3: // drawMouse:
			/*
			 * mouse_draw
			 * In:	a1	Pointer to Workstation struct
			 *	d0/d1	x,y
			 *  d2	0 - move shown  1 - move hidden  2 - hide  3 - show  >3 - change shape (pointer to mouse struct)
			 */
			D(bug("fVDI: drawMouse"));
			r->d[0] = drawMouse( (void*)r->a[1],
								 r->d[0] /* x */, r->d[1] /* y */, r->d[2] /* mode/Mouse* */ );
			break;

		// SPEEDUP functions

		case 4: // expandArea:
			/*
			 * expand_area
			 * In:	a1	VDI struct, destination MFDB, VDI struct, source MFDB
			 *	d0	height and width to move (high and low word)
			 *	d1-d2	source coordinates
			 *	d3-d4	destination coordinates
			 *	d6	background and foreground colour
			 *	d7	logic operation
			 */
			//			D(bug("fVDI: %s %x %d,%d:%d,%d (%d,%d)", "expandArea", r->d[7], r->d[1], r->d[2], r->d[3], r->d[4], r->d[6] & 0xffff, r->d[6] >> 16 ));
			r->d[0] = expandArea( (void*)get_long( (uint32)r->a[1], true),
								  (MFDB*)get_long( (uint32)r->a[1] + 12, true ) /* src MFDB* */,
								  (MFDB*)get_long( (uint32)r->a[1] + 4, true ) /* dest MFDB* */,
								  r->d[1] /* sx */, r->d[2] /* sy */,
								  r->d[3] /* dx */, r->d[4] /* dy */,
								  r->d[0] & 0xffff /* w */, r->d[0] >> 16 /* h */,
								  r->d[6] & 0xffff /* fgcolor */, r->d[6] >> 16 /* bgcolor */,
								  r->d[7] & 0xffff /* logical operation */ );
			break;

		case 5: // fillArea
			/*
			 * fill_area
			 * In:	a1	VDI struct
			 *	d0	height and width to fill (high and low word)
			 *	d1	x or table address
			 *	d2	y or table length (high) and type (low)
			 *	d3	pattern address
			 *	d4	colour
			 */
			if ( ( r->d[2] >> 16 ) != 0 )
				r->d[0] = (uint32)-1; // we can do only the basic single mode
			else {
				uint16 pattern[16];
				uint32 fPatterAddress = r->d[3];
				for( uint16 i=0; i<=30; i+=2 )
					pattern[i >> 1] = get_word( fPatterAddress + i, true );

				r->d[0] = fillArea( (void*)r->a[1],
									r->d[1] & 0xffff /* x */, r->d[2] & 0xffff /* y */,
									r->d[0] & 0xffff /* w */, r->d[0] >> 16 /* h */,
									pattern /* pattern */,
									r->d[4] & 0xffff /* fgColor */,	r->d[4] >> 16 /* bgColor */ );
			}
			break;

		case 6: // blitArea
			/*
			 * blit_area
			 * In:	a1	VDI struct, destination MFDB, VDI struct, source MFDB
			 *	d0	height and width to move (high and low word)
			 *	d1-d2	source coordinates
			 *	d3-d4	destination coordinates
			 *	d5	logic operation
			 */
			r->d[0] = blitArea( (void*)get_long( (uint32)r->a[1], true),
								(MFDB*)get_long( (uint32)r->a[1] + 12, true ) /* src MFDB* */,
								(MFDB*)get_long( (uint32)r->a[1] + 4, true ) /* dest MFDB* */,
								r->d[1] /* sx */, r->d[2] /* sy */,
								r->d[3] /* dx */, r->d[4] /* dy */,
								r->d[0] & 0xffff /* w */, r->d[0] >> 16 /* h */,
								r->d[5] & 0xffff /* logical operation */ );
			break;

		case 7: // drawLine:
			/*
			 * draw_line
			 * In:	a1	VDI struct
			 *	d0	logic operation
			 *	d1	x1 or table address
			 *	d2	y1 or table length (high) and type (low)
			 *	d3	x2 or move point count
			 *	d4	y2 or move index address
			 *	d5	pattern
			 *	d6	colour
			 */
			if ( ( r->d[2] >> 16 ) != 0 )
				r->d[0] = (uint32)-1; // we can do only one line at once
			else {
				r->d[0] = drawLine( (void*)r->a[1],
									r->d[1] /* x1 */, r->d[2] /* y1 */,
									r->d[3] /* x2 */, r->d[4] /* y2 */,
									r->d[5] & 0xffff /* pattern */,
									r->d[6] & 0xffff /* fgColor */,	r->d[6] >> 16 /* bgColor */,
									r->d[0] & 0xffff /* logical operation */ );
			}
			break;

		case 9: // setColor:
			setColor( get_long( (uint32)r->a[7] + 4, true ), get_long( (uint32)r->a[7] + 8, true ) );
			break;

		// not implemented functions
		default:
			D(bug("fVDI: Unknown %d", fncode));
			r->d[0] = 1;
    }

#ifdef DEBUG_DRAW_PALETTE
	uint16 ptrn[16] = {
		0xffff, 0xffff, 0xffff, 0xffff,
		0xffff, 0xffff, 0xffff, 0xffff,
		0xffff, 0xffff, 0xffff, 0xffff,
		0xffff, 0xffff, 0xffff, 0xffff
	};
	for( int i=0; i<16; i++ )
		hostScreen.fillArea( i<<4, hostScreen.getHeight()-16, (i<<4) + 15, hostScreen.getHeight(), ptrn, hostScreen.getPaletteColor( i ), hostScreen.getPaletteColor( 0 ) );
#endif  // DEBUG_DRAW_PALETTE

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
    if ((uint32)vwk & 1)
		return 0;

    if (!dst || !dst->address) {
		//  Workstation *wk;
		//wk = vwk->real_address;
		//|| (dst->address == wk->screen.mfdb.address)) {

		//D(bug("\nfVDI: %s %d,%d (%d %08x)", "write_pixel", (uint16)x, (uint16)y, (uint32)color, (uint32)color));
		if (!hostScreen.renderBegin())
			return 1;
		hostScreen.putPixel( x, y, color );
		hostScreen.renderEnd();
		hostScreen.update( (int16)x, (int16)y, 1, 1, true );

    } else {
		D(bug("fVDI: %s %d,%d (%d)", "write_pixel no screen", (uint16)x, (uint16)y, (uint32)color));
		//    uint32 offset;

		/* WRONG CODE!!!
		offset = (dst->wdwidth * 2 * dst->bitplanes) * y + x * sizeof(uint16);
		put_word( (uint32)dst->address + offset, color );
		*/
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
    uint32 color = 0;

    if (!src || !src->address) {
		//|| (src->address == wk->screen.mfdb.address)) {

		if (!hostScreen.renderBegin())
			return 0;
		color = hostScreen.getPixel( x, y );
		hostScreen.renderEnd();
    } else {
		D(bug("fVDI: %s %d,%d (%d)", "read_pixel no screen", (uint16)x, (uint16)y, (uint32)color));
		//    uint32 offset;

		/* WRONG CODE!!!
		offset = (src->wdwidth * 2 * src->bitplanes) * y + x * sizeof(uint16);
		color = get_word((uint32)src->address + offset, true);
		*/
    }

    return color;
}


/**
 * Set palette colour (hooked into the c_set_colors driver function by STanda)
 *
 * set_color_hook
 *	4(a7)	paletteIndex
 *	8(a7)   5+6+4 FalconTC RGB color value
 **/
void FVDIDriver::setColor( uint32 paletteIndex, uint32 color ) {
	D(bug("fVDI: setColor: %03d,%x - %x,%x,%x", paletteIndex, color,
			  (uint8) ((color >> 8) & 0xf8),
			  (uint8) ((color >> 3) & 0xf8),
			  (uint8) (color & 0x1f) << 3));

	hostScreen.setPaletteColor( paletteIndex,
								(uint8)((color >> 8) & 0xf8),
								(uint8)((color >> 3) & 0xfc),
								(uint8)(color & 0x1f) << 3);
}


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
	D(bug("fVDI: saveMouseBackground: %d,%d,%d", x, y, save ));

	if (!hostScreen.renderBegin())
		return;

	if ( save ) {
		for( uint16 i=0; i<16; i++ )
			for( uint16 j=0; j<16; j++ ) {
				Mouse.storedBackround[i][j] =	hostScreen.getPixel(x + j, y + i);
			}

		hostScreen.renderEnd();
	} else {
		for( uint16 i=0; i<16; i++ )
			for( uint16 j=0; j<16; j++ )
				hostScreen.putPixel(x + j, y + i, Mouse.storedBackround[i][j] );

		hostScreen.renderEnd();
		hostScreen.update( x, y, 16, 16, true );
	}
}

uint32 FVDIDriver::drawMouse( void *wrk, int32 x, int32 y, uint32 mode ) {
	//	uint8 r,g,b,a; SDL_GetRGBA( color, surf->format, &r, &g, &b, &a);

	D(bug("fVDI: mouse mode: %x", mode ));

	switch ( mode ) {
		case 0:  // move shown
			saveMouseBackground( Mouse.storedX, Mouse.storedY, false ); // restore
			saveMouseBackground( Mouse.storedX = x, Mouse.storedY = y, true ); // save
			break;
		case 1:  // move hidden
			return 1;
		case 2:  // hide
			saveMouseBackground( Mouse.storedX, Mouse.storedY, false ); // restore
			return 1;
		case 3:  // show
			saveMouseBackground( Mouse.storedX = x, Mouse.storedY = y, true ); // save
			break;

		default: // change pointer shape
			uint32 fPatterAddress = mode + sizeof( uint16 ) * 9;
			for( uint16 i=0; i<=30; i+=2 )
				Mouse.mask[i >> 1] = reverse_bits( get_word( fPatterAddress + i, true ) );
			fPatterAddress += 30;
			for( uint16 i=0; i<=30; i+=2 )
				Mouse.shape[i >> 1] = reverse_bits( get_word( fPatterAddress + i, true ) );

#if DEBUG > 1
			char buffer[30];
			for( uint16 i=0; i<=15; i++ ) {
				getBinary( Mouse.mask[i], buffer );
				D2(bug("fVDI: apm:%s", buffer ));
			}
			for( uint16 i=0; i<=15; i++ ) {
				getBinary( Mouse.shape[i], buffer );
				D2(bug("fVDI: apd:%s", buffer ));
			}
#endif // DEBUG == 1

			return 1;
	}

	uint16 mm[16];
	uint16 md[16];
	uint16 shift = x & 0xf;
	for( uint16 i=0; i<=15; i++ ) {
		mm[(i + (y&0xf)) & 0xf] = ( Mouse.mask[i]  << shift ) | ((Mouse.mask[i]  >> (16-shift)) & ((1<<shift)-1) );
		md[(i + (y&0xf)) & 0xf] = ( Mouse.shape[i] << shift ) | ((Mouse.shape[i] >> (16-shift)) & ((1<<shift)-1) );
	}

	if (!hostScreen.renderBegin())
		return 1;

	D(bug("fVDI: mouse x,y: %d,%d", (uint16)x, (uint16)y ));

	hostScreen.fillArea( x, y, x + 15, y + 15, mm, hostScreen.getColor( 0xfe, 0xfe, 0xfe ) /* white */ );
	hostScreen.fillArea( x, y, x + 15, y + 15, md, hostScreen.getColor( 0, 0, 0 ) /* black */ );

	hostScreen.renderEnd();
	hostScreen.update( (uint16)x, (uint16)y, 16, 16, true );

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
extern "C" {
	void keypress() {
		SDL_Event event;
		int quit = 0;
		while(! quit) {
			while (SDL_PollEvent(&event)) {
				if (event.type == SDL_KEYDOWN) {
					quit = 1;
					break;
				}
			}
		}
	}
}

uint32 FVDIDriver::expandArea(void *vwk, MFDB *src, MFDB *dest, int32 sx, int32 sy, int32 dx, int32 dy, int32 w, int32 h,
							  uint32 fgColor, uint32 bgColor, uint32 logOp)
{

	D(bug("fVDI: %s %x %d,%d:%d,%d:%d,%d (%d, %d)", "expandArea", logOp, sx, sy, dx, dy, w, h, fgColor, bgColor ));
	D(bug("fVDI: %s %x,%x : %x,%x", "expandArea - MFDB addresses", src, dest, get_long( (uint32)src, true ),get_long( (uint32)dest, true )));

	uint16 pitch = get_word( (uint32)src + 8, true ) << 1; // MFDB *src->wdwidth << 1 // the byte width (always monochrom);
	uint32 data  = get_long( (uint32)src, true ) + sy*pitch; // MFDB *src->address;

	D(bug("fVDI: %s %x, %d, %d", "expandArea - src: data address, MFDB wdwidth << 1, bitplanes", data, pitch, get_word( (uint32)src + 12, true )));

	if ( dest != NULL && get_long( (uint32)dest, true ) != 0 )
		return 1; // FIXME this is the blitToMemory NOT IMPLEMENTED YET!

	fgColor = hostScreen.getPaletteColor( fgColor );
	bgColor = hostScreen.getPaletteColor( bgColor );

	if (!hostScreen.renderBegin())
		return 1;

	for( uint16 j=0; j<h; j++ ) {
		D2(fprintf(stderr,"fVDI: bmp:"));

		uint16 theWord = get_word(data + j*pitch + ((sx>>3)&0xfffe), true);
		for( uint16 i=sx; i<sx+w; i++ ) {
			if ( i % 16 == 0 )
				theWord = get_word(data + j*pitch + ((i>>3)&0xfffe), true);

			D2(fprintf(stderr,"%s", ((theWord >> (15-(i&0xf))) & 1) ? "1" : " " ));
			switch( logOp ) {
				case 1:
					hostScreen.putPixel(dx + i - sx, dy + j, ((theWord >> (15-(i&0xf))) & 1) ? fgColor : bgColor );
					break;
				case 2:
					if ((theWord >> (15-(i&0xf))) & 1)
						hostScreen.putPixel(dx + i - sx, dy + j, fgColor );
					break;
				case 3:
					if ((theWord >> (15-(i&0xf))) & 1)
						hostScreen.putPixel(dx + i - sx, dy + j, ~ hostScreen.getPixel(dx + i - sx, dy + j) );
					break;
				case 4:
					if ( !((theWord >> (15-(i&0xf))) & 1) )
						hostScreen.putPixel(dx + i - sx, dy + j, fgColor );
					break;
			}
		}
		D2(bug("")); //newline
	}

	hostScreen.renderEnd();
	hostScreen.update( dx, dy, w, h, true );

	return 1;
}


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
uint32 FVDIDriver::fillArea(void *vwk, int32 x, int32 y, int32 w, int32 h, uint16 *pattern, uint32 fgColor, uint32 bgColor)
{
	D(bug("fVDI: %s %d,%d:%d,%d p:%x, (fgc:%d /%x/ : bgc:%d /%x/)", "fillArea", x, y, x+w-1, y+h-1, *pattern, fgColor, hostScreen.getPaletteColor( fgColor ), bgColor, hostScreen.getPaletteColor( bgColor ) ));

	fgColor = hostScreen.getPaletteColor( fgColor );
	bgColor = hostScreen.getPaletteColor( bgColor );
	uint16 logOp = get_word( (uint32)vwk + 90, true ); // Virtual *vwk->mode // fill logOp;

	if (!hostScreen.renderBegin())
		return 1;

	hostScreen.fillArea( x, y, x + w - 1, y + h - 1, pattern, fgColor, bgColor, logOp );

	hostScreen.renderEnd();
	hostScreen.update( x, y, w, h, true );

	//	keypress();

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
uint32 FVDIDriver::blitArea(void *vwk, MFDB *src, MFDB *dest, int32 sx, int32 sy, int32 dx, int32 dy, int32 w, int32 h,
							uint32 logOp)
{
	D(bug("fVDI: %s %x %d,%d:%d,%d:%d,%d", "blitArea", logOp, sx, sy, dx, dy, w, h ));
	D(bug("fVDI: %s %x,%x : %x,%x", "blitArea - MFDB addresses", src, dest, get_long( (uint32)src, true ),get_long( (uint32)dest, true )));

	if ( dest != NULL && get_long( (uint32)dest, true ) != 0 ) {
		D(bug("fVDI: blitArea to memory NOT IMPLEMENTED"));
		return 1; // FIXME this is the blitToMemory NOT IMPLEMENTED YET!
	}
	if ( src != NULL && get_long( (uint32)src, true ) != 0 ) {
		D(bug("fVDI: blitArea from memory NOT IMPLEMENTED"));

		/*
		uint16 *fvram = (uint16*)get_long( (uint32)src, true );
		uint16 bpp = get_word( (uint32)src + 12, true );
		if ( bpp < 16 ) {
		}

		for( uint16 j=0; j<h; j++ ) {
			D2(fprintf(stderr,"fVDI: bmp:"));

			uint32 wordIndex = data + j*pitch + ((sx>>3)&0xfffe);
			for( uint16 i=sx; i<sx+w; i++ ) {
				if ( i % 16 == 0 ) {
					uint32 wordIndex = data + j*pitch + ((sx>>3)&0xfffe);
					// convert the word into the chunky
				}
			}
		}
		*/

		return 1; // FIXME this is the blitFromMemory NOT IMPLEMENTED YET!
	}

	D(bug("fVDI: %s ", "blitArea - screen blit!" ));

	// if (!hostScreen.renderBegin()) // the surface must _not_ be locked for blitArea (SDL_BlitSurface)
	//		return 1;

	hostScreen.blitArea( sx, sy, dx, dy, w, h );
	hostScreen.update( sx, sy, w, h, true );
	hostScreen.update( dx, dy, w, h, true );

	return 1;
}


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
uint32 FVDIDriver::drawLine(void *vwk, int32 x1, int32 y1, int32 x2, int32 y2, 
							uint16 pattern, uint32 fgColor, uint32 bgColor, uint32 logOp )
{
	D(bug("fVDI: %s %x %d,%d:%d,%d p:%x, (fgc:%d /%x/ : bgc:%d /%x/)", "drawLine", logOp, x1, y1, x2, y2, pattern, fgColor, hostScreen.getPaletteColor( fgColor ), bgColor, hostScreen.getPaletteColor( bgColor ) ));
	fgColor = hostScreen.getPaletteColor( fgColor );
	bgColor = hostScreen.getPaletteColor( bgColor );

	hostScreen.drawLine( x1, y1, x2, y2, pattern, fgColor, bgColor, logOp );

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
 * VIDEL got the doRender flag with set
ter setRendering().
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
