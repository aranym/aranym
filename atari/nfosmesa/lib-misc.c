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

#include <GL/gl.h>

#include "lib-osmesa.h"
#include "lib-oldmesa.h"
#include "../natfeat/natfeat.h"
#include "nfosmesa_nfapi.h"

/*--- Defines ---*/

/*--- Variables ---*/

static GLubyte *gl_strings[5]={NULL,NULL,NULL,NULL,""};

/*--- Functions ---*/

const GLubyte* glGetString( GLenum name )
{
	int i, len;

	switch(name) {
		case GL_VERSION:
			i=0;
			break;
		case GL_RENDERER:
			i=1;
			break;
		case GL_VENDOR:
			i=2;
			break;
		case GL_EXTENSIONS:
			i=3;
			break;
		default:
			i=4;
			break;
	}

	if (i!=4) {
		if (gl_strings[i]==NULL) {
			len=nfCall((NFOSMESA(NFOSMESA_LENGLGETSTRING),cur_context,name));
			gl_strings[i]=(GLubyte *)Atari_MxAlloc(len+1);
			if (gl_strings[i]) {
				nfCall((NFOSMESA(NFOSMESA_PUTGLGETSTRING),cur_context,name,gl_strings[i]));
			} else {
				return gl_strings[4];
			}
		}
	}

	return gl_strings[i];
}

void freeglGetString(void)
{
	int i;
	
	for (i=0;i<4;i++) {
		if (gl_strings[i]) {
			Mfree(gl_strings[i]);
			gl_strings[i]=NULL;
		}
	}
}
