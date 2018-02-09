/* Bindings of tiny_gl.slb
 * Compile this module and link it with the application client
 */

#include <gem.h>
#include <stdlib.h>
#include <mint/slb.h>
#define NFOSMESA_NO_MANGLE
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
static SLB_HANDLE gl_slb;
static SLB_EXEC gl_exec;
static struct gl_public *gl_pub;

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



#undef glClearDepth
#undef glFrustum
#undef glOrtho
#undef gluLookAt

static void APIENTRY exec_information(void)
{
	long  __CDECL (*exec)(SLB_HANDLE, long, long, void *, void *) = (long  __CDECL (*)(SLB_HANDLE, long, long, void *, void *))gl_exec;
	(*exec)(gl_slb, 1 /*  */, SLB_NARGS(2), gl_pub, NULL);
}

static void APIENTRY exec_glBegin(GLenum mode)
{
	long  __CDECL (*exec)(SLB_HANDLE, long, long, void *, void *) = (long  __CDECL (*)(SLB_HANDLE, long, long, void *, void *))gl_exec;
	(*exec)(gl_slb, 2 /* NFOSMESA_GLBEGIN */, SLB_NARGS(2), gl_pub, &mode);
}

static void APIENTRY exec_glClear(GLbitfield mask)
{
	long  __CDECL (*exec)(SLB_HANDLE, long, long, void *, void *) = (long  __CDECL (*)(SLB_HANDLE, long, long, void *, void *))gl_exec;
	(*exec)(gl_slb, 3 /* NFOSMESA_GLCLEAR */, SLB_NARGS(2), gl_pub, &mask);
}

static void APIENTRY exec_glClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
	long  __CDECL (*exec)(SLB_HANDLE, long, long, void *, void *) = (long  __CDECL (*)(SLB_HANDLE, long, long, void *, void *))gl_exec;
	(void)green;
	(void)blue;
	(void)alpha;
	(*exec)(gl_slb, 4 /* NFOSMESA_GLCLEARCOLOR */, SLB_NARGS(2), gl_pub, &red);
}

static void APIENTRY exec_glColor3f(GLfloat red, GLfloat green, GLfloat blue)
{
	long  __CDECL (*exec)(SLB_HANDLE, long, long, void *, void *) = (long  __CDECL (*)(SLB_HANDLE, long, long, void *, void *))gl_exec;
	(void)green;
	(void)blue;
	(*exec)(gl_slb, 5 /* NFOSMESA_GLCOLOR3F */, SLB_NARGS(2), gl_pub, &red);
}

static void APIENTRY exec_glDisable(GLenum cap)
{
	long  __CDECL (*exec)(SLB_HANDLE, long, long, void *, void *) = (long  __CDECL (*)(SLB_HANDLE, long, long, void *, void *))gl_exec;
	(*exec)(gl_slb, 6 /* NFOSMESA_GLDISABLE */, SLB_NARGS(2), gl_pub, &cap);
}

static void APIENTRY exec_glEnable(GLenum cap)
{
	long  __CDECL (*exec)(SLB_HANDLE, long, long, void *, void *) = (long  __CDECL (*)(SLB_HANDLE, long, long, void *, void *))gl_exec;
	(*exec)(gl_slb, 7 /* NFOSMESA_GLENABLE */, SLB_NARGS(2), gl_pub, &cap);
}

static void APIENTRY exec_glEnd(void)
{
	long  __CDECL (*exec)(SLB_HANDLE, long, long, void *, void *) = (long  __CDECL (*)(SLB_HANDLE, long, long, void *, void *))gl_exec;
	(*exec)(gl_slb, 8 /* NFOSMESA_GLEND */, SLB_NARGS(2), gl_pub, NULL);
}

static void APIENTRY exec_glLightfv(GLenum light, GLenum pname, const GLfloat *params)
{
	long  __CDECL (*exec)(SLB_HANDLE, long, long, void *, void *) = (long  __CDECL (*)(SLB_HANDLE, long, long, void *, void *))gl_exec;
	(void)pname;
	(void)params;
	(*exec)(gl_slb, 9 /* NFOSMESA_GLLIGHTFV */, SLB_NARGS(2), gl_pub, &light);
}

static void APIENTRY exec_glLoadIdentity(void)
{
	long  __CDECL (*exec)(SLB_HANDLE, long, long, void *, void *) = (long  __CDECL (*)(SLB_HANDLE, long, long, void *, void *))gl_exec;
	(*exec)(gl_slb, 10 /* NFOSMESA_GLLOADIDENTITY */, SLB_NARGS(2), gl_pub, NULL);
}

static void APIENTRY exec_glMaterialfv(GLenum face, GLenum pname, const GLfloat *params)
{
	long  __CDECL (*exec)(SLB_HANDLE, long, long, void *, void *) = (long  __CDECL (*)(SLB_HANDLE, long, long, void *, void *))gl_exec;
	(void)pname;
	(void)params;
	(*exec)(gl_slb, 11 /* NFOSMESA_GLMATERIALFV */, SLB_NARGS(2), gl_pub, &face);
}

static void APIENTRY exec_glMatrixMode(GLenum mode)
{
	long  __CDECL (*exec)(SLB_HANDLE, long, long, void *, void *) = (long  __CDECL (*)(SLB_HANDLE, long, long, void *, void *))gl_exec;
	(*exec)(gl_slb, 12 /* NFOSMESA_GLMATRIXMODE */, SLB_NARGS(2), gl_pub, &mode);
}

