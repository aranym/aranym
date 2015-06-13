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

#include "SDL_compat.h"
#include <SDL_types.h>
#include "SDL_opengl_wrapper.h"
#include <GL/osmesa.h>
#include "../../atari/nfosmesa/gltypes.h"

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

#define ATARI_SIZEOF_DOUBLE ((size_t)8)
#define ATARI_SIZEOF_FLOAT ((size_t)4)

#if SDL_BYTEORDER == SDL_LIL_ENDIAN
#define NFOSMESA_NEED_INT_CONV 1
#else
#define NFOSMESA_NEED_INT_CONV 0
#endif

#if SDL_BYTEORDER == SDL_LIL_ENDIAN || SIZEOF_FLOAT != 4
/* FIXME: need also conversion if host float not ieee */
#define NFOSMESA_NEED_FLOAT_CONV 1
#else
#define NFOSMESA_NEED_FLOAT_CONV 0
#endif

#if SDL_BYTEORDER == SDL_LIL_ENDIAN || SIZEOF_DOUBLE != 8
/* FIXME: need also conversion if host double not ieee */
#define NFOSMESA_NEED_DOUBLE_CONV 1
#else
#define NFOSMESA_NEED_DOUBLE_CONV 0
#endif

/*--- Types ---*/

typedef struct {
	GLint size;
	GLenum type;
	GLsizei stride;
	GLsizei count;
	const GLvoid *pointer;
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
	int num_contexts;
	Uint32 cur_context;
	void *libosmesa_handle, *libgl_handle;

	/* Some special functions, which need a bit more work */
	int OpenLibrary(void);
	int CloseLibrary(void);
	void InitPointersGL(void *handle);
	void InitPointersOSMesa(void *handle);
	bool SelectContext(Uint32 ctx);
	void ConvertContext(Uint32 ctx);	/* 8 bits per channel */
	void ConvertContext16(Uint32 ctx);	/* 16 bits per channel */
	void ConvertContext32(Uint32 ctx);	/* 32 bits per channel */

	static void *APIENTRY glNop(void);
#define GL_ISNOP(f) ((void *(APIENTRY*)(void))(f) == glNop)

	void glSetError(GLenum) { }

	Uint32 LenglGetString(Uint32 ctx, GLenum name);
	void PutglGetString(Uint32 ctx, GLenum name, GLubyte *buffer);

	Uint32 LenglGetStringi(Uint32 ctx, GLenum name, GLuint index);
	void PutglGetStringi(Uint32 ctx, GLenum name, GLuint index, GLubyte *buffer);

	/* utility functions */
	inline GLfloat Atari2HostFloat(Uint32 value)
	{
		union {
			GLfloat f;
			Uint32 i;
		} u;
		u.i = value;
		return u.f;
	}

	inline Uint32 Host2AtariFloat(GLfloat value)
	{
		union {
			GLfloat f;
			Uint32 i;
		} u;
		u.f = value;
		return SDL_SwapBE32(u.i);
	}

	inline void Host2AtariFloatArray(Uint32 size, const GLfloat *src, GLfloat *dest)
	{
		Uint32 *p = (Uint32 *)dest;
		for (Uint32 i=0;i<size;i++) {
			p[i]=Host2AtariFloat(src[i]);
		}
	}

	inline void Atari2HostFloatArray(Uint32 size, const Uint32 *src, GLfloat *dest)
	{
		for (Uint32 i=0;i<size;i++) {
			dest[i]=Atari2HostFloat(SDL_SwapBE32(src[i]));
		}
	}

	inline void Atari2HostFloatPtr(Uint32 size, const GLfloat *src, GLfloat *dest)
	{
		Atari2HostFloatArray(size, (const Uint32 *)src, dest);
	}
	
	inline GLdouble Atari2HostDouble(Uint32 high, Uint32 low)
	{
		union {
			GLdouble d;
			Uint32 i[2];
		} u;

#if SDL_BYTEORDER == SDL_LIL_ENDIAN
		u.i[0]=low;
		u.i[1]=high;
#else
		u.i[0]=high;
		u.i[1]=low;
#endif
		return u.d;
	}

	inline void Host2AtariDouble(GLdouble src, GLdouble *dst)
	{
		union {
			GLdouble d;
			Uint32 i[2];
		} u;
		Uint32 *p = (Uint32 *)dst;
		
		u.d = src;
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
		p[0] = SDL_SwapBE32(u.i[1]);
		p[1] = SDL_SwapBE32(u.i[0]);
#else
		p[0] = u.i[0];
		p[1] = u.i[1];
#endif
	}

	inline void Host2AtariDoubleArray(Uint32 size, const GLdouble *src, GLdouble *dst)
	{
		union {
			GLdouble d;
			Uint32 i[2];
		} u;
		Uint32 *p = (Uint32 *)dst;
		
		for (Uint32 i=0;i<size;i++) {
			u.d = *src++;
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
			p[0] = SDL_SwapBE32(u.i[1]);
			p[1] = SDL_SwapBE32(u.i[0]);
#else
			p[0] = u.i[0];
			p[1] = u.i[1];
#endif
			p += 2;
		}
	}

	inline void Atari2HostDoubleArray(Uint32 size, const Uint32 *src, GLdouble *dest)
	{
		Uint32 i;
		
		for (i=0;i<size;i++) {
			dest[i]=Atari2HostDouble(SDL_SwapBE32(src[i<<1]),SDL_SwapBE32(src[(i<<1)+1]));
		}
	}

	inline void Atari2HostDoublePtr(Uint32 size, const GLdouble *src, GLdouble *dest)
	{
		Atari2HostDoubleArray(size, (const Uint32 *)src, dest);
	}

	inline void Atari2HostShortArray(Uint32 size, const Uint16 *src, GLushort *dest)
	{
		Uint32 i;
		GLushort *tmp = dest;

		for (i=0;i<size;i++) {
			tmp[i]=SDL_SwapBE16(src[i]);
		}
	}

	inline void Atari2HostShortPtr(Uint32 size, const Uint16 *src, GLushort *dest)
	{
		Atari2HostShortArray(size, src, dest);
	}
	
	inline void Atari2HostShortPtr(Uint32 size, const Sint16 *src, GLushort *dest)
	{
		Atari2HostShortArray(size, (const Uint16 *)src, dest);
	}
	
	inline void Atari2HostShortPtr(Uint32 size, const Sint16 *src, GLshort *dest)
	{
		Atari2HostShortArray(size, (const Uint16 *)src, (GLushort *)dest);
	}
	
	inline void Atari2HostShortPtr(Uint32 size, const Uint16 *src, GLshort *dest)
	{
		Atari2HostShortArray(size, src, (GLushort *)dest);
	}
	
	inline void Atari2HostIntArray(Uint32 size, const Uint32 *src, GLuint *dest)
	{
		Uint32 i;
		GLuint *tmp = dest;
		
		for (i=0;i<size;i++) {
			tmp[i]=SDL_SwapBE32(src[i]);
		}
	}

	inline void Atari2HostIntPtr(Uint32 size, const Uint32 *src, GLuint *dest)
	{
		Atari2HostIntArray(size, src, dest);
	}
	
	inline void Atari2HostIntPtr(Uint32 size, const Sint32 *src, GLuint *dest)
	{
		Atari2HostIntArray(size, (const Uint32 *)src, dest);
	}
	
	inline void Atari2HostIntPtr(Uint32 size, const Sint32 *src, GLint *dest)
	{
		Atari2HostIntArray(size, (const Uint32 *)src, (GLuint *)dest);
	}
	
	inline void Atari2HostIntPtr(Uint32 size, const Uint32 *src, GLint *dest)
	{
		Atari2HostIntArray(size, src, (GLuint *)dest);
	}
	
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
	void *convertPixels(GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
	void nfglArrayElementHelper(GLint i);
	void nfglInterleavedArraysHelper(GLenum format, GLsizei stride, const GLvoid *pointer);
#endif
	
	/* OSMesa functions */
	Uint32 OSMesaCreateContext( GLenum format, Uint32 sharelist );
	Uint32 OSMesaCreateContextExt( GLenum format, GLint depthBits, GLint stencilBits, GLint accumBits, Uint32 sharelist);
	void OSMesaDestroyContext( Uint32 ctx );
	GLboolean OSMesaMakeCurrent( Uint32 ctx, memptr buffer, GLenum type, GLsizei width, GLsizei height );
	Uint32 OSMesaGetCurrentContext( void );
	void OSMesaPixelStore(GLint pname, GLint value );
	void OSMesaGetIntegerv(GLint pname, GLint *value );
	GLboolean OSMesaGetDepthBuffer( Uint32 c, GLint *width, GLint *height, GLint *bytesPerValue, void **buffer );
	GLboolean OSMesaGetColorBuffer( Uint32 c, GLint *width, GLint *height, GLint *format, void **buffer );
	void *OSMesaGetProcAddress( const char *funcName );
	void OSMesaColorClamp(GLboolean enable);
	void OSMesaPostprocess(Uint32 ctx, const char *filter, GLuint enable_value);
	
	/* tinyGL functions */
	void nfglFrustumf(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat near_val, GLfloat far_val);
	void nfglOrthof(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat near_val, GLfloat far_val);
	void nfgluLookAtf(GLfloat eyeX, GLfloat eyeY, GLfloat eyeZ, GLfloat centerX, GLfloat centerY, GLfloat centerZ, GLfloat upX, GLfloat upY, GLfloat upZ);
	void nftinyglswapbuffer(memptr buf);

	/* GL functions */
	
#define GL_PROC(type, gl, name, export, upper, params, first, ret) type nf ## gl ## name params ;
#include "../../atari/nfosmesa/glfuncs.h"

public:
	const char *name() { return "OSMESA"; }
	bool isSuperOnly() { return false; }
	int32 dispatch(uint32 fncode);

	OSMesaDriver();
	virtual ~OSMesaDriver();
};

#endif /* OSMESA_H */
