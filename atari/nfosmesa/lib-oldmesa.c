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

static OSMesaContext oldmesa_ctx = NULL;
static void *oldmesa_buffer = NULL;

void *APIENTRY OSMesaCreateLDG(GLenum format, GLenum type, GLint width, GLint height)
{
	unsigned long buffer_size;
	void *buffer = NULL;
	GLenum osmesa_format;
	
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

	oldmesa_ctx = OSMesaCreateContext(osmesa_format, NULL);
	if (!oldmesa_ctx)
		return NULL;

	buffer_size = width * height;
	if (osmesa_format != OSMESA_COLOR_INDEX)
		buffer_size <<= 2;

	buffer = Atari_MxAlloc(buffer_size);

	if (buffer == NULL)
	{
		OSMesaDestroyContext(oldmesa_ctx);
		return NULL;
	}

	if (!OSMesaMakeCurrent(oldmesa_ctx, buffer, type, width, height))
	{
		Mfree(buffer);
		OSMesaDestroyContext(oldmesa_ctx);
		return NULL;
	}

	/* OSMesa draws upside down */
	OSMesaPixelStore(OSMESA_Y_UP, 0);

	memset(buffer, 0, buffer_size);
	oldmesa_buffer = buffer;
	return buffer;
}

void APIENTRY OSMesaDestroyLDG(void)
{
	if (oldmesa_ctx)
	{
		OSMesaDestroyContext(oldmesa_ctx);
		oldmesa_ctx = NULL;
	}
	if (oldmesa_buffer)
	{
		Mfree(oldmesa_buffer);
		oldmesa_buffer = NULL;
	}
}

GLsizei APIENTRY max_width(void)
{
	GLint value = 0;
	
	OSMesaGetIntegerv(OSMESA_MAX_WIDTH, &value);
	return value;
}

GLsizei APIENTRY max_height(void)
{
	GLint value = 0;
	
	OSMesaGetIntegerv(OSMESA_MAX_HEIGHT, &value);
	return value;
}


void *Atari_MxAlloc(unsigned long size)
{
	if (((Sversion()&0xFF)>=0x01) | (Sversion()>=0x1900))
	{
		return (void *)Mxalloc(size, MX_PREFTTRAM);
	}

	return (void *)Malloc(size);
}
