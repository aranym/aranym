/* Bindings of tiny_gl.ldg
 * Compile this module and link it with the application client
 */

#include <gem.h>
#include <ldg.h>
#define NFOSMESA_NO_MANGLE
#include <ldg/tiny_gl.h>

#ifndef TRUE
# define TRUE 1
# define FALSE 0
#endif

struct _gl_tiny gl;

#define GL_CHECK(x) if (x == 0) result = FALSE

int ldg_init_tiny_gl(LDG *lib)
{
	int result = TRUE;
	
#undef glFrustum
#undef glOrtho
#undef gluLookAt
#undef glClearDepth
	gl.information = (void APIENTRY (*)(void)) ldg_find("information", lib);
	GL_CHECK(gl.information);
	gl.Begin = (void APIENTRY (*)(GLenum mode)) ldg_find("glBegin", lib);
	GL_CHECK(gl.Begin);
	gl.Clear = (void APIENTRY (*)(GLbitfield mask)) ldg_find("glClear", lib);
	GL_CHECK(gl.Clear);
	gl.ClearColor = (void APIENTRY (*)(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)) ldg_find("glClearColor", lib);
	GL_CHECK(gl.ClearColor);
	gl.Color3f = (void APIENTRY (*)(GLfloat red, GLfloat green, GLfloat blue)) ldg_find("glColor3f", lib);
	GL_CHECK(gl.Color3f);
	gl.Disable = (void APIENTRY (*)(GLenum cap)) ldg_find("glDisable", lib);
	GL_CHECK(gl.Disable);
	gl.Enable = (void APIENTRY (*)(GLenum cap)) ldg_find("glEnable", lib);
	GL_CHECK(gl.Enable);
	gl.End = (void APIENTRY (*)(void)) ldg_find("glEnd", lib);
	GL_CHECK(gl.End);
	gl.Lightfv = (void APIENTRY (*)(GLenum light, GLenum pname, const GLfloat *params)) ldg_find("glLightfv", lib);
	GL_CHECK(gl.Lightfv);
	gl.LoadIdentity = (void APIENTRY (*)(void)) ldg_find("glLoadIdentity", lib);
	GL_CHECK(gl.LoadIdentity);
	gl.Materialfv = (void APIENTRY (*)(GLenum face, GLenum pname, const GLfloat *params)) ldg_find("glMaterialfv", lib);
	GL_CHECK(gl.Materialfv);
	gl.MatrixMode = (void APIENTRY (*)(GLenum mode)) ldg_find("glMatrixMode", lib);
	GL_CHECK(gl.MatrixMode);
	gl.Orthof = (void APIENTRY (*)(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat near_val, GLfloat far_val)) ldg_find("glOrtho", lib);
	GL_CHECK(gl.Orthof);
	gl.PopMatrix = (void APIENTRY (*)(void)) ldg_find("glPopMatrix", lib);
	GL_CHECK(gl.PopMatrix);
	gl.PushMatrix = (void APIENTRY (*)(void)) ldg_find("glPushMatrix", lib);
	GL_CHECK(gl.PushMatrix);
	gl.Rotatef = (void APIENTRY (*)(GLfloat angle, GLfloat x, GLfloat y, GLfloat z)) ldg_find("glRotatef", lib);
	GL_CHECK(gl.Rotatef);
	gl.TexEnvi = (void APIENTRY (*)(GLenum target, GLenum pname, GLint param)) ldg_find("glTexEnvi", lib);
	GL_CHECK(gl.TexEnvi);
	gl.TexParameteri = (void APIENTRY (*)(GLenum target, GLenum pname, GLint param)) ldg_find("glTexParameteri", lib);
	GL_CHECK(gl.TexParameteri);
	gl.Translatef = (void APIENTRY (*)(GLfloat x, GLfloat y, GLfloat z)) ldg_find("glTranslatef", lib);
	GL_CHECK(gl.Translatef);
	gl.Vertex2f = (void APIENTRY (*)(GLfloat x, GLfloat y)) ldg_find("glVertex2f", lib);
	GL_CHECK(gl.Vertex2f);
	gl.Vertex3f = (void APIENTRY (*)(GLfloat x, GLfloat y, GLfloat z)) ldg_find("glVertex3f", lib);
	GL_CHECK(gl.Vertex3f);
	gl.OSMesaCreateLDG = (void * APIENTRY (*)(GLenum format, GLenum type, GLint width, GLint height)) ldg_find("OSMesaCreateLDG", lib);
	GL_CHECK(gl.OSMesaCreateLDG);
	gl.OSMesaDestroyLDG = (void APIENTRY (*)(void)) ldg_find("OSMesaDestroyLDG", lib);
	GL_CHECK(gl.OSMesaDestroyLDG);
	gl.ArrayElement = (void APIENTRY (*)(GLint i)) ldg_find("glArrayElement", lib);
	GL_CHECK(gl.ArrayElement);
	gl.BindTexture = (void APIENTRY (*)(GLenum target, GLuint texture)) ldg_find("glBindTexture", lib);
	GL_CHECK(gl.BindTexture);
	gl.CallList = (void APIENTRY (*)(GLuint list)) ldg_find("glCallList", lib);
	GL_CHECK(gl.CallList);
	gl.ClearDepthf = (void APIENTRY (*)(GLfloat d)) ldg_find("glClearDepth", lib);
	GL_CHECK(gl.ClearDepthf);
	gl.Color4f = (void APIENTRY (*)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)) ldg_find("glColor4f", lib);
	GL_CHECK(gl.Color4f);
	gl.Color3fv = (void APIENTRY (*)(const GLfloat *v)) ldg_find("glColor3fv", lib);
	GL_CHECK(gl.Color3fv);
	gl.Color4fv = (void APIENTRY (*)(const GLfloat *v)) ldg_find("glColor4fv", lib);
	GL_CHECK(gl.Color4fv);
	gl.ColorMaterial = (void APIENTRY (*)(GLenum face, GLenum mode)) ldg_find("glColorMaterial", lib);
	GL_CHECK(gl.ColorMaterial);
	gl.ColorPointer = (void APIENTRY (*)(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)) ldg_find("glColorPointer", lib);
	GL_CHECK(gl.ColorPointer);
	gl.CullFace = (void APIENTRY (*)(GLenum mode)) ldg_find("glCullFace", lib);
	GL_CHECK(gl.CullFace);
	gl.DeleteTextures = (void APIENTRY (*)(GLsizei n, const GLuint *textures)) ldg_find("glDeleteTextures", lib);
	GL_CHECK(gl.DeleteTextures);
	gl.DisableClientState = (void APIENTRY (*)(GLenum array)) ldg_find("glDisableClientState", lib);
	GL_CHECK(gl.DisableClientState);
	gl.EnableClientState = (void APIENTRY (*)(GLenum array)) ldg_find("glEnableClientState", lib);
	GL_CHECK(gl.EnableClientState);
	gl.EndList = (void APIENTRY (*)(void)) ldg_find("glEndList", lib);
	GL_CHECK(gl.EndList);
	gl.EdgeFlag = (void APIENTRY (*)(GLboolean32 flag)) ldg_find("glEdgeFlag", lib);
	GL_CHECK(gl.EdgeFlag);
	gl.Flush = (void APIENTRY (*)(void)) ldg_find("glFlush", lib);
	GL_CHECK(gl.Flush);
	gl.FrontFace = (void APIENTRY (*)(GLenum mode)) ldg_find("glFrontFace", lib);
	GL_CHECK(gl.FrontFace);
	gl.Frustumf = (void APIENTRY (*)(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat near_val, GLfloat far_val)) ldg_find("glFrustum", lib);
	GL_CHECK(gl.Frustumf);
	gl.GenLists = (GLuint APIENTRY (*)(GLsizei range)) ldg_find("glGenLists", lib);
	GL_CHECK(gl.GenLists);
	gl.GenTextures = (void APIENTRY (*)(GLsizei n, GLuint *textures)) ldg_find("glGenTextures", lib);
	GL_CHECK(gl.GenTextures);
	gl.GetFloatv = (void APIENTRY (*)(GLenum pname, GLfloat *params)) ldg_find("glGetFloatv", lib);
	GL_CHECK(gl.GetFloatv);
	gl.GetIntegerv = (void APIENTRY (*)(GLenum pname, GLint *params)) ldg_find("glGetIntegerv", lib);
	GL_CHECK(gl.GetIntegerv);
	gl.Hint = (void APIENTRY (*)(GLenum target, GLenum mode)) ldg_find("glHint", lib);
	GL_CHECK(gl.Hint);
	gl.InitNames = (void APIENTRY (*)(void)) ldg_find("glInitNames", lib);
	GL_CHECK(gl.InitNames);
	gl.IsList = (GLboolean APIENTRY (*)(GLuint list)) ldg_find("glIsList", lib);
	GL_CHECK(gl.IsList);
	gl.Lightf = (void APIENTRY (*)(GLenum light, GLenum pname, GLfloat param)) ldg_find("glLightf", lib);
	GL_CHECK(gl.Lightf);
	gl.LightModeli = (void APIENTRY (*)(GLenum pname, GLint param)) ldg_find("glLightModeli", lib);
	GL_CHECK(gl.LightModeli);
	gl.LightModelfv = (void APIENTRY (*)(GLenum pname, const GLfloat *params)) ldg_find("glLightModelfv", lib);
	GL_CHECK(gl.LightModelfv);
	gl.LoadMatrixf = (void APIENTRY (*)(const GLfloat *m)) ldg_find("glLoadMatrixf", lib);
	GL_CHECK(gl.LoadMatrixf);
	gl.LoadName = (void APIENTRY (*)(GLuint name)) ldg_find("glLoadName", lib);
	GL_CHECK(gl.LoadName);
	gl.Materialf = (void APIENTRY (*)(GLenum face, GLenum pname, GLfloat param)) ldg_find("glMaterialf", lib);
	GL_CHECK(gl.Materialf);
	gl.MultMatrixf = (void APIENTRY (*)(const GLfloat *m)) ldg_find("glMultMatrixf", lib);
	GL_CHECK(gl.MultMatrixf);
	gl.NewList = (void APIENTRY (*)(GLuint list, GLenum mode)) ldg_find("glNewList", lib);
	GL_CHECK(gl.NewList);
	gl.Normal3f = (void APIENTRY (*)(GLfloat nx, GLfloat ny, GLfloat nz)) ldg_find("glNormal3f", lib);
	GL_CHECK(gl.Normal3f);
	gl.Normal3fv = (void APIENTRY (*)(const GLfloat *v)) ldg_find("glNormal3fv", lib);
	GL_CHECK(gl.Normal3fv);
	gl.NormalPointer = (void APIENTRY (*)(GLenum type, GLsizei stride, const GLvoid *pointer)) ldg_find("glNormalPointer", lib);
	GL_CHECK(gl.NormalPointer);
	gl.PixelStorei = (void APIENTRY (*)(GLenum pname, GLint param)) ldg_find("glPixelStorei", lib);
	GL_CHECK(gl.PixelStorei);
	gl.PolygonMode = (void APIENTRY (*)(GLenum face, GLenum mode)) ldg_find("glPolygonMode", lib);
	GL_CHECK(gl.PolygonMode);
	gl.PolygonOffset = (void APIENTRY (*)(GLfloat factor, GLfloat units)) ldg_find("glPolygonOffset", lib);
	GL_CHECK(gl.PolygonOffset);
	gl.PopName = (void APIENTRY (*)(void)) ldg_find("glPopName", lib);
	GL_CHECK(gl.PopName);
	gl.PushName = (void APIENTRY (*)(GLuint name)) ldg_find("glPushName", lib);
	GL_CHECK(gl.PushName);
	gl.RenderMode = (GLint APIENTRY (*)(GLenum mode)) ldg_find("glRenderMode", lib);
	GL_CHECK(gl.RenderMode);
	gl.SelectBuffer = (void APIENTRY (*)(GLsizei size, GLuint *buffer)) ldg_find("glSelectBuffer", lib);
	GL_CHECK(gl.SelectBuffer);
	gl.Scalef = (void APIENTRY (*)(GLfloat x, GLfloat y, GLfloat z)) ldg_find("glScalef", lib);
	GL_CHECK(gl.Scalef);
	gl.ShadeModel = (void APIENTRY (*)(GLenum mode)) ldg_find("glShadeModel", lib);
	GL_CHECK(gl.ShadeModel);
	gl.TexCoord2f = (void APIENTRY (*)(GLfloat s, GLfloat t)) ldg_find("glTexCoord2f", lib);
	GL_CHECK(gl.TexCoord2f);
	gl.TexCoord4f = (void APIENTRY (*)(GLfloat s, GLfloat t, GLfloat r, GLfloat q)) ldg_find("glTexCoord4f", lib);
	GL_CHECK(gl.TexCoord4f);
	gl.TexCoord2fv = (void APIENTRY (*)(const GLfloat *v)) ldg_find("glTexCoord2fv", lib);
	GL_CHECK(gl.TexCoord2fv);
	gl.TexCoordPointer = (void APIENTRY (*)(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)) ldg_find("glTexCoordPointer", lib);
	GL_CHECK(gl.TexCoordPointer);
	gl.TexImage2D = (void APIENTRY (*)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels)) ldg_find("glTexImage2D", lib);
	GL_CHECK(gl.TexImage2D);
	gl.Vertex4f = (void APIENTRY (*)(GLfloat x, GLfloat y, GLfloat z, GLfloat w)) ldg_find("glVertex4f", lib);
	GL_CHECK(gl.Vertex4f);
	gl.Vertex3fv = (void APIENTRY (*)(const GLfloat *v)) ldg_find("glVertex3fv", lib);
	GL_CHECK(gl.Vertex3fv);
	gl.VertexPointer = (void APIENTRY (*)(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)) ldg_find("glVertexPointer", lib);
	GL_CHECK(gl.VertexPointer);
	gl.Viewport = (void APIENTRY (*)(GLint x, GLint y, GLsizei width, GLsizei height)) ldg_find("glViewport", lib);
	GL_CHECK(gl.Viewport);
	gl.swapbuffer = (void APIENTRY (*)(void *buffer)) ldg_find("swapbuffer", lib);
	GL_CHECK(gl.swapbuffer);
	gl.max_width = (GLsizei APIENTRY (*)(void)) ldg_find("max_width", lib);
	GL_CHECK(gl.max_width);
	gl.max_height = (GLsizei APIENTRY (*)(void)) ldg_find("max_height", lib);
	GL_CHECK(gl.max_height);
	gl.DeleteLists = (void APIENTRY (*)(GLuint list, GLsizei range)) ldg_find("glDeleteLists", lib);
	GL_CHECK(gl.DeleteLists);
	gl.gluLookAtf = (void APIENTRY (*)(GLfloat eyeX, GLfloat eyeY, GLfloat eyeZ, GLfloat centerX, GLfloat centerY, GLfloat centerZ, GLfloat upX, GLfloat upY, GLfloat upZ)) ldg_find("gluLookAt", lib);
	GL_CHECK(gl.gluLookAtf);
	gl.exception_error = (void APIENTRY (*)(void (CALLBACK *exception)(GLenum param) )) ldg_find("exception_error", lib);
	GL_CHECK(gl.exception_error);
	return result;
}
#undef GL_CHECK


