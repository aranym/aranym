/* 2001 MJ */

 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Memory management
  *
  * (c) 1995 Bernd Schmidt
  */

#include "sysdeps.h"

#include "memory.h"

#if ARAM_PAGE_CHECK
uaecptr pc_page = 0xeeeeeeee;
uintptr pc_offset = 0;
uaecptr read_page = 0xeeeeeeee;
uintptr read_offset = 0;
uaecptr write_page = 0xeeeeeeee;
uintptr write_offset = 0;
#endif

#if !KNOWN_ALLOC && !NORMAL_ADDRESSING
// This part need rewrite for ARAnyM !!
// It can be taken from hatari.

#error Not prepared for your platform, maybe you need memory banks from hatari 

#endif /* !KNOWN_ALLOC && !NORMAL_ADDRESSING */
