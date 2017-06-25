/* Bindings of tiny_gl.ldg
 * Compile this module and link it with the application client
 */

#include <gem.h>
#include <stdlib.h>
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
	struct _gl_tiny *glp = &gl;
	
#undef glClearDepth
#undef glFrustum
#undef glOrtho
#undef gluLookAt
	glp->information = (void APIENTRY (*)(void)) ldg_find("information", lib);
	GL_CHECK(glp->information);
	glp->Begin = (void APIENTRY (*)(GLenum mode)) ldg_find("glBegin", lib);
	GL_CHECK(glp->Begin);
	glp->Clear = (void APIENTRY (*)(GLbitfield mask)) ldg_find("glClear", lib);
	GL_CHECK(glp->Clear);
	glp->ClearColor = (void APIENTRY (*)(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)) ldg_find("glClearColor", lib);
	GL_CHECK(glp->ClearColor);
	glp->Color3f = (void APIENTRY (*)(GLfloat red, GLfloat green, GLfloat blue)) ldg_find("glColor3f", lib);
	GL_CHECK(glp->Color3f);
	glp->Disable = (void APIENTRY (*)(GLenum cap)) ldg_find("glDisable", lib);
	GL_CHECK(glp->Disable);
	glp->Enable = (void APIENTRY (*)(GLenum cap)) ldg_find("glEnable", lib);
	GL_CHECK(glp->Enable);
	glp->End = (void APIENTRY (*)(void)) ldg_find("glEnd", lib);
	GL_CHECK(glp->End);
	glp->Lightfv = (void APIENTRY (*)(GLenum light, GLenum pname, const GLfloat *params)) ldg_find("glLightfv", lib);
	GL_CHECK(glp->Lightfv);
	glp->LoadIdentity = (void APIENTRY (*)(void)) ldg_find("glLoadIdentity", lib);
	GL_CHECK(glp->LoadIdentity);
	glp->Materialfv = (void APIENTRY (*)(GLenum face, GLenum pname, const GLfloat *params)) ldg_find("glMaterialfv", lib);
	GL_CHECK(glp->Materialfv);
	glp->MatrixMode = (void APIENTRY (*)(GLenum mode)) ldg_find("glMatrixMode", lib);
	GL_CHECK(glp->MatrixMode);
	glp->Orthof = (void APIENTRY (*)(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat near_val, GLfloat far_val)) ldg_find("glOrtho", lib);
	GL_CHECK(glp->Orthof);
	glp->PopMatrix = (void APIENTRY (*)(void)) ldg_find("glPopMatrix", lib);
	GL_CHECK(glp->PopMatrix);
	glp->PushMatrix = (void APIENTRY (*)(void)) ldg_find("glPushMatrix", lib);
	GL_CHECK(glp->PushMatrix);
	glp->Rotatef = (void APIENTRY (*)(GLfloat angle, GLfloat x, GLfloat y, GLfloat z)) ldg_find("glRotatef", lib);
	GL_CHECK(glp->Rotatef);
	glp->TexEnvi = (void APIENTRY (*)(GLenum target, GLenum pname, GLint param)) ldg_find("glTexEnvi", lib);
	GL_CHECK(glp->TexEnvi);
	glp->TexParameteri = (void APIENTRY (*)(GLenum target, GLenum pname, GLint param)) ldg_find("glTexParameteri", lib);
	GL_CHECK(glp->TexParameteri);
	glp->Translatef = (void APIENTRY (*)(GLfloat x, GLfloat y, GLfloat z)) ldg_find("glTranslatef", lib);
	GL_CHECK(glp->Translatef);
	glp->Vertex2f = (void APIENTRY (*)(GLfloat x, GLfloat y)) ldg_find("glVertex2f", lib);
	GL_CHECK(glp->Vertex2f);
	glp->Vertex3f = (void APIENTRY (*)(GLfloat x, GLfloat y, GLfloat z)) ldg_find("glVertex3f", lib);
	GL_CHECK(glp->Vertex3f);
	glp->OSMesaCreateLDG = (void * APIENTRY (*)(GLenum format, GLenum type, GLint width, GLint height)) ldg_find("OSMesaCreateLDG", lib);
	GL_CHECK(glp->OSMesaCreateLDG);
	glp->OSMesaDestroyLDG = (void APIENTRY (*)(void)) ldg_find("OSMesaDestroyLDG", lib);
	GL_CHECK(glp->OSMesaDestroyLDG);
	glp->ArrayElement = (void APIENTRY (*)(GLint i)) ldg_find("glArrayElement", lib);
	GL_CHECK(glp->ArrayElement);
	glp->BindTexture = (void APIENTRY (*)(GLenum target, GLuint texture)) ldg_find("glBindTexture", lib);
	GL_CHECK(glp->BindTexture);
	glp->CallList = (void APIENTRY (*)(GLuint list)) ldg_find("glCallList", lib);
	GL_CHECK(glp->CallList);
	glp->ClearDepthf = (void APIENTRY (*)(GLfloat d)) ldg_find("glClearDepth", lib);
	GL_CHECK(glp->ClearDepthf);
	glp->Color4f = (void APIENTRY (*)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)) ldg_find("glColor4f", lib);
	GL_CHECK(glp->Color4f);
	glp->Color3fv = (void APIENTRY (*)(const GLfloat *v)) ldg_find("glColor3fv", lib);
	GL_CHECK(glp->Color3fv);
	glp->Color4fv = (void APIENTRY (*)(const GLfloat *v)) ldg_find("glColor4fv", lib);
	GL_CHECK(glp->Color4fv);
	glp->ColorMaterial = (void APIENTRY (*)(GLenum face, GLenum mode)) ldg_find("glColorMaterial", lib);
	GL_CHECK(glp->ColorMaterial);
	glp->ColorPointer = (void APIENTRY (*)(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)) ldg_find("glColorPointer", lib);
	GL_CHECK(glp->ColorPointer);
	glp->CullFace = (void APIENTRY (*)(GLenum mode)) ldg_find("glCullFace", lib);
	GL_CHECK(glp->CullFace);
	glp->DeleteTextures = (void APIENTRY (*)(GLsizei n, const GLuint *textures)) ldg_find("glDeleteTextures", lib);
	GL_CHECK(glp->DeleteTextures);
	glp->DisableClientState = (void APIENTRY (*)(GLenum array)) ldg_find("glDisableClientState", lib);
	GL_CHECK(glp->DisableClientState);
	glp->EnableClientState = (void APIENTRY (*)(GLenum array)) ldg_find("glEnableClientState", lib);
	GL_CHECK(glp->EnableClientState);
	glp->EndList = (void APIENTRY (*)(void)) ldg_find("glEndList", lib);
	GL_CHECK(glp->EndList);
	glp->EdgeFlag = (void APIENTRY (*)(GLboolean32 flag)) ldg_find("glEdgeFlag", lib);
	GL_CHECK(glp->EdgeFlag);
	glp->Flush = (void APIENTRY (*)(void)) ldg_find("glFlush", lib);
	GL_CHECK(glp->Flush);
	glp->FrontFace = (void APIENTRY (*)(GLenum mode)) ldg_find("glFrontFace", lib);
	GL_CHECK(glp->FrontFace);
	glp->Frustumf = (void APIENTRY (*)(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat near_val, GLfloat far_val)) ldg_find("glFrustum", lib);
	GL_CHECK(glp->Frustumf);
	glp->GenLists = (GLuint APIENTRY (*)(GLsizei range)) ldg_find("glGenLists", lib);
	GL_CHECK(glp->GenLists);
	glp->GenTextures = (void APIENTRY (*)(GLsizei n, GLuint *textures)) ldg_find("glGenTextures", lib);
	GL_CHECK(glp->GenTextures);
	glp->GetFloatv = (void APIENTRY (*)(GLenum pname, GLfloat *params)) ldg_find("glGetFloatv", lib);
	GL_CHECK(glp->GetFloatv);
	glp->GetIntegerv = (void APIENTRY (*)(GLenum pname, GLint *params)) ldg_find("glGetIntegerv", lib);
	GL_CHECK(glp->GetIntegerv);
	glp->Hint = (void APIENTRY (*)(GLenum target, GLenum mode)) ldg_find("glHint", lib);
	GL_CHECK(glp->Hint);
	glp->InitNames = (void APIENTRY (*)(void)) ldg_find("glInitNames", lib);
	GL_CHECK(glp->InitNames);
	glp->IsList = (GLboolean APIENTRY (*)(GLuint list)) ldg_find("glIsList", lib);
	GL_CHECK(glp->IsList);
	glp->Lightf = (void APIENTRY (*)(GLenum light, GLenum pname, GLfloat param)) ldg_find("glLightf", lib);
	GL_CHECK(glp->Lightf);
	glp->LightModeli = (void APIENTRY (*)(GLenum pname, GLint param)) ldg_find("glLightModeli", lib);
	GL_CHECK(glp->LightModeli);
	glp->LightModelfv = (void APIENTRY (*)(GLenum pname, const GLfloat *params)) ldg_find("glLightModelfv", lib);
	GL_CHECK(glp->LightModelfv);
	glp->LoadMatrixf = (void APIENTRY (*)(const GLfloat *m)) ldg_find("glLoadMatrixf", lib);
	GL_CHECK(glp->LoadMatrixf);
	glp->LoadName = (void APIENTRY (*)(GLuint name)) ldg_find("glLoadName", lib);
	GL_CHECK(glp->LoadName);
	glp->Materialf = (void APIENTRY (*)(GLenum face, GLenum pname, GLfloat param)) ldg_find("glMaterialf", lib);
	GL_CHECK(glp->Materialf);
	glp->MultMatrixf = (void APIENTRY (*)(const GLfloat *m)) ldg_find("glMultMatrixf", lib);
	GL_CHECK(glp->MultMatrixf);
	glp->NewList = (void APIENTRY (*)(GLuint list, GLenum mode)) ldg_find("glNewList", lib);
	GL_CHECK(glp->NewList);
	glp->Normal3f = (void APIENTRY (*)(GLfloat nx, GLfloat ny, GLfloat nz)) ldg_find("glNormal3f", lib);
	GL_CHECK(glp->Normal3f);
	glp->Normal3fv = (void APIENTRY (*)(const GLfloat *v)) ldg_find("glNormal3fv", lib);
	GL_CHECK(glp->Normal3fv);
	glp->NormalPointer = (void APIENTRY (*)(GLenum type, GLsizei stride, const GLvoid *pointer)) ldg_find("glNormalPointer", lib);
	GL_CHECK(glp->NormalPointer);
	glp->PixelStorei = (void APIENTRY (*)(GLenum pname, GLint param)) ldg_find("glPixelStorei", lib);
	GL_CHECK(glp->PixelStorei);
	glp->PolygonMode = (void APIENTRY (*)(GLenum face, GLenum mode)) ldg_find("glPolygonMode", lib);
	GL_CHECK(glp->PolygonMode);
	glp->PolygonOffset = (void APIENTRY (*)(GLfloat factor, GLfloat units)) ldg_find("glPolygonOffset", lib);
	GL_CHECK(glp->PolygonOffset);
	glp->PopName = (void APIENTRY (*)(void)) ldg_find("glPopName", lib);
	GL_CHECK(glp->PopName);
	glp->PushName = (void APIENTRY (*)(GLuint name)) ldg_find("glPushName", lib);
	GL_CHECK(glp->PushName);
	glp->RenderMode = (GLint APIENTRY (*)(GLenum mode)) ldg_find("glRenderMode", lib);
	GL_CHECK(glp->RenderMode);
	glp->SelectBuffer = (void APIENTRY (*)(GLsizei size, GLuint *buffer)) ldg_find("glSelectBuffer", lib);
	GL_CHECK(glp->SelectBuffer);
	glp->Scalef = (void APIENTRY (*)(GLfloat x, GLfloat y, GLfloat z)) ldg_find("glScalef", lib);
	GL_CHECK(glp->Scalef);
	glp->ShadeModel = (void APIENTRY (*)(GLenum mode)) ldg_find("glShadeModel", lib);
	GL_CHECK(glp->ShadeModel);
	glp->TexCoord2f = (void APIENTRY (*)(GLfloat s, GLfloat t)) ldg_find("glTexCoord2f", lib);
	GL_CHECK(glp->TexCoord2f);
	glp->TexCoord4f = (void APIENTRY (*)(GLfloat s, GLfloat t, GLfloat r, GLfloat q)) ldg_find("glTexCoord4f", lib);
	GL_CHECK(glp->TexCoord4f);
	glp->TexCoord2fv = (void APIENTRY (*)(const GLfloat *v)) ldg_find("glTexCoord2fv", lib);
	GL_CHECK(glp->TexCoord2fv);
	glp->TexCoordPointer = (void APIENTRY (*)(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)) ldg_find("glTexCoordPointer", lib);
	GL_CHECK(glp->TexCoordPointer);
	glp->TexImage2D = (void APIENTRY (*)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels)) ldg_find("glTexImage2D", lib);
	GL_CHECK(glp->TexImage2D);
	glp->Vertex4f = (void APIENTRY (*)(GLfloat x, GLfloat y, GLfloat z, GLfloat w)) ldg_find("glVertex4f", lib);
	GL_CHECK(glp->Vertex4f);
	glp->Vertex3fv = (void APIENTRY (*)(const GLfloat *v)) ldg_find("glVertex3fv", lib);
	GL_CHECK(glp->Vertex3fv);
	glp->VertexPointer = (void APIENTRY (*)(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)) ldg_find("glVertexPointer", lib);
	GL_CHECK(glp->VertexPointer);
	glp->Viewport = (void APIENTRY (*)(GLint x, GLint y, GLsizei width, GLsizei height)) ldg_find("glViewport", lib);
	GL_CHECK(glp->Viewport);
	glp->swapbuffer = (void APIENTRY (*)(void *buffer)) ldg_find("swapbuffer", lib);
	GL_CHECK(glp->swapbuffer);
	glp->max_width = (GLsizei APIENTRY (*)(void)) ldg_find("max_width", lib);
	GL_CHECK(glp->max_width);
	glp->max_height = (GLsizei APIENTRY (*)(void)) ldg_find("max_height", lib);
	GL_CHECK(glp->max_height);
	glp->DeleteLists = (void APIENTRY (*)(GLuint list, GLsizei range)) ldg_find("glDeleteLists", lib);
	GL_CHECK(glp->DeleteLists);
	glp->gluLookAtf = (void APIENTRY (*)(GLfloat eyeX, GLfloat eyeY, GLfloat eyeZ, GLfloat centerX, GLfloat centerY, GLfloat centerZ, GLfloat upX, GLfloat upY, GLfloat upZ)) ldg_find("gluLookAt", lib);
	GL_CHECK(glp->gluLookAtf);
	glp->exception_error = (void APIENTRY (*)(void (CALLBACK *exception)(GLenum param) )) ldg_find("exception_error", lib);
	GL_CHECK(glp->exception_error);
	return result;
}
#undef GL_CHECK


