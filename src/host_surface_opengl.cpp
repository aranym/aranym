/*
	Rendering surface
	with OpenGL texture

	(C) 2007 ARAnyM developer team

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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef ENABLE_OPENGL

#include <SDL.h>
#include <SDL_opengl.h>

#include "dyngl.h"
#include "dirty_rects.h"
#include "host_surface.h"
#include "host_surface_opengl.h"

#define DEBUG 1
#include "debug.h"

/*--- Defines ---*/

/*--- Constructor/destructor ---*/

HostSurfaceOpenGL::HostSurfaceOpenGL(SDL_Surface *surf)
	: HostSurface(surf)
{
	createTexture();
}

HostSurfaceOpenGL::HostSurfaceOpenGL(int width, int height, int bpp)
	: HostSurface(width, height, bpp)
{
	createTexture();
}

HostSurfaceOpenGL::~HostSurfaceOpenGL(void)
{
	gl.DeleteTextures(1, &textureObject);
}

/*--- Private functions ---*/

void HostSurfaceOpenGL::createTexture(void)
{
	can_palette = false;
	use_palette = false;
	resize(getWidth(), getHeight());

	gl.GenTextures(1, &textureObject);

#ifdef GL_EXT_paletted_texture
	char *extensions = (char *) gl.GetString(GL_EXTENSIONS);

	if (strstr(extensions, "GL_EXT_paletted_texture") && (getBpp() == 8)
	    && can_palette)
	{
		use_palette = true;
	}
#endif /* GL_EXT_paletted_texture */
}

void HostSurfaceOpenGL::calcGlDimensions(int *width, int *height)
{
	int w = *width, h = *height;

	/* Minimal size */
	if (w<64) {
		w = 64;
	}
	if (h<64) {
		h = 64;
	}

	/* Align on 16 pixels boundary */
	if (w & 15) {
		w = (w | 15)+1;
	}
	if (h & 15) {
		h = (h | 15)+1;
	}

	char *extensions = (char *) gl.GetString(GL_EXTENSIONS);

	if (strstr(extensions, "GL_ARB_texture_non_power_of_two")) {
		textureTarget = GL_TEXTURE_2D;
		can_palette = true;
	}
#if defined(GL_ARB_texture_rectangle)
	else if (strstr(extensions, "GL_ARB_texture_rectangle")) {
		textureTarget = GL_TEXTURE_RECTANGLE_ARB;
		can_palette = false;
	}
#endif
#if defined(GL_EXT_texture_rectangle)
	else if (strstr(extensions, "GL_EXT_texture_rectangle")) {
		textureTarget = GL_TEXTURE_RECTANGLE_EXT;
		can_palette = false;
	}
#endif
#if defined(GL_NV_texture_rectangle)
	else if (strstr(extensions, "GL_NV_texture_rectangle")) {
		textureTarget = GL_TEXTURE_RECTANGLE_NV;
		can_palette = false;
	}
#endif
	else {
		/* Calc smallest power of two size needed */
		int w1=64, h1=64;
		while (w>w1) {
			w1<<=1;
		}
		while (h>h1) {
			h1<<=1;
		}
		w = w1;
		h = h1;

		textureTarget = GL_TEXTURE_2D;
		can_palette = true;
	}

	/* FIXME: what to do if hw do not support asked size ? */
	*width = w;
	*height = h;
}

/*--- Public functions ---*/

void HostSurfaceOpenGL::resize(int width, int height)
{
	int w=width, h=height;
	calcGlDimensions(&w, &h);
	HostSurface::resize(width,height, w,h);

	D(bug("hs_ogl(): ask %dx%d surface, got %dx%d", width,height, w,h));
}

GLenum HostSurfaceOpenGL::getTextureTarget(void)
{
	return textureTarget;
}

GLuint HostSurfaceOpenGL::getTextureObject(void)
{
	return textureObject;
}

#endif /* ENABLE_OPENGL */
