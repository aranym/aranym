/* 2001 MJ */

 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Debugger
  *
  * (c) 1995 Bernd Schmidt
  *
  */

#include "sysdeps.h"

#include <ctype.h>
#include <signal.h>

#include "config.h"
#include "penguin.h"
#include "memory.h"
#include "newcpu.h"
#include "debug.h"

#include "main.h"
#include "cpu_emulation.h"

static int debugger_active = 0;
static uaecptr skipaddr;
static int do_skip;
int debugging = 0;
int irqindebug = false;

static char old_debug_cmd[80];

// MJ static FILE *logfile;

void activate_debugger (void)
{
    do_skip = 0;
    if (debugger_active)
	return;
    debugger_active = 1;
    set_special (SPCFLAG_BRK);
    debugging = 1;
    /* use_debugger = 1; */
}

int firsthist = 0;
int lasthist = 0;
#ifdef NEED_TO_DEBUG_BADLY
struct regstruct history[MAX_HIST];
union flagu historyf[MAX_HIST];
#else
uaecptr history[MAX_HIST];
#endif

static void ignore_ws (char **c)
{
    while (**c && isspace(**c)) (*c)++;
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

static char next_char( char **c)
{
    ignore_ws (c);
    return *(*c)++;
}

static int more_params (char **c)
{
    ignore_ws (c);
    return (**c) != 0;
}

static void dumpmem (uaecptr addr, uaecptr *nxmem, int lines)
{
    broken_in = 0;
    for (;lines-- && !broken_in;) {
	int i;
	printf ("%08lx ", addr);
	for (i = 0; i < 16; i++) {
	    printf ("%04x ", get_word(addr, true)); addr += 2;
	}
	printf ("\n");
    }
    *nxmem = addr;
}

static void writeintomem (char **c)
{
    uae_u8 *p = get_real_address (0, true, false);
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

    if (addr < RAMSize) {
      p[addr] = val>>24 & 0xff;
      p[addr+1] = val>>16 & 0xff;
      p[addr+2] = val>>8 & 0xff;
      p[addr+3] = val & 0xff;
      printf("Wrote %d at %08x\n",val,addr);
    } else
      printf("Invalid address %08x\n",addr);
}

static int trace_same_insn_count;
static uae_u8 trace_insn_copy[10];
static struct regstruct trace_prev_regs;
void debug (void)
{
    char input[80];
    uaecptr nextpc,nxdis,nxmem;

    input[0] = '\n';
    input[1] = '0';
    
    if (do_skip && skipaddr == 0xC0DEDBAD) {
#if 0
	if (trace_same_insn_count > 0) {
	    if (memcmp (trace_insn_copy, regs.pc_p, 10) == 0
		&& memcmp (trace_prev_regs.regs, regs.regs, sizeof regs.regs) == 0)
	    {
		trace_same_insn_count++;
		return;
	    }
	}
	if (trace_same_insn_count > 1)
	    fprintf (logfile, "[ repeated %d times ]\n", trace_same_insn_count);
#endif
	m68k_dumpstate (&nextpc);
	trace_same_insn_count = 1;
	memcpy (trace_insn_copy, do_get_real_address(regs.pcp, true, false), 10);
	memcpy (&trace_prev_regs, &regs, sizeof regs);
    }

    if (do_skip && (m68k_getpc() != skipaddr/* || regs.a[0] != 0x1e558*/)) {
	set_special (SPCFLAG_BRK);
	return;
    }
    do_skip = 0;

    irqindebug = false;

#ifdef NEED_TO_DEBUG_BADLY
    history[lasthist] = regs;
    historyf[lasthist] = regflags;
#else
    history[lasthist] = m68k_getpc();
#endif
    if (++lasthist == MAX_HIST) lasthist = 0;
    if (lasthist == firsthist) {
	if (++firsthist == MAX_HIST) firsthist = 0;
    }

    m68k_dumpstate (&nextpc);
    nxdis = nextpc; nxmem = 0;

    for (;;) {
	char cmd, *inptr;

	printf (">");
	fflush (stdout);
	if (fgets (input, 80, stdin) == 0)
	    return;
	if (input[0] == '\n') memcpy(input, old_debug_cmd, 80);
	inptr = input;
	memcpy (old_debug_cmd, input, 80);
	cmd = next_char (&inptr);
	switch (cmd) {
	 case 'i': irqindebug = true; break;
	 case 'I': irqindebug = false; break;
	 case 'r': m68k_dumpstate (&nextpc); break;
	 case 'R': m68k_reset(); break;
	 case 'A':
	    {
	        char reg;
    	        if (more_params (&inptr))
		    if ((reg = readhex(&inptr)) < 8)
		        if (more_params (&inptr))
		            m68k_areg (regs, reg) = readhex(&inptr);
		break;
	    }
	 case 'D':
	    {
	        char reg;
    	        if (more_params (&inptr))
		    if ((reg = readhex(&inptr)) < 8)
		        if (more_params (&inptr))
		            m68k_dreg (regs, reg) = readhex(&inptr);
		break;
	    }
	 case 'W': writeintomem (&inptr); break;
	 case 'S':
	    {
		uae_u8 *memp;
		uae_u32 src, len;
		char *name;
		FILE *fp;

		if (!more_params (&inptr))
		    goto S_argh;

		name = inptr;
		while (*inptr != '\0' && !isspace (*inptr))
		    inptr++;
		if (!isspace (*inptr))
		    goto S_argh;

		*inptr = '\0';
		inptr++;
		if (!more_params (&inptr))
		    goto S_argh;
		src = readhex (&inptr);
		if (!more_params (&inptr))
		    goto S_argh;
		len = readhex (&inptr);
		if (! valid_address (src, len)) {
		    printf ("Invalid memory block\n");
		    break;
		}
		memp = get_real_address (src, true, false);
		fp = fopen (name, "w");
		if (fp == NULL) {
		    printf ("Couldn't open file\n");
		    break;
		}
		if (fwrite (memp, 1, len, fp) != len) {
		    printf ("Error writing file\n");
		}
		fclose (fp);
		break;

		S_argh:
		printf ("S command needs more arguments!\n");
		break;
	    }
	 case 'd':
	    {
		uae_u32 daddr;
		int count;

		if (more_params(&inptr))
		    daddr = readhex(&inptr);
		else
		    daddr = nxdis;
		if (more_params(&inptr))
		    count = readhex(&inptr);
		else
		    count = 10;
		m68k_disasm (daddr, &nxdis, count);
	    }
	    break;
	 case 't': 
	    if (more_params (&inptr))
		m68k_setpc (readhex (&inptr));
	    set_special (SPCFLAG_BRK); return;
	 case 'z':
	    skipaddr = nextpc;
	    do_skip = 1;
	    irqindebug = true;
	    set_special (SPCFLAG_BRK);
	    return;

	 case 'f':
	    skipaddr = readhex (&inptr);
	    do_skip = 1;
	    irqindebug = true;
	    set_special (SPCFLAG_BRK);
	    if (skipaddr == 0xC0DEDBAD) {
	        trace_same_insn_count = 0;
		memcpy (trace_insn_copy, do_get_real_address(regs.pcp, true, false), 10);
		memcpy (&trace_prev_regs, &regs, sizeof regs);
	    }
	    return;

	 case 'q': QuitEmulator();
	    debugger_active = 0;
	    debugging = 0;
	    return;

	 case 'g':
	    if (more_params (&inptr))
		m68k_setpc (readhex (&inptr));
	    fill_prefetch_0 ();
	    debugger_active = 0;
	    debugging = 0;
	    return;

	 case 'H':
	    {
		int count;
		int temp;
#ifdef NEED_TO_DEBUG_BADLY
		struct regstruct save_regs = regs;
		union flagu save_flags = regflags;
#endif

		if (more_params(&inptr))
		    count = readhex(&inptr);
		else
		    count = 10;
		if (count < 0)
		    break;
		temp = lasthist;
		while (count-- > 0 && temp != firsthist) {
		    if (temp == 0) temp = MAX_HIST-1; else temp--;
		}
		while (temp != lasthist) {
#ifdef NEED_TO_DEBUG_BADLY
		    regs = history[temp];
		    regflags = historyf[temp];
		    m68k_dumpstate (NULL);
#else
		    m68k_disasm (history[temp], NULL, 1);
#endif
		    if (++temp == MAX_HIST) temp = 0;
		}
#ifdef NEED_TO_DEBUG_BADLY
		regs = save_regs;
		regflags = save_flags;
#endif
	    }
	    break;
	 case 'm':
	    {
		uae_u32 maddr; int lines;
		if (more_params(&inptr))
		    maddr = readhex(&inptr);
		else
		    maddr = nxmem;
		if (more_params(&inptr))
		    lines = readhex(&inptr);
		else
		    lines = 16;
		dumpmem(maddr, &nxmem, lines);
	    }
	    break;
	  case 'h':
	  case '?':
	    {
		printf ("          HELP for UAE Debugger\n");
		printf ("         -----------------------\n\n");
		printf ("  g: <address>          Start execution at the current address or <address>\n");
		printf ("  r:                    Dump state of the CPU\n");
		printf ("  R:                    Reset CPU & FPU\n");
		printf ("  i:                    Enable IRQ\n");
		printf ("  I:                    Disable IRQ\n");
		printf ("  A <number> <value>:   Set Ax\n");
		printf ("  D <number> <value>:   Set Dx\n");
		printf ("  m <address> <lines>:  Memory dump starting at <address>\n");
		printf ("  d <address> <lines>:  Disassembly starting at <address>\n");
		printf ("  t <address>:          Step one instruction\n");
		printf ("  z:                    Step through one instruction - useful for JSR, DBRA etc\n");
		printf ("  f <address>:          Step forward until PC == <address>\n");
		printf ("  H <count>:            Show PC history <count> instructions\n");
		printf ("  W <address> <value>:  Write into Aranym memory\n");
		printf ("  S <file> <addr> <n>:  Save a block of Aranym memory\n");
		printf ("  h,?:                  Show this help page\n");
		printf ("  q:                    Quit the emulator. You don't want to use this command.\n\n");
	    }
	    break;

	}
    }
}
