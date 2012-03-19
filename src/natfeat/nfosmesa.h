/*
	NatFeat host OSMesa rendering

	ARAnyM (C) 2003 Patrice Mandin

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

#ifndef NFOSMESA_H
#define NFOSMESA_H

/*--- Includes ---*/

#include <SDL_types.h>
#include <SDL_opengl.h>
/*	On darwin/Mac OS X systems SDL_opengl.h includes OpenGL/gl.h instead of GL/gl.h, 
   	which does not define GLAPI and GLAPIENTRY used by GL/osmesa.h
*/
#if defined(__gl_h_) 
	#if !defined(GLAPI)
		#define GLAPI
	#endif
	#if !defined(GLAPIENTRY)
		#define GLAPIENTRY
	#endif
#endif
#include <GL/osmesa.h>

#include "nf_base.h"
#include "parameters.h"

/*--- Defines ---*/

#define MAX_OSMESA_CONTEXTS	16

#define	NFOSMESA_VERTEX_ARRAY	(1<<0)
#define	NFOSMESA_NORMAL_ARRAY	(1<<1)
#define	NFOSMESA_COLOR_ARRAY	(1<<2)
#define	NFOSMESA_INDEX_ARRAY	(1<<3)
#define	NFOSMESA_EDGEFLAG_ARRAY	(1<<4)
#define	NFOSMESA_TEXCOORD_ARRAY	(1<<5)

/*--- Types ---*/

typedef struct {
	GLint size;
	GLenum type;
	GLsizei stride;
	GLvoid *ptr;
} vertexarray_t;

typedef struct {
	OSMesaContext	ctx;
	void *src_buffer;	/* Host buffer, if channel reduction needed */
	void *dst_buffer;	/* Atari buffer */
	GLenum type;
	GLsizei width, height;

	/* conversion needed from srcformat to dstformat ? */
	SDL_bool conversion;
	GLenum srcformat, dstformat;

	/* Vertex arrays */
	int enabled_arrays;
	vertexarray_t	vertex;
	vertexarray_t	normal;
	vertexarray_t	color;
	vertexarray_t	texcoord;
	vertexarray_t	index;
	vertexarray_t	edgeflag;
} context_t;

/*--- Class ---*/

class OSMesaDriver : public NF_Base
{
protected:
	/* contexts[0] unused */
	context_t	contexts[MAX_OSMESA_CONTEXTS+1];
	int num_contexts, cur_context;
	void *libosmesa_handle, *libgl_handle;

	/* Some special functions, which need a bit more work */
	SDL_bool libgl_needed;
	int OpenLibrary(void);
	int CloseLibrary(void);
	void InitPointersGL(void);
	void InitPointersOSMesa(void);
	void SelectContext(Uint32 ctx);
	void ConvertContext(Uint32 ctx);	/* 8 bits per channel */
	void ConvertContext16(Uint32 ctx);	/* 16 bits per channel */
	void ConvertContext32(Uint32 ctx);	/* 32 bits per channel */

	/* Read parameter on m68k stack */
	Uint32 *ctx_ptr;	/* Current parameter list */
	Uint32 getStackedParameter(Uint32 n);
	float getStackedFloat(Uint32 n);

	Uint32 LenglGetString(Uint32 ctx, GLenum name);
	void PutglGetString(Uint32 ctx, GLenum name, GLubyte *buffer);

	GLdouble Atari2HostDouble(Uint32 high, Uint32 low);
	void Atari2HostDoublePtr(Uint32 size, Uint32 *src, GLdouble *dest);
	void Atari2HostFloatPtr(Uint32 size, Uint32 *src, GLfloat *dest);
	void Atari2HostIntPtr(Uint32 size, Uint32 *src, GLint *dest);
	void Atari2HostShortPtr(Uint32 size, Uint16 *src, GLshort *dest);

	Uint32 OSMesaCreateContext( GLenum format, Uint32 sharelist );
	Uint32 OSMesaCreateContextExt( GLenum format, GLint depthBits, GLint stencilBits, GLint accumBits, Uint32 sharelist);
	void OSMesaDestroyContext( Uint32 ctx );
	GLboolean OSMesaMakeCurrent( Uint32 ctx, void *buffer, GLenum type, GLsizei width, GLsizei height );
	Uint32 OSMesaGetCurrentContext( void );
	void OSMesaPixelStore(GLint pname, GLint value );
	void OSMesaGetIntegerv(GLint pname, GLint *value );
	GLboolean OSMesaGetDepthBuffer( Uint32 c, GLint *width, GLint *height, GLint *bytesPerValue, void **buffer );
	GLboolean OSMesaGetColorBuffer( Uint32 c, GLint *width, GLint *height, GLint *format, void **buffer );
	void *OSMesaGetProcAddress( const char *funcName );

#include "nfosmesa/proto-gl.h"
#if NFOSMESA_GLEXT
# include "nfosmesa/proto-glext.h"
#endif

public:
	const char *name() { return "OSMESA"; }
	bool isSuperOnly() { return true; }
	int32 dispatch(uint32 fncode);

	OSMesaDriver();
	virtual ~OSMesaDriver();
};

#endif /* OSMESA_H */
