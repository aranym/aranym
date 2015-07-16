#ifndef __GNUC__
#error "this file is for GNU-C only"
#endif
#include <osbind.h>
#include "nfosmesa_nfapi.h"
#include <mint/basepage.h>
#include "lib-osmesa.h"
#include "lib-oldmesa.h"
#include "lib-misc.h"


#define __CDECL

typedef long __CDECL (*SLB_LFUNC)(BASEPAGE *pd, long fn, long nargs, ...);

#ifndef __STRINGIFY
#define __STRINGIFY(x) __STRINGIFY1(x)
#define __STRINGIFY1(x) #x
#endif

/* generate the prototypes */
#define varargs(proto...) proto
#undef NOTHING
#undef AND
#define NOTHING
#define AND ,
#define NO_PROC
#define GL_PROC(type, gl, name, export, upper, proto, args, first, ret) static type __CDECL slb_ ## gl ## name(BASEPAGE *__basepage, long __fn, long __nwords, gl_private *private, void *first_param);
#define GL_PROC64(type, gl, name, export, upper, proto, args, first, ret) static void __CDECL slb_ ## gl ## name(BASEPAGE *__basepage, long __fn, long __nwords, gl_private *private, void *first_param, GLuint64 *retval);
#define LENGL_PROC(type, glx, name, export, upper, proto, args, first, ret) static const GLubyte *__CDECL slb_ ## gl ## name(BASEPAGE *__basepage, long __fn, long __nwords, gl_private *private, void *first_param);
#define PUTGL_PROC(type, glx, name, export, upper, proto, args, first, ret) NO_PROC
#define OSMESA_PROC(type, gl, name, export, upper, proto, args, first, ret) static type __CDECL slb_ ## OSMesa ## name(BASEPAGE *__basepage, long __fn, long __nwords, gl_private *private, void *first_param);
#define TINYGL_PROC(type, gl, name, export, upper, proto, args, first, ret) static type __CDECL slb_ ## name(BASEPAGE *__basepage, long __fn, long __nwords, gl_private *private, void *first_param);
#include "glfuncs-bynum.h"

static long __CDECL slb_libinit(BASEPAGE *base, long fn, long nwords, gl_private *private);

/* The file header of a shared library */
struct slb_head
{
	long		slh_magic;								/* Magic value (0x70004afc) */
	const char	*slh_name;								/* Name of the library */
	long		slh_version;							/* Version number */
	long		slh_flags;								/* Flags, currently 0L and unused */
	long		__CDECL (*slh_slb_init)(void);			/* Pointer to init()-function */
	void		__CDECL (*slh_slb_exit)(void);			/* Pointer to exit()-function */
	long		__CDECL (*slh_slb_open)(BASEPAGE *b);	/* Pointer to open()-function */
	long		__CDECL (*slh_slb_close)(BASEPAGE *b);	/* Pointer to close()-function */
	const char	*const *slh_names;						/* Pointer to functions names, or 0L */
	void        *sl_next;                               /* used by MetaDOS loader */
	long		slh_reserved[7];						/* Currently 0L and unused */
	long		slh_no_funcs;							/* Number of functions */
	SLB_LFUNC	slh_functions[NFOSMESA_LAST];			/* The function pointers */
};


static char const slh_name[];
static char const *const slh_names[];

static long __CDECL slb_init(void);
static void __CDECL slb_exit(void);
static long __CDECL slb_open(BASEPAGE *bp);
static long __CDECL slb_close(BASEPAGE *bp);

static void __CDECL slb_nop(void);

/*
 * The file header of a shared library
 *
 * This replaces the startup code, and must be the first thing in this file,
 * and also in the resulting executable.
 * Make sure your binutils put this into the text section.
 */
struct slb_head const _start = {
	0x70004afc,
	slh_name,
	ARANFOSMESA_NFAPI_VERSION,
	0,
	slb_init,
	slb_exit,
	slb_open,
	slb_close,
	slh_names,
	0,
	{ 0, 0, 0, 0, 0, 0, 0 },
	NFOSMESA_LAST,
	{
		(SLB_LFUNC)slb_libinit,
/* generate the function table */
#define GL_PROC(type, gl, name, export, upper, proto, args, first, ret) (SLB_LFUNC)slb_ ## gl ## name,
#define LENGL_PROC(type, glx, name, export, upper, proto, args, first, ret) (SLB_LFUNC)slb_ ## gl ## name,
#define PUTGL_PROC(type, glx, name, export, upper, proto, args, first, ret) NO_PROC
#define OSMESA_PROC(type, gl, name, export, upper, proto, args, first, ret) (SLB_LFUNC)slb_ ## OSMesa ## name,
#define TINYGL_PROC(type, gl, name, export, upper, proto, args, first, ret) (SLB_LFUNC)slb_ ## name,
#define NO_PROC (SLB_LFUNC)slb_nop,
#include "glfuncs-bynum.h"
	}
};

