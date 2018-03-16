/*
 * The ARAnyM MetaDOS driver.
 *
 * 2002 STanda
 *
 * Based on:
 * dosdir.c,v 1.11 2001/10/23 09:09:14 fna Exp
 *
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 *
 *
 * Copyright 1990,1991,1992 Eric R. Smith.
 * Copyright 1992,1993,1994 Atari Corporation.
 * All rights reserved.
 *
 */

/* DOS directory functions */

#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <sys/stat.h>
#include <osbind.h>

#include "bosmeta.h"
#include "list.h"
#include "dosdir.h"

#include "emu_tos.h"
#include "mintfake.h"

#if 0
#include "nfd.h"
#define TRACE(x) NFD(x)
#define DEBUG(x) NFD(x)
#else
#define TRACE(x)
#define DEBUG(x)
#endif


#define TOS_SEARCH	0x01

#define E_OK		0


#define NUM_SEARCH	10

static struct {
	DIR dirh;
	unsigned long srchtim;
} search_dirs[NUM_SEARCH];


static inline void
release_cookie( FCOOKIE **dir) {
	/* dummy as there is no dup_cookie in the path2cookie() */
}

/*
 * returns 1 if the given name contains a wildcard character
 */

static int
has_wild (const char *name)
{
	char c;

	while ((c = *name++) != 0)
		if (c == '*' || c == '?')
			return 1;

	return 0;
}

/*
 * void copy8_3(dest, src): convert a file name (src) into DOS 8.3 format
 * (in dest). Note the following things:
 * if a field has less than the required number of characters, it is
 * padded with blanks
 * a '*' means to pad the rest of the field with '?' characters
 * special things to watch for:
 *  "." and ".." are more or less left alone
 *  "*.*" is recognized as a special pattern, for which dest is set
 *  to just "*"
 * Long names are truncated. Any extensions after the first one are
 * ignored, i.e. foo.bar.c -> foo.bar, foo.c.bar->foo.c.
 */

static void
copy8_3 (char *dest, const char *src)
{
	char fill = ' ', c;
	int i;

	if (src[0] == '.')
	{
		if (src[1] == 0)
		{
			strcpy (dest, ".       .   ");
			return;
		}

		if (src[1] == '.' && src[2] == 0)
		{
			strcpy (dest, "..      .   ");
			return;
		}
	}

	if (src[0] == '*' && src[1] == '.' && src[2] == '*' && src[3] == 0)
	{
		dest[0] = '*';
		dest[1] = 0;
		return;
	}

	for (i = 0; i < 8; i++)
	{
		c = *src++;

		if (!c || c == '.')
			break;
		if (c == '*')
			fill = c = '?';

		*dest++ = toupper((int)c & 0xff);
	}

	while (i++ < 8)
		*dest++ = fill;

	*dest++ = '.';
	i = 0;
	fill = ' ';
	while (c && c != '.')
		c = *src++;

	if (c)
	{
		for( ;i < 3; i++)
		{
			c = *src++;

			if (!c || c == '.')
				break;

			if (c == '*')
				c = fill = '?';

			*dest++ = toupper((int)c & 0xff);
		}
	}

	while (i++ < 3)
		*dest++ = fill;

	*dest = 0;
}

/*
 * int pat_match(name, patrn): returns 1 if "name" matches the template in
 * "patrn", 0 if not. "patrn" is assumed to have been expanded in 8.3
 * format by copy8_3; "name" need not be. Any '?' characters in patrn
 * will match any character in name. Note that if "patrn" has a '*' as
 * the first character, it will always match; this will happen only if
 * the original pattern (before copy8_3 was applied) was "*.*".
 *
 * BUGS: acts a lot like the silly TOS pattern matcher.
 */

static int
pat_match (const char *name, const char *template)
{
	char expname [TOS_NAMELEN+1];
	register char *s;
	register char c;

	if (*template == '*')
		return 1;

	copy8_3 (expname, name);

	s = expname;
	while ((c = *template++) != 0)
	{
		if (c != *s && c != '?')
			return 0;
		s++;
	}

	return 1;
}


