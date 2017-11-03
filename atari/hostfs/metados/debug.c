/*
 * $Id$
 *
 * The ARAnyM MetaDOS driver.
 *
 * 2002 STan
 *
 * Based on:
 * debug.c,v 1.13 2001/12/20 13:35:09 draco Exp
 *
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 *
 *
 * Copyright 1990,1991,1992 Eric R. Smith.
 * Copyright 1992,1993,1994 Atari Corporation.
 * All rights reserved.
 *
 *
 * MiNT debugging output routines
 *
 */

# include "mint/mint.h"
# include "libkern/libkern.h"
# include "debug.h"

# include "mintproc.h"
# include "mintfake.h"

# include <stdarg.h>
# include <osbind.h>


static void VDEBUGOUT(const char *, va_list, int);


int debug_level = 10;   /* how much debugging info should we print? */
# if 1
int out_device = 2; /* BIOS device to write errors to */
# else
int out_device = 6;
# endif

/*
 * out_next[i] is the out_device value to use when the current
 * device is i and the user hits F3.
 * Cycle is CON -> PRN -> AUX -> MIDI -> 6 -> 7 -> 8 -> 9 -> CON
 * (Note: BIOS devices 6-8 exist on Mega STe and TT, 9 on TT.)
 *
 * out_device and this table are exported to bios.c and used here in HALT().
 */

/*		    0  1  2  3  4  5  6  7  8  9 */
char out_next[] = { 1, 3, 0, 6, 0, 0, 7, 8, 9, 2 };

/*
 * debug log modes:
 *
 * 0: no logging.
 * 1: log all messages, dump the log any time something happens at
 *    a level that gets shown.  Thus, if you're at debug_level 2,
 *    everything is logged, and if something at levels 1 or 2 happens,
 *    the log is dumped.
 *
 * LB_LINE_LEN is 20 greater than SPRINTF_MAX because up to 20 bytes
 * are prepended to the buffer string passed to ksprintf.
 */

# ifdef DEBUG_INFO
# define LBSIZE		50			/* number of lines */
# else
# define LBSIZE		15
# endif
# define LB_LINE_LEN	(SPRINTF_MAX + 20)	/* width of a line */

static char logbuf[LBSIZE][LB_LINE_LEN];
static ushort logtime[LBSIZE];	/* low 16 bits of 200 Hz: timestamp of msg */

int debug_logging = 0;
int logptr = 0;


/* NatFeat functions defined */
static long _NF_getid = 0x73004e75L;
static long _NF_call  = 0x73014e75L;
#define nfGetID(n)	(((long _cdecl (*)(const char *))&_NF_getid)n)
#define nfCall(n)	(((long _cdecl (*)(long, ...))&_NF_call)n)

#define nf_stderr(str) \
	(nfCall((nfGetID(("NF_STDERR")),str)))


#ifndef ARAnyM_MetaDOS

/*
 * The inner loop does this: at each newline, the keyboard is polled. If
 * you've hit a key, then it's checked: if it's ctl-alt, do_func_key is
 * called to do what it says, and that's that.  If not, then you pause the
 * output.  If you now hit a ctl-alt key, it gets done and you're still
 * paused.  Only hitting a non-ctl-alt key will get you out of the pause. 
 * (And only a non-ctl-alt key got you into it, too!)
 *
 * When out_device isn't the screen, number keys give you the same effects
 * as function keys.  The only way to get into this code, however, is to
 * have something produce debug output in the first place!  This is
 * awkward: Hit a key on out_device, then hit ctl-alt-F5 on the console so
 * bios.c will call DUMPPROC, which will call ALERT, which will call this.
 * It'll see the key you hit on out_device and drop you into this loop.
 * CTL-ALT keys make BIOS call do_func_key even when out_device isn't the
 * console.
 */

void
debug_ws(const char *s)
{
	long key;
	int scan;
	int stopped;

# ifdef ARANYM
	if (nf_debug(s))
		return;
# endif

	while (*s)
	{
		safe_Bconout(out_device, *s);
		
		while (*s == '\n' && safe_Bconstat(out_device))
		{
			stopped = 0;
			while (1)
			{
				if (out_device == 2)
				{
					/* got a key; if ctl-alt then do it */
					if ((safe_Kbshift (-1) & 0x0c) == 0x0c)
					{
						key = safe_Bconin(out_device);
						scan = (int)(((key >> 16) & 0xff));
						do_func_key(scan);
						
						goto ptoggle;
					}
					else
					{
						goto cont;
					}
				}
				else
				{
					key = safe_Bconin(out_device);
					if (key < '0' || key > '9')
					{
						if (key == 'R')
							hw_warmboot();
ptoggle:
						/* not a func key */
						if (stopped) break;
						else stopped = 1;
					}
					else
					{
						/* digit key from debug device == Fn */
						if (key == '0') scan = 0x44;
						else scan = (int)(key - '0' + 0x3a);
						do_func_key(scan);
					}
				}
			}
		}
cont:
		s++;
	}
}

