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
#ifdef HAVE_GL_OSMESA_H
#include <GL/osmesa.h>
#endif
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
#define	NFOSMESA_FOGCOORD_ARRAY	(1<<6)
#define	NFOSMESA_2NDCOLOR_ARRAY	(1<<7)
#define	NFOSMESA_ELEMENT_ARRAY	(1<<8)
#define	NFOSMESA_VARIANT_ARRAY	(1<<9)
#define	NFOSMESA_WEIGHT_ARRAY	(1<<10)
#define	NFOSMESA_MATRIX_INDEX_ARRAY	(1<<11)
#define	NFOSMESA_REPLACEMENT_CODE_ARRAY	(1<<12)

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

#if 0 /* for testing compilation of the non-conversion case */
#undef NFOSMESA_NEED_INT_CONV
#define NFOSMESA_NEED_INT_CONV 0
#undef NFOSMESA_NEED_FLOAT_CONV
#define NFOSMESA_NEED_FLOAT_CONV 0
#undef NFOSMESA_NEED_DOUBLE_CONV
#define NFOSMESA_NEED_DOUBLE_CONV 0
#endif

/*--- Types ---*/

typedef struct gl_buffer {
	GLenum name;
	GLsizei size;
	char *atari_buffer;
	char *host_buffer;
	GLenum usage;
	GLboolean normalized;
	struct gl_buffer *next;
} gl_buffer_t;

typedef enum {
	NFOSMESA_VENDOR_NONE,
	NFOSMESA_VENDOR_ARB,
	NFOSMESA_VENDOR_EXT,
	NFOSMESA_VENDOR_IBM,
	NFOSMESA_VENDOR_SGI,
	NFOSMESA_VENDOR_INTEL,
	NFOSMESA_VENDOR_APPLE,
	NFOSMESA_VENDOR_NV,
	NFOSMESA_VENDOR_ATI,
	NFOSMESA_VENDOR_SUN
} vendor_t;

typedef struct {
	GLint size;
	GLenum type;
	GLsizei atari_stride, host_stride, defstride;
	GLsizei basesize;
	GLsizei count;
	GLsizei converted;
	GLvoid *atari_pointer;
	void *host_pointer;
	GLint ptrstride;
	vendor_t vendor;
	GLsizei buffer_offset;
	GLuint id;
} vertexarray_t;

typedef struct {
	GLuint id;
	GLenum type;
	GLuint first;
	GLuint count;
	void *host_pointer;
	void *atari_pointer;
} fbo_buffer;

typedef struct {
	OSMesaContext	ctx;
	void *src_buffer;	/* Host buffer, if channel reduction needed */
	void *dst_buffer;	/* Atari buffer */
	GLenum type;
	GLsizei width, height;
	GLenum render_mode;
	GLenum error_code;
	void *feedback_buffer_host;
	void *feedback_buffer_atari;
	GLenum feedback_buffer_type;
	GLuint *select_buffer_host;
	Uint32 *select_buffer_atari;
	GLuint select_buffer_size;
	struct {
		fbo_buffer array;
		fbo_buffer atomic_counter;
		fbo_buffer copy_read;
		fbo_buffer copy_write;
		fbo_buffer dispatch_indirect;
		fbo_buffer draw_indirect;
		fbo_buffer element_array;
		fbo_buffer pixel_pack;
		fbo_buffer pixel_unpack;
		fbo_buffer query;
		fbo_buffer shader_storage;
		fbo_buffer texture;
		fbo_buffer transform_feedback;
		fbo_buffer uniform;
	} buffer_bindings;
	
	gl_buffer_t *buffers;
	
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
	vertexarray_t	fogcoord;
	vertexarray_t	secondary_color;
	vertexarray_t	element;
	vertexarray_t	variant;
	vertexarray_t	weight;
	vertexarray_t	matrixindex;
	vertexarray_t	replacement_code;
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

	void glSetError(GLenum e) { contexts[cur_context].error_code = e; }

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
		// no SDL_SwapBE32() here; already done by getStackedParameter
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

	inline void Atari2HostFloatArray(Uint32 size, const GLfloat *src, GLfloat *dest)
	{
		const Uint32 *p = (const Uint32 *)src;
		for (Uint32 i=0;i<size;i++) {
			dest[i]=Atari2HostFloat(SDL_SwapBE32(p[i]));
		}
	}

	inline GLdouble Atari2HostDouble(Uint32 high, Uint32 low)
	{
		union {
			GLdouble d;
			Uint32 i[2];
		} u;

		// no SDL_SwapBE32() here; already done by getStackedParameter
		// but need to change order of the words
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

	inline void Atari2HostDoubleArray(Uint32 size, const GLdouble *src, GLdouble *dest)
	{
		Uint32 i;
		
		const Uint32 *p = (const Uint32 *)src;
		for (i=0;i<size;i++) {
			dest[i]=Atari2HostDouble(SDL_SwapBE32(p[i<<1]),SDL_SwapBE32(p[(i<<1)+1]));
		}
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
	
	inline void Atari2HostInt64Array(Uint32 size, const Uint64 *src, GLuint64 *dest)
	{
		Uint32 i;
		GLuint64 *tmp = dest;
		
		for (i=0;i<size;i++) {
			tmp[i]=SDL_SwapBE64(src[i]);
		}
	}

	inline void Atari2HostInt64Ptr(Uint32 size, const Uint64 *src, GLuint64 *dest)
	{
		Atari2HostInt64Array(size, src, dest);
	}
	
	inline void Atari2HostInt64Ptr(Uint32 size, const Sint64 *src, GLint64 *dest)
	{
		Atari2HostInt64Array(size, (const Uint64 *)src, (GLuint64 *)dest);
	}
	
	void *load_gl_library(const char *pathlist);
	
	bool pixelParams(GLenum format, GLenum type, GLsizei &size, GLsizei &count);
	void *pixelBuffer(GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, GLsizei &size, GLsizei &count);
	void *convertPixels(GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid *pixels);
	void *convertArray(GLsizei count, GLenum type, const GLvoid *pixels);
	void nfglArrayElementHelper(GLint i);
	void convertClientArrays(GLsizei count);
	void convertClientArray(GLsizei count, vertexarray_t &array);
	void setupClientArray(GLenum texunit, vertexarray_t &array, GLint size, GLenum type, GLsizei stride, GLsizei count, GLint ptrstride, const GLvoid *pointer);
	void nfglInterleavedArraysHelper(GLenum format, GLsizei stride, const GLvoid *pointer);
	void gl_bind_buffer(GLenum target, GLuint buffer, GLuint first, GLuint count);
	void gl_get_pointer(GLenum target, GLuint index, void **data);
	
	gl_buffer_t *gl_get_buffer(GLuint name);
	gl_buffer_t *gl_make_buffer(GLuint name, GLsizei size, const void *pointer);
	vertexarray_t *gl_get_array(GLenum pname);
	
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
	
#define GL_PROC(type, gl, name, export, upper, proto, args, first, ret) type nf ## gl ## name proto ;
#include "../../atari/nfosmesa/glfuncs.h"

public:
	const char *name() { return "OSMESA"; }
	bool isSuperOnly() { return false; }
	int32 dispatch(uint32 fncode);

	OSMesaDriver();
	virtual ~OSMesaDriver();
};

#endif /* OSMESA_H */
