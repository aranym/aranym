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

#include <SDL_loadso.h>
#include <SDL_endian.h>
#include <GL/osmesa.h>

#include "sysdeps.h"
#include "cpu_emulation.h"
#include "parameters.h"
#include "nfosmesa.h"
#include "../../atari/nfosmesa/nfosmesa_nfapi.h"

#define DEBUG 1
#include "debug.h"

/*--- Defines ---*/

#define EINVFN -32

/*--- Types ---*/

typedef struct {
	#include "nfosmesa/pointers-osmesa.h"
	#include "nfosmesa/pointers-gl.h"
#if NFOSMESA_GLEXT
	#include "nfosmesa/pointers-glext.h"
#endif
} osmesa_funcs;

/*--- Variables ---*/

static osmesa_funcs fn;

/*--- Constructor/Destructor ---*/

OSMesaDriver::OSMesaDriver()
{
	D(bug("nfosmesa: OSMesaDriver()"));
	memset(contexts, 0, sizeof(contexts));
	num_contexts = 0;
	cur_context = -1;
	library_handle = NULL;
}

OSMesaDriver::~OSMesaDriver()
{
	int i;

	D(bug("nfosmesa: ~OSMesaDriver()"));
	for (i=1;i<=MAX_OSMESA_CONTEXTS;i++) {
		if (contexts[i].ctx) {
			OSMesaDestroyContext(i);
			contexts[i].ctx=NULL;
		}
	}
	num_contexts=0;
	cur_context = -1;

	if (library_handle) {
		SDL_UnloadObject(library_handle);
		library_handle = NULL;
	}
}

/*--- Public functions ---*/

char *OSMesaDriver::name()
{
	return "OSMESA";
}

bool OSMesaDriver::isSuperOnly()
{
	return false;
}

int32 OSMesaDriver::dispatch(uint32 fncode)
{
	int32 ret = 0;

/*	D(bug("nfosmesa: dispatch(%u)", fncode));*/

	switch(fncode) {
		case GET_VERSION:
    		ret = ARANFOSMESA_NFAPI_VERSION;
			break;
		case NFOSMESA_LENGLGETSTRING:
			ret = LenglGetString(getParameter(0),getParameter(1));
			break;
		case NFOSMESA_PUTGLGETSTRING:
			PutglGetString(getParameter(0),getParameter(1),(GLubyte *)Atari2HostAddr(getParameter(2)));
			break;
#include "nfosmesa/dispatch-osmesa.c"
#include "nfosmesa/dispatch-gl.c"
#if NFOSMESA_GLEXT
# include "nfosmesa/dispatch-glext.c"
#endif
		default:
			D(bug("nfosmesa: unimplemented function #%d", fncode));
			ret = EINVFN;
			break;
	}
/*	D(bug("nfosmesa: function returning with 0x%08x", ret));*/
	return ret;
}

/*--- Protected functions ---*/

int OSMesaDriver::OpenLibrary(void)
{
	D(bug("nfosmesa: OpenLibrary"));
	if (!bx_options.osmesa.enabled) {
		D(bug("nfosmesa: NFOSMesa is disabled"));
		return -1;
	}

	if (library_handle) {
		return 0;
	}

	library_handle=SDL_LoadObject(bx_options.osmesa.library);
	if (library_handle==NULL) {
		D(bug("nfosmesa: Can not load '%s' library\n", bx_options.osmesa.library));
		fprintf(stderr, "nfosmesa: %s\n", SDL_GetError());
		return -1;
	}

	fn.OSMesaCreateContext =
		(OSMesaContext (*)(GLenum,OSMesaContext))
		SDL_LoadFunction(library_handle,"OSMesaCreateContext");
	fn.OSMesaCreateContextExt =
		(OSMesaContext (*)(GLenum,GLint,GLint,GLint,OSMesaContext))
		SDL_LoadFunction(library_handle,"OSMesaCreateContextExt");
	fn.OSMesaDestroyContext =
		(void (*)(OSMesaContext))
		SDL_LoadFunction(library_handle,"OSMesaDestroyContext");
	fn.OSMesaMakeCurrent =
		(GLboolean (*)(OSMesaContext,void *,GLenum,GLsizei,GLsizei))
		SDL_LoadFunction(library_handle,"OSMesaMakeCurrent");
	fn.OSMesaGetCurrentContext =
		(OSMesaContext (*)(void))
		SDL_LoadFunction(library_handle,"OSMesaGetCurrentContext");
	fn.OSMesaPixelStore =
		(void (*)(GLint,GLint))
		SDL_LoadFunction(library_handle,"OSMesaPixelStore");
	fn.OSMesaGetIntegerv =
		(void (*)(GLint,GLint *))
		SDL_LoadFunction(library_handle,"OSMesaGetIntegerv");
	fn.OSMesaGetDepthBuffer =
		(GLboolean (*)(OSMesaContext,GLint *,GLint *,GLint *,void **))
		SDL_LoadFunction(library_handle,"OSMesaGetDepthBuffer");
	fn.OSMesaGetColorBuffer =
		(GLboolean (*)(OSMesaContext,GLint *,GLint *,GLint *,void **))
		SDL_LoadFunction(library_handle,"OSMesaGetColorBuffer");
	fn.OSMesaGetProcAddress =
		(void *(*)(const char *))
		SDL_LoadFunction(library_handle,"OSMesaGetProcAddress");

#include "nfosmesa/load-gl.c"
#if NFOSMESA_GLEXT
# include "nfosmesa/load-glext.c"
#endif

	return 0;
}