struct gl_public *ldg_load_tiny_gl(const char *libname, _WORD *gl)
{
	LDG *lib;
	struct gl_public *pub = NULL;
	size_t len;
	
	if (libname == NULL)
		libname = "tiny_gl.ldg";
	if (gl == NULL)
		gl = ldg_global;
	lib = ldg_open(libname, gl);
	if (lib != NULL)
	{
		long APIENTRY (*glInit)(void *) = (long APIENTRY (*)(void *))ldg_find("glInit", lib);
		if (glInit)
		{
			len = (*glInit)(NULL);
		} else
		{
			len = sizeof(*pub);
		}
		pub = (struct gl_public *)calloc(1, len);
		if (pub)
		{
			pub->m_alloc = malloc;
			pub->m_free = free;
			if (glInit)
				(*glInit)(pub);
			pub->libhandle = lib;
			ldg_init_tiny_gl(lib);
		} else
		{
			ldg_close(lib, gl);
		}
	}
	return pub;
}


void ldg_unload_tiny_gl(struct gl_public *pub, _WORD *gl)
{
	if (pub != NULL)
	{
		if (pub->libhandle != NULL)
		{
			if (gl == NULL)
				gl = ldg_global;
			ldg_close((LDG *)pub->libhandle, gl);
		}
		pub->m_free(pub);
	}
}


#ifdef TEST

#include <stdio.h>

int main(void)
{
	struct gl_public *pub;
	
	pub = ldg_load_tiny_gl(0, 0);
	if (pub == NULL)
		pub = ldg_load_tiny_gl("c:/gemsys/ldg/tin_gl.ldg", 0);
	if (pub == NULL)
	{
		fprintf(stderr, "tiny_gl.ldg not found\n");
		return 1;
	}
	printf("%s: %lx\n", "glBegin", gl.Begin);
	printf("%s: %lx\n", "glOrthof", gl.Orthof);
	ldg_unload_tiny_gl(pub, NULL);
	return 0;
}
#endif