LDG *ldg_load_tiny_gl(const char *libname, _WORD *gl)
{
	LDG *lib;
	
	if (libname == NULL)
		libname = "tiny_gl.ldg";
	if (gl == NULL)
		gl = ldg_global;
	lib = ldg_open(libname, gl);
	if (lib != NULL)
		ldg_init_tiny_gl(lib);
	return lib;
}


void ldg_unload_tiny_gl(LDG *lib, _WORD *gl)
{
	if (lib != NULL)
	{
		if (gl == NULL)
			gl = ldg_global;
		ldg_close(lib, gl);
	}
}


#ifdef TEST

#include <mint/arch/nf_ops.h>

int main(void)
{
	LDG *lib;
	
	lib = ldg_load_tiny_gl(0, 0);
	if (lib == NULL)
		lib = ldg_load_tiny_gl("c:/gemsys/ldg/tin_gl.ldg", 0);
	if (lib == NULL)
	{
		nf_debugprintf("tiny_gl.ldg not found\n");
		return 1;
	}
	nf_debugprintf("%s: %lx\n", "glBegin", gl.Begin);
	nf_debugprintf("%s: %lx\n", "glOrthof", gl.Orthof);
	ldg_unload_tiny_gl(lib, NULL);
	return 0;
}
#endif