#else

void
debug_ws (const char *s)
{
	nf_stderr(s);
}

#endif /* ARAnyM_MetaDOS */


/*
 * _ALERT(s) returns 1 for success and 0 for failure.
 * It attempts to write the string to "the alert pipe," u:\pipe\alert.
 * If the write fails because the pipe is full, we "succeed" anyway.
 *
 * This is called in vdebugout and also in mprot0x0.c for memory violations.
 * It's also used by the Salert() system call in dos.c.
 */

int
_ALERT(const char *s)
{
#ifndef ARAnyM_MetaDOS
	FILEPTR *fp;
	long ret;
	
	/* temporarily reduce the debug level, so errors finding
	 * u:\pipe\alert don't get reported
	 */
	int olddebug = debug_level;
	int oldlogging = debug_logging;
	
	debug_level = FORCE_LEVEL;
	debug_logging = 0;
	
	ret = FP_ALLOC(rootproc, &fp);
	if (!ret){
		ret = do_open(&fp, "u:\\pipe\\alert", (O_WRONLY | O_NDELAY), 0, NULL);
	}
	
	debug_level = olddebug;
	debug_logging = oldlogging;
	
	if (!ret)
	{
		const char *alert;
		
		/* format the string into an alert box
		 */
		if (*s == '[')
		{
			/* already an alert */
			alert = s;
		}
		else
		{
			char alertbuf[SPRINTF_MAX + 32];
			char *ptr, *end = alertbuf+sizeof(alertbuf)-8;	/* strlen "][ OK ]" +1 */
			char *lastspace;
			int counter;
			
			alert = alertbuf;
			ksprintf(alertbuf, end-alertbuf, "[1][%s", s);
			*end = 0;
			
			/* make sure no lines exceed 30 characters;
			 * also, filter out any reserved characters
			 * like '[' or ']'
			 */
			ptr = alertbuf + 4;
			counter = 0;
			lastspace = 0;
			
			while (*ptr)
			{
				if (*ptr == ' ')
				{
					lastspace = ptr;
				}
				else if (*ptr == '[')
				{
					*ptr = '(';
				}
				else if (*ptr == ']')
				{
					*ptr = ')';
				}
				else if (*ptr == '|')
				{
					*ptr = ':';
				}
				
				if (counter++ >= 29)
				{
					if (lastspace)
					{
						*lastspace = '|';
						counter = (int)(ptr - lastspace);
						lastspace = 0;
					}
					else
					{
						*ptr = '|';
						counter = 0;
					}
				}
				
				ptr++;
			}
			
			strcpy(ptr, "][ OK ]");
		}
		
		if( !fp->dev )
		{
			DEBUG(("_ALERT:fp->dev=0! (%s:%ld)", __FILE__, (long)__LINE__));
			return 0;
		}

		if( !fp->dev->write )
		{
			DEBUG(("_ALERT:fp->dev->write=0! (%s:%ld)", __FILE__, (long)__LINE__));
			return 0;
		}
		(*fp->dev->write)(fp, alert, strlen(alert) + 1);
		do_close(rootproc, fp);
		
		return 1;
	}
	
#endif /* ARAnyM_MetaDOS */
	return 0;
}

static void
VDEBUGOUT(const char *s, va_list args, int alert_flag)
{
	char *lp;
	char *lptemp;
	long len;
	
	logtime[logptr] = (ushort)(*(long *) 0x4baL);
	lptemp = lp = logbuf[logptr];
	len = LB_LINE_LEN;
	
	if (++logptr == LBSIZE)
		logptr = 0;
	
#ifndef ARAnyM_MetaDOS
	if (get_curproc())
	{
		int splen = ksprintf(lp, len, "pid %3d (%s): ", get_curproc()->pid, get_curproc()->name);
		lptemp += splen;
		len -= splen;
	}
#endif /* ARAnyM_MetaDOS */
	
	kvsprintf(lptemp, len, s, args);
	
	/* for alerts, try the alert pipe unconditionally */
	if (alert_flag && _ALERT(lp))
		return;
	
	debug_ws(lp);
	debug_ws("\r\n");
}

void _cdecl
Tracelow(const char *s, ...)
{
	if (debug_logging
	    || (debug_level >= LOW_LEVEL)
	    || (get_curproc() && (get_curproc()->debug_level >= LOW_LEVEL))
# ifdef DEBUG_INFO
	    || (get_curproc()==NULL)
# endif
	)
	{
		va_list args;
		
		va_start(args, s);
		VDEBUGOUT(s, args, 0);
		va_end(args);
	}
}

void _cdecl
Trace(const char *s, ...)
{
	if (debug_logging
	    || (debug_level >= TRACE_LEVEL)
	    || (get_curproc() && (get_curproc()->debug_level >= TRACE_LEVEL))
# ifdef DEBUG_INFO
	    || (get_curproc()==NULL)
# endif
	)
	{
		va_list args;
		
		va_start(args, s);
		VDEBUGOUT(s, args, 0);
		va_end(args);
	}
}

