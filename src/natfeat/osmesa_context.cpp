/*
	NatFeat host OSMesa rendering

	ARAnyM (C) 2004,2005 Patrice Mandin

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
#include "SDL_compat.h"
#include <SDL_loadso.h>
#include <SDL_endian.h>
#if !defined(SDL_VIDEO_DRIVER_X11) || defined(HAVE_X11_XLIB_H)
#include <SDL_syswm.h>
#endif
#include <math.h>

#include "cpu_emulation.h"
#include "parameters.h"
#include "nfosmesa.h"
#include "../../atari/nfosmesa/nfosmesa_nfapi.h"
#include "osmesa_context.h"
#if SDL_VERSION_ATLEAST(2, 0, 0)
#include "host.h"
#endif

#define DEBUG 0
#include "debug.h"

#ifdef NFOSMESA_SUPPORT
/*--- Defines ---*/

#define VDI_ARGB			0x8
#define VDI_RGB				0xf
#define DIRECT_VDI_ARGB		0x10

/*--- Classes ---*/

/*
 * class to save current pixel pack attributes,
 * set them to default values and
 * restore them in the destructor
 */
class PixelPackAttributes {
private:
	bool restore;
public:
	GLint pack_swap_bytes;
	GLint pack_lsb_first;
	GLint pack_row_length;
	GLint pack_image_height;
	GLint pack_skip_pixels;
	GLint pack_skip_rows;
	GLint pack_skip_images;
	GLint pack_alignment;
	GLint pack_invert;
	PixelPackAttributes(bool save_and_restore = true);
	virtual ~PixelPackAttributes(void);
};

/*-----------------------------------------------------------------------*/

PixelPackAttributes::PixelPackAttributes(bool save_and_restore) :
	restore(save_and_restore),
	pack_swap_bytes(GL_FALSE),
	pack_lsb_first(GL_FALSE),
	pack_row_length(0),
	pack_image_height(0),
	pack_skip_pixels(0),
	pack_skip_rows(0),
	pack_skip_images(0),
	pack_alignment(4),
	pack_invert(GL_FALSE)
{
	OSMesaDriver::fn.glGetIntegerv(GL_PACK_SWAP_BYTES, &pack_swap_bytes);
	OSMesaDriver::fn.glGetIntegerv(GL_PACK_LSB_FIRST, &pack_lsb_first);
	OSMesaDriver::fn.glGetIntegerv(GL_PACK_ROW_LENGTH, &pack_row_length);
	OSMesaDriver::fn.glGetIntegerv(GL_PACK_IMAGE_HEIGHT, &pack_image_height);
	OSMesaDriver::fn.glGetIntegerv(GL_PACK_SKIP_PIXELS, &pack_skip_pixels);
	OSMesaDriver::fn.glGetIntegerv(GL_PACK_SKIP_ROWS, &pack_skip_rows);
	OSMesaDriver::fn.glGetIntegerv(GL_PACK_SKIP_IMAGES, &pack_skip_images);
	OSMesaDriver::fn.glGetIntegerv(GL_PACK_ALIGNMENT, &pack_alignment);
	if (OffscreenContext::has_MESA_pack_invert)
		OSMesaDriver::fn.glGetIntegerv(GL_PACK_INVERT_MESA, &pack_invert);
		
	if (save_and_restore)
	{
		OSMesaDriver::fn.glPixelStorei(GL_PACK_SWAP_BYTES, GL_FALSE);
		OSMesaDriver::fn.glPixelStorei(GL_PACK_LSB_FIRST, GL_FALSE);
		OSMesaDriver::fn.glPixelStorei(GL_PACK_ROW_LENGTH, 0);
		OSMesaDriver::fn.glPixelStorei(GL_PACK_IMAGE_HEIGHT, 0);
		OSMesaDriver::fn.glPixelStorei(GL_PACK_SKIP_PIXELS, 0);
		OSMesaDriver::fn.glPixelStorei(GL_PACK_SKIP_ROWS, 0);
		OSMesaDriver::fn.glPixelStorei(GL_PACK_SKIP_IMAGES, 0);
		OSMesaDriver::fn.glPixelStorei(GL_PACK_ALIGNMENT, 4);
		if (OffscreenContext::has_MESA_pack_invert)
			OSMesaDriver::fn.glPixelStorei(GL_PACK_INVERT_MESA, GL_FALSE);
	}
}

/*-----------------------------------------------------------------------*/

PixelPackAttributes::~PixelPackAttributes(void)
{
	if (restore)
	{
		OSMesaDriver::fn.glPixelStorei(GL_PACK_SWAP_BYTES, pack_swap_bytes);
		OSMesaDriver::fn.glPixelStorei(GL_PACK_LSB_FIRST, pack_lsb_first);
		OSMesaDriver::fn.glPixelStorei(GL_PACK_ROW_LENGTH, pack_row_length);
		OSMesaDriver::fn.glPixelStorei(GL_PACK_IMAGE_HEIGHT, pack_image_height);
		OSMesaDriver::fn.glPixelStorei(GL_PACK_SKIP_PIXELS, pack_skip_pixels);
		OSMesaDriver::fn.glPixelStorei(GL_PACK_SKIP_ROWS, pack_skip_rows);
		OSMesaDriver::fn.glPixelStorei(GL_PACK_SKIP_IMAGES, pack_skip_images);
		OSMesaDriver::fn.glPixelStorei(GL_PACK_ALIGNMENT, pack_alignment);
		if (OffscreenContext::has_MESA_pack_invert)
			OSMesaDriver::fn.glPixelStorei(GL_PACK_INVERT_MESA, pack_invert);
	}
}

/*************************************************************************/
/*-----------------------------------------------------------------------*/
/*************************************************************************/

/*--- Base Class ---*/

GLboolean OffscreenContext::has_MESA_pack_invert = GL_FALSE;

OffscreenContext::OffscreenContext(void *glhandle) :
	lib_handle(glhandle),
	userRowLength(0),
	yup(GL_TRUE),
	dst_buffer(0),
	host_buffer(NULL),
	buffer_width(0),
	buffer_height(0),
	buffer_bpp(0),
	srcformat(GL_NONE),
	dstformat(GL_NONE),
	destination_bpp(0),
	conversion(false),
	swapcomponents(false),
	error_check_enabled(GL_FALSE),
	type(GL_NONE),
	width(0),
	height(0)
{
}

/*-----------------------------------------------------------------------*/

OffscreenContext::~OffscreenContext()
{
	if (host_buffer)
	{
		free(host_buffer);
		host_buffer = NULL;
	}
}

/*-----------------------------------------------------------------------*/

void OffscreenContext::PixelStore(GLint pname, GLint value)
{
	switch (pname)
	{
	case OSMESA_ROW_LENGTH:
		if (value >= 0)
			setUserRowLength(value);
		break;
	case OSMESA_Y_UP:
		setYup(value ? GL_TRUE : GL_FALSE);
		break;
	}
}

/*-----------------------------------------------------------------------*/

void OffscreenContext::GetColorBuffer(GLint *_width, GLint *_height, GLint *_format, memptr *_buffer)
{
	*_width = width;
	*_height = height;
	*_format = dstformat;
	*_buffer = dst_buffer;
}

/*-----------------------------------------------------------------------*/

void OffscreenContext::GetDepthBuffer(GLint *_width, GLint *_height, GLint *_bytesPerValue, memptr *_buffer)
{
	*_width = width;
	*_height = height;
	*_bytesPerValue = 0;
	/*
	 * Can not return pointer in host memory;
	 * if this is ever needed, we have to split it like glGetString()
	 */
	*_buffer = 0;
}

/*-----------------------------------------------------------------------*/

bool OffscreenContext::FormatHasAlpha(GLenum format)
{
	switch (format)
	{
	case OSMESA_RGBA:
	/* case GL_RGBA: */
	case OSMESA_BGRA:
	case GL_BGRA_EXT:
	case OSMESA_ARGB:
	case VDI_ARGB:
	case DIRECT_VDI_ARGB:
		return true;
	}
	return false;
}

/*-----------------------------------------------------------------------*/

void OffscreenContext::ResizeBuffer(GLsizei newBpp)
{
	if (host_buffer == NULL ||
		width != buffer_width ||
		height != buffer_height ||
		newBpp != buffer_bpp)
	{
		host_buffer = realloc(host_buffer, width * height * newBpp);
		buffer_width = width;
		buffer_height = height;
		buffer_bpp = newBpp;
	}
}

/*-----------------------------------------------------------------------*/

void OffscreenContext::ConvertContext()
{
	if (!conversion)
		return;

	switch (bx_options.osmesa.channel_size)
	{
	case 16:
		ConvertContext16();
		break;
	case 32:
		ConvertContext32();
		break;
	case 8:
	default:
		ConvertContext8();
		break;
	}
}

/*-----------------------------------------------------------------------*/

