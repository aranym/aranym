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

#ifndef HOSTSURFACEOPENGL_H
#define HOSTSURFACEOPENGL_H

#include "SDL_compat.h"
#include "SDL_opengl_wrapper.h"

class HostSurface;

class HostSurfaceOpenGL: public HostSurface
{
	private:
		GLenum textureTarget, textureFormat;
		GLuint textureObject;
		bool can_palette, use_palette, first_upload;

		void createTexture(void);
		void calcGlDimensions(int *width, int *height);
		SDL_Surface *createSdlSurface(int width, int height,
			SDL_PixelFormat *pixelFormat);

	public:
		/* Create a surface from dimensions, bpp */
		HostSurfaceOpenGL(int width, int height, int bpp);

		/* Create a surface from dimensions, and a given pixel format */
		HostSurfaceOpenGL(int width, int height, SDL_PixelFormat *pixelFormat);

		/* Create a surface from existing SDL_Surface */
		HostSurfaceOpenGL(SDL_Surface *surf);

		virtual ~HostSurfaceOpenGL();

		void update(void);
		void resize(int width, int height,
			bool force_recreate = false);
		void setPalette(SDL_Color *palette, int first, int count);
		void updateTexture(void);

		void createTextureObject(void);
		void destroyTextureObject(void);

		GLenum getTextureTarget(void);
		GLuint getTextureObject(void);
};

#endif /* HOSTSURFACEOPENGL_H */
