/*
 * ndebug.h - new ARAnyM fullscreen debugger - header file
 *
 * Copyright (c) 2001-2006 Milan Jurik of ARAnyM dev team (see AUTHORS)
 *
 * This file is part of the ARAnyM project which builds a new and powerful
 * TOS/FreeMiNT compatible virtual machine running on almost any hardware.
 *
 * ARAnyM is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * ARAnyM is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ARAnyM; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* MJ 2001 */

#ifndef NDEBUG_H
#define NDEBUG_H

#include "sysdeps.h"
#include "identify.h"
#include "memory-uae.h"
#include "cpu_emulation.h"

# include <cctype>

class ndebug {

#ifdef DEBUGGER
  typedef enum {EQUAL_value_test_8, EQUAL_value_test_16, EQUAL_value_test_32, CHANGE_value_test_8, CHANGE_value_test_16, CHANGE_value_test_32 } value_test_t;

  static const unsigned int max_breakpoints = 256;

#ifdef HAVE_TERMIOS_H
  static termios savetty;
#endif
  static bool issavettyvalid;
  static unsigned int rowlen;
  static const unsigned int dbsize = 1000;
  static unsigned int warnlen;
  static char **dbbuffer;
  static unsigned int dbstart;
  static unsigned int dbend;
  static unsigned int dbfull;
  static unsigned int actualrow;
  static unsigned int tp;
  static uaecptr skipaddr;
  static bool do_skip_value;
  static uaecptr value_addr;
  static uint32 value;
  static value_test_t value_test;
  static unsigned int do_breakpoints;
  static bool breakpoint[max_breakpoints];
  static uaecptr breakpoint_address[max_breakpoints];
  
  static char old_debug_cmd[80];
 
  static void reset_actualrow();
  static void set_actualrow(signed int);

  static unsigned int get_len(); // { return 25; }
  static unsigned int get_warnlen() {
	return warnlen = get_len() - 12;
  }

  static unsigned int get_rowlen() { return rowlen; }
  static void warn_print(FILE *);
  static void m68k_print(FILE *);
  static void instr_print(FILE *);
  static void show(FILE *);
  static void showHelp(FILE *);
  static void set_Ax(char **);
  static void set_Dx(char **);
  static void set_Px(char **);
  static void set_Sx(char **);
  static void saveintofile(FILE *, char **);
  static void errorintofile(FILE *, char **);
  static void loadintomemory(FILE *, char **);
  static void convertNo(char **);
  static uae_u32 readhex(char, char **);
  static uae_u32 readoct(char, char **);
  static uae_u32 readdec(char, char **);
  static uae_u32 readbin(char, char **);
  static char *dectobin(uae_u32);

  static void pressenkey(FILE *f) {
    char input[80];
    fprintf(f, "Press ENTER\n");
    fflush(f);
    if (fgets(input, 80, stdin) == NULL) {
        fprintf(stderr, "Internal error!\n");
        return;
    }
  }
  
  static char next_char(char **c) {
    ignore_ws (c);
    return *(*c)++;
  }

  static void ignore_ws(char **c);

  static int more_params(char **c) {
    ignore_ws (c);
    return (**c) != 0;
  }
  static uae_u32 readhex(char **c) {
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

  static void dump_traps(FILE *f) {
    for (int i = 0; trap_labels[i].name; i++) {
      fprintf(f, "$%02x: %s\t $%08x\n", trap_labels[i].adr,
        trap_labels[i].name, ReadAtariInt32(trap_labels[i].adr));
    }
  }

  static void dump_ints(FILE *f) {
    for (int i = 0; int_labels[i].name; i++) {
      fprintf(f, "$%02x: %s\t $%08x\n", int_labels[i].adr,
        int_labels[i].name, ReadAtariInt32(int_labels[i].adr));
    }
  }

  static void dumpmem(FILE *, uaecptr, uaecptr *, unsigned int);
  static void writeintomem(FILE *, char **);
  static void backtrace(FILE *, unsigned int);
  static void log2phys(FILE *, uaecptr);

  static void showTypes();
  static int canon(FILE *, uaecptr, uaecptr &, uaecptr &);
  static int icanon(FILE *, uaecptr, uaecptr &, uaecptr &);
  static int dm(FILE *, uaecptr, uaecptr &, uaecptr &);
 
#endif /* DEBUGGER */

public:
  static void dbprintf(const char *, ...) __attribute__((format(__printf__, 1, 2)));

  static void pdbprintf(const char *, ...) __attribute__((format(__printf__, 1, 2)));
  static void pdbvprintf(const char *, va_list args) __attribute__((format(__printf__, 1, 0)));
	
#ifdef DEBUGGER
  static bool do_skip;
  static void run();
  static void init();
  static void nexit();
#endif
#ifdef FULL_HISTORY
  static void showHistory(unsigned int, bool showLast = true);
#endif
};

extern "C" void guialert(const char *, ...) __attribute__((format(__printf__, 1, 2)));

#endif
