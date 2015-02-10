/* Bindings of tiny_gl.slb
 * Compile this module and link it with the application client
 */

#include <gem.h>
#include <mint/slb.h>
#include <slb/tiny_gl.h>
#include <mintbind.h>

#ifndef TRUE
# define TRUE 1
# define FALSE 0
#endif

#ifdef __PUREC__
#pragma warn -stv
#endif

struct _gl_tiny gl;

/*
 * The "nwords" argument should actually only be a "short".
 * MagiC will expect it that way, with the actual arguments
 * following.
 * However, a "short" in the actual function definition
 * will be treated as promoted to int.
 * So we pass a long instead, with the upper half
 * set to 1 + nwords to account for the extra space.
 * This also has the benefit of keeping the stack longword aligned.
 */
#undef SLB_NWORDS
#define SLB_NWORDS(_nwords) ((((long)(_nwords) + 1l) << 16) | (long)(_nwords))
#undef SLB_NARGS
#define SLB_NARGS(_nargs) SLB_NWORDS(_nargs * 2)

#ifndef __slb_lexec_defined
typedef long  __CDECL (*SLB_LEXEC)(SLB_HANDLE slb, long fn, long nwords, ...);
#define __slb_lexec_defined 1
#endif


#undef glFrustum
#undef glOrtho
#undef gluLookAt
#undef glClearDepth

static void APIENTRY exec_information(void)
{
	SLB_LEXEC exec = (SLB_LEXEC)gl_exec;
	(*exec)(gl_slb, 0, SLB_NARGS(0));
}

static void APIENTRY exec_glBegin(GLenum mode)
{
	SLB_LEXEC exec = (SLB_LEXEC)gl_exec;
	(*exec)(gl_slb, 1, SLB_NARGS(1), mode);
}

static void APIENTRY exec_glClear(GLbitfield mask)
{
	SLB_LEXEC exec = (SLB_LEXEC)gl_exec;
	(*exec)(gl_slb, 2, SLB_NARGS(1), mask);
}

static void APIENTRY exec_glClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
	SLB_LEXEC exec = (SLB_LEXEC)gl_exec;
	(*exec)(gl_slb, 3, SLB_NARGS(4), red, green, blue, alpha);
}

static void APIENTRY exec_glColor3f(GLfloat red, GLfloat green, GLfloat blue)
{
	SLB_LEXEC exec = (SLB_LEXEC)gl_exec;
	(*exec)(gl_slb, 4, SLB_NARGS(3), red, green, blue);
}

static void APIENTRY exec_glDisable(GLenum cap)
{
	SLB_LEXEC exec = (SLB_LEXEC)gl_exec;
	(*exec)(gl_slb, 5, SLB_NARGS(1), cap);
}

static void APIENTRY exec_glEnable(GLenum cap)
{
	SLB_LEXEC exec = (SLB_LEXEC)gl_exec;
	(*exec)(gl_slb, 6, SLB_NARGS(1), cap);
}

static void APIENTRY exec_glEnd(void)
{
	SLB_LEXEC exec = (SLB_LEXEC)gl_exec;
	(*exec)(gl_slb, 7, SLB_NARGS(0));
}

static void APIENTRY exec_glLightfv(GLenum light, GLenum pname, const GLfloat *params)
{
	SLB_LEXEC exec = (SLB_LEXEC)gl_exec;
	(*exec)(gl_slb, 8, SLB_NARGS(3), light, pname, params);
}

static void APIENTRY exec_glLoadIdentity(void)
{
	SLB_LEXEC exec = (SLB_LEXEC)gl_exec;
	(*exec)(gl_slb, 9, SLB_NARGS(0));
}

static void APIENTRY exec_glMaterialfv(GLenum face, GLenum pname, const GLfloat *params)
{
	SLB_LEXEC exec = (SLB_LEXEC)gl_exec;
	(*exec)(gl_slb, 10, SLB_NARGS(3), face, pname, params);
}

static void APIENTRY exec_glMatrixMode(GLenum mode)
{
	SLB_LEXEC exec = (SLB_LEXEC)gl_exec;
	(*exec)(gl_slb, 11, SLB_NARGS(1), mode);
}

static void APIENTRY exec_glOrthof(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat near_val, GLfloat far_val)
{
	SLB_LEXEC exec = (SLB_LEXEC)gl_exec;
	(*exec)(gl_slb, 12, SLB_NARGS(6), left, right, bottom, top, near_val, far_val);
}

