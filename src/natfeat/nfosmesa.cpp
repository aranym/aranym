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

#include "sysdeps.h"
#include "SDL_compat.h"
#include <SDL_loadso.h>
#include <SDL_endian.h>
#include <math.h>

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
#if defined(__APPLE__)
   /* MacOSX declares GLintptr and GLsizeiptr as "long" */
#  define PRI_IPTR "lu"
#elif SIZEOF_VOID_P >= 8
#  define PRI_IPTR PRIu64
#else
#  define PRI_IPTR "u"
#endif
#define PRI_PTR "0x%08x"

#include "osmesa_context.h"

#define DEBUG 0
#include "debug.h"
#include "verify.h"

#ifdef NFOSMESA_SUPPORT
/*--- Assumptions ---*/

/* these native types must match the Atari types */
verify(sizeof(GLshort) == 2);
verify(sizeof(GLushort) == 2);
verify(sizeof(GLint) == 4);
verify(sizeof(GLuint) == 4);
verify(sizeof(GLenum) == 4);
verify(sizeof(GLsizei) == 4);
verify(sizeof(GLfixed) == 4);
verify(sizeof(GLfloat) == ATARI_SIZEOF_FLOAT);
verify(sizeof(GLint64) == 8);
verify(sizeof(GLuint64) == 8);

/*--- Defines ---*/

#define TOS_ENOSYS -32

#define M(m,row,col)  m[col*4+row]

#ifndef GL_DEVICE_LUID_EXT
#define GL_DEVICE_LUID_EXT                0x9599
#endif
#ifndef GL_LUID_SIZE_EXT
#define GL_LUID_SIZE_EXT                  8
#endif
#ifndef GL_DEVICE_UUID_EXT
#define GL_DEVICE_UUID_EXT                0x9597
#endif
#ifndef GL_DRIVER_UUID_EXT
#define GL_DRIVER_UUID_EXT                0x9598
#endif
#ifndef GL_UUID_SIZE_EXT
#define GL_UUID_SIZE_EXT                  16
#endif

/*--- Variables ---*/

osmesa_funcs OSMesaDriver::fn;
NFGL_GETPROCADDRESS OSMesaDriver::get_procaddress;
/*
 * needed by the array constructors in glMultiDrawxx()
 * FIXME: any better way to do this?
 */
OSMesaDriver *OSMesaDriver::thisdriver;

/*--- Constructor/Destructor ---*/

OSMesaDriver::OSMesaDriver()
{
	thisdriver = this;
	D(bug("nfosmesa: OSMesaDriver()"));
	memset(contexts, 0, sizeof(contexts));
	libgl_handle = libosmesa_handle = NULL;
	reset();
}

OSMesaDriver::~OSMesaDriver()
{
	D(bug("nfosmesa: ~OSMesaDriver()"));

	reset();
	CloseLibrary();
	thisdriver = NULL;
}

void OSMesaDriver::reset()
{
	int i;

	for (i = 1; i <= MAX_OSMESA_CONTEXTS; i++)
	{
		if (contexts[i].ctx)
		{
			OSMesaDestroyContext(i);
			contexts[i].ctx = NULL;
		}
	}
	num_contexts = 0;
	cur_context = 0;
	using_mesa = false;
	get_procaddress = 0;
	lib_opened = false;
	open_succeeded = false;
	SDL_glctx = 0;
}

/* undo the effects of Atari2HostAddr for pointer arguments when they specify a buffer offset */
#define Host2AtariAddr(a) ((memptr)((uintptr_t)(a) - MEMBaseDiff))

/* Read parameter on m68k stack */

#define getStackedParameter(n) nf_params[n]
#define getStackedParameter64(n) (((GLuint64)getStackedParameter(n) << 32) | (GLuint64)getStackedParameter((n) + 1))
#define getStackedFloat(n) Atari2HostFloat(getStackedParameter(n))
#define getStackedDouble(n) Atari2HostDouble(getStackedParameter(n), getStackedParameter((n) + 1))

#if NFOSMESA_POINTER_AS_MEMARG
#define getStackedPointer(n, t) getStackedParameter(n)
#define HostAddr(addr, t) (t)((addr) ? Atari2HostAddr(addr) : NULL)
#define AtariAddr(addr, t) addr
#define AtariOffset(addr) addr
#define NFHost2AtariAddr(addr) (void *)(uintptr_t)(addr)
#else
#define getStackedPointer(n, t) (t)(nf_params[n] ? Atari2HostAddr(getStackedParameter(n)) : NULL)
#define HostAddr(addr, t) (t)(addr)
#define AtariAddr(addr, t) (t)(addr)
	/* undo the effects of Atari2HostAddr for pointer arguments when they specify a buffer offset */
#define NFHost2AtariAddr(addr) ((void *)((uintptr_t)(addr) - MEMBaseDiff))
#define AtariOffset(addr) ((unsigned int)((uintptr_t)(addr) - MEMBaseDiff))
#endif

#include "nfosmesa/paramcount-gl.c"

/*--- Public functions ---*/

int32 OSMesaDriver::dispatch(uint32 fncode)
{
	int32 ret = 0;
#if DEBUG
	const char *funcname = "???";
#endif
	
	if (fncode >= NFOSMESA_LAST)
	{
		D(bug("nfosmesa: unimplemented function #%d", fncode));
		return TOS_ENOSYS;
	}
	
	/*
	 * doing several thousand inlined calls of ReadInt32()
	 * results in
	 * - an assembler process using ~23GB memory
	 * - an ~3.5GB object file
	 * - the compiler running for about 10min on this file,
	 *   including lots of swapping (on a 16GB machine)
	 * ....
	 * so we need to fetch the parameters before entering the big switch
	 */
	
	memptr ctx_ptr;	/* Current parameter list */
	uint32_t nf_params[NFOSMESA_MAXPARAMS];

	ctx_ptr = getParameter(1);

	for (unsigned int i = 0; i < paramcount[fncode]; i++)
		nf_params[i] = ReadInt32(ctx_ptr + 4 * (i));
		
	if (fncode != NFOSMESA_OSMESAPOSTPROCESS && fncode != GET_VERSION)
	{
		/*
		 * since we create the OpenGL contexts on the same thread as SDL,
		 * we must save/restore the SDL GL context if it is in use by
		 * the screen driver.
		 */
		if (SDL_glctx)
			cur_context = 0;
		
		/*
		 * OSMesaPostprocess() cannot be called after OSMesaMakeCurrent().
		 * FIXME: this will fail if ARAnyM already has a current context
		 * that was created by a different MiNT process
		 */
		if (!SelectContext(getParameter(0)))
			return ret;
	}
	
	switch(fncode)
	{
		case GET_VERSION:
    		ret = ARANFOSMESA_NFAPI_VERSION;
			break;
		case NFOSMESA_LENGLGETSTRING:
			D(funcname = "glGetString");
			ret = LenglGetString(nf_params);
			break;
		case NFOSMESA_PUTGLGETSTRING:
			D(funcname = "glGetString");
			PutglGetString(nf_params);
			break;
		case NFOSMESA_LENGLGETSTRINGI:
			D(funcname = "glGetStringi");
			ret = LenglGetStringi(nf_params);
			break;
		case NFOSMESA_PUTGLGETSTRINGI:
			D(funcname = "glGetStringi");
			PutglGetStringi(nf_params);
			break;

		case NFOSMESA_OSMESACREATECONTEXT:
			D(funcname = "OSMesaCreateContext");
			ret = OSMesaCreateContext(nf_params);
			break;
		case NFOSMESA_OSMESACREATECONTEXTEXT:
			D(funcname = "OSMesaCreateContextExt");
			ret = OSMesaCreateContextExt(nf_params);
			break;
		case NFOSMESA_OSMESADESTROYCONTEXT:
			D(funcname = "OSMesaDestroyContext");
			OSMesaDestroyContext(
				getStackedParameter(0) /* uint32_t ctx */);
			break;
		case NFOSMESA_OSMESAMAKECURRENT:
			D(funcname = "OSMesaMakeCurrent");
			ret = OSMesaMakeCurrent(nf_params);
			break;
		case NFOSMESA_OSMESAGETCURRENTCONTEXT:
			D(funcname = "OSMesaGetCurrentContext");
			ret = OSMesaGetCurrentContext();
			break;
		case NFOSMESA_OSMESAPIXELSTORE:
			D(funcname = "OSMesaPixelStore");
			OSMesaPixelStore(nf_params);
			break;
		case NFOSMESA_OSMESAGETINTEGERV:
			D(funcname = "OSMesaGetIntegerv");
			OSMesaGetIntegerv(nf_params);
			break;
		case NFOSMESA_OSMESAGETDEPTHBUFFER:
			D(funcname = "OSMesaGetDepthBuffer");
			ret = OSMesaGetDepthBuffer(nf_params);
			break;
		case NFOSMESA_OSMESAGETCOLORBUFFER:
			D(funcname = "OSMesaGetColorBuffer");
			ret = OSMesaGetColorBuffer(nf_params);
			break;
		case NFOSMESA_OSMESAGETPROCADDRESS:
			D(funcname = "OSMesaGetProcAddress");
			ret = OSMesaGetProcAddress(nf_params);
			break;
		case NFOSMESA_OSMESACOLORCLAMP:
			D(funcname = "OSMesaColorClamp");
			OSMesaColorClamp(nf_params);
			break;
		case NFOSMESA_OSMESAPOSTPROCESS:
			D(funcname = "OSMesaPostprocess");
			OSMesaPostprocess(nf_params);
			break;

		/*
		 * maybe FIXME: functions below usually need a current context,
		 * which is not checked here.
		 */
		case NFOSMESA_GLULOOKATF:
			D(funcname = "gluLookAtf");
			nfgluLookAtf(nf_params);
			break;
		
		case NFOSMESA_GLFRUSTUMF:
			D(funcname = "glFrustumf");
			nfglFrustumf(nf_params);
			break;
		
		case NFOSMESA_GLORTHOF:
			D(funcname = "glOrthof");
			nfglOrthof(nf_params);
			break;
		
		case NFOSMESA_TINYGLSWAPBUFFER:
			D(funcname = "swapbuffer");
			nftinyglswapbuffer(nf_params);
			break;

#include "nfosmesa/dispatch-gl.c"

		case NFOSMESA_ENOSYS:
		default:
			D(bug("nfosmesa: unimplemented function #%d", fncode));
			ret = TOS_ENOSYS;
			break;
	}
/*	D(bug("nfosmesa: function returning with 0x%08x", ret));*/

#if DEBUG
	if (contexts[cur_context].error_check_enabled)
	{
		GLenum last;
		
		last = contexts[cur_context].error_code;
		if (last != GL_NO_ERROR)
		{
			PrintErrors(funcname);
		} else
		{
			last = PrintErrors(funcname);
		}
		/*
		 * stash back the last error code, so
		 * the application can still fetch it
		 */
		contexts[cur_context].error_code = last;
	}
#endif
	
	if (SDL_glctx)
		SDL_GL_SetCurrentContext(SDL_glctx);
	
	return ret;
}


