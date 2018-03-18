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


#define __WIN32_SUPP_IMPLEMENTATION
#include "sysdeps.h"
#include "maptab.h"
#include <wchar.h>

#include "win32_supp.h"

#define DEBUG 0
#include "debug.h"

#if defined(_WIN32) || defined(__CYGWIN__)

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


extern "C" int vasprintf(char **, const char *, va_list);

void guialert(const char *fmt, ...)
{
	va_list args;
	char *buf = NULL;
	int ret;
	
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	fputs("\n", stderr);
	va_end(args);
	va_start(args, fmt);
	ret = vasprintf(&buf, fmt, args);
	va_end(args);
	if (ret >= 0)
	{
		MessageBoxA(NULL, buf, "ARAnyM: error", MB_ICONSTOP);
		free(buf);
	}
}



wchar_t *win32_utf8_to_widechar(const char *name)
{
	size_t len;
	unsigned short ch;
	unsigned char c;
	wchar_t *wname, *dst;
	const char *src;
	
	if (name == NULL)
		return NULL;
	len = strlen(name);
	wname = (wchar_t *)malloc((len + 1) * sizeof(*wname));
	if (wname == NULL)
		return NULL;
	dst = wname;
	src = name;
	while (*src)
	{
		c = *src++;
		ch = c;
		if (ch < 0x80)
		{
		} else if ((ch & 0xe0) == 0xc0)
		{
			ch = ((ch & 0x1f) << 6) | (src[1] & 0x3f);
		} else
		{
			ch = ((((ch & 0x0f) << 6) | (src[1] & 0x3f)) << 6) | (src[2] & 0x3f);
		}
		*dst++ = ch;
	}
	*dst = 0;
	return wname;
}


char *win32_widechar_to_utf8(const wchar_t *wname)
{
	unsigned short ch;
	char *name, *dst;
	const wchar_t *src;
	size_t len;
	
	if (wname == NULL)
		return NULL;
	len = wcslen(wname);
	name = (char *)malloc((len * 3 + 1) * sizeof(*name));
	if (name == NULL)
		return NULL;
	dst = name;
	src = wname;
	while (*src)
	{
		ch = *src++;
		if (ch < 0x80)
		{
			*dst++ = ch;
		} else if (ch < 0x800)
		{
			*dst++ = ((ch >> 6) & 0x3f) | 0xc0;
			*dst++ = (ch & 0x3f) | 0x80;
		} else 
		{
			*dst++ = ((ch >> 12) & 0x0f) | 0xe0;
			*dst++ = ((ch >> 6) & 0x3f) | 0x80;
			*dst++ = (ch & 0x3f) | 0x80;
		}
	}
	*dst++ = 0;
	name = (char *)realloc(name, (dst - name) * sizeof(*name));
	return name;
}


#ifdef _WIN32


#undef stat
#undef wstat
#undef fstat
#undef lstat


#if defined(_FILE_OFFSET_BITS) && (_FILE_OFFSET_BITS == 64)
#ifdef _USE_32BIT_TIME_T
#define stat _stati64
#define wstat _wstati64
#define fstat _fstati64
#undef _stati64
#else
#define stat _stat64
#define wstat _wstat64
#define fstat _fstat64
#endif
#else
#ifdef __MINGW64_VERSION_MAJOR
#define stat _stat32
#define wstat _wstat32
#define fstat _fstat32
#else
#define stat _stat
#define wstat _wstat
#define fstat _fstat
#endif
#endif


extern "C" int readlink(const char *path, char *buf, size_t bufsiz)
{
	UNUSED(path);
	UNUSED(buf);
	UNUSED(bufsiz);
	errno = ENOSYS;
	return -1;
}

extern "C" int symlink(const char *oldpath, const char *newpath)
{
	UNUSED(oldpath);
	UNUSED(newpath);
	errno = ENOSYS;
	return -1;
}


extern "C" long pathconf(const char *file_name, int name)
{
	UNUSED(file_name);
	if (name == _PC_LINK_MAX)
		return 0x7fffffffL;
	errno = ENOSYS;
	return -1;
}


extern "C" int win32_truncate(const char *pathname, off_t len)
{
	int ret, err;
	wchar_t *wname = win32_utf8_to_widechar(pathname);
	
	int fd = _wopen(wname, _O_BINARY | _O_RDWR);
	free(wname);
	if (fd == -1)
		return fd;
	ret = win32_ftruncate(fd, len);
	err = errno;
	_close(fd);
	errno = err;
	return ret;
}