static void APIENTRY exec_glOrthof(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat near_val, GLfloat far_val)
{
	long  __CDECL (*exec)(SLB_HANDLE, long, long, void *, void *) = (long  __CDECL (*)(SLB_HANDLE, long, long, void *, void *))gl_exec;
	(void)right;
	(void)bottom;
	(void)top;
	(void)near_val;
	(void)far_val;
	(*exec)(gl_slb, 13 /* NFOSMESA_GLORTHOF */, SLB_NARGS(2), gl_pub, &left);
}

static void APIENTRY exec_glPopMatrix(void)
{
	long  __CDECL (*exec)(SLB_HANDLE, long, long, void *, void *) = (long  __CDECL (*)(SLB_HANDLE, long, long, void *, void *))gl_exec;
	(*exec)(gl_slb, 14 /* NFOSMESA_GLPOPMATRIX */, SLB_NARGS(2), gl_pub, NULL);
}

static void APIENTRY exec_glPushMatrix(void)
{
	long  __CDECL (*exec)(SLB_HANDLE, long, long, void *, void *) = (long  __CDECL (*)(SLB_HANDLE, long, long, void *, void *))gl_exec;
	(*exec)(gl_slb, 15 /* NFOSMESA_GLPUSHMATRIX */, SLB_NARGS(2), gl_pub, NULL);
}

static void APIENTRY exec_glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
{
	long  __CDECL (*exec)(SLB_HANDLE, long, long, void *, void *) = (long  __CDECL (*)(SLB_HANDLE, long, long, void *, void *))gl_exec;
	(void)x;
	(void)y;
	(void)z;
	(*exec)(gl_slb, 16 /* NFOSMESA_GLROTATEF */, SLB_NARGS(2), gl_pub, &angle);
}

static void APIENTRY exec_glTexEnvi(GLenum target, GLenum pname, GLint param)
{
	long  __CDECL (*exec)(SLB_HANDLE, long, long, void *, void *) = (long  __CDECL (*)(SLB_HANDLE, long, long, void *, void *))gl_exec;
	(void)pname;
	(void)param;
	(*exec)(gl_slb, 17 /* NFOSMESA_GLTEXENVI */, SLB_NARGS(2), gl_pub, &target);
}

static void APIENTRY exec_glTexParameteri(GLenum target, GLenum pname, GLint param)
{
	long  __CDECL (*exec)(SLB_HANDLE, long, long, void *, void *) = (long  __CDECL (*)(SLB_HANDLE, long, long, void *, void *))gl_exec;
	(void)pname;
	(void)param;
	(*exec)(gl_slb, 18 /* NFOSMESA_GLTEXPARAMETERI */, SLB_NARGS(2), gl_pub, &target);
}

static void APIENTRY exec_glTranslatef(GLfloat x, GLfloat y, GLfloat z)
{
	long  __CDECL (*exec)(SLB_HANDLE, long, long, void *, void *) = (long  __CDECL (*)(SLB_HANDLE, long, long, void *, void *))gl_exec;
	(void)y;
	(void)z;
	(*exec)(gl_slb, 19 /* NFOSMESA_GLTRANSLATEF */, SLB_NARGS(2), gl_pub, &x);
}

static void APIENTRY exec_glVertex2f(GLfloat x, GLfloat y)
{
	long  __CDECL (*exec)(SLB_HANDLE, long, long, void *, void *) = (long  __CDECL (*)(SLB_HANDLE, long, long, void *, void *))gl_exec;
	(void)y;
	(*exec)(gl_slb, 20 /* NFOSMESA_GLVERTEX2F */, SLB_NARGS(2), gl_pub, &x);
}

static void APIENTRY exec_glVertex3f(GLfloat x, GLfloat y, GLfloat z)
{
	long  __CDECL (*exec)(SLB_HANDLE, long, long, void *, void *) = (long  __CDECL (*)(SLB_HANDLE, long, long, void *, void *))gl_exec;
	(void)y;
	(void)z;
	(*exec)(gl_slb, 21 /* NFOSMESA_GLVERTEX3F */, SLB_NARGS(2), gl_pub, &x);
}

static void * APIENTRY exec_OSMesaCreateLDG(GLenum format, GLenum type, GLint width, GLint height)
{
	long  __CDECL (*exec)(SLB_HANDLE, long, long, void *, void *) = (long  __CDECL (*)(SLB_HANDLE, long, long, void *, void *))gl_exec;
	(void)type;
	(void)width;
	(void)height;
	return (void *)(*exec)(gl_slb, 22 /*  */, SLB_NARGS(2), gl_pub, &format);
}

static void APIENTRY exec_OSMesaDestroyLDG(void)
{
	long  __CDECL (*exec)(SLB_HANDLE, long, long, void *, void *) = (long  __CDECL (*)(SLB_HANDLE, long, long, void *, void *))gl_exec;
	(*exec)(gl_slb, 23 /*  */, SLB_NARGS(2), gl_pub, NULL);
}

static void APIENTRY exec_glArrayElement(GLint i)
{
	long  __CDECL (*exec)(SLB_HANDLE, long, long, void *, void *) = (long  __CDECL (*)(SLB_HANDLE, long, long, void *, void *))gl_exec;
	(*exec)(gl_slb, 24 /* NFOSMESA_GLARRAYELEMENT */, SLB_NARGS(2), gl_pub, &i);
}

