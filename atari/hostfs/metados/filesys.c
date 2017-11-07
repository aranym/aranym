/*
 * The ARAnyM MetaDOS driver.
 *
 * 2002 STan
 *
 * Based on:
 * filesys.c,v 1.24 2002/01/25 09:23:52 fna Exp
 *
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 *
 *
 * Copyright 1990,1991,1992 Eric R. Smith.
 * Copyright 1992,1993,1994 Atari Corp.
 * All rights reserved.
 *
 */

/*
 * various file system interface things
 */

#include "hostfs.h"
#include "mint/assert.h"
#include "mint/string.h"
#include "mint/errno.h"
#include "mint/ctype.h"
#include "mint/credentials.h"

#if 1
#define PATH2COOKIE_DB(x) TRACE(x)
#else
#define PATH2COOKIE_DB(x) DEBUG(x)
#endif


long
getxattr(FILESYS *fs, fcookie *fc, XATTR *xattr)
{
	STAT stat;
	long r;

	assert(fs->fsflags & FS_EXT_3);

	r = xfs_stat64(fs, fc, &stat);
	if (!r)
	{
		xattr->mode	= stat.mode;
		xattr->index	= stat.ino;
		xattr->dev	= stat.dev;
		xattr->rdev	= stat.rdev;
		xattr->nlink	= stat.nlink;
		xattr->uid	= stat.uid;
		xattr->gid	= stat.gid;
		xattr->size	= stat.size;
		xattr->blksize	= stat.blksize;
		xattr->nblocks	= (stat.blksize < 512) ? stat.blocks :
					stat.blocks / (stat.blksize >> 9);

		SET_XATTR_TD(xattr,m,stat.mtime.time);
		SET_XATTR_TD(xattr,a,stat.atime.time);
		SET_XATTR_TD(xattr,c,stat.ctime.time);
		xattr->attr	= 0;

		/* fake attr field a little bit */
		if (S_ISDIR(stat.mode))
			xattr->attr = FA_DIR;
		else if (!(stat.mode & 0222))
			xattr->attr = FA_RDONLY;

		xattr->reserved2 = 0;
		xattr->reserved3[0] = 0;
		xattr->reserved3[1] = 0;
	}

	return r;
}

long
getstat64(FILESYS *fs, fcookie *fc, STAT *stat)
{
	XATTR xattr;
	long r;

	assert(fs->getxattr);

	r = xfs_getxattr(fs, fc, &xattr);
	if (!r)
	{
		stat->dev	= xattr.dev;
		stat->ino	= xattr.index;
		stat->mode	= xattr.mode;
		stat->nlink	= xattr.nlink;
		stat->uid	= xattr.uid;
		stat->gid	= xattr.gid;
		stat->rdev	= xattr.rdev;

		/* no native UTC extension
		 * -> convert to unix UTC
		 */
		stat->atime.high_time = 0;
		stat->atime.time = unixtime (xattr.atime, xattr.adate) + timezone;
		stat->atime.nanoseconds = 0;

		stat->mtime.high_time = 0;
		stat->mtime.time = unixtime (xattr.mtime, xattr.mdate) + timezone;
		stat->mtime.nanoseconds = 0;

		stat->ctime.high_time = 0;
		stat->ctime.time = unixtime (xattr.ctime, xattr.cdate) + timezone;
		stat->ctime.nanoseconds = 0;

		stat->size	= xattr.size;
		stat->blocks	= (xattr.blksize < 512) ? xattr.nblocks :
					xattr.nblocks * (xattr.blksize >> 9);
		stat->blksize	= xattr.blksize;

		stat->flags	= 0;
		stat->gen	= 0;

		bzero(stat->res, sizeof(stat->res));
	}

	return r;
}


/*
 * routines for parsing path names
 */

/*
 * relpath2cookie converts a TOS file name into a file cookie representing
 * the directory the file resides in, and a character string representing
 * the name of the file in that directory. The character string is
 * copied into the "lastname" array. If lastname is NULL, then the cookie
 * returned actually represents the file, instead of just the directory
 * the file is in.
 *
 * note that lastname, if non-null, should be big enough to contain all the
 * characters in "path", since if the file system doesn't want the kernel
 * to do path name parsing we may end up just copying path to lastname
 * and returning the current or root directory, as appropriate
 *
 * "relto" is the directory relative to which the search should start.
 * if you just want the current directory, use path2cookie instead.
 *
 */