static void APIENTRY exec_glPopMatrix(void)
{
	SLB_LEXEC exec = (SLB_LEXEC)gl_exec;
	(*exec)(gl_slb, 13, SLB_NARGS(0));
}

static void APIENTRY exec_glPushMatrix(void)
{
	SLB_LEXEC exec = (SLB_LEXEC)gl_exec;
	(*exec)(gl_slb, 14, SLB_NARGS(0));
}

static void APIENTRY exec_glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
{
	SLB_LEXEC exec = (SLB_LEXEC)gl_exec;
	(*exec)(gl_slb, 15, SLB_NARGS(4), angle, x, y, z);
}

static void APIENTRY exec_glTexEnvi(GLenum target, GLenum pname, GLint param)
{
	SLB_LEXEC exec = (SLB_LEXEC)gl_exec;
	(*exec)(gl_slb, 16, SLB_NARGS(3), target, pname, param);
}

static void APIENTRY exec_glTexParameteri(GLenum target, GLenum pname, GLint param)
{
	SLB_LEXEC exec = (SLB_LEXEC)gl_exec;
	(*exec)(gl_slb, 17, SLB_NARGS(3), target, pname, param);
}

static void APIENTRY exec_glTranslatef(GLfloat x, GLfloat y, GLfloat z)
{
	SLB_LEXEC exec = (SLB_LEXEC)gl_exec;
	(*exec)(gl_slb, 18, SLB_NARGS(3), x, y, z);
}

static void APIENTRY exec_glVertex2f(GLfloat x, GLfloat y)
{
	SLB_LEXEC exec = (SLB_LEXEC)gl_exec;
	(*exec)(gl_slb, 19, SLB_NARGS(2), x, y);
}

static void APIENTRY exec_glVertex3f(GLfloat x, GLfloat y, GLfloat z)
{
	SLB_LEXEC exec = (SLB_LEXEC)gl_exec;
	(*exec)(gl_slb, 20, SLB_NARGS(3), x, y, z);
}

static void * APIENTRY exec_OSMesaCreateLDG(GLenum format, GLenum type, GLint width, GLint height)
{
	SLB_LEXEC exec = (SLB_LEXEC)gl_exec;
	return (void *)(*exec)(gl_slb, 21, SLB_NARGS(4), format, type, width, height);
}

static void APIENTRY exec_OSMesaDestroyLDG(void)
{
	SLB_LEXEC exec = (SLB_LEXEC)gl_exec;
	(*exec)(gl_slb, 22, SLB_NARGS(0));
}

static void APIENTRY exec_glArrayElement(GLint i)
{
	SLB_LEXEC exec = (SLB_LEXEC)gl_exec;
	(*exec)(gl_slb, 23, SLB_NARGS(1), i);
}

static void APIENTRY exec_glBindTexture(GLenum target, GLuint texture)
{
	SLB_LEXEC exec = (SLB_LEXEC)gl_exec;
	(*exec)(gl_slb, 24, SLB_NARGS(2), target, texture);
}

static void APIENTRY exec_glCallList(GLuint list)
{
	SLB_LEXEC exec = (SLB_LEXEC)gl_exec;
	(*exec)(gl_slb, 25, SLB_NARGS(1), list);
}

static void APIENTRY exec_glClearDepthf(GLfloat d)
{
	SLB_LEXEC exec = (SLB_LEXEC)gl_exec;
	(*exec)(gl_slb, 26, SLB_NARGS(1), d);
}

static void APIENTRY exec_glColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
	SLB_LEXEC exec = (SLB_LEXEC)gl_exec;
	(*exec)(gl_slb, 27, SLB_NARGS(4), red, green, blue, alpha);
}

static void APIENTRY exec_glColor3fv(const GLfloat *v)
{
	SLB_LEXEC exec = (SLB_LEXEC)gl_exec;
	(*exec)(gl_slb, 28, SLB_NARGS(1), v);
}

static void APIENTRY exec_glColor4fv(const GLfloat *v)
{
	SLB_LEXEC exec = (SLB_LEXEC)gl_exec;
	(*exec)(gl_slb, 29, SLB_NARGS(1), v);
}

static void APIENTRY exec_glColorMaterial(GLenum face, GLenum mode)
{
	SLB_LEXEC exec = (SLB_LEXEC)gl_exec;
	(*exec)(gl_slb, 30, SLB_NARGS(2), face, mode);
}

