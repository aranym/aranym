/*
	NatFeat VDI driver, OpenGL renderer

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

#include "sysdeps.h"
#include "cpu_emulation.h"
#include "parameters.h"
#include "hostscreen.h"
#include "nfvdi.h"
#include "nfvdi_opengl.h"

#define DEBUG 1
#include "debug.h"

#include <SDL_opengl.h>

/*--- Defines ---*/

#define EINVFN -32

/*--- Types ---*/

/*--- Variables ---*/

extern HostScreen hostScreen;

/*--- Public functions ---*/

OpenGLVdiDriver::OpenGLVdiDriver()
{
	memset(palette_red, 0, sizeof(palette_red));
	memset(palette_green, 0, sizeof(palette_green));
	memset(palette_blue, 0, sizeof(palette_blue));
}

OpenGLVdiDriver::~OpenGLVdiDriver()
{
}

/*--- Private functions ---*/

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

int32 OpenGLVdiDriver::getPixel(memptr vwk, memptr src, int32 x, int32 y)
{
	int32 color = 0;

	if (vwk & 1)
		return color;

	if (src) {
		return VdiDriver::getPixel(vwk, src, x, y);
	}

	glReadPixels(x,y,1,1, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, &color);
	color >>= 8;
//	D(bug("glvdi: getpixel"));
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
 
int32 OpenGLVdiDriver::putPixel(memptr vwk, memptr dst, int32 x, int32 y,
	uint32 color)
{
	if (vwk & 1)
		return 0;

	if (dst) {
		return VdiDriver::putPixel(vwk, dst, x, y, color);
	}

//	color=random();

	glBegin(GL_POINTS);
		glColor3i((color>>16)&0xff, (color>>8)&0xff, color&0xff);
		glVertex2i(x,y);
	glEnd();
//	D(bug("glvdi: putpixel(%d,%d,0x%08x)", x,y,color));
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

int32 OpenGLVdiDriver::drawMouse(memptr wk, int32 x, int32 y, uint32 mode,
	uint32 data, uint32 hot_x, uint32 hot_y, uint32 fgColor, uint32 bgColor,
	uint32 mouse_type)
{
	DUNUSED(wk);
	DUNUSED(x);
	DUNUSED(y);
	DUNUSED(mode);
	DUNUSED(data);
	DUNUSED(hot_x);
	DUNUSED(hot_y);
	DUNUSED(fgColor);
	DUNUSED(bgColor);
	DUNUSED(mouse_type);

	SDL_ShowCursor(1);
	D(bug("glvdi: drawmouse"));
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

int32 OpenGLVdiDriver::expandArea(memptr vwk, memptr src, int32 sx, int32 sy,
	memptr dest, int32 dx, int32 dy, int32 w, int32 h, uint32 logOp,
	uint32 fgColor, uint32 bgColor)
{
	if (dest) {
		return VdiDriver::expandArea(vwk, src, sx, sy, dest, dx, dy, w, h, logOp, fgColor, bgColor);
	}

	D(bug("glvdi: expandarea"));
	return -1;
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

int32 OpenGLVdiDriver::fillArea(memptr vwk, uint32 x_, uint32 y_, int32 w,
	int32 h, memptr pattern_addr, uint32 fgColor, uint32 bgColor,
	uint32 logOp, uint32 interior_style)
{
	DUNUSED(interior_style);
	if (hostScreen.getBpp() <= 1) {
		fgColor &= 0xff;
		bgColor &= 0xff;
	}

//	fgColor = random();
//	bgColor = random();

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

	D(bug("glvdi: %s %d %d,%d:%d,%d : %d,%d p:%x, (fgc:%lx : bgc:%lx)", "fillArea",
	      logOp, x, y, w, h, x + w - 1, x + h - 1, *pattern,
	      fgColor, bgColor));

	/* Perform rectangle fill. */

	if (!table) {
		/* Generate the pattern */
		uint32 gl_pattern[32];
		int i;

		memset(gl_pattern,0, sizeof(gl_pattern));
		for (i=0; i<32; i++) {
			gl_pattern[i] = (pattern[i&15]<<16)|pattern[i&15];
		}

		/* First, the back color */
		glColor3i((bgColor>>16)&0xff,(bgColor>>8)&0xff,bgColor&0xff);
		glBegin(GL_POLYGON);
			glVertex2i(x,y);
			glVertex2i(x+w-1,y);
			glVertex2i(x+w-1,y+h-1);
			glVertex2i(x,y+h-1);
		glEnd();

		/* Fill with fgColor */
		glEnable(GL_POLYGON_STIPPLE);
		glPolygonStipple((const GLubyte *)gl_pattern);
		glColor3i((fgColor>>16)&0xff,(fgColor>>8)&0xff,fgColor&0xff);
		glBegin(GL_POLYGON);
			glVertex2i(x,y);
			glVertex2i(x+w-1,y);
			glVertex2i(x+w-1,y+h-1);
			glVertex2i(x,y+h-1);
		glEnd();
		glDisable(GL_POLYGON_STIPPLE);
	} else {
		for(h = h - 1; h >= 0; h--) {
			y = (int16)ReadInt16(table); table+=2;
			x = (int16)ReadInt16(table); table+=2;
			w = (int16)ReadInt16(table) - x + 1; table+=2;

			/* First, the back color */
			glColor3i((bgColor>>16)&0xff,(bgColor>>8)&0xff,bgColor&0xff);
			glBegin(GL_LINES);
				glVertex2i(x,y);
				glVertex2i(x+w-1,y);
			glEnd();

			/* Fill with fgColor */
			glEnable(GL_LINE_STIPPLE);
			glLineStipple(1,pattern[y&15]);
			glColor3i((fgColor>>16)&0xff,(fgColor>>8)&0xff,fgColor&0xff);
			glBegin(GL_POLYGON);
				glVertex2i(x,y);
				glVertex2i(x+w-1,y);
			glEnd();
			glDisable(GL_LINE_STIPPLE);
		}
	}

//	D(bug("glvdi: fillarea"));
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

int32 OpenGLVdiDriver::blitArea(memptr vwk, memptr src, int32 sx, int32 sy,
	memptr dest, int32 dx, int32 dy, int32 w, int32 h, uint32 logOp)
{
	DUNUSED(vwk);
	DUNUSED(src);
	DUNUSED(sx);
	DUNUSED(sy);
	DUNUSED(dest);
	DUNUSED(dx);
	DUNUSED(dy);
	DUNUSED(w);
	DUNUSED(h);
	DUNUSED(logOp);

	D(bug("glvdi: blitarea"));
	return -1;
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

int OpenGLVdiDriver::drawSingleLine(int x1, int y1, int x2, int y2,
	uint16 pattern, uint32 fgColor, uint32 bgColor, int logOp)
{
	/* First, the back color */
	glColor3i((bgColor>>16)&0xff,(bgColor>>8)&0xff,bgColor&0xff);
	glBegin(GL_LINES);
		glVertex2i(x1,y1);
		glVertex2i(x2,y2);
	glEnd();

	/* Draw with fgColor */
	glEnable(GL_LINE_STIPPLE);
	glLineStipple(1,pattern);

	glColor3i((fgColor>>16)&0xff,(fgColor>>8)&0xff,fgColor&0xff);
	glBegin(GL_LINES);
		glVertex2i(x1,y1);
		glVertex2i(x2,y2);
	glEnd();

	glDisable(GL_LINE_STIPPLE);

	D(bug("glvdi: drawline(%d,%d,%d,%d,0x%08x,0x%08x",x1,y1,x2,y2,fgColor,bgColor));
	return 1;
}

int OpenGLVdiDriver::drawTableLine(memptr table, int length, uint16 pattern,
	uint32 fgColor, uint32 bgColor, int logOp)
{
	int x1 = (int16)ReadInt16(table); table+=2;
	int y1 = (int16)ReadInt16(table); table+=2;
	for(--length; length > 0; length--) {
		int x2 = (int16)ReadInt16(table); table+=2;
		int y2 = (int16)ReadInt16(table); table+=2;

		drawSingleLine(x1, y1, x2, y2, pattern, fgColor, bgColor, logOp);
		x1 = x2;
		y1 = y2;
	}

	return 1;
}

int OpenGLVdiDriver::drawMoveLine(memptr table, int length, memptr index, int moves,
	uint16 pattern, uint32 fgColor, uint32 bgColor, int logOp)
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

		drawSingleLine(x1, y1, x2, y2, pattern, fgColor, bgColor, logOp);

		x1 = x2;
		y1 = y2;
	}

	return 1;
}

int32 OpenGLVdiDriver::drawLine(memptr vwk, uint32 x1_, uint32 y1_, uint32 x2_,
	uint32 y2_, uint32 pattern, uint32 fgColor, uint32 bgColor,
	uint32 logOp, memptr clip)
{
	if (hostScreen.getBpp() <= 1) {
		fgColor &= 0xff;
		bgColor &= 0xff;
	}

//	fgColor = random();
//	bgColor = random();

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

	if (table) {
		if (moves)
			drawMoveLine(table, length, index, moves, pattern, fgColor, bgColor,
			             logOp);
		else {
			switch (length) {
			case 0:
				break;
			case 1:
				drawSingleLine(x1, y1, x2, y2, pattern, fgColor, bgColor, logOp);
				break;
			default:
				drawTableLine(table, length, pattern, fgColor, bgColor, logOp);
 				break;
			}
 		}
	} else
		drawSingleLine(x1, y1, x2, y2, pattern, fgColor, bgColor, logOp);

	return 1;
}

int32 OpenGLVdiDriver::fillPoly(memptr vwk, memptr points_addr, int n,
	memptr index_addr, int moves, memptr pattern_addr, uint32 fgColor,
	uint32 bgColor, uint32 logOp, uint32 interior_style, memptr clip)
{
	DUNUSED(vwk);
	DUNUSED(points_addr);
	DUNUSED(n);
	DUNUSED(index_addr);
	DUNUSED(moves);
	DUNUSED(pattern_addr);
	DUNUSED(fgColor);
	DUNUSED(bgColor);
	DUNUSED(logOp);
	DUNUSED(interior_style);
	DUNUSED(clip);

	D(bug("glvdi: fillpoly"));
	return -1;
}

void OpenGLVdiDriver::getHwColor(uint16 index, uint32 red, uint32 green,
	uint32 blue, memptr hw_value)
{
	WriteInt32(hw_value, (palette_red[index]<<16)|(palette_green[index]<<8)|palette_blue[index]);
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
 
void OpenGLVdiDriver::setColor(memptr vwk, uint32 paletteIndex, uint32 red,
	uint32 green, uint32 blue)
{
	DUNUSED(vwk);

	if (paletteIndex>255) {
		return;
	}

	palette_red[paletteIndex] = red & 0xff;
	glPixelMapuiv(GL_PIXEL_MAP_I_TO_R, 256, palette_red);
	palette_green[paletteIndex] = green & 0xff;
	glPixelMapuiv(GL_PIXEL_MAP_I_TO_G, 256, palette_green);
	palette_blue[paletteIndex] = blue & 0xff;
	glPixelMapuiv(GL_PIXEL_MAP_I_TO_B, 256, palette_blue);
}

int32 OpenGLVdiDriver::getFbAddr(void)
{
	return 0;
}