long _cdecl
relpath2cookie(struct proc *p, fcookie *relto, const char *path, char *lastname,
	       fcookie *res, int depth)
{
	char newpath[16];

	struct cwd *cwd = p->p_cwd;

	char temp2[PATH_MAX];
	char linkstuff[PATH_MAX];

	fcookie dir;
	int drv;
	XATTR xattr;
	long r;


	/* dolast: 0 == return a cookie for the directory the file is in
	 *         1 == return a cookie for the file itself, don't follow links
	 *	   2 == return a cookie for whatever the file points at
	 */
	int dolast = 0;
	int i = 0;

	if (!path)
		return ENOTDIR;

	if (*path == '\0')
		return ENOENT;

	if (!lastname)
	{
		dolast = 1;
		lastname = temp2;
	}
	else if (lastname == follow_links)
	{
		dolast = 2;
		lastname = temp2;
	}

	*lastname = '\0';

	PATH2COOKIE_DB (("relpath2cookie(%s, dolast=%d, depth=%d [relto %p, %i])",
		path, dolast, depth, relto->fs, relto->dev));

	if (depth > MAX_LINKS)
	{
		DEBUG (("Too many symbolic links"));
		return ELOOP;
	}

	/* special cases: CON:, AUX:, etc. should be converted to U:\DEV\CON,
	 * U:\DEV\AUX, etc.
	 */
# if 1
	if (path[0] && path[1] && path[2] && (path[3] == ':') && !path[4])
# else
	if (strlen (path) == 4 && path[3] == ':')
# endif
	{
		strcpy(newpath, "U:\\DEV\\");
		newpath[7] = path[0];
		newpath[8] = path[1];
		newpath[9] = path[2];

		if ((path[0] == 'N' || path[0] == 'n') &&
		    (path[1] == 'U' || path[1] == 'u') &&
		    (path[2] == 'L' || path[2] == 'l'))
		{
			/* the device file is u:\dev\null */
			newpath[10] = 'l';
			newpath[11] = '\0';
		} else
		if ((path[0] == 'M' || path[0] == 'm') &&
		    (path[1] == 'I' || path[1] == 'i') &&
		    (path[2] == 'D' || path[2] == 'd'))
		{
			/* the device file is u:\dev\midi */
			newpath[10] = 'i';
			newpath[11] = '\0';
		} else {
			/* add the NULL terminator */
			newpath[10] = '\0';
		}

		path = newpath;
	}

	/* first, check for a drive letter
	 *
	 * BUG: a '\' at the start of a symbolic link is relative to the
	 * current drive of the process, not the drive the link is located on
	 */
	/* The check if the process runs chroot used to be inside the
	 * conditional (triggering ENOTDIR for drive specs.  IMHO its
	 * cleaner to interpret it as a regular filename instead.
	 * Rationale: The path "c:/auto" is actually the same as
	 * "/c:/auto".  If the process' root directory is not "/" but
	 * maybe "/home/ftp" then we should interpret the same filename
	 * now as "/home/ftp/c:/auto".
	 */
	if (path[2] != ':' && path[1] == ':' && !cwd->root_dir)
	{
		char c = tolower ((int)path[0] & 0xff);

		if (c >= 'a' && c <= 'z')
			drv = c - 'a';
		else if (c >= '1' && c <= '6')
			drv = 26 + (c - '1');
		else
			goto nodrive;

# if 1
		/* if root_dir is set drive references are forbidden
		 */
		if (cwd->root_dir)
			return ENOTDIR;
# endif

		path += 2;

		/* remember that we saw a drive letter
		 */
		i = 1;
	}
	else
	{
nodrive:
		drv = cwd->curdrv;
	}

	/* see if the path is rooted from '\\'
	 */
	if (DIRSEP (*path))
	{
		while (DIRSEP (*path))
			path++;

		/* if root_dir is set this is our start point
		 */
		if (cwd->root_dir)
			dup_cookie (&dir, &cwd->rootdir);
		else
			dup_cookie (&dir, &cwd->root[drv]);
	}
	else
	{
		if (i)
		{
			/* an explicit drive letter was given
			 */
			dup_cookie (&dir, &cwd->curdir[drv]);
		}
		else
		{
			PATH2COOKIE_DB (("relpath2cookie: using relto (%p, %li, %i) for dir", relto->fs, relto->index, relto->dev));
			dup_cookie (&dir, relto);
		}
	}

	if (!dir.fs && !cwd->root_dir)
	{
		dup_cookie (&dir, &cwd->root[drv]);
	}

	if (!dir.fs)
	{
		DEBUG (("relpath2cookie: no file system: returning ENXIO"));
		return ENXIO;
	}

	/* here's where we come when we've gone across a mount point
	 */
restart_mount:

	if (!*path)
	{
		/* nothing more to do
		 */
		PATH2COOKIE_DB (("relpath2cookie: no more path, returning 0"));

		*res = dir;
		return 0;
	}


	if (dir.fs->fsflags & FS_KNOPARSE)
	{
		if (!dolast)
		{
			PATH2COOKIE_DB (("fs is a KNOPARSE, nothing to do"));

			strncpy (lastname, path, PATH_MAX-1);
			lastname[PATH_MAX - 1] = 0;
			r = 0;
			*res = dir;
		}
		else
		{
			PATH2COOKIE_DB (("fs is a KNOPARSE, calling lookup"));

			r = xfs_lookup (dir.fs, &dir, path, res);
			if (r == EMOUNT)
			{
				/* hmmm... a ".." at a mount point, maybe
				 */
				fcookie mounteddir;

				r = xfs_root (dir.fs, dir.dev, &mounteddir);
				if (r == 0 && drv == UNIDRV)
				{
					if (dir.fs == mounteddir.fs
						&& dir.index == mounteddir.index
						&& dir.dev == mounteddir.dev)
					{
						release_cookie (&dir);
						release_cookie (&mounteddir);
						dup_cookie (&dir, &cwd->root[UNIDRV]);
						DEBUG(("path2cookie: restarting from mount point"));
						goto restart_mount;
					}
				}
				else
				{
					if (r == 0)
						release_cookie (&mounteddir);

					r = 0;
				}
			}

			release_cookie (&dir);
		}

		PATH2COOKIE_DB (("relpath2cookie(2): returning %ld", r));
		return r;
	}


	/* parse all but (possibly) the last component of the path name
	 *
	 * rules here: at the top of the loop, &dir is the cookie of
	 * the directory we're in now, xattr is its attributes, and res is
	 * unset at the end of the loop, &dir is unset, and either r is
	 * nonzero (to indicate an error) or res is set to the final result
	 */
	r = xfs_getxattr (dir.fs, &dir, &xattr);
	if (r)
	{
		DEBUG (("couldn't get directory attributes"));
		release_cookie (&dir);
		return EINTERNAL;
	}

	while (*path)
	{
		/* we must have a directory, since there are more things
		 * in the path
		 */
		if (!S_ISDIR(xattr.mode))
		{
			PATH2COOKIE_DB (("relpath2cookie: not a directory, returning ENOTDIR"));
			release_cookie (&dir);
			r = ENOTDIR;
			break;
		}

#if 0
		/* we must also have search permission for the directory
		 */
		if (denyaccess (p->p_cred->ucr, &xattr, S_IXOTH))
		{
			DEBUG (("search permission in directory denied"));
			release_cookie (&dir);
			/* r = ENOTDIR; */
			r = EACCES;
			break;
		}
#endif

		/* skip slashes
		 */
		while (DIRSEP (*path))
			path++;

		/* next, peel off the next name in the path
		 */
		{
			register int len;
			register char c, *s;

			len = 0;
			s = lastname;
			c = *path;
			while (c && !DIRSEP (c))
			{
				if (len++ < PATH_MAX)
					*s++ = c;
				c = *++path;
			}

			*s = '\0';
		}

		/* if there are no more names in the path, and we don't want
		 * to actually look up the last name, then we're done
		 */
		if (dolast == 0)
		{
			register const char *s = path;

			while (DIRSEP (*s))
				s++;
				
			if (!*s) {
				PATH2COOKIE_DB (("relpath2cookie: no more path, breaking"));
				*res = dir;
				PATH2COOKIE_DB (("relpath2cookie: *res = [%p, %i]", res->fs, res->dev));
				break;
			}
		}

		if (cwd->root_dir)
		{
			if (samefile (&dir, &cwd->rootdir)
				&& lastname[0] == '.'
				&& lastname[1] == '.'
				&& lastname[2] == '\0')
			{
				PATH2COOKIE_DB (("relpath2cookie: can't leave root [%s] -> forward to '.'", cwd->root_dir));

				lastname[1] = '\0';
			}
		}

		PATH2COOKIE_DB (("relpath2cookie: looking up [%s]", lastname));

		r = xfs_lookup (dir.fs, &dir, lastname, res);
		if (r == EMOUNT)
		{
			fcookie mounteddir;

			r = xfs_root (dir.fs, dir.dev, &mounteddir);
			if (r == 0 && drv == UNIDRV)
			{
				if (samefile (&dir, &mounteddir))
				{
					release_cookie (&dir);
					release_cookie (&mounteddir);
					dup_cookie (&dir, &cwd->root[UNIDRV]);
					TRACE(("path2cookie: restarting from mount point"));
					goto restart_mount;
				}
				else if (r == 0)
				{
					r = EINTERNAL;
					release_cookie (&mounteddir);
					release_cookie (&dir);
					break;
				}
			}
			else if (r == 0)
			{
				*res = mounteddir;
			}
			else
			{
				release_cookie (&dir);
				break;
			}
		}
		else if (r)
		{
			release_cookie (&dir);
			break;
		}

		/* read the file attribute
		 */
		r = xfs_getxattr (res->fs, res, &xattr);
		if (r != 0)
		{
			DEBUG (("path2cookie: couldn't get file attributes"));
			release_cookie (&dir);
			release_cookie (res);
			break;
		}

		/* check for a symbolic link
		 * - if the file is a link, and we're following links, follow it
		 */
		if (S_ISLNK(xattr.mode) && (*path || dolast > 1))
		{
			{
				r = xfs_readlink (res->fs, res, linkstuff, PATH_MAX);
				release_cookie (res);
				if (r)
				{
					DEBUG (("error reading symbolic link"));
					release_cookie (&dir);
					break;
				}
				r = relpath2cookie (p, &dir, linkstuff, follow_links, res, depth + 1);
				release_cookie (&dir);
				if (r)
				{
					DEBUG (("error following symbolic link"));
					break;
				}
			}
			dir = *res;
			xfs_getxattr (res->fs, res, &xattr);
		}
		else
		{
			TRACE(("relpath2cookie: lookup ok, mode 0x%x", xattr.mode));

			release_cookie (&dir);
			dir = *res;
		}
	}

	PATH2COOKIE_DB (("relpath2cookie(3): returning %ld", r));
	return r;
}

