#ifndef SDL_OPENGL_WRAPPER_H
#define SDL_OPENGL_WRAPPER_H

#if defined(ENABLE_OPENGL)

/* on Cygwin we need the WIN32 version of the api */
#ifdef __CYGWIN__
#  include <wchar.h>
#  define __WIN32__ 1
#  define _WIN32 1
#  define __MINGW32__ 1
#  ifndef _DLL
#    define _DLL 1
#    define __dll_defined_here
#  endif
#  undef __CYGWIN__
#  define __cygwin_undefined_here
#  define __STRALIGN_H_ 1
#endif

#define GL_GLEXT_LEGACY
#include <SDL_opengl.h>

#if defined(__MACOSX__)
#include <OpenGL/gl.h>	/* Header File For The OpenGL Library */
#include <OpenGL/glu.h>	/* Header File For The GLU Library */
#elif defined(__MACOS__)
#include <gl.h>		/* Header File For The OpenGL Library */
#include <glu.h>	/* Header File For The GLU Library */
#else
#include <GL/gl.h>	/* Header File For The OpenGL Library */
#include <GL/glu.h>	/* Header File For The GLU Library */
#endif
#include "glenums.h"

/*	On darwin/Mac OS X systems SDL_opengl.h includes OpenGL/gl.h instead of GL/gl.h, 
   	which does not define GLAPI and GLAPIENTRY used by GL/osmesa.h
*/
#if defined(__gl_h_) 
	#if !defined(GLAPI)
		#define GLAPI
	#endif
	#if !defined(GLAPIENTRY)
		#define GLAPIENTRY
	#endif
#endif

#ifdef __cygwin_undefined_here
#  undef __WIN32__
#  undef _WIN32
#  undef __MINGW32__
#  ifdef __dll_defined_here
#    undef _DLL
#    undef __dll_defined_here
#  endif
#  define __CYGWIN__ 1
#  undef __cygwin_undefined_here
#endif

#endif /* ENABLE_OPENGL */

#endif /* SDL_OPENGL_WRAPPER_H */