void OffscreenContext::ConvertContext8()
{
	int x, y, srcpitch, dstpitch;

	switch (dstformat)
	{
	case OSMESA_RGB_565:
		/*
		 * assumes srcformat also == OSMESA_RGB_565
		 */
		if (host_buffer)
		{
			Uint16 *srcline, *srccol, color;
			memptr dstline, dstcol;
			
			srcline = (Uint16 *)host_buffer;
			srcpitch = width;
			dstline = dst_buffer;
			dstpitch = getUserRowLength() << 1;
			if (UpsideDown())
			{
				dstline += (height - 1) * dstpitch;
				dstpitch = -dstpitch;
			}
			for (y = 0; y < height; y++)
			{
				srccol = srcline;
				dstcol = dstline;
				for (x = 0; x < width; x++)
				{
					color = *srccol++;
					WriteInt16(dstcol, color);
					dstcol += 2;
				}
				srcline += srcpitch;
				dstline += dstpitch;
			}
		} else
		{
			/*
			 * without host_buffer; colorbuffer was already written to atari memory
			 */
			Uint16 *srcline, *srccol, color;

			srcline = (Uint16 *)Atari2HostAddr(dst_buffer);
			srcpitch = getUserRowLength();
			for (y = 0; y < height; y++)
			{
				srccol = srcline;
				for (x = 0; x < width; x++)
				{
					color = SDL_SwapBE16(*srccol);
					*srccol++ = color;
				}
				srcline += srcpitch;
			}
		}
		break;
	
	/*
	 * all others are byte formats, and we are only here
	 * because we can't do glReadPixels() directly to atari memory
	 * assumes srcformat == OSMESA_ARGB
	 */
	case OSMESA_RGB:
	case VDI_RGB:
	/* case GL_RGB: */
		{
			memptr dstline, dstcol;
			Uint8 *srcline, *srccol;
			
			srcline = (Uint8 *)host_buffer;
			srcpitch = width * 4;
			dstline = dst_buffer;
			dstpitch = getUserRowLength() * 3;
			if (UpsideDown())
			{
				dstline += (height - 1) * dstpitch;
				dstpitch = -dstpitch;
			}
			for (y = 0; y < height; y++)
			{
				srccol = srcline;
				dstcol = dstline;
				for (x = 0; x < width; x++)
				{
					WriteInt8(dstcol++, srccol[1]);
					WriteInt8(dstcol++, srccol[2]);
					WriteInt8(dstcol++, srccol[3]);
					srccol += 4;
				}
				srcline += srcpitch;
				dstline += dstpitch;
			}
		}
		break;

	case OSMESA_BGR:
	case GL_BGR:
		{
			memptr dstline, dstcol;
			Uint8 *srcline, *srccol;

			srcline = (Uint8 *)host_buffer;
			srcpitch = width * 4;
			dstline = dst_buffer;
			dstpitch = getUserRowLength() * 3;
			if (UpsideDown())
			{
				dstline += (height - 1) * dstpitch;
				dstpitch = -dstpitch;
			}
			for (y = 0; y < height; y++)
			{
				srccol = srcline;
				dstcol = dstline;
				for (x = 0; x < width; x++)
				{
					WriteInt8(dstcol++, srccol[3]);
					WriteInt8(dstcol++, srccol[2]);
					WriteInt8(dstcol++, srccol[1]);
					srccol += 4;
				}
				srcline += srcpitch;
				dstline += dstpitch;
			}
		}
		break;
	case OSMESA_BGRA:
	case GL_BGRA_EXT:
		{
			memptr dstline, dstcol;
			Uint8 *srcline, *srccol;

			srcline = (Uint8 *)host_buffer;
			srcpitch = width * 4;
			dstline = dst_buffer;
			dstpitch = getUserRowLength() * 4;
			if (UpsideDown())
			{
				dstline += (height - 1) * dstpitch;
				dstpitch = -dstpitch;
			}
			for (y = 0; y < height; y++)
			{
				srccol = srcline;
				dstcol = dstline;
				for (x = 0; x < width; x++)
				{
					WriteInt8(dstcol++, srccol[3]);
					WriteInt8(dstcol++, srccol[2]);
					WriteInt8(dstcol++, srccol[1]);
					WriteInt8(dstcol++, srccol[0]);
					srccol += 4;
				}
				srcline += srcpitch;
				dstline += dstpitch;
			}
		}
		break;
	case OSMESA_ARGB:
	case VDI_ARGB:
	case DIRECT_VDI_ARGB:
#if NFOSMESA_NEED_BYTE_CONV
		{
			memptr dstline, dstcol;
			Uint8 *srcline, *srccol;

			srcline = (Uint8 *)host_buffer;
			srcpitch = width * 4;
			dstline = dst_buffer;
			dstpitch = getUserRowLength() * 4;
			if (UpsideDown())
			{
				dstline += (height - 1) * dstpitch;
				dstpitch = -dstpitch;
			}
			for (y = 0; y < height; y++)
			{
				srccol = srcline;
				dstcol = dstline;
				for (x = 0; x < width; x++)
				{
					WriteInt8(dstcol++, srccol[0]);
					WriteInt8(dstcol++, srccol[1]);
					WriteInt8(dstcol++, srccol[2]);
					WriteInt8(dstcol++, srccol[3]);
					srccol += 4;
				}
				srcline += srcpitch;
				dstline += dstpitch;
			}
		}
#else
		{
			Uint32 *dstline, *dstcol;
			Uint32 *srcline, *srccol;

			srcline = (Uint32 *)host_buffer;
			srcpitch = width;
			dstline = (Uint32 *) Atari2HostAddr(dst_buffer);
			dstpitch = getUserRowLength();
			if (UpsideDown())
			{
				dstline += (height - 1) * dstpitch;
				dstpitch = -dstpitch;
			}
			for (y = 0; y < height; y++)
			{
				srccol = srcline;
				dstcol = dstline;
				for (x = 0; x < width; x++)
				{
					*dstcol++ = *srccol++;
				}
				srcline += srcpitch;
				dstline += dstpitch;
			}
		}
#endif
		break;
	}
}

/*-----------------------------------------------------------------------*/

/*
 * assumes srcformat == OSMESA_ARGB
 */
void OffscreenContext::ConvertContext16()
{
	int x,y, r,g,b,a, srcpitch, dstpitch, color;
	Uint16 *srcline, *srccol;

	srcline = (Uint16 *) host_buffer;
	srcpitch = width * 4;

	switch(dstformat)
	{
	case OSMESA_RGB_565:
		{
			memptr dstline, dstcol;

			dstline = dst_buffer;
			dstpitch = getUserRowLength() << 1;
			for (y = 0; y < height; y++)
			{
				srccol = srcline;
				dstcol = dstline;
				for (x = 0; x < width; x++)
				{
					srccol++; /* Skip alpha */
					r = ((*srccol++)>>11) & 31;
					g = ((*srccol++)>>10) & 63;
					b = ((*srccol++)>>11) & 31;

					color = (r<<11)|(g<<5)|b;
					WriteInt16(dstcol, color);
					dstcol += 2;
				}
				srcline += srcpitch;
				dstline += dstpitch;
			}
		}
		break;
	case OSMESA_RGB:
	case VDI_RGB:
	/* case GL_RGB: */
		{
			memptr dstline, dstcol;

			dstline = dst_buffer;
			dstpitch = getUserRowLength() * 3;
			for (y = 0; y < height; y++)
			{
				srccol = srcline;
				dstcol = dstline;
				for (x = 0; x < width; x++)
				{
					srccol++; /* Skip alpha */
					r = ((*srccol++)>>8) & 255;
					g = ((*srccol++)>>8) & 255;
					b = ((*srccol++)>>8) & 255;

					WriteInt8(dstcol++, r);
					WriteInt8(dstcol++, g);
					WriteInt8(dstcol++, b);
				}
				srcline += srcpitch;
				dstline += dstpitch;
			}
		}
		break;
	case OSMESA_BGR:
	case GL_BGR:
		{
			memptr dstline, dstcol;

			dstline = dst_buffer;
			dstpitch = getUserRowLength() * 3;
			for (y = 0; y < height; y++)
			{
				srccol = srcline;
				dstcol = dstline;
				for (x = 0; x < width; x++)
				{
					srccol++; /* Skip alpha */
					r = ((*srccol++)>>8) & 255;
					g = ((*srccol++)>>8) & 255;
					b = ((*srccol++)>>8) & 255;

					WriteInt8(dstcol++, b);
					WriteInt8(dstcol++, g);
					WriteInt8(dstcol++, r);
				}
				srcline += srcpitch;
				dstline += dstpitch;
			}
		}
		break;
	case OSMESA_BGRA:
	case GL_BGRA_EXT:
		{
			memptr dstline, dstcol;

			dstline = dst_buffer;
			dstpitch = getUserRowLength() << 2;
			for (y = 0; y < height; y++)
			{
				srccol = srcline;
				dstcol = dstline;
				for (x = 0; x < width;x ++)
				{
					a = ((*srccol++)>>8) & 255;
					r = ((*srccol++)>>8) & 255;
					g = ((*srccol++)>>8) & 255;
					b = ((*srccol++)>>8) & 255;

					color = (b<<24)|(g<<16)|(r<<8)|a;
					WriteInt32(dstcol, color);
					dstcol += 4;
				}
				srcline += srcpitch;
				dstline += dstpitch;
			}
		}
		break;
	case OSMESA_ARGB:
	case VDI_ARGB:
	case DIRECT_VDI_ARGB:
		{
			memptr dstline, dstcol;

			dstline = dst_buffer;
			dstpitch = getUserRowLength() << 2;
			for (y = 0; y < height; y++)
			{
				srccol = srcline;
				dstcol = dstline;
				for (x = 0; x < width; x++)
				{
					a = ((*srccol++)>>8) & 255;
					r = ((*srccol++)>>8) & 255;
					g = ((*srccol++)>>8) & 255;
					b = ((*srccol++)>>8) & 255;

					color = (a<<24)|(r<<16)|(g<<8)|b;
					WriteInt32(dstcol, color);
					dstcol += 4;
				}
				srcline += srcpitch;
				dstline += dstpitch;
			}
		}
		break;
	case OSMESA_RGBA:
	/* case GL_RGBA: */
		{
			memptr dstline, dstcol;

			dstline = dst_buffer;
			dstpitch = getUserRowLength() << 2;
			for (y = 0; y < height; y++)
			{
				srccol = srcline;
				dstcol = dstline;
				for (x = 0; x < width; x++)
				{
					a = ((*srccol++)>>8) & 255;
					r = ((*srccol++)>>8) & 255;
					g = ((*srccol++)>>8) & 255;
					b = ((*srccol++)>>8) & 255;

					color = (r<<24)|(g<<16)|(b<<8)|a;
					WriteInt32(dstcol, color);
					dstcol += 4;
				}
				srcline += srcpitch;
				dstline += dstpitch;
			}
		}
		break;
	}
}

/*-----------------------------------------------------------------------*/

/*
 * assumes srcformat == OSMESA_ARGB
 */
