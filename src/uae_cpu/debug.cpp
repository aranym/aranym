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
extern uint32 RAMSize;


static int debugger_active = 0;
static uaecptr skipaddr;
static int do_skip;
int debugging = 1;

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
	    printf ("%04x ", get_word(addr)); addr += 2;
	}
	printf ("\n");
    }
    *nxmem = addr;
}

static void foundmod (uae_u32 ptr, char *type)
{
    char name[21];
    uae_u8 *ptr2 = RAMBaseHost + ptr;
    int i,length;

    printf ("Found possible %s module at 0x%lx.\n", type, ptr);
    memcpy (name, ptr2, 20);
    name[20] = '\0';

    /* Browse playlist */
    length = 0;
    for (i = 0x3b8; i < 0x438; i++)
	if (ptr2[i] > length)
	    length = ptr2[i];

    length = (length+1)*1024 + 0x43c;

    /* Add sample lengths */
    ptr2 += 0x2A;
    for (i = 0; i < 31; i++, ptr2 += 30)
	length += 2*((ptr2[0]<<8)+ptr2[1]);
    
    printf ("Name \"%s\", Length 0x%lx bytes.\n", name, length);
}

static void modulesearch (void)
{
    uae_u8 *p = get_real_address (0);
    uae_u32 ptr;

    for (ptr = 0; ptr < RAMSize - 40; ptr += 2, p += 2) {
	/* Check for Mahoney & Kaktus */
	/* Anyone got the format of old 15 Sample (SoundTracker)modules? */
	if (ptr >= 0x438 && p[0] == 'M' && p[1] == '.' && p[2] == 'K' && p[3] == '.')
	    foundmod (ptr - 0x438, "ProTracker (31 samples)");

	if (ptr >= 0x438 && p[0] == 'F' && p[1] == 'L' && p[2] == 'T' && p[3] == '4')
	    foundmod (ptr - 0x438, "Startrekker");

	if (strncmp ((char *)p, "SMOD", 4) == 0) {
	    printf ("Found possible FutureComposer 1.3 module at 0x%lx, length unknown.\n", ptr);
	}
	if (strncmp ((char *)p, "FC14", 4) == 0) {
	    printf ("Found possible FutureComposer 1.4 module at 0x%lx, length unknown.\n", ptr);
	}
	if (p[0] == 0x48 && p[1] == 0xe7 && p[4] == 0x61 && p[5] == 0
	    && p[8] == 0x4c && p[9] == 0xdf && p[12] == 0x4e && p[13] == 0x75
	    && p[14] == 0x48 && p[15] == 0xe7 && p[18] == 0x61 && p[19] == 0
	    && p[22] == 0x4c && p[23] == 0xdf && p[26] == 0x4e && p[27] == 0x75) {
	    printf ("Found possible Whittaker module at 0x%lx, length unknown.\n", ptr);
	}
	if (p[4] == 0x41 && p[5] == 0xFA) {
	    int i;

	    for (i = 0; i < 0x240; i += 2)
		if (p[i] == 0xE7 && p[i + 1] == 0x42 && p[i + 2] == 0x41 && p[i + 3] == 0xFA)
		    break;
	    if (i < 0x240) {
		uae_u8 *p2 = p + i + 4;
		for (i = 0; i < 0x30; i += 2)
		    if (p2[i] == 0xD1 && p2[i + 1] == 0xFA) {
			printf ("Found possible MarkII module at %lx, length unknown.\n", ptr);
		    }
	    }
	}
    }
}