/*
 * Fsfirst/next are actually implemented in terms of opendir/readdir/closedir.
 */

long __CDECL
sys_f_sfirst (MetaDOSDTA const char *path, int attrib)
{
	static short initialized = 0;
	char *s, *slash;
	FCOOKIE *dir, *newdir;
	struct dtabuf *dta = (struct dtabuf*)dtaMD;
	long r;
	int i, havelabel;
	char temp1[PATH_MAX];

	TRACE(("Fsfirst(%s, %x)", path, attrib));

	if ( !initialized ) {
		initialized = 1;
		for (i = 0; i < NUM_SEARCH; i++) {
			search_dirs[i].dirh.list = NULL;
			search_dirs[i].dirh.current = NULL;
			search_dirs[i].srchtim = 0;
		}
	}

	r = path2cookie (path, temp1, &dir);
	if (r)
	{
		DEBUG(("Fsfirst(%s): path2cookie returned %ld", path, r));
		return r;
	}

	/* we need to split the last name (which may be a pattern) off from
	 * the rest of the path, even if FS_KNOPARSE is true
	 */
	slash = 0;
	s = temp1;
	while (*s)
	{
		if (*s == '\\')
			slash = s;
		s++;
	}

	if (slash)
	{
		*slash++ = 0;   /* slash now points to a name or pattern */
		r = name2cookie (dir->folder, temp1, &newdir);
		release_cookie (&dir);
		if (r)
		{
			DEBUG(("Fsfirst(%s): lookup returned %ld", path, r));
			return r;
		}
		dir = newdir;
	}
	else
		slash = temp1;

	/* BUG? what if there really is an empty file name?
	*/
	if (!*slash)
	{
		DEBUG(("Fsfirst: empty pattern"));
		return -ENOENT;
	}

	/* copy the pattern over into dta_pat into TOS 8.3 form
	 * remember that "slash" now points at the pattern
	 * (it follows the last, if any)
	 */
	copy8_3 (dta->dta_pat, slash);

	/* if (attrib & FA_LABEL), read the volume label
	 *
	 * BUG: the label date and time are wrong. ISO/IEC 9293 14.3.3 allows this.
	 * The Desktop set also date and time to 0 when formatting a floppy disk.
	 */
	havelabel = 0;
	if (attrib & FA_LABEL)
	{
		r = sys_dl_readlabel (dta->dta_name, TOS_NAMELEN+1);
		dta->dta_attrib = FA_LABEL;
		dta->dta_time = dta->dta_date = 0;
		dta->dta_size = 0;
		dta->magic = EVALID;
		if (r == E_OK && !pat_match (dta->dta_name, dta->dta_pat))
			r = -ENOENT;
		if ((attrib & (FA_DIR|FA_LABEL)) == FA_LABEL)
			return r;
		else if (r == E_OK)
			havelabel = 1;
	}

	DEBUG(("Fsfirst(): havelabel = %d",havelabel));

	if (!havelabel && has_wild (slash) == 0)
	{
		struct xattr xattr;

		/* no wild cards in pattern */
		r = name2cookie (dir->folder, slash, &newdir);
		if (r == E_OK)
		{
			r = getxattr (newdir, &xattr);
			release_cookie (&newdir);
		}
		release_cookie (&dir);
		if (r)
		{
			DEBUG(("Fsfirst(%s): couldn't get file attributes",path));
			return r;
		}

		dta->magic = EVALID;
		dta->dta_attrib = xattr.attr;
		dta->dta_size = xattr.size;

		strncpy (dta->dta_name, slash, TOS_NAMELEN-1);
		dta->dta_name[TOS_NAMELEN-1] = 0;
		return E_OK;
	}

	/* There is a wild card. Try to find a slot for an opendir/readdir
	 * search. NOTE: we also come here if we were asked to search for
	 * volume labels and found one.
	 */
	for (i = 0; i < NUM_SEARCH; i++)
		if (! search_dirs[i].dirh.list)
			break;

	if (i == NUM_SEARCH)
	{
		int oldest = 0;

		DEBUG(("Fsfirst(%s): having to re-use a directory slot!", path));
		for (i = 1; i < NUM_SEARCH; i++)
			if ( search_dirs[i].srchtim < search_dirs[oldest].srchtim )
				oldest = i;

		/* OK, close this directory for re-use */
		i = oldest;
		sys_dl_closedir (&search_dirs[i].dirh);

		/* invalidate re-used DTA */
		dta->magic = EVALID;
	}

	r = sys_dl_opendir (&search_dirs[i].dirh, dir->folder, TOS_SEARCH);
	DEBUG(("Fsfirst opendir(%s, %d) -> %d", dir->name, i, r));
	if (r != E_OK)
	{
		DEBUG(("Fsfirst(%s): couldn't open directory (error %ld)", path, r));
		release_cookie(&dir);
		return r;
	}

	/* set up the DTA for Fsnext */
	dta->index = i;
	dta->magic = SVALID;
	dta->dta_sattrib = attrib;

	/* OK, now basically just do Fsnext, except that instead of ENMFILES we
	 * return ENOENT.
	 * NOTE: If we already have found a volume label from the search above,
	 * then we skip the sys_f_snext and just return that.
	 */
	if (havelabel)
		return E_OK;

	r = sys_f_snext(MetaDOSDTA0pass);
	if (r == ENMFILES) r = -ENOENT;
	if (r)
		TRACE(("Fsfirst: returning %ld", r));

	/* release_cookie isn't necessary, since &dir is now stored in the
	 * DIRH structure and will be released when the search is completed
	 */
	return r;
}

