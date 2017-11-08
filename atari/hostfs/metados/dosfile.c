/*
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

#include "hostfs.h"
#include "mint/ioctl.h"


long _cdecl
sys_f_open (MetaDOSFile const char *name, short mode)
{
	struct proc *p = get_curproc();
	FILEPTR *fp = NULL;
	short fd = MIN_OPEN - 1;
	long ret;

	TRACE (("Fopen(%s, %x)", name, mode));

# if O_GLOBAL
	if (mode & O_GLOBAL)
	{
	   DEBUG(("O_GLOBAL is obsolete, please update your driver (%s)",name));
	   return EINVAL;
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
	struct proc *p = get_curproc();
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

#if 0
		/* just in case the caller tries to do something with this handle,
		 * make it point to u:\dev\null
		 */
		ret = do_open (&fp, "u:\\dev\\null", O_RDWR|O_CREAT|O_TRUNC, 0, NULL);
		if (ret) goto error;
#endif

		ret = path2cookie (p, name, temp1, &dir);
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
			DEBUG (("Fcreate(%s) failed, error %ld", name, ret));
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
	struct proc *p = get_curproc();
	FILEPTR *f;
	long r;

	DEBUG (("Fclose: %d", fd));

# ifdef WITH_SINGLE_TASK_SUPPORT
	/* this is for pure-debugger:
	 * some progs call Fclose(-1) when they exit which would
	 * cause pd to lose keyboard
	 */
	if( fd < 0 && (p->modeflags & M_SINGLE_TASK) )
	{
		DEBUG(("Fclose:return 0 for negative fd in singletask-mode."));
		return 0;
	}
# endif
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
	struct proc *p = get_curproc();
	FILEPTR *f;
	long r;

	r = GETFILEPTR (&p, &fd, &f);
	if (r) return r;

	if ((f->flags & O_RWMODE) == O_WRONLY)
	{
		DEBUG (("Fread: read on a write-only handle"));
		return EACCES;
	}

	if (f->flags & O_DIRECTORY)
	{
		DEBUG (("Fread(%i): read on a directory", fd));
		return EISDIR;
	}

#if 0
	if (is_terminal (f))
		return tty_read (f, buf, count);
#endif

	TRACELOW (("Fread: %ld bytes from handle %d to %p", count, fd, buf));
	return xdd_read (f, buf, count);
}

