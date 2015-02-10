#ifndef __GNUC__
#error "this file is for GNU-C only"
#endif
#include <osbind.h>
#include "nfosmesa_nfapi.h"
#define NFOSMESA_NO_MANGLE
#include <slb/tiny_gl.h>
#include <mint/basepage.h>
#include "lib-oldmesa.h"


#define __CDECL

typedef long __CDECL (*SLB_LFUNC)(BASEPAGE *pd, long fn, long nargs, ...);

#ifndef __STRINGIFY
#define __STRINGIFY(x) __STRINGIFY1(x)
#define __STRINGIFY1(x) #x
#endif

/* generate the prototypes */
#define varargs(proto...) proto
#define NOTHING
#define AND ,
#define GL_PROC(type, ret, name, f, desc, proto, args) static type __CDECL slb_ ## f(BASEPAGE *base, long fn, long nwords varargs proto);
#include "link-tinygl.h"

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
	SLB_LFUNC	slh_functions[NUM_TINYGL_PROCS];		/* The function pointers */
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
	2,
	0,
	slb_init,
	slb_exit,
	slb_open,
	slb_close,
	slh_names,
	0,
	{ 0, 0, 0, 0, 0, 0, 0 },
	NUM_TINYGL_PROCS,
	{
/* generate the function table */
#define GL_PROC(type, ret, name, f, desc, proto, args) (SLB_LFUNC)slb_ ## f,
#include "link-tinygl.h"
	}
};

static char const slh_name[] = "tiny_gl.slb";

/* generate the function names */
static char const *const slh_names[] = {
#define GL_PROC(type, ret, name, f, desc, proto, args) #name,
#include "link-tinygl.h"
	0
};

/* generate the wrapper functions */
#define voidf /**/
#define unused __attribute__((__unused__))
#define GL_PROC(type, ret, name, f, desc, proto, args) \
static type __CDECL slb_ ## f(BASEPAGE *bp unused, long fn unused, long nwords unused varargs proto) \
{ \
	ret f args; \
}
#include "link-tinygl.h"
	

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