static void APIENTRY exec_glColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
	SLB_LEXEC exec = (SLB_LEXEC)gl_exec;
	(*exec)(gl_slb, 31, SLB_NARGS(4), size, type, stride, pointer);
}

static void APIENTRY exec_glCullFace(GLenum mode)
{
	SLB_LEXEC exec = (SLB_LEXEC)gl_exec;
	(*exec)(gl_slb, 32, SLB_NARGS(1), mode);
}

static void APIENTRY exec_glDeleteTextures(GLsizei n, const GLuint *textures)
{
	SLB_LEXEC exec = (SLB_LEXEC)gl_exec;
	(*exec)(gl_slb, 33, SLB_NARGS(2), n, textures);
}

static void APIENTRY exec_glDisableClientState(GLenum array)
{
	SLB_LEXEC exec = (SLB_LEXEC)gl_exec;
	(*exec)(gl_slb, 34, SLB_NARGS(1), array);
}

static void APIENTRY exec_glEnableClientState(GLenum array)
{
	SLB_LEXEC exec = (SLB_LEXEC)gl_exec;
	(*exec)(gl_slb, 35, SLB_NARGS(1), array);
}

static void APIENTRY exec_glEndList(void)
{
	SLB_LEXEC exec = (SLB_LEXEC)gl_exec;
	(*exec)(gl_slb, 36, SLB_NARGS(0));
}

static void APIENTRY exec_glEdgeFlag(GLboolean32 flag)
{
	SLB_LEXEC exec = (SLB_LEXEC)gl_exec;
	(*exec)(gl_slb, 37, SLB_NARGS(1), flag);
}

static void APIENTRY exec_glFlush(void)
{
	SLB_LEXEC exec = (SLB_LEXEC)gl_exec;
	(*exec)(gl_slb, 38, SLB_NARGS(0));
}

static void APIENTRY exec_glFrontFace(GLenum mode)
{
	SLB_LEXEC exec = (SLB_LEXEC)gl_exec;
	(*exec)(gl_slb, 39, SLB_NARGS(1), mode);
}

static void APIENTRY exec_glFrustumf(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat near_val, GLfloat far_val)
{
	SLB_LEXEC exec = (SLB_LEXEC)gl_exec;
	(*exec)(gl_slb, 40, SLB_NARGS(6), left, right, bottom, top, near_val, far_val);
}

static GLuint APIENTRY exec_glGenLists(GLsizei range)
{
	SLB_LEXEC exec = (SLB_LEXEC)gl_exec;
	return (GLuint)(*exec)(gl_slb, 41, SLB_NARGS(1), range);
}

static void APIENTRY exec_glGenTextures(GLsizei n, GLuint *textures)
{
	SLB_LEXEC exec = (SLB_LEXEC)gl_exec;
	(*exec)(gl_slb, 42, SLB_NARGS(2), n, textures);
}

static void APIENTRY exec_glGetFloatv(GLenum pname, GLfloat *params)
{
	SLB_LEXEC exec = (SLB_LEXEC)gl_exec;
	(*exec)(gl_slb, 43, SLB_NARGS(2), pname, params);
}

static void APIENTRY exec_glGetIntegerv(GLenum pname, GLint *params)
{
	SLB_LEXEC exec = (SLB_LEXEC)gl_exec;
	(*exec)(gl_slb, 44, SLB_NARGS(2), pname, params);
}

static void APIENTRY exec_glHint(GLenum target, GLenum mode)
{
	SLB_LEXEC exec = (SLB_LEXEC)gl_exec;
	(*exec)(gl_slb, 45, SLB_NARGS(2), target, mode);
}

static void APIENTRY exec_glInitNames(void)
{
	SLB_LEXEC exec = (SLB_LEXEC)gl_exec;
	(*exec)(gl_slb, 46, SLB_NARGS(0));
}

static GLboolean APIENTRY exec_glIsList(GLuint list)
{
	SLB_LEXEC exec = (SLB_LEXEC)gl_exec;
	return (GLboolean)(*exec)(gl_slb, 47, SLB_NARGS(1), list);
}

static void APIENTRY exec_glLightf(GLenum light, GLenum pname, GLfloat param)
{
	SLB_LEXEC exec = (SLB_LEXEC)gl_exec;
	(*exec)(gl_slb, 48, SLB_NARGS(3), light, pname, param);
}

static void APIENTRY exec_glLightModeli(GLenum pname, GLint param)
{
	SLB_LEXEC exec = (SLB_LEXEC)gl_exec;
	(*exec)(gl_slb, 49, SLB_NARGS(2), pname, param);
}

