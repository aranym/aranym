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

#define __STDC_FORMAT_MACROS

#ifdef OS_darwin
#include <CoreFoundation/CoreFoundation.h>
#endif

#include "SDL_compat.h"
#include <SDL_loadso.h>
#include <SDL_endian.h>
#include <math.h>

#include "sysdeps.h"
#include "cpu_emulation.h"
#include "parameters.h"
#include "nfosmesa.h"
#include "../../atari/nfosmesa/nfosmesa_nfapi.h"
#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif
#ifndef PRId64
#  define PRId64 "lld"
#endif
#ifndef PRIu64
#  define PRIu64 "llu"
#endif

#define DEBUG 0
#include "debug.h"

/*--- Defines ---*/

#define EINVFN -32

#define M(m,row,col)  m[col*4+row]

#ifndef GL_VIEWPORT_BOUNDS_RANGE
#define GL_VIEWPORT_BOUNDS_RANGE          0x825D
#endif
#ifdef GL_DEPTH_BOUNDS_EXT
#define GL_DEPTH_BOUNDS_EXT               0x8891
#endif
#ifndef GL_POINT_DISTANCE_ATTENUATION
#define GL_POINT_DISTANCE_ATTENUATION_ARB 0x8129
#endif
#ifndef GL_CURRENT_RASTER_SECONDARY_COLOR
#define GL_CURRENT_RASTER_SECONDARY_COLOR 0x845F
#endif
#ifndef GL_RGBA_SIGNED_COMPONENTS_EXT
#define GL_RGBA_SIGNED_COMPONENTS_EXT     0x8C3C
#endif
#ifndef GL_NUM_COMPRESSED_TEXTURE_FORMATS
#define GL_NUM_COMPRESSED_TEXTURE_FORMATS 0x86A2
#endif
#ifndef GL_COMPRESSED_TEXTURE_FORMATS
#define GL_COMPRESSED_TEXTURE_FORMATS     0x86A3
#endif
#ifndef GL_TRANSPOSE_MODELVIEW_MATRIX
#define GL_TRANSPOSE_MODELVIEW_MATRIX     0x84E3
#endif
#ifndef GL_TRANSPOSE_PROJECTION_MATRIX
#define GL_TRANSPOSE_PROJECTION_MATRIX    0x84E4
#endif
#ifndef GL_TRANSPOSE_TEXTURE_MATRIX
#define GL_TRANSPOSE_TEXTURE_MATRIX       0x84E5
#endif
#ifndef GL_TRANSPOSE_COLOR_MATRIX
#define GL_TRANSPOSE_COLOR_MATRIX         0x84E6
#endif

/*--- Types ---*/

typedef struct {
#define GL_PROC(type, gl, name, export, upper, params, first, ret) type (APIENTRY *gl ## name) params ;
#define GLU_PROC(type, gl, name, export, upper, params, first, ret)
#define OSMESA_PROC(type, gl, name, export, upper, params, first, ret) type (APIENTRY *OSMesa ## name) params ;
#include "../../atari/nfosmesa/glfuncs.h"
} osmesa_funcs;

/*--- Variables ---*/

static osmesa_funcs fn;

/*--- Constructor/Destructor ---*/

OSMesaDriver::OSMesaDriver()
{
	D(bug("nfosmesa: OSMesaDriver()"));
	memset(contexts, 0, sizeof(contexts));
	num_contexts = 0;
	cur_context = 0;
	libgl_handle = libosmesa_handle = NULL;
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
	cur_context = 0;

	CloseLibrary();
}

/*--- Public functions ---*/

int32 OSMesaDriver::dispatch(uint32 fncode)
{
	int32 ret = 0;
	Uint32 *ctx_ptr;	/* Current parameter list */
	
	/* Read parameter on m68k stack */
#define getStackedParameter(n) SDL_SwapBE32(ctx_ptr[n])
#define getStackedParameter64(n) SDL_SwapBE64(*((Uint64 *)&ctx_ptr[n]))
#define getStackedFloat(n) Atari2HostFloat(getStackedParameter(n))
#define getStackedDouble(n) Atari2HostDouble(getStackedParameter(n), getStackedParameter(n + 1))
#define getStackedPointer(n) (ctx_ptr[n] ? Atari2HostAddr(getStackedParameter(n)) : NULL)

	if (fncode != NFOSMESA_OSMESAPOSTPROCESS)
	{
		/*
		 * OSMesaPostprocess() cannot be called after OSMesaMakeCurrent().
		 * FIXME: this will fail if ARAnyM already has a current context
		 * that was created by a different MiNT process
		 */
		SelectContext(getParameter(0));
	}
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
		case NFOSMESA_LENGLGETSTRINGI:
			ret = LenglGetStringi(getStackedParameter(0),getStackedParameter(1),getStackedParameter(1));
			break;
		case NFOSMESA_PUTGLGETSTRINGI:
			PutglGetStringi(getStackedParameter(0),getStackedParameter(1),getStackedParameter(2),(GLubyte *)Atari2HostAddr(getStackedParameter(3)));
			break;

		case NFOSMESA_OSMESACREATECONTEXT:
			ret = OSMesaCreateContext(getStackedParameter(0),getStackedParameter(1));
			break;
		case NFOSMESA_OSMESACREATECONTEXTEXT:
			ret = OSMesaCreateContextExt(getStackedParameter(0),getStackedParameter(1),getStackedParameter(2),getStackedParameter(3),getStackedParameter(4));
			break;
		case NFOSMESA_OSMESADESTROYCONTEXT:
			 OSMesaDestroyContext(getStackedParameter(0));
			break;
		case NFOSMESA_OSMESAMAKECURRENT:
			ret = OSMesaMakeCurrent(getStackedParameter(0),getStackedParameter(1),getStackedParameter(2),getStackedParameter(3),getStackedParameter(4));
			break;
		case NFOSMESA_OSMESAGETCURRENTCONTEXT:
			ret = OSMesaGetCurrentContext();
			break;
		case NFOSMESA_OSMESAPIXELSTORE:
			OSMesaPixelStore(getStackedParameter(0),getStackedParameter(1));
			break;
		case NFOSMESA_OSMESAGETINTEGERV:
			OSMesaGetIntegerv(getStackedParameter(0),(GLint *)Atari2HostAddr(getStackedParameter(1)));
			break;
		case NFOSMESA_OSMESAGETDEPTHBUFFER:
			ret = OSMesaGetDepthBuffer(getStackedParameter(0),(GLint *)Atari2HostAddr(getStackedParameter(1)),(GLint *)Atari2HostAddr(getStackedParameter(2)),(GLint *)Atari2HostAddr(getStackedParameter(3)),(void **)Atari2HostAddr(getStackedParameter(4)));
			break;
		case NFOSMESA_OSMESAGETCOLORBUFFER:
			ret = OSMesaGetColorBuffer(getStackedParameter(0),(GLint *)Atari2HostAddr(getStackedParameter(1)),(GLint *)Atari2HostAddr(getStackedParameter(2)),(GLint *)Atari2HostAddr(getStackedParameter(3)),(void **)Atari2HostAddr(getStackedParameter(4)));
			break;
		case NFOSMESA_OSMESAGETPROCADDRESS:
			/* FIXME: Native side do not need this */
			ret = (int32) (0 != OSMesaGetProcAddress((const char *)Atari2HostAddr(getStackedParameter(0))));
			break;
		case NFOSMESA_OSMESACOLORCLAMP:
#if OSMESA_VERSION >= OSMESA_VERSION_NUMBER(6, 4, 2)
			OSMesaColorClamp(getStackedParameter(0));
#else
			ret = EINVFN;
#endif
			break;
		case NFOSMESA_OSMESAPOSTPROCESS:
#if OSMESA_VERSION >= OSMESA_VERSION_NUMBER(10, 0, 0)
			OSMesaPostprocess(getStackedParameter(0), (const char *)Atari2HostAddr(getStackedParameter(1)), getStackedParameter(2));
#else
			ret = EINVFN;
#endif
			break;

		/*
		 * maybe FIXME: functions below usually need a current context,
		 * which is not checked here.
		 * Also, if some dumb program tries to call any of these
		 * without ever creating a context, OpenLibrary() will not have been called yet,
		 * and crash ARAnyM because the fn ptrs have not been initialized,
		 * but that would require to call OpenLibrary for every function call.
		 */
		case NFOSMESA_GLULOOKATF:
			nfgluLookAtf(
				getStackedFloat(0) /* GLfloat eyeX */,
				getStackedFloat(1) /* GLfloat eyeY */,
				getStackedFloat(2) /* GLfloat eyeZ */,
				getStackedFloat(3) /* GLfloat centerX */,
				getStackedFloat(4) /* GLfloat centerY */,
				getStackedFloat(5) /* GLfloat centerZ */,
				getStackedFloat(6) /* GLfloat upX */,
				getStackedFloat(7) /* GLfloat upY */,
				getStackedFloat(8) /* GLfloat upZ */);
			break;
		
		case NFOSMESA_GLFRUSTUMF:
			nfglFrustumf(
				getStackedFloat(0) /* GLfloat left */,
				getStackedFloat(1) /* GLfloat right */,
				getStackedFloat(2) /* GLfloat bottom */,
				getStackedFloat(3) /* GLfloat top */,
				getStackedFloat(4) /* GLfloat near_val */,
				getStackedFloat(5) /* GLfloat far_val */);
			break;
		
		case NFOSMESA_GLORTHOF:
			nfglOrthof(
				getStackedFloat(0) /* GLfloat left */,
				getStackedFloat(1) /* GLfloat right */,
				getStackedFloat(2) /* GLfloat bottom */,
				getStackedFloat(3) /* GLfloat top */,
				getStackedFloat(4) /* GLfloat near_val */,
				getStackedFloat(5) /* GLfloat far_val */);
			break;
		
		case NFOSMESA_TINYGLSWAPBUFFER:
			nftinyglswapbuffer(getStackedParameter(0));
			break;
		
#include "nfosmesa/dispatch-gl.c"

		case NFOSMESA_ENOSYS:
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
	bool libgl_needed = false;
	
	D(bug("nfosmesa: OpenLibrary()"));

	/* Check if channel size is correct */
	switch(bx_options.osmesa.channel_size) {
		case 16:
		case 32:
		case 8:
		case 0:
			break;
		default:
			D(bug("nfosmesa: bogus Channel size: %d", bx_options.osmesa.channel_size));
			bx_options.osmesa.channel_size = 0;
			break;
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
			panicbug("nfosmesa: %s: %s", bx_options.osmesa.libosmesa, SDL_GetError());
			return -1;
		}
		InitPointersOSMesa(libosmesa_handle);
		InitPointersGL(libosmesa_handle);
		D(bug("nfosmesa: OpenLibrary(): libOSMesa loaded"));
		if (GL_ISNOP(fn.glBegin)) {
			libgl_needed = true;
			D(bug("nfosmesa: Channel size: %d -> libGL separated from libOSMesa", bx_options.osmesa.channel_size));
		} else {
			D(bug("nfosmesa: Channel size: %d -> libGL included in libOSMesa", bx_options.osmesa.channel_size));
		}
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
			panicbug("nfosmesa: %s: %s", bx_options.osmesa.libgl, SDL_GetError());
			return -1;
		}
		InitPointersGL(libgl_handle);
		D(bug("nfosmesa: OpenLibrary(): libGL loaded"));
	}

	return 0;
}

int OSMesaDriver::CloseLibrary(void)
{
	D(bug("nfosmesa: CloseLibrary()"));

	if (libosmesa_handle) {
		SDL_UnloadObject(libosmesa_handle);
		libosmesa_handle=NULL;
	}

/* nullify OSMesa functions */
#define GL_PROC(type, gl, name, export, upper, params, first, ret) 
#define OSMESA_PROC(type, gl, name, export, upper, params, first, ret) fn.OSMesa ## name = (type (APIENTRY *) params) 0;
#include "../../atari/nfosmesa/glfuncs.h"

	D(bug("nfosmesa: CloseLibrary(): libOSMesa unloaded"));

	if (libgl_handle) {
		SDL_UnloadObject(libgl_handle);
		libgl_handle=NULL;
	}
	
/* nullify GL functions */
#define GL_PROC(type, gl, name, export, upper, params, first, ret) fn.gl ## name = (type (APIENTRY *) params) 0;
#define GLU_PROC(type, gl, name, export, upper, params, first, ret) 
#define OSMESA_PROC(type, gl, name, export, upper, params, first, ret)
#include "../../atari/nfosmesa/glfuncs.h"

	D(bug("nfosmesa: CloseLibrary(): libGL unloaded"));

	return 0;
}

void *APIENTRY OSMesaDriver::glNop(void)
{
	return 0;
}

