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

#define DEBUG 0
#include "debug.h"

#include <SDL_opengl.h>

/*--- Defines ---*/

#define EINVFN -32

static const GLenum logicOps[16]={
	GL_CLEAR, GL_AND, GL_AND_REVERSE, GL_COPY,
	GL_AND_INVERTED, GL_NOOP, GL_XOR, GL_OR,
	GL_NOR, GL_EQUIV, GL_INVERT, GL_OR_REVERSE,
	GL_COPY_INVERTED, GL_OR_INVERTED, GL_NAND, GL_SET
};

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

int32 OpenGLVdiDriver::openWorkstation(void)
{
	hostScreen.EnableOpenGLVdi();
	return VdiDriver::openWorkstation();
}

int32 OpenGLVdiDriver::closeWorkstation(void)
{
	hostScreen.DisableOpenGLVdi();
	return VdiDriver::closeWorkstation();
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

int32 OpenGLVdiDriver::getPixel(memptr vwk, memptr src, int32 x, int32 y)
{
	int32 color = 0;

	if (vwk & 1)
		return color;

	if (src)
		return VdiDriver::getPixel(vwk, src, x, y);

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

	if (dst)
		return VdiDriver::putPixel(vwk, dst, x, y, color);

	glBegin(GL_POINTS);
		glColor3ub((color>>16)&0xff, (color>>8)&0xff, color&0xff);
		glVertex2i(x,y);
	glEnd();

//	D(bug("glvdi: putpixel(%d,%d,0x%08x)", x,y,color));
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
	Uint8 *bitmap, *s, *d;
	int width, srcpitch, dstpitch, y;

	if (dest)
		return VdiDriver::expandArea(vwk, src, sx, sy, dest, dx, dy, w, h, logOp, fgColor, bgColor);

	/* Allocate temp space for monochrome bitmap */
	width = (w + 31) & ~31;
	bitmap = (Uint8 *)malloc((width*h)>>3);
	if (bitmap==NULL) {
		return -1;
	}

	/* Copy the bitmap from source */
	srcpitch = ReadInt16(src + MFDB_WDWIDTH) * 2;
	s = (Uint8 *)Atari2HostAddr(ReadInt32(src + MFDB_ADDRESS) + sy * srcpitch);
	dstpitch = width>>3;
	d = bitmap + (dstpitch * (h-1));
	for (y=0;y<h;y++) {
		memcpy(d, s, width>>3);
		s += srcpitch;
		d -= dstpitch;	
	}

	if (logOp == 1) {
		/* First, the back color */
		glColor3ub((bgColor>>16)&0xff,(bgColor>>8)&0xff,bgColor&0xff);
		glBegin(GL_QUADS);
			glVertex2i(dx, dy);
			glVertex2i(dx+w-1, dy);
			glVertex2i(dx+w-1, dy+h-1);
			glVertex2i(dx, dy+h-1);
		glEnd();
	}

	glEnable(GL_COLOR_LOGIC_OP);
	if (logOp == 3) {
		glLogicOp(GL_XOR);
		glColor3ub(0xff,0xff,0xff);
	} else {
		glLogicOp(GL_COPY);
		glColor3ub((fgColor>>16)&0xff,(fgColor>>8)&0xff,fgColor&0xff);
	}

	glRasterPos2i(dx,dy+h);
	glBitmap(w,h, 0,0, 0,0, (const GLubyte *)bitmap);
	glDisable(GL_COLOR_LOGIC_OP);

	free(bitmap);
	return 1;
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

int32 OpenGLVdiDriver::fillArea(memptr vwk, uint32 x_, uint32 y_,
	int32 w, int32 h, memptr pattern_addr, uint32 fgColor, uint32 bgColor,
	uint32 logOp, uint32 /*interior_style*/)
{
	if (hostScreen.getBpp() <= 1) {
		fgColor &= 0xff;
		bgColor &= 0xff;
	}

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
		if (logOp == 1) {
			glColor3ub((bgColor>>16)&0xff,(bgColor>>8)&0xff,bgColor&0xff);
			glBegin(GL_QUADS);
				glVertex2i(x,y);
				glVertex2i(x+w-1,y);
				glVertex2i(x+w-1,y+h-1);
				glVertex2i(x,y+h-1);
			glEnd();
		}

		glEnable(GL_COLOR_LOGIC_OP);
		if (logOp == 3) {
			glLogicOp(GL_XOR);
			glColor3ub(0xff,0xff,0xff);
		} else {
			glLogicOp(GL_COPY);
			glColor3ub((fgColor>>16)&0xff,(fgColor>>8)&0xff,fgColor&0xff);
		}

		/* Fill with fgColor */
		glEnable(GL_POLYGON_STIPPLE);
		glPolygonStipple((const GLubyte *)gl_pattern);
		glBegin(GL_POLYGON);
			glVertex2i(x,y);
			glVertex2i(x+w-1,y);
			glVertex2i(x+w-1,y+h-1);
			glVertex2i(x,y+h-1);
		glEnd();
		glDisable(GL_POLYGON_STIPPLE);

		D(bug("glvdi:  fillarea, with polygon"));
	} else {
		for(h = h - 1; h >= 0; h--) {
			y = (int16)ReadInt16(table); table+=2;
			x = (int16)ReadInt16(table); table+=2;
			w = (int16)ReadInt16(table) - x + 1; table+=2;

			/* First, the back color */
			if (logOp==1) {
				glColor3ub((bgColor>>16)&0xff,(bgColor>>8)&0xff,bgColor&0xff);
				glBegin(GL_LINES);
					glVertex2i(x,y);
					glVertex2i(x+w-1,y);
				glEnd();
			}

			glEnable(GL_COLOR_LOGIC_OP);
			if (logOp == 3) {
				glLogicOp(GL_XOR);
				glColor3ub(0xff,0xff,0xff);
			} else {
				glLogicOp(GL_COPY);
				glColor3ub((fgColor>>16)&0xff,(fgColor>>8)&0xff,fgColor&0xff);
			}

			/* Fill with fgColor */
			glEnable(GL_LINE_STIPPLE);
			glLineStipple(1,pattern[y&15]);
			glBegin(GL_LINES);
				glVertex2i(x,y);
				glVertex2i(x+w-1,y);
			glEnd();
			glDisable(GL_LINE_STIPPLE);
		}
		D(bug("glvdi:  fillarea, with horizontal lines"));
	}

	glDisable(GL_COLOR_LOGIC_OP);
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

int32 OpenGLVdiDriver::blitArea_M2S(memptr /*vwk*/, memptr src, int32 sx, int32 sy,
	memptr /*dest*/, int32 dx, int32 dy, int32 w, int32 h, uint32 logOp)
{
	int y;

	/* Clear rectangle ? */
	if ((logOp==0) || (logOp==15)) {
		if (logOp==0) {
			glColor3ub(0,0,0);
		} else if (logOp==15) {
			glColor3ub(0xff,0xff,0xff);
		}
		glBegin(GL_QUADS);
			glVertex2i(dx,dy);
			glVertex2i(dx+w-1,dy);
			glVertex2i(dx+w-1,dy+h-1);
			glVertex2i(dx,dy+h-1);
		glEnd();
		D(bug("glvdi: blit_m2s: clear rectangle"));
		return 1;
	}

	D(bug("glvdi: blit_m2s(%dx%d: %d,%d -> %d,%d, %d)",w,h,sx,sy,dx,dy,logOp));

	uint32 planes = ReadInt16(src + MFDB_NPLANES);			// MFDB *dest->bitplanes
	if ((planes!=16) && (planes!=32)) {
		D(bug("glvdi: blit_m2s: %d planes unsupported",planes));
		return -1;
	}

	uint32 srcPitch = ReadInt16(src + MFDB_WDWIDTH) * planes * 2;	// MFDB *dest->pitch
	Uint8 *srcAddress = Atari2HostAddr(ReadInt32(src));
	srcAddress += sy * srcPitch;
	srcAddress += sx * (planes>>3);

	glEnable(GL_COLOR_LOGIC_OP);
	glLogicOp(logicOps[logOp]);

	for (y=0;y<h;y++) {
		glRasterPos2i(dx,dy+y);
		switch(planes) {
			case 16:
				glDrawPixels(w,1, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, srcAddress);
				break;
			case 32:
				glDrawPixels(w,1, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8, srcAddress);
				break;
		}
		srcAddress += srcPitch;
	}

	glDisable(GL_COLOR_LOGIC_OP);
	return 1;
}

int32 OpenGLVdiDriver::blitArea_S2M(memptr /*vwk*/, memptr /*src*/, int32 sx, int32 sy,
	memptr dest, int32 dx, int32 dy, int32 w, int32 h, uint32 logOp)
{
	int y;

	D(bug("glvdi: blit_s2m(%dx%d: %d,%d -> %d,%d, %d)",w,h,sx,sy,dx,dy,logOp));

	uint32 planes = ReadInt16(dest + MFDB_NPLANES);			// MFDB *dest->bitplanes
	if ((planes!=16) && (planes!=32)) {
		D(bug("glvdi: blit_s2m: %d planes unsupported",planes));
		return -1;
	}
	if ((logOp!=0) && (logOp!=3) && (logOp!=15)) {
		D(bug("glvdi: blit_s2m: logOp %d unsupported",logOp));
		return -1;
	}

	uint32 destPitch = ReadInt16(dest + MFDB_WDWIDTH) * planes * 2;	// MFDB *dest->pitch
	Uint8 *destAddress = Atari2HostAddr(ReadInt32(dest));
	destAddress += dy * destPitch;
	destAddress += dx * (planes>>3);

	for (y=0;y<h;y++) {
		switch(logOp) {
			case 0:
				memset(destAddress, 0, destPitch);
				break;
			case 15:
				memset(destAddress, 0xff, destPitch);
				break;
			case 3:
				switch(planes) {
					case 16:
						glReadPixels(sx,hostScreen.getHeight()-(sy+y), w,1, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, destAddress);
						break;
					case 32:
						glReadPixels(sx,hostScreen.getHeight()-(sy+y), w,1, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8, destAddress);
						break;
				}
				break;
		}
		destAddress += destPitch;
	}

	return 1;
}

int32 OpenGLVdiDriver::blitArea_S2S(memptr /*vwk*/, memptr /*src*/, int32 sx,
	int32 sy, memptr /*dest*/, int32 dx, int32 dy, int32 w, int32 h, uint32 logOp)
{
	/* Copy a rectangle on itself ? */
	if ((sx==dx) && (sy==dy) && (logOp==3)) {
		D(bug("glvdi: blit_s2s: self copy"));
		return 1;
	}

	/* Clear rectangle ? */
	if ((logOp==0) || (logOp==15)) {
		if (logOp==0) {
			glColor3ub(0,0,0);
		} else if (logOp==15) {
			glColor3ub(0xff,0xff,0xff);
		}
		glBegin(GL_QUADS);
			glVertex2i(dx,dy);
			glVertex2i(dx+w-1,dy);
			glVertex2i(dx+w-1,dy+h-1);
			glVertex2i(dx,dy+h-1);
		glEnd();
		D(bug("glvdi: blit_s2s: clear rectangle"));
		return 1;
	}

	glEnable(GL_COLOR_LOGIC_OP);	
	glLogicOp(logicOps[logOp & 15]);

	glRasterPos2i(dx,dy+h-1);
	glCopyPixels(sx,hostScreen.getHeight()-(sy+h-1), w,h, GL_COLOR);

	glDisable(GL_COLOR_LOGIC_OP);
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

int OpenGLVdiDriver::drawSingleLine(int x1, int y1, int x2, int y2,
	uint16 pattern, uint32 fgColor, uint32 bgColor, int logOp)
{
	if (logOp == 1) {
		/* First, the back color */
		glColor3ub((bgColor>>16)&0xff,(bgColor>>8)&0xff,bgColor&0xff);
		glBegin(GL_LINES);
			glVertex2i(x1,y1);
			glVertex2i(x2,y2);
		glEnd();
	}

	glEnable(GL_COLOR_LOGIC_OP);
	if (logOp == 3) {
		glLogicOp(GL_XOR);
		glColor3ub(0xff,0xff,0xff);
	} else {
		glLogicOp(GL_COPY);
		glColor3ub((fgColor>>16)&0xff,(fgColor>>8)&0xff,fgColor&0xff);
	}

	glEnable(GL_LINE_STIPPLE);
	glLineStipple(1,pattern);

	/* Draw with fgColor */
	glBegin(GL_LINES);
		glVertex2i(x1,y1);
		glVertex2i(x2,y2);
	glEnd();

	glDisable(GL_LINE_STIPPLE);
	glDisable(GL_COLOR_LOGIC_OP);

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
	int clipped=0;

	if (hostScreen.getBpp() <= 1) {
		fgColor &= 0xff;
		bgColor &= 0xff;
	}

	if (clip) {
		int cx1,cy1,cx2,cy2, x,y,w,h;

		cx1=ReadInt32(clip);
		cy1=ReadInt32(clip+4);
		cx2=ReadInt32(clip+8);
		cy2=ReadInt32(clip+12);

		x=cx1;
		w=cx2-cx1+1;
		if (cx2<cx1) {
			x=cx2;
			w=cx1-cx2+1;
		}
		y=cy1;
		h=cy2-cy1+1;
		if (cy2<cy1) {
			y=cy2;
			h=cy1-cy2+1;
		}
		glScissor(x,hostScreen.getHeight()-(y+h-1),w,h);
		glEnable(GL_SCISSOR_TEST);
		clipped=1;
	}

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

	if (clipped) {
		glDisable(GL_SCISSOR_TEST);
	}
	return 1;
}

int32 OpenGLVdiDriver::fillPoly(memptr vwk, memptr points_addr, int n,
	memptr index_addr, int moves, memptr pattern_addr, uint32 fgColor,
	uint32 bgColor, uint32 logOp, uint32 interior_style, memptr clip)
{
	DUNUSED(points_addr);
	DUNUSED(index_addr);
	DUNUSED(moves);
	DUNUSED(fgColor);
	DUNUSED(bgColor);
	DUNUSED(logOp);
	DUNUSED(interior_style);
	DUNUSED(clip);

	if (vwk & 1)
		return -1;      // Don't know about any special fills
	if (!n)
		return 1;

	uint16 pattern[16];
	for(int i = 0; i < 16; ++i)
		pattern[i] = ReadInt16(pattern_addr + i * 2);

	D(bug("glvdi: fillpoly"));
	return -1;
}

void OpenGLVdiDriver::getHwColor(uint16 index, uint32 red, uint32 green,
	uint32 blue, memptr hw_value)
{
	WriteInt32(hw_value, (((red*255)/1000)<<16)|(((green*255)/1000)<<8)|((blue*255)/1000));
	D(bug("fVDI: getHwColor (%03d) %04d,%04d,%04d - %lx", index, red, green, blue, ReadInt32( hw_value )));
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
 
void OpenGLVdiDriver::setColor(memptr /*vwk*/, uint32 /*paletteIndex*/,
	uint32 /*red*/, uint32 /*green*/, uint32 /*blue*/)
{
}

int32 OpenGLVdiDriver::getFbAddr(void)
{
	return 0;
}
