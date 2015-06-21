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

#define WITH_PROTOTYPE_STRINGS 1

/*
 * Functions that return a 64-bit value.
 * The NF interface currently only returns a single value in D0,
 * so the call has to pass an extra parameter, the location where to
 * store the result value
 */
static GLuint64 call_glGetImageHandleARB(GLuint texture, GLint level, GLboolean32 layered, GLint layer, GLenum format)
{
	GLuint64 ret = 0;
	(*HostCall64_p)(NFOSMESA_GLGETIMAGEHANDLEARB, cur_context, &texture, &ret);
	return ret;
}
#define glGetImageHandleARB call_glGetImageHandleARB

static GLuint64 call_glGetImageHandleNV(GLuint texture, GLint level, GLboolean32 layered, GLint layer, GLenum format)
{
	GLuint64 ret = 0;
	(*HostCall64_p)(NFOSMESA_GLGETIMAGEHANDLENV, cur_context, &texture, &ret);
	return ret;
}
#define glGetImageHandleNV call_glGetImageHandleNV

static GLuint64 call_glGetTextureHandleARB(GLuint texture)
{
	GLuint64 ret = 0;
	(*HostCall64_p)(NFOSMESA_GLGETTEXTUREHANDLEARB, cur_context, &texture, &ret);
	return ret;
}
#define glGetTextureHandleARB call_glGetTextureHandleARB

static GLuint64 call_glGetTextureHandleNV(GLuint texture)
{
	GLuint64 ret = 0;
	(*HostCall64_p)(NFOSMESA_GLGETTEXTUREHANDLENV, cur_context, &texture, &ret);
	return ret;
}
#define glGetTextureHandleNV call_glGetTextureHandleNV

static GLuint64 call_glGetTextureSamplerHandleARB(GLuint texture, GLuint sampler)
{
	GLuint64 ret = 0;
	(*HostCall64_p)(NFOSMESA_GLGETTEXTURESAMPLERHANDLEARB, cur_context, &texture, &ret);
	return ret;
}
#define glGetTextureSamplerHandleARB call_glGetTextureSamplerHandleARB

static GLuint64 call_glGetTextureSamplerHandleNV(GLuint texture, GLuint sampler)
{
	GLuint64 ret = 0;
	(*HostCall64_p)(NFOSMESA_GLGETTEXTURESAMPLERHANDLENV, cur_context, &texture, &ret);
	return ret;
}
#define glGetTextureSamplerHandleNV call_glGetTextureSamplerHandleNV

/*--- LDG functions ---*/

static PROC const LibFunc[]={ 
#if WITH_PROTOTYPE_STRINGS
#define GL_PROC(type, gl, name, export, upper, params, first, ret) { #export, #type " " #gl #name #params, gl ## name },
#define OSMESA_PROC(type, gl, name, export, upper, params, first, ret) { #export, #type " OSMesa" #name #params, OSMesa ## name },
#else
#define GL_PROC(type, gl, name, export, upper, params, first, ret) { #export, 0, gl ## name },
#define OSMESA_PROC(type, gl, name, export, upper, params, first, ret) { #export, 0, OSMesa ## name },
#endif
	#include "glfuncs.h"		/* 12 OSMesa + 2664 GL functions + 1 GLU function */
	#include "link-oldmesa.h"	/* 5 + 8 functions for compatibility with TinyGL */
	{NULL, NULL, NULL}
};

#include "versinfo.h"

int err_old_nfapi(void)
{
	/* an error for Mesa_GL */
	return 1;
}


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