/* cheat-search by Holger Jakob */
static void cheatsearch (char **c)
{
    uae_u8 *p = get_real_address (0);
    static uae_u32 *vlist = NULL;
    uae_u32 ptr;
    uae_u32 val = 0;
    uae_u32 type = 0; /* not yet */
    uae_u32 count = 0;
    uae_u32 fcount = 0;
    uae_u32 full = 0;
    char nc;

    ignore_ws (c);

    while (isxdigit (nc = **c)) {
	(*c)++;
	val *= 10;
	nc = toupper (nc);
	if (isdigit (nc)) {
	    val += nc - '0';
	}
    }
    if (vlist == NULL) {
	vlist = (uae_u32 *)malloc (256*4);
	if (vlist != 0) {
	    for (count = 0; count<255; count++)
		vlist[count] = 0;
	    count = 0;
	    for (ptr = 0; ptr < RAMSize - 40; ptr += 2, p += 2) {
		if (ptr >= 0x438 && p[3] == (val & 0xff)
		    && p[2] == (val >> 8 & 0xff)
		    && p[1] == (val >> 16 & 0xff)
		    && p[0] == (val >> 24 & 0xff))
		{
		    if (count < 255) {
			vlist[count++]=ptr;
			printf ("%08x: %x%x%x%x\n",ptr,p[0],p[1],p[2],p[3]);
		    } else
			full = 1;
		}
	    }
	    printf ("Found %d possible addresses with %d\n",count,val);
	    printf ("Now continue with 'g' and use 'C' with a different value\n");
	}
    } else {
	for (count = 0; count<255; count++) {
	    if (p[vlist[count]+3] == (val & 0xff)
		&& p[vlist[count]+2] == (val>>8 & 0xff) 
		&& p[vlist[count]+1] == (val>>16 & 0xff)
		&& p[vlist[count]] == (val>>24 & 0xff))
	    {
		fcount++;
		printf ("%08x: %x%x%x%x\n", vlist[count], p[vlist[count]],
			p[vlist[count]+1], p[vlist[count]+2], p[vlist[count]+3]);
	    }
	}
	printf ("%d hits of %d found\n",fcount,val);
	free (vlist);
	vlist = NULL;
    }
}

static void writeintomem (char **c)
{
    uae_u8 *p = get_real_address (0);
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
	memcpy (trace_insn_copy, regs.pc_p, 10);
	memcpy (&trace_prev_regs, &regs, sizeof regs);
    }

    if (do_skip && (m68k_getpc() != skipaddr/* || regs.a[0] != 0x1e558*/)) {
	set_special (SPCFLAG_BRK);
	return;
    }
    do_skip = 0;

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
	inptr = input;
	cmd = next_char (&inptr);
	switch (cmd) {
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
	 case 'M': modulesearch (); break;
	 case 'C': cheatsearch (&inptr); break; 
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
		memp = get_real_address (src);
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
	    set_special (SPCFLAG_BRK);
	    return;

	 case 'f':
	    skipaddr = readhex (&inptr);
	    do_skip = 1;
	    set_special (SPCFLAG_BRK);
	    if (skipaddr == 0xC0DEDBAD) {
	        trace_same_insn_count = 0;
		memcpy (trace_insn_copy, regs.pc_p, 10);
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
		printf ("  A <number> <value>:   Set Ax\n");
		printf ("  D <number> <value>:   Set Dx\n");
		printf ("  m <address> <lines>:  Memory dump starting at <address>\n");
		printf ("  d <address> <lines>:  Disassembly starting at <address>\n");
		printf ("  t <address>:          Step one instruction\n");
		printf ("  z:                    Step through one instruction - useful for JSR, DBRA etc\n");
		printf ("  f <address>:          Step forward until PC == <address>\n");
		printf ("  H <count>:            Show PC history <count> instructions\n");
		printf ("  M:                    Search for *Tracker sound modules\n");
		printf ("  C <value>:            Search for values like energy or lifes in games\n");
		printf ("  W <address> <value>:  Write into Amiga memory\n");
		printf ("  S <file> <addr> <n>:  Save a block of Amiga memory\n");
		printf ("  h,?:                  Show this help page\n");
		printf ("  q:                    Quit the emulator. You don't want to use this command.\n\n");
	    }
	    break;

	}
    }
}
