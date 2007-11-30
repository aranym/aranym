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
