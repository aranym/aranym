/*
	Hostscreen, OpenGL renderer

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
#include "hostscreen.h"
#include "hostscreen_opengl.h"
#include "host_surface.h"
#include "host_surface_opengl.h"

HostScreenOpenGL::HostScreenOpenGL(void)
	: HostScreen()
{
	if (dyngl_load(bx_options.opengl.library)==0) {
		bx_options.opengl.enabled = false;
	}

	reset();
}

HostScreenOpenGL::~HostScreenOpenGL()
{
}

/*--- Public functions ---*/

void HostScreenOpenGL::drawSurfaceToScreen(HostSurface *hsurf, int *dst_x, int *dst_y)
{
	if (!bx_options.opengl.enabled) {
		HostScreen::drawSurfaceToScreen(hsurf, dst_x, dst_y);
		return;
	}

	if (!hsurf) {
		return;
	}
	SDL_Surface *sdl_surf = hsurf->getSdlSurface();

	int width = hsurf->getWidth();
	int height = hsurf->getHeight();

	SDL_Rect src_rect = {0,0, width, height};
	SDL_Rect dst_rect = {0,0, mainSurface->w, mainSurface->h};
	if (mainSurface->w > width) {
		dst_rect.x = (mainSurface->w - width) >> 1;
		dst_rect.w = width;
	} else {
		src_rect.w = mainSurface->w;
	}
	if (mainSurface->h > height) {
		dst_rect.y = (mainSurface->h - height) >> 1;
		dst_rect.h = height;
	} else {
		src_rect.h = mainSurface->h;
	}

	/* Init texturing */
	GLenum textureTarget = ((HostSurfaceOpenGL *)hsurf)->getTextureTarget();
	gl.Enable(textureTarget);
	gl.BindTexture(textureTarget, ((HostSurfaceOpenGL *)hsurf)->getTextureObject());

 	GLenum filtering = (bx_options.opengl.filtered ? GL_LINEAR : GL_NEAREST);
 	gl.TexParameteri(textureTarget, GL_TEXTURE_MAG_FILTER, filtering);
 	gl.TexParameteri(textureTarget, GL_TEXTURE_MIN_FILTER, filtering);
 	gl.TexParameteri(textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP);
 	gl.TexParameteri(textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP);
 	gl.TexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	/* Texture coordinates */
	GLfloat txWidth = width, txHeight = height;
	if (textureTarget == GL_TEXTURE_2D) {
		txWidth /= (GLfloat) sdl_surf->w;
		txHeight /= (GLfloat) sdl_surf->h;
	}
	gl.MatrixMode(GL_TEXTURE);
	gl.LoadIdentity();
	gl.Scalef(txWidth,txHeight,1.0);

	GLfloat targetX = (mainSurface->w-width)/2.0;	
	GLfloat targetY = (mainSurface->h-height)/2.0;
	GLfloat targetW = width;
	GLfloat targetH = height;

	gl.MatrixMode(GL_MODELVIEW);
	gl.LoadIdentity();
	gl.Translatef(targetX, targetY, 0.0);
	gl.Scalef(targetW, targetH, 1.0);

	gl.Begin(GL_QUADS);
		gl.TexCoord2f(0.0, 0.0);
		gl.Vertex2f(0.0, 0.0);

		gl.TexCoord2f(1.0, 0.0);
		gl.Vertex2f(1.0, 0.0);

		gl.TexCoord2f(1.0, 1.0);
		gl.Vertex2f(1.0, 1.0);

		gl.TexCoord2f(0.0, 1.0);
		gl.Vertex2f(0.0, 1.0);
	gl.End();

	gl.Disable(textureTarget);

	/* GUI need to know where it is */
	if (dst_x) {
		*dst_x = dst_rect.x;
	}
	if (dst_y) {
		*dst_y = dst_rect.y;
	}
}

void HostScreenOpenGL::refreshScreen(void)
{
	if (!bx_options.opengl.enabled) {
		HostScreen::refreshScreen();
		return;
	}

	SDL_GL_SwapBuffers();
}

HostSurface *HostScreenOpenGL::createSurface(int width, int height, int bpp)
{
	if (!bx_options.opengl.enabled) {
		return new HostSurface(width, height, bpp);
	}

	return new HostSurfaceOpenGL(width, height, bpp);
}

HostSurface *HostScreenOpenGL::createSurface(SDL_Surface *sdl_surf)
{
	if (!bx_options.opengl.enabled) {
		return new HostSurface(sdl_surf);
	}

	return new HostSurfaceOpenGL(sdl_surf);
}

#endif