void OffscreenContext::ConvertContext32()
{
#define FLOAT_TO_INT(source, value, maximum) \
	{ \
		value = (int) (source * maximum ## .0); \
		if (value>maximum) value=maximum; \
		if (value<0) value=0; \
	}

	int x,y, r,g,b,a, srcpitch, dstpitch, color;
	float *srcline, *srccol;

	srcline = (float *) host_buffer;
	srcpitch = width * 4;

	switch(dstformat)
	{
	case OSMESA_RGB_565:
		{
			memptr dstline, dstcol;

			dstline = dst_buffer;
			dstpitch = getUserRowLength() << 1;
			for (y = 0; y < height; y++)
			{
				srccol = srcline;
				dstcol = dstline;
				for (x = 0; x < width; x++)
				{
					srccol++; /* Skip alpha */
					FLOAT_TO_INT(*srccol++, r, 31);
					FLOAT_TO_INT(*srccol++, g, 63);
					FLOAT_TO_INT(*srccol++, b, 31);

					color = (r<<11)|(g<<5)|b;
					WriteInt16(dstcol, color);
					dstcol += 2;
				}
				srcline += srcpitch;
				dstline += dstpitch;
			}
		}
		break;
	case OSMESA_RGB:
	case VDI_RGB:
	/* case GL_RGB: */
		{
			memptr dstline, dstcol;

			dstline = dst_buffer;
			dstpitch = getUserRowLength() * 3;
			for (y = 0; y < height; y++)
			{
				srccol = srcline;
				dstcol = dstline;
				for (x = 0; x < width; x++)
				{
					srccol++; /* Skip alpha */
					FLOAT_TO_INT(*srccol++, r, 255);
					FLOAT_TO_INT(*srccol++, g, 255);
					FLOAT_TO_INT(*srccol++, b, 255);

					WriteInt8(dstcol++, r);
					WriteInt8(dstcol++, g);
					WriteInt8(dstcol++, b);
				}
				srcline += srcpitch;
				dstline += dstpitch;
			}
		}
		break;
	case OSMESA_BGR:
	case GL_BGR:
		{
			memptr dstline, dstcol;

			dstline = dst_buffer;
			dstpitch = getUserRowLength() * 3;
			for (y = 0; y < height; y++)
			{
				srccol = srcline;
				dstcol = dstline;
				for (x = 0; x < width; x++)
				{
					srccol++; /* Skip alpha */
					FLOAT_TO_INT(*srccol++, r, 255);
					FLOAT_TO_INT(*srccol++, g, 255);
					FLOAT_TO_INT(*srccol++, b, 255);

					WriteInt8(dstcol++, b);
					WriteInt8(dstcol++, g);
					WriteInt8(dstcol++, r);
				}
				srcline += srcpitch;
				dstline += dstpitch;
			}
		}
		break;
	case OSMESA_BGRA:
	case GL_BGRA_EXT:
		{
			memptr dstline, dstcol;

			dstline = dst_buffer;
			dstpitch = getUserRowLength() << 2;
			for (y = 0; y < height; y++)
			{
				srccol = srcline;
				dstcol = dstline;
				for (x = 0; x < width; x++)
				{
					FLOAT_TO_INT(*srccol++, a, 255);
					FLOAT_TO_INT(*srccol++, r, 255);
					FLOAT_TO_INT(*srccol++, g, 255);
					FLOAT_TO_INT(*srccol++, b, 255);

					color = (b<<24)|(g<<16)|(r<<8)|a;
					WriteInt32(dstcol, color);
					dstcol += 4;
				}
				srcline += srcpitch;
				dstline += dstpitch;
			}
		}
		break;
	case OSMESA_ARGB:
	case VDI_ARGB:
	case DIRECT_VDI_ARGB:
		{
			memptr dstline, dstcol;

			dstline = dst_buffer;
			dstpitch = getUserRowLength() << 2;
			for (y = 0; y < height; y++)
			{
				srccol = srcline;
				dstcol = dstline;
				for (x = 0; x < width; x++)
				{
					FLOAT_TO_INT(*srccol++, a, 255);
					FLOAT_TO_INT(*srccol++, r, 255);
					FLOAT_TO_INT(*srccol++, g, 255);
					FLOAT_TO_INT(*srccol++, b, 255);

					color = (a<<24)|(r<<16)|(g<<8)|b;
					WriteInt32(dstcol, color);
					dstcol += 4;
				}
				srcline += srcpitch;
				dstline += dstpitch;
			}
		}
		break;
	case OSMESA_RGBA:
	/* case GL_RGBA: */
		{
			memptr dstline, dstcol;

			dstline = dst_buffer;
			dstpitch = getUserRowLength() << 2;
			for (y = 0; y < height; y++)
			{
				srccol = srcline;
				dstcol = dstline;
				for (x = 0; x < width;x++)
				{
					FLOAT_TO_INT(*srccol++, a, 255);
					FLOAT_TO_INT(*srccol++, r, 255);
					FLOAT_TO_INT(*srccol++, g, 255);
					FLOAT_TO_INT(*srccol++, b, 255);

					color = (r<<24)|(g<<16)|(g<<8)|a;
					WriteInt32(dstcol, color);
					dstcol += 4;
				}
				srcline += srcpitch;
				dstline += dstpitch;
			}
		}
		break;
	}
}

/*************************************************************************/
/*-----------------------------------------------------------------------*/
/*************************************************************************/

/*--- Mesa Class ---*/

MesaContext::MesaContext(void *glhandle) :
	OffscreenContext(glhandle),
	ctx(0)
{
}

/*-----------------------------------------------------------------------*/

MesaContext::~MesaContext()
{
	if (ctx)
	{
		OSMesaDriver::fn.OSMesaDestroyContext(ctx);
	}
}

/*-----------------------------------------------------------------------*/

bool MesaContext::CreateContext(GLenum format, GLint depthBits, GLint stencilBits, GLint accumBits, OffscreenContext *share_ctx)
{
	MesaContext *mesa_share = (MesaContext *)share_ctx;
	OSMesaContext share = mesa_share ? mesa_share->ctx : NULL;

	dstformat = format;

	/* Select format */
	srcformat = format;
	conversion = false;
	swapcomponents = false;
	destination_bpp = 0;

	switch (dstformat)
	{
	case OSMESA_COLOR_INDEX:
	/* case GL_COLOR_INDEX: */
		dstformat = GL_COLOR_INDEX;
		destination_bpp = 1;
		break;
	case OSMESA_RGBA:
	/* case GL_RGBA: */
		dstformat = GL_RGBA;
		destination_bpp = 4;
		break;
	case OSMESA_BGRA:
	case GL_BGRA_EXT:
		dstformat = GL_BGRA_EXT;
		destination_bpp = 4;
		break;
	case OSMESA_ARGB:
	case VDI_ARGB:
	case DIRECT_VDI_ARGB:
		dstformat = OSMESA_ARGB;
		destination_bpp = 4;
		break;
	case OSMESA_RGB:
	case VDI_RGB:
	/* case GL_RGB: */
		dstformat = GL_RGB;
		destination_bpp = 3;
		break;
	case OSMESA_BGR:
	case GL_BGR:
		dstformat = GL_BGR;
		destination_bpp = 3;
		break;
	case OSMESA_RGB_565:
		destination_bpp = 2;
#if NFOSMESA_NEED_INT_CONV
		/* TODO; without FULLMMU, we might get away with just setting GL_PACK_SWAP_BYTES */
		conversion = true;
#endif
		break;
	}

	if (bx_options.osmesa.channel_size > 8 && format != OSMESA_COLOR_INDEX && format != GL_NONE)
	{
		/* We are using libOSMesa[16,32] */
		srcformat = OSMESA_ARGB;
		conversion = true;
	}

	D(bug("nfosmesa: format:0x%x -> 0x%x, conversion: %s", srcformat, dstformat, conversion ? "true" : "false"));
	D(bug("nfosmesa: depth=%d, stencil=%d, accum=%d", depthBits, stencilBits, accumBits));

	if (GL_ISAVAILABLE(OSMesaCreateContextExt))
		ctx = OSMesaDriver::fn.OSMesaCreateContextExt(srcformat, depthBits, stencilBits, accumBits, share);
	else
		ctx = OSMesaDriver::fn.OSMesaCreateContext(srcformat, share);
	return ctx != NULL;
}

/*-----------------------------------------------------------------------*/

bool MesaContext::TestContext()
{
	GLubyte pixels[256];
	
	if (!GL_ISAVAILABLE(OSMesaGetProcAddress) ||
		!GL_ISAVAILABLE(OSMesaMakeCurrent) ||
		(!GL_ISAVAILABLE(OSMesaCreateContext) && !GL_ISAVAILABLE(OSMesaCreateContextExt)) ||
		!GL_ISAVAILABLE(OSMesaDestroyContext))
	{
		D(bug("missing OSMesa functions"));
		return false;
	}
	
	if (!CreateContext(GL_RGB, 0, 0, 0, 0))
	{
		D(bug("can not create context"));
		return false;
	}

	if (!OSMesaDriver::fn.OSMesaMakeCurrent(ctx, pixels, GL_UNSIGNED_BYTE, 1, 1))
		return false;
	
	const GLubyte *extensions = OSMesaDriver::fn.glGetString(GL_EXTENSIONS);
	OffscreenContext::has_MESA_pack_invert = gl_HasExtension("GL_MESA_pack_invert", extensions);

#if DEBUG
	{
		const GLubyte *str;
		
		str = OSMesaDriver::fn.glGetString(GL_VERSION);
		D(bug("GL Version: %s", str));
		str = OSMesaDriver::fn.glGetString(GL_VENDOR);
		D(bug("GL Vendor: %s", str));
		str = OSMesaDriver::fn.glGetString(GL_RENDERER);
		D(bug("GL Renderer: %s", str));
		str = OSMesaDriver::fn.glGetString(GL_EXTENSIONS);
		D(bug("GL Extensions: %s", str));
		str = OSMesaDriver::fn.glGetString(GL_SHADING_LANGUAGE_VERSION);
		if (str != NULL)
			D(bug("GL Shading Language : %s", str));
	}
#endif

	OSMesaDriver::fn.OSMesaMakeCurrent(NULL, 0, 0, 0, 0);

	OSMesaDriver::fn.OSMesaDestroyContext(ctx);
	ctx = 0;

	return true;
}

/*-----------------------------------------------------------------------*/

GLboolean MesaContext::MakeCurrent(memptr _buffer, GLenum _type, GLsizei _width, GLsizei _height)
{
	dst_buffer = _buffer;
	type = _type;
	width = _width;
	height = _height;
	if (bx_options.osmesa.channel_size > 8)
	{
		ResizeBuffer(4 * (bx_options.osmesa.channel_size>>3));
		switch(bx_options.osmesa.channel_size)
		{
		case 16:
			type = GL_UNSIGNED_SHORT;
			break;
		case 32:
			type = GL_FLOAT;
			break;
		}
	} else
	{
		if (dstformat == OSMESA_RGB_565)
		{
			type = GL_UNSIGNED_SHORT_5_6_5;
#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_INT_CONV
			conversion = true;
#endif
			if (conversion)
				ResizeBuffer(2);
		} else
		{
			type = GL_UNSIGNED_BYTE;
#if NFOSMESA_NEED_BYTE_CONV
			if (dstformat != GL_NONE)
				conversion = true;
#endif
			if (conversion)
				ResizeBuffer(4);
		}
	}
	return MakeCurrent();
}

/*-----------------------------------------------------------------------*/

GLboolean MesaContext::MakeCurrent()
{
	void *draw_buffer;

	draw_buffer = Atari2HostAddr(dst_buffer);
	if (host_buffer)
		draw_buffer = host_buffer;
	return OSMesaDriver::fn.OSMesaMakeCurrent(ctx, draw_buffer, type, width, height);
}

/*-----------------------------------------------------------------------*/

GLboolean MesaContext::ClearCurrent()
{
	return OSMesaDriver::fn.OSMesaMakeCurrent(NULL, NULL, 0, 0, 0);
}

/*-----------------------------------------------------------------------*/

void MesaContext::Postprocess(const char *filter, GLuint enable_value)
{
	if (GL_ISAVAILABLE(OSMesaPostprocess))
		OSMesaDriver::fn.OSMesaPostprocess(ctx, filter, enable_value);
	else
		bug("nfosmesa: OSMesaPostprocess: no such function");
}

/*-----------------------------------------------------------------------*/

bool MesaContext::GetIntegerv(GLint pname, GLint *value)
{
	if (GL_ISAVAILABLE(OSMesaGetIntegerv))
		OSMesaDriver::fn.OSMesaGetIntegerv(pname, value);
	return true;
}

/*-----------------------------------------------------------------------*/

void MesaContext::PixelStore(GLint pname, GLint value)
{
	OffscreenContext::PixelStore(pname, value);
	if (GL_ISAVAILABLE(OSMesaPixelStore))
		OSMesaDriver::fn.OSMesaPixelStore(pname, value);
}

/*-----------------------------------------------------------------------*/

void MesaContext::ColorClamp(GLboolean enable)
{
	if (GL_ISAVAILABLE(OSMesaColorClamp))
		OSMesaDriver::fn.OSMesaColorClamp(enable);
	else
		bug("nfosmesa: OSMesaColorClamp: no such function");
}

/*************************************************************************/
/*-----------------------------------------------------------------------*/
/*************************************************************************/

/*--- Base Class for using OpenGL render buffers ---*/

OpenglContext::OpenglContext(void *glhandle) :
	OffscreenContext(glhandle),
	render_width(0),
	render_height(0),
	depthBits(0),
	stencilBits(0),
	accumBits(0),
	framebuffer(0),
	colorbuffer(0),
	colortex(0),
	useTeximage(false),
	texTarget(GL_TEXTURE_2D),
	depthbuffer(0),
	depthformat(GL_NONE),
	stencilbuffer(0),
	has_GL_EXT_packed_depth_stencil(GL_FALSE),
	has_GL_NV_packed_depth_stencil(GL_FALSE),
	has_GL_EXT_texture_object(GL_FALSE),
	has_GL_XX_texture_rectangle(GL_FALSE),
	has_GL_ARB_texture_non_power_of_two(GL_FALSE)
{
}

/*-----------------------------------------------------------------------*/

OpenglContext::~OpenglContext()
{
	DestroyContext();
}

/*-----------------------------------------------------------------------*/

void OpenglContext::DestroyContext()
{
	/*
	 * Delete resources
	 */
	if (colorbuffer)
	{
		OSMesaDriver::fn.glDeleteRenderbuffersEXT(1, &colorbuffer);
		D(OSMesaDriver::PrintErrors("glDeleteRenderbuffersEXT(colorbuffer)"));
		colorbuffer = 0;
	}
	if (colortex)
	{
		OSMesaDriver::fn.glDeleteTexturesEXT(1, &colortex);
		D(OSMesaDriver::PrintErrors("glDeleteTexturesEXT(colortex)"));
		colortex = 0;
	}
	if (stencilbuffer && stencilbuffer != depthbuffer)
	{
		OSMesaDriver::fn.glDeleteRenderbuffersEXT(1, &stencilbuffer);
		D(OSMesaDriver::PrintErrors("glDeleteRenderbuffersEXT(stencilbuffer)"));
	}
	stencilbuffer = 0;
	if (depthbuffer)
	{
		OSMesaDriver::fn.glDeleteRenderbuffersEXT(1, &depthbuffer);
		D(OSMesaDriver::PrintErrors("glDeleteRenderbuffersEXT(depthbuffer)"));
		depthbuffer = 0;
	}
	/*
	 * Bind 0, which means render to back buffer, as a result, fb is unbound
	 */
	if (framebuffer)
	{
		OSMesaDriver::fn.glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
		OSMesaDriver::fn.glDeleteFramebuffersEXT(1, &framebuffer);
		D(OSMesaDriver::PrintErrors("glDeleteFramebuffersEXT(framebuffer)"));
		framebuffer = 0;
	}
}

/*-----------------------------------------------------------------------*/

bool OpenglContext::CreateContext(GLenum format, GLint _depthBits, GLint _stencilBits, GLint _accumBits, OffscreenContext * /* sharelist */)
{
	const GLubyte *extensions;
	
	extensions = OSMesaDriver::fn.glGetString(GL_EXTENSIONS);
	
	if (!gl_HasExtension("GL_EXT_framebuffer_object", extensions) ||
		!GL_ISAVAILABLE(glGenFramebuffersEXT) ||
		!GL_ISAVAILABLE(glBindFramebufferEXT) ||
		!GL_ISAVAILABLE(glGenRenderbuffersEXT) ||
		!GL_ISAVAILABLE(glBindRenderbufferEXT) ||
		!GL_ISAVAILABLE(glDeleteRenderbuffersEXT) ||
		!GL_ISAVAILABLE(glDeleteFramebuffersEXT) ||
		!GL_ISAVAILABLE(glFramebufferRenderbufferEXT) ||
		!GL_ISAVAILABLE(glRenderbufferStorageEXT) ||
		!GL_ISAVAILABLE(glCheckFramebufferStatusEXT))
	{
		D(bug("no GL_EXT_framebuffer_object extension"));
		return false;
	}

	has_GL_EXT_packed_depth_stencil = gl_HasExtension("GL_EXT_packed_depth_stencil", extensions);
	has_GL_NV_packed_depth_stencil = gl_HasExtension("GL_NV_packed_depth_stencil", extensions);
	has_GL_EXT_texture_object = gl_HasExtension("GL_EXT_texture_object", extensions);
	has_GL_XX_texture_rectangle = gl_HasExtension("GL_ARB_texture_rectangle", extensions) ||
		gl_HasExtension("GL_EXT_texture_rectangle", extensions) ||
		gl_HasExtension("GL_NV_texture_rectangle", extensions);
	has_GL_ARB_texture_non_power_of_two = gl_HasExtension("GL_ARB_texture_non_power_of_two", extensions);
	
	dstformat = format;
	depthBits = _depthBits;
	stencilBits = _stencilBits;
	accumBits = _accumBits;
	
	/* Select format */
	srcformat = format;
	conversion = false;
	swapcomponents = false;
	destination_bpp = 0;
	
	switch (dstformat)
	{
	case OSMESA_COLOR_INDEX:
	/* case GL_COLOR_INDEX: */
		dstformat = GL_COLOR_INDEX;
		destination_bpp = 1;
		break;
	case OSMESA_RGBA:
	/* case GL_RGBA: */
		dstformat = GL_RGBA;
		destination_bpp = 4;
		break;
	case OSMESA_BGRA:
	case GL_BGRA_EXT:
		dstformat = GL_BGRA_EXT;
		destination_bpp = 4;
		break;
	case OSMESA_ARGB:
	case VDI_ARGB:
	case DIRECT_VDI_ARGB:
		destination_bpp = 4;
		/* GL has no GL_ARGB format */
		dstformat = GL_BGRA_EXT;
		swapcomponents = true;
		break;
	case OSMESA_RGB:
	case VDI_RGB:
	/* case GL_RGB: */
		dstformat = GL_RGB;
		destination_bpp = 3;
		break;
	case OSMESA_BGR:
	case GL_BGR:
		dstformat = GL_BGR;
		destination_bpp = 3;
		break;
	case OSMESA_RGB_565:
		destination_bpp = 2;
#if NFOSMESA_NEED_INT_CONV
		/* TODO; without FULLMMU, we might get away with just setting GL_PACK_SWAP_BYTES */
		conversion = true;
#endif
		break;
	}

#if NFOSMESA_NEED_BYTE_CONV
	conversion = true;
#endif
	
	/*
	 * the FBO can't be created until we know the dimensions,
	 * which we receive through OSMesaMakeCurrent()
	 */
	
	return true;
}

/*-----------------------------------------------------------------------*/

void OpenglContext::setYup(GLboolean enable)
{
	OffscreenContext::setYup(enable);
	/*
	 * glReadPixels() always returns pixel data upside down,
	 * and doing glReadPixels() a single row at a time to correct that
	 * is much slower than doing a single read into a temporary buffer
	 * and doing the conversion ourselves
	 */
	if (!enable && !OffscreenContext::has_MESA_pack_invert)
	{
		conversion = true;
	}
	createBuffers();
}

/*-----------------------------------------------------------------------*/

GLboolean OpenglContext::createBuffers()
{
	GLenum status;
	bool recreate = false;
	
	useTeximage = false;
	texTarget = GL_TEXTURE_2D;
	depthformat = GL_NONE;

	/*
	 * create a framebuffer object
	 */
	if (framebuffer == 0)
	{
		OSMesaDriver::fn.glGenFramebuffersEXT(1, &framebuffer);
		D(OSMesaDriver::PrintErrors("glGenFramebufferEXT"));
		if (framebuffer == 0)
			return GL_FALSE;
	}
	OSMesaDriver::fn.glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, framebuffer);
	D(OSMesaDriver::PrintErrors("glBindFramebufferEXT(framebuffer)"));

	/*
	 * if storage has already been assigned,
	 * have to recreate render buffers
	 */
	if (width != render_width || height != render_height)
	{
		if (colorbuffer)
		{
			OSMesaDriver::fn.glDeleteRenderbuffersEXT(1, &colorbuffer);
			D(OSMesaDriver::PrintErrors("glDeleteRenderbuffersEXT(colorbuffer)"));
			colorbuffer = 0;
		}
		if (colortex)
		{
			OSMesaDriver::fn.glDeleteTexturesEXT(1, &colortex);
			D(OSMesaDriver::PrintErrors("glDeleteTexturesEXT(colortex)"));
			colortex = 0;
		}
		if (stencilbuffer && stencilbuffer != depthbuffer)
		{
			OSMesaDriver::fn.glDeleteRenderbuffersEXT(1, &stencilbuffer);
			D(OSMesaDriver::PrintErrors("glDeleteRenderbuffersEXT(stencilbuffer)"));
		}
		stencilbuffer = 0;
		if (depthbuffer)
		{
			OSMesaDriver::fn.glDeleteRenderbuffersEXT(1, &depthbuffer);
			D(OSMesaDriver::PrintErrors("glDeleteRenderbuffersEXT(depthbuffer)"));
			depthbuffer = 0;
		}
		render_width = width;
		render_height = height;
		recreate = true;
	}
	
	/*
	 * Create a color buffer
	 */
	if (dstformat != GL_NONE)
	{
		if (colorbuffer == 0)
		{
			OSMesaDriver::fn.glGenRenderbuffersEXT(1, &colorbuffer);
			D(OSMesaDriver::PrintErrors("glGenRenderbuffersEXT(colorbuffer)"));
			if (colorbuffer == 0)
				return GL_FALSE;
		}
		if (colortex == 0 && has_GL_EXT_texture_object)
		{
			OSMesaDriver::fn.glGenTexturesEXT(1, &colortex);
			D(OSMesaDriver::PrintErrors("glGenTexturesEXT(colortex)"));
			if (colortex == 0)
				return GL_FALSE;
		}
	}

	/*
	 * Attach color buffer to FBO
	 */
	GLint tex_width = render_width;
	GLint tex_height = render_height;
	if (dstformat != GL_NONE && recreate)
	{
		if (false && colortex)
		{
			useTeximage = true;
			if (has_GL_ARB_texture_non_power_of_two)
			{
				texTarget = GL_TEXTURE_2D;
			} else if (has_GL_XX_texture_rectangle)
			{
				texTarget = GL_TEXTURE_RECTANGLE_ARB;
			} else
			{
				GLint tex_dim = MAX(tex_width, tex_height);
				texTarget = GL_TEXTURE_2D;
				tex_width = 64;
				while (tex_width < tex_dim)
					tex_width <<= 1;
				tex_height = tex_width;
			}
			OSMesaDriver::fn.glBindTexture(texTarget, colortex);
			D(OSMesaDriver::PrintErrors("glBindTexture(colortex)"));
			OSMesaDriver::fn.glTexParameteri(texTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			OSMesaDriver::fn.glTexParameteri(texTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			OSMesaDriver::fn.glTexParameteri(texTarget, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			OSMesaDriver::fn.glTexParameteri(texTarget, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			OSMesaDriver::fn.glTexImage2D(texTarget, 0, GL_RGBA8, tex_width, tex_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
			D(OSMesaDriver::PrintErrors("glTexImage2D"));
			OSMesaDriver::fn.glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, texTarget, colortex, 0);
			D(OSMesaDriver::PrintErrors("glFramebufferTexture2DEXT(colortex)"));
		} else
		{
			OSMesaDriver::fn.glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, colorbuffer);
			D(OSMesaDriver::PrintErrors("glBindRenderbufferEXT(colorbuffer)"));
			OSMesaDriver::fn.glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_RGBA8, tex_width, tex_height);
			D(OSMesaDriver::PrintErrors("glRenderbufferStorageEXT(colorbuffer)"));
			OSMesaDriver::fn.glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_RENDERBUFFER_EXT, colorbuffer);
			D(OSMesaDriver::PrintErrors("glFramebufferRenderbufferEXT(colorbuffer)"));
		}
	}
	
	/*
	 * Create a depth/stencil buffer
	 */
	if (depthBits > 0)
	{
		if (depthbuffer == 0)
		{
			OSMesaDriver::fn.glGenRenderbuffersEXT(1, &depthbuffer);
			D(OSMesaDriver::PrintErrors("glGenRenderbuffersEXT(depthbuffer)"));
			if (depthbuffer == 0)
				return GL_FALSE;
		}
		if (stencilBits > 0 && has_GL_EXT_packed_depth_stencil)
		{
			if (stencilbuffer == 0)
				stencilbuffer = depthbuffer;
			depthformat = GL_DEPTH24_STENCIL8_EXT;
		} else if (stencilBits > 0 && has_GL_NV_packed_depth_stencil)
		{
			if (stencilbuffer == 0)
				stencilbuffer = depthbuffer;
			depthformat = GL_UNSIGNED_INT_24_8_NV;
		} else
		{
			depthformat = GL_DEPTH_COMPONENT;
		}
	}
	if (stencilBits > 0)
	{
		if (stencilbuffer == 0)
		{
			OSMesaDriver::fn.glGenRenderbuffersEXT(1, &stencilbuffer);
			D(OSMesaDriver::PrintErrors("glGenRenderbuffersEXT(stencilbuffer)"));
			if (stencilbuffer == 0)
				return GL_FALSE;
		}
	}

	/*
	 * Attach depth & stencil buffer to FBO
	 */
	if (recreate)
	{
		if (depthbuffer)
		{
			OSMesaDriver::fn.glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, depthbuffer);
			D(OSMesaDriver::PrintErrors("glBindRenderbufferEXT(depthbuffer)"));
			OSMesaDriver::fn.glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, depthformat, tex_width, tex_height);
			D(OSMesaDriver::PrintErrors("glRenderbufferStorageEXT(depthbuffer)"));
			OSMesaDriver::fn.glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, depthbuffer);
			D(OSMesaDriver::PrintErrors("glFramebufferRenderbufferEXT(depthbuffer)"));
		}
		if (stencilbuffer)
		{
			OSMesaDriver::fn.glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, stencilbuffer);
			D(OSMesaDriver::PrintErrors("glBindRenderbufferEXT(stencilbuffer)"));
			if (stencilbuffer != depthbuffer)
			{
				OSMesaDriver::fn.glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_STENCIL_INDEX, tex_width, tex_height);
				D(OSMesaDriver::PrintErrors("glRenderbufferStorageEXT(stencilbuffer)"));
			}
			OSMesaDriver::fn.glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, stencilbuffer);
			D(OSMesaDriver::PrintErrors("glFramebufferRenderbufferEXT(stencilbuffer)"));
		}
		OSMesaDriver::fn.glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);
		D(OSMesaDriver::PrintErrors("glBindRenderbufferEXT(0)"));
	}
	
	/* FIXME: accum buffers are not directly supported */
	
	if (recreate)
	{
		status = OSMesaDriver::fn.glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
		D(OSMesaDriver::PrintErrors("glCheckFramebufferStatusEXT"));
		switch (status)
		{
		case GL_FRAMEBUFFER_COMPLETE_EXT:
			D(bug("nfosmesa: framebuffer complete"));
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT:
			D(bug("nfosmesa: incomplete attachment"));
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT:
			D(bug("nfosmesa: incomplete missing attachment"));
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
			D(bug("nfosmesa: incomplete dimensions (%dx%d, depth %d, stencil %d)", render_width, render_height, depthBits, stencilBits));
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT:
			D(bug("nfosmesa: incomplete formats"));
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT:
			D(bug("nfosmesa: incomplete draw buffer"));
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT:
			D(bug("nfosmesa: incomplete read buffer"));
			break;
		case GL_FRAMEBUFFER_UNSUPPORTED_EXT:
			D(bug("nfosmesa: unsupported format"));
			break;
		}
		if (status != GL_FRAMEBUFFER_COMPLETE_EXT)
			return GL_FALSE;
	}
		
	if (conversion)
	{
		if (swapcomponents && dstformat == GL_BGRA_EXT)
		{
			/*
			 * set it back to what was actually requested
			 */
			dstformat = OSMESA_ARGB;
			swapcomponents = false;
		}
		ResizeBuffer(4);
	}

	return GL_TRUE;
}
	
