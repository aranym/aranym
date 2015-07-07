/*
	OSMesa LDG loader

	Copyright (C) 2004	Patrice Mandin

	This software is provided 'as-is', without any express or implied
	warranty.  In no event will the authors be held liable for any damages
	arising from the use of this software.

	Permission is granted to anyone to use this software for any purpose,
	including commercial applications, and to alter it and redistribute it
	freely, subject to the following restrictions:

	1. The origin of this software must not be misrepresented; you must not
	   claim that you wrote the original software. If you use this software
	   in a product, an acknowledgment in the product documentation would be
	   appreciated but is not required.
	2. Altered source versions must be plainly marked as such, and must not be
	   misrepresented as being the original software.
	3. This notice may not be removed or altered from any source distribution.
*/

/*--- Includes ---*/

#include "lib-osmesa.h"
#include "nfosmesa_nfapi.h"

extern gl_private *private;;

#define OSMESA_PROC(type, gl, name, export, upper, proto, args, first, ret)
#define GL_GETSTRING(type, gl, name, export, upper, proto, args, first, ret)
#define GL_GETSTRINGI(type, gl, name, export, upper, proto, args, first, ret)
#define GL_PROC(type, gl, name, export, upper, proto, args, first, ret) \
type APIENTRY gl ## name proto \
{ \
	ret (*HostCall_p)(NFOSMESA_GL ## upper, private->cur_context, first); \
}
#define GL_PROC64(type, gl, name, export, upper, proto, args, first, ret) \
type APIENTRY gl ## name proto \
{ \
	GLuint64 __retval = 0; \
	(*HostCall64_p)(NFOSMESA_GL ## upper, private->cur_context, first, &__retval); \
	return __retval; \
}
#define GLU_PROC(type, gl, name, export, upper, proto, args, first, ret) \
type APIENTRY gl ## name proto \
{ \
	ret (*HostCall_p)(NFOSMESA_GLU ## upper, private->cur_context, first); \
}
#include "glfuncs.h"

void APIENTRY gluLookAtf( GLfloat eyeX, GLfloat eyeY, GLfloat eyeZ, GLfloat centerX, GLfloat centerY, GLfloat centerZ, GLfloat upX, GLfloat upY, GLfloat upZ )
{
	(*HostCall_p)(NFOSMESA_GLULOOKATF, private->cur_context, &eyeX);
}

void APIENTRY glFrustumf(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat near_val, GLfloat far_val)
{
	(*HostCall_p)(NFOSMESA_GLFRUSTUMF, private->cur_context, &left);
}

void glOrthof( GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat near_val, GLfloat far_val )
{
	(*HostCall_p)(NFOSMESA_GLORTHOF, private->cur_context, &left);
}

/* glClearDepthf() already exists in OpenGL/Mesa */
