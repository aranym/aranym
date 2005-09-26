/*
 * $Id$
 *
 * The ARAnyM MetaDOS driver.
 *
 * 2002 STan
 *
 * Based on:
 * dosfile.c,v 1.20 2001/06/13 20:21:14 fna Exp
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

/* DOS file handling routines */

# include "dosfile.h"
# include "libkern/libkern.h"

# include "mint/filedesc.h"
# include "mint/ioctl.h"
# include "mint/iov.h"
# include "mint/net.h"
# include "mint/fcntl.h"
# include "mint/emu_tos.h"
# include "mint/time.h"

# include "debug.h"
# include "filesys.h"
# include "k_fds.h"

# include "mintproc.h"
# include "mintfake.h"

/* wait condition for selecting processes which got collisions */
short select_coll;


long _cdecl
sys_f_open (MetaDOSFile const char *name, short mode)
{
	PROC *p = curproc;
	FILEPTR *fp = NULL;
	short fd = MIN_OPEN - 1;
# if O_GLOBAL
	int global = 0;
#endif
	long ret;

	TRACE (("Fopen(%s, %x)", name, mode));

# if O_GLOBAL
	if (mode & O_GLOBAL)
	{
		if (p->p_cred->ucr->euid)
		{
			DEBUG (("Fopen(%s): O_GLOBAL denied for non root"));
			return EPERM;
		}

		/* from now the sockets are clean */
		if (!stricmp (name, "u:\\dev\\socket"))
		{
			ALERT ("O_GLOBAL for sockets denied; update your network tools");
			return EINVAL;
		}

		ALERT ("Opening a global handle (%s)", name);

		p = rootproc;
		global = 1;
	}
# endif

	/* make sure the mode is legal */
	mode &= O_USER;

	/* note: file mode 3 is reserved for the kernel;
	 * for users, transmogrify it into O_RDWR (mode 2)
	 */
	if ((mode & O_RWMODE) == O_EXEC)
		mode = (mode & ~O_RWMODE) | O_RDWR;

	assert (p->p_fd && p->p_cwd);

	ret = FD_ALLOC (p, &fd, MIN_OPEN);
	if (ret) goto error;

	ret = FP_ALLOC (p, &fp);
	if (ret) goto error;

	ret = do_open (&fp, name, mode, 0, NULL);
	if (ret) goto error;

	/* activate the fp, default is to close non-standard files on exec */
	FP_DONE (p, fp, fd, FD_CLOEXEC);

# if O_GLOBAL
	if (global)
		/* we just opened a global handle */
		fd += 100;
# endif

	TRACE (("Fopen: returning %d", fd));
	return fd;

  error:
	if (fd >= MIN_OPEN) FD_REMOVE (p, fd);
	if (fp) { fp->links--; FP_FREE (fp); }

	return ret;
}

long _cdecl
sys_f_create (MetaDOSFile const char *name, short attrib)
{
	PROC *p = curproc;
	FILEPTR *fp = NULL;
	short fd = MIN_OPEN - 1;
	long ret;

	TRACE (("Fcreate(%s, %x)", name, attrib));

# if O_GLOBAL
	if (attrib & O_GLOBAL)
	{
		DEBUG (("Fcreate(%s): O_GLOBAL denied"));
		return EPERM;
	}
# endif

	assert (p->p_fd && p->p_cwd);

	ret = FD_ALLOC (p, &fd, MIN_OPEN);
	if (ret) goto error;

	ret = FP_ALLOC (p, &fp);
	if (ret) goto error;

	if (attrib == FA_LABEL)
	{
		char temp1[PATH_MAX];
		fcookie dir;

#ifndef ARAnyM_MetaDOS
		/* just in case the caller tries to do something with this handle,
		 * make it point to u:\dev\null
		 */
		ret = do_open (&fp, "u:\\dev\\null", O_RDWR|O_CREAT|O_TRUNC, 0, NULL);
		if (ret) goto error;
#endif // ARAnyM_MetaDOS

		ret = path2cookie (name, temp1, &dir);
		if (ret) goto error;

		ret = xfs_writelabel (dir.fs, &dir, temp1);
		release_cookie (&dir);
		if (ret) goto error;
	}
	else if (attrib & (FA_LABEL|FA_DIR))
	{
		DEBUG (("Fcreate(%s,%x): illegal attributes", name, attrib));
		ret = EACCES;
		goto error;
	}
	else
	{
		ret = do_open (&fp, name, O_RDWR|O_CREAT|O_TRUNC, attrib, NULL);
		if (ret)
		{
			DEBUG (("Fcreate(%s) failed, error %d", name, ret));
			goto error;
		}
	}

	/* activate the fp, default is to close non-standard files on exec */
	FP_DONE (p, fp, fd, FD_CLOEXEC);

	TRACE (("Fcreate: returning %d", fd));
	return fd;

  error:
	if (fd >= MIN_OPEN) FD_REMOVE (p, fd);
	if (fp) { fp->links--; FP_FREE (fp); }
	return ret;
}

