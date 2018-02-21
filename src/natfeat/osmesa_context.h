/*
	NatFeat host OSMesa rendering; context management

	ARAnyM (C) 2015 ARAnyM developer team

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

#ifndef NFOSMESA_CONTEXT_H
#define NFOSMESA_CONTEXT_H

#if defined(__MACOSX__)
#include <CoreFoundation/CoreFoundation.h>
#endif
#if defined(SDL_VIDEO_DRIVER_X11) && defined(HAVE_X11_XLIB_H)
#include <GL/glx.h>
#endif

/*--- Base Class ---*/

class OffscreenContext {
protected:
	/*
	 * handle of library to fetch system dependant functions from
	 */
	void *lib_handle;
	
	/*
	 * user-specified number of pixels per row
	 */
	GLint userRowLength;
	
	/*
	 * TRUE  -> Y increases upward (default)
	 * FALSE -> Y increases downward
	 */	
	GLboolean yup;
	
	/* Atari buffer */
	memptr dst_buffer;

	/* 
	 * Host buffer, if channel reduction needed (deprecated)
	 * or when we cannot pass the atari address directly
	 * (e.g. when using FULLMMU, or glReadPixels() in opposite Y-order)
	 */
	void *host_buffer;
	GLsizei buffer_width, buffer_height, buffer_bpp;

	/*
	 * format of the conversion buffer
	 */
	GLenum srcformat;
	
	/*
	 * requested destination format
	 */
	GLenum dstformat;
	
	/*
	 * number of bytes per pixel for the dstformat
	 */
	GLsizei destination_bpp;

	/*
	 * conversion needed from srcformat to dstformat ?
	 */
	bool conversion;

	/*
	 * true if dstformat was changed from OSMESA_ARGB to GL_BGRA
	 * (GL does not have something like GL_ARGB)
	 */
	bool swapcomponents;

	GLboolean error_check_enabled;
	
	void ConvertContext8();			/* 8 bits per channel */
	void ConvertContext16(void);	/* 16 bits per channel */
	void ConvertContext32(void);	/* 32 bits per channel */
	void ResizeBuffer(GLsizei newBpp);
	
	GLboolean getYup() { return yup; }
	GLint getUserRowLength() { return userRowLength ? userRowLength : width; }
	virtual void setYup(GLboolean enable) { yup = enable; }
	virtual bool UpsideDown(void) { return false; }
	virtual void setUserRowLength(GLint length) {
		userRowLength = length;
		if (userRowLength != 0 && userRowLength != width)
			conversion = true;
	}
	
	static bool FormatHasAlpha(GLenum format);

public:

	static GLboolean has_MESA_pack_invert;

	GLenum type;
	GLsizei width, height;
	OffscreenContext(void *glhandle);
	virtual ~OffscreenContext();
	virtual bool TestContext() = 0;
	virtual bool CreateContext(GLenum format, GLint depthBits, GLint stencilBits, GLint accumBits, OffscreenContext *sharelist) = 0;
	virtual GLboolean MakeCurrent(memptr buffer, GLenum type, GLsizei width, GLsizei height) = 0;
	virtual GLboolean MakeCurrent() = 0;
	virtual GLboolean ClearCurrent() = 0;
	virtual void Postprocess(const char * /* filter */, GLuint /* enable_value */) {}
	virtual bool GetIntegerv(GLint pname, GLint *value) = 0;
	virtual void PixelStore(GLint pname, GLint value);
	virtual void ColorClamp(GLboolean enable) = 0;
	virtual void ConvertContext();
	virtual void GetColorBuffer(GLint *width, GLint *height, GLint *format, memptr *buffer);
	virtual void GetDepthBuffer(GLint *width, GLint *height, GLint *bytesPerValue, memptr *buffer);

	virtual bool IsOpengl(void) = 0;
};

/*--- Class for using OSMesa offscreen rendering ---*/