/*-----------------------------------------------------------------------*/

GLboolean OpenglContext::MakeBufferCurrent(bool create_buffer)
{
	if (create_buffer)
	{
		if (!createBuffers())
			return GL_FALSE;
	}
	
	OSMesaDriver::fn.glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, framebuffer);
	D(OSMesaDriver::PrintErrors("glBindFramebufferEXT"));
	OSMesaDriver::fn.glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, colorbuffer);
	D(OSMesaDriver::PrintErrors("glBindRenderbufferEXT(colorbuffer)"));
	return GL_TRUE;
}

/*-----------------------------------------------------------------------*/

GLboolean OpenglContext::ClearCurrent()
{
	OSMesaDriver::fn.glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);
	D(OSMesaDriver::PrintErrors("glBindRenderbufferEXT(0)"));
	OSMesaDriver::fn.glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	D(OSMesaDriver::PrintErrors("glBindFramebufferEXT(0)"));
	return GL_TRUE;
}

/*-----------------------------------------------------------------------*/

bool OpenglContext::GetIntegerv(GLint pname, GLint *value)
{
	GLuint buffer;
	
	switch (pname)
	{
	case OSMESA_WIDTH:
		buffer = colorbuffer;
		if (buffer == 0)
			buffer = depthbuffer;
		OSMesaDriver::fn.glGetRenderbufferParameterivEXT(GL_RENDERBUFFER_EXT, GL_RENDERBUFFER_WIDTH, value);
		break;
	case OSMESA_HEIGHT:
		buffer = colorbuffer;
		if (buffer == 0)
			buffer = depthbuffer;
		OSMesaDriver::fn.glGetRenderbufferParameterivEXT(GL_RENDERBUFFER_EXT, GL_RENDERBUFFER_HEIGHT, value);
		break;
	case OSMESA_FORMAT:
		if (swapcomponents && dstformat == GL_BGRA_EXT)
			*value = OSMESA_ARGB;
		else
			*value = dstformat;
		break;
	case OSMESA_TYPE:
		*value = type;
		break;
	case OSMESA_ROW_LENGTH:
		*value = userRowLength;
		break;
	case OSMESA_Y_UP:
		*value = getYup();
		break;
	case OSMESA_MAX_WIDTH:
	case OSMESA_MAX_HEIGHT:
		OSMesaDriver::fn.glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE, value);
		break;
	default:
		return false;
	}
	return true;
}

