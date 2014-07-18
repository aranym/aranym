/*
	ARAnyM Debugger

	(C) 2001-2009 ARAnyM developer team

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "sysdeps.h"
#include "ndebug.h"

#ifndef DEBUG_H
#define DEBUG_H

#ifdef WHOLEDEBUG
# undef DEBUG
# define DEBUG 2
#endif

#if defined(DEBUG) && DEBUG
#define DUNUSED(x)
#else
#define DUNUSED(x)	((void)x)
#endif
#ifndef UNUSED
#define UNUSED(x)	((void)x)
#endif

#define	MAX_HIST	10000

extern unsigned int firsthist;
extern unsigned int lasthist;
extern int debugging;
extern int irqindebug;
extern bool cpu_debugging;
extern int ignore_irq;

#ifdef NEED_TO_DEBUG_BADLY
extern struct regstruct history[MAX_HIST];
extern struct flag_struct historyf[MAX_HIST];
#else
extern uaecptr history[MAX_HIST];
#endif

extern void debug(void);
extern void activate_debugger(void);
extern void deactivate_debugger(void);

/*
 *  debug.h - Debugging utilities
 *
 *  Basilisk II (C) 1997-2000 Christian Bauer
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef NEWDEBUG
#define bug ndebug::dbprintf
#define panicbug ndebug::pdbprintf
#else
#define bug dbprintf
#define panicbug pdbprintf
#endif

#if defined(DEBUG) && DEBUG
#define D(x) (x);
#else
#define D(x) ;
#endif

#if defined(DEBUG) && DEBUG > 1
#define D2(x) (x);
#else
#define D2(x) ;
#endif

#define infoprint panicbug

#endif