class MesaContext : public OffscreenContext {
protected:
	OSMesaContext ctx;
public:
	MesaContext(void *glhandle);
	virtual ~MesaContext();
	virtual bool CreateContext(GLenum format, GLint depthBits, GLint stencilBits, GLint accumBits, OffscreenContext *sharelist);
	virtual GLboolean MakeCurrent(memptr buffer, GLenum type, GLsizei width, GLsizei height);
	virtual GLboolean MakeCurrent();
	virtual GLboolean ClearCurrent();
	virtual void Postprocess(const char *filter, GLuint enable_value);
	virtual bool GetIntegerv(GLint pname, GLint *value);
	virtual void PixelStore(GLint pname, GLint value);
	virtual void ColorClamp(GLboolean enable);
	virtual bool TestContext();

	virtual bool IsOpengl(void) { return false; }
};

/*--- Base Class for using OpenGL render buffers ---*/

class OpenglContext : public OffscreenContext {
protected:
	GLint render_width;
	GLint render_height;
	GLint depthBits;
	GLint stencilBits;
	GLint accumBits;
	GLuint framebuffer;
	GLuint colorbuffer;
	GLuint colortex;
	bool useTeximage;
	GLenum texTarget;
	GLuint depthbuffer;
	GLenum depthformat;
	GLuint stencilbuffer;
	GLboolean has_GL_EXT_packed_depth_stencil;
	GLboolean has_GL_NV_packed_depth_stencil;
	GLboolean has_GL_EXT_texture_object;
	GLboolean has_GL_XX_texture_rectangle;
	GLboolean has_GL_ARB_texture_non_power_of_two;
	virtual bool CreateContext(GLenum format, GLint depthBits, GLint stencilBits, GLint accumBits, OffscreenContext *sharelist);
	void DestroyContext(void);
	GLboolean MakeBufferCurrent(bool create_buffer);
	GLboolean ClearCurrent();
	virtual void setYup(GLboolean enable);
	virtual bool UpsideDown(void) { return !getYup(); }
	GLboolean createBuffers();
public:
	OpenglContext(void *glhandle);
	virtual ~OpenglContext();
	virtual bool GetIntegerv(GLint pname, GLint *value);
	virtual void ColorClamp(GLboolean enable);
	virtual void ConvertContext();

	virtual bool IsOpengl(void) { return true; }
};

/*--- System dependant classes using OpenGL ---*/

#if defined(SDL_VIDEO_DRIVER_X11) && defined(HAVE_X11_XLIB_H)
class X11OpenglContext : public OpenglContext {
protected:
	GLXContext ctx;
	GLXPbuffer pbuffer;

	static Display *dpy;
	static Window sdlwindow;
	static int screen;

	static XVisualInfo* (*pglXChooseVisual)(Display *dpy, int, int *);
	static GLXContext (*pglXCreateContext)(Display *, XVisualInfo *, GLXContext, int);
	static int (*pglXMakeCurrent)(Display *, GLXDrawable, GLXContext);
	static GLXContext (*pglXGetCurrentContext)(void);
	static void (*pglXDestroyContext)(Display *, GLXContext);
	static const char *(*pglXQueryServerString)(Display *, int, int);
	static const char *(*pglXGetClientString)(Display *, int);
	static const char *(*pglXQueryExtensionsString)(Display *, int);
	static GLXContext (*pglXCreateContextAttribsARB) (Display *dpy, GLXFBConfig config, GLXContext share_context, Bool direct, const int *attrib_list);
	static GLXFBConfig * (*pglXChooseFBConfig) (Display *dpy, int screen, const int *attrib_list, int *nelements);
	static Bool (*pglXMakeContextCurrent) (Display *dpy, GLXDrawable draw, GLXDrawable read, GLXContext ctx);
	static GLXPbuffer (*pglXCreatePbuffer) (Display *dpy, GLXFBConfig config, const int *attrib_list);
	static void (*pglXDestroyPbuffer) (Display *dpy, GLXPbuffer pbuf);
	static int (*pXFree)(void *);

	void dispose();

public:
	X11OpenglContext(void *glhandle);
	virtual ~X11OpenglContext();
	virtual bool CreateContext(GLenum format, GLint depthBits, GLint stencilBits, GLint accumBits, OffscreenContext *sharelist);
	virtual GLboolean MakeCurrent(memptr buffer, GLenum type, GLsizei width, GLsizei height);
	virtual GLboolean MakeCurrent();
	virtual GLboolean ClearCurrent();
	virtual bool TestContext();

