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
#include "hostscreen.h"
#include "host.h"
#include "nfvdi.h"
#include "nfvdi_opengl.h"

#define DEBUG 0
#include "debug.h"

#include <SDL_endian.h>
#include "dyngl.h"

#define ENABLE_GLU_TESSELATOR 0

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

/*--- Public functions ---*/

OpenGLVdiDriver::OpenGLVdiDriver()
{
	mouse_tex_obj=0;
	memset(mouse_texture, 0, sizeof(mouse_texture));
	tess=NULL;
}

OpenGLVdiDriver::~OpenGLVdiDriver()
{
	if (mouse_tex_obj>0) {
		gl.DeleteTextures(1, &mouse_tex_obj);
		mouse_tex_obj=0;
	}

#if ENABLE_GLU_TESSELATOR
	if (tess) {
		gluDeleteTess(tess);
		tess=NULL;
	}
#endif
}

/*--- Private functions ---*/

int32 OpenGLVdiDriver::openWorkstation(void)
{
	host->video->EnableOpenGLVdi();

	if (!bx_options.nfvdi.use_host_mouse_cursor) {
		if (mouse_tex_obj==0) {
			gl.GenTextures(1, &mouse_tex_obj);
		}
	}

	return VdiDriver::openWorkstation();
}

int32 OpenGLVdiDriver::closeWorkstation(void)
{
	host->video->DisableOpenGLVdi();

	if (mouse_tex_obj>0) {
		gl.DeleteTextures(1, &mouse_tex_obj);
		mouse_tex_obj=0;
	}

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
	Uint32 color;

	if (vwk & 1)
		return 0;

	if (src)
		return VdiDriver::getPixel(vwk, src, x, y);

//	D(bug("glvdi: getpixel"));

	gl.ReadPixels(x,y,1,1, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, &color);

	/* We have a RGBA color in host byte order, convert it to ARGB */
	color >>= 8;

	/* Mask out alpha bits */
	return (color & 0x00ffffffUL);
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

	gl.Begin(GL_POINTS);
		gl.Color3ub((color>>16)&0xff, (color>>8)&0xff, color&0xff);
		gl.Vertex2i(x,y);
	gl.End();

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
 *  d2  0 - move shown  1 - move hidden  2 - hide  3 - show  >7 - change shape (pointer to mouse struct)
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

void OpenGLVdiDriver::restoreMouseBackground(void)
{
	D(bug("restore >%d %d %d %d", Mouse.storage.x, Mouse.storage.y+Mouse.storage.height, Mouse.storage.width, Mouse.storage.height));
	gl.RasterPos2i(Mouse.storage.x,Mouse.storage.y+Mouse.storage.height);
	gl.DrawPixels(Mouse.storage.width, Mouse.storage.height, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, &(Mouse.storage.background[0][0]));
}


void OpenGLVdiDriver::saveMouseBackground(int16 x, int16 y, int16 width,
	int16 height)
{
	Mouse.storage.x = x;
	Mouse.storage.y = y;
	Mouse.storage.width = width;
	Mouse.storage.height = height;

	y = host->video->getHeight()-(y+height);
	D(bug("save <%d %d %d %d", x, y, width, height));
	gl.ReadPixels(
		x, y, width, height,
		GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, &(Mouse.storage.background[0][0])
	);
}

int OpenGLVdiDriver::drawMouse(memptr wk, int32 x, int32 y, uint32 mode,
	uint32 data, uint32 hot_x, uint32 hot_y, uint32 fgColor,
	uint32 bgColor, uint32 mouse_type)
{
	if (bx_options.nfvdi.use_host_mouse_cursor) {
		return VdiDriver::drawMouse(wk,x,y,mode,data,hot_x,hot_y,fgColor,bgColor,mouse_type);
	}

	DUNUSED(wk);
	DUNUSED(fgColor);
	DUNUSED(bgColor);
	DUNUSED(mouse_type);
	switch (mode) {
		case 4:
		case 0:  // move shown
			restoreMouseBackground();
			break;
		case 5:
		case 1:  // move hidden
			return 1;
		case 2:  // hide
			restoreMouseBackground();
			return 1;
		case 3:  // show
			break;
		case 6:
		case 7:
			break;

		default: // change pointer shape
			{
				Uint16 *cur_data, *cur_mask;
				int mx,my;

				cur_data = (Uint16 *)Atari2HostAddr(data);
				cur_mask = (Uint16 *)Atari2HostAddr(mode);
				for (my=0;my<16;my++) {
					for (mx=0;mx<16;mx++) {
						Uint32 color=0xffffff00UL;	/* white */

						if (SDL_SwapBE16(cur_data[my]) & (1<<mx))
							color = 0x00000000UL;	/* black */
						if (SDL_SwapBE16(cur_mask[my]) & (1<<mx))
							color |= 0x000000ffUL;	/* opaque */
				
						mouse_texture[my][15-mx]=SDL_SwapBE32(color);
					}
				}

				gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); // scale when image bigger than texture
				gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); // scale when image smaller than texture
				gl.TexImage2D(GL_TEXTURE_2D, 0, 4, 16, 16, 0, GL_RGBA, GL_UNSIGNED_BYTE, mouse_texture);

				Mouse.hotspot.x = hot_x & 0xf;
				Mouse.hotspot.y = hot_y & 0xf;
				return 1;
			}
	}

	// handle the mouse hotspot point
	x -= Mouse.hotspot.x;
	y -= Mouse.hotspot.y;

	// beware of the edges of the screen
	int maxx=host->video->getWidth(), maxy=host->video->getHeight();

	x = (x>maxx ? maxx : (x<0 ? 0: x));
	y = (y>maxy ? maxy : (y<0 ? 0: y));

	saveMouseBackground(x,y,MIN(maxx-x, 16),MIN(maxy-y-1, 16));

	gl.Enable(GL_TEXTURE_2D);
	gl.Enable(GL_BLEND);
	gl.BlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

	/* Texture may have been unbound */
	gl.BindTexture(GL_TEXTURE_2D, mouse_tex_obj);

	/* Render the textured quad */
	gl.Begin(GL_QUADS);
		gl.TexCoord2f(0.0,0.0);
		gl.Vertex2i(x,y);

		gl.TexCoord2f(1.0,0.0);
		gl.Vertex2i(x+16,y);

		gl.TexCoord2f(1.0,1.0);
		gl.Vertex2i(x+16,y+16);

		gl.TexCoord2f(0.0,1.0);
		gl.Vertex2i(x,y+16);
	gl.End();

	gl.Disable(GL_BLEND);
	gl.Disable(GL_TEXTURE_2D);
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
	width = (w + 8 + 31) & ~31;

	bitmap = (Uint8 *)malloc((width*h)>>3);
	if (bitmap==NULL) {
		return -1;
	}

	/* Copy the bitmap from source */
	srcpitch = ReadInt16(src + MFDB_WDWIDTH) * 2;
	s = (Uint8 *)Atari2HostAddr(ReadInt32(src + MFDB_ADDRESS) + sy * srcpitch + (sx >> 3));
	dstpitch = width>>3;
	d = bitmap + (dstpitch * (h-1));
	for (y=0;y<h;y++) {
		memcpy(d, s, width>>3);
		s += srcpitch;
		d -= dstpitch;	
	}

	if (logOp == 1) {
		/* First, the back color */
		gl.Color3ub((bgColor>>16)&0xff,(bgColor>>8)&0xff,bgColor&0xff);
		gl.Begin(GL_QUADS);
			gl.Vertex2i(dx, dy);
			gl.Vertex2i(dx+w, dy);
			gl.Vertex2i(dx+w, dy+h);
			gl.Vertex2i(dx, dy+h);
		gl.End();
	}

	gl.Scissor(dx, host->video->getHeight() - (dy + h), w, h);
	gl.Enable(GL_SCISSOR_TEST);
	gl.Enable(GL_COLOR_LOGIC_OP);
	if (logOp == 3) {
		gl.LogicOp(GL_XOR);
		gl.Color3ub(0xff,0xff,0xff);
	} else {
		gl.LogicOp(GL_COPY);
		gl.Color3ub((fgColor>>16)&0xff,(fgColor>>8)&0xff,fgColor&0xff);
	}

	gl.RasterPos2i(dx,dy+h-1);
	gl.Bitmap(w+8,h, sx & 7,0, 0,0, (const GLubyte *)bitmap);
	gl.Disable(GL_COLOR_LOGIC_OP);
	gl.Disable(GL_SCISSOR_TEST);

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
	if (host->video->getBpp() <= 1) {
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
			gl.Color3ub((bgColor>>16)&0xff,(bgColor>>8)&0xff,bgColor&0xff);
			gl.Begin(GL_QUADS);
				gl.Vertex2i(x,y);
				gl.Vertex2i(x+w,y);
				gl.Vertex2i(x+w,y+h);
				gl.Vertex2i(x,y+h);
			gl.End();
		}

		gl.Enable(GL_COLOR_LOGIC_OP);
		if (logOp == 3) {
			gl.LogicOp(GL_XOR);
			gl.Color3ub(0xff,0xff,0xff);
		} else {
			gl.LogicOp(GL_COPY);
			gl.Color3ub((fgColor>>16)&0xff,(fgColor>>8)&0xff,fgColor&0xff);
		}

		/* Fill with fgColor */
		gl.Enable(GL_POLYGON_STIPPLE);
		gl.PolygonStipple((const GLubyte *)gl_pattern);
		gl.Begin(GL_POLYGON);
			gl.Vertex2i(x,y);
			gl.Vertex2i(x+w,y);
			gl.Vertex2i(x+w,y+h);
			gl.Vertex2i(x,y+h);
		gl.End();
		gl.Disable(GL_POLYGON_STIPPLE);

		D(bug("glvdi:  fillarea, with polygon"));
	} else {
		for(h = h - 1; h >= 0; h--) {
			y = (int16)ReadInt16(table); table+=2;
			x = (int16)ReadInt16(table); table+=2;
			w = (int16)ReadInt16(table) - x + 1; table+=2;

			/* First, the back color */
			if (logOp==1) {
				gl.Color3ub((bgColor>>16)&0xff,(bgColor>>8)&0xff,bgColor&0xff);
				gl.Begin(GL_LINES);
					gl.Vertex2i(x,y);
					gl.Vertex2i(x+w-1,y);
				gl.End();
			}

			gl.Enable(GL_COLOR_LOGIC_OP);
			if (logOp == 3) {
				gl.LogicOp(GL_XOR);
				gl.Color3ub(0xff,0xff,0xff);
			} else {
				gl.LogicOp(GL_COPY);
				gl.Color3ub((fgColor>>16)&0xff,(fgColor>>8)&0xff,fgColor&0xff);
			}

			/* Fill with fgColor */
			gl.Enable(GL_LINE_STIPPLE);
			gl.LineStipple(1,pattern[y&15]);
			gl.Begin(GL_LINES);
				gl.Vertex2i(x,y);
				gl.Vertex2i(x+w-1,y);
			gl.End();
			gl.Disable(GL_LINE_STIPPLE);
		}
		D(bug("glvdi:  fillarea, with horizontal lines"));
	}

	gl.Disable(GL_COLOR_LOGIC_OP);
	return 1;
}