static void APIENTRY exec_glLightModelfv(GLenum pname, const GLfloat *params)
{
	SLB_LEXEC exec = (SLB_LEXEC)gl_exec;
	(*exec)(gl_slb, 50, SLB_NARGS(2), pname, params);
}

static void APIENTRY exec_glLoadMatrixf(const GLfloat *m)
{
	SLB_LEXEC exec = (SLB_LEXEC)gl_exec;
	(*exec)(gl_slb, 51, SLB_NARGS(1), m);
}

static void APIENTRY exec_glLoadName(GLuint name)
{
	SLB_LEXEC exec = (SLB_LEXEC)gl_exec;
	(*exec)(gl_slb, 52, SLB_NARGS(1), name);
}

static void APIENTRY exec_glMaterialf(GLenum face, GLenum pname, GLfloat param)
{
	SLB_LEXEC exec = (SLB_LEXEC)gl_exec;
	(*exec)(gl_slb, 53, SLB_NARGS(3), face, pname, param);
}

static void APIENTRY exec_glMultMatrixf(const GLfloat *m)
{
	SLB_LEXEC exec = (SLB_LEXEC)gl_exec;
	(*exec)(gl_slb, 54, SLB_NARGS(1), m);
}

static void APIENTRY exec_glNewList(GLuint list, GLenum mode)
{
	SLB_LEXEC exec = (SLB_LEXEC)gl_exec;
	(*exec)(gl_slb, 55, SLB_NARGS(2), list, mode);
}

static void APIENTRY exec_glNormal3f(GLfloat nx, GLfloat ny, GLfloat nz)
{
	SLB_LEXEC exec = (SLB_LEXEC)gl_exec;
	(*exec)(gl_slb, 56, SLB_NARGS(3), nx, ny, nz);
}

static void APIENTRY exec_glNormal3fv(const GLfloat *v)
{
	SLB_LEXEC exec = (SLB_LEXEC)gl_exec;
	(*exec)(gl_slb, 57, SLB_NARGS(1), v);
}

static void APIENTRY exec_glNormalPointer(GLenum type, GLsizei stride, const GLvoid *pointer)
{
	SLB_LEXEC exec = (SLB_LEXEC)gl_exec;
	(*exec)(gl_slb, 58, SLB_NARGS(3), type, stride, pointer);
}

static void APIENTRY exec_glPixelStorei(GLenum pname, GLint param)
{
	SLB_LEXEC exec = (SLB_LEXEC)gl_exec;
	(*exec)(gl_slb, 59, SLB_NARGS(2), pname, param);
}

static void APIENTRY exec_glPolygonMode(GLenum face, GLenum mode)
{
	SLB_LEXEC exec = (SLB_LEXEC)gl_exec;
	(*exec)(gl_slb, 60, SLB_NARGS(2), face, mode);
}

static void APIENTRY exec_glPolygonOffset(GLfloat factor, GLfloat units)
{
	SLB_LEXEC exec = (SLB_LEXEC)gl_exec;
	(*exec)(gl_slb, 61, SLB_NARGS(2), factor, units);
}

static void APIENTRY exec_glPopName(void)
{
	SLB_LEXEC exec = (SLB_LEXEC)gl_exec;
	(*exec)(gl_slb, 62, SLB_NARGS(0));
}

static void APIENTRY exec_glPushName(GLuint name)
{
	SLB_LEXEC exec = (SLB_LEXEC)gl_exec;
	(*exec)(gl_slb, 63, SLB_NARGS(1), name);
}

static GLint APIENTRY exec_glRenderMode(GLenum mode)
{
	SLB_LEXEC exec = (SLB_LEXEC)gl_exec;
	return (GLint)(*exec)(gl_slb, 64, SLB_NARGS(1), mode);
}

static void APIENTRY exec_glSelectBuffer(GLsizei size, GLuint *buffer)
{
	SLB_LEXEC exec = (SLB_LEXEC)gl_exec;
	(*exec)(gl_slb, 65, SLB_NARGS(2), size, buffer);
}

static void APIENTRY exec_glScalef(GLfloat x, GLfloat y, GLfloat z)
{
	SLB_LEXEC exec = (SLB_LEXEC)gl_exec;
	(*exec)(gl_slb, 66, SLB_NARGS(3), x, y, z);
}

static void APIENTRY exec_glShadeModel(GLenum mode)
{
	SLB_LEXEC exec = (SLB_LEXEC)gl_exec;
	(*exec)(gl_slb, 67, SLB_NARGS(1), mode);
}