/*-----------------------------------------------------------------------*/

void OpenglContext::ConvertContext()
{
	GLboolean texEnabled = GL_FALSE;
	PixelPackAttributes pack;
	
	if (useTeximage)
	{
		texEnabled = OSMesaDriver::fn.glIsEnabled(texTarget);
		if (texEnabled)
			OSMesaDriver::fn.glDisable(texTarget);
	}
	
#if DEBUG
	{
		static int beenhere;
		if (beenhere++ < 10)
		{
			D(bug("OpenglContext::ConvertContext: hostbuffer=%p yup=%d dstformat=%x conversion=%d width=%d height=%d", host_buffer, getYup(), dstformat, conversion, render_width, render_height));
		}
	}
#endif
	
	if (host_buffer)
	{
		if (dstformat == OSMESA_RGB_565)
		{
			/* the conversion routines want OSMESA_RGB565 */
			GLenum dsttype = GL_UNSIGNED_SHORT_5_6_5;
			OSMesaDriver::fn.glReadPixels(0, 0, width, height, GL_RGB, dsttype, host_buffer);
		} else
		{
			GLenum dsttype = SDL_BYTEORDER == SDL_LIL_ENDIAN ? GL_UNSIGNED_INT_8_8_8_8 : GL_UNSIGNED_INT_8_8_8_8_REV;
			/* the conversion routines want GL_ARGB */
			OSMesaDriver::fn.glReadPixels(0, 0, width, height, GL_BGRA_EXT, dsttype, host_buffer);
		}
		D(OSMesaDriver::PrintErrors("glReadPixels"));
		OffscreenContext::ConvertContext();
	} else
	{
		OSMesaDriver::fn.glPixelStorei(GL_PACK_ROW_LENGTH, getUserRowLength());
		D(OSMesaDriver::PrintErrors("glPixelStorei(GL_PACK_ROW_LENGTH)"));
		if (swapcomponents)
		{
			GLenum dsttype = SDL_BYTEORDER == SDL_LIL_ENDIAN ? GL_UNSIGNED_INT_8_8_8_8 : GL_UNSIGNED_INT_8_8_8_8_REV;
			if (getYup())
			{
				OSMesaDriver::fn.glReadPixels(0, 0, width, height, dstformat, dsttype, Atari2HostAddr(dst_buffer));
			} else if (OffscreenContext::has_MESA_pack_invert)
			{
				OSMesaDriver::fn.glPixelStorei(GL_PACK_INVERT_MESA, GL_TRUE);
				OSMesaDriver::fn.glReadPixels(0, 0, width, height, dstformat, dsttype, Atari2HostAddr(dst_buffer));
			} else
			{
				/* just in case; we should not be here */
				char *ptr = (char *)Atari2HostAddr(dst_buffer);
				GLint pitch = getUserRowLength() * destination_bpp;
				for (GLsizei y = height; --y >= 0; )
				{
					OSMesaDriver::fn.glReadPixels(0, y, width, 1, dstformat, dsttype, ptr);
					ptr += pitch;
				}
			}
		} else if (dstformat == OSMESA_RGB_565)
		{
			GLenum dsttype = SDL_BYTEORDER == SDL_LIL_ENDIAN ? GL_UNSIGNED_SHORT_5_6_5 : GL_UNSIGNED_SHORT_5_6_5_REV;
			if (getYup())
			{
				OSMesaDriver::fn.glReadPixels(0, 0, width, height, GL_RGB, dsttype, Atari2HostAddr(dst_buffer));
			} else if (OffscreenContext::has_MESA_pack_invert)
			{
				OSMesaDriver::fn.glPixelStorei(GL_PACK_INVERT_MESA, GL_TRUE);
				OSMesaDriver::fn.glReadPixels(0, 0, width, height, GL_RGB, dsttype, Atari2HostAddr(dst_buffer));
			} else
			{
				/* just in case; we should not be here */
				char *ptr = (char *)Atari2HostAddr(dst_buffer);
				GLint pitch = getUserRowLength() * destination_bpp;
				for (GLsizei y = height; --y >= 0; )
				{
					OSMesaDriver::fn.glReadPixels(0, y, width, 1, GL_RGB, dsttype, ptr);
					ptr += pitch;
				}
			}
		} else
		{
			if (getYup())
			{
				OSMesaDriver::fn.glReadPixels(0, 0, width, height, dstformat, type, Atari2HostAddr(dst_buffer));
			} else if (OffscreenContext::has_MESA_pack_invert)
			{
				OSMesaDriver::fn.glPixelStorei(GL_PACK_INVERT_MESA, GL_TRUE);
				OSMesaDriver::fn.glReadPixels(0, 0, width, height, dstformat, type, Atari2HostAddr(dst_buffer));
			} else
			{
				/* just in case; we should not be here */
				char *ptr = (char *)Atari2HostAddr(dst_buffer);
				GLint pitch = getUserRowLength() * destination_bpp;
				for (GLsizei y = height; --y >= 0; )
				{
					OSMesaDriver::fn.glReadPixels(0, y, width, 1, dstformat, type, ptr);
					ptr += pitch;
				}
			}
		}
		D(OSMesaDriver::PrintErrors("glReadPixels"));
	}
	if (texEnabled)
		OSMesaDriver::fn.glEnable(texTarget);
}

/*-----------------------------------------------------------------------*/