void OpenGLVdiDriver::fillArea(uint32 x, uint32 y, uint32 w, uint32 h,
                               uint16* pattern, uint32 fgColor, uint32 bgColor,
                               uint32 logOp)
{
	/* First, the back color */
	if (logOp==1) {
		gl.Color3ub((bgColor>>16)&0xff,(bgColor>>8)&0xff,bgColor&0xff);
		gl.Begin(GL_LINES);
			gl.Vertex2i(x,y);
			gl.Vertex2i(x+w-1,y+h-1);
		gl.End();
	}

	gl.Enable(GL_COLOR_LOGIC_OP);
	if (logOp == 3) {
		gl.LogicOp(GL_XOR);
		gl.Color3ub(0xff,0xff,0xff);
	} else {
		gl.LogicOp(GL_COPY);
		gl.Color3ub((fgColor>>16)&0xff,(fgColor>>8)&0xff,fgColor&0xff);
	}

	/* Fill with fgColor */
	gl.Enable(GL_LINE_STIPPLE);
	gl.LineStipple(1,pattern[y&15]);
	gl.Begin(GL_LINES);
		gl.Vertex2i(x,y);
		gl.Vertex2i(x+w-1,y+h-1);
	gl.End();
	gl.Disable(GL_LINE_STIPPLE);

	gl.Disable(GL_COLOR_LOGIC_OP);
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
			gl.Color3ub(0,0,0);
		} else if (logOp==15) {
			gl.Color3ub(0xff,0xff,0xff);
		}
		gl.Begin(GL_QUADS);
			gl.Vertex2i(dx,dy);
			gl.Vertex2i(dx+w,dy);
			gl.Vertex2i(dx+w,dy+h);
			gl.Vertex2i(dx,dy+h);
		gl.End();
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

	gl.Enable(GL_COLOR_LOGIC_OP);
	gl.LogicOp(logicOps[logOp]);

	for (y=0;y<h;y++) {
		gl.RasterPos2i(dx,dy+y+1);
		switch(planes) {
			case 16:
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
				gl.PixelStorei(GL_UNPACK_SWAP_BYTES, GL_TRUE);
#endif
				gl.DrawPixels(w,1, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, srcAddress);
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
				gl.PixelStorei(GL_UNPACK_SWAP_BYTES, GL_FALSE);
#endif
				break;
			case 32:
				gl.DrawPixels(w,1, GL_BGRA,
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
					GL_UNSIGNED_INT_8_8_8_8,
#else
					GL_UNSIGNED_INT_8_8_8_8_REV,
#endif
					srcAddress);
				break;
		}
		srcAddress += srcPitch;
	}

	gl.Disable(GL_COLOR_LOGIC_OP);
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
		bug("glvdi: blit_s2m: logOp %d unsupported",logOp);
		return -1;
	}

	uint32 destPitch = ReadInt16(dest + MFDB_WDWIDTH) * planes * 2;	// MFDB *dest->pitch
	Uint8 *destAddress = Atari2HostAddr(ReadInt32(dest));
	destAddress += dy * destPitch;
	destAddress += dx * (planes>>3);

	for (y=0;y<h;y++) {
		switch(logOp) {
			case 0:
				memset(destAddress, 0, w*(planes>>3));
				break;
			case 15:
				memset(destAddress, 0xff, w*(planes>>3));
				break;
			case 3:
				switch(planes) {
					case 16:
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
						gl.PixelStorei(GL_PACK_SWAP_BYTES, GL_TRUE);
#endif
						gl.ReadPixels(sx,host->video->getHeight()-(sy+y+1), w,1, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, destAddress);
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
						gl.PixelStorei(GL_PACK_SWAP_BYTES, GL_FALSE);
#endif
						break;
					case 32:
						gl.ReadPixels(sx,host->video->getHeight()-(sy+y+1), w,1, GL_BGRA,
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
							GL_UNSIGNED_INT_8_8_8_8,
#else
							GL_UNSIGNED_INT_8_8_8_8_REV,
#endif
							destAddress);
						break;
				}
				break;
		}
		if (planes==32) {
			/* Clear alpha value */
			for(Uint32 n = 0; n < destPitch; n += 4) {
				destAddress[n] = 0;
			}
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
			gl.Color3ub(0,0,0);
		} else if (logOp==15) {
			gl.Color3ub(0xff,0xff,0xff);
		}
		gl.Begin(GL_QUADS);
			gl.Vertex2i(dx,dy);
			gl.Vertex2i(dx+w-1,dy);
			gl.Vertex2i(dx+w-1,dy+h-1);
			gl.Vertex2i(dx,dy+h-1);
		gl.End();
		D(bug("glvdi: blit_s2s: clear rectangle"));
		return 1;
	}

	gl.Enable(GL_COLOR_LOGIC_OP);	
	gl.LogicOp(logicOps[logOp & 15]);

