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

#define TOS_ENOSYS -32

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
#ifndef GL_DEPTH_STENCIL
#define GL_DEPTH_STENCIL                  0x84F9
#endif
#ifndef GL_RG
#define GL_RG                             0x8227
#endif
#ifndef GL_CONSTANT_COLOR0_NV
#define GL_CONSTANT_COLOR0_NV             0x852A
#endif
#ifndef GL_CONSTANT_COLOR1_NV
#define GL_CONSTANT_COLOR1_NV             0x852B
#endif
#ifndef GL_CULL_VERTEX_EYE_POSITION_EXT
#define GL_CULL_VERTEX_EYE_POSITION_EXT   0x81AB
#endif
#ifndef GL_CULL_VERTEX_OBJECT_POSITION_EXT
#define GL_CULL_VERTEX_OBJECT_POSITION_EXT 0x81AC
#endif
#ifndef GL_HALF_FLOAT
#define GL_HALF_FLOAT 0x140B
#endif
#ifndef GL_ARRAY_BUFFER
#define GL_ARRAY_BUFFER                   0x8892
#endif
#ifndef GL_ATOMIC_COUNTER_BUFFER
#define GL_ATOMIC_COUNTER_BUFFER          0x92C0
#endif
#ifndef GL_COPY_READ_BUFFER
#define GL_COPY_READ_BUFFER               0x8F36
#endif
#ifndef GL_COPY_WRITE_BUFFER
#define GL_COPY_WRITE_BUFFER              0x8F37
#endif
#ifndef GL_DISPATCH_INDIRECT_BUFFER
#define GL_DISPATCH_INDIRECT_BUFFER       0x90EE
#endif
#ifndef GL_DRAW_INDIRECT_BUFFER
#define GL_DRAW_INDIRECT_BUFFER           0x8F3F
#endif
#ifndef GL_ELEMENT_ARRAY_BUFFER
#define GL_ELEMENT_ARRAY_BUFFER           0x8893
#endif
#ifndef GL_PIXEL_PACK_BUFFER
#define GL_PIXEL_PACK_BUFFER              0x88EB
#endif
#ifndef GL_PIXEL_UNPACK_BUFFER
#define GL_PIXEL_UNPACK_BUFFER            0x88EC
#endif
#ifndef GL_QUERY_BUFFER
#define GL_QUERY_BUFFER                   0x9192
#endif
#ifndef GL_SHADER_STORAGE_BUFFER
#define GL_SHADER_STORAGE_BUFFER          0x90D2
#endif
#ifndef GL_TEXTURE_BUFFER
#define GL_TEXTURE_BUFFER                 0x8C2A
#endif
#ifndef GL_TRANSFORM_FEEDBACK_BUFFER
#define GL_TRANSFORM_FEEDBACK_BUFFER      0x8C8E
#endif
#ifndef GL_UNIFORM_BUFFER
#define GL_UNIFORM_BUFFER                 0x8A11
#endif
#ifndef GL_SECONDARY_COLOR_ARRAY
#define GL_SECONDARY_COLOR_ARRAY          0x845E
#endif
#ifndef GL_FIXED
#define GL_FIXED                          0x140C
#endif
#ifndef GL_FRAGMENT_LIGHT_MODEL_AMBIENT_SGIX
#define GL_FRAGMENT_LIGHT_MODEL_AMBIENT_SGIX 0x840A
#endif
#ifndef GL_TEXTURE_CLIPMAP_CENTER_SGIX
#define GL_TEXTURE_CLIPMAP_CENTER_SGIX    0x8171
#endif
#ifndef GL_TEXTURE_CLIPMAP_OFFSET_SGIX
#define GL_TEXTURE_CLIPMAP_OFFSET_SGIX    0x8173
#endif
#ifndef GL_TEXTURE_CLIPMAP_VIRTUAL_DEPTH_SGIX
#define GL_TEXTURE_CLIPMAP_VIRTUAL_DEPTH_SGIX 0x8174
#endif
#ifndef GL_POST_TEXTURE_FILTER_BIAS_SGIX
#define GL_POST_TEXTURE_FILTER_BIAS_SGIX  0x8179
#endif
#ifndef GL_POST_TEXTURE_FILTER_SCALE_SGIX
#define GL_POST_TEXTURE_FILTER_SCALE_SGIX 0x817A
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

#define Host2AtariIntPtr Atari2HostIntPtr

	/* Read parameter on m68k stack */
#define getStackedParameter(n) SDL_SwapBE32(ctx_ptr[n])
#define getStackedParameter64(n) SDL_SwapBE64(*((Uint64 *)&ctx_ptr[n]))
#define getStackedFloat(n) Atari2HostFloat(getStackedParameter(n))
#define getStackedDouble(n) Atari2HostDouble(getStackedParameter(n), getStackedParameter(n + 1))
#define getStackedPointer(n) (ctx_ptr[n] ? Atari2HostAddr(getStackedParameter(n)) : NULL)

	/* undo the effects of Atari2HostAddr for pointer arguments when they specify a buffer offset */
#define Host2AtariAddr(a) ((void *)((uintptr)(a) - MEMBaseDiff))

	if (fncode != NFOSMESA_OSMESAPOSTPROCESS && fncode != GET_VERSION)
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
			PutglGetString(getStackedParameter(0),getStackedParameter(1),(GLubyte *)getStackedPointer(2));
			break;
		case NFOSMESA_LENGLGETSTRINGI:
			ret = LenglGetStringi(getStackedParameter(0),getStackedParameter(1),getStackedParameter(2));
			break;
		case NFOSMESA_PUTGLGETSTRINGI:
			PutglGetStringi(getStackedParameter(0),getStackedParameter(1),getStackedParameter(2),(GLubyte *)getStackedPointer(3));
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
			OSMesaGetIntegerv(getStackedParameter(0), (GLint *)getStackedPointer(1));
			break;
		case NFOSMESA_OSMESAGETDEPTHBUFFER:
			ret = OSMesaGetDepthBuffer(getStackedParameter(0), (GLint *)getStackedPointer(1), (GLint *)getStackedPointer(2), (GLint *)getStackedPointer(3), (void **)getStackedPointer(4));
			break;
		case NFOSMESA_OSMESAGETCOLORBUFFER:
			ret = OSMesaGetColorBuffer(getStackedParameter(0), (GLint *)getStackedPointer(1), (GLint *)getStackedPointer(2), (GLint *)getStackedPointer(3), (void **)getStackedPointer(4));
			break;
		case NFOSMESA_OSMESAGETPROCADDRESS:
			/* FIXME: Native side do not need this */
			ret = (int32) (0 != OSMesaGetProcAddress((const char *)getStackedPointer(0)));
			break;
		case NFOSMESA_OSMESACOLORCLAMP:
			if (!fn.OSMesaColorClamp || GL_ISNOP(fn.OSMesaColorClamp))
				ret = TOS_ENOSYS;
			else
				OSMesaColorClamp(getStackedParameter(0));
			break;
		case NFOSMESA_OSMESAPOSTPROCESS:
			if (!fn.OSMesaPostprocess || GL_ISNOP(fn.OSMesaPostprocess))
				ret = TOS_ENOSYS;
			else
				OSMesaPostprocess(getStackedParameter(0), (const char *)getStackedPointer(1), getStackedParameter(2));
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
			ret = TOS_ENOSYS;
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
	if (sharelist) {
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
	memset((void *)&(contexts[j]),0,sizeof(context_t));

	share_ctx = NULL;
	if (sharelist > 0 && sharelist <= MAX_OSMESA_CONTEXTS) {
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
	contexts[j].render_mode = GL_RENDER;

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
		contexts[ctx].src_buffer = NULL;
	}
	if (contexts[ctx].feedback_buffer_host) {
		free(contexts[ctx].feedback_buffer_host);
		contexts[ctx].feedback_buffer_host = NULL;
	}
	contexts[ctx].feedback_buffer_type = 0;
	contexts[ctx].ctx = NULL;
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
	if (value)
		*value = SDL_SwapBE32(tmp);
}

GLboolean OSMesaDriver::OSMesaGetDepthBuffer(Uint32 c, GLint *width, GLint *height, GLint *bytesPerValue, void **buffer )
{
	D(bug("nfosmesa: OSMesaGetDepthBuffer"));
	SelectContext(c);
	if (width)
		*width = 0;
	if (height)
		*height = 0;
	if (bytesPerValue)
		*bytesPerValue = 0;
	if (buffer)
		*buffer = NULL;	/* Can not return pointer in host memory */
	return GL_FALSE;
}

