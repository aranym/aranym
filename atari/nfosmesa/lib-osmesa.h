/*
	OSMesa LDG linker, OSMesa*() functions using Natfeats

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

#ifndef LIB_OSMESA_H
#define LIB_OSMESA_H

/*--- Types ---*/

typedef struct osmesa_context *OSMesaContext;

/*--- Variables ---*/

extern unsigned long nfOSMesaId;
extern OSMesaContext cur_context;

/*--- Functions prototypes ---*/

OSMesaContext OSMesaCreateContext( GLenum format, OSMesaContext sharelist );
OSMesaContext OSMesaCreateContextExt( GLenum format, GLint depthBits, GLint stencilBits, GLint accumBits, OSMesaContext sharelist);
void OSMesaDestroyContext( OSMesaContext ctx );
GLboolean OSMesaMakeCurrent( OSMesaContext ctx, void *buffer, GLenum type, GLsizei width, GLsizei height );
OSMesaContext OSMesaGetCurrentContext( void );
void OSMesaPixelStore( GLint pname, GLint value );
void OSMesaGetIntegerv( GLint pname, GLint *value );
GLboolean OSMesaGetDepthBuffer( OSMesaContext c, GLint *width, GLint *height, GLint *bytesPerValue, void **buffer );
GLboolean OSMesaGetColorBuffer( OSMesaContext c, GLint *width, GLint *height, GLint *format, void **buffer );
void *OSMesaGetProcAddress( const char *funcName );

#endif
