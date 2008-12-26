/*
	OpenGL dynamic loader

	(C) 2006 Patrice Mandin

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

#ifndef DYNGL_H
#define DYNGL_H 1

#ifdef __cplusplus
extern "C" {
#endif

/*--- Defines ---*/

#ifndef GL_VERSION_2_0
typedef char GLchar;
#endif

#ifndef GL_MESA_program_debug
typedef void (*GLprogramcallbackMESA)(GLenum target, GLvoid *data);
#endif

#ifndef GL_EXT_timer_query
typedef int64_t GLint64EXT;
typedef uint64_t GLuint64EXT;
#endif

#ifdef WIN32
#define STDCALL __stdcall
#else
#define STDCALL
#endif

/*--- Structures ---*/

typedef struct {
	#include "dyngl_gl.h"
	#include "dyngl_glext.h"
} dyngl_funcs;

/*--- Global variables ---*/

extern dyngl_funcs gl;

/*--- Functions ---*/

int dyngl_load(char *filename);

#ifdef __cplusplus
}
#endif

#endif