GLboolean OSMesaDriver::OSMesaGetColorBuffer(Uint32 c, GLint *width, GLint *height, GLint *format, void **buffer )
{
	D(bug("nfosmesa: OSMesaGetColorBuffer(%u)", c));
	SelectContext(c);
	if (width)
		*width = 0;
	if (height)
		*height = 0;
	if (format)
		*format = 0;
	if (buffer)
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
	if (buffer)
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
	if (buffer)
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
	{ \
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


void *OSMesaDriver::convertPixels(GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid *pixels)
{
	GLsizei size, count;
	GLubyte *ptr;
	void *result;
	
	if (pixels == NULL)
		return NULL;
	switch (type)
	{
	case GL_UNSIGNED_BYTE:
	case GL_BYTE:
	case GL_UNSIGNED_BYTE_3_3_2:
	case GL_UNSIGNED_BYTE_2_3_3_REV:
    case GL_2_BYTES:
    case GL_3_BYTES:
    case GL_4_BYTES:
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
    case GL_HALF_FLOAT:
	case 2:
		size = sizeof(GLushort);
		break;
	case GL_UNSIGNED_INT:
	case GL_INT:
	case GL_UNSIGNED_INT_8_8_8_8:
	case GL_UNSIGNED_INT_8_8_8_8_REV:
	case GL_UNSIGNED_INT_10_10_10_2:
	case GL_UNSIGNED_INT_2_10_10_10_REV:
	case GL_FIXED:
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
	case GL_STENCIL_INDEX:
	case GL_DEPTH_COMPONENT:
	case 1:
		count = 1;
		break;
	case GL_LUMINANCE_ALPHA:
	case GL_DEPTH_STENCIL:
	case GL_RG:
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
	
	/* FIXME: glPixelStore parameters are not taken into account */
	count *= width * height * depth;
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

void *OSMesaDriver::convertArray(GLsizei count, GLenum type, const GLvoid *pixels)
{
	return convertPixels(count, 1, 1, 1, type, pixels);
}


void OSMesaDriver::setupClientArray(vertexarray_t &array, GLint size, GLenum type, GLsizei stride, GLsizei count, GLint ptrstride, const GLvoid *pointer)
{
	array.size = size;
	array.type = type;
	array.count = count;
	array.ptrstride = ptrstride;
	GLsizei atari_defstride;
	GLsizei basesize;
	switch (type)
	{
	case GL_UNSIGNED_BYTE:
	case GL_BYTE:
	case GL_UNSIGNED_BYTE_3_3_2:
	case GL_UNSIGNED_BYTE_2_3_3_REV:
    case GL_2_BYTES:
    case GL_3_BYTES:
    case GL_4_BYTES:
	case 1:
		array.basesize = sizeof(Uint8);
		basesize = sizeof(GLubyte);
		break;
	case GL_SHORT:
	case GL_UNSIGNED_SHORT_5_6_5:
	case GL_UNSIGNED_SHORT_5_6_5_REV:
	case GL_UNSIGNED_SHORT_4_4_4_4:
	case GL_UNSIGNED_SHORT_4_4_4_4_REV:
	case GL_UNSIGNED_SHORT_5_5_5_1:
	case GL_UNSIGNED_SHORT_1_5_5_5_REV:
    case GL_HALF_FLOAT:
	case 2:
		array.basesize = sizeof(Uint16);
		basesize = sizeof(GLshort);
		break;
	case GL_UNSIGNED_INT:
	case GL_INT:
	case GL_UNSIGNED_INT_8_8_8_8:
	case GL_UNSIGNED_INT_8_8_8_8_REV:
	case GL_UNSIGNED_INT_10_10_10_2:
	case GL_UNSIGNED_INT_2_10_10_10_REV:
	case GL_FIXED:
	case 4:
		array.basesize = sizeof(Uint32);
		basesize = sizeof(GLint);
		break;
	case GL_FLOAT:
		array.basesize = ATARI_SIZEOF_FLOAT;
		basesize = sizeof(GLfloat);
		break;
	case GL_DOUBLE:
		array.basesize = ATARI_SIZEOF_DOUBLE;
		basesize = sizeof(GLdouble);
		break;
	default:
		glSetError(GL_INVALID_ENUM);
		return;
	}
	atari_defstride = size * array.basesize;
	if (stride == 0)
	{
		stride = atari_defstride;
	}
	array.atari_stride = stride;
	array.host_stride = size * basesize;
	array.defstride = atari_defstride;
	if (array.host_pointer != array.atari_pointer)
	{
		free(array.host_pointer);
		array.host_pointer = NULL;
	}
	array.atari_pointer = pointer;
	array.converted = 0;
	array.vendor = 0;
}


void OSMesaDriver::convertClientArrays(GLsizei count)
{
	if (contexts[cur_context].enabled_arrays & NFOSMESA_VERTEX_ARRAY)
	{
		vertexarray_t &array = contexts[cur_context].vertex;
		convertClientArray(count, array);
		if (array.vendor == 1)
			fn.glVertexPointerListIBM(array.size, array.type, array.host_stride, (const void **)array.host_pointer, array.ptrstride);
		else if (array.vendor == 2)
			fn.glVertexPointervINTEL(array.size, array.type, (const void **)array.host_pointer);
		else if (array.count > 0)
			fn.glVertexPointerEXT(array.size, array.type, array.host_stride, array.count, array.host_pointer);
		else
			fn.glVertexPointer(array.size, array.type, array.host_stride, array.host_pointer);
	}
	if (contexts[cur_context].enabled_arrays & NFOSMESA_NORMAL_ARRAY)
	{
		vertexarray_t &array = contexts[cur_context].normal;
		convertClientArray(count, array);
		if (array.vendor == 1)
			fn.glNormalPointerListIBM(array.type, array.host_stride, (const void **)array.host_pointer, array.ptrstride);
		else if (array.vendor == 2)
			fn.glNormalPointervINTEL(array.type, (const void **)array.host_pointer);
		else if (array.count > 0)
			fn.glNormalPointerEXT(array.type, array.host_stride, array.count, array.host_pointer);
		else
			fn.glNormalPointer(array.type, array.host_stride, array.host_pointer);
	}
	if (contexts[cur_context].enabled_arrays & NFOSMESA_COLOR_ARRAY)
	{
		vertexarray_t &array = contexts[cur_context].color;
		convertClientArray(count, array);
		if (array.vendor == 1)
			fn.glColorPointerListIBM(array.size, array.type, array.host_stride, (const void **)array.host_pointer, array.ptrstride);
		else if (array.vendor == 2)
			fn.glColorPointervINTEL(array.size, array.type, (const void **)array.host_pointer);
		if (array.count > 0)
			fn.glColorPointerEXT(array.size, array.type, array.host_stride, array.count, array.host_pointer);
		else
			fn.glColorPointer(array.size, array.type, array.host_stride, array.host_pointer);
	}
	if (contexts[cur_context].enabled_arrays & NFOSMESA_INDEX_ARRAY)
	{
		vertexarray_t &array = contexts[cur_context].index;
		convertClientArray(count, array);
		if (array.vendor == 1)
			fn.glIndexPointerListIBM(array.type, array.host_stride, (const void **)array.host_pointer, array.ptrstride);
		else if (array.count > 0)
			fn.glIndexPointerEXT(array.type, array.host_stride, array.count, array.host_pointer);
		else
			fn.glIndexPointer(array.type, array.host_stride, array.host_pointer);
	}
	if (contexts[cur_context].enabled_arrays & NFOSMESA_EDGEFLAG_ARRAY)
	{
		vertexarray_t &array = contexts[cur_context].edgeflag;
		convertClientArray(count, array);
		if (array.vendor == 1)
			fn.glEdgeFlagPointerListIBM(array.host_stride, (const GLboolean **)array.host_pointer, array.ptrstride);
		else if (array.count > 0)
			fn.glEdgeFlagPointerEXT(array.host_stride, array.count, (const GLboolean *)array.host_pointer);
		else
			fn.glEdgeFlagPointer(array.host_stride, array.host_pointer);
	}
	if (contexts[cur_context].enabled_arrays & NFOSMESA_TEXCOORD_ARRAY)
	{
		vertexarray_t &array = contexts[cur_context].texcoord;
		convertClientArray(count, array);
		if (array.vendor == 1)
			fn.glTexCoordPointerListIBM(array.size, array.type, array.host_stride, (const void **)array.host_pointer, array.ptrstride);
		else if (array.vendor == 2)
			fn.glTexCoordPointervINTEL(array.size, array.type, (const void **)array.host_pointer);
		else if (array.count > 0)
			fn.glTexCoordPointerEXT(array.size, array.type, array.host_stride, array.count, array.host_pointer);
		else
			fn.glTexCoordPointer(array.size, array.type, array.host_stride, array.host_pointer);
	}
	if (contexts[cur_context].enabled_arrays & NFOSMESA_FOGCOORD_ARRAY)
	{
		vertexarray_t &array = contexts[cur_context].fogcoord;
		convertClientArray(count, array);
		if (array.vendor == 1)
			fn.glFogCoordPointerListIBM(array.type, array.host_stride, (const void **)array.host_pointer, array.ptrstride);
		else if (array.count >= 0)
			fn.glFogCoordPointerEXT(array.type, array.host_stride, array.host_pointer);
		else
			fn.glFogCoordPointer(array.type, array.host_stride, array.host_pointer);
	}
	if (contexts[cur_context].enabled_arrays & NFOSMESA_2NDCOLOR_ARRAY)
	{
		vertexarray_t &array = contexts[cur_context].secondary_color;
		convertClientArray(count, array);
		if (array.vendor == 1)
			fn.glSecondaryColorPointerListIBM(array.size, array.type, array.host_stride, (const void **)array.host_pointer, array.ptrstride);
		else if (array.count >= 0)
			fn.glSecondaryColorPointerEXT(array.size, array.type, array.host_stride, array.host_pointer);
		else
			fn.glSecondaryColorPointer(array.size, array.type, array.host_stride, array.host_pointer);
	}
	if (contexts[cur_context].enabled_arrays & NFOSMESA_ELEMENT_ARRAY)
	{
		vertexarray_t &array = contexts[cur_context].element;
		convertClientArray(count, array);
		if (array.vendor == 1)
			fn.glElementPointerAPPLE(array.type, array.host_pointer);
		else if (array.vendor == 2)
			fn.glElementPointerATI(array.type, array.host_pointer);
	}
}

void OSMesaDriver::convertClientArray(GLsizei count, vertexarray_t &array)
{
	if (array.count > 0 && count > array.count)
	{
		D(bug("nfosmesa: convertClientArray: count %d might exceed array size %d", count, array.count));
	}
	if (array.host_pointer == NULL || count > array.converted)
	{
		if (array.basesize != 1 || array.atari_stride != array.defstride)
		{
			array.host_pointer = realloc(array.host_pointer, count * array.host_stride);
			GLsizei n = count - array.converted;
			for (GLsizei i = 0; i < n; i++)
			{
				const char *src = (const char *)array.atari_pointer + array.converted * array.atari_stride;
				char *dst = (char *)array.host_pointer + array.converted * array.host_stride;
				if (array.type == GL_FLOAT)
					Atari2HostFloatArray(array.size, (const Uint32 *)src, (GLfloat *)dst);
				else if (array.type == GL_DOUBLE)
					Atari2HostDoubleArray(array.size, (const Uint32 *)src, (GLdouble *)dst);
				else if (array.basesize == 1)
					memcpy(dst, src, array.size);
				else if (array.basesize == 2)
					Atari2HostShortArray(array.size, (const Uint16 *)src, (GLushort *)dst);
				else /* if (array.basesize == 4) */
					Atari2HostIntArray(array.size, (const Uint32 *)src, (GLuint *)dst);
				array.converted++;
			}
		} else
		{
			array.host_pointer = (void *)array.atari_pointer;
			array.converted = count;
		}
	}
}

void OSMesaDriver::nfglArrayElementHelper(GLint i)
{
	convertClientArrays(i + 1);
	fn.glArrayElement(i);
}

void OSMesaDriver::nfglInterleavedArraysHelper(GLenum format, GLsizei stride, const GLvoid *pointer)
{
	SDL_bool enable_texcoord, enable_normal, enable_color;
	int texcoord_size, color_size, vertex_size;
	const GLubyte *texcoord_ptr,*normal_ptr,*color_ptr,*vertex_ptr;
	GLenum color_type;
	int c, f;
	int defstride;
	
	f = ATARI_SIZEOF_FLOAT;
	c = f * ((4 * sizeof(GLubyte) + (f - 1)) / f);
	
	enable_texcoord=SDL_FALSE;
	enable_normal=SDL_FALSE;
	enable_color=SDL_FALSE;
	texcoord_size=color_size=vertex_size=0;
	texcoord_ptr=normal_ptr=color_ptr=vertex_ptr=(const GLubyte *)pointer;
	color_type = GL_FLOAT;
	switch(format) {
		case GL_V2F:
			vertex_size = 2;
			defstride = 2 * f;
			break;
		case GL_V3F:
			vertex_size = 3;
			defstride = 3 * f;
			break;
		case GL_C4UB_V2F:
			vertex_size = 2;
			color_size = 4;
			enable_color = SDL_TRUE;
			vertex_ptr += c;
			color_type = GL_UNSIGNED_BYTE;
			defstride = c + 2 * f;
			break;
		case GL_C4UB_V3F:
			vertex_size = 3;
			color_size = 4;
			enable_color = SDL_TRUE;
			vertex_ptr += c;
			color_type = GL_UNSIGNED_BYTE;
			defstride = c + 3 * f;
			break;
		case GL_C3F_V3F:
			vertex_size = 3;
			color_size = 3;
			enable_color = SDL_TRUE;
			vertex_ptr += 3 * f;
			defstride = 6 * f;
			break;
		case GL_N3F_V3F:
			vertex_size = 3;
			enable_normal = SDL_TRUE;
			vertex_ptr += 3 * f;
			defstride = 6 * f;
			break;
		case GL_C4F_N3F_V3F:
			vertex_size = 3;
			color_size = 4;
			enable_color = SDL_TRUE;
			enable_normal = SDL_TRUE;
			vertex_ptr += 7 * f;
			normal_ptr += 4 * f;
			defstride = 10 * f;
			break;
		case GL_T2F_V3F:
			vertex_size = 3;
			texcoord_size = 2;
			enable_texcoord = SDL_TRUE;
			vertex_ptr += 2 * f;
			defstride = 5 * f;
			break;
		case GL_T4F_V4F:
			vertex_size = 4;
			texcoord_size = 4;
			enable_texcoord = SDL_TRUE;
			vertex_ptr += 4 * f;
			defstride = 8 * f;
			break;
		case GL_T2F_C4UB_V3F:
			vertex_size = 3;
			texcoord_size = 2;
			color_size = 4;
			enable_texcoord = SDL_TRUE;
			enable_color = SDL_TRUE;
			vertex_ptr += c + 2 * f;
			color_ptr += 2 * f;
			color_type = GL_UNSIGNED_BYTE;
			defstride = c + 5 * f;
			break;
		case GL_T2F_C3F_V3F:
			vertex_size = 3;
			texcoord_size = 2;
			color_size = 3;
			enable_texcoord = SDL_TRUE;
			enable_color = SDL_TRUE;
			vertex_ptr += 5 * f;
			color_ptr += 2 * f;
			defstride = 8 * f;
			break;
		case GL_T2F_N3F_V3F:
			vertex_size = 3;
			texcoord_size = 2;
			enable_normal = SDL_TRUE;
			enable_texcoord = SDL_TRUE;
			vertex_ptr += 5 * f;
			normal_ptr += 2 * f;
			defstride = 8 * f;
			break;
		case GL_T2F_C4F_N3F_V3F:
			vertex_size = 3;
			texcoord_size = 2;
			color_size = 4;
			enable_normal = SDL_TRUE;
			enable_texcoord = SDL_TRUE;
			enable_color = SDL_TRUE;
			vertex_ptr += 9 * f;
			normal_ptr += 6 * f;
			color_ptr += 2 * f;
			defstride = 12 * f;
			break;
		case GL_T4F_C4F_N3F_V4F:
			vertex_size = 4;
			texcoord_size = 4;
			color_size = 4;
			enable_normal = SDL_TRUE;
			enable_texcoord = SDL_TRUE;
			enable_color = SDL_TRUE;
			vertex_ptr += 11 * f;
			normal_ptr += 8 * f;
			color_ptr += 4 * f;
			defstride = 15 * f;
			break;
		default:
			glSetError(GL_INVALID_ENUM);
			return;
	}

	if (stride==0)
	{
		stride = defstride;
	}

	/*
	 * FIXME:
	 * calling the dispatch functions here would trace
	 * calls to functions the client did not call directly
	 */
	nfglDisableClientState(GL_EDGE_FLAG_ARRAY);
	nfglDisableClientState(GL_INDEX_ARRAY);
	if(enable_normal) {
		nfglEnableClientState(GL_NORMAL_ARRAY);
		nfglNormalPointer(GL_FLOAT, stride, normal_ptr);
	} else {
		nfglDisableClientState(GL_NORMAL_ARRAY);
	}
	if(enable_color) {
		nfglEnableClientState(GL_COLOR_ARRAY);
		nfglColorPointer(color_size, color_type, stride, color_ptr);
	} else {
		nfglDisableClientState(GL_COLOR_ARRAY);
	}
	if(enable_texcoord) {
		nfglEnableClientState(GL_TEXTURE_COORD_ARRAY);
		nfglTexCoordPointer(texcoord_size, GL_FLOAT, stride, texcoord_ptr);
	} else {
		nfglDisableClientState(GL_TEXTURE_COORD_ARRAY);
	}
	nfglEnableClientState(GL_VERTEX_ARRAY);
	nfglVertexPointer(vertex_size, GL_FLOAT, stride, vertex_ptr);
}

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
	case GL_TEXTURE_CLIPMAP_CENTER_SGIX:
	case GL_TEXTURE_CLIPMAP_OFFSET_SGIX:
	case GL_MAP1_TEXTURE_COORD_2:
	case GL_MAP2_TEXTURE_COORD_2:
		count = 2;
		break;
	case GL_CURRENT_NORMAL:
	case GL_POINT_DISTANCE_ATTENUATION:
	case GL_SPOT_DIRECTION:
	case GL_COLOR_INDEXES:
	case GL_TEXTURE_CLIPMAP_VIRTUAL_DEPTH_SGIX:
	case GL_MAP1_NORMAL:
	case GL_MAP1_TEXTURE_COORD_3:
	case GL_MAP1_VERTEX_3:
	case GL_MAP2_NORMAL:
	case GL_MAP2_TEXTURE_COORD_3:
	case GL_MAP2_VERTEX_3:
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
	case GL_CONSTANT_COLOR0_NV:
	case GL_CONSTANT_COLOR1_NV:
	case GL_CULL_VERTEX_OBJECT_POSITION_EXT:
	case GL_CULL_VERTEX_EYE_POSITION_EXT:
	case GL_AMBIENT:
	case GL_DIFFUSE:
	case GL_SPECULAR:
	case GL_POSITION:
	case GL_EMISSION:
	case GL_AMBIENT_AND_DIFFUSE:
	case GL_FRAGMENT_LIGHT_MODEL_AMBIENT_SGIX:
	case GL_CONVOLUTION_FILTER_SCALE:
	case GL_CONVOLUTION_FILTER_BIAS:
	case GL_CONVOLUTION_BORDER_COLOR:
	case GL_TEXTURE_BORDER_COLOR:
	case GL_POST_TEXTURE_FILTER_BIAS_SGIX:
	case GL_POST_TEXTURE_FILTER_SCALE_SGIX:
	case GL_TEXTURE_ENV_COLOR:
	case GL_OBJECT_PLANE:
	case GL_EYE_PLANE:
	case GL_MAP1_COLOR_4:
	case GL_MAP1_TEXTURE_COORD_4:
	case GL_MAP1_VERTEX_4:
	case GL_MAP2_COLOR_4:
	case GL_MAP2_TEXTURE_COORD_4:
	case GL_MAP2_VERTEX_4:
	case GL_COLOR_TABLE_SCALE:
	case GL_COLOR_TABLE_BIAS:
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
	 \
	if(n<=0 || !lists) { \
		return; \
	} \
	tmp = convertArray(n, type, lists); \
	fn.glCallLists(n, type, tmp); \
	if (tmp != lists) free(tmp)

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

#define FN_GLCOLORPOINTER(size, type, stride, pointer) \
	setupClientArray(contexts[cur_context].color, size, type, stride, -1, 0, pointer)

#define FN_GLCOLORPOINTEREXT(size, type, stride, count, pointer) \
	setupClientArray(contexts[cur_context].color, size, type, stride, count, 0, pointer)

#define FN_GLCOLORPOINTERLISTIBM(size, type, stride, pointer, ptrstride) \
	setupClientArray(contexts[cur_context].color, size, type, stride, -1, ptrstride, pointer); \
	contexts[cur_context].color.vendor = 1

#define FN_GLCOLORPOINTERVINTEL(size, type, pointer) \
	setupClientArray(contexts[cur_context].color, size, type, 0, -1, 0, pointer); \
	contexts[cur_context].color.vendor = 2

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV || NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLCOLORSUBTABLE(target, start, count, format, type, data) \
	void *tmp = convertPixels(count, 1, 1, format, type, data); \
	fn.glColorSubTable(target, start, count, format, type, tmp); \
	if (tmp != data) free(tmp)
#else
#define FN_GLCOLORSUBTABLE(target, start, count, format, type, data)	fn.glColorSubTable(target, start, count, format, type, data)
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV || NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLCOLORTABLE(target, internalformat, width, format, type, table) \
	void *tmp = convertPixels(width, 1, 1, format, type, table); \
	fn.glColorTable(target, internalformat, width, format, type, tmp); \
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
		case GL_FOG_COORDINATE_ARRAY: \
			contexts[cur_context].enabled_arrays &= ~NFOSMESA_FOGCOORD_ARRAY; \
			break; \
		case GL_SECONDARY_COLOR_ARRAY: \
			contexts[cur_context].enabled_arrays &= ~NFOSMESA_2NDCOLOR_ARRAY; \
			break; \
		case GL_ELEMENT_ARRAY_APPLE: \
			contexts[cur_context].enabled_arrays &= ~NFOSMESA_ELEMENT_ARRAY; \
			break; \
	} \
	fn.glDisableClientState(array)

#define FN_GLDRAWARRAYS(mode, first, count) \
	convertClientArrays(first + count); \
	fn.glDrawArrays(mode, first, count)

#define FN_GLDRAWARRAYSEXT(mode, first, count) \
	convertClientArrays(first + count); \
	fn.glDrawArraysEXT(mode, first, count)

#define FN_GLDRAWARRAYSINSTANCED(mode, first, count, instancecount) \
	convertClientArrays(first + count); \
	fn.glDrawArraysInstanced(mode, first, count, instancecount)

#define FN_GLDRAWELEMENTS(mode, count, type, indices) \
	void *tmp; \
	convertClientArrays(count); \
	switch(type) { \
	case GL_UNSIGNED_BYTE: \
	case GL_UNSIGNED_SHORT: \
	case GL_UNSIGNED_INT: \
		tmp = convertArray(count, type, indices); \
		break; \
	default: \
		glSetError(GL_INVALID_ENUM); \
		return; \
	} \
	fn.glDrawElements(mode, count, type, tmp); \
	if (tmp != indices) free(tmp)

#define FN_GLDRAWELEMENTSBASEVERTEX(mode, count, type, indices, basevertex) \
	void *tmp; \
	convertClientArrays(count); \
	switch(type) { \
	case GL_UNSIGNED_BYTE: \
	case GL_UNSIGNED_SHORT: \
	case GL_UNSIGNED_INT: \
		tmp = convertArray(count, type, indices); \
		break; \
	default: \
		glSetError(GL_INVALID_ENUM); \
		return; \
	} \
	fn.glDrawElementsBaseVertex(mode, count, type, tmp, basevertex); \
	if (tmp != indices) free(tmp)


#define FN_GLDRAWRANGEELEMENTS(mode, start, end, count, type, indices) \
	void *tmp; \
	convertClientArrays(count); \
	switch(type) { \
	case GL_UNSIGNED_BYTE: \
	case GL_UNSIGNED_SHORT: \
	case GL_UNSIGNED_INT: \
		tmp = convertArray(count, type, indices); \
		break; \
	default: \
		glSetError(GL_INVALID_ENUM); \
		return; \
	} \
	fn.glDrawRangeElements(mode, start, end, count, type, indices); \
	if (tmp != indices) free(tmp)

#define FN_GLDRAWRANGEELEMENTSEXT(mode, start, end, count, type, indices) \
	void *tmp; \
	convertClientArrays(count); \
	switch(type) { \
	case GL_UNSIGNED_BYTE: \
	case GL_UNSIGNED_SHORT: \
	case GL_UNSIGNED_INT: \
		tmp = convertArray(count, type, indices); \
		break; \
	default: \
		glSetError(GL_INVALID_ENUM); \
		return; \
	} \
	fn.glDrawRangeElementsEXT(mode, start, end, count, type, indices); \
	if (tmp != indices) free(tmp)

#define FN_GLDRAWRANGEELEMENTSBASEVERTEX(mode, start, end, count, type, indices, basevertex) \
	void *tmp; \
	convertClientArrays(count); \
	switch(type) { \
	case GL_UNSIGNED_BYTE: \
	case GL_UNSIGNED_SHORT: \
	case GL_UNSIGNED_INT: \
		tmp = convertArray(count, type, indices); \
		break; \
	default: \
		glSetError(GL_INVALID_ENUM); \
		return; \
	} \
	fn.glDrawRangeElementsBaseVertex(mode, start, end, count, type, indices, basevertex); \
	if (tmp != indices) free(tmp)

#define FN_GLEDGEFLAGPOINTER(stride, pointer) \
	setupClientArray(contexts[cur_context].edgeflag, 1, GL_UNSIGNED_BYTE, stride, -1, 0, pointer)

#define FN_GLEDGEFLAGPOINTEREXT(stride, count, pointer) \
	setupClientArray(contexts[cur_context].edgeflag, 1, GL_UNSIGNED_BYTE, stride, count, 0, pointer)

#define FN_GLEDGEFLAGPOINTERLISTIBM(stride, pointer, ptrstride) \
	setupClientArray(contexts[cur_context].edgeflag, 1, GL_UNSIGNED_BYTE, stride, -1, ptrstride, pointer); \
	contexts[cur_context].edgeflag.vendor = 1

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
		case GL_FOG_COORDINATE_ARRAY: \
			contexts[cur_context].enabled_arrays |= NFOSMESA_FOGCOORD_ARRAY; \
			break; \
		case GL_SECONDARY_COLOR_ARRAY: \
			contexts[cur_context].enabled_arrays |= NFOSMESA_2NDCOLOR_ARRAY; \
			break; \
		case GL_ELEMENT_ARRAY_APPLE: \
			contexts[cur_context].enabled_arrays |= NFOSMESA_ELEMENT_ARRAY; \
			break; \
	} \
	fn.glEnableClientState(array)

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
	int size = nfglGetNumParams(pname); \
	Atari2HostFloatPtr(size, params, tmp); \
	fn.glFogfv(pname, tmp)
#else
#define FN_GLFOGFV(pname, params)	fn.glFogfv(pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLFOGIV(pname, params) \
	GLint tmp[4]; \
	int size = nfglGetNumParams(pname); \
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

#define FN_GLINDEXPOINTER(type, stride, pointer) \
	setupClientArray(contexts[cur_context].index, 1, type, stride, -1, 0, pointer)

#define FN_GLINDEXPOINTEREXT(type, stride, count, pointer) \
	setupClientArray(contexts[cur_context].index, 1, type, stride, count, 0, pointer)

#define FN_GLINDEXPOINTERLISTIBM(type, stride, pointer, ptrstride) \
	setupClientArray(contexts[cur_context].index, 1, type, stride, -1, ptrstride, pointer); \
	contexts[cur_context].index.vendor = 1

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
	int size = nfglGetNumParams(pname); \
	Atari2HostFloatPtr(size, params,tmp); \
	fn.glLightModelfv(pname, tmp)
#else
#define FN_GLLIGHTMODELFV(pname, params)	fn.glLightModelfv(pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLLIGHTMODELIV(pname, params) \
	GLint tmp[4]; \
	int size = nfglGetNumParams(pname); \
	Atari2HostIntPtr(size, params, tmp); \
	fn.glLightModeliv(pname, tmp)
#else
#define FN_GLLIGHTMODELIV(pname, params)	fn.glLightModeliv(pname, params)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLLIGHTFV(light, pname, params) \
	GLfloat tmp[4]; \
	int size = nfglGetNumParams(pname); \
	Atari2HostFloatPtr(size, params, tmp); \
	fn.glLightfv(light, pname, tmp)
#else
#define FN_GLLIGHTFV(light, pname, params)	fn.glLightfv(light, pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLLIGHTIV(light, pname, params) \
	GLint tmp[4]; \
	int size = nfglGetNumParams(pname); \
	Atari2HostIntPtr(size, params, tmp); \
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

#define FN_GLNORMALPOINTER(type, stride, pointer) \
	setupClientArray(contexts[cur_context].normal, 3, type, stride, -1, 0, pointer)

#define FN_GLNORMALPOINTEREXT(type, stride, count, pointer) \
	setupClientArray(contexts[cur_context].normal, 3, type, stride, count, 0, pointer)

#define FN_GLNORMALPOINTERLISTIBM(type, stride, pointer, ptrstride) \
	setupClientArray(contexts[cur_context].normal, 3, type, stride, -1, ptrstride, pointer); \
	contexts[cur_context].normal.vendor = 1

#define FN_GLNORMALPOINTERVINTEL(type, pointer) \
	setupClientArray(contexts[cur_context].normal, 3, type, 0, -1, 0, pointer); \
	contexts[cur_context].normal.vendor = 2

#define FN_GLFOGCOORDPOINTER(type, stride, pointer) \
	setupClientArray(contexts[cur_context].fogcoord, 1, type, stride, -1, 0, pointer)

#define FN_GLFOGCOORDPOINTEREXT(type, stride, pointer) \
	setupClientArray(contexts[cur_context].fogcoord, 1, type, stride, 0, 0, pointer)

#define FN_GLFOGCOORDPOINTERLISTIBM(type, stride, pointer, ptrstride) \
	setupClientArray(contexts[cur_context].fogcoord, 1, type, stride, -1, ptrstride, pointer); \
	contexts[cur_context].fogcoord.vendor = 1

#define FN_GLSECONDARYCOLORPOINTER(size, type, stride, pointer) \
	setupClientArray(contexts[cur_context].secondary_color, size, type, stride, -1, 0, pointer)

#define FN_GLSECONDARYCOLORPOINTEREXT(size, type, stride, pointer) \
	setupClientArray(contexts[cur_context].secondary_color, size, type, stride, 0, 0, pointer)

#define FN_GLSECONDARYCOLORPOINTERLISTIBM(size, type, stride, pointer, ptrstride) \
	setupClientArray(contexts[cur_context].secondary_color, size, type, stride, -1, ptrstride, pointer); \
	contexts[cur_context].secondary_color.vendor = 1

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

#define FN_GLTEXCOORDPOINTER(size, type, stride, pointer) \
	setupClientArray(contexts[cur_context].texcoord, size, type, stride, -1, 0, pointer)

#define FN_GLTEXCOORDPOINTEREXT(size, type, stride, count, pointer) \
	setupClientArray(contexts[cur_context].texcoord, size, type, stride, count, 0, pointer)

#define FN_GLTEXCOORDPOINTERLISTIBM(size, type, stride, pointer, ptrstride) \
	setupClientArray(contexts[cur_context].texcoord, size, type, stride, -1, ptrstride, pointer); \
	contexts[cur_context].texcoord.vendor = 1

#define FN_GLTEXCOORDPOINTERVINTEL(size, type, pointer) \
	setupClientArray(contexts[cur_context].texcoord, size, type, 0, -1, 0, pointer); \
	contexts[cur_context].texcoord.vendor = 2

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

#define FN_GLVERTEXPOINTER(size, type, stride, pointer) \
	setupClientArray(contexts[cur_context].vertex, size, type, stride, -1, 0, pointer)

#define FN_GLVERTEXPOINTEREXT(size, type, stride, count, pointer) \
	setupClientArray(contexts[cur_context].vertex, size, type, stride, count, 0, pointer)

#define FN_GLVERTEXPOINTERLISTIBM(size, type, stride, pointer, ptrstride) \
	setupClientArray(contexts[cur_context].vertex, size, type, stride, -1, ptrstride, pointer); \
	contexts[cur_context].vertex.vendor = 1

#define FN_GLVERTEXPOINTERVINTEL(size, type, pointer) \
	setupClientArray(contexts[cur_context].vertex, size, type, 0, -1, 0, pointer); \
	contexts[cur_context].vertex.vendor = 2

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

#if NFOSMESA_NEED_INT_CONV
#define FN_GLAREPROGRAMSRESIDENTNV(n, programs, residences) \
	GLboolean res; \
	if (programs) { \
		GLuint *tmp = (GLuint *)malloc(n * sizeof(*tmp)); \
		if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return 0; } \
		Atari2HostIntArray(n, programs, tmp); \
		res = fn.glAreProgramsResidentNV(n, tmp, residences); \
		free(tmp); \
	} else { \
		res = fn.glAreProgramsResidentNV(n, programs, residences); \
	} \
	return res
#else
#define FN_GLAREPROGRAMSRESIDENTNV(n, programs, residences)	return fn.glAreProgramsResidentNV(n, programs, residences)
#endif

void OSMesaDriver::gl_bind_buffer(GLenum target, GLuint buffer, GLuint first, GLuint count)
{
	fbo_buffer *fbo;
	
	switch (target) {
	case GL_ARRAY_BUFFER:
		fbo = &contexts[cur_context].buffer_bindings.array;
		break;
	case GL_ATOMIC_COUNTER_BUFFER:
		fbo = &contexts[cur_context].buffer_bindings.atomic_counter;
		break;
	case GL_COPY_READ_BUFFER:
		fbo = &contexts[cur_context].buffer_bindings.copy_read;
		contexts[cur_context].buffer_bindings.copy_read.first = first;
		contexts[cur_context].buffer_bindings.copy_read.first = count;
		break;
	case GL_COPY_WRITE_BUFFER:
		fbo = &contexts[cur_context].buffer_bindings.copy_write;
		break;
	case GL_DISPATCH_INDIRECT_BUFFER:
		fbo = &contexts[cur_context].buffer_bindings.dispatch_indirect;
		break;
	case GL_DRAW_INDIRECT_BUFFER:
		fbo = &contexts[cur_context].buffer_bindings.draw_indirect;
		break;
	case GL_ELEMENT_ARRAY_BUFFER:
		fbo = &contexts[cur_context].buffer_bindings.element_array;
		break;
	case GL_PIXEL_PACK_BUFFER:
		fbo = &contexts[cur_context].buffer_bindings.pixel_pack;
		break;
	case GL_PIXEL_UNPACK_BUFFER:
		fbo = &contexts[cur_context].buffer_bindings.pixel_unpack;
		break;
	case GL_QUERY_BUFFER:
		fbo = &contexts[cur_context].buffer_bindings.query;
		break;
	case GL_SHADER_STORAGE_BUFFER:
		fbo = &contexts[cur_context].buffer_bindings.shader_storage;
		break;
	case GL_TEXTURE_BUFFER:
		fbo = &contexts[cur_context].buffer_bindings.texture;
		break;
	case GL_TRANSFORM_FEEDBACK_BUFFER:
		fbo = &contexts[cur_context].buffer_bindings.transform_feedback;
		break;
	case GL_UNIFORM_BUFFER:
		fbo = &contexts[cur_context].buffer_bindings.uniform;
		break;
	default:
		return;
	}
	fbo->id = buffer;
	fbo->first = first;
	fbo->count = count;
}

#define FN_GLBINDBUFFER(target, buffer) \
	gl_bind_buffer(target, buffer, 0, 0); \
	fn.glBindBuffer(target, buffer)

#define FN_GLBINDBUFFERBASE(target, index, buffer) \
	gl_bind_buffer(target, buffer, 0, 0); \
	fn.glBindBufferBase(target, index, buffer)

#define FN_GLBINDBUFFERBASEEXT(target, index, buffer) \
	gl_bind_buffer(target, buffer, 0, 0); \
	fn.glBindBufferBaseEXT(target, index, buffer)

#define FN_GLBINDBUFFERBASENV(target, index, buffer) \
	gl_bind_buffer(target, buffer, 0, 0); \
	fn.glBindBufferBaseNV(target, index, buffer)

#define FN_GLBINDBUFFERRANGE(target, index, buffer, offset, size) \
	gl_bind_buffer(target, buffer, 0, 0); \
	fn.glBindBufferRange(target, index, buffer, offset, size)

#define FN_GLBINDBUFFERRANGEEXT(target, index, buffer, offset, size) \
	gl_bind_buffer(target, buffer, 0, 0); \
	fn.glBindBufferRangeEXT(target, index, buffer, offset, size)

#define FN_GLBINDBUFFERRANGENV(target, index, buffer, offset, size) \
	gl_bind_buffer(target, buffer, 0, 0); \
	fn.glBindBufferRangeNV(target, index, buffer, offset, size)

#if NFOSMESA_NEED_INT_CONV
#define FN_GLBINDBUFFERSBASE(target, first, count, buffers) \
	if (buffers) { \
		GLuint *tmp = (GLuint *)malloc(count * sizeof(*tmp)); \
		if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
		Atari2HostIntArray(count, buffers, tmp); \
		for (GLsizei i = 0; i < count; i++) \
			gl_bind_buffer(target, tmp[i], first + i, 0); \
		fn.glBindBuffersBase(target, first, count, tmp); \
		free(tmp); \
	} else { \
		for (GLsizei i = 0; i < count; i++) \
			gl_bind_buffer(target, 0, first + i, 0); \
		fn.glBindBuffersBase(target, first, count, buffers); \
	}
#else
#define FN_GLBINDBUFFERSBASE(target, first, count, buffers)	fn.glBindBuffersBase(target, first, count, buffers)
#endif

#define FN_GLBINDBUFFERSRANGE(target, first, count, buffers, offsets, sizes) \
	GLuint *pbuffers; \
	GLintptr *poffsets; \
	GLsizeiptr *psizes; \
	if (buffers) { \
		pbuffers = (GLuint *)malloc(count * sizeof(*pbuffers)); \
		if (!pbuffers) { glSetError(GL_OUT_OF_MEMORY); return; } \
		Atari2HostIntArray(count, buffers, pbuffers); \
		for (GLsizei i = 0; i < count; i++) \
			gl_bind_buffer(target, pbuffers[i], first + i, 0); \
	} else { \
		pbuffers = NULL; \
		for (GLsizei i = 0; i < count; i++) \
			gl_bind_buffer(target, 0, first + i, 0); \
	} \
	if (offsets) { \
		poffsets = (GLintptr *)malloc(count * sizeof(*poffsets)); \
		if (!poffsets) { glSetError(GL_OUT_OF_MEMORY); return; } \
		const Uint32 *aoffsets = (const Uint32 *)offsets; \
		for (int i = 0; i < count; i++) \
			poffsets[i] = SDL_SwapBE32(aoffsets[i]); \
	} else { \
		poffsets = NULL; \
	} \
	if (sizes) { \
		psizes = (GLsizeiptr *)malloc(count * sizeof(*psizes)); \
		if (!psizes) { glSetError(GL_OUT_OF_MEMORY); return; } \
		const Uint32 *asizes = (const Uint32 *)sizes; \
		for (int i = 0; i < count; i++) \
			psizes[i] = SDL_SwapBE32(asizes[i]); \
	} else { \
		psizes = NULL; \
	} \
	fn.glBindBuffersRange(target, first, count, pbuffers, poffsets, psizes); \
	free(psizes); \
	free(poffsets); \
	free(pbuffers)

#if NFOSMESA_NEED_INT_CONV
#define FN_GLBINDIMAGETEXTURES(first, count, textures) \
	if (textures) { \
		GLuint *tmp = (GLuint *)malloc(count * sizeof(*tmp)); \
		if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
		Atari2HostIntArray(count, textures, tmp); \
		fn.glBindImageTextures(first, count, tmp); \
		free(tmp); \
	} else { \
		fn.glBindImageTextures(first, count, textures); \
	}
#else
#define FN_GLBINDIMAGETEXTURES(first, count, textures)	fn.glBindImageTextures(first, count, textures)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLBINDSAMPLERS(first, count, samplers) \
	if (samplers) { \
		GLuint *tmp = (GLuint *)malloc(count * sizeof(*tmp)); \
		if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
		Atari2HostIntArray(count, samplers, tmp); \
		fn.glBindSamplers(first, count, tmp); \
		free(tmp); \
	} else { \
		fn.glBindSamplers(first, count, samplers); \
	}
#else
#define FN_GLBINDSAMPLERS(first, count, samples)	fn.glBindSamplers(first, count, samplers)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLBINDTEXTURES(first, count, textures) \
	if (textures) { \
		GLuint *tmp = (GLuint *)malloc(count * sizeof(*tmp)); \
		if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
		Atari2HostIntArray(count, textures, tmp); \
		fn.glBindTextures(first, count, tmp); \
		free(tmp); \
	} else { \
		fn.glBindTextures(first, count, textures); \
	}
#else
#define FN_GLBINDTEXTURES(first, count, samples)	fn.glBindTextures(first, count, textures)
#endif

#define FN_GLBINDVERTEXBUFFERS(first, count, buffers, offsets, sizes) \
	GLuint *pbuffers; \
	GLintptr *poffsets; \
	GLsizei *psizes; \
	if (buffers) { \
		pbuffers = (GLuint *)malloc(count * sizeof(*pbuffers)); \
		if (!pbuffers) { glSetError(GL_OUT_OF_MEMORY); return; } \
		Atari2HostIntArray(count, buffers, pbuffers); \
	} else { \
		pbuffers = NULL; \
	} \
	if (offsets) { \
		poffsets = (GLintptr *)malloc(count * sizeof(*poffsets)); \
		if (!poffsets) { glSetError(GL_OUT_OF_MEMORY); return; } \
		const Uint32 *aoffsets = (const Uint32 *)offsets; \
		for (int i = 0; i < count; i++) \
			poffsets[i] = SDL_SwapBE32(aoffsets[i]); \
	} else { \
		poffsets = NULL; \
	} \
	if (sizes) { \
		psizes = (GLsizei *)malloc(count * sizeof(*psizes)); \
		if (!psizes) { glSetError(GL_OUT_OF_MEMORY); return; } \
		const Uint32 *asizes = (const Uint32 *)sizes; \
		for (int i = 0; i < count; i++) \
			psizes[i] = SDL_SwapBE32(asizes[i]); \
	} else { \
		psizes = NULL; \
	} \
	fn.glBindVertexBuffers(first, count, pbuffers, poffsets, psizes); \
	free(psizes); \
	free(poffsets); \
	free(pbuffers)

/* #define FN_GLULOOKAT(eyeX, eyeY, eyeZ, centerX, centerY, centerZ, upX, upY, upZ) */

/* glBinormalPointerEXT belongs to never completed EXT_coordinate_frame */
#define FN_GLBINORMALPOINTEREXT(type, stride, pointer) \
	fn.glBinormalPointerEXT(type, stride, pointer)

/* glTangentPointerEXT belongs to never completed EXT_coordinate_frame */
#define FN_GLTANGENTPOINTEREXT(type, stride, pointer) \
	fn.glTangentPointerEXT(type, stride, pointer)

/* nothing to do */
#define FN_GLBUFFERDATA(target, size, data, usage) \
	fn.glBufferData(target, size, data, usage)

/* nothing to do */
#define FN_GLBUFFERDATAARB(target, size, data, usage) \
	fn.glBufferDataARB(target, size, data, usage)

/* nothing to do */
#define FN_GLNAMEDBUFFERDATAEXT(buffer, size, data, usage) \
	fn.glNamedBufferDataEXT(buffer, size, data, usage)

/* nothing to do */
#define FN_GLBUFFERSTORAGE(target, size, data, flags) \
	fn.glBufferStorage(target, size, data, flags)

/* nothing to do */
#define FN_GLNAMEDBUFFERSTORAGE(buffer, size, data, flags) \
	fn.glNamedBufferStorage(buffer, size, data, flags)

/* nothing to do */
#define FN_GLBUFFERSUBDATA(target, offset, size, data) \
	fn.glBufferSubData(target, offset, size, data)

/* nothing to do */
#define FN_GLBUFFERSUBDATAARB(target, offset, size, data) \
	fn.glBufferSubDataARB(target, offset, size, data)

/* nothing to do */
#define FN_GLNAMEDBUFFERSUBDATAEXT(buffer, offset, size, data) \
	fn.glNamedBufferSubDataEXT(buffer, offset, size, data)

/* nothing to do */
#define FN_GLCLEARBUFFERDATA(target, internalformat, format, type, data) \
	fn.glClearBufferData(target, internalformat, format, type, data)

/* nothing to do */
#define FN_GLCLEARNAMEDBUFFERDATAEXT(buffer, internalformat, format, type, data) \
	fn.glClearNamedBufferDataEXT(buffer, internalformat, format, type, data)

/* nothing to do */
#define FN_GLCLEARBUFFERSUBDATA(target, internalformat, offset, size, format, type, data) \
	fn.glClearBufferSubData(target, internalformat, offset, size, format, type, data)

/* nothing to do */
#define FN_GLCLEARNAMEDBUFFERSUBDATAEXT(buffer, internalformat, offset, size, format, type, data) \
	fn.glClearNamedBufferSubDataEXT(buffer, internalformat, offset, size, format, type, data)

#if NFOSMESA_NEED_INT_CONV
#define FN_GLCLEARBUFFERIV(buffer, drawbuffer, value) \
	if (value) { \
		GLint tmp[4]; \
		switch (buffer) { \
			case GL_COLOR: Atari2HostIntPtr(4, value, tmp); break; \
			case GL_STENCIL: \
			case GL_DEPTH: Atari2HostIntPtr(1, value, tmp); break; \
		} \
		fn.glClearBufferiv(buffer, drawbuffer, tmp); \
	} else { \
		fn.glClearBufferiv(buffer, drawbuffer, value); \
	}
#else
#define FN_GLCLEARBUFFERIV(buffer, drawbuffer, value) fn.glClearBufferiv(buffer, drawbuffer, value)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLCLEARBUFFERUIV(buffer, drawbuffer, value) \
	if (value) { \
		GLuint tmp[4]; \
		switch (buffer) { \
			case GL_COLOR: Atari2HostIntPtr(4, value, tmp); break; \
			case GL_STENCIL: \
			case GL_DEPTH: Atari2HostIntPtr(1, value, tmp); break; \
		} \
		fn.glClearBufferuiv(buffer, drawbuffer, tmp); \
	} else { \
		fn.glClearBufferuiv(buffer, drawbuffer, value); \
	}
#else
#define FN_GLCLEARBUFFERUIV(buffer, drawbuffer, value) fn.glClearBufferuiv(buffer, drawbuffer, value)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLCLEARBUFFERFV(buffer, drawbuffer, value) \
	if (value) { \
		GLfloat tmp[4]; \
		switch (buffer) { \
			case GL_COLOR: Atari2HostFloatPtr(4, value, tmp); break; \
			case GL_STENCIL: \
			case GL_DEPTH: Atari2HostFloatPtr(1, value, tmp); break; \
		} \
		fn.glClearBufferfv(buffer, drawbuffer, tmp); \
	} else { \
		fn.glClearBufferfv(buffer, drawbuffer, value); \
	}
#else
#define FN_GLCLEARBUFFERFV(buffer, drawbuffer, value) fn.glClearBufferfv(buffer, drawbuffer, value)
#endif

/* FIXME for glTexImage*:
If a non-zero named buffer object is bound to the
GL_PIXEL_UNPACK_BUFFER target (see glBindBuffer) while a texture image
is specified, data is treated as a byte offset into the buffer object's
data store.
*/

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLTEXIMAGE1D(target, level, internalformat, width, border, format, type, pixels) \
	void *tmp = convertPixels(width, 1, 1, format, type, pixels); \
	fn.glTexImage1D(target, level, internalformat, width, border, format, type, tmp); \
	if (tmp != pixels) free(tmp)
#else
#define FN_GLTEXIMAGE1D(target, level, internalformat, width, border, format, type, pixels) fn.glTexImage1D(target, level, internalformat, width, border, format, type, pixels)
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLTEXIMAGE2D(target, level, internalformat, width, height, border, format, type, pixels) \
	void *tmp = convertPixels(width, height, 1, format, type, pixels); \
	fn.glTexImage2D(target, level, internalformat, width, height, border, format, type, tmp); \
	if (tmp != pixels) free(tmp)
#else
#define FN_GLTEXIMAGE2D(target, level, internalformat, width, height, border, format, type, pixels) fn.glTexImage2D(target, level, internalformat, width, height, border, format, type, pixels)
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLTEXIMAGE3D(target, level, internalformat, width, height, depth, border, format, type, pixels) \
	void *tmp = convertPixels(width, height, depth, format, type, pixels); \
	fn.glTexImage3D(target, level, internalformat, width, height, depth, border, format, type, tmp); \
	if (tmp != pixels) free(tmp)
#else
#define FN_GLTEXIMAGE3D(target, level, internalformat, width, height, depth, border, format, type, pixels) fn.glTexImage3D(target, level, internalformat, width, height, depth, border, format, type, pixels)
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLCLEARTEXIMAGE(texture, level, format, type, data) \
	/* \
	 * FIXME: we need the dimensions of the texture, \
	 * which are only avaiable through GL 4.5 glGetTextureParameter() \
	 */ \
	fn.glClearTexImage(texture, level, format, type, data)
#else
#define FN_GLCLEARTEXIMAGE(texture, level, format, type, data) fn.glClearTexImage(texture, level, format, type, data)
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLCLEARTEXSUBIMAGE(texture, level, xoffset, yoffset, zoffset, width, height, depth, format, type, data) \
	void *tmp = convertPixels(width, height, depth, format, type, data); \
	fn.glClearTexSubImage(texture, level, xoffset, yoffset, zoffset, width, height, depth, format, type, tmp); \
	if (tmp != data) free(tmp)
#else
#define FN_GLCLEARTEXSUBIMAGE(texture, level, xoffset, yoffset, zoffset, width, height, depth, format, type, data) fn.glClearTexSubImage(texture, level, xoffset, yoffset, zoffset, width, height, depth, format, type, data)
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV || NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLCOLORSUBTABLEEXT(target, start, count, format, type, table) \
	void *tmp = convertPixels(count, 1, 1, format, type, table); \
	fn.glColorSubTableEXT(target, start, count, format, type, tmp); \
	if (tmp != table) free(tmp)
#else
#define FN_GLCOLORSUBTABLEEXT(target, start, count, format, type, table) fn.glColorSubTableEXT(target, start, count, format, type, table)
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV || NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLCOLORTABLEEXT(target, internalformat, width, format, type, table) \
	void *tmp = convertPixels(width, 1, 1, format, type, table); \
	fn.glColorTableEXT(target, internalformat, width, format, type, tmp); \
	if (tmp != table) free(tmp)
#else
#define FN_GLCOLORTABLEEXT(target, internalformat, width, format, type, table) fn.glColorTableEXT(target, internalformat, width, format, type, table)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLCOLORTABLEPARAMETERFV(target, pname, params) \
	if (params) { \
		GLfloat tmp[4]; \
		Atari2HostFloatPtr(4, params, tmp); \
		fn.glColorTableParameterfv(target, pname, tmp); \
	} else { \
		fn.glColorTableParameterfv(target, pname, params); \
	}
#else
#define FN_GLCOLORTABLEPARAMETERFV(target, pname, params) fn.glColorTableParameterfv(target, pname, params)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLCOLORTABLEPARAMETERFVSGI(target, pname, params) \
	if (params) { \
		GLfloat tmp[4]; \
		Atari2HostFloatPtr(4, params, tmp); \
		fn.glColorTableParameterfvSGI(target, pname, tmp); \
	} else { \
		fn.glColorTableParameterfvSGI(target, pname, params); \
	}
#else
#define FN_GLCOLORTABLEPARAMETERFVSGI(target, pname, params) fn.glColorTableParameterfvSGI(target, pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLCOLORTABLEPARAMETERIV(target, pname, params) \
	if (params) { \
		GLint tmp[4]; \
		Atari2HostIntPtr(4, params, tmp); \
		fn.glColorTableParameteriv(target, pname, tmp); \
	} else { \
		fn.glColorTableParameteriv(target, pname, params); \
	}
#else
#define FN_GLCOLORTABLEPARAMETERIV(target, pname, params) fn.glColorTableParameteriv(target, pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLCOLORTABLEPARAMETERIVSGI(target, pname, params) \
	if (params) { \
		GLint tmp[4]; \
		Atari2HostIntPtr(4, params, tmp); \
		fn.glColorTableParameterivSGI(target, pname, tmp); \
	} else { \
		fn.glColorTableParameterivSGI(target, pname, params); \
	}
#else
#define FN_GLCOLORTABLEPARAMETERIVSGI(target, pname, params) fn.glColorTableParameterivSGI(target, pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV || NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLCOLORTABLESGI(target, internalformat, width, format, type, table) \
	void *tmp = convertPixels(width, 1, 1, format, type, table); \
	fn.glColorTableSGI(target, internalformat, width, format, type, tmp); \
	if (tmp != table) free(tmp)
#else
#define FN_GLCOLORTABLESGI(target, internalformat, width, format, type, table)	fn.glColorTableSGI(target, internalformat, width, format, type, table)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLCOMBINERPARAMETERFVNV(pname, params) \
	if (params) { \
		GLfloat tmp[4]; \
		int size = 1; \
		switch (pname) { \
			case GL_CONSTANT_COLOR0_NV: \
			case GL_CONSTANT_COLOR1_NV: size = 4; break; \
		} \
		Atari2HostFloatPtr(size, params, tmp); \
		fn.glCombinerParameterfvNV(pname, tmp); \
	} else { \
		fn.glCombinerParameterfvNV(pname, params); \
	}
#else
#define FN_GLCOMBINERPARAMETERFVNV(pname, params) fn.glCombinerParameterfvNV(pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLCOMBINERPARAMETERIVNV(pname, params) \
	if (params) { \
		GLint tmp[4]; \
		int size = 1; \
		switch (pname) { \
			case GL_CONSTANT_COLOR0_NV: \
			case GL_CONSTANT_COLOR1_NV: size = 4; break; \
		} \
		Atari2HostIntPtr(size, params, tmp); \
		fn.glCombinerParameterivNV(pname, tmp); \
	} else { \
		fn.glCombinerParameterivNV(pname, params); \
	}
#else
#define FN_GLCOMBINERPARAMETERIVNV(pname, params) fn.glCombinerParameterivNV(pname, params)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLCOMBINERSTAGEPARAMETERFVNV(stage, pname, params) \
	if (params) { \
		GLfloat tmp[4]; \
		int size = 1; \
		switch (pname) { \
			case GL_CONSTANT_COLOR0_NV: \
			case GL_CONSTANT_COLOR1_NV: size = 4; break; \
		} \
		Atari2HostFloatPtr(size, params, tmp); \
		fn.glCombinerStageParameterfvNV(stage, pname, tmp); \
	} else { \
		fn.glCombinerStageParameterfvNV(stage, pname, params); \
	}
#else
#define FN_GLCOMBINERSTAGEPARAMETERFVNV(stage, pname, params) fn.glCombinerStageParameterfvNV(stage, pname, params)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETCOMBINERSTAGEPARAMETERFVNV(stage, pname, params) \
	if (params) { \
		GLfloat tmp[4]; \
		int size = 1; \
		fn.glGetCombinerStageParameterfvNV(stage, pname, tmp); \
		switch (pname) { \
			case GL_CONSTANT_COLOR0_NV: \
			case GL_CONSTANT_COLOR1_NV: size = 4; break; \
		} \
		Host2AtariFloatArray(size, tmp, params); \
	} else { \
		fn.glGetCombinerStageParameterfvNV(stage, pname, params); \
	}
#else
#define FN_GLGETCOMBINERSTAGEPARAMETERFVNV(stage, pname, params) fn.glGetCombinerStageParameterfvNV(stage, pname, params)
#endif

#define FN_GLCOMPILESHADERINCLUDEARB(shader, count, path, length) \
	GLchar **ppath; \
	GLint *plength; \
	if (path) { \
		ppath = (GLchar **)malloc(count * sizeof(*ppath)); \
		if (!ppath) { glSetError(GL_OUT_OF_MEMORY); return; } \
		const Uint32 *p = (const Uint32 *)path; \
		/* FIXME: the pathnames here are meaningless to the host */ \
		for (GLsizei i = 0; i < count; i++) \
			ppath[i] = p[i] ? (GLchar *)Atari2HostAddr(SDL_SwapBE32(p[i])) : NULL; \
	} else { \
		ppath = NULL; \
	} \
	if (length) { \
		plength = (GLint *)malloc(count * sizeof(*plength)); \
		if (!plength) { glSetError(GL_OUT_OF_MEMORY); return; } \
		Atari2HostIntPtr(count, length, plength); \
	} else { \
		plength = NULL; \
	} \
	fn.glCompileShaderIncludeARB(shader, count, ppath, plength); \
	free(plength); \
	free(ppath)

/* nothing to do */
#define FN_GLCOMPRESSEDMULTITEXIMAGE1DEXT(texunit, target, level, internalformat, width, border, imageSize, bits) \
	fn.glCompressedMultiTexImage1DEXT(texunit, target, level, internalformat, width, border, imageSize, bits)

/* nothing to do */
#define FN_GLCOMPRESSEDMULTITEXIMAGE2DEXT(texunit, target, level, internalformat, width, height, border, imageSize, bits) \
	fn.glCompressedMultiTexImage2DEXT(texunit, target, level, internalformat, width, height, border, imageSize, bits)

/* nothing to do */
#define FN_GLCOMPRESSEDMULTITEXIMAGE3DEXT(texunit, target, level, internalformat, width, height, depth, border, imageSize, bits) \
	fn.glCompressedMultiTexImage3DEXT(texunit, target, level, internalformat, width, height, depth, border, imageSize, bits)

/* nothing to do */
#define FN_GLCOMPRESSEDMULTITEXSUBIMAGE1DEXT(texunit, target, level, xoffset, width, format, imageSize, bits) \
	fn.glCompressedMultiTexSubImage1DEXT(texunit, target, level, xoffset, width, format, imageSize, bits)

/* nothing to do */
#define FN_GLCOMPRESSEDMULTITEXSUBIMAGE2DEXT(texunit, target, level, xoffset, yoffset, width, height, format, imageSize, bits) \
	fn.glCompressedMultiTexSubImage2DEXT(texunit, target, level, xoffset, yoffset, width, height, format, imageSize, bits)

/* nothing to do */
#define FN_GLCOMPRESSEDMULTITEXSUBIMAGE3DEXT(texunit, target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, bits) \
	fn.glCompressedMultiTexSubImage3DEXT(texunit, target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, bits)

/* nothing to do */
#define FN_GLCOMPRESSEDTEXIMAGE1D(target, level, internalformat, width, border, imageSize, data) \
	fn.glCompressedTexImage1D(target, level, internalformat, width, border, imageSize, data)

/* nothing to do */
#define FN_GLCOMPRESSEDTEXIMAGE2D(target, level, internalformat, width, height, border, imageSize, data) \
	fn.glCompressedTexImage2D(target, level, internalformat, width, height, border, imageSize, data)

/* nothing to do */
#define FN_GLCOMPRESSEDTEXIMAGE3D(target, level, internalformat, width, height, depth, border, imageSize, data) \
	fn.glCompressedTexImage3D(target, level, internalformat, width, height, depth, border, imageSize, data)

/* nothing to do */
#define FN_GLCOMPRESSEDTEXIMAGE1DARB(target, level, internalformat, width, border, imageSize, data) \
	fn.glCompressedTexImage1DARB(target, level, internalformat, width, border, imageSize, data)

/* nothing to do */
#define FN_GLCOMPRESSEDTEXIMAGE2DARB(target, level, internalformat, width, height, border, imageSize, data) \
	fn.glCompressedTexImage2DARB(target, level, internalformat, width, height, border, imageSize, data)

/* nothing to do */
#define FN_GLCOMPRESSEDTEXIMAGE3DARB(target, level, internalformat, width, height, depth, border, imageSize, data) \
	fn.glCompressedTexImage3DARB(target, level, internalformat, width, height, depth, border, imageSize, data)

/* nothing to do */
#define FN_GLCOMPRESSEDTEXSUBIMAGE1D(target, level, xoffset, width, format, imageSize, data) \
	fn.glCompressedTexSubImage1D(target, level, xoffset, width, format, imageSize, data)

/* nothing to do */
#define FN_GLCOMPRESSEDTEXSUBIMAGE2D(target, level, xoffset, yoffset, width, height, format, imageSize, data) \
	fn.glCompressedTexSubImage2D(target, level, xoffset, yoffset, width, height, format, imageSize, data)

/* nothing to do */
#define FN_GLCOMPRESSEDTEXSUBIMAGE3D(target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data) \
	fn.glCompressedTexSubImage3D(target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data)

/* nothing to do */
#define FN_GLCOMPRESSEDTEXSUBIMAGE1DARB(target, level, xoffset, width, format, imageSize, data) \
	fn.glCompressedTexSubImage1DARB(target, level, xoffset, width, format, imageSize, data)

/* nothing to do */
#define FN_GLCOMPRESSEDTEXSUBIMAGE2DARB(target, level, xoffset, yoffset, width, height, format, imageSize, data) \
	fn.glCompressedTexSubImage2DARB(target, level, xoffset, yoffset, width, height, format, imageSize, data)

/* nothing to do */
#define FN_GLCOMPRESSEDTEXSUBIMAGE3DARB(target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data) \
	fn.glCompressedTexSubImage3DARB(target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data)

/* nothing to do */
#define FN_GLCOMPRESSEDTEXTUREIMAGE1DEXT(texture, target, level, internalformat, width, border, imageSize, bits) \
	fn.glCompressedTextureImage1DEXT(texture, target, level, internalformat, width, border, imageSize, bits)

/* nothing to do */
#define FN_GLCOMPRESSEDTEXTUREIMAGE2DEXT(texture, target, level, internalformat, width, height, border, imageSize, bits) \
	fn.glCompressedTextureImage2DEXT(texture, target, level, internalformat, width, height, border, imageSize, bits)

/* nothing to do */
#define FN_GLCOMPRESSEDTEXTUREIMAGE3DEXT(texture, target, level, internalformat, width, height, depth, border, imageSize, bits) \
	fn.glCompressedTextureImage3DEXT(texture, target, level, internalformat, width, height, depth, border, imageSize, bits)

/* nothing to do */
#define FN_GLCOMPRESSEDTEXTURESUBIMAGE1DEXT(texture, target, level, xoffset, width, format, imageSize, bits) \
	fn.glCompressedTextureSubImage1DEXT(texture, target, level, xoffset, width, format, imageSize, bits)

/* nothing to do */
#define FN_GLCOMPRESSEDTEXTURESUBIMAGE2DEXT(texture, target, level, xoffset, yoffset, width, height, format, imageSize, bits) \
	fn.glCompressedTextureSubImage2DEXT(texture, target, level, xoffset, yoffset, width, height, format, imageSize, bits)

/* nothing to do */
#define FN_GLCOMPRESSEDTEXTURESUBIMAGE3DEXT(texture, target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, bits) \
	fn.glCompressedTextureSubImage3DEXT(texture, target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, bits)

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLCONVOLUTIONFILTER1D(target, internalformat, width, format, type, image) \
	void *tmp = convertPixels(width, 1, 1, format, type, image); \
	fn.glConvolutionFilter1D(target, internalformat, width, format, type, tmp); \
	if (tmp != image) free(tmp)
#else
#define FN_GLCONVOLUTIONFILTER1D(target, internalformat, width, format, type, image) fn.glConvolutionFilter1D(target, internalformat, width, format, type, image)
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLCONVOLUTIONFILTER2D(target, internalformat, width, height, format, type, image) \
	void *tmp = convertPixels(width, height, 1, format, type, image); \
	fn.glConvolutionFilter2D(target, internalformat, width, height, format, type, tmp); \
	if (tmp != image) free(tmp)
#else
#define FN_GLCONVOLUTIONFILTER2D(target, internalformat, width, height, format, type, image) fn.glConvolutionFilter2D(target, internalformat, width, height, format, type, image)
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLCONVOLUTIONFILTER1DEXT(target, internalformat, width, format, type, image) \
	void *tmp = convertPixels(width, 1, 1, format, type, image); \
	fn.glConvolutionFilter1DEXT(target, internalformat, width, format, type, tmp); \
	if (tmp != image) free(tmp)
#else
#define FN_GLCONVOLUTIONFILTER1DEXT(target, internalformat, width, format, type, image) fn.glConvolutionFilter1DEXT(target, internalformat, width, format, type, image)
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLCONVOLUTIONFILTER2DEXT(target, internalformat, width, height, format, type, image) \
	void *tmp = convertPixels(width, height, 1, format, type, image); \
	fn.glConvolutionFilter2DEXT(target, internalformat, width, height, format, type, tmp); \
	if (tmp != image) free(tmp)
#else
#define FN_GLCONVOLUTIONFILTER2DEXT(target, internalformat, width, height, format, type, image) fn.glConvolutionFilter2DEXT(target, internalformat, width, height, format, type, image)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLCONVOLUTIONPARAMETERFV(target, pname, params) \
	if (params) { \
		GLfloat tmp[4]; \
		int size = nfglGetNumParams(pname); \
		Atari2HostFloatPtr(size, params, tmp); \
		fn.glConvolutionParameterfv(target, pname, tmp); \
	} else { \
		fn.glConvolutionParameterfv(target, pname, params); \
	}
#else
#define FN_GLCONVOLUTIONPARAMETERFV(target, pname, params) fn.glConvolutionParameterfv(target, pname, params)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLCONVOLUTIONPARAMETERFVEXT(target, pname, params) \
	if (params) { \
		GLfloat tmp[4]; \
		int size = nfglGetNumParams(pname); \
		Atari2HostFloatPtr(size, params, tmp); \
		fn.glConvolutionParameterfvEXT(target, pname, tmp); \
	} else { \
		fn.glConvolutionParameterfvEXT(target, pname, params); \
	}
#else
#define FN_GLCONVOLUTIONPARAMETERFVEXT(target, pname, params) fn.glConvolutionParameterfvEXT(target, pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLCONVOLUTIONPARAMETERIV(target, pname, params) \
	if (params) { \
		GLint tmp[4]; \
		int size = nfglGetNumParams(pname); \
		Atari2HostIntPtr(size, params, tmp); \
		fn.glConvolutionParameteriv(target, pname, tmp); \
	} else { \
		fn.glConvolutionParameteriv(target, pname, params); \
	}
#else
#define FN_GLCONVOLUTIONPARAMETERIV(target, pname, params) fn.glConvolutionParameteriv(target, pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLCONVOLUTIONPARAMETERIVEXT(target, pname, params) \
	if (params) { \
		GLint tmp[4]; \
		int size = nfglGetNumParams(pname); \
		Atari2HostIntPtr(size, params, tmp); \
		fn.glConvolutionParameterivEXT(target, pname, tmp); \
	} else { \
		fn.glConvolutionParameterivEXT(target, pname, params); \
	}
#else
#define FN_GLCONVOLUTIONPARAMETERIVEXT(target, pname, params) fn.glConvolutionParameterivEXT(target, pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLCONVOLUTIONPARAMETERXVOES(target, pname, params) \
	if (params) { \
		GLfixed tmp[4]; \
		int size = nfglGetNumParams(pname); \
		Atari2HostIntPtr(size, params, tmp); \
		fn.glConvolutionParameterxvOES(target, pname, tmp); \
	} else { \
		fn.glConvolutionParameterxvOES(target, pname, params); \
	}
#else
#define FN_GLCONVOLUTIONPARAMETERXVOES(target, pname, params) fn.glConvolutionParameterxvOES(target, pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLCOVERFILLPATHINSTANCEDNV(numPaths, pathNameType, paths, pathBase, coverMode, transformType, transformValues) \
	void *tmp = convertArray(numPaths, pathNameType, paths); \
	GLfloat *vals; \
	if (transformValues && numPaths) { \
		vals = (GLfloat *)malloc(numPaths * sizeof(*vals)); \
		if (!vals) { glSetError(GL_OUT_OF_MEMORY); return; } \
		Atari2HostFloatPtr(numPaths, transformValues, vals); \
	} else { \
		vals = NULL; \
	} \
	fn.glCoverFillPathInstancedNV(numPaths, pathNameType, tmp, pathBase, coverMode, transformType, vals); \
	free(vals); \
	if (tmp != paths) free(tmp)
#else
#define FN_GLCOVERFILLPATHINSTANCEDNV(numPaths, pathNameType, paths, pathBase, coverMode, transformType, transformValues) \
	fn.glCoverFillPathInstancedNV(numPaths, pathNameType, paths, pathBase, coverMode, transformType, transformValues)
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLCOVERSTROKEPATHINSTANCEDNV(numPaths, pathNameType, paths, pathBase, coverMode, transformType, transformValues) \
	void *tmp = convertArray(numPaths, pathNameType, paths); \
	GLfloat *vals; \
	if (transformValues && numPaths) { \
		vals = (GLfloat *)malloc(numPaths * sizeof(*vals)); \
		if (!vals) { glSetError(GL_OUT_OF_MEMORY); return; } \
		Atari2HostFloatPtr(numPaths, transformValues, vals); \
	} else { \
		vals = NULL; \
	} \
	fn.glCoverStrokePathInstancedNV(numPaths, pathNameType, tmp, pathBase, coverMode, transformType, vals); \
	free(vals); \
	if (tmp != paths) free(tmp)
#else
#define FN_GLCOVERSTROKEPATHINSTANCEDNV(numPaths, pathNameType, paths, pathBase, coverMode, transformType, transformValues) \
	fn.glCoverStrokePathInstancedNV(numPaths, pathNameType, paths, pathBase, coverMode, transformType, transformValues)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLCREATEPERFQUERYINTEL(queryId, queryHandle) \
	GLuint tmp[1]; \
	fn.glCreatePerfQueryINTEL(queryId, tmp); \
	Host2AtariIntPtr(1, tmp, queryHandle)
#else
#define FN_GLCREATEPERFQUERYINTEL(queryId, queryHandle) fn.glCreatePerfQueryINTEL(queryId, queryHandle)
#endif

#define FN_GLCREATESHADERPROGRAMV(type, count, strings) \
	GLchar **pstrings; \
	if (strings && count) { \
		pstrings = (GLchar **)malloc(count * sizeof(*pstrings)); \
		if (!pstrings) { glSetError(GL_OUT_OF_MEMORY); return 0; } \
		const Uint32 *p = (const Uint32 *)strings; \
		for (GLsizei i = 0; i < count; i++) \
			pstrings[i] = p[i] ? (GLchar *)Atari2HostAddr(SDL_SwapBE32(p[i])) : NULL; \
	} else { \
		pstrings = NULL; \
	} \
	GLuint ret = fn.glCreateShaderProgramv(type, count, pstrings); \
	free(pstrings); \
	return ret

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLCULLPARAMETERDVEXT(pname, params) \
	if (params) { \
		GLdouble tmp[4]; \
		int size = nfglGetNumParams(pname); \
		Atari2HostDoublePtr(size, params, tmp); \
		fn.glCullParameterdvEXT(pname, tmp); \
	} else { \
		fn.glCullParameterdvEXT(pname, params); \
	}
#else
#define FN_GLCULLPARAMETERDVEXT(pname, params) fn.glCullParameterdvEXT(pname, params)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLCULLPARAMETERFVEXT(pname, params) \
	if (params) { \
		GLfloat tmp[4]; \
		int size = nfglGetNumParams(pname); \
		Atari2HostFloatPtr(size, params, tmp); \
		fn.glCullParameterfvEXT(pname, tmp); \
	} else { \
		fn.glCullParameterfvEXT(pname, params); \
	}
#else
#define FN_GLCULLPARAMETERFVEXT(pname, params) fn.glCullParameterfvEXT(pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLDEBUGMESSAGECONTROL(source, type, severity, count, ids, enabled) \
	GLuint *tmp; \
	if (ids && count) { \
		tmp = (GLuint *)malloc(count * sizeof(*tmp)); \
		if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
		Atari2HostIntPtr(count, ids, tmp); \
	} else { \
		tmp = NULL; \
	} \
	fn.glDebugMessageControl(source, type, severity, count, tmp, enabled); \
	free(tmp)
#else
#define FN_GLDEBUGMESSAGECONTROL(source, type, severity, count, ids, enabled) \
	fn.glDebugMessageControl(source, type, severity, count, ids, enabled)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLDEBUGMESSAGECONTROLARB(source, type, severity, count, ids, enabled) \
	GLuint *tmp; \
	if (ids && count) { \
		tmp = (GLuint *)malloc(count * sizeof(*tmp)); \
		if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
		Atari2HostIntPtr(count, ids, tmp); \
	} else { \
		tmp = NULL; \
	} \
	fn.glDebugMessageControlARB(source, type, severity, count, tmp, enabled); \
	free(tmp)
#else
#define FN_GLDEBUGMESSAGECONTROLARB(source, type, severity, count, ids, enabled) \
	fn.glDebugMessageControlARB(source, type, severity, count, ids, enabled)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLDEBUGMESSAGEENABLEAMD(category, severity, count, ids, enabled) \
	GLuint *tmp; \
	if (ids && count) { \
		tmp = (GLuint *)malloc(count * sizeof(*tmp)); \
		if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
		Atari2HostIntPtr(count, ids, tmp); \
	} else { \
		tmp = NULL; \
	} \
	fn.glDebugMessageEnableAMD(category, severity, count, tmp, enabled); \
	free(tmp)
#else
#define FN_GLDEBUGMESSAGEENABLEAMD(category, severity, count, ids, enabled) \
	fn.glDebugMessageEnableAMD(category, severity, count, ids, enabled)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLDELETEBUFFERS(n, buffers) \
	GLuint *tmp; \
	if (buffers && n) { \
		tmp = (GLuint *)malloc(n * sizeof(*tmp)); \
		if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
		Atari2HostIntPtr(n, buffers, tmp); \
	} else { \
		tmp = NULL; \
	} \
	fn.glDeleteBuffers(n, tmp); \
	free(tmp)
#else
#define FN_GLDELETEBUFFERS(n, buffers) \
	fn.glDeleteBuffers(n, buffers)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLDELETEBUFFERSARB(n, buffers) \
	GLuint *tmp; \
	if (buffers && n) { \
		tmp = (GLuint *)malloc(n * sizeof(*tmp)); \
		if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
		Atari2HostIntPtr(n, buffers, tmp); \
	} else { \
		tmp = NULL; \
	} \
	fn.glDeleteBuffersARB(n, tmp); \
	free(tmp)
#else
#define FN_GLDELETEBUFFERSARB(n, buffers) \
	fn.glDeleteBuffersARB(n, buffers)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLDELETEFENCESAPPLE(n, fences) \
	GLuint *tmp; \
	if (fences && n) { \
		tmp = (GLuint *)malloc(n * sizeof(*tmp)); \
		if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
		Atari2HostIntPtr(n, fences, tmp); \
	} else { \
		tmp = NULL; \
	} \
	fn.glDeleteFencesAPPLE(n, tmp); \
	free(tmp)
#else
#define FN_GLDELETEFENCESAPPLE(n, fences) \
	fn.glDeleteFencesAPPLE(n, fences)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLDELETEFENCESNV(n, fences) \
	GLuint *tmp; \
	if (fences && n) { \
		tmp = (GLuint *)malloc(n * sizeof(*tmp)); \
		if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
		Atari2HostIntPtr(n, fences, tmp); \
	} else { \
		tmp = NULL; \
	} \
	fn.glDeleteFencesNV(n, tmp); \
	free(tmp)
#else
#define FN_GLDELETEFENCESNV(n, fences) \
	fn.glDeleteFencesNV(n, fences)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLDELETEFRAMEBUFFERS(n, framebuffers) \
	GLuint *tmp; \
	if (framebuffers && n) { \
		tmp = (GLuint *)malloc(n * sizeof(*tmp)); \
		if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
		Atari2HostIntPtr(n, framebuffers, tmp); \
	} else { \
		tmp = NULL; \
	} \
	fn.glDeleteFramebuffers(n, tmp); \
	free(tmp)
#else
#define FN_GLDELETEFRAMEBUFFERS(n, framebuffers) \
	fn.glDeleteFramebuffers(n, framebuffers)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLDELETEFRAMEBUFFERSEXT(n, framebuffers) \
	GLuint *tmp; \
	if (framebuffers && n) { \
		tmp = (GLuint *)malloc(n * sizeof(*tmp)); \
		if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
		Atari2HostIntPtr(n, framebuffers, tmp); \
	} else { \
		tmp = NULL; \
	} \
	fn.glDeleteFramebuffersEXT(n, tmp); \
	free(tmp)
#else
#define FN_GLDELETEFRAMEBUFFERSEXT(n, framebuffers) \
	fn.glDeleteFramebuffersEXT(n, framebuffers)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLDELETENAMESAMD(identifier, num, names) \
	GLuint *tmp; \
	if (num && names) { \
		tmp = (GLuint *)malloc(num * sizeof(*tmp)); \
		if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
		Atari2HostIntPtr(num, names, tmp); \
	} else { \
		tmp = NULL; \
	} \
	fn.glDeleteNamesAMD(identifier, num, tmp); \
	free(tmp)
#else
#define FN_GLDELETENAMESAMD(identifier, num, names) \
	fn.glDeleteNamesAMD(identifier, num, names)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLDELETEOCCLUSIONQUERIESNV(n, ids) \
	GLuint *tmp; \
	if (n && ids) { \
		tmp = (GLuint *)malloc(n * sizeof(*tmp)); \
		if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
		Atari2HostIntPtr(n, ids, tmp); \
	} else { \
		tmp = NULL; \
	} \
	fn.glDeleteOcclusionQueriesNV(n, tmp); \
	free(tmp)
#else
#define FN_GLDELETEOCCLUSIONQUERIESNV(n, ids) \
	fn.glDeleteOcclusionQueriesNV(n, ids)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLDEFORMATIONMAP3DSGIX(target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, w1, w2, wstride, worder, points) \
	GLdouble tmp[3]; \
	/* count(3) guessed from function name; has to be verified */ \
	Atari2HostDoublePtr(3, points, tmp); \
	fn.glDeformationMap3dSGIX(target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, w1, w2, wstride, worder, tmp)
#else
#define FN_GLDEFORMATIONMAP3DSGIX(target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, w1, w2, wstride, worder, points) \
	fn.glDeformationMap3dSGIX(target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, w1, w2, wstride, worder, points)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLDEFORMATIONMAP3FSGIX(target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, w1, w2, wstride, worder, points) \
	GLfloat tmp[3]; \
	/* count(3) guessed from function name; has to be verified */ \
	Atari2HostFloatPtr(3, points, tmp); \
	fn.glDeformationMap3fSGIX(target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, w1, w2, wstride, worder, tmp)
#else
#define FN_GLDEFORMATIONMAP3FSGIX(target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, w1, w2, wstride, worder, points) \
	fn.glDeformationMap3fSGIX(target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, w1, w2, wstride, worder, points)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLDELETEPERFMONITORSAMD(n, monitors) \
	GLuint *tmp; \
	if (n && monitors) { \
		tmp = (GLuint *)malloc(n * sizeof(*tmp)); \
		if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
		Atari2HostIntPtr(n, monitors, tmp); \
	} else { \
		tmp = NULL; \
	} \
	fn.glDeletePerfMonitorsAMD(n, tmp); \
	free(tmp)
#else
#define FN_GLDELETEPERFMONITORSAMD(n, monitors) \
	fn.glDeletePerfMonitorsAMD(n, monitors)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLDELETEPROGRAMPIPELINES(n, pipelines) \
	GLuint *tmp; \
	if (n && pipelines) { \
		tmp = (GLuint *)malloc(n * sizeof(*tmp)); \
		if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
		Atari2HostIntPtr(n, pipelines, tmp); \
	} else { \
		tmp = NULL; \
	} \
	fn.glDeleteProgramPipelines(n, tmp); \
	free(tmp)
#else
#define FN_GLDELETEPROGRAMPIPELINES(n, pipelines) \
	fn.glDeleteProgramPipelines(n, pipelines)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGENPERFMONITORSAMD(n, monitors) \
	GLuint *tmp; \
	if (n && monitors) { \
		tmp = (GLuint *)malloc(n * sizeof(*tmp)); \
		if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
		fn.glGenPerfMonitorsAMD(n, tmp); \
		Host2AtariIntPtr(n, tmp, monitors); \
	} else { \
		tmp = NULL; \
		fn.glGenPerfMonitorsAMD(n, tmp); \
	} \
	free(tmp)
#else
#define FN_GLGENPERFMONITORSAMD(n, monitors) \
	fn.glGenPerfMonitorsAMD(n, monitors)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLDELETEPROGRAMSARB(n, programs) \
	GLuint *tmp; \
	if (n && programs) { \
		tmp = (GLuint *)malloc(n * sizeof(*tmp)); \
		if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
		Atari2HostIntPtr(n, programs, tmp); \
	} else { \
		tmp = NULL; \
	} \
	fn.glDeleteProgramsARB(n, tmp); \
	free(tmp)
#else
#define FN_GLDELETEPROGRAMSARB(n, programs) \
	fn.glDeleteProgramsARB(n, programs)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLDELETEPROGRAMSNV(n, programs) \
	GLuint *tmp; \
	if (n && programs) { \
		tmp = (GLuint *)malloc(n * sizeof(*tmp)); \
		if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
		Atari2HostIntPtr(n, programs, tmp); \
	} else { \
		tmp = NULL; \
	} \
	fn.glDeleteProgramsNV(n, tmp); \
	free(tmp)
#else
#define FN_GLDELETEPROGRAMSNV(n, programs) \
	fn.glDeleteProgramsNV(n, programs)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLDELETEQUERIES(n, ids) \
	GLuint *tmp; \
	if (n && ids) { \
		tmp = (GLuint *)malloc(n * sizeof(*tmp)); \
		if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
		Atari2HostIntPtr(n, ids, tmp); \
	} else { \
		tmp = NULL; \
	} \
	fn.glDeleteQueries(n, tmp); \
	free(tmp)
#else
#define FN_GLDELETEQUERIES(n, ids) \
	fn.glDeleteQueries(n, ids)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLDELETEQUERIESARB(n, ids) \
	GLuint *tmp; \
	if (n && ids) { \
		tmp = (GLuint *)malloc(n * sizeof(*tmp)); \
		if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
		Atari2HostIntPtr(n, ids, tmp); \
	} else { \
		tmp = NULL; \
	} \
	fn.glDeleteQueriesARB(n, tmp); \
	free(tmp)
#else
#define FN_GLDELETEQUERIESARB(n, ids) \
	fn.glDeleteQueriesARB(n, ids)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLDELETERENDERBUFFERS(n, renderbuffers) \
	GLuint *tmp; \
	if (n && renderbuffers) { \
		tmp = (GLuint *)malloc(n * sizeof(*tmp)); \
		if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
		Atari2HostIntPtr(n, renderbuffers, tmp); \
	} else { \
		tmp = NULL; \
	} \
	fn.glDeleteRenderbuffers(n, tmp); \
	free(tmp)
#else
#define FN_GLDELETERENDERBUFFERS(n, renderbuffers) \
	fn.glDeleteRenderbuffers(n, renderbuffers)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLDELETERENDERBUFFERSEXT(n, renderbuffers) \
	GLuint *tmp; \
	if (n && renderbuffers) { \
		tmp = (GLuint *)malloc(n * sizeof(*tmp)); \
		if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
		Atari2HostIntPtr(n, renderbuffers, tmp); \
	} else { \
		tmp = NULL; \
	} \
	fn.glDeleteRenderbuffersEXT(n, tmp); \
	free(tmp)
#else
#define FN_GLDELETERENDERBUFFERSEXT(n, renderbuffers) \
	fn.glDeleteRenderbuffersEXT(n, renderbuffers)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLDELETESAMPLERS(n, samplers) \
	GLuint *tmp; \
	if (n && samplers) { \
		tmp = (GLuint *)malloc(n * sizeof(*tmp)); \
		if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
		Atari2HostIntPtr(n, samplers, tmp); \
	} else { \
		tmp = NULL; \
	} \
	fn.glDeleteSamplers(n, tmp); \
	free(tmp)
#else
#define FN_GLDELETESAMPLERS(n, samplers) \
	fn.glDeleteSamplers(n, samplers)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLDELETETRANSFORMFEEDBACKS(n, ids) \
	GLuint *tmp; \
	if (n && ids) { \
		tmp = (GLuint *)malloc(n * sizeof(*tmp)); \
		if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
		Atari2HostIntPtr(n, ids, tmp); \
	} else { \
		tmp = NULL; \
	} \
	fn.glDeleteTransformFeedbacks(n, tmp); \
	free(tmp)
#else
#define FN_GLDELETETRANSFORMFEEDBACKS(n, ids) \
	fn.glDeleteTransformFeedbacks(n, ids)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLDELETETRANSFORMFEEDBACKSNV(n, ids) \
	GLuint *tmp; \
	if (n && ids) { \
		tmp = (GLuint *)malloc(n * sizeof(*tmp)); \
		if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
		Atari2HostIntPtr(n, ids, tmp); \
	} else { \
		tmp = NULL; \
	} \
	fn.glDeleteTransformFeedbacksNV(n, tmp); \
	free(tmp)
#else
#define FN_GLDELETETRANSFORMFEEDBACKSNV(n, ids) \
	fn.glDeleteTransformFeedbacksNV(n, ids)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLDELETEVERTEXARRAYS(n, arrays) \
	GLuint *tmp; \
	if (n && arrays) { \
		tmp = (GLuint *)malloc(n * sizeof(*tmp)); \
		if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
		Atari2HostIntPtr(n, arrays, tmp); \
	} else { \
		tmp = NULL; \
	} \
	fn.glDeleteVertexArrays(n, tmp); \
	free(tmp)
#else
#define FN_GLDELETEVERTEXARRAYS(n, arrays) \
	fn.glDeleteVertexArrays(n, arrays)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLDELETEVERTEXARRAYSAPPLE(n, arrays) \
	GLuint *tmp; \
	if (n && arrays) { \
		tmp = (GLuint *)malloc(n * sizeof(*tmp)); \
		if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
		Atari2HostIntPtr(n, arrays, tmp); \
	} else { \
		tmp = NULL; \
	} \
	fn.glDeleteVertexArraysAPPLE(n, tmp); \
	free(tmp)
#else
#define FN_GLDELETEVERTEXARRAYSAPPLE(n, arrays) \
	fn.glDeleteVertexArraysAPPLE(n, arrays)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLDEPTHRANGEARRAYV(first, count, v) \
	GLdouble *tmp; \
	if (count && v) { \
		tmp = (GLdouble *)malloc(count * 2 * sizeof(*tmp)); \
		if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
		Atari2HostDoublePtr(count * 2, v, tmp); \
	} else { \
		tmp = NULL; \
	} \
	fn.glDepthRangeArrayv(first, count, tmp); \
	free(tmp)
#else
#define FN_GLDEPTHRANGEARRAYV(first, count, v) \
	fn.glDepthRangeArrayv(first, count, v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLDETAILTEXFUNCSGIS(target, n, points) \
	GLfloat *tmp; \
	if (n && points) { \
		tmp = (GLfloat *)malloc(n * 2 * sizeof(*tmp)); \
		if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
		Atari2HostFloatPtr(n * 2, points, tmp); \
	} else { \
		tmp = NULL; \
	} \
	fn.glDetailTexFuncSGIS(target, n, tmp); \
	free(tmp)
#else
#define FN_GLDETAILTEXFUNCSGIS(target, n, points) \
	fn.glDetailTexFuncSGIS(target, n, points)
#endif

/*
If a buffer is bound to the GL_DRAW_INDIRECT_BUFFER binding at the time
of a call to glDrawArraysIndirect, indirect is interpreted as an
offset, in basic machine units, into that buffer and the parameter data
is read from the buffer rather than from client memory. 
*/

#define FN_GLDRAWARRAYSINDIRECT(mode, indirect) \
	/* \
	 * The parameters addressed by indirect are packed into a structure that takes the form (in C): \
	 * \
	 *    typedef  struct { \
	 *        uint  count; \
	 *        uint  primCount; \
	 *        uint  first; \
	 *        uint  baseInstance; \
	 *    } DrawArraysIndirectCommand; \
	 */ \
	if (contexts[cur_context].buffer_bindings.draw_indirect.id) { \
		indirect = Host2AtariAddr(indirect); \
		fn.glDrawArraysIndirect(mode, indirect); \
	} else if (indirect) { \
		GLuint tmp[4]; \
		Atari2HostIntPtr(4, (const GLuint *)indirect, tmp); \
		GLuint count = tmp[0]; \
		convertClientArrays(count); \
		fn.glDrawArraysInstancedBaseInstance(mode, tmp[2], count, tmp[1], tmp[3]); \
	}

/*
 * The parameters addressed by indirect are packed into a structure that takes the form (in C):
 *
 *    typedef  struct {
 *        uint  count;
 *        uint  primCount;
 *        uint  firstIndex;
 *        uint  baseVertex;
 *        uint  baseInstance;
 *    } DrawArraysIndirectCommand;
 */
#define FN_GLDRAWELEMENTSINDIRECT(mode, type, indirect) \
	if (contexts[cur_context].buffer_bindings.draw_indirect.id) { \
		indirect = Host2AtariAddr(indirect); \
		fn.glDrawElementsIndirect(mode, type, indirect); \
	} else if (indirect) { \
		GLuint tmp[5]; \
		Atari2HostIntPtr(5, (const GLuint *)indirect, tmp); \
		GLuint count = tmp[0]; \
		convertClientArrays(count); \
		fn.glDrawElementsInstancedBaseVertexBaseInstance(mode, count, type, (const void *)(uintptr)tmp[2], tmp[1], tmp[3], tmp[4]); \
	}

#define FN_GLDRAWELEMENTSINSTANCED(mode, count, type, indices, instancecount) \
	void *tmp; \
	convertClientArrays(count); \
	switch(type) { \
	case GL_UNSIGNED_BYTE: \
	case GL_UNSIGNED_SHORT: \
	case GL_UNSIGNED_INT: \
		tmp = convertArray(count, type, indices); \
		break; \
	default: \
		glSetError(GL_INVALID_ENUM); \
		return; \
	} \
	fn.glDrawElementsInstanced(mode, count, type, tmp, instancecount); \
	if (tmp != indices) free(tmp)

#define FN_GLDRAWELEMENTSINSTANCEDARB(mode, count, type, indices, instancecount) \
	void *tmp; \
	convertClientArrays(count); \
	switch(type) { \
	case GL_UNSIGNED_BYTE: \
	case GL_UNSIGNED_SHORT: \
	case GL_UNSIGNED_INT: \
		tmp = convertArray(count, type, indices); \
		break; \
	default: \
		glSetError(GL_INVALID_ENUM); \
		return; \
	} \
	fn.glDrawElementsInstancedARB(mode, count, type, tmp, instancecount); \
	if (tmp != indices) free(tmp)

#define FN_GLDRAWELEMENTSINSTANCEDEXT(mode, count, type, indices, instancecount) \
	void *tmp; \
	convertClientArrays(count); \
	switch(type) { \
	case GL_UNSIGNED_BYTE: \
	case GL_UNSIGNED_SHORT: \
	case GL_UNSIGNED_INT: \
		tmp = convertArray(count, type, indices); \
		break; \
	default: \
		glSetError(GL_INVALID_ENUM); \
		return; \
	} \
	fn.glDrawElementsInstancedEXT(mode, count, type, tmp, instancecount); \
	if (tmp != indices) free(tmp)

#define FN_GLDRAWELEMENTSINSTANCEDBASEINSTANCE(mode, count, type, indices, instancecount, baseinstance) \
	void *tmp; \
	convertClientArrays(count); \
	switch(type) { \
	case GL_UNSIGNED_BYTE: \
	case GL_UNSIGNED_SHORT: \
	case GL_UNSIGNED_INT: \
		tmp = convertArray(count, type, indices); \
		break; \
	default: \
		glSetError(GL_INVALID_ENUM); \
		return; \
	} \
	fn.glDrawElementsInstancedBaseInstance(mode, count, type, tmp, instancecount, baseinstance); \
	if (tmp != indices) free(tmp)

#define FN_GLDRAWELEMENTSINSTANCEDBASEVERTEX(mode, count, type, indices, instancecount, basevertex) \
	void *tmp; \
	convertClientArrays(count); \
	switch(type) { \
	case GL_UNSIGNED_BYTE: \
	case GL_UNSIGNED_SHORT: \
	case GL_UNSIGNED_INT: \
		tmp = convertArray(count, type, indices); \
		break; \
	default: \
		glSetError(GL_INVALID_ENUM); \
		return; \
	} \
	fn.glDrawElementsInstancedBaseVertex(mode, count, type, tmp, instancecount, basevertex); \
	if (tmp != indices) free(tmp)

#define FN_GLDRAWELEMENTSINSTANCEDBASEVERTEXBASEINSTANCE(mode, count, type, indices, instancecount, basevertex, baseinstance) \
	void *tmp; \
	convertClientArrays(count); \
	switch(type) { \
	case GL_UNSIGNED_BYTE: \
	case GL_UNSIGNED_SHORT: \
	case GL_UNSIGNED_INT: \
		tmp = convertArray(count, type, indices); \
		break; \
	default: \
		glSetError(GL_INVALID_ENUM); \
		return; \
	} \
	fn.glDrawElementsInstancedBaseVertexBaseInstance(mode, count, type, tmp, instancecount, basevertex, baseinstance); \
	if (tmp != indices) free(tmp)

#if NFOSMESA_NEED_INT_CONV
#define FN_GLDRAWBUFFERS(n, bufs) \
	GLenum *tmp; \
	if (n && bufs) { \
		tmp = (GLenum *)malloc(n * sizeof(*tmp)); \
		if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
		Atari2HostIntPtr(n, bufs, tmp); \
	} else { \
		tmp = NULL; \
	} \
	fn.glDrawBuffers(n, tmp); \
	free(tmp)
#else
#define FN_GLDRAWBUFFERS(n, bufs) \
	fn.glDrawBuffers(n, bufs)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLDRAWBUFFERSARB(n, bufs) \
	GLenum *tmp; \
	if (n && bufs) { \
		tmp = (GLenum *)malloc(n * sizeof(*tmp)); \
		if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
		Atari2HostIntPtr(n, bufs, tmp); \
	} else { \
		tmp = NULL; \
	} \
	fn.glDrawBuffersARB(n, tmp); \
	free(tmp)
#else
#define FN_GLDRAWBUFFERSARB(n, bufs) \
	fn.glDrawBuffersARB(n, bufs)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLDRAWBUFFERSATI(n, bufs) \
	GLenum *tmp; \
	if (n && bufs) { \
		tmp = (GLenum *)malloc(n * sizeof(*tmp)); \
		if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
		Atari2HostIntPtr(n, bufs, tmp); \
	} else { \
		tmp = NULL; \
	} \
	fn.glDrawBuffersATI(n, tmp); \
	free(tmp)
#else
#define FN_GLDRAWBUFFERSATI(n, bufs) \
	fn.glDrawBuffersATI(n, bufs)
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLDRAWPIXELS(width, height, format, type, pixels) \
	void *tmp = convertPixels(width, height, 1, format, type, pixels); \
	fn.glDrawPixels(width, height, format, type, tmp); \
	if (tmp != pixels) free(tmp)
#else
#define FN_GLDRAWPIXELS(width, height, format, type, pixels) \
	fn.glDrawPixels(width, height, format, type, pixels)
#endif

/* nothing to do */
#define FN_GLEDGEFLAGV(flag) \
	fn.glEdgeFlagv(flag)

#define FN_GLELEMENTPOINTERAPPLE(type, pointer) \
	setupClientArray(contexts[cur_context].element, 1, type, 0, -1, 0, pointer); \
	contexts[cur_context].element.vendor = 1

#define FN_GLELEMENTPOINTERATI(type, pointer) \
	setupClientArray(contexts[cur_context].element, 1, type, 0, -1, 0, pointer); \
	contexts[cur_context].element.vendor = 2

#define FN_GLDRAWELEMENTARRAYAPPLE(mode, first, count) \
	convertClientArrays(first + count); \
	fn.glDrawElementArrayAPPLE(mode, first, count)

#define FN_GLDRAWELEMENTARRAYATI(mode, count) \
	convertClientArrays(count); \
	fn.glDrawElementArrayATI(mode, count)

#if NFOSMESA_NEED_INT_CONV
#define FN_GLEVALCOORD1XVOES(coords) \
	GLfixed tmp[1]; \
	Atari2HostIntPtr(1, coords, tmp); \
	fn.glEvalCoord1xvOES(tmp)
#else
#define FN_GLEVALCOORD1XVOES(coords) \
	fn.glEvalCoord1xvOES(coords)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLEVALCOORD2XVOES(coords) \
	GLfixed tmp[2]; \
	Atari2HostIntPtr(2, coords, tmp); \
	fn.glEvalCoord2xvOES(tmp)
#else
#define FN_GLEVALCOORD2XVOES(coords) \
	fn.glEvalCoord2xvOES(coords)
#endif

#define FN_GLRENDERMODE(mode) \
	GLenum render_mode = contexts[cur_context].render_mode; \
	GLint ret = fn.glRenderMode(mode); \
	switch (mode) { \
	case GL_RENDER: \
	case GL_SELECT: \
	case GL_FEEDBACK: \
		contexts[cur_context].render_mode = mode; \
		break; \
	} \
	switch (render_mode) { \
	case GL_FEEDBACK: \
		if (ret > 0 && contexts[cur_context].feedback_buffer_host) { \
			switch (contexts[cur_context].feedback_buffer_type) { \
			case GL_FLOAT: \
				Host2AtariFloatArray(ret, (const GLfloat *)contexts[cur_context].feedback_buffer_host, (GLfloat *)contexts[cur_context].feedback_buffer_atari); \
				break; \
			case GL_FIXED: \
				Host2AtariIntPtr(ret, (const GLfixed *)contexts[cur_context].feedback_buffer_host, (Uint32 *)contexts[cur_context].feedback_buffer_atari); \
				break; \
			} \
		} \
		break; \
	} \
	return ret

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLFEEDBACKBUFFER(size, type, buffer) \
	contexts[cur_context].feedback_buffer_atari = buffer; \
	free(contexts[cur_context].feedback_buffer_host); \
	contexts[cur_context].feedback_buffer_host = malloc(size * sizeof(GLfloat)); \
	if (!contexts[cur_context].feedback_buffer_host) { glSetError(GL_OUT_OF_MEMORY); return; } \
	contexts[cur_context].feedback_buffer_type = GL_FLOAT; \
	fn.glFeedbackBuffer(size, type, (GLfloat *)contexts[cur_context].feedback_buffer_host)
#else
#define FN_GLFEEDBACKBUFFER(size, type, buffer) \
	fn.glFeedbackBuffer(size, type, buffer)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLFEEDBACKBUFFERXOES(size, type, buffer) \
	contexts[cur_context].feedback_buffer_atari = (void *)buffer; \
	free(contexts[cur_context].feedback_buffer_host); \
	contexts[cur_context].feedback_buffer_host = malloc(size * sizeof(GLfixed)); \
	if (!contexts[cur_context].feedback_buffer_host) { glSetError(GL_OUT_OF_MEMORY); return; } \
	contexts[cur_context].feedback_buffer_type = GL_FIXED; \
	fn.glFeedbackBufferxOES(size, type, (GLfixed *)contexts[cur_context].feedback_buffer_host)
#else
#define FN_GLFEEDBACKBUFFERXOES(size, type, buffer) \
	fn.glFeedbackBufferxOES(size, type, buffer)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLEXECUTEPROGRAMNV(target, id, params) \
	GLfloat tmp[4]; \
	Atari2HostFloatPtr(4, params, tmp); \
	fn.glExecuteProgramNV(target, id, tmp)
#else
#define FN_GLEXECUTEPROGRAMNV(size, type, buffer) \
	fn.glExecuteProgramNV(target, id, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLFINISHASYNCSGIX(markerp) \
	GLuint tmp[1]; \
	GLint ret = fn.glFinishAsyncSGIX(tmp); \
	if (ret) \
		Host2AtariIntPtr(1, tmp, markerp); \
	return ret
#else
#define FN_GLFINISHASYNCSGIX(markerp) \
	return fn.glFinishAsyncSGIX(markerp)
#endif

#define FN_GLFLUSHVERTEXARRAYRANGEAPPLE(length, pointer) \
	if (pointer == contexts[cur_context].vertex.atari_pointer) \
		pointer = contexts[cur_context].vertex.host_pointer; \
	fn.glFlushVertexArrayRangeAPPLE(length,	pointer)

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLFOGCOORDDV(coord) \
	GLdouble tmp[1]; \
	Atari2HostDoublePtr(1, coord, tmp); \
	fn.glFogCoorddv(tmp)
#else
#define FN_GLFOGCOORDDV(coord) \
	fn.glFogCoorddv(coord)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLFOGCOORDDVEXT(coord) \
	GLdouble tmp[1]; \
	Atari2HostDoublePtr(1, coord, tmp); \
	fn.glFogCoorddvEXT(tmp)
#else
#define FN_GLFOGCOORDDVEXT(coord) \
	fn.glFogCoorddvEXT(coord)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLFOGCOORDFV(coord) \
	GLfloat tmp[1]; \
	Atari2HostFloatPtr(1, coord, tmp); \
	fn.glFogCoordfv(tmp)
#else
#define FN_GLFOGCOORDFV(coord) \
	fn.glFogCoordfv(coord)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLFOGCOORDFVEXT(coord) \
	GLfloat tmp[1]; \
	Atari2HostFloatPtr(1, coord, tmp); \
	fn.glFogCoordfvEXT(tmp)
#else
#define FN_GLFOGCOORDFVEXT(coord) \
	fn.glFogCoordfvEXT(coord)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLFOGFUNCSGIS(n, points) \
	GLfloat *tmp; \
	if (n && points) { \
		tmp = (GLfloat *)malloc(n * 2 * sizeof(*tmp)); \
		if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
		Atari2HostFloatPtr(n * 2, points, tmp); \
	} else { \
		tmp = NULL; \
	} \
	fn.glFogFuncSGIS(n, tmp); \
	free(tmp)
#else
#define FN_GLFOGFUNCSGIS(n, points) \
	fn.glFogFuncSGIS(n, points)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLFOGXVOES(pname, param) \
	GLfixed tmp[1]; \
	Atari2HostIntPtr(1, param, tmp); \
	fn.glFogxvOES(pname, tmp)
#else
#define FN_GLFOGXVOES(pname, param) \
	fn.glFogxvOES(pname, param)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLFRAGMENTLIGHTMODELFVSGIX(pname, params) \
	GLfloat tmp[4]; \
	int size = nfglGetNumParams(pname); \
	Atari2HostFloatPtr(size, params, tmp); \
	fn.glFragmentLightModelfvSGIX(pname, tmp)
#else
#define FN_GLFRAGMENTLIGHTMODELFVSGIX(pname, params) \
	fn.glFragmentLightModelfvSGIX(pname, params)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLFRAGMENTLIGHTFVSGIX(light, pname, params) \
	GLfloat tmp[4]; \
	int size = nfglGetNumParams(pname); \
	Atari2HostFloatPtr(size, params, tmp); \
	fn.glFragmentLightfvSGIX(light, pname, tmp)
#else
#define FN_GLFRAGMENTLIGHTFVSGIX(light, pname, params) \
	fn.glFragmentLightfvSGIX(light, pname, params)
#endif

#include "nfosmesa/call-gl.c"