static void APIENTRY exec_glBindTexture(GLenum target, GLuint texture)
{
	long  __CDECL (*exec)(SLB_HANDLE, long, long, void *, void *) = (long  __CDECL (*)(SLB_HANDLE, long, long, void *, void *))gl_exec;
	(void)texture;
	(*exec)(gl_slb, 25 /* NFOSMESA_GLBINDTEXTURE */, SLB_NARGS(2), gl_pub, &target);
}

static void APIENTRY exec_glCallList(GLuint list)
{
	long  __CDECL (*exec)(SLB_HANDLE, long, long, void *, void *) = (long  __CDECL (*)(SLB_HANDLE, long, long, void *, void *))gl_exec;
	(*exec)(gl_slb, 26 /* NFOSMESA_GLCALLLIST */, SLB_NARGS(2), gl_pub, &list);
}

static void APIENTRY exec_glClearDepthf(GLfloat d)
{
	long  __CDECL (*exec)(SLB_HANDLE, long, long, void *, void *) = (long  __CDECL (*)(SLB_HANDLE, long, long, void *, void *))gl_exec;
	(*exec)(gl_slb, 27 /* NFOSMESA_GLCLEARDEPTHF */, SLB_NARGS(2), gl_pub, &d);
}

static void APIENTRY exec_glColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
	long  __CDECL (*exec)(SLB_HANDLE, long, long, void *, void *) = (long  __CDECL (*)(SLB_HANDLE, long, long, void *, void *))gl_exec;
	(void)green;
	(void)blue;
	(void)alpha;
	(*exec)(gl_slb, 28 /* NFOSMESA_GLCOLOR4F */, SLB_NARGS(2), gl_pub, &red);
}

static void APIENTRY exec_glColor3fv(const GLfloat *v)
{
	long  __CDECL (*exec)(SLB_HANDLE, long, long, void *, void *) = (long  __CDECL (*)(SLB_HANDLE, long, long, void *, void *))gl_exec;
	(*exec)(gl_slb, 29 /* NFOSMESA_GLCOLOR3FV */, SLB_NARGS(2), gl_pub, &v);
}

static void APIENTRY exec_glColor4fv(const GLfloat *v)
{
	long  __CDECL (*exec)(SLB_HANDLE, long, long, void *, void *) = (long  __CDECL (*)(SLB_HANDLE, long, long, void *, void *))gl_exec;
	(*exec)(gl_slb, 30 /* NFOSMESA_GLCOLOR4FV */, SLB_NARGS(2), gl_pub, &v);
}

static void APIENTRY exec_glColorMaterial(GLenum face, GLenum mode)
{
	long  __CDECL (*exec)(SLB_HANDLE, long, long, void *, void *) = (long  __CDECL (*)(SLB_HANDLE, long, long, void *, void *))gl_exec;
	(void)mode;
	(*exec)(gl_slb, 31 /* NFOSMESA_GLCOLORMATERIAL */, SLB_NARGS(2), gl_pub, &face);
}

static void APIENTRY exec_glColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
	long  __CDECL (*exec)(SLB_HANDLE, long, long, void *, void *) = (long  __CDECL (*)(SLB_HANDLE, long, long, void *, void *))gl_exec;
	(void)type;
	(void)stride;
	(void)pointer;
	(*exec)(gl_slb, 32 /* NFOSMESA_GLCOLORPOINTER */, SLB_NARGS(2), gl_pub, &size);
}

static void APIENTRY exec_glCullFace(GLenum mode)
{
	long  __CDECL (*exec)(SLB_HANDLE, long, long, void *, void *) = (long  __CDECL (*)(SLB_HANDLE, long, long, void *, void *))gl_exec;
	(*exec)(gl_slb, 33 /* NFOSMESA_GLCULLFACE */, SLB_NARGS(2), gl_pub, &mode);
}

static void APIENTRY exec_glDeleteTextures(GLsizei n, const GLuint *textures)
{
	long  __CDECL (*exec)(SLB_HANDLE, long, long, void *, void *) = (long  __CDECL (*)(SLB_HANDLE, long, long, void *, void *))gl_exec;
	(void)textures;
	(*exec)(gl_slb, 34 /* NFOSMESA_GLDELETETEXTURES */, SLB_NARGS(2), gl_pub, &n);
}

static void APIENTRY exec_glDisableClientState(GLenum array)
{
	long  __CDECL (*exec)(SLB_HANDLE, long, long, void *, void *) = (long  __CDECL (*)(SLB_HANDLE, long, long, void *, void *))gl_exec;
	(*exec)(gl_slb, 35 /* NFOSMESA_GLDISABLECLIENTSTATE */, SLB_NARGS(2), gl_pub, &array);
}

static void APIENTRY exec_glEnableClientState(GLenum array)
{
	long  __CDECL (*exec)(SLB_HANDLE, long, long, void *, void *) = (long  __CDECL (*)(SLB_HANDLE, long, long, void *, void *))gl_exec;
	(*exec)(gl_slb, 36 /* NFOSMESA_GLENABLECLIENTSTATE */, SLB_NARGS(2), gl_pub, &array);
}

