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
#include "parameters.h"

#define DEBUG 0
#include "debug.h"

HostScreenOpenGL::HostScreenOpenGL(void)
	: HostScreen()
{
	if (dyngl_load(bx_options.opengl.library)==0) {
		fprintf(stderr, "Can not load OpenGL library: using software rendering mode\n");
		bx_options.opengl.enabled = false;
	}
}

HostScreenOpenGL::~HostScreenOpenGL()
{
}

/*--- Public functions ---*/

void HostScreenOpenGL::setVideoMode(int width, int height, int bpp)
{
	if (!bx_options.opengl.enabled) {
		HostScreen::setVideoMode(width, height, bpp);
		return;
	}

	if (bx_options.autozoom.fixedsize) {
		width = bx_options.autozoom.width;
		height = bx_options.autozoom.height;
	}
	if (width<MIN_WIDTH) {
		width=MIN_WIDTH;
	}
	if (height<MIN_HEIGHT) {
		height=MIN_HEIGHT;
	}

	int i, gl_bpp[4]={0,16,24,32}, screenFlags;

	screenFlags = SDL_OPENGL|SDL_RESIZABLE;
	if (bx_options.video.fullscreen) {
		screenFlags |= SDL_FULLSCREEN;
	}

	SDL_GL_SetAttribute(SDL_GL_RED_SIZE,5);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE,5);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,5);
	SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE,15);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);

	/* Tell all surfaces their texture will be destroyed.
	   OpenGL context destroyed and recreated happens on most systems.
	*/
	std::list<HostSurfaceOpenGL *>::iterator it;
	for (it=surfList.begin(); it!=surfList.end(); ++it) {
		(*it)->destroyTextureObject();
	}

	/* Now setup video mode */
	for (i=0;i<4;i++) {
		screen = SDL_SetVideoMode(width, height, gl_bpp[i], screenFlags);
		if (screen) {
			break;
		}
	}
	if (screen==NULL) {
		/* Try with default resolution */
		for (i=0;i<4;i++) {
			screen = SDL_SetVideoMode(0, 0, gl_bpp[i], screenFlags);
			if (screen) {
				break;
			}
		}
	}
	if (screen==NULL) {
		fprintf(stderr, "Can not set OpenGL video mode: using software rendering mode\n");
		bx_options.opengl.enabled = false;
		HostScreen::setVideoMode(width, height, bpp);
		return;
	}

	/* Now tell surfaces to recreate their texture */
	for (it=surfList.begin(); it!=surfList.end(); ++it) {
		(*it)->createTextureObject();
	}

	bx_options.video.fullscreen = ((screen->flags & SDL_FULLSCREEN) == SDL_FULLSCREEN);

	new_width = screen->w;
	new_height = screen->h;
	resizeDirty(screen->w, screen->h);
	forceRefreshScreen();
}

void HostScreenOpenGL::makeSnapshot(void)
{
	if (!bx_options.opengl.enabled) {
		return HostScreen::makeSnapshot();
	}

	char filename[15];
	sprintf( filename, "snap%03d.bmp", snapCounter++ );

	HostSurface *sshot_hsurf = createSurface(getWidth(), getHeight(), 32);
	if (!sshot_hsurf) {
		return;
	}
	SDL_Surface *sdl_surf = sshot_hsurf->getSdlSurface();
	if (sdl_surf) {
#ifdef GL_EXT_bgra
		int i;
		Uint8 *dst = (Uint8 *) sdl_surf->pixels;

		for (i=0;i<screen->h;i++) {
			gl.ReadPixels(0,screen->h-i-1,screen->w,1,GL_BGRA_EXT,
				GL_UNSIGNED_INT_8_8_8_8_REV, dst);
			dst += sdl_surf->pitch;
		}

		SDL_SaveBMP(sdl_surf, filename);
#else
		fprintf(stderr, "screenshot: Sorry, BGRA texture format not supported\n");
#endif
	}

	destroySurface(sshot_hsurf);
}

int HostScreenOpenGL::getBpp(void)
{
	if (!bx_options.opengl.enabled) {
		return HostScreen::getBpp();
	}

	int bpp;

	SDL_GL_GetAttribute(SDL_GL_BUFFER_SIZE, &bpp);
	return bpp;
}

