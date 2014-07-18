/*
   Various posix emulation function and utilities for cygwin/win32

   ARAnyM (C) 2014 ARAnyM developer team

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


#include "win32_supp.h"

#include "sysdeps.h"

#define DEBUG 0
#include "debug.h"

#define ISSLASH(C) ((C) == '/' || (C) == '\\')

#ifndef ERROR_MEDIA_CHECK
#  define ERROR_MEDIA_CHECK 679
#endif	/* ERROR_MEDIA_CHECK */

struct errentry
{
	DWORD oscode;						/* OS return value */
	int errnocode;						/* unix error code */
};

static struct errentry errtable[] =
{
	{ ERROR_INVALID_FUNCTION, EINVAL },
	{ ERROR_FILE_NOT_FOUND, ENOENT },
	{ ERROR_PATH_NOT_FOUND, ENOENT },
	{ ERROR_TOO_MANY_OPEN_FILES, EMFILE },
	{ ERROR_ACCESS_DENIED, EACCES },
	{ ERROR_INVALID_HANDLE, EBADF },
	{ ERROR_ARENA_TRASHED, ENOMEM },
	{ ERROR_NOT_ENOUGH_MEMORY, ENOMEM },
	{ ERROR_OUTOFMEMORY, ENOMEM },
	{ ERROR_INVALID_BLOCK, ENOMEM },
	{ ERROR_BAD_ENVIRONMENT, E2BIG },
	{ ERROR_BAD_FORMAT, ENOEXEC },
	{ ERROR_INVALID_ACCESS, EINVAL },
	{ ERROR_INVALID_DATA, EINVAL },
	{ ERROR_INVALID_DRIVE, ENOENT },
	{ ERROR_CURRENT_DIRECTORY, EACCES },
	{ ERROR_NOT_SAME_DEVICE, EXDEV },
	{ ERROR_NO_MORE_FILES, ENOENT },
	{ ERROR_LOCK_VIOLATION, EACCES },
	{ ERROR_BAD_NETPATH, ENOENT },
	{ ERROR_NETWORK_ACCESS_DENIED, EACCES },
	{ ERROR_BAD_NET_NAME, ENOENT },
	{ ERROR_FILE_EXISTS, EEXIST },
	{ ERROR_CANNOT_MAKE, EACCES },
	{ ERROR_FAIL_I24, EACCES },
	{ ERROR_INVALID_PARAMETER, EINVAL },
	{ ERROR_NO_PROC_SLOTS, EAGAIN },
	{ ERROR_DRIVE_LOCKED, EACCES },
	{ ERROR_BROKEN_PIPE, EPIPE },
	{ ERROR_DISK_FULL, ENOSPC },
	{ ERROR_HANDLE_DISK_FULL, ENOSPC },
	{ ERROR_INVALID_TARGET_HANDLE, EBADF },
	{ ERROR_INVALID_HANDLE, EINVAL },
	{ ERROR_WAIT_NO_CHILDREN, ECHILD },
	{ ERROR_CHILD_NOT_COMPLETE, ECHILD },
	{ ERROR_DIRECT_ACCESS_HANDLE, EBADF },
	{ ERROR_NEGATIVE_SEEK, EINVAL },
	{ ERROR_SEEK_ON_DEVICE, EACCES },
	{ ERROR_DIR_NOT_EMPTY, ENOTEMPTY },
	{ ERROR_NOT_LOCKED, EACCES },
	{ ERROR_BAD_PATHNAME, ENOENT },
	{ ERROR_MAX_THRDS_REACHED, EAGAIN },
	{ ERROR_LOCK_FAILED, EACCES },
	{ ERROR_ALREADY_EXISTS, EEXIST },
	{ ERROR_DUP_NAME, EEXIST },
	{ ERROR_FILENAME_EXCED_RANGE, ENOENT },
	{ ERROR_NESTING_NOT_ALLOWED, EAGAIN },
	{ ERROR_NOT_ENOUGH_QUOTA, ENOMEM },
	{ ERROR_NOT_SUPPORTED, ENOSYS },
	{ ERROR_BAD_UNIT, ENXIO },
	{ ERROR_NOT_DOS_DISK, ENXIO },
	{ ERROR_NOT_READY, EBUSY },
	{ ERROR_BAD_COMMAND, ENOSYS },
	{ ERROR_CRC, EIO },
	{ ERROR_BAD_LENGTH, ERANGE },
	{ ERROR_SEEK, EIO },
	{ ERROR_SECTOR_NOT_FOUND, EIO },
	{ ERROR_OUT_OF_PAPER, EIO },
	{ ERROR_READ_FAULT, EIO },
	{ ERROR_GEN_FAILURE, EBUSY },
	{ ERROR_SHARING_VIOLATION, EBUSY },
	{ ERROR_LOCK_VIOLATION, EBUSY },
	{ ERROR_WRONG_DISK, EBUSY },
	{ ERROR_MEDIA_CHECK, EAGAIN },
	{ ERROR_MEDIA_CHANGED, EAGAIN },
};