extern "C" int win32_ftruncate(int fd, off_t length)
{
/*
 * _chsize_s is not declared in MinGW32 headers, and missing in import libraries
 * it is also not available in Windows XP msvcrt.dll
 */
	return _chsize(fd, length);
}


static int orig_stat(const char *filename, struct stat *buf)
{
	wchar_t *wname = win32_utf8_to_widechar(filename);
	int res = wstat(wname, buf);
	free(wname);
	return res;
}


static int orig_lstat(const char *filename, struct stat *buf)
{
	return orig_stat(filename, buf);
}


extern "C" int win32_fstat(int fd, struct stat *st)
{
	return fstat(fd, st);
}


extern "C" int win32_stat(const char *name, struct stat *st)
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
		char fixed_name[PATH_MAX + 1] = {0};
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


extern "C" int win32_lstat(const char *file, struct stat *sbuf)
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
				lstat_result = win32_stat(file, sbuf);
			}
		}
	}
	D(bug("lstat %s: %d", file, lstat_result));
	return lstat_result;
}


extern "C" int win32_futimes(int fd, const struct timeval tv[2])
{
	struct timespec ts[2];
	ts[0].tv_sec = tv[0].tv_sec;
	ts[0].tv_nsec = tv[0].tv_usec * 1000;
	ts[1].tv_sec = tv[1].tv_sec;
	ts[1].tv_nsec = tv[1].tv_usec * 1000;
	return futimens(fd, ts);
}


extern "C" int win32_futimens(int fd, const struct timespec ts[2])
{
	FILETIME ac, mod;
	ULARGE_INTEGER i;
	HANDLE h;
	
/* 100ns difference between Windows and UNIX timebase. */
#define FACTOR (0x19db1ded53e8000ULL)
#define set_filetime(ft, t) \
	i.QuadPart = (t.tv_sec * 1000000000ULL + t.tv_nsec) / 100UL + FACTOR; \
	ft.dwLowDateTime = i.u.LowPart; \
	ft.dwHighDateTime = i.u.HighPart
	set_filetime(ac, ts[0]);
	set_filetime(mod, ts[1]);
#undef set_filetime
	h = (HANDLE)_get_osfhandle(fd);
	if (!SetFileTime(h, NULL, &ac, &mod))
	{
		errno = win32_errno_from_oserr(GetLastError());
		return -1;
	}
	return 0;
}


/**
 * @author Prof. A Olowofoyeku (The African Chief)
 * @author Frank Heckenbach
 * @see http://gd.tuwien.ac.at/gnu/mingw/os-hacks.h
 */
extern "C" int statfs(const char *path, struct statfs *buf)
{
	wchar_t tmp[MAX_PATH];
	wchar_t resolved_path[MAX_PATH];
	int retval = 0;
	struct stat s;
	
	DWORD sectors_per_cluster;
	DWORD bytes_per_sector;
	DWORD f_bavail;
	DWORD f_blocks;
	DWORD fsid;
	DWORD namelen;
	
	errno = 0;

	wchar_t *wpath = win32_utf8_to_widechar(path);
	GetFullPathNameW(wpath, MAX_PATH, resolved_path, NULL);
	free(wpath);
	
	/*
	 * Some broken programs (like TeraDesk) seem to call
	 * Dfree() on the filename instead of the directory name
	 */
	if (wstat(resolved_path, &s) == 0 &&
		!S_ISDIR(s.st_mode))
	{
		wchar_t *p = wcsrchr(resolved_path, '\\');
		if (*p)
			*p = '\0';
	}
	
	if (!GetDiskFreeSpaceW(resolved_path, &sectors_per_cluster,
					  &bytes_per_sector, &f_bavail, &f_blocks))
	{
		win32_set_errno(ENOENT);
		retval = -1;
	} else
	{
		buf->f_bavail = f_bavail;
		buf->f_blocks = f_blocks;
		buf->f_bsize = sectors_per_cluster * bytes_per_sector;
		buf->f_files = buf->f_blocks;
		buf->f_ffree = buf->f_bavail;
		buf->f_bfree = buf->f_bavail;
	}

	if (wcschr(resolved_path, ':') != NULL)
		resolved_path[3] = '\0';		/* we want only the root */

	/* get the FS volume information */
	if (GetVolumeInformationW
		(resolved_path, NULL, 0, &fsid, &namelen, NULL, tmp,
		 MAX_PATH))
	{
		buf->f_fsid = fsid;
		buf->f_namelen = namelen;
		if (wcsicmp(L"NTFS", tmp) == 0)
		{
			buf->f_type = NTFS_SUPER_MAGIC;
		} else
		{
			buf->f_type = MSDOS_SUPER_MAGIC;
		}
	} else
	{
		win32_set_errno(ENOENT);
		retval = -1;
	}
	return retval;
}


