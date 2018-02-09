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

#if NFOSMESA_POINTER_AS_MEMARG
#define getStackedParameter(n) params[n]
#define getStackedParameter64(n) (((GLuint64)getStackedParameter(n) << 32) | (GLuint64)getStackedParameter((n) + 1))
#define getStackedFloat(n) Atari2HostFloat(getStackedParameter(n))
#define getStackedDouble(n) Atari2HostDouble(getStackedParameter(n), getStackedParameter((n) + 1))
#define getStackedPointer(n, t) getStackedParameter(n)
#define HostAddr(addr, t) (t)((addr) ? Atari2HostAddr(addr) : NULL)
#define AtariAddr(addr, t) addr
#define AtariOffset(addr) addr
#define NFHost2AtariAddr(addr) (void *)(uintptr_t)(addr)
#else
#define getStackedParameter(n) SDL_SwapBE32(ctx_ptr[n])
#define getStackedParameter64(n) SDL_SwapBE64(*((Uint64 *)&ctx_ptr[n]))
#define getStackedFloat(n) Atari2HostFloat(getStackedParameter(n))
#define getStackedDouble(n) Atari2HostDouble(getStackedParameter(n), getStackedParameter((n) + 1))
#define getStackedPointer(n, t) (t)(ctx_ptr[n] ? Atari2HostAddr(getStackedParameter(n)) : NULL)
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
	
#if NFOSMESA_POINTER_AS_MEMARG
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
	Uint32 params[NFOSMESA_MAXPARAMS];

	ctx_ptr = getParameter(1);

	for (unsigned int i = 0; i < paramcount[fncode]; i++)
		params[i] = ReadInt32(ctx_ptr + 4 * (i));
#else
	Uint32 *ctx_ptr;	/* Current parameter list */

	ctx_ptr = (Uint32 *)Atari2HostAddr(getParameter(1));
#endif
		
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
			ret = LenglGetString(
				getStackedParameter(0) /* Uint32 ctx */,
				getStackedParameter(1) /* GLenum name */);
			break;
		case NFOSMESA_PUTGLGETSTRING:
			D(funcname = "glGetString");
			PutglGetString(
				getStackedParameter(0) /* Uint32 ctx */,
				getStackedParameter(1) /* GLenum name */,
				getStackedPointer(2, GLubyte *) /* GLubyte *buffer */);
			break;
		case NFOSMESA_LENGLGETSTRINGI:
			D(funcname = "glGetStringi");
			ret = LenglGetStringi(
				getStackedParameter(0) /* Uint32 ctx */,
				getStackedParameter(1) /* GLenum name */,
				getStackedParameter(2) /* GLuint index */);
			break;
		case NFOSMESA_PUTGLGETSTRINGI:
			D(funcname = "glGetStringi");
			PutglGetStringi(
				getStackedParameter(0) /* Uint32 ctx */,
				getStackedParameter(1) /* GLenum name */,
				getStackedParameter(2) /* GLuint index */,
				getStackedPointer(3, GLubyte *) /* GLubyte *buffer */);
			break;

		case NFOSMESA_OSMESACREATECONTEXT:
			D(funcname = "OSMesaCreateContext");
			ret = OSMesaCreateContext(
				getStackedParameter(0) /* GLenum format */,
				getStackedParameter(1) /* Uint32 sharelist */);
			break;
		case NFOSMESA_OSMESACREATECONTEXTEXT:
			D(funcname = "OSMesaCreateContextExt");
			ret = OSMesaCreateContextExt(
				getStackedParameter(0) /* GLenum format */,
				getStackedParameter(1) /* GLint depthBits */,
				getStackedParameter(2) /* GLint stencilBits */,
				getStackedParameter(3) /* GLint accumBits */,
				getStackedParameter(4) /* Uint32 sharelist */);
			break;
		case NFOSMESA_OSMESADESTROYCONTEXT:
			D(funcname = "OSMesaDestroyContext");
			OSMesaDestroyContext(
				getStackedParameter(0) /* Uint32 ctx */);
			break;
		case NFOSMESA_OSMESAMAKECURRENT:
			D(funcname = "OSMesaMakeCurrent");
			ret = OSMesaMakeCurrent(
				getStackedParameter(0) /* Uint32 ctx */,
				getStackedParameter(1) /* memptr buffer */,
				getStackedParameter(2) /* GLenum type */,
				getStackedParameter(3) /* GLsizei width */,
				getStackedParameter(4) /* GLsizei height */);
			break;
		case NFOSMESA_OSMESAGETCURRENTCONTEXT:
			D(funcname = "OSMesaGetCurrentContext");
			ret = OSMesaGetCurrentContext();
			break;
		case NFOSMESA_OSMESAPIXELSTORE:
			D(funcname = "OSMesaPixelStore");
			OSMesaPixelStore(
				getStackedParameter(0) /* GLint pname */,
				getStackedParameter(1) /* GLint value */);
			break;
		case NFOSMESA_OSMESAGETINTEGERV:
			D(funcname = "OSMesaGetIntegerv");
			OSMesaGetIntegerv(
				getStackedParameter(0) /* GLint pname */,
				getStackedPointer(1, GLint *) /* GLint *values */);
			break;
		case NFOSMESA_OSMESAGETDEPTHBUFFER:
			D(funcname = "OSMesaGetDepthBuffer");
			ret = OSMesaGetDepthBuffer(
				getStackedParameter(0) /* Uint32 ctx */,
				getStackedPointer(1, GLint *) /* GLint *width */,
				getStackedPointer(2, GLint *) /* GLint *height */,
				getStackedPointer(3, GLint *) /* GLint *bytesPerValue */,
				getStackedPointer(4, void **) /* void **buffer */);
			break;
		case NFOSMESA_OSMESAGETCOLORBUFFER:
			D(funcname = "OSMesaGetColorBuffer");
			ret = OSMesaGetColorBuffer(
				getStackedParameter(0) /* Uint32 ctx */,
				getStackedPointer(1, GLint *) /* GLint *width */,
				getStackedPointer(2, GLint *) /* GLint *height */,
				getStackedPointer(3, GLint *) /* GLint *format */,
				getStackedPointer(4, void **) /* void **buffer */);
			break;
		case NFOSMESA_OSMESAGETPROCADDRESS:
			D(funcname = "OSMesaGetProcAddress");
			ret = OSMesaGetProcAddress(
				getStackedPointer(0, const char *) /* const char *funcName */);
			break;
		case NFOSMESA_OSMESACOLORCLAMP:
			D(funcname = "OSMesaColorClamp");
			OSMesaColorClamp(getStackedParameter(0) /* GLboolean enable */);
			break;
		case NFOSMESA_OSMESAPOSTPROCESS:
			D(funcname = "OSMesaPostprocess");
			{
				GLubyte tmp[safe_strlen(getStackedPointer(1, const char *)) + 1], *filter;
				filter = Atari2HostByteArray(sizeof(tmp), getStackedPointer(1, const GLubyte *), tmp);
				OSMesaPostprocess(
					getStackedParameter(0) /* Uint32 ctx */,
					(const char *)filter,
					getStackedParameter(2) /* GLuint enable_value */);
			}
			break;

		/*
		 * maybe FIXME: functions below usually need a current context,
		 * which is not checked here.
		 */
		case NFOSMESA_GLULOOKATF:
			D(funcname = "gluLookAtf");
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
			D(funcname = "glFrustumf");
			nfglFrustumf(
				getStackedFloat(0) /* GLfloat left */,
				getStackedFloat(1) /* GLfloat right */,
				getStackedFloat(2) /* GLfloat bottom */,
				getStackedFloat(3) /* GLfloat top */,
				getStackedFloat(4) /* GLfloat near_val */,
				getStackedFloat(5) /* GLfloat far_val */);
			break;
		
		case NFOSMESA_GLORTHOF:
			D(funcname = "glOrthof");
			nfglOrthof(
				getStackedFloat(0) /* GLfloat left */,
				getStackedFloat(1) /* GLfloat right */,
				getStackedFloat(2) /* GLfloat bottom */,
				getStackedFloat(3) /* GLfloat top */,
				getStackedFloat(4) /* GLfloat near_val */,
				getStackedFloat(5) /* GLfloat far_val */);
			break;
		
		case NFOSMESA_TINYGLSWAPBUFFER:
			D(funcname = "swapbuffer");
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


bool OSMesaDriver::SelectContext(Uint32 ctx)
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

Uint32 OSMesaDriver::OSMesaCreateContext( GLenum format, Uint32 sharelist )
{
	D(bug("nfosmesa: OSMesaCreateContext(0x%x, 0x%x)", format, sharelist));
	return OSMesaCreateContextExt(format, 16, 8, (format == OSMESA_COLOR_INDEX) ? 0 : 16, sharelist);
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


Uint32 OSMesaDriver::OSMesaCreateContextExt( GLenum format, GLint depthBits, GLint stencilBits, GLint accumBits, Uint32 sharelist )
{
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


void OSMesaDriver::OSMesaDestroyContext( Uint32 ctx )
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

GLboolean OSMesaDriver::OSMesaMakeCurrent( Uint32 ctx, memptr buffer, GLenum type, GLsizei width, GLsizei height )
{
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

Uint32 OSMesaDriver::OSMesaGetCurrentContext( void )
{
	Uint32 ctx;
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

void OSMesaDriver::OSMesaPixelStore(GLint pname, GLint value)
{
	D(bug("nfosmesa: OSMesaPixelStore(0x%x, %d)", pname, value));
	Uint32 ctx = cur_context;
	if (ctx && contexts[ctx].ctx)
		contexts[ctx].ctx->PixelStore(pname, value);
}

#if NFOSMESA_POINTER_AS_MEMARG
void OSMesaDriver::OSMesaGetIntegerv(GLint pname, memptr value )
#else
void OSMesaDriver::OSMesaGetIntegerv(GLint pname, GLint *value )
#endif
{
	GLint tmp = 0;
	Uint32 ctx = cur_context;
	
	D(bug("nfosmesa: OSMesaGetIntegerv(0x%x)", pname));
	if (ctx && contexts[ctx].ctx)
		if (!contexts[ctx].ctx->GetIntegerv(pname, &tmp))
			glSetError(GL_INVALID_ENUM);
	Host2AtariIntArray(1, &tmp, value);
}

#if NFOSMESA_POINTER_AS_MEMARG
GLboolean OSMesaDriver::OSMesaGetDepthBuffer(Uint32 ctx, memptr width, memptr height, memptr bytesPerValue, memptr buffer )
#else
GLboolean OSMesaDriver::OSMesaGetDepthBuffer(Uint32 ctx, GLint *width, GLint *height, GLint *bytesPerValue, void **buffer )
#endif
{
	GLint w, h, bpp;
	memptr b;
	D(bug("nfosmesa: OSMesaGetDepthBuffer(%u)", ctx));
	if (!SelectContext(ctx) || ctx == 0)
		return GL_FALSE;
	contexts[ctx].ctx->GetDepthBuffer(&w, &h, &bpp, &b);
	Host2AtariIntArray(1, &w, width);
	Host2AtariIntArray(1, &h, height);
	Host2AtariIntArray(1, &bpp, bytesPerValue);
	Host2AtariIntArray(1, &b, AtariAddr(buffer, memptr *));
	return GL_TRUE;
}

#if NFOSMESA_POINTER_AS_MEMARG
GLboolean OSMesaDriver::OSMesaGetColorBuffer(Uint32 ctx, memptr width, memptr height, memptr format, memptr buffer )
#else
GLboolean OSMesaDriver::OSMesaGetColorBuffer(Uint32 ctx, GLint *width, GLint *height, GLint *format, void **buffer )
#endif
{
	GLint w, h, f;
	memptr b;
	D(bug("nfosmesa: OSMesaGetColorBuffer(%u)", ctx));
	if (!SelectContext(ctx) || ctx == 0)
		return GL_FALSE;
	contexts[ctx].ctx->GetColorBuffer(&w, &h, &f, &b);
	Host2AtariIntArray(1, &w, width);
	Host2AtariIntArray(1, &h, height);
	Host2AtariIntArray(1, &f, format);
	Host2AtariIntArray(1, &b, AtariAddr(buffer, memptr *));
	return GL_TRUE;
}

unsigned int OSMesaDriver::OSMesaGetProcAddress( nfcmemptr funcname )
{
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

void OSMesaDriver::OSMesaColorClamp(GLboolean enable)
{
	D(bug("nfosmesa: OSMesaColorClamp(%d)", enable));
	Uint32 ctx = cur_context;
	if (ctx == 0 || !contexts[ctx].ctx)
		return;
	contexts[ctx].ctx->ColorClamp(enable);
}

void OSMesaDriver::OSMesaPostprocess(Uint32 ctx, const char *filter, GLuint enable_value)
{
	D(bug("nfosmesa: OSMesaPostprocess(%u, %s, %d)", ctx, filter, enable_value));
	if (ctx > MAX_OSMESA_CONTEXTS || ctx == 0 || !contexts[ctx].ctx)
		return;
	/* no SelectContext() here; OSMesaPostprocess must be called without having a current context */
	contexts[ctx].ctx->Postprocess(filter, enable_value);
}

Uint32 OSMesaDriver::LenglGetString(Uint32 ctx, GLenum name)
{
	UNUSED(ctx);
	D(bug("nfosmesa: LenglGetString(%u, 0x%x)", ctx, name));
	if (!GL_ISAVAILABLE(glGetString)) return 0;
	const char *s = (const char *)fn.glGetString(name);
	if (s == NULL) return 0;
	return strlen(s);
}

#if NFOSMESA_POINTER_AS_MEMARG
void OSMesaDriver::PutglGetString(Uint32 ctx, GLenum name, memptr buffer)
#else
void OSMesaDriver::PutglGetString(Uint32 ctx, GLenum name, GLubyte *buffer)
#endif
{
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

Uint32 OSMesaDriver::LenglGetStringi(Uint32 ctx, GLenum name, GLuint index)
{
	UNUSED(ctx);
	D(bug("nfosmesa: LenglGetStringi(%u, 0x%x, %u)", ctx, name, index));
	if (!GL_ISAVAILABLE(glGetStringi)) return (Uint32)-1;
	const char *s = (const char *)fn.glGetStringi(name, index);
	if (s == NULL) return (Uint32)-1;
	return strlen(s);
}

#if NFOSMESA_POINTER_AS_MEMARG
void OSMesaDriver::PutglGetStringi(Uint32 ctx, GLenum name, GLuint index, memptr buffer)
#else
void OSMesaDriver::PutglGetStringi(Uint32 ctx, GLenum name, GLuint index, GLubyte *buffer)
#endif
{
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

void OSMesaDriver::ConvertContext(Uint32 ctx)
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
		OSMesaDriver::Host2AtariIntArray(count, (const GLuint *)src, AtariAddr(dst, Uint32 *));
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
		OSMesaDriver::Atari2HostIntArray(count, AtariAddr(pixels, const Uint32 *), (GLuint *)result);
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
					Atari2HostIntArray(array.size, AtariAddr(src, const Uint32 *), (GLuint *)dst);
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

void OSMesaDriver::nfglOrthof(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat near_val, GLfloat far_val)
{
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
	Uint32 ctx = cur_context;
	
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

/*--- Functions that return a 64-bit value ---*/

/*
 * The NF interface currently only returns a single value in D0,
 * so the call has to pass an extra parameter, the location where to
 * store the result value
 */
#define FN_GLGETIMAGEHANDLEARB(texture, level, layered, layer, format) \
	memptr retaddr = getParameter(2); \
	GLuint64 ret = fn.glGetImageHandleARB(texture, level, layered, layer, format); \
	WriteInt64(retaddr, ret); \
	return 0

#define FN_GLGETIMAGEHANDLENV(texture, level, layered, layer, format) \
	memptr retaddr = getParameter(2); \
	GLuint64 ret = fn.glGetImageHandleNV(texture, level, layered, layer, format); \
	WriteInt64(retaddr, ret); \
	return 0

#define FN_GLGETTEXTUREHANDLEARB(texture) \
	memptr retaddr = getParameter(2); \
	GLuint64 ret = fn.glGetTextureHandleARB(texture); \
	WriteInt64(retaddr, ret); \
	return 0

#define FN_GLGETTEXTUREHANDLENV(texture) \
	memptr retaddr = getParameter(2); \
	GLuint64 ret = fn.glGetTextureHandleNV(texture); \
	WriteInt64(retaddr, ret); \
	return 0

#define FN_GLGETTEXTURESAMPLERHANDLEARB(texturem, sampler) \
	memptr retaddr = getParameter(2); \
	GLuint64 ret = fn.glGetTextureSamplerHandleARB(texture, sampler); \
	WriteInt64(retaddr, ret); \
	return 0

#define FN_GLGETTEXTURESAMPLERHANDLENV(texturem, sampler) \
	memptr retaddr = getParameter(2); \
	GLuint64 ret = fn.glGetTextureSamplerHandleNV(texture, sampler); \
	WriteInt64(retaddr, ret); \
	return 0

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

/* #define FN_GLULOOKAT(eyeX, eyeY, eyeZ, centerX, centerY, centerZ, upX, upY, upZ) */

/* FIXME for glTexImage*, glTexSubImage* etc:
If a non-zero named buffer object is bound to the
GL_PIXEL_UNPACK_BUFFER target (see glBindBuffer) while a texture image
is specified, data is treated as a byte offset into the buffer object's
data store.
*/

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
				Host2AtariFloatArray(ret, (const GLfloat *)contexts[cur_context].feedback_buffer_host, AtariAddr(contexts[cur_context].feedback_buffer_atari, GLfloat *)); \
				break; \
			case GL_FIXED: \
				Host2AtariIntArray(ret, (const GLfixed *)contexts[cur_context].feedback_buffer_host, AtariAddr(contexts[cur_context].feedback_buffer_atari, GLfixed *)); \
				break; \
			} \
		} \
		break; \
	case GL_SELECT: \
		if (ret > 0 && contexts[cur_context].select_buffer_host) { \
			/* FIXME: scan select_buffer to get exact number of items to copy */ \
			Host2AtariIntArray(contexts[cur_context].select_buffer_size, contexts[cur_context].select_buffer_host, AtariAddr(contexts[cur_context].select_buffer_atari, GLuint *)); \
		} \
		break; \
	} \
	return ret

/* -------------------------------------------------------------------------- */

/*
 * GL_EXT_color_subtable
 */
#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV || NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLCOLORSUBTABLEEXT(target, start, count, format, type, table) \
	pixelBuffer buf(*this); \
	void *tmp = buf.convertPixels(count, 1, 1, format, type, (nfmemptr)table); \
	fn.glColorSubTableEXT(target, start, count, format, type, tmp)
#else
#define FN_GLCOLORSUBTABLEEXT(target, start, count, format, type, table) \
	fn.glColorSubTableEXT(target, start, count, format, type, HostAddr(table, const void *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_NV_register_combiners
 */

/* -------------------------------------------------------------------------- */

/*
 * GL_NV_register_combiners2
 */

/* -------------------------------------------------------------------------- */

/*
 * GL_ARB_transpose_matrix
 */

/* -------------------------------------------------------------------------- */

/*
 * GL_ARB_multitexture
 */

/* -------------------------------------------------------------------------- */

/*
 * GL_EXT_cull_vertex
 */

/* -------------------------------------------------------------------------- */

/*
 * GL_EXT_debug_marker
 */

/* -------------------------------------------------------------------------- */

/*
 * GL_ARB_debug_output
 */

/* -------------------------------------------------------------------------- */

/*
 * GL_AMD_debug_output
 */

/* -------------------------------------------------------------------------- */

/*
 * GL_SGIX_polynomial_ffd
 */

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLDEFORMATIONMAP3DSGIX(target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, w1, w2, wstride, worder, points) \
	GLdouble *tmp; \
	nfcmemptr ptr; \
	GLint i, j, k; \
	GLint const size = __glGetMap_Evalk(target); \
	 \
	tmp = (GLdouble *)malloc(size * uorder * vorder * worder * sizeof(GLdouble)); \
	if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
	ptr = (nfcmemptr)points; \
	for (i = 0; i < uorder; i++) { \
		for (j = 0; j < vorder; j++) { \
			for (k = 0; k < worder; k++) { \
				Atari2HostDoubleArray(size, AtariAddr(ptr + (i * ustride + j * vstride + k * wstride) * ATARI_SIZEOF_DOUBLE, const GLdouble *), &tmp[((i * vorder + j) * worder + k) * size]); \
			} \
		} \
	} \
	fn.glDeformationMap3dSGIX(target, \
		u1, u2, size * vorder * worder, uorder, \
		v1, v2, size * vorder, vorder, \
		w1, w2, size, worder, tmp \
	); \
	free(tmp)
#else
#define FN_GLDEFORMATIONMAP3DSGIX(target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, w1, w2, wstride, worder, points) \
	fn.glDeformationMap3dSGIX(target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, w1, w2, wstride, worder, HostAddr(points, const GLdouble *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLDEFORMATIONMAP3FSGIX(target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, w1, w2, wstride, worder, points) \
	GLfloat *tmp; \
	nfcmemptr ptr; \
	GLint i, j, k; \
	GLint const size = __glGetMap_Evalk(target); \
	 \
	tmp = (GLfloat *)malloc(size * uorder * vorder * worder * sizeof(GLfloat)); \
	if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
	ptr = (nfcmemptr)points; \
	for (i = 0; i < uorder; i++) { \
		for (j = 0; j < vorder; j++) { \
			for (k = 0; k < worder; k++) { \
				Atari2HostFloatArray(size, AtariAddr(ptr + (i * ustride + j * vstride + k * wstride) * ATARI_SIZEOF_FLOAT, const GLfloat *), &tmp[((i * vorder + j) * worder + k) * size]); \
			} \
		} \
	} \
	fn.glDeformationMap3fSGIX(target, \
		u1, u2, size * vorder * worder, uorder, \
		v1, v2, size * vorder, vorder, \
		w1, w2, size, worder, tmp \
	); \
	free(tmp)
#else
#define FN_GLDEFORMATIONMAP3FSGIX(target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, w1, w2, wstride, worder, points) \
	fn.glDeformationMap3fSGIX(target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, w1, w2, wstride, worder, HostAddr(points, const GLfloat *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_APPLE_fence
 */

/* -------------------------------------------------------------------------- */

/*
 * GL_NV_fence
 */

/* -------------------------------------------------------------------------- */

/*
 * GL_EXT_framebuffer_object
 */

#if NFOSMESA_NEED_INT_CONV
#define FN_GLDELETERENDERBUFFERSEXT(n, renderbuffers) \
	GLuint tmp[MAX(n, 0)], *ptmp; \
	ptmp = Atari2HostIntArray(n, renderbuffers, tmp); \
	fn.glDeleteRenderbuffersEXT(n, ptmp)
#else
#define FN_GLDELETERENDERBUFFERSEXT(n, renderbuffers) \
	fn.glDeleteRenderbuffersEXT(n, HostAddr(renderbuffers, const GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGENRENDERBUFFERSEXT(n, renderbuffers) \
	GLsizei const size = MAX(n, 0); \
	GLuint tmp[size]; \
	fn.glGenRenderbuffersEXT(n, tmp); \
	Host2AtariIntArray(n, tmp, renderbuffers)
#else
#define FN_GLGENRENDERBUFFERSEXT(n, renderbuffers) \
	fn.glGenRenderbuffersEXT(n, HostAddr(renderbuffers, GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETRENDERBUFFERPARAMETERIVEXT(target, pname, params) \
	GLint const size = 1; \
	GLint tmp[MAX(size, 16)]; \
	fn.glGetRenderbufferParameterivEXT(target, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETRENDERBUFFERPARAMETERIVEXT(target, pname, params) \
	fn.glGetRenderbufferParameterivEXT(target, pname, HostAddr(params, GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLDELETEFRAMEBUFFERSEXT(n, framebuffers) \
	GLuint tmp[MAX(n, 0)], *ptmp; \
	ptmp = Atari2HostIntArray(n, framebuffers, tmp); \
	fn.glDeleteFramebuffersEXT(n, ptmp)
#else
#define FN_GLDELETEFRAMEBUFFERSEXT(n, framebuffers) \
	fn.glDeleteFramebuffersEXT(n, HostAddr(framebuffers, const GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGENFRAMEBUFFERSEXT(n, framebuffers) \
	GLsizei const size = MAX(n, 0); \
	GLuint tmp[size]; \
	fn.glGenFramebuffersEXT(n, tmp); \
	Host2AtariIntArray(n, tmp, framebuffers)
#else
#define FN_GLGENFRAMEBUFFERSEXT(n, framebuffers) \
	fn.glGenFramebuffersEXT(n, HostAddr(framebuffers, GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETFRAMEBUFFERATTACHMENTPARAMETERIVEXT(target, attachment, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLint tmp[MAX(size, 16)]; \
	fn.glGetFramebufferAttachmentParameterivEXT(target, attachment, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETFRAMEBUFFERATTACHMENTPARAMETERIVEXT(target, attachment, pname, params) \
	fn.glGetFramebufferAttachmentParameterivEXT(target, attachment, pname, HostAddr(params, GLint *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_AMD_name_gen_delete
 */
#if NFOSMESA_NEED_INT_CONV
#define FN_GLDELETENAMESAMD(identifier, num, names) \
	GLuint tmp[MAX(num, 0)], *ptmp; \
	ptmp = Atari2HostIntArray(num, names, tmp); \
	fn.glDeleteNamesAMD(identifier, num, ptmp)
#else
#define FN_GLDELETENAMESAMD(identifier, num, names) \
	fn.glDeleteNamesAMD(identifier, num, HostAddr(names, const GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGENNAMESAMD(identifer, num, names) \
	GLsizei const size = MAX(num, 0); \
	GLuint tmp[size]; \
	fn.glGenNamesAMD(identifier, num, tmp); \
	Host2AtariIntArray(num, tmp, names)
#else
#define FN_GLGENNAMESAMD(identifer, num, names) \
	fn.glGenNamesAMD(identifer, num, HostAddr(names, GLuint *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_APPLE_vertex_array_object
 */

#if NFOSMESA_NEED_INT_CONV
#define FN_GLDELETEVERTEXARRAYSAPPLE(n, arrays) \
	GLuint tmp[MAX(n, 0)], *ptmp; \
	ptmp = Atari2HostIntArray(n, arrays, tmp); \
	fn.glDeleteVertexArraysAPPLE(n, ptmp)
#else
#define FN_GLDELETEVERTEXARRAYSAPPLE(n, arrays) \
	fn.glDeleteVertexArraysAPPLE(n, HostAddr(arrays, const GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGENVERTEXARRAYSAPPLE(n, arrays) \
	GLsizei const size = MAX(n, 0); \
	GLuint tmp[size]; \
	fn.glGenVertexArraysAPPLE(n, tmp); \
	Host2AtariIntArray(n, tmp, arrays)
#else
#define FN_GLGENVERTEXARRAYSAPPLE(n, arrays) \
	fn.glGenVertexArraysAPPLE(n, HostAddr(arrays, GLuint *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_SGIS_detail_texture
 */
#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETDETAILTEXFUNCSGIS(target, points) \
	GLint n = 0; \
	fn.glGetTexParameteriv(target, GL_DETAIL_TEXTURE_FUNC_POINTS_SGIS, &n); \
	GLfloat tmp[MAX(n, 0) * 2]; \
	fn.glGetDetailTexFuncSGIS(target, tmp); \
	Host2AtariFloatArray(n * 2, tmp, points)
#else
#define FN_GLGETDETAILTEXFUNCSGIS(target, points) \
	fn.glGetDetailTexFuncSGIS(target, HostAddr(points, GLfloat *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_ARB_draw_buffers
 */
#if NFOSMESA_NEED_INT_CONV
#define FN_GLDRAWBUFFERSARB(n, bufs) \
	GLenum tmp[MAX(n, 0)], *ptmp; \
	ptmp = Atari2HostIntArray(n, bufs, tmp); \
	fn.glDrawBuffersARB(n, ptmp)
#else
#define FN_GLDRAWBUFFERSARB(n, bufs) \
	fn.glDrawBuffersARB(n, HostAddr(bufs, const GLenum *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_ATI_draw_buffers
 */
#if NFOSMESA_NEED_INT_CONV
#define FN_GLDRAWBUFFERSATI(n, bufs) \
	GLenum tmp[MAX(n, 0)], *ptmp; \
	ptmp = Atari2HostIntArray(n, bufs, tmp); \
	fn.glDrawBuffersATI(n, ptmp)
#else
#define FN_GLDRAWBUFFERSATI(n, bufs) \
	fn.glDrawBuffersATI(n, HostAddr(bufs, const GLenum *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_ARB_draw_instanced
 */
#define FN_GLDRAWELEMENTSINSTANCEDARB(mode, count, type, indices, instancecount) \
	pixelBuffer buf(*this); \
	void *tmp; \
	convertClientArrays(count); \
	switch(type) { \
	case GL_UNSIGNED_BYTE: \
	case GL_UNSIGNED_SHORT: \
	case GL_UNSIGNED_INT: \
		tmp = buf.convertArray(count, type, (nfmemptr)indices); \
		break; \
	default: \
		glSetError(GL_INVALID_ENUM); \
		return; \
	} \
	fn.glDrawElementsInstancedARB(mode, count, type, tmp, instancecount)

#define FN_GLDRAWARRAYSINSTANCEDARB(mode, first, count, instancecount) \
	convertClientArrays(first + count); \
	fn.glDrawArraysInstancedARB(mode, first, count, instancecount)

/* -------------------------------------------------------------------------- */

/*
 * GL_EXT_draw_instanced
 */
#define FN_GLDRAWELEMENTSINSTANCEDEXT(mode, count, type, indices, instancecount) \
	void *tmp; \
	pixelBuffer buf(*this); \
	convertClientArrays(count); \
	switch(type) { \
	case GL_UNSIGNED_BYTE: \
	case GL_UNSIGNED_SHORT: \
	case GL_UNSIGNED_INT: \
		tmp = buf.convertArray(count, type, (nfmemptr)indices); \
		break; \
	default: \
		glSetError(GL_INVALID_ENUM); \
		return; \
	} \
	fn.glDrawElementsInstancedEXT(mode, count, type, tmp, instancecount)

#define FN_GLDRAWARRAYSINSTANCEDEXT(mode, first, count, instancecount) \
	convertClientArrays(first + count); \
	fn.glDrawArraysInstancedEXT(mode, first, count, instancecount)

/* -------------------------------------------------------------------------- */

/*
 * GL_EXT_draw_range_elements
 */
#define FN_GLDRAWRANGEELEMENTSEXT(mode, start, end, count, type, indices) \
	void *tmp; \
	pixelBuffer buf(*this); \
	convertClientArrays(count); \
	switch(type) { \
	case GL_UNSIGNED_BYTE: \
	case GL_UNSIGNED_SHORT: \
	case GL_UNSIGNED_INT: \
		tmp = buf.convertArray(count, type, (nfmemptr)indices); \
		break; \
	default: \
		glSetError(GL_INVALID_ENUM); \
		return; \
	} \
	fn.glDrawRangeElementsEXT(mode, start, end, count, type, tmp)

/* -------------------------------------------------------------------------- */

/*
 * GL_EXT_fog_coord
 */
#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLFOGCOORDDVEXT(coord) \
	GLdouble tmp[1]; \
	Atari2HostDoubleArray(1, coord, tmp); \
	fn.glFogCoorddvEXT(tmp)
#else
#define FN_GLFOGCOORDDVEXT(coord) \
	fn.glFogCoorddvEXT(HostAddr(coord, const GLdouble *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLFOGCOORDFVEXT(coord) \
	GLfloat tmp[1]; \
	Atari2HostFloatArray(1, coord, tmp); \
	fn.glFogCoordfvEXT(tmp)
#else
#define FN_GLFOGCOORDFVEXT(coord) \
	fn.glFogCoordfvEXT(HostAddr(coord, const GLfloat *))
#endif

#define FN_GLFOGCOORDPOINTEREXT(type, stride, pointer) \
	GLint texunit = 0; \
	fn.glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE, &texunit); \
	setupClientArray(texunit, contexts[cur_context].fogcoord, 1, type, stride, 0, 0, AtariAddr(pointer, nfmemptr)); \
	contexts[cur_context].fogcoord.vendor = NFOSMESA_VENDOR_EXT

/* -------------------------------------------------------------------------- */

/*
 * GL_SGIS_fog_function
 */
#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLFOGFUNCSGIS(n, points) \
	GLfloat tmp[MAX(n, 0) * 2], *ptmp; \
	ptmp = Atari2HostFloatArray(n * 2, points, tmp); \
	fn.glFogFuncSGIS(n, ptmp)
#else
#define FN_GLFOGFUNCSGIS(n, points) \
	fn.glFogFuncSGIS(n, HostAddr(points, const GLfloat *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETFOGFUNCSGIS(points) \
	GLint size = 0; \
	fn.glGetIntegerv(GL_FOG_FUNC_POINTS_SGIS, &size); \
	GLfloat tmp[MAX(size * 2, 0)]; \
	fn.glGetFogFuncSGIS(tmp); \
	Host2AtariFloatArray(size * 2, tmp, points)
#else
#define FN_GLGETFOGFUNCSGIS(points) \
	fn.glGetFogFuncSGIS(HostAddr(points, GLfloat *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_SGIX_fragment_lighting
 */
#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLFRAGMENTLIGHTFVSGIX(light, pname, params) \
	GLfloat tmp[4]; \
	int size = nfglGetNumParams(pname); \
	Atari2HostFloatArray(size, params, tmp); \
	fn.glFragmentLightfvSGIX(light, pname, tmp)
#else
#define FN_GLFRAGMENTLIGHTFVSGIX(light, pname, params) \
	fn.glFragmentLightfvSGIX(light, pname, HostAddr(params, const GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLFRAGMENTLIGHTIVSGIX(light, pname, params) \
	GLint tmp[4]; \
	int const size = nfglGetNumParams(pname); \
	Atari2HostIntArray(size, params, tmp); \
	fn.glFragmentLightivSGIX(light, pname, tmp)
#else
#define FN_GLFRAGMENTLIGHTIVSGIX(light, pname, params) \
	fn.glFragmentLightivSGIX(light, pname, HostAddr(params, const GLint *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLFRAGMENTLIGHTMODELFVSGIX(pname, params) \
	GLfloat tmp[4]; \
	int const size = nfglGetNumParams(pname); \
	Atari2HostFloatArray(size, params, tmp); \
	fn.glFragmentLightModelfvSGIX(pname, tmp)
#else
#define FN_GLFRAGMENTLIGHTMODELFVSGIX(pname, params) \
	fn.glFragmentLightModelfvSGIX(pname, HostAddr(params, const GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLFRAGMENTLIGHTMODELIVSGIX(pname, params) \
	GLint tmp[4]; \
	int const size = nfglGetNumParams(pname); \
	Atari2HostIntArray(size, params, tmp); \
	fn.glFragmentLightModelivSGIX(pname, tmp)
#else
#define FN_GLFRAGMENTLIGHTMODELIVSGIX(pname, params) \
	fn.glFragmentLightModelivSGIX(pname, HostAddr(params, const GLint *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLFRAGMENTMATERIALFVSGIX(face, pname, params) \
	GLfloat tmp[4]; \
	int const size = nfglGetNumParams(pname); \
	Atari2HostFloatArray(size, params, tmp); \
	fn.glFragmentMaterialfvSGIX(face, pname, tmp)
#else
#define FN_GLFRAGMENTMATERIALFVSGIX(face, pname, params) \
	fn.glFragmentMaterialfvSGIX(face, pname, HostAddr(params, const GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLFRAGMENTMATERIALIVSGIX(face, pname, params) \
	GLint tmp[4]; \
	int const size = nfglGetNumParams(pname); \
	Atari2HostIntArray(size, params, tmp); \
	fn.glFragmentMaterialivSGIX(face, pname, tmp)
#else
#define FN_GLFRAGMENTMATERIALIVSGIX(face, pname, params) \
	fn.glFragmentMaterialivSGIX(face, pname, HostAddr(params, const GLint *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETFRAGMENTLIGHTFVSGIX(light, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLfloat tmp[size]; \
	fn.glGetFragmentLightfvSGIX(light, pname, tmp); \
	Host2AtariFloatArray(size, tmp, params)
#else
#define FN_GLGETFRAGMENTLIGHTFVSGIX(size, tmp, params) \
	fn.glGetFragmentLightfvSGIX(size, tmp, HostAddr(params, GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETFRAGMENTLIGHTIVSGIX(light, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLint tmp[size]; \
	fn.glGetFragmentLightivSGIX(light, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETFRAGMENTLIGHTIVSGIX(size, tmp, params) \
	fn.glGetFragmentLightivSGIX(size, tmp, HostAddr(params, GLint *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETFRAGMENTMATERIALFVSGIX(face, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLfloat tmp[size]; \
	fn.glGetFragmentMaterialfvSGIX(face, pname, tmp); \
	Host2AtariFloatArray(size, tmp, params)
#else
#define FN_GLGETFRAGMENTMATERIALFVSGIX(face, tmp, params) \
	fn.glGetFragmentMaterialfvSGIX(face, tmp, HostAddr(params, GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETFRAGMENTMATERIALIVSGIX(face, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLint tmp[size]; \
	fn.glGetFragmentMaterialivSGIX(face, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETFRAGMENTMATERIALIVSGIX(face, tmp, params) \
	fn.glGetFragmentMaterialivSGIX(face, tmp, HostAddr(params, GLint *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_EXT_coordinate_frame
 */

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLTANGENT3BVEXT(v) \
	GLbyte tmp[3]; \
	Atari2HostByteArray(3, v, tmp); \
	fn.glTangent3bvEXT(tmp)
#else
#define FN_GLTANGENT3BVEXT(v) \
	fn.glTangent3bvEXT(HostAddr(v, const GLbyte *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLTANGENT3DVEXT(v) \
	GLdouble tmp[3]; \
	Atari2HostDoubleArray(3, v, tmp); \
	fn.glTangent3dvEXT(tmp)
#else
#define FN_GLTANGENT3DVEXT(v) \
	fn.glTangent3dvEXT(HostAddr(v, const GLdouble *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLTANGENT3FVEXT(v) \
	GLfloat tmp[3]; \
	Atari2HostFloatArray(3, v, tmp); \
	fn.glTangent3fvEXT(tmp)
#else
#define FN_GLTANGENT3FVEXT(v) \
	fn.glTangent3fvEXT(HostAddr(v, const GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLTANGENT3IVEXT(v) \
	GLint tmp[3]; \
	Atari2HostIntArray(3, v, tmp); \
	fn.glTangent3ivEXT(tmp)
#else
#define FN_GLTANGENT3IVEXT(v) \
	fn.glTangent3ivEXT(HostAddr(v, const GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLTANGENT3SVEXT(v) \
	GLshort tmp[3]; \
	Atari2HostShortArray(3, v, tmp); \
	fn.glTangent3svEXT(tmp)
#else
#define FN_GLTANGENT3SVEXT(v) \
	fn.glTangent3svEXT(HostAddr(v, const GLshort *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLBINORMAL3BVEXT(v) \
	GLbyte tmp[3]; \
	Atari2HostByteArray(3, v, tmp); \
	fn.glBinormal3bvEXT(tmp)
#else
#define FN_GLBINORMAL3BVEXT(v) \
	fn.glBinormal3bvEXT(HostAddr(v, const GLbyte *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLBINORMAL3DVEXT(v) \
	GLdouble tmp[3]; \
	Atari2HostDoubleArray(3, v, tmp); \
	fn.glBinormal3dvEXT(tmp)
#else
#define FN_GLBINORMAL3DVEXT(v) \
	fn.glBinormal3dvEXT(HostAddr(v, const GLdouble *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLBINORMAL3FVEXT(v) \
	GLfloat tmp[3]; \
	Atari2HostFloatArray(3, v, tmp); \
	fn.glBinormal3fvEXT(tmp)
#else
#define FN_GLBINORMAL3FVEXT(v) \
	fn.glBinormal3fvEXT(HostAddr(v, const GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLBINORMAL3IVEXT(v) \
	GLint tmp[3]; \
	Atari2HostIntArray(3, v, tmp); \
	fn.glBinormal3ivEXT(tmp)
#else
#define FN_GLBINORMAL3IVEXT(v) \
	fn.glBinormal3ivEXT(HostAddr(v, const GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLBINORMAL3SVEXT(v) \
	GLshort tmp[3]; \
	Atari2HostShortArray(3, v, tmp); \
	fn.glBinormal3svEXT(tmp)
#else
#define FN_GLBINORMAL3SVEXT(v) \
	fn.glBinormal3svEXT(HostAddr(v, const GLshort *))
#endif

/* glBinormalPointerEXT belongs to never completed EXT_coordinate_frame */
#define FN_GLBINORMALPOINTEREXT(type, stride, pointer) \
	fn.glBinormalPointerEXT(type, stride, HostAddr(pointer, const void *))

/* glTangentPointerEXT belongs to never completed EXT_coordinate_frame */
#define FN_GLTANGENTPOINTEREXT(type, stride, pointer) \
	fn.glTangentPointerEXT(type, stride, HostAddr(pointer, const void *))

/* -------------------------------------------------------------------------- */

/*
 * GL_NV_evaluators
 *
 * Status: Discontinued.
 *
 * NVIDIA no longer supports this extension in driver updates after
 * November 2002.  Instead, use conventional OpenGL evaluators or
 * tessellate surfaces within your application.
 */
/*
 * note that, unlike glMap2(), ustride and vstride here are in terms
 * of bytes, not floats
 */
#if NFOSMESA_NEED_FLOAT_CONV || NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLMAPCONTROLPOINTSNV(target, index, type, ustride, vstride, uorder, vorder, packed, points) \
	GLsizei size = 4; \
	GLsizei count; \
	GLsizei i, j; \
	nfcmemptr src; \
	char *tmp; \
	char *dst; \
	nfcmemptr start; \
	GLsizei ustride_host, vstride_host; \
	switch (target) { \
		case GL_EVAL_2D_NV: \
			count = uorder * vorder; \
			break; \
		case GL_EVAL_TRIANGULAR_2D_NV: \
			if (uorder != vorder) { glSetError(GL_INVALID_OPERATION); return; } \
			count = uorder * (uorder + 1) / 2; \
			break; \
		default: glSetError(GL_INVALID_ENUM); return; \
	} \
	switch (type) { \
		case GL_FLOAT: \
			vstride_host = size * sizeof(GLfloat); \
			ustride_host = vorder * vstride_host; \
			break; \
		case GL_DOUBLE: \
			vstride_host = size * sizeof(GLdouble); \
			ustride_host = vorder * vstride_host; \
			break; \
		default: glSetError(GL_INVALID_ENUM); return; \
	} \
	if (index > 15) { glSetError(GL_INVALID_VALUE); return; } \
	if (ustride < 0 || vstride < 0) { glSetError(GL_INVALID_VALUE); return; } \
	tmp = (char *)malloc(count * ustride_host); \
	if (tmp == NULL) glSetError(GL_OUT_OF_MEMORY); return; \
	start = (nfcmemptr) points; \
	for (i = 0; i < uorder; i++) { \
		for (j = 0; j < vorder; j++) { \
			if (!packed) { \
				src = start + ustride * i + vstride * j; \
			} else if (target == GL_EVAL_2D_NV) { \
				src = start + ustride * (j * uorder + i); \
			} else { \
				src = start + ustride * (j * uorder + i - j * (j - 1) / 2); \
			} \
			dst = tmp + ustride_host * i + vstride_host * j; \
			if (type == GL_FLOAT) \
				Atari2HostFloatArray(size, AtariAddr(src, const GLfloat *), (GLfloat *)dst); \
			else \
				Atari2HostDoubleArray(size, AtariAddr(src, const GLdouble *), (GLdouble *)dst); \
		} \
	} \
	fn.glMapControlPointsNV(target, index, type, ustride_host, vstride_host, uorder, vorder, GL_TRUE, tmp); \
	free(tmp)
	
#define FN_GLGETMAPCONTROLPOINTSNV(target, index, type, ustride, vstride, packed, points) \
	GLsizei size = 4; \
	GLsizei count; \
	GLint uorder = 0, vorder = 0; \
	GLsizei i, j; \
	GLsizei ustride_host, vstride_host; \
	const char *src; \
	char *tmp; \
	nfmemptr dst; \
	nfmemptr start; \
	fn.glGetMapAttribParameterivNV(target, index, GL_MAP_ATTRIB_U_ORDER_NV, &uorder); \
	fn.glGetMapAttribParameterivNV(target, index, GL_MAP_ATTRIB_V_ORDER_NV, &vorder); \
	if (uorder <= 0 || vorder <= 0) return; \
	switch (target) { \
		case GL_EVAL_2D_NV: \
			count = uorder * vorder; \
			break; \
		case GL_EVAL_TRIANGULAR_2D_NV: \
			if (uorder != vorder) { glSetError(GL_INVALID_OPERATION); return; } \
			count = uorder * (uorder + 1) / 2; \
			break; \
		default: glSetError(GL_INVALID_ENUM); return; \
	} \
	switch (type) { \
		case GL_FLOAT: \
			vstride_host = size * sizeof(GLfloat); \
			ustride_host = vorder * vstride_host; \
			break; \
		case GL_DOUBLE: \
			vstride_host = size * sizeof(GLdouble); \
			ustride_host = vorder * vstride_host; \
			break; \
		default: glSetError(GL_INVALID_ENUM); return; \
	} \
	if (index > 15) { glSetError(GL_INVALID_VALUE); return; } \
	if (ustride < 0 || vstride < 0) { glSetError(GL_INVALID_VALUE); return; } \
	tmp = (char *)malloc(count * ustride_host); \
	if (tmp == NULL) glSetError(GL_OUT_OF_MEMORY); return; \
	fn.glGetMapControlPointsNV(target, index, type, ustride_host, vstride_host, GL_TRUE, tmp); \
	start = (nfmemptr)points; \
	for (i = 0; i < uorder; i++) { \
		for (j = 0; j < vorder; j++) { \
			if (!packed) { \
				dst = start + ustride * i + vstride * j; \
			} else if (target == GL_EVAL_2D_NV) { \
				dst = start + ustride * (j * uorder + i); \
			} else { \
				dst = start + ustride * (j * uorder + i - j * (j - 1) / 2); \
			} \
			src = tmp + ustride_host * i + vstride_host * j; \
			if (type == GL_FLOAT) \
				Host2AtariFloatArray(size, (const GLfloat *)src, AtariAddr(dst, GLfloat *)); \
			else \
				Host2AtariDoubleArray(size, (const GLdouble *)src, AtariAddr(dst, GLdouble *)); \
		} \
	} \
	free(tmp)

#else
#define FN_GLMAPCONTROLPOINTSNV(target, index, type, ustride, vstride, uorder, vorder, packed, points) \
	fn.glMapControlPointsNV(target, index, type, ustride, vstride, uorder, vorder, packed, HostAddr(points, const void *))
#define FN_GLGETMAPCONTROLPOINTSNV(target, index, type, ustride, vstride, packed, points) \
	fn.glGetMapControlPointsNV(target, index, type, ustride, vstride, packed, HostAddr(points, void *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLMAPPARAMETERFVNV(target, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLfloat tmp[MAX(size, 16)], *ptmp; \
	ptmp = Atari2HostFloatArray(size, params, tmp); \
	fn.glMapParameterfvNV(target, pname, ptmp)
#define FN_GLGETMAPPARAMETERFVNV(target, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLfloat tmp[MAX(size, 16)]; \
	fn.glGetMapParameterfvNV(target, pname, tmp); \
	Host2AtariFloatArray(size, tmp, params)
#define FN_GLMAPATTRIBPARAMETERFVNV(target, index, pname, params) \
	GLint const size = 1; \
	GLfloat tmp[MAX(size, 16)], *ptmp; \
	ptmp = Atari2HostFloatArray(size, params, tmp); \
	fn.glMapAttribParameterfvNV(target, index, pname, ptmp)
#define FN_GLGETMAPATTRIBPARAMETERFVNV(target, index, pname, params) \
	GLint const size = 1; \
	GLfloat tmp[MAX(size, 16)]; \
	fn.glGetMapAttribParameterfvNV(target, index, pname, tmp); \
	Host2AtariFloatArray(size, tmp, params)
#else
#define FN_GLMAPPARAMETERFVNV(target, pname, params) \
	fn.glMapParameterfvNV(target, pname, HostAddr(params, const GLfloat *))
#define FN_GLGETMAPPARAMETERFVNV(target, pname, params) \
	fn.glGetMapParameterfvNV(target, pname, HostAddr(params, GLfloat *))
#define FN_GLMAPATTRIBPARAMETERFVNV(target, index, pname, params) \
	fn.glMapAttribParameterfvNV(target, index, pname, HostAddr(params, const GLfloat *))
#define FN_GLGETMAPATTRIBPARAMETERFVNV(target, index, pname, params) \
	fn.glGetMapAttribParameterfvNV(target, index, pname, HostAddr(params, GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLMAPPARAMETERIVNV(target, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLint tmp[MAX(size, 16)], *ptmp; \
	ptmp = Atari2HostIntArray(size, params, tmp); \
	fn.glMapParameterivNV(target, pname, ptmp)
#define FN_GLGETMAPPARAMETERIVNV(target, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLint tmp[MAX(size, 16)]; \
	fn.glGetMapParameterivNV(target, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#define FN_GLMAPATTRIBPARAMETERIVNV(target, index, pname, params) \
	GLint const size = 1; \
	GLint tmp[MAX(size, 16)], *ptmp; \
	ptmp = Atari2HostIntArray(size, params, tmp); \
	fn.glMapAttribParameterivNV(target, index, pname, ptmp)
#define FN_GLGETMAPATTRIBPARAMETERIVNV(target, index, pname, params) \
	GLint const size = 1; \
	GLint tmp[MAX(size, 16)]; \
	fn.glGetMapAttribParameterivNV(target, index, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLMAPPARAMETERIVNV(target, pname, params) \
	fn.glMapParameterivNV(target, pname, HostAddr(params, const GLint *))
#define FN_GLGETMAPPARAMETERIVNV(target, pname, params) \
	fn.glGetMapParameterivNV(target, pname, HostAddr(params, GLint *))
#define FN_GLMAPATTRIBPARAMETERIVNV(target, index, pname, params) \
	fn.glMapAttribParameterivNV(target, index, pname, HostAddr(params, const GLint *))
#define FN_GLGETMAPATTRIBPARAMETERIVNV(target, index, pname, params) \
	fn.glGetMapAttribParameterivNV(target, index, pname, HostAddr(params, GLint *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_NV_explicit_multisample
 */
#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETMULTISAMPLEFVNV(pname, index, val) \
	GLint const size = 2; \
	GLfloat tmp[MAX(size, 16)]; \
	fn.glGetMultisamplefvNV(pname, index, tmp); \
	Host2AtariFloatArray(size, tmp, val)
#else
#define FN_GLGETMULTISAMPLEFVNV(pname, index, val) \
	fn.glGetMultisamplefvNV(pname, index, HostAddr(val, GLfloat *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_INTEL_performance_query
 */

#if NFOSMESA_NEED_INT_CONV
#define FN_GLCREATEPERFQUERYINTEL(queryId, queryHandle) \
	GLuint tmp[1]; \
	fn.glCreatePerfQueryINTEL(queryId, tmp); \
	Host2AtariIntArray(1, tmp, queryHandle)
#else
#define FN_GLCREATEPERFQUERYINTEL(queryId, queryHandle) \
	fn.glCreatePerfQueryINTEL(queryId, HostAddr(queryHandle, GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETFIRSTPERFQUERYIDINTEL(queryId) \
	int const size = 1; \
	GLuint tmp[size]; \
	fn.glGetFirstPerfQueryIdINTEL(tmp); \
	Host2AtariIntArray(size, tmp, queryId)
#else
#define FN_GLGETFIRSTPERFQUERYIDINTEL(queryId) \
	fn.glGetFirstPerfQueryIdINTEL(HostAddr(queryId, GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETNEXTPERFQUERYIDINTEL(queryId, nextQueryId) \
	GLuint tmp[1]; \
	fn.glGetNextPerfQueryIdINTEL(queryId, tmp); \
	Host2AtariIntArray(1, tmp, nextQueryId)
#else
#define FN_GLGETNEXTPERFQUERYIDINTEL(queryId, nextQueryId) \
	fn.glGetNextPerfQueryIdINTEL(queryId, HostAddr(nextQueryId, GLuint *))
#endif

#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_INT_CONV
#define FN_GLGETPERFCOUNTERINFOINTEL(queryId, counterId, counterNameLength, counterName, counterDescLength, counterDesc, counterOffset, counterDataSize, counterTypeEnum, counterDataTypeEnum, rawCounterMaxValue) \
	GLint const size = 1;\
	GLuint offset[size]; \
	GLuint datasize[size]; \
	GLuint countertype[size]; \
	GLuint datatype[size]; \
	GLuint64 countermax[size]; \
	GLchar namebuf[MAX(counterNameLength, 0)]; \
	GLchar descbuf[MAX(counterDescLength, 0)]; \
	fn.glGetPerfCounterInfoINTEL(queryId, counterId, counterNameLength, namebuf, counterDescLength, descbuf, offset, datasize, countertype, datatype, countermax); \
	Host2AtariByteArray(sizeof(namebuf), namebuf, counterName); \
	Host2AtariByteArray(sizeof(descbuf), descbuf, counterDesc); \
	Host2AtariIntArray(size, offset, counterOffset); \
	Host2AtariIntArray(size, datasize, counterDataSize); \
	Host2AtariIntArray(size, countertype, counterTypeEnum); \
	Host2AtariIntArray(size, datatype, counterDataTypeEnum); \
	Host2AtariInt64Array(size, countermax, rawCounterMaxValue)
#else
#define FN_GLGETPERFCOUNTERINFOINTEL(queryId, counterId, counterNameLength, counterName, counterDescLength, counterDesc, counterOffset, counterDataSize, counterTypeEnum, counterDataTypeEnum, rawCounterMaxValue) \
	fn.glGetPerfCounterInfoINTEL(queryId, counterId, counterNameLength, HostAddr(counterName, GLchar *), counterDescLength, HostAddr(counterDesc, GLchar *), HostAddr(counterOffset, GLuint *), HostAddr(counterDataSize, GLuint *), HostAddr(counterTypeEnum, GLuint *), HostAddr(counterDataTypeEnum, GLuint *), HostAddr(rawCounterMaxValue, GLuint64 *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETPERFQUERYDATAINTEL(queryHandle, flags, dataSize, data, bytesWritten) \
	GLuint bytes = 0;\
	GLuint tmp[MAX(dataSize / sizeof(Uint32), 0)]; \
	fn.glGetPerfQueryDataINTEL(queryHandle, flags, dataSize, tmp, &bytes); \
	Host2AtariIntArray(1, &bytes, bytesWritten); \
	Host2AtariIntArray(bytes / sizeof(GLuint), tmp, AtariAddr(data, Uint32 *))
#else
#define FN_GLGETPERFQUERYDATAINTEL(queryHandle, flags, dataSize, data, bytesWritten) \
	fn.glGetPerfQueryDataINTEL(queryHandle, flags, dataSize, HostAddr(data, void *), HostAddr(bytesWritten, GLuint *))
#endif

#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_INT_CONV
#define FN_GLGETPERFQUERYIDBYNAMEINTEL(queryName, queryId) \
	GLint const size = 1;\
	GLuint tmp[size]; \
	GLchar namebuf[safe_strlen(queryName) + 1], *pname; \
	pname = Atari2HostByteArray(sizeof(namebuf), queryName, namebuf); \
	fn.glGetPerfQueryIdByNameINTEL(pname, tmp); \
	Host2AtariIntArray(size, tmp, queryId)
#else
#define FN_GLGETPERFQUERYIDBYNAMEINTEL(queryName, queryId) \
	fn.glGetPerfQueryIdByNameINTEL(HostAddr(queryName, GLchar *), HostAddr(queryId, GLuint *))
#endif

#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_INT_CONV
#define FN_GLGETPERFQUERYINFOINTEL(queryId, queryNameLength, queryName, dataSize, noCounters, noInstances, capsMask) \
	GLint const size = 1;\
	GLuint datasize[size]; \
	GLuint counters[size]; \
	GLuint instances[size]; \
	GLuint mask[size]; \
	GLchar namebuf[MAX(queryNameLength, 0)]; \
	fn.glGetPerfQueryInfoINTEL(queryId, queryNameLength, namebuf, datasize, counters, instances, mask); \
	Host2AtariByteArray(sizeof(namebuf), namebuf, queryName); \
	Host2AtariIntArray(size, datasize, dataSize); \
	Host2AtariIntArray(size, counters, noCounters); \
	Host2AtariIntArray(size, instances, noInstances); \
	Host2AtariIntArray(size, mask, capsMask)
#else
#define FN_GLGETPERFQUERYINFOINTEL(queryId, queryNameLength, queryName, dataSize, noCounters, noInstances, capsMask) \
	fn.glGetPerfQueryInfoINTEL(queryId, queryNameLength, HostAddr(queryName, GLchar *), HostAddr(dataSize, GLuint *), HostAddr(noCounters, GLuint *), HostAddr(noInstances, GLuint *), HostAddr(capsMask, GLuint *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_EXT_direct_state_access
 */
#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETFLOATINDEXEDVEXT(pname, index, params) \
	int n = nfglGetNumParams(pname); \
	GLfloat tmp[MAX(n, 16)]; \
	fn.glGetFloatIndexedvEXT(pname, index, tmp); \
	Host2AtariFloatArray(n, tmp, params)
#else
#define FN_GLGETFLOATINDEXEDVEXT(pname, index, params) \
	fn.glGetFloatIndexedvEXT(pname, index, HostAddr(params, GLfloat *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLGETDOUBLEINDEXEDVEXT(pname, index, params) \
	int n; \
	n = nfglGetNumParams(pname); \
	if (n > 16) { \
		GLdouble *tmp; \
		tmp = (GLdouble *)malloc(n * sizeof(*tmp)); \
		fn.glGetDoubleIndexedvEXT(pname, index, tmp); \
		Host2AtariDoubleArray(n, tmp, params); \
		free(tmp); \
	} else { \
		GLdouble tmp[16]; \
		fn.glGetDoubleIndexedvEXT(pname, index, tmp); \
		Host2AtariDoubleArray(n, tmp, params); \
	}
#else
#define FN_GLGETDOUBLEINDEXEDVEXT(pname, index, params) \
	fn.glGetDoubleIndexedvEXT(pname, index, HostAddr(params, GLdouble *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLGETBOOLEANINDEXEDVEXT(target, index, data) \
	GLint const size = nfglGetNumParams(target); \
	GLboolean tmp[MAX(size, 16)]; \
	fn.glGetBooleanIndexedvEXT(target, index, tmp); \
	Host2AtariByteArray(size, tmp, data)
#else
#define FN_GLGETBOOLEANINDEXEDVEXT(target, index, data) \
	fn.glGetBooleanIndexedvEXT(target, index, HostAddr(data, GLboolean *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETINTEGERINDEXEDVEXT(target, index, data) \
	GLint const size = nfglGetNumParams(target); \
	GLint tmp[MAX(size, 16)]; \
	fn.glGetIntegerIndexedvEXT(target, index, tmp); \
	Host2AtariIntArray(size, tmp, data)
#else
#define FN_GLGETINTEGERINDEXEDVEXT(target, index, data) \
	fn.glGetIntegerIndexedvEXT(target, index, HostAddr(data, GLint *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLGETCOMPRESSEDMULTITEXIMAGEEXT(texunit, target, lod, img) \
	GLint bufSize = 0; \
	fn.glGetMultiTexLevelParameterivEXT(texunit, target, lod, GL_TEXTURE_COMPRESSED_IMAGE_SIZE, &bufSize); \
	GLubyte tmp[bufSize]; \
	fn.glGetCompressedMultiTexImageEXT(texunit, target, lod, tmp); \
	Host2AtariByteArray(bufSize, tmp, img)
#else
#define FN_GLGETCOMPRESSEDMULTITEXIMAGEEXT(texunit, target, lod, img) \
	fn.glGetCompressedMultiTexImageEXT(texunit, target, lod, HostAddr(img, void *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLGETCOMPRESSEDTEXTUREIMAGEEXT(texture, target, lod, img) \
	GLint bufSize = 0; \
	fn.glGetTextureLevelParameterivEXT(texture, target, lod, GL_TEXTURE_COMPRESSED_IMAGE_SIZE, &bufSize); \
	GLubyte tmp[bufSize]; \
	fn.glGetCompressedTextureImageEXT(texture, target, lod, tmp); \
	Host2AtariByteArray(bufSize, tmp, img)
#else
#define FN_GLGETCOMPRESSEDTEXTUREIMAGEEXT(texture, target, lod, img) \
	fn.glGetCompressedTextureImageEXT(texture, target, lod, HostAddr(img, void *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLCOMPRESSEDMULTITEXIMAGE1DEXT(texunit, target, level, internalformat, width, border, imageSize, bits) \
	GLubyte tmp[MAX(imageSize, 0)], *ptmp; \
	ptmp = Atari2HostByteArray(sizeof(tmp), bits, tmp); \
	fn.glCompressedMultiTexImage1DEXT(texunit, target, level, internalformat, width, border, imageSize, ptmp)
#else
#define FN_GLCOMPRESSEDMULTITEXIMAGE1DEXT(texunit, target, level, internalformat, width, border, imageSize, bits) \
	fn.glCompressedMultiTexImage1DEXT(texunit, target, level, internalformat, width, border, imageSize, HostAddr(bits, const void *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLCOMPRESSEDMULTITEXIMAGE2DEXT(texunit, target, level, internalformat, width, height, border, imageSize, bits) \
	GLubyte tmp[MAX(imageSize, 0)], *ptmp; \
	ptmp = Atari2HostByteArray(sizeof(tmp), bits, tmp); \
	fn.glCompressedMultiTexImage2DEXT(texunit, target, level, internalformat, width, height, border, imageSize, ptmp)
#else
#define FN_GLCOMPRESSEDMULTITEXIMAGE2DEXT(texunit, target, level, internalformat, width, height, border, imageSize, bits) \
	fn.glCompressedMultiTexImage2DEXT(texunit, target, level, internalformat, width, height, border, imageSize, HostAddr(bits, const void *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLCOMPRESSEDMULTITEXIMAGE3DEXT(texunit, target, level, internalformat, width, height, depth, border, imageSize, bits) \
	GLubyte tmp[MAX(imageSize, 0)], *ptmp; \
	ptmp = Atari2HostByteArray(sizeof(tmp), bits, tmp); \
	fn.glCompressedMultiTexImage3DEXT(texunit, target, level, internalformat, width, height, depth, border, imageSize, ptmp)
#else
#define FN_GLCOMPRESSEDMULTITEXIMAGE3DEXT(texunit, target, level, internalformat, width, height, depth, border, imageSize, bits) \
	fn.glCompressedMultiTexImage3DEXT(texunit, target, level, internalformat, width, height, depth, border, imageSize, HostAddr(bits, const void *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLCOMPRESSEDMULTITEXSUBIMAGE1DEXT(texunit, target, level, xoffset, width, format, imageSize, bits) \
	GLubyte tmp[MAX(imageSize, 0)], *ptmp; \
	ptmp = Atari2HostByteArray(sizeof(tmp), bits, tmp); \
	fn.glCompressedMultiTexSubImage1DEXT(texunit, target, level, xoffset, width, format, imageSize, ptmp)
#else
#define FN_GLCOMPRESSEDMULTITEXSUBIMAGE1DEXT(texunit, target, level, xoffset, width, format, imageSize, bits) \
	fn.glCompressedMultiTexSubImage1DEXT(texunit, target, level, xoffset, width, format, imageSize, HostAddr(bits, const void *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLCOMPRESSEDMULTITEXSUBIMAGE2DEXT(texunit, target, level, xoffset, yoffset, width, height, format, imageSize, bits) \
	GLubyte tmp[MAX(imageSize, 0)], *ptmp; \
	ptmp = Atari2HostByteArray(sizeof(tmp), bits, tmp); \
	fn.glCompressedMultiTexSubImage2DEXT(texunit, target, level, xoffset, yoffset, width, height, format, imageSize, ptmp)
#else
#define FN_GLCOMPRESSEDMULTITEXSUBIMAGE2DEXT(texunit, target, level, xoffset, yoffset, width, height, format, imageSize, bits) \
	fn.glCompressedMultiTexSubImage2DEXT(texunit, target, level, xoffset, yoffset, width, height, format, imageSize, HostAddr(bits, const void *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLCOMPRESSEDMULTITEXSUBIMAGE3DEXT(texunit, target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, bits) \
	GLubyte tmp[MAX(imageSize, 0)], *ptmp; \
	ptmp = Atari2HostByteArray(sizeof(tmp), bits, tmp); \
	fn.glCompressedMultiTexSubImage3DEXT(texunit, target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, ptmp)
#else
#define FN_GLCOMPRESSEDMULTITEXSUBIMAGE3DEXT(texunit, target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, bits) \
	fn.glCompressedMultiTexSubImage3DEXT(texunit, target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, HostAddr(bits, const void *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLCOMPRESSEDTEXTUREIMAGE1DEXT(texture, target, level, internalformat, width, border, imageSize, bits) \
	GLubyte tmp[MAX(imageSize, 0)], *ptmp; \
	ptmp = Atari2HostByteArray(sizeof(tmp), bits, tmp); \
	fn.glCompressedTextureImage1DEXT(texture, target, level, internalformat, width, border, imageSize, ptmp)
#else
#define FN_GLCOMPRESSEDTEXTUREIMAGE1DEXT(texture, target, level, internalformat, width, border, imageSize, bits) \
	fn.glCompressedTextureImage1DEXT(texture, target, level, internalformat, width, border, imageSize, HostAddr(bits, const void *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLCOMPRESSEDTEXTUREIMAGE2DEXT(texture, target, level, internalformat, width, height, border, imageSize, bits) \
	GLubyte tmp[MAX(imageSize, 0)], *ptmp; \
	ptmp = Atari2HostByteArray(sizeof(tmp), bits, tmp); \
	fn.glCompressedTextureImage2DEXT(texture, target, level, internalformat, width, height, border, imageSize, ptmp)
#else
#define FN_GLCOMPRESSEDTEXTUREIMAGE2DEXT(texture, target, level, internalformat, width, height, border, imageSize, bits) \
	fn.glCompressedTextureImage2DEXT(texture, target, level, internalformat, width, height, border, imageSize, HostAddr(bits, const void *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLCOMPRESSEDTEXTUREIMAGE3DEXT(texture, target, level, internalformat, width, height, depth, border, imageSize, bits) \
	GLubyte tmp[MAX(imageSize, 0)], *ptmp; \
	ptmp = Atari2HostByteArray(sizeof(tmp), bits, tmp); \
	fn.glCompressedTextureImage3DEXT(texture, target, level, internalformat, width, height, depth, border, imageSize, ptmp)
#else
#define FN_GLCOMPRESSEDTEXTUREIMAGE3DEXT(texture, target, level, internalformat, width, height, depth, border, imageSize, bits) \
	fn.glCompressedTextureImage3DEXT(texture, target, level, internalformat, width, height, depth, border, imageSize, HostAddr(bits, const void *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLCOMPRESSEDTEXTURESUBIMAGE1DEXT(texture, target, level, xoffset, width, format, imageSize, bits) \
	GLubyte tmp[MAX(imageSize, 0)], *ptmp; \
	ptmp = Atari2HostByteArray(sizeof(tmp), bits, tmp); \
	fn.glCompressedTextureSubImage1DEXT(texture, target, level, xoffset, width, format, imageSize, ptmp)
#else
#define FN_GLCOMPRESSEDTEXTURESUBIMAGE1DEXT(texture, target, level, xoffset, width, format, imageSize, bits) \
	fn.glCompressedTextureSubImage1DEXT(texture, target, level, xoffset, width, format, imageSize, HostAddr(bits, const void *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLCOMPRESSEDTEXTURESUBIMAGE2DEXT(texture, target, level, xoffset, yoffset, width, height, format, imageSize, bits) \
	GLubyte tmp[MAX(imageSize, 0)], *ptmp; \
	ptmp = Atari2HostByteArray(sizeof(tmp), bits, tmp); \
	fn.glCompressedTextureSubImage2DEXT(texture, target, level, xoffset, yoffset, width, height, format, imageSize, ptmp)
#else
#define FN_GLCOMPRESSEDTEXTURESUBIMAGE2DEXT(texture, target, level, xoffset, yoffset, width, height, format, imageSize, bits) \
	fn.glCompressedTextureSubImage2DEXT(texture, target, level, xoffset, yoffset, width, height, format, imageSize, HostAddr(bits, const void *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLCOMPRESSEDTEXTURESUBIMAGE3DEXT(texture, target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, bits) \
	GLubyte tmp[MAX(imageSize, 0)], *ptmp; \
	ptmp = Atari2HostByteArray(sizeof(tmp), bits, tmp); \
	fn.glCompressedTextureSubImage3DEXT(texture, target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, ptmp)
#else
#define FN_GLCOMPRESSEDTEXTURESUBIMAGE3DEXT(texture, target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, bits) \
	fn.glCompressedTextureSubImage3DEXT(texture, target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, HostAddr(bits, const void *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLNAMEDBUFFERDATAEXT(buffer, size, data, usage) \
	GLubyte tmp[MAX(size, 0)], *ptmp; \
	ptmp = Atari2HostByteArray(sizeof(tmp), data, tmp); \
	fn.glNamedBufferDataEXT(buffer, size, ptmp, usage)
#else
#define FN_GLNAMEDBUFFERDATAEXT(buffer, size, data, usage) \
	fn.glNamedBufferDataEXT(buffer, size, HostAddr(data, const void *), usage)
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLNAMEDBUFFERSUBDATAEXT(buffer, offset, size, data) \
	GLubyte tmp[MAX(size, 0)], *ptmp; \
	ptmp = Atari2HostByteArray(sizeof(tmp), data, tmp); \
	fn.glNamedBufferSubDataEXT(buffer, offset, size, ptmp)
#else
#define FN_GLNAMEDBUFFERSUBDATAEXT(buffer, offset, size, data) \
	fn.glNamedBufferSubDataEXT(buffer, offset, size, HostAddr(data, const void *))
#endif

#define FN_GLGETNAMEDBUFFERPOINTERVEXT(buffer, pname, params) \
	void *tmp = NULL; \
	fn.glGetNamedBufferPointervEXT(buffer, pname, &tmp); \
	/* TODO */ \
	memptr zero = 0; \
	Host2AtariIntArray(1, &zero, AtariAddr(params, memptr *))

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETNAMEDBUFFERPARAMETERIVEXT(buffer, pname, params) \
	GLint const size = 1; \
	GLint tmp[MAX(size, 16)]; \
	fn.glGetNamedBufferParameterivEXT(buffer, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETNAMEDBUFFERPARAMETERIVEXT(buffer, pname, params) \
	fn.glGetNamedBufferParameterivEXT(buffer, pname, HostAddr(params, GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETNAMEDBUFFERPARAMETERUI64VNV(buffer, pname, params) \
	GLint const size = 1; \
	GLuint64EXT tmp[MAX(size, 16)]; \
	fn.glGetNamedBufferParameterui64vNV(buffer, pname, tmp); \
	Host2AtariInt64Array(size, tmp, params)
#else
#define FN_GLGETNAMEDBUFFERPARAMETERUI64VNV(buffer, pname, params) \
	fn.glGetNamedBufferParameterui64vNV(buffer, pname, HostAddr(params, GLuint64EXT *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETNAMEDFRAMEBUFFERATTACHMENTPARAMETERIVEXT(framebuffer, attachment, pname, params) \
	GLint const size = 1; \
	GLint tmp[MAX(size, 16)]; \
	fn.glGetNamedFramebufferAttachmentParameterivEXT(framebuffer, attachment, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETNAMEDFRAMEBUFFERATTACHMENTPARAMETERIVEXT(framebuffer, attachment, pname, params) \
	fn.glGetNamedFramebufferAttachmentParameterivEXT(framebuffer, attachment, pname, HostAddr(params, GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETNAMEDFRAMEBUFFERPARAMETERIVEXT(framebuffer, pname, params) \
	GLint const size = 1; \
	GLint tmp[size]; \
	fn.glGetNamedFramebufferParameterivEXT(framebuffer, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETNAMEDFRAMEBUFFERPARAMETERIVEXT(framebuffer, pname, params) \
	fn.glGetNamedFramebufferParameterivEXT(framebuffer, pname, HostAddr(params, GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETNAMEDRENDERBUFFERPARAMETERIVEXT(renderbuffer, pname, params) \
	GLint const size = 1; \
	GLint tmp[size]; \
	fn.glGetNamedRenderbufferParameterivEXT(renderbuffer, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETNAMEDRENDERBUFFERPARAMETERIVEXT(renderbuffer, pname, params) \
	fn.glGetNamedRenderbufferParameterivEXT(renderbuffer, pname, HostAddr(params, GLint *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETFLOATI_VEXT(pname, index, params) \
	int n = nfglGetNumParams(pname); \
	GLfloat tmp[n]; \
	fn.glGetFloati_vEXT(pname, index, tmp); \
	Host2AtariFloatArray(n, tmp, params)
#else
#define FN_GLGETFLOATI_VEXT(pname, index, params) \
	fn.glGetFloati_vEXT(pname, index, HostAddr(params, GLfloat *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLGETDOUBLEI_VEXT(pname, index, params) \
	int n = nfglGetNumParams(pname); \
	GLdouble tmp[MAX(n, 16)]; \
	fn.glGetDoublei_vEXT(pname, index, tmp); \
	Host2AtariDoubleArray(n, tmp, params)
#else
#define FN_GLGETDOUBLEI_VEXT(pname, index, params) \
	fn.glGetDoublei_vEXT(pname, index, HostAddr(params, GLdouble *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETMULTITEXENVFVEXT(texunit, target, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLfloat tmp[MAX(size, 16)]; \
	fn.glGetMultiTexEnvfvEXT(texunit, target, pname, tmp); \
	Host2AtariFloatArray(size, tmp, params)
#else
#define FN_GLGETMULTITEXENVFVEXT(texunit, target, pname, params) \
	fn.glGetMultiTexEnvfvEXT(texunit, target, pname, HostAddr(params, GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETMULTITEXENVIVEXT(texunit, target, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLint tmp[MAX(size, 16)]; \
	fn.glGetMultiTexEnvivEXT(texunit, target, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETMULTITEXENVIVEXT(texunit, target, pname, params) \
	fn.glGetMultiTexEnvivEXT(texunit, target, pname, HostAddr(params, GLint *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLGETMULTITEXGENDVEXT(texunit, coord, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLdouble tmp[MAX(size, 16)]; \
	fn.glGetMultiTexGendvEXT(texunit, coord, pname, tmp); \
	Host2AtariDoubleArray(size, tmp, params)
#else
#define FN_GLGETMULTITEXGENDVEXT(texunit, coord, pname, params) \
	fn.glGetMultiTexGendvEXT(texunit, coord, pname, HostAddr(params, GLdouble *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETMULTITEXGENFVEXT(texunit, coord, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLfloat tmp[MAX(size, 16)]; \
	fn.glGetMultiTexGenfvEXT(texunit, coord, pname, tmp); \
	Host2AtariFloatArray(size, tmp, params)
#else
#define FN_GLGETMULTITEXGENFVEXT(texunit, coord, pname, params) \
	fn.glGetMultiTexGenfvEXT(texunit, coord, pname, HostAddr(params, GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETMULTITEXGENIVEXT(texunit, coord, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLint tmp[MAX(size, 16)]; \
	fn.glGetMultiTexGenivEXT(texunit, coord, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETMULTITEXGENIVEXT(texunit, coord, pname, params) \
	fn.glGetMultiTexGenivEXT(texunit, coord, pname, HostAddr(params, GLint *))
#endif

#define FN_GLGETMULTITEXIMAGEEXT(texunit, target, level, format, type, pixels) \
	GLint width = 0, height = 1, depth = 1; \
	char *src; \
	nfmemptr dst; \
	pixelBuffer buf(*this); \
	 \
	switch (target) { \
	case GL_TEXTURE_1D: \
	case GL_PROXY_TEXTURE_1D: \
		fn.glGetMultiTexLevelParameterivEXT(texunit, target, level, GL_TEXTURE_WIDTH, &width); \
		break; \
	case GL_TEXTURE_2D: \
	case GL_PROXY_TEXTURE_2D: \
	case GL_TEXTURE_CUBE_MAP_POSITIVE_X: \
	case GL_TEXTURE_CUBE_MAP_POSITIVE_Y: \
	case GL_TEXTURE_CUBE_MAP_POSITIVE_Z: \
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_X: \
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y: \
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z: \
	case GL_PROXY_TEXTURE_CUBE_MAP: \
		fn.glGetMultiTexLevelParameterivEXT(texunit, target, level, GL_TEXTURE_WIDTH, &width); \
		fn.glGetMultiTexLevelParameterivEXT(texunit, target, level, GL_TEXTURE_HEIGHT, &height); \
		break; \
	case GL_TEXTURE_3D: \
	case GL_PROXY_TEXTURE_3D: \
		fn.glGetMultiTexLevelParameterivEXT(texunit, target, level, GL_TEXTURE_WIDTH, &width); \
		fn.glGetMultiTexLevelParameterivEXT(texunit, target, level, GL_TEXTURE_HEIGHT, &height); \
		fn.glGetMultiTexLevelParameterivEXT(texunit, target, level, GL_TEXTURE_DEPTH, &depth); \
		break; \
	default: \
		glSetError(GL_INVALID_ENUM); \
		return; \
	} \
	if (contexts[cur_context].buffer_bindings.pixel_pack.id) { \
		void *offset = NFHost2AtariAddr(pixels); \
		fn.glGetMultiTexImageEXT(texunit, target, level, format, type, offset); \
		src = contexts[cur_context].buffer_bindings.pixel_pack.host_pointer + (uintptr_t)offset; \
		dst = contexts[cur_context].buffer_bindings.pixel_pack.atari_pointer + (uintptr_t)offset; \
		if (!buf.params(width, height, depth, format, type)) return; \
	} else { \
		src = buf.hostBuffer(width, height, depth, format, type, (nfmemptr)pixels); \
		if (src == NULL) return; \
		fn.glGetMultiTexImageEXT(texunit, target, level, format, type, src); \
		dst = (nfmemptr)pixels; \
	} \
	buf.convertToAtari(src, dst)

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETMULTITEXLEVELPARAMETERFVEXT(texunit, target, level, pname, params) \
	GLint const size = 1; \
	GLfloat tmp[MAX(size, 16)]; \
	fn.glGetMultiTexLevelParameterfvEXT(texunit, target, level, pname, tmp); \
	Host2AtariFloatArray(size, tmp, params)
#else
#define FN_GLGETMULTITEXLEVELPARAMETERFVEXT(texunit, target, level, pname, params) \
	fn.glGetMultiTexLevelParameterfvEXT(texunit, target, level, pname, HostAddr(params, GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETMULTITEXLEVELPARAMETERIVEXT(texunit, target, level, pname, params) \
	GLint const size = 1; \
	GLint tmp[MAX(size, 16)]; \
	fn.glGetMultiTexLevelParameterivEXT(texunit, target, level, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETMULTITEXLEVELPARAMETERIVEXT(texunit, target, level, pname, params) \
	fn.glGetMultiTexLevelParameterivEXT(texunit, target, level, pname, HostAddr(params, GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETMULTITEXPARAMETERIIVEXT(texunit, target, pname, params) \
	GLint const size = 1; \
	GLint tmp[MAX(size, 16)]; \
	fn.glGetMultiTexParameterIivEXT(texunit, target, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETMULTITEXPARAMETERIIVEXT(texunit, target, pname, params) \
	fn.glGetMultiTexParameterIivEXT(texunit, target, pname, HostAddr(params, GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETMULTITEXPARAMETERIUIVEXT(texunit, target, pname, params) \
	GLint const size = 1; \
	GLuint tmp[MAX(size, 16)]; \
	fn.glGetMultiTexParameterIuivEXT(texunit, target, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETMULTITEXPARAMETERIUIVEXT(texunit, target, pname, params) \
	fn.glGetMultiTexParameterIuivEXT(texunit, target, pname, HostAddr(params, GLuint *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETMULTITEXPARAMETERFVEXT(texunit, target, pname, params) \
	GLint const size = 1; \
	GLfloat tmp[MAX(size, 16)]; \
	fn.glGetMultiTexParameterfvEXT(texunit, target, pname, tmp); \
	Host2AtariFloatArray(size, tmp, params)
#else
#define FN_GLGETMULTITEXPARAMETERFVEXT(texunit, target, pname, params) \
	fn.glGetMultiTexParameterfvEXT(texunit, target, pname, HostAddr(params, GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETMULTITEXPARAMETERIVEXT(texunit, target, pname, params) \
	GLint const size = 1; \
	GLint tmp[MAX(size, 16)]; \
	fn.glGetMultiTexParameterivEXT(texunit, target, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETMULTITEXPARAMETERIVEXT(texunit, target, pname, params) \
	fn.glGetMultiTexParameterivEXT(texunit, target, pname, HostAddr(params, GLint *))
#endif

#define FN_GLMULTITEXCOORDPOINTEREXT(texunit, size, type, stride, pointer) \
	setupClientArray(texunit, contexts[cur_context].texcoord, size, type, stride, -1, 0, AtariAddr(pointer, nfmemptr)); \
	contexts[cur_context].texcoord.vendor = NFOSMESA_VENDOR_EXT

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLMULTITEXENVFVEXT(texunit, target, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLfloat tmp[MAX(size, 16)]; \
	Atari2HostFloatArray(size, params, tmp); \
	fn.glMultiTexEnvfvEXT(texunit, target, pname, tmp)
#else
#define FN_GLMULTITEXENVFVEXT(texunit, target, pname, params) \
	fn.glMultiTexEnvfvEXT(texunit, target, pname, HostAddr(params, const GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLMULTITEXENVIVEXT(texunit, target, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLint tmp[MAX(size, 16)]; \
	Atari2HostIntArray(size, params, tmp); \
	fn.glMultiTexEnvivEXT(texunit, target, pname, tmp)
#else
#define FN_GLMULTITEXENVIVEXT(texunit, target, pname, params) \
	fn.glMultiTexEnvivEXT(texunit, target, pname, HostAddr(params, const GLint *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLMULTITEXGENDVEXT(texunit, coord, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLdouble tmp[MAX(size, 16)]; \
	Atari2HostDoubleArray(size, params, tmp); \
	fn.glMultiTexGendvEXT(texunit, coord, pname, tmp)
#else
#define FN_GLMULTITEXGENDVEXT(texunit, coord, pname, params) \
	fn.glMultiTexGendvEXT(texunit, coord, pname, HostAddr(params, const GLdouble *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLMULTITEXGENFVEXT(texunit, coord, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLfloat tmp[MAX(size, 16)]; \
	Atari2HostFloatArray(size, params, tmp); \
	fn.glMultiTexGenfvEXT(texunit, coord, pname, tmp)
#else
#define FN_GLMULTITEXGENFVEXT(texunit, coord, pname, params) \
	fn.glMultiTexGenfvEXT(texunit, coord, pname, HostAddr(params, const GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLMULTITEXGENIVEXT(texunit, coord, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLint tmp[MAX(size, 16)]; \
	Atari2HostIntArray(size, params, tmp); \
	fn.glMultiTexGenivEXT(texunit, coord, pname, tmp)
#else
#define FN_GLMULTITEXGENIVEXT(texunit, coord, pname, params) \
	fn.glMultiTexGenivEXT(texunit, coord, pname, HostAddr(params, const GLint *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLMULTITEXPARAMETERFVEXT(texunit, target, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLfloat tmp[MAX(size, 16)]; \
	Atari2HostFloatArray(size, params, tmp); \
	fn.glMultiTexParameterfvEXT(texunit, target, pname, tmp)
#else
#define FN_GLMULTITEXPARAMETERFVEXT(texunit, target, pname, params) \
	fn.glMultiTexParameterfvEXT(texunit, target, pname, HostAddr(params, const GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLMULTITEXPARAMETERIVEXT(texunit, target, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLint tmp[MAX(size, 16)]; \
	Atari2HostIntArray(size, params, tmp); \
	fn.glMultiTexParameterivEXT(texunit, target, pname, tmp)
#else
#define FN_GLMULTITEXPARAMETERIVEXT(texunit, target, pname, params) \
	fn.glMultiTexParameterivEXT(texunit, target, pname, HostAddr(params, const GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLMULTITEXPARAMETERIIVEXT(texunit, target, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLint tmp[MAX(size, 16)]; \
	Atari2HostIntArray(size, params, tmp); \
	fn.glMultiTexParameterIivEXT(texunit, target, pname, tmp)
#else
#define FN_GLMULTITEXPARAMETERIIVEXT(texunit, target, pname, params) \
	fn.glMultiTexParameterIivEXT(texunit, target, pname, HostAddr(params, const GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLMULTITEXPARAMETERIUIVEXT(texunit, target, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLuint tmp[MAX(size, 16)]; \
	Atari2HostIntArray(size, params, tmp); \
	fn.glMultiTexParameterIuivEXT(texunit, target, pname, tmp)
#else
#define FN_GLMULTITEXPARAMETERIUIVEXT(texunit, target, pname, params) \
	fn.glMultiTexParameterIuivEXT(texunit, target, pname, HostAddr(params, const GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLTEXTUREPARAMETERIIVEXT(texunit, target, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLint tmp[MAX(size, 16)]; \
	Atari2HostIntArray(size, params, tmp); \
	fn.glTextureParameterIivEXT(texunit, target, pname, tmp)
#else
#define FN_GLTEXTUREPARAMETERIIVEXT(texunit, target, pname, params) \
	fn.glTextureParameterIivEXT(texunit, target, pname, HostAddr(params, const GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLTEXTUREPARAMETERIUIVEXT(texunit, target, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLuint tmp[MAX(size, 16)]; \
	Atari2HostIntArray(size, params, tmp); \
	fn.glTextureParameterIuivEXT(texunit, target, pname, tmp)
#else
#define FN_GLTEXTUREPARAMETERIUIVEXT(texunit, target, pname, params) \
	fn.glTextureParameterIuivEXT(texunit, target, pname, HostAddr(params, const GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETTEXTUREPARAMETERIIVEXT(texunit, target, pname, params) \
	GLint const size = 1; \
	GLint tmp[MAX(size, 16)]; \
	fn.glGetTextureParameterIivEXT(texunit, target, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETTEXTUREPARAMETERIIVEXT(texunit, target, pname, params) \
	fn.glGetTextureParameterIivEXT(texunit, target, pname, HostAddr(params, GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETTEXTUREPARAMETERIVEXT(texture, target, pname, params) \
	GLint const size = 1; \
	GLint tmp[MAX(size, 16)]; \
	fn.glGetTextureParameterivEXT(texture, target, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETTEXTUREPARAMETERIVEXT(texture, target, pname, params) \
	fn.glGetTextureParameterivEXT(texture, target, pname, HostAddr(params, GLint *))
#endif

#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLTEXTUREIMAGE1DEXT(texture, target, level, internalformat, width, border, format, type, pixels) \
	pixelBuffer buf(*this); \
	void *tmp = buf.convertPixels(width, 1, 1, format, type, (nfmemptr)pixels); \
	fn.glTextureImage1DEXT(texture, target, level, internalformat, width, border, format, type, tmp)
#else
#define FN_GLTEXTUREIMAGE1DEXT(texture, target, level, internalformat, width, border, format, type, pixels) \
	fn.glTextureImage1DEXT(texture, target, level, internalformat, width, border, format, type, HostAddr(pixels, const void *))
#endif

#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLTEXTUREIMAGE2DEXT(texture, target, level, internalformat, width, height, border, format, type, pixels) \
	pixelBuffer buf(*this); \
	void *tmp = buf.convertPixels(width, height, 1, format, type, (nfmemptr)pixels); \
	fn.glTextureImage2DEXT(texture, target, level, internalformat, width, height, border, format, type, tmp)
#else
#define FN_GLTEXTUREIMAGE2DEXT(texture, target, level, internalformat, width, height, border, format, type, pixels) \
	fn.glTextureImage2DEXT(texture, target, level, internalformat, width, height, border, format, type, HostAddr(pixels, const void *))
#endif

#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLTEXTUREIMAGE3DEXT(texture, target, level, internalformat, width, height, depth, border, format, type, pixels) \
	pixelBuffer buf(*this); \
	void *tmp = buf.convertPixels(width, height, depth, format, type, (nfmemptr)pixels); \
	fn.glTextureImage3DEXT(texture, target, level, internalformat, width, height, depth, border, format, type, tmp)
#else
#define FN_GLTEXTUREIMAGE3DEXT(texture, target, level, internalformat, width, height, depth, border, format, type, pixels) \
	fn.glTextureImage3DEXT(texture, target, level, internalformat, width, height, depth, border, format, type, HostAddr(pixels, const void *))
#endif

#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLTEXTURESUBIMAGE1DEXT(texture, target, level, xoffset, width, format, type, pixels) \
	pixelBuffer buf(*this); \
	void *tmp = buf.convertPixels(width, 1, 1, format, type, (nfmemptr)pixels); \
	fn.glTextureSubImage1DEXT(texture, target, level, xoffset, width, format, type, tmp)
#else
#define FN_GLTEXTURESUBIMAGE1DEXT(texture, target, level, xoffset, width, format, type, pixels) \
	fn.glTextureSubImage1DEXT(texture, target, level, xoffset, width, format, type, HostAddr(pixels, const void *))
#endif

#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLTEXTURESUBIMAGE2DEXT(texture, target, level, xoffset, yoffset, width, height, format, type, pixels) \
	pixelBuffer buf(*this); \
	void *tmp = buf.convertPixels(width, height, 1, format, type, (nfmemptr)pixels); \
	fn.glTextureSubImage2DEXT(texture, target, level, xoffset, yoffset, width, height, format, type, tmp)
#else
#define FN_GLTEXTURESUBIMAGE2DEXT(texture, target, level, xoffset, yoffset, width, height, format, type, pixels) \
	fn.glTextureSubImage2DEXT(texture, target, level, xoffset, yoffset, width, height, format, type, HostAddr(pixels, const void *))
#endif

#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLTEXTURESUBIMAGE3DEXT(texture, target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels) \
	pixelBuffer buf(*this); \
	void *tmp = buf.convertPixels(width, height, depth, format, type, (nfmemptr)pixels); \
	fn.glTextureSubImage3DEXT(texture, target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, tmp)
#else
#define FN_GLTEXTURESUBIMAGE3DEXT(texture, target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels) \
	fn.glTextureSubImage3DEXT(texture, target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, HostAddr(pixels, const void *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETTEXTUREPARAMETERFVEXT(texture, target, pname, params) \
	GLint const size = 1; \
	GLfloat tmp[MAX(size, 16)]; \
	fn.glGetTextureParameterfvEXT(texture, target, pname, tmp); \
	Host2AtariFloatArray(size, tmp, params)
#else
#define FN_GLGETTEXTUREPARAMETERFVEXT(texture, target, pname, params) \
	fn.glGetTextureParameterfvEXT(texture, target, pname, HostAddr(params, GLfloat *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLTEXTUREPARAMETERFVEXT(texture, target, pname, params) \
	GLint const size = 1; \
	GLfloat tmp[MAX(size, 16)], *ptmp; \
	ptmp = Atari2HostFloatArray(size, params, tmp); \
	fn.glTextureParameterfvEXT(texture, target, pname, ptmp)
#else
#define FN_GLTEXTUREPARAMETERFVEXT(texture, target, pname, params) \
	fn.glTextureParameterfvEXT(texture, target, pname, HostAddr(params, const GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETTEXTURELEVELPARAMETERIVEXT(texture, target, level, pname, params) \
	GLint const size = 1; \
	GLint tmp[MAX(size, 16)]; \
	fn.glGetTextureLevelParameterivEXT(texture, target, level, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETTEXTURELEVELPARAMETERIVEXT(texture, target, level, pname, params) \
	fn.glGetTextureLevelParameterivEXT(texture, target, level, pname, HostAddr(params, GLint *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETTEXTURELEVELPARAMETERFVEXT(texture, target, level, pname, params) \
	GLint const size = 1; \
	GLfloat tmp[MAX(size, 16)]; \
	fn.glGetTextureLevelParameterfvEXT(texture, target, level, pname, tmp); \
	Host2AtariFloatArray(size, tmp, params)
#else
#define FN_GLGETTEXTURELEVELPARAMETERFVEXT(texture, target, level, pname, params) \
	fn.glGetTextureLevelParameterfvEXT(texture, target, level, pname, HostAddr(params, GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETTEXTUREPARAMETERIUIVEXT(texunit, target, pname, params) \
	GLint const size = 1; \
	GLuint tmp[MAX(size, 16)]; \
	fn.glGetTextureParameterIuivEXT(texunit, target, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETTEXTUREPARAMETERIUIVEXT(texunit, target, pname, params) \
	fn.glGetTextureParameterIuivEXT(texunit, target, pname, HostAddr(params, GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLTEXTUREPARAMETERIVEXT(texture, target, pname, params) \
	GLint const size = 1; \
	GLint tmp[MAX(size, 16)], *ptmp; \
	ptmp = Atari2HostIntArray(size, params, tmp); \
	fn.glTextureParameterivEXT(texture, target, pname, ptmp)
#else
#define FN_GLTEXTUREPARAMETERIVEXT(texture, target, pname, params) \
	fn.glTextureParameterivEXT(texture, target, pname, HostAddr(params, const GLint *))
#endif

#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLMULTITEXIMAGE1DEXT(texunit, target, level, internalformat, width, border, format, type, pixels) \
	pixelBuffer buf(*this); \
	void *tmp = buf.convertPixels(width, 1, 1, format, type, (nfmemptr)pixels); \
	fn.glMultiTexImage1DEXT(texunit, target, level, internalformat, width, border, format, type, tmp)
#else
#define FN_GLMULTITEXIMAGE1DEXT(texunit, target, level, internalformat, width, border, format, type, pixels) \
	fn.glMultiTexImage1DEXT(texunit, target, level, internalformat, width, border, format, type, HostAddr(pixels, const void *))
#endif

#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLMULTITEXIMAGE2DEXT(texunit, target, level, internalformat, width, height, border, format, type, pixels) \
	pixelBuffer buf(*this); \
	void *tmp = buf.convertPixels(width, height, 1, format, type, (nfmemptr)pixels); \
	fn.glMultiTexImage2DEXT(texunit, target, level, internalformat, width, height, border, format, type, tmp)
#else
#define FN_GLMULTITEXIMAGE2DEXT(texunit, target, level, internalformat, width, height, border, format, type, pixels) \
	fn.glMultiTexImage2DEXT(texunit, target, level, internalformat, width, height, border, format, type, HostAddr(pixels, const void *))
#endif

#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLMULTITEXIMAGE3DEXT(texunit, target, level, internalformat, width, height, depth, border, format, type, pixels) \
	pixelBuffer buf(*this); \
	void *tmp = buf.convertPixels(width, height, 1, format, type, (nfmemptr)pixels); \
	fn.glMultiTexImage3DEXT(texunit, target, level, internalformat, width, height, depth, border, format, type, tmp)
#else
#define FN_GLMULTITEXIMAGE3DEXT(texunit, target, level, internalformat, width, height, depth, border, format, type, pixels) \
	fn.glMultiTexImage3DEXT(texunit, target, level, internalformat, width, height, depth, border, format, type, HostAddr(pixels, const void *))
#endif

#define FN_GLGETPOINTERINDEXEDVEXT(target, index, data) \
	gl_get_pointer(target, index, HostAddr(data, void **))

#define FN_GLGETPOINTERI_VEXT(target, index, data) \
	gl_get_pointer(target, index, HostAddr(data, void **))

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLMATRIXLOADTRANSPOSEDEXT(mode, m) \
	GLint const size = 16; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, m, tmp); \
	fn.glMatrixLoadTransposedEXT(mode, tmp)
#else
#define FN_GLMATRIXLOADTRANSPOSEDEXT(mode, m) \
	fn.glMatrixLoadTransposedEXT(mode, HostAddr(m, const GLdouble *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLMATRIXLOADTRANSPOSEFEXT(mode, m) \
	GLint const size = 16; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, m, tmp); \
	fn.glMatrixLoadTransposefEXT(mode, tmp)
#else
#define FN_GLMATRIXLOADTRANSPOSEFEXT(mode, m) \
	fn.glMatrixLoadTransposefEXT(mode, HostAddr(m, const GLfloat *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLMATRIXMULTTRANSPOSEDEXT(mode, m) \
	GLint const size = 16; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, m, tmp); \
	fn.glMatrixMultTransposedEXT(mode, tmp)
#else
#define FN_GLMATRIXMULTTRANSPOSEDEXT(mode, m) \
	fn.glMatrixMultTransposedEXT(mode, HostAddr(m, const GLdouble *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLMATRIXMULTTRANSPOSEFEXT(mode, m) \
	GLint const size = 16; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, m, tmp); \
	fn.glMatrixMultTransposefEXT(mode, tmp)
#else
#define FN_GLMATRIXMULTTRANSPOSEFEXT(mode, m) \
	fn.glMatrixMultTransposefEXT(mode, HostAddr(m, const GLfloat *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLMATRIXLOADDEXT(mode, m) \
	GLint const size = 16; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, m, tmp); \
	fn.glMatrixLoaddEXT(mode, tmp)
#else
#define FN_GLMATRIXLOADDEXT(mode, m) \
	fn.glMatrixLoaddEXT(mode, HostAddr(m, const GLdouble *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLMATRIXLOADFEXT(mode, m) \
	GLint const size = 16; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, m, tmp); \
	fn.glMatrixLoadfEXT(mode, tmp)
#else
#define FN_GLMATRIXLOADFEXT(mode, m) \
	fn.glMatrixLoadfEXT(mode, HostAddr(m, const GLfloat *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLMATRIXMULTDEXT(mode, m) \
	GLint const size = 16; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, m, tmp); \
	fn.glMatrixMultdEXT(mode, tmp)
#else
#define FN_GLMATRIXMULTDEXT(mode, m) \
	fn.glMatrixMultdEXT(mode, HostAddr(m, const GLdouble *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLMATRIXMULTFEXT(mode, m) \
	GLint const size = 16; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, m, tmp); \
	fn.glMatrixMultfEXT(mode, tmp)
#else
#define FN_GLMATRIXMULTFEXT(mode, m) \
	fn.glMatrixMultfEXT(mode, HostAddr(m, const GLfloat *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPROGRAMUNIFORM1FVEXT(program, location, count, value) \
	GLint const size = 1 * count; \
	if (size <= 0) return; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, value, tmp); \
	fn.glProgramUniform1fvEXT(program, location, count, tmp)
#else
#define FN_GLPROGRAMUNIFORM1FVEXT(program, location, count, value) \
	fn.glProgramUniform1fvEXT(program, location, count, HostAddr(value, const GLfloat *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPROGRAMUNIFORM2FVEXT(program, location, count, value) \
	GLint const size = 2 * count; \
	if (size <= 0) return; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, value, tmp); \
	fn.glProgramUniform2fvEXT(program, location, count, tmp)
#else
#define FN_GLPROGRAMUNIFORM2FVEXT(program, location, count, value) \
	fn.glProgramUniform2fvEXT(program, location, count, HostAddr(value, const GLfloat *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPROGRAMUNIFORM3FVEXT(program, location, count, value) \
	GLint const size = 3 * count; \
	if (size <= 0) return; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, value, tmp); \
	fn.glProgramUniform3fvEXT(program, location, count, tmp)
#else
#define FN_GLPROGRAMUNIFORM3FVEXT(program, location, count, value) \
	fn.glProgramUniform3fvEXT(program, location, count, HostAddr(value, const GLfloat *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPROGRAMUNIFORM4FVEXT(program, location, count, value) \
	GLint const size = 4 * count; \
	if (size <= 0) return; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, value, tmp); \
	fn.glProgramUniform4fvEXT(program, location, count, tmp)
#else
#define FN_GLPROGRAMUNIFORM4FVEXT(program, location, count, value) \
	fn.glProgramUniform4fvEXT(program, location, count, HostAddr(value, const GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLPROGRAMUNIFORM1IVEXT(program, location, count, value) \
	GLint const size = 1 * count; \
	if (size <= 0) return; \
	GLint tmp[size]; \
	Atari2HostIntArray(size, value, tmp); \
	fn.glProgramUniform1ivEXT(program, location, count, tmp)
#else
#define FN_GLPROGRAMUNIFORM1IVEXT(program, location, count, value) \
	fn.glProgramUniform1ivEXT(program, location, count, HostAddr(value, const GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLPROGRAMUNIFORM2IVEXT(program, location, count, value) \
	GLint const size = 2 * count; \
	if (size <= 0) return; \
	GLint tmp[size]; \
	Atari2HostIntArray(size, value, tmp); \
	fn.glProgramUniform2ivEXT(program, location, count, tmp)
#else
#define FN_GLPROGRAMUNIFORM2IVEXT(program, location, count, value) \
	fn.glProgramUniform2ivEXT(program, location, count, HostAddr(value, const GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLPROGRAMUNIFORM3IVEXT(program, location, count, value) \
	GLint const size = 3 * count; \
	if (size <= 0) return; \
	GLint tmp[size]; \
	Atari2HostIntArray(size, value, tmp); \
	fn.glProgramUniform3ivEXT(program, location, count, tmp)
#else
#define FN_GLPROGRAMUNIFORM3IVEXT(program, location, count, value) \
	fn.glProgramUniform3ivEXT(program, location, count, HostAddr(value, const GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLPROGRAMUNIFORM4IVEXT(program, location, count, value) \
	GLint const size = 4 * count; \
	if (size <= 0) return; \
	GLint tmp[size]; \
	Atari2HostIntArray(size, value, tmp); \
	fn.glProgramUniform4ivEXT(program, location, count, tmp)
#else
#define FN_GLPROGRAMUNIFORM4IVEXT(program, location, count, value) \
	fn.glProgramUniform4ivEXT(program, location, count, HostAddr(value, const GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLPROGRAMUNIFORM1UIVEXT(program, location, count, value) \
	GLint const size = 1 * count; \
	if (size <= 0) return; \
	GLuint tmp[size]; \
	Atari2HostIntArray(size, value, tmp); \
	fn.glProgramUniform1uivEXT(program, location, count, tmp)
#else
#define FN_GLPROGRAMUNIFORM1UIVEXT(program, location, count, value) \
	fn.glProgramUniform1uivEXT(program, location, count, HostAddr(value, const GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLPROGRAMUNIFORM2UIVEXT(program, location, count, value) \
	GLint const size = 2 * count; \
	if (size <= 0) return; \
	GLuint tmp[size]; \
	Atari2HostIntArray(size, value, tmp); \
	fn.glProgramUniform2uivEXT(program, location, count, tmp)
#else
#define FN_GLPROGRAMUNIFORM2UIVEXT(program, location, count, value) \
	fn.glProgramUniform2uivEXT(program, location, count, HostAddr(value, const GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLPROGRAMUNIFORM3UIVEXT(program, location, count, value) \
	GLint const size = 3 * count; \
	if (size <= 0) return; \
	GLuint tmp[size]; \
	Atari2HostIntArray(size, value, tmp); \
	fn.glProgramUniform3uivEXT(program, location, count, tmp)
#else
#define FN_GLPROGRAMUNIFORM3UIVEXT(program, location, count, value) \
	fn.glProgramUniform3uivEXT(program, location, count, HostAddr(value, const GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLPROGRAMUNIFORM4UIVEXT(program, location, count, value) \
	GLint const size = 4 * count; \
	if (size <= 0) return; \
	GLuint tmp[size]; \
	Atari2HostIntArray(size, value, tmp); \
	fn.glProgramUniform4uivEXT(program, location, count, tmp)
#else
#define FN_GLPROGRAMUNIFORM4UIVEXT(program, location, count, value) \
	fn.glProgramUniform4uivEXT(program, location, count, HostAddr(value, const GLuint *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLPROGRAMUNIFORM1DVEXT(program, location, count, value) \
	GLint const size = 1 * count; \
	if (size <= 0) return; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, value, tmp); \
	fn.glProgramUniform1dvEXT(program, location, count, tmp)
#else
#define FN_GLPROGRAMUNIFORM1DVEXT(program, location, count, value) \
	fn.glProgramUniform1dvEXT(program, location, count, HostAddr(value, const GLdouble *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLPROGRAMUNIFORM2DVEXT(program, location, count, value) \
	GLint const size = 2 * count; \
	if (size <= 0) return; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, value, tmp); \
	fn.glProgramUniform2dvEXT(program, location, count, tmp)
#else
#define FN_GLPROGRAMUNIFORM2DVEXT(program, location, count, value) \
	fn.glProgramUniform2dvEXT(program, location, count, HostAddr(value, const GLdouble *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLPROGRAMUNIFORM3DVEXT(program, location, count, value) \
	GLint const size = 3 * count; \
	if (size <= 0) return; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, value, tmp); \
	fn.glProgramUniform3dvEXT(program, location, count, tmp)
#else
#define FN_GLPROGRAMUNIFORM3DVEXT(program, location, count, value) \
	fn.glProgramUniform3dvEXT(program, location, count, HostAddr(value, const GLdouble *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLPROGRAMUNIFORM4DVEXT(program, location, count, value) \
	GLint const size = 4 * count; \
	if (size <= 0) return; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, value, tmp); \
	fn.glProgramUniform4dvEXT(program, location, count, tmp)
#else
#define FN_GLPROGRAMUNIFORM4DVEXT(program, location, count, value) \
	fn.glProgramUniform4dvEXT(program, location, count, HostAddr(value, const GLdouble *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPROGRAMUNIFORMMATRIX2FVEXT(program, location, count, transpose, value) \
	GLint const size = 4 * count; \
	if (size <= 0) return; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, value, tmp); \
	fn.glProgramUniformMatrix2fvEXT(program, location, count, transpose, tmp)
#else
#define FN_GLPROGRAMUNIFORMMATRIX2FVEXT(program, location, count, transpose, value) \
	fn.glProgramUniformMatrix2fvEXT(program, location, count, transpose, HostAddr(value, const GLfloat *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPROGRAMUNIFORMMATRIX3FVEXT(program, location, count, transpose, value) \
	GLint const size = 9 * count; \
	if (size <= 0) return; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, value, tmp); \
	fn.glProgramUniformMatrix3fvEXT(program, location, count, transpose, tmp)
#else
#define FN_GLPROGRAMUNIFORMMATRIX3FVEXT(program, location, count, transpose, value) \
	fn.glProgramUniformMatrix3fvEXT(program, location, count, transpose, HostAddr(value, const GLfloat *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPROGRAMUNIFORMMATRIX4FVEXT(program, location, count, transpose, value) \
	GLint const size = 16 * count; \
	if (size <= 0) return; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, value, tmp); \
	fn.glProgramUniformMatrix4fvEXT(program, location, count, transpose, tmp)
#else
#define FN_GLPROGRAMUNIFORMMATRIX4FVEXT(program, location, count, transpose, value) \
	fn.glProgramUniformMatrix4fvEXT(program, location, count, transpose, HostAddr(value, const GLfloat *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPROGRAMUNIFORMMATRIX2X3FVEXT(program, location, count, transpose, value) \
	GLint const size = 6 * count; \
	if (size <= 0) return; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, value, tmp); \
	fn.glProgramUniformMatrix2x3fvEXT(program, location, count, transpose, tmp)
#else
#define FN_GLPROGRAMUNIFORMMATRIX2X3FVEXT(program, location, count, transpose, value) \
	fn.glProgramUniformMatrix2x3fvEXT(program, location, count, transpose, HostAddr(value, const GLfloat *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPROGRAMUNIFORMMATRIX3X2FVEXT(program, location, count, transpose, value) \
	GLint const size = 6 * count; \
	if (size <= 0) return; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, value, tmp); \
	fn.glProgramUniformMatrix3x2fvEXT(program, location, count, transpose, tmp)
#else
#define FN_GLPROGRAMUNIFORMMATRIX3X2FVEXT(program, location, count, transpose, value) \
	fn.glProgramUniformMatrix3x2fvEXT(program, location, count, transpose, HostAddr(value, const GLfloat *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPROGRAMUNIFORMMATRIX2X4FVEXT(program, location, count, transpose, value) \
	GLint const size = 8 * count; \
	if (size <= 0) return; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, value, tmp); \
	fn.glProgramUniformMatrix2x4fvEXT(program, location, count, transpose, tmp)
#else
#define FN_GLPROGRAMUNIFORMMATRIX2X4FVEXT(program, location, count, transpose, value) \
	fn.glProgramUniformMatrix2x4fvEXT(program, location, count, transpose, HostAddr(value, const GLfloat *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPROGRAMUNIFORMMATRIX4X2FVEXT(program, location, count, transpose, value) \
	GLint const size = 8 * count; \
	if (size <= 0) return; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, value, tmp); \
	fn.glProgramUniformMatrix4x2fvEXT(program, location, count, transpose, tmp)
#else
#define FN_GLPROGRAMUNIFORMMATRIX4X2FVEXT(program, location, count, transpose, value) \
	fn.glProgramUniformMatrix4x2fvEXT(program, location, count, transpose, HostAddr(value, const GLfloat *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPROGRAMUNIFORMMATRIX3X4FVEXT(program, location, count, transpose, value) \
	GLint const size = 12 * count; \
	if (size <= 0) return; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, value, tmp); \
	fn.glProgramUniformMatrix3x4fvEXT(program, location, count, transpose, tmp)
#else
#define FN_GLPROGRAMUNIFORMMATRIX3X4FVEXT(program, location, count, transpose, value) \
	fn.glProgramUniformMatrix3x4fvEXT(program, location, count, transpose, HostAddr(value, const GLfloat *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPROGRAMUNIFORMMATRIX4X3FVEXT(program, location, count, transpose, value) \
	GLint const size = 12 * count; \
	if (size <= 0) return; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, value, tmp); \
	fn.glProgramUniformMatrix4x3fvEXT(program, location, count, transpose, tmp)
#else
#define FN_GLPROGRAMUNIFORMMATRIX4X3FVEXT(program, location, count, transpose, value) \
	fn.glProgramUniformMatrix4x3fvEXT(program, location, count, transpose, HostAddr(value, const GLfloat *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLPROGRAMUNIFORMMATRIX2DVEXT(program, location, count, transpose, value) \
	GLint const size = 4 * count; \
	if (size <= 0) return; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, value, tmp); \
	fn.glProgramUniformMatrix2dvEXT(program, location, count, transpose, tmp)
#else
#define FN_GLPROGRAMUNIFORMMATRIX2DVEXT(program, location, count, transpose, value) \
	fn.glProgramUniformMatrix2dvEXT(program, location, count, transpose, HostAddr(value, const GLdouble *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLPROGRAMUNIFORMMATRIX3DVEXT(program, location, count, transpose, value) \
	GLint const size = 9 * count; \
	if (size <= 0) return; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, value, tmp); \
	fn.glProgramUniformMatrix3dvEXT(program, location, count, transpose, tmp)
#else
#define FN_GLPROGRAMUNIFORMMATRIX3DVEXT(program, location, count, transpose, value) \
	fn.glProgramUniformMatrix3dvEXT(program, location, count, transpose, HostAddr(value, const GLdouble *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLPROGRAMUNIFORMMATRIX4DVEXT(program, location, count, transpose, value) \
	GLint const size = 16 * count; \
	if (size <= 0) return; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, value, tmp); \
	fn.glProgramUniformMatrix4dvEXT(program, location, count, transpose, tmp)
#else
#define FN_GLPROGRAMUNIFORMMATRIX4DVEXT(program, location, count, transpose, value) \
	fn.glProgramUniformMatrix4dvEXT(program, location, count, transpose, HostAddr(value, const GLdouble *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLPROGRAMUNIFORMMATRIX2X3DVEXT(program, location, count, transpose, value) \
	GLint const size = 6 * count; \
	if (size <= 0) return; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, value, tmp); \
	fn.glProgramUniformMatrix2x3dvEXT(program, location, count, transpose, tmp)
#else
#define FN_GLPROGRAMUNIFORMMATRIX2X3DVEXT(program, location, count, transpose, value) \
	fn.glProgramUniformMatrix2x3dvEXT(program, location, count, transpose, HostAddr(value, const GLdouble *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLPROGRAMUNIFORMMATRIX3X2DVEXT(program, location, count, transpose, value) \
	GLint const size = 6 * count; \
	if (size <= 0) return; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, value, tmp); \
	fn.glProgramUniformMatrix3x2dvEXT(program, location, count, transpose, tmp)
#else
#define FN_GLPROGRAMUNIFORMMATRIX3X2DVEXT(program, location, count, transpose, value) \
	fn.glProgramUniformMatrix3x2dvEXT(program, location, count, transpose, HostAddr(value, const GLdouble *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLPROGRAMUNIFORMMATRIX2X4DVEXT(program, location, count, transpose, value) \
	GLint const size = 8 * count; \
	if (size <= 0) return; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, value, tmp); \
	fn.glProgramUniformMatrix2x4dvEXT(program, location, count, transpose, tmp)
#else
#define FN_GLPROGRAMUNIFORMMATRIX2X4DVEXT(program, location, count, transpose, value) \
	fn.glProgramUniformMatrix2x4dvEXT(program, location, count, transpose, HostAddr(value, const GLdouble *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLPROGRAMUNIFORMMATRIX4X2DVEXT(program, location, count, transpose, value) \
	GLint const size = 8 * count; \
	if (size <= 0) return; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, value, tmp); \
	fn.glProgramUniformMatrix4x2dvEXT(program, location, count, transpose, tmp)
#else
#define FN_GLPROGRAMUNIFORMMATRIX4X2DVEXT(program, location, count, transpose, value) \
	fn.glProgramUniformMatrix4x2dvEXT(program, location, count, transpose, HostAddr(value, const GLdouble *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLPROGRAMUNIFORMMATRIX3X4DVEXT(program, location, count, transpose, value) \
	GLint const size = 12 * count; \
	if (size <= 0) return; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, value, tmp); \
	fn.glProgramUniformMatrix3x4dvEXT(program, location, count, transpose, tmp)
#else
#define FN_GLPROGRAMUNIFORMMATRIX3X4DVEXT(program, location, count, transpose, value) \
	fn.glProgramUniformMatrix3x4dvEXT(program, location, count, transpose, HostAddr(value, const GLdouble *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLPROGRAMUNIFORMMATRIX4X3DVEXT(program, location, count, transpose, value) \
	GLint const size = 12 * count; \
	if (size <= 0) return; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, value, tmp); \
	fn.glProgramUniformMatrix4x3dvEXT(program, location, count, transpose, tmp)
#else
#define FN_GLPROGRAMUNIFORMMATRIX4X3DVEXT(program, location, count, transpose, value) \
	fn.glProgramUniformMatrix4x3dvEXT(program, location, count, transpose, HostAddr(value, const GLdouble *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLNAMEDPROGRAMLOCALPARAMETERS4FVEXT(program, target, index, count, params) \
	GLint const size = 4 * count; \
	if (size <= 0) return; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, params, tmp); \
	fn.glNamedProgramLocalParameters4fvEXT(program, target, index, count, tmp)
#else
#define FN_GLNAMEDPROGRAMLOCALPARAMETERS4FVEXT(program, target, index, count, params) \
	fn.glNamedProgramLocalParameters4fvEXT(program, target, index, count, HostAddr(params, const GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLNAMEDPROGRAMLOCALPARAMETERSI4IVEXT(program, target, index, count, params) \
	GLint const size = 4 * count; \
	if (size <= 0) return; \
	GLint tmp[size]; \
	Atari2HostIntArray(size, params, tmp); \
	fn.glNamedProgramLocalParametersI4ivEXT(program, target, index, count, tmp)
#else
#define FN_GLNAMEDPROGRAMLOCALPARAMETERSI4IVEXT(program, target, index, count, params) \
	fn.glNamedProgramLocalParametersI4ivEXT(program, target, index, count, HostAddr(params, const GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLNAMEDPROGRAMLOCALPARAMETERSI4UIVEXT(program, target, index, count, params) \
	GLint const size = 4 * count; \
	if (size <= 0) return; \
	GLuint tmp[size]; \
	Atari2HostIntArray(size, params, tmp); \
	fn.glNamedProgramLocalParametersI4uivEXT(program, target, index, count, tmp)
#else
#define FN_GLNAMEDPROGRAMLOCALPARAMETERSI4UIVEXT(program, target, index, count, params) \
	fn.glNamedProgramLocalParametersI4uivEXT(program, target, index, count, HostAddr(params, const GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLNAMEDPROGRAMLOCALPARAMETERI4IVEXT(program, target, index, params) \
	GLint const size = 4; \
	GLint tmp[size]; \
	Atari2HostIntArray(size, params, tmp); \
	fn.glNamedProgramLocalParameterI4ivEXT(program, target, index, tmp)
#else
#define FN_GLNAMEDPROGRAMLOCALPARAMETERI4IVEXT(program, target, index, params) \
	fn.glNamedProgramLocalParameterI4ivEXT(program, target, index, HostAddr(params, const GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLNAMEDPROGRAMLOCALPARAMETERI4UIVEXT(program, target, index, params) \
	GLint const size = 4; \
	GLuint tmp[size]; \
	Atari2HostIntArray(size, params, tmp); \
	fn.glNamedProgramLocalParameterI4uivEXT(program, target, index, tmp)
#else
#define FN_GLNAMEDPROGRAMLOCALPARAMETERI4UIVEXT(program, target, index, params) \
	fn.glNamedProgramLocalParameterI4uivEXT(program, target, index, HostAddr(params, const GLuint *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLNAMEDPROGRAMLOCALPARAMETER4DVEXT(program, target, index, params) \
	GLint const size = 4; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, params, tmp); \
	fn.glNamedProgramLocalParameter4dvEXT(program, target, index, tmp)
#else
#define FN_GLNAMEDPROGRAMLOCALPARAMETER4DVEXT(program, target, index, params) \
	fn.glNamedProgramLocalParameter4dvEXT(program, target, index, HostAddr(params, const GLdouble *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLNAMEDPROGRAMLOCALPARAMETER4FVEXT(program, target, index, params) \
	GLint const size = 4; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, params, tmp); \
	fn.glNamedProgramLocalParameter4fvEXT(program, target, index, tmp)
#else
#define FN_GLNAMEDPROGRAMLOCALPARAMETER4FVEXT(program, target, index, params) \
	fn.glNamedProgramLocalParameter4fvEXT(program, target, index, HostAddr(params, const GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETNAMEDPROGRAMLOCALPARAMETERIIVEXT(program, target, index, params) \
	GLint const size = 4; \
	GLint tmp[size]; \
	fn.glGetNamedProgramLocalParameterIivEXT(program, target, index, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETNAMEDPROGRAMLOCALPARAMETERIIVEXT(program, target, index, params) \
	fn.glGetNamedProgramLocalParameterIivEXT(program, target, index, HostAddr(params, GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETNAMEDPROGRAMLOCALPARAMETERIUIVEXT(program, target, index, params) \
	GLint const size = 4; \
	GLuint tmp[size]; \
	fn.glGetNamedProgramLocalParameterIuivEXT(program, target, index, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETNAMEDPROGRAMLOCALPARAMETERIUIVEXT(program, target, index, params) \
	fn.glGetNamedProgramLocalParameterIuivEXT(program, target, index, HostAddr(params, GLuint *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLGETNAMEDPROGRAMLOCALPARAMETERDVEXT(program, target, index, params) \
	GLint const size = 4; \
	GLdouble tmp[size]; \
	fn.glGetNamedProgramLocalParameterdvEXT(program, target, index, tmp); \
	Host2AtariDoubleArray(size, tmp, params)
#else
#define FN_GLGETNAMEDPROGRAMLOCALPARAMETERDVEXT(program, target, index, params) \
	fn.glGetNamedProgramLocalParameterdvEXT(program, target, index, HostAddr(params, GLdouble *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETNAMEDPROGRAMLOCALPARAMETERFVEXT(program, target, index, params) \
	GLint const size = 4; \
	GLfloat tmp[size]; \
	fn.glGetNamedProgramLocalParameterfvEXT(program, target, index, tmp); \
	Host2AtariFloatArray(size, tmp, params)
#else
#define FN_GLGETNAMEDPROGRAMLOCALPARAMETERFVEXT(program, target, index, params) \
	fn.glGetNamedProgramLocalParameterfvEXT(program, target, index, HostAddr(params, GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETNAMEDPROGRAMIVEXT(program, target, pname, params) \
	GLint const size = 1; \
	GLint tmp[size]; \
	fn.glGetNamedProgramivEXT(program, target, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETNAMEDPROGRAMIVEXT(program, target, pname, params) \
	fn.glGetNamedProgramivEXT(program, target, pname, HostAddr(params, GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETVERTEXARRAYINTEGERVEXT(vaobj, pname, params) \
	GLint const size = 1; \
	GLint tmp[size]; \
	fn.glGetVertexArrayIntegervEXT(vaobj, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETVERTEXARRAYINTEGERVEXT(vaobj, pname, params) \
	fn.glGetVertexArrayIntegervEXT(vaobj, pname, HostAddr(params, GLint *))
#endif

// FIXME: need to track vertex arrays
#define FN_GLGETVERTEXARRAYPOINTERVEXT(vaobj, pname, params) \
	UNUSED(vaobj); \
	UNUSED(pname); \
	UNUSED(params); \
	glSetError(GL_INVALID_OPERATION)

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETVERTEXARRAYINTEGERI_VEXT(vaobj, index, pname, params) \
	GLint const size = 1; \
	GLint tmp[size]; \
	fn.glGetVertexArrayIntegeri_vEXT(vaobj, index, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETVERTEXARRAYINTEGERI_VEXT(vaobj, index, pname, params) \
	fn.glGetVertexArrayIntegeri_vEXT(vaobj, index, pname, HostAddr(params, GLint *))
#endif

// FIXME: need to track vertex arrays
#define FN_GLGETVERTEXARRAYPOINTERI_VEXT(vaobj, index, pname, params) \
	UNUSED(vaobj); \
	UNUSED(index); \
	UNUSED(pname); \
	UNUSED(params); \
	glSetError(GL_INVALID_OPERATION)

#if NFOSMESA_NEED_BYTE_CONV
/* pname can only be PROGRAM_NAME_ASCII_ARB */
#define FN_GLGETNAMEDPROGRAMSTRINGEXT(program, target, pname, string) \
	GLint len = 0; \
	fn.glGetNamedProgramivEXT(program, target, GL_PROGRAM_LENGTH_ARB, &len); \
	if (len <= 0) return; \
	GLubyte tmp[len]; \
	fn.glGetNamedProgramStringEXT(program, target, pname, tmp); \
	Host2AtariByteArray(len, tmp, string)
#else
#define FN_GLGETNAMEDPROGRAMSTRINGEXT(program, target, pname, string) \
	fn.glGetNamedProgramStringEXT(program, target, pname, HostAddr(string, void *))
#endif

/* format can only be PROGRAM_STRING_ARB */
#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLNAMEDPROGRAMSTRINGEXT(program, target, format, len, string) \
	GLubyte tmp[MAX(len, 0)], *ptmp; \
	ptmp = Atari2HostByteArray(sizeof(tmp), string, tmp); \
	fn.glNamedProgramStringEXT(program, target, format, len, ptmp)
#else
#define FN_GLNAMEDPROGRAMSTRINGEXT(program, target, format, len, string) \
	fn.glNamedProgramStringEXT(program, target, format, len, HostAddr(string, const void *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLFRAMEBUFFERDRAWBUFFERSEXT(framebuffer, n, bufs) \
	GLenum tmp[MAX(n, 0)], *ptmp; \
	ptmp = Atari2HostIntArray(n, bufs, tmp); \
	fn.glFramebufferDrawBuffersEXT(framebuffer, n, ptmp)
#else
#define FN_GLFRAMEBUFFERDRAWBUFFERSEXT(framebuffer, n, bufs) \
	fn.glFramebufferDrawBuffersEXT(framebuffer, n, HostAddr(bufs, const GLenum *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETFRAMEBUFFERPARAMETERIVEXT(target, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLint tmp[MAX(size, 16)]; \
	fn.glGetFramebufferParameterivEXT(target, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETFRAMEBUFFERPARAMETERIVEXT(target, pname, params) \
	fn.glGetFramebufferParameterivEXT(target, pname, HostAddr(params, GLint *))
#endif

#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLMULTITEXSUBIMAGE1DEXT(texunit, target, level, xoffset, width, format, type, pixels) \
	pixelBuffer buf(*this); \
	void *tmp = buf.convertPixels(width, 1, 1, format, type, (nfmemptr)pixels); \
	fn.glMultiTexSubImage1DEXT(texunit, target, level, xoffset, width, format, type, tmp)
#else
#define FN_GLMULTITEXSUBIMAGE1DEXT(texunit, target, level, xoffset, width, format, type, pixels) \
	fn.glMultiTexSubImage1DEXT(texunit, target, level, xoffset, width, format, type, HostAddr(pixels, const void *))
#endif

#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLMULTITEXSUBIMAGE2DEXT(texunit, target, level, xoffset, yoffset, width, height, format, type, pixels) \
	pixelBuffer buf(*this); \
	void *tmp = buf.convertPixels(width, height, 1, format, type, (nfmemptr)pixels); \
	fn.glMultiTexSubImage2DEXT(texunit, target, level, xoffset, yoffset, width, height, format, type, tmp)
#else
#define FN_GLMULTITEXSUBIMAGE2DEXT(texunit, target, level, xoffset, yoffset, width, height, format, type, pixels) \
	fn.glMultiTexSubImage2DEXT(texunit, target, level, xoffset, yoffset, width, height, format, type, HostAddr(pixels, const void *))
#endif

#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLMULTITEXSUBIMAGE3DEXT(texunit, target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels) \
	pixelBuffer buf(*this); \
	void *tmp = buf.convertPixels(width, height, depth, format, type, (nfmemptr)pixels); \
	fn.glMultiTexSubImage3DEXT(texunit, target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, tmp)
#else
#define FN_GLMULTITEXSUBIMAGE3DEXT(texunit, target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels) \
	fn.glMultiTexSubImage3DEXT(texunit, target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, HostAddr(pixels, const void *))
#endif

#define FN_GLGETTEXTUREIMAGEEXT(texture, target, level, format, type, img) \
	GLint width = 0, height = 1, depth = 1; \
	char *src; \
	nfmemptr dst; \
	pixelBuffer buf(*this); \
	 \
	switch (target) { \
	case GL_TEXTURE_1D: \
	case GL_PROXY_TEXTURE_1D: \
		fn.glGetTextureLevelParameterivEXT(texture, target, level, GL_TEXTURE_WIDTH, &width); \
		break; \
	case GL_TEXTURE_2D: \
	case GL_PROXY_TEXTURE_2D: \
	case GL_TEXTURE_CUBE_MAP_POSITIVE_X: \
	case GL_TEXTURE_CUBE_MAP_POSITIVE_Y: \
	case GL_TEXTURE_CUBE_MAP_POSITIVE_Z: \
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_X: \
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y: \
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z: \
	case GL_PROXY_TEXTURE_CUBE_MAP: \
		fn.glGetTextureLevelParameterivEXT(texture, target, level, GL_TEXTURE_WIDTH, &width); \
		fn.glGetTextureLevelParameterivEXT(texture, target, level, GL_TEXTURE_HEIGHT, &height); \
		break; \
	case GL_TEXTURE_3D: \
	case GL_PROXY_TEXTURE_3D: \
		fn.glGetTextureLevelParameterivEXT(texture, target, level, GL_TEXTURE_WIDTH, &width); \
		fn.glGetTextureLevelParameterivEXT(texture, target, level, GL_TEXTURE_HEIGHT, &height); \
		fn.glGetTextureLevelParameterivEXT(texture, target, level, GL_TEXTURE_DEPTH, &depth); \
		break; \
	default: \
		glSetError(GL_INVALID_ENUM); \
		return; \
	} \
	if (contexts[cur_context].buffer_bindings.pixel_pack.id) { \
		void *offset = NFHost2AtariAddr(img); \
		fn.glGetTextureImageEXT(texture, target, level, format, type, offset); \
		src = contexts[cur_context].buffer_bindings.pixel_pack.host_pointer + (uintptr_t)offset; \
		dst = contexts[cur_context].buffer_bindings.pixel_pack.atari_pointer + (uintptr_t)offset; \
		if (!buf.params(width, height, depth, format, type)) return; \
	} else { \
		src = buf.hostBuffer(width, height, depth, format, type, (nfmemptr)img); \
		if (src == NULL) return; \
		fn.glGetTextureImageEXT(texture, target, level, format, type, src); \
		dst = (nfmemptr)img; \
	} \
	buf.convertToAtari(src, dst)

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLNAMEDBUFFERSTORAGEEXT(buffer, size, data, flags) \
	GLubyte tmp[MAX(size, 0)], *ptmp; \
	ptmp = Atari2HostByteArray(sizeof(tmp), data, tmp); \
	fn.glNamedBufferStorageEXT(buffer, size, ptmp, flags)
#else
#define FN_GLNAMEDBUFFERSTORAGEEXT(buffer, size, data, flags) \
	fn.glNamedBufferStorageEXT(buffer, size, HostAddr(data, const void *), flags)
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_ARB_clear_buffer_object
 */

#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV || NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLCLEARBUFFERDATA(target, internalformat, format, type, data) \
	pixelBuffer buf(*this); \
	void *tmp = buf.convertPixels(1, 1, 1, format, type, (nfmemptr)data); \
	fn.glClearBufferData(target, internalformat, format, type, tmp)
#else
#define FN_GLCLEARBUFFERDATA(target, internalformat, format, type, data) \
	fn.glClearBufferData(target, internalformat, format, type, HostAddr(data, const void *))
#endif

#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV || NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLCLEARNAMEDBUFFERDATAEXT(buffer, internalformat, format, type, data) \
	pixelBuffer buf(*this); \
	void *tmp = buf.convertPixels(1, 1, 1, format, type, (nfmemptr)data); \
	fn.glClearNamedBufferDataEXT(buffer, internalformat, format, type, tmp)
#else
#define FN_GLCLEARNAMEDBUFFERDATAEXT(buffer, internalformat, format, type, data) \
	fn.glClearNamedBufferDataEXT(buffer, internalformat, format, type, HostAddr(data, const void *))
#endif

#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV || NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLCLEARBUFFERSUBDATA(target, internalformat, offset, size, format, type, data) \
	pixelBuffer buf(*this); \
	void *tmp = buf.convertPixels(1, 1, 1, format, type, (nfmemptr)data); \
	fn.glClearBufferSubData(target, internalformat, offset, size, format, type, tmp)
#else
#define FN_GLCLEARBUFFERSUBDATA(target, internalformat, offset, size, format, type, data) \
	fn.glClearBufferSubData(target, internalformat, offset, size, format, type, HostAddr(data, const void *))
#endif

#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV || NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLCLEARNAMEDBUFFERSUBDATAEXT(buffer, internalformat, offset, size, format, type, data) \
	pixelBuffer buf(*this); \
	void *tmp = buf.convertPixels(1, 1, 1, format, type, (nfmemptr)data); \
	fn.glClearNamedBufferSubDataEXT(buffer, internalformat, offset, size, format, type, tmp)
#else
#define FN_GLCLEARNAMEDBUFFERSUBDATAEXT(buffer, internalformat, offset, size, format, type, data) \
	fn.glClearNamedBufferSubDataEXT(buffer, internalformat, offset, size, format, type, HostAddr(data, const void *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_EXT_gpu_shader4
 */

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLBINDFRAGDATALOCATIONEXT(program, color, name) \
	GLchar tmp[safe_strlen(name) + 1], *ptmp; \
	ptmp = Atari2HostByteArray(sizeof(tmp), name, tmp); \
	fn.glBindFragDataLocationEXT(program, color, ptmp)
#else
#define FN_GLBINDFRAGDATALOCATIONEXT(program, color, name) \
	fn.glBindFragDataLocationEXT(program, color, HostAddr(name, const GLchar *))
#endif
	
#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLGETFRAGDATALOCATIONEXT(program, name) \
	GLchar tmp[safe_strlen(name) + 1], *ptmp; \
	ptmp = Atari2HostByteArray(sizeof(tmp), name, tmp); \
	return fn.glGetFragDataLocationEXT(program, ptmp)
#else
#define FN_GLGETFRAGDATALOCATIONEXT(program, name) \
	return fn.glGetFragDataLocationEXT(program, HostAddr(name, const GLchar *))
#endif
	
#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETUNIFORMUIVEXT(program, location, params) \
	GLint const size = 1; \
	GLuint tmp[size]; \
	fn.glGetUniformuivEXT(program, location, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETUNIFORMUIVEXT(program, location, params) \
	fn.glGetUniformuivEXT(program, location, HostAddr(params, GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLUNIFORM1UIVEXT(location, count, value) \
	GLint const size = 1 * count; \
	if (size <= 0) return; \
	GLuint tmp[size]; \
	Atari2HostIntArray(size, value, tmp); \
	fn.glUniform1uivEXT(location, count, tmp)
#else
#define FN_GLUNIFORM1UIVEXT(location, count, value) \
	fn.glUniform1uivEXT(location, count, HostAddr(value, const GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLUNIFORM2UIVEXT(location, count, value) \
	GLint const size = 2 * count; \
	if (size <= 0) return; \
	GLuint tmp[size]; \
	Atari2HostIntArray(size, value, tmp); \
	fn.glUniform2uivEXT(location, count, tmp)
#else
#define FN_GLUNIFORM2UIVEXT(location, count, value) \
	fn.glUniform2uivEXT(location, count, HostAddr(value, const GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLUNIFORM3UIVEXT(location, count, value) \
	GLint const size = 3 * count; \
	if (size <= 0) return; \
	GLuint tmp[size]; \
	Atari2HostIntArray(size, value, tmp); \
	fn.glUniform3uivEXT(location, count, tmp)
#else
#define FN_GLUNIFORM3UIVEXT(location, count, value) \
	fn.glUniform3uivEXT(location, count, HostAddr(value, const GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLUNIFORM4UIVEXT(location, count, value) \
	GLint const size = 4 * count; \
	if (size <= 0) return; \
	GLuint tmp[size]; \
	Atari2HostIntArray(size, value, tmp); \
	fn.glUniform4uivEXT(location, count, tmp)
#else
#define FN_GLUNIFORM4UIVEXT(location, count, value) \
	fn.glUniform4uivEXT(location, count, HostAddr(value, const GLuint *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_AMD_gpu_shader_int64
 */

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETUNIFORMI64VNV(program, location, params) \
	GLint const size = 1; \
	GLint64 tmp[size]; \
	fn.glGetUniformi64vNV(program, location, tmp); \
	Host2AtariInt64Array(size, tmp, params)
#else
#define FN_GLGETUNIFORMI64VNV(program, location, params) \
	fn.glGetUniformi64vNV(program, location, HostAddr(params, GLint64 *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETUNIFORMUI64VNV(program, location, params) \
	GLint const size = 1; \
	GLuint64 tmp[size]; \
	fn.glGetUniformui64vNV(program, location, tmp); \
	Host2AtariInt64Array(size, tmp, params)
#else
#define FN_GLGETUNIFORMUI64VNV(program, location, params) \
	fn.glGetUniformui64vNV(program, location, HostAddr(params, GLuint64 *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_ARB_gpu_shader_int64
 */

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETUNIFORMI64VARB(program, location, params) \
	GLint const size = 1; \
	GLint64 tmp[size]; \
	fn.glGetUniformi64vARB(program, location, tmp); \
	Host2AtariInt64Array(size, tmp, params)
#else
#define FN_GLGETUNIFORMI64VARB(program, location, params) \
	fn.glGetUniformi64vARB(program, location, HostAddr(params, GLint64 *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETUNIFORMUI64VARB(program, location, params) \
	GLint const size = 1; \
	GLuint64 tmp[size]; \
	fn.glGetUniformui64vARB(program, location, tmp); \
	Host2AtariInt64Array(size, tmp, params)
#else
#define FN_GLGETUNIFORMUI64VARB(program, location, params) \
	fn.glGetUniformui64vARB(program, location, HostAddr(params, GLuint64 *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETNUNIFORMI64VARB(program, location, bufsize, params) \
	GLint64 tmp[bufsize]; \
	fn.glGetUniformi64vARB(program, location, tmp); \
	Host2AtariInt64Array(bufsize, tmp, params)
#else
#define FN_GLGETNUNIFORMI64VARB(program, location, bufsize, params) \
	fn.glGetnUniformi64vARB(program, location, bufsize, HostAddr(params, GLint64 *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETNUNIFORMUI64VARB(program, location, bufsize, params) \
	GLuint64 tmp[bufsize]; \
	fn.glGetUniformui64vARB(program, location, tmp); \
	Host2AtariInt64Array(bufsize, tmp, params)
#else
#define FN_GLGETNUNIFORMUI64VARB(program, location, bufsize, params) \
	fn.glGetnUniformui64vARB(program, location, bufsize, HostAddr(params, GLuint64 *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_EXT_vertex_shader
 */

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETINVARIANTINTEGERVEXT(id, value, data) \
	GLint const size = 1; \
	GLint tmp[size]; \
	fn.glGetInvariantIntegervEXT(id, value, tmp); \
	Host2AtariIntArray(size, tmp, data)
#else
#define FN_GLGETINVARIANTINTEGERVEXT(id, value, data) \
	fn.glGetInvariantIntegervEXT(id, value, HostAddr(data, GLint *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLGETINVARIANTBOOLEANVEXT(id, value, data) \
	GLint const size = 1; \
	GLboolean tmp[size]; \
	fn.glGetInvariantBooleanvEXT(id, value, tmp); \
	Host2AtariByteArray(size, tmp, data)
#else
#define FN_GLGETINVARIANTBOOLEANVEXT(id, value, data) \
	fn.glGetInvariantBooleanvEXT(id, value, HostAddr(data, GLboolean *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETINVARIANTFLOATVEXT(id, value, data) \
	GLint const size = 1; \
	GLfloat tmp[size]; \
	fn.glGetInvariantFloatvEXT(id, value, tmp); \
	Host2AtariFloatArray(size, tmp, data)
#else
#define FN_GLGETINVARIANTFLOATVEXT(id, value, data) \
	fn.glGetInvariantFloatvEXT(id, value, HostAddr(data, GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETLOCALCONSTANTBOOLEANVEXT(id, value, data) \
	GLint const size = 1; \
	GLboolean tmp[size]; \
	fn.glGetLocalConstantBooleanvEXT(id, value, tmp); \
	Host2AtariByteArray(size, tmp, data)
#else
#define FN_GLGETLOCALCONSTANTBOOLEANVEXT(id, value, data) \
	fn.glGetLocalConstantBooleanvEXT(id, value, HostAddr(data, GLboolean *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETLOCALCONSTANTINTEGERVEXT(id, value, data) \
	GLint const size = 1; \
	GLint tmp[size]; \
	fn.glGetLocalConstantIntegervEXT(id, value, tmp); \
	Host2AtariIntArray(size, tmp, data)
#else
#define FN_GLGETLOCALCONSTANTINTEGERVEXT(id, value, data) \
	fn.glGetLocalConstantIntegervEXT(id, value, HostAddr(data, GLint *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETLOCALCONSTANTFLOATVEXT(id, value, data) \
	GLint const size = 1; \
	GLfloat tmp[size]; \
	fn.glGetLocalConstantFloatvEXT(id, value, tmp); \
	Host2AtariFloatArray(size, tmp, data)
#else
#define FN_GLGETLOCALCONSTANTFLOATVEXT(id, value, data) \
	fn.glGetLocalConstantFloatvEXT(id, value, HostAddr(data, GLfloat *))
#endif

#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV || NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLSETINVARIANTEXT(id, type, data) \
	GLint size = 0; \
	GLint datatype = 0; \
	fn.glGetVariantIntegervEXT(id, GL_VARIANT_DATATYPE_EXT, &datatype); \
	switch (datatype) { \
		case GL_SCALAR_EXT: size = 1; break; \
		case GL_VECTOR_EXT: size = 4; break; \
		case GL_MATRIX_EXT: size = 16; break; \
	} \
	switch (type) { \
	case GL_BYTE: \
	case GL_UNSIGNED_BYTE: { \
			GLubyte tmp[size], *ptmp; \
			ptmp = Atari2HostByteArray(size, data, tmp); \
			fn.glSetInvariantEXT(id, type, ptmp); \
		} \
		break; \
	case GL_SHORT: \
	case GL_UNSIGNED_SHORT: { \
			GLushort tmp[size], *ptmp; \
			ptmp = Atari2HostShortArray(size, data, tmp); \
			fn.glSetInvariantEXT(id, type, ptmp); \
		} \
		break; \
	case GL_INT: \
	case GL_UNSIGNED_INT: { \
			GLuint tmp[size], *ptmp; \
			ptmp = Atari2HostIntArray(size, data, tmp); \
			fn.glSetInvariantEXT(id, type, ptmp); \
		} \
		break; \
	case GL_FLOAT: { \
			GLfloat tmp[size], *ptmp; \
			ptmp = Atari2HostFloatArray(size, data, tmp); \
			fn.glSetInvariantEXT(id, type, ptmp); \
		} \
		break; \
	case GL_DOUBLE: { \
			GLdouble tmp[size], *ptmp; \
			ptmp = Atari2HostDoubleArray(size, data, tmp); \
			fn.glSetInvariantEXT(id, type, ptmp); \
		} \
		break; \
	default: \
		glSetError(GL_INVALID_ENUM); \
		return; \
	}
#else
#define FN_GLSETINVARIANTEXT(id, type, data) \
	fn.glSetInvariantEXT(id, type, HostAddr(data, const void *))
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV || NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLSETLOCALCONSTANTEXT(id, type, data) \
	GLint size = 0; \
	GLint datatype = 0; \
	fn.glGetLocalConstantIntegervEXT(id, GL_LOCAL_CONSTANT_DATATYPE_EXT, &datatype); \
	switch (datatype) { \
		case GL_SCALAR_EXT: size = 1; break; \
		case GL_VECTOR_EXT: size = 4; break; \
		case GL_MATRIX_EXT: size = 16; break; \
	} \
	switch (type) { \
	case GL_BYTE: \
	case GL_UNSIGNED_BYTE: { \
			GLubyte tmp[size], *ptmp; \
			ptmp = Atari2HostByteArray(size, data, tmp); \
			fn.glSetLocalConstantEXT(id, type, ptmp); \
		} \
		break; \
	case GL_SHORT: \
	case GL_UNSIGNED_SHORT: { \
			GLushort tmp[size], *ptmp; \
			ptmp = Atari2HostShortArray(size, data, tmp); \
			fn.glSetLocalConstantEXT(id, type, ptmp); \
		} \
		break; \
	case GL_INT: \
	case GL_UNSIGNED_INT: { \
			GLuint tmp[size], *ptmp; \
			ptmp = Atari2HostIntArray(size, data, tmp); \
			fn.glSetLocalConstantEXT(id, type, ptmp); \
		} \
		break; \
	case GL_FLOAT: { \
			GLfloat tmp[size], *ptmp; \
			ptmp = Atari2HostFloatArray(size, data, tmp); \
			fn.glSetLocalConstantEXT(id, type, ptmp); \
		} \
		break; \
	case GL_DOUBLE: { \
			GLdouble tmp[size], *ptmp; \
			ptmp = Atari2HostDoubleArray(size, data, tmp); \
			fn.glSetLocalConstantEXT(id, type, ptmp); \
		} \
		break; \
	default: \
		glSetError(GL_INVALID_ENUM); \
		return; \
	}
#else
#define FN_GLSETLOCALCONSTANTEXT(id, type, data) \
	fn.glSetLocalConstantEXT(id, type, HostAddr(data, const void *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVARIANTSVEXT(id, data) \
	GLint size = 0; \
	GLint datatype = 0; \
	fn.glGetVariantIntegervEXT(id, GL_VARIANT_DATATYPE_EXT, &datatype); \
	switch (datatype) { \
		case GL_SCALAR_EXT: size = 1; break; \
		case GL_VECTOR_EXT: size = 4; break; \
		case GL_MATRIX_EXT: size = 16; break; \
	} \
	GLshort tmp[size], *ptmp; \
	ptmp = Atari2HostShortArray(size, data, tmp); \
	fn.glVariantsvEXT(id, ptmp)
#else
#define FN_GLVARIANTSVEXT(id, data) \
	fn.glVariantsvEXT(id, HostAddr(data, const GLshort *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVARIANTIVEXT(id, data) \
	GLint size = 0; \
	GLint datatype = 0; \
	fn.glGetVariantIntegervEXT(id, GL_VARIANT_DATATYPE_EXT, &datatype); \
	switch (datatype) { \
		case GL_SCALAR_EXT: size = 1; break; \
		case GL_VECTOR_EXT: size = 4; break; \
		case GL_MATRIX_EXT: size = 16; break; \
	} \
	GLint tmp[size], *ptmp; \
	ptmp = Atari2HostIntArray(size, data, tmp); \
	fn.glVariantivEXT(id, ptmp)
#else
#define FN_GLVARIANTIVEXT(id, data) \
	fn.glVariantivEXT(id, HostAddr(data, const GLint *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLVARIANTFVEXT(id, data) \
	GLint size = 0; \
	GLint datatype = 0; \
	fn.glGetVariantIntegervEXT(id, GL_VARIANT_DATATYPE_EXT, &datatype); \
	switch (datatype) { \
		case GL_SCALAR_EXT: size = 1; break; \
		case GL_VECTOR_EXT: size = 4; break; \
		case GL_MATRIX_EXT: size = 16; break; \
	} \
	GLfloat tmp[size], *ptmp; \
	ptmp = Atari2HostFloatArray(size, data, tmp); \
	fn.glVariantfvEXT(id, ptmp)
#else
#define FN_GLVARIANTFVEXT(id, data) \
	fn.glVariantfvEXT(id, HostAddr(data, const GLfloat *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLVARIANTBVEXT(id, data) \
	GLint size = 0; \
	GLint datatype = 0; \
	fn.glGetVariantIntegervEXT(id, GL_VARIANT_DATATYPE_EXT, &datatype); \
	switch (datatype) { \
		case GL_SCALAR_EXT: size = 1; break; \
		case GL_VECTOR_EXT: size = 4; break; \
		case GL_MATRIX_EXT: size = 16; break; \
	} \
	GLbyte tmp[size], *ptmp; \
	ptmp = Atari2HostByteArray(size, data, tmp); \
	fn.glVariantbvEXT(id, ptmp)
#else
#define FN_GLVARIANTBVEXT(id, data) \
	fn.glVariantbvEXT(id, HostAddr(data, const GLbyte *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLVARIANTDVEXT(id, data) \
	GLint size = 0; \
	GLint datatype = 0; \
	fn.glGetVariantIntegervEXT(id, GL_VARIANT_DATATYPE_EXT, &datatype); \
	switch (datatype) { \
		case GL_SCALAR_EXT: size = 1; break; \
		case GL_VECTOR_EXT: size = 4; break; \
		case GL_MATRIX_EXT: size = 16; break; \
	} \
	GLdouble tmp[size], *ptmp; \
	ptmp = Atari2HostDoubleArray(size, data, tmp); \
	fn.glVariantdvEXT(id, ptmp)
#else
#define FN_GLVARIANTDVEXT(id, data) \
	fn.glVariantdvEXT(id, HostAddr(data, const GLdouble *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLVARIANTUBVEXT(id, data) \
	GLint size = 0; \
	GLint datatype = 0; \
	fn.glGetVariantIntegervEXT(id, GL_VARIANT_DATATYPE_EXT, &datatype); \
	switch (datatype) { \
		case GL_SCALAR_EXT: size = 1; break; \
		case GL_VECTOR_EXT: size = 4; break; \
		case GL_MATRIX_EXT: size = 16; break; \
	} \
	GLubyte tmp[size], *ptmp; \
	ptmp = Atari2HostByteArray(size, data, tmp); \
	fn.glVariantubvEXT(id, ptmp)
#else
#define FN_GLVARIANTUBVEXT(id, data) \
	fn.glVariantubvEXT(id, HostAddr(data, const GLubyte *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVARIANTUSVEXT(id, data) \
	GLint size = 0; \
	GLint datatype = 0; \
	fn.glGetVariantIntegervEXT(id, GL_VARIANT_DATATYPE_EXT, &datatype); \
	switch (datatype) { \
		case GL_SCALAR_EXT: size = 1; break; \
		case GL_VECTOR_EXT: size = 4; break; \
		case GL_MATRIX_EXT: size = 16; break; \
	} \
	GLushort tmp[size], *ptmp; \
	ptmp = Atari2HostShortArray(size, data, tmp); \
	fn.glVariantusvEXT(id, ptmp)
#else
#define FN_GLVARIANTUSVEXT(id, data) \
	fn.glVariantusvEXT(id, HostAddr(data, const GLushort *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVARIANTUIVEXT(id, data) \
	GLint size = 0; \
	GLint datatype = 0; \
	fn.glGetVariantIntegervEXT(id, GL_VARIANT_DATATYPE_EXT, &datatype); \
	switch (datatype) { \
		case GL_SCALAR_EXT: size = 1; break; \
		case GL_VECTOR_EXT: size = 4; break; \
		case GL_MATRIX_EXT: size = 16; break; \
	} \
	GLuint tmp[size], *ptmp; \
	ptmp = Atari2HostIntArray(size, data, tmp); \
	fn.glVariantuivEXT(id, ptmp)
#else
#define FN_GLVARIANTUIVEXT(id, data) \
	fn.glVariantuivEXT(id, HostAddr(data, const GLuint *))
#endif

#define FN_GLVARIANTPOINTEREXT(id, type, stride, pointer) \
	GLint texunit = 0; \
	fn.glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE, &texunit); \
	setupClientArray(texunit, contexts[cur_context].variant, 4, type, stride, -1, 0, AtariAddr(pointer, nfmemptr)); \
	contexts[cur_context].variant.vendor = NFOSMESA_VENDOR_EXT; \
	contexts[cur_context].variant.id = id

#define FN_GLENABLEVARIANTCLIENTSTATEEXT(array) \
	switch(array) { \
		case GL_VARIANT_ARRAY_EXT: \
			contexts[cur_context].enabled_arrays |= NFOSMESA_VARIANT_ARRAY; \
			break; \
	} \
	fn.glEnableVariantClientStateEXT(array)

#define FN_GLDISABLEVARIANTCLIENTSTATEEXT(array) \
	switch(array) { \
		case GL_VARIANT_ARRAY_EXT: \
			contexts[cur_context].enabled_arrays &= ~NFOSMESA_VARIANT_ARRAY; \
			break; \
	} \
	fn.glDisableVariantClientStateEXT(array)

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLGETVARIANTBOOLEANVEXT(id, value, data) \
	GLint size = 0; \
	GLint datatype = 0; \
	fn.glGetVariantIntegervEXT(id, GL_VARIANT_DATATYPE_EXT, &datatype); \
	switch (datatype) { \
		case GL_SCALAR_EXT: size = 1; break; \
		case GL_VECTOR_EXT: size = 4; break; \
		case GL_MATRIX_EXT: size = 16; break; \
	} \
	GLboolean tmp[size]; \
	fn.glGetVariantBooleanvEXT(id, value, tmp); \
	Host2AtariByteArray(size, tmp, data)
#else
#define FN_GLGETVARIANTBOOLEANVEXT(id, value, data) \
	fn.glGetVariantBooleanvEXT(id, value, HostAddr(data, GLboolean *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETVARIANTINTEGERVEXT(id, value, data) \
	GLint size = 0; \
	GLint datatype = 0; \
	fn.glGetVariantIntegervEXT(id, GL_VARIANT_DATATYPE_EXT, &datatype); \
	switch (datatype) { \
		case GL_SCALAR_EXT: size = 1; break; \
		case GL_VECTOR_EXT: size = 4; break; \
		case GL_MATRIX_EXT: size = 16; break; \
	} \
	GLint tmp[size]; \
	fn.glGetVariantIntegervEXT(id, value, tmp); \
	Host2AtariIntArray(size, tmp, data)
#else
#define FN_GLGETVARIANTINTEGERVEXT(id, value, data) \
	fn.glGetVariantIntegervEXT(id, value, HostAddr(data, GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETVARIANTFLOATVEXT(id, value, data) \
	GLint size = 0; \
	GLint datatype = 0; \
	fn.glGetVariantIntegervEXT(id, GL_VARIANT_DATATYPE_EXT, &datatype); \
	switch (datatype) { \
		case GL_SCALAR_EXT: size = 1; break; \
		case GL_VECTOR_EXT: size = 4; break; \
		case GL_MATRIX_EXT: size = 16; break; \
	} \
	GLfloat tmp[size]; \
	fn.glGetVariantFloatvEXT(id, value, tmp); \
	Host2AtariFloatArray(size, tmp, data)
#else
#define FN_GLGETVARIANTFLOATVEXT(id, value, data) \
	fn.glGetVariantFloatvEXT(id, value, HostAddr(data, GLfloat *))
#endif

#define FN_GLGETVARIANTPOINTERVEXT(id, value, data) \
	void *ptr = 0; \
	memptr addr = 0; \
	fn.glGetVariantPointervEXT(id, value, &ptr); \
	if (ptr == contexts[cur_context].variant.host_pointer) \
		addr = (memptr)(uintptr_t)NFHost2AtariAddr(contexts[cur_context].variant.atari_pointer); \
	else \
		addr = 0; \
	Host2AtariIntArray(1, &addr, AtariAddr(data, memptr *))

/* -------------------------------------------------------------------------- */

/*
 * GL_ATI_vertex_streams
 */
#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXSTREAM1SVATI(stream, coords) \
	GLshort tmp[1]; \
	Atari2HostShortArray(1, coords, tmp); \
	fn.glVertexStream1svATI(stream, tmp)
#else
#define FN_GLVERTEXSTREAM1SVATI(stream, coords) \
	fn.glVertexStream1svATI(stream, HostAddr(coords, const GLshort *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXSTREAM1IVATI(stream, coords) \
	GLint tmp[1]; \
	Atari2HostIntArray(1, coords, tmp); \
	fn.glVertexStream1ivATI(stream, tmp)
#else
#define FN_GLVERTEXSTREAM1IVATI(stream, coords) \
	fn.glVertexStream1ivATI(stream, HostAddr(coords, const GLint *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLVERTEXSTREAM1FVATI(stream, coords) \
	GLfloat tmp[1]; \
	Atari2HostFloatArray(1, coords, tmp); \
	fn.glVertexStream1fvATI(stream, tmp)
#else
#define FN_GLVERTEXSTREAM1FVATI(stream, coords) \
	fn.glVertexStream1fvATI(stream, HostAddr(coords, const GLfloat *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLVERTEXSTREAM1DVATI(stream, coords) \
	GLdouble tmp[1]; \
	Atari2HostDoubleArray(1, coords, tmp); \
	fn.glVertexStream1dvATI(stream, tmp)
#else
#define FN_GLVERTEXSTREAM1DVATI(stream, coords) \
	fn.glVertexStream1dvATI(stream, HostAddr(coords, const GLdouble *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXSTREAM2SVATI(stream, coords) \
	GLshort tmp[2]; \
	Atari2HostShortArray(2, coords, tmp); \
	fn.glVertexStream2svATI(stream, tmp)
#else
#define FN_GLVERTEXSTREAM2SVATI(stream, coords) \
	fn.glVertexStream2svATI(stream, HostAddr(coords, const GLshort *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXSTREAM2IVATI(stream, coords) \
	GLint tmp[2]; \
	Atari2HostIntArray(2, coords, tmp); \
	fn.glVertexStream2ivATI(stream, tmp)
#else
#define FN_GLVERTEXSTREAM2IVATI(stream, coords) \
	fn.glVertexStream2ivATI(stream, HostAddr(coords, const GLint *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLVERTEXSTREAM2FVATI(stream, coords) \
	GLfloat tmp[2]; \
	Atari2HostFloatArray(2, coords, tmp); \
	fn.glVertexStream2fvATI(stream, tmp)
#else
#define FN_GLVERTEXSTREAM2FVATI(stream, coords) \
	fn.glVertexStream2fvATI(stream, HostAddr(coords, const GLfloat *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLVERTEXSTREAM2DVATI(stream, coords) \
	GLdouble tmp[2]; \
	Atari2HostDoubleArray(2, coords, tmp); \
	fn.glVertexStream2dvATI(stream, tmp)
#else
#define FN_GLVERTEXSTREAM2DVATI(stream, coords) \
	fn.glVertexStream2dvATI(stream, HostAddr(coords, const GLdouble *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXSTREAM3SVATI(stream, coords) \
	GLshort tmp[3]; \
	Atari2HostShortArray(3, coords, tmp); \
	fn.glVertexStream3svATI(stream, tmp)
#else
#define FN_GLVERTEXSTREAM3SVATI(stream, coords) \
	fn.glVertexStream3svATI(stream, HostAddr(coords, const GLshort *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXSTREAM3IVATI(stream, coords) \
	GLint tmp[3]; \
	Atari2HostIntArray(3, coords, tmp); \
	fn.glVertexStream3ivATI(stream, tmp)
#else
#define FN_GLVERTEXSTREAM3IVATI(stream, coords) \
	fn.glVertexStream3ivATI(stream, HostAddr(coords, const GLint *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLVERTEXSTREAM3FVATI(stream, coords) \
	GLfloat tmp[3]; \
	Atari2HostFloatArray(3, coords, tmp); \
	fn.glVertexStream3fvATI(stream, tmp)
#else
#define FN_GLVERTEXSTREAM3FVATI(stream, coords) \
	fn.glVertexStream3fvATI(stream, HostAddr(coords, const GLfloat *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLVERTEXSTREAM3DVATI(stream, coords) \
	GLdouble tmp[3]; \
	Atari2HostDoubleArray(3, coords, tmp); \
	fn.glVertexStream3dvATI(stream, tmp)
#else
#define FN_GLVERTEXSTREAM3DVATI(stream, coords) \
	fn.glVertexStream3dvATI(stream, HostAddr(coords, const GLdouble *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLNORMALSTREAM3BVATI(stream, coords) \
	GLbyte tmp[3]; \
	Atari2HostByteArray(3, coords, tmp); \
	fn.glNormalStream3bvATI(stream, tmp)
#else
#define FN_GLNORMALSTREAM3BVATI(stream, coords) \
	fn.glNormalStream3bvATI(stream, HostAddr(coords, const GLbyte *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLNORMALSTREAM3SVATI(stream, coords) \
	GLshort tmp[3]; \
	Atari2HostShortArray(3, coords, tmp); \
	fn.glNormalStream3svATI(stream, tmp)
#else
#define FN_GLNORMALSTREAM3SVATI(stream, coords) \
	fn.glNormalStream3svATI(stream, HostAddr(coords, const GLshort *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLNORMALSTREAM3IVATI(stream, coords) \
	GLint tmp[3]; \
	Atari2HostIntArray(3, coords, tmp); \
	fn.glNormalStream3ivATI(stream, tmp)
#else
#define FN_GLNORMALSTREAM3IVATI(stream, coords) \
	fn.glNormalStream3ivATI(stream, HostAddr(coords, const GLint *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLNORMALSTREAM3FVATI(stream, coords) \
	GLfloat tmp[3]; \
	Atari2HostFloatArray(3, coords, tmp); \
	fn.glNormalStream3fvATI(stream, tmp)
#else
#define FN_GLNORMALSTREAM3FVATI(stream, coords) \
	fn.glNormalStream3fvATI(stream, HostAddr(coords, const GLfloat *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLNORMALSTREAM3DVATI(stream, coords) \
	GLdouble tmp[3]; \
	Atari2HostDoubleArray(3, coords, tmp); \
	fn.glNormalStream3dvATI(stream, tmp)
#else
#define FN_GLNORMALSTREAM3DVATI(stream, coords) \
	fn.glNormalStream3dvATI(stream, HostAddr(coords, const GLdouble *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXSTREAM4SVATI(stream, coords) \
	GLshort tmp[4]; \
	Atari2HostShortArray(4, coords, tmp); \
	fn.glVertexStream4svATI(stream, tmp)
#else
#define FN_GLVERTEXSTREAM4SVATI(stream, coords) \
	fn.glVertexStream4svATI(stream, HostAddr(coords, const GLshort *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXSTREAM4IVATI(stream, coords) \
	GLint tmp[4]; \
	Atari2HostIntArray(4, coords, tmp); \
	fn.glVertexStream4ivATI(stream, tmp)
#else
#define FN_GLVERTEXSTREAM4IVATI(stream, coords) \
	fn.glVertexStream4ivATI(stream, HostAddr(coords, const GLint *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLVERTEXSTREAM4FVATI(stream, coords) \
	GLfloat tmp[4]; \
	Atari2HostFloatArray(4, coords, tmp); \
	fn.glVertexStream4fvATI(stream, tmp)
#else
#define FN_GLVERTEXSTREAM4FVATI(stream, coords) \
	fn.glVertexStream4fvATI(stream, HostAddr(coords, const GLfloat *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLVERTEXSTREAM4DVATI(stream, coords) \
	GLdouble tmp[4]; \
	Atari2HostDoubleArray(4, coords, tmp); \
	fn.glVertexStream4dvATI(stream, tmp)
#else
#define FN_GLVERTEXSTREAM4DVATI(stream, coords) \
	fn.glVertexStream4dvATI(stream, HostAddr(coords, const GLdouble *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_EXT_secondary_color
 */

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLSECONDARYCOLOR3BVEXT(v) \
	GLbyte tmp[3]; \
	Atari2HostByteArray(3, v, tmp); \
	fn.glSecondaryColor3bvEXT(tmp)
#else
#define FN_GLSECONDARYCOLOR3BVEXT(v) \
	fn.glSecondaryColor3bvEXT(HostAddr(v, const GLbyte *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLSECONDARYCOLOR3DVEXT(v) \
	GLdouble tmp[3]; \
	Atari2HostDoubleArray(3, v, tmp); \
	fn.glSecondaryColor3dvEXT(tmp)
#else
#define FN_GLSECONDARYCOLOR3DVEXT(v) \
	fn.glSecondaryColor3dvEXT(HostAddr(v, const GLdouble *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLSECONDARYCOLOR3FVEXT(v) \
	GLfloat tmp[3]; \
	Atari2HostFloatArray(3, v, tmp); \
	fn.glSecondaryColor3fvEXT(tmp)
#else
#define FN_GLSECONDARYCOLOR3FVEXT(v) \
	fn.glSecondaryColor3fvEXT(HostAddr(v, const GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLSECONDARYCOLOR3IVEXT(v) \
	GLint tmp[3]; \
	Atari2HostIntArray(3, v, tmp); \
	fn.glSecondaryColor3ivEXT(tmp)
#else
#define FN_GLSECONDARYCOLOR3IVEXT(v) \
	fn.glSecondaryColor3ivEXT(HostAddr(v, const GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLSECONDARYCOLOR3SVEXT(v) \
	GLshort tmp[3]; \
	Atari2HostShortArray(3, v, tmp); \
	fn.glSecondaryColor3svEXT(tmp)
#else
#define FN_GLSECONDARYCOLOR3SVEXT(v) \
	fn.glSecondaryColor3svEXT(HostAddr(v, const GLshort *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLSECONDARYCOLOR3UBVEXT(v) \
	GLubyte tmp[3]; \
	Atari2HostByteArray(3, v, tmp); \
	fn.glSecondaryColor3ubvEXT(tmp)
#else
#define FN_GLSECONDARYCOLOR3UBVEXT(v) \
	fn.glSecondaryColor3ubvEXT(HostAddr(v, const GLubyte *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLSECONDARYCOLOR3UIVEXT(v) \
	GLuint tmp[3]; \
	Atari2HostIntArray(3, v, tmp); \
	fn.glSecondaryColor3uivEXT(tmp)
#else
#define FN_GLSECONDARYCOLOR3UIVEXT(v) \
	fn.glSecondaryColor3uivEXT(HostAddr(v, const GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLSECONDARYCOLOR3USVEXT(v) \
	GLushort tmp[3]; \
	Atari2HostShortArray(3, v, tmp); \
	fn.glSecondaryColor3usvEXT(tmp)
#else
#define FN_GLSECONDARYCOLOR3USVEXT(v) \
	fn.glSecondaryColor3usvEXT(HostAddr(v, const GLushort *))
#endif

#define FN_GLSECONDARYCOLORPOINTEREXT(size, type, stride, pointer) \
	GLint texunit = 0; \
	fn.glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE, &texunit); \
	setupClientArray(texunit, contexts[cur_context].secondary_color, size, type, stride, 0, 0, AtariAddr(pointer, nfmemptr)); \
	contexts[cur_context].secondary_color.vendor = NFOSMESA_VENDOR_EXT

/* -------------------------------------------------------------------------- */

/*
 * GL_EXT_separate_shader_objects
 */
#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLCREATESHADERPROGRAMEXT(type, string) \
	GLchar stringbuf[safe_strlen(string) + 1], *pstring ; \
	pstring = Atari2HostByteArray(sizeof(stringbuf), string, stringbuf); \
	return fn.glCreateShaderProgramEXT(type, pstring)
#else
#define FN_GLCREATESHADERPROGRAMEXT(type, string) \
	return fn.glCreateShaderProgramEXT(type, HostAddr(string, const GLchar *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_ATI_vertex_array_object
 */
#define FN_GLDELETEOBJECTBUFFERATI(buffer) nfglFreeObjectBufferATI(buffer)

#define FN_GLNEWOBJECTBUFFERATI(size, pointer, usage) \
	gl_buffer_t *buf = gl_make_buffer(size, AtariAddr(pointer, nfcmemptr)); \
	if (!buf) return 0; \
	GLuint name = fn.glNewObjectBufferATI(size, buf->host_buffer, usage); \
	if (!name) return name; \
	buf->name = name; \
	buf->usage = usage; \
	return name

#define FN_GLFREEOBJECTBUFFERATI(buffer) \
	gl_buffer_t *buf = gl_get_buffer(buffer); \
	if (buf) { \
		buf->atari_buffer = 0; \
		free(buf->host_buffer); \
		buf->host_buffer = NULL; \
		buf->size = 0; \
	}

#define FN_GLUPDATEOBJECTBUFFERATI(buffer, offset, size, pointer, preserve) \
	gl_buffer_t *buf = gl_get_buffer(buffer); \
	if (!buf) buf = gl_make_buffer(offset + size, AtariAddr(pointer, nfcmemptr)); \
	if (!buf) return; \
	buf->name = buffer; \
	Atari2HostByteArray(size, AtariAddr(pointer, const char *), buf->host_buffer + offset); \
	convertClientArrays(0); \
	fn.glUpdateObjectBufferATI(buffer, offset, size, buf->host_buffer, preserve)
	
#define FN_GLGETARRAYOBJECTFVATI(array, pname, params) \
	GLint const size = 1; \
	GLfloat tmp[size]; \
	fn.glGetArrayObjectfvATI(array, pname, tmp); \
	Host2AtariFloatArray(size, tmp, params)

#define FN_GLGETARRAYOBJECTIVATI(array, pname, params) \
	GLint const size = 1; \
	GLint tmp[size]; \
	fn.glGetArrayObjectivATI(array, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)

#define FN_GLGETOBJECTBUFFERFVATI(buffer, pname, params) \
	GLint const size = 1; \
	GLfloat tmp[size]; \
	fn.glGetObjectBufferfvATI(buffer, pname, tmp); \
	Host2AtariFloatArray(size, tmp, params)

#define FN_GLGETOBJECTBUFFERIVATI(buffer, pname, params) \
	GLint const size = 1; \
	GLint tmp[size]; \
	fn.glGetObjectBufferivATI(buffer, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)

#define FN_GLGETVARIANTARRAYOBJECTFVATI(id, pname, params) \
	GLint const size = 1; \
	GLfloat tmp[size]; \
	fn.glGetVariantArrayObjectfvATI(id, pname, tmp); \
	Host2AtariFloatArray(size, tmp, params)

#define FN_GLGETVARIANTARRAYOBJECTIVATI(id, pname, params) \
	GLint const size = 1; \
	GLint tmp[size]; \
	fn.glGetVariantArrayObjectivATI(id, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)

#define FN_GLARRAYOBJECTATI(array, size, type, stride, buffer, offset) \
	vertexarray_t *arr = gl_get_array(array); \
	if (!arr) return; \
	gl_buffer_t *buf = gl_get_buffer(buffer); \
	if (!buf) return; \
	arr->buffer_offset = offset; \
	setupClientArray(0, *arr, size, type, stride, -1, 0, buf->atari_buffer + offset)

/* -------------------------------------------------------------------------- */

/*
 * GL_ATI_vertex_attrib_array_object
 */

#define FN_GLVERTEXATTRIBARRAYOBJECTATI(array, size, type, nomralized, stride, buffer, offset) \
	vertexarray_t *arr = gl_get_array(array); \
	if (!arr) return; \
	gl_buffer_t *buf = gl_get_buffer(buffer); \
	if (!buf) return; \
	arr->buffer_offset = offset; \
	buf->normalized = normalized; \
	setupClientArray(0, *arr, size, type, stride, -1, 0, buf->atari_buffer + offset)

#define FN_GLGETVERTEXATTRIBARRAYOBJECTFVATI(index, pname, params) \
	GLint const size = 1; \
	GLfloat tmp[size]; \
	fn.glGetVertexAttribArrayObjectfvATI(index, pname, tmp); \
	Host2AtariFloatArray(size, tmp, params)

#define FN_GLGETVERTEXATTRIBARRAYOBJECTIVATI(index, pname, params) \
	GLint const size = 1; \
	GLint tmp[size]; \
	fn.glGetVertexAttribArrayObjectivATI(index, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)

/* -------------------------------------------------------------------------- */

/*
 * GL_ARB_shader_objects
 */

#define FN_GLSHADERSOURCEARB(shaderObj, count, strings, length) \
	void *pstrings[MAX(count, 0)], **ppstrings; \
	GLint lengthbuf[MAX(count, 0)], *plength; \
	ppstrings = Atari2HostPtrArray(count, strings, pstrings); \
	plength = Atari2HostIntArray(count, length, lengthbuf); \
	fn.glShaderSourceARB(shaderObj, count, (const GLcharARB **)ppstrings, plength)

/* -------------------------------------------------------------------------- */

/*
 * GL_ARB_shading_language_include
 */
#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLNAMEDSTRINGARB(type, namelen, name, stringlen, string) \
	if (namelen < 0) namelen = safe_strlen(name); \
	if (stringlen < 0) stringlen = safe_strlen(string); \
	GLchar namebuf[namelen], *pname; \
	GLchar stringbuf[stringlen], *pstring; \
	pname = Atari2HostByteArray(sizeof(namebuf), name, namebuf); \
	pstring = Atari2HostByteArray(sizeof(stringbuf), string, stringbuf); \
	fn.glNamedStringARB(type, namelen, pname, stringlen, pstring)
#else
#define FN_GLNAMEDSTRINGARB(type, namelen, name, stringlen, string) \
	fn.glNamedStringARB(type, namelen, HostAddr(name, const GLchar *), stringlen, HostAddr(string, const GLchar *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLDELETENAMEDSTRINGARB(namelen, name) \
	if (namelen < 0) namelen = safe_strlen(name); \
	GLchar namebuf[namelen], *pname; \
	pname = Atari2HostByteArray(sizeof(namebuf), name, namebuf); \
	return fn.glDeleteNamedStringARB(namelen, pname)
#else
#define FN_GLDELETENAMEDSTRINGARB(namelen, name) \
	return fn.glDeleteNamedStringARB(namelen, HostAddr(name, const GLchar *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLISNAMEDSTRINGARB(namelen, name) \
	if (namelen < 0) namelen = safe_strlen(name); \
	GLchar namebuf[namelen], *pname; \
	pname = Atari2HostByteArray(sizeof(namebuf), name, namebuf); \
	return fn.glIsNamedStringARB(namelen, pname)
#else
#define FN_GLISNAMEDSTRINGARB(namelen, name) \
	return fn.glIsNamedStringARB(namelen, HostAddr(name, const GLchar *))
#endif

#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_INT_CONV
#define FN_GLGETNAMEDSTRINGIVARB(namelen, name, pname, params) \
	if (namelen < 0) namelen = safe_strlen(name); \
	GLchar namebuf[namelen], *ptmp; \
	ptmp = Atari2HostByteArray(sizeof(namebuf), name, namebuf); \
	GLint const size = 1; \
	GLint tmp[size]; \
	fn.glGetNamedStringivARB(namelen, ptmp, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETNAMEDSTRINGIVARB(namelen, name, pname, params) \
	fn.glGetNamedStringivARB(namelen, HostAddr(name, const GLchar *), pname, HostAddr(params, GLint *))
#endif

#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_INT_CONV
#define FN_GLGETNAMEDSTRINGARB(namelen, name, bufSize, stringlen, string) \
	GLint l = 0; \
	GLchar namebuf[MAX(namelen, 0)], *ptmp; \
	GLchar stringbuf[MAX(bufSize, 0)]; \
	ptmp = Atari2HostByteArray(sizeof(namebuf), name, namebuf); \
	fn.glGetNamedStringARB(namelen, ptmp, bufSize, &l, stringbuf); \
	Host2AtariByteArray(MIN(l + 1, bufSize), stringbuf, string); \
	Host2AtariIntArray(1, &l, stringlen)
#else
#define FN_GLGETNAMEDSTRINGARB(namelen, name, bufSize, stringlen, string) \
	fn.glGetNamedStringARB(namelen, HostAddr(name, const GLchar *), bufSize, HostAddr(stringlen, GLint *), HostAddr(string, char *))
#endif

#define FN_GLCOMPILESHADERINCLUDEARB(shader, count, path, length) \
	void *pathbuf[MAX(count, 0)], **ppath; \
	GLint lengthbuf[MAX(count, 0)], *plength; \
	/* FIXME: the pathnames here are meaningless to the host */ \
	ppath = Atari2HostPtrArray(count, path, pathbuf); \
	plength = Atari2HostIntArray(count, length, lengthbuf); \
	fn.glCompileShaderIncludeARB(shader, count, (const char *const *)ppath, plength)

/* -------------------------------------------------------------------------- */

/*
 * GL_APPLE_element_array
 */

#define FN_GLELEMENTPOINTERAPPLE(type, pointer) \
	GLint texunit = 0; \
	fn.glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE, &texunit); \
	setupClientArray(texunit, contexts[cur_context].element, 1, type, 0, -1, 0, AtariAddr(pointer, nfmemptr)); \
	contexts[cur_context].element.vendor = NFOSMESA_VENDOR_APPLE

#define FN_GLDRAWELEMENTARRAYAPPLE(mode, first, count) \
	convertClientArrays(first + count); \
	fn.glDrawElementArrayAPPLE(mode, first, count)

#define FN_GLDRAWRANGEELEMENTARRAYAPPLE(mode, start, end, first, count) \
	convertClientArrays(first + count); \
	fn.glDrawRangeElementArrayAPPLE(mode, start, end, first, count)

#define FN_GLMULTIDRAWELEMENTARRAYAPPLE(mode, first, count, primcount) \
	GLsizei const size = primcount; \
	GLint firstbuf[size]; \
	GLsizei countbuf[size]; \
	if (size <= 0) { glSetError(GL_INVALID_VALUE); return; } \
	Atari2HostIntArray(size, first, firstbuf); \
	Atari2HostIntArray(size, count, countbuf); \
	for (GLsizei i = 0; i < size; i++) \
		convertClientArrays(firstbuf[i] + countbuf[i]); \
	fn.glMultiDrawElementArrayAPPLE(mode, firstbuf, countbuf, primcount)

#define FN_GLMULTIDRAWRANGEELEMENTARRAYAPPLE(mode, start, end, first, count, primcount) \
	GLsizei const size = primcount; \
	GLint firstbuf[size]; \
	GLsizei countbuf[size]; \
	if (size <= 0) { glSetError(GL_INVALID_VALUE); return; } \
	Atari2HostIntArray(size, first, firstbuf); \
	Atari2HostIntArray(size, count, countbuf); \
	for (GLsizei i = 0; i < size; i++) \
		convertClientArrays(firstbuf[i] + countbuf[i]); \
	fn.glMultiDrawRangeElementArrayAPPLE(mode, start, end, firstbuf, countbuf, primcount)

/* -------------------------------------------------------------------------- */

/*
 * GL_ATI_element_array
 */

#define FN_GLELEMENTPOINTERATI(type, pointer) \
	GLint texunit = 0; \
	fn.glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE, &texunit); \
	setupClientArray(texunit, contexts[cur_context].element, 1, type, 0, -1, 0, AtariAddr(pointer, nfmemptr)); \
	contexts[cur_context].element.vendor = NFOSMESA_VENDOR_ATI

#define FN_GLDRAWELEMENTARRAYATI(mode, count) \
	convertClientArrays(count); \
	fn.glDrawElementArrayATI(mode, count)

#define FN_GLDRAWRANGEELEMENTARRAYATI(mode, start, end, count) \
	convertClientArrays(count); \
	fn.glDrawRangeElementArrayATI(mode, start, end, count)

/* -------------------------------------------------------------------------- */

/*
 * GL_APPLE_object_purgeable
 */
#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETOBJECTPARAMETERIVAPPLE(objectType, name, pname, params) \
	GLint const size = 1; \
	GLint tmp[1]; \
	fn.glGetObjectParameterivAPPLE(objectType, name, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETOBJECTPARAMETERIVAPPLE(objectType, name, pname, params) \
	fn.glGetObjectParameterivAPPLE(objectType, name, pname, HostAddr(params, GLint *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_NV_occlusion_query
 */
#if NFOSMESA_NEED_INT_CONV
#define FN_GLGENOCCLUSIONQUERIESNV(n, ids) \
	GLsizei const size = MAX(n, 0); \
	GLuint tmp[size]; \
	fn.glGenOcclusionQueriesNV(n, tmp); \
	Host2AtariIntArray(n, tmp, ids)
#else
#define FN_GLGENOCCLUSIONQUERIESNV(n, ids) \
	fn.glGenOcclusionQueriesNV(n, HostAddr(ids, GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLDELETEOCCLUSIONQUERIESNV(n, ids) \
	GLsizei const size = n; \
	GLuint tmp[size]; \
	if (size <= 0) { glSetError(GL_INVALID_VALUE); return; } \
	Atari2HostIntArray(size, ids, tmp); \
	fn.glDeleteOcclusionQueriesNV(n, tmp)
#else
#define FN_GLDELETEOCCLUSIONQUERIESNV(n, ids) \
	fn.glDeleteOcclusionQueriesNV(n, HostAddr(ids, const GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETOCCLUSIONQUERYIVNV(id, pname, params) \
	GLsizei const size = 1; \
	GLint tmp[size]; \
	fn.glGetOcclusionQueryivNV(id, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETOCCLUSIONQUERYIVNV(id, pname, params) \
	fn.glGetOcclusionQueryivNV(id, pname, HostAddr(params, GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETOCCLUSIONQUERYUIVNV(id, pname, params) \
	GLsizei const size = 1; \
	GLuint tmp[size]; \
	fn.glGetOcclusionQueryuivNV(id, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETOCCLUSIONQUERYUIVNV(id, pname, params) \
	fn.glGetOcclusionQueryuivNV(id, pname, HostAddr(params, GLuint *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_NV_path_rendering
 */

#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLCOVERFILLPATHINSTANCEDNV(numPaths, pathNameType, paths, pathBase, coverMode, transformType, transformValues) \
	pixelBuffer buf(*this); \
	void *tmp = buf.convertArray(numPaths, pathNameType, (nfmemptr)paths); \
	GLfloat transformbuf[MAX(numPaths, 0)], *vals; \
	vals = Atari2HostFloatArray(numPaths, transformValues, transformbuf); \
	fn.glCoverFillPathInstancedNV(numPaths, pathNameType, tmp, pathBase, coverMode, transformType, vals)
#else
#define FN_GLCOVERFILLPATHINSTANCEDNV(numPaths, pathNameType, paths, pathBase, coverMode, transformType, transformValues) \
	fn.glCoverFillPathInstancedNV(numPaths, pathNameType, HostAddr(paths, const void *), pathBase, coverMode, transformType, HostAddr(transformValues, const GLfloat *))
#endif

#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLCOVERSTROKEPATHINSTANCEDNV(numPaths, pathNameType, paths, pathBase, coverMode, transformType, transformValues) \
	pixelBuffer buf(*this); \
	void *tmp = buf.convertArray(numPaths, pathNameType, (nfmemptr)paths); \
	GLfloat vals[MAX(numPaths, 0)], *pvals; \
	pvals = Atari2HostFloatArray(numPaths, transformValues, vals); \
	fn.glCoverStrokePathInstancedNV(numPaths, pathNameType, tmp, pathBase, coverMode, transformType, pvals)
#else
#define FN_GLCOVERSTROKEPATHINSTANCEDNV(numPaths, pathNameType, paths, pathBase, coverMode, transformType, transformValues) \
	fn.glCoverStrokePathInstancedNV(numPaths, pathNameType, HostAddr(paths, const void *), pathBase, coverMode, transformType, HostAddr(transformValues, const GLfloat *))
#endif

#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPATHCOMMANDSNV(path, numCommands, commands, numCoords, coordType, coords) \
	pixelBuffer buf(*this); \
	GLubyte tmpcommands[MAX(numCommands, 0)], *pcommands; \
	void *tmp = buf.convertArray(numCoords, coordType, (nfmemptr)coords); \
	pcommands = Atari2HostByteArray(numCommands, commands, tmpcommands); \
	fn.glPathCommandsNV(path, numCommands, pcommands, numCoords, coordType, tmp)
#else
#define FN_GLPATHCOMMANDSNV(path, numCommands, commands, numCoords, coordType, coords) \
	fn.glPathCommandsNV(path, numCommands, HostAddr(commands, const GLubyte *), numCoords, coordType, HostAddr(coords, const void *))
#endif

#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPATHCOORDSNV(path, numCoords, coordType, coords) \
	pixelBuffer buf(*this); \
	void *tmp = buf.convertArray(numCoords, coordType, (nfmemptr)coords); \
	fn.glPathCoordsNV(path, numCoords, coordType, tmp)
#else
#define FN_GLPATHCOORDSNV(path, numCoords, coordType, coords) \
	fn.glPathCoordsNV(path, numCoords, coordType, HostAddr(coords, const void *))
#endif

#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPATHSUBCOMMANDSNV(path, commandStart, commandsToDelete, numCommands, commands, numCoords, coordType, coords) \
	pixelBuffer buf(*this); \
	GLubyte tmpcommands[MAX(numCommands, 0)], *pcommands; \
	void *tmp = buf.convertArray(numCoords, coordType, (nfmemptr)coords); \
	pcommands = Atari2HostByteArray(numCommands, commands, tmpcommands); \
	fn.glPathSubCommandsNV(path, commandStart, commandsToDelete, numCommands, pcommands, numCoords, coordType, tmp)
#else
#define FN_GLPATHSUBCOMMANDSNV(path, commandStart, commandsToDelete, numCommands, commands, numCoords, coordType, coords) \
	fn.glPathSubCommandsNV(path, commandStart, commandsToDelete, numCommands, HostAddr(commands, const GLubyte *), numCoords, coordType, HostAddr(coords, const void *))
#endif

#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPATHSUBCOORDSNV(path, coordStart, numCoords, coordType, coords) \
	pixelBuffer buf(*this); \
	void *tmp = buf.convertArray(numCoords, coordType, (nfmemptr)coords); \
	fn.glPathSubCoordsNV(path, coordStart, numCoords, coordType, tmp);
#else
#define FN_GLPATHSUBCOORDSNV(path, coordStart, numCoords, coordType, coords) \
	fn.glPathSubCoordsNV(path, coordStart, numCoords, coordType, HostAddr(coords, const void *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLPATHSTRINGNV(path, format, length, pathString) \
	char stringbuf[MAX(length, 0)], *pstring; \
	pstring = Atari2HostByteArray(length, AtariAddr(pathString, const char *), stringbuf); \
	fn.glPathStringNV(path, format, length, pstring)
#else
#define FN_GLPATHSTRINGNV(path, format, length, pathString) \
	fn.glPathStringNV(path, format, length, HostAddr(pathString, const void *))
#endif
	
#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPATHGLYPHSNV(firstPathName, fontTarget, fontName, fontStyle, numGlyphs, type, charcodes, handleMissingGlyphs, pathParameterTemplate, emScale) \
	pixelBuffer buf(*this); \
	void *tmp = buf.convertArray(numGlyphs, type, (nfmemptr)charcodes); \
	/* fontName is ascii string */ \
	GLubyte namebuf[safe_strlen(fontName) + 1], *pfontName; \
	pfontName = Atari2HostByteArray(sizeof(namebuf), fontName, namebuf); \
	fn.glPathGlyphsNV(firstPathName, fontTarget, pfontName, fontStyle, numGlyphs, type, tmp, handleMissingGlyphs, pathParameterTemplate, emScale)
#else
#define FN_GLPATHGLYPHSNV(firstPathName, fontTarget, fontName, fontStyle, numGlyphs, type, charcodes, handleMissingGlyphs, pathParameterTemplate, emScale) \
	fn.glPathGlyphsNV(firstPathName, fontTarget, HostAddr(fontName, const GLubyte *), fontStyle, numGlyphs, type, HostAddr(charcodes, const void *), handleMissingGlyphs, pathParameterTemplate, emScale)
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLPATHGLYPHRANGENV(firstPathName, fontTarget, fontName, fontStyle, firstGlyph, numGlyphs, handleMissingGlyphs, pathParameterTemplate, emScale) \
	char namebuf[safe_strlen(fontName) + 1], *pname; \
	pname = Atari2HostByteArray(sizeof(namebuf), AtariAddr(fontName, const char *), namebuf); \
	fn.glPathGlyphRangeNV(firstPathName, fontTarget, pname, fontStyle, firstGlyph, numGlyphs, handleMissingGlyphs, pathParameterTemplate, emScale)
#else
#define FN_GLPATHGLYPHRANGENV(firstPathName, fontTarget, fontName, fontStyle, firstGlyph, numGlyphs, handleMissingGlyphs, pathParameterTemplate, emScale) \
	fn.glPathGlyphRangeNV(firstPathName, fontTarget, HostAddr(fontName, const void *), fontStyle, firstGlyph, numGlyphs, handleMissingGlyphs, pathParameterTemplate, emScale)
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLWEIGHTPATHSNV(resultPath, numPaths, paths, weights) \
	GLsizei const size = numPaths; \
	GLuint pathbuf[size]; \
	GLfloat weightbuf[size]; \
	if (size <= 0) { glSetError(GL_INVALID_VALUE); return; } \
	Atari2HostIntArray(size, paths, pathbuf); \
	Atari2HostFloatArray(size, weights, weightbuf); \
	fn.glWeightPathsNV(resultPath, numPaths, pathbuf, weightbuf)
#else
#define FN_GLWEIGHTPATHSNV(resultPath, numPaths, paths, weights) \
	fn.glWeightPathsNV(resultPath, numPaths, HostAddr(paths, const GLuint *), HostAddr(weights, const GLfloat *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLTRANSFORMPATHNV(resultPath, srcPath, transformType, transformValues) \
	GLsizei size; \
	GLfloat tmp[16]; \
	switch (transformType) { \
		case GL_NONE: size = 0; break; \
		case GL_TRANSLATE_X_NV: size = 1; break; \
		case GL_TRANSLATE_Y_NV: size = 1; break; \
		case GL_TRANSLATE_2D_NV: size = 2; break; \
		case GL_TRANSLATE_3D_NV: size = 3; break; \
		case GL_AFFINE_2D_NV: size = 6; break; \
		case GL_PROJECTIVE_2D_NV: size = 6; break; \
		case GL_TRANSPOSE_AFFINE_2D_NV: size = 6; break; \
		case GL_TRANSPOSE_PROJECTIVE_2D_NV: size = 6; break; \
		case GL_AFFINE_3D_NV: size = 12; break; \
		case GL_PROJECTIVE_3D_NV: size = 12; break; \
		case GL_TRANSPOSE_AFFINE_3D_NV: size = 12; break; \
		case GL_TRANSPOSE_PROJECTIVE_3D_NV: size = 12; break; \
		default: glSetError(GL_INVALID_ENUM); return; \
	} \
	Atari2HostFloatArray(size, transformValues, tmp); \
	fn.glTransformPathNV(resultPath, srcPath, transformType, tmp)
#else
#define FN_GLTRANSFORMPATHNV(resultPath, srcPath, transformType, transformValues) \
	fn.glTransformPathNV(resultPath, srcPath, transformType, HostAddr(transformValues, const GLfloat *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPATHPARAMETERFVNV(path, pname, value) \
	GLsizei const size = nfglGetNumParams(pname); \
	GLfloat tmp[MAX(size, 16)]; \
	Atari2HostFloatArray(size, value, tmp); \
	fn.glPathParameterfvNV(path, pname, tmp)
#else
#define FN_GLPATHPARAMETERFVNV(path, pname, value) \
	fn.glPathParameterfvNV(path, pname, HostAddr(value, const GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLPATHPARAMETERIVNV(path, pname, value) \
	GLsizei const size = nfglGetNumParams(pname); \
	GLint tmp[MAX(size, 16)]; \
	Atari2HostIntArray(size, value, tmp); \
	fn.glPathParameterivNV(path, pname, tmp)
#else
#define FN_GLPATHPARAMETERIVNV(path, pname, value) \
	fn.glPathParameterivNV(path, pname, HostAddr(value, const GLint *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETPATHPARAMETERFVNV(path, pname, value) \
	GLsizei const size = nfglGetNumParams(pname); \
	GLfloat tmp[MAX(size, 16)]; \
	fn.glGetPathParameterfvNV(path, pname, tmp); \
	Host2AtariFloatArray(size, tmp, value)
#else
#define FN_GLGETPATHPARAMETERFVNV(path, pname, value) \
	fn.glGetPathParameterfvNV(path, pname, HostAddr(value, GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETPATHPARAMETERIVNV(path, pname, value) \
	GLsizei const size = nfglGetNumParams(pname); \
	GLint tmp[MAX(size, 16)]; \
	fn.glGetPathParameterivNV(path, pname, tmp); \
	Host2AtariIntArray(size, tmp, value)
#else
#define FN_GLGETPATHPARAMETERIVNV(path, pname, value) \
	fn.glGetPathParameterivNV(path, pname, HostAddr(value, GLint *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPATHDASHARRAYNV(path, dashCount, dashArray) \
	GLsizei const size = dashCount; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, dashArray, tmp); \
	fn.glPathDashArrayNV(path, dashCount, tmp)
#else
#define FN_GLPATHDASHARRAYNV(path, dashCount, dashArray) \
	fn.glPathDashArrayNV(path, dashCount, HostAddr(dashArray, const GLfloat *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETPATHDASHARRAYNV(path, dashArray) \
	GLint size = 0; \
	fn.glGetPathParameterivNV(path, GL_PATH_DASH_ARRAY_COUNT_NV, &size); \
	if (size <= 0) return; \
	GLfloat tmp[size]; \
	fn.glGetPathDashArrayNV(path, tmp); \
	Host2AtariFloatArray(size, tmp, dashArray)
#else
#define FN_GLGETPATHDASHARRAYNV(path, dashArray) \
	fn.glGetPathDashArrayNV(path, HostAddr(dashArray, GLfloat *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLGETPATHCOMMANDSNV(path, commands) \
	GLint size = 0; \
	fn.glGetPathParameterivNV(path, GL_PATH_COMMAND_COUNT_NV, &size); \
	if (size <= 0) return; \
	GLubyte tmp[size]; \
	fn.glGetPathCommandsNV(path, tmp); \
	Host2AtariByteArray(size, tmp, commands)
#else
#define FN_GLGETPATHCOMMANDSNV(path, commands) \
	fn.glGetPathCommandsNV(path, HostAddr(commands, GLubyte *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETPATHCOORDSNV(path, coords) \
	GLint size = 0; \
	fn.glGetPathParameterivNV(path, GL_PATH_COORD_COUNT_NV, &size); \
	if (size <= 0) return; \
	GLfloat tmp[size]; \
	fn.glGetPathCoordsNV(path, tmp); \
	Host2AtariFloatArray(size, tmp, coords)
#else
#define FN_GLGETPATHCOORDSNV(path, coords) \
	fn.glGetPathCoordsNV(path, HostAddr(coords, GLfloat *))
#endif

#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLSTENCILFILLPATHINSTANCEDNV(numPaths, pathNameType, paths, pathBase, fillMode, mask, transformType, transformValues) \
	GLsizei size; \
	pixelBuffer buf(*this); \
	GLfloat tmp[16]; \
	switch (transformType) { \
		case GL_NONE: size = 0; break; \
		case GL_TRANSLATE_X_NV: size = 1; break; \
		case GL_TRANSLATE_Y_NV: size = 1; break; \
		case GL_TRANSLATE_2D_NV: size = 2; break; \
		case GL_TRANSLATE_3D_NV: size = 3; break; \
		case GL_AFFINE_2D_NV: size = 6; break; \
		case GL_PROJECTIVE_2D_NV: size = 6; break; \
		case GL_TRANSPOSE_AFFINE_2D_NV: size = 6; break; \
		case GL_TRANSPOSE_PROJECTIVE_2D_NV: size = 6; break; \
		case GL_AFFINE_3D_NV: size = 12; break; \
		case GL_PROJECTIVE_3D_NV: size = 12; break; \
		case GL_TRANSPOSE_AFFINE_3D_NV: size = 12; break; \
		case GL_TRANSPOSE_PROJECTIVE_3D_NV: size = 12; break; \
		default: glSetError(GL_INVALID_ENUM); return; \
	} \
	void *ppaths = buf.convertArray(numPaths, pathNameType, (nfmemptr)paths); \
	Atari2HostFloatArray(size, transformValues, tmp); \
	fn.glStencilFillPathInstancedNV(numPaths, pathNameType, ppaths, pathBase, fillMode, mask, transformType, tmp)
#else
#define FN_GLSTENCILFILLPATHINSTANCEDNV(numPaths, pathNameType, paths, pathBase, fillMode, mask, transformType, transformValues) \
	fn.glStencilFillPathInstancedNV(numPaths, pathNameType, HostAddr(paths, const void *), pathBase, fillMode, mask, transformType, HostAddr(transformValues, const GLfloat *))
#endif

#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLSTENCILSTROKEPATHINSTANCEDNV(numPaths, pathNameType, paths, pathBase, reference, mask, transformType, transformValues) \
	GLsizei size; \
	GLfloat tmp[16]; \
	switch (transformType) { \
		case GL_NONE: size = 0; break; \
		case GL_TRANSLATE_X_NV: size = 1; break; \
		case GL_TRANSLATE_Y_NV: size = 1; break; \
		case GL_TRANSLATE_2D_NV: size = 2; break; \
		case GL_TRANSLATE_3D_NV: size = 3; break; \
		case GL_AFFINE_2D_NV: size = 6; break; \
		case GL_PROJECTIVE_2D_NV: size = 6; break; \
		case GL_TRANSPOSE_AFFINE_2D_NV: size = 6; break; \
		case GL_TRANSPOSE_PROJECTIVE_2D_NV: size = 6; break; \
		case GL_AFFINE_3D_NV: size = 12; break; \
		case GL_PROJECTIVE_3D_NV: size = 12; break; \
		case GL_TRANSPOSE_AFFINE_3D_NV: size = 12; break; \
		case GL_TRANSPOSE_PROJECTIVE_3D_NV: size = 12; break; \
		default: glSetError(GL_INVALID_ENUM); return; \
	} \
	pixelBuffer buf(*this); \
	void *ppaths = buf.convertArray(numPaths, pathNameType, (nfmemptr)paths); \
	Atari2HostFloatArray(size, transformValues, tmp); \
	fn.glStencilStrokePathInstancedNV(numPaths, pathNameType, ppaths, pathBase, reference, mask, transformType, tmp)
#else
#define FN_GLSTENCILSTROKEPATHINSTANCEDNV(numPaths, pathNameType, paths, pathBase, reference, mask, transformType, transformValues) \
	fn.glStencilStrokePathInstancedNV(numPaths, pathNameType, HostAddr(paths, const void *), pathBase, reference, mask, transformType, HostAddr(transformValues, const GLfloat *))
#endif

#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETPATHMETRICSNV(metricQueryMask, numPaths, pathNameType, paths, pathBase, stride, metrics) \
	GLsizei size = stride; \
	if (size < 0) { glSetError(GL_INVALID_VALUE); return; } \
	pixelBuffer buf(*this); \
	void *ppaths = buf.convertArray(numPaths, pathNameType, (nfmemptr)paths); \
	if (size == 0) { \
		GLbitfield mask = metricQueryMask; \
		while (mask) \
		{ \
			if (mask & 1) size += sizeof(GLfloat); \
			mask >>= 1; \
		} \
	} else { \
		size = (stride / ATARI_SIZEOF_FLOAT) * sizeof(GLfloat); \
	} \
	GLfloat *tmp = (GLfloat *)calloc(numPaths, size); \
	if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
	fn.glGetPathMetricsNV(metricQueryMask, numPaths, pathNameType, ppaths, pathBase, size, tmp); \
	Host2AtariFloatArray((size / sizeof(GLfloat)) * numPaths, tmp, metrics); \
	free(tmp)
#else
#define FN_GLGETPATHMETRICSNV(metricQueryMask, numPaths, pathNameType, paths, pathBase, stride, metrics) \
	fn.glGetPathMetricsNV(metricQueryMask, numPaths, pathNameType, HostAddr(paths, const void *), pathBase, stride, HostAddr(metrics, GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETPATHMETRICRANGENV(metricQueryMask, firstPathName, numPaths, stride, metrics) \
	GLsizei size = stride; \
	if (size < 0) { glSetError(GL_INVALID_VALUE); return; } \
	if (size == 0) { \
		GLbitfield mask = metricQueryMask; \
		while (mask) \
		{ \
			if (mask & 1) size += sizeof(GLfloat); \
			mask >>= 1; \
		} \
	} else { \
		size = (stride / ATARI_SIZEOF_FLOAT) * sizeof(GLfloat); \
	} \
	GLfloat *tmp = (GLfloat *)calloc(numPaths, size); \
	if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
	fn.glGetPathMetricRangeNV(metricQueryMask, firstPathName, numPaths, size, tmp); \
	Host2AtariFloatArray((size / sizeof(GLfloat)) * numPaths, tmp, metrics); \
	free(tmp)
#else
#define FN_GLGETPATHMETRICRANGENV(metricQueryMask, firstPathName, numPaths, stride, metrics) \
	fn.glGetPathMetricRangeNV(metricQueryMask, firstPathName, numPaths, stride, HostAddr(metrics, GLfloat *))
#endif

#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETPATHSPACINGNV(pathListMode, numPaths, pathNameType, paths, pathBase, advanceScale, kerningScale, transformType, returnedSpacing) \
	GLsizei size; \
	if (numPaths <= 1) { glSetError(GL_INVALID_VALUE); return; } \
	switch (transformType) { \
		case GL_TRANSLATE_X_NV: size = 1; break; \
		case GL_TRANSLATE_2D_NV: size = 2; break; \
		default: glSetError(GL_INVALID_ENUM); return; \
	} \
	size *= numPaths - 1; \
	pixelBuffer buf(*this); \
	void *ppaths = buf.convertArray(numPaths, pathNameType, (nfmemptr)paths); \
	GLfloat *tmp = (GLfloat *)calloc(size, sizeof(GLfloat)); \
	if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
	fn.glGetPathSpacingNV(pathListMode, numPaths, pathNameType, ppaths, pathBase, advanceScale, kerningScale, transformType, tmp); \
	Host2AtariFloatArray(size, tmp, returnedSpacing)
#else
#define FN_GLGETPATHSPACINGNV(pathListMode, numPaths, pathNameType, paths, pathBase, advanceScale, kerningScale, transformType, returnedSpacing) \
	fn.glGetPathSpacingNV(pathListMode, numPaths, pathNameType, HostAddr(paths, const void *), pathBase, advanceScale, kerningScale, transformType, HostAddr(returnedSpacing, GLfloat *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPOINTALONGPATHNV(path, startSegment, numSegments, distance, x, y, tangentX, tangentY) \
	GLsizei const size = 1; \
	GLfloat xret, yret, tangentXret, tangentYret; \
	GLboolean ret = fn.glPointAlongPathNV(path, startSegment, numSegments, distance, &xret, &yret, &tangentXret, &tangentYret); \
	Host2AtariFloatArray(size, &xret, x); \
	Host2AtariFloatArray(size, &yret, y); \
	Host2AtariFloatArray(size, &tangentXret, tangentX); \
	Host2AtariFloatArray(size, &tangentYret, tangentY); \
	return ret
#else
#define FN_GLPOINTALONGPATHNV(path, startSegment, numSegments, distance, x, y, tangentX, tangentY) \
	return fn.glPointAlongPathNV(path, startSegment, numSegments, distance, HostAddr(x, GLfloat *), HostAddr(y, GLfloat *), HostAddr(tangentX, GLfloat *), HostAddr(tangentY, GLfloat *))
#endif

#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLSTENCILTHENCOVERFILLPATHINSTANCEDNV(numPaths, pathNameType, paths, pathBase, fillMode, mask, coverMode, transformType, transformValues) \
	GLsizei size; \
	GLfloat tmp[16]; \
	switch (transformType) { \
		case GL_NONE: size = 0; break; \
		case GL_TRANSLATE_X_NV: size = 1; break; \
		case GL_TRANSLATE_Y_NV: size = 1; break; \
		case GL_TRANSLATE_2D_NV: size = 2; break; \
		case GL_TRANSLATE_3D_NV: size = 3; break; \
		case GL_AFFINE_2D_NV: size = 6; break; \
		case GL_PROJECTIVE_2D_NV: size = 6; break; \
		case GL_TRANSPOSE_AFFINE_2D_NV: size = 6; break; \
		case GL_TRANSPOSE_PROJECTIVE_2D_NV: size = 6; break; \
		case GL_AFFINE_3D_NV: size = 12; break; \
		case GL_PROJECTIVE_3D_NV: size = 12; break; \
		case GL_TRANSPOSE_AFFINE_3D_NV: size = 12; break; \
		case GL_TRANSPOSE_PROJECTIVE_3D_NV: size = 12; break; \
		default: glSetError(GL_INVALID_ENUM); return; \
	} \
	pixelBuffer buf(*this); \
	void *ppaths = buf.convertArray(numPaths, pathNameType, (nfmemptr)paths); \
	Atari2HostFloatArray(size, transformValues, tmp); \
	fn.glStencilThenCoverFillPathInstancedNV(numPaths, pathNameType, ppaths, pathBase, fillMode, mask, coverMode, transformType, tmp)
#else
#define FN_GLSTENCILTHENCOVERFILLPATHINSTANCEDNV(numPaths, pathNameType, paths, pathBase, fillMode, mask, coverMode, transformType, transformValues) \
	fn.glStencilThenCoverFillPathInstancedNV(numPaths, pathNameType, HostAddr(paths, const void *), pathBase, fillMode, mask, coverMode, transformType, HostAddr(transformValues, const GLfloat *))
#endif

#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLSTENCILTHENCOVERSTROKEPATHINSTANCEDNV(numPaths, pathNameType, paths, pathBase, reference, mask, coverMode, transformType, transformValues) \
	GLsizei size; \
	GLfloat tmp[16]; \
	switch (transformType) { \
		case GL_NONE: size = 0; break; \
		case GL_TRANSLATE_X_NV: size = 1; break; \
		case GL_TRANSLATE_Y_NV: size = 1; break; \
		case GL_TRANSLATE_2D_NV: size = 2; break; \
		case GL_TRANSLATE_3D_NV: size = 3; break; \
		case GL_AFFINE_2D_NV: size = 6; break; \
		case GL_PROJECTIVE_2D_NV: size = 6; break; \
		case GL_TRANSPOSE_AFFINE_2D_NV: size = 6; break; \
		case GL_TRANSPOSE_PROJECTIVE_2D_NV: size = 6; break; \
		case GL_AFFINE_3D_NV: size = 12; break; \
		case GL_PROJECTIVE_3D_NV: size = 12; break; \
		case GL_TRANSPOSE_AFFINE_3D_NV: size = 12; break; \
		case GL_TRANSPOSE_PROJECTIVE_3D_NV: size = 12; break; \
		default: glSetError(GL_INVALID_ENUM); return; \
	} \
	pixelBuffer buf(*this); \
	void *ppaths = buf.convertArray(numPaths, pathNameType, (nfmemptr)paths); \
	Atari2HostFloatArray(size, transformValues, tmp); \
	fn.glStencilThenCoverStrokePathInstancedNV(numPaths, pathNameType, ppaths, pathBase, reference, mask, coverMode, transformType, tmp)
#else
#define FN_GLSTENCILTHENCOVERSTROKEPATHINSTANCEDNV(numPaths, pathNameType, paths, pathBase, reference, mask, coverMode, transformType, transformValues) \
	fn.glStencilThenCoverStrokePathInstancedNV(numPaths, pathNameType, HostAddr(paths, const void *), pathBase, reference, mask, coverMode, transformType, HostAddr(transformValues, const GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLPATHGLYPHINDEXRANGENV(fontTarget, fontName, fontStyle, pathParameterTemplate, emScale, baseAndCount) \
	GLsizei const size = 2; \
	GLuint tmp[size]; \
	/* fontName is ascii string */ \
	char namebuf[safe_strlen(fontName) + 1], *pname; \
	pname = Atari2HostByteArray(sizeof(namebuf), AtariAddr(fontName, const char *), namebuf); \
	GLenum ret = fn.glPathGlyphIndexRangeNV(fontTarget, pname, fontStyle, pathParameterTemplate, emScale, tmp); \
	Host2AtariIntArray(size, tmp, baseAndCount); \
	return ret
#else
#define FN_GLPATHGLYPHINDEXRANGENV(fontTarget, fontName, fontStyle, pathParameterTemplate, emScale, baseAndCount) \
	return fn.glPathGlyphIndexRangeNV(fontTarget, HostAddr(fontName, const void *), fontStyle, pathParameterTemplate, emScale, HostAddr(baseAndCount, GLuint *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLPATHGLYPHINDEXARRAYNV(firstPathName, fontTarget, fontName, fontStyle, firstGlyph, numGlyphs, pathParameterTemplate, emScale) \
	char namebuf[safe_strlen(fontName) + 1], *pname; \
	pname = Atari2HostByteArray(sizeof(namebuf), AtariAddr(fontName, const char *), namebuf); \
	return fn.glPathGlyphIndexArrayNV(firstPathName, fontTarget, pname, fontStyle, firstGlyph, numGlyphs, pathParameterTemplate, emScale)
#else
#define FN_GLPATHGLYPHINDEXARRAYNV(firstPathName, fontTarget, fontName, fontStyle, firstGlyph, numGlyphs, pathParameterTemplate, emScale) \
	return fn.glPathGlyphIndexArrayNV(firstPathName, fontTarget, HostAddr(fontName, const void *), fontStyle, firstGlyph, numGlyphs, pathParameterTemplate, emScale)
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLPATHMEMORYGLYPHINDEXARRAYNV(firstPathName, fontTarget, fontSize, fontData, faceIndex, firstGlyphIndex, numGlyphs, pathParameterTemplate, emScale) \
	char tmp[MAX(fontSize, 0)], *ptmp; \
	ptmp = Atari2HostByteArray(fontSize, AtariAddr(fontData, const char *), tmp); \
	return fn.glPathMemoryGlyphIndexArrayNV(firstPathName, fontTarget, fontSize, ptmp, faceIndex, firstGlyphIndex, numGlyphs, pathParameterTemplate, emScale)
#else
#define FN_GLPATHMEMORYGLYPHINDEXARRAYNV(firstPathName, fontTarget, fontSize, fontData, faceIndex, firstGlyphIndex, numGlyphs, pathParameterTemplate, emScale) \
	return fn.glPathMemoryGlyphIndexArrayNV(firstPathName, fontTarget, fontSize, HostAddr(fontData, const void *), faceIndex, firstGlyphIndex, numGlyphs, pathParameterTemplate, emScale)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPROGRAMPATHFRAGMENTINPUTGENNV(program, location, genMode, components, coeffs) \
	GLsizei const size = components; \
	GLfloat tmp[size], *ptmp; \
	ptmp = Atari2HostFloatArray(size, coeffs, tmp); \
	fn.glProgramPathFragmentInputGenNV(program, location, genMode, components, ptmp)
#else
#define FN_GLPROGRAMPATHFRAGMENTINPUTGENNV(program, location, genMode, components, coeffs) \
	fn.glProgramPathFragmentInputGenNV(program, location, genMode, components, HostAddr(coeffs, const GLfloat *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPATHCOLORGENNV(color, genMode, colorFormat, coeffs) \
	pixelBuffer buf(*this); \
	if (!buf.params(1, 1, 1, GL_FLOAT, colorFormat)) return; \
	GLint const size = nfglGetNumParams(genMode) * buf.ComponentCount(); \
	GLfloat tmp[size], *ptmp; \
	ptmp = Atari2HostFloatArray(size, coeffs, tmp); \
	fn.glPathColorGenNV(color, genMode, colorFormat, ptmp)
#else
#define FN_GLPATHCOLORGENNV(color, genMode, colorFormat, coeffs) \
	fn.glPathColorGenNV(color, genMode, colorFormat, HostAddr(coeffs, const GLfloat *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETPATHCOLORGENFVNV(color, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLfloat tmp[size]; \
	fn.glGetPathColorGenfvNV(color, pname, tmp); \
	Host2AtariFloatArray(size, tmp, params)
#else
#define FN_GLGETPATHCOLORGENFVNV(color, pname, params) \
	fn.glGetPathColorGenfvNV(color, pname, HostAddr(params, GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETPATHCOLORGENIVNV(color, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLint tmp[size]; \
	fn.glGetPathColorGenivNV(color, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETPATHCOLORGENIVNV(color, pname, params) \
	fn.glGetPathColorGenivNV(color, pname, HostAddr(params, GLint *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPATHTEXGENNV(texCoordSet, genMode, components, coeffs) \
	GLint const size = components; \
	GLfloat tmp[size], *ptmp; \
	ptmp = Atari2HostFloatArray(size, coeffs, tmp); \
	fn.glPathTexGenNV(texCoordSet, genMode, components, ptmp)
#else
#define FN_GLPATHTEXGENNV(texCoordSet, genMode, components, coeffs) \
	fn.glPathTexGenNV(texCoordSet, genMode, components, HostAddr(coeffs, const GLfloat *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETPATHTEXGENFVNV(texCoordSet, pname, value) \
	GLint const size = nfglGetNumParams(pname); \
	GLfloat tmp[size]; \
	fn.glGetPathTexGenfvNV(texCoordSet, pname, tmp); \
	Host2AtariFloatArray(size, tmp, value)
#else
#define FN_GLGETPATHTEXGENFVNV(texCoordSet, pname, value) \
	fn.glGetPathTexGenfvNV(texCoordSet, pname, HostAddr(value, GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETPATHTEXGENIVNV(texCoordSet, pname, value) \
	GLint const size = nfglGetNumParams(pname); \
	GLint tmp[size]; \
	fn.glGetPathTexGenivNV(texCoordSet, pname, tmp); \
	Host2AtariIntArray(size, tmp, value)
#else
#define FN_GLGETPATHTEXGENIVNV(texCoordSet, pname, value) \
	fn.glGetPathTexGenivNV(texCoordSet, pname, HostAddr(value, GLint *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLMATRIXLOAD3X2FNV(matrixMode, m) \
	GLint const size = 6; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, m, tmp); \
	fn.glMatrixLoad3x2fNV(matrixMode, tmp)
#else
#define FN_GLMATRIXLOAD3X2FNV(matrixMode, m) \
	fn.glMatrixLoad3x2fNV(matrixMode, HostAddr(m, const GLfloat *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLMATRIXLOAD3X3FNV(matrixMode, m) \
	GLint const size = 9; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, m, tmp); \
	fn.glMatrixLoad3x3fNV(matrixMode, tmp)
#else
#define FN_GLMATRIXLOAD3X3FNV(matrixMode, m) \
	fn.glMatrixLoad3x3fNV(matrixMode, HostAddr(m, const GLfloat *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLMATRIXLOADTRANSPOSE3X3FNV(matrixMode, m) \
	GLint const size = 9; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, m, tmp); \
	fn.glMatrixLoadTranspose3x3fNV(matrixMode, tmp)
#else
#define FN_GLMATRIXLOADTRANSPOSE3X3FNV(matrixMode, m) \
	fn.glMatrixLoadTranspose3x3fNV(matrixMode, HostAddr(m, const GLfloat *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLMATRIXMULT3X2FNV(matrixMode, m) \
	GLint const size = 6; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, m, tmp); \
	fn.glMatrixMult3x2fNV(matrixMode, tmp)
#else
#define FN_GLMATRIXMULT3X2FNV(matrixMode, m) \
	fn.glMatrixMult3x2fNV(matrixMode, HostAddr(m, const GLfloat *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLMATRIXMULT3X3FNV(matrixMode, m) \
	GLint const size = 9; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, m, tmp); \
	fn.glMatrixMult3x3fNV(matrixMode, tmp)
#else
#define FN_GLMATRIXMULT3X3FNV(matrixMode, m) \
	fn.glMatrixMult3x3fNV(matrixMode, HostAddr(m, const GLfloat *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLMATRIXMULTTRANSPOSE3X3FNV(matrixMode, m) \
	GLint const size = 9; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, m, tmp); \
	fn.glMatrixMultTranspose3x3fNV(matrixMode, tmp)
#else
#define FN_GLMATRIXMULTTRANSPOSE3X3FNV(matrixMode, m) \
	fn.glMatrixLoadTranspose3x3fNV(matrixMode, HostAddr(m, const GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETPROGRAMRESOURCEFVNV(program, programInterface, index, propCount, props, bufSize, length, params) \
	GLenum propbuf[MAX(propCount, 0)], *ptmp; \
	GLsizei l = 0; \
	GLfloat parambuf[MAX(bufSize, 0)]; \
	ptmp = Atari2HostIntArray(propCount, props, propbuf); \
	fn.glGetProgramResourcefvNV(program, programInterface, index, propCount, ptmp, bufSize, &l, parambuf); \
	Host2AtariFloatArray(l, parambuf, params); \
	Host2AtariIntArray(1, &l, length)
#else
#define FN_GLGETPROGRAMRESOURCEFVNV(program, programInterface, index, propCount, props, bufSize, length, params) \
	fn.glGetProgramResourcefvNV(program, programInterface, index, propCount, HostAddr(props, const GLenum *), bufSize, HostAddr(length, GLsizei *), HostAddr(params, GLfloat *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_NV_internalformat_sample_query
 */
#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETINTERNALFORMATSAMPLEIVNV(target, internalformat, samples, pname, bufSize, params) \
	GLint parambuf[MAX(bufSize, 0)]; \
	fn.glGetInternalformatSampleivNV(target, internalformat, samples, pname, bufSize, parambuf); \
	Host2AtariIntArray(bufSize, parambuf, params)
#else
#define FN_GLGETINTERNALFORMATSAMPLEIVNV(target, internalformat, samples, pname, bufSize, params) \
	fn.glGetInternalformatSampleivNV(target, internalformat, samples, pname, bufSize, HostAddr(params, GLint *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_AMD_performance_monitor
 */
#if NFOSMESA_NEED_INT_CONV
#define FN_GLGENPERFMONITORSAMD(n, monitors) \
	GLsizei const size = MAX(n, 0); \
	GLuint tmp[size]; \
	fn.glGenPerfMonitorsAMD(n, tmp); \
	Host2AtariIntArray(n, tmp, monitors)
#else
#define FN_GLGENPERFMONITORSAMD(n, monitors) \
	fn.glGenPerfMonitorsAMD(n, HostAddr(monitors, GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLDELETEPERFMONITORSAMD(n, monitors) \
	GLuint tmp[MAX(n, 0)], *ptmp; \
	ptmp = Atari2HostIntArray(n, monitors, tmp); \
	fn.glDeletePerfMonitorsAMD(n, ptmp)
#else
#define FN_GLDELETEPERFMONITORSAMD(n, monitors) \
	fn.glDeletePerfMonitorsAMD(n, HostAddr(monitors, GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETPERFMONITORGROUPSAMD(numGroups, groupsSize, groups) \
	GLsizei const size = MAX(groupsSize, 0); \
	GLuint tmp[size]; \
	GLuint *pgroups; \
	GLint num = 0; \
	if (groups) { \
		pgroups = tmp; \
	} else { \
		pgroups = NULL; \
	} \
	fn.glGetPerfMonitorGroupsAMD(&num, groupsSize, pgroups); \
	Host2AtariIntArray(1, &num, numGroups); \
	Host2AtariIntArray(num, pgroups, groups)
#else
#define FN_GLGETPERFMONITORGROUPSAMD(numGroups, groupsSize, groups) \
	fn.glGetPerfMonitorGroupsAMD(HostAddr(numGroups, GLint *), groupsSize, HostAddr(groups, GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETPERFMONITORCOUNTERSAMD(group, numCounters, maxActiveCounters, counterSize, counters) \
	GLsizei const size = MAX(counterSize, 0); \
	GLuint tmp[size]; \
	GLuint *pcounters; \
	GLint num = 0; \
	GLint max = 0; \
	if (counters) { \
		pcounters = tmp; \
	} else { \
		pcounters = NULL; \
	} \
	fn.glGetPerfMonitorCountersAMD(group, &num, &max, counterSize, pcounters); \
	Host2AtariIntArray(1, &num, numCounters); \
	Host2AtariIntArray(1, &max, maxActiveCounters); \
	Host2AtariIntArray(num, pcounters, counters)
#else
#define FN_GLGETPERFMONITORCOUNTERSAMD(group, numCounters, maxActiveCounters, counterSize, counters) \
	fn.glGetPerfMonitorCountersAMD(group, HostAddr(numCounters, GLint *), HostAddr(maxActiveCounters, GLint *), counterSize, HostAddr(counters, GLuint *))
#endif

#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_INT_CONV
#define FN_GLGETPERFMONITORGROUPSTRINGAMD(group, bufSize, length, groupString) \
	GLsizei l = 0; \
	GLchar stringbuf[MAX(bufSize, 0)]; \
	fn.glGetPerfMonitorGroupStringAMD(group, bufSize, &l, stringbuf); \
	Host2AtariByteArray(MIN(l + 1, bufSize), stringbuf, groupString); \
	Host2AtariIntArray(1, &l, length)
#else
#define FN_GLGETPERFMONITORGROUPSTRINGAMD(group, bufSize, length, groupString) \
	fn.glGetPerfMonitorGroupStringAMD(group, bufSize, HostAddr(length, GLsizei *), HostAddr(groupString, GLchar *))
#endif

#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_INT_CONV
#define FN_GLGETPERFMONITORCOUNTERSTRINGAMD(group, counter, bufSize, length, counterString) \
	GLsizei l = 0; \
	GLchar stringbuf[MAX(bufSize, 0)]; \
	fn.glGetPerfMonitorCounterStringAMD(group, counter, bufSize, &l, stringbuf); \
	Host2AtariByteArray(MIN(l + 1, bufSize), stringbuf, counterString); \
	Host2AtariIntArray(1, &l, length)
#else
#define FN_GLGETPERFMONITORCOUNTERSTRINGAMD(group, counter, bufSize, length, counterString) \
	fn.glGetPerfMonitorCounterStringAMD(group, counter, bufSize, HostAddr(length, GLsizei *), HostAddr(counterString, GLchar *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETPERFMONITORCOUNTERINFOAMD(group, counter, pname, data) \
	switch (pname) { \
	case GL_COUNTER_TYPE_AMD: \
		{ \
			GLint const size = 1; \
			GLenum tmp[size]; \
			fn.glGetPerfMonitorCounterInfoAMD(group, counter, pname, tmp); \
			Host2AtariIntArray(size, tmp, AtariAddr(data, Uint32 *)); \
		} \
		break; \
	case GL_COUNTER_RANGE_AMD: \
		{ \
			GLenum type = 0; \
			GLint const size = 2; \
			fn.glGetPerfMonitorCounterInfoAMD(group, counter, GL_COUNTER_TYPE_AMD, &type); \
			switch (type) { \
			case GL_UNSIGNED_INT: \
				{ \
					GLuint tmp[size]; \
					fn.glGetPerfMonitorCounterInfoAMD(group, counter, pname, tmp); \
					Host2AtariIntArray(size, tmp, AtariAddr(data, Uint32 *)); \
				} \
				break; \
			case GL_UNSIGNED_INT64_AMD: \
				{ \
					GLuint64 tmp[size]; \
					fn.glGetPerfMonitorCounterInfoAMD(group, counter, pname, tmp); \
					Host2AtariInt64Array(size, tmp, AtariAddr(data, Uint64 *)); \
				} \
				break; \
			case GL_FLOAT: \
			case GL_PERCENTAGE_AMD: \
				{ \
					GLfloat tmp[size]; \
					fn.glGetPerfMonitorCounterInfoAMD(group, counter, pname, tmp); \
					Host2AtariFloatArray(size, tmp, AtariAddr(data, GLfloat *)); \
				} \
				break; \
			default: \
				glSetError(GL_INVALID_OPERATION); \
				break; \
			} \
		} \
		break; \
	default: \
		glSetError(GL_INVALID_ENUM); \
		break; \
	}
#else
#define FN_GLGETPERFMONITORCOUNTERINFOAMD(group, counter, pname, data) \
	fn.glGetPerfMonitorCounterInfoAMD(group, counter, pname, HostAddr(data, void *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLSELECTPERFMONITORCOUNTERSAMD(monitor, enable, group, numCounters, counterList) \
	GLuint tmp[MAX(numCounters, 0)], *ptmp; \
	ptmp = Atari2HostIntArray(numCounters, counterList, tmp); \
	fn.glSelectPerfMonitorCountersAMD(monitor, enable, group, numCounters, ptmp)
#else
#define FN_GLSELECTPERFMONITORCOUNTERSAMD(monitor, enable, group, numCounters, counterList) \
	fn.glSelectPerfMonitorCountersAMD(monitor, enable, group, numCounters, HostAddr(counterList, GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETPERFMONITORCOUNTERDATAAMD(monitor, pname, dataSize, data, bytesWritten) \
	GLint written = 0; \
	switch (pname) { \
	case GL_PERFMON_RESULT_AVAILABLE_AMD: \
	case GL_PERFMON_RESULT_SIZE_AMD: \
		{ \
			GLint const size = 1; \
			GLuint tmp[size]; \
			fn.glGetPerfMonitorCounterDataAMD(monitor, pname, dataSize, tmp, &written); \
			Host2AtariIntArray(size, tmp, data); \
		} \
		break; \
	case GL_PERFMON_RESULT_AMD: \
		{ \
			GLint const size = dataSize / sizeof(GLuint); \
			if (size <= 0) return; \
			GLuint tmp[size]; \
			fn.glGetPerfMonitorCounterDataAMD(monitor, pname, dataSize, tmp, &written); \
			if (written > 0 && data) Host2AtariIntArray(written / sizeof(GLuint), tmp, data); \
		} \
		break; \
	} \
	Host2AtariIntArray(1, &written, bytesWritten)
#else
#define FN_GLGETPERFMONITORCOUNTERDATAAMD(monitor, pname, dataSize, data, bytesWritten) \
	fn.glGetPerfMonitorCounterDataAMD(monitor, pname, dataSize, HostAddr(data, GLuint *), HostAddr(bytesWritten, GLint *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_NV_half_float
 */

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEX2HVNV(v) \
	GLint const size = 2; \
	GLhalfNV tmp[size]; \
	Atari2HostShortArray(size, v, tmp); \
	fn.glVertex2hvNV(tmp)
#else
#define FN_GLVERTEX2HVNV(v) \
	fn.glVertex2hvNV(HostAddr(v, const GLhalfNV *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEX3HVNV(v) \
	GLint const size = 4; \
	GLhalfNV tmp[size]; \
	Atari2HostShortArray(size, v, tmp); \
	fn.glVertex3hvNV(tmp)
#else
#define FN_GLVERTEX3HVNV(v) \
	fn.glVertex3hvNV(HostAddr(v, const GLhalfNV *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEX4HVNV(v) \
	GLint const size = 4; \
	GLhalfNV tmp[size]; \
	Atari2HostShortArray(size, v, tmp); \
	fn.glVertex4hvNV(tmp)
#else
#define FN_GLVERTEX4HVNV(v) \
	fn.glVertex4hvNV(HostAddr(v, const GLhalfNV *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLNORMAL3HVNV(v) \
	int const size = 3; \
	GLhalfNV tmp[size]; \
	Atari2HostShortArray(size, v, tmp); \
	fn.glNormal3hvNV(tmp)
#else
#define FN_GLNORMAL3HVNV(v) \
	fn.glNormal3hvNV(HostAddr(v, const GLhalfNV *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLCOLOR3HVNV(v) \
	GLhalfNV tmp[3]; \
	Atari2HostShortArray(3, v, tmp); \
	fn.glColor3hvNV(tmp)
#else
#define FN_GLCOLOR3HVNV(v) \
	fn.glColor3hvNV(HostAddr(v, const GLhalfNV *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLCOLOR4HVNV(v) \
	GLhalfNV tmp[4]; \
	Atari2HostShortArray(4, v, tmp); \
	fn.glColor4hvNV(tmp)
#else
#define FN_GLCOLOR4HVNV(v) \
	fn.glColor4hvNV(HostAddr(v, const GLhalfNV *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLTEXCOORD1HVNV(v) \
	GLhalfNV tmp[1]; \
	Atari2HostShortArray(1, v, tmp); \
	fn.glTexCoord1hvNV(tmp)
#else
#define FN_GLTEXCOORD1HVNV(v) \
	fn.glTexCoord1hvNV(HostAddr(v, const GLhalfNV *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLTEXCOORD2HVNV(v) \
	GLhalfNV tmp[2]; \
	Atari2HostShortArray(2, v, tmp); \
	fn.glTexCoord2hvNV(tmp)
#else
#define FN_GLTEXCOORD2HVNV(v) \
	fn.glTexCoord2hvNV(HostAddr(v, const GLhalfNV *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLTEXCOORD3HVNV(v) \
	GLhalfNV tmp[3]; \
	Atari2HostShortArray(3, v, tmp); \
	fn.glTexCoord3hvNV(tmp)
#else
#define FN_GLTEXCOORD3HVNV(v) \
	fn.glTexCoord3hvNV(HostAddr(v, const GLhalfNV *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLTEXCOORD4HVNV(v) \
	GLhalfNV tmp[4]; \
	Atari2HostShortArray(4, v, tmp); \
	fn.glTexCoord4hvNV(tmp)
#else
#define FN_GLTEXCOORD4HVNV(v) \
	fn.glTexCoord4hvNV(HostAddr(v, const GLhalfNV *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLMULTITEXCOORD1HVNV(target, v) \
	int const size = 1; \
	GLhalfNV tmp[size]; \
	Atari2HostShortArray(size, v, tmp); \
	fn.glMultiTexCoord1hvNV(target, tmp)
#else
#define FN_GLMULTITEXCOORD1HVNV(target, v) \
	fn.glMultiTexCoord1hvNV(target, HostAddr(v, const GLhalfNV *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLMULTITEXCOORD2HVNV(target, v) \
	int const size = 2; \
	GLhalfNV tmp[size]; \
	Atari2HostShortArray(size, v, tmp); \
	fn.glMultiTexCoord2hvNV(target, tmp)
#else
#define FN_GLMULTITEXCOORD2HVNV(target, v) \
	fn.glMultiTexCoord2hvNV(target, HostAddr(v, const GLhalfNV *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLMULTITEXCOORD3HVNV(target, v) \
	int const size = 3; \
	GLhalfNV tmp[size]; \
	Atari2HostShortArray(3, v, tmp); \
	fn.glMultiTexCoord3hvNV(target, tmp)
#else
#define FN_GLMULTITEXCOORD3HVNV(target, v) \
	fn.glMultiTexCoord3hvNV(target, HostAddr(v, const GLhalfNV *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLMULTITEXCOORD4HVNV(target, v) \
	int const size = 4; \
	GLhalfNV tmp[size]; \
	Atari2HostShortArray(size, v, tmp); \
	fn.glMultiTexCoord4hvNV(target, tmp)
#else
#define FN_GLMULTITEXCOORD4HVNV(target, v) \
	fn.glMultiTexCoord4hvNV(target, HostAddr(v, const GLhalfNV *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLFOGCOORDHVNV(fog) \
	GLhalfNV tmp[1]; \
	Atari2HostShortArray(1, fog, tmp); \
	fn.glFogCoordhvNV(tmp)
#else
#define FN_GLFOGCOORDHVNV(fog) \
	fn.glFogCoordhvNV(HostAddr(fog, const GLhalfNV *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLSECONDARYCOLOR3HVNV(v) \
	GLhalfNV tmp[3]; \
	Atari2HostShortArray(3, v, tmp); \
	fn.glSecondaryColor3hvNV(tmp)
#else
#define FN_GLSECONDARYCOLOR3HVNV(v) \
	fn.glSecondaryColor3hvNV(HostAddr(v, const GLhalfNV *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXWEIGHTHVNV(weight) \
	GLhalfNV tmp[1]; \
	Atari2HostShortArray(1, weight, tmp); \
	fn.glVertexWeighthvNV(tmp)
#else
#define FN_GLVERTEXWEIGHTHVNV(weight) \
	fn.glVertexWeighthvNV(HostAddr(weight, const GLhalfNV *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIB1HVNV(index, v) \
	GLint const size = 1; \
	GLhalfNV tmp[size]; \
	Atari2HostShortArray(size, v, tmp); \
	fn.glVertexAttrib1hvNV(index, tmp)
#else
#define FN_GLVERTEXATTRIB1HVNV(index, v) \
	fn.glVertexAttrib1hvNV(index, HostAddr(v, const GLhalfNV *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIB2HVNV(index, v) \
	GLint const size = 2; \
	GLhalfNV tmp[size]; \
	Atari2HostShortArray(size, v, tmp); \
	fn.glVertexAttrib2hvNV(index, tmp)
#else
#define FN_GLVERTEXATTRIB2HVNV(index, v) \
	fn.glVertexAttrib2hvNV(index, HostAddr(v, const GLhalfNV *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIB3HVNV(index, v) \
	GLint const size = 3; \
	GLhalfNV tmp[size]; \
	Atari2HostShortArray(size, v, tmp); \
	fn.glVertexAttrib3hvNV(index, tmp)
#else
#define FN_GLVERTEXATTRIB3HVNV(index, v) \
	fn.glVertexAttrib3hvNV(index, HostAddr(v, const GLhalfNV *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIB4HVNV(index, v) \
	GLint const size = 4; \
	GLhalfNV tmp[size]; \
	Atari2HostShortArray(size, v, tmp); \
	fn.glVertexAttrib4hvNV(index, tmp)
#else
#define FN_GLVERTEXATTRIB4HVNV(index, v) \
	fn.glVertexAttrib4hvNV(index, HostAddr(v, const GLhalfNV *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIBS1HVNV(index, n, v) \
	GLhalfNV tmp[1]; \
	Atari2HostShortArray(1, v, tmp); \
	fn.glVertexAttribs1hvNV(index, n, tmp)
#else
#define FN_GLVERTEXATTRIBS1HVNV(index, n, v) \
	fn.glVertexAttribs1hvNV(index, n, HostAddr(v, const GLhalfNV *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIBS2HVNV(index, n, v) \
	GLhalfNV tmp[2]; \
	Atari2HostShortArray(2, v, tmp); \
	fn.glVertexAttribs2hvNV(index, n, tmp)
#else
#define FN_GLVERTEXATTRIBS2HVNV(index, n, v) \
	fn.glVertexAttribs2hvNV(index, n, HostAddr(v, const GLhalfNV *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIBS3HVNV(index, n, v) \
	GLhalfNV tmp[3]; \
	Atari2HostShortArray(3, v, tmp); \
	fn.glVertexAttribs3hvNV(index, n, tmp)
#else
#define FN_GLVERTEXATTRIBS3HVNV(index, n, v) \
	fn.glVertexAttribs3hvNV(index, n, HostAddr(v, const GLhalfNV *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIBS4HVNV(index, n, v) \
	GLhalfNV tmp[4]; \
	Atari2HostShortArray(4, v, tmp); \
	fn.glVertexAttribs4hvNV(index, n, tmp)
#else
#define FN_GLVERTEXATTRIBS4HVNV(index, n, v) \
	fn.glVertexAttribs4hvNV(index, n, HostAddr(v, const GLhalfNV *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_OES_single_precision
 */

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLCLIPPLANEFOES(plane, equation) \
	GLfloat tmp[4]; \
	Atari2HostFloatArray(4, equation, tmp); \
	fn.glClipPlanefOES(plane, tmp)
#else
#define FN_GLCLIPPLANEFOES(plane, equation) \
	fn.glClipPlanefOES(plane, HostAddr(equation, const GLfloat *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETCLIPPLANEFOES(plane, equation) \
	GLfloat tmp[4]; \
	fn.glGetClipPlanefOES(plane, tmp); \
	Host2AtariFloatArray(4, tmp, equation)
#else
#define FN_GLGETCLIPPLANEFOES(plane, equation) \
	fn.glGetClipPlanefOES(plane, HostAddr(equation, GLfloat *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_OES_fixed_point
 */

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETFIXEDVOES(pname, params) \
	int const n = nfglGetNumParams(pname); \
	GLfixed tmp[MAX(n, 16)]; \
	fn.glGetFixedvOES(pname, tmp); \
	Host2AtariIntArray(n, tmp, params)
#else
#define FN_GLGETFIXEDVOES(pname, params) \
	fn.glGetFixedvOES(pname, HostAddr(params, GLfixed *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETPIXELMAPXV(map, size, values) \
	if (size <= 0) return; \
	GLfixed tmp[size]; \
	fn.glGetPixelMapxv(map, size, tmp); \
	Host2AtariIntArray(size, tmp, values)
#else
#define FN_GLGETPIXELMAPXV(map, size, values) \
	fn.glGetPixelMapxv(map, size, HostAddr(values, GLfixed *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLTEXENVXVOES(target, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLfixed tmp[MAX(size, 16)], *ptmp; \
	ptmp = Atari2HostIntArray(size, params, tmp); \
	fn.glTexEnvxvOES(target, pname, ptmp)
#else
#define FN_GLTEXENVXVOES(target, pname, params) \
	fn.glTexEnvxvOES(target, pname, HostAddr(params, const GLfixed *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETTEXENVXVOES(target, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLfixed tmp[MAX(size, 16)]; \
	fn.glGetTexEnvxvOES(target, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETTEXENVXVOES(target, pname, params) \
	fn.glGetTexEnvxvOES(target, pname, HostAddr(params, GLfixed *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLFEEDBACKBUFFERXOES(size, type, buffer) \
	contexts[cur_context].feedback_buffer_atari = (nfmemptr)buffer; \
	free(contexts[cur_context].feedback_buffer_host); \
	contexts[cur_context].feedback_buffer_host = malloc(size * sizeof(GLfixed)); \
	if (!contexts[cur_context].feedback_buffer_host) { glSetError(GL_OUT_OF_MEMORY); return; } \
	contexts[cur_context].feedback_buffer_type = GL_FIXED; \
	fn.glFeedbackBufferxOES(size, type, (GLfixed *)contexts[cur_context].feedback_buffer_host)
#else
#define FN_GLFEEDBACKBUFFERXOES(size, type, buffer) \
	fn.glFeedbackBufferxOES(size, type, HostAddr(buffer, GLfixed *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETCLIPPLANEXOES(plane, equation) \
	GLfixed tmp[4]; \
	fn.glGetClipPlanexOES(plane, tmp); \
	Host2AtariIntArray(4, tmp, equation)
#else
#define FN_GLGETCLIPPLANEXOES(plane, equation) \
	fn.glGetClipPlanexOES(plane, HostAddr(equation, GLfixed *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETLIGHTXOES(light, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLfixed tmp[MAX(size, 16)]; \
	fn.glGetLightxOES(light, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETLIGHTXOES(light, pname, params) \
	fn.glGetLightxOES(light, pname, HostAddr(params, GLfixed *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLCLIPPLANEXOES(plane, equation) \
	GLfixed tmp[4]; \
	Atari2HostIntArray(4, equation, tmp); \
	fn.glClipPlanexOES(plane, tmp)
#else
#define FN_GLCLIPPLANEXOES(plane, equation) \
	fn.glClipPlanexOES(plane, HostAddr(equation, const GLfixed *))
#endif

/* glGetMaterialxOES??? should be *params */

#if NFOSMESA_NEED_INT_CONV
#define FN_GLCOLOR3XVOES(components) \
	GLint tmp[3]; \
	Atari2HostIntArray(3, components, tmp); \
	fn.glColor3xvOES(tmp)
#else
#define FN_GLCOLOR3XVOES(components) \
	fn.glColor3xvOES(HostAddr(components, const GLfixed *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLCOLOR4XVOES(components) \
	GLfixed tmp[4]; \
	Atari2HostIntArray(4, components, tmp); \
	fn.glColor4xvOES(tmp)
#else
#define FN_GLCOLOR4XVOES(components) \
	fn.glColor4xvOES(HostAddr(components, const GLfixed *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETMAPXVOES(target, query, v) \
	GLint order[2] = { 0, 0 }; \
	GLint size; \
	GLfixed *tmp; \
	GLfixed tmpbuf[4]; \
	switch (target) { \
		case GL_MAP1_INDEX: \
		case GL_MAP1_TEXTURE_COORD_1: \
		case GL_MAP1_TEXTURE_COORD_2: \
		case GL_MAP1_VERTEX_3: \
		case GL_MAP1_NORMAL: \
		case GL_MAP1_TEXTURE_COORD_3: \
		case GL_MAP1_VERTEX_4: \
		case GL_MAP1_COLOR_4: \
		case GL_MAP1_TEXTURE_COORD_4: \
			switch (query) { \
			case GL_COEFF: \
				size = __glGetMap_Evalk(target); \
				fn.glGetMapiv(target, GL_ORDER, order); \
				if (order[0] <= 0) return; \
				size *= order[0]; \
				tmp = (GLfixed *)malloc(size * sizeof(*tmp)); \
				if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
				fn.glGetMapxvOES(target, query, tmp); \
				Host2AtariIntArray(size, tmp, v); \
				free(tmp); \
				break; \
			case GL_DOMAIN: \
				fn.glGetMapxvOES(target, query, tmpbuf); \
				size = 2; \
				Host2AtariIntArray(size, tmpbuf, v); \
				break; \
			case GL_ORDER: \
				fn.glGetMapxvOES(target, query, tmpbuf); \
				size = 1; \
				Host2AtariIntArray(size, tmpbuf, v); \
				break; \
			} \
			break; \
		case GL_MAP2_INDEX: \
		case GL_MAP2_TEXTURE_COORD_1: \
		case GL_MAP2_TEXTURE_COORD_2: \
		case GL_MAP2_VERTEX_3: \
		case GL_MAP2_NORMAL: \
		case GL_MAP2_TEXTURE_COORD_3: \
		case GL_MAP2_VERTEX_4: \
		case GL_MAP2_COLOR_4: \
		case GL_MAP2_TEXTURE_COORD_4: \
			switch (query) { \
			case GL_COEFF: \
				size = __glGetMap_Evalk(target); \
				fn.glGetMapiv(target, GL_ORDER, order); \
				if (order[0] <= 0 || order[1] <= 0) return; \
				size *= order[0] * order[1]; \
				tmp = (GLfixed *)malloc(size * sizeof(*tmp)); \
				if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
				fn.glGetMapxvOES(target, query, tmp); \
				Host2AtariIntArray(size, tmp, v); \
				free(tmp); \
				break; \
			case GL_DOMAIN: \
				fn.glGetMapxvOES(target, query, tmpbuf); \
				size = 4; \
				Host2AtariIntArray(size, tmpbuf, v); \
				break; \
			case GL_ORDER: \
				fn.glGetMapxvOES(target, query, tmpbuf); \
				size = 2; \
				Host2AtariIntArray(size, tmpbuf, v); \
				break; \
			} \
			break; \
	}
#else
#define FN_GLGETMAPXVOES(target, query, v) \
	fn.glGetMapxvOES(target, query, HostAddr(v, GLfixed *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLCONVOLUTIONPARAMETERXVOES(target, pname, params) \
	int const size = nfglGetNumParams(pname); \
	GLfixed tmp[MAX(size, 16)], *ptmp; \
	ptmp = Atari2HostIntArray(size, params, tmp); \
	fn.glConvolutionParameterxvOES(target, pname, ptmp)
#else
#define FN_GLCONVOLUTIONPARAMETERXVOES(target, pname, params) \
	fn.glConvolutionParameterxvOES(target, pname, HostAddr(params, const GLfixed *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLEVALCOORD1XVOES(coords) \
	GLfixed tmp[1]; \
	Atari2HostIntArray(1, coords, tmp); \
	fn.glEvalCoord1xvOES(tmp)
#else
#define FN_GLEVALCOORD1XVOES(coords) \
	fn.glEvalCoord1xvOES(HostAddr(coords, const GLfixed *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLEVALCOORD2XVOES(coords) \
	GLfixed tmp[2]; \
	Atari2HostIntArray(2, coords, tmp); \
	fn.glEvalCoord2xvOES(tmp)
#else
#define FN_GLEVALCOORD2XVOES(coords) \
	fn.glEvalCoord2xvOES(HostAddr(coords, const GLfixed *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLFOGXVOES(pname, param) \
	GLfixed tmp[1]; \
	Atari2HostIntArray(1, param, tmp); \
	fn.glFogxvOES(pname, tmp)
#else
#define FN_GLFOGXVOES(pname, param) \
	fn.glFogxvOES(pname, HostAddr(param, const GLfixed *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETCONVOLUTIONPARAMETERXVOES(target, pname, params) \
	int const size = nfglGetNumParams(pname); \
	GLfixed tmp[MAX(size, 16)]; \
	fn.glGetConvolutionParameterxvOES(target, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETCONVOLUTIONPARAMETERXVOES(target, pname, params) \
	fn.glGetConvolutionParameterxvOES(target, pname, HostAddr(params, GLfixed *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETHISTOGRAMPARAMETERXVOES(target, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLfixed tmp[MAX(size, 16)]; \
	fn.glGetHistogramParameterxvOES(target, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETHISTOGRAMPARAMETERXVOES(target, pname, params) \
	fn.glGetHistogramParameterxvOES(target, pname, HostAddr(params, GLfixed *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLTEXGENXVOES(coord, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLfixed tmp[MAX(size, 16)]; \
	Atari2HostIntArray(size, params, tmp); \
	fn.glTexGenxvOES(coord, pname, tmp)
#else
#define FN_GLTEXGENXVOES(coord, pname, params)	\
	fn.glTexGenxvOES(coord, pname, HostAddr(params, const GLfixed *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETTEXGENXVOES(target, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLfixed tmp[MAX(size, 16)]; \
	fn.glGetTexGenxvOES(target, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETTEXGENXVOES(target, pname, params) \
	fn.glGetTexGenxvOES(target, pname, HostAddr(params, GLfixed *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETTEXLEVELPARAMETERXVOES(target, level, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLfixed tmp[MAX(size, 16)]; \
	fn.glGetTexLevelParameterxvOES(target, level, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETTEXLEVELPARAMETERXVOES(target, level, pname, params) \
	fn.glGetTexLevelParameterxvOES(target, level, pname, HostAddr(params, GLfixed *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLTEXPARAMETERXVOES(target, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLfixed tmp[MAX(size, 16)], *ptmp; \
	ptmp = Atari2HostIntArray(size, params, tmp); \
	fn.glTexParameterxvOES(target, pname, ptmp)
#else
#define FN_GLTEXPARAMETERXVOES(target, pname, params) \
	fn.glTexParameterxvOES(target, pname, HostAddr(params, const GLfixed *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETTEXPARAMETERXVOES(target, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLfixed tmp[MAX(size, 16)]; \
	fn.glGetTexParameterxvOES(target, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETTEXPARAMETERXVOES(target, pname, params) \
	fn.glGetTexParameterxvOES(target, pname, HostAddr(params, GLfixed *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLINDEXXVOES(c) \
	GLfixed tmp[1]; \
	Atari2HostIntArray(1, c, tmp); \
	fn.glIndexxvOES(tmp)
#else
#define FN_GLINDEXXVOES(c) \
	fn.glIndexxvOES(HostAddr(c, const GLfixed *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLLIGHTMODELXVOES(pname, params) \
	int const size = nfglGetNumParams(pname); \
	GLfixed tmp[size], *ptmp; \
	ptmp = Atari2HostIntArray(size, params, tmp); \
	fn.glLightModeliv(pname, ptmp)
#else
#define FN_GLLIGHTMODELXVOES(pname, params) \
	fn.glLightModelxvOES(pname, HostAddr(params, const GLfixed *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLLIGHTXVOES(light, pname, params) \
	int const size = nfglGetNumParams(pname); \
	GLfixed tmp[size], *ptmp; \
	ptmp = Atari2HostIntArray(size, params, tmp); \
	fn.glLightxvOES(light, pname, ptmp)
#else
#define FN_GLLIGHTXVOES(light, pname, params) \
	fn.glLightxvOES(light, pname, HostAddr(params, const GLfixed *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLLOADMATRIXXOES(m)	\
	int const size = 16; \
	GLfixed tmp[size]; \
	Atari2HostIntArray(size, m, tmp); \
	fn.glLoadMatrixxOES(tmp)
#else
#define FN_GLLOADMATRIXXOES(m) \
	fn.glLoadMatrixxOES(HostAddr(m, const GLfixed *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLLOADTRANSPOSEMATRIXXOES(m)	\
	int const size = 16; \
	GLfixed tmp[size]; \
	Atari2HostIntArray(size, m, tmp); \
	fn.glLoadTransposeMatrixxOES(tmp)
#else
#define FN_GLLOADTRANSPOSEMATRIXXOES(m) \
	fn.glLoadTransposeMatrixxOES(HostAddr(m, const GLfixed *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLMATERIALXVOES(face, pname, params) \
	int const size = nfglGetNumParams(pname); \
	GLfixed tmp[size], *ptmp; \
	ptmp = Atari2HostIntArray(size, params, tmp); \
	fn.glMaterialxvOES(face, pname, ptmp)
#else
#define FN_GLMATERIALXVOES(face, pname, params) \
	fn.glMaterialxvOES(face, pname, HostAddr(params, const GLfixed *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLMULTMATRIXXOES(m)	\
	int const size = 16; \
	GLfixed tmp[size]; \
	Atari2HostIntArray(size, m, tmp); \
	fn.glMultMatrixxOES(tmp)
#else
#define FN_GLMULTMATRIXXOES(m) \
	fn.glMultMatrixxOES(HostAddr(m, const GLfixed *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLMULTTRANSPOSEMATRIXXOES(m)	\
	int const size = 16; \
	GLfixed tmp[size]; \
	Atari2HostIntArray(size, m, tmp); \
	fn.glMultTransposeMatrixxOES(tmp)
#else
#define FN_GLMULTTRANSPOSEMATRIXXOES(m) \
	fn.glMultTransposeMatrixxOES(HostAddr(m, const GLfixed *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLMULTITEXCOORD1XVOES(target, v) \
	int const size = 1; \
	GLfixed tmp[size]; \
	Atari2HostIntArray(size, v, tmp); \
	fn.glMultiTexCoord1xvOES(target, tmp)
#else
#define FN_GLMULTITEXCOORD1XVOES(target, v) \
	fn.glMultiTexCoord1xvOES(target, HostAddr(v, const GLfixed *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLMULTITEXCOORD2XVOES(target, v) \
	int const size = 2; \
	GLfixed tmp[size]; \
	Atari2HostIntArray(size, v, tmp); \
	fn.glMultiTexCoord2xvOES(target, tmp)
#else
#define FN_GLMULTITEXCOORD2XVOES(target, v) \
	fn.glMultiTexCoord2xvOES(target, HostAddr(v, const GLfixed *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLMULTITEXCOORD3XVOES(target, v) \
	int const size = 3; \
	GLfixed tmp[size]; \
	Atari2HostIntArray(size, v, tmp); \
	fn.glMultiTexCoord3xvOES(target, tmp)
#else
#define FN_GLMULTITEXCOORD3XVOES(target, v) \
	fn.glMultiTexCoord3xvOES(target, HostAddr(v, const GLfixed *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLMULTITEXCOORD4XVOES(target, v) \
	int const size = 4; \
	GLfixed tmp[size]; \
	Atari2HostIntArray(size, v, tmp); \
	fn.glMultiTexCoord4xvOES(target, tmp)
#else
#define FN_GLMULTITEXCOORD4XVOES(target, v) \
	fn.glMultiTexCoord4xvOES(target, HostAddr(v, const GLfixed *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLNORMAL3XVOES(v) \
	int const size = 3; \
	GLfixed tmp[size]; \
	Atari2HostIntArray(size, v, tmp); \
	fn.glNormal3xvOES(tmp)
#else
#define FN_GLNORMAL3XVOES(v) \
	fn.glNormal3xvOES(HostAddr(v, const GLfixed *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLPIXELMAPX(map, mapsize, values) \
	GLfixed tmp[MAX(mapsize, 0)], *ptmp; \
	ptmp = Atari2HostIntArray(mapsize, values, tmp); \
	fn.glPixelMapx(map, mapsize, ptmp)
#else
#define FN_GLPIXELMAPX(map, mapsize, values) \
	fn.glPixelMapx(map, mapsize, HostAddr(values, const GLfixed *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLPOINTPARAMETERXVOES(pname, params) \
	GLfixed tmp[1]; \
	Atari2HostIntArray(1, params, tmp); \
	fn.glPointParameterxvOES(pname, tmp)
#else
#define FN_GLPOINTPARAMETERXVOES(pname, params) \
	fn.glPointParameterxvOES(pname, HostAddr(params, const GLfixed *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLPRIORITIZETEXTURESXOES(n, textures, priorities) \
	GLuint tmp[MAX(n, 0)], *ptmp; \
	GLfixed tmp2[MAX(n, 0)], *ptmp2; \
	ptmp = Atari2HostIntArray(n, textures, tmp); \
	ptmp2 = Atari2HostIntArray(n, priorities, tmp2); \
	fn.glPrioritizeTexturesxOES(n, ptmp, ptmp2)
#else
#define FN_GLPRIORITIZETEXTURESXOES(n, textures, priorities) \
	fn.glPrioritizeTexturesxOES(n, HostAddr(textures, const GLuint *), HostAddr(priorities, const GLfixed *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLRASTERPOS2XVOES(v) \
	GLint const size = 2; \
	GLfixed tmp[size]; \
	Atari2HostIntArray(size, v, tmp); \
	fn.glRasterPos2xvOES(tmp)
#else
#define FN_GLRASTERPOS2XVOES(v) \
	fn.glRasterPos2xvOES(HostAddr(v, const GLfixed *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLRASTERPOS3XVOES(v) \
	GLint const size = 3; \
	GLfixed tmp[size]; \
	Atari2HostIntArray(size, v, tmp); \
	fn.glRasterPos3xvOES(tmp)
#else
#define FN_GLRASTERPOS3XVOES(v) \
	fn.glRasterPos3xvOES(HostAddr(v, const GLfixed *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLRASTERPOS4XVOES(v) \
	GLint const size = 3; \
	GLfixed tmp[size]; \
	Atari2HostIntArray(size, v, tmp); \
	fn.glRasterPos4xvOES(tmp)
#else
#define FN_GLRASTERPOS4XVOES(v) \
	fn.glRasterPos4xvOES(HostAddr(v, const GLfixed *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLRECTXVOES(v1, v2) \
	GLfixed tmp1[4]; \
	GLfixed tmp2[4]; \
	Atari2HostIntArray(4, v1, tmp1); \
	Atari2HostIntArray(4, v2, tmp2); \
	fn.glRectxvOES(tmp1, tmp2)
#else
#define FN_GLRECTXVOES(v1, v2) \
	fn.glRectxvOES(HostAddr(v1, const GLfixed *), HostAddr(v2, const GLfixed *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLTEXCOORD1XVOES(coords) \
	GLint const size = 1; \
	GLfixed tmp[size]; \
	Atari2HostIntArray(size, coords, tmp); \
	fn.glTexCoord1xvOES(tmp)
#else
#define FN_GLTEXCOORD1XVOES(coords) \
	fn.glTexCoord1xvOES(HostAddr(coords, const GLfixed *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLTEXCOORD2XVOES(coords) \
	GLint const size = 2; \
	GLfixed tmp[size]; \
	Atari2HostIntArray(size, coords, tmp); \
	fn.glTexCoord2xvOES(tmp)
#else
#define FN_GLTEXCOORD2XVOES(coords) \
	fn.glTexCoord2xvOES(HostAddr(coords, const GLfixed *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLTEXCOORD3XVOES(coords) \
	GLint const size = 3; \
	GLfixed tmp[size]; \
	Atari2HostIntArray(size, coords, tmp); \
	fn.glTexCoord3xvOES(tmp)
#else
#define FN_GLTEXCOORD3XVOES(coords) \
	fn.glTexCoord3xvOES(HostAddr(coords, const GLfixed *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLTEXCOORD4XVOES(coords) \
	GLint const size = 4; \
	GLfixed tmp[size]; \
	Atari2HostIntArray(size, coords, tmp); \
	fn.glTexCoord4xvOES(tmp)
#else
#define FN_GLTEXCOORD4XVOES(coords) \
	fn.glTexCoord4xvOES(HostAddr(coords, const GLfixed *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEX2XVOES(v) \
	GLint const size = 2; \
	GLfixed tmp[size]; \
	Atari2HostIntArray(size, v, tmp); \
	fn.glVertex2xvOES(tmp)
#else
#define FN_GLVERTEX2XVOES(v) \
	fn.glVertex2xvOES(HostAddr(v, const GLfixed *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEX3XVOES(v) \
	GLint const size = 3; \
	GLfixed tmp[size]; \
	Atari2HostIntArray(size, v, tmp); \
	fn.glVertex3xvOES(tmp)
#else
#define FN_GLVERTEX3XVOES(v) \
	fn.glVertex3xvOES(HostAddr(v, const GLfixed *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEX4XVOES(v) \
	GLint const size = 4; \
	GLfixed tmp[size]; \
	Atari2HostIntArray(size, v, tmp); \
	fn.glVertex4xvOES(tmp)
#else
#define FN_GLVERTEX4XVOES(v) \
	fn.glVertex4xvOES(HostAddr(v, const GLfixed *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLBITMAPXOES(width, height, xorig, yorig, xmove, ymove, bitmap) \
	if (width <= 0 || height <= 0) return; \
	GLsizei bytes_per_row = (width + 7) / 8; \
	GLsizei size = bytes_per_row * height; \
	GLubyte tmp[size], *ptmp; \
	ptmp = Atari2HostByteArray(size, bitmap, tmp); \
	fn.glBitmapxOES(width, height, xorig, yorig, xmove, ymove, ptmp)
#else
#define FN_GLBITMAPXOES(width, height, xorig, yorig, xmove, ymove, bitmap) \
	fn.glBitmapxOES(width, height, xorig, yorig, xmove, ymove, HostAddr(bitmap, const GLubyte *))
#endif
	
/* -------------------------------------------------------------------------- */

/*
 * GL_OES_byte_coordinates
 */
#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLMULTITEXCOORD1BVOES(texture, coords) \
	int const size = 1; \
	GLbyte tmp[size], *ptmp; \
	ptmp = Atari2HostByteArray(size, coords, tmp); \
	fn.glMultiTexCoord1bvOES(texture, ptmp)
#else
#define FN_GLMULTITEXCOORD1BVOES(texture, coords) \
	fn.glMultiTexCoord1bvOES(texture, HostAddr(coords, const GLbyte *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLMULTITEXCOORD2BVOES(texture, coords) \
	int const size = 2; \
	GLbyte tmp[size], *ptmp; \
	ptmp = Atari2HostByteArray(size, coords, tmp); \
	fn.glMultiTexCoord2bvOES(texture, ptmp)
#else
#define FN_GLMULTITEXCOORD2BVOES(texture, coords) \
	fn.glMultiTexCoord2bvOES(texture, HostAddr(coords, const GLbyte *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLMULTITEXCOORD3BVOES(texture, coords) \
	int const size = 3; \
	GLbyte tmp[size], *ptmp; \
	ptmp = Atari2HostByteArray(size, coords, tmp); \
	fn.glMultiTexCoord3bvOES(texture, ptmp)
#else
#define FN_GLMULTITEXCOORD3BVOES(texture, coords) \
	fn.glMultiTexCoord3bvOES(texture, HostAddr(coords, const GLbyte *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLMULTITEXCOORD4BVOES(texture, coords) \
	int const size = 4; \
	GLbyte tmp[size], *ptmp; \
	ptmp = Atari2HostByteArray(size, coords, tmp); \
	fn.glMultiTexCoord4bvOES(texture, ptmp)
#else
#define FN_GLMULTITEXCOORD4BVOES(texture, coords) \
	fn.glMultiTexCoord4bvOES(texture, HostAddr(coords, const GLbyte *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLTEXCOORD1BVOES(coords) \
	int const size = 1; \
	GLbyte tmp[size], *ptmp; \
	ptmp = Atari2HostByteArray(size, coords, tmp); \
	fn.glTexCoord1bvOES(ptmp)
#else
#define FN_GLTEXCOORD1BVOES(coords) \
	fn.glTexCoord1bvOES(HostAddr(coords, const GLbyte *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLTEXCOORD2BVOES(coords) \
	int const size = 2; \
	GLbyte tmp[size], *ptmp; \
	ptmp = Atari2HostByteArray(size, coords, tmp); \
	fn.glTexCoord2bvOES(ptmp)
#else
#define FN_GLTEXCOORD2BVOES(coords) \
	fn.glTexCoord2bvOES(HostAddr(coords, const GLbyte *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLTEXCOORD3BVOES(coords) \
	int const size = 3; \
	GLbyte tmp[size], *ptmp; \
	ptmp = Atari2HostByteArray(size, coords, tmp); \
	fn.glTexCoord3bvOES(ptmp)
#else
#define FN_GLTEXCOORD3BVOES(coords) \
	fn.glTexCoord3bvOES(HostAddr(coords, const GLbyte *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLTEXCOORD4BVOES(coords) \
	int const size = 4; \
	GLbyte tmp[size], *ptmp; \
	ptmp = Atari2HostByteArray(size, coords, tmp); \
	fn.glTexCoord4bvOES(ptmp)
#else
#define FN_GLTEXCOORD4BVOES(coords) \
	fn.glTexCoord4bvOES(HostAddr(coords, const GLbyte *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLVERTEX2BVOES(coords) \
	int const size = 2; \
	GLbyte tmp[size], *ptmp; \
	ptmp = Atari2HostByteArray(size, coords, tmp); \
	fn.glVertex2bvOES(ptmp)
#else
#define FN_GLVERTEX2BVOES(coords) \
	fn.glVertex2bvOES(HostAddr(coords, const GLbyte *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLVERTEX3BVOES(coords) \
	int const size = 3; \
	GLbyte tmp[size], *ptmp; \
	ptmp = Atari2HostByteArray(size, coords, tmp); \
	fn.glVertex3bvOES(ptmp)
#else
#define FN_GLVERTEX3BVOES(coords) \
	fn.glVertex3bvOES(HostAddr(coords, const GLbyte *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLVERTEX4BVOES(coords) \
	int const size = 4; \
	GLbyte tmp[size], *ptmp; \
	ptmp = Atari2HostByteArray(size, coords, tmp); \
	fn.glVertex4bvOES(ptmp)
#else
#define FN_GLVERTEX4BVOES(coords) \
	fn.glVertex4bvOES(HostAddr(coords, const GLbyte *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_OES_query_matrix
 */
#if NFOSMESA_NEED_INT_CONV
#define FN_GLQUERYMATRIXXOES(mantissa, exponent) \
	GLint const size = 16; \
	GLfixed m[size]; \
	GLint e[size]; \
	GLbitfield ret = fn.glQueryMatrixxOES(m, e); \
	Host2AtariIntArray(size, m, mantissa); \
	Host2AtariIntArray(size, e, exponent); \
	return ret
#else
#define FN_GLQUERYMATRIXXOES(mantissa, exponent) \
	return fn.glQueryMatrixxOES(HostAddr(mantissa, GLfixed *), HostAddr(exponent, GLint *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_SGIX_reference_plane
 */

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLREFERENCEPLANESGIX(equation) \
	GLint const size = 4; \
	GLdouble tmp[size], *ptmp; \
	ptmp = Atari2HostDoubleArray(size, equation, tmp); \
	fn.glReferencePlaneSGIX(ptmp)
#else
#define FN_GLREFERENCEPLANESGIX(equation) \
	fn.glReferencePlaneSGIX(HostAddr(equation, const GLdouble *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_SGIX_instruments
 */

/* NYI; buffer will be accessed asynchronously by glStartInstrumentsSGIX() */
#define FN_GLINSTRUMENTSBUFFERSGIX(size, buffer) \
	fn.glInstrumentsBufferSGIX(size, HostAddr(buffer, GLint *))

#if NFOSMESA_NEED_INT_CONV
#define FN_GLPOLLINSTRUMENTSSGIX(marker_p) \
	GLint const size = 1; \
	GLint tmp[size]; \
	GLint ret = fn.glPollInstrumentsSGIX(tmp); \
	Host2AtariIntArray(1, tmp, marker_p); \
	return ret
#else
#define FN_GLPOLLINSTRUMENTSSGIX(marker_p) \
	return fn.glPollInstrumentsSGIX(HostAddr(marker_p, GLint *))
#endif
	
/* -------------------------------------------------------------------------- */

/*
 * GL_SGIS_pixel_texture
 */
#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETPIXELTEXGENPARAMETERFVSGIS(pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLfloat tmp[MAX(size, 16)]; \
	fn.glGetPixelTexGenParameterfvSGIS(pname, tmp); \
	Host2AtariFloatArray(size, tmp, params)
#else
#define FN_GLGETPIXELTEXGENPARAMETERFVSGIS(pname, params) \
	fn.glGetPixelTexGenParameterfvSGIS(pname, HostAddr(params, GLfloat *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPIXELTEXGENPARAMETERFVSGIS(pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLfloat tmp[MAX(size, 16)], *ptmp; \
	ptmp = Atari2HostFloatArray(size, params, tmp); \
	fn.glPixelTexGenParameterfvSGIS(pname, ptmp)
#else
#define FN_GLPIXELTEXGENPARAMETERFVSGIS(pname, params) \
	fn.glPixelTexGenParameterfvSGIS(pname, HostAddr(params, const GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETPIXELTEXGENPARAMETERIVSGIS(pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLint tmp[MAX(size, 16)]; \
	fn.glGetPixelTexGenParameterivSGIS(pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETPIXELTEXGENPARAMETERIVSGIS(pname, params) \
	fn.glGetPixelTexGenParameterivSGIS(pname, HostAddr(params, GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLPIXELTEXGENPARAMETERIVSGIS(pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLint tmp[MAX(size, 16)], *ptmp; \
	ptmp = Atari2HostIntArray(size, params, tmp); \
	fn.glPixelTexGenParameterivSGIS(pname, ptmp)
#else
#define FN_GLPIXELTEXGENPARAMETERIVSGIS(pname, params) \
	fn.glPixelTexGenParameterivSGIS(pname, HostAddr(params, const GLint *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_EXT_paletted_texture
 */

#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV || NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLCOLORTABLEEXT(target, internalformat, width, format, type, table) \
	pixelBuffer buf(*this); \
	void *tmp = buf.convertPixels(width, 1, 1, format, type, (nfmemptr)table); \
	fn.glColorTableEXT(target, internalformat, width, format, type, tmp)
#else
#define FN_GLCOLORTABLEEXT(target, internalformat, width, format, type, table) \
	fn.glColorTableEXT(target, internalformat, width, format, type, HostAddr(table, const void *))
#endif

#define FN_GLGETCOLORTABLEEXT(target, format, type, table) \
	GLint width = 0; \
	char *src; \
	nfmemptr dst; \
	pixelBuffer buf(*this); \
	 \
	fn.glGetColorTableParameteriv(target, GL_COLOR_TABLE_WIDTH, &width); \
	if (width == 0) return; \
	if (contexts[cur_context].buffer_bindings.pixel_pack.id) { \
		void *offset = NFHost2AtariAddr(table); \
		fn.glGetColorTableEXT(target, format, type, offset); \
		src = contexts[cur_context].buffer_bindings.pixel_pack.host_pointer + (uintptr_t)offset; \
		dst = contexts[cur_context].buffer_bindings.pixel_pack.atari_pointer + (uintptr_t)offset; \
		if (!buf.params(width, 1, 1, format, type)) return; \
	} else { \
		src = buf.hostBuffer(width, 1, 1, format, type, (nfmemptr)table); \
		if (src == NULL) return; \
		fn.glGetColorTableEXT(target, format, type, src); \
		dst = (nfmemptr)table; \
	} \
	buf.convertToAtari(src, dst)

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETCOLORTABLEPARAMETERIVEXT(target, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLint tmp[size]; \
	fn.glGetColorTableParameterivEXT(target, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETCOLORTABLEPARAMETERIVEXT(target, pname, params) \
	fn.glGetColorTableParameterivEXT(target, pname, HostAddr(params, GLint *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETCOLORTABLEPARAMETERFVEXT(target, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLfloat tmp[size]; \
	fn.glGetColorTableParameterfvEXT(target, pname, tmp); \
	Host2AtariFloatArray(size, tmp, params)
#else
#define FN_GLGETCOLORTABLEPARAMETERFVEXT(target, pname, params) \
	fn.glGetColorTableParameterfvEXT(target, pname, HostAddr(params, GLfloat *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_SGI_color_table
 */

#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV || NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLCOLORTABLESGI(target, internalformat, width, format, type, table) \
	pixelBuffer buf(*this); \
	void *tmp = buf.convertPixels(width, 1, 1, format, type, (nfmemptr)table); \
	fn.glColorTableSGI(target, internalformat, width, format, type, tmp)
#else
#define FN_GLCOLORTABLESGI(target, internalformat, width, format, type, table) \
	fn.glColorTableSGI(target, internalformat, width, format, type, HostAddr(table, const void *))
#endif

#define FN_GLGETCOLORTABLESGI(target, format, type, table) \
	GLint width = 0; \
	char *src; \
	nfmemptr dst; \
	pixelBuffer buf(*this); \
	 \
	fn.glGetColorTableParameteriv(target, GL_COLOR_TABLE_WIDTH, &width); \
	if (contexts[cur_context].buffer_bindings.pixel_pack.id) { \
		void *offset = NFHost2AtariAddr(table); \
		fn.glGetColorTableSGI(target, format, type, offset); \
		src = contexts[cur_context].buffer_bindings.pixel_pack.host_pointer + (uintptr_t)offset; \
		dst = contexts[cur_context].buffer_bindings.pixel_pack.atari_pointer + (uintptr_t)offset; \
		if (!buf.params(width, 1, 1, format, type)) return; \
	} else { \
		src = buf.hostBuffer(width, 1, 1, format, type, (nfmemptr)table); \
		if (src == NULL) return; \
		fn.glGetColorTableSGI(target, format, type, src); \
		dst = (nfmemptr)table; \
	} \
	buf.convertToAtari(src, dst)

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETCOLORTABLEPARAMETERFVSGI(target, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLfloat tmp[size]; \
	fn.glGetColorTableParameterfvSGI(target, pname, tmp); \
	Host2AtariFloatArray(size, tmp, params)
#else
#define FN_GLGETCOLORTABLEPARAMETERFVSGI(target, pname, params) \
	fn.glGetColorTableParameterfvSGI(target, pname, HostAddr(params, GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETCOLORTABLEPARAMETERIVSGI(target, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLint tmp[size]; \
	fn.glGetColorTableParameterivSGI(target, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETCOLORTABLEPARAMETERIVSGI(target, pname, params) \
	fn.glGetColorTableParameterivSGI(target, pname, HostAddr(params, GLint *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLCOLORTABLEPARAMETERFVSGI(target, pname, params) \
	GLint const size = 4; \
	GLfloat tmp[size], *ptmp; \
	ptmp = Atari2HostFloatArray(size, params, tmp); \
	fn.glColorTableParameterfvSGI(target, pname, ptmp)
#else
#define FN_GLCOLORTABLEPARAMETERFVSGI(target, pname, params) \
	fn.glColorTableParameterfvSGI(target, pname, HostAddr(params, const GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLCOLORTABLEPARAMETERIVSGI(target, pname, params) \
	GLint const size = 4; \
	GLint tmp[size], *ptmp; \
	ptmp = Atari2HostIntArray(size, params, tmp); \
	fn.glColorTableParameterivSGI(target, pname, ptmp)
#else
#define FN_GLCOLORTABLEPARAMETERIVSGI(target, pname, params) \
	fn.glColorTableParameterivSGI(target, pname, HostAddr(params, const GLint *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_EXT_pixel_transform
 */
#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETPIXELTRANSFORMPARAMETERFVEXT(target, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLfloat tmp[MAX(size, 16)]; \
	fn.glGetPixelTransformParameterfvEXT(target, pname, tmp); \
	Host2AtariFloatArray(size, tmp, params)
#else
#define FN_GLGETPIXELTRANSFORMPARAMETERFVEXT(target, pname, params) \
	fn.glGetPixelTransformParameterfvEXT(target, pname, HostAddr(params, GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETPIXELTRANSFORMPARAMETERIVEXT(target, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLint tmp[MAX(size, 16)]; \
	fn.glGetPixelTransformParameterivEXT(target, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETPIXELTRANSFORMPARAMETERIVEXT(target, pname, params) \
	fn.glGetPixelTransformParameterivEXT(target, pname, HostAddr(params, GLint *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPIXELTRANSFORMPARAMETERFVEXT(target, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLfloat tmp[MAX(size, 16)], *ptmp; \
	ptmp = Atari2HostFloatArray(size, params, tmp); \
	fn.glPixelTransformParameterfvEXT(target, pname, ptmp)
#else
#define FN_GLPIXELTRANSFORMPARAMETERFVEXT(target, pname, params) \
	fn.glPixelTransformParameterfvEXT(target, pname, HostAddr(params, const GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLPIXELTRANSFORMPARAMETERIVEXT(target, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLint tmp[MAX(size, 16)], *ptmp; \
	ptmp = Atari2HostIntArray(size, params, tmp); \
	fn.glPixelTransformParameterivEXT(target, pname, ptmp)
#else
#define FN_GLPIXELTRANSFORMPARAMETERIVEXT(target, pname, params) \
	fn.glPixelTransformParameterivEXT(target, pname, HostAddr(params, const GLint *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_ARB_matrix_palette
 */
#if NFOSMESA_NEED_INT_CONV
#define FN_GLMATRIXINDEXUSVARB(size, indices) \
	if (size <= 0) return; \
	GLushort tmp[size], *ptmp; \
	ptmp = Atari2HostShortArray(size, indices, tmp); \
	fn.glMatrixIndexusvARB(size, ptmp)
#else
#define FN_GLMATRIXINDEXUSVARB(size, indices) \
	fn.glMatrixIndexusvARB(size, HostAddr(indices, GLushort *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLMATRIXINDEXUBVARB(size, indices) \
	GLubyte tmp[MAX(size, 0)], *ptmp; \
	ptmp = Atari2HostByteArray(size, indices, tmp); \
	fn.glMatrixIndexubvARB(size, ptmp)
#else
#define FN_GLMATRIXINDEXUBVARB(size, indices) \
	fn.glMatrixIndexubvARB(size, HostAddr(indices, GLubyte *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLMATRIXINDEXUIVARB(size, indices) \
	if (size <= 0) return; \
	GLuint tmp[size], *ptmp; \
	ptmp = Atari2HostIntArray(size, indices, tmp); \
	fn.glMatrixIndexuivARB(size, ptmp)
#else
#define FN_GLMATRIXINDEXUIVARB(size, indices) \
	fn.glMatrixIndexuivARB(size, HostAddr(indices, const GLuint *))
#endif

#define FN_GLMATRIXINDEXPOINTERARB(size, type, stride, pointer) \
	GLint texunit = 0; \
	fn.glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE, &texunit); \
	setupClientArray(texunit, contexts[cur_context].matrixindex, size, type, stride, -1, 0, AtariAddr(pointer, nfmemptr)); \
	contexts[cur_context].matrixindex.vendor = NFOSMESA_VENDOR_ARB

/* -------------------------------------------------------------------------- */

/*
 * GL_IBM_vertex_array_lists
 */
#define FN_GLCOLORPOINTERLISTIBM(size, type, stride, pointer, ptrstride) \
	GLint texunit = 0; \
	fn.glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE, &texunit); \
	setupClientArray(texunit, contexts[cur_context].color, size, type, stride, -1, ptrstride, AtariAddr(pointer, nfmemptr)); \
	contexts[cur_context].color.vendor = NFOSMESA_VENDOR_IBM

#define FN_GLEDGEFLAGPOINTERLISTIBM(stride, pointer, ptrstride) \
	GLint texunit = 0; \
	fn.glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE, &texunit); \
	setupClientArray(texunit, contexts[cur_context].edgeflag, 1, GL_UNSIGNED_BYTE, stride, -1, ptrstride, AtariAddr(pointer, nfmemptr)); \
	contexts[cur_context].edgeflag.vendor = NFOSMESA_VENDOR_IBM

#define FN_GLINDEXPOINTERLISTIBM(type, stride, pointer, ptrstride) \
	GLint texunit = 0; \
	fn.glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE, &texunit); \
	setupClientArray(texunit, contexts[cur_context].index, 1, type, stride, -1, ptrstride, AtariAddr(pointer, nfmemptr)); \
	contexts[cur_context].index.vendor = NFOSMESA_VENDOR_IBM

#define FN_GLNORMALPOINTERLISTIBM(type, stride, pointer, ptrstride) \
	GLint texunit = 0; \
	fn.glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE, &texunit); \
	setupClientArray(texunit, contexts[cur_context].normal, 3, type, stride, -1, ptrstride, AtariAddr(pointer, nfmemptr)); \
	contexts[cur_context].normal.vendor = NFOSMESA_VENDOR_IBM

#define FN_GLFOGCOORDPOINTERLISTIBM(type, stride, pointer, ptrstride) \
	GLint texunit = 0; \
	fn.glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE, &texunit); \
	setupClientArray(texunit, contexts[cur_context].fogcoord, 1, type, stride, -1, ptrstride, AtariAddr(pointer, nfmemptr)); \
	contexts[cur_context].fogcoord.vendor = NFOSMESA_VENDOR_IBM

#define FN_GLSECONDARYCOLORPOINTERLISTIBM(size, type, stride, pointer, ptrstride) \
	GLint texunit = 0; \
	fn.glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE, &texunit); \
	setupClientArray(texunit, contexts[cur_context].secondary_color, size, type, stride, -1, ptrstride, AtariAddr(pointer, nfmemptr)); \
	contexts[cur_context].secondary_color.vendor = NFOSMESA_VENDOR_IBM

#define FN_GLTEXCOORDPOINTERLISTIBM(size, type, stride, pointer, ptrstride) \
	GLint texunit = 0; \
	fn.glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE, &texunit); \
	setupClientArray(texunit, contexts[cur_context].texcoord, size, type, stride, -1, ptrstride, AtariAddr(pointer, nfmemptr)); \
	contexts[cur_context].texcoord.vendor = NFOSMESA_VENDOR_IBM

#define FN_GLVERTEXPOINTERLISTIBM(size, type, stride, pointer, ptrstride) \
	GLint texunit = 0; \
	fn.glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE, &texunit); \
	setupClientArray(texunit, contexts[cur_context].vertex, size, type, stride, -1, ptrstride, AtariAddr(pointer, nfmemptr)); \
	contexts[cur_context].vertex.vendor = NFOSMESA_VENDOR_IBM

/* -------------------------------------------------------------------------- */

/*
 * GL_INTEL_parallel_arrays
 */
// FIXME: pointer is array of 2-4 pointers
#define FN_GLCOLORPOINTERVINTEL(size, type, pointer) \
	GLint texunit = 0; \
	fn.glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE, &texunit); \
	setupClientArray(texunit, contexts[cur_context].color, size, type, 0, -1, 0, AtariAddr(pointer, nfmemptr)); \
	contexts[cur_context].color.vendor = NFOSMESA_VENDOR_INTEL

#define FN_GLNORMALPOINTERVINTEL(type, pointer) \
	GLint texunit = 0; \
	fn.glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE, &texunit); \
	setupClientArray(texunit, contexts[cur_context].normal, 3, type, 0, -1, 0, AtariAddr(pointer, nfmemptr)); \
	contexts[cur_context].normal.vendor = NFOSMESA_VENDOR_INTEL

#define FN_GLTEXCOORDPOINTERVINTEL(size, type, pointer) \
	GLint texunit = 0; \
	fn.glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE, &texunit); \
	setupClientArray(texunit, contexts[cur_context].texcoord, size, type, 0, -1, 0, AtariAddr(pointer, nfmemptr)); \
	contexts[cur_context].texcoord.vendor = NFOSMESA_VENDOR_INTEL

#define FN_GLVERTEXPOINTERVINTEL(size, type, pointer) \
	GLint texunit = 0; \
	fn.glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE, &texunit); \
	setupClientArray(texunit, contexts[cur_context].vertex, size, type, 0, -1, 0, AtariAddr(pointer, nfmemptr)); \
	contexts[cur_context].vertex.vendor = NFOSMESA_VENDOR_INTEL

/* -------------------------------------------------------------------------- */

/*
 * GL_EXT_vertex_array
 */
#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV || NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLARRAYELEMENTEXT(i) nfglArrayElementHelper(i)
#else
#define FN_GLARRAYELEMENTEXT(i) \
	fn.glArrayElement(i)
#endif

#define FN_GLCOLORPOINTEREXT(size, type, stride, count, pointer) \
	GLint texunit = 0; \
	fn.glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE, &texunit); \
	setupClientArray(texunit, contexts[cur_context].color, size, type, stride, count, 0, AtariAddr(pointer, nfmemptr)); \
	contexts[cur_context].color.vendor = NFOSMESA_VENDOR_EXT

#define FN_GLDRAWARRAYSEXT(mode, first, count) \
	convertClientArrays(first + count); \
	fn.glDrawArraysEXT(mode, first, count)

#define FN_GLEDGEFLAGPOINTEREXT(stride, count, pointer) \
	GLint texunit = 0; \
	fn.glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE, &texunit); \
	setupClientArray(texunit, contexts[cur_context].edgeflag, 1, GL_UNSIGNED_BYTE, stride, count, 0, AtariAddr(pointer, nfmemptr)); \
	contexts[cur_context].edgeflag.vendor = NFOSMESA_VENDOR_EXT

#define FN_GLGETPOINTERVEXT(pname, data) \
	GLint texunit = 0; \
	fn.glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE, &texunit); \
	gl_get_pointer(pname, texunit, HostAddr(data, void **))

#define FN_GLINDEXPOINTEREXT(type, stride, count, pointer) \
	GLint texunit = 0; \
	fn.glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE, &texunit); \
	setupClientArray(texunit, contexts[cur_context].index, 1, type, stride, count, 0, AtariAddr(pointer, nfmemptr)); \
	contexts[cur_context].index.vendor = NFOSMESA_VENDOR_EXT

#define FN_GLNORMALPOINTEREXT(type, stride, count, pointer) \
	GLint texunit = 0; \
	fn.glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE, &texunit); \
	setupClientArray(texunit, contexts[cur_context].normal, 3, type, stride, count, 0, AtariAddr(pointer, nfmemptr)); \
	contexts[cur_context].normal.vendor = NFOSMESA_VENDOR_EXT

#define FN_GLTEXCOORDPOINTEREXT(size, type, stride, count, pointer) \
	GLint texunit = 0; \
	fn.glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE, &texunit); \
	setupClientArray(texunit, contexts[cur_context].texcoord, size, type, stride, count, 0, AtariAddr(pointer, nfmemptr)); \
	contexts[cur_context].texcoord.vendor = NFOSMESA_VENDOR_EXT

#define FN_GLVERTEXPOINTEREXT(size, type, stride, count, pointer) \
	GLint texunit = 0; \
	fn.glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE, &texunit); \
	setupClientArray(texunit, contexts[cur_context].vertex, size, type, stride, count, 0, AtariAddr(pointer, nfmemptr)); \
	contexts[cur_context].vertex.vendor = NFOSMESA_VENDOR_EXT

/* -------------------------------------------------------------------------- */

/*
 * GL_EXT_compiled_vertex_array
 */
#define FN_GLLOCKARRAYSEXT(first, count) \
	convertClientArrays(first + count); \
	fn.glLockArraysEXT(first, count)

/* -------------------------------------------------------------------------- */

/*
 * GL_NV_gpu_program4
 */
#if NFOSMESA_NEED_INT_CONV
#define FN_GLPROGRAMLOCALPARAMETERI4IVNV(target, index, params) \
	GLint const size = 4; \
	GLint tmp[size]; \
	Atari2HostIntArray(size, params, tmp); \
	fn.glProgramLocalParameterI4ivNV(target, index, tmp)
#else
#define FN_GLPROGRAMLOCALPARAMETERI4IVNV(target, index, params) \
	fn.glProgramLocalParameterI4ivNV(target, index, HostAddr(params, const GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLPROGRAMLOCALPARAMETERSI4IVNV(target, index, count, params) \
	GLint const size = 4 * count; \
	GLint tmp[size]; \
	Atari2HostIntArray(size, params, tmp); \
	fn.glProgramLocalParametersI4ivNV(target, index, count, tmp)
#else
#define FN_GLPROGRAMLOCALPARAMETERSI4IVNV(target, index, count, params) \
	fn.glProgramLocalParametersI4ivNV(target, index, count, HostAddr(params, const GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLPROGRAMLOCALPARAMETERI4UIVNV(target, index, params) \
	GLint const size = 4; \
	GLuint tmp[size]; \
	Atari2HostIntArray(size, params, tmp); \
	fn.glProgramLocalParameterI4uivNV(target, index, tmp)
#else
#define FN_GLPROGRAMLOCALPARAMETERI4UIVNV(target, index, params) \
	fn.glProgramLocalParameterI4uivNV(target, index, HostAddr(params, const GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLPROGRAMLOCALPARAMETERSI4UIVNV(target, index, count, params) \
	GLint const size = 4 * count; \
	GLuint tmp[size]; \
	Atari2HostIntArray(size, params, tmp); \
	fn.glProgramLocalParametersI4uivNV(target, index, count, tmp)
#else
#define FN_GLPROGRAMLOCALPARAMETERSI4UIVNV(target, index, count, params) \
	fn.glProgramLocalParametersI4uivNV(target, index, count, HostAddr(params, const GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLPROGRAMENVPARAMETERI4IVNV(target, index, params) \
	GLint const size = 4; \
	GLint tmp[size]; \
	Atari2HostIntArray(size, params, tmp); \
	fn.glProgramEnvParameterI4ivNV(target, index, tmp)
#else
#define FN_GLPROGRAMENVPARAMETERI4IVNV(target, index, params) \
	fn.glProgramEnvParameterI4ivNV(target, index, HostAddr(params, const GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLPROGRAMENVPARAMETERSI4IVNV(target, index, count, params) \
	GLint const size = 4 * count; \
	GLint tmp[size]; \
	Atari2HostIntArray(size, params, tmp); \
	fn.glProgramEnvParametersI4ivNV(target, index, count, tmp)
#else
#define FN_GLPROGRAMENVPARAMETERSI4IVNV(target, index, count, params) \
	fn.glProgramEnvParametersI4ivNV(target, index, count, HostAddr(params, const GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLPROGRAMENVPARAMETERI4UIVNV(target, index, params) \
	GLint const size = 4; \
	GLuint tmp[size]; \
	Atari2HostIntArray(size, params, tmp); \
	fn.glProgramEnvParameterI4uivNV(target, index, tmp)
#else
#define FN_GLPROGRAMENVPARAMETERI4UIVNV(target, index, params) \
	fn.glProgramEnvParameterI4uivNV(target, index, HostAddr(params, const GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLPROGRAMENVPARAMETERSI4UIVNV(target, index, count, params) \
	GLint const size = 4 * count; \
	GLuint tmp[size]; \
	Atari2HostIntArray(size, params, tmp); \
	fn.glProgramEnvParametersI4uivNV(target, index, count, tmp)
#else
#define FN_GLPROGRAMENVPARAMETERSI4UIVNV(target, index, count, params) \
	fn.glProgramEnvParametersI4uivNV(target, index, count, HostAddr(params, const GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETPROGRAMLOCALPARAMETERIIVNV(target, index, params) \
	GLint const size = 1; \
	GLint tmp[size]; \
	fn.glGetProgramLocalParameterIivNV(target, index, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETPROGRAMLOCALPARAMETERIIVNV(target, index, params) \
	fn.glGetProgramLocalParameterIivNV(target, index, HostAddr(params, GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETPROGRAMLOCALPARAMETERIUIVNV(target, index, params) \
	GLint const size = 1; \
	GLuint tmp[size]; \
	fn.glGetProgramLocalParameterIuivNV(target, index, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETPROGRAMLOCALPARAMETERIUIVNV(target, index, params) \
	fn.glGetProgramLocalParameterIuivNV(target, index, HostAddr(params, GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETPROGRAMENVPARAMETERIIVNV(target, index, params) \
	GLint const size = 1; \
	GLint tmp[size]; \
	fn.glGetProgramEnvParameterIivNV(target, index, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETPROGRAMENVPARAMETERIIVNV(target, index, params) \
	fn.glGetProgramEnvParameterIivNV(target, index, HostAddr(params, GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETPROGRAMENVPARAMETERIUIVNV(target, index, params) \
	GLint const size = 1; \
	GLuint tmp[size]; \
	fn.glGetProgramEnvParameterIuivNV(target, index, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETPROGRAMENVPARAMETERIUIVNV(target, index, params) \
	fn.glGetProgramEnvParameterIuivNV(target, index, HostAddr(params, GLuint *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_NV_gpu_program5
 */
 
#if NFOSMESA_NEED_INT_CONV
#define FN_GLPROGRAMSUBROUTINEPARAMETERSUIVNV(target, count, params) \
	GLint const size = 4 * count; \
	GLuint tmp[size]; \
	Atari2HostIntArray(size, params, tmp); \
	fn.glProgramSubroutineParametersuivNV(target, count, tmp)
#else
#define FN_GLPROGRAMSUBROUTINEPARAMETERSUIVNV(target, count, params) \
	fn.glProgramSubroutineParametersuivNV(target, count, HostAddr(params, const GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETPROGRAMSUBROUTINEPARAMETERUIVNV(target, index, params) \
	GLint const size = 1; \
	GLuint tmp[size]; \
	fn.glGetProgramSubroutineParameteruivNV(target, index, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETPROGRAMSUBROUTINEPARAMETERUIVNV(target, index, params) \
	fn.glGetProgramSubroutineParameteruivNV(target, index, HostAddr(params, GLuint *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_ARB_fragment_program
 */
#if NFOSMESA_NEED_INT_CONV
#define FN_GLDELETEPROGRAMSARB(n, programs) \
	GLuint tmp[MAX(n, 0)], *ptmp; \
	ptmp = Atari2HostIntArray(n, programs, tmp); \
	fn.glDeleteProgramsARB(n, ptmp)
#else
#define FN_GLDELETEPROGRAMSARB(n, programs) \
	fn.glDeleteProgramsARB(n, HostAddr(programs, const GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGENPROGRAMSARB(n, programs) \
	GLsizei const size = MAX(n, 0); \
	GLuint tmp[size]; \
	fn.glGenProgramsARB(n, tmp); \
	Host2AtariIntArray(n, tmp, programs)
#else
#define FN_GLGENPROGRAMSARB(n, programs) \
	fn.glGenProgramsARB(n, HostAddr(programs, GLuint *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPROGRAMENVPARAMETER4FVARB(target, index, params) \
	GLfloat tmp[4]; \
	Atari2HostFloatArray(4, params, tmp); \
	fn.glProgramEnvParameter4fvARB(target, index, tmp)
#else
#define FN_GLPROGRAMENVPARAMETER4FVARB(target, index, params) \
	fn.glProgramEnvParameter4fvARB(target, index, HostAddr(params, const GLfloat *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLPROGRAMENVPARAMETER4DVARB(target, index, params) \
	GLdouble tmp[4]; \
	Atari2HostDoubleArray(4, params, tmp); \
	fn.glProgramEnvParameter4dvARB(target, index, tmp)
#else
#define FN_GLPROGRAMENVPARAMETER4DVARB(target, index, params) \
	fn.glProgramEnvParameter4dvARB(target, index, HostAddr(params, const GLdouble *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPROGRAMLOCALPARAMETER4FVARB(target, index, params) \
	GLfloat tmp[4]; \
	Atari2HostFloatArray(4, params, tmp); \
	fn.glProgramLocalParameter4fvARB(target, index, tmp)
#else
#define FN_GLPROGRAMLOCALPARAMETER4FVARB(target, index, params) \
	fn.glProgramLocalParameter4fvARB(target, index, HostAddr(params, const GLfloat *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLPROGRAMLOCALPARAMETER4DVARB(target, index, params) \
	GLdouble tmp[4]; \
	Atari2HostDoubleArray(4, params, tmp); \
	fn.glProgramLocalParameter4dvARB(target, index, tmp)
#else
#define FN_GLPROGRAMLOCALPARAMETER4DVARB(target, index, params) \
	fn.glProgramLocalParameter4dvARB(target, index, HostAddr(params, const GLdouble *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETPROGRAMENVPARAMETERFVARB(target, index, params) \
	GLfloat tmp[4]; \
	fn.glGetProgramEnvParameterfvARB(target, index, tmp); \
	Host2AtariFloatArray(4, tmp, params)
#else
#define FN_GLGETPROGRAMENVPARAMETERFVARB(target, index, params) \
	fn.glGetProgramEnvParameterfvARB(target, index, HostAddr(params, GLfloat *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLGETPROGRAMENVPARAMETERDVARB(target, index, params) \
	GLdouble tmp[4]; \
	fn.glGetProgramEnvParameterdvARB(target, index, tmp); \
	Host2AtariDoubleArray(4, tmp, params)
#else
#define FN_GLGETPROGRAMENVPARAMETERDVARB(target, index, params) \
	fn.glGetProgramEnvParameterdvARB(target, index, HostAddr(params, GLdouble *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETPROGRAMLOCALPARAMETERFVARB(target, index, params) \
	GLfloat tmp[4]; \
	fn.glGetProgramLocalParameterfvARB(target, index, tmp); \
	Host2AtariFloatArray(4, tmp, params)
#else
#define FN_GLGETPROGRAMLOCALPARAMETERFVARB(target, index, params) \
	fn.glGetProgramLocalParameterfvARB(target, index, HostAddr(params, GLfloat *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLGETPROGRAMLOCALPARAMETERDVARB(target, index, params) \
	GLdouble tmp[4]; \
	fn.glGetProgramLocalParameterdvARB(target, index, tmp); \
	Host2AtariDoubleArray(4, tmp, params)
#else
#define FN_GLGETPROGRAMLOCALPARAMETERDVARB(target, index, params) \
	fn.glGetProgramLocalParameterdvARB(target, index, HostAddr(params, GLdouble *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETPROGRAMIVARB(target, index, params) \
	GLint const size = 1; \
	GLint tmp[size]; \
	fn.glGetProgramivARB(target, index, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETPROGRAMIVARB(target, index, params) \
	fn.glGetProgramivARB(target, index, HostAddr(params, GLint *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLGETPROGRAMSTRINGARB(target, pname, string) \
	GLint size = 0; \
	fn.glGetProgramivARB(target, GL_PROGRAM_LENGTH_ARB, &size); \
	if (size <= 0) return; \
	GLubyte tmp[size + 1]; \
	fn.glGetProgramStringARB(target, pname, tmp); \
	Host2AtariByteArray(size, tmp, string)
#else
#define FN_GLGETPROGRAMSTRINGARB(target, pname, string) \
	fn.glGetProgramStringARB(target, pname, HostAddr(string, void *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLPROGRAMSTRINGARB(target, format, len, string) \
	if (len < 0) len = safe_strlen(string); \
	GLubyte tmp[len], *ptmp; \
	ptmp = Atari2HostByteArray(sizeof(tmp), string, tmp); \
	fn.glProgramStringARB(target, format, len, ptmp)
#else
#define FN_GLPROGRAMSTRINGARB(target, format, len, string) \
	fn.glProgramStringARB(target, format, len, HostAddr(string, const void *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_NV_fragment_program
 */
#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLPROGRAMNAMEDPARAMETER4DNV(id, len, name, x, y, z, w) \
	GLubyte namebuf[MAX(len, 0)], *pname; \
	pname = Atari2HostByteArray(len, name, namebuf); \
	fn.glProgramNamedParameter4dNV(id, len, pname, x, y, z, w)
#else
#define FN_GLPROGRAMNAMEDPARAMETER4DNV(id, len, name, x, y, z, w) \
	fn.glProgramNamedParameter4dNV(id, len, HostAddr(name, const GLubyte *), x, y, z, w)
#endif

#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLPROGRAMNAMEDPARAMETER4DVNV(id, len, name, v) \
	GLint const size = 4; \
	GLdouble tmp[size], *ptmp; \
	GLubyte namebuf[MAX(len, 0)], *pname; \
	ptmp = Atari2HostDoubleArray(size, v, tmp); \
	pname = Atari2HostByteArray(len, name, namebuf); \
	fn.glProgramNamedParameter4dvNV(id, len, pname, ptmp)
#else
#define FN_GLPROGRAMNAMEDPARAMETER4DVNV(id, len, name, v) \
	fn.glProgramNamedParameter4dvNV(id, len, HostAddr(name, const GLubyte *), HostAddr(v, const GLdouble *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLPROGRAMNAMEDPARAMETER4FNV(id, len, name, x, y, z, w) \
	GLubyte namebuf[MAX(len, 0)], *pname; \
	pname = Atari2HostByteArray(len, name, namebuf); \
	fn.glProgramNamedParameter4fNV(id, len, pname, x, y, z, w)
#else
#define FN_GLPROGRAMNAMEDPARAMETER4FNV(id, len, name, x, y, z, w) \
	fn.glProgramNamedParameter4fNV(id, len, HostAddr(name, const GLubyte *), x, y, z, w)
#endif

#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPROGRAMNAMEDPARAMETER4FVNV(id, len, name, v) \
	GLint const size = 4; \
	GLfloat tmp[size]; \
	GLubyte namebuf[MAX(len, 0)], *pname; \
	Atari2HostFloatArray(size, v, tmp); \
	pname = Atari2HostByteArray(len, name, namebuf); \
	fn.glProgramNamedParameter4fvNV(id, len, pname, tmp)
#else
#define FN_GLPROGRAMNAMEDPARAMETER4FVNV(id, len, name, v) \
	fn.glProgramNamedParameter4fvNV(id, len, HostAddr(name, const GLubyte *), HostAddr(v, const GLfloat *))
#endif

#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLGETPROGRAMNAMEDPARAMETERDVNV(id, len, name, v) \
	GLint const size = 4; \
	GLdouble tmp[size]; \
	GLubyte namebuf[safe_strlen(name) + 1], *pname;\
	pname = Atari2HostByteArray(sizeof(namebuf), name, namebuf); \
	fn.glGetProgramNamedParameterdvNV(id, len, pname, tmp); \
	Host2AtariDoubleArray(size, tmp, v)
#else
#define FN_GLGETPROGRAMNAMEDPARAMETERDVNV(id, len, name, v) \
	fn.glGetProgramNamedParameterdvNV(id, len, HostAddr(name, const GLubyte *), HostAddr(v, GLdouble *))
#endif

#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETPROGRAMNAMEDPARAMETERFVNV(id, len, name, v) \
	GLint const size = 4; \
	GLfloat tmp[size]; \
	GLubyte namebuf[safe_strlen(name) + 1], *pname;\
	pname = Atari2HostByteArray(sizeof(namebuf), name, namebuf); \
	fn.glGetProgramNamedParameterfvNV(id, len, pname, tmp); \
	Host2AtariFloatArray(size, tmp, v)
#else
#define FN_GLGETPROGRAMNAMEDPARAMETERFVNV(id, len, name, v) \
	fn.glGetProgramNamedParameterfvNV(id, len, HostAddr(name, const GLubyte *), HostAddr(v, GLfloat *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_NV_vertex_program
 */
#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_INT_CONV
#define FN_GLAREPROGRAMSRESIDENTNV(n, programs, residences) \
	GLboolean res; \
	GLuint tmp[MAX(n, 0)], *ptmp; \
	GLboolean resbuf[MAX(n, 0)]; \
	ptmp = Atari2HostIntArray(n, programs, tmp); \
	res = fn.glAreProgramsResidentNV(n, ptmp, resbuf); \
	Host2AtariByteArray(n, resbuf, residences); \
	return res
#else
#define FN_GLAREPROGRAMSRESIDENTNV(n, programs, residences) \
	return fn.glAreProgramsResidentNV(n, HostAddr(programs, const GLuint *), HostAddr(residences, GLboolean *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLDELETEPROGRAMSNV(n, programs) \
	GLint const size = n; \
	if (size <= 0 || !programs) return; \
	GLuint tmp[size]; \
	Atari2HostIntArray(n, programs, tmp); \
	fn.glDeleteProgramsNV(n, tmp)
#else
#define FN_GLDELETEPROGRAMSNV(n, programs) \
	fn.glDeleteProgramsNV(n, HostAddr(programs, const GLuint *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLEXECUTEPROGRAMNV(target, id, params) \
	GLfloat tmp[4]; \
	Atari2HostFloatArray(4, params, tmp); \
	fn.glExecuteProgramNV(target, id, tmp)
#else
#define FN_GLEXECUTEPROGRAMNV(size, type, buffer) \
	fn.glExecuteProgramNV(target, id, HostAddr(params, const GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGENPROGRAMSNV(n, programs) \
	GLsizei const size = MAX(n, 0); \
	GLuint tmp[size]; \
	fn.glGenProgramsNV(n, tmp); \
	Host2AtariIntArray(n, tmp, programs)
#else
#define FN_GLGENPROGRAMSNV(n, programs) \
	fn.glGenProgramsNV(n, HostAddr(programs, GLuint *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLPROGRAMPARAMETER4DVNV(target, index, v) \
	GLint const size = 4; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, v, tmp); \
	fn.glProgramParameter4dvNV(target, index, tmp)
#else
#define FN_GLPROGRAMPARAMETER4DVNV(target, index, v) \
	fn.glProgramParameter4dvNV(target, index, HostAddr(v, const GLdouble *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPROGRAMPARAMETER4FVNV(target, index, v) \
	GLint const size = 4; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, v, tmp); \
	fn.glProgramParameter4fvNV(target, index, tmp)
#else
#define FN_GLPROGRAMPARAMETER4FVNV(target, index, v) \
	fn.glProgramParameter4fvNV(target, index, HostAddr(v, const GLfloat *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLPROGRAMPARAMETERS4DVNV(target, index, count, v) \
	GLint const size = 4; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, v, tmp); \
	fn.glProgramParameters4dvNV(target, index, count, tmp)
#else
#define FN_GLPROGRAMPARAMETERS4DVNV(target, index, count, v) \
	fn.glProgramParameters4dvNV(target, index, count, HostAddr(v, const GLdouble *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPROGRAMPARAMETERS4FVNV(target, index, count, v) \
	GLint const size = 4; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, v, tmp); \
	fn.glProgramParameters4fvNV(target, index, count, tmp)
#else
#define FN_GLPROGRAMPARAMETERS4FVNV(target, index, count, v) \
	fn.glProgramParameters4fvNV(target, index, count, HostAddr(v, const GLfloat *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLGETPROGRAMPARAMETERDVNV(target, index, pname, params) \
	GLint const size = 4; \
	GLdouble tmp[size]; \
	fn.glGetProgramParameterdvNV(target, index, pname, tmp); \
	Host2AtariDoubleArray(size, tmp, params)
#else
#define FN_GLGETPROGRAMPARAMETERDVNV(target, index, pname, params) \
	fn.glGetProgramParameterdvNV(target, index, pname, HostAddr(params, GLdouble *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETPROGRAMPARAMETERFVNV(target, index, pname, params) \
	GLint const size = 4; \
	GLfloat tmp[size]; \
	fn.glGetProgramParameterfvNV(target, index, pname, tmp); \
	Host2AtariFloatArray(size, tmp, params)
#else
#define FN_GLGETPROGRAMPARAMETERFVNV(target, index, pname, params) \
	fn.glGetProgramParameterfvNV(target, index, pname, HostAddr(params, GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETPROGRAMIVNV(id, pname, params) \
	GLint const size = 1; \
	GLint tmp[size]; \
	fn.glGetProgramivNV(id, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETPROGRAMIVNV(id, pname, params) \
	fn.glGetProgramivNV(id, pname, HostAddr(params, GLint *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLGETPROGRAMSTRINGNV(id, pname, program) \
	GLint size = 0; \
	fn.glGetProgramivNV(id, GL_PROGRAM_LENGTH_NV, &size); \
	if (size <= 0) return; \
	GLubyte tmp[size + 1]; \
	fn.glGetProgramStringNV(id, pname, tmp); \
	Host2AtariByteArray(size, tmp, program)
#else
#define FN_GLGETPROGRAMSTRINGNV(id, pname, program) \
	fn.glGetProgramStringNV(id, pname, HostAddr(program, GLubyte *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLLOADPROGRAMNV(target, id, len, program) \
	GLubyte tmp[MAX(len, 0)], *ptmp; \
	ptmp = Atari2HostByteArray(len, program, tmp); \
	fn.glLoadProgramNV(target, id, len, ptmp)
#else
#define FN_GLLOADPROGRAMNV(target, id, len, program) \
	fn.glLoadProgramNV(target, id, len, HostAddr(program, GLubyte *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETTRACKMATRIXIVNV(target, address, pname, params) \
	GLint const size = 1; \
	GLint tmp[size]; \
	fn.glGetTrackMatrixivNV(target, address, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETTRACKMATRIXIVNV(target, address, pname, params) \
	fn.glGetTrackMatrixivNV(target, address, pname, HostAddr(params, GLint *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLGETVERTEXATTRIBDVNV(index, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLdouble tmp[size]; \
	fn.glGetVertexAttribdvNV(index, pname, tmp); \
	Host2AtariDoubleArray(size, tmp, params)
#else
#define FN_GLGETVERTEXATTRIBDVNV(index, pname, params) \
	fn.glGetVertexAttribdvNV(index, pname, HostAddr(params, GLdouble *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETVERTEXATTRIBFVNV(index, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLfloat tmp[size]; \
	fn.glGetVertexAttribfvNV(index, pname, tmp); \
	Host2AtariFloatArray(size, tmp, params)
#else
#define FN_GLGETVERTEXATTRIBFVNV(index, pname, params) \
	fn.glGetVertexAttribfvNV(index, pname, HostAddr(params, GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETVERTEXATTRIBIVNV(index, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLint tmp[size]; \
	fn.glGetVertexAttribivNV(index, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETVERTEXATTRIBIVNV(index, pname, params) \
	fn.glGetVertexAttribivNV(index, pname, HostAddr(params, GLint *))
#endif

/* TODO */
#define FN_GLGETVERTEXATTRIBPOINTERVNV(index, pname, pointer) \
	UNUSED(pointer); \
	void *p = 0; \
	fn.glGetVertexAttribPointervNV(index, pname, &p)

/* TODO */
#define FN_GLVERTEXATTRIBPOINTERNV(index, size, type, stride, pointer) \
	fn.glVertexAttribPointerNV(index, size, type, stride, HostAddr(pointer, const void *))

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLVERTEXATTRIB1DVNV(index, v) \
	GLint const size = 1; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, v, tmp); \
	fn.glVertexAttrib1dvNV(index, tmp)
#else
#define FN_GLVERTEXATTRIB1DVNV(index, v) \
	fn.glVertexAttrib1dvNV(index, HostAddr(v, const GLdouble *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLVERTEXATTRIB1FVNV(index, v) \
	GLint const size = 1; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, v, tmp); \
	fn.glVertexAttrib1fvNV(index, tmp)
#else
#define FN_GLVERTEXATTRIB1FVNV(index, v) \
	fn.glVertexAttrib1fvNV(index, HostAddr(v, const GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIB1SVNV(index, v) \
	GLint const size = 1; \
	GLshort tmp[size]; \
	Atari2HostShortArray(size, v, tmp); \
	fn.glVertexAttrib1svNV(index, tmp)
#else
#define FN_GLVERTEXATTRIB1SVNV(index, v) \
	fn.glVertexAttrib1svNV(index, HostAddr(v, const GLshort *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLVERTEXATTRIB2DVNV(index, v) \
	GLint const size = 2; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, v, tmp); \
	fn.glVertexAttrib2dvNV(index, tmp)
#else
#define FN_GLVERTEXATTRIB2DVNV(index, v) \
	fn.glVertexAttrib2dvNV(index, HostAddr(v, const GLdouble *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLVERTEXATTRIB2FVNV(index, v) \
	GLint const size = 2; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, v, tmp); \
	fn.glVertexAttrib2fvNV(index, tmp)
#else
#define FN_GLVERTEXATTRIB2FVNV(index, v) \
	fn.glVertexAttrib2fvNV(index, HostAddr(v, const GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIB2SVNV(index, v) \
	GLint const size = 2; \
	GLshort tmp[size]; \
	Atari2HostShortArray(size, v, tmp); \
	fn.glVertexAttrib2svNV(index, tmp)
#else
#define FN_GLVERTEXATTRIB2SVNV(index, v) \
	fn.glVertexAttrib2svNV(index, HostAddr(v, const GLshort *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLVERTEXATTRIB3DVNV(index, v) \
	GLint const size = 3; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, v, tmp); \
	fn.glVertexAttrib3dvNV(index, tmp)
#else
#define FN_GLVERTEXATTRIB3DVNV(index, v) \
	fn.glVertexAttrib3dvNV(index, HostAddr(v, const GLdouble *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLVERTEXATTRIB3FVNV(index, v) \
	GLint const size = 3; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, v, tmp); \
	fn.glVertexAttrib3fvNV(index, tmp)
#else
#define FN_GLVERTEXATTRIB3FVNV(index, v) \
	fn.glVertexAttrib3fvNV(index, HostAddr(v, const GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIB3SVNV(index, v) \
	GLint const size = 3; \
	GLshort tmp[size]; \
	Atari2HostShortArray(size, v, tmp); \
	fn.glVertexAttrib3svNV(index, tmp)
#else
#define FN_GLVERTEXATTRIB3SVNV(index, v) \
	fn.glVertexAttrib3svNV(index, HostAddr(v, const GLshort *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLVERTEXATTRIB4DVNV(index, v) \
	GLint const size = 4; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, v, tmp); \
	fn.glVertexAttrib4dvNV(index, tmp)
#else
#define FN_GLVERTEXATTRIB4DVNV(index, v) \
	fn.glVertexAttrib4dvNV(index, HostAddr(v, const GLdouble *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLVERTEXATTRIB4NUBV(index, v) \
	GLint const size = 4; \
	GLubyte tmp[size], *ptmp; \
	ptmp = Atari2HostByteArray(size, v, tmp); \
	fn.glVertexAttrib4Nubv(index, ptmp)
#else
#define FN_GLVERTEXATTRIB4NUBV(index, v) \
	fn.glVertexAttrib4Nubv(index, HostAddr(v, const GLubyte *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLVERTEXATTRIB4UBVNV(index, v) \
	GLint const size = 4; \
	GLubyte tmp[size]; \
	Atari2HostByteArray(size, v, tmp); \
	fn.glVertexAttrib4ubvNV(index, tmp)
#else
#define FN_GLVERTEXATTRIB4UBVNV(index, v) \
	fn.glVertexAttrib4ubvNV(index, HostAddr(v, const GLubyte *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLVERTEXATTRIB4FVNV(index, v) \
	GLint const size = 4; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, v, tmp); \
	fn.glVertexAttrib4fvNV(index, tmp)
#else
#define FN_GLVERTEXATTRIB4FVNV(index, v) \
	fn.glVertexAttrib4fvNV(index, HostAddr(v, const GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIB4SVNV(index, v) \
	GLint const size = 4; \
	GLshort tmp[size]; \
	Atari2HostShortArray(size, v, tmp); \
	fn.glVertexAttrib4svNV(index, tmp)
#else
#define FN_GLVERTEXATTRIB4SVNV(index, v) \
	fn.glVertexAttrib4svNV(index, HostAddr(v, const GLshort *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLVERTEXATTRIBS1DVNV(index, count, v) \
	GLint const size = 1; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, v, tmp); \
	fn.glVertexAttribs1dvNV(index, count, tmp)
#else
#define FN_GLVERTEXATTRIBS1DVNV(index, count, v) \
	fn.glVertexAttribs1dvNV(index, count, HostAddr(v, const GLdouble *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLVERTEXATTRIBS1FVNV(index, count, v) \
	GLint const size = 1; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, v, tmp); \
	fn.glVertexAttribs1fvNV(index, count, tmp)
#else
#define FN_GLVERTEXATTRIBS1FVNV(index, count, v) \
	fn.glVertexAttribs1fvNV(index, count, HostAddr(v, const GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIBS1SVNV(index, count, v) \
	GLint const size = 1; \
	GLshort tmp[size]; \
	Atari2HostShortArray(size, v, tmp); \
	fn.glVertexAttribs1svNV(index, count, tmp)
#else
#define FN_GLVERTEXATTRIBS1SVNV(index, count, v) \
	fn.glVertexAttribs1svNV(index, count, HostAddr(v, const GLshort *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLVERTEXATTRIBS2DVNV(index, count, v) \
	GLint const size = 2; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, v, tmp); \
	fn.glVertexAttribs2dvNV(index, count, tmp)
#else
#define FN_GLVERTEXATTRIBS2DVNV(index, count, v) \
	fn.glVertexAttribs2dvNV(index, count, HostAddr(v, const GLdouble *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLVERTEXATTRIBS2FVNV(index, count, v) \
	GLint const size = 2; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, v, tmp); \
	fn.glVertexAttribs2fvNV(index, count, tmp)
#else
#define FN_GLVERTEXATTRIBS2FVNV(index, count, v) \
	fn.glVertexAttribs2fvNV(index, count, HostAddr(v, const GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIBS2SVNV(index, count, v) \
	GLint const size = 2; \
	GLshort tmp[size]; \
	Atari2HostShortArray(size, v, tmp); \
	fn.glVertexAttribs2svNV(index, count, tmp)
#else
#define FN_GLVERTEXATTRIBS2SVNV(index, count, v) \
	fn.glVertexAttribs2svNV(index, count, HostAddr(v, const GLshort *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLVERTEXATTRIBS3DVNV(index, count, v) \
	GLint const size = 3; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, v, tmp); \
	fn.glVertexAttribs3dvNV(index, count, tmp)
#else
#define FN_GLVERTEXATTRIBS3DVNV(index, count, v) \
	fn.glVertexAttribs3dvNV(index, count, HostAddr(v, const GLdouble *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLVERTEXATTRIBS3FVNV(index, count, v) \
	GLint const size = 3; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, v, tmp); \
	fn.glVertexAttribs3fvNV(index, count, tmp)
#else
#define FN_GLVERTEXATTRIBS3FVNV(index, count, v) \
	fn.glVertexAttribs3fvNV(index, count, HostAddr(v, const GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIBS3SVNV(index, count, v) \
	GLint const size = 3; \
	GLshort tmp[size]; \
	Atari2HostShortArray(size, v, tmp); \
	fn.glVertexAttribs3svNV(index, count, tmp)
#else
#define FN_GLVERTEXATTRIBS3SVNV(index, count, v) \
	fn.glVertexAttribs3svNV(index, count, HostAddr(v, const GLshort *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLVERTEXATTRIBS4DVNV(index, count, v) \
	GLint const size = 4; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, v, tmp); \
	fn.glVertexAttribs4dvNV(index, count, tmp)
#else
#define FN_GLVERTEXATTRIBS4DVNV(index, count, v) \
	fn.glVertexAttribs4dvNV(index, count, HostAddr(v, const GLdouble *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLVERTEXATTRIBS4FVNV(index, count, v) \
	GLint const size = 4; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, v, tmp); \
	fn.glVertexAttribs4fvNV(index, count, tmp)
#else
#define FN_GLVERTEXATTRIBS4FVNV(index, count, v) \
	fn.glVertexAttribs4fvNV(index, count, HostAddr(v, const GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIBS4SVNV(index, count, v) \
	GLint const size = 4; \
	GLshort tmp[size]; \
	Atari2HostShortArray(size, v, tmp); \
	fn.glVertexAttribs4svNV(index, count, tmp)
#else
#define FN_GLVERTEXATTRIBS4SVNV(index, count, v) \
	fn.glVertexAttribs4svNV(index, count, HostAddr(v, const GLshort *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLVERTEXATTRIBS4UBVNV(index, count, v) \
	GLint const size = 4; \
	GLubyte tmp[size], *ptmp; \
	ptmp = Atari2HostByteArray(size, v, tmp); \
	fn.glVertexAttribs4ubvNV(index, count, ptmp)
#else
#define FN_GLVERTEXATTRIBS4UBVNV(index, count, v) \
	fn.glVertexAttribs4ubvNV(index, count, HostAddr(v, const GLubyte *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_NV_vertex_program4
 */
#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIBI1IVEXT(index, v) \
	int const size = 1; \
	GLint tmp[size], *ptmp; \
	ptmp = Atari2HostIntArray(size, v, tmp); \
	fn.glVertexAttribI1ivEXT(index, ptmp)
#else
#define FN_GLVERTEXATTRIBI1IVEXT(index, v) \
	fn.glVertexAttribI1ivEXT(index, HostAddr(v, const GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIBI2IVEXT(index, v) \
	int const size = 2; \
	GLint tmp[size], *ptmp; \
	ptmp = Atari2HostIntArray(size, v, tmp); \
	fn.glVertexAttribI2ivEXT(index, ptmp)
#else
#define FN_GLVERTEXATTRIBI2IVEXT(index, v) \
	fn.glVertexAttribI2ivEXT(index, HostAddr(v, const GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIBI3IVEXT(index, v) \
	int const size = 3; \
	GLint tmp[size], *ptmp; \
	ptmp = Atari2HostIntArray(size, v, tmp); \
	fn.glVertexAttribI3ivEXT(index, ptmp)
#else
#define FN_GLVERTEXATTRIBI3IVEXT(index, v) \
	fn.glVertexAttribI3ivEXT(index, HostAddr(v, const GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIBI4IVEXT(index, v) \
	int const size = 4; \
	GLint tmp[size], *ptmp; \
	ptmp = Atari2HostIntArray(size, v, tmp); \
	fn.glVertexAttribI4ivEXT(index, ptmp)
#else
#define FN_GLVERTEXATTRIBI4IVEXT(index, v) \
	fn.glVertexAttribI4ivEXT(index, HostAddr(v, const GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIBI1UIVEXT(index, v) \
	int const size = 1; \
	GLuint tmp[size], *ptmp; \
	ptmp = Atari2HostIntArray(size, v, tmp); \
	fn.glVertexAttribI1uivEXT(index, ptmp)
#else
#define FN_GLVERTEXATTRIBI1UIVEXT(index, v) \
	fn.glVertexAttribI1uivEXT(index, HostAddr(v, const GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIBI2UIVEXT(index, v) \
	int const size = 2; \
	GLuint tmp[size], *ptmp; \
	ptmp = Atari2HostIntArray(size, v, tmp); \
	fn.glVertexAttribI2uivEXT(index, ptmp)
#else
#define FN_GLVERTEXATTRIBI2UIVEXT(index, v) \
	fn.glVertexAttribI2uivEXT(index, HostAddr(v, const GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIBI3UIVEXT(index, v) \
	int const size = 3; \
	GLuint tmp[size], *ptmp; \
	ptmp = Atari2HostIntArray(size, v, tmp); \
	fn.glVertexAttribI3uivEXT(index, ptmp)
#else
#define FN_GLVERTEXATTRIBI3UIVEXT(index, v) \
	fn.glVertexAttribI3uivEXT(index, HostAddr(v, const GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIBI4UIVEXT(index, v) \
	int const size = 4; \
	GLuint tmp[size], *ptmp; \
	ptmp = Atari2HostIntArray(size, v, tmp); \
	fn.glVertexAttribI4uivEXT(index, ptmp)
#else
#define FN_GLVERTEXATTRIBI4UIVEXT(index, v) \
	fn.glVertexAttribI4uivEXT(index, HostAddr(v, const GLuint *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLVERTEXATTRIBI4BVEXT(index, v) \
	int const size = 4; \
	GLbyte tmp[size], *ptmp; \
	ptmp = Atari2HostByteArray(size, v, tmp); \
	fn.glVertexAttribI4bvEXT(index, ptmp)
#else
#define FN_GLVERTEXATTRIBI4BVEXT(index, v) \
	fn.glVertexAttribI4bvEXT(index, HostAddr(v, const GLbyte *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIBI4SVEXT(index, v) \
	int const size = 4; \
	GLshort tmp[size], *ptmp; \
	ptmp = Atari2HostShortArray(size, v, tmp); \
	fn.glVertexAttribI4svEXT(index, ptmp)
#else
#define FN_GLVERTEXATTRIBI4SVEXT(index, v) \
	fn.glVertexAttribI4svEXT(index, HostAddr(v, const GLshort *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLVERTEXATTRIBI4UBVEXT(index, v) \
	int const size = 4; \
	GLubyte tmp[size], *ptmp; \
	ptmp = Atari2HostByteArray(size, v, tmp); \
	fn.glVertexAttribI4ubvEXT(index, ptmp)
#else
#define FN_GLVERTEXATTRIBI4UBVEXT(index, v) \
	fn.glVertexAttribI4ubvEXT(index, HostAddr(v, const GLubyte *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIBI4USVEXT(index, v) \
	int const size = 4; \
	GLushort tmp[size], *ptmp; \
	ptmp = Atari2HostShortArray(size, v, tmp); \
	fn.glVertexAttribI4usvEXT(index, ptmp)
#else
#define FN_GLVERTEXATTRIBI4USVEXT(index, v) \
	fn.glVertexAttribI4usvEXT(index, HostAddr(v, const GLushort *))
#endif

/* TODO */
#define FN_GLVERTEXATTRIBIPOINTEREXT(index, size, type, stride, pointer) \
	fn.glVertexAttribIPointerEXT(index, size, type, stride, HostAddr(pointer, const void *))

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETVERTEXATTRIBIIVEXT(index, pname, params) \
	int const n = nfglGetNumParams(pname); \
	GLint tmp[MAX(n, 16)]; \
	fn.glGetVertexAttribIivEXT(index, pname, tmp); \
	Host2AtariIntArray(n, tmp, params)
#else
#define FN_GLGETVERTEXATTRIBIIVEXT(index, pname, params) \
	fn.glGetVertexAttribIivEXT(index, pname, HostAddr(params, GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETVERTEXATTRIBIUIVEXT(index, pname, params) \
	int const n = nfglGetNumParams(pname); \
	GLuint tmp[MAX(n, 16)]; \
	fn.glGetVertexAttribIuivEXT(index, pname, tmp); \
	Host2AtariIntArray(n, tmp, params)
#else
#define FN_GLGETVERTEXATTRIBIUIVEXT(index, pname, params) \
	fn.glGetVertexAttribIuivEXT(index, pname, HostAddr(params, GLuint *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_EXT_vertex_attrib_64bit
 */

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLVERTEXATTRIBL1DVEXT(index, v) \
	int const size = 1; \
	GLdouble tmp[size], *ptmp; \
	ptmp = Atari2HostDoubleArray(size, v, tmp); \
	fn.glVertexAttribL1dvEXT(index, ptmp)
#else
#define FN_GLVERTEXATTRIBL1DVEXT(index, v) \
	fn.glVertexAttribL1dvEXT(index, HostAddr(v, const GLdouble *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLVERTEXATTRIBL2DVEXT(index, v) \
	int const size = 2; \
	GLdouble tmp[size], *ptmp; \
	ptmp = Atari2HostDoubleArray(size, v, tmp); \
	fn.glVertexAttribL2dvEXT(index, ptmp)
#else
#define FN_GLVERTEXATTRIBL2DVEXT(index, v) \
	fn.glVertexAttribL2dvEXT(index, HostAddr(v, const GLdouble *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLVERTEXATTRIBL3DVEXT(index, v) \
	int const size = 3; \
	GLdouble tmp[size], *ptmp; \
	ptmp = Atari2HostDoubleArray(size, v, tmp); \
	fn.glVertexAttribL3dvEXT(index, ptmp)
#else
#define FN_GLVERTEXATTRIBL3DVEXT(index, v) \
	fn.glVertexAttribL3dvEXT(index, HostAddr(v, const GLdouble *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLVERTEXATTRIBL4DVEXT(index, v) \
	int const size = 4; \
	GLdouble tmp[size], *ptmp; \
	ptmp = Atari2HostDoubleArray(size, v, tmp); \
	fn.glVertexAttribL4dvEXT(index, ptmp)
#else
#define FN_GLVERTEXATTRIBL4DVEXT(index, v) \
	fn.glVertexAttribL4dvEXT(index, HostAddr(v, const GLdouble *))
#endif

/* TODO */
#define FN_GLVERTEXATTRIBLPOINTEREXT(index, size, type, stride, pointer) \
	fn.glVertexAttribLPointerEXT(index, size, type, stride, HostAddr(pointer, const void *))

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLGETVERTEXATTRIBLDVEXT(index, pname, params) \
	int const n = nfglGetNumParams(pname); \
	GLdouble tmp[MAX(n, 16)]; \
	fn.glGetVertexAttribLdvEXT(index, pname, tmp); \
	Host2AtariDoubleArray(n, tmp, params)
#else
#define FN_GLGETVERTEXATTRIBLDVEXT(index, pname, params) \
	fn.glGetVertexAttribLdvEXT(index, pname, HostAddr(params, GLdouble *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_NV_vertex_attrib_integer_64bit
 */

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIBL1I64VNV(index, v) \
	int const size = 1; \
	GLint64EXT tmp[size], *ptmp; \
	ptmp = Atari2HostInt64Array(size, v, tmp); \
	fn.glVertexAttribL1i64vNV(index, ptmp)
#else
#define FN_GLVERTEXATTRIBL1I64VNV(index, v) \
	fn.glVertexAttribL1i64vNV(index, HostAddr(v, const GLint64EXT *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIBL2I64VNV(index, v) \
	int const size = 2; \
	GLint64EXT tmp[size], *ptmp; \
	ptmp = Atari2HostInt64Array(size, v, tmp); \
	fn.glVertexAttribL2i64vNV(index, ptmp)
#else
#define FN_GLVERTEXATTRIBL2I64VNV(index, v) \
	fn.glVertexAttribL2i64vNV(index, HostAddr(v, const GLint64EXT *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIBL3I64VNV(index, v) \
	int const size = 3; \
	GLint64EXT tmp[size], *ptmp; \
	ptmp = Atari2HostInt64Array(size, v, tmp); \
	fn.glVertexAttribL3i64vNV(index, ptmp)
#else
#define FN_GLVERTEXATTRIBL3I64VNV(index, v) \
	fn.glVertexAttribL3i64vNV(index, HostAddr(v, const GLint64EXT *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIBL4I64VNV(index, v) \
	int const size = 4; \
	GLint64EXT tmp[size], *ptmp; \
	ptmp = Atari2HostInt64Array(size, v, tmp); \
	fn.glVertexAttribL4i64vNV(index, ptmp)
#else
#define FN_GLVERTEXATTRIBL4I64VNV(index, v) \
	fn.glVertexAttribL4i64vNV(index, HostAddr(v, const GLint64EXT *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIBL1UI64VNV(index, v) \
	int const size = 1; \
	GLuint64EXT tmp[size], *ptmp; \
	ptmp = Atari2HostInt64Array(size, v, tmp); \
	fn.glVertexAttribL1ui64vNV(index, ptmp)
#else
#define FN_GLVERTEXATTRIBL1UI64VNV(index, v) \
	fn.glVertexAttribL1ui64vNV(index, HostAddr(v, const GLuint64EXT *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIBL2UI64VNV(index, v) \
	int const size = 2; \
	GLuint64EXT tmp[size], *ptmp; \
	ptmp = Atari2HostInt64Array(size, v, tmp); \
	fn.glVertexAttribL2ui64vNV(index, ptmp)
#else
#define FN_GLVERTEXATTRIBL2UI64VNV(index, v) \
	fn.glVertexAttribL2ui64vNV(index, HostAddr(v, const GLuint64EXT *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIBL3UI64VNV(index, v) \
	int const size = 3; \
	GLuint64EXT tmp[size], *ptmp; \
	ptmp = Atari2HostInt64Array(size, v, tmp); \
	fn.glVertexAttribL3ui64vNV(index, ptmp)
#else
#define FN_GLVERTEXATTRIBL3UI64VNV(index, v) \
	fn.glVertexAttribL3ui64vNV(index, HostAddr(v, const GLuint64EXT *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIBL4UI64VNV(index, v) \
	int const size = 4; \
	GLuint64EXT tmp[size], *ptmp; \
	ptmp = Atari2HostInt64Array(size, v, tmp); \
	fn.glVertexAttribL4ui64vNV(index, ptmp)
#else
#define FN_GLVERTEXATTRIBL4UI64VNV(index, v) \
	fn.glVertexAttribL4ui64vNV(index, HostAddr(v, const GLuint64EXT *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETVERTEXATTRIBLI64VNV(index, pname, params) \
	int const n = nfglGetNumParams(pname); \
	GLint64EXT tmp[MAX(n, 16)]; \
	fn.glGetVertexAttribLi64vNV(index, pname, tmp); \
	Host2AtariInt64Array(n, tmp, params)
#else
#define FN_GLGETVERTEXATTRIBLI64VNV(index, pname, params) \
	fn.glGetVertexAttribLi64vNV(index, pname, HostAddr(params, GLint64EXT *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETVERTEXATTRIBLUI64VNV(index, pname, params) \
	int const n = nfglGetNumParams(pname); \
	GLuint64EXT tmp[MAX(n, 16)]; \
	fn.glGetVertexAttribLui64vNV(index, pname, tmp); \
	Host2AtariInt64Array(n, tmp, params)
#else
#define FN_GLGETVERTEXATTRIBLUI64VNV(index, pname, params) \
	fn.glGetVertexAttribLui64vNV(index, pname, HostAddr(params, GLuint64EXT *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_NV_vertex_buffer_unified_memory
 */
#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETINTEGERUI64I_VNV(value, index, data) \
	GLint const size = nfglGetNumParams(value); \
	GLuint64EXT tmp[MAX(size, 16)]; \
	fn.glGetIntegerui64i_vNV(value, index, tmp); \
	Host2AtariInt64Array(size, tmp, data)
#else
#define FN_GLGETINTEGERUI64I_VNV(value, index, data) \
	fn.glGetIntegerui64i_vNV(value, index, HostAddr(data, GLuint64EXT *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_ARB_bindless_texture
 */

#if NFOSMESA_NEED_INT_CONV
#define FN_GLUNIFORMHANDLEUI64VARB(location, count, value) \
	int const size = count; \
	if (size <= 0) return; \
	GLuint64 tmp[size], *ptmp; \
	ptmp = Atari2HostInt64Array(size, value, tmp); \
	fn.glUniformHandleui64vARB(location, count, ptmp)
#else
#define FN_GLUNIFORMHANDLEUI64VARB(location, count, value) \
	fn.glUniformHandleui64vARB(location, count, HostAddr(value, const GLuint64 *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLPROGRAMUNIFORMHANDLEUI64VARB(program, location, count, value) \
	int const size = count; \
	if (size <= 0) return; \
	GLuint64 tmp[size], *ptmp; \
	ptmp = Atari2HostInt64Array(size, value, tmp); \
	fn.glProgramUniformHandleui64vARB(program, location, count, ptmp)
#else
#define FN_GLPROGRAMUNIFORMHANDLEUI64VARB(program, location, count, value) \
	fn.glProgramUniformHandleui64vARB(program, location, count, HostAddr(value, const GLuint64 *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIBL1UI64VARB(index, v) \
	int const size = 1; \
	GLuint64EXT tmp[size], *ptmp; \
	ptmp = Atari2HostInt64Array(size, v, tmp); \
	fn.glVertexAttribL1ui64vARB(index, ptmp)
#else
#define FN_GLVERTEXATTRIBL1UI64VARB(index, v) \
	fn.glVertexAttribL1ui64vARB(index, HostAddr(v, const GLuint64EXT *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETVERTEXATTRIBLUI64VARB(index, pname, params) \
	int const n = nfglGetNumParams(pname); \
	GLuint64EXT tmp[MAX(n, 16)]; \
	fn.glGetVertexAttribLui64vARB(index, pname, tmp); \
	Host2AtariInt64Array(n, tmp, params)
#else
#define FN_GLGETVERTEXATTRIBLUI64VARB(index, pname, params) \
	fn.glGetVertexAttribLui64vARB(index, pname, HostAddr(params, GLuint64EXT *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_NV_bindless_texture
 */

#if NFOSMESA_NEED_INT_CONV
#define FN_GLUNIFORMHANDLEUI64VNV(location, count, value) \
	int const size = count; \
	if (size <= 0) return; \
	GLuint64 tmp[size], *ptmp; \
	ptmp = Atari2HostInt64Array(size, value, tmp); \
	fn.glUniformHandleui64vNV(location, count, ptmp)
#else
#define FN_GLUNIFORMHANDLEUI64VNV(location, count, value) \
	fn.glUniformHandleui64vNV(location, count, HostAddr(value, const GLuint64 *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLPROGRAMUNIFORMHANDLEUI64VNV(program, location, count, value) \
	int const size = count; \
	if (size <= 0) return; \
	GLuint64 tmp[size], *ptmp; \
	ptmp = Atari2HostInt64Array(size, value, tmp); \
	fn.glProgramUniformHandleui64vNV(program, location, count, ptmp)
#else
#define FN_GLPROGRAMUNIFORMHANDLEUI64VNV(program, location, count, value) \
	fn.glProgramUniformHandleui64vNV(program, location, count, HostAddr(value, const GLuint64 *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_NV_shader_buffer_load
 */

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETBUFFERPARAMETERUI64VNV(target, pname, params) \
	GLuint64EXT tmp[4]; \
	int const size = nfglGetNumParams(pname); \
	fn.glGetBufferParameterui64vNV(target, pname, tmp); \
	Host2AtariInt64Array(size, tmp, params)
#else
#define FN_GLGETBUFFERPARAMETERUI64VNV(target, pname, params) \
	fn.glGetBufferParameterui64vNV(target, pname, HostAddr(params, GLuint64EXT *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETINTEGERUI64VNV(value, data) \
	GLint const size = nfglGetNumParams(value); \
	GLuint64EXT tmp[MAX(size, 16)]; \
	fn.glGetIntegerui64vNV(value, tmp); \
	Host2AtariInt64Array(size, tmp, data)
#else
#define FN_GLGETINTEGERUI64VNV(value, data) \
	fn.glGetIntegerui64vNV(value, HostAddr(data, GLuint64EXT *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLUNIFORMUI64VNV(location, count, value) \
	GLint const size = count; \
	GLuint64EXT tmp[MAX(size, 16)], *ptmp; \
	ptmp = Atari2HostInt64Array(size, value, tmp); \
	fn.glUniformui64vNV(location, count, ptmp)
#else
#define FN_GLUNIFORMUI64VNV(location, count, value) \
	fn.glUniformui64vNV(location, count, HostAddr(value, const GLuint64EXT *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLPROGRAMUNIFORMUI64VNV(program, location, count, value) \
	GLint const size = count; \
	GLuint64EXT tmp[MAX(size, 16)], *ptmp; \
	ptmp = Atari2HostInt64Array(size, value, tmp); \
	fn.glProgramUniformui64vNV(program, location, count, ptmp)
#else
#define FN_GLPROGRAMUNIFORMUI64VNV(program, location, count, value) \
	fn.glProgramUniformui64vNV(program, location, count, HostAddr(value, const GLuint64EXT *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_NV_video_capture
 */

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETVIDEOCAPTUREIVNV(video_capture_slot, pname, params) \
	int const n = 1; \
	GLint tmp[MAX(n, 16)]; \
	fn.glGetVideoCaptureivNV(video_capture_slot, pname, tmp); \
	Host2AtariIntArray(n, tmp, params)
#else
#define FN_GLGETVIDEOCAPTUREIVNV(video_capture_slot, pname, params) \
	fn.glGetVideoCaptureivNV(video_capture_slot, pname, HostAddr(params, GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETVIDEOCAPTURESTREAMIVNV(video_capture_slot, stream, pname, params) \
	int const n = 1; \
	GLint tmp[MAX(n, 16)]; \
	fn.glGetVideoCaptureStreamivNV(video_capture_slot, stream, pname, tmp); \
	Host2AtariIntArray(n, tmp, params)
#else
#define FN_GLGETVIDEOCAPTURESTREAMIVNV(video_capture_slot, stream, pname, params) \
	fn.glGetVideoCaptureStreamivNV(video_capture_slot, stream, pname, HostAddr(params, GLint *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETVIDEOCAPTURESTREAMFVNV(video_capture_slot, stream, pname, params) \
	int const n = 1; \
	GLfloat tmp[MAX(n, 16)]; \
	fn.glGetVideoCaptureStreamfvNV(video_capture_slot, stream, pname, tmp); \
	Host2AtariFloatArray(n, tmp, params)
#else
#define FN_GLGETVIDEOCAPTURESTREAMFVNV(video_capture_slot, stream, pname, params) \
	fn.glGetVideoCaptureStreamfvNV(video_capture_slot, stream, pname, HostAddr(params, GLfloat *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLGETVIDEOCAPTURESTREAMDVNV(video_capture_slot, stream, pname, params) \
	int const n = 1; \
	GLdouble tmp[MAX(n, 16)]; \
	fn.glGetVideoCaptureStreamdvNV(video_capture_slot, stream, pname, tmp); \
	Host2AtariDoubleArray(n, tmp, params)
#else
#define FN_GLGETVIDEOCAPTURESTREAMDVNV(video_capture_slot, stream, pname, params) \
	fn.glGetVideoCaptureStreamdvNV(video_capture_slot, stream, pname, HostAddr(params, GLdouble *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVIDEOCAPTURENV(video_capture_slot, sequence_num, capture_time) \
	GLuint seq = 0; \
	GLuint64EXT time = 0; \
	GLenum ret = fn.glVideoCaptureNV(video_capture_slot, &seq, &time); \
	Host2AtariIntArray(1, &seq, sequence_num); \
	Host2AtariInt64Array(1, &time, capture_time); \
	return ret
#else
#define FN_GLVIDEOCAPTURENV(video_capture_slot, sequence_num, capture_time) \
	return fn.glVideoCaptureNV(video_capture_slot, HostAddr(sequence_num, GLuint *), HostAddr(capture_time, GLuint64EXT *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVIDEOCAPTURESTREAMPARAMETERIVNV(video_capture_slot, stream, pname, params) \
	int const n = 1; \
	GLint tmp[MAX(n, 16)], *ptmp; \
	ptmp = Atari2HostIntArray(n, params, tmp); \
	fn.glVideoCaptureStreamParameterivNV(video_capture_slot, stream, pname, ptmp)
#else
#define FN_GLVIDEOCAPTURESTREAMPARAMETERIVNV(video_capture_slot, stream, pname, params) \
	fn.glVideoCaptureStreamParameterivNV(video_capture_slot, stream, pname, HostAddr(params, const GLint *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLVIDEOCAPTURESTREAMPARAMETERFVNV(video_capture_slot, stream, pname, params) \
	int const n = 1; \
	GLfloat tmp[MAX(n, 16)], *ptmp; \
	ptmp = Atari2HostFloatArray(n, params, tmp); \
	fn.glVideoCaptureStreamParameterfvNV(video_capture_slot, stream, pname, ptmp)
#else
#define FN_GLVIDEOCAPTURESTREAMPARAMETERFVNV(video_capture_slot, stream, pname, params) \
	fn.glVideoCaptureStreamParameterfvNV(video_capture_slot, stream, pname, HostAddr(params, const GLfloat *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLVIDEOCAPTURESTREAMPARAMETERDVNV(video_capture_slot, stream, pname, params) \
	int const n = 1; \
	GLdouble tmp[MAX(n, 16)], *ptmp; \
	ptmp = Atari2HostDoubleArray(n, params, tmp); \
	fn.glVideoCaptureStreamParameterdvNV(video_capture_slot, stream, pname, ptmp)
#else
#define FN_GLVIDEOCAPTURESTREAMPARAMETERDVNV(video_capture_slot, stream, pname, params) \
	fn.glVideoCaptureStreamParameterdvNV(video_capture_slot, stream, pname, HostAddr(params, const GLdouble *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_NV_present_video
 */

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETVIDEOIVNV(video_slot, pname, params) \
	int const n = 1; \
	GLint tmp[MAX(n, 16)]; \
	fn.glGetVideoivNV(video_slot, pname, tmp); \
	Host2AtariIntArray(n, tmp, params)
#else
#define FN_GLGETVIDEOIVNV(video_slot, pname, params) \
	fn.glGetVideoivNV(video_slot, pname, HostAddr(params, GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETVIDEOUIVNV(video_slot, pname, params) \
	int const n = 1; \
	GLuint tmp[MAX(n, 16)]; \
	fn.glGetVideouivNV(video_slot, pname, tmp); \
	Host2AtariIntArray(n, tmp, params)
#else
#define FN_GLGETVIDEOUIVNV(video_slot, pname, params) \
	fn.glGetVideouivNV(video_slot, pname, HostAddr(params, GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETVIDEOI64VNV(video_slot, pname, params) \
	int const n = 1; \
	GLint64EXT tmp[MAX(n, 16)]; \
	fn.glGetVideoi64vNV(video_slot, pname, tmp); \
	Host2AtariInt64Array(n, tmp, params)
#else
#define FN_GLGETVIDEOI64VNV(video_slot, pname, params) \
	fn.glGetVideoi64vNV(video_slot, pname, HostAddr(params, GLint64EXT *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETVIDEOUI64VNV(video_slot, pname, params) \
	int const n = 1; \
	GLuint64EXT tmp[MAX(n, 16)]; \
	fn.glGetVideoui64vNV(video_slot, pname, tmp); \
	Host2AtariInt64Array(n, tmp, params)
#else
#define FN_GLGETVIDEOUI64VNV(video_slot, pname, params) \
	fn.glGetVideoui64vNV(video_slot, pname, HostAddr(params, GLuint64EXT *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_NV_vdpau_interop
 */
/* not implementable */

/* -------------------------------------------------------------------------- */

/*
 * GL_ARB_vertex_shader
 */
#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLBINDATTRIBLOCATIONARB(program, index, name) \
	GLchar tmp[safe_strlen(name) + 1], *ptmp; \
	ptmp = Atari2HostByteArray(sizeof(tmp), name, tmp); \
	fn.glBindAttribLocationARB(program, index, ptmp)
#else
#define FN_GLBINDATTRIBLOCATIONARB(program, index, name) \
	fn.glBindAttribLocationARB(program, index, HostAddr(name, const GLchar *))
#endif
	
#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_INT_CONV
#define FN_GLGETACTIVEATTRIBARB(program, index, maxLength, length, size, type, name) \
	GLsizei l; \
	GLint s; \
	GLenum t; \
	GLcharARB tmp[MAX(maxLength, 0)]; \
	fn.glGetActiveAttribARB(program, index, maxLength, &l, &s, &t, tmp); \
	Host2AtariByteArray(MIN(l + 1, maxLength), tmp, name); \
	Host2AtariIntArray(1, &l, length); \
	Host2AtariIntArray(1, &s, size); \
	Host2AtariIntArray(1, &t, type)
#else
#define FN_GLGETACTIVEATTRIBARB(program, index, bufSize, length, size, type, name) \
	fn.glGetActiveAttribARB(program, index, bufSize, HostAddr(length, GLsizei *), HostAddr(size, GLint *), HostAddr(type, GLenum *), HostAddr(name, GLcharARB *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLGETATTRIBLOCATIONARB(program, name) \
	GLchar namebuf[safe_strlen(name) + 1], *pname; \
	pname = Atari2HostByteArray(sizeof(namebuf), name, namebuf); \
	return fn.glGetAttribLocationARB(program, pname)
#else
#define FN_GLGETATTRIBLOCATIONARB(program, name) \
	return fn.glGetAttribLocationARB(program, HostAddr(name, const GLchar *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_ARB_vertex_program
 */
#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLVERTEXATTRIB1DVARB(index, v) \
	GLint const size = 1; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, v, tmp); \
	fn.glVertexAttrib1dvARB(index, tmp)
#else
#define FN_GLVERTEXATTRIB1DVARB(index, v) \
	fn.glVertexAttrib1dvARB(index, HostAddr(v, const GLdouble *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLVERTEXATTRIB1FVARB(index, v) \
	GLint const size = 1; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, v, tmp); \
	fn.glVertexAttrib1fvARB(index, tmp)
#else
#define FN_GLVERTEXATTRIB1FVARB(index, v) \
	fn.glVertexAttrib1fvARB(index, HostAddr(v, const GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIB1SVARB(index, v) \
	GLint const size = 1; \
	GLshort tmp[size]; \
	Atari2HostShortArray(size, v, tmp); \
	fn.glVertexAttrib1svARB(index, tmp)
#else
#define FN_GLVERTEXATTRIB1SVARB(index, v) \
	fn.glVertexAttrib1svARB(index, HostAddr(v, const GLshort *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLVERTEXATTRIB2DVARB(index, v) \
	GLint const size = 2; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, v, tmp); \
	fn.glVertexAttrib2dvARB(index, tmp)
#else
#define FN_GLVERTEXATTRIB2DVARB(index, v) \
	fn.glVertexAttrib2dvARB(index, HostAddr(v, const GLdouble *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLVERTEXATTRIB2FVARB(index, v) \
	GLint const size = 2; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, v, tmp); \
	fn.glVertexAttrib2fvARB(index, tmp)
#else
#define FN_GLVERTEXATTRIB2FVARB(index, v) \
	fn.glVertexAttrib2fvARB(index, HostAddr(v, const GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIB2SVARB(index, v) \
	GLint const size = 2; \
	GLshort tmp[size]; \
	Atari2HostShortArray(size, v, tmp); \
	fn.glVertexAttrib2svARB(index, tmp)
#else
#define FN_GLVERTEXATTRIB2SVARB(index, v) \
	fn.glVertexAttrib2svARB(index, HostAddr(v, const GLshort *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLVERTEXATTRIB3DVARB(index, v) \
	GLint const size = 3; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, v, tmp); \
	fn.glVertexAttrib3dvARB(index, tmp)
#else
#define FN_GLVERTEXATTRIB3DVARB(index, v) \
	fn.glVertexAttrib3dvARB(index, HostAddr(v, const GLdouble *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLVERTEXATTRIB3FVARB(index, v) \
	GLint const size = 3; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, v, tmp); \
	fn.glVertexAttrib3fvARB(index, tmp)
#else
#define FN_GLVERTEXATTRIB3FVARB(index, v) \
	fn.glVertexAttrib3fvARB(index, HostAddr(v, const GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIB3SVARB(index, v) \
	GLint const size = 3; \
	GLshort tmp[size]; \
	Atari2HostShortArray(size, v, tmp); \
	fn.glVertexAttrib3svARB(index, tmp)
#else
#define FN_GLVERTEXATTRIB3SVARB(index, v) \
	fn.glVertexAttrib3svARB(index, HostAddr(v, const GLshort *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLVERTEXATTRIB4NBVARB(index, v) \
	GLint const size = 4; \
	GLbyte tmp[size], *ptmp; \
	ptmp = Atari2HostByteArray(size, v, tmp); \
	fn.glVertexAttrib4NbvARB(index, ptmp)
#else
#define FN_GLVERTEXATTRIB4NBVARB(index, v) \
	fn.glVertexAttrib4NbvARB(index, HostAddr(v, const GLbyte *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLVERTEXATTRIB4NUBVARB(index, v) \
	GLint const size = 4; \
	GLubyte tmp[size], *ptmp; \
	ptmp = Atari2HostByteArray(size, v, tmp); \
	fn.glVertexAttrib4NubvARB(index, ptmp)
#else
#define FN_GLVERTEXATTRIB4NUBVARB(index, v) \
	fn.glVertexAttrib4NubvARB(index, HostAddr(v, const GLubyte *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIB4NIVARB(index, v) \
	GLint const size = 4; \
	GLint tmp[size]; \
	Atari2HostIntArray(size, v, tmp); \
	fn.glVertexAttrib4NivARB(index, tmp)
#else
#define FN_GLVERTEXATTRIB4NIVARB(index, v) \
	fn.glVertexAttrib4NivARB(index, HostAddr(v, const GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIB4NSVARB(index, v) \
	GLint const size = 4; \
	GLshort tmp[size]; \
	Atari2HostShortArray(size, v, tmp); \
	fn.glVertexAttrib4NsvARB(index, tmp)
#else
#define FN_GLVERTEXATTRIB4NSVARB(index, v) \
	fn.glVertexAttrib4NsvARB(index, HostAddr(v, const GLshort *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIB4NUIVARB(index, v) \
	GLint const size = 4; \
	GLuint tmp[size]; \
	Atari2HostIntArray(size, v, tmp); \
	fn.glVertexAttrib4NuivARB(index, tmp)
#else
#define FN_GLVERTEXATTRIB4NUIVARB(index, v) \
	fn.glVertexAttrib4NuivARB(index, HostAddr(v, const GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIB4NUSVARB(index, v) \
	GLint const size = 4; \
	GLushort tmp[size]; \
	Atari2HostShortArray(size, v, tmp); \
	fn.glVertexAttrib4NusvARB(index, tmp)
#else
#define FN_GLVERTEXATTRIB4NUSVARB(index, v) \
	fn.glVertexAttrib4NusvARB(index, HostAddr(v, const GLushort *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLVERTEXATTRIB4BVARB(index, v) \
	GLint const size = 4; \
	GLbyte tmp[size]; \
	Atari2HostByteArray(size, v, tmp); \
	fn.glVertexAttrib4bvARB(index, tmp)
#else
#define FN_GLVERTEXATTRIB4BVARB(index, v) \
	fn.glVertexAttrib4bvARB(index, HostAddr(v, const GLbyte *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLVERTEXATTRIB4UBVARB(index, v) \
	GLint const size = 4; \
	GLubyte tmp[size]; \
	Atari2HostByteArray(size, v, tmp); \
	fn.glVertexAttrib4ubvARB(index, tmp)
#else
#define FN_GLVERTEXATTRIB4UBVARB(index, v) \
	fn.glVertexAttrib4ubvARB(index, HostAddr(v, const GLubyte *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLVERTEXATTRIB4DVARB(index, v) \
	GLint const size = 4; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, v, tmp); \
	fn.glVertexAttrib4dvARB(index, tmp)
#else
#define FN_GLVERTEXATTRIB4DVARB(index, v) \
	fn.glVertexAttrib4dvARB(index, HostAddr(v, const GLdouble *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLVERTEXATTRIB4FVARB(index, v) \
	GLint const size = 4; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, v, tmp); \
	fn.glVertexAttrib4fvARB(index, tmp)
#else
#define FN_GLVERTEXATTRIB4FVARB(index, v) \
	fn.glVertexAttrib4fvARB(index, HostAddr(v, const GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIB4IVARB(index, v) \
	GLint const size = 4; \
	GLint tmp[size]; \
	Atari2HostIntArray(size, v, tmp); \
	fn.glVertexAttrib4ivARB(index, tmp)
#else
#define FN_GLVERTEXATTRIB4IVARB(index, v) \
	fn.glVertexAttrib4ivARB(index, HostAddr(v, const GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIB4SVARB(index, v) \
	GLint const size = 4; \
	GLshort tmp[size]; \
	Atari2HostShortArray(size, v, tmp); \
	fn.glVertexAttrib4svARB(index, tmp)
#else
#define FN_GLVERTEXATTRIB4SVARB(index, v) \
	fn.glVertexAttrib4svARB(index, HostAddr(v, const GLshort *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIB4UIVARB(index, v) \
	GLint const size = 4; \
	GLuint tmp[size]; \
	Atari2HostIntArray(size, v, tmp); \
	fn.glVertexAttrib4uivARB(index, tmp)
#else
#define FN_GLVERTEXATTRIB4UIVARB(index, v) \
	fn.glVertexAttrib4uivARB(index, HostAddr(v, const GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIB4USVARB(index, v) \
	GLint const size = 4; \
	GLushort tmp[size]; \
	Atari2HostShortArray(size, v, tmp); \
	fn.glVertexAttrib4usvARB(index, tmp)
#else
#define FN_GLVERTEXATTRIB4USVARB(index, v) \
	fn.glVertexAttrib4usvARB(index, HostAddr(v, const GLushort *))
#endif

/* TODO */
#define FN_GLGETVERTEXATTRIBPOINTERVARB(index, pname, pointer) \
	UNUSED(pointer); \
	void *p = 0; \
	fn.glGetVertexAttribPointervARB(index, pname, &p)

/* TODO */
#define FN_GLVERTEXATTRIBPOINTERARB(index, size, type, normalized, stride, pointer) \
	fn.glVertexAttribPointerARB(index, size, type, normalized, stride, HostAddr(pointer, const void *))

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLGETVERTEXATTRIBDVARB(index, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLdouble tmp[size]; \
	fn.glGetVertexAttribdvARB(index, pname, tmp); \
	Host2AtariDoubleArray(size, tmp, params)
#else
#define FN_GLGETVERTEXATTRIBDVARB(index, pname, params) \
	fn.glGetVertexAttribdvARB(index, pname, HostAddr(params, GLdouble *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETVERTEXATTRIBFVARB(index, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLfloat tmp[size]; \
	fn.glGetVertexAttribfvARB(index, pname, tmp); \
	Host2AtariFloatArray(size, tmp, params)
#else
#define FN_GLGETVERTEXATTRIBFVARB(index, pname, params) \
	fn.glGetVertexAttribfvARB(index, pname, HostAddr(params, GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETVERTEXATTRIBIVARB(index, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLint tmp[size]; \
	fn.glGetVertexAttribivARB(index, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETVERTEXATTRIBIVARB(index, pname, params) \
	fn.glGetVertexAttribivARB(index, pname, HostAddr(params, GLint *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_MESA_program_debug
 */
#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETPROGRAMREGISTERFVMESA(target, len, name, v) \
	GLint const size = 4; \
	GLfloat tmp[size]; \
	GLubyte namebuf[safe_strlen(name) + 1], *pname; \
	pname = Atari2HostByteArray(sizeof(namebuf), name, namebuf); \
	fn.glGetProgramRegisterfvMESA(target, len, pname, tmp); \
	Host2AtariFloatArray(size, tmp, v)
#else
#define FN_GLGETPROGRAMREGISTERFVMESA(target, len, name, v) \
	fn.glGetProgramRegisterfvMESA(target, len, HostAddr(name, const GLubyte *), HostAddr(v, GLfloat *))
#endif

/* seems to be no longer supported */
#define FN_GLPROGRAMCALLBACKMESA(target, callback, data) \
	UNUSED(target); \
	UNUSED(callback);\
	UNUSED(data); \
	glSetError(GL_INVALID_OPERATION)

/* -------------------------------------------------------------------------- */

/*
 * GL_MESA_shader_debug
 */
#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_INT_CONV
#define FN_GLGETDEBUGLOGMESA(obj, logType, shaderType, maxLength, length, debugLog) \
	GLsizei l; \
	GLcharARB tmp[MAX(maxLength, 0)]; \
	fn.glGetDebugLogMESA(obj, logType, shaderType, maxLength, &l, tmp); \
	Host2AtariByteArray(MIN(l + 1, maxLength), tmp, debugLog); \
	Host2AtariIntArray(1, &l, length)
#else
#define FN_GLGETDEBUGLOGMESA(obj, logType, shaderType, maxLength, length, debugLog) \
	fn.glGetDebugLogMESA(obj, logType, shaderType, maxLength, HostAddr(length, GLsizei *), HostAddr(debugLog, GLcharARB *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_EXT_debug_label
 */
#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_INT_CONV
#define FN_GLGETOBJECTLABELEXT(identifier, name, bufSize, length, label) \
	GLsizei l = 0; \
	GLchar labelbuf[MAX(bufSize, 0)]; \
	fn.glGetObjectLabelEXT(identifier, name, bufSize, &l, labelbuf); \
	Host2AtariIntArray(1, &l, length); \
	Host2AtariByteArray(MIN(l + 1, bufSize), labelbuf, label)
#else
#define FN_GLGETOBJECTLABELEXT(identifier, name, bufSize, length, label) \
	fn.glGetObjectLabelEXT(identifier, name, bufSize, HostAddr(length, GLsizei *), HostAddr(label, GLchar *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLLABELOBJECTEXT(type, object, length, label) \
	if (length <= 0) length = safe_strlen(label); \
	GLchar labelbuf[length], *plabel; \
	plabel = Atari2HostByteArray(length, label, labelbuf); \
	fn.glLabelObjectEXT(type, object, length, plabel)
#else
#define FN_GLLABELOBJECTEXT(type, object, length, label) \
	fn.glLabelObjectEXT(type, object, length, HostAddr(label, const GLchar *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_EXT_timer_query
 */
#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETQUERYOBJECTI64VEXT(id, pname, params) \
	GLint const size = 1; \
	GLint64 tmp[size]; \
	fn.glGetQueryObjecti64vEXT(id, pname, tmp); \
	Host2AtariInt64Array(size, tmp, params)
#else
#define FN_GLGETQUERYOBJECTI64VEXT(id, pname, params) \
	fn.glGetQueryObjecti64vEXT(id, pname, HostAddr(params, GLint64 *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETQUERYOBJECTUI64VEXT(id, pname, params) \
	GLint const size = 1; \
	GLuint64 tmp[size]; \
	fn.glGetQueryObjectui64vEXT(id, pname, tmp); \
	Host2AtariInt64Array(size, tmp, params)
#else
#define FN_GLGETQUERYOBJECTUI64VEXT(id, pname, params) \
	fn.glGetQueryObjectui64vEXT(id, pname, HostAddr(params, GLuint64 *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_ARB_occlusion_query
 */

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGENQUERIESARB(n, ids) \
	GLsizei const size = MAX(n, 0); \
	GLuint tmp[size]; \
	fn.glGenQueriesARB(n, tmp); \
	Host2AtariIntArray(n, tmp, ids)
#else
#define FN_GLGENQUERIESARB(n, ids) \
	fn.glGenQueriesARB(n, HostAddr(ids, GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLDELETEQUERIESARB(n, ids) \
	GLuint tmp[MAX(n, 0)], *ptmp; \
	ptmp = Atari2HostIntArray(n, ids, tmp); \
	fn.glDeleteQueriesARB(n, ptmp)
#else
#define FN_GLDELETEQUERIESARB(n, ids) \
	fn.glDeleteQueriesARB(n, HostAddr(ids, const GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETQUERYIVARB(target, pname, params) \
	GLint const size = 1; \
	GLint tmp[size]; \
	fn.glGetQueryivARB(target, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETQUERYIVARB(target, pname, params) \
	fn.glGetQueryivARB(target, pname, HostAddr(params, GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETQUERYOBJECTIVARB(id, pname, params) \
	GLint const size = 1; \
	GLint tmp[size]; \
	fn.glGetQueryObjectivARB(id, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETQUERYOBJECTIVARB(id, pname, params) \
	fn.glGetQueryObjectivARB(id, pname, HostAddr(params, GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETQUERYOBJECTUIVARB(id, pname, params) \
	GLint const size = 1; \
	GLuint tmp[size]; \
	fn.glGetQueryObjectuivARB(id, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETQUERYOBJECTUIVARB(id, pname, params) \
	fn.glGetQueryObjectuivARB(id, pname, HostAddr(params, GLuint *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_EXT_convolution
 */

#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLCONVOLUTIONFILTER1DEXT(target, internalformat, width, format, type, image) \
	pixelBuffer buf(*this); \
	void *tmp = buf.convertPixels(width, 1, 1, format, type, (nfmemptr)image); \
	fn.glConvolutionFilter1DEXT(target, internalformat, width, format, type, tmp)
#else
#define FN_GLCONVOLUTIONFILTER1DEXT(target, internalformat, width, format, type, image) \
	fn.glConvolutionFilter1DEXT(target, internalformat, width, format, type, HostAddr(image, const void *))
#endif

#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLCONVOLUTIONFILTER2DEXT(target, internalformat, width, height, format, type, image) \
	pixelBuffer buf(*this); \
	void *tmp = buf.convertPixels(width, height, 1, format, type, (nfmemptr)image); \
	fn.glConvolutionFilter2DEXT(target, internalformat, width, height, format, type, tmp)
#else
#define FN_GLCONVOLUTIONFILTER2DEXT(target, internalformat, width, height, format, type, image) \
	fn.glConvolutionFilter2DEXT(target, internalformat, width, height, format, type, HostAddr(image, const void *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLCONVOLUTIONPARAMETERFVEXT(target, pname, params) \
	int const size = nfglGetNumParams(pname); \
	GLfloat tmp[MAX(size, 16)], *ptmp; \
	ptmp = Atari2HostFloatArray(size, params, tmp); \
	fn.glConvolutionParameterfvEXT(target, pname, ptmp)
#else
#define FN_GLCONVOLUTIONPARAMETERFVEXT(target, pname, params) \
	fn.glConvolutionParameterfvEXT(target, pname, HostAddr(params, const GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLCONVOLUTIONPARAMETERIVEXT(target, pname, params) \
	int const size = nfglGetNumParams(pname); \
	GLint tmp[size], *ptmp; \
	ptmp = Atari2HostIntArray(size, params, tmp); \
	fn.glConvolutionParameterivEXT(target, pname, ptmp)
#else
#define FN_GLCONVOLUTIONPARAMETERIVEXT(target, pname, params) \
	fn.glConvolutionParameterivEXT(target, pname, HostAddr(params, const GLint *))
#endif

#define FN_GLGETCONVOLUTIONFILTEREXT(target, format, type, image) \
	GLint width = 0; \
	GLint height = 0; \
	char *src; \
	nfmemptr dst; \
	pixelBuffer buf(*this); \
	 \
	fn.glGetConvolutionParameteriv(target, GL_CONVOLUTION_WIDTH, &width); \
	fn.glGetConvolutionParameteriv(target, GL_CONVOLUTION_HEIGHT, &height); \
	if (contexts[cur_context].buffer_bindings.pixel_pack.id) { \
		void *offset = NFHost2AtariAddr(image); \
		fn.glGetConvolutionFilterEXT(target, format, type, offset); \
		src = contexts[cur_context].buffer_bindings.pixel_pack.host_pointer + (uintptr_t)offset; \
		dst = contexts[cur_context].buffer_bindings.pixel_pack.atari_pointer + (uintptr_t)offset; \
		if (!buf.params(width, height, 1, format, type)) return; \
	} else { \
		src = buf.hostBuffer(width, height, 1, format, type, (nfmemptr)image); \
		if (src == NULL) return; \
		fn.glGetConvolutionFilterEXT(target, format, type, src); \
		dst = (nfmemptr)image; \
	} \
	buf.convertToAtari(src, dst)

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETCONVOLUTIONPARAMETERFVEXT(target, pname, params) \
	int const size = nfglGetNumParams(pname); \
	GLfloat tmp[MAX(size, 16)]; \
	fn.glGetConvolutionParameterfvEXT(target, pname, tmp); \
	Host2AtariFloatArray(size, tmp, params)
#else
#define FN_GLGETCONVOLUTIONPARAMETERFVEXT(target, pname, params) \
	fn.glGetConvolutionParameterfvEXT(target, pname, HostAddr(params, GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETCONVOLUTIONPARAMETERIVEXT(target, pname, params) \
	int const size = nfglGetNumParams(pname); \
	GLint tmp[MAX(size, 16)]; \
	fn.glGetConvolutionParameterivEXT(target, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETCONVOLUTIONPARAMETERIVEXT(target, pname, params) \
	fn.glGetConvolutionParameterivEXT(target, pname, HostAddr(params, GLint *))
#endif

#define FN_GLGETSEPARABLEFILTEREXT(target, format, type, row, column, span) \
	GLint width = 0; \
	GLint height = 0; \
	pixelBuffer rowbuf(*this); \
	pixelBuffer colbuf(*this); \
	char *srcrow; \
	nfmemptr dstrow; \
	char *srccol; \
	nfmemptr dstcol; \
	 \
	fn.glGetConvolutionParameteriv(target, GL_CONVOLUTION_WIDTH, &width); \
	fn.glGetConvolutionParameteriv(target, GL_CONVOLUTION_HEIGHT, &height); \
	if (contexts[cur_context].buffer_bindings.pixel_pack.id) { \
		void *rowoffset = NFHost2AtariAddr(row); \
		void *coloffset = NFHost2AtariAddr(column); \
		fn.glGetSeparableFilterEXT(target, format, type, rowoffset, coloffset, HostAddr(span, void *)); \
		srcrow = contexts[cur_context].buffer_bindings.pixel_pack.host_pointer + (uintptr_t)rowoffset; \
		srccol = contexts[cur_context].buffer_bindings.pixel_pack.host_pointer + (uintptr_t)coloffset; \
		dstrow = contexts[cur_context].buffer_bindings.pixel_pack.atari_pointer + (uintptr_t)rowoffset; \
		dstcol = contexts[cur_context].buffer_bindings.pixel_pack.atari_pointer + (uintptr_t)coloffset; \
		if (!rowbuf.params(width, 1, 1, format, type)) return; \
		if (!colbuf.params(1, height, 1, format, type)) return; \
	} else { \
		srcrow = rowbuf.hostBuffer(width, 1, 1, format, type, (nfmemptr)row); \
		srccol = colbuf.hostBuffer(1, height, 1, format, type, (nfmemptr)column); \
		if (srcrow == NULL || srccol == NULL) return; \
		fn.glGetSeparableFilterEXT(target, format, type, srcrow, srccol, HostAddr(span, void *)); \
		dstrow = (nfmemptr)row; \
		dstcol = (nfmemptr)column; \
	} \
	rowbuf.convertToAtari(srcrow, dstrow); \
	colbuf.convertToAtari(srccol, dstcol)

#define FN_GLSEPARABLEFILTER2DEXT(target, internalformat, width, height, format, type, row, column) \
	pixelBuffer rowbuf(*this); \
	pixelBuffer colbuf(*this); \
	void *tmprowbuf, *tmpcolbuf; \
	 \
	if (contexts[cur_context].buffer_bindings.pixel_unpack.id) { \
		void *rowoffset = NFHost2AtariAddr(row); \
		void *coloffset = NFHost2AtariAddr(column); \
		tmprowbuf = rowbuf.convertPixels(width, 1, 1, format, type, contexts[cur_context].buffer_bindings.pixel_pack.atari_pointer + (uintptr_t)rowoffset); \
		tmpcolbuf = colbuf.convertPixels(1, height, 1, format, type, contexts[cur_context].buffer_bindings.pixel_pack.atari_pointer + (uintptr_t)coloffset); \
	} else { \
		tmprowbuf = rowbuf.convertPixels(width, 1, 1, format, type, (nfmemptr)row); \
		tmpcolbuf = colbuf.convertPixels(1, height, 1, format, type, (nfmemptr)column); \
	} \
	fn.glSeparableFilter2DEXT(target, internalformat, width, height, format, type, tmprowbuf, tmpcolbuf)

/* -------------------------------------------------------------------------- */

/*
 * GL_EXT_histogram
 */

#define FN_GLGETHISTOGRAMEXT(target, reset, format, type, values) \
	GLint width = 0; \
	char *src; \
	nfmemptr dst; \
	pixelBuffer buf(*this); \
	 \
	fn.glGetHistogramParameteriv(target, GL_HISTOGRAM_WIDTH, &width); \
	if (contexts[cur_context].buffer_bindings.pixel_pack.id) { \
		void *offset = NFHost2AtariAddr(values); \
		fn.glGetHistogramEXT(target, reset, format, type, offset); \
		src = contexts[cur_context].buffer_bindings.pixel_pack.host_pointer + (uintptr_t)offset; \
		dst = contexts[cur_context].buffer_bindings.pixel_pack.atari_pointer + (uintptr_t)offset; \
		if (!buf.params(width, 1, 1, format, type)) return; \
	} else { \
		src = buf.hostBuffer(width, 1, 1, format, type, (nfmemptr)values); \
		if (src == NULL) return; \
		fn.glGetHistogramEXT(target, reset, format, type, src); \
		dst = (nfmemptr)values; \
	} \
	buf.convertToAtari(src, dst)

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETHISTOGRAMPARAMETERFVEXT(target, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLfloat tmp[MAX(size, 16)]; \
	fn.glGetHistogramParameterfvEXT(target, pname, tmp); \
	Host2AtariFloatArray(size, tmp, params)
#else
#define FN_GLGETHISTOGRAMPARAMETERFVEXT(target, pname, params) \
	fn.glGetHistogramParameterfvEXT(target, pname, HostAddr(params, GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETHISTOGRAMPARAMETERIVEXT(target, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLint tmp[MAX(size, 16)]; \
	fn.glGetHistogramParameterivEXT(target, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETHISTOGRAMPARAMETERIVEXT(target, pname, params) \
	fn.glGetHistogramParameterivEXT(target, pname, HostAddr(params, GLint *))
#endif

/*
 * If a non-zero named buffer object is bound to the GL_PIXEL_PACK_BUFFER
 * target (see glBindBuffer) while minimum and maximum pixel values are
 * requested, values is treated as a byte offset into the buffer object's
 * data store.
 */
#define FN_GLGETMINMAXEXT(target, reset, format, type, values) \
	GLint const width = 2; \
	GLint const height = 4; \
	char result[width * height * sizeof(GLdouble)]; \
	char *src; \
	nfmemptr dst; \
	pixelBuffer buf(*this); \
	 \
	if (!buf.params(width, height, 1, format, type)) \
		return; \
	if (contexts[cur_context].buffer_bindings.pixel_pack.id) { \
		void *offset = NFHost2AtariAddr(values); \
		fn.glGetMinmaxEXT(target, reset, format, type, offset); \
		src = contexts[cur_context].buffer_bindings.pixel_pack.host_pointer + (uintptr_t)offset; \
		dst = contexts[cur_context].buffer_bindings.pixel_pack.atari_pointer + (uintptr_t)offset; \
	} else { \
		src = result; \
		fn.glGetMinmaxEXT(target, reset, format, type, src); \
		dst = (nfmemptr)values; \
	} \
	buf.convertToAtari(src, dst)

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETMINMAXPARAMETERFVEXT(target, pname, params) \
	GLint const size = 1; \
	GLfloat tmp[MAX(size, 16)]; \
	fn.glGetMinmaxParameterfvEXT(target, pname, tmp); \
	Host2AtariFloatArray(size, tmp, params)
#else
#define FN_GLGETMINMAXPARAMETERFVEXT(target, pname, params) \
	fn.glGetMinmaxParameterfvEXT(target, pname, HostAddr(params, GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETMINMAXPARAMETERIVEXT(target, pname, params) \
	GLint const size = 1; \
	GLint tmp[MAX(size, 16)]; \
	fn.glGetMinmaxParameterivEXT(target, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETMINMAXPARAMETERIVEXT(target, pname, params) \
	fn.glGetMinmaxParameterivEXT(target, pname, HostAddr(params, GLint *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_ARB_texture_compression
 */

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLCOMPRESSEDTEXIMAGE1DARB(target, level, internalformat, width, border, imageSize, data) \
	GLubyte tmp[MAX(imageSize, 0)], *ptmp; \
	ptmp = Atari2HostByteArray(sizeof(tmp), data, tmp); \
	fn.glCompressedTexImage1DARB(target, level, internalformat, width, border, imageSize, ptmp)
#else
#define FN_GLCOMPRESSEDTEXIMAGE1DARB(target, level, internalformat, width, border, imageSize, data) \
	fn.glCompressedTexImage1DARB(target, level, internalformat, width, border, imageSize, HostAddr(data, const void *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLCOMPRESSEDTEXIMAGE2DARB(target, level, internalformat, width, height, border, imageSize, data) \
	GLubyte tmp[MAX(imageSize, 0)], *ptmp; \
	ptmp = Atari2HostByteArray(sizeof(tmp), data, tmp); \
	fn.glCompressedTexImage2DARB(target, level, internalformat, width, height, border, imageSize, ptmp)
#else
#define FN_GLCOMPRESSEDTEXIMAGE2DARB(target, level, internalformat, width, height, border, imageSize, data) \
	fn.glCompressedTexImage2DARB(target, level, internalformat, width, height, border, imageSize, HostAddr(data, const void *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLCOMPRESSEDTEXIMAGE3DARB(target, level, internalformat, width, height, depth, border, imageSize, data) \
	GLubyte tmp[MAX(imageSize, 0)], *ptmp; \
	ptmp = Atari2HostByteArray(sizeof(tmp), data, tmp); \
	fn.glCompressedTexImage3DARB(target, level, internalformat, width, height, depth, border, imageSize, ptmp)
#else
#define FN_GLCOMPRESSEDTEXIMAGE3DARB(target, level, internalformat, width, height, depth, border, imageSize, data) \
	fn.glCompressedTexImage3DARB(target, level, internalformat, width, height, depth, border, imageSize, HostAddr(data, const void *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLCOMPRESSEDTEXSUBIMAGE1DARB(target, level, xoffset, width, format, imageSize, data) \
	GLubyte tmp[MAX(imageSize, 0)], *ptmp; \
	ptmp = Atari2HostByteArray(sizeof(tmp), data, tmp); \
	fn.glCompressedTexSubImage1DARB(target, level, xoffset, width, format, imageSize, ptmp)
#else
#define FN_GLCOMPRESSEDTEXSUBIMAGE1DARB(target, level, xoffset, width, format, imageSize, data) \
	fn.glCompressedTexSubImage1DARB(target, level, xoffset, width, format, imageSize, HostAddr(data, const void *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLCOMPRESSEDTEXSUBIMAGE2DARB(target, level, xoffset, yoffset, width, height, format, imageSize, data) \
	GLubyte tmp[MAX(imageSize, 0)], *ptmp; \
	ptmp = Atari2HostByteArray(sizeof(tmp), data, tmp); \
	fn.glCompressedTexSubImage2DARB(target, level, xoffset, yoffset, width, height, format, imageSize, ptmp)
#else
#define FN_GLCOMPRESSEDTEXSUBIMAGE2DARB(target, level, xoffset, yoffset, width, height, format, imageSize, data) \
	fn.glCompressedTexSubImage2DARB(target, level, xoffset, yoffset, width, height, format, imageSize, HostAddr(data, const void *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLCOMPRESSEDTEXSUBIMAGE3DARB(target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data) \
	GLubyte tmp[MAX(imageSize, 0)], *ptmp; \
	ptmp = Atari2HostByteArray(sizeof(tmp), data, tmp); \
	fn.glCompressedTexSubImage3DARB(target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, ptmp)
#else
#define FN_GLCOMPRESSEDTEXSUBIMAGE3DARB(target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data) \
	fn.glCompressedTexSubImage3DARB(target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, HostAddr(data, const void *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLGETCOMPRESSEDTEXIMAGEARB(target, level, img) \
	GLint bufSize = 0; \
	fn.glGetTexLevelParameteriv(target, level, GL_TEXTURE_COMPRESSED_IMAGE_SIZE, &bufSize); \
	GLubyte tmp[bufSize]; \
	fn.glGetCompressedTexImageARB(target, level, tmp); \
	Host2AtariByteArray(bufSize, tmp, img)
#else
#define FN_GLGETCOMPRESSEDTEXIMAGEARB(target, level, img) \
	fn.glGetCompressedTexImageARB(target, level, HostAddr(img, void *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_SGIS_sharpen_texture
 */

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETSHARPENTEXFUNCSGIS(target, points) \
	GLint size = 0; \
	fn.glGetTexParameteriv(target, GL_SHARPEN_TEXTURE_FUNC_POINTS_SGIS, &size); \
	if (size <= 0) return; \
	GLfloat tmp[size]; \
	fn.glGetSharpenTexFuncSGIS(target, tmp); \
	Host2AtariFloatArray(size, tmp, points)
#else
#define FN_GLGETSHARPENTEXFUNCSGIS(target, points) \
	fn.glGetSharpenTexFuncSGIS(target, HostAddr(points, GLfloat *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLSHARPENTEXFUNCSGIS(target, n, points) \
	GLint const size = n; \
	if (size <= 0) return; \
	GLfloat tmp[size], *ptmp; \
	ptmp = Atari2HostFloatArray(size, points, tmp); \
	fn.glSharpenTexFuncSGIS(target, size, ptmp)
#else
#define FN_GLSHARPENTEXFUNCSGIS(target, n, points) \
	fn.glSharpenTexFuncSGIS(target, n, HostAddr(points, const GLfloat *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_ATI_envmap_bumpmap
 */
#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETTEXBUMPPARAMETERFVATI(pname, param) \
	GLint size = 0; \
	switch (pname) { \
		case GL_BUMP_ROT_MATRIX_SIZE_ATI: size = 1; break; \
		case GL_BUMP_ROT_MATRIX_ATI: fn.glGetTexBumpParameterivATI(GL_BUMP_ROT_MATRIX_SIZE_ATI, &size); break; \
		case GL_BUMP_NUM_TEX_UNITS_ATI: size = 1; break; \
		case GL_BUMP_TEX_UNITS_ATI: fn.glGetTexBumpParameterivATI(GL_BUMP_NUM_TEX_UNITS_ATI, &size); break; \
		default: glSetError(GL_INVALID_ENUM); return; \
	} \
	if (size <= 0) return; \
	GLfloat tmp[size]; \
	fn.glGetTexBumpParameterfvATI(pname, tmp); \
	Host2AtariFloatArray(size, tmp, param)
#else
#define FN_GLGETTEXBUMPPARAMETERFVATI(pname, param) \
	fn.glGetTexBumpParameterfvATI(pname, HostAddr(param, GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETTEXBUMPPARAMETERIVATI(pname, param) \
	GLint size = 0; \
	switch (pname) { \
		case GL_BUMP_ROT_MATRIX_SIZE_ATI: size = 1; break; \
		case GL_BUMP_ROT_MATRIX_ATI: fn.glGetTexBumpParameterivATI(GL_BUMP_ROT_MATRIX_SIZE_ATI, &size); break; \
		case GL_BUMP_NUM_TEX_UNITS_ATI: size = 1; break; \
		case GL_BUMP_TEX_UNITS_ATI: fn.glGetTexBumpParameterivATI(GL_BUMP_NUM_TEX_UNITS_ATI, &size); break; \
		default: glSetError(GL_INVALID_ENUM); return; \
	} \
	if (size <= 0) return; \
	GLint tmp[size]; \
	fn.glGetTexBumpParameterivATI(pname, tmp); \
	Host2AtariIntArray(size, tmp, param)
#else
#define FN_GLGETTEXBUMPPARAMETERIVATI(pname, param) \
	fn.glGetTexBumpParameterivATI(pname, HostAddr(param, GLint *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLTEXBUMPPARAMETERIVATI(pname, param) \
	GLint size = 0; \
	switch (pname) { \
		case GL_BUMP_ROT_MATRIX_SIZE_ATI: size = 1; break; \
		case GL_BUMP_ROT_MATRIX_ATI: fn.glGetTexBumpParameterivATI(GL_BUMP_ROT_MATRIX_SIZE_ATI, &size); break; \
		case GL_BUMP_NUM_TEX_UNITS_ATI: size = 1; break; \
		case GL_BUMP_TEX_UNITS_ATI: fn.glGetTexBumpParameterivATI(GL_BUMP_NUM_TEX_UNITS_ATI, &size); break; \
		default: glSetError(GL_INVALID_ENUM); return; \
	} \
	if (size <= 0) return; \
	GLint tmp[size], *ptmp; \
	ptmp = Atari2HostIntArray(size, param, tmp); \
	fn.glTexBumpParameterivATI(pname, ptmp)
#else
#define FN_GLTEXBUMPPARAMETERIVATI(pname, param) \
	fn.glTexBumpParameterivATI(pname, HostAddr(param, const GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLTEXBUMPPARAMETERFVATI(pname, param) \
	GLint size = 0; \
	switch (pname) { \
		case GL_BUMP_ROT_MATRIX_SIZE_ATI: size = 1; break; \
		case GL_BUMP_ROT_MATRIX_ATI: fn.glGetTexBumpParameterivATI(GL_BUMP_ROT_MATRIX_SIZE_ATI, &size); break; \
		case GL_BUMP_NUM_TEX_UNITS_ATI: size = 1; break; \
		case GL_BUMP_TEX_UNITS_ATI: fn.glGetTexBumpParameterivATI(GL_BUMP_NUM_TEX_UNITS_ATI, &size); break; \
		default: glSetError(GL_INVALID_ENUM); return; \
	} \
	if (size <= 0) return; \
	GLfloat tmp[size], *ptmp; \
	ptmp = Atari2HostFloatArray(size, param, tmp); \
	fn.glTexBumpParameterfvATI(pname, ptmp)
#else
#define FN_GLTEXBUMPPARAMETERFVATI(pname, param) \
	fn.glTexBumpParameterfvATI(pname, HostAddr(param, const GLfloat *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_SGIS_texture_filter4
 */
#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETTEXFILTERFUNCSGIS(target, filter, weights) \
	GLint size = 0; \
	fn.glGetTexParameteriv(target, GL_TEXTURE_FILTER4_SIZE_SGIS, &size); \
	if (size <= 0) return; \
	GLfloat tmp[size]; \
	fn.glGetTexFilterFuncSGIS(target, filter, tmp); \
	Host2AtariFloatArray(size, tmp, weights)
#else
#define FN_GLGETTEXFILTERFUNCSGIS(target, filter, weights) \
	fn.glGetTexFilterFuncSGIS(target, filter, HostAddr(weights, GLfloat *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLTEXFILTERFUNCSGIS(target, filter, n, weights) \
	GLint const size = n; \
	if (size <= 0) return; \
	GLfloat tmp[size], *ptmp; \
	ptmp = Atari2HostFloatArray(size, weights, tmp); \
	fn.glTexFilterFuncSGIS(target, filter, n, ptmp)
#else
#define FN_GLTEXFILTERFUNCSGIS(target, filter, n, weights) \
	fn.glTexFilterFuncSGIS(target, filter, n, HostAddr(weights, const GLfloat *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_EXT_subtexture
 */

#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLTEXSUBIMAGE1DEXT(target, level, xoffset, width, format, type, pixels) \
	pixelBuffer buf(*this); \
	void *tmp = buf.convertPixels(width, 1, 1, format, type, (nfmemptr)pixels); \
	fn.glTexSubImage1DEXT(target, level, xoffset, width, format, type, tmp)
#else
#define FN_GLTEXSUBIMAGE1DEXT(target, level, xoffset, width, format, type, pixels) \
	fn.glTexSubImage1DEXT(target, level, xoffset, width, format, type, HostAddr(pixels, const void *))
#endif

#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLTEXSUBIMAGE2DEXT(target, level, xoffset, yoffset, width, height, format, type, pixels) \
	pixelBuffer buf(*this); \
	void *tmp = buf.convertPixels(width, height, 1, format, type, (nfmemptr)pixels); \
	fn.glTexSubImage2DEXT(target, level, xoffset, yoffset, width, height, format, type, tmp)
#else
#define FN_GLTEXSUBIMAGE2DEXT(target, level, xoffset, yoffset, width, height, format, type, pixels) \
	fn.glTexSubImage2DEXT(target, level, xoffset, yoffset, width, height, format, type, HostAddr(pixels, const void *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_EXT_texture_object
 */

#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_INT_CONV
#define FN_GLARETEXTURESRESIDENTEXT(n, textures, residences) \
	GLuint tmp[MAX(n, 0)], *ptmp; \
	GLboolean resbuf[MAX(n, 0)]; \
	GLboolean result; \
	ptmp = Atari2HostIntArray(n, textures, tmp); \
	result = fn.glAreTexturesResidentEXT(n, ptmp, resbuf); \
	Host2AtariByteArray(n, resbuf, residences); \
	return result
#else
#define FN_GLARETEXTURESRESIDENTEXT(n, textures, residences) \
	return fn.glAreTexturesResidentEXT(n, HostAddr(textures, const GLuint *), HostAddr(residences, GLboolean *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLDELETETEXTURESEXT(n, textures) \
	GLuint tmp[MAX(n, 0)], *ptmp; \
	ptmp = Atari2HostIntArray(n, textures, tmp); \
	fn.glDeleteTexturesEXT(n, ptmp)
#else
#define FN_GLDELETETEXTURESEXT(n, textures) \
	fn.glDeleteTexturesEXT(n, HostAddr(textures, const GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGENTEXTURESEXT(n, textures) \
	GLsizei const size = MAX(n, 0); \
	GLuint tmp[size]; \
	fn.glGenTexturesEXT(n, tmp); \
	Host2AtariIntArray(n, tmp, textures)
#else
#define FN_GLGENTEXTURESEXT(n, textures) \
	fn.glGenTexturesEXT(n, HostAddr(textures, GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPRIORITIZETEXTURESEXT(n, textures, priorities) \
	GLuint tmp[MAX(n, 0)], *ptmp; \
	GLclampf tmp2[MAX(n, 0)], *ptmp2; \
	ptmp = Atari2HostIntArray(n, textures, tmp); \
	ptmp2 = Atari2HostFloatArray(n, priorities, tmp2); \
	fn.glPrioritizeTexturesEXT(n, ptmp, ptmp2)
#else
#define FN_GLPRIORITIZETEXTURESEXT(n, textures, priorities) \
	fn.glPrioritizeTexturesEXT(n, HostAddr(textures, const GLuint *), HostAddr(priorities, const GLclampf *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_EXT_texture3D
 */

#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLTEXIMAGE3DEXT(target, level, internalformat, width, height, depth, border, format, type, pixels) \
	pixelBuffer buf(*this); \
	void *tmp = buf.convertPixels(width, height, depth, format, type, (nfmemptr)pixels); \
	fn.glTexImage3DEXT(target, level, internalformat, width, height, depth, border, format, type, tmp)
#else
#define FN_GLTEXIMAGE3DEXT(target, level, internalformat, width, height, depth, border, format, type, pixels) \
	fn.glTexImage3DEXT(target, level, internalformat, width, height, depth, border, format, type, HostAddr(pixels, const void *))
#endif

#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLTEXSUBIMAGE3DEXT(target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels) \
	pixelBuffer buf(*this); \
	void *tmp = buf.convertPixels(width, height, depth, format, type, (nfmemptr)pixels); \
	fn.glTexSubImage3DEXT(target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, tmp)
#else
#define FN_GLTEXSUBIMAGE3DEXT(target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels) \
	fn.glTexSubImage3DEXT(target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, HostAddr(pixels, const void *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_SGIS_texture4D
 */

#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLTEXIMAGE4DSGIS(target, level, internalformat, width, height, depth, size4d, border, format, type, pixels) \
	pixelBuffer buf(*this); \
	void *tmp = buf.convertPixels(width, height, depth * size4d, format, type, (nfmemptr)pixels); \
	fn.glTexImage4DSGIS(target, level, internalformat, width, height, depth, size4d, border, format, type, tmp)
#else
#define FN_GLTEXIMAGE4DSGIS(target, level, internalformat, width, height, depth, size4d, border, format, type, pixels) \
	fn.glTexImage4DSGIS(target, level, internalformat, width, height, depth, size4d, border, format, type, HostAddr(pixels, const void *))
#endif

#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLTEXSUBIMAGE4DSGIS(target, level, xoffset, yoffset, zoffset, woffset, width, height, depth, size4d, format, type, pixels) \
	pixelBuffer buf(*this); \
	void *tmp = buf.convertPixels(width, height, depth * size4d, format, type, (nfmemptr)pixels); \
	fn.glTexSubImage4DSGIS(target, level, xoffset, yoffset, zoffset, woffset, width, height, depth, size4d, format, type, tmp)
#else
#define FN_GLTEXSUBIMAGE4DSGIS(target, level, xoffset, yoffset, zoffset, woffset, width, height, depth, size4d, format, type, pixels) \
	fn.glTexSubImage4DSGIS(target, level, xoffset, yoffset, zoffset, woffset, width, height, depth, size4d, format, type, HostAddr(pixels, const void *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_EXT_texture_integer
 */
#if NFOSMESA_NEED_INT_CONV
#define FN_GLTEXPARAMETERIIVEXT(target, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLint tmp[MAX(size, 16)]; \
	Atari2HostIntArray(size, params, tmp); \
	fn.glTexParameterIivEXT(target, pname, tmp)
#else
#define FN_GLTEXPARAMETERIIVEXT(target, pname, params) \
	fn.glTexParameterIivEXT(target, pname, HostAddr(params, const GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLTEXPARAMETERIUIVEXT(target, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLuint tmp[MAX(size, 16)]; \
	Atari2HostIntArray(size, params, tmp); \
	fn.glTexParameterIuivEXT(target, pname, tmp)
#else
#define FN_GLTEXPARAMETERIUIVEXT(target, pname, params) \
	fn.glTexParameterIuivEXT(target, pname, HostAddr(params, const GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETTEXPARAMETERIIVEXT(target, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLint tmp[MAX(size, 16)]; \
	fn.glGetTexParameterIivEXT(target, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETTEXPARAMETERIIVEXT(target, pname, params) \
	fn.glGetTexParameterIivEXT(target, pname, HostAddr(params, GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETTEXPARAMETERIUIVEXT(target, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLuint tmp[MAX(size, 16)]; \
	fn.glGetTexParameterIuivEXT(target, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETTEXPARAMETERIUIVEXT(target, pname, params) \
	fn.glGetTexParameterIuivEXT(target, pname, HostAddr(params, GLuint *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_APPLE_texture_range
 */
/* NYI */
#define FN_GLTEXTURERANGEAPPLE(target, length, pointer) \
	fn.glTextureRangeAPPLE(target, length, pointer)
#define FN_GLGETTEXPARAMETERPOINTERVAPPLE(target, pname, params) \
	fn.glGetTexParameterPointervAPPLE(target, pname, params)
	
/* -------------------------------------------------------------------------- */

/*
 * GL_EXT_transform_feedback
 */
#define FN_GLBINDBUFFERRANGEEXT(target, index, buffer, offset, size) \
	gl_bind_buffer(target, buffer, 0, 0); \
	fn.glBindBufferRangeEXT(target, index, buffer, offset, size)

#define FN_GLBINDBUFFERBASEEXT(target, index, buffer) \
	gl_bind_buffer(target, buffer, 0, 0); \
	fn.glBindBufferBaseEXT(target, index, buffer)

#define FN_GLTRANSFORMFEEDBACKVARYINGSEXT(program, count, strings, bufferMode) \
	void *pstrings[MAX(count, 0)], **ppstrings; \
	ppstrings = Atari2HostPtrArray(count, strings, pstrings); \
	fn.glTransformFeedbackVaryingsEXT(program, count, (const char **)ppstrings, bufferMode)

#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_INT_CONV
#define FN_GLGETTRANSFORMFEEDBACKVARYINGEXT(program, index, bufSize, length, size, type, name) \
	GLsizei l = 0; \
	GLsizei s = 0; \
	GLenum t = 0; \
	GLchar namebuf[MAX(bufSize, 0)]; \
	fn.glGetTransformFeedbackVaryingEXT(program, index, bufSize, &l, &s, &t, namebuf); \
	Host2AtariByteArray(MIN(l + 1, bufSize), namebuf, name); \
	Host2AtariIntArray(1, &l, length); \
	Host2AtariIntArray(1, &s, size); \
	Host2AtariIntArray(1, &t, type)
#else
#define FN_GLGETTRANSFORMFEEDBACKVARYINGEXT(program, index, bufSize, length, size, type, name) \
	fn.glGetTransformFeedbackVaryingEXT(program, index, bufSize, HostAddr(length, GLsizei *), HostAddr(size, GLsizei *), HostAddr(type, GLenum *), HostAddr(name, GLchar *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_NV_transform_feedback
 */

#define FN_GLBINDBUFFERRANGENV(target, index, buffer, offset, size) \
	gl_bind_buffer(target, buffer, 0, 0); \
	fn.glBindBufferRangeNV(target, index, buffer, offset, size)

#define FN_GLBINDBUFFERBASENV(target, index, buffer) \
	gl_bind_buffer(target, buffer, 0, 0); \
	fn.glBindBufferBaseNV(target, index, buffer)

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLACTIVEVARYINGNV(program, name) \
	GLchar tmp[safe_strlen(name) + 1], *ptmp; \
	ptmp = Atari2HostByteArray(sizeof(tmp), name, tmp); \
	fn.glActiveVaryingNV(program, ptmp)
#else
#define FN_GLACTIVEVARYINGNV(program, name) \
	fn.glActiveVaryingNV(program, HostAddr(name, const GLchar *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLGETVARYINGLOCATIONNV(program, name) \
	GLchar tmp[safe_strlen(name) + 1], *ptmp; \
	ptmp = Atari2HostByteArray(sizeof(tmp), name, tmp); \
	return fn.glGetVaryingLocationNV(program, ptmp)
#else
#define FN_GLGETVARYINGLOCATIONNV(program, name) \
	return fn.glGetVaryingLocationNV(program, HostAddr(name, const GLchar *))
#endif

#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_INT_CONV
#define FN_GLGETACTIVEVARYINGNV(program, index, bufSize, length, size, type, name) \
	GLsizei l; \
	GLsizei s; \
	GLenum t; \
	GLchar namebuf[MAX(bufSize, 0)]; \
	fn.glGetActiveVaryingNV(program, index, bufSize, &l, &s, &t, namebuf); \
	Host2AtariByteArray(MIN(l + 1, bufSize), namebuf, name); \
	Host2AtariIntArray(1, &l, length); \
	Host2AtariIntArray(1, &s, size); \
	Host2AtariIntArray(1, &t, type)
#else
#define FN_GLGETACTIVEVARYINGNV(program, index, bufSize, length, size, type, name) \
	fn.glGetActiveVaryingNV(program, index, bufSize, HostAddr(length, GLsizei *), HostAddr(size, GLsizei *), HostAddr(type, GLenum *), HostAddr(name, GLchar *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLTRANSFORMFEEDBACKATTRIBSNV(count, attribs, bufferMode) \
	GLsizei const size = count; \
	GLint tmp[MAX(size, 0)], *ptmp; \
	ptmp = Atari2HostIntArray(size, attribs, tmp);  \
	fn.glTransformFeedbackAttribsNV(count, ptmp, bufferMode)
#else
#define FN_GLTRANSFORMFEEDBACKATTRIBSNV(count, attribs, bufferMode) \
	fn.glTransformFeedbackAttribsNV(count, HostAddr(attribs, const GLint *), bufferMode)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLTRANSFORMFEEDBACKVARYINGSNV(program, count, locations, bufferMode) \
	GLsizei const size = count; \
	GLint tmp[MAX(size, 0)], *ptmp; \
	ptmp = Atari2HostIntArray(size, locations, tmp); \
	fn.glTransformFeedbackVaryingsNV(program, count, ptmp, bufferMode)
#else
#define FN_GLTRANSFORMFEEDBACKVARYINGSNV(program, count, locations, bufferMode) \
	fn.glTransformFeedbackVaryingsNV(program, count, HostAddr(locations, const GLint *), bufferMode)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETTRANSFORMFEEDBACKVARYINGNV(program, index, location) \
	GLsizei l = 0; \
	fn.glGetTransformFeedbackVaryingNV(program, index, &l); \
	Host2AtariIntArray(1, &l, location)
#else
#define FN_GLGETTRANSFORMFEEDBACKVARYINGNV(program, index, location) \
	fn.glGetTransformFeedbackVaryingNV(program, index, HostAddr(location, GLsizei *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLTRANSFORMFEEDBACKSTREAMATTRIBSNV(count, attribs, nbuffers, bufstreams, bufferMode) \
	GLint a[MAX(count, 0)], *pattribs; \
	GLint b[MAX(nbuffers, 0)], *pbufstreams; \
	pattribs = Atari2HostIntArray(count, attribs, a); \
	pbufstreams = Atari2HostIntArray(nbuffers, bufstreams, b); \
	fn.glTransformFeedbackStreamAttribsNV(count, pattribs, nbuffers, pbufstreams, bufferMode)
#else
#define FN_GLTRANSFORMFEEDBACKSTREAMATTRIBSNV(count, attribs, nbuffers, bufstreams, bufferMode) \
	fn.glTransformFeedbackStreamAttribsNV(count, HostAddr(attribs, const GLint *), nbuffers, HostAddr(bufstreams, const GLint *), bufferMode)
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_NV_transform_feedback2
 */

#if NFOSMESA_NEED_INT_CONV
#define FN_GLDELETETRANSFORMFEEDBACKSNV(n, ids) \
	GLsizei const size = n; \
	if (size <= 0 || !ids) return; \
	GLuint tmp[size]; \
	Atari2HostIntArray(size, ids, tmp); \
	fn.glDeleteTransformFeedbacksNV(size, tmp)
#else
#define FN_GLDELETETRANSFORMFEEDBACKSNV(n, ids) \
	fn.glDeleteTransformFeedbacksNV(n, HostAddr(ids, const GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGENTRANSFORMFEEDBACKSNV(n, ids) \
	GLsizei const size = MAX(n, 0); \
	GLuint tmp[size]; \
	fn.glGenTransformFeedbacksNV(n, tmp); \
	Host2AtariIntArray(n, tmp, ids)
#else
#define FN_GLGENTRANSFORMFEEDBACKSNV(n, ids) \
	fn.glGenTransformFeedbacksNV(n, HostAddr(ids, GLuint *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_ARB_robustness
 */

#define FN_GLGETNTEXIMAGEARB(target, level, format, type, bufSize, img) \
	GLint width = 0, height = 1, depth = 1; \
	char *src; \
	nfmemptr dst; \
	pixelBuffer buf(*this); \
	 \
	switch (target) { \
	case GL_TEXTURE_1D: \
	case GL_PROXY_TEXTURE_1D: \
		fn.glGetTexLevelParameteriv(target, level, GL_TEXTURE_WIDTH, &width); \
		break; \
	case GL_TEXTURE_2D: \
	case GL_PROXY_TEXTURE_2D: \
	case GL_TEXTURE_CUBE_MAP_POSITIVE_X: \
	case GL_TEXTURE_CUBE_MAP_POSITIVE_Y: \
	case GL_TEXTURE_CUBE_MAP_POSITIVE_Z: \
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_X: \
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y: \
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z: \
	case GL_PROXY_TEXTURE_CUBE_MAP: \
		fn.glGetTexLevelParameteriv(target, level, GL_TEXTURE_WIDTH, &width); \
		fn.glGetTexLevelParameteriv(target, level, GL_TEXTURE_HEIGHT, &height); \
		break; \
	case GL_TEXTURE_3D: \
	case GL_PROXY_TEXTURE_3D: \
		fn.glGetTexLevelParameteriv(target, level, GL_TEXTURE_WIDTH, &width); \
		fn.glGetTexLevelParameteriv(target, level, GL_TEXTURE_HEIGHT, &height); \
		fn.glGetTexLevelParameteriv(target, level, GL_TEXTURE_DEPTH, &depth); \
		break; \
	default: \
		glSetError(GL_INVALID_ENUM); \
		return; \
	} \
	if (contexts[cur_context].buffer_bindings.pixel_pack.id) { \
		void *offset = NFHost2AtariAddr(img); \
		fn.glGetnTexImageARB(target, level, format, type, bufSize, offset); \
		src = contexts[cur_context].buffer_bindings.pixel_pack.host_pointer + (uintptr_t)offset; \
		dst = contexts[cur_context].buffer_bindings.pixel_pack.atari_pointer + (uintptr_t)offset; \
		if (!buf.params(width, height, depth, format, type)) return; \
	} else { \
		src = buf.hostBuffer(width, height, depth, format, type, (nfmemptr)img); \
		if (src == NULL) return; \
		fn.glGetnTexImageARB(target, level, format, type, bufSize, src); \
		dst = (nfmemptr)img; \
	} \
	buf.convertToAtari(src, dst)

#define FN_GLREADNPIXELSARB(x, y, width, height, format, type, bufSize, img) \
	char *src; \
	nfmemptr dst; \
	pixelBuffer buf(*this); \
	 \
	if (contexts[cur_context].buffer_bindings.pixel_pack.id) { \
		void *offset = NFHost2AtariAddr(img); \
		fn.glReadnPixelsARB(x, y, width, height, format, type, bufSize, offset); \
		src = contexts[cur_context].buffer_bindings.pixel_pack.host_pointer + (uintptr_t)offset; \
		dst = contexts[cur_context].buffer_bindings.pixel_pack.atari_pointer + (uintptr_t)offset; \
		if (!buf.params(bufSize, format, type)) return; \
	} else { \
		src = buf.hostBuffer(bufSize, format, type, (nfmemptr)img); \
		if (src == NULL) return; \
		fn.glReadnPixelsARB(x, y, width, height, format, type, bufSize, src); \
		dst = (nfmemptr)img; \
	} \
	buf.convertToAtari(src, dst)

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLGETNCOMPRESSEDTEXIMAGEARB(target, lod, bufSize, img) \
	GLubyte tmp[MAX(bufSize, 0)]; \
	fn.glGetnCompressedTexImageARB(target, lod, bufSize, tmp); \
	Host2AtariByteArray(bufSize, tmp, img)
#else
#define FN_GLGETNCOMPRESSEDTEXIMAGEARB(target, lod, bufSize, img) \
	fn.glGetnCompressedTexImageARB(target, lod, bufSize, HostAddr(img, void *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETNUNIFORMFVARB(program, location, bufSize, params) \
	GLint const size = bufSize / ATARI_SIZEOF_FLOAT; \
	if (size <= 0) return; \
	GLfloat tmp[size]; \
	fn.glGetnUniformfvARB(program, location, size, tmp); \
	Host2AtariFloatArray(size, tmp, params)
#else
#define FN_GLGETNUNIFORMFVARB(program, location, bufSize, params) \
	fn.glGetnUniformfvARB(program, location, bufSize, HostAddr(params, GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETNUNIFORMIVARB(program, location, bufSize, params) \
	GLint const size = bufSize / sizeof(GLint); \
	if (size <= 0) return; \
	GLint tmp[size]; \
	fn.glGetnUniformivARB(program, location, size, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETNUNIFORMIVARB(program, location, bufSize, params) \
	fn.glGetnUniformivARB(program, location, bufSize, HostAddr(params, GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETNUNIFORMUIVARB(program, location, bufSize, params) \
	GLint const size = bufSize / sizeof(GLuint); \
	if (size <= 0) return; \
	GLuint tmp[size]; \
	fn.glGetnUniformuivARB(program, location, size, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETNUNIFORMUIVARB(program, location, bufSize, params) \
	fn.glGetnUniformuivARB(program, location, bufSize, HostAddr(params, GLuint *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLGETNUNIFORMDVARB(program, location, bufSize, params) \
	GLint const size = bufSize / ATARI_SIZEOF_DOUBLE; \
	if (size <= 0) return; \
	GLdouble tmp[size]; \
	fn.glGetnUniformdvARB(program, location, size, tmp); \
	Host2AtariDoubleArray(size, tmp, params)
#else
#define FN_GLGETNUNIFORMDVARB(program, location, bufSize, params) \
	fn.glGetnUniformdvARB(program, location, bufSize, HostAddr(params, GLdouble *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLGETNMAPDVARB(target, query, bufSize, v) \
	GLint const size = bufSize / ATARI_SIZEOF_DOUBLE; \
	if (size <= 0) return; \
	GLdouble tmp[size]; \
	fn.glGetnMapdvARB(target, query, size * sizeof(*tmp), tmp); \
	Host2AtariDoubleArray(size, tmp, v)
#else
#define FN_GLGETNMAPDVARB(target, query, bufSize, v) \
	fn.glGetnMapdvARB(target, query, bufSize, HostAddr(v, GLdouble *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETNMAPFVARB(target, query, bufSize, v) \
	GLint const size = bufSize / ATARI_SIZEOF_FLOAT; \
	if (size <= 0) return; \
	GLfloat tmp[size]; \
	fn.glGetnMapfvARB(target, query, size * sizeof(*tmp), tmp); \
	Host2AtariFloatArray(size, tmp, v)
#else
#define FN_GLGETNMAPFVARB(target, query, bufSize, v) \
	fn.glGetnMapfvARB(target, query, bufSize, HostAddr(v, GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETNMAPIVARB(target, query, bufSize, v) \
	GLint const size = bufSize / sizeof(Uint32); \
	if (size <= 0) return; \
	GLint tmp[size]; \
	fn.glGetnMapivARB(target, query, size * sizeof(*tmp), tmp); \
	Host2AtariIntArray(size, tmp, v)
#else
#define FN_GLGETNMAPIVARB(target, query, bufSize, v) \
	fn.glGetnMapivARB(target, query, bufSize, HostAddr(v, GLint *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETNPIXELMAPFVARB(map, bufSize, v) \
	GLint const size = bufSize / ATARI_SIZEOF_FLOAT; \
	if (size <= 0) return; \
	GLfloat tmp[size]; \
	fn.glGetnPixelMapfvARB(map, size * sizeof(*tmp), tmp); \
	Host2AtariFloatArray(size, tmp, v)
#else
#define FN_GLGETNPIXELMAPFVARB(map, bufSize, v) \
	fn.glGetnPixelMapfvARB(map, bufSize, HostAddr(v, GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETNPIXELMAPUIVARB(map, bufSize, v) \
	GLint const size = bufSize / sizeof(Uint32); \
	if (size <= 0) return; \
	GLuint tmp[size]; \
	fn.glGetnPixelMapuivARB(map, size * sizeof(*tmp), tmp); \
	Host2AtariIntArray(size, tmp, v)
#else
#define FN_GLGETNPIXELMAPUIVARB(map, bufSize, v) \
	fn.glGetnPixelMapuivARB(map, bufSize, HostAddr(v, GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETNPIXELMAPUSVARB(map, bufSize, v) \
	GLint const size = bufSize / sizeof(Uint16); \
	if (size <= 0) return; \
	GLushort tmp[size]; \
	fn.glGetnPixelMapusvARB(map, size * sizeof(*tmp), tmp); \
	Host2AtariShortArray(size, tmp, v)
#else
#define FN_GLGETNPIXELMAPUSVARB(map, bufSize, v) \
	fn.glGetnPixelMapusvARB(map, bufSize, HostAddr(v, GLushort *))
#endif

/*
 * FIXME: If a non-zero named buffer object is bound to the GL_PIXEL_PACK_BUFFER target 
            (see glBindBuffer) while a polygon stipple pattern is
            requested, pattern is treated as a byte offset into the buffer object's data store.
        
 */
#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLGETNPOLYGONSTIPPLEARB(bufSize, mask) \
	GLubyte tmp[bufSize]; \
	fn.glGetnPolygonStippleARB(bufSize, tmp); \
	Host2AtariByteArray(bufSize, tmp, mask)
#else
#define FN_GLGETNPOLYGONSTIPPLEARB(bufSize, mask) \
	fn.glGetnPolygonStippleARB(bufSize, HostAddr(mask, GLubyte *))
#endif

#define FN_GLGETNCOLORTABLEARB(target, format, type, bufSize, table) \
	char *src; \
	nfmemptr dst; \
	pixelBuffer buf(*this); \
	 \
	if (contexts[cur_context].buffer_bindings.pixel_pack.id) { \
		void *offset = NFHost2AtariAddr(table); \
		fn.glGetColorTableEXT(target, format, type, offset); \
		src = contexts[cur_context].buffer_bindings.pixel_pack.host_pointer + (uintptr_t)offset; \
		dst = contexts[cur_context].buffer_bindings.pixel_pack.atari_pointer + (uintptr_t)offset; \
		if (!buf.params(bufSize, format, type)) return; \
	} else { \
		src = buf.hostBuffer(bufSize, format, type, (nfmemptr)table); \
		if (src == NULL) return; \
		fn.glGetnColorTableARB(target, format, type, bufSize, src); \
		dst = (nfmemptr)table; \
	} \
	buf.convertToAtari(src, dst)

#define FN_GLGETNCONVOLUTIONFILTERARB(target, format, type, bufSize, image) \
	char *src; \
	nfmemptr dst; \
	pixelBuffer buf(*this); \
	 \
	if (contexts[cur_context].buffer_bindings.pixel_pack.id) { \
		void *offset = NFHost2AtariAddr(image); \
		fn.glGetnConvolutionFilterARB(target, format, type, bufSize, offset); \
		src = contexts[cur_context].buffer_bindings.pixel_pack.host_pointer + (uintptr_t)offset; \
		dst = contexts[cur_context].buffer_bindings.pixel_pack.atari_pointer + (uintptr_t)offset; \
		if (!buf.params(bufSize, format, type)) return; \
	} else { \
		src = buf.hostBuffer(bufSize, format, type, (nfmemptr)image); \
		if (src == NULL) return; \
		fn.glGetnConvolutionFilterARB(target, format, type, bufSize, src); \
		dst = (nfmemptr)image; \
	} \
	buf.convertToAtari(src, dst)

#define FN_GLGETNSEPARABLEFILTERARB(target, format, type, rowBufSize, row, columnBufSize, column, span) \
	pixelBuffer rowbuf(*this); \
	pixelBuffer colbuf(*this); \
	char *srcrow; \
	nfmemptr dstrow; \
	char *srccol; \
	nfmemptr dstcol; \
	 \
	if (contexts[cur_context].buffer_bindings.pixel_pack.id) { \
		void *rowoffset = NFHost2AtariAddr(row); \
		void *coloffset = NFHost2AtariAddr(column); \
		fn.glGetnSeparableFilterARB(target, format, type, rowBufSize, rowoffset, columnBufSize, coloffset, HostAddr(span, void *)); \
		srcrow = contexts[cur_context].buffer_bindings.pixel_pack.host_pointer + (uintptr_t)rowoffset; \
		srccol = contexts[cur_context].buffer_bindings.pixel_pack.host_pointer + (uintptr_t)coloffset; \
		dstrow = contexts[cur_context].buffer_bindings.pixel_pack.atari_pointer + (uintptr_t)rowoffset; \
		dstcol = contexts[cur_context].buffer_bindings.pixel_pack.atari_pointer + (uintptr_t)coloffset; \
		if (!rowbuf.params(rowBufSize, format, type)) return; \
		if (!colbuf.params(columnBufSize, format, type)) return; \
	} else { \
		srcrow = rowbuf.hostBuffer(rowBufSize, format, type, (nfmemptr)row); \
		srccol = colbuf.hostBuffer(columnBufSize, format, type, (nfmemptr)column); \
		if (srcrow == NULL || srccol == NULL) return; \
		fn.glGetnSeparableFilterARB(target, format, type, rowBufSize, srcrow, columnBufSize, srccol, HostAddr(span, void *)); \
		dstrow = (nfmemptr)row; \
		dstcol = (nfmemptr)column; \
	} \
	rowbuf.convertToAtari(srcrow, dstrow); \
	colbuf.convertToAtari(srccol, dstcol)

#define FN_GLGETNHISTOGRAMARB(target, reset, format, type, bufSize, values) \
	GLint width = 0; \
	char *src; \
	nfmemptr dst; \
	pixelBuffer buf(*this); \
	 \
	fn.glGetHistogramParameteriv(target, GL_HISTOGRAM_WIDTH, &width); \
	if (contexts[cur_context].buffer_bindings.pixel_pack.id) { \
		void *offset = NFHost2AtariAddr(values); \
		fn.glGetnHistogramARB(target, reset, format, type, bufSize, offset); \
		src = contexts[cur_context].buffer_bindings.pixel_pack.host_pointer + (uintptr_t)offset; \
		dst = contexts[cur_context].buffer_bindings.pixel_pack.atari_pointer + (uintptr_t)offset; \
		if (!buf.params(bufSize, format, type)) return; \
	} else { \
		src = buf.hostBuffer(bufSize, format, type, (nfmemptr)values); \
		if (src == NULL) return; \
		fn.glGetnHistogramARB(target, reset, format, type, bufSize, src); \
		dst = (nfmemptr)values; \
	} \
	buf.convertToAtari(src, dst)

#define FN_GLGETNMINMAXARB(target, reset, format, type, bufSize, values) \
	char *src; \
	nfmemptr dst; \
	pixelBuffer buf(*this); \
	 \
	if (!buf.params(bufSize, format, type)) \
		return; \
	char result[bufSize]; \
	if (contexts[cur_context].buffer_bindings.pixel_pack.id) { \
		void *offset = NFHost2AtariAddr(values); \
		fn.glGetnMinmaxARB(target, reset, format, type, bufSize, offset); \
		src = contexts[cur_context].buffer_bindings.pixel_pack.host_pointer + (uintptr_t)offset; \
		dst = contexts[cur_context].buffer_bindings.pixel_pack.atari_pointer + (uintptr_t)offset; \
	} else { \
		src = result; \
		fn.glGetnMinmaxARB(target, reset, format, type, bufSize, src); \
		dst = (nfmemptr)values; \
	} \
	buf.convertToAtari(src, dst)

/* -------------------------------------------------------------------------- */

/*
 * GL_SGIX_igloo_interface
 */
/* TODO: no infos */
#define FN_GLIGLOOINTERFACESGIX(pname, params) \
	fn.glIglooInterfaceSGIX(pname, HostAddr(params, const void *))

/* -------------------------------------------------------------------------- */

/*
 * GL_SGIX_list_priority
 */
#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETLISTPARAMETERFVSGIX(list, pname, params) \
	GLint const size = 1; \
	GLfloat tmp[size]; \
	fn.glGetListParameterfvSGIX(list, pname, tmp); \
	Host2AtariFloatArray(size, tmp, params)
#else
#define FN_GLGETLISTPARAMETERFVSGIX(list, pname, params) \
	fn.glGetListParameterfvSGIX(list, pname, HostAddr(params, GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETLISTPARAMETERIVSGIX(list, pname, params) \
	GLint const size = 1; \
	GLint tmp[size]; \
	fn.glGetListParameterivSGIX(list, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETLISTPARAMETERIVSGIX(list, pname, params) \
	fn.glGetListParameterivSGIX(list, pname, HostAddr(params, GLint *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLLISTPARAMETERFVSGIX(list, pname, params) \
	GLint const size = 1; \
	GLfloat tmp[size], *ptmp; \
	ptmp = Atari2HostFloatArray(size, params, tmp); \
	fn.glListParameterfvSGIX(list, pname, ptmp)
#else
#define FN_GLLISTPARAMETERFVSGIX(list, pname, params) \
	fn.glListParameterfvSGIX(list, pname, HostAddr(params, const GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLLISTPARAMETERIVSGIX(list, pname, params) \
	GLint const size = 1; \
	GLint tmp[size], *ptmp; \
	ptmp = Atari2HostIntArray(size, params, tmp); \
	fn.glListParameterivSGIX(list, pname, ptmp)
#else
#define FN_GLLISTPARAMETERIVSGIX(list, pname, params) \
	fn.glListParameterivSGIX(list, pname, HostAddr(params, const GLint *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_HP_image_transform
 */

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETIMAGETRANSFORMPARAMETERFVHP(target, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLfloat tmp[MAX(size, 16)]; \
	fn.glGetImageTransformParameterfvHP(target, pname, tmp); \
	Host2AtariFloatArray(size, tmp, params)
#else
#define FN_GLGETIMAGETRANSFORMPARAMETERFVHP(target, pname, params) \
	fn.glGetImageTransformParameterfvHP(target, pname, HostAddr(params, GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETIMAGETRANSFORMPARAMETERIVHP(target, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLint tmp[MAX(size, 16)]; \
	fn.glGetImageTransformParameterivHP(target, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETIMAGETRANSFORMPARAMETERIVHP(target, pname, params) \
	fn.glGetImageTransformParameterivHP(target, pname, HostAddr(params, GLint *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLIMAGETRANSFORMPARAMETERFVHP(target, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLfloat tmp[MAX(size, 16)], *ptmp; \
	ptmp = Atari2HostFloatArray(size, params, tmp); \
	fn.glImageTransformParameterfvHP(target, pname, ptmp)
#else
#define FN_GLIMAGETRANSFORMPARAMETERFVHP(target, pname, params) \
	fn.glImageTransformParameterfvHP(target, pname, HostAddr(params, const GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLIMAGETRANSFORMPARAMETERIVHP(target, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLint tmp[MAX(size, 16)], *ptmp; \
	ptmp = Atari2HostIntArray(size, params, tmp); \
	fn.glImageTransformParameterivHP(target, pname, ptmp)
#else
#define FN_GLIMAGETRANSFORMPARAMETERIVHP(target, pname, params) \
	fn.glImageTransformParameterivHP(target, pname, HostAddr(params, const GLint *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_APPLE_vertex_program_evaluators
 */
#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLMAPVERTEXATTRIB1DAPPLE(index, size, u1, u2, stride, order, points) \
	GLdouble *tmp; \
	nfcmemptr ptr; \
	GLint i; \
	 \
	tmp = (GLdouble *)malloc(size * order * sizeof(*tmp)); \
	if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
	ptr = (nfcmemptr)points; \
	for (i = 0; i < order; i++) { \
		Atari2HostDoubleArray(size, AtariAddr(ptr, const GLdouble *), &tmp[i*size]); \
		ptr += stride; \
	} \
	fn.glMapVertexAttrib1dAPPLE(index, size, u1, u2, size * sizeof(GLdouble), order, tmp); \
	free(tmp)
#else
#define FN_GLMAPVERTEXATTRIB1DAPPLE(index, size, u1, u2, stride, order, points) \
	fn.glMapVertexAttrib1dAPPLE(index, size, u1, u2, stride, order, HostAddr(points, const GLdouble *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLMAPVERTEXATTRIB1FAPPLE(index, size, u1, u2, stride, order, points) \
	GLfloat *tmp; \
	nfcmemptr ptr; \
	GLint i; \
	 \
	tmp = (GLfloat *)malloc(size * order * sizeof(*tmp)); \
	if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
	ptr = (nfcmemptr)points; \
	for (i = 0; i < order; i++) { \
		Atari2HostFloatArray(size, AtariAddr(ptr, const GLfloat *), &tmp[i*size]); \
		ptr += stride; \
	} \
	fn.glMapVertexAttrib1fAPPLE(index, size, u1, u2, size * sizeof(GLfloat), order, tmp); \
	free(tmp)
#else
#define FN_GLMAPVERTEXATTRIB1FAPPLE(index, size, u1, u2, stride, order, points) \
	fn.glMapVertexAttrib1fAPPLE(index, size, u1, u2, stride, order, HostAddr(points, const GLfloat *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLMAPVERTEXATTRIB2DAPPLE(index, size, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points) \
	GLdouble *tmp; \
	nfcmemptr ptr; \
	GLint i, j; \
	 \
	tmp = (GLdouble *)malloc(size * uorder * vorder * sizeof(*tmp)); \
	if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
	ptr = (nfcmemptr)points; \
	for (i = 0; i < uorder; i++) { \
		for (j = 0;j < vorder ;j++) { \
			Atari2HostDoubleArray(size, AtariAddr(ptr + (i * ustride + j * vstride) * ATARI_SIZEOF_DOUBLE, const GLdouble *), &tmp[(i * vorder + j) * size]); \
		} \
	} \
	fn.glMapVertexAttrib2dAPPLE(index, size, u1, u2, size * vorder * sizeof(GLdouble), uorder, v1, v2, size * sizeof(GLdouble), vorder, tmp); \
	free(tmp)
#else
#define FN_GLMAPVERTEXATTRIB2DAPPLE(index, size, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points) \
	fn.glMapVertexAttrib2dAPPLE(index, size, u1, u2, ustride, uorder, v1, v2, vstride, vorder, HostAddr(points, const GLdouble *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLMAPVERTEXATTRIB2FAPPLE(index, size, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points) \
	GLfloat *tmp; \
	nfcmemptr ptr; \
	GLint i, j; \
	 \
	tmp = (GLfloat *)malloc(size * uorder * vorder * sizeof(*tmp)); \
	if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
	ptr = (nfcmemptr)points; \
	for (i = 0; i < uorder; i++) { \
		for (j = 0;j < vorder ;j++) { \
			Atari2HostFloatArray(size, AtariAddr(ptr + (i * ustride + j * vstride) * ATARI_SIZEOF_FLOAT, const GLfloat *), &tmp[(i * vorder + j) * size]); \
		} \
	} \
	fn.glMapVertexAttrib2fAPPLE(index, size, u1, u2, size * vorder * sizeof(GLfloat), uorder, v1, v2, size * sizeof(GLfloat), vorder, tmp); \
	free(tmp)
#else
#define FN_GLMAPVERTEXATTRIB2FAPPLE(index, size, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points) \
	fn.glMapVertexAttrib2fAPPLE(index, size, u1, u2, ustride, uorder, v1, v2, vstride, vorder, HostAddr(points, const GLfloat *))
#endif

#define FN_GLMULTIDRAWARRAYSEXT(mode, first, count, drawcount) \
	GLsizei const size = drawcount; \
	if (size <= 0) return; \
	GLsizei firstbuf[size]; \
	GLsizei countbuf[size]; \
	Atari2HostIntArray(size, first, firstbuf); \
	Atari2HostIntArray(size, count, countbuf); \
	for (GLsizei i = 0; i < size; i++) \
		convertClientArrays(firstbuf[i] + countbuf[i]); \
	fn.glMultiDrawArraysEXT(mode, firstbuf, countbuf, drawcount)

#define FN_GLMULTIDRAWELEMENTSEXT(mode, count, type, indices, drawcount) \
	GLsizei const size = drawcount; \
	if (size <= 0) return; \
	GLsizei countbuf[size]; \
	nfmemptr indbuf[size]; \
	void *indptr[size]; \
	pixelBuffer **pbuf; \
	Atari2HostIntArray(size, count, countbuf); \
	pbuf = new pixelBuffer *[size]; \
	Atari2HostPtrArray(size, AtariAddr(indices, const void **), indbuf); \
	for (GLsizei i = 0; i < size; i++) { \
		convertClientArrays(countbuf[i]); \
		switch(type) { \
		case GL_UNSIGNED_BYTE: \
		case GL_UNSIGNED_SHORT: \
		case GL_UNSIGNED_INT: \
			pbuf[i] = new pixelBuffer(); \
			indptr[i] = pbuf[i]->convertArray(countbuf[i], type, indbuf[i]); \
			break; \
		default: \
			glSetError(GL_INVALID_ENUM); \
			return; \
		} \
	} \
	fn.glMultiDrawElementsEXT(mode, countbuf, type, indptr, drawcount); \
	for (GLsizei i = 0; i < size; i++) { \
		delete pbuf[i]; \
	} \
	delete [] pbuf

/* -------------------------------------------------------------------------- */

/*
 * GL_AMD_multi_draw_indirect
 */

#define FN_GLMULTIDRAWARRAYSINDIRECTAMD(mode, indirect, drawcount, stride) \
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
		void *offset = NFHost2AtariAddr(indirect); \
		fn.glMultiDrawArraysIndirectAMD(mode, offset, drawcount, stride); \
	} else if (indirect) { \
		if (stride == 0) stride = 4 * sizeof(Uint32); \
		nfcmemptr indptr = (nfcmemptr)indirect; \
		for (GLsizei n = 0; n < drawcount; n++) { \
			GLuint tmp[4] = { 0, 0, 0, 0 }; \
			Atari2HostIntArray(4, indptr, tmp); \
			GLuint count = tmp[0]; \
			convertClientArrays(count); \
			fn.glDrawArraysInstancedBaseInstance(mode, tmp[2], count, tmp[1], tmp[3]); \
			indptr += stride; \
		} \
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
 *    } DrawElemntsIndirectCommand;
 */
#define FN_GLMULTIDRAWELEMENTSINDIRECTAMD(mode, type, indirect, drawcount, stride) \
	if (contexts[cur_context].buffer_bindings.draw_indirect.id) { \
		void *offset = NFHost2AtariAddr(indirect); \
		fn.glMultiDrawElementsIndirectAMD(mode, type, offset, drawcount, stride); \
	} else if (indirect) { \
		if (stride == 0) stride = 5 * sizeof(Uint32); \
		nfmemptr indptr = (nfmemptr)indirect; \
		for (GLsizei n = 0; n < drawcount; n++) { \
			GLuint tmp[5] = { 0, 0, 0, 0, 0 }; \
			Atari2HostIntArray(5, indptr, tmp); \
			GLuint count = tmp[0]; \
			convertClientArrays(count); \
			fn.glDrawElementsInstancedBaseVertexBaseInstance(mode, count, type, (const void *)(uintptr_t)tmp[2], tmp[1], tmp[3], tmp[4]); \
			indptr += stride; \
		} \
	}

/* -------------------------------------------------------------------------- */

/*
 * GL_NV_bindless_multi_draw_indirect
 */

#define FN_GLMULTIDRAWARRAYSINDIRECTBINDLESSNV(mode, indirect, drawcount, stride, vertexBufferCount) \
	if (contexts[cur_context].buffer_bindings.draw_indirect.id) { \
		void *offset = NFHost2AtariAddr(indirect); \
		fn.glMultiDrawArraysIndirectBindlessNV(mode, offset, drawcount, stride, vertexBufferCount); \
	}

#define FN_GLMULTIDRAWELEMENTSINDIRECTBINDLESSNV(mode, type, indirect, drawcount, stride, vertexBufferCount) \
	if (contexts[cur_context].buffer_bindings.draw_indirect.id) { \
		void *offset = NFHost2AtariAddr(indirect); \
		fn.glMultiDrawElementsIndirectBindlessNV(mode, type, offset, drawcount, stride, vertexBufferCount); \
	}

/* -------------------------------------------------------------------------- */

/*
 * GL_IBM_multimode_draw_arrays
 */
#define FN_GLMULTIMODEDRAWARRAYSIBM(mode, first, count, primcount, modestride) \
	GLsizei const size = primcount; \
	if (size <= 0) return; \
	GLsizei firstbuf[size]; \
	GLsizei countbuf[size]; \
	GLsizei const modecount = modestride == 0 ? 1 : primcount; \
	GLenum modes[modecount]; \
	Atari2HostIntArray(size, first, firstbuf); \
	Atari2HostIntArray(size, count, countbuf); \
	nfmemptr modeptr = (nfmemptr)mode; \
	for (GLsizei i = 0; i < modecount; i++) \
		Atari2HostIntArray(1, AtariAddr(modeptr + i * modestride, const GLenum *), &modes[i]); \
	if (modestride != 0) modestride = sizeof(GLenum); \
	for (GLsizei i = 0; i < size; i++) \
		convertClientArrays(firstbuf[i] + countbuf[i]); \
	fn.glMultiModeDrawArraysIBM(modes, firstbuf, countbuf, primcount, modestride)

#define FN_GLMULTIMODEDRAWELEMENTSIBM(mode, count, type, indices, primcount, modestride) \
	GLsizei const size = primcount; \
	if (size <= 0) return; \
	GLsizei countbuf[size]; \
	nfmemptr indbuf[size]; \
	void *indptr[size]; \
	pixelBuffer **pbuf; \
	Atari2HostIntArray(size, count, countbuf); \
	pbuf = new pixelBuffer *[size]; \
	Atari2HostPtrArray(size, AtariAddr(indices, const void **), indbuf); \
	for (GLsizei i = 0; i < size; i++) { \
		convertClientArrays(countbuf[i]); \
		switch(type) { \
		case GL_UNSIGNED_BYTE: \
		case GL_UNSIGNED_SHORT: \
		case GL_UNSIGNED_INT: \
			pbuf[i] = new pixelBuffer(); \
			indptr[i] = pbuf[i]->convertArray(countbuf[i], type, indbuf[i]); \
			break; \
		default: \
			glSetError(GL_INVALID_ENUM); \
			return; \
		} \
	} \
	GLsizei const modecount = modestride == 0 ? 1 : primcount; \
	GLenum modes[modecount]; \
	nfmemptr modeptr = (nfmemptr)mode; \
	for (GLsizei i = 0; i < modecount; i++) \
		Atari2HostIntArray(1, AtariAddr(modeptr + i * modestride, const GLenum *), &modes[i]); \
	if (modestride != 0) modestride = sizeof(GLenum); \
	fn.glMultiModeDrawElementsIBM(modes, countbuf, type, indptr, primcount, modestride); \
	for (GLsizei i = 0; i < size; i++) { \
		delete pbuf[i]; \
	} \
	delete [] pbuf

/* -------------------------------------------------------------------------- */

/*
 * GL_NV_pixel_data_range
 */
#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLPIXELDATARANGENV(target, length, pointer) \
	GLubyte tmp[MAX(length, 0)], *ptmp; \
	ptmp = Atari2HostByteArray(length, AtariAddr(pointer, const GLubyte *), tmp); \
	fn.glPixelDataRangeNV(target, length, ptmp)
#else
#define FN_GLPIXELDATARANGENV(target, length, pointer) \
	fn.glPixelDataRangeNV(target, length, HostAddr(pointer, const void *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_ARB_point_parameters
 */

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPOINTPARAMETERFVARB(pname, params) \
	GLfloat tmp[1]; \
	Atari2HostFloatArray(1, params, tmp); \
	fn.glPointParameterfvARB(pname, tmp)
#else
#define FN_GLPOINTPARAMETERFVARB(pname, params) \
	fn.glPointParameterfvARB(pname, HostAddr(params, const GLfloat *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_EXT_point_parameters
 */

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPOINTPARAMETERFVEXT(pname, params) \
	GLfloat tmp[1]; \
	Atari2HostFloatArray(1, params, tmp); \
	fn.glPointParameterfvEXT(pname, tmp)
#else
#define FN_GLPOINTPARAMETERFVEXT(pname, params) \
	fn.glPointParameterfvEXT(pname, HostAddr(params, const GLfloat *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_SGIS_point_parameters
 */

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPOINTPARAMETERFVSGIS(pname, params) \
	GLfloat tmp[1]; \
	Atari2HostFloatArray(1, params, tmp); \
	fn.glPointParameterfvSGIS(pname, tmp)
#else
#define FN_GLPOINTPARAMETERFVSGIS(pname, params) \
	fn.glPointParameterfvSGIS(pname, HostAddr(params, const GLfloat *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_NV_point_sprite
 */

#if NFOSMESA_NEED_INT_CONV
#define FN_GLPOINTPARAMETERIVNV(pname, params) \
	GLint tmp[1]; \
	Atari2HostIntArray(1, params, tmp); \
	fn.glPointParameterivNV(pname, tmp)
#else
#define FN_GLPOINTPARAMETERIVNV(pname, params) \
	fn.glPointParameterivNV(pname, HostAddr(params, const GLint *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_APPLE_vertex_point_size
 */
/* TODO: no infos */
#define FN_GLPOINTSIZEPOINTERAPPLE(type, stride, pointer) \
	fn.glPointSizePointerAPPLE(type, stride, HostAddr(pointer, const void *))

/* -------------------------------------------------------------------------- */

/*
 * GL_SGIX_async
 */

#if NFOSMESA_NEED_INT_CONV
#define FN_GLFINISHASYNCSGIX(markerp) \
	GLuint tmp[1]; \
	GLint ret = fn.glFinishAsyncSGIX(tmp); \
	if (ret) \
		Host2AtariIntArray(1, tmp, markerp); \
	return ret
#else
#define FN_GLFINISHASYNCSGIX(markerp) \
	return fn.glFinishAsyncSGIX(HostAddr(markerp, GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLPOLLASYNCSGIX(markerp) \
	GLuint tmp[1]; \
	GLint ret = fn.glPollAsyncSGIX(tmp); \
	if (ret) \
		Host2AtariIntArray(1, tmp, markerp); \
	return ret
#else
#define FN_GLPOLLASYNCSGIX(markerp) \
	return fn.glPollAsyncSGIX(HostAddr(markerp, GLuint *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_NV_parameter_buffer_object
 */

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPROGRAMBUFFERPARAMETERSFVNV(target, bindingIndex, wordIndex, count, params) \
	GLsizei const size = count; \
	if (count <= 0) return; \
	GLfloat tmp[size], *ptmp; \
	ptmp = Atari2HostFloatArray(size, params, tmp); \
	fn.glProgramBufferParametersfvNV(target, bindingIndex, wordIndex, count, ptmp)
#else
#define FN_GLPROGRAMBUFFERPARAMETERSFVNV(target, bindingIndex, wordIndex, count, params) \
	fn.glProgramBufferParametersfvNV(target, bindingIndex, wordIndex, count, HostAddr(params, const GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLPROGRAMBUFFERPARAMETERSIIVNV(target, bindingIndex, wordIndex, count, params) \
	GLsizei const size = count; \
	if (count <= 0) return; \
	GLint tmp[size], *ptmp; \
	ptmp = Atari2HostIntArray(size, params, tmp); \
	fn.glProgramBufferParametersIivNV(target, bindingIndex, wordIndex, count, ptmp)
#else
#define FN_GLPROGRAMBUFFERPARAMETERSIIVNV(target, bindingIndex, wordIndex, count, params) \
	fn.glProgramBufferParametersIivNV(target, bindingIndex, wordIndex, count, HostAddr(params, const GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLPROGRAMBUFFERPARAMETERSIUIVNV(target, bindingIndex, wordIndex, count, params) \
	GLsizei const size = count; \
	if (count <= 0) return; \
	GLuint tmp[size], *ptmp; \
	ptmp = Atari2HostIntArray(size, params, tmp); \
	fn.glProgramBufferParametersIuivNV(target, bindingIndex, wordIndex, count, ptmp)
#else
#define FN_GLPROGRAMBUFFERPARAMETERSIUIVNV(target, bindingIndex, wordIndex, count, params) \
	fn.glProgramBufferParametersIuivNV(target, bindingIndex, wordIndex, count, HostAddr(params, const GLuint *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_EXT_gpu_program_parameters
 */

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPROGRAMENVPARAMETERS4FVEXT(target, index, count, params) \
	GLint const size = 4 * count; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, params, tmp); \
	fn.glProgramEnvParameters4fvEXT(target, index, count, tmp)
#else
#define FN_GLPROGRAMENVPARAMETERS4FVEXT(target, index, count, params) \
	fn.glProgramEnvParameters4fvEXT(target, index, count, HostAddr(params, const GLfloat *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPROGRAMLOCALPARAMETERS4FVEXT(target, index, count, params) \
	GLint const size = 4 * count; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, params, tmp); \
	fn.glProgramLocalParameters4fvEXT(target, index, count, tmp)
#else
#define FN_GLPROGRAMLOCALPARAMETERS4FVEXT(target, index, count, params) \
	fn.glProgramLocalParameters4fvEXT(target, index, count, HostAddr(params, const GLfloat *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_SUN_triangle_list
 */

#if NFOSMESA_NEED_INT_CONV
#define FN_GLREPLACEMENTCODEUIVSUN(code) \
	GLint const size = 1; \
	GLuint tmp[size]; \
	Atari2HostIntArray(size, code, tmp); \
	fn.glReplacementCodeuivSUN(tmp)
#else
#define FN_GLREPLACEMENTCODEUIVSUN(code) \
	fn.glReplacementCodeuivSUN(HostAddr(code, const GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLREPLACEMENTCODEUSVSUN(code) \
	GLint const size = 1; \
	GLushort tmp[size]; \
	Atari2HostShortArray(size, code, tmp); \
	fn.glReplacementCodeusvSUN(tmp)
#else
#define FN_GLREPLACEMENTCODEUSVSUN(code) \
	fn.glReplacementCodeusvSUN(HostAddr(code, const GLushort *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLREPLACEMENTCODEUBVSUN(code) \
	GLint const size = 1; \
	GLubyte tmp[size]; \
	Atari2HostByteArray(size, code, tmp); \
	fn.glReplacementCodeubvSUN(tmp)
#else
#define FN_GLREPLACEMENTCODEUBVSUN(code) \
	fn.glReplacementCodeubvSUN(HostAddr(code, const GLubyte *))
#endif

#define FN_GLREPLACEMENTCODEPOINTERSUN(type, stride, pointer) \
	GLint texunit = 0; \
	fn.glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE, &texunit); \
	setupClientArray(texunit, contexts[cur_context].replacement_code, 1, type, stride, -1, 0, AtariAddr(pointer, nfmemptr)); \
	contexts[cur_context].replacement_code.vendor = NFOSMESA_VENDOR_SUN

/* -------------------------------------------------------------------------- */

/*
 * GL_SUN_vertex
 */

#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLCOLOR4UBVERTEX2FVSUN(c, v) \
	GLubyte color[4]; \
	GLfloat tmp[2]; \
	Atari2HostByteArray(4, c, color); \
	Atari2HostFloatArray(2, v, tmp); \
	fn.glColor4ubVertex2fvSUN(color, tmp)
#else
#define FN_GLCOLOR4UBVERTEX2FVSUN(c, v) \
	fn.glColor4ubVertex2fvSUN(HostAddr(c, const GLubyte *), HostAddr(v, const GLfloat *))
#endif

#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLCOLOR4UBVERTEX3FVSUN(c, v) \
	GLubyte color[4]; \
	GLfloat tmp[3]; \
	Atari2HostByteArray(4, c, color); \
	Atari2HostFloatArray(3, v, tmp); \
	fn.glColor4ubVertex3fvSUN(color, tmp)
#else
#define FN_GLCOLOR4UBVERTEX3FVSUN(c, v) \
	fn.glColor4ubVertex3fvSUN(HostAddr(c, const GLubyte *), HostAddr(v, const GLfloat *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLCOLOR3FVERTEX3FVSUN(c, v) \
	GLfloat tmp1[3]; \
	GLfloat tmp2[3]; \
	Atari2HostFloatArray(3, c, tmp1); \
	Atari2HostFloatArray(3, v, tmp2); \
	fn.glColor3fVertex3fvSUN(tmp1, tmp2)
#else
#define FN_GLCOLOR3FVERTEX3FVSUN(c, v) \
	fn.glColor3fVertex3fvSUN(HostAddr(c, const GLfloat *), HostAddr(v, const GLfloat *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLNORMAL3FVERTEX3FVSUN(n, v) \
	GLfloat tmp1[3]; \
	GLfloat tmp2[3]; \
	Atari2HostFloatArray(3, n, tmp1); \
	Atari2HostFloatArray(3, v, tmp2); \
	fn.glNormal3fVertex3fvSUN(tmp1, tmp2)
#else
#define FN_GLNORMAL3FVERTEX3FVSUN(n, v) \
	fn.glNormal3fVertex3fvSUN(HostAddr(n, const GLfloat *), HostAddr(v, const GLfloat *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLCOLOR4FNORMAL3FVERTEX3FVSUN(c, n, v) \
	GLfloat tmp1[4]; \
	GLfloat tmp2[3]; \
	GLfloat tmp3[3]; \
	Atari2HostFloatArray(4, c, tmp1); \
	Atari2HostFloatArray(3, n, tmp2); \
	Atari2HostFloatArray(3, v, tmp3); \
	fn.glColor4fNormal3fVertex3fvSUN(tmp1, tmp2, tmp3)
#else
#define FN_GLCOLOR4FNORMAL3FVERTEX3FVSUN(c, n, v) \
	fn.glColor4fNormal3fVertex3fvSUN(HostAddr(c, const GLfloat *), HostAddr(n, const GLfloat *), HostAddr(v, const GLfloat *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLTEXCOORD2FVERTEX3FVSUN(tc, v) \
	GLfloat tmp1[2]; \
	GLfloat tmp2[3]; \
	Atari2HostFloatArray(2, tc, tmp1); \
	Atari2HostFloatArray(3, v, tmp2); \
	fn.glTexCoord2fVertex3fvSUN(tmp1, tmp2)
#else
#define FN_GLTEXCOORD2FVERTEX3FVSUN(tc, v) \
	fn.glTexCoord2fVertex3fvSUN(HostAddr(tc, const GLfloat *), HostAddr(v, const GLfloat *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLTEXCOORD4FVERTEX4FVSUN(tc, v) \
	GLfloat tmp1[4]; \
	GLfloat tmp2[4]; \
	Atari2HostFloatArray(4, tc, tmp1); \
	Atari2HostFloatArray(4, v, tmp2); \
	fn.glTexCoord4fVertex4fvSUN(tmp1, tmp2)
#else
#define FN_GLTEXCOORD4FVERTEX4FVSUN(tc, v) \
	fn.glTexCoord4fVertex4fvSUN(HostAddr(tc, const GLfloat *), HostAddr(v, const GLfloat *))
#endif

#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLTEXCOORD2FCOLOR4UBVERTEX3FVSUN(tc, c, v) \
	GLubyte color[4]; \
	GLfloat tmp1[2]; \
	GLfloat tmp2[3]; \
	Atari2HostByteArray(4, c, color); \
	Atari2HostFloatArray(2, tc, tmp1); \
	Atari2HostFloatArray(3, v, tmp2); \
	fn.glTexCoord2fColor4ubVertex3fvSUN(tmp1, color, tmp2)
#else
#define FN_GLTEXCOORD2FCOLOR4UBVERTEX3FVSUN(tc, c, v) \
	fn.glTexCoord2fColor4ubVertex3fvSUN(HostAddr(tc, const GLfloat *), HostAddr(c, const GLubyte *), HostAddr(v, const GLfloat *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLTEXCOORD2FCOLOR3FVERTEX3FVSUN(tc, c, v) \
	GLfloat tmp1[2]; \
	GLfloat tmp2[3]; \
	GLfloat tmp3[3]; \
	Atari2HostFloatArray(2, tc, tmp1); \
	Atari2HostFloatArray(3, c, tmp2); \
	Atari2HostFloatArray(3, v, tmp3); \
	fn.glTexCoord2fColor3fVertex3fvSUN(tmp1, tmp2, tmp3)
#else
#define FN_GLTEXCOORD2FCOLOR3FVERTEX3FVSUN(tc, c, v) \
	fn.glTexCoord2fColor3fVertex3fvSUN(HostAddr(tc, const GLfloat *), HostAddr(c, const GLfloat *), HostAddr(v, const GLfloat *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLTEXCOORD2FNORMAL3FVERTEX3FVSUN(tc, n, v) \
	GLfloat tmp1[2]; \
	GLfloat tmp2[3]; \
	GLfloat tmp3[3]; \
	Atari2HostFloatArray(2, tc, tmp1); \
	Atari2HostFloatArray(3, n, tmp2); \
	Atari2HostFloatArray(3, v, tmp3); \
	fn.glTexCoord2fNormal3fVertex3fvSUN(tmp1, tmp2, tmp3)
#else
#define FN_GLTEXCOORD2FNORMAL3FVERTEX3FVSUN(tc, n, v) \
	fn.glTexCoord2fNormal3fVertex3fvSUN(HostAddr(tc, const GLfloat *), HostAddr(n, const GLfloat *), HostAddr(v, const GLfloat *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLTEXCOORD2FCOLOR4FNORMAL3FVERTEX3FVSUN(tc, c, n, v) \
	GLfloat tmp1[2]; \
	GLfloat tmp2[4]; \
	GLfloat tmp3[3]; \
	GLfloat tmp4[3]; \
	Atari2HostFloatArray(2, tc, tmp1); \
	Atari2HostFloatArray(4, c, tmp2); \
	Atari2HostFloatArray(3, n, tmp3); \
	Atari2HostFloatArray(3, v, tmp4); \
	fn.glTexCoord2fColor4fNormal3fVertex3fvSUN(tmp1, tmp2, tmp3, tmp4)
#else
#define FN_GLTEXCOORD2FCOLOR4FNORMAL3FVERTEX3FVSUN(tc, c, n, v) \
	fn.glTexCoord2fColor4fNormal3fVertex3fvSUN(HostAddr(tc, const GLfloat *), HostAddr(c, const GLfloat *), HostAddr(n, const GLfloat *), HostAddr(v, const GLfloat *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLTEXCOORD4FCOLOR4FNORMAL3FVERTEX4FVSUN(tc, c, n, v) \
	GLfloat tmp1[4]; \
	GLfloat tmp2[4]; \
	GLfloat tmp3[3]; \
	GLfloat tmp4[4]; \
	Atari2HostFloatArray(4, tc, tmp1); \
	Atari2HostFloatArray(4, c, tmp2); \
	Atari2HostFloatArray(3, n, tmp3); \
	Atari2HostFloatArray(4, v, tmp4); \
	fn.glTexCoord4fColor4fNormal3fVertex4fvSUN(tmp1, tmp2, tmp3, tmp4)
#else
#define FN_GLTEXCOORD4FCOLOR4FNORMAL3FVERTEX4FVSUN(tc, c, n, v) \
	fn.glTexCoord4fColor4fNormal3fVertex4fvSUN(HostAddr(tc, const GLfloat *), HostAddr(c, const GLfloat *), HostAddr(n, const GLfloat *), HostAddr(v, const GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLREPLACEMENTCODEUIVERTEX3FVSUN(rc, v) \
	GLuint tmp1[1]; \
	GLfloat tmp2[3]; \
	Atari2HostIntArray(1, rc, tmp1); \
	Atari2HostFloatArray(3, v, tmp2); \
	fn.glReplacementCodeuiVertex3fvSUN(tmp1, tmp2)
#else
#define FN_GLREPLACEMENTCODEUIVERTEX3FVSUN(rc, v) \
	fn.glReplacementCodeuiVertex3fvSUN(HostAddr(rc, const GLuint *), HostAddr(v, const GLfloat *))
#endif

#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLREPLACEMENTCODEUICOLOR4UBVERTEX3FVSUN(rc, c, v) \
	GLuint tmp1[1]; \
	GLubyte color[4]; \
	GLfloat tmp2[3]; \
	Atari2HostIntArray(1, rc, tmp1); \
	Atari2HostByteArray(4, c, color); \
	Atari2HostFloatArray(3, v, tmp2); \
	fn.glReplacementCodeuiColor4ubVertex3fvSUN(tmp1, color, tmp2)
#else
#define FN_GLREPLACEMENTCODEUICOLOR4UBVERTEX3FVSUN(rc, c, v) \
	fn.glReplacementCodeuiColor4ubVertex3fvSUN(HostAddr(rc, const GLuint *), HostAddr(c, const GLubyte *), HostAddr(v, const GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLREPLACEMENTCODEUICOLOR3FVERTEX3FVSUN(rc, c, v) \
	GLuint tmp1[1]; \
	GLfloat tmp2[3]; \
	GLfloat tmp3[3]; \
	Atari2HostIntArray(1, rc, tmp1); \
	Atari2HostFloatArray(3, c, tmp2); \
	Atari2HostFloatArray(3, v, tmp3); \
	fn.glReplacementCodeuiColor3fVertex3fvSUN(tmp1, tmp2, tmp3)
#else
#define FN_GLREPLACEMENTCODEUICOLOR3FVERTEX3FVSUN(rc, c, v) \
	fn.glReplacementCodeuiColor3fVertex3fvSUN(HostAddr(rc, const GLuint *), HostAddr(c, const GLfloat *), HostAddr(v, const GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLREPLACEMENTCODEUINORMAL3FVERTEX3FVSUN(rc, n, v) \
	GLuint tmp1[1]; \
	GLfloat tmp2[3]; \
	GLfloat tmp3[3]; \
	Atari2HostIntArray(1, rc, tmp1); \
	Atari2HostFloatArray(3, n, tmp2); \
	Atari2HostFloatArray(3, v, tmp3); \
	fn.glReplacementCodeuiNormal3fVertex3fvSUN(tmp1, tmp2, tmp3)
#else
#define FN_GLREPLACEMENTCODEUINORMAL3FVERTEX3FVSUN(rc, n, v) \
	fn.glReplacementCodeuiNormal3fVertex3fvSUN(HostAddr(rc, const GLuint *), HostAddr(n, const GLfloat *), HostAddr(v, const GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLREPLACEMENTCODEUICOLOR4FNORMAL3FVERTEX3FVSUN(rc, c, n, v) \
	GLuint tmp1[1]; \
	GLfloat tmp2[4]; \
	GLfloat tmp3[3]; \
	GLfloat tmp4[3]; \
	Atari2HostIntArray(1, rc, tmp1); \
	Atari2HostFloatArray(4, c, tmp2); \
	Atari2HostFloatArray(3, n, tmp3); \
	Atari2HostFloatArray(3, v, tmp4); \
	fn.glReplacementCodeuiColor4fNormal3fVertex3fvSUN(tmp1, tmp2, tmp3, tmp4)
#else
#define FN_GLREPLACEMENTCODEUICOLOR4FNORMAL3FVERTEX3FVSUN(rc, c, n, v) \
	fn.glReplacementCodeuiColor4fNormal3fVertex3fvSUN(HostAddr(rc, const GLuint *), HostAddr(c, const GLfloat *), HostAddr(n, const GLfloat *), HostAddr(v, const GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLREPLACEMENTCODEUITEXCOORD2FVERTEX3FVSUN(rc, tc, v) \
	GLuint tmp1[1]; \
	GLfloat tmp2[2]; \
	GLfloat tmp3[3]; \
	Atari2HostIntArray(1, rc, tmp1); \
	Atari2HostFloatArray(2, tc, tmp2); \
	Atari2HostFloatArray(3, v, tmp3); \
	fn.glReplacementCodeuiTexCoord2fVertex3fvSUN(tmp1, tmp2, tmp3)
#else
#define FN_GLREPLACEMENTCODEUITEXCOORD2FVERTEX3FVSUN(rc, tc, v) \
	fn.glReplacementCodeuiTexCoord2fVertex3fvSUN(HostAddr(rc, const GLuint *), HostAddr(tc, const GLfloat *), HostAddr(v, const GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLREPLACEMENTCODEUITEXCOORD2FNORMAL3FVERTEX3FVSUN(rc, tc, n, v) \
	GLuint tmp1[1]; \
	GLfloat tmp2[2]; \
	GLfloat tmp3[3]; \
	GLfloat tmp4[3]; \
	Atari2HostIntArray(1, rc, tmp1); \
	Atari2HostFloatArray(2, tc, tmp2); \
	Atari2HostFloatArray(3, n, tmp3); \
	Atari2HostFloatArray(3, v, tmp4); \
	fn.glReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN(tmp1, tmp2, tmp3, tmp4)
#else
#define FN_GLREPLACEMENTCODEUITEXCOORD2FNORMAL3FVERTEX3FVSUN(rc, tc, n, v) \
	fn.glReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN(HostAddr(rc, const GLuint *), HostAddr(tc, const GLfloat *), HostAddr(n, const GLfloat *), HostAddr(v, const GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLREPLACEMENTCODEUITEXCOORD2FCOLOR4FNORMAL3FVERTEX3FVSUN(rc, tc, c, n, v) \
	GLuint tmp1[1]; \
	GLfloat tmp2[2]; \
	GLfloat tmp3[4]; \
	GLfloat tmp4[3]; \
	GLfloat tmp5[3]; \
	Atari2HostIntArray(1, rc, tmp1); \
	Atari2HostFloatArray(2, tc, tmp2); \
	Atari2HostFloatArray(4, c, tmp3); \
	Atari2HostFloatArray(3, n, tmp4); \
	Atari2HostFloatArray(3, v, tmp5); \
	fn.glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN(tmp1, tmp2, tmp3, tmp4, tmp5)
#else
#define FN_GLREPLACEMENTCODEUITEXCOORD2FCOLOR4FNORMAL3FVERTEX3FVSUN(rc, tc, c, n, v) \
	fn.glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN(HostAddr(rc, const GLuint *), HostAddr(tc, const GLfloat *), HostAddr(c, const GLfloat *), HostAddr(n, const GLfloat *), HostAddr(v, const GLfloat *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_ATI_fragment_shader
 */

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLSETFRAGMENTSHADERCONSTANTATI(dst, value) \
	GLint const size = 4; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, value, tmp); \
	fn.glSetFragmentShaderConstantATI(dst, tmp)
#else
#define FN_GLSETFRAGMENTSHADERCONSTANTATI(dst, value) \
	fn.glSetFragmentShaderConstantATI(dst, HostAddr(value, const GLfloat *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_AMD_sample_positions
 */

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLSETMULTISAMPLEFVAMD(pname, index, val) \
	GLint const size = 2; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, val, tmp); \
	fn.glSetMultisamplefvAMD(pname, index, tmp)
#else
#define FN_GLSETMULTISAMPLEFVAMD(pname, index, val) \
	fn.glSetMultisamplefvAMD(pname, index, HostAddr(val, const GLfloat *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_SGIX_sprite
 */

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLSPRITEPARAMETERFVSGIX(pname, params) \
	GLint const size = 1; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, params, tmp); \
	fn.glSpriteParameterfvSGIX(pname, tmp)
#else
#define FN_GLSPRITEPARAMETERFVSGIX(pname, params) \
	fn.glSpriteParameterfvSGIX(pname, HostAddr(params, const GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLSPRITEPARAMETERIVSGIX(pname, params) \
	GLint const size = 1; \
	GLint tmp[size]; \
	Atari2HostIntArray(size, params, tmp); \
	fn.glSpriteParameterivSGIX(pname, tmp)
#else
#define FN_GLSPRITEPARAMETERIVSGIX(pname, params) \
	fn.glSpriteParameterivSGIX(pname, HostAddr(params, const GLint *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_GREMEDY_string_marker
 */
#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLSTRINGMARKERGREMEDY(len, string) \
	char tmp[MAX(len, 0)], *ptmp; \
	ptmp = Atari2HostByteArray(len, AtariAddr(string, const char *), tmp); \
	fn.glStringMarkerGREMEDY(len, ptmp)
#else
#define FN_GLSTRINGMARKERGREMEDY(len, string) \
	fn.glStringMarkerGREMEDY(len, HostAddr(string, const char *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_MESA_trace
 */
#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLNEWTRACEMESA(mask, traceName) \
	GLubyte tmp[safe_strlen(traceName)], *ptmp; \
	ptmp = Atari2HostByteArray(sizeof(tmp), traceName, tmp); \
	fn.glNewTraceMESA(mask, ptmp)
#else
#define FN_GLNEWTRACEMESA(mask, traceName) \
	fn.glNewTraceMESA(mask, HostAddr(traceName, const GLubyte *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLTRACECOMMENTMESA(comment) \
	GLubyte tmp[safe_strlen(comment)], *ptmp; \
	ptmp = Atari2HostByteArray(sizeof(tmp), comment, tmp); \
	fn.glTraceCommentMESA(ptmp)
#else
#define FN_GLTRACECOMMENTMESA(comment) \
	fn.glTraceCommentMESA(HostAddr(comment, const GLubyte *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLTRACELISTMESA(name, comment) \
	GLubyte tmp[safe_strlen(comment)], *ptmp; \
	ptmp = Atari2HostByteArray(sizeof(tmp), comment, tmp); \
	fn.glTraceListMESA(name, ptmp)
#else
#define FN_GLTRACELISTMESA(name, comment) \
	fn.glTraceListMESA(name, HostAddr(comment, const GLubyte *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLTRACETEXTUREMESA(name, comment) \
	GLubyte tmp[safe_strlen(comment)], *ptmp; \
	ptmp = Atari2HostByteArray(sizeof(tmp), comment, tmp); \
	fn.glTraceTextureMESA(name, ptmp)
#else
#define FN_GLTRACETEXTUREMESA(name, comment) \
	fn.glTraceTextureMESA(name, HostAddr(comment, const GLubyte *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLTRACEPOINTERMESA(pointer, comment) \
	GLubyte tmp[safe_strlen(comment) + 1], *ptmp; \
	ptmp = Atari2HostByteArray(sizeof(tmp), comment, tmp); \
	fn.glTracePointerMESA(HostAddr(pointer, void *), ptmp)
#else
#define FN_GLTRACEPOINTERMESA(pointer, comment) \
	fn.glTracePointerMESA(HostAddr(pointer, void *), HostAddr(comment, const GLubyte *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLTRACEPOINTERRANGEMESA(first, last, comment) \
	GLubyte tmp[safe_strlen(comment) + 1], *ptmp; \
	ptmp = Atari2HostByteArray(sizeof(tmp), comment, tmp); \
	fn.glTracePointerRangeMESA(HostAddr(first, const void *), HostAddr(last, const void *), ptmp)
#else
#define FN_GLTRACEPOINTERRANGEMESA(first, last, comment) \
	fn.glTracePointerRangeMESA(HostAddr(first, const void *), HostAddr(last, const void *), HostAddr(comment, const GLubyte *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_APPLE_vertex_array_range
 */

#define FN_GLFLUSHVERTEXARRAYRANGEAPPLE(length, pointer) \
	UNUSED(pointer); \
	fn.glFlushVertexArrayRangeAPPLE(length, contexts[cur_context].vertex.host_pointer)

#define FN_GLVERTEXARRAYRANGEAPPLE(length, pointer) \
	fn.glVertexArrayRangeAPPLE(length, HostAddr(pointer, void *))

/* -------------------------------------------------------------------------- */

/*
 * GL_NV_vertex_array_range
 */

#define FN_GLVERTEXARRAYRANGENV(length, pointer) \
	fn.glVertexArrayRangeNV(length, HostAddr(pointer, void *))

/* -------------------------------------------------------------------------- */

/*
 * GL_EXT_vertex_weighting
 */

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLVERTEXWEIGHTFVEXT(weight) \
	GLfloat tmp[1]; \
	Atari2HostFloatArray(1, weight, tmp); \
	fn.glVertexWeightfvEXT(tmp)
#else
#define FN_GLVERTEXWEIGHTFVEXT(weight) \
	fn.glVertexWeightfvEXT(HostAddr(weight, const GLfloat *))
#endif

#define FN_GLVERTEXWEIGHTPOINTEREXT(size, type, stride, pointer) \
	GLint texunit = 0; \
	fn.glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE, &texunit); \
	setupClientArray(texunit, contexts[cur_context].weight, size, type, stride, -1, 0, AtariAddr(pointer, nfmemptr)); \
	contexts[cur_context].weight.vendor = NFOSMESA_VENDOR_EXT

/* -------------------------------------------------------------------------- */

/*
 * GL_ARB_vertex_buffer_object
 */

#define FN_GLBINDBUFFERARB(target, buffer) \
	gl_bind_buffer(target, buffer, 0, 0); \
	fn.glBindBufferARB(target, buffer)

#if NFOSMESA_NEED_INT_CONV
#define FN_GLDELETEBUFFERSARB(n, buffers) \
	GLuint tmp[MAX(n, 0)], *ptmp; \
	ptmp = Atari2HostIntArray(n, buffers, tmp); \
	fn.glDeleteBuffersARB(n, ptmp)
#else
#define FN_GLDELETEBUFFERSARB(n, buffers) \
	fn.glDeleteBuffersARB(n, HostAddr(buffers, const GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGENBUFFERSARB(n, buffers) \
	GLsizei const size = MAX(n, 0); \
	GLuint tmp[size]; \
	fn.glGenBuffersARB(n, tmp); \
	Host2AtariIntArray(n, tmp, buffers)
#else
#define FN_GLGENBUFFERSARB(n, buffers) \
	fn.glGenBuffersARB(n, HostAddr(buffers, GLuint *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLBUFFERDATAARB(target, size, data, usage) \
	GLubyte tmp[MAX(size, 0)], *ptmp; \
	ptmp = Atari2HostByteArray(sizeof(tmp), data, tmp); \
	fn.glBufferDataARB(target, size, ptmp, usage)
#else
#define FN_GLBUFFERDATAARB(target, size, data, usage) \
	fn.glBufferDataARB(target, size, HostAddr(data, const void *), usage)
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLBUFFERSUBDATAARB(target, offset, size, data) \
	GLubyte tmp[MAX(size, 0)], *ptmp; \
	ptmp = Atari2HostByteArray(sizeof(tmp), data, tmp); \
	fn.glBufferSubDataARB(target, offset, size, ptmp)
#else
#define FN_GLBUFFERSUBDATAARB(target, offset, size, data) \
	fn.glBufferSubDataARB(target, offset, size, HostAddr(data, const void *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETBUFFERPARAMETERIVARB(target, pname, params) \
	GLint tmp[4]; \
	int const size = nfglGetNumParams(pname); \
	fn.glGetBufferParameterivARB(target, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETBUFFERPARAMETERIVARB(target, pname, params) \
	fn.glGetBufferParameterivARB(target, pname, HostAddr(params, GLint *))
#endif

#define FN_GLGETBUFFERPOINTERVARB(target, pname, params) \
	void *tmp = NULL; \
	fn.glGetBufferPointervARB(target, pname, &tmp); \
	/* TODO */ \
	memptr zero = 0; \
	Host2AtariIntArray(1, &zero, AtariAddr(params, memptr *))

/* -------------------------------------------------------------------------- */

/*
 * GL_ARB_vertex_blend
 */

#if NFOSMESA_NEED_INT_CONV
#define FN_GLWEIGHTSVARB(size, weights) \
	GLint const count = size; \
	GLshort tmp[MAX(count, 0)], *ptmp; \
	ptmp = Atari2HostShortArray(count, weights, tmp); \
	fn.glWeightsvARB(size, ptmp)
#else
#define FN_GLWEIGHTSVARB(size, weights) \
	fn.glWeightsvARB(size, HostAddr(weights, const GLshort *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLWEIGHTIVARB(size, weights) \
	GLint const count = size; \
	GLint tmp[MAX(count, 0)], *ptmp; \
	ptmp = Atari2HostIntArray(count, weights, tmp); \
	fn.glWeightivARB(size, ptmp)
#else
#define FN_GLWEIGHTIVARB(size, weights) \
	fn.glWeightivARB(size, HostAddr(weights, const GLint *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLWEIGHTFVARB(size, weights) \
	GLint const count = size; \
	GLfloat tmp[MAX(count, 0)], *ptmp; \
	ptmp = Atari2HostFloatArray(count, weights, tmp); \
	fn.glWeightfvARB(size, ptmp)
#else
#define FN_GLWEIGHTFVARB(size, weights) \
	fn.glWeightfvARB(size, HostAddr(weights, const GLfloat *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLWEIGHTBVARB(size, weights) \
	GLint const count = size; \
	GLbyte tmp[MAX(count, 0)], *ptmp; \
	ptmp = Atari2HostByteArray(count, weights, tmp); \
	fn.glWeightbvARB(size, ptmp)
#else
#define FN_GLWEIGHTBVARB(size, weights) \
	fn.glWeightbvARB(size, HostAddr(weights, const GLbyte *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLWEIGHTDVARB(size, weights) \
	GLint const count = size; \
	GLdouble tmp[MAX(count, 0)], *ptmp; \
	ptmp = Atari2HostDoubleArray(count, weights, tmp);  \
	fn.glWeightdvARB(size, ptmp)
#else
#define FN_GLWEIGHTDVARB(size, weights) \
	fn.glWeightdvARB(size, HostAddr(weights, const GLdouble *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLWEIGHTUBVARB(size, weights) \
	GLint const count = size; \
	GLubyte tmp[MAX(count, 0)], *ptmp; \
	ptmp = Atari2HostByteArray(count, weights, tmp); \
	fn.glWeightubvARB(size, ptmp)
#else
#define FN_GLWEIGHTUBVARB(size, weights) \
	fn.glWeightubvARB(size, HostAddr(weights, const GLubyte *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLWEIGHTUSVARB(size, weights) \
	GLint const count = size; \
	GLushort tmp[MAX(count, 0)], *ptmp; \
	ptmp = Atari2HostShortArray(count, weights, tmp); \
	fn.glWeightusvARB(size, ptmp)
#else
#define FN_GLWEIGHTUSVARB(size, weights) \
	fn.glWeightusvARB(size, HostAddr(weights, const GLushort *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLWEIGHTUIVARB(size, weights) \
	GLint const count = size; \
	GLuint tmp[MAX(count, 0)], *ptmp; \
	ptmp = Atari2HostIntArray(count, weights, tmp); \
	fn.glWeightuivARB(size, ptmp)
#else
#define FN_GLWEIGHTUIVARB(size, weights) \
	fn.glWeightuivARB(size, HostAddr(weights, const GLuint *))
#endif

#define FN_GLWEIGHTPOINTERARB(size, type, stride, pointer) \
	GLint texunit = 0; \
	fn.glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE, &texunit); \
	setupClientArray(texunit, contexts[cur_context].weight, size, type, stride, -1, 0, AtariAddr(pointer, nfmemptr)); \
	contexts[cur_context].weight.vendor = NFOSMESA_VENDOR_ARB

/* -------------------------------------------------------------------------- */

/*
 * GL_ARB_window_pos
 */

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLWINDOWPOS2DVARB(v) \
	GLdouble tmp[2]; \
	Atari2HostDoubleArray(2, v, tmp); \
	fn.glWindowPos2dvARB(tmp)
#else
#define FN_GLWINDOWPOS2DVARB(v) \
	fn.glWindowPos2dvARB(HostAddr(v, const GLdouble *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLWINDOWPOS2FVARB(v) \
	GLfloat tmp[2]; \
	Atari2HostFloatArray(2, v, tmp); \
	fn.glWindowPos2fvARB(tmp)
#else
#define FN_GLWINDOWPOS2FVARB(v) \
	fn.glWindowPos2fvARB(HostAddr(v, const GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLWINDOWPOS2IVARB(v) \
	GLint tmp[2]; \
	Atari2HostIntArray(2, v, tmp); \
	fn.glWindowPos2ivARB(tmp)
#else
#define FN_GLWINDOWPOS2IVARB(v) \
	fn.glWindowPos2ivARB(HostAddr(v, const GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLWINDOWPOS2SVARB(v) \
	GLshort tmp[2]; \
	Atari2HostShortArray(2, v, tmp); \
	fn.glWindowPos2svARB(tmp)
#else
#define FN_GLWINDOWPOS2SVARB(v) \
	fn.glWindowPos2svARB(HostAddr(v, const GLshort *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLWINDOWPOS3DVARB(v) \
	GLdouble tmp[3]; \
	Atari2HostDoubleArray(3, v, tmp); \
	fn.glWindowPos3dvARB(tmp)
#else
#define FN_GLWINDOWPOS3DVARB(v) \
	fn.glWindowPos3dvARB(HostAddr(v, const GLdouble *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLWINDOWPOS3FVARB(v) \
	GLfloat tmp[3]; \
	Atari2HostFloatArray(3, v, tmp); \
	fn.glWindowPos3fvARB(tmp)
#else
#define FN_GLWINDOWPOS3FVARB(v) \
	fn.glWindowPos3fvARB(HostAddr(v, const GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLWINDOWPOS3IVARB(v) \
	GLint tmp[3]; \
	Atari2HostIntArray(3, v, tmp); \
	fn.glWindowPos3ivARB(tmp)
#else
#define FN_GLWINDOWPOS3IVARB(v) \
	fn.glWindowPos3ivARB(HostAddr(v, const GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLWINDOWPOS3SVARB(v) \
	GLshort tmp[3]; \
	Atari2HostShortArray(3, v, tmp); \
	fn.glWindowPos3svARB(tmp)
#else
#define FN_GLWINDOWPOS3SVARB(v) \
	fn.glWindowPos3svARB(HostAddr(v, const GLshort *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_MESA_window_pos
 */
#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLWINDOWPOS2DVMESA(v) \
	GLdouble tmp[2]; \
	Atari2HostDoubleArray(2, v, tmp); \
	fn.glWindowPos2dvMESA(tmp)
#else
#define FN_GLWINDOWPOS2DVMESA(v) \
	fn.glWindowPos2dvMESA(HostAddr(v, const GLdouble *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLWINDOWPOS2FVMESA(v) \
	GLfloat tmp[2]; \
	Atari2HostFloatArray(2, v, tmp); \
	fn.glWindowPos2fvMESA(tmp)
#else
#define FN_GLWINDOWPOS2FVMESA(v) \
	fn.glWindowPos2fvMESA(HostAddr(v, const GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLWINDOWPOS2IVMESA(v) \
	GLint tmp[2]; \
	Atari2HostIntArray(2, v, tmp); \
	fn.glWindowPos2ivMESA(tmp)
#else
#define FN_GLWINDOWPOS2IVMESA(v) \
	fn.glWindowPos2ivMESA(HostAddr(v, const GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLWINDOWPOS2SVMESA(v) \
	GLshort tmp[2]; \
	Atari2HostShortArray(2, v, tmp); \
	fn.glWindowPos2svMESA(tmp)
#else
#define FN_GLWINDOWPOS2SVMESA(v) \
	fn.glWindowPos2svMESA(HostAddr(v, const GLshort *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLWINDOWPOS3DVMESA(v) \
	GLdouble tmp[3]; \
	Atari2HostDoubleArray(3, v, tmp); \
	fn.glWindowPos3dvMESA(tmp)
#else
#define FN_GLWINDOWPOS3DVMESA(v) \
	fn.glWindowPos3dvMESA(HostAddr(v, const GLdouble *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLWINDOWPOS3FVMESA(v) \
	GLfloat tmp[3]; \
	Atari2HostFloatArray(3, v, tmp); \
	fn.glWindowPos3fvMESA(tmp)
#else
#define FN_GLWINDOWPOS3FVMESA(v) \
	fn.glWindowPos3fvMESA(HostAddr(v, const GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLWINDOWPOS3IVMESA(v) \
	GLint tmp[3]; \
	Atari2HostIntArray(3, v, tmp); \
	fn.glWindowPos3ivMESA(tmp)
#else
#define FN_GLWINDOWPOS3IVMESA(v) \
	fn.glWindowPos3ivMESA(HostAddr(v, const GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLWINDOWPOS3SVMESA(v) \
	GLshort tmp[3]; \
	Atari2HostShortArray(3, v, tmp); \
	fn.glWindowPos3svMESA(tmp)
#else
#define FN_GLWINDOWPOS3SVMESA(v) \
	fn.glWindowPos3svMESA(HostAddr(v, const GLshort *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLWINDOWPOS4DVMESA(v) \
	GLdouble tmp[4]; \
	Atari2HostDoubleArray(4, v, tmp); \
	fn.glWindowPos4dvMESA(tmp)
#else
#define FN_GLWINDOWPOS4DVMESA(v) \
	fn.glWindowPos4dvMESA(HostAddr(v, const GLdouble *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLWINDOWPOS4FVMESA(v) \
	GLfloat tmp[4]; \
	Atari2HostFloatArray(4, v, tmp); \
	fn.glWindowPos4fvMESA(tmp)
#else
#define FN_GLWINDOWPOS4FVMESA(v) \
	fn.glWindowPos4fvMESA(HostAddr(v, const GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLWINDOWPOS4IVMESA(v) \
	GLint tmp[4]; \
	Atari2HostIntArray(4, v, tmp); \
	fn.glWindowPos4ivMESA(tmp)
#else
#define FN_GLWINDOWPOS4IVMESA(v) \
	fn.glWindowPos4ivMESA(HostAddr(v, const GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLWINDOWPOS4SVMESA(v) \
	GLshort tmp[4]; \
	Atari2HostShortArray(4, v, tmp); \
	fn.glWindowPos4svMESA(tmp)
#else
#define FN_GLWINDOWPOS4SVMESA(v) \
	fn.glWindowPos4svMESA(HostAddr(v, const GLshort *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_NV_bindless_multi_draw_indirect_count
 */

#define FN_GLMULTIDRAWARRAYSINDIRECTBINDLESSCOUNTNV(mode, indirect, drawcount, maxDrawCount, stride, vertexBufferCount) \
	if (contexts[cur_context].buffer_bindings.draw_indirect.id) { \
		void *offset = NFHost2AtariAddr(indirect); \
		fn.glMultiDrawArraysIndirectBindlessCountNV(mode, offset, drawcount, maxDrawCount, stride, vertexBufferCount); \
	}

#define FN_GLMULTIDRAWELEMENTSINDIRECTBINDLESSCOUNTNV(mode, type, indirect, drawcount, maxDrawCount, stride, vertexBufferCount) \
	if (contexts[cur_context].buffer_bindings.draw_indirect.id) { \
		void *offset = NFHost2AtariAddr(indirect); \
		fn.glMultiDrawElementsIndirectBindlessCountNV(mode, type, offset, drawcount, maxDrawCount, stride, vertexBufferCount); \
	}

/* -------------------------------------------------------------------------- */

/*
 * GL_NV_command_list
 */

#if NFOSMESA_NEED_INT_CONV
#define FN_GLCREATESTATESNV(n, states) \
	GLuint tmp[MAX(n, 0)]; \
	fn.glCreateStatesNV(n, tmp); \
	Host2AtariIntArray(n, tmp, states)
#else
#define FN_GLCREATESTATESNV(n, states) \
	fn.glCreateStatesNV(n, HostAddr(states, GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLDELETESTATESNV(n, states) \
	GLuint tmp[MAX(n, 0)], *ptmp; \
	ptmp = Atari2HostIntArray(n, states, tmp); \
	fn.glDeleteStatesNV(n, ptmp)
#else
#define FN_GLDELETESTATESNV(n, states) \
	fn.glDeleteStatesNV(n, HostAddr(states, const GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLCREATECOMMANDLISTSNV(n, lists) \
	GLuint tmp[MAX(n, 0)]; \
	fn.glCreateCommandListsNV(n, tmp); \
	Host2AtariIntArray(n, tmp, lists)
#else
#define FN_GLCREATECOMMANDLISTSNV(n, lists) \
	fn.glCreateCommandListsNV(n, HostAddr(lists, GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLDELETECOMMANDLISTSNV(n, lists) \
	GLuint tmp[MAX(n, 0)], *ptmp; \
	ptmp = Atari2HostIntArray(n, lists, tmp); \
	fn.glDeleteCommandListsNV(n, ptmp)
#else
#define FN_GLDELETECOMMANDLISTSNV(n, lists) \
	fn.glDeleteCommandListsNV(n, HostAddr(lists, const GLuint *))
#endif

#define FN_GLDRAWCOMMANDSNV(primitiveMode, buffer, indirects, sizes, count) \
	GLsizei const size = MAX(count, 0); \
	GLintptr tmpind[size], *pindirects; \
	GLsizei tmpsiz[size], *psizes; \
	pindirects = Atari2HostIntptrArray(count, indirects, tmpind); \
	psizes = Atari2HostIntArray(count, sizes, tmpsiz); \
	fn.glDrawCommandsNV(primitiveMode, buffer, pindirects, psizes, count)

#if NFOSMESA_NEED_INT_CONV
#define FN_GLDRAWCOMMANDSADDRESSNV(primitiveMode, indirects, sizes, count) \
	GLuint64 tmpind[MAX(count, 0)], *pind; \
	GLsizei tmpsiz[MAX(count, 0)], *psiz; \
	pind = Atari2HostInt64Array(count, indirects, tmpind); \
	psiz = Atari2HostIntArray(count, sizes, tmpsiz); \
	fn.glDrawCommandsAddressNV(primitiveMode, pind, psiz, count)
#else
#define FN_GLDRAWCOMMANDSADDRESSNV(primitiveMode, indirects, sizes, count) \
	fn.glDrawCommandsAddressNV(primitiveMode, HostAddr(indirects, const GLuint64 *), HostAddr(sizes, const GLsizei *), count)
#endif

#define FN_GLDRAWCOMMANDSSTATESNV(buffer, indirects, sizes, states, fbos, count) \
	GLsizei const size = MAX(count, 0); \
	GLintptr tmpind[size], *pindirects; \
	GLsizei tmpsiz[size], *psizes; \
	GLuint tmpstates[size], *pstates; \
	GLuint tmpfbos[size], *pfbos; \
	pindirects = Atari2HostIntptrArray(count, indirects, tmpind); \
	psizes = Atari2HostIntArray(count, sizes, tmpsiz); \
	pstates = Atari2HostIntArray(count, states, tmpstates); \
	pfbos = Atari2HostIntArray(count, fbos, tmpfbos); \
	fn.glDrawCommandsStatesNV(buffer, pindirects, psizes, pstates, pfbos, count)

#if NFOSMESA_NEED_INT_CONV
#define FN_GLDRAWCOMMANDSSTATESADDRESSNV(indirects, sizes, states, fbos, count) \
	GLuint64 tmpind[MAX(count, 0)], *pind; \
	GLsizei tmpsiz[MAX(count, 0)], *psiz; \
	GLuint tmpstates[MAX(count, 0)], *pstates; \
	GLuint tmpfbos[MAX(count, 0)], *pfbos; \
	pind = Atari2HostInt64Array(count, indirects, tmpind); \
	psiz = Atari2HostIntArray(count, sizes, tmpsiz); \
	pstates = Atari2HostIntArray(count, states, tmpstates); \
	pfbos = Atari2HostIntArray(count, fbos, tmpfbos); \
	fn.glDrawCommandsStatesAddressNV(pind, psiz, pstates, pfbos, count)
#else
#define FN_GLDRAWCOMMANDSSTATESADDRESSNV(indirects, sizes, states, fbos, count) \
	fn.glDrawCommandsStatesAddressNV(HostAddr(indirects, const GLuint64 *), HostAddr(sizes, const GLsizei *), HostAddr(states, const GLuint *), HostAddr(fbos, const GLuint *), count)
#endif

#define FN_GLLISTDRAWCOMMANDSSTATESCLIENTNV(list, segment, indirects, sizes, states, fbos, count) \
	void *tmpind[MAX(count, 0)], **pindirects; \
	GLsizei tmpsiz[MAX(count, 0)], *psizes; \
	GLuint tmpstates[MAX(count, 0)], *pstates; \
	GLuint tmpfbos[MAX(count, 0)], *pfbos; \
	pindirects = Atari2HostPtrArray(count, indirects, tmpind); \
	psizes = Atari2HostIntArray(count, sizes, tmpsiz); \
	pstates = Atari2HostIntArray(count, states, tmpstates); \
	pfbos = Atari2HostIntArray(count, fbos, tmpfbos); \
	fn.glListDrawCommandsStatesClientNV(list, segment, (const void **)pindirects, psizes, pstates, pfbos, count)

/* -------------------------------------------------------------------------- */

/*
 * GL_NV_framebuffer_mixed_samples
 */

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLCOVERAGEMODULATIONTABLENV(n, v) \
	GLint s = 0; \
	fn.glGetIntegerv(GL_COVERAGE_MODULATION_TABLE_SIZE_NV, &s); \
	GLfloat tmp[MAX(s, 0)], *ptmp; \
	ptmp = Atari2HostFloatArray(s, v, tmp); \
	fn.glCoverageModulationTableNV(n, ptmp)
#else
#define FN_GLCOVERAGEMODULATIONTABLENV(n, v) \
	fn.glCoverageModulationTableNV(n, HostAddr(v, const GLfloat *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETCOVERAGEMODULATIONTABLENV(n, v) \
	GLint s = 0; \
	fn.glGetIntegerv(GL_COVERAGE_MODULATION_TABLE_SIZE_NV, &s); \
	GLfloat tmp[MAX(s, 0)]; \
	fn.glGetCoverageModulationTableNV(n, tmp); \
	Host2AtariFloatArray(s, tmp, v)
#else
#define FN_GLGETCOVERAGEMODULATIONTABLENV(n, v) \
	fn.glGetCoverageModulationTableNV(n, HostAddr(v, GLfloat *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_NV_sample_locations
 */

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLFRAMEBUFFERSAMPLELOCATIONSFVNV(target, start, count, v) \
	GLfloat tmp[MAX(count * 2, 0)], *ptmp; \
	ptmp = Atari2HostFloatArray(count * 2, v, tmp); \
	fn.glFramebufferSampleLocationsfvNV(target, start, count, ptmp)
#else
#define FN_GLFRAMEBUFFERSAMPLELOCATIONSFVNV(target, start, count, v) \
	fn.glFramebufferSampleLocationsfvNV(target, start, count, HostAddr(v, const GLfloat *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLNAMEDFRAMEBUFFERSAMPLELOCATIONSFVNV(framebuffer, start, count, v) \
	GLfloat tmp[MAX(count * 2, 0)], *ptmp; \
	ptmp = Atari2HostFloatArray(count * 2, v, tmp); \
	fn.glNamedFramebufferSampleLocationsfvNV(framebuffer, start, count, ptmp)
#else
#define FN_GLNAMEDFRAMEBUFFERSAMPLELOCATIONSFVNV(framebuffer, start, count, v) \
	fn.glNamedFramebufferSampleLocationsfvNV(framebuffer, start, count, HostAddr(v, const GLfloat *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * ARB_indirect_parameters
 */

#define FN_GLMULTIDRAWARRAYSINDIRECTCOUNTARB(mode, indirect, drawcount, maxdrawcount, stride) \
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
		void *offset = NFHost2AtariAddr(indirect); \
		fn.glMultiDrawArraysIndirectCountARB(mode, offset, drawcount, maxdrawcount, stride); \
	} else if (indirect) { \
		if (stride == 0) stride = 4 * sizeof(Uint32); \
		nfcmemptr indptr = (nfcmemptr)indirect; \
		for (GLsizei n = 0; n < maxdrawcount; n++) { \
			GLuint tmp[4] = { 0, 0, 0, 0 }; \
			Atari2HostIntArray(4, indptr, tmp); \
			GLuint count = tmp[0]; \
			convertClientArrays(count); \
			fn.glDrawArraysInstancedBaseInstance(mode, tmp[2], count, tmp[1], tmp[3]); \
			indptr += stride; \
		} \
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
 *    } DrawElemntsIndirectCommand;
 */
#define FN_GLMULTIDRAWELEMENTSINDIRECTCOUNTARB(mode, type, indirect, drawcount, maxdrawcount, stride) \
	if (contexts[cur_context].buffer_bindings.draw_indirect.id) { \
		void *offset = NFHost2AtariAddr(indirect); \
		fn.glMultiDrawElementsIndirectCountARB(mode, type, offset, drawcount, maxdrawcount, stride); \
	} else if (indirect) { \
		if (stride == 0) stride = 5 * sizeof(Uint32); \
		nfcmemptr pind = (nfcmemptr)indirect; \
		for (GLsizei n = 0; n < maxdrawcount; n++) { \
			GLuint tmp[5] = { 0, 0, 0, 0, 0 }; \
			Atari2HostIntArray(5, pind, tmp); \
			GLuint count = tmp[0]; \
			convertClientArrays(count); \
			fn.glDrawElementsInstancedBaseVertexBaseInstance(mode, count, type, (const void *)(uintptr_t)tmp[2], tmp[1], tmp[3], tmp[4]); \
			pind += stride; \
		} \
	}

/* -------------------------------------------------------------------------- */

/*
 * GL_EXT_memory_object_win32
 */
/* FIXME: does not make much sense to return Win32 handle to Atari? */
#if NFOSMESA_NEED_INT_CONV
#define FN_GLIMPORTMEMORYWIN32HANDLEEXT(memory, size, handleType, handle) \
	GLintptr tmp[1]; \
	fn.glImportMemoryWin32HandleEXT(memory, size, handleType, tmp); \
	Host2AtariIntArray(1, (const Uint32 *)tmp, AtariAddr(handle, GLuint *))
#else
#define FN_GLIMPORTMEMORYWIN32HANDLEEXT(memory, size, handleType, handle) \
	fn.glImportMemoryWin32HandleEXT(memory, size, handleType, HostAddr(handle, void *))
#endif
	
/* -------------------------------------------------------------------------- */

/*
 * GL_EXT_semaphore_win32
 */
/* FIXME: does not make much sense to return Win32 handle to Atari? */
#if NFOSMESA_NEED_INT_CONV
#define FN_GLIMPORTSEMAPHOREWIN32HANDLEEXT(semaphore, handleType, handle) \
	GLintptr tmp[1]; \
	fn.glImportSemaphoreWin32HandleEXT(semaphore, handleType, tmp); \
	Host2AtariIntArray(1, (const Uint32 *)tmp, AtariAddr(handle, GLuint *))
#else
#define FN_GLIMPORTSEMAPHOREWIN32HANDLEEXT(semaphore, handleType, handle) \
	fn.glImportSemaphoreWin32HandleEXT(semaphore, handleType, HostAddr(handle, void *))
#endif
	
/* -------------------------------------------------------------------------- */

/*
 * GL_EXT_external_buffer
 */
#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLBUFFERSTORAGEEXTERNALEXT(target, offset, size, clientBuffer, flags) \
	GLubyte tmp[MAX(size, 0)], *ptmp; \
	ptmp = Atari2HostByteArray(sizeof(tmp), AtariAddr(clientBuffer, void *), tmp); \
	fn.glBufferStorageExternalEXT(target, offset, size, ptmp, flags)
#else
#define FN_GLBUFFERSTORAGEEXTERNALEXT(target, offset, size, clientBuffer, flags) \
	fn.glBufferStorageExternalEXT(target, offset, size, AtariAddr(clientBuffer, void *), flags)
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLNAMEDBUFFERSTORAGEEXTERNALEXT(buffer, offset, size, clientBuffer, flags) \
	GLubyte tmp[MAX(size, 0)], *ptmp; \
	ptmp = Atari2HostByteArray(sizeof(tmp), AtariAddr(clientBuffer, void *), tmp); \
	fn.glNamedBufferStorageExternalEXT(buffer, offset, size, ptmp, flags)
#else
#define FN_GLNAMEDBUFFERSTORAGEEXTERNALEXT(buffer, offset, size, clientBuffer, flags) \
	fn.glNamedBufferStorageExternalEXT(buffer, offset, size, AtariAddr(clientBuffer, void *), flags)
#endif

/* -------------------------------------------------------------------------- */

/*
 * Version 1.1
 */

#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_INT_CONV
#define FN_GLARETEXTURESRESIDENT(n, textures, residences) \
	GLuint tmp[MAX(n, 0)], *ptmp; \
	GLboolean resbuf[MAX(n, 0)]; \
	GLboolean result; \
	ptmp = Atari2HostIntArray(n, textures, tmp); \
	result = fn.glAreTexturesResident(n, ptmp, resbuf); \
	Host2AtariByteArray(n, resbuf, residences); \
	return result
#else
#define FN_GLARETEXTURESRESIDENT(n, textures, residences) \
	return fn.glAreTexturesResident(n, HostAddr(textures, const GLuint *), HostAddr(residences, GLboolean *))
#endif

#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV || NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLARRAYELEMENT(i) nfglArrayElementHelper(i)
#else
#define FN_GLARRAYELEMENT(i) fn.glArrayElement(i)
#endif

#define FN_GLCALLLISTS(n, type, lists) \
	pixelBuffer buf(*this); \
	void *tmp = buf.convertArray(n, type, (nfmemptr)lists); \
	fn.glCallLists(n, type, tmp)

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLBITMAP(width, height, xorig, yorig, xmove, ymove, bitmap) \
	if (width <= 0 || height <= 0) return; \
	GLsizei bytes_per_row = (width + 7) / 8; \
	GLsizei size = bytes_per_row * height; \
	GLubyte tmp[size], *ptmp; \
	ptmp = Atari2HostByteArray(size, bitmap, tmp); \
	fn.glBitmap(width, height, xorig, yorig, xmove, ymove, ptmp)
#else
#define FN_GLBITMAP(width, height, xorig, yorig, xmove, ymove, bitmap) \
	fn.glBitmap(width, height, xorig, yorig, xmove, ymove, HostAddr(bitmap, const GLubyte *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLCLIPPLANE(plane, equation) \
	GLdouble tmp[4]; \
	Atari2HostDoubleArray(4, equation, tmp); \
	fn.glClipPlane(plane, tmp)
#else
#define FN_GLCLIPPLANE(plane, equation) \
	fn.glClipPlane(plane, HostAddr(equation, const GLdouble *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLCOLOR3BV(v) \
	GLbyte tmp[3]; \
	Atari2HostByteArray(3, v, tmp); \
	fn.glColor3bv(tmp)
#else
#define FN_GLCOLOR3BV(v) \
	fn.glColor3bv(HostAddr(v, const GLbyte *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLCOLOR3UBV(v) \
	GLubyte tmp[3]; \
	Atari2HostByteArray(3, v, tmp); \
	fn.glColor3ubv(tmp)
#else
#define FN_GLCOLOR3UBV(v) \
	fn.glColor3ubv(HostAddr(v, const GLubyte *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLCOLOR3DV(v) \
	GLdouble tmp[3]; \
	Atari2HostDoubleArray(3, v, tmp); \
	fn.glColor3dv(tmp)
#else
#define FN_GLCOLOR3DV(v) \
	fn.glColor3dv(HostAddr(v, const GLdouble *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLCOLOR3FV(v) \
	GLfloat tmp[3]; \
	Atari2HostFloatArray(3, v, tmp); \
	fn.glColor3fv(tmp)
#else
#define FN_GLCOLOR3FV(v) \
	fn.glColor3fv(HostAddr(v, const GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLCOLOR3IV(v) \
	GLint tmp[3]; \
	Atari2HostIntArray(3, v, tmp); \
	fn.glColor3iv(tmp)
#else
#define FN_GLCOLOR3IV(v) \
	fn.glColor3iv(HostAddr(v, const GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLCOLOR3SV(v) \
	GLshort tmp[3]; \
	Atari2HostShortArray(3, v, tmp); \
	fn.glColor3sv(tmp)
#else
#define FN_GLCOLOR3SV(v) \
	fn.glColor3sv(HostAddr(v, const GLshort *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLCOLOR3UIV(v) \
	GLuint tmp[3]; \
	Atari2HostIntArray(3, v, tmp); \
	fn.glColor3uiv(tmp)
#else
#define FN_GLCOLOR3UIV(v) \
	fn.glColor3uiv(HostAddr(v, const GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLCOLOR3USV(v) \
	GLushort tmp[3]; \
	Atari2HostShortArray(3, v, tmp); \
	fn.glColor3usv(tmp)
#else
#define FN_GLCOLOR3USV(v) \
	fn.glColor3usv(HostAddr(v, const GLushort *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLCOLOR4BV(v) \
	GLbyte tmp[4]; \
	Atari2HostByteArray(4, v, tmp); \
	fn.glColor4bv(tmp)
#else
#define FN_GLCOLOR4BV(v) \
	fn.glColor4bv(HostAddr(v, const GLbyte *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLCOLOR4UBV(v) \
	GLubyte tmp[4]; \
	Atari2HostByteArray(4, v, tmp); \
	fn.glColor4ubv(tmp)
#else
#define FN_GLCOLOR4UBV(v) \
	fn.glColor4ubv(HostAddr(v, const GLubyte *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLCOLOR4DV(v) \
	GLdouble tmp[4]; \
	Atari2HostDoubleArray(4, v, tmp); \
	fn.glColor4dv(tmp)
#else
#define FN_GLCOLOR4DV(v) \
	fn.glColor4dv(HostAddr(v, const GLdouble *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLCOLOR4FV(v) \
	GLfloat tmp[4]; \
	Atari2HostFloatArray(4, v, tmp); \
	fn.glColor4fv(tmp)
#else
#define FN_GLCOLOR4FV(v) \
	fn.glColor4fv(HostAddr(v, const GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLCOLOR4IV(v) \
	GLint tmp[4]; \
	Atari2HostIntArray(4, v, tmp); \
	fn.glColor4iv(tmp)
#else
#define FN_GLCOLOR4IV(v) \
	fn.glColor4iv(HostAddr(v, const GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLCOLOR4SV(v) \
	GLshort tmp[4]; \
	Atari2HostShortArray(4, v, tmp); \
	fn.glColor4sv(tmp)
#else
#define FN_GLCOLOR4SV(v) \
	fn.glColor4sv(HostAddr(v, const GLshort *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLCOLOR4UIV(v) \
	GLuint tmp[4]; \
	Atari2HostIntArray(4, v, tmp); \
	fn.glColor4uiv(tmp)
#else
#define FN_GLCOLOR4UIV(v) \
	fn.glColor4uiv(HostAddr(v, const GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLCOLOR4USV(v) \
	GLushort tmp[4]; \
	Atari2HostShortArray(4, v, tmp); \
	fn.glColor4usv(tmp)
#else
#define FN_GLCOLOR4USV(v) \
	fn.glColor4usv(HostAddr(v, const GLushort *))
#endif

#define FN_GLCOLORPOINTER(size, type, stride, pointer) \
	GLint texunit = 0; \
	fn.glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE, &texunit); \
	setupClientArray(texunit, contexts[cur_context].color, size, type, stride, -1, 0, AtariAddr(pointer, nfmemptr))

#if NFOSMESA_NEED_INT_CONV
#define FN_GLDELETETEXTURES(n, textures) \
	GLuint tmp[MAX(n, 0)], *ptmp; \
	ptmp = Atari2HostIntArray(n, textures, tmp); \
	fn.glDeleteTextures(n, ptmp)
#else
#define FN_GLDELETETEXTURES(n, textures) \
	fn.glDeleteTextures(n, HostAddr(textures, const GLuint *))
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
		case GL_WEIGHT_ARRAY_ARB: \
			contexts[cur_context].enabled_arrays &= ~NFOSMESA_WEIGHT_ARRAY; \
			break; \
		case GL_MATRIX_INDEX_ARRAY_ARB: \
			contexts[cur_context].enabled_arrays &= ~NFOSMESA_MATRIX_INDEX_ARRAY; \
			break; \
		case GL_REPLACEMENT_CODE_ARRAY_SUN: \
			contexts[cur_context].enabled_arrays &= ~NFOSMESA_REPLACEMENT_CODE_ARRAY; \
			break; \
	} \
	fn.glDisableClientState(array)

#define FN_GLDRAWARRAYS(mode, first, count) \
	convertClientArrays(first + count); \
	fn.glDrawArrays(mode, first, count)

#define FN_GLDRAWELEMENTS(mode, count, type, indices) \
	void *tmp; \
	convertClientArrays(count); \
	pixelBuffer buf(*this); \
	switch(type) { \
	case GL_UNSIGNED_BYTE: \
	case GL_UNSIGNED_SHORT: \
	case GL_UNSIGNED_INT: \
		tmp = buf.convertArray(count, type, (nfmemptr)indices); \
		break; \
	default: \
		glSetError(GL_INVALID_ENUM); \
		return; \
	} \
	fn.glDrawElements(mode, count, type, tmp)

#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLDRAWPIXELS(width, height, format, type, pixels) \
	pixelBuffer buf(*this); \
	void *tmp = buf.convertPixels(width, height, 1, format, type, (nfmemptr)pixels); \
	fn.glDrawPixels(width, height, format, type, tmp)
#else
#define FN_GLDRAWPIXELS(width, height, format, type, pixels) \
	fn.glDrawPixels(width, height, format, type, HostAddr(pixels, const void *))
#endif

#define FN_GLEDGEFLAGPOINTER(stride, pointer) \
	GLint texunit = 0; \
	fn.glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE, &texunit); \
	setupClientArray(texunit, contexts[cur_context].edgeflag, 1, GL_UNSIGNED_BYTE, stride, -1, 0, AtariAddr(pointer, nfmemptr))

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLEDGEFLAGV(flag) \
	GLboolean tmp[1]; \
	Atari2HostByteArray(1, flag, tmp); \
 	fn.glEdgeFlagv(tmp)
#else
#define FN_GLEDGEFLAGV(flag) \
	fn.glEdgeFlagv(HostAddr(flag, const GLboolean *))
#endif

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
		case GL_WEIGHT_ARRAY_ARB: \
			contexts[cur_context].enabled_arrays |= NFOSMESA_WEIGHT_ARRAY; \
			break; \
		case GL_MATRIX_INDEX_ARRAY_ARB: \
			contexts[cur_context].enabled_arrays |= NFOSMESA_MATRIX_INDEX_ARRAY; \
			break; \
		case GL_REPLACEMENT_CODE_ARRAY_SUN: \
			contexts[cur_context].enabled_arrays |= NFOSMESA_REPLACEMENT_CODE_ARRAY; \
			break; \
	} \
	fn.glEnableClientState(array)

#define FN_GLENABLE(cap) \
	switch (cap) { \
	case GL_NFOSMESA_ERROR_CHECK: \
		contexts[cur_context].error_check_enabled = GL_TRUE; \
		return; \
	} \
	fn.glEnable(cap)

#define FN_GLDISABLE(cap) \
	switch (cap) { \
	case GL_NFOSMESA_ERROR_CHECK: \
		contexts[cur_context].error_check_enabled = GL_FALSE; \
		return; \
	} \
	fn.glDisable(cap)

#define FN_GLISENABLED(cap) \
	switch (cap) { \
	case GL_NFOSMESA_ERROR_CHECK: \
		return contexts[cur_context].error_check_enabled; \
	} \
	return fn.glIsEnabled(cap)

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLEVALCOORD1DV(u) \
	GLdouble tmp[1]; \
	Atari2HostDoubleArray(1, u, tmp); \
	fn.glEvalCoord1dv(tmp)
#else
#define FN_GLEVALCOORD1DV(u) \
	fn.glEvalCoord1dv(HostAddr(u, const GLdouble *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLEVALCOORD1FV(u) \
	GLfloat tmp[1]; \
	Atari2HostFloatArray(1, u, tmp); \
	fn.glEvalCoord1fv(tmp)
#else
#define FN_GLEVALCOORD1FV(u) \
	fn.glEvalCoord1fv(HostAddr(u, const GLfloat *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLEVALCOORD2DV(u) \
	GLdouble tmp[2]; \
	Atari2HostDoubleArray(2, u, tmp); \
	fn.glEvalCoord2dv(tmp)
#else
#define FN_GLEVALCOORD2DV(u) \
	fn.glEvalCoord2dv(HostAddr(u, const GLdouble *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLEVALCOORD2FV(u) \
	GLfloat tmp[2]; \
	Atari2HostFloatArray(2, u, tmp); \
	fn.glEvalCoord2fv(tmp)
#else
#define FN_GLEVALCOORD2FV(u) \
	fn.glEvalCoord2fv(HostAddr(u, const GLfloat *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLFEEDBACKBUFFER(size, type, buffer) \
	contexts[cur_context].feedback_buffer_atari = (nfmemptr)buffer; \
	free(contexts[cur_context].feedback_buffer_host); \
	contexts[cur_context].feedback_buffer_host = malloc(size * sizeof(GLfloat)); \
	if (!contexts[cur_context].feedback_buffer_host) { glSetError(GL_OUT_OF_MEMORY); return; } \
	contexts[cur_context].feedback_buffer_type = GL_FLOAT; \
	fn.glFeedbackBuffer(size, type, (GLfloat *)contexts[cur_context].feedback_buffer_host)
#else
#define FN_GLFEEDBACKBUFFER(size, type, buffer) \
	fn.glFeedbackBuffer(size, type, HostAddr(buffer, GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV || NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLFINISH() \
	fn.glFinish(); \
	ConvertContext(cur_context)
#else
#define FN_GLFINISH() fn.glFinish()
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV || NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLFLUSH() \
	fn.glFlush(); \
	ConvertContext(cur_context)
#else
#define FN_GLFLUSH() fn.glFlush()
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLFOGFV(pname, params) \
	int const size = nfglGetNumParams(pname); \
	GLfloat tmp[MAX(size, 16)]; \
	Atari2HostFloatArray(size, params, tmp); \
	fn.glFogfv(pname, tmp)
#else
#define FN_GLFOGFV(pname, params) \
	fn.glFogfv(pname, HostAddr(params, const GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLFOGIV(pname, params) \
	int const size = nfglGetNumParams(pname); \
	GLint tmp[MAX(size, 16)]; \
	Atari2HostIntArray(size, params, tmp); \
	fn.glFogiv(pname, tmp)
#else
#define FN_GLFOGIV(pname, params) \
	fn.glFogiv(pname, HostAddr(params, const GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGENTEXTURES(n, textures) \
	GLsizei const size = MAX(n, 0); \
	GLuint tmp[size]; \
	fn.glGenTextures(n, tmp); \
	Host2AtariIntArray(n, tmp, textures)
#else
#define FN_GLGENTEXTURES(n, textures) \
	fn.glGenTextures(n, HostAddr(textures, GLuint *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLGETCLIPPLANE(plane, equation) \
	GLdouble tmp[4]; \
	fn.glGetClipPlane(plane, tmp); \
	Host2AtariDoubleArray(4, tmp, equation)
#else
#define FN_GLGETCLIPPLANE(plane, equation) \
	fn.glGetClipPlane(plane, HostAddr(equation, GLdouble *))
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
#define FN_GLGETDOUBLEV(pname, params) \
	fn.glGetDoublev(pname, HostAddr(params, GLdouble *))
#endif

#define FN_GLGETERROR() \
	GLenum e = contexts[cur_context].error_code; \
	contexts[cur_context].error_code = GL_NO_ERROR; \
	if (e != GL_NO_ERROR) return e; \
	return fn.glGetError()

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
#define FN_GLGETFLOATV(pname, params) \
	fn.glGetFloatv(pname, HostAddr(params, GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETINTEGERV(pname, params) \
	int n; \
	n = nfglGetNumParams(pname); \
	if (n > 16) { \
		GLint *tmp; \
		tmp = (GLint *)malloc(n * sizeof(*tmp)); \
		fn.glGetIntegerv(pname, tmp); \
		Host2AtariIntArray(n, tmp, params); \
		free(tmp); \
	} else { \
		GLint tmp[16]; \
		fn.glGetIntegerv(pname, tmp); \
		Host2AtariIntArray(n, tmp, params); \
	}
#else
#define FN_GLGETINTEGERV(pname, params) \
	fn.glGetIntegerv(pname, HostAddr(params, GLint *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLGETBOOLEANV(pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLboolean tmp[MAX(size, 16)]; \
	fn.glGetBooleanv(pname, tmp); \
	Host2AtariByteArray(size, tmp, params)
#else
#define FN_GLGETBOOLEANV(pname, params) \
	fn.glGetBooleanv(pname, HostAddr(params, GLboolean *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETLIGHTFV(light, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLfloat tmp[MAX(size, 16)]; \
	fn.glGetLightfv(light, pname, tmp); \
	Host2AtariFloatArray(size, tmp, params)
#else
#define FN_GLGETLIGHTFV(light, pname, params) \
	fn.glGetLightfv(light, pname, HostAddr(params, GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETLIGHTIV(light, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLint tmp[MAX(size, 16)]; \
	fn.glGetLightiv(light, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETLIGHTIV(light, pname, params) \
	fn.glGetLightiv(light, pname, HostAddr(params, GLint *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLGETMAPDV(target, query, v) \
	GLint order[2] = { 0, 0 }; \
	GLint size; \
	GLdouble *tmp; \
	GLdouble tmpbuf[4]; \
	switch (target) { \
		case GL_MAP1_INDEX: \
		case GL_MAP1_TEXTURE_COORD_1: \
		case GL_MAP1_TEXTURE_COORD_2: \
		case GL_MAP1_VERTEX_3: \
		case GL_MAP1_NORMAL: \
		case GL_MAP1_TEXTURE_COORD_3: \
		case GL_MAP1_VERTEX_4: \
		case GL_MAP1_COLOR_4: \
		case GL_MAP1_TEXTURE_COORD_4: \
			switch (query) { \
			case GL_COEFF: \
				size = __glGetMap_Evalk(target); \
				fn.glGetMapiv(target, GL_ORDER, order); \
				if (order[0] <= 0) return; \
				size *= order[0]; \
				tmp = (GLdouble *)malloc(size * sizeof(*tmp)); \
				if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
				fn.glGetMapdv(target, query, tmp); \
				Host2AtariDoubleArray(size, tmp, v); \
				free(tmp); \
				break; \
			case GL_DOMAIN: \
				fn.glGetMapdv(target, query, tmpbuf); \
				size = 2; \
				Host2AtariDoubleArray(size, tmpbuf, v); \
				break; \
			case GL_ORDER: \
				fn.glGetMapdv(target, query, tmpbuf); \
				size = 1; \
				Host2AtariDoubleArray(size, tmpbuf, v); \
				break; \
			} \
			break; \
		case GL_MAP2_INDEX: \
		case GL_MAP2_TEXTURE_COORD_1: \
		case GL_MAP2_TEXTURE_COORD_2: \
		case GL_MAP2_VERTEX_3: \
		case GL_MAP2_NORMAL: \
		case GL_MAP2_TEXTURE_COORD_3: \
		case GL_MAP2_VERTEX_4: \
		case GL_MAP2_COLOR_4: \
		case GL_MAP2_TEXTURE_COORD_4: \
			switch (query) { \
			case GL_COEFF: \
				size = __glGetMap_Evalk(target); \
				fn.glGetMapiv(target, GL_ORDER, order); \
				if (order[0] <= 0 || order[1] <= 0) return; \
				size *= order[0] * order[1]; \
				tmp = (GLdouble *)malloc(size * sizeof(*tmp)); \
				if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
				fn.glGetMapdv(target, query, tmp); \
				Host2AtariDoubleArray(size, tmp, v); \
				free(tmp); \
				break; \
			case GL_DOMAIN: \
				fn.glGetMapdv(target, query, tmpbuf); \
				size = 4; \
				Host2AtariDoubleArray(size, tmpbuf, v); \
				break; \
			case GL_ORDER: \
				fn.glGetMapdv(target, query, tmpbuf); \
				size = 2; \
				Host2AtariDoubleArray(size, tmpbuf, v); \
				break; \
			} \
			break; \
	}
#else
#define FN_GLGETMAPDV(target, query, v) \
	fn.glGetMapdv(target, query, HostAddr(v, GLdouble *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETMAPFV(target, query, v) \
	GLint order[2] = { 0, 0 }; \
	GLint size; \
	GLfloat *tmp; \
	GLfloat tmpbuf[4]; \
	switch (target) { \
		case GL_MAP1_INDEX: \
		case GL_MAP1_TEXTURE_COORD_1: \
		case GL_MAP1_TEXTURE_COORD_2: \
		case GL_MAP1_VERTEX_3: \
		case GL_MAP1_NORMAL: \
		case GL_MAP1_TEXTURE_COORD_3: \
		case GL_MAP1_VERTEX_4: \
		case GL_MAP1_COLOR_4: \
		case GL_MAP1_TEXTURE_COORD_4: \
			switch (query) { \
			case GL_COEFF: \
				size = __glGetMap_Evalk(target); \
				fn.glGetMapiv(target, GL_ORDER, order); \
				if (order[0] <= 0) return; \
				size *= order[0]; \
				tmp = (GLfloat *)malloc(size * sizeof(*tmp)); \
				if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
				fn.glGetMapfv(target, query, tmp); \
				Host2AtariFloatArray(size, tmp, v); \
				free(tmp); \
				break; \
			case GL_DOMAIN: \
				fn.glGetMapfv(target, query, tmpbuf); \
				size = 2; \
				Host2AtariFloatArray(size, tmpbuf, v); \
				break; \
			case GL_ORDER: \
				fn.glGetMapfv(target, query, tmpbuf); \
				size = 1; \
				Host2AtariFloatArray(size, tmpbuf, v); \
				break; \
			} \
			break; \
		case GL_MAP2_INDEX: \
		case GL_MAP2_TEXTURE_COORD_1: \
		case GL_MAP2_TEXTURE_COORD_2: \
		case GL_MAP2_VERTEX_3: \
		case GL_MAP2_NORMAL: \
		case GL_MAP2_TEXTURE_COORD_3: \
		case GL_MAP2_VERTEX_4: \
		case GL_MAP2_COLOR_4: \
		case GL_MAP2_TEXTURE_COORD_4: \
			switch (query) { \
			case GL_COEFF: \
				size = __glGetMap_Evalk(target); \
				fn.glGetMapiv(target, GL_ORDER, order); \
				if (order[0] <= 0 || order[1] <= 0) return; \
				size *= order[0] * order[1]; \
				tmp = (GLfloat *)malloc(size * sizeof(*tmp)); \
				if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
				fn.glGetMapfv(target, query, tmp); \
				Host2AtariFloatArray(size, tmp, v); \
				free(tmp); \
				break; \
			case GL_DOMAIN: \
				fn.glGetMapfv(target, query, tmpbuf); \
				size = 4; \
				Host2AtariFloatArray(size, tmpbuf, v); \
				break; \
			case GL_ORDER: \
				fn.glGetMapfv(target, query, tmpbuf); \
				size = 2; \
				Host2AtariFloatArray(size, tmpbuf, v); \
				break; \
			} \
			break; \
	}
#else
#define FN_GLGETMAPFV(target, query, v) \
	fn.glGetMapfv(target, query, HostAddr(v, GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETMAPIV(target, query, v) \
	GLint order[2] = { 0, 0 }; \
	GLint size; \
	GLint *tmp; \
	GLint tmpbuf[4]; \
	switch (target) { \
		case GL_MAP1_INDEX: \
		case GL_MAP1_TEXTURE_COORD_1: \
		case GL_MAP1_TEXTURE_COORD_2: \
		case GL_MAP1_VERTEX_3: \
		case GL_MAP1_NORMAL: \
		case GL_MAP1_TEXTURE_COORD_3: \
		case GL_MAP1_VERTEX_4: \
		case GL_MAP1_COLOR_4: \
		case GL_MAP1_TEXTURE_COORD_4: \
			switch (query) { \
			case GL_COEFF: \
				size = __glGetMap_Evalk(target); \
				fn.glGetMapiv(target, GL_ORDER, order); \
				if (order[0] <= 0) return; \
				size *= order[0]; \
				tmp = (GLint *)malloc(size * sizeof(*tmp)); \
				if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
				fn.glGetMapiv(target, query, tmp); \
				Host2AtariIntArray(size, tmp, v); \
				free(tmp); \
				break; \
			case GL_DOMAIN: \
				fn.glGetMapiv(target, query, tmpbuf); \
				size = 2; \
				Host2AtariIntArray(size, tmpbuf, v); \
				break; \
			case GL_ORDER: \
				fn.glGetMapiv(target, query, tmpbuf); \
				size = 1; \
				Host2AtariIntArray(size, tmpbuf, v); \
				break; \
			} \
			break; \
		case GL_MAP2_INDEX: \
		case GL_MAP2_TEXTURE_COORD_1: \
		case GL_MAP2_TEXTURE_COORD_2: \
		case GL_MAP2_VERTEX_3: \
		case GL_MAP2_NORMAL: \
		case GL_MAP2_TEXTURE_COORD_3: \
		case GL_MAP2_VERTEX_4: \
		case GL_MAP2_COLOR_4: \
		case GL_MAP2_TEXTURE_COORD_4: \
			switch (query) { \
			case GL_COEFF: \
				size = __glGetMap_Evalk(target); \
				fn.glGetMapiv(target, GL_ORDER, order); \
				if (order[0] <= 0 || order[1] <= 0) return; \
				size *= order[0] * order[1]; \
				tmp = (GLint *)malloc(size * sizeof(*tmp)); \
				if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
				fn.glGetMapiv(target, query, tmp); \
				Host2AtariIntArray(size, tmp, v); \
				free(tmp); \
				break; \
			case GL_DOMAIN: \
				fn.glGetMapiv(target, query, tmpbuf); \
				size = 4; \
				Host2AtariIntArray(size, tmpbuf, v); \
				break; \
			case GL_ORDER: \
				fn.glGetMapiv(target, query, tmpbuf); \
				size = 2; \
				Host2AtariIntArray(size, tmpbuf, v); \
				break; \
			} \
			break; \
	}
#else
#define FN_GLGETMAPIV(target, query, v) \
	fn.glGetMapiv(target, query, HostAddr(v, GLint *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETMATERIALFV(face, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLfloat tmp[MAX(size, 16)]; \
	fn.glGetMaterialfv(face, pname, tmp); \
	Host2AtariFloatArray(size, tmp, params)
#else
#define FN_GLGETMATERIALFV(face, pname, params) \
	fn.glGetMaterialfv(face, pname, HostAddr(params, GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETMATERIALIV(face, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLint tmp[MAX(size, 16)]; \
	fn.glGetMaterialiv(face, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETMATERIALIV(face, pname, params) \
	fn.glGetMaterialiv(face, pname, HostAddr(params, GLint *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETPIXELMAPFV(map, values) \
	GLint const size = nfglPixelmapSize(map);\
	if (size <= 0) return; \
	GLfloat tmp[size]; \
	fn.glGetPixelMapfv(map, tmp); \
	Host2AtariFloatArray(size, tmp, values)
#else
#define FN_GLGETPIXELMAPFV(map, values) \
	fn.glGetPixelMapfv(map, HostAddr(values, GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETPIXELMAPUIV(map, values) \
	GLint const size = nfglPixelmapSize(map);\
	if (size <= 0) return; \
	GLuint tmp[size]; \
	fn.glGetPixelMapuiv(map, tmp); \
	Host2AtariIntArray(size, tmp, values)
#else
#define FN_GLGETPIXELMAPUIV(map, values) \
	fn.glGetPixelMapuiv(map, HostAddr(values, GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETPIXELMAPUSV(map, values) \
	GLint const size = nfglPixelmapSize(map);\
	if (size <= 0) return; \
	GLushort tmp[size]; \
	fn.glGetPixelMapusv(map, tmp); \
	Host2AtariShortArray(size, tmp, values)
#else
#define FN_GLGETPIXELMAPUSV(map, values) \
	fn.glGetPixelMapusv(map, HostAddr(values, GLushort *))
#endif

#define FN_GLGETPOINTERV(pname, data) \
	GLint texunit = 0; \
	fn.glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE, &texunit); \
	gl_get_pointer(pname, texunit, HostAddr(data, void **))

/*
 * FIXME: If a non-zero named buffer object is bound to the GL_PIXEL_PACK_BUFFER target 
            (see glBindBuffer) while a polygon stipple pattern is
            requested, pattern is treated as a byte offset into the buffer object's data store.
        
 */
#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLGETPOLYGONSTIPPLE(mask) \
	GLint const size = 32 * 32 / 8; \
	GLubyte tmp[size]; \
	fn.glGetPolygonStipple(tmp); \
	Host2AtariByteArray(size, tmp, mask)
#else
#define FN_GLGETPOLYGONSTIPPLE(mask) \
	fn.glGetPolygonStipple(HostAddr(mask, GLubyte *))
#endif

/*
 * FIXME: If a non-zero named buffer object is bound to the GL_PIXEL_UNPACK_BUFFER target 
            (see glBindBuffer) while a stipple pattern is
            specified, pattern is treated as a byte offset into the buffer object's data store.
 */
#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLPOLYGONSTIPPLE(pattern) \
	GLint const size = 32 * 32 / 8; \
	GLubyte tmp[size], *ptmp; \
	ptmp = Atari2HostByteArray(size, pattern, tmp); \
	fn.glPolygonStipple(ptmp)
#else
#define FN_GLPOLYGONSTIPPLE(pattern) \
	fn.glPolygonStipple(HostAddr(pattern, GLubyte *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETTEXENVFV(target, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLfloat tmp[MAX(size, 16)]; \
	fn.glGetTexEnvfv(target, pname, tmp); \
	Host2AtariFloatArray(size, tmp, params)
#else
#define FN_GLGETTEXENVFV(target, pname, params) \
	fn.glGetTexEnvfv(target, pname, HostAddr(params, GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETTEXENVIV(target, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLint tmp[MAX(size, 16)]; \
	fn.glGetTexEnviv(target, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETTEXENVIV(target, pname, params) \
	fn.glGetTexEnviv(target, pname, HostAddr(params, GLint *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLGETTEXGENDV(target, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLdouble tmp[MAX(size, 16)]; \
	fn.glGetTexGendv(target, pname, tmp); \
	Host2AtariDoubleArray(size, tmp, params)
#else
#define FN_GLGETTEXGENDV(target, pname, params) \
	fn.glGetTexGendv(target, pname, HostAddr(params, GLdouble *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETTEXGENFV(target, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLfloat tmp[MAX(size, 16)]; \
	fn.glGetTexGenfv(target, pname, tmp); \
	Host2AtariFloatArray(size, tmp, params)
#else
#define FN_GLGETTEXGENFV(target, pname, params) \
	fn.glGetTexGenfv(target, pname, HostAddr(params, GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETTEXGENIV(target, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLint tmp[MAX(size, 16)]; \
	fn.glGetTexGeniv(target, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETTEXGENIV(target, pname, params) \
	fn.glGetTexGeniv(target, pname, HostAddr(params, GLint *))
#endif

#define FN_GLGETTEXIMAGE(target, level, format, type, img) \
	GLint width = 0, height = 1, depth = 1; \
	char *src; \
	nfmemptr dst; \
	pixelBuffer buf(*this); \
	 \
	switch (target) { \
	case GL_TEXTURE_1D: \
	case GL_PROXY_TEXTURE_1D: \
		fn.glGetTexLevelParameteriv(target, level, GL_TEXTURE_WIDTH, &width); \
		break; \
	case GL_TEXTURE_2D: \
	case GL_PROXY_TEXTURE_2D: \
	case GL_TEXTURE_CUBE_MAP_POSITIVE_X: \
	case GL_TEXTURE_CUBE_MAP_POSITIVE_Y: \
	case GL_TEXTURE_CUBE_MAP_POSITIVE_Z: \
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_X: \
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y: \
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z: \
	case GL_PROXY_TEXTURE_CUBE_MAP: \
		fn.glGetTexLevelParameteriv(target, level, GL_TEXTURE_WIDTH, &width); \
		fn.glGetTexLevelParameteriv(target, level, GL_TEXTURE_HEIGHT, &height); \
		break; \
	case GL_TEXTURE_3D: \
	case GL_PROXY_TEXTURE_3D: \
		fn.glGetTexLevelParameteriv(target, level, GL_TEXTURE_WIDTH, &width); \
		fn.glGetTexLevelParameteriv(target, level, GL_TEXTURE_HEIGHT, &height); \
		fn.glGetTexLevelParameteriv(target, level, GL_TEXTURE_DEPTH, &depth); \
		break; \
	default: \
		glSetError(GL_INVALID_ENUM); \
		return; \
	} \
	if (contexts[cur_context].buffer_bindings.pixel_pack.id) { \
		void *offset = NFHost2AtariAddr(img); \
		fn.glGetTexImage(target, level, format, type, offset); \
		src = contexts[cur_context].buffer_bindings.pixel_pack.host_pointer + (uintptr_t)offset; \
		dst = contexts[cur_context].buffer_bindings.pixel_pack.atari_pointer + (uintptr_t)offset; \
		if (!buf.params(width, height, depth, format, type)) return; \
	} else { \
		src = buf.hostBuffer(width, height, depth, format, type, (nfmemptr)img); \
		if (src == NULL) return; \
		fn.glGetTexImage(target, level, format, type, src); \
		dst = (nfmemptr)img; \
	} \
	buf.convertToAtari(src, dst)

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETTEXLEVELPARAMETERFV(target, level, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLfloat tmp[MAX(size, 16)]; \
	fn.glGetTexLevelParameterfv(target, level, pname, tmp); \
	Host2AtariFloatArray(size, tmp, params)
#else
#define FN_GLGETTEXLEVELPARAMETERFV(target, level, pname, params) \
	fn.glGetTexLevelParameterfv(target, level, pname, HostAddr(params, GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETTEXLEVELPARAMETERIV(target, level, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLint tmp[MAX(size, 16)]; \
	fn.glGetTexLevelParameteriv(target, level, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETTEXLEVELPARAMETERIV(target, level, pname, params) \
	fn.glGetTexLevelParameteriv(target, level, pname, HostAddr(params, GLint *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETTEXPARAMETERFV(target, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLfloat tmp[MAX(size, 16)]; \
	fn.glGetTexParameterfv(target, pname, tmp); \
	Host2AtariFloatArray(size, tmp, params)
#else
#define FN_GLGETTEXPARAMETERFV(target, pname, params) \
	fn.glGetTexParameterfv(target, pname, HostAddr(params, GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETTEXPARAMETERIV(target, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLint tmp[MAX(size, 16)]; \
	fn.glGetTexParameteriv(target, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETTEXPARAMETERIV(target, pname, params) \
	fn.glGetTexParameteriv(target, pname, HostAddr(params, GLint *))
#endif

#define FN_GLINDEXPOINTER(type, stride, pointer) \
	GLint texunit = 0; \
	fn.glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE, &texunit); \
	setupClientArray(texunit, contexts[cur_context].index, 1, type, stride, -1, 0, AtariAddr(pointer, nfmemptr))

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLINDEXDV(c) \
	GLdouble tmp[1]; \
	Atari2HostDoubleArray(1, c, tmp); \
	fn.glIndexdv(tmp)
#else
#define FN_GLINDEXDV(c) \
	fn.glIndexdv(HostAddr(c, const GLdouble *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLINDEXFV(c) \
	GLfloat tmp[1]; \
	Atari2HostFloatArray(1, c, tmp); \
	fn.glIndexfv(tmp)
#else
#define FN_GLINDEXFV(c) \
	fn.glIndexfv(HostAddr(c, const GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLINDEXIV(c) \
	GLint tmp[1]; \
	Atari2HostIntArray(1, c, tmp); \
	fn.glIndexiv(tmp)
#else
#define FN_GLINDEXIV(c) \
	fn.glIndexiv(HostAddr(c, const GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLINDEXSV(c) \
	GLshort tmp[1]; \
	Atari2HostShortArray(1, c, tmp); \
	fn.glIndexsv(tmp)
#else
#define FN_GLINDEXSV(c) \
	fn.glIndexsv(HostAddr(c, const GLshort *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLINDEXUBV(c) \
	GLubyte tmp[1]; \
	Atari2HostByteArray(1, c, tmp); \
	fn.glIndexubv(tmp)
#else
#define FN_GLINDEXUBV(c) \
	fn.glIndexubv(HostAddr(c, const GLubyte *))
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV || NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLINTERLEAVEDARRAYS(format, stride, pointer) \
	nfglInterleavedArraysHelper(format, stride, AtariAddr(pointer, nfmemptr))
#else
#define FN_GLINTERLEAVEDARRAYS(format, stride, pointer) \
	fn.glInterleavedArrays(format, stride, HostAddr(pointer, const void *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLLIGHTMODELFV(pname, params) \
	GLfloat tmp[4]; \
	int const size = nfglGetNumParams(pname); \
	Atari2HostFloatArray(size, params,tmp); \
	fn.glLightModelfv(pname, tmp)
#else
#define FN_GLLIGHTMODELFV(pname, params) \
	fn.glLightModelfv(pname, HostAddr(params, const GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLLIGHTMODELIV(pname, params) \
	int const size = nfglGetNumParams(pname); \
	GLint tmp[size], *ptmp; \
	ptmp = Atari2HostIntArray(size, params, tmp); \
	fn.glLightModeliv(pname, ptmp)
#else
#define FN_GLLIGHTMODELIV(pname, params) \
	fn.glLightModeliv(pname, HostAddr(params, const GLint *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLLIGHTFV(light, pname, params) \
	GLfloat tmp[4]; \
	int const size = nfglGetNumParams(pname); \
	Atari2HostFloatArray(size, params, tmp); \
	fn.glLightfv(light, pname, tmp)
#else
#define FN_GLLIGHTFV(light, pname, params) \
	fn.glLightfv(light, pname, HostAddr(params, const GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLLIGHTIV(light, pname, params) \
	int const size = nfglGetNumParams(pname); \
	GLint tmp[size], *ptmp; \
	ptmp = Atari2HostIntArray(size, params, tmp); \
	fn.glLightiv(light, pname, ptmp)
#else
#define FN_GLLIGHTIV(light, pname, params) \
	fn.glLightiv(light, pname, HostAddr(params, const GLint *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLLOADMATRIXD(m)	\
	int const size = 16; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, m, tmp); \
	fn.glLoadMatrixd(tmp)
#else
#define FN_GLLOADMATRIXD(m) \
	fn.glLoadMatrixd(HostAddr(m, const GLdouble *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLLOADMATRIXF(m)	\
	int const size = 16; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, m, tmp); \
	fn.glLoadMatrixf(tmp)
#else
#define FN_GLLOADMATRIXF(m) \
	fn.glLoadMatrixf(HostAddr(m, const GLfloat *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLMAP1D(target, u1, u2, stride, order, points) \
	GLdouble *tmp; \
	nfcmemptr ptr; \
	GLint i; \
	GLint const size = __glGetMap_Evalk(target); \
	 \
	tmp = (GLdouble *)malloc(size * order * sizeof(GLdouble)); \
	if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
	ptr = (nfcmemptr)points; \
	for (i = 0; i < order; i++) { \
		Atari2HostDoubleArray(size, AtariAddr(ptr, const GLdouble *), &tmp[i*size]); \
		ptr += stride * ATARI_SIZEOF_DOUBLE; \
	} \
	fn.glMap1d(target, u1, u2, size, order, tmp); \
	free(tmp)
#else
#define FN_GLMAP1D(target, u1, u2, stride, order, points) \
	fn.glMap1d(target, u1, u2, stride, order, HostAddr(points, const GLdouble *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLMAP1F(target, u1, u2, stride, order, points) \
	GLfloat *tmp; \
	nfcmemptr ptr; \
	GLint i; \
	GLint const size = __glGetMap_Evalk(target); \
	 \
	tmp = (GLfloat *)malloc(size * order * sizeof(GLfloat)); \
	if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
	ptr = (nfcmemptr)points; \
	for (i = 0; i < order; i++) { \
		Atari2HostFloatArray(size, AtariAddr(ptr, const GLfloat *), &tmp[i * size]); \
		ptr += stride * ATARI_SIZEOF_FLOAT; \
	} \
	fn.glMap1f(target, u1, u2, size, order, tmp); \
	free(tmp)
#else
#define FN_GLMAP1F(target, u1, u2, stride, order, points) \
	fn.glMap1f(target, u1, u2, stride, order, HostAddr(points, const GLfloat *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLMAP2D(target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points) \
	GLdouble *tmp; \
	nfcmemptr ptr; \
	GLint i, j; \
	GLint const size = __glGetMap_Evalk(target); \
	 \
	tmp = (GLdouble *)malloc(size * uorder * vorder * sizeof(GLdouble)); \
	if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
	ptr = (nfcmemptr)points; \
	for (i = 0; i < uorder; i++) { \
		for (j = 0;j < vorder; j++) { \
			Atari2HostDoubleArray(size, AtariAddr(ptr + (i * ustride + j * vstride) * ATARI_SIZEOF_DOUBLE, const GLdouble *), &tmp[(i * vorder + j) * size]); \
		} \
	} \
	fn.glMap2d(target, \
		u1, u2, size * vorder, uorder, \
		v1, v2, size, vorder, tmp \
	); \
	free(tmp)
#else
#define FN_GLMAP2D(target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points) \
	fn.glMap2d(target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, HostAddr(points, const GLdouble *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLMAP2F(target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points) \
	GLfloat *tmp; \
	nfcmemptr ptr; \
	GLint i, j; \
	GLint const size = __glGetMap_Evalk(target); \
	 \
	tmp = (GLfloat *)malloc(size * uorder * vorder *sizeof(GLfloat)); \
	if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
	ptr = (nfcmemptr)points; \
	for (i = 0; i < uorder; i++) { \
		for (j = 0; j < vorder; j++) { \
			Atari2HostFloatArray(size, AtariAddr(ptr + (i * ustride + j * vstride) * ATARI_SIZEOF_FLOAT, const GLfloat *), &tmp[(i * vorder + j) * size]); \
		} \
	} \
	fn.glMap2f(target, \
		u1, u2, size * vorder, uorder, \
		v1, v2, size, vorder, tmp \
	); \
	free(tmp)
#else
#define FN_GLMAP2F(target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points) \
	fn.glMap2f(target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, HostAddr(points, const GLfloat *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLMATERIALFV(face, pname, params) \
	int const size = nfglGetNumParams(pname); \
	GLfloat tmp[size], *ptmp; \
	ptmp = Atari2HostFloatArray(size, params,tmp); \
	fn.glMaterialfv(face, pname, ptmp)
#else
#define FN_GLMATERIALFV(face, pname, params) \
	fn.glMaterialfv(face, pname, HostAddr(params, const GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLMATERIALIV(face, pname, params) \
	int const size = nfglGetNumParams(pname); \
	GLint tmp[size], *ptmp; \
	ptmp = Atari2HostIntArray(size, params, tmp); \
	fn.glMaterialiv(face, pname, ptmp)
#else
#define FN_GLMATERIALIV(face, pname, params) \
	fn.glMaterialiv(face, pname, HostAddr(params, const GLint *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLMULTMATRIXD(m)	\
	int const size = 16; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, m, tmp); \
	fn.glMultMatrixd(tmp)
#else
#define FN_GLMULTMATRIXD(m) \
	fn.glMultMatrixd(HostAddr(m, const GLdouble *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLMULTMATRIXF(m) \
	int const size = 16; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, m, tmp); \
	fn.glMultMatrixf(tmp)
#else
#define FN_GLMULTMATRIXF(m) \
	fn.glMultMatrixf(HostAddr(m, const GLfloat *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLNORMAL3BV(v) \
	int const size = 3; \
	GLbyte tmp[size]; \
	Atari2HostByteArray(size, v, tmp); \
	fn.glNormal3bv(tmp)
#else
#define FN_GLNORMAL3BV(v) \
	fn.glNormal3bv(HostAddr(v, const GLbyte *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLNORMAL3DV(v) \
	int const size = 3; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, v, tmp); \
	fn.glNormal3dv(tmp)
#else
#define FN_GLNORMAL3DV(v) \
	fn.glNormal3dv(HostAddr(v, const GLdouble *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLNORMAL3FV(v) \
	int const size = 3; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, v, tmp); \
	fn.glNormal3fv(tmp)
#else
#define FN_GLNORMAL3FV(v) \
	fn.glNormal3fv(HostAddr(v, const GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLNORMAL3IV(v) \
	int const size = 3; \
	GLint tmp[size]; \
	Atari2HostIntArray(size, v, tmp); \
	fn.glNormal3iv(tmp)
#else
#define FN_GLNORMAL3IV(v) \
	fn.glNormal3iv(HostAddr(v, const GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLNORMAL3SV(v) \
	int const size = 3; \
	GLshort tmp[size]; \
	Atari2HostShortArray(size, v, tmp); \
	fn.glNormal3sv(tmp)
#else
#define FN_GLNORMAL3SV(v) \
	fn.glNormal3sv(HostAddr(v, const GLshort *))
#endif

#define FN_GLNORMALPOINTER(type, stride, pointer) \
	GLint texunit = 0; \
	fn.glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE, &texunit); \
	setupClientArray(texunit, contexts[cur_context].normal, 3, type, stride, -1, 0, AtariAddr(pointer, nfmemptr))

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPIXELMAPFV(map, mapsize, values) \
	GLfloat tmp[MAX(mapsize, 0)], *ptmp; \
	ptmp = Atari2HostFloatArray(mapsize, values, tmp); \
	fn.glPixelMapfv(map, mapsize, ptmp)
#else
#define FN_GLPIXELMAPFV(map, mapsize, values) \
	fn.glPixelMapfv(map, mapsize, HostAddr(values, const GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLPIXELMAPUIV(map, mapsize, values) \
	GLuint tmp[MAX(mapsize, 0)], *ptmp; \
	ptmp = Atari2HostIntArray(mapsize, values, tmp); \
	fn.glPixelMapuiv(map, mapsize, ptmp)
#else
#define FN_GLPIXELMAPUIV(map, mapsize, values) \
	fn.glPixelMapuiv(map, mapsize, HostAddr(values, const GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLPIXELMAPUSV(map, mapsize, values) \
	GLushort tmp[MAX(mapsize, 0)], *ptmp; \
	ptmp = Atari2HostShortArray(mapsize, values, tmp); \
	fn.glPixelMapusv(map, mapsize, ptmp)
#else
#define FN_GLPIXELMAPUSV(map, mapsize, values) \
	fn.glPixelMapusv(map, mapsize, HostAddr(values, const GLushort *))
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPRIORITIZETEXTURES(n, textures, priorities) \
	GLuint tmp[MAX(n, 0)], *ptmp; \
	GLclampf tmp2[MAX(n, 0)], *ptmp2; \
	ptmp = Atari2HostIntArray(n, textures, tmp); \
	ptmp2 = Atari2HostFloatArray(n, priorities, tmp2); \
	fn.glPrioritizeTextures(n, ptmp, ptmp2)
#else
#define FN_GLPRIORITIZETEXTURES(n, textures, priorities) \
	fn.glPrioritizeTextures(n, HostAddr(textures, const GLuint *), HostAddr(priorities, const GLclampf *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLRASTERPOS2DV(v) \
	GLdouble tmp[2]; \
	Atari2HostDoubleArray(2, v, tmp); \
	fn.glRasterPos2dv(tmp)
#else
#define FN_GLRASTERPOS2DV(v) \
	fn.glRasterPos2dv(HostAddr(v, const GLdouble *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLRASTERPOS2FV(v) \
	GLfloat tmp[2]; \
	Atari2HostFloatArray(2, v, tmp); \
	fn.glRasterPos2fv(tmp)
#else
#define FN_GLRASTERPOS2FV(v) \
	fn.glRasterPos2fv(HostAddr(v, const GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLRASTERPOS2IV(v) \
	GLint const size = 2; \
	GLint tmp[size]; \
	Atari2HostIntArray(size, v, tmp); \
	fn.glRasterPos2iv(tmp)
#else
#define FN_GLRASTERPOS2IV(v) \
	fn.glRasterPos2iv(HostAddr(v, const GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLRASTERPOS2SV(v) \
	GLshort tmp[2]; \
	Atari2HostShortArray(2, v, tmp); \
	fn.glRasterPos2sv(tmp)
#else
#define FN_GLRASTERPOS2SV(v) \
	fn.glRasterPos2sv(HostAddr(v, const GLshort *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLRASTERPOS3DV(v) \
	GLdouble tmp[3]; \
	Atari2HostDoubleArray(3, v, tmp); \
	fn.glRasterPos3dv(tmp)
#else
#define FN_GLRASTERPOS3DV(v) \
	fn.glRasterPos3dv(HostAddr(v, const GLdouble *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLRASTERPOS3FV(v) \
	GLfloat tmp[3]; \
	Atari2HostFloatArray(3, v, tmp); \
	fn.glRasterPos3fv(tmp)
#else
#define FN_GLRASTERPOS3FV(v) \
	fn.glRasterPos3fv(HostAddr(v, const GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLRASTERPOS3IV(v) \
	GLint tmp[3]; \
	Atari2HostIntArray(3, v, tmp); \
	fn.glRasterPos3iv(tmp)
#else
#define FN_GLRASTERPOS3IV(v) \
	fn.glRasterPos3iv(HostAddr(v, const GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLRASTERPOS3SV(v) \
	GLshort tmp[3]; \
	Atari2HostShortArray(3, v, tmp); \
	fn.glRasterPos3sv(tmp)
#else
#define FN_GLRASTERPOS3SV(v) \
	fn.glRasterPos3sv(HostAddr(v, const GLshort *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLRASTERPOS4DV(v) \
	GLdouble tmp[4]; \
	Atari2HostDoubleArray(4, v, tmp); \
	fn.glRasterPos4dv(tmp)
#else
#define FN_GLRASTERPOS4DV(v) \
	fn.glRasterPos4dv(HostAddr(v, const GLdouble *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLRASTERPOS4FV(v) \
	GLfloat tmp[4]; \
	Atari2HostFloatArray(4, v, tmp); \
	fn.glRasterPos4fv(tmp)
#else
#define FN_GLRASTERPOS4FV(v) \
	fn.glRasterPos4fv(HostAddr(v, const GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLRASTERPOS4IV(v) \
	GLint tmp[4]; \
	Atari2HostIntArray(4, v, tmp); \
	fn.glRasterPos4iv(tmp)
#else
#define FN_GLRASTERPOS4IV(v) \
	fn.glRasterPos4iv(HostAddr(v, const GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLRASTERPOS4SV(v) \
	GLshort tmp[4]; \
	Atari2HostShortArray(4, v, tmp); \
	fn.glRasterPos4sv(tmp)
#else
#define FN_GLRASTERPOS4SV(v) \
	fn.glRasterPos4sv(HostAddr(v, const GLshort *))
#endif

#define FN_GLREADPIXELS(x, y, width, height, format, type, img) \
	GLint const depth = 1; \
	char *src; \
	nfmemptr dst; \
	pixelBuffer buf(*this); \
	 \
	if (contexts[cur_context].buffer_bindings.pixel_pack.id) { \
		void *offset = NFHost2AtariAddr(img); \
		fn.glReadPixels(x, y, width, height, format, type, offset); \
		src = contexts[cur_context].buffer_bindings.pixel_pack.host_pointer + (uintptr_t)offset; \
		dst = contexts[cur_context].buffer_bindings.pixel_pack.atari_pointer + (uintptr_t)offset; \
		if (!buf.params(width, height, depth, format, type)) return; \
	} else { \
		src = buf.hostBuffer(width, height, depth, format, type, (nfmemptr)img); \
		if (src == NULL) return; \
		fn.glReadPixels(x, y, width, height, format, type, src); \
		dst = (nfmemptr)img; \
	} \
	buf.convertToAtari(src, dst)

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLRECTDV(v1, v2) \
	GLdouble tmp1[4]; \
	GLdouble tmp2[4]; \
	Atari2HostDoubleArray(4, v1, tmp1); \
	Atari2HostDoubleArray(4, v2, tmp2); \
	fn.glRectdv(tmp1, tmp2)
#else
#define FN_GLRECTDV(v1, v2) \
	fn.glRectdv(HostAddr(v1, const GLdouble *), HostAddr(v2, const GLdouble *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLRECTFV(v1, v2) \
	GLfloat tmp1[4]; \
	GLfloat tmp2[4]; \
	Atari2HostFloatArray(4, v1, tmp1); \
	Atari2HostFloatArray(4, v2, tmp2); \
	fn.glRectfv(tmp1, tmp2)
#else
#define FN_GLRECTFV(v1, v2) \
	fn.glRectfv(HostAddr(v1, const GLfloat *), HostAddr(v2, const GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLRECTIV(v1, v2) \
	GLint tmp1[4]; \
	GLint tmp2[4]; \
	Atari2HostIntArray(4, v1, tmp1); \
	Atari2HostIntArray(4, v2, tmp2); \
	fn.glRectiv(tmp1, tmp2)
#else
#define FN_GLRECTIV(v1, v2) \
	fn.glRectiv(HostAddr(v1, const GLint *), HostAddr(v2, const GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLRECTSV(v1, v2) \
	GLshort tmp1[4]; \
	GLshort tmp2[4]; \
	Atari2HostShortArray(4, v1, tmp1); \
	Atari2HostShortArray(4, v2, tmp2); \
	fn.glRectsv(tmp1, tmp2)
#else
#define FN_GLRECTSV(v1, v2) \
	fn.glRectsv(HostAddr(v1, const GLshort *), HostAddr(v2, const GLshort *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLSELECTBUFFER(size, buffer) \
	contexts[cur_context].select_buffer_atari = (nfmemptr)buffer; \
	free(contexts[cur_context].select_buffer_host); \
	contexts[cur_context].select_buffer_host = (GLuint *)calloc(size, sizeof(GLuint)); \
	if (!contexts[cur_context].select_buffer_host) { glSetError(GL_OUT_OF_MEMORY); return; } \
	contexts[cur_context].select_buffer_size = size; \
	fn.glSelectBuffer(size, contexts[cur_context].select_buffer_host)
#else
#define FN_GLSELECTBUFFER(size, buffer) \
	fn.glSelectBuffer(size, HostAddr(buffer, GLuint *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLTEXCOORD1DV(v) \
	GLdouble tmp[1]; \
	Atari2HostDoubleArray(1, v, tmp); \
	fn.glTexCoord1dv(tmp)
#else
#define FN_GLTEXCOORD1DV(v) \
	fn.glTexCoord1dv(HostAddr(v, const GLdouble *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLTEXCOORD1FV(v) \
	GLfloat tmp[1]; \
	Atari2HostFloatArray(1, v, tmp); \
	fn.glTexCoord1fv(tmp)
#else
#define FN_GLTEXCOORD1FV(v) \
	fn.glTexCoord1fv(HostAddr(v, const GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLTEXCOORD1IV(v) \
	GLint tmp[1]; \
	Atari2HostIntArray(1, v, tmp); \
	fn.glTexCoord1iv(tmp)
#else
#define FN_GLTEXCOORD1IV(v) \
	fn.glTexCoord1iv(HostAddr(v, const GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLTEXCOORD1SV(v) \
	GLshort tmp[1]; \
	Atari2HostShortArray(1, v, tmp); \
	fn.glTexCoord1sv(tmp)
#else
#define FN_GLTEXCOORD1SV(v) \
	fn.glTexCoord1sv(HostAddr(v, const GLshort *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLTEXCOORD2DV(v) \
	GLdouble tmp[2]; \
	Atari2HostDoubleArray(2, v, tmp); \
	fn.glTexCoord2dv(tmp)
#else
#define FN_GLTEXCOORD2DV(v) \
	fn.glTexCoord2dv(HostAddr(v, const GLdouble *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLTEXCOORD2FV(v) \
	GLfloat tmp[2]; \
	Atari2HostFloatArray(2, v, tmp); \
	fn.glTexCoord2fv(tmp)
#else
#define FN_GLTEXCOORD2FV(v) \
	fn.glTexCoord2fv(HostAddr(v, const GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLTEXCOORD2IV(v) \
	GLint tmp[2]; \
	Atari2HostIntArray(2, v, tmp); \
	fn.glTexCoord2iv(tmp)
#else
#define FN_GLTEXCOORD2IV(v) \
	fn.glTexCoord2iv(HostAddr(v, const GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLTEXCOORD2SV(v) \
	GLshort tmp[2]; \
	Atari2HostShortArray(2, v, tmp); \
	fn.glTexCoord2sv(tmp)
#else
#define FN_GLTEXCOORD2SV(v) \
	fn.glTexCoord2sv(HostAddr(v, const GLshort *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLTEXCOORD3DV(v) \
	GLdouble tmp[3]; \
	Atari2HostDoubleArray(3, v, tmp); \
	fn.glTexCoord3dv(tmp)
#else
#define FN_GLTEXCOORD3DV(v) \
	fn.glTexCoord3dv(HostAddr(v, const GLdouble *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLTEXCOORD3FV(v) \
	GLfloat tmp[3]; \
	Atari2HostFloatArray(3, v, tmp); \
	fn.glTexCoord3fv(tmp)
#else
#define FN_GLTEXCOORD3FV(v) \
	fn.glTexCoord3fv(HostAddr(v, const GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLTEXCOORD3IV(v) \
	GLint tmp[3]; \
	Atari2HostIntArray(3, v, tmp); \
	fn.glTexCoord3iv(tmp)
#else
#define FN_GLTEXCOORD3IV(v) \
	fn.glTexCoord3iv(HostAddr(v, const GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLTEXCOORD3SV(v) \
	GLshort tmp[3]; \
	Atari2HostShortArray(3, v, tmp); \
	fn.glTexCoord3sv(tmp)
#else
#define FN_GLTEXCOORD3SV(v) \
	fn.glTexCoord3sv(HostAddr(v, const GLshort *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLTEXCOORD4DV(v) \
	GLdouble tmp[4]; \
	Atari2HostDoubleArray(4, v, tmp); \
	fn.glTexCoord4dv(tmp)
#else
#define FN_GLTEXCOORD4DV(v) \
	fn.glTexCoord4dv(HostAddr(v, const GLdouble *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLTEXCOORD4FV(v) \
	GLfloat tmp[4]; \
	Atari2HostFloatArray(4, v, tmp); \
	fn.glTexCoord4fv(tmp)
#else
#define FN_GLTEXCOORD4FV(v) \
	fn.glTexCoord4fv(HostAddr(v, const GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLTEXCOORD4IV(v) \
	GLint tmp[4]; \
	Atari2HostIntArray(4, v, tmp); \
	fn.glTexCoord4iv(tmp)
#else
#define FN_GLTEXCOORD4IV(v) \
	fn.glTexCoord4iv(HostAddr(v, const GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLTEXCOORD4SV(v) \
	GLshort tmp[4]; \
	Atari2HostShortArray(4, v, tmp); \
	fn.glTexCoord4sv(tmp)
#else
#define FN_GLTEXCOORD4SV(v) \
	fn.glTexCoord4sv(HostAddr(v, const GLshort *))
#endif

#define FN_GLTEXCOORDPOINTER(size, type, stride, pointer) \
	GLint texunit = 0; \
	fn.glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE, &texunit); \
	setupClientArray(texunit, contexts[cur_context].texcoord, size, type, stride, -1, 0, AtariAddr(pointer, nfmemptr))

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLTEXENVFV(target, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLfloat tmp[MAX(size, 16)], *ptmp; \
	ptmp = Atari2HostFloatArray(size, params, tmp); \
	fn.glTexEnvfv(target, pname, ptmp)
#else
#define FN_GLTEXENVFV(target, pname, params) \
	fn.glTexEnvfv(target, pname, HostAddr(params, const GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLTEXENVIV(target, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLint tmp[MAX(size, 16)], *ptmp; \
	ptmp = Atari2HostIntArray(size, params, tmp); \
	fn.glTexEnviv(target, pname, ptmp)
#else
#define FN_GLTEXENVIV(target, pname, params) \
	fn.glTexEnviv(target, pname, HostAddr(params, const GLint *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLTEXGENDV(coord, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLdouble tmp[MAX(size, 16)]; \
	Atari2HostDoubleArray(size, params, tmp); \
	fn.glTexGendv(coord, pname, tmp)
#else
#define FN_GLTEXGENDV(coord, pname, params) \
	fn.glTexGendv(coord, pname, HostAddr(params, const GLdouble *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLTEXGENFV(coord, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLfloat tmp[MAX(size, 16)]; \
	Atari2HostFloatArray(size, params, tmp); \
	fn.glTexGenfv(coord, pname, tmp)
#else
#define FN_GLTEXGENFV(coord, pname, params) \
	fn.glTexGenfv(coord, pname, HostAddr(params, const GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLTEXGENIV(coord, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLint tmp[MAX(size, 16)]; \
	Atari2HostIntArray(size, params, tmp); \
	fn.glTexGeniv(coord, pname, tmp)
#else
#define FN_GLTEXGENIV(coord, pname, params)	\
	fn.glTexGeniv(coord, pname, HostAddr(params, const GLint *))
#endif

#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLTEXIMAGE1D(target, level, internalformat, width, border, format, type, pixels) \
	pixelBuffer buf(*this); \
	void *tmp = buf.convertPixels(width, 1, 1, format, type, (nfmemptr)pixels); \
	fn.glTexImage1D(target, level, internalformat, width, border, format, type, tmp)
#else
#define FN_GLTEXIMAGE1D(target, level, internalformat, width, border, format, type, pixels) \
	fn.glTexImage1D(target, level, internalformat, width, border, format, type, HostAddr(pixels, const void *))
#endif

#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLTEXIMAGE2D(target, level, internalformat, width, height, border, format, type, pixels) \
	pixelBuffer buf(*this); \
	void *tmp = buf.convertPixels(width, height, 1, format, type, (nfmemptr)pixels); \
	fn.glTexImage2D(target, level, internalformat, width, height, border, format, type, tmp)
#else
#define FN_GLTEXIMAGE2D(target, level, internalformat, width, height, border, format, type, pixels) \
	fn.glTexImage2D(target, level, internalformat, width, height, border, format, type, HostAddr(pixels, const void *))
#endif

#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLTEXSUBIMAGE1D(target, level, xoffset, width, format, type, pixels) \
	pixelBuffer buf(*this); \
	void *tmp = buf.convertPixels(width, 1, 1, format, type, (nfmemptr)pixels); \
	fn.glTexSubImage1D(target, level, xoffset, width, format, type, tmp)
#else
#define FN_GLTEXSUBIMAGE1D(target, level, xoffset, width, format, type, pixels) \
	fn.glTexSubImage1D(target, level, xoffset, width, format, type, HostAddr(pixels, const void *))
#endif

#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLTEXSUBIMAGE2D(target, level, xoffset, yoffset, width, height, format, type, pixels) \
	pixelBuffer buf(*this); \
	void *tmp = buf.convertPixels(width, height, 1, format, type, (nfmemptr)pixels); \
	fn.glTexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, tmp)
#else
#define FN_GLTEXSUBIMAGE2D(target, level, xoffset, yoffset, width, height, format, type, pixels) \
	fn.glTexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, HostAddr(pixels, const void *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLTEXPARAMETERFV(target, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLfloat tmp[MAX(size, 16)], *ptmp; \
	ptmp = Atari2HostFloatArray(size, params, tmp); \
	fn.glTexParameterfv(target, pname, ptmp)
#else
#define FN_GLTEXPARAMETERFV(target, pname, params) \
	fn.glTexParameterfv(target, pname, HostAddr(params, const GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLTEXPARAMETERIV(target, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLint tmp[MAX(size, 16)], *ptmp; \
	ptmp = Atari2HostIntArray(size, params, tmp); \
	fn.glTexParameteriv(target, pname, ptmp)
#else
#define FN_GLTEXPARAMETERIV(target, pname, params) \
	fn.glTexParameteriv(target, pname, HostAddr(params, const GLint *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLVERTEX2DV(v) \
	GLint const size = 2; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, v, tmp); \
	fn.glVertex2dv(tmp)
#else
#define FN_GLVERTEX2DV(v) \
	fn.glVertex2dv(HostAddr(v, const GLdouble *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLVERTEX2FV(v) \
	GLint const size = 2; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, v, tmp); \
	fn.glVertex2fv(tmp)
#else
#define FN_GLVERTEX2FV(v) \
	fn.glVertex2fv(HostAddr(v, const GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEX2IV(v) \
	GLint const size = 2; \
	GLint tmp[size]; \
	Atari2HostIntArray(size, v, tmp); \
	fn.glVertex2iv(tmp)
#else
#define FN_GLVERTEX2IV(v) \
	fn.glVertex2iv(HostAddr(v, const GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEX2SV(v) \
	GLint const size = 2; \
	GLshort tmp[size]; \
	Atari2HostShortArray(size, v, tmp); \
	fn.glVertex2sv(tmp)
#else
#define FN_GLVERTEX2SV(v) \
	fn.glVertex2sv(HostAddr(v, const GLshort *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLVERTEX3DV(v) \
	GLint const size = 3; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, v, tmp); \
	fn.glVertex3dv(tmp)
#else
#define FN_GLVERTEX3DV(v) \
	fn.glVertex3dv(HostAddr(v, const GLdouble *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLVERTEX3FV(v) \
	GLint const size = 3; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(3, v, tmp); \
	fn.glVertex3fv(tmp)
#else
#define FN_GLVERTEX3FV(v) \
	fn.glVertex3fv(HostAddr(v, const GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEX3IV(v) \
	GLint const size = 3; \
	GLint tmp[size]; \
	Atari2HostIntArray(size, v, tmp); \
	fn.glVertex3iv(tmp)
#else
#define FN_GLVERTEX3IV(v) \
	fn.glVertex3iv(HostAddr(v, const GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEX3SV(v) \
	GLint const size = 3; \
	GLshort tmp[size]; \
	Atari2HostShortArray(size, v, tmp); \
	fn.glVertex3sv(tmp)
#else
#define FN_GLVERTEX3SV(v) \
	fn.glVertex3sv(HostAddr(v, const GLshort *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLVERTEX4DV(v) \
	GLint const size = 4; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, v, tmp); \
	fn.glVertex4dv(tmp)
#else
#define FN_GLVERTEX4DV(v) \
	fn.glVertex4dv(HostAddr(v, const GLdouble *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLVERTEX4FV(v) \
	GLint const size = 4; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, v, tmp); \
	fn.glVertex4fv(tmp)
#else
#define FN_GLVERTEX4FV(v) \
	fn.glVertex4fv(HostAddr(v, const GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEX4IV(v) \
	GLint const size = 4; \
	GLint tmp[size]; \
	Atari2HostIntArray(size, v, tmp); \
	fn.glVertex4iv(tmp)
#else
#define FN_GLVERTEX4IV(v) \
	fn.glVertex4iv(HostAddr(v, const GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEX4SV(v) \
	GLint const size = 4; \
	GLshort tmp[size]; \
	Atari2HostShortArray(size, v, tmp); \
	fn.glVertex4sv(tmp)
#else
#define FN_GLVERTEX4SV(v) \
	fn.glVertex4sv(HostAddr(v, const GLshort *))
#endif

#define FN_GLVERTEXPOINTER(size, type, stride, pointer) \
	GLint texunit = 0; \
	fn.glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE, &texunit); \
	setupClientArray(texunit, contexts[cur_context].vertex, size, type, stride, -1, 0, AtariAddr(pointer, nfmemptr))

/* -------------------------------------------------------------------------- */

/*
 * Version 1.2
 */

#define FN_GLDRAWRANGEELEMENTS(mode, start, end, count, type, indices) \
	void *tmp; \
	convertClientArrays(count); \
	pixelBuffer buf(*this); \
	switch(type) { \
	case GL_UNSIGNED_BYTE: \
	case GL_UNSIGNED_SHORT: \
	case GL_UNSIGNED_INT: \
		tmp = buf.convertArray(count, type, (nfmemptr)indices); \
		break; \
	default: \
		glSetError(GL_INVALID_ENUM); \
		return; \
	} \
	fn.glDrawRangeElements(mode, start, end, count, type, tmp)

#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLTEXIMAGE3D(target, level, internalformat, width, height, depth, border, format, type, pixels) \
	pixelBuffer buf(*this); \
	void *tmp = buf.convertPixels(width, height, depth, format, type, (nfmemptr)pixels); \
	fn.glTexImage3D(target, level, internalformat, width, height, depth, border, format, type, tmp)
#else
#define FN_GLTEXIMAGE3D(target, level, internalformat, width, height, depth, border, format, type, pixels) \
	fn.glTexImage3D(target, level, internalformat, width, height, depth, border, format, type, HostAddr(pixels, const void *))
#endif

#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLTEXSUBIMAGE3D(target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels) \
	pixelBuffer buf(*this); \
	void *tmp = buf.convertPixels(width, height, depth, format, type, (nfmemptr)pixels); \
	fn.glTexSubImage3D(target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, tmp)
#else
#define FN_GLTEXSUBIMAGE3D(target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels) \
	fn.glTexSubImage3D(target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, HostAddr(pixels, const void *))
#endif

#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV || NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLCOLORTABLE(target, internalformat, width, format, type, table) \
	pixelBuffer buf(*this); \
	void *tmp = buf.convertPixels(width, 1, 1, format, type, (nfmemptr)table); \
	fn.glColorTable(target, internalformat, width, format, type, tmp)
#else
#define FN_GLCOLORTABLE(target, internalformat, width, format, type, table) \
	fn.glColorTable(target, internalformat, width, format, type, HostAddr(table, const void *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLCOLORTABLEPARAMETERFV(target, pname, params) \
	GLint const size = 4; \
	GLfloat tmp[size], *ptmp; \
	ptmp = Atari2HostFloatArray(size, params, tmp); \
	fn.glColorTableParameterfv(target, pname, ptmp)
#else
#define FN_GLCOLORTABLEPARAMETERFV(target, pname, params) \
	fn.glColorTableParameterfv(target, pname, HostAddr(params, const GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLCOLORTABLEPARAMETERIV(target, pname, params) \
	GLint const size = 4; \
	GLint tmp[size], *ptmp; \
	ptmp = Atari2HostIntArray(size, params, tmp); \
	fn.glColorTableParameteriv(target, pname, ptmp)
#else
#define FN_GLCOLORTABLEPARAMETERIV(target, pname, params) \
	fn.glColorTableParameteriv(target, pname, HostAddr(params, const GLint *))
#endif

/*
 * If a non-zero named buffer object is bound to the GL_PIXEL_PACK_BUFFER
 * target (see glBindBuffer) while a histogram table is requested, table
 * is treated as a byte offset into the buffer object's data store.
 */
#define FN_GLGETCOLORTABLE(target, format, type, table) \
	GLint width = 0; \
	char *src; \
	nfmemptr dst; \
	pixelBuffer buf(*this); \
	 \
	fn.glGetColorTableParameteriv(target, GL_COLOR_TABLE_WIDTH, &width); \
	if (contexts[cur_context].buffer_bindings.pixel_pack.id) { \
		void *offset = NFHost2AtariAddr(table); \
		fn.glGetColorTable(target, format, type, offset); \
		src = contexts[cur_context].buffer_bindings.pixel_pack.host_pointer + (uintptr_t)offset; \
		dst = contexts[cur_context].buffer_bindings.pixel_pack.atari_pointer + (uintptr_t)offset; \
		if (!buf.params(width, 1, 1, format, type)) return; \
	} else { \
		src = buf.hostBuffer(width, 1, 1, format, type, (nfmemptr)table); \
		if (src == NULL) return; \
		fn.glGetColorTable(target, format, type, src); \
		dst = (nfmemptr)table; \
	} \
	buf.convertToAtari(src, dst)

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETCOLORTABLEPARAMETERFV(target, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLfloat tmp[size]; \
	fn.glGetColorTableParameterfv(target, pname, tmp); \
	Host2AtariFloatArray(size, tmp, params)
#else
#define FN_GLGETCOLORTABLEPARAMETERFV(target, pname, params) \
	fn.glGetColorTableParameterfv(target, pname, HostAddr(params, GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETCOLORTABLEPARAMETERIV(target, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLint tmp[size]; \
	fn.glGetColorTableParameteriv(target, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETCOLORTABLEPARAMETERIV(target, pname, params) \
	fn.glGetColorTableParameteriv(target, pname, HostAddr(params, GLint *))
#endif

#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV || NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLCOLORSUBTABLE(target, start, count, format, type, data) \
	pixelBuffer buf(*this); \
	void *tmp = buf.convertPixels(count, 1, 1, format, type, (nfmemptr)data); \
	fn.glColorSubTable(target, start, count, format, type, tmp)
#else
#define FN_GLCOLORSUBTABLE(target, start, count, format, type, data) \
	fn.glColorSubTable(target, start, count, format, type, HostAddr(data, const void *))
#endif

#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLCONVOLUTIONFILTER1D(target, internalformat, width, format, type, image) \
	pixelBuffer buf(*this); \
	void *tmp = buf.convertPixels(width, 1, 1, format, type, (nfmemptr)image); \
	fn.glConvolutionFilter1D(target, internalformat, width, format, type, tmp)
#else
#define FN_GLCONVOLUTIONFILTER1D(target, internalformat, width, format, type, image) \
	fn.glConvolutionFilter1D(target, internalformat, width, format, type, HostAddr(image, const void *))
#endif

#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLCONVOLUTIONFILTER2D(target, internalformat, width, height, format, type, image) \
	pixelBuffer buf(*this); \
	void *tmp = buf.convertPixels(width, height, 1, format, type, (nfmemptr)image); \
	fn.glConvolutionFilter2D(target, internalformat, width, height, format, type, tmp)
#else
#define FN_GLCONVOLUTIONFILTER2D(target, internalformat, width, height, format, type, image) \
	fn.glConvolutionFilter2D(target, internalformat, width, height, format, type, HostAddr(image, const void *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLCONVOLUTIONPARAMETERFV(target, pname, params) \
	int const size = nfglGetNumParams(pname); \
	GLfloat tmp[MAX(size, 16)], *ptmp; \
	ptmp = Atari2HostFloatArray(size, params, tmp); \
	fn.glConvolutionParameterfv(target, pname, ptmp)
#else
#define FN_GLCONVOLUTIONPARAMETERFV(target, pname, params) \
	fn.glConvolutionParameterfv(target, pname, HostAddr(params, const GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLCONVOLUTIONPARAMETERIV(target, pname, params) \
	int const size = nfglGetNumParams(pname); \
	GLint tmp[size], *ptmp; \
	ptmp = Atari2HostIntArray(size, params, tmp); \
	fn.glConvolutionParameteriv(target, pname, ptmp)
#else
#define FN_GLCONVOLUTIONPARAMETERIV(target, pname, params) \
	fn.glConvolutionParameteriv(target, pname, HostAddr(params, GLint *))
#endif

/*
 * If a non-zero named buffer object is bound to the GL_PIXEL_PACK_BUFFER
 * target (see glBindBuffer) while a convolution filter is requested,
 * image is treated as a byte offset into the buffer object's data store.
 */
#define FN_GLGETCONVOLUTIONFILTER(target, format, type, image) \
	GLint width = 0; \
	GLint height = 0; \
	char *src; \
	nfmemptr dst; \
	pixelBuffer buf(*this); \
	 \
	fn.glGetConvolutionParameteriv(target, GL_CONVOLUTION_WIDTH, &width); \
	fn.glGetConvolutionParameteriv(target, GL_CONVOLUTION_HEIGHT, &height); \
	if (contexts[cur_context].buffer_bindings.pixel_pack.id) { \
		void *offset = NFHost2AtariAddr(image); \
		fn.glGetConvolutionFilter(target, format, type, offset); \
		src = contexts[cur_context].buffer_bindings.pixel_pack.host_pointer + (uintptr_t)offset; \
		dst = contexts[cur_context].buffer_bindings.pixel_pack.atari_pointer + (uintptr_t)offset; \
		if (!buf.params(width, height, 1, format, type)) return; \
	} else { \
		src = buf.hostBuffer(width, height, 1, format, type, (nfmemptr)image); \
		if (src == NULL) return; \
		fn.glGetConvolutionFilter(target, format, type, src); \
		dst = (nfmemptr)image; \
	} \
	buf.convertToAtari(src, dst)

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETCONVOLUTIONPARAMETERFV(target, pname, params) \
	int const size = nfglGetNumParams(pname); \
	GLfloat tmp[MAX(size, 16)]; \
	fn.glGetConvolutionParameterfv(target, pname, tmp); \
	Host2AtariFloatArray(size, tmp, params)
#else
#define FN_GLGETCONVOLUTIONPARAMETERFV(target, pname, params) \
	fn.glGetConvolutionParameterfv(target, pname, HostAddr(params, GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETCONVOLUTIONPARAMETERIV(target, pname, params) \
	int const size = nfglGetNumParams(pname); \
	GLint tmp[MAX(size, 16)]; \
	fn.glGetConvolutionParameteriv(target, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETCONVOLUTIONPARAMETERIV(target, pname, params) \
	fn.glGetConvolutionParameteriv(target, pname, HostAddr(params, GLint *))
#endif

#define FN_GLGETSEPARABLEFILTER(target, format, type, row, column, span) \
	GLint width = 0; \
	GLint height = 0; \
	pixelBuffer rowbuf(*this); \
	pixelBuffer colbuf(*this); \
	char *srcrow; \
	nfmemptr dstrow; \
	char *srccol; \
	nfmemptr dstcol; \
	 \
	fn.glGetConvolutionParameteriv(target, GL_CONVOLUTION_WIDTH, &width); \
	fn.glGetConvolutionParameteriv(target, GL_CONVOLUTION_HEIGHT, &height); \
	if (contexts[cur_context].buffer_bindings.pixel_pack.id) { \
		void *rowoffset = NFHost2AtariAddr(row); \
		void *coloffset = NFHost2AtariAddr(column); \
		fn.glGetSeparableFilter(target, format, type, rowoffset, coloffset, HostAddr(span, void *)); \
		srcrow = contexts[cur_context].buffer_bindings.pixel_pack.host_pointer + (uintptr_t)rowoffset; \
		srccol = contexts[cur_context].buffer_bindings.pixel_pack.host_pointer + (uintptr_t)coloffset; \
		dstrow = contexts[cur_context].buffer_bindings.pixel_pack.atari_pointer + (uintptr_t)rowoffset; \
		dstcol = contexts[cur_context].buffer_bindings.pixel_pack.atari_pointer + (uintptr_t)coloffset; \
		if (!rowbuf.params(width, 1, 1, format, type)) return; \
		if (!colbuf.params(1, height, 1, format, type)) return; \
	} else { \
		srcrow = rowbuf.hostBuffer(width, 1, 1, format, type, (nfmemptr)row); \
		srccol = colbuf.hostBuffer(1, height, 1, format, type, (nfmemptr)column); \
		if (srcrow == NULL || srccol == NULL) return; \
		fn.glGetSeparableFilter(target, format, type, srcrow, srccol, HostAddr(span, void *)); \
		dstrow = (nfmemptr)row; \
		dstcol = (nfmemptr)column; \
	} \
	rowbuf.convertToAtari(srcrow, dstrow); \
	colbuf.convertToAtari(srccol, dstcol)

#define FN_GLSEPARABLEFILTER2D(target, internalformat, width, height, format, type, row, column) \
	pixelBuffer rowbuf(*this); \
	pixelBuffer colbuf(*this); \
	void *tmprowbuf; \
	void *tmpcolbuf; \
	 \
	if (contexts[cur_context].buffer_bindings.pixel_unpack.id) { \
		void *rowoffset = NFHost2AtariAddr(row); \
		void *coloffset = NFHost2AtariAddr(column); \
		tmprowbuf = rowbuf.convertPixels(width, 1, 1, format, type, contexts[cur_context].buffer_bindings.pixel_pack.atari_pointer + (uintptr_t)rowoffset); \
		tmpcolbuf = colbuf.convertPixels(1, height, 1, format, type, contexts[cur_context].buffer_bindings.pixel_pack.atari_pointer + (uintptr_t)coloffset); \
	} else { \
		tmprowbuf = rowbuf.convertPixels(width, 1, 1, format, type, (nfmemptr)row); \
		tmpcolbuf = colbuf.convertPixels(1, height, 1, format, type, (nfmemptr)column); \
	} \
	fn.glSeparableFilter2D(target, internalformat, width, height, format, type, tmprowbuf, tmpcolbuf)

/*
 * If a non-zero named buffer object is bound to the GL_PIXEL_PACK_BUFFER
 * target (see glBindBuffer) while a histogram table is requested, table
 * is treated as a byte offset into the buffer object's data store.
 */
#define FN_GLGETHISTOGRAM(target, reset, format, type, values) \
	GLint width = 0; \
	char *src; \
	nfmemptr dst; \
	pixelBuffer buf(*this); \
	 \
	fn.glGetHistogramParameteriv(target, GL_HISTOGRAM_WIDTH, &width); \
	if (contexts[cur_context].buffer_bindings.pixel_pack.id) { \
		void *offset = NFHost2AtariAddr(values); \
		fn.glGetHistogram(target, reset, format, type, offset); \
		src = contexts[cur_context].buffer_bindings.pixel_pack.host_pointer + (uintptr_t)offset; \
		dst = contexts[cur_context].buffer_bindings.pixel_pack.atari_pointer + (uintptr_t)offset; \
		if (!buf.params(width, 1, 1, format, type)) return; \
	} else { \
		src = buf.hostBuffer(width, 1, 1, format, type, (nfmemptr)values); \
		if (src == NULL) return; \
		fn.glGetHistogram(target, reset, format, type, src); \
		dst = (nfmemptr)values; \
	} \
	buf.convertToAtari(src, dst)

/*
 * If a non-zero named buffer object is bound to the GL_PIXEL_PACK_BUFFER
 * target (see glBindBuffer) while minimum and maximum pixel values are
 * requested, values is treated as a byte offset into the buffer object's
 * data store.     
 */
#define FN_GLGETMINMAX(target, reset, format, type, values) \
	GLint const width = 2; \
	GLint const height = 4; \
	char result[width * height * sizeof(GLdouble)]; \
	char *src; \
	nfmemptr dst; \
	pixelBuffer buf(*this); \
	 \
	if (!buf.params(width, height, 1, format, type)) \
		return; \
	if (contexts[cur_context].buffer_bindings.pixel_pack.id) { \
		void *offset = NFHost2AtariAddr(values); \
		fn.glGetMinmax(target, reset, format, type, offset); \
		src = contexts[cur_context].buffer_bindings.pixel_pack.host_pointer + (uintptr_t)offset; \
		dst = contexts[cur_context].buffer_bindings.pixel_pack.atari_pointer + (uintptr_t)offset; \
	} else { \
		src = result; \
		fn.glGetMinmax(target, reset, format, type, src); \
		dst = (nfmemptr)values; \
	} \
	buf.convertToAtari(src, dst)

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETMINMAXPARAMETERFV(target, pname, params) \
	GLint const size = 1; \
	GLfloat tmp[MAX(size, 16)]; \
	fn.glGetMinmaxParameterfv(target, pname, tmp); \
	Host2AtariFloatArray(size, tmp, params)
#else
#define FN_GLGETMINMAXPARAMETERFV(target, pname, params) \
	fn.glGetMinmaxParameterfv(target, pname, HostAddr(params, GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETMINMAXPARAMETERIV(target, pname, params) \
	GLint const size = 1; \
	GLint tmp[MAX(size, 16)]; \
	fn.glGetMinmaxParameteriv(target, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETMINMAXPARAMETERIV(target, pname, params) \
	fn.glGetMinmaxParameteriv(target, pname, HostAddr(params, GLint *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * Version 1.3
 */

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLCOMPRESSEDTEXIMAGE1D(target, level, internalformat, width, border, imageSize, data) \
	GLubyte tmp[MAX(imageSize, 0)], *ptmp; \
	ptmp = Atari2HostByteArray(sizeof(tmp), data, tmp); \
	fn.glCompressedTexImage1D(target, level, internalformat, width, border, imageSize, ptmp)
#else
#define FN_GLCOMPRESSEDTEXIMAGE1D(target, level, internalformat, width, border, imageSize, data) \
	fn.glCompressedTexImage1D(target, level, internalformat, width, border, imageSize, HostAddr(data, const void *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLCOMPRESSEDTEXIMAGE2D(target, level, internalformat, width, height, border, imageSize, data) \
	GLubyte tmp[MAX(imageSize, 0)], *ptmp; \
	ptmp = Atari2HostByteArray(sizeof(tmp), data, tmp); \
	fn.glCompressedTexImage2D(target, level, internalformat, width, height, border, imageSize, ptmp)
#else
#define FN_GLCOMPRESSEDTEXIMAGE2D(target, level, internalformat, width, height, border, imageSize, data) \
	fn.glCompressedTexImage2D(target, level, internalformat, width, height, border, imageSize, HostAddr(data, const void *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLCOMPRESSEDTEXIMAGE3D(target, level, internalformat, width, height, depth, border, imageSize, data) \
	GLubyte tmp[MAX(imageSize, 0)], *ptmp; \
	ptmp = Atari2HostByteArray(sizeof(tmp), data, tmp); \
	fn.glCompressedTexImage3D(target, level, internalformat, width, height, depth, border, imageSize, ptmp)
#else
#define FN_GLCOMPRESSEDTEXIMAGE3D(target, level, internalformat, width, height, depth, border, imageSize, data) \
	fn.glCompressedTexImage3D(target, level, internalformat, width, height, depth, border, imageSize, HostAddr(data, const void *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLCOMPRESSEDTEXSUBIMAGE1D(target, level, xoffset, width, format, imageSize, data) \
	GLubyte tmp[MAX(imageSize, 0)], *ptmp; \
	ptmp = Atari2HostByteArray(sizeof(tmp), data, tmp); \
	fn.glCompressedTexSubImage1D(target, level, xoffset, width, format, imageSize, ptmp)
#else
#define FN_GLCOMPRESSEDTEXSUBIMAGE1D(target, level, xoffset, width, format, imageSize, data) \
	fn.glCompressedTexSubImage1D(target, level, xoffset, width, format, imageSize, HostAddr(data, const void *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLCOMPRESSEDTEXSUBIMAGE2D(target, level, xoffset, yoffset, width, height, format, imageSize, data) \
	GLubyte tmp[MAX(imageSize, 0)], *ptmp; \
	ptmp = Atari2HostByteArray(sizeof(tmp), data, tmp); \
	fn.glCompressedTexSubImage2D(target, level, xoffset, yoffset, width, height, format, imageSize, ptmp)
#else
#define FN_GLCOMPRESSEDTEXSUBIMAGE2D(target, level, xoffset, yoffset, width, height, format, imageSize, data) \
	fn.glCompressedTexSubImage2D(target, level, xoffset, yoffset, width, height, format, imageSize, HostAddr(data, const void *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLCOMPRESSEDTEXSUBIMAGE3D(target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data) \
	GLubyte tmp[MAX(imageSize, 0)], *ptmp; \
	ptmp = Atari2HostByteArray(sizeof(tmp), data, tmp); \
	fn.glCompressedTexSubImage3D(target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, ptmp)
#else
#define FN_GLCOMPRESSEDTEXSUBIMAGE3D(target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data) \
	fn.glCompressedTexSubImage3D(target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, HostAddr(data, const void *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLGETCOMPRESSEDTEXIMAGE(target, level, img) \
	GLint bufSize = 0; \
	fn.glGetTexLevelParameteriv(target, level, GL_TEXTURE_COMPRESSED_IMAGE_SIZE, &bufSize); \
	GLubyte tmp[bufSize]; \
	fn.glGetCompressedTexImage(target, level, tmp); \
	Host2AtariByteArray(bufSize, tmp, img)
#else
#define FN_GLGETCOMPRESSEDTEXIMAGE(target, level, img) \
	fn.glGetCompressedTexImage(target, level, HostAddr(img, void *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLMULTITEXCOORD1DV(target, v) \
	GLdouble tmp[1]; \
	Atari2HostDoubleArray(1, v, tmp); \
	fn.glMultiTexCoord1dv(target, tmp)
#else
#define FN_GLMULTITEXCOORD1DV(target, v) \
	fn.glMultiTexCoord1dv(target, HostAddr(v, const GLdouble *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLMULTITEXCOORD1FV(target, v) \
	GLfloat tmp[1]; \
	Atari2HostFloatArray(1, v, tmp); \
	fn.glMultiTexCoord1fv(target, tmp)
#else
#define FN_GLMULTITEXCOORD1FV(target, v) \
	fn.glMultiTexCoord1fv(target, HostAddr(v, const GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLMULTITEXCOORD1IV(target, v) \
	GLint tmp[1]; \
	Atari2HostIntArray(1, v, tmp); \
	fn.glMultiTexCoord1iv(target, tmp)
#else
#define FN_GLMULTITEXCOORD1IV(target, v) \
	fn.glMultiTexCoord1iv(target, HostAddr(v, const GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLMULTITEXCOORD1SV(target, v) \
	GLshort tmp[1]; \
	Atari2HostShortArray(1, v, tmp); \
	fn.glMultiTexCoord1sv(target, tmp)
#else
#define FN_GLMULTITEXCOORD1SV(target, v) \
	fn.glMultiTexCoord1sv(target, HostAddr(v, const GLshort *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLMULTITEXCOORD2DV(target, v) \
	GLdouble tmp[2]; \
	Atari2HostDoubleArray(2, v, tmp); \
	fn.glMultiTexCoord2dv(target, tmp)
#else
#define FN_GLMULTITEXCOORD2DV(target, v) \
	fn.glMultiTexCoord2dv(target, HostAddr(v, const GLdouble *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLMULTITEXCOORD2FV(target, v) \
	GLfloat tmp[2]; \
	Atari2HostFloatArray(2, v, tmp); \
	fn.glMultiTexCoord2fv(target, tmp)
#else
#define FN_GLMULTITEXCOORD2FV(target, v) \
	fn.glMultiTexCoord2fv(target, HostAddr(v, const GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLMULTITEXCOORD2IV(target, v) \
	GLint tmp[2]; \
	Atari2HostIntArray(2, v, tmp); \
	fn.glMultiTexCoord2iv(target, tmp)
#else
#define FN_GLMULTITEXCOORD2IV(target, v) \
	fn.glMultiTexCoord2iv(target, HostAddr(v, const GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLMULTITEXCOORD2SV(target, v) \
	GLshort tmp[2]; \
	Atari2HostShortArray(2, v, tmp); \
	fn.glMultiTexCoord2sv(target, tmp)
#else
#define FN_GLMULTITEXCOORD2SV(target, v) \
	fn.glMultiTexCoord2sv(target, HostAddr(v, const GLshort *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLMULTITEXCOORD3DV(target, v) \
	GLdouble tmp[3]; \
	Atari2HostDoubleArray(3, v, tmp); \
	fn.glMultiTexCoord3dv(target, tmp)
#else
#define FN_GLMULTITEXCOORD3DV(target, v) \
	fn.glMultiTexCoord3dv(target, HostAddr(v, const GLdouble *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLMULTITEXCOORD3FV(target, v) \
	GLfloat tmp[3]; \
	Atari2HostFloatArray(3, v, tmp); \
	fn.glMultiTexCoord3fv(target, tmp)
#else
#define FN_GLMULTITEXCOORD3FV(target, v) \
	fn.glMultiTexCoord3fv(target, HostAddr(v, const GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLMULTITEXCOORD3IV(target, v) \
	GLint tmp[3]; \
	Atari2HostIntArray(3, v, tmp); \
	fn.glMultiTexCoord3iv(target, tmp)
#else
#define FN_GLMULTITEXCOORD3IV(target, v) \
	fn.glMultiTexCoord3iv(target, HostAddr(v, const GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLMULTITEXCOORD3SV(target, v) \
	GLshort tmp[3]; \
	Atari2HostShortArray(3, v, tmp); \
	fn.glMultiTexCoord3sv(target, tmp)
#else
#define FN_GLMULTITEXCOORD3SV(target, v) \
	fn.glMultiTexCoord3sv(target, HostAddr(v, const GLshort *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLMULTITEXCOORD4DV(target, v) \
	GLdouble tmp[4]; \
	Atari2HostDoubleArray(4, v, tmp); \
	fn.glMultiTexCoord4dv(target, tmp)
#else
#define FN_GLMULTITEXCOORD4DV(target, v) \
	fn.glMultiTexCoord4dv(target, HostAddr(v, const GLdouble *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLMULTITEXCOORD4FV(target, v) \
	GLfloat tmp[4]; \
	Atari2HostFloatArray(4, v, tmp); \
	fn.glMultiTexCoord4fv(target, tmp)
#else
#define FN_GLMULTITEXCOORD4FV(target, v) \
	fn.glMultiTexCoord4fv(target, HostAddr(v, const GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLMULTITEXCOORD4IV(target, v) \
	GLint tmp[4]; \
	Atari2HostIntArray(4, v, tmp); \
	fn.glMultiTexCoord4iv(target, tmp)
#else
#define FN_GLMULTITEXCOORD4IV(target, v) \
	fn.glMultiTexCoord4iv(target, HostAddr(v, const GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLMULTITEXCOORD4SV(target, v) \
	int const size = 4; \
	GLshort tmp[size]; \
	Atari2HostShortArray(size, v, tmp); \
	fn.glMultiTexCoord4sv(target, tmp)
#else
#define FN_GLMULTITEXCOORD4SV(target, v) \
	fn.glMultiTexCoord4sv(target, HostAddr(v, const GLshort *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLLOADTRANSPOSEMATRIXF(m) \
	GLfloat tmp[16]; \
	Atari2HostFloatArray(16, m, tmp); \
	fn.glLoadTransposeMatrixf(tmp)
#else
#define FN_GLLOADTRANSPOSEMATRIXF(m) \
	fn.glLoadTransposeMatrixf(HostAddr(m, const GLfloat *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLLOADTRANSPOSEMATRIXD(m) \
	GLdouble tmp[16]; \
	Atari2HostDoubleArray(16, m, tmp); \
	fn.glLoadTransposeMatrixd(tmp)
#else
#define FN_GLLOADTRANSPOSEMATRIXD(m) \
	fn.glLoadTransposeMatrixd(HostAddr(m, const GLdouble *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLMULTTRANSPOSEMATRIXD(m) \
	GLdouble tmp[16]; \
	Atari2HostDoubleArray(16, m, tmp); \
	fn.glMultTransposeMatrixd(tmp)
#else
#define FN_GLMULTTRANSPOSEMATRIXD(m) \
	fn.glMultTransposeMatrixd(HostAddr(m, const GLdouble *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLMULTTRANSPOSEMATRIXF(m) \
	GLfloat tmp[16]; \
	Atari2HostFloatArray(16, m, tmp); \
	fn.glMultTransposeMatrixf(tmp)
#else
#define FN_GLMULTTRANSPOSEMATRIXF(m) \
	fn.glMultTransposeMatrixf(HostAddr(m, const GLfloat *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * Version 1.4
 */

#define FN_GLMULTIDRAWARRAYS(mode, first, count, drawcount) \
	GLsizei const size = drawcount; \
	if (size <= 0) return; \
	GLsizei firstbuf[size]; \
	GLsizei countbuf[size]; \
	Atari2HostIntArray(size, first, firstbuf); \
	Atari2HostIntArray(size, count, countbuf); \
	for (GLsizei i = 0; i < size; i++) \
		convertClientArrays(firstbuf[i] + countbuf[i]); \
	fn.glMultiDrawArrays(mode, firstbuf, countbuf, drawcount)

#define FN_GLMULTIDRAWELEMENTS(mode, count, type, indices, drawcount) \
	GLsizei const size = drawcount; \
	if (size <= 0) return; \
	GLsizei countbuf[size]; \
	nfmemptr indbuf[size]; \
	void *indptr[size]; \
	pixelBuffer **pbuf; \
	Atari2HostIntArray(size, count, countbuf); \
	pbuf = new pixelBuffer *[size]; \
	Atari2HostPtrArray(size, AtariAddr(indices, const void **), indbuf); \
	for (GLsizei i = 0; i < size; i++) { \
		convertClientArrays(countbuf[i]); \
		switch(type) { \
		case GL_UNSIGNED_BYTE: \
		case GL_UNSIGNED_SHORT: \
		case GL_UNSIGNED_INT: \
			pbuf[i] = new pixelBuffer(); \
			indptr[i] = pbuf[i]->convertArray(countbuf[i], type, indbuf[i]); \
			break; \
		default: \
			glSetError(GL_INVALID_ENUM); \
			return; \
		} \
	} \
	fn.glMultiDrawElements(mode, countbuf, type, indptr, drawcount); \
	for (GLsizei i = 0; i < size; i++) { \
		delete pbuf[i]; \
	} \
	delete [] pbuf

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPOINTPARAMETERFV(pname, params) \
	GLfloat tmp[1]; \
	Atari2HostFloatArray(1, params, tmp); \
	fn.glPointParameterfv(pname, tmp)
#else
#define FN_GLPOINTPARAMETERFV(pname, params) \
	fn.glPointParameterfv(pname, HostAddr(params, const GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLPOINTPARAMETERIV(pname, params) \
	GLint tmp[1]; \
	Atari2HostIntArray(1, params, tmp); \
	fn.glPointParameteriv(pname, tmp)
#else
#define FN_GLPOINTPARAMETERIV(pname, params) \
	fn.glPointParameteriv(pname, HostAddr(params, const GLint *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLFOGCOORDFV(coord) \
	GLfloat tmp[1]; \
	Atari2HostFloatArray(1, coord, tmp); \
	fn.glFogCoordfv(tmp)
#else
#define FN_GLFOGCOORDFV(coord) \
	fn.glFogCoordfv(HostAddr(coord, const GLfloat *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLFOGCOORDDV(coord) \
	GLdouble tmp[1]; \
	Atari2HostDoubleArray(1, coord, tmp); \
	fn.glFogCoorddv(tmp)
#else
#define FN_GLFOGCOORDDV(coord) \
	fn.glFogCoorddv(HostAddr(coord, const GLdouble *))
#endif

#define FN_GLFOGCOORDPOINTER(type, stride, pointer) \
	GLint texunit = 0; \
	fn.glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE, &texunit); \
	setupClientArray(texunit, contexts[cur_context].fogcoord, 1, type, stride, -1, 0, AtariAddr(pointer, nfmemptr))

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLSECONDARYCOLOR3BV(v) \
	GLbyte tmp[3]; \
	Atari2HostByteArray(3, v, tmp); \
	fn.glSecondaryColor3bv(tmp)
#else
#define FN_GLSECONDARYCOLOR3BV(v) \
	fn.glSecondaryColor3bv(HostAddr(v, const GLbyte *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLSECONDARYCOLOR3DV(v) \
	GLdouble tmp[3]; \
	Atari2HostDoubleArray(3, v, tmp); \
	fn.glSecondaryColor3dv(tmp)
#else
#define FN_GLSECONDARYCOLOR3DV(v) \
	fn.glSecondaryColor3dv(HostAddr(v, const GLdouble *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLSECONDARYCOLOR3FV(v) \
	GLfloat tmp[3]; \
	Atari2HostFloatArray(3, v, tmp); \
	fn.glSecondaryColor3fv(tmp)
#else
#define FN_GLSECONDARYCOLOR3FV(v) \
	fn.glSecondaryColor3fv(HostAddr(v, const GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLSECONDARYCOLOR3IV(v) \
	GLint tmp[3]; \
	Atari2HostIntArray(3, v, tmp); \
	fn.glSecondaryColor3iv(tmp)
#else
#define FN_GLSECONDARYCOLOR3IV(v) \
	fn.glSecondaryColor3iv(HostAddr(v, const GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLSECONDARYCOLOR3SV(v) \
	GLshort tmp[3]; \
	Atari2HostShortArray(3, v, tmp); \
	fn.glSecondaryColor3sv(tmp)
#else
#define FN_GLSECONDARYCOLOR3SV(v) \
	fn.glSecondaryColor3sv(HostAddr(v, const GLshort *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLSECONDARYCOLOR3UBV(v) \
	GLubyte tmp[3]; \
	Atari2HostByteArray(3, v, tmp); \
	fn.glSecondaryColor3ubv(tmp)
#else
#define FN_GLSECONDARYCOLOR3UBV(v) \
	fn.glSecondaryColor3ubv(HostAddr(v, const GLubyte *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLSECONDARYCOLOR3UIV(v) \
	GLuint tmp[3]; \
	Atari2HostIntArray(3, v, tmp); \
	fn.glSecondaryColor3uiv(tmp)
#else
#define FN_GLSECONDARYCOLOR3UIV(v) \
	fn.glSecondaryColor3uiv(HostAddr(v, const GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLSECONDARYCOLOR3USV(v) \
	GLushort tmp[3]; \
	Atari2HostShortArray(3, v, tmp); \
	fn.glSecondaryColor3usv(tmp)
#else
#define FN_GLSECONDARYCOLOR3USV(v) \
	fn.glSecondaryColor3usv(HostAddr(v, const GLushort *))
#endif

#define FN_GLSECONDARYCOLORPOINTER(size, type, stride, pointer) \
	GLint texunit = 0; \
	fn.glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE, &texunit); \
	setupClientArray(texunit, contexts[cur_context].secondary_color, size, type, stride, -1, 0, AtariAddr(pointer, nfmemptr))

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLWINDOWPOS2DV(v) \
	GLdouble tmp[2]; \
	Atari2HostDoubleArray(2, v, tmp); \
	fn.glWindowPos2dv(tmp)
#else
#define FN_GLWINDOWPOS2DV(v) \
	fn.glWindowPos2dv(HostAddr(v, const GLdouble *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLWINDOWPOS2FV(v) \
	GLfloat tmp[2]; \
	Atari2HostFloatArray(2, v, tmp); \
	fn.glWindowPos2fv(tmp)
#else
#define FN_GLWINDOWPOS2FV(v) \
	fn.glWindowPos2fv(HostAddr(v, const GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLWINDOWPOS2IV(v) \
	GLint tmp[2]; \
	Atari2HostIntArray(2, v, tmp); \
	fn.glWindowPos2iv(tmp)
#else
#define FN_GLWINDOWPOS2IV(v) \
	fn.glWindowPos2iv(HostAddr(v, const GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLWINDOWPOS2SV(v) \
	GLshort tmp[2]; \
	Atari2HostShortArray(2, v, tmp); \
	fn.glWindowPos2sv(tmp)
#else
#define FN_GLWINDOWPOS2SV(v) \
	fn.glWindowPos2sv(HostAddr(v, const GLshort *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLWINDOWPOS3DV(v) \
	GLdouble tmp[3]; \
	Atari2HostDoubleArray(3, v, tmp); \
	fn.glWindowPos3dv(tmp)
#else
#define FN_GLWINDOWPOS3DV(v) \
	fn.glWindowPos3dv(HostAddr(v, const GLdouble *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLWINDOWPOS3FV(v) \
	GLfloat tmp[3]; \
	Atari2HostFloatArray(3, v, tmp); \
	fn.glWindowPos3fv(tmp)
#else
#define FN_GLWINDOWPOS3FV(v) \
	fn.glWindowPos3fv(HostAddr(v, const GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLWINDOWPOS3IV(v) \
	GLint tmp[3]; \
	Atari2HostIntArray(3, v, tmp); \
	fn.glWindowPos3iv(tmp)
#else
#define FN_GLWINDOWPOS3IV(v) \
	fn.glWindowPos3iv(HostAddr(v, const GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLWINDOWPOS3SV(v) \
	GLshort tmp[3]; \
	Atari2HostShortArray(3, v, tmp); \
	fn.glWindowPos3sv(tmp)
#else
#define FN_GLWINDOWPOS3SV(v) \
	fn.glWindowPos3sv(HostAddr(v, const GLshort *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * Version 1.5
 */

#define FN_GLBINDBUFFER(target, buffer) \
	gl_bind_buffer(target, buffer, 0, 0); \
	fn.glBindBuffer(target, buffer)

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGENQUERIES(n, ids) \
	GLsizei const size = MAX(n, 0); \
	GLuint tmp[size]; \
	fn.glGenQueries(n, tmp); \
	Host2AtariIntArray(n, tmp, ids)
#else
#define FN_GLGENQUERIES(n, ids) \
	fn.glGenQueries(n, HostAddr(ids, GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLDELETEQUERIES(n, ids) \
	GLuint tmp[MAX(n, 0)], *ptmp; \
	ptmp = Atari2HostIntArray(n, ids, tmp); \
	fn.glDeleteQueries(n, ptmp)
#else
#define FN_GLDELETEQUERIES(n, ids) \
	fn.glDeleteQueries(n, HostAddr(ids, const GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLDELETEBUFFERS(n, buffers) \
	GLuint tmp[MAX(n, 0)], *ptmp; \
	ptmp = Atari2HostIntArray(n, buffers, tmp); \
	fn.glDeleteBuffers(n, ptmp)
#else
#define FN_GLDELETEBUFFERS(n, buffers) \
	fn.glDeleteBuffers(n, HostAddr(buffers, const GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGENBUFFERS(n, buffers) \
	GLsizei const size = MAX(n, 0); \
	GLuint tmp[size]; \
	fn.glGenBuffers(n, tmp); \
	Host2AtariIntArray(n, tmp, buffers)
#else
#define FN_GLGENBUFFERS(n, buffers) \
	fn.glGenBuffers(n, HostAddr(buffers, GLuint *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLBUFFERDATA(target, size, data, usage) \
	GLubyte tmp[MAX(size, 0)], *ptmp; \
	ptmp = Atari2HostByteArray(sizeof(tmp), data, tmp); \
	fn.glBufferData(target, size, ptmp, usage)
#else
#define FN_GLBUFFERDATA(target, size, data, usage) \
	fn.glBufferData(target, size, HostAddr(data, const void *), usage)
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLBUFFERSUBDATA(target, offset, size, data) \
	GLubyte tmp[MAX(size, 0)], *ptmp; \
	ptmp = Atari2HostByteArray(sizeof(tmp), data, tmp); \
	fn.glBufferSubData(target, offset, size, ptmp)
#else
#define FN_GLBUFFERSUBDATA(target, offset, size, data) \
	fn.glBufferSubData(target, offset, size, HostAddr(data, const void *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETBUFFERPARAMETERIV(target, pname, params) \
	GLint tmp[4]; \
	int const size = nfglGetNumParams(pname); \
	fn.glGetBufferParameteriv(target, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETBUFFERPARAMETERIV(target, pname, params) \
	fn.glGetBufferParameteriv(target, pname, HostAddr(params, GLint *))
#endif

#define FN_GLGETBUFFERPOINTERV(target, pname, params) \
	void *tmp = NULL; \
	fn.glGetBufferPointerv(target, pname, &tmp); \
	/* TODO */ \
	memptr zero = 0; \
	Host2AtariIntArray(1, &zero, AtariAddr(params, memptr *))

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETQUERYIV(target, pname, params) \
	GLint const size = 1; \
	GLint tmp[size]; \
	fn.glGetQueryiv(target, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETQUERYIV(target, pname, params) \
	fn.glGetQueryiv(target, pname, HostAddr(params, GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETQUERYOBJECTIV(id, pname, params) \
	GLint const size = 1; \
	GLint tmp[size]; \
	fn.glGetQueryObjectiv(id, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETQUERYOBJECTIV(id, pname, params) \
	fn.glGetQueryObjectiv(id, pname, HostAddr(params, GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETQUERYOBJECTUIV(id, pname, params) \
	GLint const size = 1; \
	GLuint tmp[size]; \
	fn.glGetQueryObjectuiv(id, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETQUERYOBJECTUIV(id, pname, params) \
	fn.glGetQueryObjectuiv(id, pname, HostAddr(params, GLuint *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETHISTOGRAMPARAMETERFV(target, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLfloat tmp[MAX(size, 16)]; \
	fn.glGetHistogramParameterfv(target, pname, tmp); \
	Host2AtariFloatArray(size, tmp, params)
#else
#define FN_GLGETHISTOGRAMPARAMETERFV(target, pname, params) \
	fn.glGetHistogramParameterfv(target, pname, HostAddr(params, GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETHISTOGRAMPARAMETERIV(target, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLint tmp[MAX(size, 16)]; \
	fn.glGetHistogramParameteriv(target, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETHISTOGRAMPARAMETERIV(target, pname, params) \
	fn.glGetHistogramParameteriv(target, pname, HostAddr(params, GLint *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * Version 2.0
 */
#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLBINDATTRIBLOCATION(program, index, name) \
	GLchar tmp[safe_strlen(name) + 1], *ptmp; \
	ptmp = Atari2HostByteArray(sizeof(tmp), name, tmp); \
	fn.glBindAttribLocation(program, index, ptmp)
#else
#define FN_GLBINDATTRIBLOCATION(program, index, name) \
	fn.glBindAttribLocation(program, index, HostAddr(name, const GLchar *))
#endif
	
#if NFOSMESA_NEED_INT_CONV
#define FN_GLDRAWBUFFERS(n, bufs) \
	GLenum tmp[MAX(n, 0)], *ptmp; \
	ptmp = Atari2HostIntArray(n, bufs, tmp); \
	fn.glDrawBuffers(n, ptmp)
#else
#define FN_GLDRAWBUFFERS(n, bufs) \
	fn.glDrawBuffers(n, HostAddr(bufs, const GLuint *))
#endif

#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_INT_CONV
#define FN_GLGETACTIVEATTRIB(program, index, bufSize, length, size, type, name) \
	GLsizei l; \
	GLint s; \
	GLenum t; \
	GLchar tmp[MAX(bufSize, 0)]; \
	fn.glGetActiveAttrib(program, index, bufSize, &l, &s, &t, tmp); \
	Host2AtariByteArray(MIN(l + 1, bufSize), tmp, name); \
	Host2AtariIntArray(1, &l, length); \
	Host2AtariIntArray(1, &s, size); \
	Host2AtariIntArray(1, &t, type)
#else
#define FN_GLGETACTIVEATTRIB(program, index, bufSize, length, size, type, name) \
	fn.glGetActiveAttrib(program, index, bufSize, HostAddr(length, GLsizei *), HostAddr(size, GLint *), HostAddr(type, GLenum *), HostAddr(name, GLchar *))
#endif

#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_INT_CONV
#define FN_GLGETACTIVEUNIFORM(program, index, bufSize, length, size, type, name) \
	GLsizei l; \
	GLint s; \
	GLenum t; \
	GLchar namebuf[MAX(bufSize, 0)]; \
	fn.glGetActiveUniform(program, index, bufSize, &l, &s, &t, namebuf); \
	Host2AtariByteArray(MIN(l + 1, bufSize), namebuf, name); \
	Host2AtariIntArray(1, &l, length); \
	Host2AtariIntArray(1, &s, size); \
	Host2AtariIntArray(1, &t, type)
#else
#define FN_GLGETACTIVEUNIFORM(program, index, bufSize, length, size, type, name) \
	fn.glGetActiveUniform(program, index, bufSize, HostAddr(length, GLsizei *), HostAddr(size, GLint *), HostAddr(type, GLenum *), HostAddr(name, GLchar *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETATTACHEDSHADERS(program, maxCount, count, obj) \
	GLsizei size = 0; \
	GLuint tmp[MAX(maxCount, 0)]; \
	fn.glGetAttachedShaders(program, maxCount, &size, tmp); \
	Host2AtariIntArray(1, &size, count); \
	Host2AtariIntArray(size, tmp, obj)
#else
#define FN_GLGETATTACHEDSHADERS(program, maxCount, count, obj) \
	fn.glGetAttachedShaders(program, maxCount, HostAddr(count, GLsizei *), HostAddr(obj, GLuint *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLGETATTRIBLOCATION(program, name) \
	GLchar namebuf[safe_strlen(name) + 1], *pname ; \
	pname = Atari2HostByteArray(sizeof(namebuf), name, namebuf); \
	return fn.glGetAttribLocation(program, pname)
#else
#define FN_GLGETATTRIBLOCATION(program, name) \
	return fn.glGetAttribLocation(program, HostAddr(name, const GLchar *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLUNIFORM1FV(location, count, value)	\
	GLint const size = 1 * count; \
	if (size <= 0) return; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, value, tmp); \
	fn.glUniform1fv(location, count, tmp)
#else
#define FN_GLUNIFORM1FV(location, count, value) \
	fn.glUniform1fv(location, count, HostAddr(value, const GLfloat *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLUNIFORM2FV(location, count, value) \
	GLint const size = 2 * count; \
	if (size <= 0) return; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, value, tmp); \
	fn.glUniform2fv(location, count, tmp)
#else
#define FN_GLUNIFORM2FV(location, count, value) \
	fn.glUniform2fv(location, count, HostAddr(value, const GLfloat *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLUNIFORM3FV(location, count, value) \
	GLint const size = 3 * count; \
	if (size <= 0) return; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, value, tmp); \
	fn.glUniform3fv(location, count, tmp)
#else
#define FN_GLUNIFORM3FV(location, count, value) \
	fn.glUniform3fv(location, count, HostAddr(value, const GLfloat *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLUNIFORM4FV(location, count, value) \
	GLint const size = 4 * count; \
	if (size <= 0) return; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, value, tmp); \
	fn.glUniform4fv(location, count, tmp)
#else
#define FN_GLUNIFORM4FV(location, count, value) \
	fn.glUniform4fv(location, count, HostAddr(value, const GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLUNIFORM1IV(location, count, value) \
	GLint const size = 1 * count; \
	if (size <= 0) return; \
	GLint tmp[size]; \
	Atari2HostIntArray(size, value, tmp); \
	fn.glUniform1iv(location, count, tmp)
#else
#define FN_GLUNIFORM1IV(location, count, value) \
	fn.glUniform1iv(location, count, HostAddr(value, const GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLUNIFORM2IV(location, count, value) \
	GLint const size = 2 * count; \
	if (size <= 0) return; \
	GLint tmp[size]; \
	Atari2HostIntArray(size, value, tmp); \
	fn.glUniform2iv(location, count, tmp)
#else
#define FN_GLUNIFORM2IV(location, count, value) \
	fn.glUniform2iv(location, count, HostAddr(value, const GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLUNIFORM3IV(location, count, value) \
	GLint const size = 3 * count; \
	if (size <= 0) return; \
	GLint tmp[size]; \
	Atari2HostIntArray(size, value, tmp); \
	fn.glUniform3iv(location, count, tmp)
#else
#define FN_GLUNIFORM3IV(location, count, value) \
	fn.glUniform3iv(location, count, HostAddr(value, const GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLUNIFORM4IV(location, count, value) \
	GLint const size = 4 * count; \
	if (size <= 0) return; \
	GLint tmp[size]; \
	Atari2HostIntArray(size, value, tmp); \
	fn.glUniform4iv(location, count, tmp)
#else
#define FN_GLUNIFORM4IV(location, count, value) \
	fn.glUniform4iv(location, count, HostAddr(value, const GLint *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLVERTEXATTRIB1DV(index, v) \
	GLint const size = 1; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, v, tmp); \
	fn.glVertexAttrib1dv(index, tmp)
#else
#define FN_GLVERTEXATTRIB1DV(index, v) \
	fn.glVertexAttrib1dv(index, HostAddr(v, const GLdouble *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLVERTEXATTRIB1FV(index, v) \
	GLint const size = 1; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, v, tmp); \
	fn.glVertexAttrib1fv(index, tmp)
#else
#define FN_GLVERTEXATTRIB1FV(index, v) \
	fn.glVertexAttrib1fv(index, HostAddr(v, const GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIB1SV(index, v) \
	GLint const size = 1; \
	GLshort tmp[size]; \
	Atari2HostShortArray(size, v, tmp); \
	fn.glVertexAttrib1sv(index, tmp)
#else
#define FN_GLVERTEXATTRIB1SV(index, v) \
	fn.glVertexAttrib1sv(index, HostAddr(v, const GLshort *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLVERTEXATTRIB2DV(index, v) \
	GLint const size = 2; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, v, tmp); \
	fn.glVertexAttrib2dv(index, tmp)
#else
#define FN_GLVERTEXATTRIB2DV(index, v) \
	fn.glVertexAttrib2dv(index, HostAddr(v, const GLdouble *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLVERTEXATTRIB2FV(index, v) \
	GLint const size = 2; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, v, tmp); \
	fn.glVertexAttrib2fv(index, tmp)
#else
#define FN_GLVERTEXATTRIB2FV(index, v) \
	fn.glVertexAttrib2fv(index, HostAddr(v, const GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIB2SV(index, v) \
	GLint const size = 2; \
	GLshort tmp[size]; \
	Atari2HostShortArray(size, v, tmp); \
	fn.glVertexAttrib2sv(index, tmp)
#else
#define FN_GLVERTEXATTRIB2SV(index, v) \
	fn.glVertexAttrib2sv(index, HostAddr(v, const GLshort *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLVERTEXATTRIB3DV(index, v) \
	GLint const size = 3; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, v, tmp); \
	fn.glVertexAttrib3dv(index, tmp)
#else
#define FN_GLVERTEXATTRIB3DV(index, v) \
	fn.glVertexAttrib3dv(index, HostAddr(v, const GLdouble *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLVERTEXATTRIB3FV(index, v) \
	GLint const size = 3; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, v, tmp); \
	fn.glVertexAttrib3fv(index, tmp)
#else
#define FN_GLVERTEXATTRIB3FV(index, v) \
	fn.glVertexAttrib3fv(index, HostAddr(v, const GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIB3SV(index, v) \
	GLint const size = 3; \
	GLshort tmp[size]; \
	Atari2HostShortArray(size, v, tmp); \
	fn.glVertexAttrib3sv(index, tmp)
#else
#define FN_GLVERTEXATTRIB3SV(index, v) \
	fn.glVertexAttrib3sv(index, HostAddr(v, const GLshort *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLVERTEXATTRIB4NBV(index, v) \
	GLint const size = 4; \
	GLbyte tmp[size], *ptmp; \
	ptmp = Atari2HostByteArray(size, v, tmp); \
	fn.glVertexAttrib4Nbv(index, ptmp)
#else
#define FN_GLVERTEXATTRIB4NBV(index, v) \
	fn.glVertexAttrib4Nbv(index, HostAddr(v, const GLbyte *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIB4NIV(index, v) \
	GLint const size = 4; \
	GLint tmp[size], *ptmp; \
	ptmp = Atari2HostIntArray(size, v, tmp); \
	fn.glVertexAttrib4Niv(index, ptmp)
#else
#define FN_GLVERTEXATTRIB4NIV(index, v) \
	fn.glVertexAttrib4Niv(index, HostAddr(v, const GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIB4NSV(index, v) \
	GLint const size = 4; \
	GLshort tmp[size]; \
	Atari2HostShortArray(size, v, tmp); \
	fn.glVertexAttrib4Nsv(index, tmp)
#else
#define FN_GLVERTEXATTRIB4NSV(index, v) \
	fn.glVertexAttrib4Nsv(index, HostAddr(v, const GLshort *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIB4NUIV(index, v) \
	GLint const size = 4; \
	GLuint tmp[size]; \
	Atari2HostIntArray(size, v, tmp); \
	fn.glVertexAttrib4Nuiv(index, tmp)
#else
#define FN_GLVERTEXATTRIB4NUIV(index, v) \
	fn.glVertexAttrib4Nuiv(index, HostAddr(v, const GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIB4NUSV(index, v) \
	GLint const size = 4; \
	GLushort tmp[size]; \
	Atari2HostShortArray(size, v, tmp); \
	fn.glVertexAttrib4Nusv(index, tmp)
#else
#define FN_GLVERTEXATTRIB4NUSV(index, v) \
	fn.glVertexAttrib4Nusv(index, HostAddr(v, const GLushort *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLVERTEXATTRIB4BV(index, v) \
	GLint const size = 4; \
	GLbyte tmp[size]; \
	Atari2HostByteArray(size, v, tmp); \
	fn.glVertexAttrib4bv(index, tmp)
#else
#define FN_GLVERTEXATTRIB4BV(index, v) \
	fn.glVertexAttrib4bv(index, HostAddr(v, const GLbyte *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLVERTEXATTRIB4UBV(index, v) \
	GLint const size = 4; \
	GLubyte tmp[size]; \
	Atari2HostByteArray(size, v, tmp); \
	fn.glVertexAttrib4ubv(index, tmp)
#else
#define FN_GLVERTEXATTRIB4UBV(index, v) \
	fn.glVertexAttrib4ubv(index, HostAddr(v, const GLubyte *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLVERTEXATTRIB4DV(index, v) \
	GLint const size = 4; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, v, tmp); \
	fn.glVertexAttrib4dv(index, tmp)
#else
#define FN_GLVERTEXATTRIB4DV(index, v) \
	fn.glVertexAttrib4dv(index, HostAddr(v, const GLdouble *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLVERTEXATTRIB4FV(index, v) \
	GLint const size = 4; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, v, tmp); \
	fn.glVertexAttrib4fv(index, tmp)
#else
#define FN_GLVERTEXATTRIB4FV(index, v) \
	fn.glVertexAttrib4fv(index, HostAddr(v, const GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIB4IV(index, v) \
	GLint const size = 4; \
	GLint tmp[size]; \
	Atari2HostIntArray(size, v, tmp); \
	fn.glVertexAttrib4iv(index, tmp)
#else
#define FN_GLVERTEXATTRIB4IV(index, v) \
	fn.glVertexAttrib4iv(index, HostAddr(v, const GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIB4SV(index, v) \
	GLint const size = 4; \
	GLshort tmp[size]; \
	Atari2HostShortArray(size, v, tmp); \
	fn.glVertexAttrib4sv(index, tmp)
#else
#define FN_GLVERTEXATTRIB4SV(index, v) \
	fn.glVertexAttrib4sv(index, HostAddr(v, const GLshort *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIB4UIV(index, v) \
	GLint const size = 4; \
	GLuint tmp[size]; \
	Atari2HostIntArray(size, v, tmp); \
	fn.glVertexAttrib4uiv(index, tmp)
#else
#define FN_GLVERTEXATTRIB4UIV(index, v) \
	fn.glVertexAttrib4uiv(index, HostAddr(v, const GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIB4USV(index, v) \
	GLint const size = 4; \
	GLushort tmp[size]; \
	Atari2HostShortArray(size, v, tmp); \
	fn.glVertexAttrib4usv(index, tmp)
#else
#define FN_GLVERTEXATTRIB4USV(index, v) \
	fn.glVertexAttrib4usv(index, HostAddr(v, const GLushort *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETPROGRAMIV(target, index, params) \
	GLint const size = 1; \
	GLint tmp[size]; \
	fn.glGetProgramiv(target, index, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETPROGRAMIV(target, index, params) \
	fn.glGetProgramiv(target, index, HostAddr(params, GLint *))
#endif

#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_INT_CONV
#define FN_GLGETPROGRAMINFOLOG(program, bufSize, length, infoLog) \
	GLint l = 0; \
	GLchar logbuf[MAX(bufSize, 0)]; \
	fn.glGetProgramInfoLog(program, bufSize, &l, logbuf); \
	Host2AtariByteArray(MIN(l + 1, bufSize), logbuf, infoLog); \
	Host2AtariIntArray(1, &l, length)
#else
#define FN_GLGETPROGRAMINFOLOG(program, bufSize, length, infoLog) \
	fn.glGetProgramInfoLog(program, bufSize, HostAddr(length, GLsizei *), HostAddr(infoLog, GLchar *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETSHADERIV(shader, pname, params) \
	GLint const size = 1; \
	GLint tmp[size]; \
	fn.glGetShaderiv(shader, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETSHADERIV(shader, pname, params) \
	fn.glGetShaderiv(shader, pname, HostAddr(params, GLint *))
#endif

#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_INT_CONV
#define FN_GLGETSHADERINFOLOG(program, bufSize, length, infoLog) \
	GLint l = 0; \
	GLchar infobuf[MAX(bufSize, 0)]; \
	fn.glGetShaderInfoLog(program, bufSize, &l, infobuf); \
	Host2AtariByteArray(MIN(l + 1, bufSize), infobuf, infoLog); \
	Host2AtariIntArray(1, &l, length)
#else
#define FN_GLGETSHADERINFOLOG(program, bufSize, length, infoLog) \
	fn.glGetShaderInfoLog(program, bufSize, HostAddr(length, GLint *), HostAddr(infoLog, GLchar *))
#endif

#define FN_GLSHADERSOURCE(shader, count, strings, length) \
	void *pstrings[MAX(count, 0)], **ppstrings; \
	GLint lengthbuf[MAX(count, 0)], *plength; \
	ppstrings = Atari2HostPtrArray(count, strings, pstrings); \
	plength = Atari2HostIntArray(count, length, lengthbuf); \
	fn.glShaderSource(shader, count, (const GLchar **)ppstrings, plength)

#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_INT_CONV
#define FN_GLGETSHADERSOURCE(program, maxLength, length, source) \
	GLsizei l = 0; \
	GLchar srcbuf[MAX(maxLength, 0)]; \
	fn.glGetShaderSource(program, maxLength, &l, srcbuf); \
	Host2AtariByteArray(MIN(l + 1, maxLength), srcbuf, source); \
	Host2AtariIntArray(1, &l, length)
#else
#define FN_GLGETSHADERSOURCE(program, maxLength, length, source) \
	fn.glGetShaderSource(program, maxLength, HostAddr(length, GLsizei *), HostAddr(source, GLchar *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLGETUNIFORMLOCATION(program, name) \
	GLchar tmp[safe_strlen(name) + 1], *ptmp; \
	ptmp = Atari2HostByteArray(sizeof(tmp), name, tmp); \
	return fn.glGetUniformLocation(program, ptmp)
#else
#define FN_GLGETUNIFORMLOCATION(program, nama) \
	return fn.glGetUniformLocation(program, HostAddr(name, const GLchar *))
#endif
	
#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETUNIFORMFV(program, location, params) \
	GLint const size = 1; \
	GLfloat tmp[size]; \
	fn.glGetUniformfv(program, location, tmp); \
	Host2AtariFloatArray(size, tmp, params)
#else
#define FN_GLGETUNIFORMFV(program, location, params) \
	fn.glGetUniformfv(program, location, HostAddr(params, GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETUNIFORMIV(program, location, params) \
	GLint const size = 1; \
	GLint tmp[size]; \
	fn.glGetUniformiv(program, location, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETUNIFORMIV(program, location, params) \
	fn.glGetUniformiv(program, location, HostAddr(params, GLint *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLGETVERTEXATTRIBDV(index, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLdouble tmp[size]; \
	fn.glGetVertexAttribdv(index, pname, tmp); \
	Host2AtariDoubleArray(size, tmp, params)
#else
#define FN_GLGETVERTEXATTRIBDV(index, pname, params) \
	fn.glGetVertexAttribdv(index, pname, HostAddr(params, GLdouble *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETVERTEXATTRIBFV(index, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLfloat tmp[size]; \
	fn.glGetVertexAttribfv(index, pname, tmp); \
	Host2AtariFloatArray(size, tmp, params)
#else
#define FN_GLGETVERTEXATTRIBFV(index, pname, params) \
	fn.glGetVertexAttribfv(index, pname, HostAddr(params, GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETVERTEXATTRIBIV(index, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLint tmp[size]; \
	fn.glGetVertexAttribiv(index, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETVERTEXATTRIBIV(index, pname, params) \
	fn.glGetVertexAttribiv(index, pname, HostAddr(params, GLint *))
#endif

/* TODO */
#define FN_GLGETVERTEXATTRIBPOINTERV(index, pname, pointer) \
	UNUSED(pointer); \
	void *p = 0; \
	fn.glGetVertexAttribPointerv(index, pname, &p)

/* TODO */
#define FN_GLVERTEXATTRIBPOINTER(index, size, type, normalized, stride, pointer) \
	UNUSED(pointer); \
	void *p = 0; \
	fn.glVertexAttribPointer(index, size, type, normalized, stride, &p)

#if NFOSMESA_NEED_INT_CONV
#define FN_GLREQUESTRESIDENTPROGRAMSNV(n, programs) \
	GLint const size = n; \
	if (size <= 0 || !programs) return; \
	GLuint tmp[size]; \
	Atari2HostIntArray(size, programs, tmp); \
	fn.glRequestResidentProgramsNV(n, tmp)
#else
#define FN_GLREQUESTRESIDENTPROGRAMSNV(n, programs) \
	fn.glRequestResidentProgramsNV(n, HostAddr(programs, const GLuint *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLUNIFORMMATRIX2FV(location, count, transpose, value) \
	GLint const size = 4 * count; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, value, tmp); \
	fn.glUniformMatrix2fv(location, count, transpose, tmp)
#else
#define FN_GLUNIFORMMATRIX2FV(location, count, transpose, value) \
	fn.glUniformMatrix2fv(location, count, transpose, HostAddr(value, const GLfloat *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLUNIFORMMATRIX3FV(location, count, transpose, value) \
	GLint const size = 9 * count; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, value, tmp); \
	fn.glUniformMatrix3fv(location, count, transpose, tmp)
#else
#define FN_GLUNIFORMMATRIX3FV(location, count, transpose, value) \
	fn.glUniformMatrix3fv(location, count, transpose, HostAddr(value, const GLfloat *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLUNIFORMMATRIX4FV(location, count, transpose, value) \
	GLint const size = 16 * count; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, value, tmp); \
	fn.glUniformMatrix4fv(location, count, transpose, tmp)
#else
#define FN_GLUNIFORMMATRIX4FV(location, count, transpose, value) \
	fn.glUniformMatrix4fv(location, count, transpose, HostAddr(value, const GLfloat *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * Version 2.1
 */

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLUNIFORMMATRIX2X3FV(location, count, transpose, value) \
	GLint const size = 6 * count; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, value, tmp); \
	fn.glUniformMatrix2x3fv(location, count, transpose, tmp)
#else
#define FN_GLUNIFORMMATRIX2X3FV(location, count, transpose, value) \
	fn.glUniformMatrix2x3fv(location, count, transpose, HostAddr(value, const GLfloat *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLUNIFORMMATRIX3X2FV(location, count, transpose, value) \
	GLint const size = 6 * count; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, value, tmp); \
	fn.glUniformMatrix3x2fv(location, count, transpose, tmp)
#else
#define FN_GLUNIFORMMATRIX3X2FV(location, count, transpose, value) \
	fn.glUniformMatrix3x2fv(location, count, transpose, HostAddr(value, const GLfloat *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLUNIFORMMATRIX2X4FV(location, count, transpose, value) \
	GLint const size = 8 * count; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, value, tmp); \
	fn.glUniformMatrix2x4fv(location, count, transpose, tmp)
#else
#define FN_GLUNIFORMMATRIX2X4FV(location, count, transpose, value) \
	fn.glUniformMatrix2x4fv(location, count, transpose, HostAddr(value, const GLfloat *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLUNIFORMMATRIX4X2FV(location, count, transpose, value) \
	GLint const size = 8 * count; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, value, tmp); \
	fn.glUniformMatrix4x2fv(location, count, transpose, tmp)
#else
#define FN_GLUNIFORMMATRIX4X2FV(location, count, transpose, value) \
	fn.glUniformMatrix4x2fv(location, count, transpose, HostAddr(value, const GLfloat *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLUNIFORMMATRIX3X4FV(location, count, transpose, value) \
	GLint const size = 12 * count; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, value, tmp); \
	fn.glUniformMatrix3x4fv(location, count, transpose, tmp)
#else
#define FN_GLUNIFORMMATRIX3X4FV(location, count, transpose, value) \
	fn.glUniformMatrix3x4fv(location, count, transpose, HostAddr(value, const GLfloat *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLUNIFORMMATRIX4X3FV(location, count, transpose, value) \
	GLint const size = 12 * count; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, value, tmp); \
	fn.glUniformMatrix4x3fv(location, count, transpose, tmp)
#else
#define FN_GLUNIFORMMATRIX4X3FV(location, count, transpose, value) \
	fn.glUniformMatrix4x3fv(location, count, transpose, HostAddr(value, const GLfloat *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * Version 3.0
 */

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLBINDFRAGDATALOCATION(program, color, name) \
	GLchar tmp[safe_strlen(name) + 1], *ptmp; \
	ptmp = Atari2HostByteArray(sizeof(tmp), name, tmp); \
	fn.glBindFragDataLocation(program, color, ptmp)
#else
#define FN_GLBINDFRAGDATALOCATION(program, color, name) \
	fn.glBindFragDataLocation(program, color, HostAddr(name, const GLchar *))
#endif
	
#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLGETFRAGDATAINDEX(program, name) \
	GLchar tmp[safe_strlen(name) + 1], *ptmp; \
	ptmp = Atari2HostByteArray(sizeof(tmp), name, tmp); \
	return fn.glGetFragDataIndex(program, ptmp)
#else
#define FN_GLGETFRAGDATAINDEX(program, name) \
	return fn.glGetFragDataIndex(program, HostAddr(name, const GLchar *))
#endif
	
#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLGETFRAGDATALOCATION(program, name) \
	GLchar tmp[safe_strlen(name) + 1], *ptmp; \
	ptmp = Atari2HostByteArray(sizeof(tmp), name, tmp); \
	return fn.glGetFragDataLocation(program, ptmp)
#else
#define FN_GLGETFRAGDATALOCATION(program, name) \
	return fn.glGetFragDataLocation(program, HostAddr(name, const GLchar *))
#endif
	
#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLGETBOOLEANI_V(target, index, data) \
	GLint const size = nfglGetNumParams(target); \
	GLboolean tmp[MAX(size, 16)]; \
	fn.glGetBooleani_v(target, index, tmp); \
	Host2AtariByteArray(size, tmp, data)
#else
#define FN_GLGETBOOLEANI_V(target, index, data) \
	fn.glGetBooleani_v(target, index, HostAddr(data, GLboolean *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETINTEGERI_V(target, index, data) \
	int const n = nfglGetNumParams(target); \
	GLint tmp[MAX(n, 16)]; \
	fn.glGetIntegeri_v(target, index, tmp); \
	Host2AtariIntArray(n, tmp, data)
#else
#define FN_GLGETINTEGERI_V(target, index, data) \
	fn.glGetIntegeri_v(target, index, HostAddr(data, GLint *))
#endif

#define FN_GLBINDBUFFERRANGE(target, index, buffer, offset, size) \
	gl_bind_buffer(target, buffer, 0, 0); \
	fn.glBindBufferRange(target, index, buffer, offset, size)

#define FN_GLBINDBUFFERBASE(target, index, buffer) \
	gl_bind_buffer(target, buffer, 0, 0); \
	fn.glBindBufferBase(target, index, buffer)

#define FN_GLTRANSFORMFEEDBACKVARYINGS(program, count, strings, bufferMode) \
	void *pstrings[MAX(count, 0)], **ppstrings; \
	ppstrings = Atari2HostPtrArray(count, strings, pstrings); \
	fn.glTransformFeedbackVaryings(program, count, (const GLchar *const *)ppstrings, bufferMode)

#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_INT_CONV
#define FN_GLGETTRANSFORMFEEDBACKVARYING(program, index, bufSize, length, size, type, name) \
	GLsizei l = 0; \
	GLsizei s = 0; \
	GLenum t = 0; \
	GLchar namebuf[MAX(bufSize, 0)]; \
	fn.glGetTransformFeedbackVarying(program, index, bufSize, &l, &s, &t, namebuf); \
	Host2AtariByteArray(MIN(l + 1, bufSize), namebuf, name); \
	Host2AtariIntArray(1, &l, length); \
	Host2AtariIntArray(1, &s, size); \
	Host2AtariIntArray(1, &t, type)
#else
#define FN_GLGETTRANSFORMFEEDBACKVARYING(program, index, bufSize, length, size, type, name) \
	fn.glGetTransformFeedbackVarying(program, index, bufSize, HostAddr(length, GLsizei *), HostAddr(size, GLsizei *), HostAddr(type, GLenum *), HostAddr(name, GLchar *))
#endif

/* TODO */
#define FN_GLVERTEXATTRIBIPOINTER(index, size, type, stride, pointer) \
	fn.glVertexAttribIPointer(index, size, type, stride, HostAddr(pointer, const void *))

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETVERTEXATTRIBIIV(index, pname, params) \
	int const n = nfglGetNumParams(pname); \
	GLint tmp[MAX(n, 16)]; \
	fn.glGetVertexAttribIiv(index, pname, tmp); \
	Host2AtariIntArray(n, tmp, params)
#else
#define FN_GLGETVERTEXATTRIBIIV(index, pname, params) \
	fn.glGetVertexAttribIiv(index, pname, HostAddr(params, GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETVERTEXATTRIBIUIV(index, pname, params) \
	int const n = nfglGetNumParams(pname); \
	GLuint tmp[MAX(n, 16)]; \
	fn.glGetVertexAttribIuiv(index, pname, tmp); \
	Host2AtariIntArray(n, tmp, params)
#else
#define FN_GLGETVERTEXATTRIBIUIV(index, pname, params) \
	fn.glGetVertexAttribIuiv(index, pname, HostAddr(params, GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIBI1IV(index, v) \
	int const size = 1; \
	GLint tmp[size], *ptmp; \
	ptmp = Atari2HostIntArray(size, v, tmp); \
	fn.glVertexAttribI1iv(index, ptmp)
#else
#define FN_GLVERTEXATTRIBI1IV(index, v) \
	fn.glVertexAttribI1iv(index, HostAddr(v, const GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIBI2IV(index, v) \
	int const size = 2; \
	GLint tmp[size], *ptmp; \
	ptmp = Atari2HostIntArray(size, v, tmp); \
	fn.glVertexAttribI2iv(index, ptmp)
#else
#define FN_GLVERTEXATTRIBI2IV(index, v) \
	fn.glVertexAttribI2iv(index, HostAddr(v, const GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIBI3IV(index, v) \
	int const size = 3; \
	GLint tmp[size], *ptmp; \
	ptmp = Atari2HostIntArray(size, v, tmp); \
	fn.glVertexAttribI3iv(index, ptmp)
#else
#define FN_GLVERTEXATTRIBI3IV(index, v) \
	fn.glVertexAttribI3iv(index, HostAddr(v, const GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIBI4IV(index, v) \
	int const size = 4; \
	GLint tmp[size], *ptmp; \
	ptmp = Atari2HostIntArray(size, v, tmp); \
	fn.glVertexAttribI4iv(index, ptmp)
#else
#define FN_GLVERTEXATTRIBI4IV(index, v) \
	fn.glVertexAttribI4iv(index, HostAddr(v, const GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIBI1UIV(index, v) \
	int const size = 1; \
	GLuint tmp[size], *ptmp; \
	ptmp = Atari2HostIntArray(size, v, tmp); \
	fn.glVertexAttribI1uiv(index, ptmp)
#else
#define FN_GLVERTEXATTRIBI1UIV(index, v) \
	fn.glVertexAttribI1uiv(index, HostAddr(v, const GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIBI2UIV(index, v) \
	int const size = 2; \
	GLuint tmp[size], *ptmp; \
	ptmp = Atari2HostIntArray(size, v, tmp); \
	fn.glVertexAttribI2uiv(index, ptmp)
#else
#define FN_GLVERTEXATTRIBI2UIV(index, v) \
	fn.glVertexAttribI2uiv(index, HostAddr(v, const GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIBI3UIV(index, v) \
	int const size = 3; \
	GLuint tmp[size], *ptmp; \
	ptmp = Atari2HostIntArray(size, v, tmp); \
	fn.glVertexAttribI3uiv(index, ptmp)
#else
#define FN_GLVERTEXATTRIBI3UIV(index, v) \
	fn.glVertexAttribI3uiv(index, HostAddr(v, const GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIBI4UIV(index, v) \
	int const size = 4; \
	GLuint tmp[size], *ptmp; \
	ptmp = Atari2HostIntArray(size, v, tmp); \
	fn.glVertexAttribI4uiv(index, ptmp)
#else
#define FN_GLVERTEXATTRIBI4UIV(index, v) \
	fn.glVertexAttribI4uiv(index, HostAddr(v, const GLuint *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLVERTEXATTRIBI4BV(index, v) \
	int const size = 4; \
	GLbyte tmp[size], *ptmp; \
	ptmp = Atari2HostByteArray(size, v, tmp); \
	fn.glVertexAttribI4bv(index, ptmp)
#else
#define FN_GLVERTEXATTRIBI4BV(index, v) \
	fn.glVertexAttribI4bv(index, HostAddr(v, const GLbyte *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIBI4SV(index, v) \
	int const size = 4; \
	GLshort tmp[size], *ptmp; \
	ptmp = Atari2HostShortArray(size, v, tmp); \
	fn.glVertexAttribI4sv(index, ptmp)
#else
#define FN_GLVERTEXATTRIBI4SV(index, v) \
	fn.glVertexAttribI4sv(index, HostAddr(v, const GLshort *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLVERTEXATTRIBI4UBV(index, v) \
	int const size = 4; \
	GLubyte tmp[size], *ptmp; \
	ptmp = Atari2HostByteArray(size, v, tmp); \
	fn.glVertexAttribI4ubv(index, ptmp)
#else
#define FN_GLVERTEXATTRIBI4UBV(index, v) \
	fn.glVertexAttribI4ubv(index, HostAddr(v, const GLubyte *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIBI4USV(index, v) \
	int const size = 4; \
	GLushort tmp[size], *ptmp; \
	ptmp = Atari2HostShortArray(size, v, tmp); \
	fn.glVertexAttribI4usv(index, ptmp)
#else
#define FN_GLVERTEXATTRIBI4USV(index, v) \
	fn.glVertexAttribI4usv(index, HostAddr(v, const GLushort *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETUNIFORMUIV(program, location, params) \
	GLint const size = 1; \
	GLuint tmp[size]; \
	fn.glGetUniformuiv(program, location, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETUNIFORMUIV(program, location, params) \
	fn.glGetUniformuiv(program, location, HostAddr(params, GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLUNIFORM1UIV(location, count, value) \
	GLint const size = 1 * count; \
	GLuint tmp[MAX(size, 0)]; \
	Atari2HostIntArray(size, value, tmp); \
	fn.glUniform1uiv(location, count, tmp)
#else
#define FN_GLUNIFORM1UIV(location, count, value) \
	fn.glUniform1uiv(location, count, HostAddr(value, const GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLUNIFORM2UIV(location, count, value) \
	GLint const size = 2 * count; \
	GLuint tmp[MAX(size, 0)], *ptmp; \
	ptmp = Atari2HostIntArray(size, value, tmp); \
	fn.glUniform2uiv(location, count, ptmp)
#else
#define FN_GLUNIFORM2UIV(location, count, value) \
	fn.glUniform2uiv(location, count, HostAddr(value, const GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLUNIFORM3UIV(location, count, value) \
	GLint const size = 3 * count; \
	if (size <= 0) return; \
	GLuint tmp[size]; \
	Atari2HostIntArray(size, value, tmp); \
	fn.glUniform3uiv(location, count, tmp)
#else
#define FN_GLUNIFORM3UIV(location, count, value) \
	fn.glUniform3uiv(location, count, HostAddr(value, const GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLUNIFORM4UIV(location, count, value) \
	GLint const size = 4 * count; \
	if (size <= 0) return; \
	GLuint tmp[size]; \
	Atari2HostIntArray(size, value, tmp); \
	fn.glUniform4uiv(location, count, tmp)
#else
#define FN_GLUNIFORM4UIV(location, count, value) \
	fn.glUniform4uiv(location, count, HostAddr(value, const GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLTEXPARAMETERIIV(target, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLint tmp[MAX(size, 16)]; \
	Atari2HostIntArray(size, params, tmp); \
	fn.glTexParameterIiv(target, pname, tmp)
#else
#define FN_GLTEXPARAMETERIIV(target, pname, params) \
	fn.glTexParameterIiv(target, pname, HostAddr(params, const GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLTEXPARAMETERIUIV(target, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLuint tmp[MAX(size, 16)]; \
	Atari2HostIntArray(size, params, tmp); \
	fn.glTexParameterIuiv(target, pname, tmp)
#else
#define FN_GLTEXPARAMETERIUIV(target, pname, params) \
	fn.glTexParameterIuiv(target, pname, HostAddr(params, const GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETTEXPARAMETERIIV(target, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLint tmp[MAX(size, 16)]; \
	fn.glGetTexParameterIiv(target, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETTEXPARAMETERIIV(target, pname, params) \
	fn.glGetTexParameterIiv(target, pname, HostAddr(params, GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETTEXPARAMETERIUIV(target, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLuint tmp[MAX(size, 16)]; \
	fn.glGetTexParameterIuiv(target, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETTEXPARAMETERIUIV(target, pname, params) \
	fn.glGetTexParameterIuiv(target, pname, HostAddr(params, GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLCLEARBUFFERIV(buffer, drawbuffer, value) \
	GLuint size = 0; \
	GLint tmp[4], *ptmp; \
	switch (buffer) { \
		case GL_COLOR: size = 4; break; \
		case GL_STENCIL: \
		case GL_DEPTH: size = 1; break; \
		case GL_DEPTH_STENCIL: size = 2; break; \
	} \
	ptmp = Atari2HostIntArray(size, value, tmp); \
	fn.glClearBufferiv(buffer, drawbuffer, ptmp)
#else
#define FN_GLCLEARBUFFERIV(buffer, drawbuffer, value) \
	fn.glClearBufferiv(buffer, drawbuffer, HostAddr(value, const GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLCLEARBUFFERUIV(buffer, drawbuffer, value) \
	GLuint size = 0; \
	GLuint tmp[4], *ptmp; \
	switch (buffer) { \
		case GL_COLOR: size = 4; break; \
		case GL_STENCIL: \
		case GL_DEPTH: size = 1; break; \
		case GL_DEPTH_STENCIL: size = 2; break; \
	} \
	ptmp = Atari2HostIntArray(size, value, tmp); \
	fn.glClearBufferuiv(buffer, drawbuffer, ptmp)
#else
#define FN_GLCLEARBUFFERUIV(buffer, drawbuffer, value) \
	fn.glClearBufferuiv(buffer, drawbuffer, HostAddr(value, const GLuint *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLCLEARBUFFERFV(buffer, drawbuffer, value) \
	GLfloat tmp[4], *ptmp; \
	GLuint size = 0; \
	switch (buffer) { \
		case GL_COLOR: size = 4; break; \
		case GL_STENCIL: \
		case GL_DEPTH: size = 1; break; \
		case GL_DEPTH_STENCIL: size = 2; break; \
	} \
	ptmp = Atari2HostFloatArray(size, value, tmp); \
	fn.glClearBufferfv(buffer, drawbuffer, ptmp)
#else
#define FN_GLCLEARBUFFERFV(buffer, drawbuffer, value) \
	fn.glClearBufferfv(buffer, drawbuffer, HostAddr(value, const GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLDELETERENDERBUFFERS(n, renderbuffers) \
	GLuint tmp[MAX(n, 0)], *ptmp; \
	ptmp = Atari2HostIntArray(n, renderbuffers, tmp); \
	fn.glDeleteRenderbuffers(n, ptmp)
#else
#define FN_GLDELETERENDERBUFFERS(n, renderbuffers) \
	fn.glDeleteRenderbuffers(n, HostAddr(renderbuffers, const GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGENRENDERBUFFERS(n, renderbuffers) \
	GLsizei const size = MAX(n, 0); \
	GLuint tmp[size]; \
	fn.glGenRenderbuffers(n, tmp); \
	Host2AtariIntArray(n, tmp, renderbuffers)
#else
#define FN_GLGENRENDERBUFFERS(n, renderbuffers) \
	fn.glGenRenderbuffers(n, HostAddr(renderbuffers, GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETRENDERBUFFERPARAMETERIV(target, pname, params) \
	GLint const size = 1; \
	GLint tmp[size]; \
	fn.glGetRenderbufferParameteriv(target, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETRENDERBUFFERPARAMETERIV(target, pname, params) \
	fn.glGetRenderbufferParameteriv(target, pname, HostAddr(params, GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLDELETEFRAMEBUFFERS(n, framebuffers) \
	GLuint tmp[MAX(n, 0)], *ptmp; \
	ptmp = Atari2HostIntArray(n, framebuffers, tmp); \
	fn.glDeleteFramebuffers(n, ptmp)
#else
#define FN_GLDELETEFRAMEBUFFERS(n, framebuffers) \
	fn.glDeleteFramebuffers(n, HostAddr(framebuffers, const GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGENFRAMEBUFFERS(n, framebuffers) \
	GLsizei const size = MAX(n, 0); \
	GLuint tmp[size]; \
	fn.glGenFramebuffers(n, tmp); \
	Host2AtariIntArray(n, tmp, framebuffers)
#else
#define FN_GLGENFRAMEBUFFERS(n, framebuffers) \
	fn.glGenFramebuffers(n, HostAddr(framebuffers, GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETFRAMEBUFFERATTACHMENTPARAMETERIV(target, attachment, pname, params) \
	GLint const size = 1; \
	GLint tmp[MAX(size, 16)]; \
	fn.glGetFramebufferAttachmentParameteriv(target, attachment, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETFRAMEBUFFERATTACHMENTPARAMETERIV(target, attachment, pname, params) \
	fn.glGetFramebufferAttachmentParameteriv(target, attachment, pname, HostAddr(params, GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLDELETEVERTEXARRAYS(n, arrays) \
	GLuint tmp[MAX(n, 0)], *ptmp; \
	ptmp = Atari2HostIntArray(n, arrays, tmp); \
	fn.glDeleteVertexArrays(n, ptmp)
#else
#define FN_GLDELETEVERTEXARRAYS(n, arrays) \
	fn.glDeleteVertexArrays(n, HostAddr(arrays, const GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGENVERTEXARRAYS(n, arrays) \
	GLsizei const size = MAX(n, 0); \
	GLuint tmp[size]; \
	fn.glGenVertexArrays(n, tmp); \
	Host2AtariIntArray(n, tmp, arrays)
#else
#define FN_GLGENVERTEXARRAYS(n, arrays) \
	fn.glGenVertexArrays(n, HostAddr(arrays, GLuint *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * Version 3.1
 */

#define FN_GLDRAWARRAYSINSTANCED(mode, first, count, instancecount) \
	convertClientArrays(first + count); \
	fn.glDrawArraysInstanced(mode, first, count, instancecount)

#define FN_GLDRAWELEMENTSINSTANCED(mode, count, type, indices, instancecount) \
	void *tmp; \
	convertClientArrays(count); \
	pixelBuffer buf(*this); \
	switch(type) { \
	case GL_UNSIGNED_BYTE: \
	case GL_UNSIGNED_SHORT: \
	case GL_UNSIGNED_INT: \
		tmp = buf.convertArray(count, type, (nfmemptr)indices); \
		break; \
	default: \
		glSetError(GL_INVALID_ENUM); \
		return; \
	} \
	fn.glDrawElementsInstanced(mode, count, type, tmp, instancecount)

#define FN_GLGETUNIFORMINDICES(program, uniformCount, uniformNames, uniformIndices) \
	GLsizei const size = uniformCount; \
	if (size <= 0 || !uniformNames || !uniformIndices) return; \
	void *pstrings[size]; \
	GLuint indices[size]; \
	Atari2HostPtrArray(size, uniformNames, pstrings); \
	fn.glGetUniformIndices(program, size, (const GLchar *const *)pstrings, indices); \
	Host2AtariIntArray(size, indices, uniformIndices)

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETACTIVEUNIFORMSIV(program, uniformCount, uniformIndices, pname, params) \
	GLsizei const size = uniformCount; \
	if (size <= 0 || !uniformIndices) return; \
	GLuint tmp[size]; \
	GLint p[size]; \
	Atari2HostIntArray(uniformCount, uniformIndices, tmp); \
	fn.glGetActiveUniformsiv(program, uniformCount, tmp, pname, p); \
	Host2AtariIntArray(uniformCount, p, params)
#else
#define FN_GLGETACTIVEUNIFORMSIV(program, uniformCount, uniformIndices, pname, params) \
	fn.glGetActiveUniformsiv(program, uniformCount, HostAddr(uniformIndices, const GLuint *), pname, HostAddr(params, GLint *))
#endif

#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_INT_CONV
#define FN_GLGETACTIVEUNIFORMNAME(program, index, bufSize, length, name) \
	GLsizei l = 0; \
	GLchar namebuf[MAX(bufSize, 0)]; \
	fn.glGetActiveUniformName(program, index, bufSize, &l, namebuf); \
	Host2AtariByteArray(MIN(l + 1, bufSize), namebuf, name); \
	Host2AtariIntArray(1, &l, length)
#else
#define FN_GLGETACTIVEUNIFORMNAME(program, index, bufSize, length, name) \
	fn.glGetActiveUniformName(program, index, bufSize, HostAddr(length, GLsizei *), HostAddr(name, GLchar *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLGETUNIFORMBLOCKINDEX(program, uniformBlockName) \
	GLchar tmp[safe_strlen(uniformBlockName) + 1], *ptmp; \
	ptmp = Atari2HostByteArray(sizeof(tmp), uniformBlockName, tmp); \
	return fn.glGetUniformBlockIndex(program, ptmp)
#else
#define FN_GLGETUNIFORMBLOCKINDEX(program, uniformBlockName) \
	return fn.glGetUniformBlockIndex(program, HostAddr(uniformBlockName, const GLchar *))
#endif
	
#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETACTIVEUNIFORMBLOCKIV(program, uniformBlockIndex, pname, params) \
	GLint size = 1; \
	switch (pname) { \
		case GL_UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES: fn.glGetActiveUniformBlockiv(program, uniformBlockIndex, GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS, &size); break; \
	} \
	GLint tmp[size]; \
	fn.glGetActiveUniformBlockiv(program, uniformBlockIndex, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETACTIVEUNIFORMBLOCKIV(program, uniformBlockIndex, pname, params) \
	fn.glGetActiveUniformBlockiv(program, uniformBlockIndex, pname, HostAddr(params, GLint *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLGETACTIVEUNIFORMBLOCKINDEX(program, uniformBlockName) \
	GLchar namebuf[safe_strlen(uniformBlockName) + 1], *pname; \
	pname = Atari2HostByteArray(sizeof(namebuf), uniformBlockName, namebuf); \
	return fn.glGetActiveUniformBlockIndex(program, pname)
#else
#define FN_GLGETACTIVEUNIFORMBLOCKINDEX(program, uniformBlockName) \
	return fn.glGetActiveUniformBlockIndex(program, HostAddr(uniformBlockName, const GLchar *))
#endif

#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_INT_CONV
#define FN_GLGETACTIVEUNIFORMBLOCKNAME(program, index, bufSize, length, uniformBlockName) \
	GLsizei l = 0; \
	GLchar namebuf[MAX(bufSize, 0)]; \
	fn.glGetActiveUniformBlockName(program, index, bufSize, &l, namebuf); \
	Host2AtariIntArray(1, &l, length); \
	Host2AtariByteArray(MIN(l + 1, bufSize), namebuf, uniformBlockName)
#else
#define FN_GLGETACTIVEUNIFORMBLOCKNAME(program, index, bufSize, length, uniformBlockName) \
	fn.glGetActiveUniformBlockName(program, index, bufSize, HostAddr(length, GLsizei *), HostAddr(uniformBlockName, GLchar *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * Version 3.2
 */

#define FN_GLDRAWELEMENTSBASEVERTEX(mode, count, type, indices, basevertex) \
	void *tmp; \
	convertClientArrays(count); \
	pixelBuffer buf(*this); \
	switch(type) { \
	case GL_UNSIGNED_BYTE: \
	case GL_UNSIGNED_SHORT: \
	case GL_UNSIGNED_INT: \
		tmp = buf.convertArray(count, type, (nfmemptr)indices); \
		break; \
	default: \
		glSetError(GL_INVALID_ENUM); \
		return; \
	} \
	fn.glDrawElementsBaseVertex(mode, count, type, tmp, basevertex)

#define FN_GLDRAWRANGEELEMENTSBASEVERTEX(mode, start, end, count, type, indices, basevertex) \
	void *tmp; \
	convertClientArrays(count); \
	pixelBuffer buf(*this); \
	switch(type) { \
	case GL_UNSIGNED_BYTE: \
	case GL_UNSIGNED_SHORT: \
	case GL_UNSIGNED_INT: \
		tmp = buf.convertArray(count, type, (nfmemptr)indices); \
		break; \
	default: \
		glSetError(GL_INVALID_ENUM); \
		return; \
	} \
	fn.glDrawRangeElementsBaseVertex(mode, start, end, count, type, tmp, basevertex)

#define FN_GLMULTIDRAWELEMENTSBASEVERTEX(mode, count, type, indices, drawcount, basevertex) \
	if (drawcount <= 0 || !indices || !basevertex) return; \
	nfmemptr tmpind[drawcount]; \
	GLsizei tmpcount[drawcount]; \
	GLint tmpbase[drawcount]; \
	Atari2HostIntArray(drawcount, count, tmpcount); \
	Atari2HostMemPtrArray(drawcount, indices, tmpind); \
	Atari2HostIntArray(drawcount, basevertex, tmpbase); \
	for (GLsizei i = 0; i < drawcount; i++) { \
		nfglDrawElementsBaseVertex(mode, tmpcount[i], type, tmpind[i], tmpbase[i]); \
	}
	
#define FN_GLDRAWELEMENTSINSTANCEDBASEVERTEX(mode, count, type, indices, instancecount, basevertex) \
	void *tmp; \
	convertClientArrays(count); \
	pixelBuffer buf(*this); \
	switch(type) { \
	case GL_UNSIGNED_BYTE: \
	case GL_UNSIGNED_SHORT: \
	case GL_UNSIGNED_INT: \
		tmp = buf.convertArray(count, type, (nfmemptr)indices); \
		break; \
	default: \
		glSetError(GL_INVALID_ENUM); \
		return; \
	} \
	fn.glDrawElementsInstancedBaseVertex(mode, count, type, tmp, instancecount, basevertex)

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETINTEGER64V(pname, data) \
	GLint const size = nfglGetNumParams(pname); \
	GLint64 tmp[MAX(size, 16)]; \
	fn.glGetInteger64v(pname, tmp); \
	Host2AtariInt64Array(size, tmp, data)
#else
#define FN_GLGETINTEGER64V(pname, data) \
	fn.glGetInteger64v(pname, HostAddr(data, GLint64 *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETINTEGER64I_V(target, index, data) \
	GLint const size = nfglGetNumParams(target); \
	GLint64 tmp[MAX(size, 16)]; \
	fn.glGetInteger64i_v(target, index, tmp); \
	Host2AtariInt64Array(size, tmp, data)
#else
#define FN_GLGETINTEGER64I_V(target, index, data) \
	fn.glGetInteger64i_v(target, index, HostAddr(data, GLint64 *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETBUFFERPARAMETERI64V(target, pname, params) \
	int const size = nfglGetNumParams(pname); \
	GLint64 tmp[MAX(size, 16)]; \
	fn.glGetBufferParameteri64v(target, pname, tmp); \
	Host2AtariInt64Array(size, tmp, params)
#else
#define FN_GLGETBUFFERPARAMETERI64V(target, pname, params) \
	fn.glGetBufferParameteri64v(target, pname, HostAddr(params, GLint64 *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETMULTISAMPLEFV(pname, index, val) \
	GLint const size = 2; \
	GLfloat tmp[MAX(size, 16)]; \
	fn.glGetMultisamplefv(pname, index, tmp); \
	Host2AtariFloatArray(size, tmp, val)
#else
#define FN_GLGETMULTISAMPLEFV(pname, index, val) \
	fn.glGetMultisamplefv(pname, index, HostAddr(val, GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETSYNCIV(sync, pname, bufSize, length, values) \
	int const size = bufSize; \
	if (size <= 0) return; \
	GLint tmp[size]; \
	GLint l = 0; \
	fn.glGetSynciv(sync, pname, bufSize, &l, tmp); \
	Host2AtariIntArray(1, &l, length); \
	Host2AtariIntArray(l, tmp, values)
#else
#define FN_GLGETSYNCIV(sync, pname, bufSize, length, values) \
	fn.glGetSynciv(sync, pname, bufSize, HostAddr(length, GLint *), HostAddr(values, GLint *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * Version 3.3
 */

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLBINDFRAGDATALOCATIONINDEXED(program, color, index, name) \
	GLchar tmp[safe_strlen(name) + 1], *ptmp; \
	ptmp = Atari2HostByteArray(sizeof(tmp), name, tmp); \
	fn.glBindFragDataLocationIndexed(program, color, index, ptmp)
#else
#define FN_GLBINDFRAGDATALOCATIONINDEXED(program, color, index, name) \
	fn.glBindFragDataLocationIndexed(program, color, index, HostAddr(name, const GLchar *))
#endif
	
#if NFOSMESA_NEED_INT_CONV
#define FN_GLGENSAMPLERS(count, samplers) \
	GLsizei const size = MAX(count, 0); \
	GLuint tmp[size]; \
	fn.glGenSamplers(count, tmp); \
	Host2AtariIntArray(count, tmp, samplers)
#else
#define FN_GLGENSAMPLERS(count, samplers) \
	fn.glGenSamplers(count, HostAddr(samplers, GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLDELETESAMPLERS(n, samplers) \
	GLsizei const size = n; \
	GLuint tmp[MAX(size, 0)], *ptmp; \
	ptmp = Atari2HostIntArray(size, samplers, tmp); \
	fn.glDeleteSamplers(n, ptmp)
#else
#define FN_GLDELETESAMPLERS(n, samplers) \
	fn.glDeleteSamplers(n, HostAddr(samplers, const GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLSAMPLERPARAMETERIV(sampler, pname, params) \
	GLsizei const size = nfglGetNumParams(pname); \
	GLint tmp[size], *ptmp; \
	ptmp = Atari2HostIntArray(size, params, tmp); \
	fn.glSamplerParameteriv(sampler, pname, ptmp)
#else
#define FN_GLSAMPLERPARAMETERIV(sampler, pname, params) \
	fn.glSamplerParameteriv(sampler, pname, HostAddr(params, const GLint *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLSAMPLERPARAMETERFV(sampler, pname, params) \
	GLsizei const size = nfglGetNumParams(pname); \
	GLfloat tmp[size], *ptmp; \
	ptmp = Atari2HostFloatArray(size, params, tmp); \
	fn.glSamplerParameterfv(sampler, pname, ptmp)
#else
#define FN_GLSAMPLERPARAMETERFV(sampler, pname, params) \
	fn.glSamplerParameterfv(sampler, pname, HostAddr(params, const GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLSAMPLERPARAMETERIIV(sampler, pname, params) \
	GLsizei const size = nfglGetNumParams(pname); \
	GLint tmp[size], *ptmp; \
	ptmp = Atari2HostIntArray(size, params, tmp); \
	fn.glSamplerParameterIiv(sampler, pname, ptmp)
#else
#define FN_GLSAMPLERPARAMETERIIV(sampler, pname, params) \
	fn.glSamplerParameterIiv(sampler, pname, HostAddr(params, const GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLSAMPLERPARAMETERIUIV(sampler, pname, params) \
	GLsizei const size = nfglGetNumParams(pname); \
	GLuint tmp[size], *ptmp; \
	ptmp = Atari2HostIntArray(size, params, tmp); \
	fn.glSamplerParameterIuiv(sampler, pname, ptmp)
#else
#define FN_GLSAMPLERPARAMETERIUIV(sampler, pname, params) \
	fn.glSamplerParameterIuiv(sampler, pname, HostAddr(params, const GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETSAMPLERPARAMETERIV(sampler, pname, params) \
	GLsizei const size = nfglGetNumParams(pname); \
	GLint tmp[size]; \
	fn.glGetSamplerParameteriv(sampler, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETSAMPLERPARAMETERIV(sampler, pname, params) \
	fn.glGetSamplerParameteriv(sampler, pname, HostAddr(params, GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETSAMPLERPARAMETERIIV(sampler, pname, params) \
	GLsizei const size = nfglGetNumParams(pname); \
	GLint tmp[size]; \
	fn.glGetSamplerParameterIiv(sampler, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETSAMPLERPARAMETERIIV(sampler, pname, params) \
	fn.glGetSamplerParameterIiv(sampler, pname, HostAddr(params, GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETSAMPLERPARAMETERIUIV(sampler, pname, params) \
	GLsizei const size = nfglGetNumParams(pname); \
	GLuint tmp[size]; \
	fn.glGetSamplerParameterIuiv(sampler, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETSAMPLERPARAMETERIUIV(sampler, pname, params) \
	fn.glGetSamplerParameterIuiv(sampler, pname, HostAddr(params, GLuint *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETSAMPLERPARAMETERFV(sampler, pname, params) \
	GLsizei const size = nfglGetNumParams(pname); \
	GLfloat tmp[size]; \
	fn.glGetSamplerParameterfv(sampler, pname, tmp); \
	Host2AtariFloatArray(size, tmp, params)
#else
#define FN_GLGETSAMPLERPARAMETERFV(sampler, pname, params) \
	fn.glGetSamplerParameterfv(sampler, pname, HostAddr(params, GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETQUERYOBJECTI64V(id, pname, params) \
	GLint const size = 1; \
	GLint64 tmp[size]; \
	fn.glGetQueryObjecti64v(id, pname, tmp); \
	Host2AtariInt64Array(size, tmp, params)
#else
#define FN_GLGETQUERYOBJECTI64V(id, pname, params) \
	fn.glGetQueryObjecti64v(id, pname, HostAddr(params, GLint64 *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETQUERYOBJECTUI64V(id, pname, params) \
	GLint const size = 1; \
	GLuint64 tmp[size]; \
	fn.glGetQueryObjectui64v(id, pname, tmp); \
	Host2AtariInt64Array(size, tmp, params)
#else
#define FN_GLGETQUERYOBJECTUI64V(id, pname, params) \
	fn.glGetQueryObjectui64v(id, pname, HostAddr(params, GLuint64 *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIBP1UIV(index, type, normalized, value) \
	GLint const size = 1; \
	GLuint tmp[size]; \
	Atari2HostIntArray(size, value, tmp); \
	fn.glVertexAttribP1uiv(index, type, normalized, tmp)
#else
#define FN_GLVERTEXATTRIBP1UIV(index, type, normalized, value) \
	fn.glVertexAttribP1uiv(index, type, normalized, HostAddr(value, const GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIBP2UIV(index, type, normalized, value) \
	GLint const size = 1; \
	GLuint tmp[size]; \
	Atari2HostIntArray(size, value, tmp); \
	fn.glVertexAttribP2uiv(index, type, normalized, tmp)
#else
#define FN_GLVERTEXATTRIBP2UIV(index, type, normalized, value) \
	fn.glVertexAttribP2uiv(index, type, normalized, HostAddr(value, const GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIBP3UIV(index, type, normalized, value) \
	GLint const size = 1; \
	GLuint tmp[size]; \
	Atari2HostIntArray(size, value, tmp); \
	fn.glVertexAttribP3uiv(index, type, normalized, tmp)
#else
#define FN_GLVERTEXATTRIBP3UIV(index, type, normalized, value) \
	fn.glVertexAttribP3uiv(index, type, normalized, HostAddr(value, const GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIBP4UIV(index, type, normalized, value) \
	GLint const size = 1; \
	GLuint tmp[size]; \
	Atari2HostIntArray(size, value, tmp); \
	fn.glVertexAttribP4uiv(index, type, normalized, tmp)
#else
#define FN_GLVERTEXATTRIBP4UIV(index, type, normalized, value) \
	fn.glVertexAttribP4uiv(index, type, normalized, HostAddr(value, const GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXP2UIV(type, value) \
	GLint const size = 1; \
	GLuint tmp[size]; \
	Atari2HostIntArray(size, value, tmp); \
	fn.glVertexP2uiv(type, tmp)
#else
#define FN_GLVERTEXP2UIV(type, value) \
	fn.glVertexP2uiv(type, HostAddr(value, const GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXP3UIV(type, value) \
	GLint const size = 1; \
	GLuint tmp[size]; \
	Atari2HostIntArray(size, value, tmp); \
	fn.glVertexP3uiv(type, tmp)
#else
#define FN_GLVERTEXP3UIV(type, value) \
	fn.glVertexP3uiv(type, HostAddr(value, const GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXP4UIV(type, value) \
	GLint const size = 1; \
	GLuint tmp[size]; \
	Atari2HostIntArray(size, value, tmp); \
	fn.glVertexP4uiv(type, tmp)
#else
#define FN_GLVERTEXP4UIV(type, value) \
	fn.glVertexP4uiv(type, HostAddr(value, const GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLTEXCOORDP1UIV(type, coords) \
	GLint const size = 1; \
	GLuint tmp[size]; \
	Atari2HostIntArray(size, coords, tmp); \
	fn.glTexCoordP1uiv(type, tmp)
#else
#define FN_GLTEXCOORDP1UIV(type, coords) \
	fn.glTexCoordP1uiv(type, HostAddr(coords, const GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLTEXCOORDP2UIV(type, coords) \
	GLint const size = 1; \
	GLuint tmp[size]; \
	Atari2HostIntArray(size, coords, tmp); \
	fn.glTexCoordP2uiv(type, tmp)
#else
#define FN_GLTEXCOORDP2UIV(type, coords) \
	fn.glTexCoordP2uiv(type, HostAddr(coords, const GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLTEXCOORDP3UIV(type, coords) \
	GLint const size = 3; \
	GLuint tmp[size]; \
	Atari2HostIntArray(size, coords, tmp); \
	fn.glTexCoordP3uiv(type, tmp)
#else
#define FN_GLTEXCOORDP3UIV(type, coords) \
	fn.glTexCoordP3uiv(type, HostAddr(coords, const GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLTEXCOORDP4UIV(type, coords) \
	GLint const size = 1; \
	GLuint tmp[size]; \
	Atari2HostIntArray(size, coords, tmp); \
	fn.glTexCoordP4uiv(type, tmp)
#else
#define FN_GLTEXCOORDP4UIV(type, coords) \
	fn.glTexCoordP4uiv(type, HostAddr(coords, const GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLMULTITEXCOORDP1UIV(texture, type, coords) \
	GLint const size = 1; \
	GLuint tmp[size]; \
	Atari2HostIntArray(size, coords, tmp); \
	fn.glMultiTexCoordP1uiv(texture, type, tmp)
#else
#define FN_GLMULTITEXCOORDP1UIV(texture, type, coords) \
	fn.glMultiTexCoordP1uiv(texture, type, HostAddr(coords, const GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLMULTITEXCOORDP2UIV(texture, type, coords) \
	GLint const size = 1; \
	GLuint tmp[size]; \
	Atari2HostIntArray(size, coords, tmp); \
	fn.glMultiTexCoordP2uiv(texture, type, tmp)
#else
#define FN_GLMULTITEXCOORDP2UIV(texture, type, coords) \
	fn.glMultiTexCoordP2uiv(texture, type, HostAddr(coords, const GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLMULTITEXCOORDP3UIV(texture, type, coords) \
	GLint const size = 1; \
	GLuint tmp[size]; \
	Atari2HostIntArray(size, coords, tmp); \
	fn.glMultiTexCoordP3uiv(texture, type, tmp)
#else
#define FN_GLMULTITEXCOORDP3UIV(texture, type, coords) \
	fn.glMultiTexCoordP3uiv(texture, type, HostAddr(coords, const GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLMULTITEXCOORDP4UIV(texture, type, coords) \
	GLint const size = 1; \
	GLuint tmp[size]; \
	Atari2HostIntArray(size, coords, tmp); \
	fn.glMultiTexCoordP4uiv(texture, type, tmp)
#else
#define FN_GLMULTITEXCOORDP4UIV(texture, type, coords) \
	fn.glMultiTexCoordP4uiv(texture, type, HostAddr(coords, const GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLNORMALP3UIV(type, coords) \
	GLint const size = 1; \
	GLuint tmp[size]; \
	Atari2HostIntArray(size, coords, tmp); \
	fn.glNormalP3uiv(type, tmp)
#else
#define FN_GLNORMALP3UIV(type, coords) \
	fn.glNormalP3uiv(type, HostAddr(coords, const GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLCOLORP3UIV(type, color) \
	GLint const size = 1; \
	GLuint tmp[size]; \
	Atari2HostIntArray(size, color, tmp); \
	fn.glColorP3uiv(type, tmp)
#else
#define FN_GLCOLORP3UIV(type, color) \
	fn.glColorP3uiv(type, HostAddr(color, const GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLCOLORP4UIV(type, color) \
	GLint const size = 1; \
	GLuint tmp[size]; \
	Atari2HostIntArray(size, color, tmp); \
	fn.glColorP4uiv(type, tmp)
#else
#define FN_GLCOLORP4UIV(type, color) \
	fn.glColorP4uiv(type, HostAddr(color, const GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLSECONDARYCOLORP3UIV(type, color) \
	GLint const size = 1; \
	GLuint tmp[size]; \
	Atari2HostIntArray(size, color, tmp); \
	fn.glSecondaryColorP3uiv(type, tmp)
#else
#define FN_GLSECONDARYCOLORP3UIV(type, color) \
	fn.glSecondaryColorP3uiv(type, HostAddr(color, const GLuint *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * Version 4.0
 */

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
		void *offset = NFHost2AtariAddr(indirect); \
		fn.glDrawArraysIndirect(mode, offset); \
	} else if (indirect) { \
		GLuint tmp[4]; \
		Atari2HostIntArray(4, AtariAddr(indirect, const GLuint *), tmp); \
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
 *    } DrawElementsIndirectCommand;
 */
#define FN_GLDRAWELEMENTSINDIRECT(mode, type, indirect) \
	if (contexts[cur_context].buffer_bindings.draw_indirect.id) { \
		void *offset = NFHost2AtariAddr(indirect); \
		fn.glDrawElementsIndirect(mode, type, offset); \
	} else if (indirect) { \
		GLuint tmp[5]; \
		Atari2HostIntArray(5, AtariAddr(indirect, const GLuint *), tmp); \
		GLuint count = tmp[0]; \
		convertClientArrays(count); \
		fn.glDrawElementsInstancedBaseVertexBaseInstance(mode, count, type, (const void *)(uintptr_t)tmp[2], tmp[1], tmp[3], tmp[4]); \
	}

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETACTIVESUBROUTINEUNIFORMIV(program, shadertype, index, pname, values) \
	GLint size = 1; \
	switch (pname) { \
		case GL_COMPATIBLE_SUBROUTINES: fn.glGetActiveSubroutineUniformiv(program, shadertype, index, GL_NUM_COMPATIBLE_SUBROUTINES, &size); break; \
	} \
	GLint tmp[size]; \
	fn.glGetActiveSubroutineUniformiv(program, shadertype, index, pname, tmp); \
	Host2AtariIntArray(size, tmp, values)
#else
#define FN_GLGETACTIVESUBROUTINEUNIFORMIV(program, shadertype, index, pname, values) \
	fn.glGetActiveSubroutineUniformiv(program, shadertype, index, pname, HostAddr(values, GLint *))
#endif

#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_INT_CONV
#define FN_GLGETACTIVESUBROUTINEUNIFORMNAME(program, shadertype, index, bufSize, length, name) \
	GLsizei l = 0; \
	GLchar tmp[MAX(bufSize, 0)]; \
	fn.glGetActiveSubroutineUniformName(program, shadertype, index, bufSize, &l, tmp); \
	Host2AtariByteArray(MIN(l + 1, bufSize), tmp, name); \
	Host2AtariIntArray(1, &l, length)
#else
#define FN_GLGETACTIVESUBROUTINEUNIFORMNAME(program, shadertype, index, bufSize, length, name) \
	fn.glGetActiveSubroutineUniformName(program, shadertype, index, bufSize, HostAddr(length, GLsizei *), HostAddr(name, GLchar *))
#endif

#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_INT_CONV
#define FN_GLGETACTIVESUBROUTINENAME(program, shadertype, index, bufSize, length, name) \
	GLsizei l = 0; \
	GLchar tmp[MAX(bufSize, 0)]; \
	fn.glGetActiveSubroutineName(program, shadertype, index, bufSize, &l, tmp); \
	Host2AtariByteArray(MIN(l + 1, bufSize), tmp, name); \
	Host2AtariIntArray(1, &l, length)
#else
#define FN_GLGETACTIVESUBROUTINENAME(program, shadertype, index, bufSize, length, name) \
	fn.glGetActiveSubroutineName(program, shadertype, index, bufSize, HostAddr(length, GLsizei *), HostAddr(name, GLchar *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLDELETETRANSFORMFEEDBACKS(n, ids) \
	GLsizei const size = n; \
	if (size <= 0 || !ids) return; \
	GLuint tmp[size]; \
	Atari2HostIntArray(size, ids, tmp); \
	fn.glDeleteTransformFeedbacks(size, tmp)
#else
#define FN_GLDELETETRANSFORMFEEDBACKS(n, ids) \
	fn.glDeleteTransformFeedbacks(n, HostAddr(ids, const GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGENTRANSFORMFEEDBACKS(n, ids) \
	GLsizei const size = MAX(n, 0); \
	GLuint tmp[size]; \
	fn.glGenTransformFeedbacks(n, tmp); \
	Host2AtariIntArray(n, tmp, ids)
#else
#define FN_GLGENTRANSFORMFEEDBACKS(n, ids) \
	fn.glGenTransformFeedbacks(n, HostAddr(ids, GLuint *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLUNIFORM1DV(location, count, value) \
	GLint const size = 1 * count; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, value, tmp); \
	fn.glUniform1dv(location, count, tmp)
#else
#define FN_GLUNIFORM1DV(location, count, value) \
	fn.glUniform1dv(location, count, HostAddr(value, GLdouble *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLUNIFORM2DV(location, count, value) \
	GLint const size = 2 * count; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, value, tmp); \
	fn.glUniform2dv(location, count, tmp)
#else
#define FN_GLUNIFORM2DV(location, count, value) \
	fn.glUniform2dv(location, count, HostAddr(value, const GLdouble *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLUNIFORM3DV(location, count, value) \
	GLint const size = 3 * count; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, value, tmp); \
	fn.glUniform3dv(location, count, tmp)
#else
#define FN_GLUNIFORM3DV(location, count, value) \
	fn.glUniform3dv(location, count, HostAddr(value, const GLdouble *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLUNIFORM4DV(location, count, value) \
	GLint const size = 4 * count; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, value, tmp); \
	fn.glUniform4dv(location, count, tmp)
#else
#define FN_GLUNIFORM4DV(location, count, value) \
	fn.glUniform4dv(location, count, HostAddr(value, const GLdouble *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLUNIFORMMATRIX2DV(location, count, transpose, value) \
	GLint const size = 4 * count; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, value, tmp); \
	fn.glUniformMatrix2dv(location, count, transpose, tmp)
#else
#define FN_GLUNIFORMMATRIX2DV(location, count, transpose, value) \
	fn.glUniformMatrix2dv(location, count, transpose, HostAddr(value, const GLdouble *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLUNIFORMMATRIX3DV(location, count, transpose, value) \
	GLint const size = 9 * count; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, value, tmp); \
	fn.glUniformMatrix3dv(location, count, transpose, tmp)
#else
#define FN_GLUNIFORMMATRIX3DV(location, count, transpose, value) \
	fn.glUniformMatrix3dv(location, count, transpose, HostAddr(value, const GLdouble *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLUNIFORMMATRIX4DV(location, count, transpose, value) \
	GLint const size = 16 * count; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, value, tmp); \
	fn.glUniformMatrix4dv(location, count, transpose, tmp)
#else
#define FN_GLUNIFORMMATRIX4DV(location, count, transpose, value) \
	fn.glUniformMatrix4dv(location, count, transpose, HostAddr(value, const GLdouble *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLUNIFORMMATRIX2X3DV(location, count, transpose, value) \
	GLint const size = 6 * count; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, value, tmp); \
	fn.glUniformMatrix2x3dv(location, count, transpose, tmp)
#else
#define FN_GLUNIFORMMATRIX2X3DV(location, count, transpose, value) \
	fn.glUniformMatrix2x3dv(location, count, transpose, HostAddr(value, const GLdouble *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLUNIFORMMATRIX2X4DV(location, count, transpose, value) \
	GLint const size = 8 * count; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, value, tmp); \
	fn.glUniformMatrix2x4dv(location, count, transpose, tmp)
#else
#define FN_GLUNIFORMMATRIX2X4DV(location, count, transpose, value) \
	fn.glUniformMatrix2x4dv(location, count, transpose, HostAddr(value, const GLdouble *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLUNIFORMMATRIX3X2DV(location, count, transpose, value) \
	GLint const size = 6 * count; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, value, tmp); \
	fn.glUniformMatrix3x2dv(location, count, transpose, tmp)
#else
#define FN_GLUNIFORMMATRIX3X2DV(location, count, transpose, value) \
	fn.glUniformMatrix3x2dv(location, count, transpose, HostAddr(value, const GLdouble *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLUNIFORMMATRIX3X4DV(location, count, transpose, value) \
	GLint const size = 12 * count; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, value, tmp); \
	fn.glUniformMatrix3x4dv(location, count, transpose, tmp)
#else
#define FN_GLUNIFORMMATRIX3X4DV(location, count, transpose, value) \
	fn.glUniformMatrix3x4dv(location, count, transpose, HostAddr(value, const GLdouble *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLUNIFORMMATRIX4X2DV(location, count, transpose, value) \
	GLint const size = 8 * count; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, value, tmp); \
	fn.glUniformMatrix4x2dv(location, count, transpose, tmp)
#else
#define FN_GLUNIFORMMATRIX4X2DV(location, count, transpose, value) \
	fn.glUniformMatrix4x2dv(location, count, transpose, HostAddr(value, const GLdouble *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLUNIFORMMATRIX4X3DV(location, count, transpose, value) \
	GLint const size = 12 * count; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, value, tmp); \
	fn.glUniformMatrix4x3dv(location, count, transpose, tmp)
#else
#define FN_GLUNIFORMMATRIX4X3DV(location, count, transpose, value) \
	fn.glUniformMatrix4x3dv(location, count, transpose, HostAddr(value, const GLdouble *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLGETSUBROUTINEUNIFORMLOCATION(program, shadertype, name) \
	GLchar tmp[safe_strlen(name) + 1], *ptmp; \
	ptmp = Atari2HostByteArray(sizeof(tmp), name, tmp); \
	return fn.glGetSubroutineUniformLocation(program, shadertype, ptmp)
#else
#define FN_GLGETSUBROUTINEUNIFORMLOCATION(program, shadertype, name) \
	return fn.glGetSubroutineUniformLocation(program, shadertype, HostAddr(name, const GLchar *))
#endif
	
#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLGETSUBROUTINEINDEX(program, shadertype, name) \
	GLchar tmp[safe_strlen(name) + 1], *ptmp; \
	ptmp = Atari2HostByteArray(sizeof(tmp), name, tmp); \
	return fn.glGetSubroutineIndex(program, shadertype, ptmp)
#else
#define FN_GLGETSUBROUTINEINDEX(program, shadertype, name) \
	return fn.glGetSubroutineIndex(program, shadertype, HostAddr(name, const GLchar *))
#endif
	
#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLGETUNIFORMDV(program, location, params) \
	GLint const size = 1; \
	GLdouble tmp[size]; \
	fn.glGetUniformdv(program, location, tmp); \
	Host2AtariDoubleArray(size, tmp, params)
#else
#define FN_GLGETUNIFORMDV(program, location, params) \
	fn.glGetUniformdv(program, location, HostAddr(params, GLdouble *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLUNIFORMSUBROUTINESUIV(shadertype, count, indices) \
	GLint const size = count; \
	if (size <= 0 || !indices) return; \
	GLuint tmp[size]; \
	Atari2HostIntArray(size, indices, tmp); \
	fn.glUniformSubroutinesuiv(shadertype, count, tmp)
#else
#define FN_GLUNIFORMSUBROUTINESUIV(shadertype, count, indices) \
	fn.glUniformSubroutinesuiv(shadertype, count, HostAddr(indices, const GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETUNIFORMSUBROUTINEUIV(shadertype, location, params) \
	GLint program = 0; \
	fn.glGetIntegerv(GL_ACTIVE_PROGRAM, &program); \
	GLint size = 0; \
	fn.glGetProgramStageiv(program, shadertype, GL_ACTIVE_SUBROUTINE_UNIFORM_LOCATIONS, &size); \
	if (size <= 0 || !params) return; \
	GLuint tmp[size]; \
	fn.glGetUniformSubroutineuiv(shadertype, location, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETUNIFORMSUBROUTINEUIV(shadertype, location, params) \
	fn.glGetUniformSubroutineuiv(shadertype, location, HostAddr(params, GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETPROGRAMSTAGEIV(program, shadertype, pname, params) \
	GLint const size = 1; \
	GLint tmp[size]; \
	fn.glGetProgramStageiv(program, shadertype, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETPROGRAMSTAGEIV(program, shadertype, pname, params) \
	fn.glGetProgramStageiv(program, shadertype, pname, HostAddr(params, GLint *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPATCHPARAMETERFV(pname, values) \
	GLint size = 0; \
	fn.glGetIntegerv(GL_PATCH_VERTICES, &size); \
	if (size <= 0 || !values) return; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, values, tmp); \
	fn.glPatchParameterfv(pname, tmp)
#else
#define FN_GLPATCHPARAMETERFV(pname, values) \
	fn.glPatchParameterfv(pname, HostAddr(values, GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETQUERYINDEXEDIV(target, index, pname, params) \
	GLint const size = 1; \
	GLint tmp[size]; \
	fn.glGetQueryIndexediv(target, index, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETQUERYINDEXEDIV(target, index, pname, params) \
	fn.glGetQueryIndexediv(target, index, pname, HostAddr(params, GLint *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * Version 4.1
 */

#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_INT_CONV
#define FN_GLSHADERBINARY(count, shaders, binaryformat, binary, length) \
	GLuint tmp[MAX(count, 0)], *ptmp; \
	GLubyte binbuf[MAX(length, 0)], *pbin; \
	ptmp = Atari2HostIntArray(count, shaders, tmp); \
	pbin = Atari2HostByteArray(length, binary, binbuf); \
	fn.glShaderBinary(count, ptmp, binaryformat, pbin, length)
#else
#define FN_GLSHADERBINARY(count, shaders, binaryformat, binary, length) \
	fn.glShaderBinary(count, HostAddr(shaders, const GLuint *), binaryformat, HostAddr(binary, const void *), length)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETSHADERPRECISIONFORMAT(shadertype, precisiontype, range, precision) \
	GLint const size = 1; \
	GLint r = 0, p = 0; \
	fn.glGetShaderPrecisionFormat(shadertype, precisiontype, &r, &p); \
	Host2AtariIntArray(size, &r, range); \
	Host2AtariIntArray(size, &p, precision)
#else
#define FN_GLGETSHADERPRECISIONFORMAT(shadertype, precisiontype, range, precision) \
	fn.glGetShaderPrecisionFormat(shadertype, precisiontype, HostAddr(range, GLint *), HostAddr(precision, GLint *))
#endif

#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_INT_CONV
#define FN_GLGETPROGRAMBINARY(program, bufSize, length, binaryFormat, binary) \
	GLint const size = 1; \
	GLsizei l = 0; \
	GLenum format = 0; \
	GLubyte binbuf[MAX(bufSize, 0)]; \
	fn.glGetProgramBinary(program, bufSize, &l, &format, binbuf); \
	Host2AtariByteArray(l, binbuf, binary); \
	Host2AtariIntArray(size, &l, length); \
	Host2AtariIntArray(size, &format, binaryFormat)
#else
#define FN_GLGETPROGRAMBINARY(program, bufSize, length, binaryFormat, binary) \
	fn.glGetProgramBinary(program, bufSize, HostAddr(length, GLsizei *), HostAddr(binaryFormat, GLenum *), HostAddr(binary, void *))
#endif

#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_INT_CONV
#define FN_GLPROGRAMBINARY(program, binaryformat, binary, length) \
	GLubyte binbuf[MAX(length, 0)], *pbin; \
	pbin = Atari2HostByteArray(length, binary, binbuf); \
	fn.glProgramBinary(program, binaryformat, pbin, length)
#else
#define FN_GLPROGRAMBINARY(program, binaryformat, binary, length) \
	fn.glProgramBinary(program, binaryformat, HostAddr(binary, const void *), length)
#endif

#define FN_GLCREATESHADERPROGRAMV(type, count, strings) \
	void *pstrings[MAX(count, 0)], **ppstrings; \
	ppstrings = Atari2HostPtrArray(count, strings, pstrings); \
	return fn.glCreateShaderProgramv(type, count, (const GLchar *const *)ppstrings)

#if NFOSMESA_NEED_INT_CONV
#define FN_GLDELETEPROGRAMPIPELINES(n, pipelines) \
	GLuint tmp[MAX(n, 0)], *ptmp; \
	ptmp = Atari2HostIntArray(n, pipelines, tmp); \
	fn.glDeleteProgramPipelines(n, ptmp)
#else
#define FN_GLDELETEPROGRAMPIPELINES(n, pipelines) \
	fn.glDeleteProgramPipelines(n, HostAddr(pipelines, const GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGENPROGRAMPIPELINES(n, pipelines) \
	GLuint tmp[MAX(n, 0)]; \
	fn.glGenProgramPipelines(n, tmp); \
	Host2AtariIntArray(n, tmp, pipelines)
#else
#define FN_GLGENPROGRAMPIPELINES(n, pipelines) \
	fn.glGenProgramPipelines(n, HostAddr(pipelines, GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETPROGRAMPIPELINEIV(pipeline, pname, params) \
	GLsizei const size = nfglGetNumParams(pname); \
	GLint tmp[size]; \
	fn.glGetProgramPipelineiv(pipeline, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETPROGRAMPIPELINEIV(pipeline, pname, params) \
	fn.glGetProgramPipelineiv(pipeline, pname, HostAddr(params, GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLPROGRAMUNIFORM1IV(program, location, count, value) \
	GLint const size = 1 * count; \
	if (size <= 0) return; \
	GLint tmp[size]; \
	Atari2HostIntArray(size, value, tmp); \
	fn.glProgramUniform1iv(program, location, count, tmp)
#else
#define FN_GLPROGRAMUNIFORM1IV(program, location, count, value) \
	fn.glProgramUniform1iv(program, location, count, HostAddr(value, const GLint *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPROGRAMUNIFORM1FV(program, location, count, value) \
	GLint const size = 1 * count; \
	if (size <= 0) return; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, value, tmp); \
	fn.glProgramUniform1fv(program, location, count, tmp)
#else
#define FN_GLPROGRAMUNIFORM1FV(program, location, count, value) \
	fn.glProgramUniform1fv(program, location, count, HostAddr(value, const GLfloat *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLPROGRAMUNIFORM1DV(program, location, count, value) \
	GLint const size = 1 * count; \
	if (size <= 0) return; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, value, tmp); \
	fn.glProgramUniform1dv(program, location, count, tmp)
#else
#define FN_GLPROGRAMUNIFORM1DV(program, location, count, value) \
	fn.glProgramUniform1dv(program, location, count, HostAddr(value, const GLdouble *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLPROGRAMUNIFORM1UIV(program, location, count, value) \
	GLint const size = 1 * count; \
	if (size <= 0) return; \
	GLuint tmp[size]; \
	Atari2HostIntArray(size, value, tmp); \
	fn.glProgramUniform1uiv(program, location, count, tmp)
#else
#define FN_GLPROGRAMUNIFORM1UIV(program, location, count, value) \
	fn.glProgramUniform1uiv(program, location, count, HostAddr(value, const GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLPROGRAMUNIFORM2IV(program, location, count, value) \
	GLint const size = 2 * count; \
	if (size <= 0) return; \
	GLint tmp[size]; \
	Atari2HostIntArray(size, value, tmp); \
	fn.glProgramUniform2iv(program, location, count, tmp)
#else
#define FN_GLPROGRAMUNIFORM2IV(program, location, count, value) \
	fn.glProgramUniform2iv(program, location, count, HostAddr(value, const GLint *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPROGRAMUNIFORM2FV(program, location, count, value) \
	GLint const size = 2 * count; \
	if (size <= 0) return; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, value, tmp); \
	fn.glProgramUniform2fv(program, location, count, tmp)
#else
#define FN_GLPROGRAMUNIFORM2FV(program, location, count, value) \
	fn.glProgramUniform2fv(program, location, count, HostAddr(value, const GLfloat *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLPROGRAMUNIFORM2DV(program, location, count, value) \
	GLint const size = 2 * count; \
	if (size <= 0) return; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, value, tmp); \
	fn.glProgramUniform2dv(program, location, count, tmp)
#else
#define FN_GLPROGRAMUNIFORM2DV(program, location, count, value) \
	fn.glProgramUniform2dv(program, location, count, HostAddr(value, const GLdouble *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLPROGRAMUNIFORM2UIV(program, location, count, value) \
	GLint const size = 2 * count; \
	if (size <= 0) return; \
	GLuint tmp[size]; \
	Atari2HostIntArray(size, value, tmp); \
	fn.glProgramUniform2uiv(program, location, count, tmp)
#else
#define FN_GLPROGRAMUNIFORM2UIV(program, location, count, value) \
	fn.glProgramUniform2uiv(program, location, count, HostAddr(value, const GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLPROGRAMUNIFORM3IV(program, location, count, value) \
	GLint const size = 3 * count; \
	if (size <= 0) return; \
	GLint tmp[size]; \
	Atari2HostIntArray(size, value, tmp); \
	fn.glProgramUniform3iv(program, location, count, tmp)
#else
#define FN_GLPROGRAMUNIFORM3IV(program, location, count, value) \
	fn.glProgramUniform3iv(program, location, count, HostAddr(value, const GLint *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPROGRAMUNIFORM3FV(program, location, count, value) \
	GLint const size = 3 * count; \
	if (size <= 0) return; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, value, tmp); \
	fn.glProgramUniform3fv(program, location, count, tmp)
#else
#define FN_GLPROGRAMUNIFORM3FV(program, location, count, value) \
	fn.glProgramUniform3fv(program, location, count, HostAddr(value, const GLfloat *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLPROGRAMUNIFORM3DV(program, location, count, value) \
	GLint const size = 3 * count; \
	if (size <= 0) return; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, value, tmp); \
	fn.glProgramUniform3dv(program, location, count, tmp)
#else
#define FN_GLPROGRAMUNIFORM3DV(program, location, count, value) \
	fn.glProgramUniform3dv(program, location, count, HostAddr(value, const GLdouble *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLPROGRAMUNIFORM3UIV(program, location, count, value) \
	GLint const size = 3 * count; \
	if (size <= 0) return; \
	GLuint tmp[size]; \
	Atari2HostIntArray(size, value, tmp); \
	fn.glProgramUniform3uiv(program, location, count, tmp)
#else
#define FN_GLPROGRAMUNIFORM3UIV(program, location, count, value) \
	fn.glProgramUniform3uiv(program, location, count, HostAddr(value, const GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLPROGRAMUNIFORM4IV(program, location, count, value) \
	GLint const size = 4 * count; \
	if (size <= 0) return; \
	GLint tmp[size]; \
	Atari2HostIntArray(size, value, tmp); \
	fn.glProgramUniform4iv(program, location, count, tmp)
#else
#define FN_GLPROGRAMUNIFORM4IV(program, location, count, value) \
	fn.glProgramUniform4iv(program, location, count, HostAddr(value, const GLint *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPROGRAMUNIFORM4FV(program, location, count, value) \
	GLint const size = 4 * count; \
	if (size <= 0) return; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, value, tmp); \
	fn.glProgramUniform4fv(program, location, count, tmp)
#else
#define FN_GLPROGRAMUNIFORM4FV(program, location, count, value) \
	fn.glProgramUniform4fv(program, location, count, HostAddr(value, const GLfloat *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLPROGRAMUNIFORM4DV(program, location, count, value) \
	GLint const size = 4 * count; \
	if (size <= 0) return; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, value, tmp); \
	fn.glProgramUniform4dv(program, location, count, tmp)
#else
#define FN_GLPROGRAMUNIFORM4DV(program, location, count, value) \
	fn.glProgramUniform4dv(program, location, count, HostAddr(value, const GLdouble *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLPROGRAMUNIFORM4UIV(program, location, count, value) \
	GLint const size = 4 * count; \
	if (size <= 0) return; \
	GLuint tmp[size]; \
	Atari2HostIntArray(size, value, tmp); \
	fn.glProgramUniform4uiv(program, location, count, tmp)
#else
#define FN_GLPROGRAMUNIFORM4UIV(program, location, count, value) \
	fn.glProgramUniform4uiv(program, location, count, HostAddr(value, const GLuint *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPROGRAMUNIFORMMATRIX2FV(program, location, count, transpose, value) \
	GLint const size = 4 * count; \
	if (size <= 0) return; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, value, tmp); \
	fn.glProgramUniformMatrix2fv(program, location, count, transpose, tmp)
#else
#define FN_GLPROGRAMUNIFORMMATRIX2FV(program, location, count, transpose, value) \
	fn.glProgramUniformMatrix2fv(program, location, count, transpose, HostAddr(value, const GLfloat *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPROGRAMUNIFORMMATRIX3FV(program, location, count, transpose, value) \
	GLint const size = 9 * count; \
	if (size <= 0) return; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, value, tmp); \
	fn.glProgramUniformMatrix3fv(program, location, count, transpose, tmp)
#else
#define FN_GLPROGRAMUNIFORMMATRIX3FV(program, location, count, transpose, value) \
	fn.glProgramUniformMatrix3fv(program, location, count, transpose, HostAddr(value, const GLfloat *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPROGRAMUNIFORMMATRIX4FV(program, location, count, transpose, value) \
	GLint const size = 16 * count; \
	if (size <= 0) return; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, value, tmp); \
	fn.glProgramUniformMatrix4fv(program, location, count, transpose, tmp)
#else
#define FN_GLPROGRAMUNIFORMMATRIX4FV(program, location, count, transpose, value) \
	fn.glProgramUniformMatrix4fv(program, location, count, transpose, HostAddr(value, const GLfloat *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLPROGRAMUNIFORMMATRIX2DV(program, location, count, transpose, value) \
	GLint const size = 4 * count; \
	if (size <= 0) return; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, value, tmp); \
	fn.glProgramUniformMatrix2dv(program, location, count, transpose, tmp)
#else
#define FN_GLPROGRAMUNIFORMMATRIX2DV(program, location, count, transpose, value) \
	fn.glProgramUniformMatrix2dv(program, location, count, transpose, HostAddr(value, const GLdouble *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLPROGRAMUNIFORMMATRIX3DV(program, location, count, transpose, value) \
	GLint const size = 9 * count; \
	if (size <= 0) return; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, value, tmp); \
	fn.glProgramUniformMatrix3dv(program, location, count, transpose, tmp)
#else
#define FN_GLPROGRAMUNIFORMMATRIX3DV(program, location, count, transpose, value) \
	fn.glProgramUniformMatrix3dv(program, location, count, transpose, HostAddr(value, const GLdouble *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLPROGRAMUNIFORMMATRIX4DV(program, location, count, transpose, value) \
	GLint const size = 16 * count; \
	if (size <= 0) return; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, value, tmp); \
	fn.glProgramUniformMatrix4dv(program, location, count, transpose, tmp)
#else
#define FN_GLPROGRAMUNIFORMMATRIX4DV(program, location, count, transpose, value) \
	fn.glProgramUniformMatrix4dv(program, location, count, transpose, HostAddr(value, const GLdouble *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPROGRAMUNIFORMMATRIX2X3FV(program, location, count, transpose, value) \
	GLint const size = 6 * count; \
	if (size <= 0) return; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, value, tmp); \
	fn.glProgramUniformMatrix2x3fv(program, location, count, transpose, tmp)
#else
#define FN_GLPROGRAMUNIFORMMATRIX2X3FV(program, location, count, transpose, value) \
	fn.glProgramUniformMatrix2x3fv(program, location, count, transpose, HostAddr(value, const GLfloat *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPROGRAMUNIFORMMATRIX3X2FV(program, location, count, transpose, value) \
	GLint const size = 6 * count; \
	if (size <= 0) return; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, value, tmp); \
	fn.glProgramUniformMatrix3x2fv(program, location, count, transpose, tmp)
#else
#define FN_GLPROGRAMUNIFORMMATRIX3X2FV(program, location, count, transpose, value) \
	fn.glProgramUniformMatrix3x2fv(program, location, count, transpose, HostAddr(value, const GLfloat *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPROGRAMUNIFORMMATRIX2X4FV(program, location, count, transpose, value) \
	GLint const size = 8 * count; \
	if (size <= 0) return; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, value, tmp); \
	fn.glProgramUniformMatrix2x4fv(program, location, count, transpose, tmp)
#else
#define FN_GLPROGRAMUNIFORMMATRIX2X4FV(program, location, count, transpose, value) \
	fn.glProgramUniformMatrix2x4fv(program, location, count, transpose, HostAddr(value, const GLfloat *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPROGRAMUNIFORMMATRIX4X2FV(program, location, count, transpose, value) \
	GLint const size = 8 * count; \
	if (size <= 0) return; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, value, tmp); \
	fn.glProgramUniformMatrix4x2fv(program, location, count, transpose, tmp)
#else
#define FN_GLPROGRAMUNIFORMMATRIX4X2FV(program, location, count, transpose, value) \
	fn.glProgramUniformMatrix4x2fv(program, location, count, transpose, HostAddr(value, const GLfloat *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPROGRAMUNIFORMMATRIX3X4FV(program, location, count, transpose, value) \
	GLint const size = 12 * count; \
	if (size <= 0) return; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, value, tmp); \
	fn.glProgramUniformMatrix3x4fv(program, location, count, transpose, tmp)
#else
#define FN_GLPROGRAMUNIFORMMATRIX3X4FV(program, location, count, transpose, value) \
	fn.glProgramUniformMatrix3x4fv(program, location, count, transpose, HostAddr(value, const GLfloat *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPROGRAMUNIFORMMATRIX4X3FV(program, location, count, transpose, value) \
	GLint const size = 12 * count; \
	if (size <= 0) return; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, value, tmp); \
	fn.glProgramUniformMatrix4x3fv(program, location, count, transpose, tmp)
#else
#define FN_GLPROGRAMUNIFORMMATRIX4X3FV(program, location, count, transpose, value) \
	fn.glProgramUniformMatrix4x3fv(program, location, count, transpose, HostAddr(value, const GLfloat *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLPROGRAMUNIFORMMATRIX2X3DV(program, location, count, transpose, value) \
	GLint const size = 6 * count; \
	if (size <= 0) return; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, value, tmp); \
	fn.glProgramUniformMatrix2x3dv(program, location, count, transpose, tmp)
#else
#define FN_GLPROGRAMUNIFORMMATRIX2X3DV(program, location, count, transpose, value) \
	fn.glProgramUniformMatrix2x3dv(program, location, count, transpose, HostAddr(value, const GLdouble *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLPROGRAMUNIFORMMATRIX3X2DV(program, location, count, transpose, value) \
	GLint const size = 6 * count; \
	if (size <= 0) return; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, value, tmp); \
	fn.glProgramUniformMatrix3x2dv(program, location, count, transpose, tmp)
#else
#define FN_GLPROGRAMUNIFORMMATRIX3X2DV(program, location, count, transpose, value) \
	fn.glProgramUniformMatrix3x2dv(program, location, count, transpose, HostAddr(value, const GLdouble *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLPROGRAMUNIFORMMATRIX2X4DV(program, location, count, transpose, value) \
	GLint const size = 8 * count; \
	if (size <= 0) return; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, value, tmp); \
	fn.glProgramUniformMatrix2x4dv(program, location, count, transpose, tmp)
#else
#define FN_GLPROGRAMUNIFORMMATRIX2X4DV(program, location, count, transpose, value) \
	fn.glProgramUniformMatrix2x4dv(program, location, count, transpose, HostAddr(value, const GLdouble *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLPROGRAMUNIFORMMATRIX4X2DV(program, location, count, transpose, value) \
	GLint const size = 8 * count; \
	if (size <= 0) return; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, value, tmp); \
	fn.glProgramUniformMatrix4x2dv(program, location, count, transpose, tmp)
#else
#define FN_GLPROGRAMUNIFORMMATRIX4X2DV(program, location, count, transpose, value) \
	fn.glProgramUniformMatrix4x2dv(program, location, count, transpose, HostAddr(value, const GLdouble *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLPROGRAMUNIFORMMATRIX3X4DV(program, location, count, transpose, value) \
	GLint const size = 12 * count; \
	if (size <= 0) return; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, value, tmp); \
	fn.glProgramUniformMatrix3x4dv(program, location, count, transpose, tmp)
#else
#define FN_GLPROGRAMUNIFORMMATRIX3X4DV(program, location, count, transpose, value) \
	fn.glProgramUniformMatrix3x4dv(program, location, count, transpose, HostAddr(value, const GLdouble *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLPROGRAMUNIFORMMATRIX4X3DV(program, location, count, transpose, value) \
	GLint const size = 12 * count; \
	if (size <= 0) return; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, value, tmp); \
	fn.glProgramUniformMatrix4x3dv(program, location, count, transpose, tmp)
#else
#define FN_GLPROGRAMUNIFORMMATRIX4X3DV(program, location, count, transpose, value) \
	fn.glProgramUniformMatrix4x3dv(program, location, count, transpose, HostAddr(value, const GLdouble *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETFLOATI_V(pname, index, params) \
	int n = nfglGetNumParams(pname); \
	GLfloat tmp[MAX(n, 16)]; \
	fn.glGetFloati_v(pname, index, tmp); \
	Host2AtariFloatArray(n, tmp, params)
#else
#define FN_GLGETFLOATI_V(pname, index, params) \
	fn.glGetFloati_v(pname, index, HostAddr(params, GLfloat *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLGETDOUBLEI_V(pname, index, params) \
	int n = nfglGetNumParams(pname); \
	GLdouble tmp[MAX(n, 16)]; \
	fn.glGetDoublei_v(pname, index, tmp); \
	Host2AtariDoubleArray(n, tmp, params)
#else
#define FN_GLGETDOUBLEI_V(pname, index, params) \
	fn.glGetDoublei_v(pname, index, HostAddr(params, GLdouble *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLVERTEXATTRIBL1DV(index, v) \
	GLint const size = 1; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, v, tmp); \
	fn.glVertexAttribL1dv(index, tmp)
#else
#define FN_GLVERTEXATTRIBL1DV(index, v) \
	fn.glVertexAttribL1dv(index, HostAddr(v, const GLdouble *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLVERTEXATTRIBL2DV(index, v) \
	GLint const size = 2; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, v, tmp); \
	fn.glVertexAttribL2dv(index, tmp)
#else
#define FN_GLVERTEXATTRIBL2DV(index, v) \
	fn.glVertexAttribL2dv(index, HostAddr(v, const GLdouble *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLVERTEXATTRIBL3DV(index, v) \
	GLint const size = 3; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, v, tmp); \
	fn.glVertexAttribL3dv(index, tmp)
#else
#define FN_GLVERTEXATTRIBL3DV(index, v) \
	fn.glVertexAttribL3dv(index, HostAddr(v, const GLdouble *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLVERTEXATTRIBL4DV(index, v) \
	GLint const size = 4; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, v, tmp); \
	fn.glVertexAttribL4dv(index, tmp)
#else
#define FN_GLVERTEXATTRIBL4DV(index, v) \
	fn.glVertexAttribL4dv(index, HostAddr(v, const GLdouble *))
#endif

/* TODO */
#define FN_GLVERTEXATTRIBLPOINTER(index, size, type, stride, pointer) \
	fn.glVertexAttribLPointer(index, size, type, stride, HostAddr(pointer, const void *))

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLGETVERTEXATTRIBLDV(index, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLdouble tmp[size]; \
	fn.glGetVertexAttribLdv(index, pname, tmp); \
	Host2AtariDoubleArray(size, tmp, params)
#else
#define FN_GLGETVERTEXATTRIBLDV(index, pname, params) \
	fn.glGetVertexAttribLdv(index, pname, HostAddr(params, GLdouble *))
#endif

#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_INT_CONV
#define FN_GLGETPROGRAMPIPELINEINFOLOG(pipeline, bufSize, length, infoLog) \
	GLsizei l = 0; \
	GLchar tmp[MAX(bufSize, 0)]; \
	fn.glGetProgramPipelineInfoLog(pipeline, bufSize, &l, tmp); \
	Host2AtariByteArray(MIN(l + 1, bufSize), tmp, infoLog); \
	Host2AtariIntArray(1, &l, length)
#else
#define FN_GLGETPROGRAMPIPELINEINFOLOG(pipeline, bufSize, length, infoLog) \
	fn.glGetProgramPipelineInfoLog(pipeline, bufSize, HostAddr(length, GLsizei *), HostAddr(infoLog, GLchar *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLVIEWPORTARRAYV(first, count, v) \
	GLsizei const size = 4 * count; \
	if (size <= 0) return; \
	GLfloat tmp[size], *ptmp; \
	ptmp = Atari2HostFloatArray(size, v, tmp); \
	fn.glViewportArrayv(first, count, ptmp)
#else
#define FN_GLVIEWPORTARRAYV(first, count, v) \
	fn.glViewportArrayv(first, count, HostAddr(v, const GLfloat *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLVIEWPORTINDEXEDFV(index, v) \
	GLsizei const size = 4; \
	GLfloat tmp[size], *ptmp; \
	ptmp = Atari2HostFloatArray(size, v, tmp); \
	fn.glViewportIndexedfv(index, ptmp)
#else
#define FN_GLVIEWPORTINDEXEDFV(index, v) \
	fn.glViewportIndexedfv(index, HostAddr(v, const GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLSCISSORARRAYV(first, count, v) \
	GLsizei const size = 4 * count; \
	if (size <= 0) return; \
	GLint tmp[size]; \
	Atari2HostIntArray(size, v, tmp); \
	fn.glScissorArrayv(first, count, tmp)
#else
#define FN_GLSCISSORARRAYV(first, count, v) \
	fn.glScissorArrayv(first, count, HostAddr(v, const GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLSCISSORINDEXEDV(index, v) \
	GLsizei const size = 4; \
	GLint tmp[size]; \
	Atari2HostIntArray(size, v, tmp); \
	fn.glScissorIndexedv(index, tmp)
#else
#define FN_GLSCISSORINDEXEDV(index, v) \
	fn.glScissorIndexedv(index, HostAddr(v, const GLint *))
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLDEPTHRANGEARRAYV(first, count, v) \
	GLsizei const size = 2 * count; \
	if (size <= 0) return; \
	GLdouble tmp[size], *ptmp; \
	ptmp = Atari2HostDoubleArray(size, v, tmp); \
	fn.glDepthRangeArrayv(first, count, ptmp)
#else
#define FN_GLDEPTHRANGEARRAYV(first, count, v) \
	fn.glDepthRangeArrayv(first, count, HostAddr(v, const GLdouble *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * Version 4.2
 */

#define FN_GLDRAWELEMENTSINSTANCEDBASEINSTANCE(mode, count, type, indices, instancecount, baseinstance) \
	void *tmp; \
	convertClientArrays(count); \
	pixelBuffer buf(*this); \
	switch(type) { \
	case GL_UNSIGNED_BYTE: \
	case GL_UNSIGNED_SHORT: \
	case GL_UNSIGNED_INT: \
		tmp = buf.convertArray(count, type, (nfmemptr)indices); \
		break; \
	default: \
		glSetError(GL_INVALID_ENUM); \
		return; \
	} \
	fn.glDrawElementsInstancedBaseInstance(mode, count, type, tmp, instancecount, baseinstance)

#define FN_GLDRAWELEMENTSINSTANCEDBASEVERTEXBASEINSTANCE(mode, count, type, indices, instancecount, basevertex, baseinstance) \
	void *tmp; \
	convertClientArrays(count); \
	pixelBuffer buf(*this); \
	switch(type) { \
	case GL_UNSIGNED_BYTE: \
	case GL_UNSIGNED_SHORT: \
	case GL_UNSIGNED_INT: \
		tmp = buf.convertArray(count, type, (nfmemptr)indices); \
		break; \
	default: \
		glSetError(GL_INVALID_ENUM); \
		return; \
	} \
	fn.glDrawElementsInstancedBaseVertexBaseInstance(mode, count, type, tmp, instancecount, basevertex, baseinstance)

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETINTERNALFORMATIV(target, internalformat, pname, bufSize, params) \
	GLint const size = 1; \
	GLint tmp[size]; \
	fn.glGetInternalformativ(target, internalformat, pname, bufSize, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETINTERNALFORMATIV(target, internalformat, pname, bufSize, params) \
	fn.glGetInternalformativ(target, internalformat, pname, bufSize, HostAddr(params, GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETACTIVEATOMICCOUNTERBUFFERIV(program, bufferIndex, pname, params) \
	GLint n; \
	if (pname == GL_ATOMIC_COUNTER_BUFFER_ACTIVE_ATOMIC_COUNTER_INDICES) { \
		n = 0; \
		fn.glGetActiveAtomicCounterBufferiv(program, bufferIndex, GL_ATOMIC_COUNTER_BUFFER_ACTIVE_ATOMIC_COUNTERS, &n); \
	} else { \
		n = 1; \
	} \
	if (n > 0 && params) { \
		GLint tmp[n]; \
		fn.glGetActiveAtomicCounterBufferiv(program, bufferIndex, pname, tmp); \
		Host2AtariIntArray(n, tmp, params); \
	}
#else
#define FN_GLGETACTIVEATOMICCOUNTERBUFFERIV(program, bufferIndex, pname, params) \
	fn.glGetActiveAtomicCounterBufferiv(program, bufferIndex, pname, HostAddr(params, GLint *))
#endif

/* -------------------------------------------------------------------------- */

/*
 * Version 4.3
 */

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETFRAMEBUFFERPARAMETERIV(target, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLint tmp[MAX(size, 16)]; \
	fn.glGetFramebufferParameteriv(target, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETFRAMEBUFFERPARAMETERIV(target, pname, params) \
	fn.glGetFramebufferParameteriv(target, pname, HostAddr(params, GLint *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLDEBUGMESSAGEINSERT(source, type, id, severity, length, buf) \
	if (length < 0) length = safe_strlen(buf); \
	GLchar stringbuf[length], *pstring; \
	pstring = Atari2HostByteArray(length, buf, stringbuf); \
	return fn.glDebugMessageInsert(source, type, id, severity, length, pstring)
#else
#define FN_GLDEBUGMESSAGEINSERT(source, type, id, severity, length, buf) \
	return fn.glDebugMessageInsert(source, type, id, severity, length, HostAddr(buf, const GLchar *))
#endif

#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_INT_CONV
#define FN_GLGETDEBUGMESSAGELOG(count, bufSize, sources, types, ids, severities, lengths, messageLog) \
	GLenum *psources; \
	GLenum *ptypes; \
	GLuint *pids; \
	GLenum *pseverities; \
	GLsizei *plengths; \
	GLchar *pmessageLog; \
	if (sources ) { \
		psources = (GLenum *)malloc(count * sizeof(*psources)); \
		if (psources == NULL) { glSetError(GL_OUT_OF_MEMORY); return 0; } \
	} else { \
		psources = NULL; \
	} \
	if (types) { \
		ptypes = (GLenum *)malloc(count * sizeof(*ptypes)); \
		if (ptypes == NULL) { glSetError(GL_OUT_OF_MEMORY); return 0; } \
	} else { \
		ptypes = NULL; \
	} \
	if (ids) { \
		pids = (GLuint *)malloc(count * sizeof(*pids)); \
		if (pids == NULL) { glSetError(GL_OUT_OF_MEMORY); return 0; } \
	} else { \
		pids = NULL; \
	} \
	if (severities) { \
		pseverities = (GLenum *)malloc(count * sizeof(*pseverities)); \
		if (pseverities == NULL) { glSetError(GL_OUT_OF_MEMORY); return 0; } \
	} else { \
		pseverities = NULL; \
	} \
	if (lengths) { \
		plengths = (GLsizei *)malloc(count * sizeof(*plengths)); \
		if (plengths == NULL) { glSetError(GL_OUT_OF_MEMORY); return 0; } \
	} else { \
		plengths = NULL; \
	} \
	if (messageLog) { \
		pmessageLog = (GLchar *)malloc(bufSize); \
		if (pmessageLog == NULL) { glSetError(GL_OUT_OF_MEMORY); return 0; } \
	} else { \
		pmessageLog = NULL; \
	} \
	count = fn.glGetDebugMessageLog(count, bufSize, psources, ptypes, pids, pseverities, plengths, pmessageLog); \
	Host2AtariIntArray(count, psources, sources); free(psources); \
	Host2AtariIntArray(count, ptypes, types); free(ptypes); \
	Host2AtariIntArray(count, pids, ids); free(pids); \
	Host2AtariIntArray(count, pseverities, severities); free(pseverities); \
	Host2AtariIntArray(count, plengths, lengths); free(plengths); \
	Host2AtariByteArray(bufSize, pmessageLog, messageLog); free(pmessageLog); \
	return count
#else
#define FN_GLGETDEBUGMESSAGELOG(count, bufSize, sources, types, ids, severities, lengths, messageLog) \
	return fn.glGetDebugMessageLog(count, bufSize, HostAddr(sources, GLenum *), HostAddr(types, GLenum *), HostAddr(ids, GLuint *), HostAddr(severities, GLenum *), HostAddr(lengths, GLsizei *), HostAddr(messageLog, GLchar *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLDEBUGMESSAGECONTROL(source, type, severity, count, ids, enabled) \
	GLuint tmp[MAX(count, 0)], *ptmp; \
	ptmp = Atari2HostIntArray(count, ids, tmp); \
	fn.glDebugMessageControl(source, type, severity, count, ptmp, enabled)
#else
#define FN_GLDEBUGMESSAGECONTROL(source, type, severity, count, ids, enabled) \
	fn.glDebugMessageControl(source, type, severity, count, HostAddr(ids, const GLuint *), enabled)
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLPUSHDEBUGGROUP(source, name, length, label) \
	if (length < 0) length = safe_strlen(label); \
	GLchar labelbuf[length], *plabel; \
	plabel = Atari2HostByteArray(length, label, labelbuf); \
	fn.glPushDebugGroup(source, name, length, plabel)
#else
#define FN_GLPUSHDEBUGGROUP(source, name, length, label) \
	fn.glPushDebugGroup(source, name, length, HostAddr(label, const GLchar *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLOBJECTLABEL(identifier, name, length, label) \
	if (length < 0) length = safe_strlen(label); \
	GLchar labelbuf[length], *plabel; \
	plabel = Atari2HostByteArray(length, label, labelbuf); \
	fn.glObjectLabel(identifier, name, length, plabel)
#else
#define FN_GLOBJECTLABEL(identifier, name, length, label) \
	fn.glObjectLabel(identifier, name, length, HostAddr(label, const GLchar *))
#endif

#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_INT_CONV
#define FN_GLGETOBJECTLABEL(identifier, name, bufSize, length, label) \
	GLsizei l = 0; \
	GLchar labelbuf[MAX(bufSize, 0)]; \
	fn.glGetObjectLabel(identifier, name, bufSize, &l, labelbuf); \
	Host2AtariIntArray(1, &l, length); \
	Host2AtariByteArray(MIN(l + 1, bufSize), labelbuf, label)
#else
#define FN_GLGETOBJECTLABEL(identifier, name, bufSize, length, label) \
	fn.glGetObjectLabel(identifier, name, bufSize, HostAddr(length, GLsizei *), HostAddr(label, GLchar *))
#endif

#define FN_GLOBJECTPTRLABEL(identifier, length, label) \
	if (length < 0) length = safe_strlen(label) + 1; \
	GLchar tmp[length], *plabel; \
	plabel = Atari2HostByteArray(length, label, tmp); \
	void *pid = NFHost2AtariAddr(identifier); \
	fn.glObjectPtrLabel(pid, length, plabel)

#define FN_GLGETOBJECTPTRLABEL(identifier, bufSize, length, label) \
	GLsizei l; \
	GLchar namebuf[MAX(bufSize, 0)]; \
	void *pid = NFHost2AtariAddr(identifier); \
	fn.glGetObjectPtrLabel(pid, bufSize, &l, namebuf); \
	Host2AtariByteArray(MIN(l + 1, bufSize), namebuf, label); \
	Host2AtariIntArray(1, &l, length)

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETINTERNALFORMATI64V(target, internalformat, pname, bufSize, params) \
	GLint const size = 1; \
	GLint64 tmp[size]; \
	fn.glGetInternalformati64v(target, internalformat, pname, bufSize, tmp); \
	Host2AtariInt64Array(size, tmp, params)
#else
#define FN_GLGETINTERNALFORMATI64V(target, internalformat, pname, bufSize, params) \
	fn.glGetInternalformati64v(target, internalformat, pname, bufSize, HostAddr(params, GLint64 *))
#endif

/*
 * The parameters addressed by indirect are packed into a structure that takes the form (in C):
 *
 *  typedef  struct {
 *      uint  num_groups_x;
 *      uint  num_groups_y;
 *      uint  num_groups_z;
 *  } DispatchIndirectCommand;
 */
#define FN_GLDISPATCHCOMPUTEINDIRECT(indirect) \
	if (contexts[cur_context].buffer_bindings.dispatch_indirect.id) { \
		fn.glDispatchComputeIndirect(indirect); \
	} else { \
		glSetError(GL_INVALID_OPERATION); \
	}

#define FN_GLINVALIDATEBUFFERDATA(buffer) \
	gl_buffer_t *buf = gl_get_buffer(buffer); \
	if (buf) { \
		buf->atari_buffer = 0; \
		free(buf->host_buffer); \
		buf->host_buffer = NULL; \
		buf->size = 0; \
	}

#if NFOSMESA_NEED_INT_CONV
#define FN_GLINVALIDATEFRAMEBUFFER(target, numAttachments, attachments) \
	GLenum tmp[MAX(numAttachments, 0)], *ptmp; \
	ptmp = Atari2HostIntArray(numAttachments, attachments, tmp); \
	fn.glInvalidateFramebuffer(target, numAttachments, ptmp)
#else
#define FN_GLINVALIDATEFRAMEBUFFER(target, numAttachments, attachments) \
	fn.glInvalidateFramebuffer(target, numAttachments, HostAddr(attachments, const GLenum *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLINVALIDATESUBFRAMEBUFFER(target, numAttachments, attachments, x, y, width, height) \
	GLenum tmp[MAX(numAttachments, 0)], *ptmp; \
	ptmp = Atari2HostIntArray(numAttachments, attachments, tmp); \
	fn.glInvalidateSubFramebuffer(target, numAttachments, ptmp, x, y, width, height)
#else
#define FN_GLINVALIDATESUBFRAMEBUFFER(target, numAttachments, attachments, x, y, width, height) \
	fn.glInvalidateSubFramebuffer(target, numAttachments, HostAddr(attachments, const GLenum *), x, y, width, height)
#endif

#define FN_GLMULTIDRAWARRAYSINDIRECT(mode, indirect, drawcount, stride) \
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
		void *offset = NFHost2AtariAddr(indirect); \
		fn.glMultiDrawArraysIndirect(mode, offset, drawcount, stride); \
	} else if (indirect) { \
		if (stride == 0) stride = 4 * sizeof(Uint32); \
		nfcmemptr indptr = (nfcmemptr)indirect; \
		for (GLsizei n = 0; n < drawcount; n++) { \
			GLuint tmp[4] = { 0, 0, 0, 0 }; \
			Atari2HostIntArray(4, indptr, tmp); \
			GLuint count = tmp[0]; \
			convertClientArrays(count); \
			fn.glDrawArraysInstancedBaseInstance(mode, tmp[2], count, tmp[1], tmp[3]); \
			indptr += stride; \
		} \
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
 *    } DrawElemntsIndirectCommand;
 */
#define FN_GLMULTIDRAWELEMENTSINDIRECT(mode, type, indirect, drawcount, stride) \
	if (contexts[cur_context].buffer_bindings.draw_indirect.id) { \
		void *offset = NFHost2AtariAddr(indirect); \
		fn.glMultiDrawElementsIndirect(mode, type, offset, drawcount, stride); \
	} else if (indirect) { \
		if (stride == 0) stride = 5 * sizeof(Uint32); \
		nfcmemptr pind = (nfcmemptr)indirect; \
		for (GLsizei n = 0; n < drawcount; n++) { \
			GLuint tmp[5] = { 0, 0, 0, 0, 0 }; \
			Atari2HostIntArray(5, pind, tmp); \
			GLuint count = tmp[0]; \
			convertClientArrays(count); \
			fn.glDrawElementsInstancedBaseVertexBaseInstance(mode, count, type, (const void *)(uintptr_t)tmp[2], tmp[1], tmp[3], tmp[4]); \
			pind += stride; \
		} \
	}

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETPROGRAMINTERFACEIV(program, programInterface, pname, params) \
	GLint const size = 1; \
	GLint tmp[size]; \
	fn.glGetProgramInterfaceiv(program, programInterface, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETPROGRAMINTERFACEIV(program, programInterface, pname, params) \
	fn.glGetProgramInterfaceiv(program, programInterface, pname, HostAddr(params, GLint *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLGETPROGRAMRESOURCEINDEX(program, programInterface, name) \
	GLchar tmp[safe_strlen(name) + 1], *ptmp; \
	ptmp = Atari2HostByteArray(sizeof(tmp), name, tmp); \
	return fn.glGetProgramResourceIndex(program, programInterface, ptmp)
#else
#define FN_GLGETPROGRAMRESOURCEINDEX(program, programInterface, name) \
	return fn.glGetProgramResourceIndex(program, programInterface, HostAddr(name, const GLchar *))
#endif
	
#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_INT_CONV
#define FN_GLGETPROGRAMRESOURCENAME(program, programInterface, index, bufSize, length, name) \
	GLsizei l = 0; \
	GLchar namebuf[MAX(bufSize, 0)]; \
	fn.glGetProgramResourceName(program, programInterface, index, bufSize, &l, namebuf); \
	Host2AtariByteArray(MIN(l + 1, bufSize), namebuf, name); \
	Host2AtariIntArray(1, &l, length)
#else
#define FN_GLGETPROGRAMRESOURCENAME(program, programInterface, index, bufSize, length, name) \
	fn.glGetProgramResourceName(program, programInterface, index, bufSize, HostAddr(length, GLsizei *), HostAddr(name, GLchar *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETPROGRAMRESOURCEIV(program, programInterface, index, propCount, props, bufSize, length, params) \
	GLint const size = MAX(propCount, 0); \
	GLenum tmp[size], *ptmp; \
	GLint tmpparam[MAX(bufSize, 0)]; \
	GLsizei l = 0; \
	ptmp = Atari2HostIntArray(size, props, tmp); \
	fn.glGetProgramResourceiv(program, programInterface, index, propCount, ptmp, bufSize, &l, tmpparam); \
	Host2AtariIntArray(1, &l, length); \
	Host2AtariIntArray(l, tmpparam, params)
#else
#define FN_GLGETPROGRAMRESOURCEIV(program, programInterface, index, propCount, props, bufSize, length, params) \
	fn.glGetProgramResourceiv(program, programInterface, index, propCount, HostAddr(props, const GLenum *), bufSize, HostAddr(length, GLsizei *), HostAddr(params, GLint *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLGETPROGRAMRESOURCELOCATION(program, programInterface, name) \
	GLchar tmp[safe_strlen(name) + 1], *ptmp; \
	ptmp = Atari2HostByteArray(sizeof(tmp), name, tmp); \
	return fn.glGetProgramResourceLocation(program, programInterface, ptmp)
#else
#define FN_GLGETPROGRAMRESOURCELOCATION(program, programInterface, name) \
	return fn.glGetProgramResourceLocation(program, programInterface, HostAddr(name, const GLchar *))
#endif
	
#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLGETPROGRAMRESOURCELOCATIONINDEX(program, programInterface, name) \
	GLchar tmp[safe_strlen(name) + 1], *ptmp; \
	ptmp = Atari2HostByteArray(sizeof(tmp), name, tmp); \
	return fn.glGetProgramResourceLocationIndex(program, programInterface, ptmp)
#else
#define FN_GLGETPROGRAMRESOURCELOCATIONINDEX(program, programInterface, name) \
	return fn.glGetProgramResourceLocationIndex(program, programInterface, HostAddr(name, const GLchar *))
#endif
	
/* TODO */
#define FN_GLBINDVERTEXBUFFER(bindingindex, buffer, offset, stride) \
	fn.glBindVertexBuffer(bindingindex, buffer, offset, stride)

/* -------------------------------------------------------------------------- */

/*
 * Version 4.4
 */
#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLBUFFERSTORAGE(target, size, data, flags) \
	GLubyte tmp[MAX(size, 0)], *ptmp; \
	ptmp = Atari2HostByteArray(sizeof(tmp), data, tmp); \
	fn.glBufferStorage(target, size, ptmp, flags)
#else
#define FN_GLBUFFERSTORAGE(target, size, data, flags) \
	fn.glBufferStorage(target, size, HostAddr(data, const void *), flags)
#endif

#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLCLEARTEXIMAGE(texture, level, format, type, data) \
	pixelBuffer buf(*this); \
	void *tmp = buf.convertPixels(1, 1, 1, format, type, (nfmemptr)data); \
	fn.glClearTexImage(texture, level, format, type, tmp)
#else
#define FN_GLCLEARTEXIMAGE(texture, level, format, type, data) \
	fn.glClearTexImage(texture, level, format, type, HostAddr(data, const void *))
#endif

#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLCLEARTEXSUBIMAGE(texture, level, xoffset, yoffset, zoffset, width, height, depth, format, type, data) \
	pixelBuffer buf(*this); \
	void *tmp = buf.convertPixels(width, height, depth, format, type, (nfmemptr)data); \
	fn.glClearTexSubImage(texture, level, xoffset, yoffset, zoffset, width, height, depth, format, type, tmp)
#else
#define FN_GLCLEARTEXSUBIMAGE(texture, level, xoffset, yoffset, zoffset, width, height, depth, format, type, data) \
	fn.glClearTexSubImage(texture, level, xoffset, yoffset, zoffset, width, height, depth, format, type, HostAddr(data, const void *))
#endif

#define FN_GLBINDBUFFERSBASE(target, first, count, buffers) \
	if (buffers) { \
		GLuint tmp[MAX(count, 0)]; \
		Atari2HostIntArray(count, buffers, tmp); \
		for (GLsizei i = 0; i < count; i++) \
			gl_bind_buffer(target, tmp[i], first + i, 0); \
		fn.glBindBuffersBase(target, first, count, tmp); \
	} else { \
		for (GLsizei i = 0; i < count; i++) \
			gl_bind_buffer(target, 0, first + i, 0); \
		fn.glBindBuffersBase(target, first, count, NULL); \
	}

#define FN_GLBINDBUFFERSRANGE(target, first, count, buffers, offsets, sizes) \
	GLsizei const size = MAX(count, 0); \
	GLuint tmpbuffers[size], *pbuffers; \
	GLintptr tmpoffsets[size], *poffsets; \
	GLsizeiptr tmpsizes[size], *psizes; \
	pbuffers = Atari2HostIntArray(count, buffers, tmpbuffers); \
	if (pbuffers) { \
		for (GLsizei i = 0; i < count; i++) \
			gl_bind_buffer(target, pbuffers[i], first + i, 0); \
	} else { \
		for (GLsizei i = 0; i < count; i++) \
			gl_bind_buffer(target, 0, first + i, 0); \
	} \
	poffsets = Atari2HostIntptrArray(count, offsets, tmpoffsets); \
	psizes = Atari2HostIntptrArray(count, sizes, tmpsizes); \
	fn.glBindBuffersRange(target, first, count, pbuffers, poffsets, psizes)

#if NFOSMESA_NEED_INT_CONV
#define FN_GLBINDTEXTURES(first, count, textures) \
	GLuint tmp[MAX(count, 0)], *ptmp; \
	ptmp = Atari2HostIntArray(count, textures, tmp); \
	fn.glBindTextures(first, count, ptmp)
#else
#define FN_GLBINDTEXTURES(first, count, samples) \
	fn.glBindTextures(first, count, HostAddr(textures, const GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLBINDSAMPLERS(first, count, samplers) \
	GLuint tmp[MAX(count, 0)], *ptmp; \
	ptmp = Atari2HostIntArray(count, samplers, tmp); \
	fn.glBindSamplers(first, count, ptmp)
#else
#define FN_GLBINDSAMPLERS(first, count, samples) \
	fn.glBindSamplers(first, count, HostAddr(samplers, const GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLBINDIMAGETEXTURES(first, count, textures) \
	GLuint tmp[MAX(count, 0)], *ptmp; \
	ptmp = Atari2HostIntArray(count, textures, tmp); \
	fn.glBindImageTextures(first, count, ptmp)
#else
#define FN_GLBINDIMAGETEXTURES(first, count, textures) \
	fn.glBindImageTextures(first, count, HostAddr(textures, const GLuint *))
#endif

#define FN_GLBINDVERTEXBUFFERS(first, count, buffers, offsets, sizes) \
	GLsizei const size = MAX(count, 0); \
	GLuint tmpbuffers[size], *pbuffers; \
	GLintptr tmpoffsets[size], *poffsets; \
	GLsizei tmpsiz[size], *psizes; \
	pbuffers = Atari2HostIntArray(count, buffers, tmpbuffers); \
	poffsets = Atari2HostIntptrArray(count, offsets, tmpoffsets); \
	psizes = Atari2HostIntArray(count, sizes, tmpsiz); \
	fn.glBindVertexBuffers(first, count, pbuffers, poffsets, psizes)

/* -------------------------------------------------------------------------- */

/*
 * Version 4.5
 */

#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV || NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLCLEARNAMEDBUFFERDATA(buffer, internalformat, format, type, data) \
	pixelBuffer buf(*this); \
	void *tmp = buf.convertPixels(1, 1, 1, format, type, (nfmemptr)data); \
	fn.glClearNamedBufferData(buffer, internalformat, format, type, tmp)
#else
#define FN_GLCLEARNAMEDBUFFERDATA(buffer, internalformat, format, type, data) \
	fn.glClearNamedBufferData(buffer, internalformat, format, type, HostAddr(data, const void *))
#endif

#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV || NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLCLEARNAMEDBUFFERSUBDATA(buffer, internalformat, offset, size, format, type, data) \
	pixelBuffer buf(*this); \
	void *tmp = buf.convertPixels(1, 1, 1, format, type, (nfmemptr)data); \
	fn.glClearNamedBufferSubData(buffer, internalformat, offset, size, format, type, tmp)
#else
#define FN_GLCLEARNAMEDBUFFERSUBDATA(buffer, internalformat, offset, size, format, type, data) \
	fn.glClearNamedBufferSubData(buffer, internalformat, offset, size, format, type, HostAddr(data, const void *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLCLEARNAMEDFRAMEBUFFERIV(framebuffer, buffer, drawbuffer, value) \
	GLint tmp[4], *ptmp; \
	GLuint size = 0; \
	switch (buffer) { \
		case GL_COLOR: size = 4; break; \
		case GL_STENCIL: \
		case GL_DEPTH: size = 1; break; \
		case GL_DEPTH_STENCIL: size = 2; break; \
	} \
	ptmp = Atari2HostIntArray(size, value, tmp); \
	fn.glClearNamedFramebufferiv(framebuffer, buffer, drawbuffer, ptmp)
#else
#define FN_GLCLEARNAMEDFRAMEBUFFERIV(framebuffer, buffer, drawbuffer, value) \
	fn.glClearNamedFramebufferiv(framebuffer, buffer, drawbuffer, HostAddr(value, const GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLCLEARNAMEDFRAMEBUFFERUIV(framebuffer, buffer, drawbuffer, value) \
	GLuint tmp[4], *ptmp; \
	GLuint size = 0; \
	switch (buffer) { \
		case GL_COLOR: size = 4; break; \
		case GL_STENCIL: \
		case GL_DEPTH: size = 1; break; \
		case GL_DEPTH_STENCIL: size = 2; break; \
	} \
	ptmp = Atari2HostIntArray(size, value, tmp); \
	fn.glClearNamedFramebufferuiv(framebuffer, buffer, drawbuffer, ptmp)
#else
#define FN_GLCLEARNAMEDFRAMEBUFFERUIV(framebuffer, buffer, drawbuffer, value) \
	fn.glClearNamedFramebufferuiv(framebuffer, buffer, drawbuffer, HostAddr(value, const GLuint *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLCLEARNAMEDFRAMEBUFFERFV(framebuffer, buffer, drawbuffer, value) \
	GLfloat tmp[4], *ptmp; \
	GLuint size = 0; \
	switch (buffer) { \
		case GL_COLOR: size = 4; break; \
		case GL_STENCIL: \
		case GL_DEPTH: size = 1; break; \
		case GL_DEPTH_STENCIL: size = 2; break; \
	} \
	ptmp = Atari2HostFloatArray(size, value, tmp); \
	fn.glClearNamedFramebufferfv(framebuffer, buffer, drawbuffer, ptmp)
#else
#define FN_GLCLEARNAMEDFRAMEBUFFERFV(framebuffer, buffer, drawbuffer, value) \
	fn.glClearNamedFramebufferfv(framebuffer, buffer, drawbuffer, HostAddr(value, const GLfloat *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLCOMPRESSEDTEXTURESUBIMAGE1D(texture, level, xoffset, width, format, imageSize, bits) \
	GLubyte tmp[MAX(imageSize, 0)], *ptmp; \
	ptmp = Atari2HostByteArray(sizeof(tmp), bits, tmp); \
	fn.glCompressedTextureSubImage1D(texture, level, xoffset, width, format, imageSize, ptmp)
#else
#define FN_GLCOMPRESSEDTEXTURESUBIMAGE1D(texture, level, xoffset, width, format, imageSize, bits) \
	fn.glCompressedTextureSubImage1D(texture, level, xoffset, width, format, imageSize, HostAddr(bits, const void *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLCOMPRESSEDTEXTURESUBIMAGE2D(texture, level, xoffset, yoffset, width, height, format, imageSize, bits) \
	GLubyte tmp[MAX(imageSize, 0)], *ptmp; \
	ptmp = Atari2HostByteArray(sizeof(tmp), bits, tmp); \
	fn.glCompressedTextureSubImage2D(texture, level, xoffset, yoffset, width, height, format, imageSize, ptmp)
#else
#define FN_GLCOMPRESSEDTEXTURESUBIMAGE2D(texture, level, xoffset, yoffset, width, height, format, imageSize, bits) \
	fn.glCompressedTextureSubImage2D(texture, level, xoffset, yoffset, width, height, format, imageSize, HostAddr(bits, const void *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLCOMPRESSEDTEXTURESUBIMAGE3D(texture, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, bits) \
	GLubyte tmp[MAX(imageSize, 0)], *ptmp; \
	ptmp = Atari2HostByteArray(sizeof(tmp), bits, tmp); \
	fn.glCompressedTextureSubImage3D(texture, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, ptmp)
#else
#define FN_GLCOMPRESSEDTEXTURESUBIMAGE3D(texture, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, bits) \
	fn.glCompressedTextureSubImage3D(texture, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, HostAddr(bits, const void *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLCREATEBUFFERS(n, buffers) \
	GLuint tmp[MAX(n, 0)]; \
	fn.glCreateBuffers(n, tmp); \
	Host2AtariIntArray(n, tmp, buffers)
#else
#define FN_GLCREATEBUFFERS(n, buffers) \
	fn.glCreateBuffers(n, HostAddr(buffers, GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLCREATEFRAMEBUFFERS(n, framebuffers) \
	GLuint tmp[MAX(n, 0)]; \
	fn.glCreateFramebuffers(n, tmp); \
	Host2AtariIntArray(n, tmp, framebuffers)
#else
#define FN_GLCREATEFRAMEBUFFERS(n, framebuffers) \
	fn.glCreateFramebuffers(n, HostAddr(framebuffers, GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLCREATEPROGRAMPIPELINES(n, pipelines) \
	GLuint tmp[MAX(n, 0)]; \
	fn.glCreateProgramPipelines(n, tmp); \
	Host2AtariIntArray(n, tmp, pipelines)
#else
#define FN_GLCREATEPROGRAMPIPELINES(n, pipelines) \
	fn.glCreateProgramPipelines(n, HostAddr(pipelines, GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLCREATEQUERIES(target, n, pipelines) \
	GLuint tmp[MAX(n, 0)]; \
	fn.glCreateQueries(target, n, tmp); \
	Host2AtariIntArray(n, tmp, pipelines)
#else
#define FN_GLCREATEQUERIES(target, n, pipelines) \
	fn.glCreateQueries(target, n, HostAddr(pipelines, GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLCREATERENDERBUFFERS(n, renderbuffers) \
	GLuint tmp[MAX(n, 0)]; \
	fn.glCreateRenderbuffers(n, tmp); \
	Host2AtariIntArray(n, tmp, renderbuffers)
#else
#define FN_GLCREATERENDERBUFFERS(n, renderbuffers) \
	fn.glCreateRenderbuffers(n, HostAddr(renderbuffers, GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLCREATESAMPLERS(n, samplers) \
	GLuint tmp[MAX(n, 0)]; \
	fn.glCreateSamplers(n, tmp); \
	Host2AtariIntArray(n, tmp, samplers)
#else
#define FN_GLCREATESAMPLERS(n, samplers) \
	fn.glCreateSamplers(n, HostAddr(samplers, GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLCREATETEXTURES(target, n, textures) \
	GLuint tmp[MAX(n, 0)]; \
	fn.glCreateTextures(target, n, tmp); \
	Host2AtariIntArray(n, tmp, textures)
#else
#define FN_GLCREATETEXTURES(target, n, textures) \
	fn.glCreateTextures(target, n, HostAddr(textures, GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLCREATETRANSFORMFEEDBACKS(n, ids) \
	GLuint tmp[MAX(n, 0)]; \
	fn.glCreateTransformFeedbacks(n, tmp); \
	Host2AtariIntArray(n, tmp, ids)
#else
#define FN_GLCREATETRANSFORMFEEDBACKS(n, ids) \
	fn.glCreateTransformFeedbacks(n, HostAddr(ids, GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLCREATEVERTEXARRAYS(n, arrays) \
	GLuint tmp[MAX(n, 0)]; \
	fn.glCreateVertexArrays(n, tmp); \
	Host2AtariIntArray(n, tmp, arrays)
#else
#define FN_GLCREATEVERTEXARRAYS(n, arrays) \
	fn.glCreateVertexArrays(n, HostAddr(arrays, GLuint *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLGETCOMPRESSEDTEXTUREIMAGE(texture, level, bufSize, pixels) \
	GLubyte tmp[MAX(bufSize, 0)]; \
	fn.glGetCompressedTextureImage(texture, level, bufSize, tmp); \
	Host2AtariByteArray(sizeof(tmp), tmp, pixels)
#else
#define FN_GLGETCOMPRESSEDTEXTUREIMAGE(texture, level, bufSize, pixels) \
	fn.glGetCompressedTextureImage(texture, level, bufSize, HostAddr(pixels, void *))
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLGETCOMPRESSEDTEXTURESUBIMAGE(texture, level, xoffset, yoffset, zoffset, width, height, depth, bufSize, pixels) \
	GLubyte tmp[MAX(bufSize, 0)]; \
	fn.glGetCompressedTextureSubImage(texture, level, xoffset, yoffset, zoffset, width, height, depth, bufSize, tmp); \
	Host2AtariByteArray(sizeof(tmp), tmp, pixels)
#else
#define FN_GLGETCOMPRESSEDTEXTURESUBIMAGE(texture, level, xoffset, yoffset, zoffset, width, height, depth, bufSize, pixels) \
	fn.glGetCompressedTextureSubImage(texture, level, xoffset, yoffset, zoffset, width, height, depth, bufSize, HostAddr(pixels, void *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETNAMEDBUFFERPARAMETERI64V(buffer, pname, params) \
	GLint const size = 1; \
	GLint64 tmp[MAX(size, 16)]; \
	fn.glGetNamedBufferParameteri64v(buffer, pname, tmp); \
	Host2AtariInt64Array(size, tmp, params)
#else
#define FN_GLGETNAMEDBUFFERPARAMETERI64V(buffer, pname, params) \
	fn.glGetNamedBufferParameteri64v(buffer, pname, HostAddr(params, GLint64 *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETNAMEDBUFFERPARAMETERIV(buffer, pname, params) \
	GLint const size = 1; \
	GLint tmp[MAX(size, 16)]; \
	fn.glGetNamedBufferParameteriv(buffer, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETNAMEDBUFFERPARAMETERIV(buffer, pname, params) \
	fn.glGetNamedBufferParameteriv(buffer, pname, HostAddr(params, GLint *))
#endif

#define FN_GLGETNAMEDBUFFERPOINTERV(buffer, pname, params) \
	void *tmp = NULL; \
	fn.glGetNamedBufferPointerv(buffer, pname, &tmp); \
	/* TODO */ \
	memptr zero = 0; \
	Host2AtariIntArray(1, &zero, AtariAddr(params, memptr *))

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETNAMEDFRAMEBUFFERATTACHMENTPARAMETERIV(framebuffer, attachment, pname, params) \
	GLint const size = 1; \
	GLint tmp[MAX(size, 16)]; \
	fn.glGetNamedFramebufferAttachmentParameteriv(framebuffer, attachment, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETNAMEDFRAMEBUFFERATTACHMENTPARAMETERIV(framebuffer, attachment, pname, params) \
	fn.glGetNamedFramebufferAttachmentParameteriv(framebuffer, attachment, pname, HostAddr(params, GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETNAMEDFRAMEBUFFERPARAMETERIV(framebuffer, pname, params) \
	GLint const size = 1; \
	GLint tmp[size]; \
	fn.glGetNamedFramebufferParameteriv(framebuffer, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETNAMEDFRAMEBUFFERPARAMETERIV(framebuffer, pname, params) \
	fn.glGetNamedFramebufferParameteriv(framebuffer, pname, HostAddr(params, GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETNAMEDRENDERBUFFERPARAMETERIV(renderbuffer, pname, params) \
	GLint const size = 1; \
	GLint tmp[size]; \
	fn.glGetNamedRenderbufferParameteriv(renderbuffer, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETNAMEDRENDERBUFFERPARAMETERIV(renderbuffer, pname, params) \
	fn.glGetNamedRenderbufferParameteriv(renderbuffer, pname, HostAddr(params, GLint *))
#endif

#define FN_GLGETTEXTUREIMAGE(texture, level, format, type, bufSize, img) \
	GLint width = 0, height = 1, depth = 1; \
	char *src; \
	nfmemptr dst; \
	pixelBuffer buf(*this); \
	GLint target = 0; \
	 \
	fn.glGetTextureParameteriv(texture, GL_TEXTURE_TARGET, &target); \
	switch (target) { \
	case GL_TEXTURE_1D: \
	case GL_PROXY_TEXTURE_1D: \
		fn.glGetTextureLevelParameteriv(texture, level, GL_TEXTURE_WIDTH, &width); \
		break; \
	case GL_TEXTURE_2D: \
	case GL_PROXY_TEXTURE_2D: \
	case GL_TEXTURE_CUBE_MAP_POSITIVE_X: \
	case GL_TEXTURE_CUBE_MAP_POSITIVE_Y: \
	case GL_TEXTURE_CUBE_MAP_POSITIVE_Z: \
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_X: \
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y: \
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z: \
	case GL_PROXY_TEXTURE_CUBE_MAP: \
		fn.glGetTextureLevelParameteriv(texture, level, GL_TEXTURE_WIDTH, &width); \
		fn.glGetTextureLevelParameteriv(texture, level, GL_TEXTURE_HEIGHT, &height); \
		break; \
	case GL_TEXTURE_3D: \
	case GL_PROXY_TEXTURE_3D: \
		fn.glGetTextureLevelParameteriv(texture, level, GL_TEXTURE_WIDTH, &width); \
		fn.glGetTextureLevelParameteriv(texture, level, GL_TEXTURE_HEIGHT, &height); \
		fn.glGetTextureLevelParameteriv(texture, level, GL_TEXTURE_DEPTH, &depth); \
		break; \
	default: \
		glSetError(GL_INVALID_ENUM); \
		return; \
	} \
	if (contexts[cur_context].buffer_bindings.pixel_pack.id) { \
		void *offset = NFHost2AtariAddr(img); \
		fn.glGetTextureImage(texture, level, format, type, bufSize, offset); \
		src = contexts[cur_context].buffer_bindings.pixel_pack.host_pointer + (uintptr_t)offset; \
		dst = contexts[cur_context].buffer_bindings.pixel_pack.atari_pointer + (uintptr_t)offset; \
		if (!buf.params(bufSize, format, type)) return; \
	} else { \
		src = buf.hostBuffer(bufSize, format, type, (nfmemptr)img); \
		if (src == NULL) return; \
		fn.glGetTextureImage(texture, level, format, type, bufSize, src); \
		dst = (nfmemptr)img; \
	} \
	buf.convertToAtari(src, dst)

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETTEXTURELEVELPARAMETERIV(texture, level, pname, params) \
	GLint const size = 1; \
	GLint tmp[MAX(size, 16)]; \
	fn.glGetTextureLevelParameteriv(texture, level, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETTEXTURELEVELPARAMETERIV(texture, level, pname, params) \
	fn.glGetTextureLevelParameteriv(texture, level, pname, HostAddr(params, GLint *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETTEXTURELEVELPARAMETERFV(texture, level, pname, params) \
	GLint const size = 1; \
	GLfloat tmp[MAX(size, 16)]; \
	fn.glGetTextureLevelParameterfv(texture, level, pname, tmp); \
	Host2AtariFloatArray(size, tmp, params)
#else
#define FN_GLGETTEXTURELEVELPARAMETERFV(texture, level, pname, params) \
	fn.glGetTextureLevelParameterfv(texture, level, pname, HostAddr(params, GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETTEXTUREPARAMETERIIV(texunit, pname, params) \
	GLint const size = 1; \
	GLint tmp[MAX(size, 16)]; \
	fn.glGetTextureParameterIiv(texunit, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETTEXTUREPARAMETERIIV(texunit, pname, params) \
	fn.glGetTextureParameterIiv(texunit, pname, HostAddr(params, GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETTEXTUREPARAMETERIUIV(texunit, pname, params) \
	GLint const size = 1; \
	GLuint tmp[MAX(size, 16)]; \
	fn.glGetTextureParameterIuiv(texunit, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETTEXTUREPARAMETERIUIV(texunit, pname, params) \
	fn.glGetTextureParameterIuiv(texunit, pname, HostAddr(params, GLuint *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETTEXTUREPARAMETERFV(texture, pname, params) \
	GLint const size = 1; \
	GLfloat tmp[MAX(size, 16)]; \
	fn.glGetTextureParameterfv(texture, pname, tmp); \
	Host2AtariFloatArray(size, tmp, params)
#else
#define FN_GLGETTEXTUREPARAMETERFV(texture, pname, params) \
	fn.glGetTextureParameterfv(texture, pname, HostAddr(params, GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETTEXTUREPARAMETERIV(texture, pname, params) \
	GLint const size = 1; \
	GLint tmp[MAX(size, 16)]; \
	fn.glGetTextureParameteriv(texture, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETTEXTUREPARAMETERIV(texture, pname, params) \
	fn.glGetTextureParameteriv(texture, pname, HostAddr(params, GLint *))
#endif

#define FN_GLGETTEXTURESUBIMAGE(texture, level, xoffset, yoffset, zoffset, width, height, depth, format, type, bufSize, img) \
	char *src; \
	nfmemptr dst; \
	pixelBuffer buf(*this); \
	 \
	if (contexts[cur_context].buffer_bindings.pixel_pack.id) { \
		void *offset = NFHost2AtariAddr(img); \
		fn.glGetTextureSubImage(texture, level, xoffset, yoffset, zoffset, width, height, depth, format, type, bufSize, offset); \
		src = contexts[cur_context].buffer_bindings.pixel_pack.host_pointer + (uintptr_t)offset; \
		dst = contexts[cur_context].buffer_bindings.pixel_pack.atari_pointer + (uintptr_t)offset; \
		if (!buf.params(bufSize, format, type)) return; \
	} else { \
		src = buf.hostBuffer(bufSize, format, type, (nfmemptr)img); \
		if (src == NULL) return; \
		fn.glGetTextureSubImage(texture, level, xoffset, yoffset, zoffset, width, height, depth, format, type, bufSize, src); \
		dst = (nfmemptr)img; \
	} \
	buf.convertToAtari(src, dst)

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETTRANSFORMFEEDBACKI64_V(xfb, pname, index, params) \
	GLint const size = 1; \
	GLint64 tmp[size]; \
	fn.glGetTransformFeedbacki64_v(xfb, pname, index, tmp); \
	Host2AtariInt64Array(size, tmp, params)
#else
#define FN_GLGETTRANSFORMFEEDBACKI64_V(xfb, pname, index, params) \
	fn.glGetTransformFeedbacki64_v(xfb, pname, index, HostAddr(params, GLint64 *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETTRANSFORMFEEDBACKI_V(xfb, pname, index, params) \
	GLint const size = 1; \
	GLint tmp[size]; \
	fn.glGetTransformFeedbacki_v(xfb, pname, index, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETTRANSFORMFEEDBACKI_V(xfb, pname, index, params) \
	fn.glGetTransformFeedbacki_v(xfb, pname, index, HostAddr(params, GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETTRANSFORMFEEDBACKIV(xfb, pname, params) \
	GLint const size = 1; \
	GLint tmp[size]; \
	fn.glGetTransformFeedbackiv(xfb, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETTRANSFORMFEEDBACKIV(xfb, pname, params) \
	fn.glGetTransformFeedbackiv(xfb, pname, HostAddr(params, GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETVERTEXARRAYINDEXED64IV(vaobj, index, pname, params) \
	GLint const size = 1; \
	GLint64 tmp[size]; \
	fn.glGetVertexArrayIndexed64iv(vaobj, index, pname, tmp); \
	Host2AtariInt64Array(size, tmp, params)
#else
#define FN_GLGETVERTEXARRAYINDEXED64IV(vaobj, index, pname, params) \
	fn.glGetVertexArrayIndexed64iv(vaobj, index, pname, HostAddr(params, GLint64 *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETVERTEXARRAYINDEXEDIV(vaobj, index, pname, params) \
	GLint const size = 1; \
	GLint tmp[size]; \
	fn.glGetVertexArrayIndexediv(vaobj, index, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETVERTEXARRAYINDEXEDIV(vaobj, index, pname, params) \
	fn.glGetVertexArrayIndexediv(vaobj, index, pname, HostAddr(params, GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETVERTEXARRAYIV(vaobj, pname, params) \
	GLint const size = 1; \
	GLint tmp[size]; \
	fn.glGetVertexArrayiv(vaobj, pname, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETVERTEXARRAYIV(vaobj, pname, params) \
	fn.glGetVertexArrayiv(vaobj, pname, HostAddr(params, GLint *))
#endif

#define FN_GLGETNCOLORTABLE(target, format, type, bufSize, table) \
	char *src; \
	nfmemptr dst; \
	pixelBuffer buf(*this); \
	 \
	if (contexts[cur_context].buffer_bindings.pixel_pack.id) { \
		void *offset = NFHost2AtariAddr(table); \
		fn.glGetColorTableEXT(target, format, type, offset); \
		src = contexts[cur_context].buffer_bindings.pixel_pack.host_pointer + (uintptr_t)offset; \
		dst = contexts[cur_context].buffer_bindings.pixel_pack.atari_pointer + (uintptr_t)offset; \
		if (!buf.params(bufSize, format, type)) return;\
	} else { \
		src = buf.hostBuffer(bufSize, format, type, (nfmemptr)table); \
		if (src == NULL) return; \
		fn.glGetnColorTable(target, format, type, bufSize, src); \
		dst = (nfmemptr)table; \
	} \
	buf.convertToAtari(src, dst)

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLGETNCOMPRESSEDTEXIMAGE(target, lod, bufSize, img) \
	GLubyte tmp[MAX(bufSize, 0)]; \
	fn.glGetnCompressedTexImage(target, lod, bufSize, tmp); \
	Host2AtariByteArray(bufSize, tmp, img)
#else
#define FN_GLGETNCOMPRESSEDTEXIMAGE(target, lod, bufSize, img) \
	fn.glGetnCompressedTexImage(target, lod, bufSize, HostAddr(img, void *))
#endif

#define FN_GLGETNCONVOLUTIONFILTER(target, format, type, bufSize, image) \
	char *src; \
	nfmemptr dst; \
	pixelBuffer buf(*this); \
	 \
	if (contexts[cur_context].buffer_bindings.pixel_pack.id) { \
		void *offset = NFHost2AtariAddr(image); \
		fn.glGetnConvolutionFilter(target, format, type, bufSize, offset); \
		src = contexts[cur_context].buffer_bindings.pixel_pack.host_pointer + (uintptr_t)offset; \
		dst = contexts[cur_context].buffer_bindings.pixel_pack.atari_pointer + (uintptr_t)offset; \
		if (!buf.params(bufSize, format, type)) return; \
	} else { \
		src = buf.hostBuffer(bufSize, format, type, (nfmemptr)image); \
		if (src == NULL) return; \
		fn.glGetnConvolutionFilter(target, format, type, bufSize, src); \
		dst = (nfmemptr)image; \
	} \
	buf.convertToAtari(src, dst)

#define FN_GLGETNHISTOGRAM(target, reset, format, type, bufSize, values) \
	GLint width = 0; \
	char *src; \
	nfmemptr dst; \
	pixelBuffer buf(*this); \
	 \
	fn.glGetHistogramParameteriv(target, GL_HISTOGRAM_WIDTH, &width); \
	if (contexts[cur_context].buffer_bindings.pixel_pack.id) { \
		void *offset = NFHost2AtariAddr(values); \
		fn.glGetnHistogram(target, reset, format, type, bufSize, offset); \
		src = contexts[cur_context].buffer_bindings.pixel_pack.host_pointer + (uintptr_t)offset; \
		dst = contexts[cur_context].buffer_bindings.pixel_pack.atari_pointer + (uintptr_t)offset; \
		if (!buf.params(bufSize, format, type)) return; \
	} else { \
		src = buf.hostBuffer(bufSize, format, type, (nfmemptr)values); \
		if (src == NULL) return; \
		fn.glGetnHistogram(target, reset, format, type, bufSize, src); \
		dst = (nfmemptr)values; \
	} \
	buf.convertToAtari(src, dst)

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLGETNMAPDV(target, query, bufSize, v) \
	GLint const size = bufSize / ATARI_SIZEOF_DOUBLE; \
	if (size <= 0) return; \
	GLdouble tmp[size]; \
	fn.glGetnMapdv(target, query, sizeof(tmp), tmp); \
	Host2AtariDoubleArray(size, tmp, v)
#else
#define FN_GLGETNMAPDV(target, query, bufSize, v) \
	fn.glGetnMapdv(target, query, bufSize, HostAddr(v, GLdouble *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETNMAPFV(target, query, bufSize, v) \
	GLint const size = bufSize / ATARI_SIZEOF_DOUBLE; \
	if (size <= 0) return; \
	GLfloat tmp[size]; \
	fn.glGetnMapfv(target, query, sizeof(tmp), tmp); \
	Host2AtariFloatArray(size, tmp, v)
#else
#define FN_GLGETNMAPFV(target, query, bufSize, v) \
	fn.glGetnMapfv(target, query, bufSize, HostAddr(v, GLfloat *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETNMAPIV(target, query, bufSize, v) \
	GLint const size = bufSize / ATARI_SIZEOF_DOUBLE; \
	if (size <= 0) return; \
	GLint tmp[size]; \
	fn.glGetnMapiv(target, query, sizeof(tmp), tmp); \
	Host2AtariIntArray(size, tmp, v)
#else
#define FN_GLGETNMAPIV(target, query, bufSize, v) \
	fn.glGetnMapiv(target, query, bufSize, HostAddr(v, GLint *))
#endif

#define FN_GLGETNMINMAX(target, reset, format, type, bufSize, values) \
	char *src; \
	nfmemptr dst; \
	pixelBuffer buf(*this); \
	 \
	if (!buf.params(bufSize, format, type)) return; \
	char result[bufSize]; \
	if (contexts[cur_context].buffer_bindings.pixel_pack.id) { \
		void *offset = NFHost2AtariAddr(values); \
		fn.glGetnMinmax(target, reset, format, type, bufSize, offset); \
		src = contexts[cur_context].buffer_bindings.pixel_pack.host_pointer + (uintptr_t)offset; \
		dst = contexts[cur_context].buffer_bindings.pixel_pack.atari_pointer + (uintptr_t)offset; \
	} else { \
		src = result; \
		fn.glGetnMinmax(target, reset, format, type, bufSize, src); \
		dst = (nfmemptr)values; \
	} \
	buf.convertToAtari(src, dst)

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETNPIXELMAPFV(map, bufSize, v) \
	GLint const size = bufSize / ATARI_SIZEOF_FLOAT; \
	if (size <= 0) return; \
	GLfloat tmp[size]; \
	fn.glGetnPixelMapfv(map, sizeof(tmp), tmp); \
	Host2AtariFloatArray(size, tmp, v)
#else
#define FN_GLGETNPIXELMAPFV(map, bufSize, v) \
	fn.glGetnPixelMapfv(map, bufSize, HostAddr(v, GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETNPIXELMAPUIV(map, bufSize, v) \
	GLint const size = bufSize / sizeof(Uint32); \
	if (size <= 0) return; \
	GLuint tmp[size]; \
	fn.glGetnPixelMapuiv(map, sizeof(tmp), tmp); \
	Host2AtariIntArray(size, tmp, v)
#else
#define FN_GLGETNPIXELMAPUIV(map, bufSize, v) \
	fn.glGetnPixelMapuiv(map, bufSize, HostAddr(v, GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETNPIXELMAPUSV(map, bufSize, v) \
	GLint const size = bufSize / sizeof(Uint16); \
	if (size <= 0) return; \
	GLushort tmp[size]; \
	fn.glGetnPixelMapusv(map, sizeof(tmp), tmp); \
	Host2AtariShortArray(size, tmp, v)
#else
#define FN_GLGETNPIXELMAPUSV(map, bufSize, v) \
	fn.glGetnPixelMapusv(map, bufSize, HostAddr(v, GLushort *))
#endif

/*
 * FIXME: If a non-zero named buffer object is bound to the GL_PIXEL_PACK_BUFFER target 
            (see glBindBuffer) while a polygon stipple pattern is
            requested, pattern is treated as a byte offset into the buffer object's data store.
        
 */
#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLGETNPOLYGONSTIPPLE(bufSize, mask) \
	GLubyte tmp[bufSize]; \
	fn.glGetnPolygonStipple(bufSize, tmp); \
	Host2AtariByteArray(bufSize, tmp, mask)
#else
#define FN_GLGETNPOLYGONSTIPPLE(bufSize, mask) \
	fn.glGetnPolygonStipple(bufSize, HostAddr(mask, GLubyte *))
#endif

#define FN_GLGETNSEPARABLEFILTER(target, format, type, rowBufSize, row, columnBufSize, column, span) \
	pixelBuffer rowbuf(*this); \
	pixelBuffer colbuf(*this); \
	char *srcrow; \
	nfmemptr dstrow; \
	char *srccol; \
	nfmemptr dstcol; \
	 \
	if (contexts[cur_context].buffer_bindings.pixel_pack.id) { \
		void *rowoffset = NFHost2AtariAddr(row); \
		void *coloffset = NFHost2AtariAddr(column); \
		fn.glGetnSeparableFilter(target, format, type, rowBufSize, rowoffset, columnBufSize, coloffset, HostAddr(span, void *)); \
		srcrow = contexts[cur_context].buffer_bindings.pixel_pack.host_pointer + (uintptr_t)rowoffset; \
		srccol = contexts[cur_context].buffer_bindings.pixel_pack.host_pointer + (uintptr_t)coloffset; \
		dstrow = contexts[cur_context].buffer_bindings.pixel_pack.atari_pointer + (uintptr_t)rowoffset; \
		dstcol = contexts[cur_context].buffer_bindings.pixel_pack.atari_pointer + (uintptr_t)coloffset; \
		if (!rowbuf.params(rowBufSize, format, type)) return; \
		if (!colbuf.params(columnBufSize, format, type)) return; \
	} else { \
		srcrow = rowbuf.hostBuffer(rowBufSize, format, type, (nfmemptr)row); \
		srccol = colbuf.hostBuffer(columnBufSize, format, type, (nfmemptr)column); \
		if (srcrow == NULL || srccol == NULL) return; \
		fn.glGetnSeparableFilter(target, format, type, rowBufSize, srcrow, columnBufSize, srccol, HostAddr(span, void *)); \
		dstrow = (nfmemptr)row; \
		dstcol = (nfmemptr)column; \
	} \
	rowbuf.convertToAtari(srcrow, dstrow); \
	colbuf.convertToAtari(srccol, dstcol)

#define FN_GLGETNTEXIMAGE(target, level, format, type, bufSize, img) \
	GLint width = 0, height = 1, depth = 1; \
	char *src; \
	nfmemptr dst; \
	pixelBuffer buf(*this); \
	 \
	switch (target) { \
	case GL_TEXTURE_1D: \
	case GL_PROXY_TEXTURE_1D: \
		fn.glGetTexLevelParameteriv(target, level, GL_TEXTURE_WIDTH, &width); \
		break; \
	case GL_TEXTURE_2D: \
	case GL_PROXY_TEXTURE_2D: \
	case GL_TEXTURE_CUBE_MAP_POSITIVE_X: \
	case GL_TEXTURE_CUBE_MAP_POSITIVE_Y: \
	case GL_TEXTURE_CUBE_MAP_POSITIVE_Z: \
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_X: \
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y: \
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z: \
	case GL_PROXY_TEXTURE_CUBE_MAP: \
		fn.glGetTexLevelParameteriv(target, level, GL_TEXTURE_WIDTH, &width); \
		fn.glGetTexLevelParameteriv(target, level, GL_TEXTURE_HEIGHT, &height); \
		break; \
	case GL_TEXTURE_3D: \
	case GL_PROXY_TEXTURE_3D: \
		fn.glGetTexLevelParameteriv(target, level, GL_TEXTURE_WIDTH, &width); \
		fn.glGetTexLevelParameteriv(target, level, GL_TEXTURE_HEIGHT, &height); \
		fn.glGetTexLevelParameteriv(target, level, GL_TEXTURE_DEPTH, &depth); \
		break; \
	default: \
		glSetError(GL_INVALID_ENUM); \
		return; \
	} \
	if (contexts[cur_context].buffer_bindings.pixel_pack.id) { \
		void *offset = NFHost2AtariAddr(img); \
		fn.glGetnTexImage(target, level, format, type, bufSize, offset); \
		src = contexts[cur_context].buffer_bindings.pixel_pack.host_pointer + (uintptr_t)offset; \
		dst = contexts[cur_context].buffer_bindings.pixel_pack.atari_pointer + (uintptr_t)offset; \
		if (!buf.params(width, height, depth, format, type)) return; \
	} else { \
		src = buf.hostBuffer(width, height, depth, format, type, (nfmemptr)img); \
		if (src == NULL) return; \
		fn.glGetnTexImage(target, level, format, type, bufSize, src); \
		dst = (nfmemptr)img; \
	} \
	buf.convertToAtari(src, dst)

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLGETNUNIFORMDV(program, location, bufSize, params) \
	GLint const size = bufSize / ATARI_SIZEOF_DOUBLE; \
	if (size <= 0) return; \
	GLdouble tmp[size]; \
	fn.glGetnUniformdv(program, location, size, tmp); \
	Host2AtariDoubleArray(size, tmp, params)
#else
#define FN_GLGETNUNIFORMDV(program, location, bufSize, params) \
	fn.glGetnUniformdv(program, location, bufSize, HostAddr(params, GLdouble *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETNUNIFORMFV(program, location, bufSize, params) \
	GLint const size = bufSize / ATARI_SIZEOF_FLOAT; \
	if (size <= 0) return; \
	GLfloat tmp[size]; \
	fn.glGetnUniformfv(program, location, size, tmp); \
	Host2AtariFloatArray(size, tmp, params)
#else
#define FN_GLGETNUNIFORMFV(program, location, bufSize, params) \
	fn.glGetnUniformfv(program, location, bufSize, HostAddr(params, GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETNUNIFORMIV(program, location, bufSize, params) \
	GLint const size = bufSize / sizeof(GLint); \
	if (size <= 0) return; \
	GLint tmp[size]; \
	fn.glGetnUniformiv(program, location, size, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETNUNIFORMIV(program, location, bufSize, params) \
	fn.glGetnUniformiv(program, location, bufSize, HostAddr(params, GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETNUNIFORMUIV(program, location, bufSize, params) \
	GLint const size = bufSize / sizeof(GLuint); \
	if (size <= 0) return; \
	GLuint tmp[size]; \
	fn.glGetnUniformuiv(program, location, size, tmp); \
	Host2AtariIntArray(size, tmp, params)
#else
#define FN_GLGETNUNIFORMUIV(program, location, bufSize, params) \
	fn.glGetnUniformuiv(program, location, bufSize, HostAddr(params, GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLINVALIDATENAMEDFRAMEBUFFERDATA(framebuffer, numAttachments, attachments) \
	GLuint tmp[MAX(numAttachments, 0)], *ptmp; \
	ptmp = Atari2HostIntArray(numAttachments, attachments, tmp); \
	fn.glInvalidateNamedFramebufferData(framebuffer, numAttachments, ptmp)
#else
#define FN_GLINVALIDATENAMEDFRAMEBUFFERDATA(framebuffer, numAttachments, attachments) \
	fn.glInvalidateNamedFramebufferData(framebuffer, numAttachments, HostAddr(attachments, const GLuint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLINVALIDATENAMEDFRAMEBUFFERSUBDATA(framebuffer, numAttachments, attachments, x, y, width, height) \
	GLuint tmp[MAX(numAttachments, 0)], *ptmp; \
	ptmp = Atari2HostIntArray(numAttachments, attachments, tmp); \
	fn.glInvalidateNamedFramebufferSubData(framebuffer, numAttachments, ptmp, x, y, width, height)
#else
#define FN_GLINVALIDATENAMEDFRAMEBUFFERSUBDATA(framebuffer, numAttachments, attachments, x, y, width, height) \
	fn.glInvalidateNamedFramebufferSubData(framebuffer, numAttachments, HostAddr(attachments, const GLuint *), x, y, width, height)
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLNAMEDBUFFERDATA(buffer, size, data, usage) \
	GLubyte tmp[MAX(size, 0)], *ptmp; \
	ptmp = Atari2HostByteArray(sizeof(tmp), data, tmp); \
	fn.glNamedBufferData(buffer, size, ptmp, usage)
#else
#define FN_GLNAMEDBUFFERDATA(buffer, size, data, usage) \
	fn.glNamedBufferData(buffer, size, HostAddr(data, const void *), usage)
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLNAMEDBUFFERSTORAGE(buffer, size, data, flags) \
	GLubyte tmp[MAX(size, 0)], *ptmp; \
	ptmp = Atari2HostByteArray(sizeof(tmp), data, tmp); \
	fn.glNamedBufferStorage(buffer, size, ptmp, flags)
#else
#define FN_GLNAMEDBUFFERSTORAGE(buffer, size, data, flags) \
	fn.glNamedBufferStorage(buffer, size, HostAddr(data, const void *), flags)
#endif

#if NFOSMESA_NEED_BYTE_CONV
#define FN_GLNAMEDBUFFERSUBDATA(buffer, offset, size, data) \
	GLubyte tmp[MAX(size, 0)], *ptmp; \
	ptmp = Atari2HostByteArray(sizeof(tmp), data, tmp); \
	fn.glNamedBufferSubData(buffer, offset, size, ptmp)
#else
#define FN_GLNAMEDBUFFERSUBDATA(buffer, offset, size, data) \
	fn.glNamedBufferSubData(buffer, offset, size, HostAddr(data, const void *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLNAMEDFRAMEBUFFERDRAWBUFFERS(framebuffer, n, bufs) \
	GLuint tmp[MAX(n, 0)], *ptmp; \
	ptmp = Atari2HostIntArray(n, bufs, tmp); \
	fn.glNamedFramebufferDrawBuffers(framebuffer, n, ptmp)
#else
#define FN_GLNAMEDFRAMEBUFFERDRAWBUFFERS(framebuffer, n, bufs) \
	fn.glNamedFramebufferDrawBuffers(framebuffer, n, HostAddr(bufs, const GLuint *))
#endif

#define FN_GLREADNPIXELS(x, y, width, height, format, type, bufSize, img) \
	char *src; \
	nfmemptr dst; \
	pixelBuffer buf(*this); \
	 \
	if (contexts[cur_context].buffer_bindings.pixel_pack.id) { \
		void *offset = NFHost2AtariAddr(img); \
		fn.glReadnPixels(x, y, width, height, format, type, bufSize, offset); \
		src = contexts[cur_context].buffer_bindings.pixel_pack.host_pointer + (uintptr_t)offset; \
		dst = contexts[cur_context].buffer_bindings.pixel_pack.atari_pointer + (uintptr_t)offset; \
		if (!buf.params(bufSize, format, type)) return; \
	} else { \
		src = buf.hostBuffer(bufSize, format, type, (nfmemptr)img); \
		if (src == NULL) return; \
		fn.glReadnPixels(x, y, width, height, format, type, bufSize, src); \
		dst = (nfmemptr)img; \
	} \
	buf.convertToAtari(src, dst)

#if NFOSMESA_NEED_INT_CONV
#define FN_GLTEXTUREPARAMETERIIV(texunit, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLint tmp[MAX(size, 16)]; \
	Atari2HostIntArray(size, params, tmp); \
	fn.glTextureParameterIiv(texunit, pname, tmp)
#else
#define FN_GLTEXTUREPARAMETERIIV(texunit, pname, params) \
	fn.glTextureParameterIiv(texunit, pname, HostAddr(params, GLint *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLTEXTUREPARAMETERIUIV(texunit, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLuint tmp[MAX(size, 16)]; \
	Atari2HostIntArray(size, params, tmp); \
	fn.glTextureParameterIuiv(texunit, pname, tmp)
#else
#define FN_GLTEXTUREPARAMETERIUIV(texunit, pname, params) \
	fn.glTextureParameterIuiv(texunit, pname, HostAddr(params, const GLuint *))
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLTEXTUREPARAMETERFV(texture, pname, params) \
	GLint const size = 1; \
	GLfloat tmp[MAX(size, 16)], *ptmp; \
	ptmp = Atari2HostFloatArray(size, params, tmp); \
	fn.glTextureParameterfv(texture, pname, ptmp)
#else
#define FN_GLTEXTUREPARAMETERFV(texture, pname, params) \
	fn.glTextureParameterfv(texture, pname, HostAddr(params, const GLfloat *))
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLTEXTUREPARAMETERIV(texture, pname, params) \
	GLint const size = 1; \
	GLint tmp[MAX(size, 16)], *ptmp; \
	ptmp = Atari2HostIntArray(size, params, tmp); \
	fn.glTextureParameteriv(texture, pname, ptmp)
#else
#define FN_GLTEXTUREPARAMETERIV(texture, pname, params) \
	fn.glTextureParameteriv(texture, pname, HostAddr(params, const GLint *))
#endif

#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLTEXTURESUBIMAGE1D(texture, level, xoffset, width, format, type, pixels) \
	pixelBuffer buf(*this); \
	void *tmp = buf.convertPixels(width, 1, 1, format, type, (nfmemptr)pixels); \
	fn.glTextureSubImage1D(texture, level, xoffset, width, format, type, tmp)
#else
#define FN_GLTEXTURESUBIMAGE1D(texture, level, xoffset, width, format, type, pixels) \
	fn.glTextureSubImage1D(texture, level, xoffset, width, format, type, HostAddr(pixels, const void *))
#endif

#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLTEXTURESUBIMAGE2D(texture, level, xoffset, yoffset, width, height, format, type, pixels) \
	pixelBuffer buf(*this); \
	void *tmp = buf.convertPixels(width, height, 1, format, type, (nfmemptr)pixels); \
	fn.glTextureSubImage2D(texture, level, xoffset, yoffset, width, height, format, type, tmp)
#else
#define FN_GLTEXTURESUBIMAGE2D(texture, level, xoffset, yoffset, width, height, format, type, pixels) \
	fn.glTextureSubImage2D(texture, level, xoffset, yoffset, width, height, format, type, HostAddr(pixels, const void *))
#endif

#if NFOSMESA_NEED_BYTE_CONV || NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLTEXTURESUBIMAGE3D(texture, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels) \
	pixelBuffer buf(*this); \
	void *tmp = buf.convertPixels(width, height, depth, format, type, (nfmemptr)pixels); \
	fn.glTextureSubImage3D(texture, level, xoffset, yoffset, zoffset, width, height, depth, format, type, tmp)
#else
#define FN_GLTEXTURESUBIMAGE3D(texture, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels) \
	fn.glTextureSubImage3D(texture, level, xoffset, yoffset, zoffset, width, height, depth, format, type, HostAddr(pixels, const void *))
#endif

#define FN_GLVERTEXARRAYVERTEXBUFFERS(vaobj, first, count, buffers, offsets, strides) \
	GLsizei const size = MAX(count, 0); \
	GLuint tmpbufs[size], *pbufs; \
	GLsizei tmpstrides[size], *pstrides; \
	GLintptr tmpoffs[size], *poffsets; \
	pbufs = Atari2HostIntArray(count, buffers, tmpbufs); \
	poffsets = Atari2HostIntptrArray(count, offsets, tmpoffs); \
	pstrides = Atari2HostIntArray(count, strides, tmpstrides); \
	fn.glVertexArrayVertexBuffers(vaobj, first, count, pbufs, poffsets, pstrides)

/* -------------------------------------------------------------------------- */

/*
 * Version 4.6
 */

#define FN_GLMULTIDRAWARRAYSINDIRECTCOUNT(mode, indirect, drawcount, maxdrawcount, stride) \
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
		void *offset = NFHost2AtariAddr(indirect); \
		fn.glMultiDrawArraysIndirectCount(mode, offset, drawcount, maxdrawcount, stride); \
	} else if (indirect) { \
		if (stride == 0) stride = 4 * sizeof(Uint32); \
		nfcmemptr indptr = (nfcmemptr)indirect; \
		for (GLsizei n = 0; n < maxdrawcount; n++) { \
			GLuint tmp[4] = { 0, 0, 0, 0 }; \
			Atari2HostIntArray(4, indptr, tmp); \
			GLuint count = tmp[0]; \
			convertClientArrays(count); \
			fn.glDrawArraysInstancedBaseInstance(mode, tmp[2], count, tmp[1], tmp[3]); \
			indptr += stride; \
		} \
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
 *    } DrawElemntsIndirectCommand;
 */
#define FN_GLMULTIDRAWELEMENTSINDIRECTCOUNT(mode, type, indirect, drawcount, maxdrawcount, stride) \
	if (contexts[cur_context].buffer_bindings.draw_indirect.id) { \
		void *offset = NFHost2AtariAddr(indirect); \
		fn.glMultiDrawElementsIndirectCount(mode, type, offset, drawcount, maxdrawcount, stride); \
	} else if (indirect) { \
		if (stride == 0) stride = 5 * sizeof(Uint32); \
		nfcmemptr pind = (nfcmemptr)indirect; \
		for (GLsizei n = 0; n < maxdrawcount; n++) { \
			GLuint tmp[5] = { 0, 0, 0, 0, 0 }; \
			Atari2HostIntArray(5, pind, tmp); \
			GLuint count = tmp[0]; \
			convertClientArrays(count); \
			fn.glDrawElementsInstancedBaseVertexBaseInstance(mode, count, type, (const void *)(uintptr_t)tmp[2], tmp[1], tmp[3], tmp[4]); \
			pind += stride; \
		} \
	}

/* -------------------------------------------------------------------------- */

#include "nfosmesa/call-gl.c"

#endif