static void APIENTRY exec_glTexCoord2f(GLfloat s, GLfloat t)
{
	SLB_LEXEC exec = (SLB_LEXEC)gl_exec;
	(*exec)(gl_slb, 68, SLB_NARGS(2), s, t);
}

static void APIENTRY exec_glTexCoord4f(GLfloat s, GLfloat t, GLfloat r, GLfloat q)
{
	SLB_LEXEC exec = (SLB_LEXEC)gl_exec;
	(*exec)(gl_slb, 69, SLB_NARGS(4), s, t, r, q);
}

static void APIENTRY exec_glTexCoord2fv(const GLfloat *v)
{
	SLB_LEXEC exec = (SLB_LEXEC)gl_exec;
	(*exec)(gl_slb, 70, SLB_NARGS(1), v);
}

static void APIENTRY exec_glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
	SLB_LEXEC exec = (SLB_LEXEC)gl_exec;
	(*exec)(gl_slb, 71, SLB_NARGS(4), size, type, stride, pointer);
}

static void APIENTRY exec_glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels)
{
	SLB_LEXEC exec = (SLB_LEXEC)gl_exec;
	(*exec)(gl_slb, 72, SLB_NARGS(9), target, level, internalformat, width, height, border, format, type, pixels);
}

static void APIENTRY exec_glVertex4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
	SLB_LEXEC exec = (SLB_LEXEC)gl_exec;
	(*exec)(gl_slb, 73, SLB_NARGS(4), x, y, z, w);
}

static void APIENTRY exec_glVertex3fv(const GLfloat *v)
{
	SLB_LEXEC exec = (SLB_LEXEC)gl_exec;
	(*exec)(gl_slb, 74, SLB_NARGS(1), v);
}

static void APIENTRY exec_glVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
	SLB_LEXEC exec = (SLB_LEXEC)gl_exec;
	(*exec)(gl_slb, 75, SLB_NARGS(4), size, type, stride, pointer);
}

static void APIENTRY exec_glViewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
	SLB_LEXEC exec = (SLB_LEXEC)gl_exec;
	(*exec)(gl_slb, 76, SLB_NARGS(4), x, y, width, height);
}

static void APIENTRY exec_swapbuffer(void *buffer)
{
	SLB_LEXEC exec = (SLB_LEXEC)gl_exec;
	(*exec)(gl_slb, 77, SLB_NARGS(1), buffer);
}

static GLsizei APIENTRY exec_max_width(void)
{
	SLB_LEXEC exec = (SLB_LEXEC)gl_exec;
	return (GLsizei)(*exec)(gl_slb, 78, SLB_NARGS(0));
}

static GLsizei APIENTRY exec_max_height(void)
{
	SLB_LEXEC exec = (SLB_LEXEC)gl_exec;
	return (GLsizei)(*exec)(gl_slb, 79, SLB_NARGS(0));
}

static void APIENTRY exec_glDeleteLists(GLuint list, GLsizei range)
{
	SLB_LEXEC exec = (SLB_LEXEC)gl_exec;
	(*exec)(gl_slb, 80, SLB_NARGS(2), list, range);
}

static void APIENTRY exec_gluLookAtf(GLfloat eyeX, GLfloat eyeY, GLfloat eyeZ, GLfloat centerX, GLfloat centerY, GLfloat centerZ, GLfloat upX, GLfloat upY, GLfloat upZ)
{
	SLB_LEXEC exec = (SLB_LEXEC)gl_exec;
	(*exec)(gl_slb, 81, SLB_NARGS(9), eyeX, eyeY, eyeZ, centerX, centerY, centerZ, upX, upY, upZ);
}

static void APIENTRY exec_exception_error(void (CALLBACK *exception)(GLenum param) )
{
	SLB_LEXEC exec = (SLB_LEXEC)gl_exec;
	(*exec)(gl_slb, 82, SLB_NARGS(1), exception);
}