#if 0
	gl.RasterPos2i(dx,dy+h-1);
	gl.CopyPixels(sx,host->video->getHeight()-(sy+h-1), w,h, GL_COLOR);
#else
	if (sy >= dy) {
	  if (dy + h == (int32)host->video->getHeight())
	    h--;
	  gl.RasterPos2i(dx,dy+h);
	  gl.CopyPixels(sx,host->video->getHeight()-(sy+h), w,h, GL_COLOR);
	} else {
	  int srcy = host->video->getHeight()-(sy+h);
	  if (dy + h < (int32)host->video->getHeight()) {
	    gl.RasterPos2i(dx,dy+h);
	    gl.CopyPixels(sx,srcy, w,h, GL_COLOR);
	  } else {
	    gl.RasterPos2i(dx, host->video->getHeight() - 1);
	    gl.CopyPixels(sx,srcy+1, w,h, GL_COLOR);
	  }
	}
#endif

	gl.Disable(GL_COLOR_LOGIC_OP);
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
	if ((logOp == 1) && (pattern != 0xffff)) {
		/* First, the back color */
		gl.Color3ub((bgColor>>16)&0xff,(bgColor>>8)&0xff,bgColor&0xff);
		if ((x1 == x2) && (y1 == y2)) {
			gl.Begin(GL_POINTS);
				gl.Vertex2i(x1,y1);
			gl.End();
		} else {
			gl.Begin(GL_LINES);
				gl.Vertex2i(x1,y1);
				gl.Vertex2i(x2,y2);
			gl.End();
		}
	}

	gl.Enable(GL_COLOR_LOGIC_OP);
	if (logOp == 3) {
		gl.LogicOp(GL_XOR);
		gl.Color3ub(0xff,0xff,0xff);
	} else {
		gl.LogicOp(GL_COPY);
		gl.Color3ub((fgColor>>16)&0xff,(fgColor>>8)&0xff,fgColor&0xff);
	}

	gl.Enable(GL_LINE_STIPPLE);
	gl.LineStipple(1,pattern);

	/* Draw with fgColor */
	if ((x1 == x2) && (y1 == y2)) {
		gl.Begin(GL_POINTS);
			gl.Vertex2i(x1,y1);
		gl.End();
	} else {
		gl.Begin(GL_LINES);
			gl.Vertex2i(x1,y1);
			gl.Vertex2i(x2,y2);
		gl.End();
	}

	gl.Disable(GL_LINE_STIPPLE);
	gl.Disable(GL_COLOR_LOGIC_OP);

	D(bug("glvdi: drawline(%d,%d,%d,%d,0x%08x,0x%08x",x1,y1,x2,y2,fgColor,bgColor));
	return 1;
}

