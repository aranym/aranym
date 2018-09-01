#include <mint/basepage.h>
#include <mint/osbind.h>
#include <string.h>
#include <stdlib.h>

#define MINFREE	(8L * 1024L)		/* free at least this much mem on top */

long _stksize = 8 * 1024;

BASEPAGE *_base;
int errno;
extern char **__environ;
extern int main(void);
void __main (void) { }

static long parseargs(BASEPAGE *);

extern void _setstack(char *newsp);

void _crtinit(void)
{
	register BASEPAGE *bp;
	register long m;
	register long freemem;
	
	bp = _base;

	/* m = # bytes used by environment + args */
	m = parseargs(bp);

	/* make m the total number of bytes required by program sans stack/heap */
	m += (bp->p_tlen + bp->p_dlen + bp->p_blen + sizeof(BASEPAGE));
	m = (m + 3L) & (~3L);

	/* freemem the amount of free mem accounting for MINFREE at top */
	if ((freemem = (long)bp->p_hitpa - (long)bp - MINFREE - m) <= 0L)
		goto notenough;
	
	/* make m the total number of bytes including stack */
	m += _stksize;

	/* make sure there's enough room for the stack */
	if (((long)bp + m) > ((long)bp->p_hitpa - MINFREE))
		goto notenough;

	/* set up the new stack to bp + m */
	_setstack((char *)bp + m);

	/* shrink the TPA */
	(void)Mshrink(bp, m);

	Pterm(main());
	__builtin_unreachable();
	
notenough:
	(void) Cconws("Fatal error: insufficient memory\r\n"
	              "Hint: either decrease stack size using 'stack' command (not recomended)\r\n"
		          "      or increase TPA_INITIALMEM value in mint.cnf.\r\n");
	Pterm(-1);
	__builtin_unreachable();
}


/*
 * parseargs(bp): parse the environment and arguments pointed to by the
 * basepage. Return the number of bytes of environment and arguments
 * that have been appended to the bss area (the environ and argv arrays
 * are put here, as is a temporary buffer for the command line, if
 * necessary).
 *
 * The MWC extended argument passing scheme is assumed.
 *
 */
static long parseargs(BASEPAGE *bp)
{
	long count = 4;		/* compensate for aligning */
	char *from;
	char **envp;
	/* flag to indicate desktop-style arg. passing */
	long desktoparg;

	/* handle the environment first */

	__environ = envp = (char **)(( (long)bp->p_bbase + bp->p_blen + 4) & (~3));
	from = bp->p_env;
	while (*from)
	{
		*envp++ = from;
		count += 4;
		desktoparg = 1;
		while (*from)
		{
			if (*from == '=')
				desktoparg = 0;
			from++;
		}
		from++;		/* skip 0 */

/* the desktop (and some shells) use the environment in the wrong
 * way, putting in "PATH=\0C:\0" instead of "PATH=C:". so if we
 * find an "environment variable" without an '=' in it, we
 * see if the last environment variable ended with '=\0', and
 * if so we append this one to the last one
 */
		if (desktoparg && envp > &__environ[1]) 
		{
			/* launched from desktop -- fix up env */
			char *p, *q;

			q = envp[-2];	/* current one is envp[-1] */
			while (*q)
				q++;
			if (q[-1] == '=')
			{
				p = *--envp;
				while (*p)
					*q++ = *p++;
				*q = '\0';
			}
		}
	}
	*envp++ = (char *)0;
	count += 4;

	return count + 4;
}
