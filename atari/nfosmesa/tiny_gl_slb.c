#ifndef __GNUC__
#error "this file is for GNU-C only"
#endif
#include <osbind.h>
#include "nfosmesa_nfapi.h"
#include <slb/tiny_gl.h>


#define __CDECL

typedef long __CDECL (*SLB_FUNC)(BASEPAGE *pd, long fn, short nargs, ...);

#define UNDERSCORE "_"

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
	long		slh_reserved[8];						/* Currently 0L and unused */
	long		slh_no_funcs;							/* Number of functions */
	SLB_FUNC	slh_functions[];						/* The function pointers */
};


#ifndef __STRINGIFY
#define __STRINGIFY(x) __STRINGIFY1(x)
#define __STRINGIFY1(x) #x
#endif

/* first include is onyl to get definition of NUM_TINYGL_PROCS */
#define GL_PROC(name, f, desc)
#include "link-tinygl.h"

/*
 * This replaces the startup code, and must be the first thing in this file,
 * and also in the resulting executable
 */
#define SLB_HEAD(version, flags, no_funcs) \
	__asm__ ("\
	.text\n\
	.dc.l 0x70004afc\n\
	.dc.l slh_name\n\
	.dc.l " __STRINGIFY(version) "\n\
	.dc.l " __STRINGIFY(flags) "\n\
	.dc.l " UNDERSCORE "slb_init\n\
	.dc.l " UNDERSCORE "slb_exit\n\
	.dc.l " UNDERSCORE "slb_open\n\
	.dc.l " UNDERSCORE "slb_close\n\
	.dc.l slh_names\n\
	.dc.l 0,0,0,0,0,0,0,0\n\
	.dc.l " __STRINGIFY(no_funcs) "\n");

/* spit out the header */
SLB_HEAD(1, 0, NUM_TINYGL_PROCS)

/* generate the function table */
#define GL_PROC(name, f, desc) __asm__(".dc.l " UNDERSCORE #f "_execwrap\n");
#include "link-tinygl.h"

__asm__ ("\
slh_name:	.asciz \"tiny_gl.slb\"\n\
slh_names:\n");

/* generate the function names */
#define GL_PROC(name, f, desc) __asm__(".asciz \"" name "\"\n");
#include "link-tinygl.h"
__asm__ (".even\n");

/* generate the wrapper functions */
/* move return pc, and pop BASEPAGE *, function #, and number of args */
#define GL_PROC(name, f, desc) \
	__asm__(UNDERSCORE #f "_execwrap:\n\
	move.l (sp),10(sp)\n\
	lea 10(sp),sp\n\
	braw " UNDERSCORE #f "\n");
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
static __attribute__((used))
long __CDECL slb_init(void)
{
	return 0;
}


static __attribute__((used))
void __CDECL slb_exit(void)
{
}


static __attribute__((used))
long __CDECL slb_open(BASEPAGE *pd)
{
	(void) pd;
	return 0;
}


static __attribute__((used))
void __CDECL slb_close(BASEPAGE *pd)
{
	(void) pd;
}


#include "versinfo.h"

void APIENTRY tinyglinformation(void)
{
	(void) Cconws("TinyGL NFOSMesa API Version " __STRINGIFY(ARANFOSMESA_NFAPI_VERSION) " " ASCII_ARCH_TARGET " " ASCII_COMPILER "\r\n");
}
