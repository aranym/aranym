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


// The Atari structures offsets
#define MOUSE_HOTSPOT_X            8
#define MOUSE_HOTSPOT_Y            10
#define MOUSE_BGCOLOR              12
#define MOUSE_FGCOLOR              14
#define MOUSE_MASK                 16
#define MOUSE_SHAPE                48
#define MFDB_WDWIDTH                8
#define MFDB_BITPLANES             12
#define VWK_SCREEN_MFDB_ADDRESS    24
#define VWK_CLIP_RECT              80
#define VWK_MODE                   90


#if SDL_BYTEORDER == SDL_BIG_ENDIAN

#define put_dtriplet( address, data ) \
{ \
	put_byte((address)  , ((data) >> 16) & 0xff ); \
	put_byte((address)+1, ((data) >> 8) & 0xff ); \
	put_byte((address)+2, (data) & 0xff ); \
}

#define get_dtriplet( address, dataFlag ) \
	( (get_byte((address),dataFlag) << 16) | (get_byte((address)+1,dataFlag) << 8) | get_byte((address)+2,dataFlag) )

#else

#define put_dtriplet( address, data ) \
{ \
	put_byte((address)  , (data) & 0xff ); \
	put_byte((address)+1, ((data) >> 8) & 0xff ); \
	put_byte((address)+2, ((data) >> 16) & 0xff ); \
}

#define get_dtriplet( address, dataFlag ) \
	( (get_byte((address)+2,dataFlag) << 16) | (get_byte((address)+1,dataFlag) << 8) | get_byte((address),dataFlag) )

#endif // SDL_BYTEORDER == SDL_BIG_ENDIAN



void FVDIDriver::dispatch( uint32 fncode, M68kRegisters *r )
{
	MFDB dst;
	MFDB src;

	// fix the stack (the fncode was pushed onto the stack)
	r->a[7] += 4;

	videl.setRendering( false );

	switch (fncode) {
		// NEEDED functions

		case 1: // getPixel:
			/*
			 * read_pixel
			 * In:  a1  VDI struct, source MFDB
			 *  d1  x
			 *  d2  y
			 */
			D(bug("fVDI: %s %d,%d", "readPixel", (uint16)r->d[1], (uint16)r->d[2] ));
			r->d[0] = getPixel( (void*)r->a[1],
								fetchMFDB( &src, get_long( (uint32)r->a[1] + 4, true ) ),
								r->d[1] /* x */, r->d[2] /* y */);
			break;

		case 2: // putPixel:
			/*
			 * write_pixel
			 * In:  a1  VDI struct, destination MFDB
			 *  d0  colour
			 *  d1  x or table address
			 *  d2  y or table length (high) and type (low)
			 * XXX: ?
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
			 * In:  a1  Pointer to Workstation struct
			 *  d0/d1   x,y
			 *  d2  0 - move shown  1 - move hidden  2 - hide  3 - show  >3 - change shape (pointer to mouse struct)
			 */
			r->d[0] = drawMouse( (void*)r->a[1],
								 (int16)r->d[0] /* x */, (int16)r->d[1] /* y */, r->d[2] /* mode/Mouse* */ );
			break;

		// SPEEDUP functions

		case 4: // expandArea:
			/*
			 * expand_area
			 * In:  a1  VDI struct, destination MFDB, VDI struct, source MFDB
			 *  d0  height and width to move (high and low word)
			 *  d1-d2   source coordinates
			 *  d3-d4   destination coordinates
			 *  d6  background and foreground colour
			 *  d7  logic operation
			 */

			if (r->a[1] & 1)
				r->d[0] = (uint32)-1; // we can do only one line at once
			else
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
			 * In:  a1  VDI struct
			 *  d0  height and width to fill (high and low word)
			 *  d1  x or table address
			 *  d2  y or table length (high) and type (low)
			 *  d3  pattern address
			 *  d4  colour
			 */
			if (r->a[1] & 1)
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
									r->d[4] & 0xffff /* fgColor */, r->d[4] >> 16 /* bgColor */ );
			}
			break;

		case 6: // blitArea
			/*
			 * blit_area
			 * In:  a1  VDI struct, destination MFDB, VDI struct, source MFDB
			 *  d0  height and width to move (high and low word)
			 *  d1-d2   source coordinates
			 *  d3-d4   destination coordinates
			 *  d7  logic operation
			 */
			r->d[0] = blitArea( (void*)get_long( (uint32)r->a[1], true),
								(MFDB*)get_long( (uint32)r->a[1] + 12, true ) /* src MFDB* */,
								(MFDB*)get_long( (uint32)r->a[1] + 4, true ) /* dest MFDB* */,
								r->d[1] /* sx */, r->d[2] /* sy */,
								r->d[3] /* dx */, r->d[4] /* dy */,
								r->d[0] & 0xffff /* w */, r->d[0] >> 16 /* h */,
								r->d[7] & 0xffff /* logical operation */ );
			break;

		case 7: // drawLine:
			/*
			 * draw_line
			 * In:  a1  VDI struct
			 *  d0  logic operation
			 *  d1  x1 or table address
			 *  d2  y1 or table length (high) and type (low)
			 *  d3  x2 or move point count
			 *  d4  y2 or move index address
			 *  d5  pattern
			 *  d6  colour
			 */
			if (r->a[1] & 1)
				r->d[0] = (uint32)-1; // we can do only one line at once
			else {
				r->d[0] = drawLine( (void*)r->a[1],
									r->d[1] /* x1 */, r->d[2] /* y1 */,
									r->d[3] /* x2 */, r->d[4] /* y2 */,
									r->d[5] & 0xffff /* pattern */,
									r->d[6] & 0xffff /* fgColor */, r->d[6] >> 16 /* bgColor */,
									r->d[0] & 0xffff /* logical operation */ );
			}
			break;

		case 9: // setColor:
			setColor( get_long( (uint32)r->a[7] + 4, true ), get_word( (uint32)r->a[7] + 8, true ),
					  get_word( (uint32)r->a[7] + 10, true ), get_word( (uint32)r->a[7] + 12, true ) );
			break;

		case 10: // setResolution:
			setResolution( get_long( (uint32)r->a[7] + 4, true ), get_long( (uint32)r->a[7] + 8, true ),
						   get_long( (uint32)r->a[7] + 12, true ), get_long( (uint32)r->a[7] + 16, true ) );
			break;

		case 20: // debug_aranym:
			bug("fVDI: DEBUG %d", get_long( (uint32)r->a[7] + 4, true ) );
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
		hostScreen.fillArea( i<<4, hostScreen.getHeight()-16, 15, 16, ptrn, hostScreen.getPaletteColor( i ), hostScreen.getPaletteColor( 0 ) );
