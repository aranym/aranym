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

/*
 * wether atari addresses are passed as memptr
 * or are already converted to Host Addresses
 * when passed to the member functions;
 * FULLMMU needs memptr to allow for MMU emulation,
 * otherwise direct addresses might be a bit faster,
 * but may generate a SEGFAULT in the host rather
 * than a BUSERROR in the emulator
 */
#ifdef FULLMMU
#define NFOSMESA_POINTER_AS_MEMARG 1
#else
#define NFOSMESA_POINTER_AS_MEMARG 0
#endif

#if defined(FULLMMU) || NFOSMESA_POINTER_AS_MEMARG
#define NFOSMESA_NEED_BYTE_CONV 1
#else
#define NFOSMESA_NEED_BYTE_CONV 0
#endif

#if SDL_BYTEORDER == SDL_LIL_ENDIAN || defined(FULLMMU) || NFOSMESA_POINTER_AS_MEMARG
#define NFOSMESA_NEED_INT_CONV 1
#else
#define NFOSMESA_NEED_INT_CONV 0
#endif

#if SDL_BYTEORDER == SDL_LIL_ENDIAN || SIZEOF_FLOAT != 4 || defined(FULLMMU) || NFOSMESA_POINTER_AS_MEMARG
/* FIXME: need also conversion if host float not ieee */
#define NFOSMESA_NEED_FLOAT_CONV 1
#else
#define NFOSMESA_NEED_FLOAT_CONV 0
#endif

#if SDL_BYTEORDER == SDL_LIL_ENDIAN || SIZEOF_DOUBLE != 8 || defined(FULLMMU) || NFOSMESA_POINTER_AS_MEMARG
/* FIXME: need also conversion if host double not ieee */
#define NFOSMESA_NEED_DOUBLE_CONV 1
#else
#define NFOSMESA_NEED_DOUBLE_CONV 0
#endif

#if 0 /* for testing compilation of the non-conversion case */
#undef NFOSMESA_NEED_BYTE_CONV
#define NFOSMESA_NEED_BYTE_CONV 0
#undef NFOSMESA_NEED_INT_CONV
#define NFOSMESA_NEED_INT_CONV 0
#undef NFOSMESA_NEED_FLOAT_CONV
#define NFOSMESA_NEED_FLOAT_CONV 0
#undef NFOSMESA_NEED_DOUBLE_CONV
#define NFOSMESA_NEED_DOUBLE_CONV 0
#endif

/*--- Types ---*/

#if NFOSMESA_POINTER_AS_MEMARG
typedef memptr nfmemptr;
typedef memptr nfcmemptr;
#else
typedef char *nfmemptr;
typedef const char *nfcmemptr;
#endif

typedef struct gl_buffer {
	GLenum name;
	GLsizei size;
	nfmemptr atari_buffer;
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
	nfmemptr atari_pointer;
	char *host_pointer;
	GLint ptrstride;
	vendor_t vendor;
	GLsizei buffer_offset;
	GLuint id;
	bool alloced;
} vertexarray_t;

typedef struct {
	GLuint id;
	GLenum type;
	GLuint first;
	GLuint count;
	char *host_pointer;
	nfmemptr atari_pointer;
} fbo_buffer;

class OffscreenContext;