static void APIENTRY exec_glEndList(void)
{
	long  __CDECL (*exec)(SLB_HANDLE, long, long, void *, void *) = (long  __CDECL (*)(SLB_HANDLE, long, long, void *, void *))gl_exec;
	(*exec)(gl_slb, 37 /* NFOSMESA_GLENDLIST */, SLB_NARGS(2), gl_pub, NULL);
}

static void APIENTRY exec_glEdgeFlag(GLboolean32 flag)
{
	long  __CDECL (*exec)(SLB_HANDLE, long, long, void *, void *) = (long  __CDECL (*)(SLB_HANDLE, long, long, void *, void *))gl_exec;
	(*exec)(gl_slb, 38 /* NFOSMESA_GLEDGEFLAG */, SLB_NARGS(2), gl_pub, &flag);
}

static void APIENTRY exec_glFlush(void)
{
	long  __CDECL (*exec)(SLB_HANDLE, long, long, void *, void *) = (long  __CDECL (*)(SLB_HANDLE, long, long, void *, void *))gl_exec;
	(*exec)(gl_slb, 39 /* NFOSMESA_GLFLUSH */, SLB_NARGS(2), gl_pub, NULL);
}

static void APIENTRY exec_glFrontFace(GLenum mode)
{
	long  __CDECL (*exec)(SLB_HANDLE, long, long, void *, void *) = (long  __CDECL (*)(SLB_HANDLE, long, long, void *, void *))gl_exec;
	(*exec)(gl_slb, 40 /* NFOSMESA_GLFRONTFACE */, SLB_NARGS(2), gl_pub, &mode);
}

static void APIENTRY exec_glFrustumf(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat near_val, GLfloat far_val)
{
	long  __CDECL (*exec)(SLB_HANDLE, long, long, void *, void *) = (long  __CDECL (*)(SLB_HANDLE, long, long, void *, void *))gl_exec;
	(void)right;
	(void)bottom;
	(void)top;
	(void)near_val;
	(void)far_val;
	(*exec)(gl_slb, 41 /* NFOSMESA_GLFRUSTUMF */, SLB_NARGS(2), gl_pub, &left);
}

static GLuint APIENTRY exec_glGenLists(GLsizei range)
{
	long  __CDECL (*exec)(SLB_HANDLE, long, long, void *, void *) = (long  __CDECL (*)(SLB_HANDLE, long, long, void *, void *))gl_exec;
	return (GLuint)(*exec)(gl_slb, 42 /* NFOSMESA_GLGENLISTS */, SLB_NARGS(2), gl_pub, &range);
}

static void APIENTRY exec_glGenTextures(GLsizei n, GLuint *textures)
{
	long  __CDECL (*exec)(SLB_HANDLE, long, long, void *, void *) = (long  __CDECL (*)(SLB_HANDLE, long, long, void *, void *))gl_exec;
	(void)textures;
	(*exec)(gl_slb, 43 /* NFOSMESA_GLGENTEXTURES */, SLB_NARGS(2), gl_pub, &n);
}

static void APIENTRY exec_glGetFloatv(GLenum pname, GLfloat *params)
{
	long  __CDECL (*exec)(SLB_HANDLE, long, long, void *, void *) = (long  __CDECL (*)(SLB_HANDLE, long, long, void *, void *))gl_exec;
	(void)params;
	(*exec)(gl_slb, 44 /* NFOSMESA_GLGETFLOATV */, SLB_NARGS(2), gl_pub, &pname);
}

static void APIENTRY exec_glGetIntegerv(GLenum pname, GLint *params)
{
	long  __CDECL (*exec)(SLB_HANDLE, long, long, void *, void *) = (long  __CDECL (*)(SLB_HANDLE, long, long, void *, void *))gl_exec;
	(void)params;
	(*exec)(gl_slb, 45 /* NFOSMESA_GLGETINTEGERV */, SLB_NARGS(2), gl_pub, &pname);
}

static void APIENTRY exec_glHint(GLenum target, GLenum mode)
{
	long  __CDECL (*exec)(SLB_HANDLE, long, long, void *, void *) = (long  __CDECL (*)(SLB_HANDLE, long, long, void *, void *))gl_exec;
	(void)mode;
	(*exec)(gl_slb, 46 /* NFOSMESA_GLHINT */, SLB_NARGS(2), gl_pub, &target);
}

static void APIENTRY exec_glInitNames(void)
{
	long  __CDECL (*exec)(SLB_HANDLE, long, long, void *, void *) = (long  __CDECL (*)(SLB_HANDLE, long, long, void *, void *))gl_exec;
	(*exec)(gl_slb, 47 /* NFOSMESA_GLINITNAMES */, SLB_NARGS(2), gl_pub, NULL);
}

static GLboolean APIENTRY exec_glIsList(GLuint list)
{
	long  __CDECL (*exec)(SLB_HANDLE, long, long, void *, void *) = (long  __CDECL (*)(SLB_HANDLE, long, long, void *, void *))gl_exec;
	return (GLboolean)(*exec)(gl_slb, 48 /* NFOSMESA_GLISLIST */, SLB_NARGS(2), gl_pub, &list);
}

static void APIENTRY exec_glLightf(GLenum light, GLenum pname, GLfloat param)
{
	long  __CDECL (*exec)(SLB_HANDLE, long, long, void *, void *) = (long  __CDECL (*)(SLB_HANDLE, long, long, void *, void *))gl_exec;
	(void)pname;
	(void)param;
	(*exec)(gl_slb, 49 /* NFOSMESA_GLLIGHTF */, SLB_NARGS(2), gl_pub, &light);
}

