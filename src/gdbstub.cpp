/*
 * gdbstub.cpp - GDB stub code
 *
 * Copyright (c) 2001-2005 Milan Jurik of ARAnyM dev team (see AUTHORS)
 *
 * Based on Bochs code
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

#include "sysdeps.h"

#include "gdbstub.h"

#include "cpummu.h"
#include "newcpu.h"

#include "cpu_emulation.h"

#include "main.h"

# include <cstdlib>

#ifdef __MINGW32__
#include <winsock2.h>
#define SIGTRAP 5
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <signal.h>
#include <netdb.h>
#endif

#define NEED_CPU_REG_SHORTCUTS 1

#define DEBUG 2
#include "debug.h"

#define LOG_THIS genlog->
#define IFDBG(x) x

int port_number = 1234;

#define GDBSTUB_EXECUTION_BREAKPOINT    (0xac1)
#define GDBSTUB_TRACE                   (0xac2)
#define GDBSTUB_USER_BREAK              (0xac3)
#define GDBSTUB_COMMAND                 (0xacf)

int gdbstub::listen_socket_fd = 0;
int gdbstub::socket_fd = 0;
int gdbstub::last_stop_reason = GDBSTUB_STOP_NO_REASON;
last_command_t gdbstub::last_command = no_command;


int gdbstub::hex(char ch)
{
   if ((ch >= 'a') && (ch <= 'f')) return(ch - 'a' + 10);
   if ((ch >= '0') && (ch <= '9')) return(ch - '0');
   if ((ch >= 'A') && (ch <= 'F')) return(ch - 'A' + 10);
   return(-1);
}

void gdbstub::put_debug_char(char ch)
{
  send(socket_fd, &ch, 1, 0);
}

char gdbstub::get_debug_char(void)
{
   char ch;
   
   recv(socket_fd, &ch, 1, 0);
   
   return(ch);
}

static const char hexchars[]="0123456789abcdef";

void gdbstub::put_reply(char *buffer)
{
   unsigned char csum;
   int i;
   
   D(bug("put_buffer %s", buffer));
   
   do
     { 
       put_debug_char('$');
       
       csum = 0;
       
       i = 0;
       while (buffer[i] != 0)
         {
            put_debug_char(buffer[i]);
            csum = csum + buffer[i];
            i++;
         }
       
       put_debug_char('#');
       put_debug_char(hexchars[csum >> 4]);
       put_debug_char(hexchars[csum % 16]);
     } while (get_debug_char() != '+');
}

void gdbstub::get_command(char *buffer)
{
   unsigned char checksum;
   unsigned char xmitcsum;
   char ch;
   unsigned int count;
   unsigned int i;
   
   do
     {
       while ((ch = get_debug_char()) != '$');
       
       checksum = 0;
       xmitcsum = 0;
       count = 0;
   
       while (1)
         {
            ch = get_debug_char();
            if (ch == '#') break;
            checksum = checksum + ch;
            buffer[count] = ch;
            count++;
         }
       buffer[count] = 0;
       
       if (ch == '#')
         {
            xmitcsum = hex(get_debug_char()) << 4;
            xmitcsum += hex(get_debug_char());
            if (checksum != xmitcsum)
              {
                 D2(bug("Bad checksum"));
              }
         }
       
       if (checksum != xmitcsum)
         {
            put_debug_char('-');
         }
       else
         {
            put_debug_char('+');
            if (buffer[2] == ':')
              {
                 put_debug_char(buffer[0]);
                 put_debug_char(buffer[1]);
                 count = strlen(buffer);
                 for (i = 3; i <= count; i++)
                   {
                      buffer[i - 3] = buffer[i];
                   }
              }
         }
     } while (checksum != xmitcsum);   
}

void gdbstub::hex2mem(char* buf, unsigned char* mem, int count)
{
   int i;
   unsigned char ch;
   
   for (i = 0; i<count; i++)
     {
       ch = hex(*buf++) << 4;
       ch = ch + hex(*buf++);
       *mem = ch;
       mem++;
     }
}

char *gdbstub::mem2hex(char* mem, char* buf, int count)
{
   int i;
   unsigned char ch;
   
   for (i = 0; i<count; i++)
     {
       ch = *mem;
       mem++;
       *buf = hexchars[ch >> 4];
       buf++;
       *buf = hexchars[ch % 16];
       buf++;  
     }
   *buf = 0;
   return(buf);
}

static int continue_thread = -1;
static int other_thread = 0;

#define NUMREGS (17)
#define NUMREGSBYTES (NUMREGS * 4)
static int registers[NUMREGS];

#define MAX_BREAKPOINTS (255)
static unsigned int breakpoints[MAX_BREAKPOINTS] = {0,};
static unsigned int nr_breakpoints = 0;

static int stub_trace_flag = 0;

static int instr_count = 0;

int gdbstub::saved_pc = 0;

int gdbstub::check(memptr pc)
{
   unsigned int i;
   unsigned char ch;
   long arg;
   int r;
#if defined(__CYGWIN__) || defined(__MINGW32__)
   fd_set fds;
   struct timeval tv = {0, 0};
#endif

   instr_count++;
   
   if ((instr_count % 500) == 0)
     {
#if !defined(__CYGWIN__) && !defined(__MINGW32__)
       arg = fcntl(socket_fd, F_GETFL);
       fcntl(socket_fd, F_SETFL, arg | O_NONBLOCK);
       r = recv(socket_fd, &ch, 1, 0);
       fcntl(socket_fd, F_SETFL, arg);
#else
       FD_ZERO(&fds);
       FD_SET(socket_fd, &fds);
        r = select(socket_fd + 1, &fds, NULL, NULL, &tv);
       if (r == 1)
         {
           r = recv(socket_fd, (char *)&ch, 1, 0);
         }
#endif   
       if (r == 1)
         {
            D2(bug("Got byte %x", (unsigned int)ch));
            last_stop_reason = GDBSTUB_USER_BREAK;
            return(GDBSTUB_USER_BREAK);
         }
     }
   
   if (last_command != no_command)
     {
       last_stop_reason = GDBSTUB_COMMAND;
       return GDBSTUB_COMMAND;
     }
   // why is trace before breakpoints? does that mean it would never
   // hit a breakpoint during tracing?
   if (stub_trace_flag == 1)
     {
       last_stop_reason = GDBSTUB_TRACE;
       return(GDBSTUB_TRACE);
     }
   for (i = 0; i < nr_breakpoints; i++)
     {
       if (pc == breakpoints[i])
         {
            D2(bug("found breakpoint at %x", pc));
            last_stop_reason = GDBSTUB_EXECUTION_BREAKPOINT;
            return(GDBSTUB_EXECUTION_BREAKPOINT);
         }
     }
   last_stop_reason = GDBSTUB_STOP_NO_REASON;
   return(GDBSTUB_STOP_NO_REASON);
}

int gdbstub::remove_breakpoint(memptr addr, int len)
{
   unsigned int i;
   
   if (len != 1)
     {
       return(0);
     }
   
   for (i = 0; i < MAX_BREAKPOINTS; i++)
     {
       if (breakpoints[i] == addr)
         {
            D2(bug("Removing breakpoint at %x", addr));
            breakpoints[i] = 0;
            return(1);
         }
     }
   return(0);
}

void gdbstub::insert_breakpoint(memptr addr)
{
   unsigned int i;
   
   D2(bug("setting breakpoint at %x", addr));
   
   for (i = 0; i < (unsigned)MAX_BREAKPOINTS; i++)
     {
       if (breakpoints[i] == 0)
         {
            breakpoints[i] = addr;
            if (i >= nr_breakpoints)
              {
                 nr_breakpoints = i + 1;
              }
            return;
         }
     }
   D2(bug("No slot for breakpoint"));
}

void gdbstub::write_signal(char* buf, int signal)
{
   buf[0] = hexchars[signal >> 4];
   buf[1] = hexchars[signal % 16];
   buf[2] = 0;
}

int gdbstub::access_linear(memptr laddress,
                        unsigned int len,
                        bool write,
                        uae_u8* data)
{
  memptr phys;
  bool valid;
   
#if 0
   if (((laddress & 0xfff) + len) > 4096)
     {
       valid = access_linear(laddress,
                             4096 - (laddress & 0xfff),
                             rw,
                             data);
       if (!valid)
         {
            return(valid);
         }
       valid = access_linear(laddress,
                             len + (laddress & 0xfff) - 4096,
                             rw,
                             (Bit8u *)((unsigned int)data + 
                                      4096 - (laddress & 0xfff)));
       return(valid);
     }
#endif

#if 0
   BX_CPU(0)->dbg_xlate_linear2phy((Bit32u)laddress, 
                                       (Bit32u*)&phys, 
                                       (bool*)&valid);
#else
# ifdef FULLMMU
   int pomv;
   if (regs.mmu_enabled) {
	if (valid = ((pomv = mmu_translate(laddress, FC_DATA, 0, m68k_getpc(), sz_word, 1)) ? valid_address(pomv, 0, m68k_getpc(), sz_word): false)) {
	   phys = mmu_translate(laddress, FC_DATA, 0, m68k_getpc(), sz_word, 0);
	}
   } else {
# endif
	valid = valid_address(laddress, 0, m68k_getpc(), sz_word);
	phys = laddress;
# ifdef FULLMMU
   }
# endif
#endif

   if (!valid)
     {
       return(0);
     }
   
   while(len--) {
	if (write)
		WriteInt8(phys++, *data++);
	else
		*data++ = ReadInt8(phys++);
   }
   return(valid);
}

void gdbstub::debug_loop(void)
{
   char buffer[255];
   char obuf[255];
   int ne;
   unsigned char mem[255];
   
   ne = 0;
   
   while (ne == 0)
     {
       if (last_command == no_command) {
         get_command(buffer);
         D(bug("get_buffer %s", buffer));
       } else {
	 switch (last_command)
	   {
	     case continue_command:
		 buffer[0] = 'c';
		 break;
	     case step_command:
		 buffer[0] = 's';
		 break;
             default:
		 D(bug("Unknown previous command"));
		 break;
	   }
	 D(bug("last_command %c", buffer[0]));
       }
       switch (buffer[0])
         {
          case 'c':
              {
		 if (last_command == no_command) {
                   int new_pc;
                 
                   if (buffer[1] != 0)
                     {
                        new_pc = atoi(buffer + 1);
                      
                        D2(bug("continuing at %x", new_pc));
#if 0
		      for (int i=0; i<BX_SMP_PROCESSORS; i++) {
                        BX_CPU(i)->invalidate_prefetch_q();
		      }
#endif
                        saved_pc = m68k_getpc();
                      
                        m68k_setpc(new_pc);
                     }
                 
                   stub_trace_flag = 0;
#if 0
                 bx_cpu.ispanic = 0;
                 bx_cpu.cpu_loop(-1);              
                 if (bx_cpu.ispanic)
                 {
                    last_stop_reason = GDBSTUB_EXECUTION_BREAKPOINT;
                 }
#endif
		   SPCFLAGS_SET( SPCFLAG_BRK );
		   ne = 1;
		   last_command = continue_command;
		 } else {
#if 0
                 DEV_vga_refresh();
#endif
                   char buf[255];
		   last_command = no_command;
                   if (buffer[1] != 0)
                     {
#if 0
                      bx_cpu.invalidate_prefetch_q();
#endif
                        m68k_setpc(saved_pc);
                     }
                 
                   D2(bug("stopped with %x", last_stop_reason));                               
                   buf[0] = 'S';
                   if (last_stop_reason == GDBSTUB_EXECUTION_BREAKPOINT ||
                       last_stop_reason == GDBSTUB_TRACE)
                     {
                        write_signal(&buf[1], SIGTRAP);
                     }
                     else
                     {
                        write_signal(&buf[1], 0);
                     }
                   put_reply(buf);
		 }
                 break;
              }
            
          case 's':
              {
                 if (last_command == no_command)
		 {
                   D2(bug("stepping"));
                   stub_trace_flag = 1;
#if 0
                 bx_cpu.cpu_loop(-1);
#endif
		   SPCFLAGS_SET( SPCFLAG_BRK );
		   ne = 1;
		   last_command = step_command;
		 } else {
#if 0
                 DEV_vga_refresh();
#endif
		   last_command = no_command;
                   char buf[255];
                   stub_trace_flag = 0;
                   D2(bug("stopped with %x", last_stop_reason));
                   buf[0] = 'S';
                   if (last_stop_reason == GDBSTUB_EXECUTION_BREAKPOINT ||
                       last_stop_reason == GDBSTUB_TRACE)
                     {
                        write_signal(&buf[1], SIGTRAP);
                     }
                   else
                     {
                        write_signal(&buf[1], SIGTRAP);
                     }
                   put_reply(buf);
		 }
                 break;
              }
            
          case 'M':
              {
                 int addr;
                 int len;
                 unsigned char mem[255];
                 char* ebuf;
                 
                 addr = strtoul(&buffer[1], &ebuf, 16);
                 len = strtoul(ebuf + 1, &ebuf, 16);
                 hex2mem(ebuf + 1, mem, len);          
                 
                 if (len == 1 && mem[0] == 0xcc)
                   {
                      insert_breakpoint(addr);
                      put_reply("OK");
                   }
                 else if (remove_breakpoint(addr, len))
                   {
                      put_reply("OK");
                   }
                 else
                   {
                      if (access_linear(addr,
                                        len,
                                        true,
                                        mem))
                        {
                           put_reply("OK");
                        }
                      else
                        {
                           put_reply("ENN");
                        }
                   }
                 break;                    
              }
            
          case 'm':
              {
                 int addr;
                 int len;
                 char* ebuf;
                 
                 addr = strtoul(&buffer[1], &ebuf, 16);
                 len = strtoul(ebuf + 1, NULL, 16);
                 D2(bug("addr %x len %x", addr, len));
                 
                 access_linear(addr,
                               len,
                               false,
                               mem);
                 mem2hex((char *)mem, obuf, len);
                 put_reply(obuf);
                 break;
              }
          case 'P':
              {
                 int reg;
                 int value;
                 char* ebuf;
                 
                 reg = strtoul(&buffer[1], &ebuf, 16);
                 value = ntohl(strtoul(ebuf + 1, &ebuf, 16));
                 
                 D2(bug("reg %d set to %x", reg, value));
                 
                 switch (reg)
                   {
#if 0
                    case 1:
                      EAX = value;
                      break;
                      
                    case 2:
                      ECX = value;
                      break;
                      
                    case 3:
                      EBX = value;
                      break;
                      
                    case 4:
                      ESP = value;
                      break;
                      
                    case 5:
                      EBP = value;
                      break;
                      
                    case 6:
                      ESI = value;
                      break;
                      
                    case 7:
                      EDI = value;
                      break;
#endif
                    case 8:
                      m68k_setpc(value);
#if 0
                      BX_CPU_THIS_PTR invalidate_prefetch_q();
#endif
                      break;
                      
                    default:
                      break;
                   }
                 
                 put_reply("OK");
                 
                 break;
              }
            
          case 'g':
            registers[0] = regs.regs[0];
            registers[1] = regs.regs[1];
            registers[2] = regs.regs[2];
            registers[3] = regs.regs[3];
            registers[4] = regs.regs[4];
            registers[5] = regs.regs[5];
            registers[6] = regs.regs[6];
            registers[7] = regs.regs[7];
#if 0
	    if (last_stop_reason == GDBSTUB_EXECUTION_BREAKPOINT)
              {
                 registers[8] = m68k_getpc() + 1;
              }
            else
              {
                 registers[8] = m68k_getpc();
              }
            registers[9] = BX_CPU_THIS_PTR read_eflags();
            registers[10] = 
              BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].selector.value;
            registers[11] = 
              BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].selector.value;
            registers[12] = 
              BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].selector.value;
            registers[13] = 
              BX_CPU_THIS_PTR sregs[BX_SEG_REG_ES].selector.value;
            registers[14] = 
              BX_CPU_THIS_PTR sregs[BX_SEG_REG_FS].selector.value;
            registers[15] = 
              BX_CPU_THIS_PTR sregs[BX_SEG_REG_GS].selector.value;
#endif
            mem2hex((char *)registers, obuf, NUMREGSBYTES);
            put_reply(obuf);
            break;
            
          case '?':
            sprintf(obuf, "S%02x", SIGTRAP);
            put_reply(obuf);
            break;
            
          case 'H':
            if (buffer[1] == 'c')
              {
                 continue_thread = strtol(&buffer[2], NULL, 16);
                 put_reply("OK");
              }
            else if (buffer[1] == 'g')
              {
                 other_thread = strtol(&buffer[2], NULL, 16);
                 put_reply("OK");
              }
            else
              {
                 put_reply("ENN");
              }
            break;
            
          case 'q':
            if (buffer[1] == 'C')
              {
                 sprintf(obuf,"%Lx", (unsigned long long int)1);
                 put_reply(obuf);
              }
            else if (strncmp(&buffer[1], "Offsets", strlen("Offsets")) == 0)
              {
                 sprintf(obuf,
                         "Text=%x;Data=%x;Bss=%x",
                         bx_options.gdbstub.text_base, 
                         bx_options.gdbstub.data_base, 
                         bx_options.gdbstub.bss_base);
                 put_reply(obuf);
              }
            else
              {
                 put_reply("ENN");
              }          
            break;
            
          case 'k':
            infoprint("Debugger asked us to quit");
	    QuitEmulator();
	    ne = 1;
            break;
            
          default:
            put_reply("");
            break;
         }
     }
}

void gdbstub::wait_for_connect(int portn)
{
   struct sockaddr_in sockaddr;
   socklen_t sockaddr_len;
   struct protoent *protoent;
   int r;
   int opt;
   
   listen_socket_fd = socket(PF_INET, SOCK_STREAM, 0);
   if (listen_socket_fd == -1)
     {
       panicbug("Failed to create socket\n");
       exit(1);
     }
   
   /* Allow rapid reuse of this port */
   opt = 1;
