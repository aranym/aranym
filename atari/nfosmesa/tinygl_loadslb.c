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

#undef glFrustum
#undef glOrtho
#undef gluLookAt
#undef glClearDepth

static void APIENTRY exec_information(void)
{
	struct information_args {
		SLB_HANDLE slb;
		long fn;
		short nwords;
	} args;
	void __CDECL (*exec)(struct information_args) = (void __CDECL (*)(struct information_args))gl_exec;
	args.slb = gl_slb;
	args.fn = 0;
	args.nwords = 0;
	(*exec)(args);
}

static void APIENTRY exec_glBegin(GLenum mode)
{
	struct Begin_args {
		SLB_HANDLE slb;
		long fn;
		short nwords;
		GLenum mode;
	} args;
	void __CDECL (*exec)(struct Begin_args) = (void __CDECL (*)(struct Begin_args))gl_exec;
	args.slb = gl_slb;
	args.fn = 1;
	args.nwords = 2;
	args.mode = mode;
	(*exec)(args);
}

static void APIENTRY exec_glClear(GLbitfield mask)
{
	struct Clear_args {
		SLB_HANDLE slb;
		long fn;
		short nwords;
		GLbitfield mask;
	} args;
	void __CDECL (*exec)(struct Clear_args) = (void __CDECL (*)(struct Clear_args))gl_exec;
	args.slb = gl_slb;
	args.fn = 2;
	args.nwords = 2;
	args.mask = mask;
	(*exec)(args);
}

static void APIENTRY exec_glClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
	struct ClearColor_args {
		SLB_HANDLE slb;
		long fn;
		short nwords;
		GLclampf red;
		GLclampf green;
		GLclampf blue;
		GLclampf alpha;
	} args;
	void __CDECL (*exec)(struct ClearColor_args) = (void __CDECL (*)(struct ClearColor_args))gl_exec;
	args.slb = gl_slb;
	args.fn = 3;
	args.nwords = 8;
	args.red = red;
	args.green = green;
	args.blue = blue;
	args.alpha = alpha;
	(*exec)(args);
}

static void APIENTRY exec_glColor3f(GLfloat red, GLfloat green, GLfloat blue)
{
	struct Color3f_args {
		SLB_HANDLE slb;
		long fn;
		short nwords;
		GLfloat red;
		GLfloat green;
		GLfloat blue;
	} args;
	void __CDECL (*exec)(struct Color3f_args) = (void __CDECL (*)(struct Color3f_args))gl_exec;
	args.slb = gl_slb;
	args.fn = 4;
	args.nwords = 6;
	args.red = red;
	args.green = green;
	args.blue = blue;
	(*exec)(args);
}

static void APIENTRY exec_glDisable(GLenum cap)
{
	struct Disable_args {
		SLB_HANDLE slb;
		long fn;
		short nwords;
		GLenum cap;
	} args;
	void __CDECL (*exec)(struct Disable_args) = (void __CDECL (*)(struct Disable_args))gl_exec;
	args.slb = gl_slb;
	args.fn = 5;
	args.nwords = 2;
	args.cap = cap;
	(*exec)(args);
}

static void APIENTRY exec_glEnable(GLenum cap)
{
	struct Enable_args {
		SLB_HANDLE slb;
		long fn;
		short nwords;
		GLenum cap;
	} args;
	void __CDECL (*exec)(struct Enable_args) = (void __CDECL (*)(struct Enable_args))gl_exec;
	args.slb = gl_slb;
	args.fn = 6;
	args.nwords = 2;
	args.cap = cap;
	(*exec)(args);
}

static void APIENTRY exec_glEnd(void)
{
	struct End_args {
		SLB_HANDLE slb;
		long fn;
		short nwords;
	} args;
	void __CDECL (*exec)(struct End_args) = (void __CDECL (*)(struct End_args))gl_exec;
	args.slb = gl_slb;
	args.fn = 7;
	args.nwords = 0;
	(*exec)(args);
}

static void APIENTRY exec_glLightfv(GLenum light, GLenum pname, const GLfloat *params)
{
	struct Lightfv_args {
		SLB_HANDLE slb;
		long fn;
		short nwords;
		GLenum light;
		GLenum pname;
		const GLfloat *params;
	} args;
	void __CDECL (*exec)(struct Lightfv_args) = (void __CDECL (*)(struct Lightfv_args))gl_exec;
	args.slb = gl_slb;
	args.fn = 8;
	args.nwords = 6;
	args.light = light;
	args.pname = pname;
	args.params = params;
	(*exec)(args);
}

static void APIENTRY exec_glLoadIdentity(void)
{
	struct LoadIdentity_args {
		SLB_HANDLE slb;
		long fn;
		short nwords;
	} args;
	void __CDECL (*exec)(struct LoadIdentity_args) = (void __CDECL (*)(struct LoadIdentity_args))gl_exec;
	args.slb = gl_slb;
	args.fn = 9;
	args.nwords = 0;
	(*exec)(args);
}

static void APIENTRY exec_glMaterialfv(GLenum face, GLenum pname, const GLfloat *params)
{
	struct Materialfv_args {
		SLB_HANDLE slb;
		long fn;
		short nwords;
		GLenum face;
		GLenum pname;
		const GLfloat *params;
	} args;
	void __CDECL (*exec)(struct Materialfv_args) = (void __CDECL (*)(struct Materialfv_args))gl_exec;
	args.slb = gl_slb;
	args.fn = 10;
	args.nwords = 6;
	args.face = face;
	args.pname = pname;
	args.params = params;
	(*exec)(args);
}

