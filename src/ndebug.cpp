/*
 * $Header$
 *
 * MJ 2001
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "main.h"
#include "debug.h"
#include "ndebug.h"
#include "sysdeps.h"
#include "memory.h"
#include "newcpu.h"
#ifndef HAVE_GNU_SOURCE

#error vasprintf def?

#endif

#include "parameters.h"

#ifdef NDEBUG
int ndebug::dbprintf(char *s, ...) {
#else
int dbprintf(char *s, ...) {
#endif
  va_list a;
  va_start(a, s);
#ifdef NDEBUG
    if (start_debug) {
      int i;
      if ((dbend == dbstart) && (dbfull)) dbstart = (dbstart + 1) % dbsize;
      dbfull = 1;
      if (dbbuffer[dbend] != NULL) free(dbbuffer[dbend]);
      i = vasprintf(&dbbuffer[dbend++], s, a);
      dbend %= dbsize;
      if ((dbstart > dbend) || (dbend >= get_warnlen())) 
        aktualrow = (aktualrow + 1) % dbsize;
      return i;
    } else {
#endif
      int i;
      char *c;
      i = vasprintf(&c, s, a);
      i += fprintf(stderr, c);
      i += fprintf(stderr, "\n");
      free(c);
      return i;
#ifdef NDEBUG
    }
#endif
    va_end(a);
  }

/*
 * $Log$
 *
 *
 */