#define MIN_EXEC_ERROR ERROR_INVALID_STARTING_CODESEG
#define MAX_EXEC_ERROR ERROR_INFLOOP_IN_RELOC_CHAIN

#define MIN_EACCES_RANGE ERROR_WRITE_PROTECT
#define MAX_EACCES_RANGE ERROR_SHARING_BUFFER_EXCEEDED


int win32_errno_from_oserr(DWORD oserrno, int deferrno)
{
	unsigned int i;

	for (i = 0; i < sizeof(errtable) / sizeof(errtable[0]); ++i)
	{
		if (oserrno == errtable[i].oscode)
		{
			return errtable[i].errnocode;
		}
	}

	if (oserrno >= MIN_EACCES_RANGE && oserrno <= MAX_EACCES_RANGE)
		return EACCES;
	if (oserrno >= MIN_EXEC_ERROR && oserrno <= MAX_EXEC_ERROR)
		return ENOEXEC;
	return deferrno;
}


const char *win32_errstring(DWORD err)
{
	static char buf[2048];
	size_t len;

	buf[0] = 0;
	FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, err, 0, buf, sizeof(buf) / sizeof(buf[0]), NULL);
	len = strlen(buf);
	while (len && (buf[len - 1] == '\n' || buf[len - 1] == '\r'))
		buf[--len] = '\0';
	return buf;
}


#ifdef OS_mingw

int readlink(const char *path, char *buf, size_t bufsiz)
{
	UNUSED(path);
	UNUSED(buf);
	UNUSED(bufsiz);
	errno = ENOSYS;
	return -1;
}

int symlink(const char *oldpath, const char *newpath)
{
	UNUSED(oldpath);
	UNUSED(newpath);
	errno = ENOSYS;
	return -1;
}


long pathconf(const char *file_name, int name)
{
	UNUSED(file_name);
	if (name == _PC_LINK_MAX)
		return 0x7fffffffL;
	errno = ENOSYS;
	return -1;
}


int truncate(const char *file_name, off_t name)
{
	UNUSED(file_name);
	UNUSED(name);
	errno = ENOSYS;
	return -1;
}


#undef stat
static int orig_stat(const char *filename, struct stat *buf)
{
	return stat(filename, buf);
}


#undef lstat
static int orig_lstat(const char *filename, struct stat *buf)
{
	return stat(filename, buf);
}