extern "C" char *win32_realpath(const char *path, char *resolved)
{
	int path_max = PATH_MAX;
	wchar_t *tmp = (wchar_t *)malloc(path_max * sizeof(*tmp));
	wchar_t *wpath = win32_utf8_to_widechar(path);
	if (GetFullPathNameW(wpath, path_max, tmp, NULL) == 0)
	{
		resolved = NULL;
	} else
	{
		char *utf8 = win32_widechar_to_utf8(tmp);
		if (resolved == NULL)
		{
			resolved = utf8;
		} else
		{
			strcpy(resolved, utf8);
			free(utf8);
		}
	}
	free(tmp);
	return resolved;
}


extern "C" int win32_open(const char *pathname, int flags, ...)
{
	wchar_t *wname;
	int res;
	
	mode_t m = 0;
	if (flags & O_CREAT)
	{
		va_list args;
		va_start(args, flags);
		m = va_arg(args, int);
		va_end(args);
	}
	wname = win32_utf8_to_widechar(pathname);
	res = _wopen(wname, flags, m);
	free(wname);
	return res;
}


extern "C" int win32_unlink(const char *pathname)
{
	wchar_t *wname;
	int res;
	
	wname = win32_utf8_to_widechar(pathname);
	res = _wunlink(wname);
	free(wname);
	return res;
}


extern "C" int win32_rmdir(const char *pathname)
{
	wchar_t *wname;
	int res;
	
	wname = win32_utf8_to_widechar(pathname);
	res = _wrmdir(wname);
	free(wname);
	return res;
}


extern "C" int win32_mkdir(const char *pathname, ...)
{
	wchar_t *wname;
	int res;
	
	wname = win32_utf8_to_widechar(pathname);
	res = _wmkdir(wname);
	free(wname);
	return res;
}


extern "C" int win32_chmod(const char *pathname, int mode)
{
	wchar_t *wname;
	int res;
	
	wname = win32_utf8_to_widechar(pathname);
	res = _wchmod(wname, mode);
	free(wname);
	return res;
}


extern "C" int win32_rename(const char *oldpath, const char *newpath)
{
	wchar_t *woldpath, *wnewpath;
	int res;
	
	woldpath = win32_utf8_to_widechar(oldpath);
	wnewpath = win32_utf8_to_widechar(newpath);
	res = _wrename(woldpath, wnewpath);
	free(wnewpath);
	free(woldpath);
	return res;
}


extern "C" char *win32_getcwd(char *buf, int size)
{
	wchar_t wbuf[MAX_PATH];
	char *name;
	
	if (!buf)
	{
		errno = EFAULT;
		return NULL;
	}
	if (_wgetcwd(wbuf, MAX_PATH) == NULL)
		return NULL;
	name = win32_widechar_to_utf8(wbuf);
	strncpy(buf, name, size);
	free(name);
	return buf;
}


/*
 * dirent.c
 * Borrowed from MinGW, modified to work with utf8 filenames.
 * Note that the routines work with a _WDIR structure, but return a DIR *.
 * If you expect you can peek in the DIR structure, you loose.
 */

#define SUFFIX	L"*"
#define	SLASH	L"\\"

/*
 * opendir
 *
 * Returns a pointer to a DIR structure appropriately filled in to begin
 * searching a directory.
 */