void OpenglContext::ColorClamp(GLboolean enable)
{
	GLenum clamp = enable ? GL_TRUE : GL_FIXED_ONLY;
	if (GL_ISAVAILABLE(glClampColor))
		OSMesaDriver::fn.glClampColor(GL_CLAMP_FRAGMENT_COLOR, clamp);
	else if (GL_ISAVAILABLE(glClampColorARB))
		OSMesaDriver::fn.glClampColorARB(GL_CLAMP_FRAGMENT_COLOR, clamp);
	else
		bug("nfosmesa: OSMesaColorClamp: no such function");
}

/*************************************************************************/
/*-----------------------------------------------------------------------*/
/*************************************************************************/

/* X11 */

#if defined(SDL_VIDEO_DRIVER_X11) && defined(HAVE_X11_XLIB_H)

Display *X11OpenglContext::dpy;
Window X11OpenglContext::sdlwindow;
int X11OpenglContext::screen;

XVisualInfo* (*X11OpenglContext::pglXChooseVisual)(Display *dpy, int, int *);
GLXContext (*X11OpenglContext::pglXCreateContext)(Display *, XVisualInfo *, GLXContext, int);
int (*X11OpenglContext::pglXMakeCurrent)(Display *, GLXDrawable, GLXContext);
GLXContext (*X11OpenglContext::pglXGetCurrentContext)(void);
void (*X11OpenglContext::pglXDestroyContext)(Display *, GLXContext);
const char *(*X11OpenglContext::pglXQueryServerString)(Display *, int, int);
const char *(*X11OpenglContext::pglXGetClientString)(Display *, int);
const char *(*X11OpenglContext::pglXQueryExtensionsString)(Display *, int);
GLXContext (*X11OpenglContext::pglXCreateContextAttribsARB) (Display *dpy, GLXFBConfig config, GLXContext share_context, Bool direct, const int *attrib_list);
GLXFBConfig * (*X11OpenglContext::pglXChooseFBConfig) (Display *dpy, int screen, const int *attrib_list, int *nelements);
Bool (*X11OpenglContext::pglXMakeContextCurrent) (Display *dpy, GLXDrawable draw, GLXDrawable read, GLXContext ctx);
GLXPbuffer (*X11OpenglContext::pglXCreatePbuffer) (Display *dpy, GLXFBConfig config, const int *attrib_list);
void (*X11OpenglContext::pglXDestroyPbuffer) (Display *dpy, GLXPbuffer pbuf);
int (*X11OpenglContext::pXFree)(void *);

/*-----------------------------------------------------------------------*/

X11OpenglContext::X11OpenglContext(void *glhandle) :
	OpenglContext(glhandle),
	ctx(NULL)
{
	InitPointers(lib_handle);
}

/*-----------------------------------------------------------------------*/

X11OpenglContext::~X11OpenglContext()
{
	dispose();
}

/*-----------------------------------------------------------------------*/

void X11OpenglContext::InitPointers(void *lib_handle)
{
	SDL_SysWMinfo info;
	
	SDL_VERSION(&info.version);
	if (
#if SDL_VERSION_ATLEAST(2, 0, 0)
		(host && host->video && SDL_GetWindowWMInfo(host->video->Window(), &info) > 0)
#else
		SDL_GetWMInfo(&info) > 0
#endif
		&& info.subsystem == SDL_SYSWM_X11)
	{
		dpy = info.info.x11.display;
		sdlwindow = info.info.x11.window;
		screen = DefaultScreen(dpy);
		pglXChooseVisual = (XVisualInfo *(*)(Display *, int, int *))SDL_LoadFunction(lib_handle, "glXChooseVisual");
		pglXCreateContext = (GLXContext (*)(Display *, XVisualInfo *, GLXContext, int))SDL_LoadFunction(lib_handle, "glXCreateContext");
		pglXMakeCurrent = (int (*)(Display *, GLXDrawable, GLXContext))SDL_LoadFunction(lib_handle, "glXMakeCurrent");
		pglXGetCurrentContext = (GLXContext (*)(void))SDL_LoadFunction(lib_handle, "glXGetCurrentContext");
		pglXDestroyContext = (void (*)(Display *, GLXContext)) SDL_LoadFunction(lib_handle, "glXDestroyContext");
		pglXQueryServerString = (const char *(*)(Display *, int, int))SDL_LoadFunction(lib_handle, "glXQueryServerString");
		pglXGetClientString = (const char *(*)(Display *, int))SDL_LoadFunction(lib_handle, "glXGetClientString");
		pglXQueryExtensionsString = (const char *(*)(Display *, int))SDL_LoadFunction(lib_handle, "glXQueryExtensionsString");
		pglXCreateContextAttribsARB = (GLXContext (*) (Display *, GLXFBConfig, GLXContext, Bool, const int *)) SDL_LoadFunction(lib_handle, "glXCreateContextAttribsARB");
		pglXChooseFBConfig = (GLXFBConfig *(*) (Display *, int, const int *, int *)) SDL_LoadFunction(lib_handle, "glXChooseFBConfig");
		pglXMakeContextCurrent = (Bool (*) (Display *, GLXDrawable draw, GLXDrawable read, GLXContext ctx)) SDL_LoadFunction(lib_handle, "glXMakeContextCurrent");
		pglXCreatePbuffer = (GLXPbuffer (*) (Display *, GLXFBConfig, const int *)) SDL_LoadFunction(lib_handle, "glXCreatePbuffer");
		pglXDestroyPbuffer = (void (*) (Display *, GLXPbuffer)) SDL_LoadFunction(lib_handle, "glXDestroyPbuffer");
	
		/*
		 * look up dynamically too, because SDL loads X11 also dynamically,
		 * and the appplication is not neccessarily linked to X11
		 */
		pXFree = (int (*)(void *))SDL_LoadFunction(lib_handle, "XFree");
	}
}	

/*-----------------------------------------------------------------------*/

void X11OpenglContext::dispose()
{
	OpenglContext::DestroyContext();
	if (ctx)
	{
		pglXDestroyContext(dpy, ctx);
		ctx = 0;
	}
	if (pbuffer)
	{
		pglXDestroyPbuffer(dpy, pbuffer);
		pbuffer = None;
	}
}

/*-----------------------------------------------------------------------*/

bool X11OpenglContext::CreateContext(GLenum format, GLint _depthBits, GLint _stencilBits, GLint _accumBits, OffscreenContext *share_ctx)
{
	int i;
	X11OpenglContext *opengl_share = (X11OpenglContext *)share_ctx;
	GLXFBConfig *fbConfigs;
	GLXFBConfig config;
	int num_configs = 0;
	int attributeList[40];
	int context_attribs[] = { None };
	int pbuffer_attribs[] = { GLX_PBUFFER_WIDTH, 32, GLX_PBUFFER_HEIGHT, 32, None };
	
	i = 0;
	attributeList[i++] = GLX_DRAWABLE_TYPE;
	attributeList[i++] = GLX_PBUFFER_BIT;
	attributeList[i++] = GLX_DOUBLEBUFFER;
	attributeList[i++] = False;
	if (format == GL_COLOR_INDEX)
	{
		attributeList[i++] = GLX_BUFFER_SIZE;
		attributeList[i++] = 8;
		attributeList[i++] = GLX_RENDER_TYPE;
		attributeList[i++] = GLX_COLOR_INDEX_BIT;
	} else
	{
		attributeList[i++] = GLX_RENDER_TYPE;
		attributeList[i++] = GLX_RGBA_BIT;
		attributeList[i++] = GLX_RED_SIZE;
		attributeList[i++] = 8;
		attributeList[i++] = GLX_GREEN_SIZE;
		attributeList[i++] = 8;
		attributeList[i++] = GLX_BLUE_SIZE;
		attributeList[i++] = 8;
		if (FormatHasAlpha(format))
		{
			attributeList[i++] = GLX_ALPHA_SIZE;
			attributeList[i++] = 8;
		}
	}
	if (_depthBits > 0)
	{
		attributeList[i++] = GLX_DEPTH_SIZE;
		attributeList[i++] = _depthBits;
	}
	if (_stencilBits > 0)
	{
		attributeList[i++] = GLX_STENCIL_SIZE;
		attributeList[i++] = _stencilBits;
	}
	if (_accumBits > 0)
	{
		attributeList[i++] = GLX_ACCUM_RED_SIZE;
		attributeList[i++] = 8;
		attributeList[i++] = GLX_ACCUM_GREEN_SIZE;
		attributeList[i++] = 8;
		attributeList[i++] = GLX_ACCUM_BLUE_SIZE;
		attributeList[i++] = 8;
		if (FormatHasAlpha(format))
		{
			attributeList[i++] = GLX_ACCUM_ALPHA_SIZE;
			attributeList[i++] = 8;
		}
	}
	attributeList[i] = None;

	fbConfigs = pglXChooseFBConfig(dpy, screen, attributeList, &num_configs);
	if (fbConfigs == NULL || num_configs == 0)
	{
		D(bug("No suitable visual"));
		pXFree(fbConfigs);
		return false;
	}
	config = fbConfigs[0];
	ctx = pglXCreateContextAttribsARB(dpy, config, opengl_share ? opengl_share->ctx : NULL, True, context_attribs);
	pbuffer = pglXCreatePbuffer(dpy, config, pbuffer_attribs);
	pXFree(fbConfigs);
	if (!ctx || !pbuffer)
		return false;
	if (!pglXMakeContextCurrent(dpy, pbuffer, pbuffer, ctx))
		return false;
	if (!OpenglContext::CreateContext(format, _depthBits, _stencilBits, _accumBits, share_ctx))
		return false;
	return true;
}

/*-----------------------------------------------------------------------*/

bool X11OpenglContext::TestContext()
{
	if (!dpy)
	{
		D(bug("TestContext:: not running on X11"));
		return false;
	}
	if (pglXChooseVisual == 0 ||
		pglXCreateContext == 0 ||
		pglXMakeCurrent == 0 ||
		pglXDestroyContext == 0 ||
		pglXCreateContextAttribsARB == 0 ||
		pglXChooseFBConfig == 0 ||
		pglXCreatePbuffer == 0 ||
		pglXMakeContextCurrent == 0 ||
		pglXDestroyPbuffer == 0 ||
		pXFree == 0)
	{
		D(bug("missing glX functions"));
		return false;
	}
	/*
	 * we need a context to get usable values from glGetString()
	 */
	if (!CreateContext(GL_RGB, 0, 0, 0, NULL))
	{
		D(bug("can not create context"));
		dispose();
		return false;
	}

	const GLubyte *extensions = OSMesaDriver::fn.glGetString(GL_EXTENSIONS);
	OffscreenContext::has_MESA_pack_invert = gl_HasExtension("GL_MESA_pack_invert", extensions);
	
#if DEBUG
	{
		const GLubyte *str;
		
		str = OSMesaDriver::fn.glGetString(GL_VERSION);
		D(bug("GL Version: %s", str));
		str = OSMesaDriver::fn.glGetString(GL_VENDOR);
		D(bug("GL Vendor: %s", str));
		str = OSMesaDriver::fn.glGetString(GL_RENDERER);
		D(bug("GL Renderer: %s", str));
		str = OSMesaDriver::fn.glGetString(GL_EXTENSIONS);
		D(bug("GL Extensions: %s", str));
		str = OSMesaDriver::fn.glGetString(GL_SHADING_LANGUAGE_VERSION);
		if (str != NULL)
			D(bug("GL Shading Language : %s", str));
		if (pglXQueryServerString)
		{
			str = (const GLubyte *)pglXQueryServerString(dpy, screen, GLX_VENDOR);
			D(bug("GL Server Vendor : %s", str));
			str = (const GLubyte *)pglXQueryServerString(dpy, screen, GLX_VERSION);
			D(bug("GL Server Version : %s", str));
			str = (const GLubyte *)pglXQueryServerString(dpy, screen, GLX_EXTENSIONS);
			D(bug("GL Server Extensions : %s", str));
		}
		if (pglXQueryExtensionsString)
		{
			str = (const GLubyte *)pglXQueryExtensionsString(dpy, screen);
			D(bug("GL Server XExtensions : %s", str));
		}
		if (pglXGetClientString)
		{
			str = (const GLubyte *)pglXGetClientString(dpy, GLX_VENDOR);
			D(bug("GL Client Vendor : %s", str));
			str = (const GLubyte *)pglXGetClientString(dpy, GLX_VERSION);
			D(bug("GL Client Version : %s", str));
			str = (const GLubyte *)pglXGetClientString(dpy, GLX_EXTENSIONS);
			D(bug("GL Client Extensions : %s", str));
		}
	}
#endif

	dispose();
	return true;
}