static void APIENTRY exec_glLightModeli(GLenum pname, GLint param)
{
	long  __CDECL (*exec)(SLB_HANDLE, long, long, void *, void *) = (long  __CDECL (*)(SLB_HANDLE, long, long, void *, void *))gl_exec;
	(void)param;
	(*exec)(gl_slb, 50 /* NFOSMESA_GLLIGHTMODELI */, SLB_NARGS(2), gl_pub, &pname);
}

static void APIENTRY exec_glLightModelfv(GLenum pname, const GLfloat *params)
{
	long  __CDECL (*exec)(SLB_HANDLE, long, long, void *, void *) = (long  __CDECL (*)(SLB_HANDLE, long, long, void *, void *))gl_exec;
	(void)params;
	(*exec)(gl_slb, 51 /* NFOSMESA_GLLIGHTMODELFV */, SLB_NARGS(2), gl_pub, &pname);
}

static void APIENTRY exec_glLoadMatrixf(const GLfloat *m)
{
	long  __CDECL (*exec)(SLB_HANDLE, long, long, void *, void *) = (long  __CDECL (*)(SLB_HANDLE, long, long, void *, void *))gl_exec;
	(*exec)(gl_slb, 52 /* NFOSMESA_GLLOADMATRIXF */, SLB_NARGS(2), gl_pub, &m);
}

static void APIENTRY exec_glLoadName(GLuint name)
{
	long  __CDECL (*exec)(SLB_HANDLE, long, long, void *, void *) = (long  __CDECL (*)(SLB_HANDLE, long, long, void *, void *))gl_exec;
	(*exec)(gl_slb, 53 /* NFOSMESA_GLLOADNAME */, SLB_NARGS(2), gl_pub, &name);
}

static void APIENTRY exec_glMaterialf(GLenum face, GLenum pname, GLfloat param)
{
	long  __CDECL (*exec)(SLB_HANDLE, long, long, void *, void *) = (long  __CDECL (*)(SLB_HANDLE, long, long, void *, void *))gl_exec;
	(void)pname;
	(void)param;
	(*exec)(gl_slb, 54 /* NFOSMESA_GLMATERIALF */, SLB_NARGS(2), gl_pub, &face);
}

static void APIENTRY exec_glMultMatrixf(const GLfloat *m)
{
	long  __CDECL (*exec)(SLB_HANDLE, long, long, void *, void *) = (long  __CDECL (*)(SLB_HANDLE, long, long, void *, void *))gl_exec;
	(*exec)(gl_slb, 55 /* NFOSMESA_GLMULTMATRIXF */, SLB_NARGS(2), gl_pub, &m);
}

static void APIENTRY exec_glNewList(GLuint list, GLenum mode)
{
	long  __CDECL (*exec)(SLB_HANDLE, long, long, void *, void *) = (long  __CDECL (*)(SLB_HANDLE, long, long, void *, void *))gl_exec;
	(void)mode;
	(*exec)(gl_slb, 56 /* NFOSMESA_GLNEWLIST */, SLB_NARGS(2), gl_pub, &list);
}

static void APIENTRY exec_glNormal3f(GLfloat nx, GLfloat ny, GLfloat nz)
{
	long  __CDECL (*exec)(SLB_HANDLE, long, long, void *, void *) = (long  __CDECL (*)(SLB_HANDLE, long, long, void *, void *))gl_exec;
	(void)ny;
	(void)nz;
	(*exec)(gl_slb, 57 /* NFOSMESA_GLNORMAL3F */, SLB_NARGS(2), gl_pub, &nx);
}

static void APIENTRY exec_glNormal3fv(const GLfloat *v)
{
	long  __CDECL (*exec)(SLB_HANDLE, long, long, void *, void *) = (long  __CDECL (*)(SLB_HANDLE, long, long, void *, void *))gl_exec;
	(*exec)(gl_slb, 58 /* NFOSMESA_GLNORMAL3FV */, SLB_NARGS(2), gl_pub, &v);
}

static void APIENTRY exec_glNormalPointer(GLenum type, GLsizei stride, const GLvoid *pointer)
{
	long  __CDECL (*exec)(SLB_HANDLE, long, long, void *, void *) = (long  __CDECL (*)(SLB_HANDLE, long, long, void *, void *))gl_exec;
	(void)stride;
	(void)pointer;
	(*exec)(gl_slb, 59 /* NFOSMESA_GLNORMALPOINTER */, SLB_NARGS(2), gl_pub, &type);
}

static void APIENTRY exec_glPixelStorei(GLenum pname, GLint param)
{
	long  __CDECL (*exec)(SLB_HANDLE, long, long, void *, void *) = (long  __CDECL (*)(SLB_HANDLE, long, long, void *, void *))gl_exec;
	(void)param;
	(*exec)(gl_slb, 60 /* NFOSMESA_GLPIXELSTOREI */, SLB_NARGS(2), gl_pub, &pname);
}

static void APIENTRY exec_glPolygonMode(GLenum face, GLenum mode)
{
	long  __CDECL (*exec)(SLB_HANDLE, long, long, void *, void *) = (long  __CDECL (*)(SLB_HANDLE, long, long, void *, void *))gl_exec;
	(void)mode;
	(*exec)(gl_slb, 61 /* NFOSMESA_GLPOLYGONMODE */, SLB_NARGS(2), gl_pub, &face);
}