#if __MINGW32__
   r = setsockopt(listen_socket_fd, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt));
#else
   r = setsockopt(listen_socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif
   if (r == -1)
     {
       D(bug("setsockopt(SO_REUSEADDR) failed\n"));
     }
   
   memset (&sockaddr, '\000', sizeof sockaddr);
#if BX_HAVE_SOCKADDR_IN_SIN_LEN
   // if you don't have sin_len change that to #if 0.  This is the subject of
   // bug [ 626840 ] no 'sin_len' in 'struct sockaddr_in'.
   sockaddr.sin_len = sizeof sockaddr;
#endif
   sockaddr.sin_family = AF_INET;
   sockaddr.sin_port = htons(portn);
   sockaddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

   r = bind(listen_socket_fd, (struct sockaddr *)&sockaddr, sizeof(sockaddr));
   if (r == -1)
     {
       panicbug("Failed to bind socket");
       exit(1);
     }

   r = listen(listen_socket_fd, 0);
   if (r == -1)
     {
       panicbug("Failed to listen on socket");
       exit(1);
     }
   
   sockaddr_len = sizeof sockaddr;
   socket_fd = accept(listen_socket_fd, (struct sockaddr *)&sockaddr, &sockaddr_len);
   if (socket_fd == -1)
     {
       panicbug("Failed to accept on socket");
       exit(1);
     }
   close(listen_socket_fd);
   
   protoent = getprotobyname ("tcp");
   if (!protoent)
     {
       panicbug("getprotobyname (\"tcp\") failed");
       return;
     }

   /* Disable Nagle - allow small packets to be sent without delay. */
   opt = 1;
#ifdef __MINGW32__
   r = setsockopt (socket_fd, protoent->p_proto, TCP_NODELAY, (const char *)&opt, sizeof(opt));
#else
   r = setsockopt (socket_fd, protoent->p_proto, TCP_NODELAY, &opt, sizeof(opt));
#endif
   if (r == -1)
     {
       D(bug("setsockopt(TCP_NODELAY) failed"));
     }
}

int gdbstub::init(int portn)
{
#ifdef __MINGW32__
   WSADATA wsaData;
   WSAStartup(2, &wsaData);
#endif

   /* Wait for connect */

   infoprint("Waiting for gdb connection on localhost: %d", portn);
   wait_for_connect(portn);
   
   /* Do debugger command loop */
   debug_loop();
   return 0;
}

