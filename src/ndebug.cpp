/*
 * $Header$
 *
 * MJ 2001
 */

#include "sysdeps.h"

#include "memory.h"
#include "newcpu.h"
#include "m68k.h"
#include "cpu_emulation.h"
#include "input.h"
#include "cpummu.h"
#include "fpu/fpu.h"

#define DEBUG 1
#include "debug.h"

#ifdef HAVE_NEW_HEADERS
# include <cstdarg>
# include <cstdlib>
#else
# include <stdarg.h>
# include <stdlib.h>
#endif

#ifndef HAVE_GNU_SOURCE

/* NDEBUG needs vasprintf, implementation in GNU binutils (libiberty) */
extern "C" int vasprintf(char **, const char *, va_list);

#endif

#include "parameters.h"

#ifdef NEWDEBUG

#ifndef HAVE_STRDUP
extern "C" char *strdup(const char *s)
{
	char *n = (char *)malloc(strlen(s) + 1);
	strcpy(n, s);
	return n;
}
#endif

static termios savetty;

unsigned int ndebug::rowlen = 78;
//unsigned int ndebug::dbsize = 1000;
unsigned int ndebug::warnlen = 10;
char **ndebug::dbbuffer = NULL;
unsigned int ndebug::dbstart = 0;
unsigned int ndebug::dbend = 0;
unsigned int ndebug::dbfull = 0;
unsigned int ndebug::aktualrow = 0;
unsigned int ndebug::tp = 0;
bool ndebug::do_skip = false;
uaecptr ndebug::skipaddr = 0;
bool ndebug::do_skip_value = false;
uaecptr ndebug::value_addr = 0;
uint32 ndebug::value;
value_test_t  ndebug::value_test;

uint32 ndebug::do_breakpoints = 0;
bool ndebug::breakpoint[max_breakpoints];
uaecptr ndebug::breakpoint_address[max_breakpoints];
  
char ndebug::old_debug_cmd[80] = "                                                                               ";
static char *strhelp[] = {"Help:\n",
	" h, ?                 show this help\n",
	" Q                    quit ARAnyM\n",
	" R                    reset CPU\n",
	" A <number> <value>   set Ax\n",
	" D <number> <value>   set Dx\n",
	" P <number> <value>   set USP(0), ISP(1), MSP(2), VBR(3), IMASK(4), PC(5)\n",
	" S <number> <value>   set TC(0), SRP(1), URP(2), DTT0(3), DTT1(4),\n",
	"                      ITT0(5), ITT1(6), CACR(7), CAAR(8)\n",
	" i                    enable/disable IRQ\n",
	" I                    enable/disable IRQ debugging\n",
	" g <address>          start execution at the current address or <address>\n",
	" t <address>          step one instruction at current addres or <address>\n",
	" z                    step through one instruction - useful for JSR, DBRA etc\n",
	" f <address>          step forward until PC=<address>\n",
	" m <address> <lines>  memory dump starting at <address>\n",
	" d <address> <lines>  disassembly starting at <address>\n",
	" E                    dump contents of interrupt registers\n",
	" e                    dump contents of trap vectors\n",
	" b <lines>            dump contents of stack's top\n",
	" T <address>          translate log2phys address\n",
	" s <file> <addr> <n>  save a block of ARAnyM memory\n",
	" l <file> <addr>      load file to ARAnyM memory\n",
	" W <address> <value>  write into ARAnyM memory\n",
	" N <number>           convert No.\n",
	" C                    show numbers of debug types\n",
	" c <number>           change to debug type No.\n",
	" q                    change to debug type No. 0\n",
	" j | J                browse up\n",
	" k | K                browse down\n",
	" r                    refresh D(bug()) | reset aktual row\n",
	" L <file>             save debugger's info to <file>\n",
	" v X <addr> <value>   step forward until (<address>) = <value> (X=b,w,l)\n",
	" V X <address>        step forward until (<address>) changed (X=b,w,l)\n",
	" B <address>          toggle breakpoint on <address>\n",
#ifdef FULL_HISTORY
	" H <lines>            show history of PC\n",
#endif
#ifdef FULLMMU
	" u                    dump the MMU translation tables and state\n",
#endif
	NULL
};

void ndebug::ignore_ws (char **c) {
  while (**c && isspace(**c)) (*c)++;
}

void ndebug::reset_aktualrow()
{
	if (dbend < get_warnlen()) {
		aktualrow = dbfull ? (dbsize - (get_warnlen() - dbend)) : 0;
	} else {
		aktualrow = dbend - get_warnlen();
	}
}

void ndebug::set_aktualrow(signed int r)
{
	if (r == 0)
		return;
	if (r < 0) {
		r = -r;
		r %= dbsize;
		if (aktualrow < (unsigned int) r) {
			aktualrow = dbfull ? (dbsize - (r - aktualrow)) : 0;
		} else {
			aktualrow -= r;
		}
	} else {
		if ((aktualrow + r) < dbend) {
			aktualrow += r;
		} else {
			aktualrow =
				dbfull ? (dbsize - (dbend - (aktualrow + r))) : dbend;
		}
	}
}

#endif

