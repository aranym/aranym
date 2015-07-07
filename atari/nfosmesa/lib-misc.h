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

#ifndef LIB_MISC_H
#define LIB_MISC_H

/*--- Includes ---*/

/*--- Functions ---*/

void internal_glInit(gl_private *private);

const GLubyte* APIENTRY glGetString( GLenum name );
const GLubyte* APIENTRY glGetStringi( GLenum name, GLuint index );

const GLubyte* APIENTRY internal_glGetString( gl_private *private, GLenum name );
const GLubyte* APIENTRY internal_glGetStringi( gl_private *private, GLenum name, GLuint index );
void freeglGetString(gl_private *private, OSMesaContext ctx);
int err_old_nfapi(void);
int gl_exception_error(gl_private *private, GLenum exception);
void gl_fatal_error(gl_private *private, GLenum error, long except, const char *format);
void APIENTRY internal_tinyglexception_error(gl_private *private, void CALLBACK (*exception)(GLenum param));
void APIENTRY internal_tinyglswapbuffer(gl_private *private, void *buf);

#endif