static char const slh_name[] = "osmesa.slb";

#define unused __attribute__((__unused__))

static long __CDECL slb_libinit(BASEPAGE *__bp unused, long __fn unused, long __nwords unused, gl_private *private)
{
	internal_glInit(private);
	return sizeof(*private);
}


/* generate the function names */
static char const *const slh_names[] = {
	"glInit", /* slb_libinit */
#define GL_PROC(type, gl, name, export, upper, proto, args, first, ret) #gl #name,
#define LENGL_PROC(type, glx, name, export, upper, proto, args, first, ret) "gl" #name,
#define PUTGL_PROC(type, glx, name, export, upper, proto, args, first, ret) NO_PROC
#define OSMESA_PROC(type, gl, name, export, upper, proto, args, first, ret) #gl #name,
#define TINYGL_PROC(type, gl, name, export, upper, proto, args, first, ret) #name,
#define NO_PROC 0,
#include "glfuncs-bynum.h"
	0
};



/* generate the wrapper functions */
#define voidf /**/
#define GL_PROC(type, gl, name, export, upper, proto, args, first, ret) \
static type __CDECL slb_ ## gl ## name(BASEPAGE *__bp unused, long __fn unused, long __nwords unused, gl_private *private, void *first_param) \
{ \
	ret (*HostCall_p)(NFOSMESA_GL ## upper, private->cur_context, first_param); \
}
#define GL_PROC64(type, gl, name, export, upper, proto, args, first, ret) \
static void __CDECL slb_ ## gl ## name(BASEPAGE *__bp unused, long __fn unused, long __nwords unused, gl_private *private, void *first_param, GLuint64 *retval) \
{ \
	(*HostCall64_p)(NFOSMESA_GL ## upper, private->cur_context, first_param, retval); \
}
#define GLU_PROC(type, gl, name, export, upper, proto, args, first, ret) \
static type __CDECL slb_ ## gl ## name(BASEPAGE *__bp unused, long __fn unused, long __nwords unused, gl_private *private, void *first_param) \
{ \
	ret (*HostCall_p)(NFOSMESA_GLU ## upper, private->cur_context, first_param); \
}
#define OSMESA_PROC(type, gl, name, export, upper, proto, args, first, ret) \
static type __CDECL slb_ ## gl ## name(BASEPAGE *__bp unused, long __fn unused, long __nwords unused, gl_private *private, void *first_param) \
{ \
	ret (*HostCall_p)(NFOSMESA_OSMESA ## upper, private->cur_context, first_param); \
}
#define NO_OSMESAGETCURRENTCONTEXT
#define NO_OSMESADESTROYCONTEXT
#define NO_OSMESAMAKECURRENT
#define NO_OSMESAGETPROCADDRESS
#define NO_GLGETSTRING
#define NO_GLGETSTRINGI
#include "glfuncs.h"
	

static void __CDECL slb_nop(void)
{
}


/* some functions do need special work */

static const GLubyte *__CDECL slb_glGetString(BASEPAGE *__bp unused, long __fn unused, long __nwords unused, gl_private *private, void *first_param)
{
	GLenum *args = (GLenum *)first_param;
	return internal_glGetString(private, args[0]);
}


static const GLubyte *__CDECL slb_glGetStringi(BASEPAGE *__bp unused, long __fn unused, long __nwords unused, gl_private *private, void *first_param)
{
	GLenum *args = (GLenum *)first_param;
	return internal_glGetStringi(private, args[0], args[1]);
}


static OSMesaContext __CDECL slb_OSMesaGetCurrentContext(BASEPAGE *__bp unused, long __fn unused, long __nwords unused, gl_private *private, void *first_param unused)
{
	return internal_OSMesaGetCurrentContext(private);
}

static void __CDECL slb_OSMesaDestroyContext(BASEPAGE *__bp unused, long __fn unused, long __nwords unused, gl_private *private, void *first_param)
{
	OSMesaContext ctx = *((OSMesaContext *)first_param);
	return internal_OSMesaDestroyContext(private, ctx);
}


static GLboolean __CDECL slb_OSMesaMakeCurrent(BASEPAGE *__bp unused, long __fn unused, long __nwords unused, gl_private *private, void *first_param)
{
	GLboolean ret = (GLboolean)(*HostCall_p)(NFOSMESA_OSMESAMAKECURRENT, private->cur_context, first_param);
	if (ret)
	{
		OSMesaContext ctx = *((OSMesaContext *)first_param);
		private->cur_context = ctx;
	}
	return ret;
}


static OSMESAproc __CDECL slb_OSMesaGetProcAddress(BASEPAGE *__bp unused, long __fn unused, long __nwords unused, gl_private *private, void *first_param)
{
	const char *funcname = *((const char *const *)first_param);
	return internal_OSMesaGetProcAddress(private, funcname);
}



/* entry points of TinyGL functions */
static void *__CDECL slb_OSMesaCreateLDG(BASEPAGE *__bp unused, long __fn unused, long __nwords unused, gl_private *private, void *first_param)
{
	GLenum *args = (GLenum *)first_param;
	return internal_OSMesaCreateLDG(private, args[0], args[1], args[2], args[3]);
}


static void __CDECL slb_OSMesaDestroyLDG(BASEPAGE *__bp unused, long __fn unused, long __nwords unused, gl_private *private, void *first_param unused)
{
	internal_OSMesaDestroyLDG(private);
}


static GLsizei __CDECL slb_max_width(BASEPAGE *__bp unused, long __fn unused, long __nwords unused, gl_private *private, void *first_param unused)
{
	return internal_max_width(private);
}


static GLsizei __CDECL slb_max_height(BASEPAGE *__bp unused, long __fn unused, long __nwords unused, gl_private *private, void *first_param unused)
{
	return internal_max_height(private);
}


static void __CDECL slb_gluLookAtf(BASEPAGE *__bp unused, long __fn unused, long __nwords unused, gl_private *private, void *first_param)
{
	(*HostCall_p)(NFOSMESA_GLULOOKATF, private->cur_context, first_param);
}


static void __CDECL slb_glFrustumf(BASEPAGE *__bp unused, long __fn unused, long __nwords unused, gl_private *private, void *first_param)
{
	(*HostCall_p)(NFOSMESA_GLFRUSTUMF, private->cur_context, first_param);
}


static void __CDECL slb_glOrthof(BASEPAGE *__bp unused, long __fn unused, long __nwords unused, gl_private *private, void *first_param)
{
	(*HostCall_p)(NFOSMESA_GLORTHOF, private->cur_context, first_param);
}


static void __CDECL slb_exception_error(BASEPAGE *__bp unused, long __fn unused, long __nwords unused, gl_private *private, void *first_param)
{
	void CALLBACK (*exception)(GLenum param) = *((void CALLBACK (**)(GLenum))first_param);
	internal_tinyglexception_error(private, exception);
}


static void __CDECL slb_swapbuffer(BASEPAGE *__bp unused, long __fn unused, long __nwords unused, gl_private *private, void *first_param)
{
	void *buf = *((void **)first_param);
	internal_tinyglswapbuffer(private, buf);
}


static void __CDECL slb_information(BASEPAGE *__bp unused, long __fn unused, long __nwords unused, gl_private *private unused, void *first_param unused)
{
	tinyglinformation();
}



int err_old_nfapi(void)
{
	/* an error for Mesa_GL */
	return 1;
}


/*
 * these are not optional and cannot be set
 * to zero in the header, even if they
 * currently don't do anything
 */
static long __CDECL slb_init(void)
{
	return 0;
}


static void __CDECL slb_exit(void)
{
}


static long __CDECL slb_open(BASEPAGE *pd)
{
	(void) pd;
	return 0;
}


static long __CDECL slb_close(BASEPAGE *pd)
{
	(void) pd;
	return 0;
}


#include "versinfo.h"

void APIENTRY tinyglinformation(void)
{
	(void) Cconws("Mesa library NFOSMesa API Version " __STRINGIFY(ARANFOSMESA_NFAPI_VERSION) " " ASCII_ARCH_TARGET " " ASCII_COMPILER "\r\n");
}