/*
 * Counter for Fsfirst/Fsnext, so that we know which search slots are
 * least recently used. This is updated once per second by the code
 * in timeout.c.
 *
 * BUG: 1/second is pretty low granularity
 */

long __CDECL
sys_f_snext (MetaDOSDTA0)
{
	DIR *dirh;
	struct dtabuf *dta = (struct dtabuf*)dtaMD;
	short attr;

	TRACE (("Fsnext"));

	if (dta->magic == EVALID)
	{
		DEBUG (("Fsnext(%lx): DTA marked a failing search", dta));
		return -ENMFILES;
	}

	if (dta->magic != SVALID)
	{
		DEBUG (("Fsnext(%lx): dta incorrectly set up", dta));
		return -ENOSYS;
	}

	/* get the slot pointer */
	search_dirs[ dta->index ].srchtim++;
	dirh = &search_dirs[ dta->index ].dirh;

	/* BUG: sys_f_snext and readdir should check for disk media changes
	*/
	for(;;)
	{
		char buf[TOS_NAMELEN+1];
		long r = sys_dl_readdir( dirh, buf, TOS_NAMELEN+1);
		if (r == EBADARG)
		{
			DEBUG(("Fsnext: name too long"));
			continue;   /* TOS programs never see these names */
		}

		if (r != E_OK)
		{
			sys_dl_closedir (dirh);

			dta->magic = EVALID;

			if (r != ENMFILES)
				DEBUG(("Fsnext: returning %ld", r));
			return r;
		}

		if (!pat_match (buf, dta->dta_pat))
			continue;   /* different patterns */
		attr = dirh->current->attr;

		/* silly TOS rules for matching attributes */
		if (attr == 0)
			break;

		if (attr & (FA_CHANGED|FA_RDONLY))
			break;

		if (attr & dta->dta_sattrib)
			break;
	}

	dta->dta_attrib = attr;
	dta->dta_size = 0;
	strncpy (dta->dta_name, dirh->current->name, TOS_NAMELEN-1);

	/* convert to upper characters (we are in TOS domain) */
	strupr (dta->dta_name);

	DEBUG(("Fsnext: %s\n", dta->dta_name));
	return E_OK;
}

