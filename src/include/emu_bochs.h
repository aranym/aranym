
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

#define BX_DEBUG(a)	D2(panicbug(a))
#define BX_PANIC(a)	panicbug(a)
#define BX_INFO(a)	infoprint(a)
#define BX_ASSERT(x) do {if (!(x)) BX_PANIC(("failed assertion \"%s\" at %s:%d\n", #x, __FILE__, __LINE__));} while (0)
#define BX_INSERTED	true
#define BX_EJECTED	false

#define bx_ptr_t void *

#endif
