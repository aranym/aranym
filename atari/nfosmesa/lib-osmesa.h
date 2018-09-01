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

#include <stddef.h>
#include <stdint.h>

/*--- Types ---*/

typedef struct osmesa_context *OSMesaContext;

#include "gltypes.h"

#define MAX_OSMESA_CONTEXTS    16

typedef struct {
	void *oldmesa_buffer;
	GLubyte *gl_strings[7];
} gl_context;

typedef struct {
	struct gl_public pub;				/* must be fisrt element of structure */
	OSMesaContext cur_context;
	void CALLBACK (*gl_exception)(GLenum param);
	gl_context contexts[MAX_OSMESA_CONTEXTS];
} gl_private;

#define CTX_TO_IDX(ctx) ((unsigned int)(ctx))

/*--- Variables ---*/

extern long (*HostCall_p)(unsigned long function_number, OSMesaContext ctx, void *first_param);
#ifndef TINYGL_ONLY
extern void (*HostCall64_p)(unsigned long function_number, OSMesaContext ctx, void *first_param, GLuint64 *retvalue);
#endif

/*--- Functions prototypes ---*/

void APIENTRY internal_OSMesaPixelStore(gl_private *private, GLint pname, GLint value);
OSMesaContext APIENTRY internal_OSMesaCreateContext(gl_private *private, GLenum format, OSMesaContext sharelist);
OSMesaContext APIENTRY internal_OSMesaCreateContextExt(gl_private *private, GLenum format, GLint depthBits, GLint stencilBits, GLint accumBits, OSMesaContext sharelist);
void APIENTRY internal_OSMesaDestroyContext(gl_private *private, OSMesaContext ctx);
GLboolean APIENTRY internal_OSMesaMakeCurrent(gl_private *private, OSMesaContext ctx, void *buffer, GLenum type, GLsizei width, GLsizei height);
OSMesaContext APIENTRY internal_OSMesaGetCurrentContext(gl_private *private);
void APIENTRY internal_OSMesaPixelStore(gl_private *private, GLint pname, GLint value);
void APIENTRY internal_OSMesaGetIntegerv(gl_private *private, GLint pname, GLint *value);
GLboolean APIENTRY internal_OSMesaGetDepthBuffer(gl_private *private, OSMesaContext c, GLint *width, GLint *height, GLint *bytesPerValue, void **buffer );
GLboolean APIENTRY internal_OSMesaGetColorBuffer(gl_private *private, OSMesaContext c, GLint *width, GLint *height, GLint *format, void **buffer );
OSMESAproc APIENTRY internal_OSMesaGetProcAddress(gl_private *private, const char *funcName);
void APIENTRY internal_OSMesaColorClamp(gl_private *private, GLboolean32 enable);
void APIENTRY internal_OSMesaPostprocess(gl_private *private, OSMesaContext osmesa, const char *filter, GLuint enable_value);
OSMesaContext APIENTRY internal_OSMesaCreateContextAttribs(gl_private *private, const GLint *attribList, OSMesaContext sharelist);

#endif
