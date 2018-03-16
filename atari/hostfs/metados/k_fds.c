/*
 * The ARAnyM MetaDOS driver.
 *
 * 2002 STan
 *
 * Based on:
 * k_fds.c,v 1.10 2001/06/13 20:21:20 fna Exp
 *
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 *
 *
 * Copyright 2000 Frank Naumann <fnaumann@freemint.de>
 * All rights reserved.
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *
 * Author: Frank Naumann <fnaumann@freemint.de>
 * Started: 2001-01-12
 *
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 *
 */

#include "hostfs.h"
#include "mint/errno.h"
#include "mint/fcntl.h"
#include "mint/credentials.h"
#include "mint/assert.h"
#include "mint/string.h"


/* do_open(f, name, rwmode, attr, x)
 *
 * f      - pointer to FIELPTR *
 * name   - file name
 * rwmode - file access mode
 * attr   - TOS attributes for created files (if applicable)
 * x      - filled in with attributes of opened file (can be NULL)
 */
long
do_open (FILEPTR **f, const char *name, int rwmode, int attr, XATTR *x)
{
	struct proc *p = get_curproc();

	fcookie dir, fc;
	long devsp = 0;
	DEVDRV *dev;
	long r;
	XATTR xattr;
	unsigned perm;
	int creating, exec_check;
	char temp1[PATH_MAX];
	short cur_gid, cur_egid;

	TRACE (("do_open(%s)", name));

	/*
	 * first step: get a cookie for the directory
	 */
	r = path2cookie (p, name, temp1, &dir);
	if (r)
	{
		DEBUG (("do_open(%s): error %ld", name, r));
		return r;
	}

	/*
	 * If temp1 is a NULL string, then use the name again.
	 *
	 * This can occur when trying to open the ROOT directory of a
	 * drive. i.e. /, or C:/ or D:/ etc.
	 */
	if (*temp1 == '\0') {
		strncpy(temp1, name, PATH_MAX);
	}

	/*
	 * second step: try to locate the file itself
	 */
	r = relpath2cookie (p, &dir, temp1, follow_links, &fc, 0);

# ifdef CREATE_PIPES
	/*
	 * file found: this is an error if (O_CREAT|O_EXCL) are set
	 *	...or if this is Fcreate with nonzero attr on the pipe filesystem
	 */
	if ((r == 0) && ((rwmode & (O_CREAT|O_EXCL)) == (O_CREAT|O_EXCL) ||
			(attr && fc.fs == &pipe_filesys &&
			(rwmode & (O_CREAT|O_TRUNC)) == (O_CREAT|O_TRUNC))))
# else
	/*
	 * file found: this is an error if (O_CREAT|O_EXCL) are set
	 */
	if ((r == 0) && ((rwmode & (O_CREAT|O_EXCL)) == (O_CREAT|O_EXCL)))
# endif
	{
		DEBUG (("do_open(%s): file already exists", name));
		release_cookie (&fc);
		release_cookie (&dir);

		return EACCES;
	}

	/* file not found: maybe we should create it
	 * note that if r != 0, the fc cookie is invalid (so we don't need to
	 * release it)
	 */
	if (r == ENOENT && (rwmode & O_CREAT))
	{
		/* check first for write permission in the directory */
		r = xfs_getxattr (dir.fs, &dir, &xattr);
#if 0
		if (r == 0)
		{
			if (denyaccess (p->p_cred->ucr, &xattr, S_IWOTH))
				r = EACCES;
		}
#endif

		if (r)
		{
			DEBUG(("do_open(%s): couldn't get "
			      "write permission on directory", name));
			release_cookie (&dir);
			return r;
		}

		assert (p->p_cred);

		/* fake gid if directories setgid bit set */
		cur_gid = p->p_cred->rgid;
		cur_egid = p->p_cred->ucr->egid;

		if (xattr.mode & S_ISGID)
		{
			p->p_cred->rgid = xattr.gid;

			p->p_cred->ucr = copy_cred (p->p_cred->ucr);
			p->p_cred->ucr->egid = xattr.gid;
		}

		assert (p->p_cwd);

		r = xfs_creat (dir.fs, &dir, temp1,
			(S_IFREG|DEFAULT_MODE) & (~p->p_cwd->cmask), attr, &fc);

		p->p_cred->rgid = cur_gid;
		p->p_cred->ucr->egid = cur_egid;

		if (r)
		{
			DEBUG(("do_open(%s): error %ld while creating file",
				name, r));
			release_cookie (&dir);
			return r;
		}

		creating = 1;
	}
	else if (r)
	{
		DEBUG(("do_open(%s): error %ld while searching for file",
			name, r));
		release_cookie (&dir);
		return r;
	}
	else
	{
		creating = 0;
	}

	/* check now for permission to actually access the file
	 */
	r = xfs_getxattr (fc.fs, &fc, &xattr);
	if (r)
	{
		DEBUG(("do_open(%s): couldn't get file attributes", name));
		release_cookie (&dir);
		release_cookie (&fc);
		return r;
	}

	DEBUG(("do_open(%s): mode 0x%x", name, xattr.mode));

	/* we don't do directories
	 */
	if (S_ISDIR(xattr.mode))
	{
		DEBUG(("do_open(%s): file is a directory", name));
		release_cookie (&dir);
		release_cookie (&fc);
		return ENOENT;
	}

	exec_check = 0;
	switch (rwmode & O_RWMODE)
	{
		case O_WRONLY:
			perm = S_IWOTH;
			break;
		case O_RDWR:
			perm = S_IROTH|S_IWOTH;
			break;
		case O_EXEC:
			if (fc.fs->fsflags & FS_NOXBIT)
				perm = S_IROTH;
			else
			{
				perm = S_IXOTH;

				assert (p->p_cred);

				if (p->p_cred->ucr->euid == 0)
					exec_check = 1;	/* superuser needs 1 x bit */
			}
			break;
		case O_RDONLY:
			perm = S_IROTH;
			break;
		default:
			break;
	}

	/* access checking;  additionally, the superuser needs at least one
	 * execute right to execute a file
	 */
	if ((exec_check && ((xattr.mode & (S_IXUSR|S_IXGRP|S_IXOTH)) == 0))
		|| (!creating && 0 /* denyaccess (p->p_cred->ucr, &xattr, perm) */))
	{
		DEBUG(("do_open(%s): access to file denied", name));
		release_cookie (&dir);
		release_cookie (&fc);
		return EACCES;
	}

	/* an extra check for write access -- even the superuser shouldn't
	 * write to files with the FA_RDONLY attribute bit set (unless,
	 * we just created the file, or unless the file is on the proc
	 * file system and hence FA_RDONLY has a different meaning)
	 */
	if (!creating && (xattr.attr & FA_RDONLY))
	{
		if ((rwmode & O_RWMODE) == O_RDWR || (rwmode & O_RWMODE) == O_WRONLY)
		{
			DEBUG(("do_open(%s): can't write a read-only file", name));
			release_cookie (&dir);
			release_cookie (&fc);
			return EACCES;
		}
	}

	/* if writing to a setuid or setgid file, clear those bits
	 */
	if ((perm & S_IWOTH) && (xattr.mode & (S_ISUID|S_ISGID)))
	{
		xattr.mode &= ~(S_ISUID|S_ISGID);
		xfs_chmode (fc.fs, &fc, (xattr.mode & ~S_IFMT));
	}

	/* If the caller asked for the attributes of the opened file, copy them over.
	 */
	if (x) *x = xattr;

	/* So far, so good. Let's get the device driver now, and try to
	 * actually open the file.
	 */
	dev = xfs_getdev (fc.fs, &fc, &devsp);
	if (!dev)
	{
		DEBUG (("do_open(%s): device driver not found (%li)", name, devsp));
		release_cookie (&dir);
		release_cookie (&fc);
		return devsp ? devsp : EINTERNAL;
	}

	assert (f && *f);

	if (dev == &fakedev)
	{
		/* fake BIOS devices */
		FILEPTR *fp;

		assert (p->p_fd);

		fp = p->p_fd->ofiles[devsp];
		if (!fp || fp == (FILEPTR *) 1)
			return EBADF;

		(*f)->links--;
		FP_FREE (*f);

		*f = fp;
		fp->links++;

		release_cookie (&dir);
		release_cookie (&fc);

		return 0;
	}

	(*f)->links = 1;
	(*f)->flags = rwmode;
	(*f)->pos = 0;
	(*f)->devinfo = devsp;
	(*f)->fc = fc;
	(*f)->dev = dev;
	release_cookie (&dir);

	r = xdd_open (*f);
	if (r < E_OK)
	{
		DEBUG(("do_open(%s): device open failed with error %ld", name, r));
		release_cookie (&fc);
		return r;
	}

	DEBUG(("do_open(%s) -> 0", name));
	return 0;
}




/*
 * helper function for do_close: this closes the indicated file pointer which
 * is assumed to be associated with process p. The extra parameter is necessary
 * because f_midipipe mucks with file pointers of other processes, so
 * sometimes p != curproc.
 *
 * Note that the function changedrv() in filesys.c can call this routine.
 * in that case, f->dev will be 0 to represent an invalid device, and
 * we cannot call the device close routine.
 */

long
do_close (struct proc *p, FILEPTR *f)
{
	long r = E_OK;

	if (!f) return EBADF;
	if (f == (FILEPTR *) 1)
		return E_OK;

	/* if this file is "select'd" by this process, unselect it
	 * (this is just in case we were killed by a signal)
	 */

	f->links--;
	if (f->links < 0)
	{
		DEBUG(("do_close on invalid file struct! (links = %i)", f->links));
		/*		return 0; */
	}

	if (f->dev)
	{
		r = xdd_close (f, p->pid);
		if (r) DEBUG (("close: device close failed"));
	}

	if (f->links <= 0)
	{
		release_cookie (&f->fc);
		FP_FREE (f);
	}

	return  r;
}
