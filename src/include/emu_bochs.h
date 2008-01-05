
#ifndef _emu_bochs_h
#define _emu_bochs_h

extern "C" {
#ifndef WIN32
#  include <unistd.h>
#else
#  include <io.h>
#endif
#ifdef macintosh
#  include "macutils.h"
#  define SuperDrive "[fd:]"
#endif
}

// Hacks for win32: always return regular file.
#ifdef WIN32
#ifndef __MINGW32__
#  define S_ISREG(st_mode) 1
#  define S_ISCHR(st_mode) 0

// VCPP includes also are missing these
#  define off_t long
#  define ssize_t int
#endif

// win32 has snprintf though with different name.
#define snprintf _snprintf

#endif


// moje triky
#define BX_DEBUG(a)	printf a
#define BX_PANIC(a)	printf a
#define BX_INFO(a)	printf a
#define BX_ASSERT(x) do {if (!(x)) BX_PANIC(("failed assertion \"%s\" at %s:%d\n", #x, __FILE__, __LINE__));} while (0)
#define BX_INSERTED	true
#define BX_EJECTED	false
// konec mych triku

#define bx_ptr_t void *

#endif
