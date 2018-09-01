/*
	OSMesa LDG linker, misc functions

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

extern gl_private *private;;

/*--- Defines ---*/

/*--- Functions ---*/


const GLubyte* APIENTRY glGetString( GLenum name )
{
	return internal_glGetString(private, name);
}


const GLubyte* APIENTRY glGetStringi( GLenum name, GLuint index )
{
	return internal_glGetStringi(private, name, index);
}


OSMesaContext APIENTRY OSMesaCreateContext(GLenum format, OSMesaContext sharelist)
{
	return internal_OSMesaCreateContext(private, format, sharelist);
}


OSMesaContext APIENTRY OSMesaCreateContextAttribs(const GLint *attribList, OSMesaContext sharelist)
{
	return internal_OSMesaCreateContextAttribs(private, attribList, sharelist);
}


void APIENTRY OSMesaDestroyContext(OSMesaContext ctx)
{
	internal_OSMesaDestroyContext(private, ctx);
}


OSMesaContext APIENTRY OSMesaCreateContextExt(GLenum format, GLint depthBits, GLint stencilBits, GLint accumBits, OSMesaContext sharelist)
{
	return internal_OSMesaCreateContextExt(private, format, depthBits, stencilBits, accumBits, sharelist);
}


GLboolean APIENTRY OSMesaMakeCurrent(OSMesaContext ctx, void *buffer, GLenum type, GLsizei width, GLsizei height)
{
	return internal_OSMesaMakeCurrent(private, ctx, buffer, type, width, height);
}


OSMesaContext APIENTRY OSMesaGetCurrentContext(void)
{
	return internal_OSMesaGetCurrentContext(private);
}


void APIENTRY OSMesaPixelStore(GLint pname, GLint value)
{
	internal_OSMesaPixelStore(private, pname, value);
}


void APIENTRY OSMesaGetIntegerv(GLint pname, GLint *value)
{
	internal_OSMesaGetIntegerv(private, pname, value);
}


GLboolean APIENTRY OSMesaGetDepthBuffer(OSMesaContext c, GLint *width, GLint *height, GLint *bytesPerValue, void **buffer)
{
	return internal_OSMesaGetDepthBuffer(private, c, width, height, bytesPerValue, buffer);
}


GLboolean APIENTRY OSMesaGetColorBuffer(OSMesaContext c, GLint *width, GLint *height, GLint *format, void **buffer )
{
	return internal_OSMesaGetColorBuffer(private, c, width, height, format, buffer);
}


OSMESAproc APIENTRY OSMesaGetProcAddress(const char *funcName)
{
	return internal_OSMesaGetProcAddress(private, funcName);
}


void APIENTRY OSMesaColorClamp(GLboolean32 enable)
{
	internal_OSMesaColorClamp(private, enable);
}


void APIENTRY OSMesaPostprocess(OSMesaContext osmesa, const char *filter, GLuint enable_value)
{
	internal_OSMesaPostprocess(private, osmesa, filter, enable_value);
}


void *APIENTRY OSMesaCreateLDG(GLenum format, GLenum type, GLint width, GLint height)
{
	return internal_OSMesaCreateLDG(private, format, type, width, height);
}


void APIENTRY OSMesaDestroyLDG(void)
{
	internal_OSMesaDestroyLDG(private);
}


GLsizei APIENTRY max_width(void)
{
	return internal_max_width(private);
}


GLsizei APIENTRY max_height(void)
{
	return internal_max_height(private);
}


void APIENTRY tinyglswapbuffer(void *buf)
{
	internal_tinyglswapbuffer(private, buf);
}


void APIENTRY tinyglexception_error(void CALLBACK (*exception)(GLenum param))
{
	internal_tinyglexception_error(private, exception);
}