long _cdecl
path2cookie(struct proc *p, const char *path, char *lastname, fcookie *res)
{
	struct cwd *cwd = p->p_cwd;

	/* AHDI sometimes will keep insisting that a media change occured;
	 * we limit the number of retrys to avoid hanging the system
	 */
# define MAX_TRYS 4
	int trycnt = MAX_TRYS - 1;

	fcookie *dir = &cwd->curdir[cwd->curdrv];
	long r;

restart:
	r = relpath2cookie(p, dir, path, lastname, res, 0);
	if (r == ECHMEDIA && trycnt--)
	{
		DEBUG(("path2cookie: restarting due to media change"));
		goto restart;
	}

	return r;
}

/*
 * release_cookie: tell the file system owner that a cookie is no
 * longer in use by the kernel
 *
 * release_cookie doesn't release anymore unless there is no entry in
 * the cookie cache.  Otherwise, we just let the cookie get released
 * through clobber_cookie when the cache fills or is killed through the
 * routine above - EKL
 */
void _cdecl
release_cookie (fcookie *fc)
{
	if (fc)
	{
		FILESYS *fs;

		fs = fc->fs;
		if (fs && fs->release)
			xfs_release (fs, fc);
	}
}

/*
 * Make a new cookie (newc) which is a duplicate of the old cookie
 * (oldc). This may be something the file system is interested in,
 * so we give it a chance to do the duplication; if it doesn't
 * want to, we just copy.
 */