int OpenGLVdiDriver::drawTableLine(memptr table, int length, uint16 pattern,
	uint32 fgColor, uint32 bgColor, int logOp)
{
	int x, y, tmp_length;
	memptr tmp_table;

	if ((logOp == 1) && (pattern != 0xffff)) {
		/* First, the back color */
		gl.Color3ub((bgColor>>16)&0xff,(bgColor>>8)&0xff,bgColor&0xff);
		tmp_length = length;
		tmp_table = table;
		if ((ReadInt16(table) == ReadInt16(table + length * 4 - 4)) &&
		    (ReadInt16(table + 2) == ReadInt16(table + length * 4 - 2))) {
			length--;
			gl.Begin(GL_LINE_LOOP);
		} else {
			gl.Begin(GL_LINE_STRIP);
		}
		for(; length > 0; length--) {
			x = (int16)ReadInt16(table); table += 2;
			y = (int16)ReadInt16(table); table += 2;
			gl.Vertex2i(x, y);
		}
		length = tmp_length;
		table = tmp_table;
		gl.End();
	}

	gl.Enable(GL_COLOR_LOGIC_OP);
	if (logOp == 3) {
		gl.LogicOp(GL_XOR);
		gl.Color3ub(0xff,0xff,0xff);
	} else {
		gl.LogicOp(GL_COPY);
		gl.Color3ub((fgColor>>16)&0xff,(fgColor>>8)&0xff,fgColor&0xff);
	}

	gl.Enable(GL_LINE_STIPPLE);
	gl.LineStipple(1,pattern);

	/* Draw with fgColor */
	if ((ReadInt16(table) == ReadInt16(table + length * 4 - 4)) &&
	    (ReadInt16(table + 2) == ReadInt16(table + length * 4 - 2))) {
		length--;
		gl.Begin(GL_LINE_LOOP);
	} else {
		gl.Begin(GL_LINE_STRIP);
	}
	for(; length > 0; length--) {
		x = (int16)ReadInt16(table); table += 2;
		y = (int16)ReadInt16(table); table += 2;
		gl.Vertex2i(x, y);
	}
	gl.End();

	gl.Disable(GL_LINE_STIPPLE);
	gl.Disable(GL_COLOR_LOGIC_OP);

	D(bug("glvdi: drawTableLine(%d,0x%08x,0x%08x",length,fgColor,bgColor));
	return 1;
}