GLenum OSMesaDriver::PrintErrors(const char *funcname)
{
	GLenum err, last;
	
	err = last = fn.glGetError();
	while (err != GL_NO_ERROR)
	{
		bug("nfosmesa: error: %s: %s", funcname, gl_enum_name(err));
		err = fn.glGetError();
	}
	return last;
}

/*--- Protected functions ---*/

void OSMesaDriver::glSetError(GLenum e)
{
	contexts[cur_context].error_code = e;
	D(bug("glSetError(%s)", gl_enum_name(e)));
}


void *OSMesaDriver::load_gl_library(const char *pathlist)
{
	void *handle;
	char **path;
	
	handle = NULL;
	path = split_pathlist(pathlist);
	if (path != NULL)
	{
		for (int i = 0; handle == NULL && path[i] != NULL; i++)
		{
			handle = SDL_LoadObject(path[i]);
		}
#ifdef OS_darwin
		/* If loading failed, try to load from executable directory */
		if (handle == NULL)
		{
			char exedir[MAXPATHLEN];
			char curdir[MAXPATHLEN];
			getcwd(curdir, MAXPATHLEN);
			CFURLRef url = CFBundleCopyExecutableURL(CFBundleGetMainBundle());
			CFURLRef url2 = CFURLCreateCopyDeletingLastPathComponent(0, url);
			CFURLGetFileSystemRepresentation(url2, false, (UInt8 *)exedir, MAXPATHLEN);
			CFRelease(url2);
			CFRelease(url);
			chdir(exedir);
			for (int i = 0; handle == NULL && path[i] != NULL; i++)
			{
				handle = SDL_LoadObject(path[i]);
			}
			chdir(curdir);
		}
#endif
		free(path);
	}
	return handle;
}