long _cdecl
sys_f_write (MetaDOSFile short fd, long count, const char *buf)
{
	struct proc *p = get_curproc();
	FILEPTR *f;
	long r;

	r = GETFILEPTR (&p, &fd, &f);
	if (r) return r;

	if ((f->flags & O_RWMODE) == O_RDONLY)
	{
		DEBUG (("Fwrite: write on a read-only handle"));
		return EACCES;
	}

#if 0
	if (is_terminal (f))
		return tty_write (f, buf, count);
#endif

	/* Prevent broken device drivers from wiping the disk.
	 * We return a zero rather than a negative error code
	 * to help programs those don't handle GEMDOS errors
	 * returned by Fwrite()
	 */
	if (count <= 0)
	{
		DEBUG (("Fwrite: invalid count: %ld", count));
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
	struct proc *p = get_curproc();
	FILEPTR *f;
	long r;

	TRACE (("Fseek(%ld, %d) on handle %d", place, how, fd));

	r = GETFILEPTR (&p, &fd, &f);
	if (r) return r;

#if 0
	if (is_terminal (f))
		return ESPIPE;
#endif

	r = xdd_lseek (f, place, how);
	TRACE (("Fseek: returning %ld", r));
	return r;
}


long _cdecl
sys_f_datime (MetaDOSFile ushort *timeptr, short fd, short wflag)
{
	struct proc *p = get_curproc();
	FILEPTR *f;
	long r;

	TRACE (("%s(%i)", __FUNCTION__, fd));

	r = GETFILEPTR (&p, &fd, &f);
	if (r) return r;

#if 0
	/* some programs use Fdatime to test for TTY devices */
	if (is_terminal (f))
		return EACCES;
#endif

	if (f->fc.fs && f->fc.fs->fsflags & FS_EXT_3)
	{
		unsigned long ut = 0;
		if (wflag)
			ut = unixtime(timeptr[0], timeptr[1]) + timezone;
		r = xdd_datime(f, (ushort *)&ut, wflag);
		if (!r && !wflag) {
			ut = dostime(ut - timezone);
			timeptr[1] = (unsigned short)ut;
			ut >>= 16;
			timeptr[0] = (unsigned short)ut;
		}
		return r;
	}

	return xdd_datime (f, timeptr, wflag);
}

/*
 * extensions to GEMDOS:
 */

static long
sys__ffstat_1_12 (struct file *f, XATTR *xattr)
{
	long ret;

#if 0
# ifdef OLDSOCKDEVEMU
	if (f->dev == &sockdev || f->dev == &sockdevemu)
# else
	if (f->dev == &sockdev)
# endif
		return so_fstat_old (f, xattr);
#endif

	if (!f->fc.fs)
	{
		DEBUG (("sys__ffstat_1_12: no xfs!"));
		return ENOSYS;
	}

	ret = xdd_ioctl(f, FSTAT, xattr);
	if (ret == ENOSYS)
		ret = xfs_getxattr (f->fc.fs, &f->fc, xattr);
	if ((ret == E_OK) && (f->fc.fs->fsflags & FS_EXT_3))
	{
		xtime_to_local_dos(xattr,m);
		xtime_to_local_dos(xattr,a);
		xtime_to_local_dos(xattr,c);
	}

	return ret;
}

static long
sys__ffstat_1_16 (struct file *f, struct stat *st)
{
	long ret;

#if 0
# ifdef OLDSOCKDEVEMU
	if (f->dev == &sockdev || f->dev == &sockdevemu)
# else
	if (f->dev == &sockdev)
# endif
		return so_fstat (f, st);
#endif

	if (!f->fc.fs)
	{
		DEBUG (("sys__ffstat_1_16: no xfs"));
		return ENOSYS;
	}

	ret = xdd_ioctl(f, FSTAT64, st);
	if (ret == ENOSYS)
		ret = xfs_stat64 (f->fc.fs, &f->fc, st);
	return ret;
}

long _cdecl
sys_ffstat (MetaDOSFile short fd, struct stat *st)
{
	struct proc *p = get_curproc();
	FILEPTR	*f;
	long ret;

	ret = GETFILEPTR (&p, &fd, &f);
	if (ret) return ret;

	return sys__ffstat_1_16 (f, st);
}

/*
 * f_cntl: a combination "ioctl" and "fcntl". Some functions are
 * handled here, if they apply to the file descriptors directly
 * (e.g. F_DUPFD) or if they're easily translated into file system
 * functions (e.g. FSTAT). Others are passed on to the device driver
 * via dev->ioctl.
 */

long _cdecl
sys_f_cntl (MetaDOSFile short fd, long arg, short cmd)
{
	struct proc *p = get_curproc();
	FILEPTR	*f;
	long r;

	TRACE (("Fcntl(%i, cmd=0x%x)", fd, cmd));

	if (cmd == F_DUPFD || cmd == F_DUPFD_CLOEXEC) {
#if 0
  		return do_dup (fd, arg, cmd == F_DUPFD_CLOEXEC ? 1: 0);
#else
		return ENOSYS;
#endif
	}

	TRACE(("Fcntl getfileptr"));
	r = GETFILEPTR (&p, &fd, &f);
	TRACE(("Fcntl r = %lx", r));
	if (r) return r;

	switch (cmd)
	{
		case F_GETFD:
			TRACE (("Fcntl F_GETFD"));
#if 0
			return p->p_fd->ofileflags[fd];
#else
			return 0;
#endif
		case F_SETFD:
			TRACE (("Fcntl F_SETFD"));
#if 0
			p->p_fd->ofileflags[fd] = arg;
#endif
			return E_OK;
		case F_GETFL:
			TRACE (("Fcntl F_GETFL"));
			return (f->flags & O_USER);
		case F_SETFL:
			TRACE (("Fcntl F_SETFL"));

			/* make sure only user bits set */
			arg &= O_USER;

			/* make sure the file access and sharing modes are not changed */
			arg &= ~(O_RWMODE | O_SHMODE);
			arg |= f->flags & (O_RWMODE | O_SHMODE);

			/* set user bits to arg */
			f->flags &= ~O_USER;
			f->flags |= arg;

			return E_OK;
		case FSTAT:
			TRACE (("Fcntl FSTAT (%i, %lx) on \"%s\" -> %li", fd, arg, xfs_name (&(f->fc)), r));
			return sys__ffstat_1_12 (f, (XATTR *) arg);
		case FSTAT64:
			TRACE (("Fcntl FSTAT64 (%i, %lx) on \"%s\" -> %li", fd, arg, xfs_name(&(f->fc)), r));
			return sys__ffstat_1_16 (f, (struct stat *) arg);
		case FUTIME:
			TRACE (("Fcntl FUTIME"));
			if (f->fc.fs && (f->fc.fs->fsflags & FS_EXT_3) && arg)
			{
				MUTIMBUF *buf = (MUTIMBUF *) arg;
				ulong t [2];

				t[0] = unixtime (buf->actime, buf->acdate) + timezone;
				t[1] = unixtime (buf->modtime, buf->moddate) + timezone;
				return xdd_ioctl (f, FUTIME_UTC, (void *) t);
			}
			break;
	}

	/* fall through to device ioctl */

	TRACE (("Fcntl mode %x: calling ioctl", cmd));
#if 0
	if (is_terminal (f))
	{
		/* tty in the middle of a hangup? */
		while (((struct tty *) f->devinfo)->hup_ospeed)
			sleep (IO_Q, (long) &((struct tty *) f->devinfo)->state);

		if (cmd == FIONREAD
			|| cmd == FIONWRITE
			|| cmd == TIOCSTART
			|| cmd == TIOCSTOP
			|| cmd == TIOCSBRK
			|| cmd == TIOCFLUSH)
		{
			r = tty_ioctl (f, cmd, (void *) arg);
		}
		else
		{
			r = (*f->dev->ioctl)(f, cmd, (void *) arg);
			if (r == ENOSYS)
				r = tty_ioctl (f, cmd, (void *) arg);
		}
	}
	else
#endif
		r = xdd_ioctl (f, cmd, (void *) arg);

	return r;
}


/*
 * GEMDOS extension: Ffchown(fh, uid, gid) changes the user and group
 * ownerships of a open file to "uid" and "gid" respectively.
 */

long _cdecl
sys_f_fchown (MetaDOSFile short fd, short uid, short gid)
{
	struct proc *p = get_curproc();
	FILEPTR *f;
	long r;

	TRACE (("Ffchown(%d,%i,%i)", fd, uid, gid));

	r = GETFILEPTR (&p, &fd, &f);
	if (r) return r;

	if (!(f->fc.fs))
	{
		DEBUG (("Ffchown: not a valid filesystem"));
		return ENOSYS;
	}

	if ((f->flags & O_RWMODE) == O_RDONLY)
	{
		DEBUG (("Ffchown: write on a read-only handle"));
		return EPERM;
	}

	/* MiNT acts like _POSIX_CHOWN_RESTRICTED: a non-privileged process
	 * can only change the ownership of a file that is owned by this
	 * user, to the effective group id of the process or one of its
	 * supplementary groups
	 */
#if 0
	if (p->p_cred->ucr->euid)
	{
		XATTR xattr;

		if (p->p_cred->ucr->egid != gid && !groupmember (p->p_cred->ucr, gid))
			r = EACCES;
		else
			r = xfs_getxattr (f->fc.fs, &(f->fc), &xattr);

		if (r)
		{
			DEBUG (("Ffchown(%i): unable to get file attributes", fd));
			return r;
		}

		if (xattr.uid != p->p_cred->ucr->euid || xattr.uid != uid)
		{
			DEBUG (("Ffchown(%i): not the file's owner", fd));
			return EACCES;
		}

		r = xfs_chown (f->fc.fs, &(f->fc), uid, gid);

		/* POSIX 5.6.5.2: if name refers to a regular file the
		 * set-user-ID and set-group-ID bits of the file mode shall
		 * be cleared upon successful return from the call to chown,
		 * unless the call is made by a process with the appropriate
		 * privileges. Note that POSIX leaves the behaviour
		 * unspecified for all other file types. At least for
		 * directories with BSD-like setgid semantics, these bits
		 * should be left unchanged.
		 */
		if (!r && !S_ISDIR(xattr.mode)
			&& (xattr.mode & (S_ISUID | S_ISGID)))
		{
			long s;

			s = xfs_chmode (f->fc.fs, &(f->fc), xattr.mode & ~(S_ISUID | S_ISGID));
			if (!s)
				DEBUG (("Ffchown: chmode returned %ld (ignored)", s));
		}
	}
	else
#endif
		r = xfs_chown (f->fc.fs, &(f->fc), uid, gid);

	return r;
}

/*
 * GEMDOS extension: Fchmod (fh, mode) changes a file's access
 * permissions on a open file.
 */

long _cdecl
sys_f_fchmod (MetaDOSFile short fd, ushort mode)
{
	struct proc *p = get_curproc();
	FILEPTR *f;
	long r;
	XATTR xattr;

	TRACE (("Ffchmod(%i, %i)", fd, mode));

	r = GETFILEPTR (&p, &fd, &f);
	if (r) return r;

	if (!(f->fc.fs))
	{
		DEBUG (("Ffchmod: not a valid filesystem"));
		return ENOSYS;
	}

	r = xfs_getxattr (f->fc.fs, &(f->fc), &xattr);
	if (r)
	{
		DEBUG (("Ffchmod(%i): couldn't get file attributes", fd));
	}
#if 0
	else if (p->p_cred->ucr->euid && p->p_cred->ucr->euid != xattr.uid)
	{
		DEBUG (("Ffchmod(%i): not the file's owner", fd));
		r = EACCES;
	}
#endif
	else
	{
		r = xfs_chmode (f->fc.fs, &(f->fc), mode & ~S_IFMT);
		if (r)
			DEBUG (("Ffchmod: error %ld", r));
	}

	return r;
}