int OSMesaDriver::CloseLibrary(void)
{
	D(bug("nfosmesa: CloseLibrary"));
	if (!library_handle) {
		return -1;
	}

	SDL_UnloadObject(library_handle);

	fn.OSMesaCreateContext =
		(OSMesaContext (*)(GLenum,OSMesaContext))
		NULL;
	fn.OSMesaCreateContextExt =
		(OSMesaContext (*)(GLenum,GLint,GLint,GLint,OSMesaContext))
		NULL;
	fn.OSMesaDestroyContext =
		(void (*)(OSMesaContext))
		NULL;
	fn.OSMesaMakeCurrent =
		(GLboolean (*)(OSMesaContext,void *,GLenum,GLsizei,GLsizei))
		NULL;
	fn.OSMesaGetCurrentContext =
		(OSMesaContext (*)(void))
		NULL;
	fn.OSMesaPixelStore =
		(void (*)(GLint,GLint))
		NULL;
	fn.OSMesaGetIntegerv =
		(void (*)(GLint,GLint *))
		NULL;
	fn.OSMesaGetDepthBuffer =
		(GLboolean (*)(OSMesaContext,GLint *,GLint *,GLint *,void **))
		NULL;
	fn.OSMesaGetColorBuffer =
		(GLboolean (*)(OSMesaContext,GLint *,GLint *,GLint *,void **))
		NULL;
	fn.OSMesaGetProcAddress =
		(void *(*)(const char *))
		NULL;

#include "nfosmesa/unload-gl.c"
#if NFOSMESA_GLEXT
# include "nfosmesa/unload-glext.c"
#endif

	return 0;
}

void OSMesaDriver::SelectContext(Uint32 ctx)
{
	if ((ctx>MAX_OSMESA_CONTEXTS) || (cur_context<0)) {
		return;
	}
	if ((Uint32)cur_context != ctx) {
		fn.OSMesaMakeCurrent(contexts[ctx].ctx, contexts[ctx].buffer, contexts[ctx].type, contexts[ctx].width, contexts[ctx].height);
		cur_context = ctx;
	}
}

Uint32 OSMesaDriver::OSMesaCreateContext( GLenum format, Uint32 sharelist )
{
	D(bug("nfosmesa: OSMesaCreateContext"));
	return OSMesaCreateContextExt(format, 16, 8, (format == OSMESA_COLOR_INDEX) ? 0 : 16, sharelist);
}