static void slb_init_tiny_gl(void)
{
	gl.information = exec_information;
	gl.Begin = exec_glBegin;
	gl.Clear = exec_glClear;
	gl.ClearColor = exec_glClearColor;
	gl.Color3f = exec_glColor3f;
	gl.Disable = exec_glDisable;
	gl.Enable = exec_glEnable;
	gl.End = exec_glEnd;
	gl.Lightfv = exec_glLightfv;
	gl.LoadIdentity = exec_glLoadIdentity;
	gl.Materialfv = exec_glMaterialfv;
	gl.MatrixMode = exec_glMatrixMode;
	gl.Orthof = exec_glOrthof;
	gl.PopMatrix = exec_glPopMatrix;
	gl.PushMatrix = exec_glPushMatrix;
	gl.Rotatef = exec_glRotatef;
	gl.TexEnvi = exec_glTexEnvi;
	gl.TexParameteri = exec_glTexParameteri;
	gl.Translatef = exec_glTranslatef;
	gl.Vertex2f = exec_glVertex2f;
	gl.Vertex3f = exec_glVertex3f;
	gl.OSMesaCreateLDG = exec_OSMesaCreateLDG;
	gl.OSMesaDestroyLDG = exec_OSMesaDestroyLDG;
	gl.ArrayElement = exec_glArrayElement;
	gl.BindTexture = exec_glBindTexture;
	gl.CallList = exec_glCallList;
	gl.ClearDepthf = exec_glClearDepthf;
	gl.Color4f = exec_glColor4f;
	gl.Color3fv = exec_glColor3fv;
	gl.Color4fv = exec_glColor4fv;
	gl.ColorMaterial = exec_glColorMaterial;
	gl.ColorPointer = exec_glColorPointer;
	gl.CullFace = exec_glCullFace;
	gl.DeleteTextures = exec_glDeleteTextures;
	gl.DisableClientState = exec_glDisableClientState;
	gl.EnableClientState = exec_glEnableClientState;
	gl.EndList = exec_glEndList;
	gl.EdgeFlag = exec_glEdgeFlag;
	gl.Flush = exec_glFlush;
	gl.FrontFace = exec_glFrontFace;
	gl.Frustumf = exec_glFrustumf;
	gl.GenLists = exec_glGenLists;
	gl.GenTextures = exec_glGenTextures;
	gl.GetFloatv = exec_glGetFloatv;
	gl.GetIntegerv = exec_glGetIntegerv;
	gl.Hint = exec_glHint;
	gl.InitNames = exec_glInitNames;
	gl.IsList = exec_glIsList;
	gl.Lightf = exec_glLightf;
	gl.LightModeli = exec_glLightModeli;
	gl.LightModelfv = exec_glLightModelfv;
	gl.LoadMatrixf = exec_glLoadMatrixf;
	gl.LoadName = exec_glLoadName;
	gl.Materialf = exec_glMaterialf;
	gl.MultMatrixf = exec_glMultMatrixf;
	gl.NewList = exec_glNewList;
	gl.Normal3f = exec_glNormal3f;
	gl.Normal3fv = exec_glNormal3fv;
	gl.NormalPointer = exec_glNormalPointer;
	gl.PixelStorei = exec_glPixelStorei;
	gl.PolygonMode = exec_glPolygonMode;
	gl.PolygonOffset = exec_glPolygonOffset;
	gl.PopName = exec_glPopName;
	gl.PushName = exec_glPushName;
	gl.RenderMode = exec_glRenderMode;
	gl.SelectBuffer = exec_glSelectBuffer;
	gl.Scalef = exec_glScalef;
	gl.ShadeModel = exec_glShadeModel;
	gl.TexCoord2f = exec_glTexCoord2f;
	gl.TexCoord4f = exec_glTexCoord4f;
	gl.TexCoord2fv = exec_glTexCoord2fv;
	gl.TexCoordPointer = exec_glTexCoordPointer;
	gl.TexImage2D = exec_glTexImage2D;
	gl.Vertex4f = exec_glVertex4f;
	gl.Vertex3fv = exec_glVertex3fv;
	gl.VertexPointer = exec_glVertexPointer;
	gl.Viewport = exec_glViewport;
	gl.swapbuffer = exec_swapbuffer;
	gl.max_width = exec_max_width;
	gl.max_height = exec_max_height;
	gl.DeleteLists = exec_glDeleteLists;
	gl.gluLookAtf = exec_gluLookAtf;
	gl.exception_error = exec_exception_error;
}


long slb_load_tiny_gl(const char *path, long min_version)
{
	long ret;
	
	ret = Slbopen("tiny_gl.slb", path, min_version, &gl_slb, &gl_exec);
	if (ret >= 0)
	{
		slb_init_tiny_gl();
	}
	return ret;
}


void slb_unload_tiny_gl(void)
{
	if (gl_slb != NULL)
	{
		Slbclose(gl_slb);
		gl_slb = 0;
	}
}
