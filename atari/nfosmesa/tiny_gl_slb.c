#ifndef __GNUC__
#error "this file is for GNU-C only"
#endif
#include <osbind.h>
#include "nfosmesa_nfapi.h"
#define NFOSMESA_NO_MANGLE
#define TINYGL_ONLY
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
#define GL_PROC(type, ret, name, f, desc, proto, args) static type __CDECL slb_ ## f(BASEPAGE *base, long fn, long nwords, gl_private *priv, void *first_param);
#include "link-tinygl.h"

static long __CDECL slb_libinit(BASEPAGE *base, long fn, long nwords, gl_private *priv);

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
	SLB_LFUNC	slh_functions[NUM_TINYGL_PROCS + 1];	/* The function pointers */
};


static char const slh_name[];
static char const *const slh_names[];

static long __CDECL slb_init(void);
static void __CDECL slb_exit(void);
static long __CDECL slb_open(BASEPAGE *bp);
static long __CDECL slb_close(BASEPAGE *bp);

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
	NUM_TINYGL_PROCS + 1,
	{
		(SLB_LFUNC)slb_libinit,
/* generate the function table */
#define GL_PROC(type, ret, name, f, desc, proto, args) (SLB_LFUNC)slb_ ## f,
#include "link-tinygl.h"
	}
};

static char const slh_name[] = "tiny_gl.slb";

#define unused __attribute__((__unused__))

static long __CDECL slb_libinit(BASEPAGE *__bp unused, long __fn unused, long __nwords unused, gl_private *priv)
{
	internal_glInit(priv);
	return sizeof(*priv);
}


/* generate the function names */
static char const *const slh_names[] = {
	"glInit", /* slb_libinit */
#define GL_PROC(type, ret, name, f, desc, proto, args) name,
#include "link-tinygl.h"
	0
};

/* generate the wrapper functions */
#define voidf /**/
#define OSMESA_PROC(type, gl, name, export, upper, proto, args, first, ret)
#define GL_GETSTRING(type, gl, name, export, upper, proto, args, first, ret)
#define GL_GETSTRINGI(type, gl, name, export, upper, proto, args, first, ret)
#define GL_PROC(type, gl, name, export, upper, proto, args, first, ret) \
static type __CDECL slb_ ## gl ## name(BASEPAGE *__bp unused, long __fn unused, long __nwords unused, gl_private *priv, void *first_param) \
{ \
	ret (*HostCall_p)(NFOSMESA_GL ## upper, priv->cur_context, first_param); \
}
#define GLU_PROC(type, gl, name, export, upper, proto, args, first, ret) \
static type __CDECL slb_ ## gl ## name(BASEPAGE *bp unused, long fn unused, long nwords unused, gl_private *priv, void *first_param) \
{ \
	ret (*HostCall_p)(NFOSMESA_GLU ## upper, priv->cur_context, first_param); \
}
#include "glfuncs.h"
	

/* entry points of TinyGL functions */
static void *__CDECL slb_OSMesaCreateLDG(BASEPAGE *__bp unused, long __fn unused, long __nwords unused, gl_private *priv, void *first_param)
{
	GLenum *args = (GLenum *)first_param;
	return internal_OSMesaCreateLDG(priv, args[0], args[1], args[2], args[3]);
}


static void __CDECL slb_OSMesaDestroyLDG(BASEPAGE *__bp unused, long __fn unused, long __nwords unused, gl_private *priv, void *first_param unused)
{
	internal_OSMesaDestroyLDG(priv);
}


static GLsizei __CDECL slb_max_width(BASEPAGE *__bp unused, long __fn unused, long __nwords unused, gl_private *priv, void *first_param unused)
{
	return internal_max_width(priv);
}


static GLsizei __CDECL slb_max_height(BASEPAGE *__bp unused, long __fn unused, long __nwords unused, gl_private *priv, void *first_param unused)
{
	return internal_max_height(priv);
}


static void __CDECL slb_gluLookAtf(BASEPAGE *__bp unused, long __fn unused, long __nwords unused, gl_private *priv, void *first_param)
{
	(*HostCall_p)(NFOSMESA_GLULOOKATF, priv->cur_context, first_param);
}


static void __CDECL slb_glFrustumf(BASEPAGE *__bp unused, long __fn unused, long __nwords unused, gl_private *priv, void *first_param)
{
	(*HostCall_p)(NFOSMESA_GLFRUSTUMF, priv->cur_context, first_param);
}


static void __CDECL slb_glOrthof(BASEPAGE *__bp unused, long __fn unused, long __nwords unused, gl_private *priv, void *first_param)
{
	(*HostCall_p)(NFOSMESA_GLORTHOF, priv->cur_context, first_param);
}


static void __CDECL slb_tinyglexception_error(BASEPAGE *__bp unused, long __fn unused, long __nwords unused, gl_private *priv, void *first_param)
{
	void CALLBACK (*exception)(GLenum param) = *((void CALLBACK (**)(GLenum))first_param);
	internal_tinyglexception_error(priv, exception);
}


static void __CDECL slb_tinyglswapbuffer(BASEPAGE *__bp unused, long __fn unused, long __nwords unused, gl_private *priv, void *first_param)
{
	void *buf = *((void **)first_param);
	internal_tinyglswapbuffer(priv, buf);
}


static void __CDECL slb_tinyglinformation(BASEPAGE *__bp unused, long __fn unused, long __nwords unused, gl_private *priv unused, void *first_param unused)
{
	tinyglinformation();
}



int err_old_nfapi(void)
{
	/* not an error for TinyGL; the 83 functions should always be present */
	return 0;
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
	(void) Cconws("TinyGL NFOSMesa API Version " __STRINGIFY(ARANFOSMESA_NFAPI_VERSION) " " ASCII_ARCH_TARGET " " ASCII_COMPILER "\r\n");
}