DIR *win32_opendir(const char *szPath)
{
	_WDIR *nd;
	unsigned int rc;
	wchar_t szFullPath[MAX_PATH];

	errno = 0;

	if (!szPath)
	{
		errno = EFAULT;
		return (DIR *) 0;
	}
	if (szPath[0] == '\0')
	{
		errno = ENOTDIR;
		return (DIR *) 0;
	}
	/* Attempt to determine if the given path really is a directory. */
	rc = GetFileAttributes(szPath);
	if (rc == INVALID_FILE_ATTRIBUTES)
	{
		/* call GetLastError for more error info */
		win32_set_errno(ENOENT);
		return (DIR *) 0;
	}
	if (!(rc & FILE_ATTRIBUTE_DIRECTORY))
	{
		/* Error, entry exists but not a directory. */
		errno = ENOTDIR;
		return (DIR *) 0;
	}
	/* Make an absolute pathname.  */
	wchar_t *wpath = win32_utf8_to_widechar(szPath);
	GetFullPathNameW(wpath, MAX_PATH, szFullPath, NULL);
	free(wpath);

	/* Allocate enough space to store DIR structure and the complete
	   * directory path given. */
	nd = (_WDIR *) malloc(sizeof(*nd) + (wcslen(szFullPath) + wcslen(SLASH) + wcslen(SUFFIX) + 1) * sizeof(nd->dd_name[0]));

	if (!nd)
	{
		/* Error, out of memory. */
		errno = ENOMEM;
		return (DIR *) 0;
	}
	/* Create the search expression. */
	wcscpy(nd->dd_name, szFullPath);

	/* Add on a slash if the path does not end with one. */
	if (nd->dd_name[0] != '\0' &&
		nd->dd_name[wcslen(nd->dd_name) - 1] != '/' &&
		nd->dd_name[wcslen(nd->dd_name) - 1] != '\\')
	{
		wcscat(nd->dd_name, SLASH);
	}
	/* Add on the search pattern */
	wcscat(nd->dd_name, SUFFIX);

	/* Initialize handle to -1 so that a premature closedir doesn't try
	   * to call _findclose on it. */
	nd->dd_handle = -1;

	/* Initialize the status. */
	nd->dd_stat = 0;

	/* Initialize the dirent structure. ino and reclen are invalid under
	   * Win32, and name simply points at the appropriate part of the
	   * findfirst_t structure. */
	nd->dd_dir.d_ino = 0;
	nd->dd_dir.d_reclen = 0;
	nd->dd_dir.d_namlen = 0;
	nd->dd_dir.d_name[0] = 0;

	return (DIR *)nd;
}


/*
 * readdir
 *
 * Return a pointer to a dirent structure filled with the information on the
 * next entry in the directory.
 */
struct dirent *win32_readdir(DIR *_dirp)
{
	_WDIR *dirp = (_WDIR *)_dirp;
	errno = 0;
	
	/* Check for valid DIR struct. */
	if (!dirp)
	{
		errno = EFAULT;
		return (struct dirent *) 0;
	}
	if (dirp->dd_stat < 0)
	{
		/* We have already returned all files in the directory
		   * (or the structure has an invalid dd_stat). */
		return (struct dirent *) 0;
	} else if (dirp->dd_stat == 0)
	{
		/* We haven't started the search yet. */
		/* Start the search */
		dirp->dd_handle = _wfindfirst(dirp->dd_name, &(dirp->dd_dta));

		if (dirp->dd_handle == -1)
		{
			/* Whoops! Seems there are no files in that
			   * directory. */
			dirp->dd_stat = -1;
		} else
		{
			dirp->dd_stat = 1;
		}
	} else
	{
		/* Get the next search entry. */
		if (_wfindnext(dirp->dd_handle, &(dirp->dd_dta)))
		{
			/* We are off the end or otherwise error.
			   _findnext sets errno to ENOENT if no more file
			   Undo this. */
			DWORD winerr = GetLastError();

			if (winerr == ERROR_NO_MORE_FILES)
				errno = 0;
			_findclose(dirp->dd_handle);
			dirp->dd_handle = -1;
			dirp->dd_stat = -1;
		} else
		{
			/* Update the status to indicate the correct
			   * number. */
			dirp->dd_stat++;
		}
	}

	if (dirp->dd_stat > 0)
	{
		/* Successfully got an entry. Everything about the file is
		 * already appropriately filled in except the length of the
		 * file name.
		 */
		/*
		 * convert the name from widechar to utf8 and copy it,
		 * This is safe because dirent and _wdirent are identical
		 * except for dd_name, which is the last field
		 */
		char *name = win32_widechar_to_utf8(dirp->dd_dta.name);
		dirp->dd_dir.d_namlen = strlen(name);
		strcpy((char *)dirp->dd_dir.d_name, name);
		free(name);
		return (struct dirent *) &dirp->dd_dir;
	}
	return (struct dirent *) 0;
}


/*
 * closedir
 *
 * Frees up resources allocated by opendir.
 */
int win32_closedir(DIR *_dirp)
{
	_WDIR *dirp = (_WDIR *)_dirp;
	int rc;

	errno = 0;
	rc = 0;

	if (!dirp)
	{
		errno = EFAULT;
		return -1;
	}
	if (dirp->dd_handle != -1)
	{
		rc = _findclose(dirp->dd_handle);
	}
	/* Delete the dir structure. */
	free(dirp);

	return rc;
}