typedef struct {
	OffscreenContext *ctx;
	GLenum render_mode;
	GLenum error_code;
	void *feedback_buffer_host;
	nfmemptr feedback_buffer_atari;
	GLenum feedback_buffer_type;
	GLuint *select_buffer_host;
	nfmemptr select_buffer_atari;
	GLuint select_buffer_size;
	GLboolean error_check_enabled;
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
	
	GLuint share_ctx;

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

typedef void (APIENTRY *NFGL_PROC)(void);
typedef NFGL_PROC (APIENTRY *NFGL_GETPROCADDRESS)(const char *funcname);


/*--- Types ---*/

typedef struct {
#define GL_PROC(type, gl, name, export, upper, proto, args, first, ret) type (APIENTRY *gl ## name) proto ;
#define GLU_PROC(type, gl, name, export, upper, proto, args, first, ret)
#define OSMESA_PROC(type, gl, name, export, upper, proto, args, first, ret) type (APIENTRY *OSMesa ## name) proto ;
#include "../../atari/nfosmesa/glfuncs.h"
} osmesa_funcs;

#define GL_ISAVAILABLE(f) (OSMesaDriver::fn.f)

/*--- Class ---*/

class OSMesaDriver : public NF_Base
{
protected:
	/* contexts[0] unused */
	context_t contexts[MAX_OSMESA_CONTEXTS+1];
	int num_contexts;
	Uint32 cur_context;
	void *libosmesa_handle, *libgl_handle;
	static OSMesaDriver *thisdriver;
	
	bool lib_opened;
	bool open_succeeded;
	/* if true, we are using the software Mesa library;
	   if false, we are using OpenGL via Render buffers */
	bool using_mesa;
	static NFGL_GETPROCADDRESS get_procaddress;
	OffscreenContext *TryCreateContext(void);
	SDL_GLContext SDL_glctx;
	
	bool OpenLibrary(void);
	void CloseLibrary(void);
	void CloseMesaLibrary(void);
	void CloseGLLibrary(void);
	static void InitPointersOSMesa(void *handle);
	bool SelectContext(Uint32 ctx);
	void ConvertContext(Uint32 ctx);

	void glSetError(GLenum e);

	/* Some special functions, which need a bit more work */
	Uint32 LenglGetString(Uint32 ctx, GLenum name);
#if NFOSMESA_POINTER_AS_MEMARG
	void PutglGetString(Uint32 ctx, GLenum name, memptr buffer);
#else
	void PutglGetString(Uint32 ctx, GLenum name, GLubyte *buffer);
#endif

	Uint32 LenglGetStringi(Uint32 ctx, GLenum name, GLuint index);
#if NFOSMESA_POINTER_AS_MEMARG
	void PutglGetStringi(Uint32 ctx, GLenum name, GLuint index, memptr buffer);
#else
	void PutglGetStringi(Uint32 ctx, GLenum name, GLuint index, GLubyte *buffer);
#endif
	static GLsizei nfglPixelmapSize(GLenum pname);
	static int nfglGetNumParams(GLenum pname);
	static GLint __glGetMap_Evalk(GLenum target);

	/* utility functions */
	static inline GLfloat Atari2HostFloat(Uint32 value)
	{
		union {
			GLfloat f;
			Uint32 i;
		} u;
		// no SDL_SwapBE32() here; already done by getStackedParameter
		u.i = value;
		return u.f;
	}

	static inline Uint32 Host2AtariFloat(GLfloat value)
	{
		union {
			GLfloat f;
			Uint32 i;
		} u;
		u.f = value;
		return u.i;
	}

	static inline GLdouble Atari2HostDouble(Uint32 high, Uint32 low)
	{
		union {
			GLdouble d;
			Uint32 i[2];
		} u;

		// no SDL_SwapBE32() here; already done by getStackedParameter
		// but need to change order of the words
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
		u.i[0] = low;
		u.i[1] = high;
#else
		u.i[0] = high;
		u.i[1] = low;
#endif
		return u.d;
	}

#if NFOSMESA_POINTER_AS_MEMARG
	static inline void Host2AtariFloatArray(GLsizei size, const GLfloat *src, memptr dest)
	{
		if (!dest) return;
		for (GLsizei i = 0; i < size; i++) {
			WriteInt32(dest, Host2AtariFloat(src[i]));
			dest += ATARI_SIZEOF_FLOAT;
		}
	}

	static inline GLfloat *Atari2HostFloatArray(GLsizei size, memptr src, GLfloat *dest)
	{
		if (!src) return NULL;
		for (GLsizei i = 0; i < size; i++) {
			dest[i] = Atari2HostFloat(ReadInt32(src));
			src += ATARI_SIZEOF_FLOAT;
		}
		return dest;
	}

	static inline void Host2AtariDoubleArray(GLsizei size, const GLdouble *src, memptr dst)
	{
		union {
			GLdouble d;
			Uint64 i;
		} u;
		
		if (!dst) return;
		for (GLsizei i = 0; i < size; i++) {
			u.d = *src++;
			WriteInt64(dst, u.i);
			dst += ATARI_SIZEOF_DOUBLE;
		}
	}

	static inline GLdouble *Atari2HostDoubleArray(GLsizei size, memptr src, GLdouble *dst)
	{
		GLsizei i;
		union {
			GLdouble d;
			Uint64 i;
		} u;
		
		if (!src) return NULL;
		for (i = 0; i < size; i++) {
			u.i = ReadInt64(src);
			dst[i] = u.d;
			src += ATARI_SIZEOF_DOUBLE;
		}
		return dst;
	}

	static inline GLushort *Atari2HostShortArray(GLsizei size, memptr src, GLushort *dest)
	{
		GLsizei i;

		if (!src) return NULL;
		for (i = 0; i < size; i++) {
			dest[i] = ReadInt16(src);
			src += 2;
		}
		return dest;
	}

	static inline GLshort *Atari2HostShortArray(GLsizei size, memptr src, GLshort *dest)
	{
		return (GLshort *)Atari2HostShortArray(size, src, (GLushort *)dest);
	}
	
	static inline void Host2AtariShortArray(GLsizei size, const Uint16 *src, memptr dest)
	{
		GLsizei i;

		if (!dest) return;
		for (i = 0; i < size; i++) {
			WriteInt16(dest, src[i]);
			dest += 2;
		}
	}

	static inline void Host2AtariShortArray(GLsizei size, const Sint16 *src, memptr dest)
	{
		Host2AtariShortArray(size, (const Uint16 *)src, dest);
	}
	
	static inline GLuint *Atari2HostIntArray(GLsizei size, memptr src, GLuint *dest)
	{
		GLsizei i;
		
		if (!src) return NULL;
		for (i = 0; i < size; i++) {
			dest[i] = ReadInt32(src);
			src += 4;
		}
		return dest;
	}

	static inline GLint *Atari2HostIntArray(GLsizei size, memptr src, GLint *dest)
	{
		return (GLint *)Atari2HostIntArray(size, src, (GLuint *)dest);
	}
	
	/* has to be handled separate because hosts GLintptr is different than Ataris */ \
	static inline GLintptr *Atari2HostIntptrArray(GLsizei size, memptr src, GLintptr *dest)
	{
		GLsizei i;
		
		if (!src) return NULL;
		for (i = 0; i < size; i++) {
			dest[i] = ReadInt32(src);
			src += 4;
		}
		return dest;
	}

	static inline void Host2AtariIntArray(GLsizei size, const GLuint *src, memptr dest)
	{
		GLsizei i;
		
		if (!dest) return;
		for (i = 0; i < size; i++) {
			WriteInt32(dest, src[i]);
			dest += 4;
		}
	}

	static inline void Host2AtariIntArray(GLsizei size, const GLint *src, memptr dest)
	{
		Host2AtariIntArray(size, (const GLuint *)src, dest);
	}
	
#ifdef __APPLE__
	static inline void Host2AtariHandleARB(int size, const GLhandleARB *src, memptr dest)
	{
		int i;
		
		if (!dest) return;
		for (i = 0; i < size; i++)
		{
			Uint32 h = (Uint32)(uintptr_t)src[i];
			WriteInt32(dest, h);
			dest += 4;
		}
	}
#else
	static inline void Host2AtariHandleARB(int size, const GLhandleARB *src, memptr dest)
	{
		Host2AtariIntArray(size, src, dest);
	}
#endif

	static inline GLuint64 *Atari2HostInt64Array(GLsizei size, memptr src, GLuint64 *dest)
	{
		GLsizei i;
		
		if (!src) return NULL;
		for (i = 0; i < size; i++) {
			dest[i] = ReadInt64(src);
			src += 8;
		}
		return dest;
	}

	static inline GLint64 *Atari2HostInt64Array(GLsizei size, memptr src, GLint64 *dest)
	{
		return (GLint64 *)Atari2HostInt64Array(size, src, (GLuint64 *)dest);
	}
	
	static inline void Host2AtariInt64Array(GLsizei size, const Uint64 *src, memptr dest)
	{
		GLsizei i;
		
		if (!dest) return;
		for (i = 0; i < size; i++) {
			WriteInt64(dest, src[i]);
			dest += 8;
		}
	}

	static inline void Host2AtariInt64Array(GLsizei size, const Sint64 *src, memptr dest)
	{
		Host2AtariInt64Array(size, (const Uint64 *)src, dest);
	}
	
	static inline void **Atari2HostPtrArray(GLsizei size, memptr src, void **dest)
	{
		if (!src) return NULL;
		for (GLsizei i = 0; i < size; i++)
		{
			memptr p = ReadInt32(src);
			dest[i] = p ? Atari2HostAddr(p) : NULL;
			src += 4;
		}
		return dest;
	}
	
	static inline memptr *Atari2HostMemPtrArray(GLsizei size, memptr src, memptr *dest)
	{
		if (!src) return NULL;
		for (GLsizei i = 0; i < size; i++)
		{
			memptr p = ReadInt32(src);
			dest[i] = p;
			src += 4;
		}
		return dest;
	}
	
	static inline memptr *Atari2HostPtrArray(GLsizei size, memptr src, memptr *dest)
	{
		if (!src) return NULL;
		for (GLsizei i = 0; i < size; i++)
		{
			memptr p = ReadInt32(src);
			dest[i] = p;
			src += 4;
		}
		return dest;
	}
	
	static inline GLubyte *Atari2HostByteArray(GLsizei size, memptr src, GLubyte *dest)
	{
		GLsizei i;
		
		if (!src) return NULL;
		for (i = 0; i < size; i++) {
			dest[i] = ReadInt8(src);
			src += 1;
		}
		return dest;
	}

	static inline GLbyte *Atari2HostByteArray(GLsizei size, memptr src, GLbyte *dest)
	{
		return (GLbyte *)Atari2HostByteArray(size, src, (GLubyte *)dest);
	}

	static inline char *Atari2HostByteArray(GLsizei size, memptr src, char *dest)
	{
		return (char *)Atari2HostByteArray(size, src, (GLubyte *)dest);
	}

	static inline void Host2AtariByteArray(GLsizei size, const GLubyte *src, memptr dest)
	{
		GLsizei i;

		if (!dest) return;
		for (i = 0; i < size; i++) {
			WriteInt8(dest, src[i]);
			dest += 1;
		}
	}

	static inline void Host2AtariByteArray(GLsizei size, const GLbyte *src, memptr dest)
	{
		Host2AtariByteArray(size, (const GLubyte *)src, dest);
	}

	static inline void Host2AtariByteArray(GLsizei size, const GLchar *src, memptr dest)
	{
		Host2AtariByteArray(size, (const GLubyte *)src, dest);
	}

	static inline size_t safe_strlen(memptr src)
	{
		size_t i;
		char c;
		
		if (!src) return 0;
		i = 1;
		for (;;)
		{
			c = ReadInt8(src);
			if (!c)
				break;
			src += 1;
		}
		return i;
	}

#else

	static inline void Host2AtariFloatArray(GLsizei size, const GLfloat *src, GLfloat *dest)
	{
		Uint32 *p = (Uint32 *)dest;
		if (!dest) return;
		for (GLsizei i = 0; i < size; i++) {
			p[i] = SDL_SwapBE32(Host2AtariFloat(src[i]));
		}
	}

	static inline GLfloat *Atari2HostFloatArray(GLsizei size, const GLfloat *src, GLfloat *dest)
	{
		const Uint32 *p = (const Uint32 *)src;
		if (!src) return NULL;
		for (GLsizei i = 0; i < size; i++) {
			dest[i] = Atari2HostFloat(SDL_SwapBE32(p[i]));
		}
		return dest;
	}

	static inline GLfloat *Atari2HostFloatArray(GLsizei size, const void *src, GLfloat *dest)
	{
		return Atari2HostFloatArray(size, (const GLfloat *)src, dest);
	}
	
	static inline void Host2AtariDoubleArray(GLsizei size, const GLdouble *src, GLdouble *dst)
	{
		union {
			GLdouble d;
			Uint64 i;
		} u;
		Uint64 *p = (Uint64 *)dst;
		
		if (!dst) return;
		for (GLsizei i = 0; i < size; i++) {
			u.d = *src++;
			*p = SDL_SwapBE64(u.i);
			p++;
		}
	}

	static inline GLdouble *Atari2HostDoubleArray(GLsizei size, const GLdouble *src, GLdouble *dest)
	{
		GLsizei i;
		
		const Uint32 *p = (const Uint32 *)src;
		if (!src) return NULL;
		for (i = 0; i < size; i++) {
			dest[i] = Atari2HostDouble(SDL_SwapBE32(p[i<<1]),SDL_SwapBE32(p[(i<<1)+1]));
		}
		return dest;
	}

	static inline GLdouble *Atari2HostDoubleArray(GLsizei size, const void *src, GLdouble *dest)
	{
		return Atari2HostDoubleArray(size, (const GLdouble *)src, dest);
	}
	
	static inline GLushort *Atari2HostShortArray(GLsizei size, const Uint16 *src, GLushort *dest)
	{
		GLsizei i;
		
		if (!src) return NULL;
		for (i = 0; i < size; i++) {
			dest[i] = SDL_SwapBE16(src[i]);
		}
		return dest;
	}

	static inline GLshort *Atari2HostShortArray(GLsizei size, const Sint16 *src, GLshort *dest)
	{
		return (GLshort *)Atari2HostShortArray(size, (const Uint16 *)src, (GLushort *)dest);
	}
	
	static inline GLushort *Atari2HostShortArray(GLsizei size, const void *src, GLushort *dest)
	{
		return Atari2HostShortArray(size, (const Uint16 *)src, dest);
	}
	
	static inline void Host2AtariShortArray(GLsizei size, const Uint16 *src, GLushort *dest)
	{
		GLsizei i;

		if (!dest) return;
		for (i = 0; i < size; i++) {
			dest[i] = SDL_SwapBE16(src[i]);
		}
	}

	static inline void Host2AtariShortArray(GLsizei size, const Sint16 *src, GLshort *dest)
	{
		Host2AtariShortArray(size, (const Uint16 *)src, (GLushort *)dest);
	}
	
	static inline GLuint *Atari2HostIntArray(GLsizei size, const Uint32 *src, GLuint *dest)
	{
		GLsizei i;
		
		if (!src) return NULL;
		for (i = 0; i < size; i++) {
			dest[i] = SDL_SwapBE32(src[i]);
		}
		return dest;
	}

	static inline GLint *Atari2HostIntArray(GLsizei size, const Sint32 *src, GLint *dest)
	{
		return (GLint *)Atari2HostIntArray(size, (const Uint32 *)src, (GLuint *)dest);
	}
	
	static inline GLuint *Atari2HostIntArray(GLsizei size, const void *src, GLuint *dest)
	{
		return Atari2HostIntArray(size, (const Uint32 *)src, dest);
	}
	
	/* has to be handled separate because hosts GLintptr is different than Ataris */ \
	static inline GLintptr *Atari2HostIntptrArray(GLsizei size, const GLintptr *src, GLintptr *dest)
	{
		GLsizei i;
		
		if (!src) return NULL;
		const Uint32 *p = (const Uint32 *)src; \
		for (i = 0; i < size; i++) {
			dest[i] = SDL_SwapBE32(p[i]);
		}
		return dest;
	}

	static inline void Host2AtariIntArray(GLsizei size, const Uint32 *src, GLuint *dest)
	{
		GLsizei i;
		
		if (!dest) return;
		for (i = 0; i < size; i++) {
			dest[i] = SDL_SwapBE32(src[i]);
		}
	}

	static inline void Host2AtariIntArray(GLsizei size, const Sint32 *src, GLint *dest)
	{
		Host2AtariIntArray(size, (const Uint32 *)src, (GLuint *)dest);
	}
	
#ifdef __APPLE__
	static inline void Host2AtariHandleARB(int size, const GLhandleARB *src, GLhandleARB *dest)
	{
		int i;
			
		if (!dest) return;
		GLuint *p = (GLuint *)dest;
		for (i = 0; i < size; i++)
		{
			Uint32 h = (Uint32)(uintptr_t)src[i];
			p[i] = SDL_SwapBE32(h);
		}
	}
#else
	static inline void Host2AtariHandleARB(int size, const GLhandleARB *src, GLhandleARB *dest)
	{
		Host2AtariIntArray(size, src, dest);
	}
#endif

    static inline GLuint64 *Atari2HostInt64Array(GLsizei size, const Uint64 *src, GLuint64 *dest)
	{
		GLsizei i;
		
		if (!src) return NULL;
		for (i = 0; i < size; i++) {
			dest[i] = SDL_SwapBE64(src[i]);
		}
		return dest;
	}

	static inline GLint64 *Atari2HostInt64Array(GLsizei size, const Sint64 *src, GLint64 *dest)
	{
		return (GLint64 *)Atari2HostInt64Array(size, (const Uint64 *)src, (GLuint64 *)dest);
	}
	
	static inline void Host2AtariInt64Array(GLsizei size, const Uint64 *src, GLuint64 *dest)
	{
		GLsizei i;
		
		if (!dest) return;
		for (i = 0; i < size; i++) {
			dest[i] = SDL_SwapBE64(src[i]);
		}
	}

	static inline void Host2AtariInt64Array(GLsizei size, const Sint64 *src, GLint64 *dest)
	{
		Host2AtariInt64Array(size, (const Uint64 *)src, (GLuint64 *)dest);
	}
	
	static inline void **Atari2HostPtrArray(GLsizei size, const void **src, void **dest)
	{
		if (!src) return NULL;
		const memptr *tmp = (const memptr *)src;
		for (GLsizei i = 0; i < size; i++)
		{
			memptr p = SDL_SwapBE32(tmp[i]);
			dest[i] = p ? Atari2HostAddr(p) : NULL;
		}
		return dest;
	}
	
	static inline nfmemptr *Atari2HostPtrArray(GLsizei size, const void **src, char **dest)
	{
		return (nfmemptr *)Atari2HostPtrArray(size, src, (void **)dest);
	}
	
	static inline void **Atari2HostPtrArray(GLsizei size, const char *const *src, void **dest)
	{
		return Atari2HostPtrArray(size, (const void **)src, dest);
	}
	
	static inline void **Atari2HostPtrArray(GLsizei size, const void *const *src, void **dest)
	{
		return Atari2HostPtrArray(size, (const void **)src, dest);
	}
	
	static inline void **Atari2HostPtrArray(GLsizei size, char **src, void **dest)
	{
		return Atari2HostPtrArray(size, (const void **)src, dest);
	}
	
	static inline void **Atari2HostMemPtrArray(GLsizei size, const void *const *src, nfmemptr *dest)
	{
		return Atari2HostPtrArray(size, (const void **)src, (void **)dest);
	}
	
	static inline GLubyte *Atari2HostByteArray(GLsizei size, const GLubyte *src, GLubyte *dest)
	{
		GLsizei i;

		if (!src) return NULL;
		for (i = 0; i < size; i++) {
			dest[i] = src[i];
		}
		return dest;
	}

	static inline GLbyte *Atari2HostByteArray(GLsizei size, const GLbyte *src, GLbyte *dest)
	{
		return (GLbyte *)Atari2HostByteArray(size, (const GLubyte *)src, (GLubyte *)dest);
	}

	static inline GLchar *Atari2HostByteArray(GLsizei size, const GLchar *src, GLchar *dest)
	{
		return (GLchar *)Atari2HostByteArray(size, (const GLubyte *)src, (GLubyte *)dest);
	}

	static inline GLubyte *Atari2HostByteArray(GLsizei size, const void *src, GLubyte *dest)
	{
		return Atari2HostByteArray(size, (const GLubyte *)src, dest);
	}

	static inline void Host2AtariByteArray(GLsizei size, const GLubyte *src, GLubyte *dest)
	{
		GLsizei i;

		if (!dest) return;
		for (i = 0; i < size; i++) {
			dest[i] = src[i];
		}
	}

	static inline void Host2AtariByteArray(GLsizei size, const GLbyte *src, GLbyte *dest)
	{
		Host2AtariByteArray(size, (const GLubyte *)src, (GLubyte *)dest);
	}

	static inline void Host2AtariByteArray(GLsizei size, const GLchar *src, GLchar *dest)
	{
		Host2AtariByteArray(size, (const GLubyte *)src, (GLubyte *)dest);
	}

	static inline void Host2AtariByteArray(GLsizei size, const GLubyte *src, void *dest)
	{
		Host2AtariByteArray(size, src, (GLubyte *)dest);
	}

	static inline size_t safe_strlen(const char *src)
	{
		if (!src) return 0;
		return strlen(src);
	}

	static inline size_t safe_strlen(const unsigned char *src)
	{
		return safe_strlen((const char *)src);
	}

	static inline size_t safe_strlen(const void *src)
	{
		return safe_strlen((const char *)src);
	}

#endif

	class pixelBuffer {
	private:
		OSMesaDriver &driver;
		GLsizei width;
		GLsizei height;
		GLsizei depth;
		GLenum format;
		GLenum type;
		GLsizei basesize;
		GLsizei componentcount;
		GLsizei count;
		size_t bufsize;
		char *buffer;
		bool alloced;
		bool valid;
	public:
		pixelBuffer(OSMesaDriver &_driver, GLsizei _width, GLsizei _height, GLsizei _depth, GLenum _format, GLenum _type) :
			driver(_driver),
			width(_width),
			height(_height),
			depth(_depth),
			format(_format),
			type(_type),
			basesize(0),
			componentcount(0),
			buffer(NULL),
			alloced(false),
			valid(false)
		{
		}
		pixelBuffer(OSMesaDriver &_driver) :
			driver(_driver),
			width(0),
			height(0),
			depth(0),
			format(0),
			type(0),
			basesize(0),
			componentcount(0),
			count(0),
			bufsize(0),
			buffer(NULL),
			alloced(false),
			valid(false)
		{
		}
		pixelBuffer() :
			driver(*thisdriver),
			width(0),
			height(0),
			depth(0),
			format(0),
			type(0),
			basesize(0),
			componentcount(0),
			count(0),
			bufsize(0),
			buffer(NULL),
			alloced(false),
			valid(false)
		{
		}
		virtual ~pixelBuffer() {
			if (alloced)
				free(buffer);
		}
		bool validate();
		GLsizei ComponentCount() { return componentcount; }
		GLsizei Count() { return count; }
		char *hostBuffer(GLsizei _width, GLsizei _height, GLsizei _depth, GLenum _format, GLenum _type, nfmemptr dest);
		char *hostBuffer(GLsizei _bufSize, GLenum _format, GLenum _type, nfmemptr dest);
		bool params(GLsizei _width, GLsizei _height, GLsizei _depth, GLenum _format, GLenum _type);
		bool params(GLsizei _bufSize, GLenum _format, GLenum _type);
		void convertToAtari(const char *src, nfmemptr dst);
		void *convertArray(GLsizei _count, GLenum _type, nfmemptr src);
		void *convertPixels(GLsizei _width, GLsizei _height, GLsizei _depth, GLenum _format, GLenum _type, nfmemptr src);
	};
	
	void *load_gl_library(const char *pathlist);
	
	void nfglArrayElementHelper(GLint i);
	void convertClientArrays(GLsizei count);
	void convertClientArray(GLsizei count, vertexarray_t &array);
	void setupClientArray(GLenum texunit, vertexarray_t &array, GLint size, GLenum type, GLsizei stride, GLsizei count, GLint ptrstride, nfmemptr pointer);
	void nfglInterleavedArraysHelper(GLenum format, GLsizei stride, nfmemptr pointer);
	void gl_bind_buffer(GLenum target, GLuint buffer, GLuint first, GLuint count);
	void gl_get_pointer(GLenum target, GLuint index, void **data);
	
	gl_buffer_t *gl_get_buffer(GLuint name);
	gl_buffer_t *gl_make_buffer(GLsizei size, nfcmemptr pointer);
	vertexarray_t *gl_get_array(GLenum pname);
	
	/* OSMesa functions */
	Uint32 OSMesaCreateContext( GLenum format, Uint32 sharelist );
	Uint32 OSMesaCreateContextExt( GLenum format, GLint depthBits, GLint stencilBits, GLint accumBits, Uint32 sharelist);
	void OSMesaDestroyContext( Uint32 ctx );
	GLboolean OSMesaMakeCurrent( Uint32 ctx, memptr buffer, GLenum type, GLsizei width, GLsizei height );
	Uint32 OSMesaGetCurrentContext( void );
	void OSMesaPixelStore(GLint pname, GLint value );
#if NFOSMESA_POINTER_AS_MEMARG
	void OSMesaGetIntegerv(GLint pname, memptr value );
	GLboolean OSMesaGetDepthBuffer( Uint32 c, memptr width, memptr height, memptr bytesPerValue, memptr buffer );
	GLboolean OSMesaGetColorBuffer( Uint32 c, memptr width, memptr height, memptr format, memptr buffer );
#else
	void OSMesaGetIntegerv(GLint pname, GLint *value );
	GLboolean OSMesaGetDepthBuffer( Uint32 c, GLint *width, GLint *height, GLint *bytesPerValue, void **buffer );
	GLboolean OSMesaGetColorBuffer( Uint32 c, GLint *width, GLint *height, GLint *format, void **buffer );
#endif
	void OSMesaPostprocess(Uint32 ctx, const char *filter, GLuint enable_value);
	unsigned int OSMesaGetProcAddress( nfcmemptr funcName );
	void OSMesaColorClamp(GLboolean enable);
	
	/* tinyGL functions */
	void nfglFrustumf(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat near_val, GLfloat far_val);
	void nfglOrthof(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat near_val, GLfloat far_val);
	void nfgluLookAtf(GLfloat eyeX, GLfloat eyeY, GLfloat eyeZ, GLfloat centerX, GLfloat centerY, GLfloat centerZ, GLfloat upX, GLfloat upY, GLfloat upZ);
	void nftinyglswapbuffer(memptr buf);

	/* GL functions */
	
#if NFOSMESA_POINTER_AS_MEMARG
#define GL_PROC(type, gl, name, export, upper, proto, args, first, ret)
#define GL_PROCM(type, gl, name, export, upper, proto, args, first, ret) type nf ## gl ## name proto ;
#else
#define GL_PROC(type, gl, name, export, upper, proto, args, first, ret) type nf ## gl ## name proto ;
#endif
#include "../../atari/nfosmesa/glfuncs.h"

public:
	const char *name() { return "OSMESA"; }
	bool isSuperOnly() { return false; }
	int32 dispatch(uint32 fncode);

	static osmesa_funcs fn;
	static void InitPointersGL(void *handle);

	static GLenum PrintErrors(const char *funcname);
	
	OSMesaDriver();
	virtual ~OSMesaDriver();
	void reset();
};

#endif /* OSMESA_H */