static void APIENTRY exec_glPolygonOffset(GLfloat factor, GLfloat units)
{
	long  __CDECL (*exec)(SLB_HANDLE, long, long, void *, void *) = (long  __CDECL (*)(SLB_HANDLE, long, long, void *, void *))gl_exec;
	(void)units;
	(*exec)(gl_slb, 62 /* NFOSMESA_GLPOLYGONOFFSET */, SLB_NARGS(2), gl_pub, &factor);
}

static void APIENTRY exec_glPopName(void)
{
	long  __CDECL (*exec)(SLB_HANDLE, long, long, void *, void *) = (long  __CDECL (*)(SLB_HANDLE, long, long, void *, void *))gl_exec;
	(*exec)(gl_slb, 63 /* NFOSMESA_GLPOPNAME */, SLB_NARGS(2), gl_pub, NULL);
}

static void APIENTRY exec_glPushName(GLuint name)
{
	long  __CDECL (*exec)(SLB_HANDLE, long, long, void *, void *) = (long  __CDECL (*)(SLB_HANDLE, long, long, void *, void *))gl_exec;
	(*exec)(gl_slb, 64 /* NFOSMESA_GLPUSHNAME */, SLB_NARGS(2), gl_pub, &name);
}

static GLint APIENTRY exec_glRenderMode(GLenum mode)
{
	long  __CDECL (*exec)(SLB_HANDLE, long, long, void *, void *) = (long  __CDECL (*)(SLB_HANDLE, long, long, void *, void *))gl_exec;
	return (GLint)(*exec)(gl_slb, 65 /* NFOSMESA_GLRENDERMODE */, SLB_NARGS(2), gl_pub, &mode);
}

static void APIENTRY exec_glSelectBuffer(GLsizei size, GLuint *buffer)
{
	long  __CDECL (*exec)(SLB_HANDLE, long, long, void *, void *) = (long  __CDECL (*)(SLB_HANDLE, long, long, void *, void *))gl_exec;
	(void)buffer;
	(*exec)(gl_slb, 66 /* NFOSMESA_GLSELECTBUFFER */, SLB_NARGS(2), gl_pub, &size);
}

static void APIENTRY exec_glScalef(GLfloat x, GLfloat y, GLfloat z)
{
	long  __CDECL (*exec)(SLB_HANDLE, long, long, void *, void *) = (long  __CDECL (*)(SLB_HANDLE, long, long, void *, void *))gl_exec;
	(void)y;
	(void)z;
	(*exec)(gl_slb, 67 /* NFOSMESA_GLSCALEF */, SLB_NARGS(2), gl_pub, &x);
}

static void APIENTRY exec_glShadeModel(GLenum mode)
{
	long  __CDECL (*exec)(SLB_HANDLE, long, long, void *, void *) = (long  __CDECL (*)(SLB_HANDLE, long, long, void *, void *))gl_exec;
	(*exec)(gl_slb, 68 /* NFOSMESA_GLSHADEMODEL */, SLB_NARGS(2), gl_pub, &mode);
}

static void APIENTRY exec_glTexCoord2f(GLfloat s, GLfloat t)
{
	long  __CDECL (*exec)(SLB_HANDLE, long, long, void *, void *) = (long  __CDECL (*)(SLB_HANDLE, long, long, void *, void *))gl_exec;
	(void)t;
	(*exec)(gl_slb, 69 /* NFOSMESA_GLTEXCOORD2F */, SLB_NARGS(2), gl_pub, &s);
}

static void APIENTRY exec_glTexCoord4f(GLfloat s, GLfloat t, GLfloat r, GLfloat q)
{
	long  __CDECL (*exec)(SLB_HANDLE, long, long, void *, void *) = (long  __CDECL (*)(SLB_HANDLE, long, long, void *, void *))gl_exec;
	(void)t;
	(void)r;
	(void)q;
	(*exec)(gl_slb, 70 /* NFOSMESA_GLTEXCOORD4F */, SLB_NARGS(2), gl_pub, &s);
}

static void APIENTRY exec_glTexCoord2fv(const GLfloat *v)
{
	long  __CDECL (*exec)(SLB_HANDLE, long, long, void *, void *) = (long  __CDECL (*)(SLB_HANDLE, long, long, void *, void *))gl_exec;
	(*exec)(gl_slb, 71 /* NFOSMESA_GLTEXCOORD2FV */, SLB_NARGS(2), gl_pub, &v);
}

static void APIENTRY exec_glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
	long  __CDECL (*exec)(SLB_HANDLE, long, long, void *, void *) = (long  __CDECL (*)(SLB_HANDLE, long, long, void *, void *))gl_exec;
	(void)type;
	(void)stride;
	(void)pointer;
	(*exec)(gl_slb, 72 /* NFOSMESA_GLTEXCOORDPOINTER */, SLB_NARGS(2), gl_pub, &size);
}

static void APIENTRY exec_glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels)
{
	long  __CDECL (*exec)(SLB_HANDLE, long, long, void *, void *) = (long  __CDECL (*)(SLB_HANDLE, long, long, void *, void *))gl_exec;
	(void)level;
	(void)internalformat;
	(void)width;
	(void)height;
	(void)border;
	(void)format;
	(void)type;
	(void)pixels;
	(*exec)(gl_slb, 73 /* NFOSMESA_GLTEXIMAGE2D */, SLB_NARGS(2), gl_pub, &target);
}

