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
#include <SDL.h>
#include <SDL_opengl.h>

#include "dyngl.h"

#ifdef __cplusplus
extern "C" {
#endif

/*--- Global variables ---*/

dyngl_funcs gl;

/*--- Functions ---*/

int dyngl_load(char *filename)
{
	int lib_loaded = 0;

	if (strlen(filename)>1) {
		if (SDL_GL_LoadLibrary(filename)<0) {
			fprintf(stderr, "Can not load OpenGL library from <%s>\n", filename);
		} else {
			lib_loaded = 1;
		}
	}

	if (!lib_loaded) {
		/* Try to load default */
		if (SDL_GL_LoadLibrary(NULL)<0) {
			return 0;
		}
		fprintf(stderr, "Loaded default OpenGL library\n");
	}

	#include "dyngl_gl.c"
	#include "dyngl_glext.c"

	return 1;
}

#ifdef __cplusplus
}
#endif