void
dup_cookie (fcookie *newc, fcookie *oldc)
{
	FILESYS *fs;

	fs = oldc->fs;
	if (fs && fs->dupcookie)
		xfs_dupcookie (fs, newc, oldc);
	else
		*newc = *oldc;
}


/*
 * check to see that a file is a directory, and that write permission
 * is granted; return an error code, or 0 if everything is ok.
 */
long
dir_access(struct ucred *cred, fcookie *dir, ushort perm, ushort *mode)
{
	XATTR xattr;
	long r;

	r = xfs_getxattr(dir->fs, dir, &xattr);
	if (r)
	{
		DEBUG(("dir_access: file system returned %ld", r));
		return r;
	}

	if (!S_ISDIR(xattr.mode))
	{
		DEBUG(("file is not a directory"));
		return ENOTDIR;
	}

#if 0
	if (denyaccess(cred, &xattr, perm))
	{
		DEBUG(("no permission for directory"));
		return EACCES;
	}
#endif

	*mode = xattr.mode;

	return 0;
}

/*
 * returns 1 if the given name contains a wildcard character
 */
int
has_wild(const char *name)
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
 *	"." and ".." are more or less left alone
 *	"*.*" is recognized as a special pattern, for which dest is set
 *	to just "*"
 * Long names are truncated. Any extensions after the first one are
 * ignored, i.e. foo.bar.c -> foo.bar, foo.c.bar->foo.c.
 */
void
copy8_3(char *dest, const char *src)
{
	char fill = ' ', c;
	int i;

	if (src[0] == '.')
	{
		if (src[1] == 0)
		{
			strcpy(dest, ".       .   ");
			return;
		}

		if (src[1] == '.' && src[2] == 0)
		{
			strcpy(dest, "..      .   ");
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
int
pat_match(const char *name, const char *template)
{
	char expname[TOS_NAMELEN+1];
	register char *s;
	register char c;

	if (*template == '*')
		return 1;

	copy8_3(expname, name);

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
 * int samefile(fcookie *a, fcookie *b): returns 1 if the two cookies
 * refer to the same file or directory, 0 otherwise
 */
int
samefile(fcookie *a, fcookie *b)
{
	if (a->fs == b->fs && a->dev == b->dev && a->index == b->index)
		return 1;

	return 0;
}
