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

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*--- Defines ---*/

#include "../../atari/nfosmesa/gltypes.h"

/*--- Structures ---*/

typedef struct {
#define GL_PROC(type, gl, name, export, upper, proto, args, first, ret) type (APIENTRY *name) proto ;
#define GLU_PROC(type, gl, name, export, upper, proot, args, first, ret)
#include "../../atari/nfosmesa/glfuncs.h"
} dyngl_funcs;

/*--- Global variables ---*/

extern dyngl_funcs gl;

/*--- Functions ---*/

int dyngl_load(const char *filename);

#ifdef __cplusplus
}
#endif

#endif