static void APIENTRY exec_glVertex4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
	long  __CDECL (*exec)(SLB_HANDLE, long, long, void *, void *) = (long  __CDECL (*)(SLB_HANDLE, long, long, void *, void *))gl_exec;
	(void)y;
	(void)z;
	(void)w;
	(*exec)(gl_slb, 74 /* NFOSMESA_GLVERTEX4F */, SLB_NARGS(2), gl_pub, &x);
}

static void APIENTRY exec_glVertex3fv(const GLfloat *v)
{
	long  __CDECL (*exec)(SLB_HANDLE, long, long, void *, void *) = (long  __CDECL (*)(SLB_HANDLE, long, long, void *, void *))gl_exec;
	(*exec)(gl_slb, 75 /* NFOSMESA_GLVERTEX3FV */, SLB_NARGS(2), gl_pub, &v);
}

static void APIENTRY exec_glVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
	long  __CDECL (*exec)(SLB_HANDLE, long, long, void *, void *) = (long  __CDECL (*)(SLB_HANDLE, long, long, void *, void *))gl_exec;
	(void)type;
	(void)stride;
	(void)pointer;
	(*exec)(gl_slb, 76 /* NFOSMESA_GLVERTEXPOINTER */, SLB_NARGS(2), gl_pub, &size);
}

static void APIENTRY exec_glViewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
	long  __CDECL (*exec)(SLB_HANDLE, long, long, void *, void *) = (long  __CDECL (*)(SLB_HANDLE, long, long, void *, void *))gl_exec;
	(void)y;
	(void)width;
	(void)height;
	(*exec)(gl_slb, 77 /* NFOSMESA_GLVIEWPORT */, SLB_NARGS(2), gl_pub, &x);
}

static void APIENTRY exec_swapbuffer(void *buffer)
{
	long  __CDECL (*exec)(SLB_HANDLE, long, long, void *, void *) = (long  __CDECL (*)(SLB_HANDLE, long, long, void *, void *))gl_exec;
	(*exec)(gl_slb, 78 /* NFOSMESA_TINYGLSWAPBUFFER */, SLB_NARGS(2), gl_pub, &buffer);
}

static GLsizei APIENTRY exec_max_width(void)
{
	long  __CDECL (*exec)(SLB_HANDLE, long, long, void *, void *) = (long  __CDECL (*)(SLB_HANDLE, long, long, void *, void *))gl_exec;
	return (GLsizei)(*exec)(gl_slb, 79 /*  */, SLB_NARGS(2), gl_pub, NULL);
}

static GLsizei APIENTRY exec_max_height(void)
{
	long  __CDECL (*exec)(SLB_HANDLE, long, long, void *, void *) = (long  __CDECL (*)(SLB_HANDLE, long, long, void *, void *))gl_exec;
	return (GLsizei)(*exec)(gl_slb, 80 /*  */, SLB_NARGS(2), gl_pub, NULL);
}

static void APIENTRY exec_glDeleteLists(GLuint list, GLsizei range)
{
	long  __CDECL (*exec)(SLB_HANDLE, long, long, void *, void *) = (long  __CDECL (*)(SLB_HANDLE, long, long, void *, void *))gl_exec;
	(void)range;
	(*exec)(gl_slb, 81 /* NFOSMESA_GLDELETELISTS */, SLB_NARGS(2), gl_pub, &list);
}

static void APIENTRY exec_gluLookAtf(GLfloat eyeX, GLfloat eyeY, GLfloat eyeZ, GLfloat centerX, GLfloat centerY, GLfloat centerZ, GLfloat upX, GLfloat upY, GLfloat upZ)
{
	long  __CDECL (*exec)(SLB_HANDLE, long, long, void *, void *) = (long  __CDECL (*)(SLB_HANDLE, long, long, void *, void *))gl_exec;
	(void)eyeY;
	(void)eyeZ;
	(void)centerX;
	(void)centerY;
	(void)centerZ;
	(void)upX;
	(void)upY;
	(void)upZ;
	(*exec)(gl_slb, 82 /* NFOSMESA_GLULOOKATF */, SLB_NARGS(2), gl_pub, &eyeX);
}

static void APIENTRY exec_exception_error(void (CALLBACK *exception)(GLenum param) )
{
	long  __CDECL (*exec)(SLB_HANDLE, long, long, void *, void *) = (long  __CDECL (*)(SLB_HANDLE, long, long, void *, void *))gl_exec;
	(*exec)(gl_slb, 83 /*  */, SLB_NARGS(2), gl_pub, &exception);
}