Uint32 OSMesaDriver::OSMesaCreateContextExt( GLenum format, GLint depthBits, GLint stencilBits, GLint accumBits, Uint32 sharelist)
{
	int i,j;
	OSMesaContext share_ctx;
	GLenum osmesa_format;

	D(bug("nfosmesa: OSMesaCreateContextExt"));

	if (sharelist || (num_contexts==MAX_OSMESA_CONTEXTS)) {
		return 0;
	}

	if (num_contexts==0) {
		if (OpenLibrary()<0) {
			return 0;
		}
		D(bug("nfosmesa: Library loaded"));
	}

	/* Find a free context */
	j=0;
	for (i=1;i<=MAX_OSMESA_CONTEXTS;i++) {
		if (contexts[i].ctx==NULL) {
			j=i;
			break;
		}
	}

	/* Create our host OSMesa context */
	if (j==0) {
		D(bug("nfosmesa: No free context found"));
		return 0;
	}
	share_ctx = NULL;
	if (sharelist) {
		share_ctx = contexts[sharelist].ctx;
	}

	/* Select format */
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
	switch(format) {
		case OSMESA_BGRA:
			osmesa_format = OSMESA_ARGB;
			contexts[j].conversion = SDL_FALSE;
			break;
		case OSMESA_ARGB:
			osmesa_format = OSMESA_BGRA;
			contexts[j].conversion = SDL_FALSE;
			break;
		case OSMESA_RGBA:
			osmesa_format = OSMESA_BGRA;
			contexts[j].conversion = SDL_TRUE;
			break;
		case OSMESA_RGB_565:
			osmesa_format = OSMESA_RGB_565;
			contexts[j].conversion = SDL_TRUE;
			break;
		case OSMESA_RGB:
		case OSMESA_BGR:
		case OSMESA_COLOR_INDEX:
		default:
			osmesa_format = format;
			contexts[j].conversion = SDL_FALSE;
			break;
	}
#else
	osmesa_format = format;
	contexts[j].conversion = SDL_FALSE;
#endif

	contexts[j].ctx=fn.OSMesaCreateContextExt(osmesa_format,depthBits,stencilBits,accumBits,share_ctx);
	if (contexts[j].ctx==NULL) {
		D(bug("nfosmesa: Can not create context"));
		return 0;
	}
	contexts[j].srcformat = osmesa_format;
	contexts[j].dstformat = format;
	num_contexts++;
	return j;
}

void OSMesaDriver::OSMesaDestroyContext( Uint32 ctx )
{
	D(bug("nfosmesa: OSMesaDestroyContext"));
	if (ctx>MAX_OSMESA_CONTEXTS) {
		return;
	}
	
	if (!contexts[ctx].ctx) {
		return;
	}

	fn.OSMesaDestroyContext(contexts[ctx].ctx);
	num_contexts--;
	contexts[ctx].ctx=NULL;
/*
	if (num_contexts==0) {
		CloseLibrary();
	}
*/
}

GLboolean OSMesaDriver::OSMesaMakeCurrent( Uint32 ctx, void *buffer, GLenum type, GLsizei width, GLsizei height )
{
	D(bug("nfosmesa: OSMesaMakeCurrent"));
	if (ctx>MAX_OSMESA_CONTEXTS) {
		return GL_FALSE;
	}
	
	if (!contexts[ctx].ctx) {
		return GL_FALSE;
	}

	cur_context = ctx;
	contexts[ctx].buffer = buffer;
	contexts[ctx].type = type;
	contexts[ctx].width = width;
	contexts[ctx].height = height;
	return fn.OSMesaMakeCurrent(contexts[ctx].ctx, buffer, type, width, height);
}

Uint32 OSMesaDriver::OSMesaGetCurrentContext( void )
{
	D(bug("nfosmesa: OSMesaGetCurrentContext"));
	return cur_context;
}

void OSMesaDriver::OSMesaPixelStore( Uint32 c, GLint pname, GLint value )
{
	D(bug("nfosmesa: OSMesaPixelStore"));
	SelectContext(c);
	fn.OSMesaPixelStore(pname, value);
}

void OSMesaDriver::OSMesaGetIntegerv( Uint32 c, GLint pname, GLint *value )
{
	GLint tmp;

	D(bug("nfosmesa: OSMesaGetIntegerv"));
	SelectContext(c);
	fn.OSMesaGetIntegerv(pname, &tmp);
	*value = SDL_SwapBE32(tmp);
}

GLboolean OSMesaDriver::OSMesaGetDepthBuffer( Uint32 c, GLint *width, GLint *height, GLint *bytesPerValue, void **buffer )
{
	D(bug("nfosmesa: OSMesaGetDepthBuffer"));
	SelectContext(c);
	*width = SDL_SwapBE32(contexts[c].width);
	*height = SDL_SwapBE32(contexts[c].height);
	return GL_FALSE;
}