int OpenGLVdiDriver::drawMoveLine(memptr table, int length, memptr index, int moves,
	uint16 pattern, uint32 fgColor, uint32 bgColor, int logOp)
{
	int x1 = (int16)ReadInt16(table); table+=2;
	int y1 = (int16)ReadInt16(table); table+=2;
	moves *= 2;
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
	int cx1,cy1,cx2,cy2;

	if (host->video->getBpp() <= 1) {
		fgColor &= 0xff;
		bgColor &= 0xff;
	}

	cx1=ReadInt32(clip);
	cy1=ReadInt32(clip+4);
	cx2=ReadInt32(clip+8);
	cy2=ReadInt32(clip+12);

	gl.Scissor(cx1,host->video->getHeight()-(cy2+1),cx2-cx1+1,cy2-cy1+1);
	gl.Enable(GL_SCISSOR_TEST);

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

	gl.Disable(GL_SCISSOR_TEST);
	return 1;
}

/* Polygon tesselator callback functions */

#if ENABLE_GLU_TESSELATOR

#ifndef CALLBACK
#define CALLBACK
#endif

extern "C" {
	static void CALLBACK tess_begin(GLenum which) {
		gl.Begin(which);
	}

	static void CALLBACK tess_end(void) {
		gl.End();
	}

	static void CALLBACK tess_error(GLenum errorCode) {
		fprintf(stderr,"glvdi: Tesselation error: %s\n", gluErrorString(errorCode));
	}
}
#endif