/*-----------------------------------------------------------------------*/

GLboolean X11OpenglContext::MakeCurrent(memptr _buffer, GLenum _type, GLsizei _width, GLsizei _height)
{
	dst_buffer = _buffer;
	type = _type;
	width = _width;
	height = _height;
	if (!pglXMakeContextCurrent(dpy, pbuffer, pbuffer, ctx))
		return GL_FALSE;
	return OpenglContext::MakeBufferCurrent(true);
}

/*-----------------------------------------------------------------------*/

GLboolean X11OpenglContext::MakeCurrent()
{
	if (!pglXMakeContextCurrent(dpy, pbuffer, pbuffer, ctx))
		return GL_FALSE;
	return OpenglContext::MakeBufferCurrent(false);
}

/*-----------------------------------------------------------------------*/

GLboolean X11OpenglContext::ClearCurrent()
{
	OpenglContext::ClearCurrent();
	if (!pglXMakeContextCurrent(dpy, None, None, NULL))
		return GL_FALSE;
	return GL_TRUE;
}

/*-----------------------------------------------------------------------*/

#if !defined(SDL_VIDEO_DRIVER_QUARTZ) && !defined(SDL_VIDEO_DRIVER_COCOA)

#if !SDL_VERSION_ATLEAST(2, 0, 0)

SDL_GLContext X11OpenglContext::GetSDLContext(void)
{
	if (!dpy || !pglXGetCurrentContext)
		return NULL;
	return pglXGetCurrentContext();
}

SDL_GLContext SDL_GL_GetCurrentContext(void)
{
	return X11OpenglContext::GetSDLContext();
}
#endif

/*-----------------------------------------------------------------------*/

void X11OpenglContext::SetSDLContext(SDL_GLContext ctx)
{
	if (!dpy || !pglXMakeCurrent)
		return;
	pglXMakeCurrent(dpy, sdlwindow, (GLXContext)ctx);
}

void SDL_GL_SetCurrentContext(SDL_GLContext ctx)
{
	X11OpenglContext::SetSDLContext(ctx);
}

#endif

#endif /* SDL_VIDEO_DRIVER_X11 */

/*************************************************************************/
/*-----------------------------------------------------------------------*/
/*************************************************************************/

/* Windows */

#if defined(SDL_VIDEO_DRIVER_WINDOWS)

bool Win32OpenglContext::windClassRegistered;
HWND Win32OpenglContext::tmpHwnd;
HWND Win32OpenglContext::sdlwindow;
HDC Win32OpenglContext::tmphdc;

int (WINAPI *Win32OpenglContext::pwglGetPixelFormat)(HDC);
int (WINAPI *Win32OpenglContext::pwglChoosePixelFormat) (HDC hDc, const PIXELFORMATDESCRIPTOR *pPfd);
WINBOOL (WINAPI *Win32OpenglContext::pwglSetPixelFormat) (HDC hDc, int ipfd, const PIXELFORMATDESCRIPTOR *ppfd);
WINBOOL (WINAPI *Win32OpenglContext::pwglSwapBuffers)(HDC);
int (WINAPI *Win32OpenglContext::pwglDescribePixelFormat)(HDC hdc, int iPixelFormat, UINT nBytes, PIXELFORMATDESCRIPTOR *ppfd);

WINBOOL (WINAPI *Win32OpenglContext::pwglCopyContext)(HGLRC, HGLRC, UINT);
HGLRC (WINAPI *Win32OpenglContext::pwglCreateContext)(HDC hdc);
HGLRC WINAPI (*Win32OpenglContext::pwglCreateLayerContext)(HDC, int);
WINBOOL (WINAPI *Win32OpenglContext::pwglDeleteContext)(HGLRC);
HGLRC (WINAPI *Win32OpenglContext::pwglGetCurrentContext)(void);
HDC (WINAPI *Win32OpenglContext::pwglGetCurrentDC)(void);
WINBOOL (WINAPI *Win32OpenglContext::pwglMakeCurrent)(HDC, HGLRC);
WINBOOL (WINAPI *Win32OpenglContext::pwglShareLists)(HGLRC, HGLRC);
const GLubyte * (WINAPI *Win32OpenglContext::pwglGetExtensionsStringARB) (HDC hdc);
const GLubyte * (WINAPI *Win32OpenglContext::pwglGetExtensionsStringEXT) (void);

LONG CALLBACK Win32OpenglContext::tmpWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

/*-----------------------------------------------------------------------*/

void Win32OpenglContext::InitPointers(void *glhandle)
{
	HMODULE gdi = GetModuleHandleA("gdi32.dll");
	
	pwglGetPixelFormat = (int (WINAPI *)(HDC)) GetProcAddress(gdi, "GetPixelFormat");
	pwglChoosePixelFormat = (int (WINAPI *) (HDC, const PIXELFORMATDESCRIPTOR *)) GetProcAddress(gdi, "ChoosePixelFormat");
	pwglSetPixelFormat = (WINBOOL (WINAPI *) (HDC, int, const PIXELFORMATDESCRIPTOR *)) GetProcAddress(gdi, "SetPixelFormat");
	pwglSwapBuffers = (WINBOOL (WINAPI *)(HDC)) GetProcAddress(gdi, "SwapBuffers");
	pwglDescribePixelFormat = (int (WINAPI *)(HDC, int, UINT, PIXELFORMATDESCRIPTOR *)) GetProcAddress(gdi, "DescribePixelFormat");
	
	pwglCopyContext = (WINBOOL (WINAPI *)(HGLRC, HGLRC, UINT))SDL_LoadFunction(glhandle, "wglCopyContext");
	pwglCreateContext = (HGLRC (WINAPI *)(HDC))SDL_LoadFunction(glhandle, "wglCreateContext");
	pwglCreateLayerContext = (HGLRC WINAPI (*)(HDC, int))SDL_LoadFunction(glhandle, "wglCreateLayerContext");
	pwglDeleteContext = (WINBOOL (WINAPI *)(HGLRC)) SDL_LoadFunction(glhandle, "wglDeleteContext");
	pwglGetCurrentContext = (HGLRC (WINAPI *)(void)) SDL_LoadFunction(glhandle, "wglGetCurrentContext");
	pwglGetCurrentDC = (HDC (WINAPI *)(void)) SDL_LoadFunction(glhandle, "wglGetCurrentDC");
	pwglMakeCurrent = (WINBOOL (WINAPI *)(HDC, HGLRC))SDL_LoadFunction(glhandle, "wglMakeCurrent");
	pwglShareLists = (WINBOOL (WINAPI *)(HGLRC, HGLRC)) SDL_LoadFunction(glhandle, "wglShareLists");
	pwglGetExtensionsStringARB = (const GLubyte * (WINAPI *) (HDC)) SDL_LoadFunction(glhandle, "wglGetExtensionsStringARB");
	pwglGetExtensionsStringEXT = (const GLubyte * (WINAPI *) (void)) SDL_LoadFunction(glhandle, "wglGetExtensionsStringEXT");
}

/*-----------------------------------------------------------------------*/

Win32OpenglContext::Win32OpenglContext(void *glhandle) :
	OpenglContext(glhandle),
	ctx(NULL),
	iPixelFormat(0)
{
	InitPointers(lib_handle);
}

/*-----------------------------------------------------------------------*/

Win32OpenglContext::~Win32OpenglContext()
{
	dispose();
}

/*-----------------------------------------------------------------------*/

void Win32OpenglContext::dispose()
{
	OpenglContext::DestroyContext();
	if (ctx)
	{
		pwglDeleteContext(ctx);
		ctx = 0;
	}
}

/*-----------------------------------------------------------------------*/

HGLRC Win32OpenglContext::CreateTmpContext(int &iPixelFormat, GLint _colorBits, GLint _depthBits, GLint _stencilBits, GLint _accumBits)
{
	PIXELFORMATDESCRIPTOR pfd;
	HGLRC tmp_ctx;
	
	if (!windClassRegistered)
	{
		WNDCLASS wc;
		HINSTANCE hInst;
	
		hInst = GetModuleHandle(NULL);
		
		wc.style		= CS_BYTEALIGNCLIENT | CS_OWNDC;
		wc.lpfnWndProc	= tmpWndProc;
		wc.cbClsExtra	= 0;
		wc.cbWndExtra	= 0;
		wc.hInstance	= hInst;
		wc.hIcon		= NULL;
		wc.hCursor		= NULL;
		wc.hbrBackground	= NULL;
		wc.lpszMenuName	= NULL;
		wc.lpszClassName	= "Win32OpenglTmpWindow";
		if (!RegisterClass(&wc))
		{
			D(bug("can't register window class: %s", win32_errstring(GetLastError())));
			return NULL;
		}
	
		tmpHwnd = CreateWindow("Win32OpenglTmpWindow", "", WS_POPUP | WS_DISABLED,
		                    0, 0, 10, 10,
		                    NULL, NULL, hInst, NULL);
	
		if (tmpHwnd == NULL)
		{
			D(bug("can't create window: %s", win32_errstring(GetLastError())));
			return NULL;
		}
		tmphdc = GetDC(tmpHwnd);
		if (tmphdc == NULL)
		{
			D(bug("can't get DC: %s", win32_errstring(GetLastError())));
			return NULL;
		}
		windClassRegistered = true;
	}

	if (pwglCreateContext == 0 ||
		pwglMakeCurrent == 0 ||
		pwglDeleteContext == 0 ||
		pwglChoosePixelFormat == 0 ||
		pwglGetPixelFormat == 0 ||
		pwglSetPixelFormat == 0)
	{
		D(bug("missing WGL functions"));
		return NULL;
	}

	memset(&pfd, 0, sizeof(pfd));
	pfd.nSize = sizeof(pfd);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL;
	pfd.iPixelType = _colorBits > 8 ? PFD_TYPE_RGBA : PFD_TYPE_COLORINDEX;
	pfd.cColorBits = _colorBits;
	pfd.cDepthBits = _depthBits;
	pfd.cStencilBits = _stencilBits;
	pfd.cAccumBits = _accumBits;
	pfd.cRedBits = 8;
	pfd.cGreenBits = 8;
	pfd.cBlueBits = 8;
	
	iPixelFormat = pwglChoosePixelFormat(tmphdc, &pfd);

	if (iPixelFormat == 0)
	{
		D(bug("pixel format not available: %s", win32_errstring(GetLastError())));
		return NULL;
	}

	if (!pwglSetPixelFormat(tmphdc, iPixelFormat, &pfd))
	{
		D(bug("can't set pixel format: %s", win32_errstring(GetLastError())));
		return NULL;
	}

	pwglMakeCurrent(NULL, NULL);
	
	tmp_ctx = pwglCreateContext(tmphdc);

	if (!pwglMakeCurrent(tmphdc, tmp_ctx))
	{
		D(bug("can't set context: %s", win32_errstring(GetLastError())));
		return NULL;
	}
	return tmp_ctx;
}

