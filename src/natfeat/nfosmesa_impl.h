#define __STDC_FORMAT_MACROS

#include "sysdeps.h"

#ifdef NFOSMESA_SUPPORT

#include "SDL_compat.h"
#include <SDL_loadso.h>
#include <SDL_endian.h>
#include <math.h>

#include "cpu_emulation.h"
#include "parameters.h"
#include "nfosmesa.h"
#include "../../atari/nfosmesa/nfosmesa_nfapi.h"
#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif
#ifndef PRId64
#  define PRId64 "lld"
#endif
#ifndef PRIu64
#  define PRIu64 "llu"
#endif
#if defined(__APPLE__)
   /* MacOSX declares GLintptr and GLsizeiptr as "long" */
#  define PRI_IPTR "lu"
#elif SIZEOF_VOID_P >= 8
#  define PRI_IPTR PRIu64
#else
#  define PRI_IPTR "u"
#endif
#define PRI_PTR "0x%08x"

#include "osmesa_context.h"

#define DEBUG 0
#include "debug.h"

/* undo the effects of Atari2HostAddr for pointer arguments when they specify a buffer offset */
#define Host2AtariAddr(a) ((memptr)((uintptr_t)(a) - MEMBaseDiff))

/* Read parameter on m68k stack */

#define getStackedParameter(n) nf_params[n]
#define getStackedParameter64(n) (((GLuint64)getStackedParameter(n) << 32) | (GLuint64)getStackedParameter((n) + 1))
#define getStackedFloat(n) Atari2HostFloat(getStackedParameter(n))
#define getStackedDouble(n) Atari2HostDouble(getStackedParameter(n), getStackedParameter((n) + 1))

#if NFOSMESA_POINTER_AS_MEMARG
#define getStackedPointer(n, t) getStackedParameter(n)
#define HostAddr(addr, t) (t)((addr) ? Atari2HostAddr(addr) : NULL)
#define AtariAddr(addr, t) addr
#define AtariOffset(addr) addr
#define NFHost2AtariAddr(addr) (void *)(uintptr_t)(addr)
#else
#define getStackedPointer(n, t) (t)(nf_params[n] ? Atari2HostAddr(getStackedParameter(n)) : NULL)
#define HostAddr(addr, t) (t)(addr)
#define AtariAddr(addr, t) (t)(addr)
	/* undo the effects of Atari2HostAddr for pointer arguments when they specify a buffer offset */
#define NFHost2AtariAddr(addr) ((void *)((uintptr_t)(addr) - MEMBaseDiff))
#define AtariOffset(addr) ((unsigned int)((uintptr_t)(addr) - MEMBaseDiff))
#endif

#endif /* NFOSMESA_SUPPORT */