static void APIENTRY exec_glMatrixMode(GLenum mode)
{
	struct MatrixMode_args {
		SLB_HANDLE slb;
		long fn;
		short nwords;
		GLenum mode;
	} args;
	void __CDECL (*exec)(struct MatrixMode_args) = (void __CDECL (*)(struct MatrixMode_args))gl_exec;
	args.slb = gl_slb;
	args.fn = 11;
	args.nwords = 2;
	args.mode = mode;
	(*exec)(args);
}

static void APIENTRY exec_glOrthof(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat near_val, GLfloat far_val)
{
	struct Orthof_args {
		SLB_HANDLE slb;
		long fn;
		short nwords;
		GLfloat left;
		GLfloat right;
		GLfloat bottom;
		GLfloat top;
		GLfloat near_val;
		GLfloat far_val;
	} args;
	void __CDECL (*exec)(struct Orthof_args) = (void __CDECL (*)(struct Orthof_args))gl_exec;
	args.slb = gl_slb;
	args.fn = 12;
	args.nwords = 12;
	args.left = left;
	args.right = right;
	args.bottom = bottom;
	args.top = top;
	args.near_val = near_val;
	args.far_val = far_val;
	(*exec)(args);
}

static void APIENTRY exec_glPopMatrix(void)
{
	struct PopMatrix_args {
		SLB_HANDLE slb;
		long fn;
		short nwords;
	} args;
	void __CDECL (*exec)(struct PopMatrix_args) = (void __CDECL (*)(struct PopMatrix_args))gl_exec;
	args.slb = gl_slb;
	args.fn = 13;
	args.nwords = 0;
	(*exec)(args);
}

static void APIENTRY exec_glPushMatrix(void)
{
	struct PushMatrix_args {
		SLB_HANDLE slb;
		long fn;
		short nwords;
	} args;
	void __CDECL (*exec)(struct PushMatrix_args) = (void __CDECL (*)(struct PushMatrix_args))gl_exec;
	args.slb = gl_slb;
	args.fn = 14;
	args.nwords = 0;
	(*exec)(args);
}

static void APIENTRY exec_glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
{
	struct Rotatef_args {
		SLB_HANDLE slb;
		long fn;
		short nwords;
		GLfloat angle;
		GLfloat x;
		GLfloat y;
		GLfloat z;
	} args;
	void __CDECL (*exec)(struct Rotatef_args) = (void __CDECL (*)(struct Rotatef_args))gl_exec;
	args.slb = gl_slb;
	args.fn = 15;
	args.nwords = 8;
	args.angle = angle;
	args.x = x;
	args.y = y;
	args.z = z;
	(*exec)(args);
}

static void APIENTRY exec_glTexEnvi(GLenum target, GLenum pname, GLint param)
{
	struct TexEnvi_args {
		SLB_HANDLE slb;
		long fn;
		short nwords;
		GLenum target;
		GLenum pname;
		GLint param;
	} args;
	void __CDECL (*exec)(struct TexEnvi_args) = (void __CDECL (*)(struct TexEnvi_args))gl_exec;
	args.slb = gl_slb;
	args.fn = 16;
	args.nwords = 6;
	args.target = target;
	args.pname = pname;
	args.param = param;
	(*exec)(args);
}

static void APIENTRY exec_glTexParameteri(GLenum target, GLenum pname, GLint param)
{
	struct TexParameteri_args {
		SLB_HANDLE slb;
		long fn;
		short nwords;
		GLenum target;
		GLenum pname;
		GLint param;
	} args;
	void __CDECL (*exec)(struct TexParameteri_args) = (void __CDECL (*)(struct TexParameteri_args))gl_exec;
	args.slb = gl_slb;
	args.fn = 17;
	args.nwords = 6;
	args.target = target;
	args.pname = pname;
	args.param = param;
	(*exec)(args);
}

static void APIENTRY exec_glTranslatef(GLfloat x, GLfloat y, GLfloat z)
{
	struct Translatef_args {
		SLB_HANDLE slb;
		long fn;
		short nwords;
		GLfloat x;
		GLfloat y;
		GLfloat z;
	} args;
	void __CDECL (*exec)(struct Translatef_args) = (void __CDECL (*)(struct Translatef_args))gl_exec;
	args.slb = gl_slb;
	args.fn = 18;
	args.nwords = 6;
	args.x = x;
	args.y = y;
	args.z = z;
	(*exec)(args);
}

static void APIENTRY exec_glVertex2f(GLfloat x, GLfloat y)
{
	struct Vertex2f_args {
		SLB_HANDLE slb;
		long fn;
		short nwords;
		GLfloat x;
		GLfloat y;
	} args;
	void __CDECL (*exec)(struct Vertex2f_args) = (void __CDECL (*)(struct Vertex2f_args))gl_exec;
	args.slb = gl_slb;
	args.fn = 19;
	args.nwords = 4;
	args.x = x;
	args.y = y;
	(*exec)(args);
}

static void APIENTRY exec_glVertex3f(GLfloat x, GLfloat y, GLfloat z)
{
	struct Vertex3f_args {
		SLB_HANDLE slb;
		long fn;
		short nwords;
		GLfloat x;
		GLfloat y;
		GLfloat z;
	} args;
	void __CDECL (*exec)(struct Vertex3f_args) = (void __CDECL (*)(struct Vertex3f_args))gl_exec;
	args.slb = gl_slb;
	args.fn = 20;
	args.nwords = 6;
	args.x = x;
	args.y = y;
	args.z = z;
	(*exec)(args);
}

