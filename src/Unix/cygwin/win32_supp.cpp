/*
	NatFeat host CD-ROM access, Win32 CD-ROM driver

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


#ifndef ERROR_MEDIA_CHECK
# define ERROR_MEDIA_CHECK 679
#endif

struct errentry
{
	DWORD oscode;					/* OS return value */
	int errnocode;					/* unix error code */
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

	for (i = 0; i < sizeof(errtable)/sizeof(errtable[0]); ++i)
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
