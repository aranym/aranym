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

#include <stdio.h>
#include "SDL_compat.h"
#include "SDL_opengl_wrapper.h"

#include "dyngl.h"

#ifdef __cplusplus
extern "C" {
#endif

/*--- Global variables ---*/

dyngl_funcs gl;

/*--- Functions ---*/

int dyngl_load(const char *filename)
{
	int res = 1;
	
	if (filename != NULL && strlen(filename)>1) {
		if (SDL_GL_LoadLibrary(filename)<0)
			return -1;
	} else {
		/* Try to load default */
		if (SDL_GL_LoadLibrary(NULL)<0) {
			return -1;
		}
		res = 0;
	}

#define GL_PROC(type, gl, name, export, upper, proto, args, first, ret) gl.name = SDL_GL_GetProcAddress(#gl #name);
#define GLU_PROC(type, gl, name, export, upper, proto, args, first, ret)
#include "../../atari/nfosmesa/glfuncs.h"

	return res;
}

#ifdef __cplusplus
}
#endif