#ifdef NEWDEBUG
int ndebug::dbprintf(char *s, ...)
{
#else
int dbprintf(char *s, ...)
{
#endif
	va_list a;
	va_start(a, s);
#ifdef NEWDEBUG
	if (bx_options.startup.debugger) {
		int i;
		if (dbbuffer[dbend] != NULL)
			free(dbbuffer[dbend]);
		i = vasprintf(&dbbuffer[dbend++], s, a);
		if (dbend == dbsize) dbend = 0;
		if (dbstart == dbend) dbstart++;
		if (dbstart == dbsize) dbstart = 0;
		reset_aktualrow();
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
#ifdef NEWDEBUG
	}
#endif
	va_end(a);
}

#ifdef NEWDEBUG
int ndebug::pdbprintf(char *s, ...)
{
#else
int pdbprintf(char *s, ...)
{
#endif
	int i = 0;
	va_list a;
	va_start(a, s);
#ifdef NEWDEBUG
	if (bx_options.startup.debugger) {
		if (dbbuffer[dbend] != NULL)
			free(dbbuffer[dbend]);
		i += vasprintf(&dbbuffer[dbend++], s, a);
		if (dbend == dbsize) dbend = 0;
		if (dbstart == dbend) dbstart++;
		if (dbstart == dbsize) dbstart = 0;
		reset_aktualrow();
	}
#endif
	char *c;
	i += vasprintf(&c, s, a);
	i += fprintf(stderr, c);
	i += fprintf(stderr, "\n");
	free(c);
	return i;
	va_end(a);
}


#ifdef NEWDEBUG
void ndebug::warn_print(FILE * f)
{
	unsigned int ar;
	char *s = (char *) malloc((rowlen + 1) * sizeof(char));
	for (unsigned int i = 0; i < get_warnlen(); i++) {
		ar = (aktualrow + i) % dbsize;
		if (dbbuffer[ar] != NULL
			&& (((dbstart <= dbend) && (ar >= dbstart) && (ar <= dbend))
			|| (dbstart > dbend) && (i < dbsize))) {
			snprintf(s, rowlen, dbbuffer[ar]);
			fprintf(f, s);
			for (unsigned int j = 0; j < (rowlen - strlen(s)); j++)
				putc(' ', f);
		} else {
			for (unsigned int j = 0; j < rowlen; j++)
				putc(' ', f);
		}
		fprintf(f, "|\n");
	}
	free(s);
}

void ndebug::m68k_print(FILE * f)
{
	if (regs.s == 0)
		regs.usp = m68k_areg(regs, 7);
	if (regs.s && regs.m)
		regs.msp = m68k_areg(regs, 7);
	if (regs.s && regs.m == 0)
		regs.isp = m68k_areg(regs, 7);
	fprintf(f, " A0: %08lx A4: %08lx   D0: %08lx D4: %08lx  USP=%08lx\n",
			(unsigned long) m68k_areg(regs, 0),
			(unsigned long) m68k_areg(regs, 4),
			(unsigned long) m68k_dreg(regs, 0),
			(unsigned long) m68k_dreg(regs, 4), (unsigned long) regs.usp);
	fprintf(f, " A1: %08lx A5: %08lx   D1: %08lx D5: %08lx  ISP=%08lx\n",
			(unsigned long) m68k_areg(regs, 1),
			(unsigned long) m68k_areg(regs, 5),
			(unsigned long) m68k_dreg(regs, 1),
			(unsigned long) m68k_dreg(regs, 5), (unsigned long) regs.isp);
	fprintf(f, " A2: %08lx A6: %08lx   D2: %08lx D6: %08lx  MSP=%08lx\n",
			(unsigned long) m68k_areg(regs, 2),
			(unsigned long) m68k_areg(regs, 6),
			(unsigned long) m68k_dreg(regs, 2),
			(unsigned long) m68k_dreg(regs, 6), (unsigned long) regs.msp);
	fprintf(f, " A3: %08lx A7: %08lx   D3: %08lx D7: %08lx  VBR=%08lx\n",
			(unsigned long) m68k_areg(regs, 3),
			(unsigned long) m68k_areg(regs, 7),
			(unsigned long) m68k_dreg(regs, 3),
			(unsigned long) m68k_dreg(regs, 7), (unsigned long) regs.vbr);
	fprintf(f, "T=%d%d S=%d M=%d X=%d N=%d Z=%d V=%d C=%d           TC=%04x\n",
	    regs.t1, regs.t0, regs.s, regs.m,
	    GET_XFLG, GET_NFLG, GET_ZFLG, GET_VFLG, GET_CFLG,
	    		   (unsigned int) ((((int) regs.mmu_enabled) << 15) |
				(((int) regs.mmu_pagesize) << 14)));

	fprintf(f, "CACR=%08lx DTT0=%08lx ITT0=%08lx SRP=%08lx  SFC=%d%d%d\n",
		   (unsigned long) regs.cacr, (unsigned long) regs.dtt0,
		   (unsigned long) regs.itt0, (unsigned long) regs.srp,
		   (int) (regs.sfc / 4), (int) ((regs.sfc & 2) / 2),
		   (int) (regs.sfc % 2));
	fprintf(f, "CAAR=%08lx DTT1=%08lx ITT1=%08lx URP=%08lx  DFC=%d%d%d\n",
		   (unsigned long) regs.caar, (unsigned long) regs.dtt1,
		   (unsigned long) regs.itt1, (unsigned long) regs.urp,
		   (int) (regs.dfc / 4), (int) ((regs.dfc & 2) / 2),
		   (int) (regs.dfc % 2));

    for (unsigned int i = 0; i < 8; i++) {
	fprintf(f, "FP%d: %Lg ", i, fpu.registers[i]);
	if (i == 3) fprintf(f, "N=%d Z=%d\n",
	    (fpu.fpsr.condition_codes & 0x8000000) != 0,
	    (fpu.fpsr.condition_codes & 0x4000000) != 0);
	if (i == 7) fprintf (f, "I=%d NAN=%d\n",
		(fpu.fpsr.condition_codes & 0x2000000) != 0,
		(fpu.fpsr.condition_codes & 0x1000000) != 0);
    }
}

void ndebug::instr_print(FILE * f)
{
	uaecptr nextpc;
	newm68k_disasm(f, m68k_getpc(), &nextpc, 1);
	fprintf(f, "next PC: %08lx\n", (unsigned long) nextpc);
}

void ndebug::show(FILE *f)
{
	warn_print(f);
	m68k_print(f);
	instr_print(f);
}

void ndebug::showHelp(FILE *f)
{
	int i = 0;
	while (strhelp[i] != NULL) {
		for (unsigned int j = 0; j < (get_len() - 2); j++) {
			if (strhelp[i] == NULL) break;
			fprintf(f, "%s",strhelp[i++]);
		}
		if (strhelp[i] != NULL) pressenkey(f);
	}
}

void ndebug::set_Ax(char **inl) {
	char reg;
	if (more_params(inl)) if ((reg = readhex(inl)) < 8)
		if (more_params(inl)) m68k_areg(regs, reg) = readhex(inl);
}

void ndebug::set_Dx(char **inl) {
	int reg;
	if (more_params(inl)) if ((reg = readhex(inl)) < 8)
		if (more_params(inl)) m68k_dreg(regs, reg) = readhex(inl);
}

void ndebug::set_Px(char **inl) {
	char reg;
	if (more_params(inl)) if ((reg = readhex(inl)) < 6)
		if (more_params(inl)) switch (reg) {
			case 0:
				regs.usp = readhex(inl);
				break;
			case 1:
				regs.isp = readhex(inl);
				break;
			case 2:
				regs.msp = readhex(inl);
				break;
			case 3:
				regs.vbr = readhex(inl);
				break;
			case 4:
				regs.intmask = readhex(inl);
				break;
			case 5: m68k_setpc((uaecptr)readhex(inl));
				break;
		}
}

void ndebug::set_Sx(char **inl) {
	char reg;
	unsigned int r;
	if (more_params(inl)) if ((reg = readhex(inl)) < 9)
		if (more_params(inl)) switch (reg) {
			case 0:
				r = readhex(inl);
#ifdef FULLMMU
				mmu_set_tc(r & 0xc000);
#endif
				break;
			case 1:
				regs.srp = readhex(inl);
				break;
			case 2:
				regs.urp = readhex(inl);
				break;
			case 3:
				regs.dtt0 = readhex(inl);
				break;
			case 4:
				regs.dtt1 = readhex(inl);
				break;
			case 5:
				regs.itt0 = readhex(inl);
				break;
			case 6:
				regs.itt1 = readhex(inl);
				break;
			case 7:
				regs.cacr = readhex(inl);
				break;
			case 8:
				regs.caar = readhex(inl);
				break;
		}
}

void ndebug::errorintofile(FILE *, char **inl) {
	char *name;
	FILE *fp;
	unsigned int ar;
	
	if (!more_params (inl)) {
		bug("L command needs more arguments!");
		return;
	}
	
	name = *inl;
	while (**inl != '\0' && !isspace (**inl)) (*inl)++;
	**inl = '\0';

	fp = fopen (name, "w");
	if (fp == NULL) {
		bug("Couldn't open file");
		return;
	}

        for (unsigned int i = 0; i < dbsize; i++) {
		ar = (dbstart + i) % dbsize;
		if (dbbuffer[ar] != NULL
			&& (((dbstart <= dbend) && (ar >= dbstart) && (ar <= dbend))
			|| (dbstart > dbend) && (i < dbsize))) {
			fprintf(fp, "%s\n", dbbuffer[ar]);
		}
	}
								
	fclose (fp);
	return;
}



void ndebug::saveintofile(FILE *, char **inl) {
	volatile uae_u32 src, len;
	char *name;
	FILE *fp;

	if (!more_params (inl)) goto S_argh;

	name = *inl;
	while (**inl != '\0' && !isspace (**inl)) (*inl)++;
	if (!isspace (**inl)) goto S_argh;

	**inl = '\0';
	(*inl)++;
	if (!more_params (inl)) goto S_argh;
	src = readhex (inl);
	if (!more_params (inl)) goto S_argh;
	len = readhex (inl);

	if (!ValidAtariAddr(src, false, len)) {
		bug("Invalid memory address\n");
		return;
	}

	fp = fopen (name, "w");
	if (fp == NULL) {
		bug("Couldn't open file");
		return;
	}

	for(unsigned int i = 0; i < len; i++) 
		if (fputc(ReadAtariInt8(src++), fp) == EOF) {
			bug("Error writing file");
			break;
		}

	fclose (fp);
	return;

S_argh:
	bug("s command needs more arguments!");
}

void ndebug::loadintomemory(FILE *, char **inl) {
	uae_u32 src;
	char *name;
	FILE *fp;
	int c;

	if (!more_params (inl)) goto S_argh;

	name = *inl;
	while (**inl != '\0' && !isspace (**inl)) (*inl)++;
	if (!isspace (**inl)) goto S_argh;

	**inl = '\0';
	(*inl)++;
	if (!more_params (inl)) goto S_argh;
	src = readhex (inl);

	fp = fopen (name, "r");
	if (fp == NULL) {
		bug("Couldn't open file");
		return;
	}

	while ((c = fgetc(fp)) != EOF) WriteAtariInt8(src++, (char)c)
		;

	fclose (fp);
	return;

S_argh:
	bug("l command needs more arguments!");
}

void ndebug::writeintomem(FILE *, char **c) {
	uae_u32 addr = 0;
	uae_u32 val = 0;
	char nc;

	ignore_ws(c);
	while (isxdigit(nc = **c)) {
		(*c)++;
		addr *= 16;
		nc = toupper(nc);
		if (isdigit(nc)) {
		    addr += nc - '0';
		} else {
		    addr += nc - 'A' + 10;
		}
	}
	ignore_ws(c);
	while (isxdigit(nc = **c)) {
		(*c)++;
		val *= 10;
		nc = toupper(nc);
		if (isdigit(nc)) {
		    val += nc - '0';
		}
	}
	ignore_ws(c);
	addr = addr < 0xff000000 ? addr : addr & 0x00ffffff;
	if (addr < (RAMSize + ROMSize + FastRAMSize)) {
		if ((addr & 0xfff00000) == 0x00f00000) {
			switch (**c) {
				case 'b':
					val &= 0x000000ff;
					HWput_b(addr, val);
					break;
				case 'w':
					val &= 0x0000ffff;
					HWput_w(addr, val);
					break;
				default:
					HWput_l(addr, val);
			}
		} else {
			WriteAtariInt32(addr, val);
		}
		bug("Wrote %d at %08x", val ,addr);
	} else {
		bug("Invalid address %08x", addr);
	}
}

void ndebug::backtrace(FILE *f, volatile unsigned int lines) {
	volatile uaecptr st = m68k_areg(regs,7);
	while (lines-- > 0) {
		if (valid_address(st, 0, 0, sz_long))
			fprintf (f, "%08lx: %08x\n", (unsigned long)st, ReadAtariInt32(st));
		else
			fprintf (f, " unknown address\n");
		st += 2;
	}
}

void ndebug::log2phys(FILE *, uaecptr addr) {
#ifdef FULLMMU
	if (regs.mmu_enabled) {
setjmpagain:
		JMP_BUF excep_env_old;
		memcpy(excep_env_old, excep_env, sizeof(JMP_BUF));
	        int prb = SETJMP(excep_env);
        	if (prb == 2) {
			bug("Bus error");
	                goto setjmpagain;
        	} if (prb == 3) {
			bug("Access error");
			goto setjmpagain;
		} if (prb != 0) {
			bug("Unknonw access error - %d", prb);
			goto setjmpagain;
		}
		bug("MMU enabled: %08lx -> %08lx",	(unsigned long)addr,
			(unsigned long)mmu_translate(addr, FC_DATA, 0, m68k_getpc(), sz_long, 0));
		memcpy(excep_env, excep_env_old, sizeof(JMP_BUF));
	} else {
#endif /* FULLMMU */
		bug("MMU disabled: %08lx", (unsigned long)addr);
#ifdef FULLMMU
	}
#endif
}

unsigned int ndebug::get_len() {
// Derived from pine 4.21 - termout.unx
     struct winsize win;
     if(ioctl(1, TIOCGWINSZ, &win) >= 0			/* 1 is stdout */
	|| ioctl(0, TIOCGWINSZ, &win) >= 0){		/* 0 is stdin */
	if (win.ws_row == 0) {
          fprintf(stderr, "ioctl(TIOCWINSZ) failed\n");
	  exit(-1);
	}
    } else {
      fprintf(stderr, "ioctl(TIOCWINSZ) failed\n");
      exit(-1);
    }
    return win.ws_row;
}

void ndebug::showTypes() {
	bug("States of debugger:");
	bug("0:        classical canonical debugger (like UAE)");
	bug("1:        direct (icanonical) debugger");
	bug("2:        memory browser");
	bug("3:        MMU TT browser");
}

int ndebug::canon(FILE *f, bool wasGrabbed, uaecptr nextpc, uaecptr &nxdis, uaecptr &nxmem) {
	char input[80];
	char cmd, *inptr;
	int count;
	uae_u32 daddr;
	uae_u32 maddr;

	show(f);

	fprintf(f, ">");
	fflush(f);
	if (fgets(input, 80, stdin) == NULL) {
		if (wasGrabbed) grabMouse(true);	// lock keyboard and mouse
		return 0;
	}
	if (input[0] == '\n') strcpy(input, old_debug_cmd);
	inptr = input;
	strcpy(old_debug_cmd, input);
	cmd = next_char(&inptr);
	switch (cmd) {
		case '?':
		case 'h':
			showHelp(f);
			pressenkey(f);
			break;
		case 'Q':
			fprintf(f, "Really quit?\n");
			if (fgets(input, 80, stdin) == NULL)
				QuitEmulator();
			if (input[0] == 'y' || input[0] == 'Y')
				QuitEmulator();
			break;
		case 'R':
			m68k_reset();
			break;
		case 'i':
			irqindebug = !irqindebug; bug("IRQ %s\n", irqindebug ? "enabled" : "disabled"); break;
			break;
		case 'I':
			ignore_irq = !ignore_irq; bug("IRQ debugging %s\n", ignore_irq ? "enabled" : "disabled"); break;
			break;
		case 'e':
			dump_traps (f);
			pressenkey(f);
			break;
		case 'E':
			dump_ints (f);
			pressenkey(f);
			break;
		case 'A':
			set_Ax(&inptr);
			break;
		case 'D':
			set_Dx(&inptr);
			break;
		case 'P':
			set_Px(&inptr);
			break;
		case 'S':
			set_Sx(&inptr);
			break;
		case 'g':
			if (more_params(&inptr)) m68k_setpc(readhex(&inptr));
			fill_prefetch_0();
			if (wasGrabbed) grabMouse(true);
		//	deactivate_debugger();
			if (do_breakpoints)
				SPCFLAGS_SET( SPCFLAG_BRK );
			return 0;
		case 't':
			if (more_params(&inptr)) m68k_setpc(readhex(&inptr));
			SPCFLAGS_SET( SPCFLAG_BRK );
			return 0;
		case 'd':
			if (more_params(&inptr)) daddr = readhex(&inptr);
				else daddr = nxdis;
			if (more_params(&inptr)) count = readhex(&inptr);
				else count = get_len() - 3;
			newm68k_disasm(f, daddr, &nxdis, count);
			pressenkey(f);
			break;
		case 'm':
			if (more_params(&inptr)) maddr = readhex(&inptr);
				else maddr = nxmem;
			if (more_params(&inptr)) count = readhex(&inptr);
				else count = (get_len() - 2) / 2;
			dumpmem(f, maddr, &nxmem, count);
			pressenkey(f);
			break;
		case 'b':
			if (more_params(&inptr)) count = readhex(&inptr);
				else count = get_len() - 3;
			backtrace(f, count);
			pressenkey(f);
			break;
		case 'T':
			if (more_params(&inptr)) maddr = readhex(&inptr);
				else maddr = nxmem;
			log2phys(f, maddr);
			break;
		case 's':
			saveintofile(f, &inptr);
			break;
		case 'l':
			loadintomemory(f, &inptr);
			break;
		case 'W':
			writeintomem(f, &inptr);
			break;
		case 'N':
			convertNo(&inptr);
			break;
		case 'C':
			showTypes();
			break;
		case 'c':
			if (more_params(&inptr)) tp = readhex(&inptr);
				else bug("c command needs one argument!");
			break;
		case 'j':
			set_aktualrow(-1);
			break;
		case 'J':
			set_aktualrow(1 - get_warnlen());
			break;
		case 'k':
			set_aktualrow(1);
			break;
		case 'K':
			set_aktualrow(get_warnlen() - 1);
			break;
		case 'r':
			reset_aktualrow();
			break;
		case 'z':
			skipaddr = nextpc;
			do_skip = true;
			irqindebug = true;
			SPCFLAGS_SET( SPCFLAG_BRK );
			if (wasGrabbed) grabMouse(true);
			return 0;
		case 'f':
			if (!more_params(&inptr)) {
				bug("f command needs one parameter!");
				break;
			}
			skipaddr = readhex(&inptr);
			do_skip = true;
			irqindebug = true;
			SPCFLAGS_SET( SPCFLAG_BRK );
			if (wasGrabbed) grabMouse(true);
			return 0;
		case 'B':
			{
				unsigned int i;
				uaecptr address;
				if (!more_params(&inptr)) {
					bug("f command needs one parameter!");
					break;
				}
				address = readhex(&inptr);
				for (i = 0; i < max_breakpoints; i++)
					if ((breakpoint[i]) && (address == breakpoint_address[i]))
						break;
				if (i < max_breakpoints) {
					breakpoint[i] = false;
					do_breakpoints--;
					bug("Breakpoint on %08x removed", address);
					break;
				}
				for (i = 0; i <max_breakpoints; i++)
					if (!breakpoint[i])
						break;
				if (i == max_breakpoints) {
					bug("Maximum breakpoints reached - %d", max_breakpoints);
					break;
				}
				breakpoint_address[i] = address;
				breakpoint[i] = true;
				do_breakpoints++;
				bug("Breakpoint on %08x added", address);
			}
			break;
		case 'v':
			if (!more_params(&inptr)) {
				bug("v command needs one parameter!");
				break;
			}
			switch (inptr[0]) {
				case 'b':
					inptr += 2;
					value_test = EQUAL_value_test_8;
					value_addr = readhex(&inptr);
					value = (uint8)readhex(&inptr);
					do_skip_value = true;
					irqindebug = true;
					SPCFLAGS_SET( SPCFLAG_BRK );
					if (wasGrabbed) grabMouse(true);
					return 0;
				case 'w':
					inptr += 2;
					value_test = EQUAL_value_test_16;
					value_addr = readhex(&inptr);
					value = (uint16)readhex(&inptr);
					do_skip_value = true;
					irqindebug = true;
					SPCFLAGS_SET( SPCFLAG_BRK );
					if (wasGrabbed) grabMouse(true);
					return 0;
				case 'l':
					inptr += 2;
					value_test = EQUAL_value_test_32;
					value_addr = readhex(&inptr);
					value = readhex(&inptr);
					do_skip_value = true;
					irqindebug = true;
					SPCFLAGS_SET( SPCFLAG_BRK );
					if (wasGrabbed) grabMouse(true);
					return 0;
				default:
					bug("v command needs for the 1st parameter b, w or l!");
					break;
			}
			break;
		case 'V':
			if (!more_params(&inptr)) {
				bug("V command needs one parameter!");
				break;
			}
			switch (inptr[0]) {
				case 'b':
					inptr += 2;
					value_test = CHANGE_value_test_8;
					value_addr = readhex(&inptr);
					value = ReadAtariInt8(value_addr);
					do_skip_value = true;
					irqindebug = true;
					SPCFLAGS_SET( SPCFLAG_BRK );
					if (wasGrabbed) grabMouse(true);
					return 0;
				case 'w':
					inptr += 2;
					value_test = CHANGE_value_test_16;
					value_addr = readhex(&inptr);
					value = ReadAtariInt16(value_addr);
					do_skip_value = true;
					irqindebug = true;
					SPCFLAGS_SET( SPCFLAG_BRK );
					if (wasGrabbed) grabMouse(true);
					return 0;
				case 'l':
					inptr += 2;
					value_test = CHANGE_value_test_32;
					value_addr = readhex(&inptr);
					value = ReadAtariInt32(value_addr);
					do_skip_value = true;
					irqindebug = true;
					SPCFLAGS_SET( SPCFLAG_BRK );
					if (wasGrabbed) grabMouse(true);
					return 0;
				default:
					bug("V command needs for the 1st parameter b, w or l!");
					break;
			}
			break;
		case 'L':
			errorintofile(f, &inptr);
			break;
#ifdef FULL_HISTORY
		case 'H':
			if (!more_params(&inptr)) count = 10;
				else count = readhex(&inptr);
			showHistory(count);
			break;
#endif
#ifdef FULLMMU
		case 'u':
			mmu_dump_tables();
			break;
#endif
		case 'q':
			tp = 0;
			break;
	}
	if (do_breakpoints)
		SPCFLAGS_SET( SPCFLAG_BRK );
	return 1;
}

int ndebug::icanon(FILE *f, bool, uaecptr, uaecptr &nxdis, uaecptr &nxmem) {
	struct termios newtty;
	char buffer[1];
	int count;
	uae_u32 daddr;
	uae_u32 maddr;

	show(f);
	fprintf(f, "|");
	fflush(f);
	newtty = savetty;
	newtty.c_lflag &= ~ICANON;
	newtty.c_lflag &= ~ECHO;
	newtty.c_cc[VMIN] = 1;
	newtty.c_cc[VTIME] = 1;
	if (tcsetattr(0, TCSAFLUSH, &newtty) == -1) {
		fprintf(stderr, "tcsetattr error\n");
		exit(-1);
	}
	for (;;) {
		if (read(0, buffer, sizeof(buffer))) {
			switch (buffer[0]) {
				case 't':
					SPCFLAGS_SET( SPCFLAG_BRK );
					fprintf(stderr, "\n");
					tcsetattr(0, TCSAFLUSH, &savetty);
					fflush(stderr);
					return 0;
				case 'd':
					daddr = nxdis;
					count = get_len() - 3;
					fprintf(stderr, "\n");
					tcsetattr(0, TCSAFLUSH, &savetty);
					fflush(stderr);
					newm68k_disasm(f, daddr, &nxdis, count);
					tcsetattr(0, TCSAFLUSH, &newtty);
					read(0, buffer, sizeof(buffer));
					break;
				case 'm':
					maddr = nxmem;
					count = (get_len() - 2) / 2;
					fprintf(stderr, "\n");
					tcsetattr(0, TCSAFLUSH, &savetty);
					fflush(stderr);
					dumpmem(f, maddr, &nxmem, count);
					tcsetattr(0, TCSAFLUSH, &newtty);
					read(0, buffer, sizeof(buffer));
					break;
				case 'T':
					maddr = nxmem;
					log2phys(f, maddr);
					break;
				case 'j':
					set_aktualrow(-1);
					break;
				case 'J':
					set_aktualrow(1 - get_warnlen());
					break;
				case 'k':
					set_aktualrow(1);
					break;
				case 'K':
					set_aktualrow(get_warnlen() - 1);
					break;
				case 'r':
					reset_aktualrow();
					break;
				case 'q':
					tp = 0;
					break;
			}
			break;
		}
	}
	
	fprintf(stderr, "\n");
	tcsetattr(0, TCSAFLUSH, &savetty);
	fflush(stderr);
	return 1;
}

int ndebug::dm(FILE *f, bool, uaecptr, uaecptr &, uaecptr &nxmem) {
	struct termios newtty;
	char buffer[1];
	int count;
	uae_u32 maddr;

	newtty = savetty;
	newtty.c_lflag &= ~ICANON;
	newtty.c_lflag &= ~ECHO;
	newtty.c_cc[VMIN] = 1;
	newtty.c_cc[VTIME] = 1;
	if (tcsetattr(0, TCSAFLUSH, &newtty) == -1) {
		fprintf(stderr, "tcsetattr error\n");
		exit(-1);
	}
	for (;;) {
		maddr = nxmem;
		count = (get_len() - 2) / 2;
		fprintf(stderr, "\n");
		tcsetattr(0, TCSAFLUSH, &savetty);
		fflush(stderr);
		dumpmem(f, maddr, &nxmem, count);
		tcsetattr(0, TCSAFLUSH, &newtty);
		if (read(0, buffer, sizeof(buffer)) > 0) {
			switch (buffer[0]) {
				case 't':
					nxmem = maddr;
					SPCFLAGS_SET( SPCFLAG_BRK );
					fprintf(stderr, "\n");
					tcsetattr(0, TCSAFLUSH, &savetty);
					fflush(stderr);
					return 0;
				case 'T':
					maddr = nxmem;
					log2phys(f, maddr);
					break;
				case 'j':
					nxmem = maddr - 32;
					if (nxmem > RAMSize + ROMSize + FastRAMSize) nxmem = 0;
					break;
				case 'J':
					nxmem = maddr + 32 - 32 * count;
					if (nxmem > RAMSize + ROMSize + FastRAMSize) nxmem = 0;
					break;
				case 'k':
					nxmem = maddr + 32;
					if (nxmem > RAMSize + ROMSize + FastRAMSize) nxmem = RAMSize + ROMSize + FastRAMSize;
					break;
				case 'K':
					nxmem = maddr + 32 * count - 32;
					if (nxmem > RAMSize + ROMSize + FastRAMSize) nxmem = RAMSize + ROMSize + FastRAMSize;
					break;
				case 'q':
					tp = 0;
					break;
				default:
					nxmem = maddr;
					break;
			}
			break;
		}
	}
	
	fprintf(stderr, "\n");
	tcsetattr(0, TCSAFLUSH, &savetty);
	fflush(stderr);
	return 1;
}

void ndebug::run() {
	uaecptr nextpc, nxdis, nxmem;
	newm68k_disasm(stderr, m68k_getpc(), &nextpc, 0);
	nxdis = nextpc;
	nxmem = 0;

	if (do_skip || do_skip_value || do_breakpoints) {
		bool success = false;

		if (do_skip && (m68k_getpc() == skipaddr)) {
			do_skip = false;
			success = true;
		}

		if (do_skip_value) {
			switch (value_test) {
				case EQUAL_value_test_8:
					if (ReadAtariInt8(value_addr) == value) {
						do_skip_value = false;
						success = true;
					}
					break;
				case EQUAL_value_test_16:
					if (ReadAtariInt16(value_addr) == value){
						do_skip_value = false;
						success = true;
					}
					break;
				case EQUAL_value_test_32:
					if (ReadAtariInt32(value_addr) == value){
						do_skip_value = false;
						success = true;
					}
					break;
				case CHANGE_value_test_8:
					if (ReadAtariInt8(value_addr) != value) {
						do_skip_value = false;
						success = true;
					}
					break;
				case CHANGE_value_test_16:
					if (ReadAtariInt16(value_addr) != value){
						do_skip_value = false;
						success = true;
					}
					break;
				case CHANGE_value_test_32:
					if (ReadAtariInt32(value_addr) != value){
						do_skip_value = false;
						success = true;
					}
					break;
			}
		}

		if (do_breakpoints) {
			unsigned int i;
			for (i = 0; i < max_breakpoints; i++)
				if ((breakpoint[i]) && (breakpoint_address[i] == m68k_getpc()))
					break;
			if (i < max_breakpoints)
				success = true;
		}

		if (!success) {
			SPCFLAGS_SET( SPCFLAG_BRK );
			return;
		}
	}

	irqindebug = false;
	// release keyboard and mouse control
	bool wasGrabbed = grabMouse(false);

	for (;;) {
		switch(tp) {
			default:
			case 0:
				if (canon(stderr, wasGrabbed, nextpc, nxdis, nxmem) == 0) return;
				break;
			case 1:
				if (icanon(stderr, wasGrabbed, nextpc, nxdis, nxmem) == 0) return;
				break;
			case 2:
				if (dm(stderr, wasGrabbed, nextpc, nxdis, nxmem) == 0) return;
				break;
		}	
	}
}

void ndebug::init()
{
	if ((dbbuffer = (char **) malloc(dbsize * sizeof(char *))) == NULL) {
		fprintf(stderr, "Not enough memory!");
		exit(-1);
	}
	for (unsigned int i = 0; i < dbsize; i++)
		dbbuffer[i] = NULL;
	for (unsigned int i = 0; i < max_breakpoints; i++)
		breakpoint[i] = false;
	if (tcgetattr(0, &savetty) == -1) {
		fprintf(stderr, "tcgetattr error!\n");
		exit(-1);
	}
}

void ndebug::nexit() {
	tcsetattr(0, TCSAFLUSH, &savetty);
}

void ndebug::dumpmem(FILE *f, volatile uaecptr addr, uaecptr * nxmem, volatile unsigned int lns)
{
	JMP_BUF excep_env_old;
	memcpy(excep_env_old, excep_env, sizeof(JMP_BUF));
	uaecptr a;
	broken_in = 0;
	for (; lns-- && !broken_in;) {
		volatile int i;
		fprintf(f, "%08lx: \n", (unsigned long)addr);
		for (i = 0; i < 16; i++) {
setjmpagain:
        		if (SETJMP(excep_env) != 0) {
				fprintf (f, "UA   ");
				addr += 2;
				if (++i == 16) break;
		                goto setjmpagain;
		        }
			a = addr < 0xff000000 ? addr : addr & 0x00ffffff;
			if ((addr & 0xfff00000) == 0x00f00000) {
				fprintf(f, "%04x ", (unsigned int) HWget_w(a));
			}
			else {
				fprintf(f, "%04x ", (unsigned int) ReadAtariInt16(a));
			}
			addr += 2;
		}
		fprintf(f, "\n");
	}
	*nxmem = addr;
	memcpy(excep_env, excep_env_old, sizeof(JMP_BUF));
}

uae_u32 ndebug::readhex (char v, char **c)
{
    char nc;
    uae_u32 val;
    if (isxdigit(v)) {
	v = toupper(v);
	if (isdigit(v)) {
	    val = v - '0';
	} else {
	    val = v - 'A' + 10;
	}
    } else return 0;

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

uae_u32 ndebug::readoct (char v, char **c)
{
    char nc;
    uae_u32 val;
    if (isdigit(v) && (v < '8')) val = v - '0'; else return 0;

    while (isdigit(nc = **c) && (nc < '8')) {
	(*c)++;
	val *= 8;
	val += nc - '0';
    }
    return val;
}

uae_u32 ndebug::readbin (char v, char **c)
{
    char nc;
    uae_u32 val;
    if (isdigit(v) && (v < '2')) val = v - '0'; else return 0;

    while (isdigit(nc = **c) && (nc < '2')) {
	(*c)++;
	val *= 2;
	val += nc - '0';
    }
    return val;
}

uae_u32 ndebug::readdec (char v, char **c)
{
    char nc;
    uae_u32 val;
    if (isdigit(v)) val = v - '0'; else return 0;

    while (isdigit(nc = **c)) {
	(*c)++;
	val *= 10;
	val += nc - '0';
    }
    return val;
}

char *ndebug::dectobin (uae_u32 val)
{
	char *s = (char *)malloc(sizeof(char));
	if (s == NULL) {
		fprintf(stderr, "Not enough memory!");
		exit(-1);
	}

	while (val)
	{
		char *ps = strdup(s);
		if (ps == NULL) {
			fprintf(stderr, "Not enough memory!");
			exit(-1);
		}
		free(s);
		s = (char *)malloc((strlen(ps) + 2) * sizeof(char));
		if (s == NULL) {
			fprintf(stderr, "Not enough memory!");
			exit(-1);
		}
		s[0] = val % 2 + '0';
		val /= 2;
		strcpy(s + 1, ps);
	}
	return s;
}

void ndebug::convertNo(char **s) {
	char c;
	uae_u32 val = 0;
	ignore_ws(s);
	switch (c = next_char(s)) {
		case '0':
			if ((c = next_char(s)) == 'x')
				val = readhex(next_char(s), s);
			else val = readoct(c, s);
			break;
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			val = readdec(c, s);
			break;
		default: bug("N command needs one argument!");
			 return;
	}
	bug("%d 0x%X 0%o %s", val, val, val, dectobin(val));
}

#ifdef FULL_HISTORY
void ndebug::showHistory(unsigned int count) {
	unsigned int temp = lasthist;
	
	while (count-- > 0 && temp != firsthist) {
	    if (temp == 0) temp = MAX_HIST-1; else temp--;
	}
	bug("History:");
	while (temp != lasthist) {
#ifndef NEED_TO_DEBUG_BADLY
		showDisasm(history[temp]);
#endif
		//bug("%04x", history[temp]);
		if (++temp == MAX_HIST) temp = 0;
	}
}
#endif

#endif

/*
 * $Log$
 * Revision 1.34  2004/07/09 18:41:56  milan
 * (sig)set/longjmp corrections
 *
 * Revision 1.33  2003/12/27 21:02:40  joy
 * warning fixed
 *
 * Revision 1.32  2003/09/03 20:15:55  milan
 * ndebug compilation warnings fixed
 *
 * Revision 1.31  2003/07/11 18:51:03  milan
 * some JIT bugs corrected
 * CD-ROM product ID corrected for little endian
 *
 * Revision 1.30  2002/10/15 21:26:53  milan
 * non-cheaders support (for MipsPro C/C++ compiler)
 *
 * Revision 1.29  2002/10/02 22:04:32  milan
 * ndebug supports breakpoints
 *
 * Revision 1.28  2002/09/30 21:57:33  milan
 * v&V commands added to ndebug, (<address>) vs. <value> operations
 * typo fix in configure.ac script
 *
 * Revision 1.27  2002/09/28 13:03:45  joy
 * ndebug::showHistory does not work if NEED_TO_DEBUG_BADLY is enabled
 *
 * Revision 1.26  2002/09/10 18:44:52  milan
 * ISO C++ forbids assignment of arrays
 *
 * Revision 1.25  2002/08/01 22:21:47  joy
 * compiler warning fixed
 *
 * Revision 1.24  2002/07/23 08:43:50  pmandin
 * gcc-3 friendliness
 *
 * Revision 1.23  2002/04/22 18:30:50  milan
 * header files reform
 *
 * Revision 1.22  2002/02/26 21:08:13  milan
 * address validation in CPU interface
 *
 * Revision 1.21  2002/02/25 23:52:13  milan
 * PC, stack, table68k - synchronized with MMU (Furlong) and JIT (Gwenole)
 *
 * Revision 1.17  2002/02/20 13:04:07  milan
 * Makefile's dependencies disabled - problems with make depend
 * valid_address
 *
 * Revision 1.16  2002/02/19 20:04:05  milan
 * src/ <-> CPU interaction cleaned
 * memory access cleaned
 *
 * Revision 1.15  2002/02/14 16:53:45  milan
 * new command for ndebug - 'L'
 *
 * Revision 1.14  2002/02/01 20:19:38  milan
 * ioctl -> tcgetattr, tcsetattr
 *
 * Revision 1.13  2002/01/31 19:37:47  milan
 * panicbug, cleaning
 *
 * Revision 1.12  2002/01/18 23:43:25  milan
 * fgetc returns int
 * abs needs stdlib.h
 *
 * Revision 1.11  2002/01/18 20:37:47  milan
 * FixedSizeFastRAM & Makefile fixed
 *
 * Revision 1.10  2002/01/08 16:13:18  joy
 * config variables moved from global ones to bx_options struct.
 *
 * Revision 1.9  2001/12/07 00:31:31  milan
 * small bug in 'z' command of ndebug corrected, big one stays, it needs better solution
 *
 * Revision 1.8  2001/11/28 09:36:05  milan
 * cleaning
 * update of config.* file
 *
 * Revision 1.7  2001/11/21 13:29:51  milan
 * cleanning & portability
 *
 * Revision 1.6  2001/11/06 20:36:54  milan
 * MMU's corrections
 *
 * Revision 1.5  2001/10/29 08:15:45  milan
 * some changes around debuggers
 *
 * Revision 1.4  2001/10/25 22:33:18  milan
 * fullhistory's support in ndebug
 *
 * Revision 1.3  2001/10/03 11:10:03  milan
 * Px supports now P5 <addr>, PC=<addr>
 *
 * Revision 1.2  2001/10/02 19:13:28  milan
 * ndebug, malloc
 *
 * Revision 1.1  2001/07/21 18:13:29  milan
 * sclerosis, sorry, ndebug added
 *
 *
 *
 */