static void * APIENTRY exec_OSMesaCreateLDG(GLenum format, GLenum type, GLint width, GLint height)
{
	struct OSMesaCreateLDG_args {
		SLB_HANDLE slb;
		long fn;
		short nwords;
		GLenum format;
		GLenum type;
		GLint width;
		GLint height;
	} args;
	void * __CDECL (*exec)(struct OSMesaCreateLDG_args) = (void * __CDECL (*)(struct OSMesaCreateLDG_args))gl_exec;
	args.slb = gl_slb;
	args.fn = 21;
	args.nwords = 8;
	args.format = format;
	args.type = type;
	args.width = width;
	args.height = height;
	return (*exec)(args);
}

static void APIENTRY exec_OSMesaDestroyLDG(void)
{
	struct OSMesaDestroyLDG_args {
		SLB_HANDLE slb;
		long fn;
		short nwords;
	} args;
	void __CDECL (*exec)(struct OSMesaDestroyLDG_args) = (void __CDECL (*)(struct OSMesaDestroyLDG_args))gl_exec;
	args.slb = gl_slb;
	args.fn = 22;
	args.nwords = 0;
	(*exec)(args);
}

static void APIENTRY exec_glArrayElement(GLint i)
{
	struct ArrayElement_args {
		SLB_HANDLE slb;
		long fn;
		short nwords;
		GLint i;
	} args;
	void __CDECL (*exec)(struct ArrayElement_args) = (void __CDECL (*)(struct ArrayElement_args))gl_exec;
	args.slb = gl_slb;
	args.fn = 23;
	args.nwords = 2;
	args.i = i;
	(*exec)(args);
}

static void APIENTRY exec_glBindTexture(GLenum target, GLuint texture)
{
	struct BindTexture_args {
		SLB_HANDLE slb;
		long fn;
		short nwords;
		GLenum target;
		GLuint texture;
	} args;
	void __CDECL (*exec)(struct BindTexture_args) = (void __CDECL (*)(struct BindTexture_args))gl_exec;
	args.slb = gl_slb;
	args.fn = 24;
	args.nwords = 4;
	args.target = target;
	args.texture = texture;
	(*exec)(args);
}

static void APIENTRY exec_glCallList(GLuint list)
{
	struct CallList_args {
		SLB_HANDLE slb;
		long fn;
		short nwords;
		GLuint list;
	} args;
	void __CDECL (*exec)(struct CallList_args) = (void __CDECL (*)(struct CallList_args))gl_exec;
	args.slb = gl_slb;
	args.fn = 25;
	args.nwords = 2;
	args.list = list;
	(*exec)(args);
}

static void APIENTRY exec_glClearDepthf(GLfloat d)
{
	struct ClearDepthf_args {
		SLB_HANDLE slb;
		long fn;
		short nwords;
		GLfloat d;
	} args;
	void __CDECL (*exec)(struct ClearDepthf_args) = (void __CDECL (*)(struct ClearDepthf_args))gl_exec;
	args.slb = gl_slb;
	args.fn = 26;
	args.nwords = 2;
	args.d = d;
	(*exec)(args);
}

static void APIENTRY exec_glColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
	struct Color4f_args {
		SLB_HANDLE slb;
		long fn;
		short nwords;
		GLfloat red;
		GLfloat green;
		GLfloat blue;
		GLfloat alpha;
	} args;
	void __CDECL (*exec)(struct Color4f_args) = (void __CDECL (*)(struct Color4f_args))gl_exec;
	args.slb = gl_slb;
	args.fn = 27;
	args.nwords = 8;
	args.red = red;
	args.green = green;
	args.blue = blue;
	args.alpha = alpha;
	(*exec)(args);
}

static void APIENTRY exec_glColor3fv(const GLfloat *v)
{
	struct Color3fv_args {
		SLB_HANDLE slb;
		long fn;
		short nwords;
		const GLfloat *v;
	} args;
	void __CDECL (*exec)(struct Color3fv_args) = (void __CDECL (*)(struct Color3fv_args))gl_exec;
	args.slb = gl_slb;
	args.fn = 28;
	args.nwords = 2;
	args.v = v;
	(*exec)(args);
}

static void APIENTRY exec_glColor4fv(const GLfloat *v)
{
	struct Color4fv_args {
		SLB_HANDLE slb;
		long fn;
		short nwords;
		const GLfloat *v;
	} args;
	void __CDECL (*exec)(struct Color4fv_args) = (void __CDECL (*)(struct Color4fv_args))gl_exec;
	args.slb = gl_slb;
	args.fn = 29;
	args.nwords = 2;
	args.v = v;
	(*exec)(args);
}

static void APIENTRY exec_glColorMaterial(GLenum face, GLenum mode)
{
	struct ColorMaterial_args {
		SLB_HANDLE slb;
		long fn;
		short nwords;
		GLenum face;
		GLenum mode;
	} args;
	void __CDECL (*exec)(struct ColorMaterial_args) = (void __CDECL (*)(struct ColorMaterial_args))gl_exec;
	args.slb = gl_slb;
	args.fn = 30;
	args.nwords = 4;
	args.face = face;
	args.mode = mode;
	(*exec)(args);
}

static void APIENTRY exec_glColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
	struct ColorPointer_args {
		SLB_HANDLE slb;
		long fn;
		short nwords;
		GLint size;
		GLenum type;
		GLsizei stride;
		const GLvoid *pointer;
	} args;
	void __CDECL (*exec)(struct ColorPointer_args) = (void __CDECL (*)(struct ColorPointer_args))gl_exec;
	args.slb = gl_slb;
	args.fn = 31;
	args.nwords = 8;
	args.size = size;
	args.type = type;
	args.stride = stride;
	args.pointer = pointer;
	(*exec)(args);
}