#endif  // DEBUG_DRAW_PALETTE

}


FVDIDriver::MFDB* FVDIDriver::fetchMFDB( FVDIDriver::MFDB* mfdb, uint32 pmfdb )
{
	if ( pmfdb != 0 ) {
		mfdb->address = get_long( pmfdb, true );
		return mfdb;
	}

	return NULL;
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
uint32 FVDIDriver::putPixel(void *vwk, MFDB *dst, int32 x, int32 y, uint32 color)
{
	if ((uint32)vwk & 1)
		return 0;

	if (!dst || !dst->address || dst->address==get_long( get_long( (uint32)vwk, true ) + VWK_SCREEN_MFDB_ADDRESS, true )) {
		// To screen
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
uint32 FVDIDriver::getPixel(void *vwk, MFDB *src, int32 x, int32 y)
{
	uint32 color = 0;

	if (!src || !src->address || src->address==get_long( get_long( (uint32)vwk, true ) + VWK_SCREEN_MFDB_ADDRESS, true )) {
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
 *  4(a7)   paletteIndex
 *  8(a7)   red component byte value
 *  10(a7)  green component byte value
 *  12(a7)  blue component byte value
 **/
void FVDIDriver::setColor( uint32 paletteIndex, uint32 red, uint32 green, uint32 blue )
{
	D(bug("fVDI: setColor: %03d - %3d,%3d,%3d - %02x,%02x,%02x", paletteIndex, red, green, blue, (uint8)(((red<<8)-1)/1000), (uint8)(((green<<8)-1)/1000), (uint8)(((blue<<8)-1)/1000) ));

	if ( hostScreen.getBpp() == 2 ) // 5+6+5 rounding needed
		hostScreen.setPaletteColor( paletteIndex,
									((red * ((1L << 5) - 1) + 500L) / 1000) << 3,
									((green * ((1L << 6) - 1) + 500L) / 1000) << 2,
									((blue * ((1L << 5) - 1) + 500L) / 1000) << 3 );
	else // 8+8+8 graphics
		hostScreen.setPaletteColor( paletteIndex,
									((red * ((1L << 8) - 1) + 500L) / 1000),
									((green * ((1L << 8) - 1) + 500L) / 1000),
									((blue * ((1L << 8) - 1) + 500L) / 1000) );

	/*
	  (uint8)((color >> 8) & 0xf8),
	  (uint8)((color >> 3) & 0xfc),
	  (uint8)(color & 0x1f) << 3);
	*/
}


void FVDIDriver::setResolution( int32 width, int32 height, int32 depth, int32 freq )
{
	D(bug("fVDI: setResolution: %dx%dx%d@%d", width, height, depth, freq ));
	hostScreen.setWindowSize( width, height, depth );
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
extern "C" {
#if DEBUG > 0
	static void getBinary( uint16 data, char *buffer ) {
		for( uint16 i=0; i<=15; i++ ) {
			buffer[i] = (data & 1)?'1':' ';
			data>>=1;
		}
		buffer[16]='\0';
	}
#endif
	static uint16 reverse_bits( uint16 data ) {
		uint16 res = 0;
		for( uint16 i=0; i<=15; i++ )
			res |= (( data >> i ) & 1 ) << ( 15 - i );
		return res;
	}
}


void FVDIDriver::restoreMouseBackground()
{
	int16 x = Mouse.storage.x;
	int16 y = Mouse.storage.y;

	if (!hostScreen.renderBegin())
		return;

	for( uint16 i=0; i<Mouse.storage.height; i++ )
		for( uint16 j=0; j<Mouse.storage.width; j++ )
			hostScreen.putPixel(x + j, y + i, Mouse.storage.background[i][j] );

	hostScreen.renderEnd();
	hostScreen.update( x, y, Mouse.storage.width, Mouse.storage.height, true );
}


void FVDIDriver::saveMouseBackground( int16 x, int16 y, int16 width, int16 height )
{
	D2(bug("fVDI: saveMouseBackground: %d,%d,%d,%d", x, y, width, height ));

	if (!hostScreen.renderBegin())
		return;

	for( uint16 i=0; i<height; i++ )
		for( uint16 j=0; j<width; j++ ) {
			Mouse.storage.background[i][j] = hostScreen.getPixel(x + j, y + i);
		}

	hostScreen.renderEnd();

	Mouse.storage.x = x;
	Mouse.storage.y = y;
	Mouse.storage.height = height;
	Mouse.storage.width = width;
}


uint32 FVDIDriver::drawMouse( void *wrk, int16 x, int16 y, uint32 mode )
{
	D2(bug("fVDI: mouse mode: %x", mode ));

	switch ( mode ) {
		case 0:  // move shown
			restoreMouseBackground();
			break;
		case 1:  // move hidden
			return 1;
		case 2:  // hide
			restoreMouseBackground();
			return 1;
		case 3:  // show
			break;

		default: // change pointer shape
			uint32 fPatterAddress = mode + MOUSE_MASK;
			for( uint16 i=0; i<32; i+=2 )
				Mouse.mask[i >> 1] = reverse_bits( get_word( fPatterAddress + i, true ) );
			fPatterAddress  = mode + MOUSE_SHAPE;
			for( uint16 i=0; i<32; i+=2 )
				Mouse.shape[i >> 1] = reverse_bits( get_word( fPatterAddress + i, true ) );

			Mouse.storage.color.foreground = hostScreen.getPaletteColor( get_word( mode + MOUSE_FGCOLOR, true ) );
			Mouse.storage.color.background = hostScreen.getPaletteColor( get_word( mode + MOUSE_BGCOLOR, true ) );
			Mouse.hotspot.x = get_word( mode + MOUSE_HOTSPOT_X, true );
			Mouse.hotspot.y = get_word( mode + MOUSE_HOTSPOT_Y, true );

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

	// handle the mouse hotspot point
	x -= Mouse.hotspot.x;
	y -= Mouse.hotspot.y;

	// roll the pattern properly
	uint16 mm[16];
	uint16 md[16];
	uint16 shift = x & 0xf;
	for( uint16 i=0; i<=15; i++ ) {
		mm[(i + (y&0xf)) & 0xf] = ( Mouse.mask[i]  << shift ) | ((Mouse.mask[i]  >> (16-shift)) & ((1<<shift)-1) );
		md[(i + (y&0xf)) & 0xf] = ( Mouse.shape[i] << shift ) | ((Mouse.shape[i] >> (16-shift)) & ((1<<shift)-1) );
	}

	// beware of the edges of the screen
	int16 w, h;

	if ( x < 0 ) {
		w = 16 + x;
		x = 0;
	} else {
		w = 16;
		if ( (int16)x + 16 >= (int32)hostScreen.getWidth() )
			w = hostScreen.getWidth() - (int16)x;
	}
	if ( y < 0 ) {
		h = 16 + y;
		y = 0;
	} else {
		h = 16;
		if ( (int16)y + 16 >= (int32)hostScreen.getHeight() )
			h = hostScreen.getHeight() - (int16)y;
	}

	D2(bug("fVDI: mouse x,y: %d,%d,%d,%d (%x,%x)", x, y, w, h,
		   Mouse.storage.color.background, Mouse.storage.color.foreground ));

	// draw the mouse
	if (!hostScreen.renderBegin())
		return 1;

	saveMouseBackground( x, y, w, h );

	hostScreen.fillArea( x, y, w, h, mm, Mouse.storage.color.background );
	hostScreen.fillArea( x, y, w, h, md, Mouse.storage.color.foreground );

	hostScreen.renderEnd();
	hostScreen.update( (uint16)x, (uint16)y, w, h, true );

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
	D2(bug("fVDI: %s %x,%x : %x,%x", "expandArea - MFDB addresses", src, dest, get_long( (uint32)src, true ),get_long( (uint32)dest, true )));

	uint16 pitch = get_word( (uint32)src + MFDB_WDWIDTH, true ) << 1; // the byte width (always monochrom);
	uint32 data  = get_long( (uint32)src, true ) + sy*pitch; // MFDB *src->address;

	D2(bug("fVDI: %s %x, %d, %d", "expandArea - src: data address, MFDB wdwidth << 1, bitplanes", data, pitch, get_word( (uint32)src + MFDB_BITPLANES, true )));
	D2(bug("fVDI: %s %x, %d, %d", "expandArea - dst: data address, MFDB wdwidth << 1, bitplanes", get_long( (uint32)dest, true ), get_word( (uint32)dest + MFDB_WDWIDTH, true ) * (get_word( (uint32)dest + MFDB_BITPLANES, true )>>2), get_word( (uint32)dest + MFDB_BITPLANES, true )));

	fgColor = hostScreen.getPaletteColor( fgColor );
	bgColor = hostScreen.getPaletteColor( bgColor );

	if ( dest != NULL && // no MFDB structure
		 get_long( (uint32)dest, true ) != 0 && // mfdb->address == 0 => screen
		 get_long( (uint32)dest, true ) != get_long( get_long( (uint32)vwk, true ) + VWK_SCREEN_MFDB_ADDRESS, true ) ) {
		// mfdb->address = videoramstart

		D(bug("fVDI: expandArea M->M"));

		uint32 destPlanes  = (uint32)get_word( (uint32)dest + MFDB_BITPLANES, true );
		uint32 destPitch   = get_word( (uint32)dest + MFDB_WDWIDTH, true ) * destPlanes << 1; // MFDB *dest->pitch
		uint32 destAddress = get_long( (uint32)dest, true );

		switch( destPlanes ) {
			case 16:
				for( uint16 j=0; j<h; j++ ) {
					D2(fprintf(stderr,"fVDI: bmp:"));

					uint16 theWord = get_word(data + j*pitch + ((sx>>3)&0xfffe), true);
					for( uint16 i=sx; i<sx+w; i++ ) {
						if ( i % 16 == 0 )
							theWord = get_word(data + j*pitch + ((i>>3)&0xfffe), true);

						D2(fprintf(stderr,"%s", ((theWord >> (15-(i&0xf))) & 1) ? "1" : " " ));
						switch( logOp ) {
							case 1:
								put_word(destAddress + ((dx+i-sx)<<1) + (dy+j)*destPitch, ((theWord >> (15-(i&0xf))) & 1) ? fgColor : bgColor );
								break;
							case 2:
								if ((theWord >> (15-(i&0xf))) & 1)
									put_word(destAddress + ((dx+i-sx)<<1) + (dy+j)*destPitch, fgColor );
								break;
							case 3:
								if ((theWord >> (15-(i&0xf))) & 1)
									put_word(destAddress + ((dx+i-sx)<<1) + (dy+j)*destPitch, ~ get_word(destAddress + ((dx+i-sx)<<1) + (dy+j)*destPitch, true ) );
								break;
							case 4:
								if ( !((theWord >> (15-(i&0xf))) & 1) )
									put_word(destAddress + ((dx+i-sx)<<1) + (dy+j)*destPitch, fgColor );
								break;
						}
					}
					D2(bug("")); //newline
				}
				break;
			case 24:
				for( uint16 j=0; j<h; j++ ) {
					D2(fprintf(stderr,"fVDI: bmp:"));

					uint16 theWord = get_word(data + j*pitch + ((sx>>3)&0xfffe), true);
					for( uint16 i=sx; i<sx+w; i++ ) {
						if ( i % 16 == 0 )
							theWord = get_word(data + j*pitch + ((i>>3)&0xfffe), true);

						D2(fprintf(stderr,"%s", ((theWord >> (15-(i&0xf))) & 1) ? "1" : " " ));
						uint16 xoffset = ((dx+i-sx)<<1)+(dx+i-sx);
						switch( logOp ) {
							case 1:
								put_dtriplet(destAddress + xoffset + (dy+j)*destPitch, ((theWord >> (15-(i&0xf))) & 1) ? fgColor : bgColor );
								break;
							case 2:
								if ((theWord >> (15-(i&0xf))) & 1)
									put_dtriplet(destAddress + xoffset + (dy+j)*destPitch, fgColor );
								break;
							case 3:
								if ((theWord >> (15-(i&0xf))) & 1)
									put_dtriplet(destAddress + xoffset + (dy+j)*destPitch, ~ get_word(destAddress + ((dx+i-sx)<<1) + (dy+j)*destPitch, true ) );
								break;
							case 4:
								if ( !((theWord >> (15-(i&0xf))) & 1) )
									put_dtriplet(destAddress + xoffset + (dy+j)*destPitch, fgColor );
								break;
						}
					}
					D2(bug("")); //newline
				}
				break;
			case 32:
				for( uint16 j=0; j<h; j++ ) {
					D2(fprintf(stderr,"fVDI: bmp:"));

					uint16 theWord = get_word(data + j*pitch + ((sx>>3)&0xfffe), true);
					for( uint16 i=sx; i<sx+w; i++ ) {
						if ( i % 16 == 0 )
							theWord = get_word(data + j*pitch + ((i>>3)&0xfffe), true);

						D2(fprintf(stderr,"%s", ((theWord >> (15-(i&0xf))) & 1) ? "1" : " " ));
						switch( logOp ) {
							case 1:
								put_long(destAddress + ((dx+i-sx)<<2) + (dy+j)*destPitch, ((theWord >> (15-(i&0xf))) & 1) ? fgColor : bgColor );
								break;
							case 2:
								if ((theWord >> (15-(i&0xf))) & 1)
									put_long(destAddress + ((dx+i-sx)<<2) + (dy+j)*destPitch, fgColor );
								break;
							case 3:
								if ((theWord >> (15-(i&0xf))) & 1)
									put_long(destAddress + ((dx+i-sx)<<2) + (dy+j)*destPitch, ~ get_word(destAddress + ((dx+i-sx)<<1) + (dy+j)*destPitch, true ) );
								break;
							case 4:
								if ( !((theWord >> (15-(i&0xf))) & 1) )
									put_long(destAddress + ((dx+i-sx)<<2) + (dy+j)*destPitch, fgColor );
								break;
						}
					}
					D2(bug("")); //newline
				}
				break;
		}

		return 1;
	}

	if (!hostScreen.renderBegin())
		return 1;

	D2(bug("fVDI: expandArea M->S"));
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
uint32 FVDIDriver::fillArea(void *vwk, int32 x, int32 y, int32 w, int32 h, uint16 *pattern, uint32 fgColor, uint32 bgColor)
{
	uint16 logOp = get_word( (uint32)vwk + VWK_MODE, true ); // Virtual *vwk->mode // fill logOp;
	D(bug("fVDI: %s %d %d,%d:%d,%d : %d,%d p:%x, (fgc:%d /%x/ : bgc:%d /%x/)", "fillArea", logOp, x, y, w, h, x+w-1, x+h-1, *pattern, fgColor, hostScreen.getPaletteColor( fgColor ), bgColor, hostScreen.getPaletteColor( bgColor ) ));

	fgColor = hostScreen.getPaletteColor( fgColor );
	bgColor = hostScreen.getPaletteColor( bgColor );

	if (!hostScreen.renderBegin())
		return 1;

	hostScreen.fillArea( x, y, w, h, pattern, fgColor, bgColor, logOp );

	hostScreen.renderEnd();
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
#define applyBlitLogOperation( logicalOperation, destinationData, sourceData ) \
	switch(logicalOperation) { \
		case 0: \
			destinationData = 0; \
			break; \
		case 1: \
			destinationData = sourceData & destinationData; \
			break; \
		case 2: \
			destinationData = sourceData & ~destinationData; \
			break; \
		case 3: \
			destinationData = sourceData; \
			break; \
		case 4: \
			destinationData = ~sourceData & destinationData; \
			break; \
		case 5: \
			destinationData = destinationData; \
			break; \
		case 6: \
			destinationData = sourceData ^ destinationData; \
			break; \
		case 7: \
			destinationData = sourceData | destinationData; \
			break; \
		case 8: \
			destinationData = ~(sourceData | destinationData); \
			break; \
		case 9: \
			destinationData = ~(sourceData ^ destinationData); \
			break; \
		case 10: \
			destinationData = ~destinationData; \
			break; \
		case 11: \
			destinationData = sourceData | ~destinationData; \
			break; \
		case 12: \
			destinationData = ~sourceData; \
			break; \
		case 13: \
			destinationData = ~sourceData | destinationData; \
			break; \
		case 14: \
			destinationData = ~(sourceData & destinationData); \
			break; \
		case 15: \
			destinationData = 0xffff; \
			break; \
	}


extern "C" {
static void chunkyToBitplane( uint8 *sdlPixelData, uint16 bpp, uint16 bitplaneWords[8] )
{
	memset( bitplaneWords, 0, sizeof(bitplaneWords) ); // clear the color values for the 8 pixels (word length)

	for (int l = 0; l < 16; l++) {
		uint8 data = sdlPixelData[l]; // note: this is about 2000 dryhstones sppedup (the local variable)

		bitplaneWords[0] <<= 1; bitplaneWords[0] |= (data >> 7) & 1;
		bitplaneWords[1] <<= 1; bitplaneWords[1] |= (data >> 6) & 1;
		bitplaneWords[2] <<= 1; bitplaneWords[2] |= (data >> 5) & 1;
		bitplaneWords[3] <<= 1; bitplaneWords[3] |= (data >> 4) & 1;
		bitplaneWords[4] <<= 1; bitplaneWords[4] |= (data >> 3) & 1;
		bitplaneWords[5] <<= 1; bitplaneWords[5] |= (data >> 2) & 1;
		bitplaneWords[6] <<= 1; bitplaneWords[6] |= (data >> 1) & 1;
		bitplaneWords[7] <<= 1; bitplaneWords[7] |= (data >> 0) & 1;
	}
}
}

uint32 FVDIDriver::blitArea(void *vwk, MFDB *src, MFDB *dest, int32 sx, int32 sy, int32 dx, int32 dy, int32 w, int32 h,
							uint32 logOp)
{
	D(bug("fVDI: %s %x %d,%d:%d,%d:%d,%d", "blitArea", logOp, sx, sy, dx, dy, w, h ));
	D(bug("fVDI: %s %x,%x : %x,%x", "blitArea - MFDB addresses", src, dest, (src)?(get_long( (uint32)src, true )):0,(dest)?(get_long( (uint32)dest, true )):0));

	uint32 screenMFDBAddr = get_long( (uint32)vwk, true ) + VWK_SCREEN_MFDB_ADDRESS;
	uint32 screenPlanes = (uint32)get_word( screenMFDBAddr + MFDB_BITPLANES, true );
	uint32 videoRam = get_long( screenMFDBAddr, true );

	//uint32 screenPitch = get_word( screenMFDBAddr + MFDB_WDWIDTH, true ) * screenPlanes << 1;
	D(bug("fVDI: screen args: address %x, pitch %d, planes %d", videoRam, screenPitch, screenPlanes));

	bool toMemory = ( dest != NULL && get_long( (uint32)dest, true ) != 0 && get_long( (uint32)dest, true ) != videoRam );
	if ( src != NULL && get_long( (uint32)src, true ) != 0 && get_long( (uint32)src, true ) != videoRam ) { // fromMemory
		uint32 planes = get_word( (uint32)src + MFDB_BITPLANES, true ); // MFDB *src->bitplanes
		uint32 pitch  = get_word( (uint32)src + MFDB_WDWIDTH, true ) * planes << 1; // MFDB *src->pitch
		uint16 *data  = (uint16*)(get_long( (uint32)src, true ) + sy*pitch); // MFDB *src->address host OS address

		D(bug("fVDI: blitArea M->: address %x, pitch %d, planes %d", data, pitch, planes));

		if ( toMemory ) {
			// the destPlanes is always the same?
			planes = get_word( (uint32)dest + MFDB_BITPLANES, true ); // MFDB *dest->bitplanes
			uint32 destPitch = get_word( (uint32)dest + MFDB_WDWIDTH, true ) * planes << 1; // MFDB *dest->pitch
			uint32 destAddress = get_long( (uint32)dest, true );

			D(bug("fVDI: blitArea M->M"));

			uint32 srcData;
			uint32 destData;

			switch( planes ) {
				case 16:
					for( uint16 j=0; j<h; j++ )
						for( uint16 i=sx; i<sx+w; i++ ) {
							srcData = get_word((uint32)data + j*pitch + ((i*planes)>>3), true);
							destData = get_word( destAddress + ((dx+i-sx)<<1) + (dy+j)*destPitch, true );
							applyBlitLogOperation( logOp, destData, srcData );
							put_word( destAddress + ((dx+i-sx)<<1) + (dy+j)*destPitch, destData );
						}
					break;
				case 24:
					for( uint16 j=0; j<h; j++ )
						for( uint16 i=sx; i<sx+w; i++ ) {
							uint16 xoffset = ((dx+i-sx)<<1)+(dx+i-sx);
							srcData = get_dtriplet((uint32)data + j*pitch + ((i*planes)>>3), true);
							destData = get_dtriplet( destAddress + xoffset + (dy+j)*destPitch, true );
							applyBlitLogOperation( logOp, destData, srcData );
							put_dtriplet( destAddress + xoffset + (dy+j)*destPitch, destData );
						}
					break;
				case 32:
					for( uint16 j=0; j<h; j++ )
						for( uint16 i=sx; i<sx+w; i++ ) {
							srcData = get_long((uint32)data + j*pitch + ((i*planes)>>3), true);
							destData = get_long( destAddress + ((dx+i-sx)<<2) + (dy+j)*destPitch, true );
							applyBlitLogOperation( logOp, destData, srcData );
							put_long( destAddress + ((dx+i-sx)<<2) + (dy+j)*destPitch, destData );
						}
					break;

				default:
					if ( planes < 16 ) {
						D(bug("fVDI: blitArea M->M: NOT TESTED bitplaneToCunky conversion"));
					}
			}
			return 1;
		}


		D(bug("fVDI: blitArea M->S"));

		if (!hostScreen.renderBegin())
			return 1;

		D(bug("fVDI: M->S: address %x, pitch %d, planes %d", data, pitch, planes));

		uint32 srcData;
		uint32 destData;

		switch( planes ) {
			case 16:
				for( uint16 j=0; j<h; j++ )
					for( uint16 i=sx; i<sx+w; i++ ) {
						srcData = get_word((uint32)data + j*pitch + ((i*planes)>>3), true);
						//uint16 destData = get_word( videoRam + ((dx+i-sx)<<1) + (dy+j)*screenPitch, true ); // shadow?
						destData = hostScreen.getPixel( dx + i - sx, dy + j );
						applyBlitLogOperation( logOp, destData, srcData );
						//put_word( videoRam + ((dx+i-sx)<<1) + (dy+j)*screenPitch, destData ); // shadow?
						hostScreen.putPixel( dx+i-sx, dy+j, destData );
					}
				break;
			case 24:
				for( uint16 j=0; j<h; j++ )
					for( uint16 i=sx; i<sx+w; i++ ) {
						srcData = get_dtriplet((uint32)data + j*pitch + ((i*planes)>>3), true);
						destData = hostScreen.getPixel( dx + i - sx, dy + j );
						applyBlitLogOperation( logOp, destData, srcData );
						hostScreen.putPixel( dx+i-sx, dy+j, destData );
					}
				break;
			case 32:
				for( uint16 j=0; j<h; j++ )
					for( uint16 i=sx; i<sx+w; i++ ) {
						srcData = get_long((uint32)data + j*pitch + ((i*planes)>>3), true);
						destData = hostScreen.getPixel( dx + i - sx, dy + j );
						applyBlitLogOperation( logOp, destData, srcData );
						hostScreen.putPixel( dx+i-sx, dy+j, destData );
					}
				break;

			default: // bitplane modes...
				if ( planes < 16 ) {
					uint8 color[16];

					D(bug("fVDI: blitArea M->S: bitplaneToCunky conversion"));
					data = (uint16*)get_real_address_direct((uint32)data); // FIXME: Hack! Should use the get_X() methods

					for( uint16 j=0; j<h; j++ ) {
						uint32 wordIndex = (j*pitch>>1) + (sx>>4)*planes;
						hostScreen.bitplaneToChunky( &data[ wordIndex ], planes, color );

						for( uint16 i=sx; i<sx+w; i++ ) {
							uint8 bitNo = i & 0xf;
							if ( bitNo == 0 ) {
								uint32 wordIndex = (j*pitch>>1) + (i>>4)*planes;
								hostScreen.bitplaneToChunky( &data[ wordIndex ], planes, color );
							}
							hostScreen.putPixel(dx+i-sx, dy+j, color[ bitNo ] );
						}
					}
				}
		}

		hostScreen.renderEnd();
		hostScreen.update( dx, dy, w, h, true );

		return 1;
	}

	if ( toMemory ) {
		D(bug("fVDI: blitArea S->M"));

		if (!hostScreen.renderBegin())
			return 1;

		//uint32 planes = screenPlanes;
		//uint32  pitch = screenPitch;
		//uint16* data = (uint16*)videoRam + sy*pitch;

		uint32 planes = get_word( (uint32)dest + MFDB_BITPLANES, true ); // MFDB *dest->bitplanes
		uint32 destPitch = get_word( (uint32)dest + MFDB_WDWIDTH, true ) * planes << 1; // MFDB *dest->pitch
		uint32 destAddress = get_long( (uint32)dest, true );

		D(bug("fVDI: S->M: address %x, pitch %d, planes %d", destAddress, destPitch, planes));

		uint32 srcData;
		uint32 destData;

		switch( planes ) {
			case 16:
				for( uint16 j=0; j<h; j++ )
					for( uint16 i=sx; i<sx+w; i++ ) {
						//uint16 srcData = get_word( (uint32)data + ((i*planes)>>3) + j*pitch, true ); // from the shadow?
						srcData = hostScreen.getPixel( i, sy + j );
						destData = get_word( destAddress + ((dx+i-sx)<<1) + (dy+j)*destPitch, true );
						applyBlitLogOperation( logOp, destData, srcData );
						put_word( destAddress + ((dx+i-sx)<<1) + (dy+j)*destPitch, destData );
					}
				break;
			case 24:
				for( uint16 j=0; j<h; j++ )
					for( uint16 i=sx; i<sx+w; i++ ) {
						uint16 xoffset = ((dx+i-sx)<<1)+(dx+i-sx);
						srcData = hostScreen.getPixel( i, sy + j );
						destData = get_dtriplet( destAddress + xoffset + (dy+j)*destPitch, true );
						applyBlitLogOperation( logOp, destData, srcData );
						put_dtriplet( destAddress + xoffset + (dy+j)*destPitch, destData );
					}
				break;
			case 32:
				for( uint16 j=0; j<h; j++ )
					for( uint16 i=sx; i<sx+w; i++ ) {
						srcData = hostScreen.getPixel( i, sy + j );
						destData = get_long( destAddress + ((dx+i-sx)<<2) + (dy+j)*destPitch, true );
						applyBlitLogOperation( logOp, destData, srcData );
						put_long( destAddress + ((dx+i-sx)<<2) + (dy+j)*destPitch, destData );
					}
				break;

			default:
				if ( planes < 16 ) {
					D(bug("fVDI: blitArea S->M: bitplane conversion"));

					uint16 bitplanePixels[8];
					uint32 pitch = hostScreen.getPitch();
					uint8* data = (uint8*)hostScreen.getVideoramAddress() + sy*pitch;

					for( int32 j=0; j<h; j++ ) {
						uint32 pixelPosition = j*pitch + sx & ~0xf; // div 16
						chunkyToBitplane( data + pixelPosition, planes, bitplanePixels );
						for( uint16 d=0; d<planes; d++ )
							put_word( destAddress + ((((dx>>4)*planes)+planes-d-1)<<1) + (dy+j)*destPitch, bitplanePixels[d] );

						for( uint16 i=sx; i<sx+w; i++ ) {
							uint8 bitNo = i & 0xf;
							if ( bitNo == 0 ) {
								uint32 wordIndex = ((dx+i-sx)>>4)*planes;
								uint32 pixelPosition = j*pitch + i & ~0xf; // div 16
								chunkyToBitplane( data + pixelPosition, planes, bitplanePixels );
								for( uint16 d=0; d<planes; d++ )
									put_word( destAddress + ((wordIndex+planes-d-1)<<1) + (dy+j)*destPitch, bitplanePixels[d] );
							}
						}
					}
				}
		}

		hostScreen.renderEnd();
		return 1;
	}

	D(bug("fVDI: %s ", "blitArea - S->S!" ));

	// if (!hostScreen.renderBegin()) // the surface must _not_ be locked for blitArea (SDL_BlitSurface)
	//      return 1;

	// FIXME: There are no logical operation implementation ATM
	// for S->S blits... -> SDL does the whole thing at once
	if ( logOp == 3 )
		hostScreen.blitArea( sx, sy, dx, dy, w, h );
	else
		D(bug("fVDI: %s %d", "blitArea - S->S! mode NOT IMPLEMENTED!!!", logOp ));

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


extern "C" {
	static inline int clipEncode (int16 x, int16 y, int16 left, int16 top, int16 right, int16 bottom)
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
}

static inline int clipLine(void *vwk, int16 *x1, int16 *y1, int16 *x2, int16 *y2)
{
	int16 left,right,top,bottom;
	int code1, code2;
	int draw = 0;
	int16 swaptmp;
	float m;

	if ( !get_word( (uint32)vwk + VWK_CLIP_RECT, true ) )
		return 1; // clipping is off

	/* Get clipping boundary */
	left   = get_word( (uint32)vwk + VWK_CLIP_RECT + 2, true );
	top    = get_word( (uint32)vwk + VWK_CLIP_RECT + 4, true );
	right  = get_word( (uint32)vwk + VWK_CLIP_RECT + 6, true );
	bottom = get_word( (uint32)vwk + VWK_CLIP_RECT + 8, true );

	D2(bug("fVDI: %s %d,%d:%d,%d", "clipLineTO", left, top, right, bottom ));

	while (1) {
		code1 = clipEncode (*x1, *y1, left, top, right, bottom);
		code2 = clipEncode (*x2, *y2, left, top, right, bottom);
		if (CLIP_ACCEPT(code1, code2)) {
			draw = 1;
			break;
		} else if (CLIP_REJECT(code1, code2)) {
			break;
		} else {
			if(CLIP_INSIDE (code1)) {
				int16* swapPtr;
				swapPtr = x2; x2 = x1; x1 = swapPtr;
				swapPtr = y2; y2 = y1; y1 = swapPtr;
				swaptmp = code2; code2 = code1; code1 = swaptmp;
			}
			if (*x2 != *x1) {
				m = (*y2 - *y1) / (float)(*x2 - *x1);
			} else {
				m = 1.0f;
			}
			if (code1 & CLIP_LEFT_EDGE) {
				*y1 += (int16)((left - *x1) * m);
				*x1 = left;
			} else if (code1 & CLIP_RIGHT_EDGE) {
				*y1 += (int16)((right - *x1) * m);
				*x1 = right;
			} else if (code1 & CLIP_BOTTOM_EDGE) {
				if (*x2 != *x1) {
					*x1 += (int16)((bottom - *y1) / m);
				}
				*y1 = bottom;
			} else if (code1 & CLIP_TOP_EDGE) {
				if (*x2 != *x1) {
					*x1 += (int16)((top - *y1) / m);
				}
				*y1 = top;
			}
		}
	}

	D2(bug("fVDI: %s %d,%d:%d,%d", "clipLineEND", *x1, *y1, *x2, *y2 ));

	return draw;
}


uint32 FVDIDriver::drawLine(void *vwk, int32 x1, int32 y1, int32 x2, int32 y2,
							uint16 pattern, uint32 fgColor, uint32 bgColor, uint32 logOp )
{
	int16 sx1 = (int16)x1;
	int16 sy1 = (int16)y1;
	int16 sx2 = (int16)x2;
	int16 sy2 = (int16)y2; // FIXME!!!

	D(bug("fVDI: %s %x %d,%d:%d,%d p:%x, (fgc:%d /%x/ : bgc:%d /%x/)", "drawLine", logOp, x1, y1, x2, y2, pattern, fgColor, hostScreen.getPaletteColor( fgColor ), bgColor, hostScreen.getPaletteColor( bgColor ) ));

	if ( !clipLine( vwk, &sx1, &sy1, &sx2, &sy2 ) ) // do not draw the line when it is out
		return 1;

	fgColor = hostScreen.getPaletteColor( fgColor );
	bgColor = hostScreen.getPaletteColor( bgColor );

	hostScreen.drawLine( sx1, sy1, sx2, sy2, pattern, fgColor, bgColor, logOp );

	int32 dx, dy, lx, ly;
	if ( sx1 >= sx2 ) {
		lx = sx2;
		dx = sx1 - sx2 + 1;
	} else {
		lx = sx1;
		dx = sx2 - sx1 + 1;
	}
	if ( sy1 >= sy2 ) {
		ly = sy2;
		dy = sy1 - sy2 + 1;
	} else {
		ly = sy1;
		dy = sy2 - sy1 + 1;
	}

	D2(bug("fVDI: %s %d,%d:%d,%d", "drawLineUp", lx, ly, dx, dy ));
	hostScreen.update( lx, ly, dx, dy, true );

	return 1;
}


/*
 * $Log$
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
