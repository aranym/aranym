/*
	OSMesa LDG linker, misc functions

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
#include <string.h>

#include <mint/osbind.h>

#include "lib-osmesa.h"
#include "lib-oldmesa.h"
#include "nfosmesa_nfapi.h"

/*--- Defines ---*/

#define GL_VENDOR     0x1F00
#define GL_RENDERER   0x1F01
#define GL_VERSION    0x1F02
#define GL_EXTENSIONS 0x1F03
#define GL_SHADING_LANGUAGE_VERSION       0x8B8C

/*--- Variables ---*/

static GLubyte *gl_strings[7]={(GLubyte *)"",NULL,NULL,NULL,NULL};
static void CALLBACK (*gl_exception)(GLenum param);

/*--- Functions ---*/

const GLubyte* APIENTRY glGetString( GLenum name )
{
	int i, len;

	switch(name) {
		case GL_VERSION:
			i=1;
			break;
		case GL_RENDERER:
			i=2;
			break;
		case GL_VENDOR:
			i=3;
			break;
		case GL_EXTENSIONS:
			i=4;
			break;
		default:
			i=0;
			break;
	}

	if (i!=0) {
		if (gl_strings[i]==NULL) {
			unsigned long params[3];
			
			params[0] = (unsigned long) cur_context;
			params[1] = (unsigned long) name;

			len=(int)(*HostCall_p)(NFOSMESA_LENGLGETSTRING,cur_context,params);
			gl_strings[i]=(GLubyte *)Atari_MxAlloc(len+1);
			if (gl_strings[i]) {
				params[0] = (unsigned long) cur_context;
				params[1] = (unsigned long) name;
				params[2] = (unsigned long) gl_strings[i];
				(*HostCall_p)(NFOSMESA_PUTGLGETSTRING,cur_context,params);
			} else {
				return gl_strings[0];
			}
		}
	}

	return gl_strings[i];
}

const GLubyte* APIENTRY glGetStringi( GLenum name, GLuint index )
{
	int i, len;

	switch(name) {
		case GL_EXTENSIONS:
			i=5;
			break;
		case GL_SHADING_LANGUAGE_VERSION:
			i=6;
			break;
		default:
			i=0;
			break;
	}

	if (i!=0) {
		if (gl_strings[i]!=NULL) {
			Mfree(gl_strings[i]);
			gl_strings[i]=NULL;
		}
		{
			unsigned long params[4];
			
			params[0] = (unsigned long) cur_context;
			params[1] = (unsigned long) name;
			params[2] = (unsigned long) index;

			len=(int)(*HostCall_p)(NFOSMESA_LENGLGETSTRINGI,cur_context,params);
			if (len < 0)
				return NULL;
			gl_strings[i]=(GLubyte *)Atari_MxAlloc(len+1);
			if (gl_strings[i]) {
				params[0] = (unsigned long) cur_context;
				params[1] = (unsigned long) name;
				params[2] = (unsigned long) index;
				params[3] = (unsigned long) gl_strings[i];
				(*HostCall_p)(NFOSMESA_PUTGLGETSTRINGI,cur_context,params);
			} else {
				return gl_strings[0];
			}
		}
	}

	return gl_strings[i];
}

void freeglGetString(void)
{
	int i;
	
	for (i=1;i<7;i++) {
		if (gl_strings[i]) {
			Mfree(gl_strings[i]);
			gl_strings[i]=NULL;
		}
	}
}

void APIENTRY gluLookAtf( GLfloat eyeX, GLfloat eyeY, GLfloat eyeZ, GLfloat centerX, GLfloat centerY, GLfloat centerZ, GLfloat upX, GLfloat upY, GLfloat upZ )
{
	(*HostCall_p)(NFOSMESA_GLULOOKATF, cur_context, &eyeX);
}

void APIENTRY glFrustumf(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat near_val, GLfloat far_val)
{
	(*HostCall_p)(NFOSMESA_GLFRUSTUMF, cur_context, &left);
}

void glOrthof( GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat near_val, GLfloat far_val )
{
	(*HostCall_p)(NFOSMESA_GLORTHOF, cur_context, &left);
}

/* glClearDepthf() already exists in OpenGL/Mesa */

void APIENTRY tinyglswapbuffer(void *buf)
{
	(*HostCall_p)(NFOSMESA_TINYGLSWAPBUFFER, cur_context, &buf);
}


void APIENTRY tinyglexception_error(void CALLBACK (*exception)(GLenum param))
{
	gl_exception = exception;
}


int gl_exception_error(GLenum exception)
{
	if (gl_exception)
	{
		(*gl_exception)(exception);
		return 1;
	}
	return 0;
}


void gl_fatal_error(GLenum error, long except, const char *format)
{
	if (!gl_exception_error(except))
	{
		static char const err[] = "TinyGL: fatal error: ";
		(void) Fwrite(2, sizeof(err) - 1, err);
		(void) Fwrite(2, strlen(format), format);
		(void) Fwrite(2, 2, "\r\n");
	}
}