static void APIENTRY exec_glCullFace(GLenum mode)
{
	struct CullFace_args {
		SLB_HANDLE slb;
		long fn;
		short nwords;
		GLenum mode;
	} args;
	void __CDECL (*exec)(struct CullFace_args) = (void __CDECL (*)(struct CullFace_args))gl_exec;
	args.slb = gl_slb;
	args.fn = 32;
	args.nwords = 2;
	args.mode = mode;
	(*exec)(args);
}

static void APIENTRY exec_glDeleteTextures(GLsizei n, const GLuint *textures)
{
	struct DeleteTextures_args {
		SLB_HANDLE slb;
		long fn;
		short nwords;
		GLsizei n;
		const GLuint *textures;
	} args;
	void __CDECL (*exec)(struct DeleteTextures_args) = (void __CDECL (*)(struct DeleteTextures_args))gl_exec;
	args.slb = gl_slb;
	args.fn = 33;
	args.nwords = 4;
	args.n = n;
	args.textures = textures;
	(*exec)(args);
}

static void APIENTRY exec_glDisableClientState(GLenum array)
{
	struct DisableClientState_args {
		SLB_HANDLE slb;
		long fn;
		short nwords;
		GLenum array;
	} args;
	void __CDECL (*exec)(struct DisableClientState_args) = (void __CDECL (*)(struct DisableClientState_args))gl_exec;
	args.slb = gl_slb;
	args.fn = 34;
	args.nwords = 2;
	args.array = array;
	(*exec)(args);
}

static void APIENTRY exec_glEnableClientState(GLenum array)
{
	struct EnableClientState_args {
		SLB_HANDLE slb;
		long fn;
		short nwords;
		GLenum array;
	} args;
	void __CDECL (*exec)(struct EnableClientState_args) = (void __CDECL (*)(struct EnableClientState_args))gl_exec;
	args.slb = gl_slb;
	args.fn = 35;
	args.nwords = 2;
	args.array = array;
	(*exec)(args);
}

static void APIENTRY exec_glEndList(void)
{
	struct EndList_args {
		SLB_HANDLE slb;
		long fn;
		short nwords;
	} args;
	void __CDECL (*exec)(struct EndList_args) = (void __CDECL (*)(struct EndList_args))gl_exec;
	args.slb = gl_slb;
	args.fn = 36;
	args.nwords = 0;
	(*exec)(args);
}

static void APIENTRY exec_glEdgeFlag(GLboolean32 flag)
{
	struct EdgeFlag_args {
		SLB_HANDLE slb;
		long fn;
		short nwords;
		GLboolean32 flag;
	} args;
	void __CDECL (*exec)(struct EdgeFlag_args) = (void __CDECL (*)(struct EdgeFlag_args))gl_exec;
	args.slb = gl_slb;
	args.fn = 37;
	args.nwords = 2;
	args.flag = flag;
	(*exec)(args);
}

static void APIENTRY exec_glFlush(void)
{
	struct Flush_args {
		SLB_HANDLE slb;
		long fn;
		short nwords;
	} args;
	void __CDECL (*exec)(struct Flush_args) = (void __CDECL (*)(struct Flush_args))gl_exec;
	args.slb = gl_slb;
	args.fn = 38;
	args.nwords = 0;
	(*exec)(args);
}

static void APIENTRY exec_glFrontFace(GLenum mode)
{
	struct FrontFace_args {
		SLB_HANDLE slb;
		long fn;
		short nwords;
		GLenum mode;
	} args;
	void __CDECL (*exec)(struct FrontFace_args) = (void __CDECL (*)(struct FrontFace_args))gl_exec;
	args.slb = gl_slb;
	args.fn = 39;
	args.nwords = 2;
	args.mode = mode;
	(*exec)(args);
}

static void APIENTRY exec_glFrustumf(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat near_val, GLfloat far_val)
{
	struct Frustumf_args {
		SLB_HANDLE slb;
		long fn;
		short nwords;
		GLfloat left;
		GLfloat right;
		GLfloat bottom;
		GLfloat top;
		GLfloat near_val;
		GLfloat far_val;
	} args;
	void __CDECL (*exec)(struct Frustumf_args) = (void __CDECL (*)(struct Frustumf_args))gl_exec;
	args.slb = gl_slb;
	args.fn = 40;
	args.nwords = 12;
	args.left = left;
	args.right = right;
	args.bottom = bottom;
	args.top = top;
	args.near_val = near_val;
	args.far_val = far_val;
	(*exec)(args);
}

static GLuint APIENTRY exec_glGenLists(GLsizei range)
{
	struct GenLists_args {
		SLB_HANDLE slb;
		long fn;
		short nwords;
		GLsizei range;
	} args;
	GLuint __CDECL (*exec)(struct GenLists_args) = (GLuint __CDECL (*)(struct GenLists_args))gl_exec;
	args.slb = gl_slb;
	args.fn = 41;
	args.nwords = 2;
	args.range = range;
	return (*exec)(args);
}

static void APIENTRY exec_glGenTextures(GLsizei n, GLuint *textures)
{
	struct GenTextures_args {
		SLB_HANDLE slb;
		long fn;
		short nwords;
		GLsizei n;
		GLuint *textures;
	} args;
	void __CDECL (*exec)(struct GenTextures_args) = (void __CDECL (*)(struct GenTextures_args))gl_exec;
	args.slb = gl_slb;
	args.fn = 42;
	args.nwords = 4;
	args.n = n;
	args.textures = textures;
	(*exec)(args);
}