int32 OpenGLVdiDriver::fillPoly(memptr vwk, memptr points_addr, int n,
	memptr index_addr, int moves, memptr pattern_addr, uint32 fgColor,
	uint32 bgColor, uint32 logOp, uint32 interior_style, memptr clip)
{
#if !ENABLE_GLU_TESSELATOR
	return VdiDriver::fillPoly(vwk, points_addr, n, index_addr, moves,
	                           pattern_addr, fgColor, bgColor, logOp,
	                           interior_style, clip);
#else
	int i,cx1,cy1,cx2,cy2, tess_list;
	GLdouble *poly_coords;
	uint32 gl_pattern[32];

	DUNUSED(index_addr);
	DUNUSED(interior_style);

	if (vwk & 1)
		return -1;      // Don't know about any special fills
	if (!n)
		return 1;

	if (moves) {
		D(bug("nfvdi:opengl: fillPoly():moves table unsupported"));
		return 1;
	}

	uint16 pattern[16];
	for(i=0;i<16;i++)
		pattern[i] = ReadInt16(pattern_addr + i * 2);

	memset(gl_pattern,0, sizeof(gl_pattern));
	for (i=0; i<32; i++) {
		gl_pattern[i] = (pattern[i&15]<<16)|pattern[i&15];
	}

	cx1=ReadInt32(clip);
	cy1=ReadInt32(clip+4);
	cx2=ReadInt32(clip+8);
	cy2=ReadInt32(clip+12);

	gl.Scissor(cx1,host->video->getHeight()-(cy2+1),cx2-cx1+1,cy2-cy1+1);
	gl.Enable(GL_SCISSOR_TEST);

	/* Create tesselator */
	if (!tess) {
		tess=gluNewTess();
		gluTessCallback(tess,GLU_TESS_VERTEX,(_GLUfuncptr)gl.Vertex3dv);
		gluTessCallback(tess,GLU_TESS_BEGIN,(_GLUfuncptr)tess_begin);
		gluTessCallback(tess,GLU_TESS_END,(_GLUfuncptr)tess_end);
		gluTessCallback(tess,GLU_TESS_ERROR,(_GLUfuncptr)tess_error);
	}

	/* Tesselate polygon in list */
	tess_list = gl.GenLists(1);
	poly_coords = new GLdouble[3*n];	

	gl.NewList(tess_list, GL_COMPILE);
		gluTessBeginPolygon(tess, NULL);
			gluTessBeginContour(tess);
			for (i=0;i<n;i++) {
				poly_coords[i*3+0] = (GLdouble) ReadInt16(points_addr + i*4);
				poly_coords[i*3+1] = (GLdouble) ReadInt16(points_addr + i*4 +2);
				poly_coords[i*3+2] = 0.0;
				gluTessVertex(tess, &poly_coords[i*3], &poly_coords[i*3]);
			}
			gluTessEndContour(tess);
		gluTessEndPolygon(tess);
	gl.EndList();

	delete poly_coords;

	if (logOp == 1) {
		/* First, the back color */
		gl.Color3ub((bgColor>>16)&0xff,(bgColor>>8)&0xff,bgColor&0xff);
		gl.CallList(tess_list);
	}

	gl.Enable(GL_COLOR_LOGIC_OP);
	if (logOp == 3) {
		gl.LogicOp(GL_XOR);
		gl.Color3ub(0xff,0xff,0xff);
	} else {
		gl.LogicOp(GL_COPY);
		gl.Color3ub((fgColor>>16)&0xff,(fgColor>>8)&0xff,fgColor&0xff);
	}

	gl.Enable(GL_POLYGON_STIPPLE);
	gl.PolygonStipple((const GLubyte *)gl_pattern);

	gl.CallList(tess_list);

	gl.DeleteLists(tess_list,1);
	gl.Disable(GL_POLYGON_STIPPLE);
	gl.Disable(GL_SCISSOR_TEST);
	D(bug("glvdi: fillpoly"));
	return 1;
#endif
}