long _cdecl
sys_f_close (MetaDOSFile short fd)
{
	PROC *p = curproc;
	FILEPTR *f;
	long r;

	TRACE (("Fclose: %d", fd));

	r = GETFILEPTR (&p, &fd, &f);
	if (r) return r;

	r = do_close (p, f);

	/* XXX do this before do_close? */
	FD_REMOVE (p, fd);

# if 0
	/* standard handles should be restored to default values
	 * in TOS domain!
	 *
	 * XXX: why?
	 */
	if (p->domain == DOM_TOS)
	{
		f = NULL;

		if (fd == 0 || fd == 1)
			f = p->p_fd->ofiles[-1];
		else if (fd == 2 || fd == 3)
			f = p->p_fd->ofiles[-fd];

		if (f)
		{
			FP_DONE (p, f, fd, 0);
			f->links++;
		}
	}
# endif

	return r;
}

long _cdecl
sys_f_read (MetaDOSFile short fd, long count, char *buf)
{
	PROC *p = curproc;
	FILEPTR *f;
	long r;

	r = GETFILEPTR (&p, &fd, &f);
	if (r) return r;

	if ((f->flags & O_RWMODE) == O_WRONLY)
	{
		DEBUG (("Fread: read on a write-only handle"));
		return EACCES;
	}

	if (is_terminal (f))
		return tty_read (f, buf, count);

	TRACELOW (("Fread: %ld bytes from handle %d to %lx", count, fd, buf));
	return xdd_read (f, buf, count);
}

long _cdecl
sys_f_write (MetaDOSFile short fd, long count, const char *buf)
{
	PROC *p = curproc;
	FILEPTR *f;
	long r;

	r = GETFILEPTR (&p, &fd, &f);
	if (r) return r;

	if ((f->flags & O_RWMODE) == O_RDONLY)
	{
		DEBUG (("Fwrite: write on a read-only handle"));
		return EACCES;
	}

	if (is_terminal (f))
		return tty_write (f, buf, count);

	/* Prevent broken device drivers from wiping the disk.
	 * We return a zero rather than a negative error code
	 * to help programs those don't handle GEMDOS errors
	 * returned by Fwrite()
	 */
	if (count <= 0)
	{
		DEBUG (("Fwrite: invalid count: %d", count));
		return 0;
	}

	/* it would be faster to do this in the device driver, but this
	 * way the drivers are easier to write
	 */
	if (f->flags & O_APPEND)
	{
		r = xdd_lseek (f, 0L, SEEK_END);
		/* ignore errors from unseekable files (e.g. pipes) */
		if (r == EACCES)
			r = 0;
	} else
		r = 0;

	if (r >= 0)
	{
		TRACELOW (("Fwrite: %ld bytes to handle %d", count, fd));
		r = xdd_write (f, buf, count);
	}

	if (r < 0)
		DEBUG (("Fwrite: error %ld", r));

	return r;
}

long _cdecl
sys_f_seek (MetaDOSFile long place, short fd, short how)
{
	PROC *p = curproc;
	FILEPTR *f;
	long r;

	TRACE (("Fseek(%ld, %d) on handle %d", place, how, fd));

	r = GETFILEPTR (&p, &fd, &f);
	if (r) return r;

	if (is_terminal (f))
		return 0;

	r = xdd_lseek (f, place, how);
	TRACE (("Fseek: returning %ld", r));
	return r;
}


long _cdecl
sys_f_datime (MetaDOSFile ushort *timeptr, short fd, short wflag)
{
	PROC *p = curproc;
	FILEPTR *f;
	long r;

	TRACE (("%s(%i)", __FUNCTION__, fd));

	r = GETFILEPTR (&p, &fd, &f);
	if (r) return r;

	/* some programs use Fdatime to test for TTY devices */
	if (is_terminal (f))
		return EACCES;

	if (f->fc.fs && f->fc.fs->fsflags & FS_EXT_3)
	{
		ulong t = 0;
		/* long r; */

		if (wflag)
			t = unixtime (timeptr [0], timeptr [1]) + timezone;

		r = xdd_datime (f, (ushort *) &t, wflag);

		if (!r && !wflag)
			*(long *) timeptr = dostime (t - timezone);

		return r;
	}

	return xdd_datime (f, timeptr, wflag);
}

