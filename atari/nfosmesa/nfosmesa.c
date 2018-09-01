/*
	OSMesa LDG linker

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

/*--- Includes ---*/

#include <stdlib.h>

#include <gem.h>
#include <ldg.h>
#include <osbind.h>

/*--- Functions prototypes ---*/

#include "lib-osmesa.h"
#include "lib-oldmesa.h"
#include "lib-misc.h"
#include "nfosmesa_nfapi.h"

#define OSMESA_PROC(type, gl, name, export, upper, proto, args, first, ret) extern type APIENTRY OSMesa ## name proto ;
#define GL_PROC(type, gl, name, export, upper, proto, args, first, ret) extern type APIENTRY gl ## name proto ;
#include "glfuncs.h"

#define WITH_PROTOTYPE_STRINGS 1

gl_private *private;

/*--- LDG functions ---*/

static long GLAPIENTRY ldg_libinit(gl_private *priv)
{
	private = priv;
	internal_glInit(private);
	return sizeof(*private);
}

static PROC const LibFunc[]={ 
#if WITH_PROTOTYPE_STRINGS
	{ "glInit", "glInit(void *)", ldg_libinit },
#define GL_PROC(type, gl, name, export, upper, proto, args, first, ret) { #export, #type " " #gl #name #proto, gl ## name },
#define OSMESA_PROC(type, gl, name, export, upper, proto, args, first, ret) { #export, #type " OSMesa" #name #proto, OSMesa ## name },
#else
	{ "glInit", 0, ldg_libinit },
#define GL_PROC(type, gl, name, export, upper, proto, args, first, ret) { #export, 0, gl ## name },
#define OSMESA_PROC(type, gl, name, export, upper, proto, args, first, ret) { #export, 0, OSMesa ## name },
#endif
	#include "glfuncs.h"		/* 12 OSMesa + numerous GL functions + 1 GLU function */
	#include "link-oldmesa.h"	/* 5 + 8 functions for compatibility with TinyGL */
	{NULL, NULL, NULL}
};

#include "versinfo.h"

int err_old_nfapi(void)
{
	/* an error for Mesa_GL */
	return 1;
}


#ifndef __STRINGIFY
#define __STRING(x)	#x
#define __STRINGIFY(x)	__STRING(x)
#endif

char const __Ident_osmesa[] = "$OSMesa: NFOSMesa API Version " __STRINGIFY(ARANFOSMESA_NFAPI_VERSION) " " ASCII_ARCH_TARGET " " ASCII_COMPILER " $";

static LDGLIB const LibLdg = { 
	/* library version */
	0x0A15,
	/* count of functions in library */
	sizeof(LibFunc) / sizeof(LibFunc[0]) - 1,
	/* function addresses */
	LibFunc,
	/* Library information string */
	"Mesa library NFOSMesa API Version " __STRINGIFY(ARANFOSMESA_NFAPI_VERSION) " " ASCII_ARCH_TARGET " " ASCII_COMPILER,
	/* Library flags */
	LDG_NOT_SHARED,
	NULL,
	0
};

void APIENTRY tinyglinformation(void)
{
	(void) Cconws(LibLdg.infos);
	(void) Cconws("\r\n");
}


/*
 * Main part : communication with LDG-system
 */

int main(void)
{
	if (ldg_init(&LibLdg) != 0)
	{
		err_init("This program is a LDG library");
	}
	return 0;
}