GLboolean OSMesaDriver::OSMesaGetColorBuffer( Uint32 c, GLint *width, GLint *height, GLint *format, void **buffer )
{
	D(bug("nfosmesa: OSMesaGetColorBuffer"));
	SelectContext(c);
	*width = SDL_SwapBE32(contexts[c].width);
	*height = SDL_SwapBE32(contexts[c].height);
	return GL_FALSE;
}

void *OSMesaDriver::OSMesaGetProcAddress( const char *funcName )
{
	D(bug("nfosmesa: OSMesaGetProcAddress"));
	return NULL;
}

Uint32 OSMesaDriver::LenglGetString(Uint32 ctx, GLenum name)
{
	D(bug("nfosmesa: LenglGetString"));
	return strlen((const char *)glGetString(ctx,name));
}

void OSMesaDriver::PutglGetString(Uint32 ctx, GLenum name, GLubyte *buffer)
{
	D(bug("nfosmesa: PutglGetString"));
	strcpy((char *)buffer,(const char *)glGetString(ctx,name));
}

GLdouble OSMesaDriver::Atari2HostDouble(Uint32 high, Uint32 low)
{
	GLdouble tmp;
	Uint32 *ptr;

	ptr = (Uint32 *)&tmp;
	ptr[0]=low;
	ptr[1]=high;
/*	D(bug("nfosmesa: 0x%08x:%08x -> %.3f",high,low,tmp));*/
	return tmp;
}

void OSMesaDriver::Atari2HostDoublePtr(Uint32 size, Uint32 *src, GLdouble *dest)
{
	Uint32 i;
	
	for (i=0;i<size;i++) {
		dest[i]=Atari2HostDouble(SDL_SwapBE32(src[i<<1]),SDL_SwapBE32(src[(i<<1)+1]));
	}
}

GLfloat OSMesaDriver::Atari2HostFloat(Uint32 high, Uint32 low)
{
	GLfloat tmp;

	tmp=(GLfloat)Atari2HostDouble(high,low);
	return tmp;
}

void OSMesaDriver::Atari2HostFloatPtr(Uint32 size, Uint32 *src, GLfloat *dest)
{
	Uint32 i,*tmp;
	
	tmp = (Uint32 *)dest;
	
	for (i=0;i<size;i++) {
		tmp[i]=SDL_SwapBE32(src[i]);
	}
}

void OSMesaDriver::ConvertContext(Uint32 ctx)
{
	int x,y, srcpitch;

	if (contexts[ctx].conversion==SDL_FALSE) {
		return;
	}

	D(bug("nfosmesa: ConvertContext"));

	switch(contexts[ctx].srcformat) {
		case OSMESA_RGB_565:
			{
				Uint16 *srcline,*srccol,color;

	D(bug("nfosmesa: ConvertContext LE:565->BE:565, %dx%d",contexts[ctx].width,contexts[ctx].height));
				srcline = (Uint16 *)contexts[ctx].buffer;
				srcpitch = contexts[ctx].width;
				for (y=0;y<contexts[ctx].height;y++) {
					srccol=srcline;
					for (x=0;x<contexts[ctx].width;x++) {
						color=SDL_SwapBE16(*srccol);
						*srccol++=color;
					}
					srcline += srcpitch;
				}
			}
			break;
		case OSMESA_BGRA: /* le:bgra to be:rgba */
			{
				Uint32 *srcline,*srccol,color;

	D(bug("nfosmesa: ConvertContext LE:BGRA->BE:RGBA"));
				srcline = (Uint32 *)contexts[ctx].buffer;
				srcpitch = contexts[ctx].width;
				for (y=0;y<contexts[ctx].height;y++) {
					srccol = srcline;
					for (x=0;x<contexts[ctx].width;x++) {
						color=*srccol; /* le:bgra = be:argb */
						color=(((color&0xffffff00)>>8)|((color&0xff)<<24)); /* le:abgr = be:rgba */
						*srccol++=color;
					}
					srcline+=srcpitch;
				}
			}
			break;
	}
}

#include "nfosmesa/call-gl.c"
#if NFOSMESA_GLEXT
# include "nfosmesa/call-glext.c"
#endif