static void APIENTRY exec_glGetFloatv(GLenum pname, GLfloat *params)
{
	struct GetFloatv_args {
		SLB_HANDLE slb;
		long fn;
		short nwords;
		GLenum pname;
		GLfloat *params;
	} args;
	void __CDECL (*exec)(struct GetFloatv_args) = (void __CDECL (*)(struct GetFloatv_args))gl_exec;
	args.slb = gl_slb;
	args.fn = 43;
	args.nwords = 4;
	args.pname = pname;
	args.params = params;
	(*exec)(args);
}

static void APIENTRY exec_glGetIntegerv(GLenum pname, GLint *params)
{
	struct GetIntegerv_args {
		SLB_HANDLE slb;
		long fn;
		short nwords;
		GLenum pname;
		GLint *params;
	} args;
	void __CDECL (*exec)(struct GetIntegerv_args) = (void __CDECL (*)(struct GetIntegerv_args))gl_exec;
	args.slb = gl_slb;
	args.fn = 44;
	args.nwords = 4;
	args.pname = pname;
	args.params = params;
	(*exec)(args);
}

static void APIENTRY exec_glHint(GLenum target, GLenum mode)
{
	struct Hint_args {
		SLB_HANDLE slb;
		long fn;
		short nwords;
		GLenum target;
		GLenum mode;
	} args;
	void __CDECL (*exec)(struct Hint_args) = (void __CDECL (*)(struct Hint_args))gl_exec;
	args.slb = gl_slb;
	args.fn = 45;
	args.nwords = 4;
	args.target = target;
	args.mode = mode;
	(*exec)(args);
}

static void APIENTRY exec_glInitNames(void)
{
	struct InitNames_args {
		SLB_HANDLE slb;
		long fn;
		short nwords;
	} args;
	void __CDECL (*exec)(struct InitNames_args) = (void __CDECL (*)(struct InitNames_args))gl_exec;
	args.slb = gl_slb;
	args.fn = 46;
	args.nwords = 0;
	(*exec)(args);
}

static GLboolean APIENTRY exec_glIsList(GLuint list)
{
	struct IsList_args {
		SLB_HANDLE slb;
		long fn;
		short nwords;
		GLuint list;
	} args;
	GLboolean __CDECL (*exec)(struct IsList_args) = (GLboolean __CDECL (*)(struct IsList_args))gl_exec;
	args.slb = gl_slb;
	args.fn = 47;
	args.nwords = 2;
	args.list = list;
	return (*exec)(args);
}

static void APIENTRY exec_glLightf(GLenum light, GLenum pname, GLfloat param)
{
	struct Lightf_args {
		SLB_HANDLE slb;
		long fn;
		short nwords;
		GLenum light;
		GLenum pname;
		GLfloat param;
	} args;
	void __CDECL (*exec)(struct Lightf_args) = (void __CDECL (*)(struct Lightf_args))gl_exec;
	args.slb = gl_slb;
	args.fn = 48;
	args.nwords = 6;
	args.light = light;
	args.pname = pname;
	args.param = param;
	(*exec)(args);
}

static void APIENTRY exec_glLightModeli(GLenum pname, GLint param)
{
	struct LightModeli_args {
		SLB_HANDLE slb;
		long fn;
		short nwords;
		GLenum pname;
		GLint param;
	} args;
	void __CDECL (*exec)(struct LightModeli_args) = (void __CDECL (*)(struct LightModeli_args))gl_exec;
	args.slb = gl_slb;
	args.fn = 49;
	args.nwords = 4;
	args.pname = pname;
	args.param = param;
	(*exec)(args);
}

static void APIENTRY exec_glLightModelfv(GLenum pname, const GLfloat *params)
{
	struct LightModelfv_args {
		SLB_HANDLE slb;
		long fn;
		short nwords;
		GLenum pname;
		const GLfloat *params;
	} args;
	void __CDECL (*exec)(struct LightModelfv_args) = (void __CDECL (*)(struct LightModelfv_args))gl_exec;
	args.slb = gl_slb;
	args.fn = 50;
	args.nwords = 4;
	args.pname = pname;
	args.params = params;
	(*exec)(args);
}

static void APIENTRY exec_glLoadMatrixf(const GLfloat *m)
{
	struct LoadMatrixf_args {
		SLB_HANDLE slb;
		long fn;
		short nwords;
		const GLfloat *m;
	} args;
	void __CDECL (*exec)(struct LoadMatrixf_args) = (void __CDECL (*)(struct LoadMatrixf_args))gl_exec;
	args.slb = gl_slb;
	args.fn = 51;
	args.nwords = 2;
	args.m = m;
	(*exec)(args);
}

static void APIENTRY exec_glLoadName(GLuint name)
{
	struct LoadName_args {
		SLB_HANDLE slb;
		long fn;
		short nwords;
		GLuint name;
	} args;
	void __CDECL (*exec)(struct LoadName_args) = (void __CDECL (*)(struct LoadName_args))gl_exec;
	args.slb = gl_slb;
	args.fn = 52;
	args.nwords = 2;
	args.name = name;
	(*exec)(args);
}

static void APIENTRY exec_glMaterialf(GLenum face, GLenum pname, GLfloat param)
{
	struct Materialf_args {
		SLB_HANDLE slb;
		long fn;
		short nwords;
		GLenum face;
		GLenum pname;
		GLfloat param;
	} args;
	void __CDECL (*exec)(struct Materialf_args) = (void __CDECL (*)(struct Materialf_args))gl_exec;
	args.slb = gl_slb;
	args.fn = 53;
	args.nwords = 6;
	args.face = face;
	args.pname = pname;
	args.param = param;
	(*exec)(args);
}

