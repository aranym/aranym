#ifndef WITH_PROTOTYPE_STRINGS
# define WITH_PROTOTYPE_STRINGS 1
#endif
#if WITH_PROTOTYPE_STRINGS
#define GL_PROC(name, f, desc) { name, desc, f },
#else
#define GL_PROC(name, f, desc) { name, 0, f },
#endif

GL_PROC("OSMesaCreateLDG", OSMesaCreateLDG, "void *OSMesaCreateLDG(GLenum format, GLenum type, GLint width, GLint height)")
GL_PROC("OSMesaDestroyLDG", OSMesaDestroyLDG, "void OSMesaDestroyLDG(void)")
GL_PROC("max_width", max_width, "GLsizei max_width(void)")
GL_PROC("max_height", max_height, "GLsizei max_height(void)")
GL_PROC("glOrtho", glOrthof, "void glOrthof(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat near_val, GLfloat far_val)")

GL_PROC("swapbuffer", tinyglswapbuffer, "void swapbuffer(void *buf)")
GL_PROC("exception_error", tinyglexception_error, "void exception_error(void CALLBACK (*exception)(GLenum param))")
GL_PROC("information", tinyglinformation, "void information(void)")
GL_PROC("glFrustum", glFrustumf, "void glFrustumf(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat near_val, GLfloat far_val)")
GL_PROC("glClearDepth", glClearDepthf, "void glClearDepthf(GLfloat depth)")
GL_PROC("gluLookAt", gluLookAtf, "void gluLookAtf(GLfloat eyeX, GLfloat eyeY, GLfloat eyeZ, GLfloat centerX, GLfloat centerY, GLfloat centerZ, GLfloat upX, GLfloat upY, GLfloat upZ)")

#undef GL_PROC
