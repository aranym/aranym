/* MJ 2001 */

#ifndef NDEBUG_H
#define NDEBUG_H

#include <stdio.h>
#include <ctype.h>
#include "sysdeps.h"

#ifdef NDEBUG

class ndebug {
  static unsigned int rowlen;
  static const unsigned int dbsize = 1000;
  static unsigned int warnlen;
  static char **dbbuffer;
  static unsigned int dbstart;
  static unsigned int dbend;
  static unsigned int dbfull;
  static unsigned int aktualrow;
  static char old_debug_cmd[80];
 
  static void reset_aktualrow();
  static void set_aktualrow(signed int);
  static unsigned int get_warnlen() { return warnlen; }
  static unsigned int get_rowlen() { return rowlen; }
  static void warn_print(FILE *);
  static void m68k_print(FILE *);
  static void show();
  static void showHelp();

  static char next_char( char **c) {
    ignore_ws (c);
    return *(*c)++;
  }
  static void ignore_ws (char **c) {
    while (**c && isspace(**c)) (*c)++;
  }
  static int more_params (char **c)
  {
    ignore_ws (c);
    return (**c) != 0;
  }
  static uae_u32 readhex (char **c)
  {
    uae_u32 val = 0;
    char nc;

    ignore_ws (c);

    while (isxdigit(nc = **c)) {
	(*c)++;
	val *= 16;
	nc = toupper(nc);
	if (isdigit(nc)) {
	    val += nc - '0';
	} else {
	    val += nc - 'A' + 10;
	}
    }
    return val;
  }

  
#endif

#ifdef NDEBUG
public:
  static
#endif
  int dbprintf(char *, ...);

#ifdef NDEBUG
  static void run();
  static void init();
};
#endif

#endif
