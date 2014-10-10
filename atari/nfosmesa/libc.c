#include <stddef.h>
#include <string.h>
#include <stdlib.h>

/*
 * Selection of functions that are (directly or indirectly)
 * referenced by libldg.a
 */

char **__environ;

/*
 * only called form crtinit.
 * Must be called extern though, otherwise it gets inlined,
 * making __builtin_return_address() return the return address of
 * main.
 */
void _setstack(char *newsp)
{
	register void *retpc __asm__ ("a0") = __builtin_return_address(0);
	
	__asm__ volatile (
		"\tmove.l %1,sp\n"		/* set new stack pointer */
		"\tsubl	#64+4, sp\n"	/* push some unused space for buggy OS and a
								   fake parameter to be popped by the caller */
		"\tjmp (%0)\n"
	: /* outputs */
	: "a"(retpc), "r"(newsp) /* inputs */
	);
	__builtin_unreachable();
}

/*
 * strlen - length of string (not including NUL)
 */
size_t strlen(const char *scan)
{
	register const char *start = scan+1;

	if (!scan) return 0;
	while (*scan++ != '\0')
		continue;
	return (size_t)((long)scan - (long)start);
}


/*
 * strncmp - compare at most n characters of string s1 to s2
 */
/* <0 for <, 0 for ==, >0 for > */
int strncmp(const char *scan1, const char *scan2, size_t n)
{
	register unsigned char c1, c2;
	register long count;  /* FIXME!!! Has to be size_t but that requires
				a rewrite (Guido).  */

	if (!scan1) {
		return scan2 ? -1 : 0;
	}
	if (!scan2) return 1;
	count = n;
	do {
		c1 = (unsigned char) *scan1++; c2 = (unsigned char) *scan2++;
	} while (--count >= 0 && c1 && c1 == c2);

	if (count < 0)
		return(0);

	/*
	 * The following case analysis is necessary so that characters
	 * which look negative collate low against normal characters but
	 * high against the end-of-string NUL.
	 */
	if (c1 == c2)
		return(0);
	else if (c1 == '\0')
		return(-1);
	else if (c2 == '\0')
		return(1);
	else
		return(c1 - c2);
}


char *getenv(const char *tag)
{
	char **var;
	char *name;
	size_t len;

	if (!__environ)
		return 0;

	len = strlen (tag);

	for (var = __environ; (name = *var) != 0; var++) {
		if (!strncmp(name, tag, len) && name[len] == '=')
			return name+len+1;
	}

	return 0;
}


/*
 * Convert a string to a long int.
 * Not strictly compliant, but good enough for our purpose.
 */
long int atol(const char *nptr)
{
	long val = 0;
	
	while (*nptr)
	{
		val = val * 10 + (*nptr - '0');
		nptr++;
	}
	return val;
}


void *memset(void *s, int c, size_t n)
{
	register char *p = s;
	while (n)
	{
		*p++ = c;
		n--;
	}
	return s;
}
