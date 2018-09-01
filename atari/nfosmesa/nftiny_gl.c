/*
	NFTinyGL LDG linker

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
#include "../natfeat/nf_ops.h"


#define WITH_PROTOTYPE_STRINGS 1

gl_private *private;

/*--- functions that "throw" exceptions ---*/

#ifdef TGL_ENABLE_CHECKS

GLenum GLAPIENTRY glGetError(void)
{
	return (*HostCall_p)(NFOSMESA_GLGETERROR, private->cur_context, NULL);
}

static GLenum clear_gl_error(void)
{
	GLenum i;
	
	i = glGetError();
	if (i != GL_NO_ERROR)
	{
		while (glGetError() != GL_NO_ERROR)
			;
	}
	return i;
}

static void GLAPIENTRY check_glBindTexture(GLenum target, GLuint texture)
{
	GLenum e;
	
	clear_gl_error();
	glBindTexture(target, texture);
	if ((e = glGetError()) != GL_NO_ERROR)
	{
		gl_fatal_error(private, e, 8, "glBindTexture: unsupported option");
	}
}
#define glBindTexture check_glBindTexture

static void GLAPIENTRY check_glTexImage2D(GLenum target, GLint level, GLint components, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels)
{
	GLenum e;
	
	clear_gl_error();
	glTexImage2D(target, level, components, width, height, border, format, type, pixels);
	if ((e = glGetError()) != GL_NO_ERROR)
	{
		gl_fatal_error(private, e, 7, "glTexImage2D: combination of parameters not handled");
	}
}
#define glTexImage2D check_glTexImage2D

static void GLAPIENTRY check_glTexEnvi(GLenum target, GLenum pname, GLint param)
{
	GLenum e;
	
	clear_gl_error();
	glTexEnvi(target, pname, param);
	if ((e = glGetError()) != GL_NO_ERROR)
	{
		gl_fatal_error(private, e, 8, "glTexEnv: unsupported option");
	}
}
#define glTexEnvi check_glTexEnvi

static void GLAPIENTRY check_glTexParameteri(GLenum target, GLenum pname, GLint param)
{
	GLenum e;
	
	clear_gl_error();
	glTexParameteri(target, pname, param);
	if ((e = glGetError()) != GL_NO_ERROR)
	{
		gl_fatal_error(private, e, 8, "glTexParameter: unsupported option");
	}
}
#define glTexParameteri check_glTexParameteri

static void GLAPIENTRY check_glPixelStorei(GLenum pname, GLint param)
{
	GLenum e;
	
	clear_gl_error();
	glPixelStorei(pname, param);
	if ((e = glGetError()) != GL_NO_ERROR)
	{
		gl_fatal_error(private, e, 9, "glPixelStore: unsupported option");
	}
}
#define glPixelStorei check_glPixelStorei

static void GLAPIENTRY check_glGetIntegerv(GLenum pname, GLint *params)
{
	GLenum e;
	
	clear_gl_error();
	glGetIntegerv(pname, params);
	if ((e = glGetError()) != GL_NO_ERROR)
	{
		gl_fatal_error(private, e, 3, "glGetIntegerv: option not implemented");
	}
}
#define glGetIntegerv check_glGetIntegerv

static void GLAPIENTRY check_glGetFloatv(GLenum pname, GLfloat *v)
{
	GLenum e;
	
	clear_gl_error();
	glGetFloatv(pname, v);
	if ((e = glGetError()) != GL_NO_ERROR)
	{
		gl_fatal_error(private, e, 102, "glGetFloatv: option not implemented");
	}
}
#define glGetFloatv check_glGetFloatv

static void GLAPIENTRY check_glCallList(GLuint list)
{
	GLenum e;
	
	clear_gl_error();
	glCallList(list);
	if ((e = glGetError()) != GL_NO_ERROR)
	{
		gl_fatal_error(private, e, 2, "glCallList: list not defined");
	}
}
#define glCallList check_glCallList

