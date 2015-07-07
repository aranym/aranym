/*
	OSMesa LDG linker, old functions prototypes

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

#ifndef LIB_OLDMESA_H
#define LIB_OLDMESA_H

/*--- Defines ---*/

/* Mxalloc parameters */
#ifndef MX_STRAM
#define MX_STRAM 0
#define MX_TTRAM 1
#define MX_PREFSTRAM 2
#define MX_PREFTTRAM 3
#endif

/*--- Functions prototypes ---*/

void err_init(const char *str);

void *APIENTRY OSMesaCreateLDG(GLenum format, GLenum type, GLint width, GLint height );
void APIENTRY OSMesaDestroyLDG(void);
GLsizei APIENTRY max_width(void);
GLsizei APIENTRY max_height(void);

void *APIENTRY internal_OSMesaCreateLDG(gl_private *private, GLenum format, GLenum type, GLint width, GLint height );
void APIENTRY internal_OSMesaDestroyLDG(gl_private *private);
GLsizei APIENTRY internal_max_width(gl_private *private);
GLsizei APIENTRY internal_max_height(gl_private *private);
void APIENTRY glOrthof( GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat near_val, GLfloat far_val );

void APIENTRY glClearDepthf(GLfloat depth);
void APIENTRY glFrustumf(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat near_val, GLfloat far_val );
void APIENTRY gluLookAtf( GLfloat eyeX, GLfloat eyeY, GLfloat eyeZ, GLfloat centerX, GLfloat centerY, GLfloat centerZ, GLfloat upX, GLfloat upY, GLfloat upZ );

void APIENTRY tinyglswapbuffer(void *buf);
void APIENTRY tinyglexception_error(void CALLBACK (*exception)(GLenum param));
void APIENTRY tinyglinformation(void);

#endif /* OLDMESA_H */