int32 OpenGLVdiDriver::drawText(memptr vwk, memptr text, uint32 length,
				int32 dx, int32 dy, memptr font,
				uint32 ch_w, uint32 ch_h, uint32 fgColor, uint32 bgColor,
				uint32 logOp, memptr clip)
{
	DUNUSED(vwk);
	int32 cx1, cy1, cx2, cy2;
	cx1 = ReadInt32(clip);
	cy1 = ReadInt32(clip + 4);
	cx2 = ReadInt32(clip + 8);
	cy2 = ReadInt32(clip + 12);

	int width, count;
	if ((dy + (int32)ch_h <= cy1) || (dy > cy2) || (dx > cx2) ||
	    (dx + (width = length * (int32)ch_w) <= cx1))
		return 1;
	if (dx + width - (int32)ch_w > cx2) {
		count   = dx + width - cx2 - 1;
		count  /= ch_w;
		length -= count;
	}
	if (dx + (int32)ch_w <= cx1) {
		count   = cx1 - dx;
		count  /= ch_w;
		length -= count;
		dx     += ch_w * count;
		text   += count * 2;
	}

	/* Allocate temporary space for monochrome bitmap */
	width = (ch_w * length + 31) & ~31;	/* Changed meaning of width from above! */
	Uint8 *bitmap = (Uint8 *)malloc((width * ch_h) >> 3);
	if (!bitmap) {
		return -1;
	}

	uint8 *fontaddr = Atari2HostAddr(font);
	width >>= 3;
	for(uint32 i = 0; i < length; i++) {
	  Uint8 *chardata = &fontaddr[ReadInt16(text) * 16];
	  text += 2;
	  Uint8 *ptr = bitmap + i + (ch_h - 1) * width;
	  for(uint32 j = 0; j < ch_h; j++) {
	    *ptr = *chardata++;
	    ptr -= width;
	  }
	}

	int sx, sy, w, h;
	sx = 0;
	sy = 0;
	w = ch_w * length;
	h = ch_h;

	gl.Scissor(cx1,           host->video->getHeight() - cy2 - 1,
		  cx2 - cx1 + 1, cy2 - cy1 + 1);
	gl.Enable(GL_SCISSOR_TEST);

	if (logOp == 1) {
		/* First, the back color */
		gl.Color3ub((bgColor >> 16) & 0xff, (bgColor >> 8) & 0xff,
			   bgColor & 0xff);
		gl.Begin(GL_QUADS);
			gl.Vertex2i(dx,     dy);
			gl.Vertex2i(dx + w, dy);
			gl.Vertex2i(dx + w, dy + h);
			gl.Vertex2i(dx,     dy + h);
		gl.End();
	}

	gl.Enable(GL_COLOR_LOGIC_OP);
	if (logOp == 3) {
		gl.LogicOp(GL_XOR);
		gl.Color3ub(0xff, 0xff, 0xff);
	} else {
		gl.LogicOp(GL_COPY);
		gl.Color3ub((fgColor >> 16) & 0xff, (fgColor >> 8) & 0xff,
			   fgColor & 0xff);
	}

	sx = 0;
	if (dx < 0) {
	  sx = -dx;
	  dx = 0;
	}
	gl.RasterPos2i(dx, dy + h - 1);
	gl.Bitmap(w, h,  sx, 0,  0, 0, (const GLubyte *)bitmap);

	gl.Disable(GL_COLOR_LOGIC_OP);
	gl.Disable(GL_SCISSOR_TEST);

	free(bitmap);

	return 1;
}

void OpenGLVdiDriver::getHwColor(uint16 /*index*/, uint32 red, uint32 green,
	uint32 blue, memptr hw_value)
{
	WriteInt32(hw_value, (((red * 255 + 500) / 1000) << 16) |
		   (((green * 255 + 500) / 1000) << 8)|((blue * 255 + 500) / 1000));
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