static void GLAPIENTRY check_glVertex2f(GLfloat x, GLfloat y)
{
	GLenum e;
	
	clear_gl_error();
	glVertex2f(x, y);
	if ((e = glGetError()) != GL_NO_ERROR)
	{
		gl_fatal_error(private, e, 2, "unable to allocate GLVertex array");
	}
}
#define glVertex2f check_glVertex2f

static void GLAPIENTRY check_glVertex3f(GLfloat x, GLfloat y, GLfloat z)
{
	GLenum e;
	
	clear_gl_error();
	glVertex3f(x, y, z);
	if ((e = glGetError()) != GL_NO_ERROR)
	{
		gl_fatal_error(private, e, 2, "unable to allocate GLVertex array");
	}
}
#define glVertex3f check_glVertex3f

static void GLAPIENTRY check_glVertex4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
	GLenum e;
	
	clear_gl_error();
	glVertex4f(x, y, z, w);
	if ((e = glGetError()) != GL_NO_ERROR)
	{
		gl_fatal_error(private, e, 2, "unable to allocate GLVertex array");
	}
}
#define glVertex4f check_glVertex4f

static void GLAPIENTRY check_glVertex3fv(const GLfloat *v)
{
	GLenum e;
	
	clear_gl_error();
	glVertex3fv(v);
	if ((e = glGetError()) != GL_NO_ERROR)
	{
		if (e == GL_OUT_OF_MEMORY)
			gl_fatal_error(private, e, 2, "unable to allocate GLVertex array");
		else
			gl_fatal_error(private, e, 10, "glBegin: type not handled");
	}
}
#define glVertex3fv check_glVertex3fv

static void GLAPIENTRY check_glViewport(GLint x, GLint y, GLint width, GLint height)
{
	GLenum e;
	
	clear_gl_error();
	glViewport(x, y, width, height);
	if ((e = glGetError()) != GL_NO_ERROR)
	{
		if (width <= 0 || height <= 0)
			gl_fatal_error(private, e, 5, "glViewport: size too small");
		else
			gl_fatal_error(private, e, 4, "glViewport: error while resizing display");
	}
}
#define glViewport check_glViewport

static void GLAPIENTRY check_glEnable(GLenum cap)
{
	GLenum e;
	
	clear_gl_error();
	glEnable(cap);
	if ((e = glGetError()) != GL_NO_ERROR)
	{
		gl_fatal_error(private, e, 103, "glEnableDisable: unsupported code");
	}
}
#define glEnable check_glEnable

static void GLAPIENTRY check_glDisable(GLenum cap)
{
	GLenum e;
	
	clear_gl_error();
	glDisable(cap);
	if ((e = glGetError()) != GL_NO_ERROR)
	{
		gl_fatal_error(private, e, 103, "glEnableDisable: unsupported code");
	}
}
#define glDisable check_glDisable

#endif /* TGL_ENABLE_CHECKS */

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
#define GL_PROC(type, ret, name, f, desc, proto, args) { name, desc, f },
#else
	{ "glInit", 0, ldg_libinit },
#define GL_PROC(type, ret, name, f, desc, proto, args) { name, 0, f },
#endif
	#include "link-tinygl.h"	/* 83 functions */
	{NULL, NULL, NULL}
};

#include "versinfo.h"

#ifndef __STRINGIFY
#define __STRING(x)	#x
#define __STRINGIFY(x)	__STRING(x)
#endif

char const __Ident_tinygl[] = "$NFTinyGL: TinyGL NFOSMesa API Version " __STRINGIFY(ARANFOSMESA_NFAPI_VERSION) " " ASCII_ARCH_TARGET " " ASCII_COMPILER " $";

int err_old_nfapi(void)
{
	/* not an error for TinyGL; the 83 functions should always be present */
	return 0;
}


static LDGLIB const LibLdg = { 
	/* library version */
	0x0100,
	/* count of functions in library */
	sizeof(LibFunc) / sizeof(LibFunc[0]) - 1,
	/* function addresses */
	LibFunc,
	/* Library information string */
	"TinyGL NFOSMesa API Version " __STRINGIFY(ARANFOSMESA_NFAPI_VERSION) " " ASCII_ARCH_TARGET " " ASCII_COMPILER,
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