void OSMesaDriver::InitPointersGL(void *handle)
{
	D(bug("nfosmesa: InitPointersGL()"));

#define GL_PROC(type, gl, name, export, upper, params, first, ret) \
	fn.gl ## name = (type (APIENTRY *) params) SDL_LoadFunction(handle, "gl" #name); \
	if (fn.gl ## name == 0) fn.gl ## name = (type (APIENTRY *) params)glNop;
#define GLU_PROC(type, gl, name, export, upper, params, first, ret)
#define OSMESA_PROC(type, gl, name, export, upper, params, first, ret)
#include "../../atari/nfosmesa/glfuncs.h"
}

void OSMesaDriver::InitPointersOSMesa(void *handle)
{
	D(bug("nfosmesa: InitPointersOSMesa()"));

#define GL_PROC(type, gl, name, export, upper, params, first, ret) 
#define OSMESA_PROC(type, gl, name, export, upper, params, first, ret) \
	fn.OSMesa ## name = (type (APIENTRY *) params) SDL_LoadFunction(handle, "OSMesa" #name); \
	if (fn.OSMesa ## name == 0) fn.OSMesa ## name = (type (APIENTRY *) params)glNop;
#include "../../atari/nfosmesa/glfuncs.h"
}

bool OSMesaDriver::SelectContext(Uint32 ctx)
{
	void *draw_buffer;
	bool ret = true;
	
	if (ctx>MAX_OSMESA_CONTEXTS) {
		D(bug("nfosmesa: SelectContext: %d out of bounds",ctx));
		return false;
	}
	if (!fn.OSMesaMakeCurrent)
	{
		/* can happen if we did not load the library yet, e.g because no context was created yet */
		return false;
	}
	if (cur_context != ctx) {
		if (ctx != 0)
		{
			draw_buffer = contexts[ctx].dst_buffer;
			if (contexts[ctx].src_buffer) {
				draw_buffer = contexts[ctx].src_buffer;
			}
			ret = GL_TRUE == fn.OSMesaMakeCurrent(contexts[ctx].ctx, draw_buffer, contexts[ctx].type, contexts[ctx].width, contexts[ctx].height);
		} else
		{
			ret = GL_TRUE == fn.OSMesaMakeCurrent(NULL, NULL, 0, 0, 0);
		}
		if (ret)
		{
			D(bug("nfosmesa: SelectContext: %d is current",ctx));
		} else
		{
			D(bug("nfosmesa: SelectContext: %d failed",ctx));
		}
		cur_context = ctx;
	}
	return ret;
}

Uint32 OSMesaDriver::OSMesaCreateContext( GLenum format, Uint32 sharelist )
{
	D(bug("nfosmesa: OSMesaCreateContext(0x%x, 0x%x)", format, sharelist));
	return OSMesaCreateContextExt(format, 16, 8, (format == OSMESA_COLOR_INDEX) ? 0 : 16, sharelist);
}

Uint32 OSMesaDriver::OSMesaCreateContextExt( GLenum format, GLint depthBits, GLint stencilBits, GLint accumBits, Uint32 sharelist)
{
	int i,j;
	OSMesaContext share_ctx;
	GLenum osmesa_format;

	D(bug("nfosmesa: OSMesaCreateContextExt(0x%x,%d,%d,%d,0x%08x)",format,depthBits,stencilBits,accumBits,sharelist));

	/* TODO: shared contexts */
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

	if (bx_options.osmesa.channel_size > 8 && (format!=OSMESA_COLOR_INDEX)) {
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

	D(bug("nfosmesa: format:0x%x -> 0x%x, conversion: %s", osmesa_format, format, contexts[j].conversion ? "true" : "false"));
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
	D(bug("nfosmesa: OSMesaDestroyContext(%u)", ctx));
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
	if (ctx == cur_context)
		cur_context = 0;
/*
	if (num_contexts==0) {
		CloseLibrary();
	}
*/
}

GLboolean OSMesaDriver::OSMesaMakeCurrent( Uint32 ctx, memptr buffer, GLenum type, GLsizei width, GLsizei height )
{
	void *draw_buffer;
	GLboolean ret;
	
	D(bug("nfosmesa: OSMesaMakeCurrent(%u,$%08x,%d,%d,%d)",ctx,buffer,type,width,height));
	if (ctx>MAX_OSMESA_CONTEXTS) {
		return GL_FALSE;
	}
	
	if (ctx != 0 && (!fn.OSMesaMakeCurrent || !contexts[ctx].ctx)) {
		return GL_FALSE;
	}

	if (ctx != 0)
	{
		contexts[ctx].dst_buffer = draw_buffer = Atari2HostAddr(buffer);
		contexts[ctx].type = type;
		if (bx_options.osmesa.channel_size > 8) {
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
		} else {
			contexts[ctx].type = GL_UNSIGNED_BYTE;
		}
		contexts[ctx].width = width;
		contexts[ctx].height = height;
		ret = fn.OSMesaMakeCurrent(contexts[ctx].ctx, draw_buffer, contexts[ctx].type, width, height);
	} else
	{
		if (fn.OSMesaMakeCurrent)
			ret = fn.OSMesaMakeCurrent(NULL, NULL, 0, 0, 0);
		else
			ret = GL_TRUE;
	}
	if (ret)
	{
		cur_context = ctx;
		D(bug("nfosmesa: MakeCurrent: %d is current",ctx));
	} else
	{
		D(bug("nfosmesa: MakeCurrent: %d failed",ctx));
	}
	return ret;
}

Uint32 OSMesaDriver::OSMesaGetCurrentContext( void )
{
	D(bug("nfosmesa: OSMesaGetCurrentContext() -> %u", cur_context));
	return cur_context;
}

void OSMesaDriver::OSMesaPixelStore(GLint pname, GLint value )
{
	D(bug("nfosmesa: OSMesaPixelStore(0x%x, %d)", pname, value));
	if (SelectContext(cur_context))
		fn.OSMesaPixelStore(pname, value);
}

void OSMesaDriver::OSMesaGetIntegerv(GLint pname, GLint *value )
{
	GLint tmp = 0;

	D(bug("nfosmesa: OSMesaGetIntegerv(0x%x)", pname));
	if (SelectContext(cur_context))
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
	D(bug("nfosmesa: OSMesaGetColorBuffer(%u)", c));
	SelectContext(c);
	*width = 0;
	*height = 0;
	*format = 0;
	*buffer = NULL;	/* Can not return pointer in host memory */
	return GL_FALSE;
}

void *OSMesaDriver::OSMesaGetProcAddress( const char *funcName )
{
	D(bug("nfosmesa: OSMesaGetProcAddress(%s)", funcName));
	OpenLibrary();
	return (void *)fn.OSMesaGetProcAddress(funcName);
}

void OSMesaDriver::OSMesaColorClamp(GLboolean enable)
{
	D(bug("nfosmesa: OSMesaColorClamp(%d)", enable));
	OpenLibrary();
	if (!SelectContext(cur_context))
		return;
	if (fn.OSMesaColorClamp)
		fn.OSMesaColorClamp(enable);
	else
		bug("nfosmesa: OSMesaColorClamp: no such function");
}

void OSMesaDriver::OSMesaPostprocess(Uint32 ctx, const char *filter, GLuint enable_value)
{
	D(bug("nfosmesa: OSMesaPostprocess(%u, %s, %d)", ctx, filter, enable_value));
	if (ctx>MAX_OSMESA_CONTEXTS || ctx == 0 || !contexts[ctx].ctx)
		return;
	/* no SelectContext() here; OSMesaPostprocess must be called without having a current context */
	if (fn.OSMesaPostprocess)
		fn.OSMesaPostprocess(contexts[ctx].ctx, filter, enable_value);
	else
		bug("nfosmesa: OSMesaPostprocess: no such function");
}

Uint32 OSMesaDriver::LenglGetString(Uint32 ctx, GLenum name)
{
	D(bug("nfosmesa: LenglGetString(%u, 0x%x)", ctx, name));
	OpenLibrary();
	SelectContext(ctx);
	const char *s = (const char *)fn.glGetString(name);
	if (s == NULL) return 0;
	return strlen(s);
}

void OSMesaDriver::PutglGetString(Uint32 ctx, GLenum name, GLubyte *buffer)
{
	D(bug("nfosmesa: PutglGetString(%u, 0x%x, %p)", ctx, name, buffer));
	OpenLibrary();
	SelectContext(ctx);
	const char *s = (const char *)fn.glGetString(name);
	strcpy((char *)buffer, s ? s : "");
}

Uint32 OSMesaDriver::LenglGetStringi(Uint32 ctx, GLenum name, GLuint index)
{
	D(bug("nfosmesa: LenglGetStringi(%u, 0x%x, %u)", ctx, name, index));
	OpenLibrary();
	SelectContext(ctx);
	const char *s = (const char *)fn.glGetStringi(name, index);
	if (s == NULL) return (Uint32)-1;
	return strlen(s);
}

void OSMesaDriver::PutglGetStringi(Uint32 ctx, GLenum name, GLuint index, GLubyte *buffer)
{
	D(bug("nfosmesa: PutglGetStringi(%u, 0x%x, %d, %p)", ctx, name, index, buffer));
	OpenLibrary();
	SelectContext(ctx);
	const char *s = (const char *)fn.glGetStringi(name, index);
	strcpy((char *)buffer, s ? s : "");
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


#if SDL_BYTEORDER == SDL_LIL_ENDIAN

void *OSMesaDriver::convertPixels(GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels)
{
	GLsizei size, count;
	GLubyte *ptr;
	void *result;
	
	switch (type)
	{
	case GL_UNSIGNED_BYTE:
	case GL_BYTE:
	case GL_UNSIGNED_BYTE_3_3_2:
	case GL_UNSIGNED_BYTE_2_3_3_REV:
	case 1:
		return (void *)pixels;
	case GL_UNSIGNED_SHORT:
	case GL_SHORT:
	case GL_UNSIGNED_SHORT_5_6_5:
	case GL_UNSIGNED_SHORT_5_6_5_REV:
	case GL_UNSIGNED_SHORT_4_4_4_4:
	case GL_UNSIGNED_SHORT_4_4_4_4_REV:
	case GL_UNSIGNED_SHORT_5_5_5_1:
	case GL_UNSIGNED_SHORT_1_5_5_5_REV:
	case 2:
		size = sizeof(GLushort);
		break;
	case GL_UNSIGNED_INT:
	case GL_INT:
	case GL_UNSIGNED_INT_8_8_8_8:
	case GL_UNSIGNED_INT_8_8_8_8_REV:
	case GL_UNSIGNED_INT_10_10_10_2:
	case GL_UNSIGNED_INT_2_10_10_10_REV:
	case 4:
		size = sizeof(GLuint);
		break;
	case GL_FLOAT:
		size = sizeof(GLfloat);
		break;
	default:
		glSetError(GL_INVALID_ENUM);
		return NULL;
	}
	switch (format)
	{
	case GL_RED:
	case GL_GREEN:
	case GL_BLUE:
	case GL_ALPHA:
	case GL_LUMINANCE:
	case 1:
		count = 1;
		break;
	case GL_LUMINANCE_ALPHA:
	case 2:
		count = 2;
		break;
	case GL_RGB:
	case GL_BGR:
	case 3:
		count = 3;
		break;
	case GL_RGBA:
	case GL_BGRA:
	case 4:
		count = 4;
		break;
	default:
		glSetError(GL_INVALID_ENUM);
		return NULL;
	}
	
	count *= width * height;
	result = malloc(size * count);
	if (result == NULL)
		return result;
	ptr = (GLubyte *)result;
	if (type == GL_FLOAT)
		Atari2HostFloatArray(count, (const Uint32 *)pixels, (GLfloat *)ptr);
	else if (size == 2)
		Atari2HostShortArray(count, (const Uint16 *)pixels, (GLushort *)ptr);
	else /* if (size == 4) */
		Atari2HostIntArray(count, (const Uint32 *)pixels, (GLuint *)ptr);
	return result;
}

void OSMesaDriver::nfglArrayElementHelper(GLint i)
{
	int stride;
	const GLubyte *ptr;

	if(contexts[cur_context].enabled_arrays & NFOSMESA_NORMAL_ARRAY) {
		stride=contexts[cur_context].normal.stride;
		ptr=(const GLubyte *)contexts[cur_context].normal.pointer;
		if(stride==0) {
			stride = contexts[cur_context].normal.size;
			switch(contexts[cur_context].normal.type) {
				case GL_SHORT:
					stride *= sizeof(GLshort);
					break;
				case GL_INT:
					stride *= sizeof(GLint);
					break;
				case GL_FLOAT:
					stride *= ATARI_SIZEOF_FLOAT;
					break;
				case GL_DOUBLE:
					stride *= ATARI_SIZEOF_DOUBLE;
					break;
			}
		}
		ptr += i*stride;
		switch(contexts[cur_context].normal.type) {
			case GL_BYTE:
				nfglNormal3bv((const GLbyte *)ptr);
				break;
			case GL_SHORT:
				nfglNormal3sv((const GLshort *)ptr);
				break;
			case GL_INT:
				nfglNormal3iv((const GLint *)ptr);
				break;
			case GL_FLOAT:
				nfglNormal3fv((const GLfloat *)ptr);
				break;
			case GL_DOUBLE:
				nfglNormal3dv((const GLdouble *)ptr);
				break;
		}
	}

	if(contexts[cur_context].enabled_arrays & NFOSMESA_INDEX_ARRAY) {
		stride=contexts[cur_context].index.stride;
		ptr=(const GLubyte *)contexts[cur_context].index.pointer;
		if(stride==0) {
			stride = contexts[cur_context].index.size;
			switch(contexts[cur_context].index.type) {
				case GL_SHORT:
					stride *= sizeof(GLshort);
					break;
				case GL_INT:
					stride *= sizeof(GLint);
					break;
				case GL_FLOAT:
					stride *= ATARI_SIZEOF_FLOAT;
					break;
				case GL_DOUBLE:
					stride *= ATARI_SIZEOF_DOUBLE;
					break;
			}
		}
		ptr += i*stride;
		switch(contexts[cur_context].index.type) {
			case GL_UNSIGNED_BYTE:
				nfglIndexubv((const GLubyte *)ptr);
				break;
			case GL_SHORT:
				nfglIndexsv((const GLshort *)ptr);
				break;
			case GL_INT:
				nfglIndexiv((const GLint *)ptr);
				break;
			case GL_FLOAT:
				nfglIndexfv((const GLfloat *)ptr);
				break;
			case GL_DOUBLE:
				nfglIndexdv((const GLdouble *)ptr);
				break;
		}
	}

	if(contexts[cur_context].enabled_arrays & NFOSMESA_COLOR_ARRAY) {
		stride=contexts[cur_context].color.stride;
		ptr=(const GLubyte *)contexts[cur_context].color.pointer;
		if(stride==0) {
			stride = contexts[cur_context].color.size;
			switch(contexts[cur_context].color.type) {
				case GL_SHORT:
				case GL_UNSIGNED_SHORT:
					stride *= sizeof(GLshort);
					break;
				case GL_INT:
				case GL_UNSIGNED_INT:
					stride *= sizeof(GLint);
					break;
				case GL_FLOAT:
					stride *= ATARI_SIZEOF_FLOAT;
					break;
				case GL_DOUBLE:
					stride *= ATARI_SIZEOF_DOUBLE;
					break;
			}
		}
		ptr += i*stride;
		switch(contexts[cur_context].color.type) {
			case GL_BYTE:
				switch(contexts[cur_context].color.size) {
					case 3:
						nfglColor3bv((const GLbyte *)ptr);
						break;
					case 4:
						nfglColor4bv((const GLbyte *)ptr);
						break;
				}
				break;
			case GL_UNSIGNED_BYTE:
				switch(contexts[cur_context].color.size) {
					case 3:
						nfglColor3ubv((const GLubyte *)ptr);
						break;
					case 4:
						nfglColor4ubv((const GLubyte *)ptr);
						break;
				}
				break;
			case GL_SHORT:
				switch(contexts[cur_context].color.size) {
					case 3:
						nfglColor3sv((const GLshort *)ptr);
						break;
					case 4:
						nfglColor4sv((const GLshort *)ptr);
						break;
				}
				break;
			case GL_UNSIGNED_SHORT:
				switch(contexts[cur_context].color.size) {
					case 3:
						nfglColor3usv((const GLushort *)ptr);
						break;
					case 4:
						nfglColor4usv((const GLushort *)ptr);
						break;
				}
				break;
			case GL_INT:
				switch(contexts[cur_context].color.size) {
					case 3:
						nfglColor3iv((const GLint *)ptr);
						break;
					case 4:
						nfglColor4iv((const GLint *)ptr);
						break;
				}
				break;
			case GL_UNSIGNED_INT:
				switch(contexts[cur_context].color.size) {
					case 3:
						nfglColor3uiv((const GLuint *)ptr);
						break;
					case 4:
						nfglColor4uiv((const GLuint *)ptr);
						break;
				}
				break;
			case GL_FLOAT:
				switch(contexts[cur_context].color.size) {
					case 3:
						nfglColor3fv((const GLfloat *)ptr);
						break;
					case 4:
						nfglColor4fv((const GLfloat *)ptr);
						break;
				}
				break;
			case GL_DOUBLE:
				switch(contexts[cur_context].color.size) {
					case 3:
						nfglColor3dv((const GLdouble *)ptr);
						break;
					case 4:
						nfglColor4dv((const GLdouble *)ptr);
						break;
				}
				break;
		}
	}

	if(contexts[cur_context].enabled_arrays & NFOSMESA_EDGEFLAG_ARRAY) {
		stride=contexts[cur_context].edgeflag.stride;
		ptr=(const GLubyte *)contexts[cur_context].edgeflag.pointer;
		if(stride==0) {
			stride = contexts[cur_context].edgeflag.size;
		}
		ptr += i*stride;
		nfglEdgeFlagv((const GLboolean *)ptr);
	}

	if(contexts[cur_context].enabled_arrays & NFOSMESA_TEXCOORD_ARRAY) {
		stride=contexts[cur_context].texcoord.stride;
		ptr=(const GLubyte *)contexts[cur_context].texcoord.pointer;
		if(stride==0) {
			stride = contexts[cur_context].texcoord.size;
			switch(contexts[cur_context].texcoord.type) {
				case GL_SHORT:
					stride *= sizeof(GLshort);
					break;
				case GL_INT:
					stride *= sizeof(GLint);
					break;
				case GL_FLOAT:
					stride *= ATARI_SIZEOF_FLOAT;
					break;
				case GL_DOUBLE:
					stride *= ATARI_SIZEOF_DOUBLE;
					break;
			}
		}
		ptr += i*stride;
		switch(contexts[cur_context].texcoord.type) {
			case GL_SHORT:
				switch(contexts[cur_context].texcoord.size) {
					case 1:
						nfglTexCoord1sv((const GLshort *)ptr);
						break;
					case 2:
						nfglTexCoord2sv((const GLshort *)ptr);
						break;
					case 3:
						nfglTexCoord3sv((const GLshort *)ptr);
						break;
					case 4:
						nfglTexCoord4sv((const GLshort *)ptr);
						break;
				}
				break;
			case GL_INT:
				switch(contexts[cur_context].texcoord.size) {
					case 1:
						nfglTexCoord1iv((const GLint *)ptr);
						break;
					case 2:
						nfglTexCoord2iv((const GLint *)ptr);
						break;
					case 3:
						nfglTexCoord3iv((const GLint *)ptr);
						break;
					case 4:
						nfglTexCoord4iv((const GLint *)ptr);
						break;
				}
				break;
			case GL_FLOAT:
				switch(contexts[cur_context].texcoord.size) {
					case 1:
						nfglTexCoord1fv((const GLfloat *)ptr);
						break;
					case 2:
						nfglTexCoord2fv((const GLfloat *)ptr);
						break;
					case 3:
						nfglTexCoord3fv((const GLfloat *)ptr);
						break;
					case 4:
						nfglTexCoord4fv((const GLfloat *)ptr);
						break;
				}
				break;
			case GL_DOUBLE:
				switch(contexts[cur_context].texcoord.size) {
					case 1:
						nfglTexCoord1dv((const GLdouble *)ptr);
						break;
					case 2:
						nfglTexCoord2dv((const GLdouble *)ptr);
						break;
					case 3:
						nfglTexCoord3dv((const GLdouble *)ptr);
						break;
					case 4:
						nfglTexCoord4dv((const GLdouble *)ptr);
						break;
				}
				break;
		}
	}

	if(contexts[cur_context].enabled_arrays & NFOSMESA_VERTEX_ARRAY) {
		stride=contexts[cur_context].vertex.stride;
		ptr=(const GLubyte *)contexts[cur_context].vertex.pointer;
		if(stride==0) {
			stride = contexts[cur_context].vertex.size;
			switch(contexts[cur_context].vertex.type) {
				case GL_SHORT:
					stride *= sizeof(GLshort);
					break;
				case GL_INT:
					stride *= sizeof(GLint);
					break;
				case GL_FLOAT:
					stride *= ATARI_SIZEOF_FLOAT;
					break;
				case GL_DOUBLE:
					stride *= ATARI_SIZEOF_DOUBLE;
					break;
			}
		}
		ptr += i*stride;
		switch(contexts[cur_context].vertex.type) {
			case GL_SHORT:
				switch(contexts[cur_context].vertex.size) {
					case 2:
						nfglVertex2sv((const GLshort *)ptr);
						break;
					case 3:
						nfglVertex3sv((const GLshort *)ptr);
						break;
					case 4:
						nfglVertex4sv((const GLshort *)ptr);
						break;
				}
				break;
			case GL_INT:
				switch(contexts[cur_context].vertex.size) {
					case 2:
						nfglVertex2iv((const GLint *)ptr);
						break;
					case 3:
						nfglVertex3iv((const GLint *)ptr);
						break;
					case 4:
						nfglVertex4iv((const GLint *)ptr);
						break;
				}
				break;
			case GL_FLOAT:
				switch(contexts[cur_context].vertex.size) {
					case 2:
						nfglVertex2fv((const GLfloat *)ptr);
						break;
					case 3:
						nfglVertex3fv((const GLfloat *)ptr);
						break;
					case 4:
						nfglVertex4fv((const GLfloat *)ptr);
						break;
				}
				break;
			case GL_DOUBLE:
				switch(contexts[cur_context].vertex.size) {
					case 2:
						nfglVertex2dv((const GLdouble *)ptr);
						break;
					case 3:
						nfglVertex3dv((const GLdouble *)ptr);
						break;
					case 4:
						nfglVertex4dv((const GLdouble *)ptr);
						break;
				}
				break;
		}
	}
}

void OSMesaDriver::nfglInterleavedArraysHelper(GLenum format, GLsizei stride, const GLvoid *pointer)
{
	SDL_bool enable_texcoord, enable_normal, enable_color;
	int texcoord_size, color_size, vertex_size;
	int texcoord_stride, color_stride, vertex_stride, normal_stride;
	GLubyte *texcoord_ptr,*normal_ptr,*color_ptr,*vertex_ptr;
	GLenum color_type;
	enable_texcoord=SDL_FALSE;
	enable_normal=SDL_FALSE;
	enable_color=SDL_FALSE;
	texcoord_size=color_size=vertex_size=0;
	texcoord_stride=color_stride=vertex_stride=normal_stride=stride;
	texcoord_ptr=normal_ptr=color_ptr=vertex_ptr=(GLubyte *)pointer;
	color_type = GL_FLOAT;
	switch(format) {
		case GL_V2F:
			vertex_size=2;
			break;
		case GL_V3F:
			vertex_size=3;
			break;
		case GL_C4UB_V2F:
			vertex_size=2;
			color_size=4;
			enable_color=SDL_TRUE;
			vertex_ptr += color_size*sizeof(GLubyte);
			color_type = GL_UNSIGNED_BYTE;
			if(stride==0) {
				color_stride = vertex_stride =
					(color_size*sizeof(GLubyte))+(vertex_size*ATARI_SIZEOF_FLOAT);
			}
			break;
		case GL_C4UB_V3F:
			vertex_size=3;
			color_size=4;
			enable_color=SDL_TRUE;
			vertex_ptr += color_size*sizeof(GLubyte);
			color_type = GL_UNSIGNED_BYTE;
			if(stride==0) {
				color_stride = vertex_stride =
					(color_size*sizeof(GLubyte))+(vertex_size*ATARI_SIZEOF_FLOAT);
			}
			break;
		case GL_C3F_V3F:
			vertex_size=3;
			color_size=3;
			enable_color=SDL_TRUE;
			vertex_ptr += color_size*ATARI_SIZEOF_FLOAT;
			if(stride==0) {
				color_stride = vertex_stride =
					(color_size+vertex_size)*ATARI_SIZEOF_FLOAT;
			}
			break;
		case GL_N3F_V3F:
			vertex_size=3;
			enable_normal=SDL_TRUE;
			vertex_ptr += 3*ATARI_SIZEOF_FLOAT;
			if(stride==0) {
				normal_stride = vertex_stride =
					(vertex_size+3)*ATARI_SIZEOF_FLOAT;
			}
			break;
		case GL_C4F_N3F_V3F:
			vertex_size=3;
			color_size=4;
			enable_color=SDL_TRUE;
			enable_normal=SDL_TRUE;
			vertex_ptr +=(color_size+3)*ATARI_SIZEOF_FLOAT;
			normal_ptr += color_size*ATARI_SIZEOF_FLOAT;
			if(stride==0) {
				normal_stride = vertex_stride = color_stride =
					(color_size+vertex_size+3)*ATARI_SIZEOF_FLOAT;
			}
			break;
		case GL_T2F_V3F:
			vertex_size=3;
			texcoord_size=2;
			enable_texcoord=SDL_TRUE;
			vertex_ptr += texcoord_size*ATARI_SIZEOF_FLOAT;
			if(stride==0) {
				texcoord_stride = vertex_stride =
					(texcoord_size+vertex_size)*ATARI_SIZEOF_FLOAT;
			}
			break;
		case GL_T4F_V4F:
			vertex_size=4;
			texcoord_size=4;
			enable_texcoord=SDL_TRUE;
			vertex_ptr += texcoord_size*ATARI_SIZEOF_FLOAT;
			if(stride==0) {
				texcoord_stride = vertex_stride =
					(texcoord_size+vertex_size)*ATARI_SIZEOF_FLOAT;
			}
			break;
		case GL_T2F_C4UB_V3F:
			vertex_size=3;
			texcoord_size=2;
			color_size=4;
			enable_texcoord=SDL_TRUE;
			enable_color=SDL_TRUE;
			vertex_ptr += texcoord_size*ATARI_SIZEOF_FLOAT;
			vertex_ptr += color_size*sizeof(GLubyte);
			color_ptr += texcoord_size*ATARI_SIZEOF_FLOAT;
			color_type = GL_UNSIGNED_BYTE;
			if(stride==0) {
				texcoord_stride = vertex_stride = color_stride =
					((texcoord_size+vertex_size)*ATARI_SIZEOF_FLOAT)+(color_size*sizeof(GLubyte));
			}
			break;
		case GL_T2F_C3F_V3F:
			vertex_size=3;
			texcoord_size=2;
			color_size=3;
			enable_texcoord=SDL_TRUE;
			enable_color=SDL_TRUE;
			vertex_ptr +=(texcoord_size+color_size)*ATARI_SIZEOF_FLOAT;
			color_ptr += texcoord_size*ATARI_SIZEOF_FLOAT;
			if(stride==0) {
				texcoord_stride = vertex_stride = color_stride =
					(texcoord_size+vertex_size+color_size)*ATARI_SIZEOF_FLOAT;
			}
			break;
		case GL_T2F_N3F_V3F:
			vertex_size=3;
			texcoord_size=2;
			enable_normal=SDL_TRUE;
			enable_texcoord=SDL_TRUE;
			vertex_ptr +=(texcoord_size+3)*ATARI_SIZEOF_FLOAT;
			normal_ptr += texcoord_size*ATARI_SIZEOF_FLOAT;
			if(stride==0) {
				texcoord_stride = vertex_stride = normal_stride =
					(texcoord_size+vertex_size+3)*ATARI_SIZEOF_FLOAT;
			}
			break;
		case GL_T2F_C4F_N3F_V3F:
			vertex_size=3;
			texcoord_size=2;
			color_size=4;
			enable_normal=SDL_TRUE;
			enable_texcoord=SDL_TRUE;
			enable_color=SDL_TRUE;
			vertex_ptr +=(texcoord_size+color_size+3)*ATARI_SIZEOF_FLOAT;
			normal_ptr +=(texcoord_size+color_size)*ATARI_SIZEOF_FLOAT;
			color_ptr += texcoord_size*ATARI_SIZEOF_FLOAT;
			if(stride==0) {
				texcoord_stride = vertex_stride = color_stride = normal_stride =
					(texcoord_size+vertex_size+color_size+3)*ATARI_SIZEOF_FLOAT;
			}
			break;
		case GL_T4F_C4F_N3F_V4F:
			vertex_size=4;
			texcoord_size=4;
			color_size=4;
			enable_normal=SDL_TRUE;
			enable_texcoord=SDL_TRUE;
			enable_color=SDL_TRUE;
			vertex_ptr +=(texcoord_size+color_size+3)*ATARI_SIZEOF_FLOAT;
			normal_ptr +=(texcoord_size+color_size)*ATARI_SIZEOF_FLOAT;
			color_ptr += texcoord_size*ATARI_SIZEOF_FLOAT;
			if(stride==0) {
				texcoord_stride = vertex_stride = color_stride = normal_stride =
					(texcoord_size+vertex_size+color_size+3)*ATARI_SIZEOF_FLOAT;
			}
			break;
	}
	nfglDisableClientState(GL_EDGE_FLAG_ARRAY);
	nfglDisableClientState(GL_INDEX_ARRAY);
	if(enable_normal) {
		nfglEnableClientState(GL_NORMAL_ARRAY);
		nfglNormalPointer(GL_FLOAT, normal_stride, (GLfloat *)normal_ptr);
	} else {
		nfglDisableClientState(GL_NORMAL_ARRAY);
	}
	if(enable_color) {
		nfglEnableClientState(GL_COLOR_ARRAY);
		if(color_type == GL_FLOAT) {
			nfglColorPointer(color_size, GL_FLOAT, color_stride, (GLfloat *)color_ptr);
		} else {
			nfglColorPointer(color_size, GL_UNSIGNED_BYTE, color_stride, (GLubyte *)color_ptr);
		}
	} else {
		nfglDisableClientState(GL_COLOR_ARRAY);
	}
	if(enable_texcoord) {
		nfglEnableClientState(GL_TEXTURE_COORD_ARRAY);
		nfglTexCoordPointer(texcoord_size, GL_FLOAT, texcoord_stride, (GLfloat *)texcoord_ptr);
	} else {
		nfglDisableClientState(GL_TEXTURE_COORD_ARRAY);
	}
	nfglEnableClientState(GL_VERTEX_ARRAY);
	nfglVertexPointer(vertex_size, GL_FLOAT, vertex_stride, (GLfloat *)vertex_ptr);
}
#endif

/*---
 * wrappers for functions that take float arguments, and sometimes only exist as
 * functions with double as arguments in GL
 ---*/

void OSMesaDriver::nfglFrustumf(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat near_val, GLfloat far_val)
{
	D(bug("nfosmesa: glFrustumf(%f, %f, %f, %f, %f, %f)", left, right, bottom, top, near_val, far_val));
	if (!GL_ISNOP(fn.glFrustumfOES))
		fn.glFrustumfOES(left, right, bottom, top, near_val, far_val);
	else
		fn.glFrustum(left, right, bottom, top, near_val, far_val);
}

void OSMesaDriver::nfglOrthof(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat near_val, GLfloat far_val)
{
	D(bug("nfosmesa: glOrthof(%f, %f, %f, %f, %f, %f)", left, right, bottom, top, near_val, far_val));
	if (!GL_ISNOP(fn.glOrthofOES))
		fn.glOrthofOES(left, right, bottom, top, near_val, far_val);
	else
		fn.glOrtho(left, right, bottom, top, near_val, far_val);
}

/* glClearDepthf already exists in GL */

/*---
 * GLU functions
 ---*/

void OSMesaDriver::nfgluLookAtf(GLfloat eyeX, GLfloat eyeY, GLfloat eyeZ, GLfloat centerX, GLfloat centerY, GLfloat centerZ, GLfloat upX, GLfloat upY, GLfloat upZ )
{
	GLfloat m[16];
	GLfloat x[3], y[3], z[3];
	GLfloat mag;

	D(bug("nfosmesa: gluLookAtf(%f, %f, %f, %f, %f, %f, %f, %f, %f)", eyeX, eyeY, eyeZ, centerX, centerY, centerZ, upX, upY, upZ));

	/* Make rotation matrix */

	/* Z vector */
	z[0] = eyeX - centerX;
	z[1] = eyeY - centerY;
	z[2] = eyeZ - centerZ;
	mag = sqrtf(z[0] * z[0] + z[1] * z[1] + z[2] * z[2]);
	if (mag)
	{
		z[0] /= mag;
		z[1] /= mag;
		z[2] /= mag;
	}

	/* Y vector */
	y[0] = upX;
	y[1] = upY;
	y[2] = upZ;

	/* X vector = Y cross Z */
	x[0] = y[1] * z[2] - y[2] * z[1];
	x[1] = -y[0] * z[2] + y[2] * z[0];
	x[2] = y[0] * z[1] - y[1] * z[0];

	/* Recompute Y = Z cross X */
	y[0] = z[1] * x[2] - z[2] * x[1];
	y[1] = -z[0] * x[2] + z[2] * x[0];
	y[2] = z[0] * x[1] - z[1] * x[0];

	/* cross product gives area of parallelogram, which is < 1.0 for
	 * non-perpendicular unit-length vectors; so normalize x, y here
	 */

	mag = sqrtf(x[0] * x[0] + x[1] * x[1] + x[2] * x[2]);
	if (mag)
	{
		x[0] /= mag;
		x[1] /= mag;
		x[2] /= mag;
	}

	mag = sqrtf(y[0] * y[0] + y[1] * y[1] + y[2] * y[2]);
	if (mag)
	{
		y[0] /= mag;
		y[1] /= mag;
		y[2] /= mag;
	}

	M(m, 0, 0) = x[0]; M(m, 0, 1) = x[1]; M(m, 0, 2) = x[2]; M(m, 0, 3) = 0.0; 
	M(m, 1, 0) = y[0]; M(m, 1, 1) = y[1]; M(m, 1, 2) = y[2]; M(m, 1, 3) = 0.0; 
	M(m, 2, 0) = z[0]; M(m, 2, 1) = z[1]; M(m, 2, 2) = z[2]; M(m, 2, 3) = 0.0;
	M(m, 3, 0) = 0.0;  M(m, 3, 1) = 0.0;  M(m, 3, 2) = 0.0;  M(m, 3, 3) = 1.0;

	fn.glMultMatrixf(m);

	/* Translate Eye to Origin */
	fn.glTranslatef(-eyeX, -eyeY, -eyeZ);
}

void OSMesaDriver::nfgluLookAt( GLdouble eyeX, GLdouble eyeY, GLdouble eyeZ, GLdouble centerX, GLdouble centerY, GLdouble centerZ, GLdouble upX, GLdouble upY, GLdouble upZ )
{
	D(bug("nfosmesa: gluLookAt(%f, %f, %f, %f, %f, %f, %f, %f, %f)", eyeX, eyeY, eyeZ, centerX, centerY, centerZ, upX, upY, upZ));
	nfgluLookAtf(eyeX, eyeY, eyeZ, centerX, centerY, centerZ, upX, upY, upZ);
}

/*---
 * other functions needed by tiny_gl.ldg
 ---*/

void OSMesaDriver::nftinyglswapbuffer(memptr buffer)
{
	void *draw_buffer;
	Uint32 ctx = cur_context;
	
	D(bug("nfosmesa: swapbuffer($%08x)", buffer));

	if (ctx>MAX_OSMESA_CONTEXTS) {
		return;
	}
	
	if (ctx != 0 && (!fn.OSMesaMakeCurrent || !contexts[ctx].ctx)) {
		return;
	}

	if (ctx != 0)
	{
		contexts[ctx].dst_buffer = draw_buffer = Atari2HostAddr(buffer);
		if (bx_options.osmesa.channel_size > 8) {
			if (contexts[ctx].src_buffer) {
				free(contexts[ctx].src_buffer);
			}
			contexts[ctx].src_buffer = draw_buffer = malloc(contexts[ctx].width * contexts[ctx].height * 4 * (bx_options.osmesa.channel_size>>3));
			D(bug("nfosmesa: Allocated shadow buffer for channel reduction"));
		}
		fn.OSMesaMakeCurrent(contexts[ctx].ctx, draw_buffer, contexts[ctx].type, contexts[ctx].width, contexts[ctx].height);
	}
}

/*--- conversion macros used in generated code ---*/

#if NFOSMESA_NEED_INT_CONV

#define FN_GLARETEXTURESRESIDENT(n, textures, residences) \
	GLuint *tmp; \
	GLboolean result=GL_FALSE; \
	if(n<=0) { \
		return result; \
	} \
	tmp=(GLuint *)malloc(n*sizeof(GLuint)); \
	if(tmp) { \
		Atari2HostIntPtr(n, textures, tmp); \
		result = fn.glAreTexturesResident(n, tmp, residences); \
		free(tmp); \
	} \
	return result

#define FN_GLARETEXTURESRESIDENTEXT(n, textures, residences) \
	GLuint *tmp; \
	GLboolean result=GL_FALSE; \
	if(n<=0) { \
		return result; \
	} \
	tmp=(GLuint *)malloc(n*sizeof(GLuint)); \
	if(tmp) { \
		Atari2HostIntPtr(n, textures, tmp); \
		result = fn.glAreTexturesResidentEXT(n, tmp, residences); \
		free(tmp); \
	} \
	return result

#else

#define FN_GLARETEXTURESRESIDENT(n, textures, residences) return fn.glAreTexturesResident(n, textures, residences)
#define FN_GLARETEXTURESRESIDENTEXT(n, textures, residences) return fn.glAreTexturesResidentEXT(n, textures, residences)

#endif


#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV || NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLARRAYELEMENT(i) nfglArrayElementHelper(i)
#else
#define FN_GLARRAYELEMENT(i) fn.glArrayElement(i)
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV || NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLARRAYELEMENTEXT(i) nfglArrayElementHelper(i)
#else
#define FN_GLARRAYELEMENTEXT(i) fn.glArrayElement(i)
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV || NFOSMESA_NEED_DOUBLE_CONV
static int nfglGetNumParams(GLenum pname)
{
	GLint count = 1;
	
	switch (pname)
	{
	case GL_ALIASED_LINE_WIDTH_RANGE:
	case GL_ALIASED_POINT_SIZE_RANGE:
	case GL_DEPTH_RANGE:
	case GL_DEPTH_BOUNDS_EXT:
	case GL_LINE_WIDTH_RANGE:
	case GL_MAP1_GRID_DOMAIN:
	case GL_MAP2_GRID_SEGMENTS:
	case GL_MAX_VIEWPORT_DIMS:
	case GL_POINT_SIZE_RANGE:
	case GL_POLYGON_MODE:
	case GL_VIEWPORT_BOUNDS_RANGE:
	/* case GL_SMOOTH_LINE_WIDTH_RANGE: same as GL_LINE_WIDTH_RANGE */
	/* case GL_SMOOTH_POINT_SIZE_RANGE: same as GL_POINT_SIZE_RANGE */
		count = 2;
		break;
	case GL_CURRENT_NORMAL:
	case GL_POINT_DISTANCE_ATTENUATION:
		count = 3;
		break;
	case GL_ACCUM_CLEAR_VALUE:
	case GL_BLEND_COLOR:
	/* case GL_BLEND_COLOR_EXT: same as GL_BLEND_COLOR */
	case GL_COLOR_CLEAR_VALUE:
	case GL_COLOR_WRITEMASK:
	case GL_CURRENT_COLOR:
	case GL_CURRENT_RASTER_COLOR:
	case GL_CURRENT_RASTER_POSITION:
	case GL_CURRENT_RASTER_SECONDARY_COLOR:
	case GL_CURRENT_RASTER_TEXTURE_COORDS:
	case GL_CURRENT_SECONDARY_COLOR:
	case GL_CURRENT_TEXTURE_COORDS:
	case GL_FOG_COLOR:
	case GL_LIGHT_MODEL_AMBIENT:
	case GL_MAP2_GRID_DOMAIN:
	case GL_RGBA_SIGNED_COMPONENTS_EXT:
	case GL_SCISSOR_BOX:
	case GL_VIEWPORT:
		count = 4;
		break;
	case GL_COLOR_MATRIX:
	case GL_MODELVIEW_MATRIX:
	case GL_PROJECTION_MATRIX:
	case GL_TEXTURE_MATRIX:
	case GL_TRANSPOSE_MODELVIEW_MATRIX:
	case GL_TRANSPOSE_PROJECTION_MATRIX:
	case GL_TRANSPOSE_TEXTURE_MATRIX:
	case GL_TRANSPOSE_COLOR_MATRIX:
		count = 16;
		break;
	case GL_COMPRESSED_TEXTURE_FORMATS:
		fn.glGetIntegerv(GL_NUM_COMPRESSED_TEXTURE_FORMATS, &count);
		break;
	}
	return count;
}
#endif


#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLBINORMAL3DVEXT(v) GLdouble tmp[3]; \
	Atari2HostDoublePtr(3, v, tmp); \
	fn.glBinormal3dvEXT(tmp)
#else
#define FN_GLBINORMAL3DVEXT(v) 	fn.glBinormal3dvEXT(v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLBINORMAL3FVEXT(v)	GLfloat tmp[3]; \
	Atari2HostFloatPtr(3, v, tmp); \
	fn.glBinormal3fvEXT(tmp)
#else
#define FN_GLBINORMAL3FVEXT(v)	fn.glBinormal3fvEXT(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLBINORMAL3IVEXT(v)	GLint tmp[3]; \
	Atari2HostIntPtr(3, v, tmp); \
	fn.glBinormal3ivEXT(tmp)
#else
#define FN_GLBINORMAL3IVEXT(v)	fn.glBinormal3ivEXT(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLBINORMAL3SVEXT(v)	GLshort tmp[3]; \
	Atari2HostShortPtr(3, v, tmp); \
	fn.glBinormal3svEXT(tmp)
#else
#define FN_GLBINORMAL3SVEXT(v)	fn.glBinormal3svEXT(v)
#endif

#define FN_GLCALLLISTS(n, type, lists) \
	void *tmp; \
	int i; \
	GLshort *src16,*dst16; \
	GLint *src32,*dst32; \
	GLfloat *dstf; \
	 \
	if(n<=0) { \
		return; \
	} \
	switch(type) { \
		case GL_BYTE: \
		case GL_UNSIGNED_BYTE: \
		case GL_2_BYTES: \
		case GL_3_BYTES: \
		case GL_4_BYTES: \
			fn.glCallLists(n, type, lists); \
			return; \
		case GL_SHORT: \
		case GL_UNSIGNED_SHORT: \
			tmp = malloc(n*sizeof(GLshort)); \
			if(!tmp) { \
				return; \
			} \
			dst16 =(GLshort *)tmp; \
			src16 =(GLshort *)lists; \
			for(i=0;i<n;i++) { \
				*dst16++ = SDL_SwapBE16(*src16++); \
			} \
			break; \
		case GL_INT: \
		case GL_UNSIGNED_INT: \
			tmp = malloc(n*sizeof(GLint)); \
			if(!tmp) { \
				return; \
			} \
			dst32 =(GLint *)tmp; \
			src32 =(GLint *)lists; \
			for(i=0;i<n;i++) { \
				*dst32++ = SDL_SwapBE32(*src32++); \
			} \
			break; \
		case GL_FLOAT: \
			tmp = malloc(n*sizeof(GLfloat)); \
			if(!tmp) { \
				return; \
			} \
			dstf =(GLfloat *)tmp; \
			src32 = (GLint *)lists; \
			for(i=0;i<n;i++) { \
				*dstf++ = Atari2HostFloat(*src32++); \
			} \
			break; \
		default: \
			glSetError(GL_INVALID_ENUM); \
			return; \
	} \
	fn.glCallLists(n, type, tmp); \
	if(tmp) { \
		free(tmp); \
	}

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLCLIPPLANE(plane, equation)	GLdouble tmp[4]; \
	Atari2HostDoublePtr(4, equation, tmp); \
	fn.glClipPlane(plane, tmp)
#else
#define FN_GLCLIPPLANE(plane, equation)	fn.glClipPlane(plane, equation)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLCLIPPLANEFOES(plane, equation)	GLfloat tmp[4]; \
	Atari2HostFloatPtr(4, equation, tmp); \
	fn.glClipPlanefOES(plane, tmp)
#else
#define FN_GLCLIPPLANEFOES(plane, equation)	fn.glClipPlanefOES(plane, equation)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLCLIPPLANEXOES(plane, equation)	GLfixed tmp[4]; \
	Atari2HostIntPtr(4, equation, tmp); \
	fn.glClipPlanexOES(plane, tmp)
#else
#define FN_GLCLIPPLANEXOES(plane, equation)	fn.glClipPlanexOES(plane, equation)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLCOLOR3DV(v)	GLdouble tmp[3]; \
	Atari2HostDoublePtr(3, v, tmp); \
	fn.glColor3dv(tmp)
#else
#define FN_GLCOLOR3DV(v)	fn.glColor3dv(v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLCOLOR3FVERTEX3FVSUN(c, v)	GLfloat tmp1[3], tmp2[3]; \
	Atari2HostFloatPtr(3, c, tmp1); \
	Atari2HostFloatPtr(3, v, tmp2); \
	fn.glColor3fVertex3fvSUN(tmp1, tmp2)
#else
#define FN_GLCOLOR3FVERTEX3FVSUN(c, v)	fn.glColor3fVertex3fvSUN(c, v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLCOLOR3FV(v)	GLfloat tmp[3]; \
	Atari2HostFloatPtr(3, v, tmp); \
	fn.glColor3fv(tmp)
#else
#define FN_GLCOLOR3FV(v)	fn.glColor3fv(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLCOLOR3HVNV(v)	GLhalfNV tmp[3]; \
	Atari2HostShortPtr(3, v, tmp); \
	fn.glColor3hvNV(tmp)
#else
#define FN_GLCOLOR3HVNV(v)	fn.glColor3hvNV(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLCOLOR3IV(v)	GLint tmp[3]; \
	Atari2HostIntPtr(3, v, tmp); \
	fn.glColor3iv(tmp)
#else
#define FN_GLCOLOR3IV(v)	fn.glColor3iv(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLCOLOR3SV(v)	GLshort tmp[3]; \
	Atari2HostShortPtr(3, v, tmp); \
	fn.glColor3sv(tmp)
#else
#define FN_GLCOLOR3SV(v)	fn.glColor3sv(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLCOLOR3UIV(v)	GLuint tmp[3]; \
	Atari2HostIntPtr(3, v, tmp); \
	fn.glColor3uiv(tmp)
#else
#define FN_GLCOLOR3UIV(v)	fn.glColor3uiv(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLCOLOR3USV(v)	GLushort tmp[3]; \
	Atari2HostShortPtr(3, v, tmp); \
	fn.glColor3usv(tmp)
#else
#define FN_GLCOLOR3USV(v)	fn.glColor3usv(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLCOLOR3XVOES(components)	GLint tmp[3]; \
	Atari2HostIntPtr(3, components, tmp); \
	fn.glColor3xvOES(tmp)
#else
#define FN_GLCOLOR3XVOES(components)	fn.glColor3xvOES(components)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLCOLOR4DV(v)	GLdouble tmp[4]; \
	Atari2HostDoublePtr(4, v, tmp); \
	fn.glColor4dv(tmp)
#else
#define FN_GLCOLOR4DV(v)	fn.glColor4dv(v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLCOLOR4FNORMAL3FVERTEX3FVSUN(c, n, v)	GLfloat tmp1[4],tmp2[3],tmp3[3]; \
	Atari2HostFloatPtr(4, c, tmp1); \
	Atari2HostFloatPtr(3, n, tmp2); \
	Atari2HostFloatPtr(3, v, tmp3); \
	fn.glColor4fNormal3fVertex3fvSUN(tmp1, tmp2, tmp3)
#else
#define FN_GLCOLOR4FNORMAL3FVERTEX3FVSUN(c, n, v)	fn.glColor4fNormal3fVertex3fvSUN(c, n, v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLCOLOR4FV(v)	GLfloat tmp[4]; \
	Atari2HostFloatPtr(4, v, tmp); \
	fn.glColor4fv(tmp)
#else
#define FN_GLCOLOR4FV(v)	fn.glColor4fv(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLCOLOR4HVNV(v)	GLhalfNV tmp[4]; \
	Atari2HostShortPtr(4, v, tmp); \
	fn.glColor4hvNV(tmp)
#else
#define FN_GLCOLOR4HVNV(v)	fn.glColor4hvNV(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLCOLOR4IV(v)	GLint tmp[4]; \
	Atari2HostIntPtr(4, v, tmp); \
	fn.glColor4iv(tmp)
#else
#define FN_GLCOLOR4IV(v)	fn.glColor4iv(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLCOLOR4SV(v)	GLshort tmp[4]; \
	Atari2HostShortPtr(4, v, tmp); \
	fn.glColor4sv(tmp)
#else
#define FN_GLCOLOR4SV(v)	fn.glColor4sv(v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLCOLOR4UBVERTEX2FVSUN(c, v)	GLfloat tmp[2]; \
	Atari2HostFloatPtr(2, v, tmp); \
	fn.glColor4ubVertex2fvSUN(c, tmp)
#else
#define FN_GLCOLOR4UBVERTEX2FVSUN(c, v)	fn.glColor4ubVertex2fvSUN(c, v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLCOLOR4UBVERTEX3FVSUN(c, v)	GLfloat tmp[3]; \
	Atari2HostFloatPtr(3, v, tmp); \
	fn.glColor4ubVertex3fvSUN(c, tmp)
#else
#define FN_GLCOLOR4UBVERTEX3FVSUN(c, v)	fn.glColor4ubVertex3fvSUN(c, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLCOLOR4UIV(v)	GLuint tmp[4]; \
	Atari2HostIntPtr(4, v, tmp); \
	fn.glColor4uiv(tmp)
#else
#define FN_GLCOLOR4UIV(v)	fn.glColor4uiv(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLCOLOR4USV(v)	GLushort tmp[4]; \
	Atari2HostShortPtr(4, v, tmp); \
	fn.glColor4usv(tmp)
#else
#define FN_GLCOLOR4USV(v)	fn.glColor4usv(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLCOLOR4XVOES(components)	GLfixed tmp[4]; \
	Atari2HostIntPtr(4, components, tmp); \
	fn.glColor4xvOES(tmp)
#else
#define FN_GLCOLOR4XVOES(components)	fn.glColor4xvOES(components)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLCOLORP3UIV(type, color)	GLuint tmp[3]; \
	Atari2HostIntPtr(3, color, tmp); \
	fn.glColorP3uiv(type, tmp)
#else
#define FN_GLCOLORP3UIV(type, color)	fn.glColorP3uiv(type, color)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLCOLORP4UIV(type, color)	GLuint tmp[4]; \
	Atari2HostIntPtr(4, color, tmp); \
	fn.glColorP4uiv(type, tmp)
#else
#define FN_GLCOLORP4UIV(type, color)	fn.glColorP4uiv(type, color)
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV || NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLCOLORPOINTER(size, type, stride, pointer) \
	contexts[cur_context].color.size = size; \
	contexts[cur_context].color.type = type; \
	contexts[cur_context].color.stride = stride; \
	contexts[cur_context].color.count = -1; \
	contexts[cur_context].color.pointer = pointer; \
	fn.glColorPointer(size, type, stride, pointer)
#else
#define FN_GLCOLORPOINTER(size, type, stride, pointer)	fn.glColorPointer(size, type, stride, pointer)
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV || NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLCOLORPOINTEREXT(size, type, stride, count, pointer) \
	contexts[cur_context].color.size = size; \
	contexts[cur_context].color.type = type; \
	contexts[cur_context].color.stride = stride; \
	contexts[cur_context].color.count = count; \
	contexts[cur_context].color.pointer = pointer; \
	fn.glColorPointerEXT(size, type, stride, count, pointer)
#else
#define FN_GLCOLORPOINTEREXT(size, type, stride, count, pointer)	fn.glColorPointerEXT(size, type, stride, count, pointer)
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV || NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLCOLORSUBTABLE(target, start, count, format, type, data)  \
	void *tmp = convertPixels(count, 1, format, type, data); \
	if (tmp) fn.glColorSubTable(target, start, count, format, type, tmp); \
	if (tmp != data) free(tmp)
#else
#define FN_GLCOLORSUBTABLE(target, start, count, format, type, data)	fn.glColorSubTable(target, start, count, format, type, data)
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV || NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLCOLORTABLE(target, internalformat, width, format, type, table) \
	void *tmp = convertPixels(width, 1, format, type, table); \
	if (tmp) fn.glColorTable(target, internalformat, width, format, type, tmp); \
	if (tmp != table) free(tmp)
#else
#define FN_GLCOLORTABLE(target, internalformat, width, format, type, table)	fn.glColorTable(target, internalformat, width, format, type, table)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLDELETETEXTURES(n, textures)	GLuint *tmp; \
	if(n<=0) { \
		return; \
	} \
	tmp=(GLuint *)malloc(n*sizeof(GLuint)); \
	if(tmp) { \
		Atari2HostIntPtr(n, textures, tmp); \
		fn.glDeleteTextures(n, tmp); \
		free(tmp); \
	} else { \
		glSetError(GL_OUT_OF_MEMORY); \
	}
#else
#define FN_GLDELETETEXTURES(n, textures)	fn.glDeleteTextures(n, textures)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLDELETETEXTURESEXT(n, textures)	GLuint *tmp; \
	if(n<=0) { \
		return; \
	} \
	tmp=(GLuint *)malloc(n*sizeof(GLuint)); \
	if(tmp) { \
		Atari2HostIntPtr(n, textures, tmp); \
		fn.glDeleteTexturesEXT(n, tmp); \
		free(tmp); \
	} else { \
		glSetError(GL_OUT_OF_MEMORY); \
	}
	
#else
#define FN_GLDELETETEXTURESEXT(n, textures)	fn.glDeleteTexturesEXT(n, textures)
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV || NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLDISABLECLIENTSTATE(array) \
	switch(array) { \
		case GL_VERTEX_ARRAY: \
			contexts[cur_context].enabled_arrays &= ~NFOSMESA_VERTEX_ARRAY; \
			break; \
		case GL_NORMAL_ARRAY: \
			contexts[cur_context].enabled_arrays &= ~NFOSMESA_NORMAL_ARRAY; \
			break; \
		case GL_COLOR_ARRAY: \
			contexts[cur_context].enabled_arrays &= ~NFOSMESA_COLOR_ARRAY; \
			break; \
		case GL_INDEX_ARRAY: \
			contexts[cur_context].enabled_arrays &= ~NFOSMESA_INDEX_ARRAY; \
			break; \
		case GL_EDGE_FLAG_ARRAY: \
			contexts[cur_context].enabled_arrays &= ~NFOSMESA_EDGEFLAG_ARRAY; \
			break; \
		case GL_TEXTURE_COORD_ARRAY: \
			contexts[cur_context].enabled_arrays &= ~NFOSMESA_TEXCOORD_ARRAY; \
			break; \
	} \
	fn.glDisableClientState(array)
#else
#define FN_GLDISABLECLIENTSTATE(array)	fn.glDisableClientState(array)
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV || NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLDRAWARRAYS(mode, first, count) \
	{ \
		int i; \
		fn.glBegin(	mode); \
		for(i=0;i<count;i++) { \
			nfglArrayElement(first+i); \
		} \
		fn.glEnd(); \
	}
#else
#define FN_GLDRAWARRAYS(mode, first, count)	fn.glDrawArrays(mode, first, count)
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV || NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLDRAWELEMENTS(mode, count, type, indices) \
	{ \
		int i; \
		Uint32 *ptr32; \
		Uint16 *ptr16; \
		Uint8 *ptr8; \
		ptr32 =(Uint32 *)indices; \
		ptr16 =(Uint16 *)indices; \
		ptr8 =(Uint8 *)indices; \
		fn.glBegin(	mode); \
		for(i=0;i<count;i++) { \
			switch(type) { \
				case GL_UNSIGNED_BYTE: \
					nfglArrayElement(ptr8[i]); \
					break; \
				case GL_UNSIGNED_SHORT: \
					nfglArrayElement(SDL_SwapBE16(ptr16[i])); \
					break; \
				case GL_UNSIGNED_INT: \
					nfglArrayElement(SDL_SwapBE32(ptr32[i])); \
					break; \
			} \
		} \
		fn.glEnd(); \
	}	
#else
#define FN_GLDRAWELEMENTS(mode, count, type, indices)	fn.glDrawElements(mode, count, type, indices)
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV || NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLDRAWRANGEELEMENTS(mode, start, end, count, type, indices) \
	{ \
		int i; \
		Uint32 *ptr32,i32; \
		Uint16 *ptr16,i16; \
		Uint8 *ptr8; \
		ptr32 =(Uint32 *)indices; \
		ptr16 =(Uint16 *)indices; \
		ptr8 =(Uint8 *)indices; \
		fn.glBegin(	mode); \
		for(i=0;i<count;i++) { \
			switch(type) { \
				case GL_UNSIGNED_BYTE: \
					if((ptr8[i]>=start) &&(ptr8[i]<=end)) { \
						nfglArrayElement(ptr8[i]); \
					} \
					break; \
				case GL_UNSIGNED_SHORT: \
					i16=SDL_SwapBE16(ptr16[i]); \
					if((i16>=start) &&(i16<=end)) { \
						nfglArrayElement(i16); \
					} \
					break; \
				case GL_UNSIGNED_INT: \
					i32=SDL_SwapBE32(ptr32[i]); \
					if((i32>=start) &&(i32<=end)) { \
						nfglArrayElement(i32); \
					} \
					break; \
			} \
		} \
		fn.glEnd(); \
	}	
#else
#define FN_GLDRAWRANGEELEMENTS(mode, start, end, count, type, indices)	fn.glDrawRangeElements(mode, start, end, count, type, indices)
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV || NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLEDGEFLAGPOINTER(stride, pointer)  \
	contexts[cur_context].edgeflag.size = 1; \
	contexts[cur_context].edgeflag.type = GL_UNSIGNED_BYTE; \
	contexts[cur_context].edgeflag.stride = stride; \
	contexts[cur_context].edgeflag.count = -1; \
	contexts[cur_context].edgeflag.pointer = pointer; \
	fn.glEdgeFlagPointer(stride, pointer)
#else
#define FN_GLEDGEFLAGPOINTER(stride, pointer)	fn.glEdgeFlagPointer(stride, pointer)
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV || NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLENABLECLIENTSTATE(array) \
	switch(array) { \
		case GL_VERTEX_ARRAY: \
			contexts[cur_context].enabled_arrays |= NFOSMESA_VERTEX_ARRAY; \
			break; \
		case GL_NORMAL_ARRAY: \
			contexts[cur_context].enabled_arrays |= NFOSMESA_NORMAL_ARRAY; \
			break; \
		case GL_COLOR_ARRAY: \
			contexts[cur_context].enabled_arrays |= NFOSMESA_COLOR_ARRAY; \
			break; \
		case GL_INDEX_ARRAY: \
			contexts[cur_context].enabled_arrays |= NFOSMESA_INDEX_ARRAY; \
			break; \
		case GL_EDGE_FLAG_ARRAY: \
			contexts[cur_context].enabled_arrays |= NFOSMESA_EDGEFLAG_ARRAY; \
			break; \
		case GL_TEXTURE_COORD_ARRAY: \
			contexts[cur_context].enabled_arrays |= NFOSMESA_TEXCOORD_ARRAY; \
			break; \
	} \
	fn.glEnableClientState(array)
#else
#define FN_GLENABLECLIENTSTATE(array)	fn.glEnableClientState(array)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLEVALCOORD1DV(u)	GLdouble tmp[1]; \
	Atari2HostDoublePtr(1, u, tmp); \
	fn.glEvalCoord1dv(tmp)
#else
#define FN_GLEVALCOORD1DV(u)	fn.glEvalCoord1dv(u)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLEVALCOORD1FV(u)	GLfloat tmp[1]; \
	Atari2HostFloatPtr(1, u, tmp); \
	fn.glEvalCoord1fv(tmp)
#else
#define FN_GLEVALCOORD1FV(u)	fn.glEvalCoord1fv(u)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLEVALCOORD2DV(u)	GLdouble tmp[2]; \
	Atari2HostDoublePtr(2, u, tmp); \
	fn.glEvalCoord2dv(tmp)
#else
#define FN_GLEVALCOORD2DV(u)	fn.glEvalCoord2dv(u)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLEVALCOORD2FV(u)	GLfloat tmp[2]; \
	Atari2HostFloatPtr(2, u, tmp); \
	fn.glEvalCoord2fv(tmp)
#else
#define FN_GLEVALCOORD2FV(u)	fn.glEvalCoord2fv(u)
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV || NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLFINISH() \
	fn.glFinish(); \
	ConvertContext(cur_context)
#else
#define FN_GLFINISH() fn.glFinish()
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV || NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLFLUSH()	fn.glFlush(); \
	ConvertContext(cur_context)
#else
#define FN_GLFLUSH()	fn.glFlush()
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLFOGCOORDHVNV(fog)	GLhalfNV tmp[1]; \
	Atari2HostShortPtr(1, fog, tmp); \
	fn.glFogCoordhvNV(tmp)
#else
#define FN_GLFOGCOORDHVNV(fog)	fn.glFogCoordhvNV(fog)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLFOGFV(pname, params) \
	GLfloat tmp[4]; \
	int size; \
	switch(pname) { \
		case GL_FOG_COLOR: \
			size=4; \
			break; \
		default: \
			size=1; \
			break; \
	} \
	Atari2HostFloatPtr(size, params, tmp); \
	fn.glFogfv(pname, tmp)
#else
#define FN_GLFOGFV(pname, params)	fn.glFogfv(pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLFOGIV(pname, params) \
	GLint tmp[4]; \
	int size; \
	switch(pname) { \
		case GL_FOG_COLOR: \
			size=4; \
			break; \
		default: \
			size=1; \
			break; \
	} \
	Atari2HostIntPtr(size, params, tmp); \
	fn.glFogiv(pname, tmp)
#else
#define FN_GLFOGIV(pname, params)	fn.glFogiv(pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGENTEXTURES(n, textures) \
	int i; \
	fn.glGenTextures(n, textures); \
	for(i=0;i<n;i++) { \
		GLuint numtex; \
		 \
		numtex=textures[i]; \
		textures[i]=SDL_SwapBE32(numtex); \
	}
#else
#define FN_GLGENTEXTURES(n, textures)	fn.glGenTextures(n, textures)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETFLOATV(pname, params) \
	int n; \
	n = nfglGetNumParams(pname); \
	if (n > 16) { \
		GLfloat *tmp; \
		tmp = (GLfloat *)malloc(n * sizeof(*tmp)); \
		fn.glGetFloatv(pname, tmp); \
		Host2AtariFloatArray(n, tmp, params); \
		free(tmp); \
	} else { \
		GLfloat tmp[16]; \
		fn.glGetFloatv(pname, tmp); \
		Host2AtariFloatArray(n, tmp, params); \
	}
#else
#define FN_GLGETFLOATV(pname, params)	fn.glGetFloatv(pname, params)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLGETDOUBLEV(pname, params) \
	int n; \
	n = nfglGetNumParams(pname); \
	if (n > 16) { \
		GLdouble *tmp; \
		tmp = (GLdouble *)malloc(n * sizeof(*tmp)); \
		fn.glGetDoublev(pname, tmp); \
		Host2AtariDoubleArray(n, tmp, params); \
		free(tmp); \
	} else { \
		GLdouble tmp[16]; \
		fn.glGetDoublev(pname, tmp); \
		Host2AtariDoubleArray(n, tmp, params); \
	}
#else
#define FN_GLGETDOUBLEV(pname, params)	fn.glGetDoublev(pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETINTEGERV(pname, params) \
	int i, n; \
	n = nfglGetNumParams(pname); \
	if (n > 16) { \
		GLint *tmp; \
		tmp = (GLint *)malloc(n * sizeof(*tmp)); \
		fn.glGetIntegerv(pname, tmp); \
		for (i = 0; i < n; i++) \
			params[i] = SDL_SwapBE32(tmp[i]); \
		free(tmp); \
	} else { \
		GLint tmp[16]; \
		fn.glGetIntegerv(pname, tmp); \
		for (i = 0; i < n; i++) \
			params[i] = SDL_SwapBE32(tmp[i]); \
	}
#else
#define FN_GLGETINTEGERV(pname, params)	fn.glGetIntegerv(pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV || NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLINDEXPOINTER(type, stride, pointer) \
	contexts[cur_context].index.size = 1; \
	contexts[cur_context].index.type = type; \
	contexts[cur_context].index.stride = stride; \
	contexts[cur_context].index.count = -1; \
	contexts[cur_context].index.pointer = pointer; \
	fn.glIndexPointer(type, stride, pointer)
#else
#define FN_GLINDEXPOINTER(type, stride, pointer)	fn.glIndexPointer(type, stride, pointer)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLINDEXDV(c)	GLdouble tmp[1]; \
	Atari2HostDoublePtr(1, c, tmp); \
	fn.glIndexdv(tmp)
#else
#define FN_GLINDEXDV(c)	fn.glIndexdv(c)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLINDEXFV(c)	GLfloat tmp[1]; \
	Atari2HostFloatPtr(1, c, tmp); \
	fn.glIndexfv(tmp)
#else
#define FN_GLINDEXFV(c)	fn.glIndexfv(c)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLINDEXIV(c)	GLint tmp[1]; \
	Atari2HostIntPtr(1, c, tmp); \
	fn.glIndexiv(tmp)
#else
#define FN_GLINDEXIV(c)	fn.glIndexiv(c)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLINDEXSV(c)	GLshort tmp[1]; \
	Atari2HostShortPtr(1, c, tmp); \
	fn.glIndexsv(tmp)
#else
#define FN_GLINDEXSV(c)	fn.glIndexsv(c)
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV || NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLINTERLEAVEDARRAYS(format, stride, pointer)	nfglInterleavedArraysHelper(format, stride, pointer)
#else
#define FN_GLINTERLEAVEDARRAYS(format, stride, pointer)	fn.glInterleavedArrays(format, stride, pointer)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLLIGHTMODELFV(pname, params) \
	GLfloat tmp[4]; \
	int size; \
	switch(pname) { \
		case GL_LIGHT_MODEL_AMBIENT: \
			size=4; \
			break; \
		default: \
			size=1; \
			break; \
	} \
	Atari2HostFloatPtr(size, params,tmp); \
	fn.glLightModelfv(pname, tmp)
#else
#define FN_GLLIGHTMODELFV(pname, params)	fn.glLightModelfv(pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLLIGHTMODELIV(pname, params) \
	GLint tmp[4]; \
	int size; \
	switch(pname) { \
		case GL_LIGHT_MODEL_AMBIENT: \
			size=4; \
			break; \
		default: \
			size=1; \
			break; \
	} \
	Atari2HostIntPtr(size, params, tmp); \
	fn.glLightModeliv(pname, tmp)
#else
#define FN_GLLIGHTMODELIV(pname, params)	fn.glLightModeliv(pname, params)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLLIGHTFV(light, pname, params)	GLfloat tmp[4]; \
	Atari2HostFloatPtr(4, params,tmp); \
	fn.glLightfv(light, pname, tmp)
#else
#define FN_GLLIGHTFV(light, pname, params)	fn.glLightfv(light, pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLLIGHTIV(light, pname, params)	GLint tmp[4]; \
	Atari2HostIntPtr(4, params, tmp); \
	fn.glLightiv(light, pname, tmp)
#else
#define FN_GLLIGHTIV(light, pname, params)	fn.glLightiv(light, pname, params)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLLOADMATRIXD(m)	GLdouble tmp[16]; \
	Atari2HostDoublePtr(16, m, tmp); \
	fn.glLoadMatrixd(tmp)
#else
#define FN_GLLOADMATRIXD(m)	fn.glLoadMatrixd(m)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLLOADMATRIXF(m)	GLfloat tmp[16]; \
	Atari2HostFloatPtr(16, m, tmp); \
	fn.glLoadMatrixf(tmp)
#else
#define FN_GLLOADMATRIXF(m)	fn.glLoadMatrixf(m)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLLOADTRANSPOSEMATRIXD(m)	GLdouble tmp[16]; \
	Atari2HostDoublePtr(16, m, tmp); \
	fn.glLoadTransposeMatrixd(tmp)
#else
#define FN_GLLOADTRANSPOSEMATRIXD(m)	fn.glLoadTransposeMatrixd(m)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLLOADTRANSPOSEMATRIXDARB(m)	GLdouble tmp[16]; \
	Atari2HostDoublePtr(16, m, tmp); \
	fn.glLoadTransposeMatrixdARB(tmp)
#else
#define FN_GLLOADTRANSPOSEMATRIXDARB(m)	fn.glLoadTransposeMatrixdARB(m)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLLOADTRANSPOSEMATRIXF(m)	GLfloat tmp[16]; \
	Atari2HostFloatPtr(16, m, tmp); \
	fn.glLoadTransposeMatrixf(m)
#else
#define FN_GLLOADTRANSPOSEMATRIXF(m)	fn.glLoadTransposeMatrixf(m)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLLOADTRANSPOSEMATRIXFARB(m)	GLfloat tmp[16]; \
	Atari2HostFloatPtr(16, m, tmp); \
	fn.glLoadTransposeMatrixfARB(m)
#else
#define FN_GLLOADTRANSPOSEMATRIXFARB(m)	fn.glLoadTransposeMatrixfARB(m)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLMAP1D(target, u1, u2, stride, order, points) \
	{ \
		GLdouble *tmp; \
		const GLubyte *ptr; \
		int size, i,j; \
		 \
		switch(target) { \
			case GL_MAP1_INDEX: \
			case GL_MAP1_TEXTURE_COORD_1: \
				size=1; \
				break; \
			case GL_MAP1_TEXTURE_COORD_2: \
				size=2; \
				break; \
			case GL_MAP1_VERTEX_3: \
			case GL_MAP1_NORMAL: \
			case GL_MAP1_TEXTURE_COORD_3: \
				size=3; \
				break; \
			default: \
				size=4; \
				break; \
		} \
		tmp=(GLdouble *)malloc(size*order*sizeof(GLdouble)); \
		if(tmp) { \
			ptr =(const GLubyte *)points; \
			for(i=0;i<order;i++) { \
				for(j=0;j<size;j++) { \
					Atari2HostDoublePtr(1, (const GLdouble *)&ptr[j * ATARI_SIZEOF_DOUBLE], &tmp[i*size+j]); \
				} \
				ptr += stride * ATARI_SIZEOF_DOUBLE; \
			} \
			fn.glMap1d(target, u1, u2, size, order, tmp); \
			free(tmp); \
		} \
	}
#else
#define FN_GLMAP1D(target, u1, u2, stride, order, points)	fn.glMap1d(target, u1, u2, stride, order, points)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLMAP1F(target, u1, u2, stride, order, points) \
	{ \
		GLfloat *tmp; \
		const GLubyte *ptr; \
		int size, i,j; \
		 \
		switch(target) { \
			case GL_MAP1_INDEX: \
			case GL_MAP1_TEXTURE_COORD_1: \
				size=1; \
				break; \
			case GL_MAP1_TEXTURE_COORD_2: \
				size=2; \
				break; \
			case GL_MAP1_VERTEX_3: \
			case GL_MAP1_NORMAL: \
			case GL_MAP1_TEXTURE_COORD_3: \
				size=3; \
				break; \
			default: \
				size=4; \
				break; \
		} \
		tmp=(GLfloat *)malloc(size*order*sizeof(GLfloat)); \
		if(tmp) { \
			ptr =(const GLubyte *)points; \
			for(i=0;i<order;i++) { \
				for(j=0;j<size;j++) { \
					Atari2HostFloatPtr(1, (const GLfloat *)&ptr[j * ATARI_SIZEOF_FLOAT], &tmp[i*size+j]); \
				} \
				ptr += stride * ATARI_SIZEOF_FLOAT; \
			} \
			fn.glMap1f(target, u1, u2, size, order, tmp); \
			free(tmp); \
		} \
	}
#else
#define FN_GLMAP1F(target, u1, u2, stride, order, points)	fn.glMap1f(target, u1, u2, stride, order, points)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLMAP2D(target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points) \
	{ \
		GLdouble *tmp1,*tmp2; \
		const GLubyte *ptr1,*ptr2; \
		int size, i,j; \
		 \
		switch(target) { \
			case GL_MAP2_INDEX: \
			case GL_MAP2_TEXTURE_COORD_1: \
				size=1; \
				break; \
			case GL_MAP2_TEXTURE_COORD_2: \
				size=2; \
				break; \
			case GL_MAP2_VERTEX_3: \
			case GL_MAP2_NORMAL: \
			case GL_MAP2_TEXTURE_COORD_3: \
				size=3; \
				break; \
			default: \
				size=4; \
				break; \
		} \
		tmp1=(GLdouble *)malloc(size*(uorder+vorder)*sizeof(GLdouble)); \
		if(tmp1) { \
			ptr1 =(const GLubyte *)points; \
			for(i=0;i<uorder;i++) { \
				for(j=0;j<size;j++) { \
					Atari2HostDoublePtr(1, (const GLdouble *)&ptr1[j * ATARI_SIZEOF_DOUBLE], &tmp1[i*size+j]); \
				} \
				ptr1 += ustride * ATARI_SIZEOF_DOUBLE; \
			} \
			tmp2=&tmp1[uorder*size]; \
			ptr2 =(const GLubyte *)points; \
			for(i=0;i<vorder;i++) { \
				for(j=0;j<size;j++) { \
					Atari2HostDoublePtr(1, (const GLdouble *)&ptr2[j * ATARI_SIZEOF_DOUBLE], &tmp2[i*size+j]); \
				} \
				ptr2 += vstride * ATARI_SIZEOF_DOUBLE; \
			} \
			fn.glMap2d(target, \
				u1, u2, size, uorder, \
				v1, v2, size*uorder, vorder, tmp1 \
			); \
			free(tmp1); \
		} \
	}
#else
#define FN_GLMAP2D(target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points)	fn.glMap2d(target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLMAP2F(target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points) \
	{ \
		GLfloat *tmp1,*tmp2; \
		const GLubyte *ptr1,*ptr2; \
		int size, i,j; \
		 \
		switch(target) { \
			case GL_MAP2_INDEX: \
			case GL_MAP2_TEXTURE_COORD_1: \
				size=1; \
				break; \
			case GL_MAP2_TEXTURE_COORD_2: \
				size=2; \
				break; \
			case GL_MAP2_VERTEX_3: \
			case GL_MAP2_NORMAL: \
			case GL_MAP2_TEXTURE_COORD_3: \
				size=3; \
				break; \
			default: \
				size=4; \
				break; \
		} \
		tmp1=(GLfloat *)malloc(size*(uorder+vorder)*sizeof(GLfloat)); \
		if(tmp1) { \
			ptr1 =(const GLubyte *)points; \
			for(i=0;i<uorder;i++) { \
				for(j=0;j<size;j++) { \
					Atari2HostFloatPtr(1, (const GLfloat *)&ptr1[j * ATARI_SIZEOF_FLOAT], &tmp1[i*size+j]); \
				} \
				ptr1 += ustride * ATARI_SIZEOF_FLOAT; \
			} \
			tmp2= &tmp1[uorder*size]; \
			ptr2 =(GLubyte *)points; \
			for(i=0;i<vorder;i++) { \
				for(j=0;j<size;j++) { \
					Atari2HostFloatPtr(1, (const GLfloat *)&ptr2[j * ATARI_SIZEOF_FLOAT], &tmp2[i*size+j]); \
				} \
				ptr2 += vstride * ATARI_SIZEOF_FLOAT; \
			} \
			fn.glMap2f(target, \
				u1, u2, size, uorder, \
				v1, v2, size*uorder, vorder, tmp1 \
			); \
			free(tmp1); \
		} \
	}
#else
#define FN_GLMAP2F(target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points)	fn.glMap2f(target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLMATERIALFV(face, pname, params) \
	GLfloat tmp[4]; \
	int size; \
	switch(pname) { \
		case GL_SHININESS: \
			size=1; \
			break; \
		case GL_COLOR_INDEXES: \
			size=3; \
			break; \
		default: \
			size=4; \
			break; \
	} \
	Atari2HostFloatPtr(size, params,tmp); \
	fn.glMaterialfv(face, pname, tmp)
#else
#define FN_GLMATERIALFV(face, pname, params)	fn.glMaterialfv(face, pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLMATERIALIV(face, pname, params) \
	GLint tmp[4]; \
	int size; \
	switch(pname) { \
		case GL_SHININESS: \
			size=1; \
			break; \
		case GL_COLOR_INDEXES: \
			size=3; \
			break; \
		default: \
			size=4; \
			break; \
	} \
	Atari2HostIntPtr(size, params, tmp); \
	fn.glMaterialiv(face, pname, tmp)
#else
#define FN_GLMATERIALIV(face, pname, params)	fn.glMaterialiv(face, pname, params)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLMULTMATRIXD(m)	GLdouble tmp[16]; \
	Atari2HostDoublePtr(16, m, tmp); \
	fn.glMultMatrixd(tmp)
#else
#define FN_GLMULTMATRIXD(m)	fn.glMultMatrixd(m)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLMULTMATRIXF(m)	GLfloat tmp[16]; \
	Atari2HostFloatPtr(16, m, tmp); \
	fn.glMultMatrixf(tmp)
#else
#define FN_GLMULTMATRIXF(m)	fn.glMultMatrixf(m)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLMULTTRANSPOSEMATRIXD(m)	GLdouble tmp[16]; \
	Atari2HostDoublePtr(16, m, tmp); \
	fn.glMultTransposeMatrixd(tmp)
#else
#define FN_GLMULTTRANSPOSEMATRIXD(m)	fn.glMultTransposeMatrixd(m)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLMULTTRANSPOSEMATRIXDARB(m)	GLdouble tmp[16]; \
	Atari2HostDoublePtr(16, m, tmp); \
	fn.glMultTransposeMatrixdARB(tmp)
#else
#define FN_GLMULTTRANSPOSEMATRIXDARB(m)	fn.glMultTransposeMatrixdARB(m)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLMULTTRANSPOSEMATRIXF(m)	GLfloat tmp[16]; \
	Atari2HostFloatPtr(16, m, tmp); \
	fn.glMultTransposeMatrixf(tmp)
#else
#define FN_GLMULTTRANSPOSEMATRIXF(m)	fn.glMultTransposeMatrixf(m)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLMULTTRANSPOSEMATRIXFARB(m)	GLfloat tmp[16]; \
	Atari2HostFloatPtr(16, m, tmp); \
	fn.glMultTransposeMatrixfARB(tmp)
#else
#define FN_GLMULTTRANSPOSEMATRIXFARB(m)	fn.glMultTransposeMatrixfARB(m)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLMULTITEXCOORD1DV(target, v)	GLdouble tmp[1]; \
	Atari2HostDoublePtr(1, v, tmp); \
	fn.glMultiTexCoord1dv(target, tmp)
#else
#define FN_GLMULTITEXCOORD1DV(target, v)	fn.glMultiTexCoord1dv(target, v)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLMULTITEXCOORD1DVARB(target, v)	GLdouble tmp[1]; \
	Atari2HostDoublePtr(1, v, tmp); \
	fn.glMultiTexCoord1dvARB(target, tmp)
#else
#define FN_GLMULTITEXCOORD1DVARB(target, v)	fn.glMultiTexCoord1dvARB(target, v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLMULTITEXCOORD1FV(target, v)	GLfloat tmp[1]; \
	Atari2HostFloatPtr(1, v, tmp); \
	fn.glMultiTexCoord1fv(target, tmp)
#else
#define FN_GLMULTITEXCOORD1FV(target, v)	fn.glMultiTexCoord1fv(target, v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLMULTITEXCOORD1FVARB(target, v)	GLfloat tmp[1]; \
	Atari2HostFloatPtr(1, v, tmp); \
	fn.glMultiTexCoord1fvARB(target, tmp)
#else
#define FN_GLMULTITEXCOORD1FVARB(target, v)	fn.glMultiTexCoord1fvARB(target, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLMULTITEXCOORD1HVNV(target, v)	GLhalfNV tmp[1]; \
	Atari2HostShortPtr(1, v, tmp); \
	fn.glMultiTexCoord1hvNV(target, tmp)
#else
#define FN_GLMULTITEXCOORD1HVNV(target, v)	fn.glMultiTexCoord1hvNV(target, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLMULTITEXCOORD1IV(target, v)	GLint tmp[1]; \
	Atari2HostIntPtr(1, v, tmp); \
	fn.glMultiTexCoord1iv(target, tmp)
#else
#define FN_GLMULTITEXCOORD1IV(target, v)	fn.glMultiTexCoord1iv(target, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLMULTITEXCOORD1IVARB(target, v)	GLint tmp[1]; \
	Atari2HostIntPtr(1, v, tmp); \
	fn.glMultiTexCoord1ivARB(target, tmp)
#else
#define FN_GLMULTITEXCOORD1IVARB(target, v)	fn.glMultiTexCoord1ivARB(target, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLMULTITEXCOORD1SV(target, v)	GLshort tmp[1]; \
	Atari2HostShortPtr(1, v, tmp); \
	fn.glMultiTexCoord1sv(target, tmp)
#else
#define FN_GLMULTITEXCOORD1SV(target, v)	fn.glMultiTexCoord1sv(target, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLMULTITEXCOORD1SVARB(target, v)	GLshort tmp[1]; \
	Atari2HostShortPtr(1, v, tmp); \
	fn.glMultiTexCoord1svARB(target, tmp)
#else
#define FN_GLMULTITEXCOORD1SVARB(target, v)	fn.glMultiTexCoord1svARB(target, v)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLMULTITEXCOORD2DV(target, v)	GLdouble tmp[2]; \
	Atari2HostDoublePtr(2, v, tmp); \
	fn.glMultiTexCoord2dv(target, tmp)
#else
#define FN_GLMULTITEXCOORD2DV(target, v)	fn.glMultiTexCoord2dv(target, v)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLMULTITEXCOORD2DVARB(target, v)	GLdouble tmp[2]; \
	Atari2HostDoublePtr(2, v, tmp); \
	fn.glMultiTexCoord2dvARB(target, tmp)
#else
#define FN_GLMULTITEXCOORD2DVARB(target, v)	fn.glMultiTexCoord2dvARB(target, v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLMULTITEXCOORD2FV(target, v)	GLfloat tmp[2]; \
	Atari2HostFloatPtr(2, v, tmp); \
	fn.glMultiTexCoord2fv(target, tmp)
#else
#define FN_GLMULTITEXCOORD2FV(target, v)	fn.glMultiTexCoord2fv(target, v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLMULTITEXCOORD2FVARB(target, v)	GLfloat tmp[2]; \
	Atari2HostFloatPtr(2, v, tmp); \
	fn.glMultiTexCoord2fvARB(target, tmp)
#else
#define FN_GLMULTITEXCOORD2FVARB(target, v)	fn.glMultiTexCoord2fvARB(target, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLMULTITEXCOORD2HVNV(target, v)	GLhalfNV tmp[2]; \
	Atari2HostShortPtr(2, v, tmp); \
	fn.glMultiTexCoord2hvNV(target, tmp)
#else
#define FN_GLMULTITEXCOORD2HVNV(target, v)	fn.glMultiTexCoord2hvNV(target, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLMULTITEXCOORD2IV(target, v)	GLint tmp[2]; \
	Atari2HostIntPtr(2, v, tmp); \
	fn.glMultiTexCoord2iv(target, tmp)
#else
#define FN_GLMULTITEXCOORD2IV(target, v)	fn.glMultiTexCoord2iv(target, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLMULTITEXCOORD2IVARB(target, v)	GLint tmp[2]; \
	Atari2HostIntPtr(2, v, tmp); \
	fn.glMultiTexCoord2ivARB(target, tmp)
#else
#define FN_GLMULTITEXCOORD2IVARB(target, v)	fn.glMultiTexCoord2ivARB(target, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLMULTITEXCOORD2SV(target, v)	GLshort tmp[2]; \
	Atari2HostShortPtr(2, v, tmp); \
	fn.glMultiTexCoord2sv(target, tmp)
#else
#define FN_GLMULTITEXCOORD2SV(target, v)	fn.glMultiTexCoord2sv(target, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLMULTITEXCOORD2SVARB(target, v)	GLshort tmp[2]; \
	Atari2HostShortPtr(2, v, tmp); \
	fn.glMultiTexCoord2svARB(target, tmp)
#else
#define FN_GLMULTITEXCOORD2SVARB(target, v)	fn.glMultiTexCoord2svARB(target, v)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLMULTITEXCOORD3DV(target, v)	GLdouble tmp[3]; \
	Atari2HostDoublePtr(3, v, tmp); \
	fn.glMultiTexCoord3dv(target, tmp)
#else
#define FN_GLMULTITEXCOORD3DV(target, v)	fn.glMultiTexCoord3dv(target, v)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLMULTITEXCOORD3DVARB(target, v)	GLdouble tmp[3]; \
	Atari2HostDoublePtr(3, v, tmp); \
	fn.glMultiTexCoord3dvARB(target, tmp)
#else
#define FN_GLMULTITEXCOORD3DVARB(target, v)	fn.glMultiTexCoord3dvARB(target, v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLMULTITEXCOORD3FV(target, v)	GLfloat tmp[3]; \
	Atari2HostFloatPtr(3, v, tmp); \
	fn.glMultiTexCoord3fv(target, tmp)
#else
#define FN_GLMULTITEXCOORD3FV(target, v)	fn.glMultiTexCoord3fv(target, v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLMULTITEXCOORD3FVARB(target, v)	GLfloat tmp[3]; \
	Atari2HostFloatPtr(3, v, tmp); \
	fn.glMultiTexCoord3fvARB(target, tmp)
#else
#define FN_GLMULTITEXCOORD3FVARB(target, v)	fn.glMultiTexCoord3fvARB(target, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLMULTITEXCOORD3HVNV(target, v)	GLhalfNV tmp[3]; \
	Atari2HostShortPtr(3, v, tmp); \
	fn.glMultiTexCoord3hvNV(target, tmp)
#else
#define FN_GLMULTITEXCOORD3HVNV(target, v)	fn.glMultiTexCoord3hvNV(target, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLMULTITEXCOORD3IV(target, v)	GLint tmp[3]; \
	Atari2HostIntPtr(3, v, tmp); \
	fn.glMultiTexCoord3iv(target, tmp)
#else
#define FN_GLMULTITEXCOORD3IV(target, v)	fn.glMultiTexCoord3iv(target, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLMULTITEXCOORD3IVARB(target, v)	GLint tmp[3]; \
	Atari2HostIntPtr(3, v, tmp); \
	fn.glMultiTexCoord3ivARB(target, tmp)
#else
#define FN_GLMULTITEXCOORD3IVARB(target, v)	fn.glMultiTexCoord3ivARB(target, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLMULTITEXCOORD3SV(target, v)	GLshort tmp[3]; \
	Atari2HostShortPtr(3, v, tmp); \
	fn.glMultiTexCoord3sv(target, tmp)
#else
#define FN_GLMULTITEXCOORD3SV(target, v)	fn.glMultiTexCoord3sv(target, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLMULTITEXCOORD3SVARB(target, v)	GLshort tmp[3]; \
	Atari2HostShortPtr(3, v, tmp); \
	fn.glMultiTexCoord3svARB(target, tmp)
#else
#define FN_GLMULTITEXCOORD3SVARB(target, v)	fn.glMultiTexCoord3svARB(target, v)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLMULTITEXCOORD4DV(target, v)	GLdouble tmp[4]; \
	Atari2HostDoublePtr(4, v, tmp); \
	fn.glMultiTexCoord4dv(target, tmp)
#else
#define FN_GLMULTITEXCOORD4DV(target, v)	fn.glMultiTexCoord4dv(target, v)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLMULTITEXCOORD4DVARB(target, v)	GLdouble tmp[4]; \
	Atari2HostDoublePtr(4, v, tmp); \
	fn.glMultiTexCoord4dvARB(target, tmp)
#else
#define FN_GLMULTITEXCOORD4DVARB(target, v)	fn.glMultiTexCoord4dvARB(target, v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLMULTITEXCOORD4FV(target, v)	GLfloat tmp[4]; \
	Atari2HostFloatPtr(4, v, tmp); \
	fn.glMultiTexCoord4fv(target, tmp)
#else
#define FN_GLMULTITEXCOORD4FV(target, v)	fn.glMultiTexCoord4fv(target, v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLMULTITEXCOORD4FVARB(target, v)	GLfloat tmp[4]; \
	Atari2HostFloatPtr(4, v, tmp); \
	fn.glMultiTexCoord4fvARB(target, tmp)
#else
#define FN_GLMULTITEXCOORD4FVARB(target, v)	fn.glMultiTexCoord4fvARB(target, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLMULTITEXCOORD4HVNV(target, v)	GLhalfNV tmp[4]; \
	Atari2HostShortPtr(4, v, tmp); \
	fn.glMultiTexCoord4hvNV(target, tmp)
#else
#define FN_GLMULTITEXCOORD4HVNV(target, v)	fn.glMultiTexCoord4hvNV(target, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLMULTITEXCOORD4IV(target, v)	GLint tmp[4]; \
	Atari2HostIntPtr(4, v, tmp); \
	fn.glMultiTexCoord4iv(target, tmp)
#else
#define FN_GLMULTITEXCOORD4IV(target, v)	fn.glMultiTexCoord4iv(target, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLMULTITEXCOORD4IVARB(target, v)	GLint tmp[4]; \
	Atari2HostIntPtr(4, v, tmp); \
	fn.glMultiTexCoord4ivARB(target, tmp)
#else
#define FN_GLMULTITEXCOORD4IVARB(target, v)	fn.glMultiTexCoord4ivARB(target, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLMULTITEXCOORD4SV(target, v)	GLshort tmp[4]; \
	Atari2HostShortPtr(4, v, tmp); \
	fn.glMultiTexCoord4sv(target, tmp)
#else
#define FN_GLMULTITEXCOORD4SV(target, v)	fn.glMultiTexCoord4sv(target, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLMULTITEXCOORD4SVARB(target, v)	GLshort tmp[4]; \
	Atari2HostShortPtr(4, v, tmp); \
	fn.glMultiTexCoord4svARB(target, tmp)
#else
#define FN_GLMULTITEXCOORD4SVARB(target, v)	fn.glMultiTexCoord4svARB(target, v)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLNORMAL3DV(v)	GLdouble tmp[3]; \
	Atari2HostDoublePtr(3, v, tmp); \
	fn.glNormal3dv(tmp)
#else
#define FN_GLNORMAL3DV(v)	fn.glNormal3dv(v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLNORMAL3FVERTEX3FVSUN(n, v)	GLfloat tmp1[3],tmp2[3]; \
	Atari2HostFloatPtr(3, n, tmp1); \
	Atari2HostFloatPtr(3, v, tmp2); \
	fn.glNormal3fVertex3fvSUN(tmp1, tmp2)
#else
#define FN_GLNORMAL3FVERTEX3FVSUN(n, v)	fn.glNormal3fVertex3fvSUN(n, v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLNORMAL3FV(v)	GLfloat tmp[3]; \
	Atari2HostFloatPtr(3, v, tmp); \
	fn.glNormal3fv(tmp)
#else
#define FN_GLNORMAL3FV(v)	fn.glNormal3fv(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLNORMAL3HVNV(v)	GLhalfNV tmp[3]; \
	Atari2HostShortPtr(3, v, tmp); \
	fn.glNormal3hvNV(tmp)
#else
#define FN_GLNORMAL3HVNV(v)	fn.glNormal3hvNV(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLNORMAL3IV(v)	GLint tmp[3]; \
	Atari2HostIntPtr(3, v, tmp); \
	fn.glNormal3iv(tmp)
#else
#define FN_GLNORMAL3IV(v)	fn.glNormal3iv(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLNORMAL3SV(v)	GLshort tmp[3]; \
	Atari2HostShortPtr(3, v, tmp); \
	fn.glNormal3sv(tmp)
#else
#define FN_GLNORMAL3SV(v)	fn.glNormal3sv(v)
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV || NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLNORMALPOINTER(type, stride, pointer) \
	contexts[cur_context].normal.size = 3; \
	contexts[cur_context].normal.type = type; \
	contexts[cur_context].normal.stride = stride; \
	contexts[cur_context].normal.count = -1; \
	contexts[cur_context].normal.pointer = pointer; \
	fn.glNormalPointer(type, stride, pointer)
#else
#define FN_GLNORMALPOINTER(type, stride, pointer)	fn.glNormalPointer(type, stride, pointer)
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV || NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLNORMALPOINTEREXT(type, stride, count, pointer) \
	contexts[cur_context].normal.size = 3; \
	contexts[cur_context].normal.type = type; \
	contexts[cur_context].normal.stride = stride; \
	contexts[cur_context].normal.count = count; \
	contexts[cur_context].normal.pointer = pointer; \
	fn.glNormalPointerEXT(type, stride, count, pointer)
#else
#define FN_GLNORMALPOINTEREXT(type, stride, count, pointer)	fn.glNormalPointerEXT(type, stride, count, pointer)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLNORMALSTREAM3DVATI(stream, coords)	GLdouble tmp[3]; \
	Atari2HostDoublePtr(3, coords, tmp); \
	fn.glNormalStream3dvATI(stream, tmp)
#else
#define FN_GLNORMALSTREAM3DVATI(stream, coords)	fn.glNormalStream3dvATI(stream, coords)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLNORMALSTREAM3FVATI(stream, coords)	GLfloat tmp[3]; \
	Atari2HostFloatPtr(3, coords, tmp); \
	fn.glNormalStream3fvATI(stream, tmp)
#else
#define FN_GLNORMALSTREAM3FVATI(stream, coords)	fn.glNormalStream3fvATI(stream, coords)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLNORMALSTREAM3IVATI(stream, coords)	GLint tmp[3]; \
	Atari2HostIntPtr(3, coords, tmp); \
	fn.glNormalStream3ivATI(stream, tmp)
#else
#define FN_GLNORMALSTREAM3IVATI(stream, coords)	fn.glNormalStream3ivATI(stream, coords)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLNORMALSTREAM3SVATI(stream, coords)	GLshort tmp[3]; \
	Atari2HostShortPtr(3, coords, tmp); \
	fn.glNormalStream3svATI(stream, tmp)
#else
#define FN_GLNORMALSTREAM3SVATI(stream, coords)	fn.glNormalStream3svATI(stream, coords)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPIXELMAPFV(map, mapsize, values)	GLfloat *tmp; \
	tmp=(GLfloat *)malloc(mapsize*sizeof(GLfloat)); \
	if(tmp) { \
		Atari2HostFloatPtr(mapsize, values, tmp); \
		fn.glPixelMapfv(map, mapsize, tmp); \
		free(tmp); \
	}
#else
#define FN_GLPIXELMAPFV(map, mapsize, values)	fn.glPixelMapfv(map, mapsize, values)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLPIXELMAPUIV(map, mapsize, values)	GLuint *tmp; \
	tmp=(GLuint *)malloc(mapsize*sizeof(GLuint)); \
	if(tmp) { \
		Atari2HostIntPtr(mapsize, values, tmp); \
		fn.glPixelMapuiv(map, mapsize, tmp); \
		free(tmp); \
	}
#else
#define FN_GLPIXELMAPUIV(map, mapsize, values)	fn.glPixelMapuiv(map, mapsize, values)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLPIXELMAPUSV(map, mapsize, values)	GLushort *tmp; \
	tmp=(GLushort *)malloc(mapsize*sizeof(GLushort)); \
	if(tmp) { \
		Atari2HostShortPtr(mapsize, values, tmp); \
		fn.glPixelMapusv(map, mapsize, tmp); \
		free(tmp); \
	}
#else
#define FN_GLPIXELMAPUSV(map, mapsize, values)	fn.glPixelMapusv(map, mapsize, values)
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPRIORITIZETEXTURES(n, textures, priorities) \
	GLuint *tmp; \
	GLclampf *tmp2; \
	if(n<=0) { \
		return; \
	} \
	tmp=(GLuint *)malloc(n*sizeof(GLuint)); \
	if(tmp) { \
		tmp2=(GLclampf *)malloc(n*sizeof(GLclampf)); \
		if(tmp2) { \
			Atari2HostIntPtr(n, textures, tmp); \
			Atari2HostFloatPtr(n, priorities, tmp2); \
			fn.glPrioritizeTextures(n, tmp, tmp2); \
			free(tmp2); \
		} \
		free(tmp); \
	}
#else
#define FN_GLPRIORITIZETEXTURES(n, textures, priorities)	fn.glPrioritizeTextures(n, textures, priorities)
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPRIORITIZETEXTURESEXT(n, textures, priorities) \
	GLuint *tmp; \
	GLclampf *tmp2; \
	if(n<=0) { \
		return; \
	} \
	tmp=(GLuint *)malloc(n*sizeof(GLuint)); \
	if(tmp) { \
		tmp2=(GLclampf *)malloc(n*sizeof(GLclampf)); \
		if(tmp2) { \
			Atari2HostIntPtr(n, textures, tmp); \
			Atari2HostFloatPtr(n, priorities, tmp2); \
			fn.glPrioritizeTexturesEXT(n, tmp, tmp2); \
			free(tmp2); \
		} \
		free(tmp); \
	}
#else
#define FN_GLPRIORITIZETEXTURESEXT(n, textures, priorities)	fn.glPrioritizeTexturesEXT(n, textures, priorities)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLPROGRAMENVPARAMETER4DVARB(target, index, params)	GLdouble tmp[4]; \
	Atari2HostDoublePtr(4, params, tmp); \
	fn.glProgramEnvParameter4dvARB(target, index, tmp)
#else
#define FN_GLPROGRAMENVPARAMETER4DVARB(target, index, params)	fn.glProgramEnvParameter4dvARB(target, index, params)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPROGRAMENVPARAMETER4FVARB(target, index, params)	GLfloat tmp[4]; \
	Atari2HostFloatPtr(4, params, tmp); \
	fn.glProgramEnvParameter4fvARB(target, index, tmp)
#else
#define FN_GLPROGRAMENVPARAMETER4FVARB(target, index, params)	fn.glProgramEnvParameter4fvARB(target, index, params)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLPROGRAMLOCALPARAMETER4DVARB(target, index, params)	GLdouble tmp[4]; \
	Atari2HostDoublePtr(4, params, tmp); \
	fn.glProgramLocalParameter4dvARB(target, index, tmp)
#else
#define FN_GLPROGRAMLOCALPARAMETER4DVARB(target, index, params)	fn.glProgramLocalParameter4dvARB(target, index, params)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPROGRAMLOCALPARAMETER4FVARB(target, index, params)	GLfloat tmp[4]; \
	Atari2HostFloatPtr(4, params, tmp); \
	fn.glProgramLocalParameter4fvARB(target, index, tmp)
#else
#define FN_GLPROGRAMLOCALPARAMETER4FVARB(target, index, params)	fn.glProgramLocalParameter4fvARB(target, index, params)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLPROGRAMNAMEDPARAMETER4DVNV(id, len, name, v)	GLdouble tmp[4]; \
	Atari2HostDoublePtr(4, v, tmp); \
	fn.glProgramNamedParameter4dvNV(id, len, name, tmp)
#else
#define FN_GLPROGRAMNAMEDPARAMETER4DVNV(id, len, name, v)	fn.glProgramNamedParameter4dvNV(id, len, name, v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPROGRAMNAMEDPARAMETER4FVNV(id, len, name, v)	GLfloat tmp[4]; \
	Atari2HostFloatPtr(4, v, tmp); \
	fn.glProgramNamedParameter4fvNV(id, len, name, tmp)
#else
#define FN_GLPROGRAMNAMEDPARAMETER4FVNV(id, len, name, v)	fn.glProgramNamedParameter4fvNV(id, len, name, v)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLPROGRAMPARAMETER4DVNV(target, index, v)	GLdouble tmp[4]; \
	Atari2HostDoublePtr(4, v, tmp); \
	fn.glProgramParameter4dvNV(target, index, tmp)
#else
#define FN_GLPROGRAMPARAMETER4DVNV(target, index, v)	fn.glProgramParameter4dvNV(target, index, v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPROGRAMPARAMETER4FVNV(target, index, v)	GLfloat tmp[4]; \
	Atari2HostFloatPtr(4, v, tmp); \
	fn.glProgramParameter4fvNV(target, index, tmp)
#else
#define FN_GLPROGRAMPARAMETER4FVNV(target, index, v)	fn.glProgramParameter4fvNV(target, index, v)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLPROGRAMPARAMETERS4DVNV(target, index, count, v)	GLdouble tmp[4]; \
	Atari2HostDoublePtr(4, v, tmp); \
	fn.glProgramParameters4dvNV(target, index, count, tmp)
#else
#define FN_GLPROGRAMPARAMETERS4DVNV(target, index, count, v)	fn.glProgramParameters4dvNV(target, index, count, v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPROGRAMPARAMETERS4FVNV(target, index, count, v)	GLfloat tmp[4]; \
	Atari2HostFloatPtr(4, v, tmp); \
	fn.glProgramParameters4fvNV(target, index, count, tmp)
#else
#define FN_GLPROGRAMPARAMETERS4FVNV(target, index, count, v)	fn.glProgramParameters4fvNV(target, index, count, v)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLRASTERPOS2DV(v)	GLdouble tmp[2]; \
	Atari2HostDoublePtr(2, v, tmp); \
	fn.glRasterPos2dv(tmp)
#else
#define FN_GLRASTERPOS2DV(v)	fn.glRasterPos2dv(v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLRASTERPOS2FV(v)	GLfloat tmp[2]; \
	Atari2HostFloatPtr(2, v, tmp); \
	fn.glRasterPos2fv(tmp)
#else
#define FN_GLRASTERPOS2FV(v)	fn.glRasterPos2fv(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLRASTERPOS2IV(v)	GLint tmp[2]; \
	Atari2HostIntPtr(2, v, tmp); \
	fn.glRasterPos2iv(tmp)
#else
#define FN_GLRASTERPOS2IV(v)	fn.glRasterPos2iv(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLRASTERPOS2SV(v)	GLshort tmp[2]; \
	Atari2HostShortPtr(2, v, tmp); \
	fn.glRasterPos2sv(tmp)
#else
#define FN_GLRASTERPOS2SV(v)	fn.glRasterPos2sv(v)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLRASTERPOS3DV(v)	GLdouble tmp[3]; \
	Atari2HostDoublePtr(3, v, tmp); \
	fn.glRasterPos3dv(tmp)
#else
#define FN_GLRASTERPOS3DV(v)	fn.glRasterPos3dv(v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLRASTERPOS3FV(v)	GLfloat tmp[3]; \
	Atari2HostFloatPtr(3, v, tmp); \
	fn.glRasterPos3fv(tmp)
#else
#define FN_GLRASTERPOS3FV(v)	fn.glRasterPos3fv(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLRASTERPOS3IV(v)	GLint tmp[3]; \
	Atari2HostIntPtr(3, v, tmp); \
	fn.glRasterPos3iv(tmp)
#else
#define FN_GLRASTERPOS3IV(v)	fn.glRasterPos3iv(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLRASTERPOS3SV(v)	GLshort tmp[3]; \
	Atari2HostShortPtr(3, v, tmp); \
	fn.glRasterPos3sv(tmp)
#else
#define FN_GLRASTERPOS3SV(v)	fn.glRasterPos3sv(v)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLRASTERPOS4DV(v)	GLdouble tmp[4]; \
	Atari2HostDoublePtr(4, v, tmp); \
	fn.glRasterPos4dv(tmp)
#else
#define FN_GLRASTERPOS4DV(v)	fn.glRasterPos4dv(v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLRASTERPOS4FV(v)	GLfloat tmp[4]; \
	Atari2HostFloatPtr(4, v, tmp); \
	fn.glRasterPos4fv(tmp)
#else
#define FN_GLRASTERPOS4FV(v)	fn.glRasterPos4fv(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLRASTERPOS4IV(v)	GLint tmp[4]; \
	Atari2HostIntPtr(4, v, tmp); \
	fn.glRasterPos4iv(tmp)
#else
#define FN_GLRASTERPOS4IV(v)	fn.glRasterPos4iv(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLRASTERPOS4SV(v)	GLshort tmp[4]; \
	Atari2HostShortPtr(4, v, tmp); \
	fn.glRasterPos4sv(tmp)
#else
#define FN_GLRASTERPOS4SV(v)	fn.glRasterPos4sv(v)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLRECTDV(v1, v2)	GLdouble tmp1[4],tmp2[4]; \
	Atari2HostDoublePtr(4, v1, tmp1); \
	Atari2HostDoublePtr(4, v2, tmp2); \
	fn.glRectdv(tmp1, tmp2)
#else
#define FN_GLRECTDV(v1, v2)	fn.glRectdv(v1, v2)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLRECTFV(v1, v2)	GLfloat tmp1[4],tmp2[4]; \
	Atari2HostFloatPtr(4, v1, tmp1); \
	Atari2HostFloatPtr(4, v2, tmp2); \
	fn.glRectfv(tmp1, tmp2)
#else
#define FN_GLRECTFV(v1, v2)	fn.glRectfv(v1, v2)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLRECTIV(v1, v2)	GLint tmp1[4],tmp2[4]; \
	Atari2HostIntPtr(4, v1, tmp1); \
	Atari2HostIntPtr(4, v2, tmp2); \
	fn.glRectiv(tmp1, tmp2)
#else
#define FN_GLRECTIV(v1, v2)	fn.glRectiv(v1, v2)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLRECTSV(v1, v2)	GLshort tmp1[4],tmp2[4]; \
	Atari2HostShortPtr(4, v1, tmp1); \
	Atari2HostShortPtr(4, v2, tmp2); \
	fn.glRectsv(tmp1, tmp2)
#else
#define FN_GLRECTSV(v1, v2)	fn.glRectsv(v1, v2)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLSECONDARYCOLOR3DV(v)	GLdouble tmp[3]; \
	Atari2HostDoublePtr(3, v, tmp); \
	fn.glSecondaryColor3dv(tmp)
#else
#define FN_GLSECONDARYCOLOR3DV(v)	fn.glSecondaryColor3dv(v)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLSECONDARYCOLOR3DVEXT(v)	GLdouble tmp[3]; \
	Atari2HostDoublePtr(3, v, tmp); \
	fn.glSecondaryColor3dvEXT(tmp)
#else
#define FN_GLSECONDARYCOLOR3DVEXT(v)	fn.glSecondaryColor3dvEXT(v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLSECONDARYCOLOR3FV(v)	GLfloat tmp[3]; \
	Atari2HostFloatPtr(3, v, tmp); \
	fn.glSecondaryColor3fv(tmp)
#else
#define FN_GLSECONDARYCOLOR3FV(v)	fn.glSecondaryColor3fv(v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLSECONDARYCOLOR3FVEXT(v)	GLfloat tmp[3]; \
	Atari2HostFloatPtr(3, v, tmp); \
	fn.glSecondaryColor3fvEXT(tmp)
#else
#define FN_GLSECONDARYCOLOR3FVEXT(v)	fn.glSecondaryColor3fvEXT(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLSECONDARYCOLOR3HVNV(v)	GLhalfNV tmp[3]; \
	Atari2HostShortPtr(3, v, tmp); \
	fn.glSecondaryColor3hvNV(tmp)
#else
#define FN_GLSECONDARYCOLOR3HVNV(v)	fn.glSecondaryColor3hvNV(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLSECONDARYCOLOR3IV(v)	GLint tmp[3]; \
	Atari2HostIntPtr(3, v, tmp); \
	fn.glSecondaryColor3iv(tmp)
#else
#define FN_GLSECONDARYCOLOR3IV(v)	fn.glSecondaryColor3iv(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLSECONDARYCOLOR3IVEXT(v)	GLint tmp[3]; \
	Atari2HostIntPtr(3, v, tmp); \
	fn.glSecondaryColor3ivEXT(tmp)
#else
#define FN_GLSECONDARYCOLOR3IVEXT(v)	fn.glSecondaryColor3ivEXT(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLSECONDARYCOLOR3SV(v)	GLshort tmp[3]; \
	Atari2HostShortPtr(3, v, tmp); \
	fn.glSecondaryColor3sv(tmp)
#else
#define FN_GLSECONDARYCOLOR3SV(v)	fn.glSecondaryColor3sv(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLSECONDARYCOLOR3SVEXT(v)	GLshort tmp[3]; \
	Atari2HostShortPtr(3, v, tmp); \
	fn.glSecondaryColor3svEXT(tmp)
#else
#define FN_GLSECONDARYCOLOR3SVEXT(v)	fn.glSecondaryColor3svEXT(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLSECONDARYCOLOR3UIV(v)	GLuint tmp[3]; \
	Atari2HostIntPtr(3, v, tmp); \
	fn.glSecondaryColor3uiv(tmp)
#else
#define FN_GLSECONDARYCOLOR3UIV(v)	fn.glSecondaryColor3uiv(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLSECONDARYCOLOR3UIVEXT(v)	GLuint tmp[3]; \
	Atari2HostIntPtr(3, v, tmp); \
	fn.glSecondaryColor3uivEXT(tmp)
#else
#define FN_GLSECONDARYCOLOR3UIVEXT(v)	fn.glSecondaryColor3uivEXT(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLSECONDARYCOLOR3USV(v)	GLushort tmp[3]; \
	Atari2HostShortPtr(3, v, tmp); \
	fn.glSecondaryColor3usv(tmp)
#else
#define FN_GLSECONDARYCOLOR3USV(v)	fn.glSecondaryColor3usv(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLSECONDARYCOLOR3USVEXT(v)	GLushort tmp[3]; \
	Atari2HostShortPtr(3, v, tmp); \
	fn.glSecondaryColor3usvEXT(tmp)
#else
#define FN_GLSECONDARYCOLOR3USVEXT(v)	fn.glSecondaryColor3usvEXT(v)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLTANGENT3DVEXT(v)	GLdouble tmp[3]; \
	Atari2HostDoublePtr(3, v, tmp); \
	fn.glTangent3dvEXT(tmp)
#else
#define FN_GLTANGENT3DVEXT(v)	fn.glTangent3dvEXT(v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLTANGENT3FVEXT(v)	GLfloat tmp[3]; \
	Atari2HostFloatPtr(3, v, tmp); \
	fn.glTangent3fvEXT(tmp)
#else
#define FN_GLTANGENT3FVEXT(v)	fn.glTangent3fvEXT(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLTANGENT3IVEXT(v)	GLint tmp[3]; \
	Atari2HostIntPtr(3, v, tmp); \
	fn.glTangent3ivEXT(tmp)
#else
#define FN_GLTANGENT3IVEXT(v)	fn.glTangent3ivEXT(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLTANGENT3SVEXT(v)	GLshort tmp[3]; \
	Atari2HostShortPtr(3, v, tmp); \
	fn.glTangent3svEXT(tmp)
#else
#define FN_GLTANGENT3SVEXT(v)	fn.glTangent3svEXT(v)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLTEXCOORD1DV(v)	GLdouble tmp[1]; \
	Atari2HostDoublePtr(1, v, tmp); \
	fn.glTexCoord1dv(tmp)
#else
#define FN_GLTEXCOORD1DV(v)	fn.glTexCoord1dv(v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLTEXCOORD1FV(v)	GLfloat tmp[1]; \
	Atari2HostFloatPtr(1, v, tmp); \
	fn.glTexCoord1fv(tmp)
#else
#define FN_GLTEXCOORD1FV(v)	fn.glTexCoord1fv(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLTEXCOORD1HVNV(v)	GLhalfNV tmp[1]; \
	Atari2HostShortPtr(1, v, tmp); \
	fn.glTexCoord1hvNV(tmp)
#else
#define FN_GLTEXCOORD1HVNV(v)	fn.glTexCoord1hvNV(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLTEXCOORD1IV(v)	GLint tmp[1]; \
	Atari2HostIntPtr(1, v, tmp); \
	fn.glTexCoord1iv(tmp)
#else
#define FN_GLTEXCOORD1IV(v)	fn.glTexCoord1iv(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLTEXCOORD1SV(v)	GLshort tmp[1]; \
	Atari2HostShortPtr(1, v, tmp); \
	fn.glTexCoord1sv(tmp)
#else
#define FN_GLTEXCOORD1SV(v)	fn.glTexCoord1sv(v)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLTEXCOORD2DV(v)	GLdouble tmp[2]; \
	Atari2HostDoublePtr(2, v, tmp); \
	fn.glTexCoord2dv(tmp)
#else
#define FN_GLTEXCOORD2DV(v)	fn.glTexCoord2dv(v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLTEXCOORD2FCOLOR3FVERTEX3FVSUN(tc, c, v) \
	GLfloat tmp1[2],tmp2[3],tmp3[3]; \
	Atari2HostFloatPtr(2, tc, tmp1); \
	Atari2HostFloatPtr(3, c, tmp2); \
	Atari2HostFloatPtr(3, v, tmp3); \
	fn.glTexCoord2fColor3fVertex3fvSUN(tmp1, tmp2, tmp3)
#else
#define FN_GLTEXCOORD2FCOLOR3FVERTEX3FVSUN(tc, c, v)	fn.glTexCoord2fColor3fVertex3fvSUN(tc, c, v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLTEXCOORD2FCOLOR4FNORMAL3FVERTEX3FVSUN(tc, c, n, v) \
	GLfloat tmp1[2],tmp2[4],tmp3[3],tmp4[3]; \
	Atari2HostFloatPtr(2, tc, tmp1); \
	Atari2HostFloatPtr(4, c, tmp2); \
	Atari2HostFloatPtr(3, n, tmp3); \
	Atari2HostFloatPtr(3, v, tmp4); \
	fn.glTexCoord2fColor4fNormal3fVertex3fvSUN(tmp1, tmp2, tmp3, tmp4)
#else
#define FN_GLTEXCOORD2FCOLOR4FNORMAL3FVERTEX3FVSUN(tc, c, n, v)	fn.glTexCoord2fColor4fNormal3fVertex3fvSUN(tc, c, n, v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLTEXCOORD2FCOLOR4UBVERTEX3FVSUN(tc, c, v)	GLfloat tmp1[2],tmp2[3]; \
	Atari2HostFloatPtr(2, tc, tmp1); \
	Atari2HostFloatPtr(3, v, tmp2); \
	fn.glTexCoord2fColor4ubVertex3fvSUN(tmp1, c, tmp2)
#else
#define FN_GLTEXCOORD2FCOLOR4UBVERTEX3FVSUN(tc, c, v)	fn.glTexCoord2fColor4ubVertex3fvSUN(tc, c, v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLTEXCOORD2FNORMAL3FVERTEX3FVSUN(tc, n, v)	GLfloat tmp1[2],tmp2[3],tmp3[3]; \
	Atari2HostFloatPtr(2, tc, tmp1); \
	Atari2HostFloatPtr(3, n, tmp2); \
	Atari2HostFloatPtr(3, v, tmp3); \
	fn.glTexCoord2fNormal3fVertex3fvSUN(tmp1, tmp2, tmp3)
#else
#define FN_GLTEXCOORD2FNORMAL3FVERTEX3FVSUN(tc, n, v)	fn.glTexCoord2fNormal3fVertex3fvSUN(tc, n, v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLTEXCOORD2FVERTEX3FVSUN(tc, v)	GLfloat tmp1[2],tmp2[3]; \
	Atari2HostFloatPtr(2, tc, tmp1); \
	Atari2HostFloatPtr(3, v, tmp2); \
	fn.glTexCoord2fVertex3fvSUN(tmp1, tmp2)
#else
#define FN_GLTEXCOORD2FVERTEX3FVSUN(tc, v)	fn.glTexCoord2fVertex3fvSUN(tc, v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLTEXCOORD2FV(v)	GLfloat tmp[2]; \
	Atari2HostFloatPtr(2, v, tmp); \
	fn.glTexCoord2fv(tmp)
#else
#define FN_GLTEXCOORD2FV(v)	fn.glTexCoord2fv(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLTEXCOORD2HVNV(v)	GLhalfNV tmp[2]; \
	Atari2HostShortPtr(2, v, tmp); \
	fn.glTexCoord2hvNV(tmp)
#else
#define FN_GLTEXCOORD2HVNV(v)	fn.glTexCoord2hvNV(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLTEXCOORD2IV(v)	GLint tmp[2]; \
	Atari2HostIntPtr(2, v, tmp); \
	fn.glTexCoord2iv(tmp)
#else
#define FN_GLTEXCOORD2IV(v)	fn.glTexCoord2iv(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLTEXCOORD2SV(v)	GLshort tmp[2]; \
	Atari2HostShortPtr(2, v, tmp); \
	fn.glTexCoord2sv(tmp)
#else
#define FN_GLTEXCOORD2SV(v)	fn.glTexCoord2sv(v)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLTEXCOORD3DV(v)	GLdouble tmp[3]; \
	Atari2HostDoublePtr(3, v, tmp); \
	fn.glTexCoord3dv(tmp)
#else
#define FN_GLTEXCOORD3DV(v)	fn.glTexCoord3dv(v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLTEXCOORD3FV(v)	GLfloat tmp[3]; \
	Atari2HostFloatPtr(3, v, tmp); \
	fn.glTexCoord3fv(tmp)
#else
#define FN_GLTEXCOORD3FV(v)	fn.glTexCoord3fv(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLTEXCOORD3HVNV(v)	GLhalfNV tmp[3]; \
	Atari2HostShortPtr(3, v, tmp); \
	fn.glTexCoord3hvNV(tmp)
#else
#define FN_GLTEXCOORD3HVNV(v)	fn.glTexCoord3hvNV(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLTEXCOORD3IV(v)	GLint tmp[3]; \
	Atari2HostIntPtr(3, v, tmp); \
	fn.glTexCoord3iv(tmp)
#else
#define FN_GLTEXCOORD3IV(v)	fn.glTexCoord3iv(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLTEXCOORD3SV(v)	GLshort tmp[3]; \
	Atari2HostShortPtr(3, v, tmp); \
	fn.glTexCoord3sv(tmp)
#else
#define FN_GLTEXCOORD3SV(v)	fn.glTexCoord3sv(v)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLTEXCOORD4DV(v)	GLdouble tmp[4]; \
	Atari2HostDoublePtr(4, v, tmp); \
	fn.glTexCoord4dv(tmp)
#else
#define FN_GLTEXCOORD4DV(v)	fn.glTexCoord4dv(v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLTEXCOORD4FCOLOR4FNORMAL3FVERTEX4FVSUN(tc, c, n, v) \
	GLfloat tmp1[4],tmp2[4],tmp3[3],tmp4[4]; \
	Atari2HostFloatPtr(4, tc, tmp1); \
	Atari2HostFloatPtr(4, c, tmp2); \
	Atari2HostFloatPtr(3, n, tmp3); \
	Atari2HostFloatPtr(4, v, tmp4); \
	fn.glTexCoord4fColor4fNormal3fVertex4fvSUN(tmp1, tmp2, tmp3, tmp4)
#else
#define FN_GLTEXCOORD4FCOLOR4FNORMAL3FVERTEX4FVSUN(tc, c, n, v)	fn.glTexCoord4fColor4fNormal3fVertex4fvSUN(tc, c, n, v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLTEXCOORD4FVERTEX4FVSUN(tc, v) \
	GLfloat tmp1[4],tmp2[4]; \
	Atari2HostFloatPtr(4, tc, tmp1); \
	Atari2HostFloatPtr(4, v, tmp2); \
	fn.glTexCoord4fVertex4fvSUN(tmp1, tmp2)
#else
#define FN_GLTEXCOORD4FVERTEX4FVSUN(tc, v)	fn.glTexCoord4fVertex4fvSUN(tc, v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLTEXCOORD4FV(v)	GLfloat tmp[4]; \
	Atari2HostFloatPtr(4, v, tmp); \
	fn.glTexCoord4fv(tmp)
#else
#define FN_GLTEXCOORD4FV(v)	fn.glTexCoord4fv(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLTEXCOORD4HVNV(v)	GLhalfNV tmp[4]; \
	Atari2HostShortPtr(4, v, tmp); \
	fn.glTexCoord4hvNV(tmp)
#else
#define FN_GLTEXCOORD4HVNV(v)	fn.glTexCoord4hvNV(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLTEXCOORD4IV(v)	GLint tmp[4]; \
	Atari2HostIntPtr(4, v, tmp); \
	fn.glTexCoord4iv(tmp)
#else
#define FN_GLTEXCOORD4IV(v)	fn.glTexCoord4iv(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLTEXCOORD4SV(v)	GLshort tmp[4]; \
	Atari2HostShortPtr(4, v, tmp); \
	fn.glTexCoord4sv(tmp)
#else
#define FN_GLTEXCOORD4SV(v)	fn.glTexCoord4sv(v)
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV || NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLTEXCOORDPOINTER(size, type, stride, pointer) \
	contexts[cur_context].texcoord.size = size; \
	contexts[cur_context].texcoord.type = type; \
	contexts[cur_context].texcoord.stride = stride; \
	contexts[cur_context].texcoord.count = -1; \
	contexts[cur_context].texcoord.pointer = pointer; \
	fn.glTexCoordPointer(size, type, stride, pointer)
#else
#define FN_GLTEXCOORDPOINTER(size, type, stride, pointer)	fn.glTexCoordPointer(size, type, stride, pointer)
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV || NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLTEXCOORDPOINTEREXT(size, type, stride, count, pointer) \
	contexts[cur_context].texcoord.size = size; \
	contexts[cur_context].texcoord.type = type; \
	contexts[cur_context].texcoord.stride = stride; \
	contexts[cur_context].texcoord.count = count; \
	contexts[cur_context].texcoord.pointer = pointer; \
	fn.glTexCoordPointerEXT(size, type, stride, count, pointer)
#else
#define FN_GLTEXCOORDPOINTEREXT(size, type, stride, count, pointer)	fn.glTexCoordPointerEXT(size, type, stride, count, pointer)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLTEXENVFV(target, pname, params) \
	GLfloat tmp[4]; \
	if(pname==GL_TEXTURE_ENV_COLOR) { \
		Atari2HostFloatPtr(4, params, tmp); \
	} else { \
		Atari2HostFloatPtr(1, params, tmp); \
	} \
	fn.glTexEnvfv(target, pname, tmp)
#else
#define FN_GLTEXENVFV(target, pname, params)	fn.glTexEnvfv(target, pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLTEXENVIV(target, pname, params) \
	GLint tmp[4]; \
	if(pname==GL_TEXTURE_ENV_COLOR) { \
		Atari2HostIntPtr(4, params, tmp); \
	} else { \
		Atari2HostIntPtr(1, params, tmp); \
	} \
	fn.glTexEnviv(target, pname, tmp)
#else
#define FN_GLTEXENVIV(target, pname, params)	fn.glTexEnviv(target, pname, params)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLTEXGENDV(coord, pname, params)	GLdouble tmp[4]; \
	Atari2HostDoublePtr(4, params, tmp); \
	fn.glTexGendv(coord, pname, tmp)
#else
#define FN_GLTEXGENDV(coord, pname, params)	fn.glTexGendv(coord, pname, params)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLTEXGENFV(coord, pname, params)	GLfloat tmp[4]; \
	Atari2HostFloatPtr(4, params, tmp); \
	fn.glTexGenfv(coord, pname, tmp)
#else
#define FN_GLTEXGENFV(coord, pname, params)	fn.glTexGenfv(coord, pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLTEXGENIV(coord, pname, params)	GLint tmp[4]; \
	Atari2HostIntPtr(4, params, tmp); \
	fn.glTexGeniv(coord, pname, tmp)
#else
#define FN_GLTEXGENIV(coord, pname, params)	fn.glTexGeniv(coord, pname, params)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLTEXPARAMETERFV(target, pname, params) \
	GLfloat tmp[4]; \
	if(pname==GL_TEXTURE_BORDER_COLOR) \
		Atari2HostFloatPtr(4, params, tmp); \
	else \
		Atari2HostFloatPtr(1, params, tmp); \
	fn.glTexParameterfv(target, pname, tmp)
#else
#define FN_GLTEXPARAMETERFV(target, pname, params)	fn.glTexParameterfv(target, pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLTEXPARAMETERIV(target, pname, params)	GLint tmp[4]; \
	if(pname==GL_TEXTURE_BORDER_COLOR) \
		Atari2HostIntPtr(4, params, tmp); \
	else \
		Atari2HostIntPtr(1, params, tmp); \
	fn.glTexParameteriv(target, pname, tmp)
#else
#define FN_GLTEXPARAMETERIV(target, pname, params)	fn.glTexParameteriv(target, pname, params)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLUNIFORM1FV(location, count, value)	GLfloat tmp[1]; \
	Atari2HostFloatPtr(1, value, tmp); \
	fn.glUniform1fv(location, count, tmp)
#else
#define FN_GLUNIFORM1FV(location, count, value)	fn.glUniform1fv(location, count, value)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLUNIFORM1FVARB(location, count, value)	GLfloat tmp[1]; \
	Atari2HostFloatPtr(1, value, tmp); \
	fn.glUniform1fvARB(location, count, tmp)
#else
#define FN_GLUNIFORM1FVARB(location, count, value)	fn.glUniform1fvARB(location, count, value)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLUNIFORM1IV(location, count, value)	GLint tmp[1]; \
	Atari2HostIntPtr(1, value, tmp); \
	fn.glUniform1iv(location, count, tmp)
#else
#define FN_GLUNIFORM1IV(location, count, value)	fn.glUniform1iv(location, count, value)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLUNIFORM1IVARB(location, count, value)	GLint tmp[1]; \
	Atari2HostIntPtr(1, value, tmp); \
	fn.glUniform1ivARB(location, count, tmp)
#else
#define FN_GLUNIFORM1IVARB(location, count, value)	fn.glUniform1ivARB(location, count, value)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLUNIFORM2FV(location, count, value)	GLfloat tmp[2]; \
	Atari2HostFloatPtr(2, value, tmp); \
	fn.glUniform2fv(location, count, tmp)
#else
#define FN_GLUNIFORM2FV(location, count, value)	fn.glUniform2fv(location, count, value)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLUNIFORM2FVARB(location, count, value)	GLfloat tmp[2]; \
	Atari2HostFloatPtr(2, value, tmp); \
	fn.glUniform2fvARB(location, count, tmp)
#else
#define FN_GLUNIFORM2FVARB(location, count, value)	fn.glUniform2fvARB(location, count, value)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLUNIFORM2IV(location, count, value)	GLint tmp[2]; \
	Atari2HostIntPtr(2, value, tmp); \
	fn.glUniform2iv(location, count, tmp)
#else
#define FN_GLUNIFORM2IV(location, count, value)	fn.glUniform2iv(location, count, value)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLUNIFORM2IVARB(location, count, value)	GLint tmp[2]; \
	Atari2HostIntPtr(2, value, tmp); \
	fn.glUniform2ivARB(location, count, tmp)
#else
#define FN_GLUNIFORM2IVARB(location, count, value)	fn.glUniform2ivARB(location, count, value)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLUNIFORM3FV(location, count, value)	GLfloat tmp[3]; \
	Atari2HostFloatPtr(3, value, tmp); \
	fn.glUniform3fv(location, count, tmp)
#else
#define FN_GLUNIFORM3FV(location, count, value)	fn.glUniform3fv(location, count, value)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLUNIFORM3FVARB(location, count, value)	GLfloat tmp[3]; \
	Atari2HostFloatPtr(3, value, tmp); \
	fn.glUniform3fvARB(location, count, tmp)
#else
#define FN_GLUNIFORM3FVARB(location, count, value)	fn.glUniform3fvARB(location, count, value)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLUNIFORM3IV(location, count, value)	GLint tmp[3]; \
	Atari2HostIntPtr(3, value, tmp); \
	fn.glUniform3iv(location, count, tmp)
#else
#define FN_GLUNIFORM3IV(location, count, value)	fn.glUniform3iv(location, count, value)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLUNIFORM3IVARB(location, count, value)	GLint tmp[3]; \
	Atari2HostIntPtr(3, value, tmp); \
	fn.glUniform3ivARB(location, count, tmp)
#else
#define FN_GLUNIFORM3IVARB(location, count, value)	fn.glUniform3ivARB(location, count, value)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLUNIFORM4FV(location, count, value)	GLfloat tmp[4]; \
	Atari2HostFloatPtr(4, value, tmp); \
	fn.glUniform4fv(location, count, tmp)
#else
#define FN_GLUNIFORM4FV(location, count, value)	fn.glUniform4fv(location, count, value)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLUNIFORM4FVARB(location, count, value)	GLfloat tmp[4]; \
	Atari2HostFloatPtr(4, value, tmp); \
	fn.glUniform4fvARB(location, count, tmp)
#else
#define FN_GLUNIFORM4FVARB(location, count, value)	fn.glUniform4fvARB(location, count, value)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLUNIFORM4IV(location, count, value)	GLint tmp[4]; \
	Atari2HostIntPtr(4, value, tmp); \
	fn.glUniform4iv(location, count, tmp)
#else
#define FN_GLUNIFORM4IV(location, count, value)	fn.glUniform4iv(location, count, value)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLUNIFORM4IVARB(location, count, value)	GLint tmp[4]; \
	Atari2HostIntPtr(4, value, tmp); \
	fn.glUniform4ivARB(location, count, tmp)
#else
#define FN_GLUNIFORM4IVARB(location, count, value)	fn.glUniform4ivARB(location, count, value)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLVERTEX2DV(v)	GLdouble tmp[2]; \
	Atari2HostDoublePtr(2, v, tmp); \
	fn.glVertex2dv(tmp)
#else
#define FN_GLVERTEX2DV(v)	fn.glVertex2dv(v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLVERTEX2FV(v)	GLfloat tmp[2]; \
	Atari2HostFloatPtr(2, v, tmp); \
	fn.glVertex2fv(tmp)
#else
#define FN_GLVERTEX2FV(v)	fn.glVertex2fv(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEX2HVNV(v)	GLhalfNV tmp[2]; \
	Atari2HostShortPtr(2, v, tmp); \
	fn.glVertex2hvNV(tmp)
#else
#define FN_GLVERTEX2HVNV(v)	fn.glVertex2hvNV(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEX2IV(v)	GLint tmp[2]; \
	Atari2HostIntPtr(2, v, tmp); \
	fn.glVertex2iv(tmp)
#else
#define FN_GLVERTEX2IV(v)	fn.glVertex2iv(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEX2SV(v)	GLshort tmp[2]; \
	Atari2HostShortPtr(2, v, tmp); \
	fn.glVertex2sv(tmp)
#else
#define FN_GLVERTEX2SV(v)	fn.glVertex2sv(v)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLVERTEX3DV(v)	GLdouble tmp[3]; \
	Atari2HostDoublePtr(3, v, tmp); \
	fn.glVertex3dv(tmp)
#else
#define FN_GLVERTEX3DV(v)	fn.glVertex3dv(v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLVERTEX3FV(v)	GLfloat tmp[3]; \
	Atari2HostFloatPtr(3, v, tmp); \
	fn.glVertex3fv(tmp)
#else
#define FN_GLVERTEX3FV(v)	fn.glVertex3fv(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEX3HVNV(v)	GLhalfNV tmp[3]; \
	Atari2HostShortPtr(3, v, tmp); \
	fn.glVertex3hvNV(tmp)
#else
#define FN_GLVERTEX3HVNV(v)	fn.glVertex3hvNV(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEX3IV(v)	GLint tmp[3]; \
	Atari2HostIntPtr(3, v, tmp); \
	fn.glVertex3iv(tmp)
#else
#define FN_GLVERTEX3IV(v)	fn.glVertex3iv(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEX3SV(v)	GLshort tmp[3]; \
	Atari2HostShortPtr(3, v, tmp); \
	fn.glVertex3sv(tmp)
#else
#define FN_GLVERTEX3SV(v)	fn.glVertex3sv(v)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLVERTEX4DV(v)	GLdouble tmp[4]; \
	Atari2HostDoublePtr(4, v, tmp); \
	fn.glVertex4dv(tmp)
#else
#define FN_GLVERTEX4DV(v)	fn.glVertex4dv(v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLVERTEX4FV(v)	GLfloat tmp[4]; \
	Atari2HostFloatPtr(4, v, tmp); \
	fn.glVertex4fv(tmp)
#else
#define FN_GLVERTEX4FV(v)	fn.glVertex4fv(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEX4HVNV(v)	GLhalfNV tmp[4]; \
	Atari2HostShortPtr(4, v, tmp); \
	fn.glVertex4hvNV(tmp)
#else
#define FN_GLVERTEX4HVNV(v)	fn.glVertex4hvNV(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEX4IV(v)	GLint tmp[4]; \
	Atari2HostIntPtr(4, v, tmp); \
	fn.glVertex4iv(tmp)
#else
#define FN_GLVERTEX4IV(v)	fn.glVertex4iv(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEX4SV(v)	GLshort tmp[4]; \
	Atari2HostShortPtr(4, v, tmp); \
	fn.glVertex4sv(tmp)
#else
#define FN_GLVERTEX4SV(v)	fn.glVertex4sv(v)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLVERTEXATTRIB1DV(index, v)	GLdouble tmp[1]; \
	Atari2HostDoublePtr(1, v, tmp); \
	fn.glVertexAttrib1dv(index, tmp)
#else
#define FN_GLVERTEXATTRIB1DV(index, v)	fn.glVertexAttrib1dv(index, v)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLVERTEXATTRIB1DVARB(index, v)	GLdouble tmp[1]; \
	Atari2HostDoublePtr(1, v, tmp); \
	fn.glVertexAttrib1dvARB(index, tmp)
#else
#define FN_GLVERTEXATTRIB1DVARB(index, v)	fn.glVertexAttrib1dvARB(index, v)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLVERTEXATTRIB1DVNV(index, v)	GLdouble tmp[1]; \
	Atari2HostDoublePtr(1, v, tmp); \
	fn.glVertexAttrib1dvNV(index, tmp)
#else
#define FN_GLVERTEXATTRIB1DVNV(index, v)	fn.glVertexAttrib1dvNV(index, v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLVERTEXATTRIB1FV(index, v)	GLfloat tmp[1]; \
	Atari2HostFloatPtr(1, v, tmp); \
	fn.glVertexAttrib1fv(index, tmp)
#else
#define FN_GLVERTEXATTRIB1FV(index, v)	fn.glVertexAttrib1fv(index, v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLVERTEXATTRIB1FVARB(index, v)	GLfloat tmp[1]; \
	Atari2HostFloatPtr(1, v, tmp); \
	fn.glVertexAttrib1fvARB(index, tmp)
#else
#define FN_GLVERTEXATTRIB1FVARB(index, v)	fn.glVertexAttrib1fvARB(index, v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLVERTEXATTRIB1FVNV(index, v)	GLfloat tmp[1]; \
	Atari2HostFloatPtr(1, v, tmp); \
	fn.glVertexAttrib1fvNV(index, tmp)
#else
#define FN_GLVERTEXATTRIB1FVNV(index, v)	fn.glVertexAttrib1fvNV(index, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIB1HVNV(index, v)	GLhalfNV tmp[1]; \
	Atari2HostShortPtr(1, v, tmp); \
	fn.glVertexAttrib1hvNV(index, tmp)
#else
#define FN_GLVERTEXATTRIB1HVNV(index, v)	fn.glVertexAttrib1hvNV(index, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIB1SV(index, v)	GLshort tmp[1]; \
	Atari2HostShortPtr(1, v, tmp); \
	fn.glVertexAttrib1sv(index, tmp)
#else
#define FN_GLVERTEXATTRIB1SV(index, v)	fn.glVertexAttrib1sv(index, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIB1SVARB(index, v)	GLshort tmp[1]; \
	Atari2HostShortPtr(1, v, tmp); \
	fn.glVertexAttrib1svARB(index, tmp)
#else
#define FN_GLVERTEXATTRIB1SVARB(index, v)	fn.glVertexAttrib1svARB(index, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIB1SVNV(index, v)	GLshort tmp[1]; \
	Atari2HostShortPtr(1, v, tmp); \
	fn.glVertexAttrib1svNV(index, tmp)
#else
#define FN_GLVERTEXATTRIB1SVNV(index, v)	fn.glVertexAttrib1svNV(index, v)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLVERTEXATTRIB2DV(index, v)	GLdouble tmp[2]; \
	Atari2HostDoublePtr(2, v, tmp); \
	fn.glVertexAttrib2dv(index, tmp)
#else
#define FN_GLVERTEXATTRIB2DV(index, v)	fn.glVertexAttrib2dv(index, v)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLVERTEXATTRIB2DVARB(index, v)	GLdouble tmp[2]; \
	Atari2HostDoublePtr(2, v, tmp); \
	fn.glVertexAttrib2dvARB(index, tmp)
#else
#define FN_GLVERTEXATTRIB2DVARB(index, v)	fn.glVertexAttrib2dvARB(index, v)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLVERTEXATTRIB2DVNV(index, v)	GLdouble tmp[2]; \
	Atari2HostDoublePtr(2, v, tmp); \
	fn.glVertexAttrib2dvNV(index, tmp)
#else
#define FN_GLVERTEXATTRIB2DVNV(index, v)	fn.glVertexAttrib2dvNV(index, v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLVERTEXATTRIB2FV(index, v)	GLfloat tmp[2]; \
	Atari2HostFloatPtr(2, v, tmp); \
	fn.glVertexAttrib2fv(index, tmp)
#else
#define FN_GLVERTEXATTRIB2FV(index, v)	fn.glVertexAttrib2fv(index, v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLVERTEXATTRIB2FVARB(index, v)	GLfloat tmp[2]; \
	Atari2HostFloatPtr(2, v, tmp); \
	fn.glVertexAttrib2fvARB(index, tmp)
#else
#define FN_GLVERTEXATTRIB2FVARB(index, v)	fn.glVertexAttrib2fvARB(index, v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLVERTEXATTRIB2FVNV(index, v)	GLfloat tmp[2]; \
	Atari2HostFloatPtr(2, v, tmp); \
	fn.glVertexAttrib2fvNV(index, tmp)
#else
#define FN_GLVERTEXATTRIB2FVNV(index, v)	fn.glVertexAttrib2fvNV(index, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIB2HVNV(index, v)	GLhalfNV tmp[2]; \
	Atari2HostShortPtr(2, v, tmp); \
	fn.glVertexAttrib2hvNV(index, tmp)
#else
#define FN_GLVERTEXATTRIB2HVNV(index, v)	fn.glVertexAttrib2hvNV(index, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIB2SV(index, v)	GLshort tmp[2]; \
	Atari2HostShortPtr(2, v, tmp); \
	fn.glVertexAttrib2sv(index, tmp)
#else
#define FN_GLVERTEXATTRIB2SV(index, v)	fn.glVertexAttrib2sv(index, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIB2SVARB(index, v)	GLshort tmp[2]; \
	Atari2HostShortPtr(2, v, tmp); \
	fn.glVertexAttrib2svARB(index, tmp)
#else
#define FN_GLVERTEXATTRIB2SVARB(index, v)	fn.glVertexAttrib2svARB(index, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIB2SVNV(index, v)	GLshort tmp[2]; \
	Atari2HostShortPtr(2, v, tmp); \
	fn.glVertexAttrib2svNV(index, tmp)
#else
#define FN_GLVERTEXATTRIB2SVNV(index, v)	fn.glVertexAttrib2svNV(index, v)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLVERTEXATTRIB3DV(index, v)	GLdouble tmp[3]; \
	Atari2HostDoublePtr(3, v, tmp); \
	fn.glVertexAttrib3dv(index, tmp)
#else
#define FN_GLVERTEXATTRIB3DV(index, v)	fn.glVertexAttrib3dv(index, v)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLVERTEXATTRIB3DVARB(index, v)	GLdouble tmp[3]; \
	Atari2HostDoublePtr(3, v, tmp); \
	fn.glVertexAttrib3dvARB(index, tmp)
#else
#define FN_GLVERTEXATTRIB3DVARB(index, v)	fn.glVertexAttrib3dvARB(index, v)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLVERTEXATTRIB3DVNV(index, v)	GLdouble tmp[3]; \
	Atari2HostDoublePtr(3, v, tmp); \
	fn.glVertexAttrib3dvNV(index, tmp)
#else
#define FN_GLVERTEXATTRIB3DVNV(index, v)	fn.glVertexAttrib3dvNV(index, v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLVERTEXATTRIB3FV(index, v)	GLfloat tmp[3]; \
	Atari2HostFloatPtr(3, v, tmp); \
	fn.glVertexAttrib3fv(index, tmp)
#else
#define FN_GLVERTEXATTRIB3FV(index, v)	fn.glVertexAttrib3fv(index, v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLVERTEXATTRIB3FVARB(index, v)	GLfloat tmp[3]; \
	Atari2HostFloatPtr(3, v, tmp); \
	fn.glVertexAttrib3fvARB(index, tmp)
#else
#define FN_GLVERTEXATTRIB3FVARB(index, v)	fn.glVertexAttrib3fvARB(index, v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLVERTEXATTRIB3FVNV(index, v)	GLfloat tmp[3]; \
	Atari2HostFloatPtr(3, v, tmp); \
	fn.glVertexAttrib3fvNV(index, tmp)
#else
#define FN_GLVERTEXATTRIB3FVNV(index, v)	fn.glVertexAttrib3fvNV(index, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIB3HVNV(index, v)	GLhalfNV tmp[3]; \
	Atari2HostShortPtr(3, v, tmp); \
	fn.glVertexAttrib3hvNV(index, tmp)
#else
#define FN_GLVERTEXATTRIB3HVNV(index, v)	fn.glVertexAttrib3hvNV(index, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIB3SV(index, v)	GLshort tmp[3]; \
	Atari2HostShortPtr(3, v, tmp); \
	fn.glVertexAttrib3sv(index, tmp)
#else
#define FN_GLVERTEXATTRIB3SV(index, v)	fn.glVertexAttrib3sv(index, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIB3SVARB(index, v)	GLshort tmp[3]; \
	Atari2HostShortPtr(3, v, tmp); \
	fn.glVertexAttrib3svARB(index, tmp)
#else
#define FN_GLVERTEXATTRIB3SVARB(index, v)	fn.glVertexAttrib3svARB(index, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIB3SVNV(index, v)	GLshort tmp[3]; \
	Atari2HostShortPtr(3, v, tmp); \
	fn.glVertexAttrib3svNV(index, tmp)
#else
#define FN_GLVERTEXATTRIB3SVNV(index, v)	fn.glVertexAttrib3svNV(index, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIB4NIV(index, v)	GLint tmp[4]; \
	Atari2HostIntPtr(4, v, tmp); \
	fn.glVertexAttrib4Niv(index, tmp)
#else
#define FN_GLVERTEXATTRIB4NIV(index, v)	fn.glVertexAttrib4Niv(index, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIB4NIVARB(index, v)	GLint tmp[4]; \
	Atari2HostIntPtr(4, v, tmp); \
	fn.glVertexAttrib4NivARB(index, tmp)
#else
#define FN_GLVERTEXATTRIB4NIVARB(index, v)	fn.glVertexAttrib4NivARB(index, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIB4NSV(index, v)	GLshort tmp[4]; \
	Atari2HostShortPtr(4, v, tmp); \
	fn.glVertexAttrib4Nsv(index, tmp)
#else
#define FN_GLVERTEXATTRIB4NSV(index, v)	fn.glVertexAttrib4Nsv(index, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIB4NSVARB(index, v)	GLshort tmp[4]; \
	Atari2HostShortPtr(4, v, tmp); \
	fn.glVertexAttrib4NsvARB(index, tmp)
#else
#define FN_GLVERTEXATTRIB4NSVARB(index, v)	fn.glVertexAttrib4NsvARB(index, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIB4NUIV(index, v)	GLuint tmp[4]; \
	Atari2HostIntPtr(4, v, tmp); \
	fn.glVertexAttrib4Nuiv(index, tmp)
#else
#define FN_GLVERTEXATTRIB4NUIV(index, v)	fn.glVertexAttrib4Nuiv(index, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIB4NUIVARB(index, v)	GLuint tmp[4]; \
	Atari2HostIntPtr(4, v, tmp); \
	fn.glVertexAttrib4NuivARB(index, tmp)
#else
#define FN_GLVERTEXATTRIB4NUIVARB(index, v)	fn.glVertexAttrib4NuivARB(index, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIB4NUSV(index, v)	GLushort tmp[4]; \
	Atari2HostShortPtr(4, v, tmp); \
	fn.glVertexAttrib4Nusv(index, tmp)
#else
#define FN_GLVERTEXATTRIB4NUSV(index, v)	fn.glVertexAttrib4Nusv(index, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIB4NUSVARB(index, v)	GLushort tmp[4]; \
	Atari2HostShortPtr(4, v, tmp); \
	fn.glVertexAttrib4NusvARB(index, tmp)
#else
#define FN_GLVERTEXATTRIB4NUSVARB(index, v)	fn.glVertexAttrib4NusvARB(index, v)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLVERTEXATTRIB4DV(index, v)	GLdouble tmp[4]; \
	Atari2HostDoublePtr(4, v, tmp); \
	fn.glVertexAttrib4dv(index, tmp)
#else
#define FN_GLVERTEXATTRIB4DV(index, v)	fn.glVertexAttrib4dv(index, v)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLVERTEXATTRIB4DVARB(index, v)	GLdouble tmp[4]; \
	Atari2HostDoublePtr(4, v, tmp); \
	fn.glVertexAttrib4dvARB(index, tmp)
#else
#define FN_GLVERTEXATTRIB4DVARB(index, v)	fn.glVertexAttrib4dvARB(index, v)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLVERTEXATTRIB4DVNV(index, v)	GLdouble tmp[4]; \
	Atari2HostDoublePtr(4, v, tmp); \
	fn.glVertexAttrib4dvNV(index, tmp)
#else
#define FN_GLVERTEXATTRIB4DVNV(index, v)	fn.glVertexAttrib4dvNV(index, v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLVERTEXATTRIB4FV(index, v)	GLfloat tmp[4]; \
	Atari2HostFloatPtr(4, v, tmp); \
	fn.glVertexAttrib4fv(index, tmp)
#else
#define FN_GLVERTEXATTRIB4FV(index, v)	fn.glVertexAttrib4fv(index, v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLVERTEXATTRIB4FVARB(index, v)	GLfloat tmp[4]; \
	Atari2HostFloatPtr(4, v, tmp); \
	fn.glVertexAttrib4fvARB(index, tmp)
#else
#define FN_GLVERTEXATTRIB4FVARB(index, v)	fn.glVertexAttrib4fvARB(index, v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLVERTEXATTRIB4FVNV(index, v)	GLfloat tmp[4]; \
	Atari2HostFloatPtr(4, v, tmp); \
	fn.glVertexAttrib4fvNV(index, tmp)
#else
#define FN_GLVERTEXATTRIB4FVNV(index, v)	fn.glVertexAttrib4fvNV(index, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIB4HVNV(index, v)	GLhalfNV tmp[4]; \
	Atari2HostShortPtr(4, v, tmp); \
	fn.glVertexAttrib4hvNV(index, tmp)
#else
#define FN_GLVERTEXATTRIB4HVNV(index, v)	fn.glVertexAttrib4hvNV(index, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIB4IV(index, v)	GLint tmp[4]; \
	Atari2HostIntPtr(4, v, tmp); \
	fn.glVertexAttrib4iv(index, tmp)
#else
#define FN_GLVERTEXATTRIB4IV(index, v)	fn.glVertexAttrib4iv(index, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIB4IVARB(index, v)	GLint tmp[4]; \
	Atari2HostIntPtr(4, v, tmp); \
	fn.glVertexAttrib4ivARB(index, tmp)
#else
#define FN_GLVERTEXATTRIB4IVARB(index, v)	fn.glVertexAttrib4ivARB(index, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIB4SV(index, v)	GLshort tmp[4]; \
	Atari2HostShortPtr(4, v, tmp); \
	fn.glVertexAttrib4sv(index, tmp)
#else
#define FN_GLVERTEXATTRIB4SV(index, v)	fn.glVertexAttrib4sv(index, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIB4SVARB(index, v)	GLshort tmp[4]; \
	Atari2HostShortPtr(4, v, tmp); \
	fn.glVertexAttrib4svARB(index, tmp)
#else
#define FN_GLVERTEXATTRIB4SVARB(index, v)	fn.glVertexAttrib4svARB(index, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIB4SVNV(index, v)	GLshort tmp[4]; \
	Atari2HostShortPtr(4, v, tmp); \
	fn.glVertexAttrib4svNV(index, tmp)
#else
#define FN_GLVERTEXATTRIB4SVNV(index, v)	fn.glVertexAttrib4svNV(index, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIB4UIV(index, v)	GLuint tmp[4]; \
	Atari2HostIntPtr(4, v, tmp); \
	fn.glVertexAttrib4uiv(index, tmp)
#else
#define FN_GLVERTEXATTRIB4UIV(index, v)	fn.glVertexAttrib4uiv(index, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIB4UIVARB(index, v)	GLuint tmp[4]; \
	Atari2HostIntPtr(4, v, tmp); \
	fn.glVertexAttrib4uivARB(index, tmp)
#else
#define FN_GLVERTEXATTRIB4UIVARB(index, v)	fn.glVertexAttrib4uivARB(index, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIB4USV(index, v)	GLushort tmp[4]; \
	Atari2HostShortPtr(4, v, tmp); \
	fn.glVertexAttrib4usv(index, tmp)
#else
#define FN_GLVERTEXATTRIB4USV(index, v)	fn.glVertexAttrib4usv(index, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIB4USVARB(index, v)	GLushort tmp[4]; \
	Atari2HostShortPtr(4, v, tmp); \
	fn.glVertexAttrib4usvARB(index, tmp)
#else
#define FN_GLVERTEXATTRIB4USVARB(index, v)	fn.glVertexAttrib4usvARB(index, v)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLVERTEXATTRIBS1DVNV(index, count, v)	GLdouble tmp[1]; \
	Atari2HostDoublePtr(1, v, tmp); \
	fn.glVertexAttribs1dvNV(index, count, tmp)
#else
#define FN_GLVERTEXATTRIBS1DVNV(index, count, v)	fn.glVertexAttribs1dvNV(index, count, v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLVERTEXATTRIBS1FVNV(index, count, v)	GLfloat tmp[1]; \
	Atari2HostFloatPtr(1, v, tmp); \
	fn.glVertexAttribs1fvNV(index, count, tmp)
#else
#define FN_GLVERTEXATTRIBS1FVNV(index, count, v)	fn.glVertexAttribs1fvNV(index, count, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIBS1HVNV(index, n, v)	GLhalfNV tmp[1]; \
	Atari2HostShortPtr(1, v, tmp); \
	fn.glVertexAttribs1hvNV(index, n, tmp)
#else
#define FN_GLVERTEXATTRIBS1HVNV(index, n, v)	fn.glVertexAttribs1hvNV(index, n, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIBS1SVNV(index, count, v)	GLshort tmp[1]; \
	Atari2HostShortPtr(1, v, tmp); \
	fn.glVertexAttribs1svNV(index, count, tmp)
#else
#define FN_GLVERTEXATTRIBS1SVNV(index, count, v)	fn.glVertexAttribs1svNV(index, count, v)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLVERTEXATTRIBS2DVNV(index, count, v)	GLdouble tmp[2]; \
	Atari2HostDoublePtr(2, v, tmp); \
	fn.glVertexAttribs2dvNV(index, count, tmp)
#else
#define FN_GLVERTEXATTRIBS2DVNV(index, count, v)	fn.glVertexAttribs2dvNV(index, count, v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLVERTEXATTRIBS2FVNV(index, count, v)	GLfloat tmp[2]; \
	Atari2HostFloatPtr(2, v, tmp); \
	fn.glVertexAttribs2fvNV(index, count, tmp)
#else
#define FN_GLVERTEXATTRIBS2FVNV(index, count, v)	fn.glVertexAttribs2fvNV(index, count, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIBS2HVNV(index, n, v)	GLhalfNV tmp[2]; \
	Atari2HostShortPtr(2, v, tmp); \
	fn.glVertexAttribs2hvNV(index, n, tmp)
#else
#define FN_GLVERTEXATTRIBS2HVNV(index, n, v)	fn.glVertexAttribs2hvNV(index, n, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIBS2SVNV(index, count, v)	GLshort tmp[2]; \
	Atari2HostShortPtr(2, v, tmp); \
	fn.glVertexAttribs2svNV(index, count, tmp)
#else
#define FN_GLVERTEXATTRIBS2SVNV(index, count, v)	fn.glVertexAttribs2svNV(index, count, v)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLVERTEXATTRIBS3DVNV(index, count, v)	GLdouble tmp[3]; \
	Atari2HostDoublePtr(3, v, tmp); \
	fn.glVertexAttribs3dvNV(index, count, tmp)
#else
#define FN_GLVERTEXATTRIBS3DVNV(index, count, v)	fn.glVertexAttribs3dvNV(index, count, v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLVERTEXATTRIBS3FVNV(index, count, v)	GLfloat tmp[3]; \
	Atari2HostFloatPtr(3, v, tmp); \
	fn.glVertexAttribs3fvNV(index, count, tmp)
#else
#define FN_GLVERTEXATTRIBS3FVNV(index, count, v)	fn.glVertexAttribs3fvNV(index, count, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIBS3HVNV(index, n, v)	GLhalfNV tmp[3]; \
	Atari2HostShortPtr(3, v, tmp); \
	fn.glVertexAttribs3hvNV(index, n, tmp)
#else
#define FN_GLVERTEXATTRIBS3HVNV(index, n, v)	fn.glVertexAttribs3hvNV(index, n, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIBS3SVNV(index, count, v)	GLshort tmp[3]; \
	Atari2HostShortPtr(3, v, tmp); \
	fn.glVertexAttribs3svNV(index, count, tmp)
#else
#define FN_GLVERTEXATTRIBS3SVNV(index, count, v)	fn.glVertexAttribs3svNV(index, count, v)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLVERTEXATTRIBS4DVNV(index, count, v)	GLdouble tmp[4]; \
	Atari2HostDoublePtr(4, v, tmp); \
	fn.glVertexAttribs4dvNV(index, count, tmp)
#else
#define FN_GLVERTEXATTRIBS4DVNV(index, count, v)	fn.glVertexAttribs4dvNV(index, count, v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLVERTEXATTRIBS4FVNV(index, count, v)	GLfloat tmp[4]; \
	Atari2HostFloatPtr(4, v, tmp); \
	fn.glVertexAttribs4fvNV(index, count, tmp)
#else
#define FN_GLVERTEXATTRIBS4FVNV(index, count, v)	fn.glVertexAttribs4fvNV(index, count, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIBS4HVNV(index, n, v)	GLhalfNV tmp[4]; \
	Atari2HostShortPtr(4, v, tmp); \
	fn.glVertexAttribs4hvNV(index, n, tmp)
#else
#define FN_GLVERTEXATTRIBS4HVNV(index, n, v)	fn.glVertexAttribs4hvNV(index, n, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIBS4SVNV(index, count, v)	GLshort tmp[4]; \
	Atari2HostShortPtr(4, v, tmp); \
	fn.glVertexAttribs4svNV(index, count, tmp)
#else
#define FN_GLVERTEXATTRIBS4SVNV(index, count, v)	fn.glVertexAttribs4svNV(index, count, v)
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV || NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLVERTEXPOINTER(size, type, stride, pointer) \
	contexts[cur_context].vertex.size = size; \
	contexts[cur_context].vertex.type = type; \
	contexts[cur_context].vertex.stride = stride; \
	contexts[cur_context].vertex.count = -1; \
	contexts[cur_context].vertex.pointer = pointer; \
	fn.glVertexPointer(size, type, stride, pointer)
#else
#define FN_GLVERTEXPOINTER(size, type, stride, pointer)	fn.glVertexPointer(size, type, stride, pointer)
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV || NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLVERTEXPOINTEREXT(size, type, stride, count, pointer) \
	contexts[cur_context].vertex.size = size; \
	contexts[cur_context].vertex.type = type; \
	contexts[cur_context].vertex.stride = stride; \
	contexts[cur_context].vertex.count = count; \
	contexts[cur_context].vertex.pointer = pointer; \
	fn.glVertexPointerEXT(size, type, stride, count, pointer)
#else
#define FN_GLVERTEXPOINTEREXT(size, type, stride, count, pointer)	fn.glVertexPointerEXT(size, type, stride, count, pointer)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLVERTEXSTREAM1DVATI(stream, coords)	GLdouble tmp[1]; \
	Atari2HostDoublePtr(1, coords, tmp); \
	fn.glVertexStream1dvATI(stream, tmp)
#else
#define FN_GLVERTEXSTREAM1DVATI(stream, coords)	fn.glVertexStream1dvATI(stream, coords)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLVERTEXSTREAM1FVATI(stream, coords)	GLfloat tmp[1]; \
	Atari2HostFloatPtr(1, coords, tmp); \
	fn.glVertexStream1fvATI(stream, tmp)
#else
#define FN_GLVERTEXSTREAM1FVATI(stream, coords)	fn.glVertexStream1fvATI(stream, coords)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXSTREAM1IVATI(stream, coords)	GLint tmp[1]; \
	Atari2HostIntPtr(1, coords, tmp); \
	fn.glVertexStream1ivATI(stream, tmp)
#else
#define FN_GLVERTEXSTREAM1IVATI(stream, coords)	fn.glVertexStream1ivATI(stream, coords)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXSTREAM1SVATI(stream, coords)	GLshort tmp[1]; \
	Atari2HostShortPtr(1, coords, tmp); \
	fn.glVertexStream1svATI(stream, tmp)
#else
#define FN_GLVERTEXSTREAM1SVATI(stream, coords)	fn.glVertexStream1svATI(stream, coords)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLVERTEXSTREAM2DVATI(stream, coords)	GLdouble tmp[2]; \
	Atari2HostDoublePtr(2, coords, tmp); \
	fn.glVertexStream2dvATI(stream, tmp)
#else
#define FN_GLVERTEXSTREAM2DVATI(stream, coords)	fn.glVertexStream2dvATI(stream, coords)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLVERTEXSTREAM2FVATI(stream, coords)	GLfloat tmp[2]; \
	Atari2HostFloatPtr(2, coords, tmp); \
	fn.glVertexStream2fvATI(stream, tmp)
#else
#define FN_GLVERTEXSTREAM2FVATI(stream, coords)	fn.glVertexStream2fvATI(stream, coords)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXSTREAM2IVATI(stream, coords)	GLint tmp[2]; \
	Atari2HostIntPtr(2, coords, tmp); \
	fn.glVertexStream2ivATI(stream, tmp)
#else
#define FN_GLVERTEXSTREAM2IVATI(stream, coords)	fn.glVertexStream2ivATI(stream, coords)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXSTREAM2SVATI(stream, coords)	GLshort tmp[2]; \
	Atari2HostShortPtr(2, coords, tmp); \
	fn.glVertexStream2svATI(stream, tmp)
#else
#define FN_GLVERTEXSTREAM2SVATI(stream, coords)	fn.glVertexStream2svATI(stream, coords)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLVERTEXSTREAM3DVATI(stream, coords)	GLdouble tmp[3]; \
	Atari2HostDoublePtr(3, coords, tmp); \
	fn.glVertexStream3dvATI(stream, tmp)
#else
#define FN_GLVERTEXSTREAM3DVATI(stream, coords)	fn.glVertexStream3dvATI(stream, coords)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLVERTEXSTREAM3FVATI(stream, coords)	GLfloat tmp[3]; \
	Atari2HostFloatPtr(3, coords, tmp); \
	fn.glVertexStream3fvATI(stream, tmp)
#else
#define FN_GLVERTEXSTREAM3FVATI(stream, coords)	fn.glVertexStream3fvATI(stream, coords)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXSTREAM3IVATI(stream, coords)	GLint tmp[3]; \
	Atari2HostIntPtr(3, coords, tmp); \
	fn.glVertexStream3ivATI(stream, tmp)
#else
#define FN_GLVERTEXSTREAM3IVATI(stream, coords)	fn.glVertexStream3ivATI(stream, coords)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXSTREAM3SVATI(stream, coords)	GLshort tmp[3]; \
	Atari2HostShortPtr(3, coords, tmp); \
	fn.glVertexStream3svATI(stream, tmp)
#else
#define FN_GLVERTEXSTREAM3SVATI(stream, coords)	fn.glVertexStream3svATI(stream, coords)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLVERTEXSTREAM4DVATI(stream, coords)	GLdouble tmp[4]; \
	Atari2HostDoublePtr(4, coords, tmp); \
	fn.glVertexStream4dvATI(stream, tmp)
#else
#define FN_GLVERTEXSTREAM4DVATI(stream, coords)	fn.glVertexStream4dvATI(stream, coords)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLVERTEXSTREAM4FVATI(stream, coords)	GLfloat tmp[4]; \
	Atari2HostFloatPtr(4, coords, tmp); \
	fn.glVertexStream4fvATI(stream, tmp)
#else
#define FN_GLVERTEXSTREAM4FVATI(stream, coords)	fn.glVertexStream4fvATI(stream, coords)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXSTREAM4IVATI(stream, coords)	GLint tmp[4]; \
	Atari2HostIntPtr(4, coords, tmp); \
	fn.glVertexStream4ivATI(stream, tmp)
#else
#define FN_GLVERTEXSTREAM4IVATI(stream, coords)	fn.glVertexStream4ivATI(stream, coords)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXSTREAM4SVATI(stream, coords)	GLshort tmp[4]; \
	Atari2HostShortPtr(4, coords, tmp); \
	fn.glVertexStream4svATI(stream, tmp)
#else
#define FN_GLVERTEXSTREAM4SVATI(stream, coords)	fn.glVertexStream4svATI(stream, coords)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLVERTEXWEIGHTFVEXT(weight)	GLfloat tmp[1]; \
	Atari2HostFloatPtr(1, weight, tmp); \
	fn.glVertexWeightfvEXT(tmp)
#else
#define FN_GLVERTEXWEIGHTFVEXT(weight)	fn.glVertexWeightfvEXT(weight)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXWEIGHTHVNV(weight)	GLhalfNV tmp[1]; \
	Atari2HostShortPtr(1, weight, tmp); \
	fn.glVertexWeighthvNV(tmp)
#else
#define FN_GLVERTEXWEIGHTHVNV(weight)	fn.glVertexWeighthvNV(weight)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLWINDOWPOS2DV(v)	GLdouble tmp[2]; \
	Atari2HostDoublePtr(2, v, tmp); \
	fn.glWindowPos2dv(tmp)
#else
#define FN_GLWINDOWPOS2DV(v)	fn.glWindowPos2dv(v)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLWINDOWPOS2DVARB(v)	GLdouble tmp[2]; \
	Atari2HostDoublePtr(2, v, tmp); \
	fn.glWindowPos2dvARB(tmp)
#else
#define FN_GLWINDOWPOS2DVARB(v)	fn.glWindowPos2dvARB(v)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLWINDOWPOS2DVMESA(v)	GLdouble tmp[2]; \
	Atari2HostDoublePtr(2, v, tmp); \
	fn.glWindowPos2dvMESA(tmp)
#else
#define FN_GLWINDOWPOS2DVMESA(v)	fn.glWindowPos2dvMESA(v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLWINDOWPOS2FV(v)	GLfloat tmp[2]; \
	Atari2HostFloatPtr(2, v, tmp); \
	fn.glWindowPos2fv(tmp)
#else
#define FN_GLWINDOWPOS2FV(v)	fn.glWindowPos2fv(v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLWINDOWPOS2FVARB(v)	GLfloat tmp[2]; \
	Atari2HostFloatPtr(2, v, tmp); \
	fn.glWindowPos2fvARB(tmp)
#else
#define FN_GLWINDOWPOS2FVARB(v)	fn.glWindowPos2fvARB(v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLWINDOWPOS2FVMESA(v)	GLfloat tmp[2]; \
	Atari2HostFloatPtr(2, v, tmp); \
	fn.glWindowPos2fvMESA(tmp)
#else
#define FN_GLWINDOWPOS2FVMESA(v)	fn.glWindowPos2fvMESA(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLWINDOWPOS2IV(v)	GLint tmp[2]; \
	Atari2HostIntPtr(2, v, tmp); \
	fn.glWindowPos2iv(tmp)
#else
#define FN_GLWINDOWPOS2IV(v)	fn.glWindowPos2iv(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLWINDOWPOS2IVARB(v)	GLint tmp[2]; \
	Atari2HostIntPtr(2, v, tmp); \
	fn.glWindowPos2ivARB(tmp)
#else
#define FN_GLWINDOWPOS2IVARB(v)	fn.glWindowPos2ivARB(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLWINDOWPOS2IVMESA(v)	GLint tmp[2]; \
	Atari2HostIntPtr(2, v, tmp); \
	fn.glWindowPos2ivMESA(tmp)
#else
#define FN_GLWINDOWPOS2IVMESA(v)	fn.glWindowPos2ivMESA(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLWINDOWPOS2SV(v)	GLshort tmp[2]; \
	Atari2HostShortPtr(2, v, tmp); \
	fn.glWindowPos2sv(tmp)
#else
#define FN_GLWINDOWPOS2SV(v)	fn.glWindowPos2sv(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLWINDOWPOS2SVARB(v)	GLshort tmp[2]; \
	Atari2HostShortPtr(2, v, tmp); \
	fn.glWindowPos2svARB(tmp)
#else
#define FN_GLWINDOWPOS2SVARB(v)	fn.glWindowPos2svARB(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLWINDOWPOS2SVMESA(v)	GLshort tmp[2]; \
	Atari2HostShortPtr(2, v, tmp); \
	fn.glWindowPos2svMESA(tmp)
#else
#define FN_GLWINDOWPOS2SVMESA(v)	fn.glWindowPos2svMESA(v)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLWINDOWPOS3DV(v)	GLdouble tmp[3]; \
	Atari2HostDoublePtr(3, v, tmp); \
	fn.glWindowPos3dv(tmp)
#else
#define FN_GLWINDOWPOS3DV(v)	fn.glWindowPos3dv(v)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLWINDOWPOS3DVARB(v)	GLdouble tmp[3]; \
	Atari2HostDoublePtr(3, v, tmp); \
	fn.glWindowPos3dvARB(tmp)
#else
#define FN_GLWINDOWPOS3DVARB(v)	fn.glWindowPos3dvARB(v)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLWINDOWPOS3DVMESA(v)	GLdouble tmp[3]; \
	Atari2HostDoublePtr(3, v, tmp); \
	fn.glWindowPos3dvMESA(tmp)
#else
#define FN_GLWINDOWPOS3DVMESA(v)	fn.glWindowPos3dvMESA(v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLWINDOWPOS3FV(v)	GLfloat tmp[3]; \
	Atari2HostFloatPtr(3, v, tmp); \
	fn.glWindowPos3fv(tmp)
#else
#define FN_GLWINDOWPOS3FV(v)	fn.glWindowPos3fv(v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLWINDOWPOS3FVARB(v)	GLfloat tmp[3]; \
	Atari2HostFloatPtr(3, v, tmp); \
	fn.glWindowPos3fvARB(tmp)
#else
#define FN_GLWINDOWPOS3FVARB(v)	fn.glWindowPos3fvARB(v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLWINDOWPOS3FVMESA(v)	GLfloat tmp[3]; \
	Atari2HostFloatPtr(3, v, tmp); \
	fn.glWindowPos3fvMESA(tmp)
#else
#define FN_GLWINDOWPOS3FVMESA(v)	fn.glWindowPos3fvMESA(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLWINDOWPOS3IV(v)	GLint tmp[3]; \
	Atari2HostIntPtr(3, v, tmp); \
	fn.glWindowPos3iv(tmp)
#else
#define FN_GLWINDOWPOS3IV(v)	fn.glWindowPos3iv(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLWINDOWPOS3IVARB(v)	GLint tmp[3]; \
	Atari2HostIntPtr(3, v, tmp); \
	fn.glWindowPos3ivARB(tmp)
#else
#define FN_GLWINDOWPOS3IVARB(v)	fn.glWindowPos3ivARB(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLWINDOWPOS3IVMESA(v)	GLint tmp[3]; \
	Atari2HostIntPtr(3, v, tmp); \
	fn.glWindowPos3ivMESA(tmp)
#else
#define FN_GLWINDOWPOS3IVMESA(v)	fn.glWindowPos3ivMESA(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLWINDOWPOS3SV(v)	GLshort tmp[3]; \
	Atari2HostShortPtr(3, v, tmp); \
	fn.glWindowPos3sv(tmp)
#else
#define FN_GLWINDOWPOS3SV(v)	fn.glWindowPos3sv(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLWINDOWPOS3SVARB(v)	GLshort tmp[3]; \
	Atari2HostShortPtr(3, v, tmp); \
	fn.glWindowPos3svARB(tmp)
#else
#define FN_GLWINDOWPOS3SVARB(v)	fn.glWindowPos3svARB(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLWINDOWPOS3SVMESA(v)	GLshort tmp[3]; \
	Atari2HostShortPtr(3, v, tmp); \
	fn.glWindowPos3svMESA(tmp)
#else
#define FN_GLWINDOWPOS3SVMESA(v)	fn.glWindowPos3svMESA(v)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLWINDOWPOS4DVMESA(v)	GLdouble tmp[4]; \
	Atari2HostDoublePtr(4, v, tmp); \
	fn.glWindowPos4dvMESA(tmp)
#else
#define FN_GLWINDOWPOS4DVMESA(v)	fn.glWindowPos4dvMESA(v)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLWINDOWPOS4DVMESA(v)	GLdouble tmp[4]; \
	Atari2HostDoublePtr(4, v, tmp); \
	fn.glWindowPos4dvMESA(tmp)
#else
#define FN_GLWINDOWPOS4DVMESA(v)	fn.glWindowPos4dvMESA(v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLWINDOWPOS4FVMESA(v)	GLfloat tmp[4]; \
	Atari2HostFloatPtr(4, v, tmp); \
	fn.glWindowPos4fvMESA(tmp)
#else
#define FN_GLWINDOWPOS4FVMESA(v)	fn.glWindowPos4fvMESA(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLWINDOWPOS4IVMESA(v)	GLint tmp[4]; \
	Atari2HostIntPtr(4, v, tmp); \
	fn.glWindowPos4ivMESA(tmp)
#else
#define FN_GLWINDOWPOS4IVMESA(v)	fn.glWindowPos4ivMESA(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLWINDOWPOS4SVMESA(v)	GLshort tmp[4]; \
	Atari2HostShortPtr(4, v, tmp); \
	fn.glWindowPos4svMESA(tmp)
#else
#define FN_GLWINDOWPOS4SVMESA(v)	fn.glWindowPos4svMESA(v)
#endif

/* #define FN_GLULOOKAT(eyeX, eyeY, eyeZ, centerX, centerY, centerZ, upX, upY, upZ) */

#include "nfosmesa/call-gl.c"