static void APIENTRY exec_glMultMatrixf(const GLfloat *m)
{
	struct MultMatrixf_args {
		SLB_HANDLE slb;
		long fn;
		short nwords;
		const GLfloat *m;
	} args;
	void __CDECL (*exec)(struct MultMatrixf_args) = (void __CDECL (*)(struct MultMatrixf_args))gl_exec;
	args.slb = gl_slb;
	args.fn = 54;
	args.nwords = 2;
	args.m = m;
	(*exec)(args);
}

static void APIENTRY exec_glNewList(GLuint list, GLenum mode)
{
	struct NewList_args {
		SLB_HANDLE slb;
		long fn;
		short nwords;
		GLuint list;
		GLenum mode;
	} args;
	void __CDECL (*exec)(struct NewList_args) = (void __CDECL (*)(struct NewList_args))gl_exec;
	args.slb = gl_slb;
	args.fn = 55;
	args.nwords = 4;
	args.list = list;
	args.mode = mode;
	(*exec)(args);
}

static void APIENTRY exec_glNormal3f(GLfloat nx, GLfloat ny, GLfloat nz)
{
	struct Normal3f_args {
		SLB_HANDLE slb;
		long fn;
		short nwords;
		GLfloat nx;
		GLfloat ny;
		GLfloat nz;
	} args;
	void __CDECL (*exec)(struct Normal3f_args) = (void __CDECL (*)(struct Normal3f_args))gl_exec;
	args.slb = gl_slb;
	args.fn = 56;
	args.nwords = 6;
	args.nx = nx;
	args.ny = ny;
	args.nz = nz;
	(*exec)(args);
}

static void APIENTRY exec_glNormal3fv(const GLfloat *v)
{
	struct Normal3fv_args {
		SLB_HANDLE slb;
		long fn;
		short nwords;
		const GLfloat *v;
	} args;
	void __CDECL (*exec)(struct Normal3fv_args) = (void __CDECL (*)(struct Normal3fv_args))gl_exec;
	args.slb = gl_slb;
	args.fn = 57;
	args.nwords = 2;
	args.v = v;
	(*exec)(args);
}

static void APIENTRY exec_glNormalPointer(GLenum type, GLsizei stride, const GLvoid *pointer)
{
	struct NormalPointer_args {
		SLB_HANDLE slb;
		long fn;
		short nwords;
		GLenum type;
		GLsizei stride;
		const GLvoid *pointer;
	} args;
	void __CDECL (*exec)(struct NormalPointer_args) = (void __CDECL (*)(struct NormalPointer_args))gl_exec;
	args.slb = gl_slb;
	args.fn = 58;
	args.nwords = 6;
	args.type = type;
	args.stride = stride;
	args.pointer = pointer;
	(*exec)(args);
}

static void APIENTRY exec_glPixelStorei(GLenum pname, GLint param)
{
	struct PixelStorei_args {
		SLB_HANDLE slb;
		long fn;
		short nwords;
		GLenum pname;
		GLint param;
	} args;
	void __CDECL (*exec)(struct PixelStorei_args) = (void __CDECL (*)(struct PixelStorei_args))gl_exec;
	args.slb = gl_slb;
	args.fn = 59;
	args.nwords = 4;
	args.pname = pname;
	args.param = param;
	(*exec)(args);
}

static void APIENTRY exec_glPolygonMode(GLenum face, GLenum mode)
{
	struct PolygonMode_args {
		SLB_HANDLE slb;
		long fn;
		short nwords;
		GLenum face;
		GLenum mode;
	} args;
	void __CDECL (*exec)(struct PolygonMode_args) = (void __CDECL (*)(struct PolygonMode_args))gl_exec;
	args.slb = gl_slb;
	args.fn = 60;
	args.nwords = 4;
	args.face = face;
	args.mode = mode;
	(*exec)(args);
}

static void APIENTRY exec_glPolygonOffset(GLfloat factor, GLfloat units)
{
	struct PolygonOffset_args {
		SLB_HANDLE slb;
		long fn;
		short nwords;
		GLfloat factor;
		GLfloat units;
	} args;
	void __CDECL (*exec)(struct PolygonOffset_args) = (void __CDECL (*)(struct PolygonOffset_args))gl_exec;
	args.slb = gl_slb;
	args.fn = 61;
	args.nwords = 4;
	args.factor = factor;
	args.units = units;
	(*exec)(args);
}

static void APIENTRY exec_glPopName(void)
{
	struct PopName_args {
		SLB_HANDLE slb;
		long fn;
		short nwords;
	} args;
	void __CDECL (*exec)(struct PopName_args) = (void __CDECL (*)(struct PopName_args))gl_exec;
	args.slb = gl_slb;
	args.fn = 62;
	args.nwords = 0;
	(*exec)(args);
}

static void APIENTRY exec_glPushName(GLuint name)
{
	struct PushName_args {
		SLB_HANDLE slb;
		long fn;
		short nwords;
		GLuint name;
	} args;
	void __CDECL (*exec)(struct PushName_args) = (void __CDECL (*)(struct PushName_args))gl_exec;
	args.slb = gl_slb;
	args.fn = 63;
	args.nwords = 2;
	args.name = name;
	(*exec)(args);
}

