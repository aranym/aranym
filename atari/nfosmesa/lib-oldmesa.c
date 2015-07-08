/*
	OSMesa LDG linker, compatibility functions with old mesa_gl.ldg/tiny_gl.ldg

	Copyright (C) 2004	Patrice Mandin

	This library is free software; you can redistribute it and/or
	modify it under the terms of the GNU Lesser General Public
	License as published by the Free Software Foundation; either
	version 2.1 of the License, or (at your option) any later version.

	This library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public
	License along with this library; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
*/

/*--- Includes ---*/

#include <stdlib.h>
#include <string.h>

#include <mint/osbind.h>

#include "lib-osmesa.h"
#include "lib-oldmesa.h"
#include "lib-misc.h"

/*--- Defines ---*/

#define VDI_ARGB		0x8
#define VDI_RGB			0xf
#define DIRECT_VDI_ARGB	0x10

#define GL_RGB                                     0x1907
#define GL_COLOR_INDEX                             0x1900

#define OSMESA_ARGB            0x2
#define OSMESA_RGB             GL_RGB
#define OSMESA_COLOR_INDEX     GL_COLOR_INDEX
#define OSMESA_MAX_WIDTH       0x24
#define OSMESA_MAX_HEIGHT      0x25
#define OSMESA_Y_UP            0x11

/*--- Functions ---*/

void *APIENTRY internal_OSMesaCreateLDG(gl_private *private, GLenum format, GLenum type, GLint width, GLint height)
{
	size_t buffer_size;
	void *buffer = NULL;
	GLenum osmesa_format;
	OSMesaContext oldmesa_ctx;
	unsigned int ctx;
	
	switch (format)
	{
	case VDI_ARGB:
	case DIRECT_VDI_ARGB:
		osmesa_format = OSMESA_ARGB;
		break;
	case VDI_RGB:
		osmesa_format = OSMESA_RGB;
		break;
	default:
		osmesa_format = format;
		break;
	}

	oldmesa_ctx = internal_OSMesaCreateContext(private, osmesa_format, NULL);
	if (!oldmesa_ctx)
		return NULL;

	{
		size_t wdwidth = (width + 15) >> 4;
		size_t pitch = wdwidth << 1;
		if (osmesa_format != OSMESA_COLOR_INDEX)
			pitch <<= 5;
		else
			pitch <<= 3;
		buffer_size = pitch * height;
	}
	
	buffer = private->pub.m_alloc(buffer_size);
	
	if (buffer == NULL)
	{
		internal_OSMesaDestroyContext(private, oldmesa_ctx);
		return NULL;
	}

	if (!internal_OSMesaMakeCurrent(private, oldmesa_ctx, buffer, type, width, height))
	{
		private->pub.m_free(buffer);
		internal_OSMesaDestroyContext(private, oldmesa_ctx);
#ifdef TGL_ENABLE_CHECKS
		gl_fatal_error(private, GL_OUT_OF_MEMORY, 13, "out of memory");
#endif
		return NULL;
	}

	/* OSMesa draws upside down */
	internal_OSMesaPixelStore(private, OSMESA_Y_UP, 0);

	memset(buffer, 0, buffer_size);
	ctx = CTX_TO_IDX(oldmesa_ctx);
	private->contexts[ctx].oldmesa_buffer = buffer;
	return buffer;
}


void APIENTRY internal_OSMesaDestroyLDG(gl_private *private)
{
	unsigned int ctx = CTX_TO_IDX(private->cur_context);
	if (private->contexts[ctx].oldmesa_buffer)
	{
		private->pub.m_free(private->contexts[ctx].oldmesa_buffer);
		private->contexts[ctx].oldmesa_buffer = NULL;
	}
	if (private->cur_context)
	{
		internal_OSMesaDestroyContext(private, private->cur_context);
		private->cur_context = NULL;
	}
}


GLsizei APIENTRY internal_max_width(gl_private *private)
{
	GLint value = 0;
	
	internal_OSMesaGetIntegerv(private, OSMESA_MAX_WIDTH, &value);
	return value;
}


GLsizei APIENTRY internal_max_height(gl_private *private)
{
	GLint value = 0;
	
	internal_OSMesaGetIntegerv(private, OSMESA_MAX_HEIGHT, &value);
	return value;
}