void _cdecl
Debug(const char *s, ...)
{
	if (debug_logging
	    || (debug_level >= DEBUG_LEVEL)
	    || (get_curproc() && (get_curproc()->debug_level >= DEBUG_LEVEL))
# ifdef DEBUG_INFO
	    || (get_curproc()==NULL)
# endif
	)
	{
		va_list args;
		
		va_start(args, s);
		VDEBUGOUT(s, args, 0);
		va_end(args);
	}
	
	if (debug_logging
	    && ((debug_level >= DEBUG_LEVEL)
	        || (get_curproc() && (get_curproc()->debug_level >= DEBUG_LEVEL)))) 
	{
		DUMPLOG();
	}
}

void _cdecl
ALERT(const char *s, ...)
{
	if (debug_logging || debug_level >= ALERT_LEVEL)
	{
		va_list args;
		
		va_start(args, s);
		VDEBUGOUT(s, args, 1);
		va_end(args);
	}
	
	if (debug_logging && (debug_level >= ALERT_LEVEL))
		DUMPLOG();
}

void _cdecl
FORCE(const char *s, ...)
{
	if (debug_level >= FORCE_LEVEL)
	{
		va_list args;
		
		va_start(args, s);
		VDEBUGOUT(s, args, 0);
		va_end(args);
	}
	
	/* don't dump log here - hardly ever what you mean to do. */
}

void
DUMPLOG(void)
{
	char *end;
	char *start;
	ushort *timeptr;
	char timebuf[6];
	
	/* logbuf [logptr] is the oldest string here */
	
	end = start = logbuf[logptr];
	timeptr = &logtime[logptr];
	
	do
	{
		if (*start)
		{
			ksprintf(timebuf, sizeof(timebuf), "%04x ", *timeptr);
			debug_ws(timebuf);
			debug_ws(start);
			debug_ws("\r\n");
			*start = '\0';
		}
		
		start += LB_LINE_LEN;
		timeptr++;
		
		if (start == logbuf[LBSIZE])
		{
			start = logbuf[0];
			timeptr = &logtime[0];
		}
	}
	while (start != end);

	logptr = 0;
}

EXITING _cdecl
FATAL(const char *s, ...)
{
	va_list args;
	
	va_start(args, s);
	VDEBUGOUT(s, args, 0);
	va_end(args);
	
	if (debug_logging)
		DUMPLOG();
	
	HALT();
}


/* uk: a normal halt() function without an error message. This function
 *	   may only be called if all processes are shut down and the file
 *	   systems are synced.
 */

EXITING
halt (void)
{
#ifndef ARAnyM_MetaDOS
	long r;
	long key;
	int scan;

	DEBUG (("halt() called, system halting...\r\n"));
	debug_ws ("MSG_system_halted");

	sys_q[READY_Q] = 0; /* prevent context switches */
	restr_intr();		/* restore interrupts to normal */

	for(;;)
	{
		/* get a key; if ctl-alt then do it, else halt */
		key = Bconin (out_device);

		if ((key & 0x0c000000L) == 0x0c000000L)
		{
			scan = (int) ((key >> 16) & 0xff);
			do_func_key (scan);
		}
		else
		{
			break;
		}
	}

	for(;;)
	{
		debug_ws ("MSG_system_halted");
		r = Bconin (2);
		if ((r & 0x0ff) == 'x')
		{
		}
	}
#else
	for (;;) {
		debug_ws ("MSG_system_halted");
	}
#endif /* ARAnyM_MetaDOS */
}

EXITING
HALT (void)
{
#ifndef ARAnyM_MetaDOS
	long r;
	long key;
	int scan;

	DEBUG (("Fatal MiNT error: adjust debug level and hit a key...\r\n"));
	debug_ws ("MSG_fatal_reboot");

	sys_q[READY_Q] = 0; /* prevent context switches */
	restr_intr ();		/* restore interrupts to normal */

	for (;;)
	{
		/* get a key; if ctl-alt then do it, else halt */
		key = Bconin (2);

		if ((key & 0x0c000000L) == 0x0c000000L)
		{
			scan = (int) ((key >> 16) & 0xff);
			do_func_key (scan);
		}
		else
			break;
	}

	for (;;)
	{
		debug_ws ("MSG_fatal_reboot");
		r = Bconin (2);

		if (((r & 0x0ff) == 'x') || ((r & 0xff) == 's'))
		{
			close_filesys ();

			/* if the user pressed 's', try to sync before halting the system */
			if ((r & 0xff) == 's')
			{
				debug_ws (MSG_debug_syncing);
				s_ync ();
				debug_ws (MSG_debug_syncdone);
			}
		}
	}
#else
	for (;;) {
		debug_ws ("MSG_fatal_reboot");
	}
#endif /* ARAnyM_MetaDOS */
}
