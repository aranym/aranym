/*
 * gdbstub.h - GDB stub code - declaration
 *
 * Copyright (c) 2001-2005 Milan Jurik of ARAnyM dev team (see AUTHORS)
 *
 * Inspired by Bochs
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


#ifndef GDBSTUB_H
# define GDBSTUB_H

# include "sysdeps.h"

# ifdef GDBSTUB

extern int port_number;

typedef enum {
	no_command, continue_command, step_command
} last_command_t;

class gdbstub {
  static int hex(char ch);
  static void put_debug_char(char ch);
  static char get_debug_char(void);
  static void put_reply(char *buffer);
  static void get_command(char *buffer);
  static void hex2mem(char* buf, unsigned char* mem, int count);
  static char *mem2hex(char* mem, char* buf, int count);
  static int remove_breakpoint(memptr addr, int len);
  static void insert_breakpoint(memptr addr);
  static void write_signal(char* buf, int signal);
  static int access_linear(memptr laddress, unsigned int len, bool write, uae_u8 *data);
  static void wait_for_connect(int portn);
  static int socket_fd;
  static int listen_socket_fd;
  static int last_stop_reason;
  static last_command_t last_command;
  static int saved_pc;
public:
  static int init(int portn);
  static void debug_loop(void);
  static int check(memptr pc);
};

#  define GDBSTUB_STOP_NO_REASON          (0xac0)

#  define GDBSTUB_TEXT_BASE (0x0)
#  define GDBSTUB_DATA_BASE (0x0)
#  define GDBSTUB_BSS_BASE (0x0)

# endif /* GDBSTUB */
#endif /* GDBSTUB_H */