void HostScreenOpenGL::drawSurfaceToScreen(HostSurface *hsurf, int *dst_x, int *dst_y)
{
	if (!bx_options.opengl.enabled) {
		HostScreen::drawSurfaceToScreen(hsurf, dst_x, dst_y);
		return;
	}

	if (!hsurf) {
		return;
	}
	hsurf->update();

	SDL_Surface *sdl_surf = hsurf->getSdlSurface();

	int width = hsurf->getWidth();
	int height = hsurf->getHeight();

	SDL_Rect src_rect = {0,0, width, height};
	SDL_Rect dst_rect = {0,0, screen->w, screen->h};
	if (screen->w > width) {
		dst_rect.x = (screen->w - width) >> 1;
		dst_rect.w = width;
	} else {
		src_rect.w = screen->w;
	}
	if (screen->h > height) {
		dst_rect.y = (screen->h - height) >> 1;
		dst_rect.h = height;
	} else {
		src_rect.h = screen->h;
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

	bool use_alpha = (hsurf->getParam(HostSurface::SURF_USE_ALPHA) != 0);
	if (use_alpha) {
	 	gl.TexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

		gl.Enable(GL_BLEND);
		gl.BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	} else {
	 	gl.TexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

		gl.Enable(GL_BLEND);
		gl.BlendFunc(GL_ONE, GL_ZERO);
	}

	/* Texture coordinates */
	GLfloat txWidth = width, txHeight = height;
	if (textureTarget == GL_TEXTURE_2D) {
		txWidth /= (GLfloat) sdl_surf->w;
		txHeight /= (GLfloat) sdl_surf->h;
	}
	gl.MatrixMode(GL_TEXTURE);
	gl.PushMatrix();
	gl.Scalef(txWidth,txHeight,1.0);

	GLfloat targetW = width;
	GLfloat targetH = height;
	if (bx_options.autozoom.enabled
	    && (hsurf->getParam(HostSurface::SURF_DRAW)
	      == HostSurface::DRAW_RESCALE))
	{
		targetW = screen->w;
		targetH = screen->h;
		if (bx_options.autozoom.integercoefs && (screen->w>=width)
			&& (screen->h>=height))
		{
			int coefx = (int) (screen->w / width);
			int coefy = (int) (screen->h / height);
			targetW = (GLfloat) (width * coefx);
			targetH = (GLfloat) (height * coefy);
		}
	}
	GLfloat targetX = (screen->w-targetW)*0.5;	
	GLfloat targetY = (screen->h-targetH)*0.5;

	gl.MatrixMode(GL_MODELVIEW);
	gl.PushMatrix();
	gl.Translatef(targetX, targetY, 0.0);
	gl.Scalef(targetW, targetH, 1.0);

	if (use_alpha) {
		gl.Color4f(1.0,1.0,1.0,hsurf->getParam(HostSurface::SURF_ALPHA)/100.0);
	}
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

	gl.MatrixMode(GL_TEXTURE);
	gl.PopMatrix();
	gl.MatrixMode(GL_MODELVIEW);
	gl.PopMatrix();

	gl.Disable(GL_BLEND);
	gl.Disable(textureTarget);

	/* GUI need to know where it is */
	if (dst_x) {
		*dst_x = dst_rect.x;
	}
	if (dst_y) {
		*dst_y = dst_rect.y;
	}
}

void HostScreenOpenGL::initScreen(void)
{
	if (!bx_options.opengl.enabled) {
		return;
	}

	gl.Viewport(0, 0, screen->w, screen->h);

	gl.MatrixMode(GL_PROJECTION);
	gl.LoadIdentity();
	gl.Ortho(0.0, screen->w, screen->h, 0.0, -1.0, 1.0);

	gl.MatrixMode(GL_TEXTURE);
	gl.LoadIdentity();

	gl.MatrixMode(GL_MODELVIEW);
	gl.LoadIdentity();
	gl.Translatef(0.375, 0.375, 0.0);
}

void HostScreenOpenGL::clearScreen(void)
{
	if (!bx_options.opengl.enabled) {
		HostScreen::clearScreen();
		return;
	}

	gl.ClearColor(0.0,0.0,0.0,0.0);
	gl.Clear(GL_COLOR_BUFFER_BIT);
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
		return HostScreen::createSurface(width, height, bpp);
	}

	HostSurfaceOpenGL *hsurf = new HostSurfaceOpenGL(width, height, bpp);
	if (hsurf) {
		surfList.push_back(hsurf);
	}
	return hsurf;
}

void HostScreenOpenGL::destroySurface(HostSurface *hsurf)
{
	if (!bx_options.opengl.enabled) {
		HostScreen::destroySurface(hsurf);
		return;
	}

	if (hsurf) {
		surfList.remove((HostSurfaceOpenGL *) hsurf);
	}
	delete hsurf;
}

#endif