bool OSMesaDriver::OpenLibrary(void)
{
	SDL_GLContext sdl_context = 0;
	if (lib_opened)
		return open_succeeded;
	
	lib_opened = true;
	bool libgl_needed = false;
	
	using_mesa = false;
	get_procaddress = (NFGL_GETPROCADDRESS)0;
	
	/* Check if channel size is correct */
	switch (bx_options.osmesa.channel_size)
	{
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
	if (libosmesa_handle == NULL)
		libosmesa_handle = load_gl_library(bx_options.osmesa.libosmesa);
	if (libosmesa_handle == NULL)
	{
		libgl_needed = true;
	} else
	{
		InitPointersOSMesa(libosmesa_handle);
		if (GL_ISAVAILABLE(OSMesaGetProcAddress) &&
			GL_ISAVAILABLE(OSMesaMakeCurrent) &&
			(GL_ISAVAILABLE(OSMesaCreateContext) || GL_ISAVAILABLE(OSMesaCreateContextExt)) &&
			GL_ISAVAILABLE(OSMesaDestroyContext))
		{
			using_mesa = true;
			get_procaddress = fn.OSMesaGetProcAddress;
			InitPointersGL(libosmesa_handle);
			D(bug("nfosmesa: OpenLibrary(): libOSMesa loaded"));
			if (!GL_ISAVAILABLE(glBegin))
			{
				libgl_needed = true;
				D(bug("nfosmesa: Channel size: %d -> libGL separated from libOSMesa", bx_options.osmesa.channel_size));
			} else
			{
				D(bug("nfosmesa: Channel size: %d -> libGL included in libOSMesa", bx_options.osmesa.channel_size));
			}
		} else
		{
			bug("nfosmesa: ignoring OSMesa - missing functions");
			
			libgl_needed = true;
			CloseMesaLibrary();
		}
	}

	open_succeeded = true;

	/* Load LibGL if needed */
	if (libgl_needed)
	{
		/*
		 * FIXME: should probably reuse bx_options.opengl.library,
		 * otherwise we might end up loading 2 different OpenGL libraries
		 */
		if (libgl_handle == NULL)
			libgl_handle = load_gl_library(bx_options.osmesa.libgl);
		if (libgl_handle == NULL)
		{
			D(bug("nfosmesa: Can not load '%s' library", bx_options.osmesa.libgl));
			panicbug("nfosmesa: %s: %s", bx_options.osmesa.libgl, SDL_GetError());
			open_succeeded = false;
		} else
		{
#if defined(SDL_VIDEO_DRIVER_WINDOWS)
			sdl_context = SDL_GL_GetCurrentContext();
			
			if (get_procaddress == 0)
				get_procaddress = (NFGL_GETPROCADDRESS)SDL_LoadFunction(libgl_handle, "wglGetProcAddress");
			if (get_procaddress == 0)
				get_procaddress = (NFGL_GETPROCADDRESS)SDL_LoadFunction(libgl_handle, "wglGetProcAddressARB");
			if (get_procaddress == 0)
				get_procaddress = (NFGL_GETPROCADDRESS)SDL_LoadFunction(libgl_handle, "wglGetProcAddressEXT");
			/*
			 * on windows, if we don't have a GL context already,
			 * we have to create one BEFORE retrieving the function pointers
			 */
			HGLRC tmp_ctx = 0;
			int iPixelFormat = 0;
			if (sdl_context == 0)
			{
				Win32OpenglContext::InitPointers(libgl_handle);
				tmp_ctx = Win32OpenglContext::CreateTmpContext(iPixelFormat);
			}
			
#elif defined(SDL_VIDEO_DRIVER_QUARTZ) || defined(SDL_VIDEO_DRIVER_COCOA)
			sdl_context = SDL_GL_GetCurrentContext();

#elif defined(SDL_VIDEO_DRIVER_X11)
			X11OpenglContext::InitPointers(libgl_handle);
			sdl_context = SDL_GL_GetCurrentContext();
			if (get_procaddress == 0)
				get_procaddress = (NFGL_GETPROCADDRESS)SDL_LoadFunction(libgl_handle, "glXGetProcAddress");
			if (get_procaddress == 0)
				get_procaddress = (NFGL_GETPROCADDRESS)SDL_LoadFunction(libgl_handle, "glXGetProcAddressARB");
#endif
	
			InitPointersGL(libgl_handle);
			D(bug("nfosmesa: OpenLibrary(): libGL loaded"));
	
#if defined(SDL_VIDEO_DRIVER_WINDOWS)
			Win32OpenglContext::DeleteTmpContext(tmp_ctx);
#endif
	
			if (!GL_ISAVAILABLE(glGetString))
			{
				bug("nfosmesa: ignoring libGL - missing functions");
					
				CloseGLLibrary();
				open_succeeded = false;
			}
		}
	}

	if (open_succeeded)
	{
		OffscreenContext *ctx;
		
		ctx = TryCreateContext();
		if (ctx)
		{
			if (!ctx->TestContext())
			{
				delete ctx;
				ctx = NULL;
				panicbug("nfosmesa: missing extensions");
			}
		} else
		{
			panicbug("nfosmesa: unsupported video driver");
		}
		if (ctx == NULL)
		{
			open_succeeded = false;
		} else
		{
			if (ctx->IsOpengl())
			{
				SDL_glctx = sdl_context;
			}
			delete ctx;
		}
	}

	if (sdl_context)
		SDL_GL_SetCurrentContext(sdl_context);
	
	return true;
}

void OSMesaDriver::CloseLibrary(void)
{
	D(bug("nfosmesa: CloseLibrary()"));

	CloseMesaLibrary();
	CloseGLLibrary();
}


void OSMesaDriver::CloseMesaLibrary(void)
{
	if (libosmesa_handle)
	{
		SDL_UnloadObject(libosmesa_handle);
		libosmesa_handle = NULL;
		D(bug("nfosmesa: CloseLibrary(): libOSMesa unloaded"));
	}
/* nullify OSMesa functions */
#define GL_PROC(type, gl, name, export, upper, proto, args, first, ret) 
#define OSMESA_PROC(type, gl, name, export, upper, proto, args, first, ret) fn.OSMesa ## name = (type (APIENTRY *) proto) 0;
#include "../../atari/nfosmesa/glfuncs.h"
}


void OSMesaDriver::CloseGLLibrary(void)
{
	if (libgl_handle)
	{
		SDL_UnloadObject(libgl_handle);
		libgl_handle = NULL;
		D(bug("nfosmesa: CloseLibrary(): libGL unloaded"));
	}
/* nullify GL functions */
#define GL_PROC(type, gl, name, export, upper, proto, args, first, ret) fn.gl ## name = (type (APIENTRY *) proto) 0;
#define GLU_PROC(type, gl, name, export, upper, proto, args, first, ret) 
#define OSMESA_PROC(type, gl, name, export, upper, proto, args, first, ret)
#include "../../atari/nfosmesa/glfuncs.h"
}


void OSMesaDriver::InitPointersGL(void *handle)
{
	D(bug("nfosmesa: InitPointersGL()"));

#define GL_PROC(type, gl, name, export, upper, proto, args, first, ret) \
	fn.gl ## name = (type (APIENTRY *) proto) SDL_LoadFunction(handle, "gl" #name); \
	if (fn.gl ## name == 0 && get_procaddress) \
		fn.gl ## name = (type (APIENTRY *) proto) get_procaddress("gl" #name);
#define GLU_PROC(type, gl, name, export, upper, proto, args, first, ret)
#define OSMESA_PROC(type, gl, name, export, upper, proto, args, first, ret)
#include "../../atari/nfosmesa/glfuncs.h"
}

void OSMesaDriver::InitPointersOSMesa(void *handle)
{
	D(bug("nfosmesa: InitPointersOSMesa()"));

#define GL_PROC(type, gl, name, export, upper, proto, args, first, ret) 
#define OSMESA_PROC(type, gl, name, export, upper, proto, args, first, ret) \
	fn.OSMesa ## name = (type (APIENTRY *) proto) SDL_LoadFunction(handle, "OSMesa" #name);
#include "../../atari/nfosmesa/glfuncs.h"
}


bool OSMesaDriver::SelectContext(uint32_t ctx)
{
	bool ret = true;
	
	if (ctx > MAX_OSMESA_CONTEXTS)
	{
		D(bug("nfosmesa: SelectContext: %d out of bounds", ctx));
		return false;
	}
	/* can happen if we did not load the library yet, e.g because no context was created yet */
	if (OpenLibrary() == false)
		return false;
	if (cur_context != ctx)
	{
		context_t *context = &contexts[ctx];
		if (ctx == 0 && contexts[cur_context].ctx)
			ret = contexts[cur_context].ctx->ClearCurrent();
		else if (context->ctx)
			ret = context->ctx->MakeCurrent();
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

uint32_t OSMesaDriver::OSMesaCreateContext(const uint32_t *nf_params)
{
	GLenum format = getStackedParameter(0);
	uint32_t sharelist = getStackedParameter(1);
	D(bug("nfosmesa: OSMesaCreateContext(0x%x, 0x%x)", format, sharelist));
	uint32_t extparams[5] = { format, 16, 8, format == OSMESA_COLOR_INDEX ? 0u : 16u, sharelist };
	return OSMesaCreateContextExt(extparams);
}


OffscreenContext *OSMesaDriver::TryCreateContext(void)
{
	OffscreenContext *ctx = NULL;

	if (using_mesa)
	{
		ctx = new MesaContext(libosmesa_handle);
	} else
	{
#if defined(SDL_VIDEO_DRIVER_COCOA)
		if (ctx == NULL && SDL_IsVideoDriver("cocoa")))
			ctx = new QuartzOpenglContext(libgl_handle);
#endif
#if defined(SDL_VIDEO_DRIVER_QUARTZ)
		if (ctx == NULL && SDL_IsVideoDriver("Quartz"))
			ctx = new QuartzOpenglContext(libgl_handle);
#endif
#if defined(SDL_VIDEO_DRIVER_WINDIB)
		if (ctx == NULL && SDL_IsVideoDriver("windib"))
			ctx = new Win32OpenglContext(libgl_handle);
#endif
#if defined(SDL_VIDEO_DRIVER_DDRAW)
		if (ctx == NULL && SDL_IsVideoDriver("directx"))
			ctx = new Win32OpenglContext(libgl_handle);
#endif
#if defined(SDL_VIDEO_DRIVER_WINDOWS)
		if (ctx == NULL && SDL_IsVideoDriver("windows"))
			ctx = new Win32OpenglContext(libgl_handle);
#endif
#if defined(SDL_VIDEO_DRIVER_X11)
		if (ctx == NULL && SDL_IsVideoDriver("x11"))
			ctx = new X11OpenglContext(libgl_handle);
#endif
	}
	
	return ctx;
}


uint32_t OSMesaDriver::OSMesaCreateContextExt(const uint32_t *nf_params)
{
	GLenum format = getStackedParameter(0);
	GLint depthBits = getStackedParameter(1);
	GLint stencilBits = getStackedParameter(2);
	GLint accumBits = getStackedParameter(3);
	uint32_t sharelist = getStackedParameter(4);
	int i,j;
	OffscreenContext *share_ctx;
	
	D(bug("nfosmesa: OSMesaCreateContextExt(0x%x,%d,%d,%d,0x%08x)",format,depthBits,stencilBits,accumBits,sharelist));

	/* TODO: shared contexts */
	if (sharelist)
		return 0;

	/* Find a free context */
	j = 0;
	for (i = 1; i <= MAX_OSMESA_CONTEXTS; i++)
	{
		if (contexts[i].ctx == NULL)
		{
			j = i;
			break;
		}
	}

	/* Create our host OSMesa context */
	if (j == 0)
	{
		D(bug("nfosmesa: No free context found"));
		return 0;
	}

	context_t *context = &contexts[j];
	memset((void *)(context), 0, sizeof(context_t));

	share_ctx = NULL;
	if (sharelist > 0 && sharelist <= MAX_OSMESA_CONTEXTS)
		share_ctx = contexts[sharelist].ctx;

	context->enabled_arrays = 0;
	context->render_mode = GL_RENDER;
	context->error_code = GL_NO_ERROR;

	context->ctx = TryCreateContext();
	if (!context->ctx || !context->ctx->CreateContext(format, depthBits, stencilBits, accumBits, share_ctx))
	{
		D(bug("nfosmesa: Can not create context"));
		if (context->ctx)
			delete context->ctx;
		return 0;
	}
	context->share_ctx = sharelist;
	num_contexts++;
	return j;
}


void OSMesaDriver::OSMesaDestroyContext( uint32_t ctx )
{
	D(bug("nfosmesa: OSMesaDestroyContext(%u)", ctx));
	if (ctx > MAX_OSMESA_CONTEXTS || !contexts[ctx].ctx)
	{
		bug("nfosmesa: OSMesaDestroyContext(%u): invalid context", ctx);
		return;
	}
	context_t *context = &contexts[ctx];
	
	delete context->ctx;
	context->ctx = NULL;
	
	num_contexts--;
	if (context->feedback_buffer_host)
	{
		free(context->feedback_buffer_host);
		context->feedback_buffer_host = NULL;
	}
	context->feedback_buffer_type = 0;
	if (context->select_buffer_host)
	{
		free(context->select_buffer_host);
		context->select_buffer_host = NULL;
	}
	context->select_buffer_size = 0;
	for (int i = 0; i <= MAX_OSMESA_CONTEXTS; i++)
		if (contexts[i].share_ctx == ctx)
			contexts[i].share_ctx = 0;
	if (ctx == cur_context)
		cur_context = 0;
/*
	if (num_contexts == 0)
	{
		CloseLibrary();
	}
*/
}

GLboolean OSMesaDriver::OSMesaMakeCurrent(const uint32_t *nf_params)
{
	uint32_t ctx = getStackedParameter(0);
	memptr buffer = getStackedParameter(1);
	GLenum type = getStackedParameter(2);
	GLsizei width = getStackedParameter(3);
	GLsizei height = getStackedParameter(4);
	GLboolean ret = GL_TRUE;
	
	D(bug("nfosmesa: OSMesaMakeCurrent(%u,$%08x,%s,%d,%d)", ctx, buffer, gl_enum_name(type), width, height));
	if (ctx > MAX_OSMESA_CONTEXTS)
		return GL_FALSE;
	
	if (ctx != 0 && !contexts[ctx].ctx)
		return GL_FALSE;

	if (ctx != 0)
	{
		OffscreenContext *context = contexts[ctx].ctx;
		ret = context->MakeCurrent(buffer, type, width, height);
	} else
	{
		OffscreenContext *context = contexts[cur_context].ctx;
		if (context)
			ret = context->ClearCurrent();
	}
	if (ret)
	{
		cur_context = ctx;
		D(bug("nfosmesa: MakeCurrent: %d is current", ctx));
	} else
	{
		D(bug("nfosmesa: MakeCurrent: %d failed", ctx));
	}
	return ret;
}

uint32_t OSMesaDriver::OSMesaGetCurrentContext(void)
{
	uint32_t ctx;
#if 0
	/*
	 * wrong; the host manages his current context for all processes using NFOSMesa;
	 * return interface parameter instead
	 */
	ctx = cur_context;
	D(bug("nfosmesa: OSMesaGetCurrentContext() -> %u", ctx));
#else
	ctx = getParameter(0);
#endif
	return ctx;
}

void OSMesaDriver::OSMesaPixelStore(const uint32_t *nf_params)
{
	GLint pname = getStackedParameter(0);
	GLint value = getStackedParameter(1);
	D(bug("nfosmesa: OSMesaPixelStore(0x%x, %d)", pname, value));
	uint32_t ctx = cur_context;
	if (ctx && contexts[ctx].ctx)
		contexts[ctx].ctx->PixelStore(pname, value);
}

void OSMesaDriver::OSMesaGetIntegerv(const uint32_t *nf_params)
{
	GLint pname = getStackedParameter(0);
	memptr value = getStackedParameter(1);
	GLint tmp = 0;
	uint32_t ctx = cur_context;
	
	D(bug("nfosmesa: OSMesaGetIntegerv(0x%x)", pname));
	if (ctx && contexts[ctx].ctx)
		if (!contexts[ctx].ctx->GetIntegerv(pname, &tmp))
			glSetError(GL_INVALID_ENUM);
	if (value)
		WriteInt32(value, tmp);
}

GLboolean OSMesaDriver::OSMesaGetDepthBuffer(const uint32_t *nf_params)
{
	uint32_t ctx = getStackedParameter(0);
	memptr width = getStackedParameter(1);
	memptr height = getStackedParameter(2);
	memptr bytesPerValue = getStackedParameter(3);
	memptr buffer = getStackedParameter(4);
	GLint w, h, bpp;
	memptr b;
	D(bug("nfosmesa: OSMesaGetDepthBuffer(%u)", ctx));
	if (!SelectContext(ctx) || ctx == 0)
		return GL_FALSE;
	contexts[ctx].ctx->GetDepthBuffer(&w, &h, &bpp, &b);
	if (width)
		WriteInt32(width, w);
	if (height)
		WriteInt32(height, h);
	if (bytesPerValue)
		WriteInt32(bytesPerValue, bpp);
	if (buffer)
		WriteInt32(buffer, b);
	return GL_TRUE;
}

GLboolean OSMesaDriver::OSMesaGetColorBuffer(const uint32_t *nf_params)
{
	uint32_t ctx = getStackedParameter(0);
	memptr width = getStackedParameter(1);
	memptr height = getStackedParameter(2);
	memptr format = getStackedParameter(3);
	memptr buffer = getStackedParameter(4);
	GLint w, h, f;
	memptr b;
	D(bug("nfosmesa: OSMesaGetColorBuffer(%u)", ctx));
	if (!SelectContext(ctx) || ctx == 0)
		return GL_FALSE;
	contexts[ctx].ctx->GetColorBuffer(&w, &h, &f, &b);
	if (width)
		WriteInt32(width, w);
	if (height)
		WriteInt32(height, h);
	if (format)
		WriteInt32(format, f);
	if (buffer)
		WriteInt32(buffer, b);
	return GL_TRUE;
}

unsigned int OSMesaDriver::OSMesaGetProcAddress(const uint32_t *nf_params)
{
	nfcmemptr funcname = getStackedPointer(0, const char *);
	unsigned int ret = 0;
	if (!funcname)
		return 0;
	char tmp[safe_strlen(funcname) + 1], *funcName;
	funcName = Atari2HostByteArray(sizeof(tmp), funcname, tmp);
	NFGL_PROC p = 0;
	if (get_procaddress)
	{
		p = get_procaddress(funcName);
		/* WTF, the entry in the lookup table in OSMesa names it "OSMesaPixelsStore" */
		if (p == 0 && strcmp(funcName, "OSMesaPixelStore") == 0)
			p = get_procaddress("OSMesaPixelsStore");
	}
	if (p == 0 && libgl_handle)
		p = (NFGL_PROC)SDL_LoadFunction(libgl_handle, funcName);
	if (p == 0 && libosmesa_handle)
		p = (NFGL_PROC)SDL_LoadFunction(libosmesa_handle, funcName);
	
	if (p)
	{
		/*
		 * return the corresponding function number of the NF API,
		 * allowing the atari side to look it up in a table and
		 * return a usable function pointer.
		 * Do a binary search here.
		 */
		int a, b, c;
		int dir;
		
		a = 0;
		b = (int)(sizeof(gl_functionnames) / sizeof(gl_functionnames[0]));
		while (a < b)
		{
			c = (a + b) >> 1;				/* == ((a + b) / 2) */
			dir = strcmp(funcName, gl_functionnames[c].name);
			if (dir == 0)
			{
				ret = gl_functionnames[c].funcno;
				break;
			}
			if (dir < 0)
				b = c;
			else
				a = c + 1;
		}
	}
	D(bug("nfosmesa: OSMesaGetProcAddress(\"%s\"): %p", funcName, p));
	return ret;
}

void OSMesaDriver::OSMesaColorClamp(const uint32_t *nf_params)
{
	GLboolean enable = getStackedParameter(0);
	D(bug("nfosmesa: OSMesaColorClamp(%d)", enable));
	uint32_t ctx = cur_context;
	if (ctx == 0 || !contexts[ctx].ctx)
		return;
	contexts[ctx].ctx->ColorClamp(enable);
}

void OSMesaDriver::OSMesaPostprocess(const uint32_t *nf_params)
{
	uint32_t ctx = getStackedParameter(0);
	nfcmemptr filterName = getStackedPointer(1, const char *);
	GLuint enable_value = getStackedParameter(2);
	GLubyte tmp[safe_strlen(filterName) + 1], *filter;
	filter = Atari2HostByteArray(sizeof(tmp), filterName, tmp);
	D(bug("nfosmesa: OSMesaPostprocess(%u, %s, %d)", ctx, filter, enable_value));
	if (ctx > MAX_OSMESA_CONTEXTS || ctx == 0 || !contexts[ctx].ctx)
		return;
	/* no SelectContext() here; OSMesaPostprocess must be called without having a current context */
	contexts[ctx].ctx->Postprocess((const char *)filter, enable_value);
}

uint32_t OSMesaDriver::LenglGetString(const uint32_t *nf_params)
{
	uint32_t ctx = getStackedParameter(0);
	GLenum name = getStackedParameter(1);
	UNUSED(ctx);
	D(bug("nfosmesa: LenglGetString(%u, 0x%x)", ctx, name));
	if (!GL_ISAVAILABLE(glGetString)) return 0;
	const char *s = (const char *)fn.glGetString(name);
	if (s == NULL) return 0;
	return strlen(s);
}

void OSMesaDriver::PutglGetString(const uint32_t *nf_params)
{
	uint32_t ctx = getStackedParameter(0);
	GLenum name = getStackedParameter(1);
#if NFOSMESA_POINTER_AS_MEMARG
	memptr buffer = getStackedPointer(2, GLubyte *);
#else
	GLubyte *buffer = getStackedPointer(2, GLubyte *);
#endif
	UNUSED(ctx);
	const char *s = (const char *)(GL_ISAVAILABLE(glGetString) ? fn.glGetString(name) : 0);
	D(bug("nfosmesa: PutglGetString(%u, 0x%x, " PRI_PTR "): %s", ctx, name, AtariOffset(buffer), s));
	if (buffer)
	{
		if (!s) s = "";
#if NFOSMESA_POINTER_AS_MEMARG
		Host2AtariSafeStrncpy(buffer, s, strlen(s) + 1);
#else
		strcpy((char *)buffer, s);
#endif
	}
}

uint32_t OSMesaDriver::LenglGetStringi(const uint32_t *nf_params)
{
	uint32_t ctx = getStackedParameter(0);
	GLenum name = getStackedParameter(1);
	GLuint index = getStackedParameter(2);
	UNUSED(ctx);
	D(bug("nfosmesa: LenglGetStringi(%u, 0x%x, %u)", ctx, name, index));
	if (!GL_ISAVAILABLE(glGetStringi)) return (uint32_t)-1;
	const char *s = (const char *)fn.glGetStringi(name, index);
	if (s == NULL) return (uint32_t)-1;
	return strlen(s);
}

void OSMesaDriver::PutglGetStringi(const uint32_t *nf_params)
{
	uint32_t ctx = getStackedParameter(0);
	GLenum name = getStackedParameter(1);
	GLuint index = getStackedParameter(2);
#if NFOSMESA_POINTER_AS_MEMARG
	memptr buffer = getStackedPointer(2, GLubyte *);
#else
	GLubyte *buffer = getStackedPointer(2, GLubyte *);
#endif
	UNUSED(ctx);
	D(bug("nfosmesa: PutglGetStringi(%u, 0x%x, %d, " PRI_PTR ")", ctx, name, index, AtariOffset(buffer)));
	const char *s = (const char *)(GL_ISAVAILABLE(glGetStringi) ? fn.glGetStringi(name, index) : 0);
	if (buffer)
	{
		if (!s) s = "";
#if NFOSMESA_POINTER_AS_MEMARG
		Host2AtariSafeStrncpy(buffer, s, strlen(s) + 1);
#else
		strcpy((char *)buffer, s);
#endif
	}
}

void OSMesaDriver::ConvertContext(uint32_t ctx)
{
	if (ctx == 0 || !contexts[ctx].ctx)
		return;
	
	D(bug("nfosmesa: ConvertContext"));
	contexts[ctx].ctx->ConvertContext();
}

bool OSMesaDriver::pixelBuffer::params(GLsizei _width, GLsizei _height, GLsizei _depth, GLenum _format, GLenum _type)
{
	width = _width;
	height = _height;
	depth = _depth;
	format = _format;
	type = _type;
	if (!validate())
		return false;
	if (count <= 0)
		return false;
	return true;
}


bool OSMesaDriver::pixelBuffer::params(GLsizei _bufSize, GLenum _format, GLenum _type)
{
	format = _format;
	type = _type;
	if (!validate())
		return false;
	bufsize = _bufSize;
	count = bufsize / basesize;
	if (count <= 0)
		return false;
	return true;
}


bool OSMesaDriver::pixelBuffer::validate()
{
	switch (type)
	{
	case GL_UNSIGNED_BYTE:
	case GL_BYTE:
	case GL_UNSIGNED_BYTE_3_3_2:
	case GL_UNSIGNED_BYTE_2_3_3_REV:
	case GL_BITMAP:
	case GL_UTF8_NV:
    case GL_2_BYTES:
    case GL_3_BYTES:
    case GL_4_BYTES:
	case 1:
		basesize = sizeof(GLubyte);
		break;
	case GL_UNSIGNED_SHORT:
	case GL_SHORT:
	case GL_UNSIGNED_SHORT_5_6_5:
	case GL_UNSIGNED_SHORT_5_6_5_REV:
	case GL_UNSIGNED_SHORT_4_4_4_4:
	case GL_UNSIGNED_SHORT_4_4_4_4_REV:
	case GL_UNSIGNED_SHORT_5_5_5_1:
	case GL_UNSIGNED_SHORT_1_5_5_5_REV:
    case GL_HALF_FLOAT:
	case GL_UTF16_NV:
	case 2:
		basesize = sizeof(GLushort);
		break;
	case GL_UNSIGNED_INT:
	case GL_INT:
	case GL_UNSIGNED_INT_8_8_8_8:
	case GL_UNSIGNED_INT_8_8_8_8_REV:
	case GL_UNSIGNED_INT_10_10_10_2:
	case GL_UNSIGNED_INT_2_10_10_10_REV:
	case GL_FIXED:
	case 4:
		basesize = sizeof(GLuint);
		break;
	case GL_FLOAT:
		basesize = sizeof(GLfloat);
		break;
	default:
		driver.glSetError(GL_INVALID_ENUM);
		return false;
	}
	switch (format)
	{
	case GL_RED:
	case GL_RED_INTEGER_EXT:
	case GL_GREEN:
	case GL_GREEN_INTEGER_EXT:
	case GL_BLUE:
	case GL_BLUE_INTEGER_EXT:
	case GL_ALPHA:
	case GL_ALPHA_INTEGER_EXT:
	case GL_LUMINANCE:
	case GL_LUMINANCE_INTEGER_EXT:
	case GL_STENCIL_INDEX:
	case GL_DEPTH_COMPONENT:
	case GL_COLOR_INDEX:
	case 1:
		componentcount = 1;
		break;
	case GL_LUMINANCE_ALPHA:
	case GL_LUMINANCE_ALPHA_INTEGER_EXT:
	case GL_DEPTH_STENCIL:
	case GL_RG:
	case 2:
		componentcount = 2;
		break;
	case GL_RGB:
	case GL_RGB_INTEGER_EXT:
	case GL_BGR:
	case GL_BGR_INTEGER_EXT:
	case 3:
		componentcount = 3;
		break;
	case GL_RGBA:
	case GL_RGBA_INTEGER_EXT:
	case GL_BGRA_EXT:
	case GL_BGRA_INTEGER_EXT:
	case 4:
		componentcount = 4;
		break;
	default:
		driver.glSetError(GL_INVALID_ENUM);
		return false;
	}
	valid = true;
	/* FIXME: glPixelStore parameters are not taken into account */
	count = componentcount * width * height * depth;
	switch (type)
	{
    case GL_2_BYTES:
    	count *= 2;
    	break;
    case GL_3_BYTES:
    	count *= 3;
    	break;
    case GL_4_BYTES:
    	count *= 4;
    	break;
	}
	bufsize = basesize * count;
	return true;
}


char *OSMesaDriver::pixelBuffer::hostBuffer(GLsizei _width, GLsizei _height, GLsizei _depth, GLenum _format, GLenum _type, nfmemptr dest)
{
	if (!params(_width, _height, _depth, _format, _type) || !dest)
		return NULL;
	if (basesize == 1 && !NFOSMESA_NEED_BYTE_CONV)
	{
		buffer = HostAddr(dest, char *);
	} else
	{
		buffer = (char *)malloc(bufsize);
		alloced = true;
	}
	return buffer;
}


char *OSMesaDriver::pixelBuffer::hostBuffer(GLsizei _bufSize, GLenum _format, GLenum _type, nfmemptr dest)
{
	if (!params(_bufSize, _format, _type) || !dest)
		return NULL;
	if (basesize == 1 && !NFOSMESA_NEED_BYTE_CONV)
	{
		buffer = HostAddr(dest, char *);
	} else
	{
		buffer = (char *)malloc(bufsize);
		alloced = true;
	}
	return buffer;
}


void OSMesaDriver::pixelBuffer::convertToAtari(const char *src, nfmemptr dst)
{
	if (!valid || !dst || HostAddr(dst, const char *) == src)
		return;
	if (type == GL_FLOAT)
		OSMesaDriver::Host2AtariFloatArray(count, (const GLfloat *)src, AtariAddr(dst, GLfloat *));
	else if (basesize == 1)
		OSMesaDriver::Host2AtariByteArray(count, (const GLubyte *)src, AtariAddr(dst, GLubyte *));
	else if (basesize == 2)
		OSMesaDriver::Host2AtariShortArray(count, (const GLushort *)src, AtariAddr(dst, Uint16 *));
	else /* if (basesize == 4) */
		OSMesaDriver::Host2AtariIntArray(count, (const GLuint *)src, AtariAddr(dst, uint32_t *));
}


void *OSMesaDriver::pixelBuffer::convertPixels(GLsizei _width, GLsizei _height, GLsizei _depth, GLenum _format, GLenum _type, nfmemptr pixels)
{
	void *result;

	result = (void *)hostBuffer(_width, _height, _depth, _format, _type, pixels);
	if (!result || HostAddr(pixels, void *) == result)
		return result;
	
	/* FIXME: glPixelStore parameters are not taken into account */
	if (type == GL_FLOAT)
		OSMesaDriver::Atari2HostFloatArray(count, AtariAddr(pixels, const GLfloat *), (GLfloat *)result);
	else if (basesize == 1)
		OSMesaDriver::Atari2HostByteArray(count, AtariAddr(pixels, const GLubyte *), (GLubyte *)result);
	else if (basesize == 2)
		OSMesaDriver::Atari2HostShortArray(count, AtariAddr(pixels, const Uint16 *), (GLushort *)result);
	else /* if (basesize == 4) */
		OSMesaDriver::Atari2HostIntArray(count, AtariAddr(pixels, const uint32_t *), (GLuint *)result);
	return result;
}


void *OSMesaDriver::pixelBuffer::convertArray(GLsizei _count, GLenum _type, nfmemptr pixels)
{
	return convertPixels(_count, 1, 1, 1, _type, pixels);
}


void OSMesaDriver::setupClientArray(GLenum texunit, vertexarray_t &array, GLint size, GLenum type, GLsizei stride, GLsizei count, GLint ptrstride, nfmemptr pointer)
{
	UNUSED(texunit); // FIXME
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
	case GL_BITMAP:
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
		array.basesize = sizeof(uint32_t);
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
	if (array.alloced)
	{
		free(array.host_pointer);
		array.host_pointer = NULL;
		array.alloced = false;
	}
	array.atari_pointer = pointer;
	array.converted = 0;
	array.vendor = NFOSMESA_VENDOR_NONE;
}


void OSMesaDriver::convertClientArrays(GLsizei count)
{
	if (contexts[cur_context].enabled_arrays & NFOSMESA_VERTEX_ARRAY)
	{
		vertexarray_t &array = contexts[cur_context].vertex;
		convertClientArray(count, array);
		if (array.vendor == NFOSMESA_VENDOR_IBM)
			fn.glVertexPointerListIBM(array.size, array.type, array.host_stride, (const void **)array.host_pointer, array.ptrstride);
		else if (array.vendor == NFOSMESA_VENDOR_INTEL)
			fn.glVertexPointervINTEL(array.size, array.type, (const void **)array.host_pointer);
		else if (array.vendor == NFOSMESA_VENDOR_EXT)
			fn.glVertexPointerEXT(array.size, array.type, array.host_stride, array.count, array.host_pointer);
		else
			fn.glVertexPointer(array.size, array.type, array.host_stride, array.host_pointer);
	}
	if (contexts[cur_context].enabled_arrays & NFOSMESA_NORMAL_ARRAY)
	{
		vertexarray_t &array = contexts[cur_context].normal;
		convertClientArray(count, array);
		if (array.vendor == NFOSMESA_VENDOR_IBM)
			fn.glNormalPointerListIBM(array.type, array.host_stride, (const void **)array.host_pointer, array.ptrstride);
		else if (array.vendor == NFOSMESA_VENDOR_INTEL)
			fn.glNormalPointervINTEL(array.type, (const void **)array.host_pointer);
		else if (array.vendor == NFOSMESA_VENDOR_EXT)
			fn.glNormalPointerEXT(array.type, array.host_stride, array.count, array.host_pointer);
		else
			fn.glNormalPointer(array.type, array.host_stride, array.host_pointer);
	}
	if (contexts[cur_context].enabled_arrays & NFOSMESA_COLOR_ARRAY)
	{
		vertexarray_t &array = contexts[cur_context].color;
		convertClientArray(count, array);
		if (array.vendor == NFOSMESA_VENDOR_IBM)
			fn.glColorPointerListIBM(array.size, array.type, array.host_stride, (const void **)array.host_pointer, array.ptrstride);
		else if (array.vendor == NFOSMESA_VENDOR_INTEL)
			fn.glColorPointervINTEL(array.size, array.type, (const void **)array.host_pointer);
		if (array.vendor == NFOSMESA_VENDOR_EXT)
			fn.glColorPointerEXT(array.size, array.type, array.host_stride, array.count, array.host_pointer);
		else
			fn.glColorPointer(array.size, array.type, array.host_stride, array.host_pointer);
	}
	if (contexts[cur_context].enabled_arrays & NFOSMESA_INDEX_ARRAY)
	{
		vertexarray_t &array = contexts[cur_context].index;
		convertClientArray(count, array);
		if (array.vendor == NFOSMESA_VENDOR_IBM)
			fn.glIndexPointerListIBM(array.type, array.host_stride, (const void **)array.host_pointer, array.ptrstride);
		else if (array.vendor == NFOSMESA_VENDOR_EXT)
			fn.glIndexPointerEXT(array.type, array.host_stride, array.count, array.host_pointer);
		else
			fn.glIndexPointer(array.type, array.host_stride, array.host_pointer);
	}
	if (contexts[cur_context].enabled_arrays & NFOSMESA_EDGEFLAG_ARRAY)
	{
		vertexarray_t &array = contexts[cur_context].edgeflag;
		convertClientArray(count, array);
		if (array.vendor == NFOSMESA_VENDOR_IBM)
			fn.glEdgeFlagPointerListIBM(array.host_stride, (const GLboolean **)array.host_pointer, array.ptrstride);
		else if (array.vendor == NFOSMESA_VENDOR_EXT)
			fn.glEdgeFlagPointerEXT(array.host_stride, array.count, (const GLboolean *)array.host_pointer);
		else
			fn.glEdgeFlagPointer(array.host_stride, array.host_pointer);
	}
	if (contexts[cur_context].enabled_arrays & NFOSMESA_TEXCOORD_ARRAY)
	{
		vertexarray_t &array = contexts[cur_context].texcoord;
		convertClientArray(count, array);
		if (array.vendor == NFOSMESA_VENDOR_IBM)
			fn.glTexCoordPointerListIBM(array.size, array.type, array.host_stride, (const void **)array.host_pointer, array.ptrstride);
		else if (array.vendor == NFOSMESA_VENDOR_INTEL)
			fn.glTexCoordPointervINTEL(array.size, array.type, (const void **)array.host_pointer);
		else if (array.vendor == NFOSMESA_VENDOR_EXT)
			fn.glTexCoordPointerEXT(array.size, array.type, array.host_stride, array.count, array.host_pointer);
		else
			fn.glTexCoordPointer(array.size, array.type, array.host_stride, array.host_pointer);
	}
	if (contexts[cur_context].enabled_arrays & NFOSMESA_FOGCOORD_ARRAY)
	{
		vertexarray_t &array = contexts[cur_context].fogcoord;
		convertClientArray(count, array);
		if (array.vendor == NFOSMESA_VENDOR_IBM)
			fn.glFogCoordPointerListIBM(array.type, array.host_stride, (const void **)array.host_pointer, array.ptrstride);
		else if (array.vendor == NFOSMESA_VENDOR_EXT)
			fn.glFogCoordPointerEXT(array.type, array.host_stride, array.host_pointer);
		else
			fn.glFogCoordPointer(array.type, array.host_stride, array.host_pointer);
	}
	if (contexts[cur_context].enabled_arrays & NFOSMESA_2NDCOLOR_ARRAY)
	{
		vertexarray_t &array = contexts[cur_context].secondary_color;
		convertClientArray(count, array);
		if (array.vendor == NFOSMESA_VENDOR_IBM)
			fn.glSecondaryColorPointerListIBM(array.size, array.type, array.host_stride, (const void **)array.host_pointer, array.ptrstride);
		else if (array.vendor == NFOSMESA_VENDOR_EXT)
			fn.glSecondaryColorPointerEXT(array.size, array.type, array.host_stride, array.host_pointer);
		else
			fn.glSecondaryColorPointer(array.size, array.type, array.host_stride, array.host_pointer);
	}
	if (contexts[cur_context].enabled_arrays & NFOSMESA_ELEMENT_ARRAY)
	{
		vertexarray_t &array = contexts[cur_context].element;
		convertClientArray(count, array);
		if (array.vendor == NFOSMESA_VENDOR_APPLE)
			fn.glElementPointerAPPLE(array.type, array.host_pointer);
		else if (array.vendor == NFOSMESA_VENDOR_ATI)
			fn.glElementPointerATI(array.type, array.host_pointer);
	}
	if (contexts[cur_context].enabled_arrays & NFOSMESA_VARIANT_ARRAY)
	{
		vertexarray_t &array = contexts[cur_context].variant;
		convertClientArray(count, array);
		if (array.vendor == NFOSMESA_VENDOR_EXT)
			fn.glVariantPointerEXT(array.id, array.type, array.host_stride, array.host_pointer);
	}
	if (contexts[cur_context].enabled_arrays & NFOSMESA_WEIGHT_ARRAY)
	{
		vertexarray_t &array = contexts[cur_context].weight;
		convertClientArray(count, array);
		if (array.vendor == NFOSMESA_VENDOR_ARB)
			fn.glWeightPointerARB(array.size, array.type, array.host_stride, array.host_pointer);
		else if (array.vendor == NFOSMESA_VENDOR_EXT)
			fn.glVertexWeightPointerEXT(array.size, array.type, array.host_stride, array.host_pointer);
	}
	if (contexts[cur_context].enabled_arrays & NFOSMESA_MATRIX_INDEX_ARRAY)
	{
		vertexarray_t &array = contexts[cur_context].matrixindex;
		convertClientArray(count, array);
		if (array.vendor == NFOSMESA_VENDOR_ARB)
			fn.glMatrixIndexPointerARB(array.size, array.type, array.host_stride, array.host_pointer);
	}
	if (contexts[cur_context].enabled_arrays & NFOSMESA_REPLACEMENT_CODE_ARRAY)
	{
		vertexarray_t &array = contexts[cur_context].replacement_code;
		convertClientArray(count, array);
		if (array.vendor == NFOSMESA_VENDOR_SUN)
			fn.glReplacementCodePointerSUN(array.type, array.host_stride, (const void **)array.host_pointer);
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
			array.host_pointer = (char *)realloc((void *)array.host_pointer, count * array.host_stride);
			GLsizei n = count - array.converted;
			for (GLsizei i = 0; i < n; i++)
			{
				nfmemptr src = array.atari_pointer + array.converted * array.atari_stride;
				char *dst = array.host_pointer + array.converted * array.host_stride;
				if (array.type == GL_FLOAT)
					Atari2HostFloatArray(array.size, AtariAddr(src, const GLfloat *), (GLfloat *)dst);
				else if (array.type == GL_DOUBLE)
					Atari2HostDoubleArray(array.size, AtariAddr(src, const GLdouble *), (GLdouble *)dst);
				else if (array.basesize == 1)
#if NFOSMESA_POINTER_AS_MEMARG
					memcpy(dst, Atari2HostAddr(src), array.size);
#else
					memcpy(dst, src, array.size);
#endif
				else if (array.basesize == 2)
					Atari2HostShortArray(array.size, AtariAddr(src, const Uint16 *), (GLushort *)dst);
				else /* if (array.basesize == 4) */
					Atari2HostIntArray(array.size, AtariAddr(src, const uint32_t *), (GLuint *)dst);
				array.converted++;
			}
			array.alloced = true;
		} else
		{
			array.host_pointer = HostAddr(array.atari_pointer, char *);
			array.converted = count;
		}
	}
}

void OSMesaDriver::nfglArrayElementHelper(GLint i)
{
	convertClientArrays(i + 1);
	fn.glArrayElement(i);
}

void OSMesaDriver::nfglInterleavedArraysHelper(GLenum format, GLsizei stride, nfmemptr pointer)
{
	SDL_bool enable_texcoord, enable_normal, enable_color;
	int texcoord_size, color_size, vertex_size;
	nfmemptr texcoord_ptr, normal_ptr, color_ptr, vertex_ptr;
	GLenum color_type;
	int c, f;
	int defstride;
	
	f = ATARI_SIZEOF_FLOAT;
	c = f * ((4 * sizeof(GLubyte) + (f - 1)) / f);
	
	enable_texcoord=SDL_FALSE;
	enable_normal=SDL_FALSE;
	enable_color=SDL_FALSE;
	texcoord_size=  color_size = vertex_size = 0;
	texcoord_ptr = normal_ptr = color_ptr = vertex_ptr = pointer;
	color_type = GL_FLOAT;
	switch(format)
	{
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
			/* FIXME: GL_R1UI_* from GL_SUN_triangle_list not handled */
			glSetError(GL_INVALID_ENUM);
			return;
	}

	if (stride == 0)
	{
		stride = defstride;
	}

	fn.glDisableClientState(GL_EDGE_FLAG_ARRAY);
	fn.glDisableClientState(GL_INDEX_ARRAY);
	if(enable_normal) {
		fn.glEnableClientState(GL_NORMAL_ARRAY);
		fn.glNormalPointer(GL_FLOAT, stride, normal_ptr);
	} else {
		fn.glDisableClientState(GL_NORMAL_ARRAY);
	}
	if(enable_color) {
		fn.glEnableClientState(GL_COLOR_ARRAY);
		fn.glColorPointer(color_size, color_type, stride, color_ptr);
	} else {
		fn.glDisableClientState(GL_COLOR_ARRAY);
	}
	if(enable_texcoord) {
		fn.glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		fn.glTexCoordPointer(texcoord_size, GL_FLOAT, stride, texcoord_ptr);
	} else {
		fn.glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	}
	fn.glEnableClientState(GL_VERTEX_ARRAY);
	fn.glVertexPointer(vertex_size, GL_FLOAT, stride, vertex_ptr);
}

/*---
 * wrappers for functions that take float arguments, and sometimes only exist as
 * functions with double as arguments in GL
 ---*/

void OSMesaDriver::nfglFrustumf(const uint32_t *nf_params)
{
	GLfloat left = getStackedFloat(0);
	GLfloat right = getStackedFloat(1);
	GLfloat bottom = getStackedFloat(2);
	GLfloat top = getStackedFloat(3);
	GLfloat near_val = getStackedFloat(4);
	GLfloat far_val = getStackedFloat(5);
	D(bug("nfosmesa: glFrustumf(%f, %f, %f, %f, %f, %f)", left, right, bottom, top, near_val, far_val));
#if 0
	if (GL_ISAVAILABLE(glFrustumfOES))
		fn.glFrustumfOES(left, right, bottom, top, near_val, far_val);
	else
		fn.glFrustum(left, right, bottom, top, near_val, far_val);
#else
	GLfloat m[16];
	GLfloat x, y, A, B, C, D;
	
	if (near_val <= 0.0 ||
		far_val <= 0.0 ||
		near_val == far_val ||
		left == right ||
		top == bottom)
	{
		glSetError(GL_INVALID_VALUE);
		return;
	}

	x = (2.0F * near_val) / (right - left);
	y = (2.0F * near_val) / (top - bottom);
	A = (right + left) / (right - left);
	B = (top + bottom) / (top - bottom);
	C = -(far_val + near_val) / (far_val - near_val);
	D = -(2.0F * far_val * near_val) / (far_val - near_val);

	M(m, 0, 0) = x;     M(m, 0, 1) = 0.0F;  M(m, 0, 2) = A;      M(m, 0, 3) = 0.0F;
	M(m, 1, 0) = 0.0F;  M(m, 1, 1) = y;     M(m, 1, 2) = B;      M(m, 1, 3) = 0.0F;
	M(m, 2, 0) = 0.0F;  M(m, 2, 1) = 0.0F;  M(m, 2, 2) = C;      M(m, 2, 3) = D;
	M(m, 3, 0) = 0.0F;  M(m, 3, 1) = 0.0F;  M(m, 3, 2) = -1.0F;  M(m, 3, 3) = 0.0F;
	fn.glMultMatrixf(m);
#endif
}

void OSMesaDriver::nfglOrthof(const uint32_t *nf_params)
{
	GLfloat left = getStackedFloat(0);
	GLfloat right = getStackedFloat(1);
	GLfloat bottom = getStackedFloat(2);
	GLfloat top = getStackedFloat(3);
	GLfloat near_val = getStackedFloat(4);
	GLfloat far_val = getStackedFloat(5);
	D(bug("nfosmesa: glOrthof(%f, %f, %f, %f, %f, %f)", left, right, bottom, top, near_val, far_val));
#if 0
	if (GL_ISAVAILABLE(glOrthofOES))
		fn.glOrthofOES(left, right, bottom, top, near_val, far_val);
	else
		fn.glOrtho(left, right, bottom, top, near_val, far_val);
#else
	GLfloat m[16];

	if (left == right ||
		bottom == top ||
		near_val == far_val)
	{
		glSetError(GL_INVALID_VALUE);
		return;
	}

	M(m, 0, 0) = 2.0F / (right - left);
	M(m, 0, 1) = 0.0F;
	M(m, 0, 2) = 0.0F;
	M(m, 0, 3) = -(right + left) / (right - left);

	M(m, 1, 0) = 0.0F;
	M(m, 1, 1) = 2.0F / (top - bottom);
	M(m, 1, 2) = 0.0F;
	M(m, 1, 3) = -(top + bottom) / (top - bottom);

	M(m, 2, 0) = 0.0F;
	M(m, 2, 1) = 0.0F;
	M(m, 2, 2) = -2.0F / (far_val - near_val);
	M(m, 2, 3) = -(far_val + near_val) / (far_val - near_val);

	M(m, 3, 0) = 0.0F;
	M(m, 3, 1) = 0.0F;
	M(m, 3, 2) = 0.0F;
	M(m, 3, 3) = 1.0F;

	fn.glMultMatrixf(m);
#endif
}

/* glClearDepthf already exists in GL */

/*---
 * GLU functions
 ---*/

void OSMesaDriver::nfgluLookAtf(const uint32_t *nf_params)
{
	GLfloat eyeX = getStackedFloat(0);
	GLfloat eyeY = getStackedFloat(1);
	GLfloat eyeZ = getStackedFloat(2);
	GLfloat centerX = getStackedFloat(3);
	GLfloat centerY = getStackedFloat(4);
	GLfloat centerZ = getStackedFloat(5);
	GLfloat upX = getStackedFloat(6);
	GLfloat upY = getStackedFloat(7);
	GLfloat upZ = getStackedFloat(8);
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

void OSMesaDriver::nfgluLookAt(const uint32_t *nf_params)
{
	GLdouble eyeX = getStackedDouble(0);
	GLdouble eyeY = getStackedDouble(1);
	GLdouble eyeZ = getStackedDouble(2);
	GLdouble centerX = getStackedDouble(3);
	GLdouble centerY = getStackedDouble(4);
	GLdouble centerZ = getStackedDouble(5);
	GLdouble upX = getStackedDouble(6);
	GLdouble upY = getStackedDouble(7);
	GLdouble upZ = getStackedDouble(8);
	D(bug("nfosmesa: gluLookAt(%f, %f, %f, %f, %f, %f, %f, %f, %f)", eyeX, eyeY, eyeZ, centerX, centerY, centerZ, upX, upY, upZ));
	GLdouble m[16];
	GLdouble x[3], y[3], z[3];
	GLdouble mag;

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

	mag = sqrt(x[0] * x[0] + x[1] * x[1] + x[2] * x[2]);
	if (mag)
	{
		x[0] /= mag;
		x[1] /= mag;
		x[2] /= mag;
	}

	mag = sqrt(y[0] * y[0] + y[1] * y[1] + y[2] * y[2]);
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

	fn.glMultMatrixd(m);

	/* Translate Eye to Origin */
	fn.glTranslated(-eyeX, -eyeY, -eyeZ);
}

/*---
 * other functions needed by tiny_gl.ldg
 ---*/

void OSMesaDriver::nftinyglswapbuffer(const uint32_t *nf_params)
{
	memptr buffer = getStackedParameter(0);
	uint32_t ctx = cur_context;
	
	D(bug("nfosmesa: swapbuffer($%08x)", buffer));

	if (ctx > MAX_OSMESA_CONTEXTS)
		return;
	
	if (ctx != 0 && !contexts[ctx].ctx)
		return;

	if (ctx != 0)
	{
		OffscreenContext *context = contexts[ctx].ctx;
		context->MakeCurrent(buffer, context->type, context->width, context->height);
	}
}


/*--- helper methods for object buffers ---*/

vertexarray_t *OSMesaDriver::gl_get_array(GLenum pname)
{
	switch (pname)
	{
		case GL_VERTEX_ARRAY: return &contexts[cur_context].vertex;
		case GL_NORMAL_ARRAY: return &contexts[cur_context].normal;
		case GL_COLOR_ARRAY: return &contexts[cur_context].color;
		case GL_INDEX_ARRAY: return &contexts[cur_context].index;
		case GL_EDGE_FLAG_ARRAY: return &contexts[cur_context].edgeflag;
		case GL_TEXTURE_COORD_ARRAY: return &contexts[cur_context].texcoord;
		case GL_FOG_COORD_ARRAY: return &contexts[cur_context].fogcoord;
		case GL_SECONDARY_COLOR_ARRAY: return &contexts[cur_context].secondary_color;
		case GL_ELEMENT_ARRAY_BUFFER: return &contexts[cur_context].element;
		case GL_VARIANT_ARRAY_EXT: return &contexts[cur_context].element;
	}
	return NULL;
}

gl_buffer_t *OSMesaDriver::gl_get_buffer(GLuint name)
{
	/* TODO */
	UNUSED(name);
	return NULL;
}

gl_buffer_t *OSMesaDriver::gl_make_buffer(GLsizei size, nfcmemptr pointer)
{
	/* TODO */
	UNUSED(size);
	UNUSED(pointer);
	return NULL;
}

/*--- various helper functions ---*/

GLsizei OSMesaDriver::nfglPixelmapSize(GLenum pname)
{
	GLint size = 0;
	switch (pname)
	{
		case GL_PIXEL_MAP_I_TO_I: pname = GL_PIXEL_MAP_I_TO_I_SIZE; break;
		case GL_PIXEL_MAP_S_TO_S: pname = GL_PIXEL_MAP_S_TO_S_SIZE; break;
		case GL_PIXEL_MAP_I_TO_R: pname = GL_PIXEL_MAP_I_TO_R_SIZE; break;
		case GL_PIXEL_MAP_I_TO_G: pname = GL_PIXEL_MAP_I_TO_G_SIZE; break;
		case GL_PIXEL_MAP_I_TO_B: pname = GL_PIXEL_MAP_I_TO_B_SIZE; break;
		case GL_PIXEL_MAP_I_TO_A: pname = GL_PIXEL_MAP_I_TO_A_SIZE; break;
		case GL_PIXEL_MAP_R_TO_R: pname = GL_PIXEL_MAP_R_TO_R_SIZE; break;
		case GL_PIXEL_MAP_G_TO_G: pname = GL_PIXEL_MAP_G_TO_G_SIZE; break;
		case GL_PIXEL_MAP_B_TO_B: pname = GL_PIXEL_MAP_B_TO_B_SIZE; break;
		case GL_PIXEL_MAP_A_TO_A: pname = GL_PIXEL_MAP_A_TO_A_SIZE; break;
		default: return size;
	}
	fn.glGetIntegerv(pname, &size);
	return size;
}

int OSMesaDriver::nfglGetNumParams(GLenum pname)
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
	case GL_CURRENT_RASTER_NORMAL_SGIX:
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
	case GL_EVAL_VERTEX_ATTRIB2_NV: /* vertex normal */
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
	case GL_EVAL_VERTEX_ATTRIB0_NV: /* vertex position */
	case GL_EVAL_VERTEX_ATTRIB3_NV: /* primary color */
	case GL_EVAL_VERTEX_ATTRIB4_NV: /* secondary color */
	case GL_EVAL_VERTEX_ATTRIB8_NV: /* texture coord 0 */
	case GL_EVAL_VERTEX_ATTRIB9_NV: /* texture coord 1 */
	case GL_EVAL_VERTEX_ATTRIB10_NV: /* texture coord 2 */
	case GL_EVAL_VERTEX_ATTRIB11_NV: /* texture coord 3 */
	case GL_EVAL_VERTEX_ATTRIB12_NV: /* texture coord 4 */
	case GL_EVAL_VERTEX_ATTRIB13_NV: /* texture coord 5 */
	case GL_EVAL_VERTEX_ATTRIB14_NV: /* texture coord 6 */
	case GL_EVAL_VERTEX_ATTRIB15_NV: /* texture coord 7 */
	case GL_BOUNDING_BOX_OF_BOUNDING_BOXES_NV:
	case GL_PATH_OBJECT_BOUNDING_BOX_NV:
	case GL_PATH_FILL_BOUNDING_BOX_NV:
	case GL_PATH_STROKE_BOUNDING_BOX_NV:
	case GL_CURRENT_VERTEX_ATTRIB:
	case GL_REFERENCE_PLANE_EQUATION_SGIX:
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
	case GL_PATH_GEN_COEFF_NV:
		count = 16;
		break;
	case GL_COMPRESSED_TEXTURE_FORMATS:
		fn.glGetIntegerv(GL_NUM_COMPRESSED_TEXTURE_FORMATS, &count);
		break;
	case GL_SHADER_BINARY_FORMATS:
		fn.glGetIntegerv(GL_NUM_SHADER_BINARY_FORMATS, &count);
		break;
	case GL_PROGRAM_BINARY_FORMATS:
		fn.glGetIntegerv(GL_NUM_PROGRAM_BINARY_FORMATS, &count);
		break;
	case GL_EVAL_VERTEX_ATTRIB1_NV: /* vertex weight */
	case GL_EVAL_VERTEX_ATTRIB5_NV: /* fog coord */
		break;
	case GL_DEVICE_LUID_EXT:
		count = GL_LUID_SIZE_EXT;
		break;
	case GL_DEVICE_UUID_EXT:
	case GL_DRIVER_UUID_EXT:
		count = GL_UUID_SIZE_EXT;
		break;
	/* TODO: */
	case GL_COMBINER_INPUT_NV:
	case GL_COMBINER_COMPONENT_USAGE_NV:
	case GL_COMBINER_MAPPING_NV:
		break;
	}
	return count;
}

GLint OSMesaDriver::__glGetMap_Evalk(GLenum target)
{
	switch (target)
	{
		case GL_MAP1_INDEX:
		case GL_MAP1_TEXTURE_COORD_1:
		case GL_MAP2_INDEX:
		case GL_MAP2_TEXTURE_COORD_1:
			return 1;
		case GL_MAP1_TEXTURE_COORD_2:
		case GL_MAP2_TEXTURE_COORD_2:
			return 2;
		case GL_MAP1_VERTEX_3:
		case GL_MAP1_NORMAL:
		case GL_MAP1_TEXTURE_COORD_3:
		case GL_MAP2_VERTEX_3:
		case GL_MAP2_NORMAL:
		case GL_MAP2_TEXTURE_COORD_3:
			return 3;
		case GL_MAP1_VERTEX_4:
		case GL_MAP1_COLOR_4:
		case GL_MAP1_TEXTURE_COORD_4:
		case GL_MAP2_VERTEX_4:
		case GL_MAP2_COLOR_4:
		case GL_MAP2_TEXTURE_COORD_4:
		default:
			return 4;
	}
}

/*--- conversion macros used in generated code ---*/

void OSMesaDriver::gl_bind_buffer(GLenum target, GLuint buffer, GLuint first, GLuint count)
{
	fbo_buffer *fbo;
	
	switch (target)
	{
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


/* XXX todo */
void OSMesaDriver::gl_get_pointer(GLenum target, GLuint index, void **data)
{
	fbo_buffer *fbo;
	memptr *pdata = (memptr *)data;
	
	UNUSED(index); // FIXME
	switch (target)
	{
	case GL_ARRAY_BUFFER:
		fbo = &contexts[cur_context].buffer_bindings.array;
		break;
	case GL_ATOMIC_COUNTER_BUFFER:
		fbo = &contexts[cur_context].buffer_bindings.atomic_counter;
		break;
	case GL_COPY_READ_BUFFER:
		fbo = &contexts[cur_context].buffer_bindings.copy_read;
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
	case GL_SELECTION_BUFFER_POINTER:
		*pdata = (memptr)(uintptr_t)NFHost2AtariAddr(contexts[cur_context].select_buffer_atari);
		return;
	case GL_FEEDBACK_BUFFER_POINTER:
		*pdata = (memptr)(uintptr_t)NFHost2AtariAddr(contexts[cur_context].feedback_buffer_atari);
		return;
	default:
		glSetError(GL_INVALID_ENUM);
		return;
	}
	// not sure about this
	*pdata = (memptr)(uintptr_t)NFHost2AtariAddr(fbo->atari_pointer);
}

#include "nfosmesa_macros.h"
#include "nfosmesa/call-gl.c"

#endif /* NFOSMESA_SUPPORT */
