/* acconfig.h
   This file is in the public domain.

   Descriptive text for the C preprocessor macros that
   the distributed Autoconf macros can define.
   No software package will use all of them; autoheader copies the ones
   your configure.in uses into your configuration header file templates.

   The entries are in sort -df order: alphabetical, case insensitive,
   ignoring punctuation (such as underscores).  Although this order
   can split up related entries, it makes it easier to check whether
   a given entry is in the file.

   Leave the following blank line there!!  Autoheader needs it.  */


/* Define to 'off_t' if <sys/types.h> doesn't define. */
#undef loff_t

/* Define if you have the atanh function. */
#undef HAVE_ATANH

/* Define if using "mon". */
#undef ENABLE_MON

/* Define if using native 68k mode. */
#undef ENABLE_NATIVE_M68K

/* Define if your system support GNU SOURCEs (vasprintf function) */
#undef HAVE_GNU_SOURCE

/* Define if needed */
#undef _XOPEN_SOURCE

/* Define if using some debugger */
#undef DEBUGGER

/* Define if using New Debugger */
#undef NEWDEBUG

/* Define if using full MMU */
#undef FULLMMU

/* Define if you want direct truecolor output */
#undef DIRECT_TRUECOLOR

/* Define if you want accelerated blitter output */
#undef BLITTER_MEMMOVE

/* Define if you want even more accelerated blitter output */
#undef BLITTER_SDLBLIT

/* Define if you want to try out a different memory boundary check (maybe faster) */
#undef CHECK_BOUNDARY_BY_ARRAY

/* Define if you want TV conf GUI and have TV lib */
#undef HAVE_TVISION

/* Define if your system has a working vm_allocate()-based memory allocator */
#undef HAVE_MACH_VM

/* Define if your system has a working mmap()-based memory allocator */
#undef HAVE_MMAP_VM

/* Define if <sys/mman.h> defines MAP_ANON and mmap()'ing with MAP_ANON works */
#undef HAVE_MMAP_ANON

/* Define if <sys/mman.h> defines MAP_ANONYMOUS and mmap()'ing with MAP_ANONYMOUS works */
#undef HAVE_MMAP_ANONYMOUS

/* Define if you want to use full history */
#undef FULL_HISTORY

/* Define if you want to use direct access to host's fs */
#undef EXTFS_SUPPORT

/* Define if <getopt.h> knows getopt_long */
#undef HAVE_GETOPT_H


/* Leave that blank line there!!  Autoheader needs it.
   If you're adding to this file, keep in mind:
   The entries are in sort -df order: alphabetical, case insensitive,
   ignoring punctuation (such as underscores).  */