	static void InitPointers(void *lib_handle);
	static SDL_GLContext GetSDLContext(void);
	static void SetSDLContext(SDL_GLContext ctx);
};
#endif

#if defined(SDL_VIDEO_DRIVER_WINDOWS)
class Win32OpenglContext : public OpenglContext {
private:
	static LONG CALLBACK tmpWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	static bool windClassRegistered;
	static HWND tmpHwnd;
	static HDC tmphdc;
	static HWND sdlwindow;
protected:
	HGLRC ctx;
	int iPixelFormat;

	static int (WINAPI *pwglGetPixelFormat)(HDC);
	static int (WINAPI *pwglChoosePixelFormat) (HDC hDc, const PIXELFORMATDESCRIPTOR *pPfd);
	static WINBOOL (WINAPI *pwglSetPixelFormat) (HDC hDc, int ipfd, const PIXELFORMATDESCRIPTOR *ppfd);
	static WINBOOL (WINAPI *pwglSwapBuffers)(HDC);
	static int (WINAPI *pwglDescribePixelFormat)(HDC hdc, int iPixelFormat, UINT nBytes, PIXELFORMATDESCRIPTOR *ppfd);
	
	static WINBOOL (WINAPI *pwglCopyContext)(HGLRC, HGLRC, UINT);
	static HGLRC (WINAPI *pwglCreateContext)(HDC hdc);
	static HGLRC WINAPI (*pwglCreateLayerContext)(HDC, int);
	static WINBOOL (WINAPI *pwglDeleteContext)(HGLRC);
	static HGLRC (WINAPI *pwglGetCurrentContext)(void);
	static HDC (WINAPI *pwglGetCurrentDC)(void);
	static WINBOOL (WINAPI *pwglMakeCurrent)(HDC, HGLRC);
	static WINBOOL (WINAPI *pwglShareLists)(HGLRC, HGLRC);
	static const GLubyte * (WINAPI *pwglGetExtensionsStringARB) (HDC hdc);
	static const GLubyte * (WINAPI *pwglGetExtensionsStringEXT) (void);

	void dispose();

public:
	Win32OpenglContext(void *glhandle);
	virtual ~Win32OpenglContext();
	virtual bool CreateContext(GLenum format, GLint depthBits, GLint stencilBits, GLint accumBits, OffscreenContext *sharelist);
	virtual GLboolean MakeCurrent(memptr buffer, GLenum type, GLsizei width, GLsizei height);
	virtual GLboolean MakeCurrent();
	virtual GLboolean ClearCurrent();
	virtual bool TestContext();

	static void InitPointers(void *lib_handle);
	static HGLRC CreateTmpContext(int &iPixelFormat, GLint _colorBits = 32, GLint _depthBits = 0, GLint _stencilBits = 0, GLint _accumBits = 0);
	static void DeleteTmpContext(HGLRC tmp_ctx);
	static SDL_GLContext GetSDLContext(void);
	static void SetSDLContext(SDL_GLContext ctx);
};
#endif

#if defined(SDL_VIDEO_DRIVER_QUARTZ) || defined(SDL_VIDEO_DRIVER_COCOA)

#include <OpenGL/OpenGL.h>

class QuartzOpenglContext : public OpenglContext {
protected:
	CGLContextObj ctx;

	void dispose();

public:
	QuartzOpenglContext(void *glhandle);
	virtual ~QuartzOpenglContext();
	virtual bool CreateContext(GLenum format, GLint depthBits, GLint stencilBits, GLint accumBits, OffscreenContext *sharelist);
	virtual GLboolean MakeCurrent(memptr buffer, GLenum type, GLsizei width, GLsizei height);
	virtual GLboolean MakeCurrent();
	virtual GLboolean ClearCurrent();
	virtual bool TestContext();
};
#endif

#endif /* NFOSMESA_CONTEXT_H */