static GLint APIENTRY exec_glRenderMode(GLenum mode)
{
	struct RenderMode_args {
		SLB_HANDLE slb;
		long fn;
		short nwords;
		GLenum mode;
	} args;
	GLint __CDECL (*exec)(struct RenderMode_args) = (GLint __CDECL (*)(struct RenderMode_args))gl_exec;
	args.slb = gl_slb;
	args.fn = 64;
	args.nwords = 2;
	args.mode = mode;
	return (*exec)(args);
}

static void APIENTRY exec_glSelectBuffer(GLsizei size, GLuint *buffer)
{
	struct SelectBuffer_args {
		SLB_HANDLE slb;
		long fn;
		short nwords;
		GLsizei size;
		GLuint *buffer;
	} args;
	void __CDECL (*exec)(struct SelectBuffer_args) = (void __CDECL (*)(struct SelectBuffer_args))gl_exec;
	args.slb = gl_slb;
	args.fn = 65;
	args.nwords = 4;
	args.size = size;
	args.buffer = buffer;
	(*exec)(args);
}

static void APIENTRY exec_glScalef(GLfloat x, GLfloat y, GLfloat z)
{
	struct Scalef_args {
		SLB_HANDLE slb;
		long fn;
		short nwords;
		GLfloat x;
		GLfloat y;
		GLfloat z;
	} args;
	void __CDECL (*exec)(struct Scalef_args) = (void __CDECL (*)(struct Scalef_args))gl_exec;
	args.slb = gl_slb;
	args.fn = 66;
	args.nwords = 6;
	args.x = x;
	args.y = y;
	args.z = z;
	(*exec)(args);
}

static void APIENTRY exec_glShadeModel(GLenum mode)
{
	struct ShadeModel_args {
		SLB_HANDLE slb;
		long fn;
		short nwords;
		GLenum mode;
	} args;
	void __CDECL (*exec)(struct ShadeModel_args) = (void __CDECL (*)(struct ShadeModel_args))gl_exec;
	args.slb = gl_slb;
	args.fn = 67;
	args.nwords = 2;
	args.mode = mode;
	(*exec)(args);
}

static void APIENTRY exec_glTexCoord2f(GLfloat s, GLfloat t)
{
	struct TexCoord2f_args {
		SLB_HANDLE slb;
		long fn;
		short nwords;
		GLfloat s;
		GLfloat t;
	} args;
	void __CDECL (*exec)(struct TexCoord2f_args) = (void __CDECL (*)(struct TexCoord2f_args))gl_exec;
	args.slb = gl_slb;
	args.fn = 68;
	args.nwords = 4;
	args.s = s;
	args.t = t;
	(*exec)(args);
}

static void APIENTRY exec_glTexCoord4f(GLfloat s, GLfloat t, GLfloat r, GLfloat q)
{
	struct TexCoord4f_args {
		SLB_HANDLE slb;
		long fn;
		short nwords;
		GLfloat s;
		GLfloat t;
		GLfloat r;
		GLfloat q;
	} args;
	void __CDECL (*exec)(struct TexCoord4f_args) = (void __CDECL (*)(struct TexCoord4f_args))gl_exec;
	args.slb = gl_slb;
	args.fn = 69;
	args.nwords = 8;
	args.s = s;
	args.t = t;
	args.r = r;
	args.q = q;
	(*exec)(args);
}

static void APIENTRY exec_glTexCoord2fv(const GLfloat *v)
{
	struct TexCoord2fv_args {
		SLB_HANDLE slb;
		long fn;
		short nwords;
		const GLfloat *v;
	} args;
	void __CDECL (*exec)(struct TexCoord2fv_args) = (void __CDECL (*)(struct TexCoord2fv_args))gl_exec;
	args.slb = gl_slb;
	args.fn = 70;
	args.nwords = 2;
	args.v = v;
	(*exec)(args);
}

static void APIENTRY exec_glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
	struct TexCoordPointer_args {
		SLB_HANDLE slb;
		long fn;
		short nwords;
		GLint size;
		GLenum type;
		GLsizei stride;
		const GLvoid *pointer;
	} args;
	void __CDECL (*exec)(struct TexCoordPointer_args) = (void __CDECL (*)(struct TexCoordPointer_args))gl_exec;
	args.slb = gl_slb;
	args.fn = 71;
	args.nwords = 8;
	args.size = size;
	args.type = type;
	args.stride = stride;
	args.pointer = pointer;
	(*exec)(args);
}

static void APIENTRY exec_glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels)
{
	struct TexImage2D_args {
		SLB_HANDLE slb;
		long fn;
		short nwords;
		GLenum target;
		GLint level;
		GLint internalformat;
		GLsizei width;
		GLsizei height;
		GLint border;
		GLenum format;
		GLenum type;
		const GLvoid *pixels;
	} args;
	void __CDECL (*exec)(struct TexImage2D_args) = (void __CDECL (*)(struct TexImage2D_args))gl_exec;
	args.slb = gl_slb;
	args.fn = 72;
	args.nwords = 18;
	args.target = target;
	args.level = level;
	args.internalformat = internalformat;
	args.width = width;
	args.height = height;
	args.border = border;
	args.format = format;
	args.type = type;
	args.pixels = pixels;
	(*exec)(args);
}

static void APIENTRY exec_glVertex4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
	struct Vertex4f_args {
		SLB_HANDLE slb;
		long fn;
		short nwords;
		GLfloat x;
		GLfloat y;
		GLfloat z;
		GLfloat w;
	} args;
	void __CDECL (*exec)(struct Vertex4f_args) = (void __CDECL (*)(struct Vertex4f_args))gl_exec;
	args.slb = gl_slb;
	args.fn = 73;
	args.nwords = 8;
	args.x = x;
	args.y = y;
	args.z = z;
	args.w = w;
	(*exec)(args);
}