int win32_stat(const char *name, struct stat *st)
{
	int result = orig_lstat(name, st);

	if (result == -1 && errno == ENOENT)
	{
		/* Due to mingw's oddities, there are some directories (like
		   c:\) where stat() only succeeds with a trailing slash, and
		   other directories (like c:\windows) where stat() only
		   succeeds without a trailing slash.  But we want the two to be
		   synonymous, since chdir() manages either style.  Likewise, Mingw also
		   reports ENOENT for names longer than PATH_MAX, when we want
		   ENAMETOOLONG, and for stat("file/"), when we want ENOTDIR.
		   Fortunately, mingw PATH_MAX is small enough for stack
		   allocation.  */
		char fixed_name[PATH_MAX + 1] =
		{0};
		size_t len = strlen(name);
		bool check_dir = false;

		if (PATH_MAX <= len)
			errno = ENAMETOOLONG;
		else if (len)
		{
			strcpy(fixed_name, name);
			if (ISSLASH(fixed_name[len - 1]))
			{
				check_dir = true;
				while (len && ISSLASH(fixed_name[len - 1]))
					fixed_name[--len] = '\0';
				if (!len)
					fixed_name[0] = '/';
			} else
				fixed_name[len++] = '/';
			result = orig_stat(fixed_name, st);
			if (result == 0 && check_dir && !S_ISDIR(st->st_mode))
			{
				result = -1;
				errno = ENOTDIR;
			}
		}
	}
	D(bug("stat %s: %d", name, result));
	return result;
}


int win32_lstat(const char *file, struct stat *sbuf)
{
	size_t len;
	int lstat_result = win32_stat(file, sbuf);

	if (lstat_result == 0)
	{
		len = strlen(file);
		if (ISSLASH(file[len - 1]) && !S_ISDIR(sbuf->st_mode))
		{
			/* At this point, a trailing slash is only permitted on
			   symlink-to-dir; but it should have found information on the
			   directory, not the symlink.  Call stat() to get info about the
			   link's referent.  Our replacement stat guarantees valid results,
			   even if the symlink is not pointing to a directory.  */
			if (!S_ISLNK(sbuf->st_mode))
			{
				errno = ENOTDIR;
				lstat_result = -1;
			} else
			{
				lstat_result = win32_stat(file, sbuf);;
			}
		}
	}
	D(bug("lstat %s: %d", file, lstat_result));
	return lstat_result;
}


int futimes(int fd, const struct timeval tv[2])
{
	UNUSED(fd);
	UNUSED(tv);
	errno = ENOSYS;
	return -1;
}

/**
 * @author Prof. A Olowofoyeku (The African Chief)
 * @author Frank Heckenbach
 * @see http://gd.tuwien.ac.at/gnu/mingw/os-hacks.h
 */
int statfs(const char *path, struct statfs *buf)
{
	char tmp[MAX_PATH],
	 resolved_path[MAX_PATH];
	int retval = 0;

	errno = 0;

#  ifdef HAVE_REALPATH
	realpath(path, resolved_path);
#  else	/* ERROR_MEDIA_CHECK */
	strncpy(resolved_path, path, MAX_PATH);
#  endif	/* HAVE_REALPATH */

	long sectors_per_cluster,
	 bytes_per_sector;

	if (!GetDiskFreeSpaceA(resolved_path, (DWORD *) & sectors_per_cluster,
					  (DWORD *) & bytes_per_sector, (DWORD *) & buf->f_bavail,
						   (DWORD *) & buf->f_blocks))
	{
		errno = ENOENT;
		retval = -1;
	} else
	{
		buf->f_bsize = sectors_per_cluster * bytes_per_sector;
		buf->f_files = buf->f_blocks;
		buf->f_ffree = buf->f_bavail;
		buf->f_bfree = buf->f_bavail;
	}

	/* get the FS volume information */
	if (strspn(":", resolved_path) > 0)
		resolved_path[3] = '\0';		/* we want only the root */
	if (GetVolumeInformation
		(resolved_path, NULL, 0, (DWORD *) & buf->f_fsid, (DWORD *) & buf->f_namelen, NULL, tmp,
		 MAX_PATH))
	{
		if (strcasecmp("NTFS", tmp) == 0)
		{
			buf->f_type = NTFS_SUPER_MAGIC;
		} else
		{
			buf->f_type = MSDOS_SUPER_MAGIC;
		}
	} else
	{
		errno = ENOENT;
		retval = -1;
	}
	return retval;
}

#endif /* OS_mingw */
