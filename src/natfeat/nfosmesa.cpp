/*
	NatFeat host OSMesa rendering

	ARAnyM (C) 2004,2005 Patrice Mandin

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

#ifdef OS_darwin
#include <CoreFoundation/CoreFoundation.h>
#endif

#include <SDL_loadso.h>
#include <SDL_endian.h>
#define NFOSMESA_GLEXT	0

#include "sysdeps.h"
#include "cpu_emulation.h"
#include "parameters.h"
#include "nfosmesa.h"
#include "../../atari/nfosmesa/nfosmesa_nfapi.h"

#define DEBUG 0
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
	libgl_handle = libosmesa_handle = NULL;
	libgl_needed = SDL_FALSE;
	ctx_ptr = NULL;
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

	CloseLibrary();
}

/*--- Public functions ---*/

int32 OSMesaDriver::dispatch(uint32 fncode)
{
	int32 ret = 0;
	
	SelectContext(getParameter(0));
	ctx_ptr = (Uint32 *)Atari2HostAddr(getParameter(1));

	switch(fncode) {
		case GET_VERSION:
    		ret = ARANFOSMESA_NFAPI_VERSION;
			break;
		case NFOSMESA_LENGLGETSTRING:
			ret = LenglGetString(getStackedParameter(0),getStackedParameter(1));
			break;
		case NFOSMESA_PUTGLGETSTRING:
			PutglGetString(getStackedParameter(0),getStackedParameter(1),(GLubyte *)Atari2HostAddr(getStackedParameter(2)));
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
#ifdef OS_darwin
	char exedir[MAXPATHLEN];
	char curdir[MAXPATHLEN];
	getcwd(curdir, MAXPATHLEN);
	CFURLRef url = CFBundleCopyExecutableURL(CFBundleGetMainBundle());
	CFURLRef url2 = CFURLCreateCopyDeletingLastPathComponent(0, url);
	CFURLGetFileSystemRepresentation(url2, false, (UInt8 *)exedir, MAXPATHLEN);
	CFRelease(url2);
	CFRelease(url);
#endif
	
	D(bug("nfosmesa: OpenLibrary"));

	/* Check if channel size is correct */
	switch(bx_options.osmesa.channel_size) {
		case 16:
		case 32:
			D(bug("nfosmesa: Channel size: %d -> libGL included in libOSMesa", bx_options.osmesa.channel_size));
			libgl_needed = SDL_FALSE;
			break;
		default:
			D(bug("nfosmesa: Channel size: %d -> libGL separated from libOSMesa", bx_options.osmesa.channel_size));
			libgl_needed = SDL_TRUE;
			break;
	}

	/* Load LibGL if needed */
	if ((libgl_handle==NULL) && libgl_needed) {
		libgl_handle=SDL_LoadObject(bx_options.osmesa.libgl);
#ifdef OS_darwin
		/* If loading failed, try to load from executable directory */
		if (libgl_handle==NULL) {
			chdir(exedir);
			libgl_handle=SDL_LoadObject(bx_options.osmesa.libgl);
			chdir(curdir);
		}
#endif
		if (libgl_handle==NULL) {
			D(bug("nfosmesa: Can not load '%s' library", bx_options.osmesa.libgl));
			fprintf(stderr, "nfosmesa: %s\n", SDL_GetError());
			return -1;
		}
		InitPointersGL();
		D(bug("nfosmesa: OpenLibrary(): libGL loaded"));
	}

	/* Load libOSMesa */
	if (libosmesa_handle==NULL) {
		libosmesa_handle=SDL_LoadObject(bx_options.osmesa.libosmesa);
#ifdef OS_darwin
		/* If loading failed, try to load from executable directory */
		if (libosmesa_handle==NULL) {
			chdir(exedir);
			libosmesa_handle=SDL_LoadObject(bx_options.osmesa.libosmesa);
			chdir(curdir);
		}
#endif
		if (libosmesa_handle==NULL) {
			D(bug("nfosmesa: Can not load '%s' library", bx_options.osmesa.libosmesa));
			fprintf(stderr, "nfosmesa: %s\n", SDL_GetError());
			return -1;
		}
		if (!libgl_needed) {
			libgl_handle = libosmesa_handle;
			InitPointersGL();
			libgl_handle = NULL;
		}
		InitPointersOSMesa();
		D(bug("nfosmesa: OpenLibrary(): libOSMesa loaded"));
	}

	return 0;
}

int OSMesaDriver::CloseLibrary(void)
{
	D(bug("nfosmesa: CloseLibrary"));

	if (libosmesa_handle) {
		SDL_UnloadObject(libosmesa_handle);
		libosmesa_handle=NULL;
	}

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
			(GLboolean (*)(GLint *,GLint *,GLint *,void **))
			NULL;
		fn.OSMesaGetColorBuffer =
			(GLboolean (*)(GLint *,GLint *,GLint *,void **))
			NULL;
		fn.OSMesaGetProcAddress =
			(void *(*)(const char *))
			NULL;

	D(bug("nfosmesa: CloseLibrary(): libOSMesa unloaded"));

	if (libgl_handle) {
		SDL_UnloadObject(libgl_handle);
		libgl_handle=NULL;
	}

#include "nfosmesa/unload-gl.c"
#if NFOSMESA_GLEXT
# include "nfosmesa/unload-glext.c"
#endif

	D(bug("nfosmesa: CloseLibrary(): libGL unloaded"));

	return 0;
}

void OSMesaDriver::InitPointersGL(void)
{
	D(bug("nfosmesa: InitPointersGL()"));

#include "nfosmesa/load-gl.c"
#if NFOSMESA_GLEXT
# include "nfosmesa/load-glext.c"
#endif
}

void OSMesaDriver::InitPointersOSMesa(void)
{
	D(bug("nfosmesa: InitPointersOSMesa()"));

		fn.OSMesaCreateContext =
			(OSMesaContext (*)(GLenum,OSMesaContext))
			SDL_LoadFunction(libosmesa_handle,"OSMesaCreateContext");
		fn.OSMesaCreateContextExt =
			(OSMesaContext (*)(GLenum,GLint,GLint,GLint,OSMesaContext))
			SDL_LoadFunction(libosmesa_handle,"OSMesaCreateContextExt");
		fn.OSMesaDestroyContext =
			(void (*)(OSMesaContext))
			SDL_LoadFunction(libosmesa_handle,"OSMesaDestroyContext");
		fn.OSMesaMakeCurrent =
			(GLboolean (*)(OSMesaContext,void *,GLenum,GLsizei,GLsizei))
			SDL_LoadFunction(libosmesa_handle,"OSMesaMakeCurrent");
		fn.OSMesaGetCurrentContext =
			(OSMesaContext (*)(void))
			SDL_LoadFunction(libosmesa_handle,"OSMesaGetCurrentContext");
		fn.OSMesaPixelStore =
			(void (*)(GLint,GLint))
			SDL_LoadFunction(libosmesa_handle,"OSMesaPixelStore");
		fn.OSMesaGetIntegerv =
			(void (*)(GLint,GLint *))
			SDL_LoadFunction(libosmesa_handle,"OSMesaGetIntegerv");
		fn.OSMesaGetDepthBuffer =
			(GLboolean (*)(GLint *,GLint *,GLint *,void **))
			SDL_LoadFunction(libosmesa_handle,"OSMesaGetDepthBuffer");
		fn.OSMesaGetColorBuffer =
			(GLboolean (*)(GLint *,GLint *,GLint *,void **))
			SDL_LoadFunction(libosmesa_handle,"OSMesaGetColorBuffer");
		fn.OSMesaGetProcAddress =
			(void *(*)(const char *))
			SDL_LoadFunction(libosmesa_handle,"OSMesaGetProcAddress");
}

void OSMesaDriver::SelectContext(Uint32 ctx)
{
	void *draw_buffer;

	if ((ctx>MAX_OSMESA_CONTEXTS) || (ctx==0)) {
		D(bug("nfosmesa: SelectContext: %d out of bounds",ctx));
		return;
	}
	if ((Uint32)cur_context != ctx) {
		draw_buffer = contexts[ctx].dst_buffer;
		if (contexts[ctx].src_buffer) {
			draw_buffer = contexts[ctx].src_buffer;
		}
		fn.OSMesaMakeCurrent(contexts[ctx].ctx, draw_buffer, contexts[ctx].type, contexts[ctx].width, contexts[ctx].height);
		D(bug("nfosmesa: SelectContext: %d is current",ctx));
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

	D(bug("nfosmesa: OSMesaCreateContextExt(%d,%d,%d,%d,0x%08x)",format,depthBits,stencilBits,accumBits,sharelist));

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
	osmesa_format = format;
	contexts[j].conversion = SDL_FALSE;

#if SDL_BYTEORDER == SDL_LIL_ENDIAN
	if (format == OSMESA_RGB_565) {
		osmesa_format = OSMESA_RGB_565;
		contexts[j].conversion = SDL_TRUE;
	}
#endif

	if (!libgl_needed && (format!=OSMESA_COLOR_INDEX)) {
		/* We are using libOSMesa[16,32] */
		osmesa_format = OSMESA_ARGB;
		contexts[j].conversion = SDL_TRUE;
	}

	contexts[j].enabled_arrays=0;
	memset((void *)&(contexts[j].vertex),0,sizeof(vertexarray_t));
	memset((void *)&(contexts[j].normal),0,sizeof(vertexarray_t));
	memset((void *)&(contexts[j].texcoord),0,sizeof(vertexarray_t));
	memset((void *)&(contexts[j].color),0,sizeof(vertexarray_t));
	memset((void *)&(contexts[j].index),0,sizeof(vertexarray_t));
	memset((void *)&(contexts[j].edgeflag),0,sizeof(vertexarray_t));

	D(bug("nfosmesa: format:%d -> %d, conversion: %s", osmesa_format, format, contexts[j].conversion ? "true" : "false"));
	D(bug("nfosmesa: depth=%d, stencil=%d, accum=%d", depthBits, stencilBits, accumBits));
	contexts[j].ctx=fn.OSMesaCreateContextExt(osmesa_format,depthBits,stencilBits,accumBits,share_ctx);
	if (contexts[j].ctx==NULL) {
		D(bug("nfosmesa: Can not create context"));
		return 0;
	}
	contexts[j].srcformat = osmesa_format;
	contexts[j].dstformat = format;
	contexts[j].src_buffer = contexts[j].dst_buffer = NULL;
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
	if (contexts[ctx].src_buffer) {
		free(contexts[ctx].src_buffer);
	}
	contexts[ctx].ctx=NULL;
/*
	if (num_contexts==0) {
		CloseLibrary();
	}
*/
}

GLboolean OSMesaDriver::OSMesaMakeCurrent( Uint32 ctx, void *buffer, GLenum type, GLsizei width, GLsizei height )
{
	void *draw_buffer;

	D(bug("nfosmesa: OSMesaMakeCurrent(%d,0x%08x,%d,%d,%d)",ctx,buffer,type,width,height));
	if (ctx>MAX_OSMESA_CONTEXTS) {
		return GL_FALSE;
	}
	
	if (!contexts[ctx].ctx) {
		return GL_FALSE;
	}

	cur_context = ctx;
	contexts[ctx].dst_buffer = draw_buffer = buffer;
	contexts[ctx].type = type;
	if (!libgl_needed) {
		if (contexts[ctx].src_buffer) {
			free(contexts[ctx].src_buffer);
		}
		contexts[ctx].src_buffer = draw_buffer = malloc(width * height * 4 * (bx_options.osmesa.channel_size>>3));
		D(bug("nfosmesa: Allocated shadow buffer for channel reduction"));
		switch(bx_options.osmesa.channel_size) {
			case 16:
				contexts[ctx].type = GL_UNSIGNED_SHORT;
				break;
			case 32:
				contexts[ctx].type = GL_FLOAT;
				break;
		}
	}
	contexts[ctx].width = width;
	contexts[ctx].height = height;
	return fn.OSMesaMakeCurrent(contexts[ctx].ctx, draw_buffer, contexts[ctx].type, width, height);
}

Uint32 OSMesaDriver::OSMesaGetCurrentContext( void )
{
	D(bug("nfosmesa: OSMesaGetCurrentContext"));
	return cur_context;
}

void OSMesaDriver::OSMesaPixelStore(GLint pname, GLint value )
{
	D(bug("nfosmesa: OSMesaPixelStore"));
	SelectContext(cur_context);
	fn.OSMesaPixelStore(pname, value);
}

void OSMesaDriver::OSMesaGetIntegerv(GLint pname, GLint *value )
{
	GLint tmp;

	D(bug("nfosmesa: OSMesaGetIntegerv"));
	SelectContext(cur_context);
	fn.OSMesaGetIntegerv(pname, &tmp);
	*value = SDL_SwapBE32(tmp);
}

GLboolean OSMesaDriver::OSMesaGetDepthBuffer(Uint32 c, GLint *width, GLint *height, GLint *bytesPerValue, void **buffer )
{
	D(bug("nfosmesa: OSMesaGetDepthBuffer"));
	SelectContext(c);
	*width = 0;
	*height = 0;
	*bytesPerValue = 0;
	*buffer = NULL;	/* Can not return pointer in host memory */
	return GL_FALSE;
}

GLboolean OSMesaDriver::OSMesaGetColorBuffer(Uint32 c, GLint *width, GLint *height, GLint *format, void **buffer )
{
	D(bug("nfosmesa: OSMesaGetColorBuffer"));
	SelectContext(c);
	*width = 0;
	*height = 0;
	*format = 0;
	*buffer = NULL;	/* Can not return pointer in host memory */
	return GL_FALSE;
}

void *OSMesaDriver::OSMesaGetProcAddress( const char */*funcName*/ )
{
	D(bug("nfosmesa: OSMesaGetProcAddress"));
	return NULL;
}

Uint32 OSMesaDriver::LenglGetString(Uint32 ctx, GLenum name)
{
	D(bug("nfosmesa: LenglGetString"));
	SelectContext(ctx);
#if NFOSMESA_GLEXT
	return strlen((const char *)nfglGetString(name));
#else
	switch(name) {
		case GL_EXTENSIONS:
		case GL_VERSION:
			return 4;
		default:
			return strlen((const char *)nfglGetString(name));
	}
#endif
}

void OSMesaDriver::PutglGetString(Uint32 ctx, GLenum name, GLubyte *buffer)
{
	D(bug("nfosmesa: PutglGetString"));
	SelectContext(ctx);
#if NFOSMESA_GLEXT
	strcpy((char *)buffer,(const char *)nfglGetString(name));
#else
	switch(name) {
		case GL_EXTENSIONS:
			strcpy((char *)buffer, "");
			break;
		case GL_VERSION:
			strcpy((char *)buffer, "1.0");
			break;
		default:
			strcpy((char *)buffer,(const char *)nfglGetString(name));
			break;
	}
#endif
}

GLdouble OSMesaDriver::Atari2HostDouble(Uint32 high, Uint32 low)
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

void OSMesaDriver::Atari2HostDoublePtr(Uint32 size, Uint32 *src, GLdouble *dest)
{
	Uint32 i;
	
	for (i=0;i<size;i++) {
		dest[i]=Atari2HostDouble(SDL_SwapBE32(src[i<<1]),SDL_SwapBE32(src[(i<<1)+1]));
	}
}

static GLfloat Atari2HostFloat(Uint32 value)
{
	union {
		GLfloat f;
		Uint32 i;
	} u;
	u.i = value;
	return u.f;
}

void OSMesaDriver::Atari2HostFloatPtr(Uint32 size, Uint32 *src, GLfloat *dest)
{
	for (Uint32 i=0;i<size;i++) {
		dest[i]=Atari2HostFloat(SDL_SwapBE32(src[i]));
	}
}

void OSMesaDriver::Atari2HostIntPtr(Uint32 size, Uint32 *src, GLint *dest)
{
	Uint32 i,*tmp;
	
	tmp = (Uint32 *)dest;
	
	for (i=0;i<size;i++) {
		tmp[i]=SDL_SwapBE32(src[i]);
	}
}

void OSMesaDriver::Atari2HostShortPtr(Uint32 size, Uint16 *src, GLshort *dest)
{
	Uint32 i;
	Uint16 *tmp;
	
	tmp = (Uint16 *)dest;
	
	for (i=0;i<size;i++) {
		tmp[i]=SDL_SwapBE16(src[i]);
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
				srcline = (Uint16 *)contexts[ctx].dst_buffer;
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
		case OSMESA_ARGB:	/* 16 or 32 bits per channel */
			switch (bx_options.osmesa.channel_size) {
				case 16:
					ConvertContext16(ctx);
					break;
				case 32:
					ConvertContext32(ctx);
					break;
				default:
					D(bug("nfosmesa: ConvertContext: Unsupported channel size"));
					break;
			}
			break;
	}
}

void OSMesaDriver::ConvertContext16(Uint32 ctx)
{
	int x,y, r,g,b,a, srcpitch, dstpitch, color;
	Uint16 *srcline, *srccol;

	srcline = (Uint16 *) contexts[ctx].src_buffer;
	srcpitch = contexts[ctx].width * 4;

	switch(contexts[ctx].dstformat) {
		case OSMESA_RGB_565:
			{
				Uint16 *dstline, *dstcol;

				dstline = (Uint16 *) contexts[ctx].dst_buffer;
				dstpitch = contexts[ctx].width;
				for (y=0;y<contexts[ctx].height;y++) {
					srccol = srcline;
					dstcol = dstline;
					for (x=0;x<contexts[ctx].width;x++) {
						srccol++; /* Skip alpha */
						r = ((*srccol++)>>11) & 31;
						g = ((*srccol++)>>10) & 63;
						b = ((*srccol++)>>11) & 31;

						color = (r<<11)|(g<<5)|b;
						*dstcol++ = SDL_SwapBE16(color);
					}
					srcline += srcpitch;
					dstline += dstpitch;
				}
			}
			break;
		case OSMESA_RGB:
			{
				Uint8 *dstline, *dstcol;

				dstline = (Uint8 *) contexts[ctx].dst_buffer;
				dstpitch = contexts[ctx].width * 3;
				for (y=0;y<contexts[ctx].height;y++) {
					srccol = srcline;
					dstcol = dstline;
					for (x=0;x<contexts[ctx].width;x++) {
						srccol++; /* Skip alpha */
						r = ((*srccol++)>>8) & 255;
						g = ((*srccol++)>>8) & 255;
						b = ((*srccol++)>>8) & 255;

						*dstcol++ = r;
						*dstcol++ = g;
						*dstcol++ = b;
					}
					srcline += srcpitch;
					dstline += dstpitch;
				}
			}
			break;
		case OSMESA_BGR:
			{
				Uint8 *dstline, *dstcol;

				dstline = (Uint8 *) contexts[ctx].dst_buffer;
				dstpitch = contexts[ctx].width * 3;
				for (y=0;y<contexts[ctx].height;y++) {
					srccol = srcline;
					dstcol = dstline;
					for (x=0;x<contexts[ctx].width;x++) {
						srccol++; /* Skip alpha */
						r = ((*srccol++)>>8) & 255;
						g = ((*srccol++)>>8) & 255;
						b = ((*srccol++)>>8) & 255;

						*dstcol++ = b;
						*dstcol++ = g;
						*dstcol++ = r;
					}
					srcline += srcpitch;
					dstline += dstpitch;
				}
			}
			break;
		case OSMESA_BGRA:
			{
				Uint32 *dstline, *dstcol;

				dstline = (Uint32 *) contexts[ctx].dst_buffer;
				dstpitch = contexts[ctx].width;
				for (y=0;y<contexts[ctx].height;y++) {
					srccol = srcline;
					dstcol = dstline;
					for (x=0;x<contexts[ctx].width;x++) {
						a = ((*srccol++)>>8) & 255;
						r = ((*srccol++)>>8) & 255;
						g = ((*srccol++)>>8) & 255;
						b = ((*srccol++)>>8) & 255;

						color = (b<<24)|(g<<16)|(r<<8)|a;
						*dstcol++ = SDL_SwapBE32(color);
					}
					srcline += srcpitch;
					dstline += dstpitch;
				}
			}
			break;
		case OSMESA_ARGB:
			{
				Uint32 *dstline, *dstcol;

				dstline = (Uint32 *) contexts[ctx].dst_buffer;
				dstpitch = contexts[ctx].width;
				for (y=0;y<contexts[ctx].height;y++) {
					srccol = srcline;
					dstcol = dstline;
					for (x=0;x<contexts[ctx].width;x++) {
						a = ((*srccol++)>>8) & 255;
						r = ((*srccol++)>>8) & 255;
						g = ((*srccol++)>>8) & 255;
						b = ((*srccol++)>>8) & 255;

						color = (a<<24)|(r<<16)|(g<<8)|b;
						*dstcol++ = SDL_SwapBE32(color);
					}
					srcline += srcpitch;
					dstline += dstpitch;
				}
			}
			break;
		case OSMESA_RGBA:
			{
				Uint32 *dstline, *dstcol;

				dstline = (Uint32 *) contexts[ctx].dst_buffer;
				dstpitch = contexts[ctx].width;
				for (y=0;y<contexts[ctx].height;y++) {
					srccol = srcline;
					dstcol = dstline;
					for (x=0;x<contexts[ctx].width;x++) {
						a = ((*srccol++)>>8) & 255;
						r = ((*srccol++)>>8) & 255;
						g = ((*srccol++)>>8) & 255;
						b = ((*srccol++)>>8) & 255;

						color = (r<<24)|(g<<16)|(b<<8)|a;
						*dstcol++ = SDL_SwapBE32(color);
					}
					srcline += srcpitch;
					dstline += dstpitch;
				}
			}
			break;
	}
}

void OSMesaDriver::ConvertContext32(Uint32 ctx)
{
#define FLOAT_TO_INT(source, value, maximum) \
	{	\
		value = (int) (source * maximum ## .0); \
		if (value>maximum) value=maximum; \
		if (value<0) value=0; \
	}

	int x,y, r,g,b,a, srcpitch, dstpitch, color;
	float *srcline, *srccol;

	srcline = (float *) contexts[ctx].src_buffer;
	srcpitch = contexts[ctx].width * 4;

	switch(contexts[ctx].dstformat) {
		case OSMESA_RGB_565:
			{
				Uint16 *dstline, *dstcol;

				dstline = (Uint16 *) contexts[ctx].dst_buffer;
				dstpitch = contexts[ctx].width;
				for (y=0;y<contexts[ctx].height;y++) {
					srccol = srcline;
					dstcol = dstline;
					for (x=0;x<contexts[ctx].width;x++) {
						srccol++; /* Skip alpha */
						FLOAT_TO_INT(*srccol++, r, 31);
						FLOAT_TO_INT(*srccol++, g, 63);
						FLOAT_TO_INT(*srccol++, b, 31);

						color = (r<<11)|(g<<5)|b;
						*dstcol++ = SDL_SwapBE16(color);
					}
					srcline += srcpitch;
					dstline += dstpitch;
				}
			}
			break;
		case OSMESA_RGB:
			{
				Uint8 *dstline, *dstcol;

				dstline = (Uint8 *) contexts[ctx].dst_buffer;
				dstpitch = contexts[ctx].width * 3;
				for (y=0;y<contexts[ctx].height;y++) {
					srccol = srcline;
					dstcol = dstline;
					for (x=0;x<contexts[ctx].width;x++) {
						srccol++; /* Skip alpha */
						FLOAT_TO_INT(*srccol++, r, 255);
						FLOAT_TO_INT(*srccol++, g, 255);
						FLOAT_TO_INT(*srccol++, b, 255);

						*dstcol++ = r;
						*dstcol++ = g;
						*dstcol++ = b;
					}
					srcline += srcpitch;
					dstline += dstpitch;
				}
			}
			break;
		case OSMESA_BGR:
			{
				Uint8 *dstline, *dstcol;

				dstline = (Uint8 *) contexts[ctx].dst_buffer;
				dstpitch = contexts[ctx].width * 3;
				for (y=0;y<contexts[ctx].height;y++) {
					srccol = srcline;
					dstcol = dstline;
					for (x=0;x<contexts[ctx].width;x++) {
						srccol++; /* Skip alpha */
						FLOAT_TO_INT(*srccol++, r, 255);
						FLOAT_TO_INT(*srccol++, g, 255);
						FLOAT_TO_INT(*srccol++, b, 255);

						*dstcol++ = b;
						*dstcol++ = g;
						*dstcol++ = r;
					}
					srcline += srcpitch;
					dstline += dstpitch;
				}
			}
			break;
		case OSMESA_BGRA:
			{
				Uint32 *dstline, *dstcol;

				dstline = (Uint32 *) contexts[ctx].dst_buffer;
				dstpitch = contexts[ctx].width;
				for (y=0;y<contexts[ctx].height;y++) {
					srccol = srcline;
					dstcol = dstline;
					for (x=0;x<contexts[ctx].width;x++) {
						FLOAT_TO_INT(*srccol++, a, 255);
						FLOAT_TO_INT(*srccol++, r, 255);
						FLOAT_TO_INT(*srccol++, g, 255);
						FLOAT_TO_INT(*srccol++, b, 255);

						color = (b<<24)|(g<<16)|(r<<8)|a;
						*dstcol++ = SDL_SwapBE32(color);
					}
					srcline += srcpitch;
					dstline += dstpitch;
				}
			}
			break;
		case OSMESA_ARGB:
			{
				Uint32 *dstline, *dstcol;

				dstline = (Uint32 *) contexts[ctx].dst_buffer;
				dstpitch = contexts[ctx].width;
				for (y=0;y<contexts[ctx].height;y++) {
					srccol = srcline;
					dstcol = dstline;
					for (x=0;x<contexts[ctx].width;x++) {
						FLOAT_TO_INT(*srccol++, a, 255);
						FLOAT_TO_INT(*srccol++, r, 255);
						FLOAT_TO_INT(*srccol++, g, 255);
						FLOAT_TO_INT(*srccol++, b, 255);

						color = (a<<24)|(r<<16)|(g<<8)|b;
						*dstcol++ = SDL_SwapBE32(color);
					}
					srcline += srcpitch;
					dstline += dstpitch;
				}
			}
			break;
		case OSMESA_RGBA:
			{
				Uint32 *dstline, *dstcol;

				dstline = (Uint32 *) contexts[ctx].dst_buffer;
				dstpitch = contexts[ctx].width;
				for (y=0;y<contexts[ctx].height;y++) {
					srccol = srcline;
					dstcol = dstline;
					for (x=0;x<contexts[ctx].width;x++) {
						FLOAT_TO_INT(*srccol++, a, 255);
						FLOAT_TO_INT(*srccol++, r, 255);
						FLOAT_TO_INT(*srccol++, g, 255);
						FLOAT_TO_INT(*srccol++, b, 255);

						color = (r<<24)|(g<<16)|(g<<8)|a;
						*dstcol++ = SDL_SwapBE32(color);
					}
					srcline += srcpitch;
					dstline += dstpitch;
				}
			}
			break;
	}
}

Uint32 OSMesaDriver::getStackedParameter(Uint32 n)
{
	return SDL_SwapBE32(ctx_ptr[n]);
}

float OSMesaDriver::getStackedFloat(Uint32 n)
{
	union { float f; Uint32 i; } u;

	u.i = SDL_SwapBE32(ctx_ptr[n]);
	return u.f;
}

#include "nfosmesa/call-gl.c"
#if NFOSMESA_GLEXT
# include "nfosmesa/call-glext.c"
#endif