static void slb_init_tiny_gl(void)
{
	struct _gl_tiny *glp = &gl;
	glp->information = exec_information;
	glp->Begin = exec_glBegin;
	glp->Clear = exec_glClear;
	glp->ClearColor = exec_glClearColor;
	glp->Color3f = exec_glColor3f;
	glp->Disable = exec_glDisable;
	glp->Enable = exec_glEnable;
	glp->End = exec_glEnd;
	glp->Lightfv = exec_glLightfv;
	glp->LoadIdentity = exec_glLoadIdentity;
	glp->Materialfv = exec_glMaterialfv;
	glp->MatrixMode = exec_glMatrixMode;
	glp->Orthof = exec_glOrthof;
	glp->PopMatrix = exec_glPopMatrix;
	glp->PushMatrix = exec_glPushMatrix;
	glp->Rotatef = exec_glRotatef;
	glp->TexEnvi = exec_glTexEnvi;
	glp->TexParameteri = exec_glTexParameteri;
	glp->Translatef = exec_glTranslatef;
	glp->Vertex2f = exec_glVertex2f;
	glp->Vertex3f = exec_glVertex3f;
	glp->OSMesaCreateLDG = exec_OSMesaCreateLDG;
	glp->OSMesaDestroyLDG = exec_OSMesaDestroyLDG;
	glp->ArrayElement = exec_glArrayElement;
	glp->BindTexture = exec_glBindTexture;
	glp->CallList = exec_glCallList;
	glp->ClearDepthf = exec_glClearDepthf;
	glp->Color4f = exec_glColor4f;
	glp->Color3fv = exec_glColor3fv;
	glp->Color4fv = exec_glColor4fv;
	glp->ColorMaterial = exec_glColorMaterial;
	glp->ColorPointer = exec_glColorPointer;
	glp->CullFace = exec_glCullFace;
	glp->DeleteTextures = exec_glDeleteTextures;
	glp->DisableClientState = exec_glDisableClientState;
	glp->EnableClientState = exec_glEnableClientState;
	glp->EndList = exec_glEndList;
	glp->EdgeFlag = exec_glEdgeFlag;
	glp->Flush = exec_glFlush;
	glp->FrontFace = exec_glFrontFace;
	glp->Frustumf = exec_glFrustumf;
	glp->GenLists = exec_glGenLists;
	glp->GenTextures = exec_glGenTextures;
	glp->GetFloatv = exec_glGetFloatv;
	glp->GetIntegerv = exec_glGetIntegerv;
	glp->Hint = exec_glHint;
	glp->InitNames = exec_glInitNames;
	glp->IsList = exec_glIsList;
	glp->Lightf = exec_glLightf;
	glp->LightModeli = exec_glLightModeli;
	glp->LightModelfv = exec_glLightModelfv;
	glp->LoadMatrixf = exec_glLoadMatrixf;
	glp->LoadName = exec_glLoadName;
	glp->Materialf = exec_glMaterialf;
	glp->MultMatrixf = exec_glMultMatrixf;
	glp->NewList = exec_glNewList;
	glp->Normal3f = exec_glNormal3f;
	glp->Normal3fv = exec_glNormal3fv;
	glp->NormalPointer = exec_glNormalPointer;
	glp->PixelStorei = exec_glPixelStorei;
	glp->PolygonMode = exec_glPolygonMode;
	glp->PolygonOffset = exec_glPolygonOffset;
	glp->PopName = exec_glPopName;
	glp->PushName = exec_glPushName;
	glp->RenderMode = exec_glRenderMode;
	glp->SelectBuffer = exec_glSelectBuffer;
	glp->Scalef = exec_glScalef;
	glp->ShadeModel = exec_glShadeModel;
	glp->TexCoord2f = exec_glTexCoord2f;
	glp->TexCoord4f = exec_glTexCoord4f;
	glp->TexCoord2fv = exec_glTexCoord2fv;
	glp->TexCoordPointer = exec_glTexCoordPointer;
	glp->TexImage2D = exec_glTexImage2D;
	glp->Vertex4f = exec_glVertex4f;
	glp->Vertex3fv = exec_glVertex3fv;
	glp->VertexPointer = exec_glVertexPointer;
	glp->Viewport = exec_glViewport;
	glp->swapbuffer = exec_swapbuffer;
	glp->max_width = exec_max_width;
	glp->max_height = exec_max_height;
	glp->DeleteLists = exec_glDeleteLists;
	glp->gluLookAtf = exec_gluLookAtf;
	glp->exception_error = exec_exception_error;
}


struct gl_public *slb_load_tiny_gl(const char *path)
{
	long ret;
	size_t len;
	struct gl_public *pub = NULL;
	
	/*
	 * Slbopen() checks the name of the file with the
	 * compiled-in library name, so there is no way
	 * to pass an alternative filename here
	 */
	ret = Slbopen("tiny_gl.slb", path, 4 /* ARANFOSMESA_NFAPI_VERSION */, &gl_slb, &gl_exec);
	if (ret >= 0)
	{
		long  __CDECL (*exec)(SLB_HANDLE, long, long, void *) = (long  __CDECL (*)(SLB_HANDLE, long, long, void *))gl_exec;
		
		len = (*exec)(gl_slb, 0, SLB_NARGS(1), NULL);
		pub = gl_pub = (struct gl_public *)calloc(1, len);
		if (pub)
		{
			pub->m_alloc = malloc;
			pub->m_free = free;
			pub->libhandle = gl_slb;
			pub->libexec = gl_exec;
			(*exec)(gl_slb, 0, SLB_NARGS(1), pub);
			slb_init_tiny_gl();
		}
	}
	return pub;
}


void slb_unload_tiny_gl(struct gl_public *pub)
{
	if (pub != NULL)
	{
		if (pub->libhandle != NULL)
		{
			Slbclose(pub->libhandle);
			gl_slb = 0;
		}
		pub->m_free(pub);
	}
}