/*
 * rewinddir
 *
 * Return to the beginning of the directory "stream". We simply call findclose
 * and then reset things like an opendir.
 */
void win32_rewinddir(DIR *_dirp)
{
	_WDIR *dirp = (_WDIR *)_dirp;
	errno = 0;

	if (!dirp)
	{
		errno = EFAULT;
		return;
	}
	if (dirp->dd_handle != -1)
	{
		_findclose(dirp->dd_handle);
	}
	dirp->dd_handle = -1;
	dirp->dd_stat = 0;
}


/*
 * telldir
 *
 * Returns the "position" in the "directory stream" which can be used with
 * seekdir to go back to an old entry. We simply return the value in stat.
 */
long win32_telldir(DIR *_dirp)
{
	_WDIR *dirp = (_WDIR *)_dirp;
	errno = 0;

	if (!dirp)
	{
		errno = EFAULT;
		return -1;
	}
	return dirp->dd_stat;
}


/*
 * seekdir
 *
 * Seek to an entry previously returned by telldir. We rewind the directory
 * and call readdir repeatedly until either dd_stat is the position number
 * or -1 (off the end). This is not perfect, in that the directory may
 * have changed while we weren't looking. But that is probably the case with
 * any such system.
 */
void win32_seekdir(DIR *_dirp, long lPos)
{
	_WDIR *dirp = (_WDIR *)_dirp;
	errno = 0;

	if (!dirp)
	{
		errno = EFAULT;
		return;
	}
	if (lPos < -1)
	{
		/* Seeking to an invalid position. */
		errno = EINVAL;
		return;
	} else if (lPos == -1)
	{
		/* Seek past end. */
		if (dirp->dd_handle != -1)
		{
			_findclose(dirp->dd_handle);
		}
		dirp->dd_handle = -1;
		dirp->dd_stat = -1;
	} else
	{
		/* Rewind and read forward to the appropriate index. */
		win32_rewinddir((DIR *)dirp);

		while ((dirp->dd_stat < lPos) && win32_readdir((DIR *)dirp))
			;
	}
}


int win32_execv(const char *path, char *const *argv)
{
	wchar_t *wpath = win32_utf8_to_widechar(path);
	size_t len;
	const char *cp;
	const char *const *p;
	char *buf;
	wchar_t *wbuf;
	int ret;

	len = 0;
	p = argv;
	while ((cp = *p) != NULL)
	{
		len += strlen(cp) + 1;
		if (*cp == '\0' || strchr(cp, ' ') != NULL)
			len += 2;
		p++;
	}

	buf = (char *)malloc(len);
	if (buf == NULL)
		return ENOMEM;

	p = argv;
	*buf = '\0';
	while ((cp = *p) != NULL)
	{
		if (*cp == '\0' || strchr(cp, ' ') != NULL)
		{
			strcat(buf, "\"");
			strcat(buf, cp);
			strcat(buf, "\"");
		} else
		{
			strcat(buf, cp);
		}
		p++;
		if (*p != NULL)
			strcat(buf, " ");
	}

	wbuf = win32_utf8_to_widechar(buf);
	free(buf);

	{
		STARTUPINFOW StartupInfo;
		PROCESS_INFORMATION ProcessInformation;

		StartupInfo.cb = sizeof(StartupInfo);
		GetStartupInfoW(&StartupInfo);
		StartupInfo.wShowWindow = SW_SHOWNORMAL;
		StartupInfo.dwFlags = STARTF_USESHOWWINDOW;

		if (CreateProcessW(
							  wpath, /* pointer to name of executable module */
							  wbuf, /* pointer to command line string */
							  NULL, /* pointer to process security attributes */
							  NULL, /* pointer to thread security attributes */
							  FALSE, /* handle inheritance flag */
							  0, /* creation flags */
							  NULL, /* pointer to new environment block */
							  NULL, /* pointer to current directory name */
							  & StartupInfo, /* pointer to STARTUPINFO */
							  & ProcessInformation /*  pointer to PROCESS_INFORMATION */
			))
		{
			ret = ProcessInformation.dwProcessId;
			CloseHandle(ProcessInformation.hThread);
			/* maybe needed later, when we wait for it */
			CloseHandle(ProcessInformation.hProcess);
		} else
		{
			ret = -win32_errno_from_oserr(GetLastError(), ENOENT);
		}
	}

	free(wbuf);
	free(wpath);

	return ret;
}


#endif /* _WIN32 */

#endif /* _WIN32 || __CYGWIN__ */
