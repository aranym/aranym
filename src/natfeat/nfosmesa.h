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
#include <GL/osmesa.h>

#include "nf_base.h"
#include "parameters.h"

/*--- Defines ---*/

#define MAX_OSMESA_CONTEXTS	16

/*--- Types ---*/

typedef struct {
	OSMesaContext	ctx;
	void *buffer;
	GLenum type;
	GLsizei width, height;
	SDL_bool conversion;	/* conversion needed from srcformat to dstformat ? */
	GLenum srcformat, dstformat;
} context_t;

/*--- Class ---*/

class OSMesaDriver : public NF_Base
{
	protected:
		/* contexts[0] unused */
		context_t	contexts[MAX_OSMESA_CONTEXTS+1];
		int num_contexts, cur_context;
		void *library_handle;

		/* Some special functions, which need a bit more work */
		int OpenLibrary(void);
		int CloseLibrary(void);
		void SelectContext(Uint32 ctx);
		Uint32 LenglGetString(Uint32 ctx, GLenum name);
		void PutglGetString(Uint32 ctx, GLenum name, GLubyte *buffer);
		GLdouble Atari2HostDouble(Uint32 high, Uint32 low);
		void Atari2HostDoublePtr(Uint32 size, Uint32 *src, GLdouble *dest);
		GLfloat Atari2HostFloat(Uint32 high, Uint32 low);
		void Atari2HostFloatPtr(Uint32 size, Uint32 *src, GLfloat *dest);
		void Atari2HostIntPtr(Uint32 size, Uint32 *src, GLint *dest);
		void Atari2HostShortPtr(Uint32 size, Uint16 *src, GLshort *dest);
		void ConvertContext(Uint32 ctx);

		Uint32 OSMesaCreateContext( GLenum format, Uint32 sharelist );
		Uint32 OSMesaCreateContextExt( GLenum format, GLint depthBits, GLint stencilBits, GLint accumBits, Uint32 sharelist);
		void OSMesaDestroyContext( Uint32 ctx );
		GLboolean OSMesaMakeCurrent( Uint32 ctx, void *buffer, GLenum type, GLsizei width, GLsizei height );
		Uint32 OSMesaGetCurrentContext( void );
		void OSMesaPixelStore( Uint32 c, GLint pname, GLint value );
		void OSMesaGetIntegerv( Uint32 c, GLint pname, GLint *value );
		GLboolean OSMesaGetDepthBuffer( Uint32 c, GLint *width, GLint *height, GLint *bytesPerValue, void **buffer );
		GLboolean OSMesaGetColorBuffer( Uint32 c, GLint *width, GLint *height, GLint *format, void **buffer );
		void *OSMesaGetProcAddress( const char *funcName );


#include "nfosmesa/proto-gl.h"
#if NFOSMESA_GLEXT
# include "nfosmesa/proto-glext.h"
#endif

	public:
		char *name();
		bool isSuperOnly();
		int32 dispatch(uint32 fncode);

		OSMesaDriver();
		virtual ~OSMesaDriver();
};

#endif /* OSMESA_H */
