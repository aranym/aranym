
#ifndef _emu_bochs_h
#define _emu_bochs_h

extern "C" {
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#ifndef WIN32
#  include <unistd.h>
#else
#  include <io.h>
#endif
#include <time.h>
#ifdef macintosh
#  include <types.h>
#  include <stat.h>
#  include <utime.h>
#else
#  ifndef WIN32
#    include <sys/time.h>
#  endif
#  include <sys/types.h>
#  include <sys/stat.h>
#endif
#include <ctype.h>
#include <string.h>
#include <fcntl.h>
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
#define put(a)
#define settype(a)
#define BX_DEBUG(a)	printf a
#define BX_PANIC(a)	printf a
#define BX_INFO(a)	printf a
#define BX_ASSERT(x) do {if (!(x)) BX_PANIC(("failed assertion \"%s\" at %s:%d\n", #x, __FILE__, __LINE__));} while (0)
#define BX_INSERTED	10
#define BX_EJECTED	11
// konec mych triku

#endif
