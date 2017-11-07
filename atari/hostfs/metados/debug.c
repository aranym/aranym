/*
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

#include "hostfs.h"
#include "nf_ops.h"
#include "mint/string.h"

#include <osbind.h>


int debug_level = 10;   /* how much debugging info should we print? */

/*
 * debug log modes:
 *
 * 0: no logging.
 * 1: log all messages, dump the log any time something happens at
 *    a level that gets shown.  Thus, if you're at debug_level 2,
 *    everything is logged, and if something at levels 1 or 2 happens,
 *    the log is dumped.
 */

int debug_logging = 0;


void
debug_ws (const char *s)
{
	nf_debug(s);
}

static void
VDEBUGOUT(const char *s, va_list args, int alert_flag)
{
	char lptemp[256];
	
	kvsprintf(lptemp, sizeof(lptemp), s, args);
	
	debug_ws(lptemp);
	debug_ws("\r\n");
}

# ifdef DEBUG_INFO
void _cdecl
Tracelow(const char *s, ...)
{
	if (debug_logging
	    || (debug_level >= LOW_LEVEL)
	    || (get_curproc() && (get_curproc()->debug_level >= LOW_LEVEL))
	    || (get_curproc()==NULL)
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
	    || (get_curproc()==NULL)
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
	    || (get_curproc()==NULL)
	)
	{
		va_list args;
		
		va_start(args, s);
		VDEBUGOUT(s, args, 0);
		va_end(args);
	}
}

# endif

EXITING _cdecl
FATAL(const char *s, ...)
{
	va_list args;
	
	va_start(args, s);
	VDEBUGOUT(s, args, 0);
	va_end(args);
	
	HALT();
}


/* uk: a normal halt() function without an error message. This function
 *	   may only be called if all processes are shut down and the file
 *	   systems are synced.
 */

EXITING
halt (void)
{
	debug_ws ("MSG_system_halted");
	for (;;) {
	}
}

EXITING
HALT (void)
{
	debug_ws ("MSG_fatal_reboot");
	for (;;) {
	}
}