static void APIENTRY exec_glVertex3fv(const GLfloat *v)
{
	struct Vertex3fv_args {
		SLB_HANDLE slb;
		long fn;
		short nwords;
		const GLfloat *v;
	} args;
	void __CDECL (*exec)(struct Vertex3fv_args) = (void __CDECL (*)(struct Vertex3fv_args))gl_exec;
	args.slb = gl_slb;
	args.fn = 74;
	args.nwords = 2;
	args.v = v;
	(*exec)(args);
}

static void APIENTRY exec_glVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
	struct VertexPointer_args {
		SLB_HANDLE slb;
		long fn;
		short nwords;
		GLint size;
		GLenum type;
		GLsizei stride;
		const GLvoid *pointer;
	} args;
	void __CDECL (*exec)(struct VertexPointer_args) = (void __CDECL (*)(struct VertexPointer_args))gl_exec;
	args.slb = gl_slb;
	args.fn = 75;
	args.nwords = 8;
	args.size = size;
	args.type = type;
	args.stride = stride;
	args.pointer = pointer;
	(*exec)(args);
}

static void APIENTRY exec_glViewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
	struct Viewport_args {
		SLB_HANDLE slb;
		long fn;
		short nwords;
		GLint x;
		GLint y;
		GLsizei width;
		GLsizei height;
	} args;
	void __CDECL (*exec)(struct Viewport_args) = (void __CDECL (*)(struct Viewport_args))gl_exec;
	args.slb = gl_slb;
	args.fn = 76;
	args.nwords = 8;
	args.x = x;
	args.y = y;
	args.width = width;
	args.height = height;
	(*exec)(args);
}

static void APIENTRY exec_swapbuffer(void *buffer)
{
	struct swapbuffer_args {
		SLB_HANDLE slb;
		long fn;
		short nwords;
		void *buffer;
	} args;
	void __CDECL (*exec)(struct swapbuffer_args) = (void __CDECL (*)(struct swapbuffer_args))gl_exec;
	args.slb = gl_slb;
	args.fn = 77;
	args.nwords = 2;
	args.buffer = buffer;
	(*exec)(args);
}

static GLsizei APIENTRY exec_max_width(void)
{
	struct max_width_args {
		SLB_HANDLE slb;
		long fn;
		short nwords;
	} args;
	GLsizei __CDECL (*exec)(struct max_width_args) = (GLsizei __CDECL (*)(struct max_width_args))gl_exec;
	args.slb = gl_slb;
	args.fn = 78;
	args.nwords = 0;
	return (*exec)(args);
}

static GLsizei APIENTRY exec_max_height(void)
{
	struct max_height_args {
		SLB_HANDLE slb;
		long fn;
		short nwords;
	} args;
	GLsizei __CDECL (*exec)(struct max_height_args) = (GLsizei __CDECL (*)(struct max_height_args))gl_exec;
	args.slb = gl_slb;
	args.fn = 79;
	args.nwords = 0;
	return (*exec)(args);
}

static void APIENTRY exec_glDeleteLists(GLuint list, GLsizei range)
{
	struct DeleteLists_args {
		SLB_HANDLE slb;
		long fn;
		short nwords;
		GLuint list;
		GLsizei range;
	} args;
	void __CDECL (*exec)(struct DeleteLists_args) = (void __CDECL (*)(struct DeleteLists_args))gl_exec;
	args.slb = gl_slb;
	args.fn = 80;
	args.nwords = 4;
	args.list = list;
	args.range = range;
	(*exec)(args);
}

static void APIENTRY exec_gluLookAtf(GLfloat eyeX, GLfloat eyeY, GLfloat eyeZ, GLfloat centerX, GLfloat centerY, GLfloat centerZ, GLfloat upX, GLfloat upY, GLfloat upZ)
{
	struct LookAtf_args {
		SLB_HANDLE slb;
		long fn;
		short nwords;
		GLfloat eyeX;
		GLfloat eyeY;
		GLfloat eyeZ;
		GLfloat centerX;
		GLfloat centerY;
		GLfloat centerZ;
		GLfloat upX;
		GLfloat upY;
		GLfloat upZ;
	} args;
	void __CDECL (*exec)(struct LookAtf_args) = (void __CDECL (*)(struct LookAtf_args))gl_exec;
	args.slb = gl_slb;
	args.fn = 81;
	args.nwords = 18;
	args.eyeX = eyeX;
	args.eyeY = eyeY;
	args.eyeZ = eyeZ;
	args.centerX = centerX;
	args.centerY = centerY;
	args.centerZ = centerZ;
	args.upX = upX;
	args.upY = upY;
	args.upZ = upZ;
	(*exec)(args);
}

static void APIENTRY exec_exception_error(void (CALLBACK *exception)(GLenum param) )
{
	struct exception_error_args {
		SLB_HANDLE slb;
		long fn;
		short nwords;
		void (CALLBACK *exception)(GLenum param) ;
	} args;
	void __CDECL (*exec)(struct exception_error_args) = (void __CDECL (*)(struct exception_error_args))gl_exec;
	args.slb = gl_slb;
	args.fn = 82;
	args.nwords = 2;
	args.exception = exception;
	(*exec)(args);
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


long slb_load_tiny_gl(const char *libname, const char *path, long min_version)
{
	long ret;
	
	if (libname == NULL)
		libname = "tiny_gl.slb";
	ret = Slbopen(libname, path, min_version, &gl_slb, &gl_exec);
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