/*-----------------------------------------------------------------------*/

void Win32OpenglContext::DeleteTmpContext(HGLRC tmp_ctx)
{
	if (tmp_ctx)
	{
		pwglMakeCurrent(NULL, NULL);
		pwglDeleteContext(tmp_ctx);
	}
}

/*-----------------------------------------------------------------------*/

bool Win32OpenglContext::CreateContext(GLenum format, GLint _depthBits, GLint _stencilBits, GLint _accumBits, OffscreenContext *share_ctx)
{
	bool ok = false;
	Win32OpenglContext *opengl_share = (Win32OpenglContext *)share_ctx;
	
	GLint _colorBits = format == GL_COLOR_INDEX ? 8 : 32;
	ctx = CreateTmpContext(iPixelFormat, _colorBits, _depthBits, _stencilBits, _accumBits);
	if (ctx)
	{
		if (OpenglContext::CreateContext(format, _depthBits, _stencilBits, _accumBits, share_ctx))
			ok = true;
		if (opengl_share)
			if (!pwglShareLists || !pwglShareLists(opengl_share->ctx, ctx))
			{
				D(bug("wglShareLists: sharing display lists failed"));
				ok = false;
			}
	}
	return ok;
}

/*-----------------------------------------------------------------------*/

bool Win32OpenglContext::TestContext()
{
	/*
	 * we need a context to get usable values from glGetString()
	 */
	if (!CreateContext(GL_RGB, 0, 0, 0, NULL))
	{
		D(bug("can not create context"));
		dispose();
		return false;
	}
	
	const GLubyte *extensions = OSMesaDriver::fn.glGetString(GL_EXTENSIONS);
	OffscreenContext::has_MESA_pack_invert = gl_HasExtension("GL_MESA_pack_invert", extensions);
	
#if DEBUG
	{
		const GLubyte *str;
	
		str = OSMesaDriver::fn.glGetString(GL_VERSION);
		D(bug("GL Version: %s", str));
		str = OSMesaDriver::fn.glGetString(GL_VENDOR);
		D(bug("GL Vendor: %s", str));
		str = OSMesaDriver::fn.glGetString(GL_RENDERER);
		D(bug("GL Renderer: %s", str));
		str = OSMesaDriver::fn.glGetString(GL_EXTENSIONS);
		D(bug("GL Extensions: %s", str));
		str = OSMesaDriver::fn.glGetString(GL_SHADING_LANGUAGE_VERSION);
		if (str != NULL)
			D(bug("GL Shading Language : %s", str));
		if (pwglGetExtensionsStringARB)
		{
			str = pwglGetExtensionsStringARB(tmphdc);
			D(bug("Server Extensions : %s", str));
		} else if (pwglGetExtensionsStringEXT)
		{
			str = pwglGetExtensionsStringEXT();
			D(bug("Server Extensions : %s", str));
		}
	}
#endif
	
	dispose();
	return true;
}

/*-----------------------------------------------------------------------*/

GLboolean Win32OpenglContext::MakeCurrent(memptr _buffer, GLenum _type, GLsizei _width, GLsizei _height)
{
	GLboolean ok = GL_FALSE;
	
	dst_buffer = _buffer;
	type = _type;
	width = _width;
	height = _height;
	if (pwglMakeCurrent(tmphdc, ctx))
		ok = OpenglContext::MakeBufferCurrent(true);
	return ok;
}

/*-----------------------------------------------------------------------*/

GLboolean Win32OpenglContext::MakeCurrent()
{
	GLboolean ok = GL_FALSE;
	
	if (pwglMakeCurrent(tmphdc, ctx))
		ok = OpenglContext::MakeBufferCurrent(false);
	return ok;
}

/*-----------------------------------------------------------------------*/

GLboolean Win32OpenglContext::ClearCurrent()
{
	OpenglContext::ClearCurrent();
	if (!pwglMakeCurrent(NULL, NULL))
		return GL_FALSE;
	return GL_TRUE;
}

/*-----------------------------------------------------------------------*/

#if !SDL_VERSION_ATLEAST(2, 0, 0)

SDL_GLContext Win32OpenglContext::GetSDLContext(void)
{
	SDL_SysWMinfo info;
	
	SDL_VERSION(&info.version);
	if (SDL_GetWMInfo(&info) > 0)
	{
		sdlwindow = info.window;
		return (SDL_GLContext)info.hglrc;
	}
	return NULL;
}

SDL_GLContext SDL_GL_GetCurrentContext(void)
{
	return Win32OpenglContext::GetSDLContext();
}

#endif

/*-----------------------------------------------------------------------*/

void Win32OpenglContext::SetSDLContext(SDL_GLContext ctx)
{
	if (sdlwindow && pwglMakeCurrent)
	{
		HDC hdc = GetDC(sdlwindow);
		pwglMakeCurrent(hdc, (HGLRC)ctx);
		ReleaseDC(sdlwindow, hdc);
	}
}


void SDL_GL_SetCurrentContext(SDL_GLContext ctx)
{
	Win32OpenglContext::SetSDLContext(ctx);
}

#endif /* SDL_VIDEO_DRIVER_WINDOWS */

/*************************************************************************/
/*-----------------------------------------------------------------------*/
/*************************************************************************/

/* MacOSX CoreGraphics/Quartz/Cocoa */

#if defined(SDL_VIDEO_DRIVER_QUARTZ) || defined(SDL_VIDEO_DRIVER_COCOA)

QuartzOpenglContext::QuartzOpenglContext(void *glhandle) :
	OpenglContext(glhandle),
	ctx(NULL)
{
}

/*-----------------------------------------------------------------------*/

QuartzOpenglContext::~QuartzOpenglContext()
{
	dispose();
}

/*-----------------------------------------------------------------------*/

void QuartzOpenglContext::dispose()
{
	OpenglContext::DestroyContext();
	if (ctx)
	{
		CGLDestroyContext(ctx);
		ctx = 0;
	}
}

/*-----------------------------------------------------------------------*/

bool QuartzOpenglContext::CreateContext(GLenum format, GLint _depthBits, GLint _stencilBits, GLint _accumBits, OffscreenContext *share_ctx)
{
	CGLPixelFormatAttribute attribs[20];
	int i;
	int npix = 0;
	CGLError error;
	CGLPixelFormatObj pixelformat = 0;
	
	ctx = NULL;
	i = 0;
	attribs[i++] = kCGLPFACompliant;
	attribs[i++] = kCGLPFAColorSize;
	attribs[i++] = (CGLPixelFormatAttribute)(format == GL_COLOR_INDEX ? 8 : 24);
	if (_depthBits > 0)
	{
		attribs[i++] = kCGLPFADepthSize;
		attribs[i++] = (CGLPixelFormatAttribute)_depthBits;
	}
	if (_stencilBits > 0)
	{
		attribs[i++] = kCGLPFAStencilSize;
		attribs[i++] = (CGLPixelFormatAttribute)_stencilBits;
	}
	if (_accumBits > 0)
	{
		attribs[i++] = kCGLPFAAccumSize;
		attribs[i++] = (CGLPixelFormatAttribute)_accumBits;
	}
	attribs[i++] = kCGLPFAAccelerated;
	attribs[i] = (CGLPixelFormatAttribute)0;
	
	error = CGLChoosePixelFormat(attribs, &pixelformat, &npix);
	if (pixelformat == 0)
	{
		/* retry without accelerated */
		attribs[--i] = (CGLPixelFormatAttribute)0;
		error = CGLChoosePixelFormat(attribs, &pixelformat, &npix);
	}
	if (pixelformat == 0)
	{
		D(bug("No suitable visual: %s", CGLErrorString(error)));
		return false;
	}
	
	QuartzOpenglContext *opengl_share = (QuartzOpenglContext *)share_ctx;
	error = CGLCreateContext(pixelformat, opengl_share ? opengl_share->ctx : NULL, &ctx);
	CGLDestroyPixelFormat(pixelformat);
	if (ctx == NULL)
	{
		D(bug("CGLCreateContext failed: %s", CGLErrorString(error)));
		return false;
	}
	
	error = CGLSetCurrentContext(ctx);
	if (error != kCGLNoError)
	{
		D(bug("CGLSetCurrentContext failed: %s", CGLErrorString(error)));
		return false;
	}
	if (!OpenglContext::CreateContext(format, _depthBits, _stencilBits, _accumBits, share_ctx))
		return false;
	return true;
}

/*-----------------------------------------------------------------------*/

bool QuartzOpenglContext::TestContext()
{
	/*
	 * we need a context to get usable values from glGetString()
	 */
	if (!CreateContext(GL_RGB, 0, 0, 0, NULL))
	{
		D(bug("can not create context"));
		dispose();
		return false;
	}

	const GLubyte *extensions = OSMesaDriver::fn.glGetString(GL_EXTENSIONS);
	OffscreenContext::has_MESA_pack_invert = gl_HasExtension("GL_MESA_pack_invert", extensions);
	
#if DEBUG
	{
		const GLubyte *str;
		
		str = OSMesaDriver::fn.glGetString(GL_VERSION);
		D(bug("GL Version: %s", str));
		str = OSMesaDriver::fn.glGetString(GL_VENDOR);
		D(bug("GL Vendor: %s", str));
		str = OSMesaDriver::fn.glGetString(GL_RENDERER);
		D(bug("GL Renderer: %s", str));
		str = OSMesaDriver::fn.glGetString(GL_EXTENSIONS);
		D(bug("GL Extensions: %s", str));
		str = OSMesaDriver::fn.glGetString(GL_SHADING_LANGUAGE_VERSION);
		if (str != NULL)
			D(bug("GL Shading Language : %s", str));
	}
#endif

	dispose();
	return true;
}

/*-----------------------------------------------------------------------*/

GLboolean QuartzOpenglContext::MakeCurrent(memptr _buffer, GLenum _type, GLsizei _width, GLsizei _height)
{
	dst_buffer = _buffer;
	type = _type;
	width = _width;
	height = _height;
	CGLError error = CGLSetCurrentContext(ctx);
	if (error != kCGLNoError)
	{
		D(bug("CGLSetCurrentContext failed: %s", CGLErrorString(error)));
		return GL_FALSE;
	}
	return OpenglContext::MakeBufferCurrent(true);
}

/*-----------------------------------------------------------------------*/

GLboolean QuartzOpenglContext::MakeCurrent()
{
	CGLError error = CGLSetCurrentContext(ctx);
	if (error != kCGLNoError)
	{
		D(bug("CGLSetCurrentContext failed: %s", CGLErrorString(error)));
		return GL_FALSE;
	}
	return OpenglContext::MakeBufferCurrent(false);
}

/*-----------------------------------------------------------------------*/

GLboolean QuartzOpenglContext::ClearCurrent()
{
	OpenglContext::ClearCurrent();
	if (CGLSetCurrentContext(NULL) != kCGLNoError)
		return GL_FALSE;
	return GL_TRUE;
}

#endif
#endif
