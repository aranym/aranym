/*
	OSMesa LDG linker, OSMesa*() functions using Natfeats

	Copyright (C) 2004	Patrice Mandin

	This library is free software; you can redistribute it and/or
	modify it under the terms of the GNU Lesser General Public
	License as published by the Free Software Foundation; either
	version 2.1 of the License, or (at your option) any later version.

	This library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public
	License along with this library; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
*/

#ifndef LIB_OSMESA_H
#define LIB_OSMESA_H

#include <stddef.h>
#include <stdint.h>

/*--- Types ---*/

typedef struct osmesa_context *OSMesaContext;

#include "gltypes.h"

/*--- Variables ---*/

extern OSMesaContext cur_context;

extern int (*HostCall_p)(int function_number, OSMesaContext ctx, void *first_param);

/*--- Functions prototypes ---*/

#define OSMESA_PROC(type, gl, name, export, upper, params, first, ret) extern type APIENTRY OSMesa ## name params ;
#define GL_PROC(type, gl, name, export, upper, params, first, ret) extern type APIENTRY gl ## name params ;
#include "glfuncs.h"

#endif
