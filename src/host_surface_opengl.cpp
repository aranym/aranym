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

#include "SDL_compat.h"
#include "SDL_opengl_wrapper.h"

#include "dyngl.h"
#include "dirty_rects.h"
#include "host_surface.h"
#include "host_surface_opengl.h"

#define DEBUG 0
#include "debug.h"

/*--- Defines ---*/

/*--- Constructor/destructor ---*/

HostSurfaceOpenGL::HostSurfaceOpenGL(int width, int height, int bpp)
	: HostSurface(width, height, bpp)
{
	createTexture();
}

HostSurfaceOpenGL::HostSurfaceOpenGL(int width, int height, SDL_PixelFormat *pixelFormat)
	: HostSurface(width, height, pixelFormat)
{
	createTexture();
}

HostSurfaceOpenGL::~HostSurfaceOpenGL(void)
{
	destroyTextureObject();
}


/*--- Private functions ---*/

void HostSurfaceOpenGL::createTexture(void)
{
	can_palette = false;
	use_palette = false;
	resize(getWidth(), getHeight(), true);

	createTextureObject();
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

	const GLubyte *extensions = gl.GetString(GL_EXTENSIONS);

	if (gl_HasExtension("GL_ARB_texture_non_power_of_two", extensions)) {
		textureTarget = GL_TEXTURE_2D;
		can_palette = true;
	} else if (gl_HasExtension("GL_ARB_texture_rectangle", extensions)) {
		textureTarget = GL_TEXTURE_RECTANGLE_ARB;
		can_palette = false;
	} else if (gl_HasExtension("GL_EXT_texture_rectangle", extensions)) {
		textureTarget = GL_TEXTURE_RECTANGLE;
		can_palette = false;
	} else if (gl_HasExtension("GL_NV_texture_rectangle", extensions)) {
		textureTarget = GL_TEXTURE_RECTANGLE_NV;
		can_palette = false;
	} else {
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

SDL_Surface *HostSurfaceOpenGL::createSdlSurface(int width, int height,
	SDL_PixelFormat *pixelFormat)
{
	SDL_PixelFormat glPixelFormat;
	memcpy(&glPixelFormat, pixelFormat, sizeof(SDL_PixelFormat));

	textureFormat = GL_RGBA;
	switch(pixelFormat->BitsPerPixel) {
		case 8:
			textureFormat = GL_COLOR_INDEX;
			break;
		case 15:
			/* GL_RGB5_A1, byteswap at updateTexture time */
			glPixelFormat.Rmask = 31<<10;
			glPixelFormat.Gmask = 31<<5;
			glPixelFormat.Bmask = 31;
			glPixelFormat.Amask = 1<<15;
			break;
		case 16:
			/* GL_RGB, 16bits, byteswap at updateTexture time */
			textureFormat = GL_RGB;
			glPixelFormat.Rmask = 31<<11;
			glPixelFormat.Gmask = 63<<5;
			glPixelFormat.Bmask = 31;
			glPixelFormat.Amask = 0;
			break;
		case 24:
			/* GL_RGB, 24bits */
			textureFormat = GL_RGB;
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
			glPixelFormat.Rmask = 255;
			glPixelFormat.Gmask = 255<<8;
			glPixelFormat.Bmask = 255<<16;
			glPixelFormat.Amask = 0;
#else
			glPixelFormat.Rmask = 255<<16;
			glPixelFormat.Gmask = 255<<8;
			glPixelFormat.Bmask = 255;
			glPixelFormat.Amask = 0;
#endif
			break;
		case 32:
			/* FVDI driver is hardcoded to ARGB, so try to use a
			   compatible texture format if available */

			bool has_ext_bgra = gl_HasExtension("GL_EXT_bgra", gl.GetString(GL_EXTENSIONS));

			if (has_ext_bgra) {
				textureFormat = GL_BGRA_EXT;

				/* Only change format if some alpha was requested
				   till fvdi fixed */
				if (pixelFormat->Amask) {
					/* FIXME: is it the same on big endian ? */
					glPixelFormat.Rmask = 255<<16;
					glPixelFormat.Gmask = 255<<8;
					glPixelFormat.Bmask = 255;
					glPixelFormat.Amask = 255<<24;
				}
			}
			break;
	}

	return HostSurface::createSdlSurface(width,height,&glPixelFormat);
}

/*--- Public functions ---*/

void HostSurfaceOpenGL::update(void)
{
	updateTexture();
}

void HostSurfaceOpenGL::resize(int width, int height, bool force_recreate)
{
	int w=width, h=height;
	calcGlDimensions(&w, &h);
	HostSurface::resize(width,height, w,h, force_recreate);

	first_upload = true;
	updateTexture();
}

void HostSurfaceOpenGL::setPalette(SDL_Color *palette, int first, int count)
{
	HostSurface::setPalette(palette,first, count);

	if (!surface) {
		return;
	}

	/* Reupload palette if needed */

	if ((getBpp()==8) && use_palette) {
		Uint8 mapP[256*3];
		Uint8 *pMap = mapP;
		SDL_Color *palette = surface->format->palette->colors;

		for (int i=0;i<surface->format->palette->ncolors;i++) {
			*pMap++ = palette[i].r;
			*pMap++ = palette[i].g;
			*pMap++ = palette[i].b;
		}

		gl.BindTexture(textureTarget, textureObject);
		gl.ColorTableEXT(textureTarget, GL_RGB, 256, 
			GL_RGB, GL_UNSIGNED_BYTE, mapP);
		return;
	}

	/* Reupload texture if needed */

	updateTexture();
}

void HostSurfaceOpenGL::updateTexture(void)
{
	if (!surface) {
		return;
	}

	gl.BindTexture(textureTarget, textureObject);

	GLfloat mapR[256], mapG[256], mapB[256], mapA[256];

	GLenum internalFormat = GL_RGBA;
	GLenum pixelType = GL_UNSIGNED_BYTE;
	switch (getBpp()) {
		case 8:
			{
				SDL_Color *palette = surface->format->palette->colors;
				if (use_palette) {
					Uint8 mapP[256*3];
					Uint8 *pMap = mapP;

					internalFormat = GL_COLOR_INDEX8_EXT;
					for (int i=0;i<surface->format->palette->ncolors;i++) {
						*pMap++ = palette[i].r;
						*pMap++ = palette[i].g;
						*pMap++ = palette[i].b;
					}
					gl.ColorTableEXT(textureTarget, GL_RGB, 256, 
						GL_RGB, GL_UNSIGNED_BYTE, mapP);
				} else
				{
					memset(mapR, 0, sizeof(mapR));
					memset(mapG, 0, sizeof(mapG));
					memset(mapB, 0, sizeof(mapB));
					memset(mapA, 0, sizeof(mapA));
					for (int i=0;i<surface->format->palette->ncolors;i++) {
						mapR[i] = palette[i].r / 255.0;
						mapG[i] = palette[i].g / 255.0;
						mapB[i] = palette[i].b / 255.0;
						mapA[i] = 1.0;
					}
					gl.PixelTransferi(GL_MAP_COLOR, GL_TRUE);
					gl.PixelMapfv(GL_PIXEL_MAP_I_TO_R, 256, mapR);
					gl.PixelMapfv(GL_PIXEL_MAP_I_TO_G, 256, mapG);
					gl.PixelMapfv(GL_PIXEL_MAP_I_TO_B, 256, mapB);
					gl.PixelMapfv(GL_PIXEL_MAP_I_TO_A, 256, mapA);
				}
			}
			break;
		case 16:
			pixelType = GL_UNSIGNED_SHORT_5_6_5;
			/* FIXME: care about endianness ? */
			break;
		case 24:
			/* FIXME: care about endianness ? */
			break;
		case 32:
			/* FIXME: care about endianness ? */
			if (textureFormat == GL_BGRA_EXT) {
				pixelType = GL_UNSIGNED_INT_8_8_8_8_REV;
			}
			break;
	}

	if (first_upload) {
		/* Complete texture upload the first time, and only parts after */
		gl.TexImage2D(textureTarget,0, internalFormat, surface->w, surface->h, 0,
			textureFormat, pixelType, surface->pixels
		);
		first_upload = false;
	} else {
		gl.PixelStorei(GL_UNPACK_ROW_LENGTH, surface->w);

		int x,y;
		Uint8 *dirtyArray = getDirtyRects();
		for (y=0; y<getDirtyHeight(); y++) {
			for (x=0; x<getDirtyWidth(); x++) {
				if (!dirtyArray[y*getDirtyWidth()+x]) {
					continue;
				}

				Uint8 *surfPixels = (Uint8 *) surface->pixels;
				surfPixels += (y<<4) * surface->pitch;
				surfPixels += (x<<4) * surface->format->BytesPerPixel;				
				gl.TexSubImage2D(textureTarget,0, x<<4,y<<4,16,16,
					textureFormat, pixelType, surfPixels);
			}
		}
		clearDirtyRects();

		gl.PixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	}

	switch (getBpp()) {
		case 8:
			if (!use_palette) {
				gl.PixelTransferi(GL_MAP_COLOR, GL_FALSE);
			}
			break;
		case 16:
			/* FIXME: care about endianness ? */
			break;
		case 24:
			/* FIXME: care about endianness ? */
			break;
		case 32:
			/* FIXME: care about endianness ? */
			break;
	}
}

void HostSurfaceOpenGL::createTextureObject(void)
{
	gl.GenTextures(1, &textureObject);

	if (gl_HasExtension("GL_EXT_paletted_texture", gl.GetString(GL_EXTENSIONS)) && (getBpp() == 8)
	    && can_palette)
	{
		use_palette = true;
	}

	first_upload = true;
	updateTexture();
}

void HostSurfaceOpenGL::destroyTextureObject(void)
{
	gl.DeleteTextures(1, &textureObject);
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
