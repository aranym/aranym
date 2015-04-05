#ifndef WIN32_SUPP_H
#define WIN32_SUPP_H

#if defined(_WIN32) || defined(__CYGWIN__)
#include "windows_ver.h"
#include <errno.h>

#ifdef __cplusplus
int win32_errno_from_oserr(DWORD oserrno, int deferrno = EINVAL);
inline void win32_set_errno(int deferrno = EINVAL) { errno = win32_errno_from_oserr(GetLastError(), deferrno); }
const char *win32_errstring(DWORD err);
#endif

#endif

#if defined _WIN32

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef S_IXOTH
#define	S_IXOTH	(S_IXGRP >> 3)	/* Execute by others.  */
#endif
#ifndef S_IWOTH
#define	S_IWOTH	(S_IWGRP >> 3)	/* Write by others.  */
#endif
#ifndef S_IROTH
#define	S_IROTH	(S_IRGRP >> 3)	/* Read by others.  */
#endif
#ifndef S_IXGRP
#define	S_IXGRP	(S_IXUSR >> 3)	/* Execute by group.  */
#endif
#ifndef S_IWGRP
#define	S_IWGRP	(S_IWUSR >> 3)	/* Write by group.  */
#endif
#ifndef S_IRGRP
#define	S_IRGRP	(S_IXUSR >> 3)	/* Execute by group.  */
#endif
#ifndef S_ISVTX
#define S_ISVTX 0
#endif
#ifndef S_ISGID
#define S_ISGID 0
#endif
#ifndef S_ISUID
#define S_ISUID 0
#endif
#define S_ISLNK(a) 0
#ifndef S_IFLNK
#define S_IFLNK 0
#endif
#ifndef O_NONBLOCK
#define O_NONBLOCK 0
#endif
#ifndef O_NOCTTY
#define O_NOCTTY 0
#endif
#ifndef S_IRWXG
#define	S_IRWXG	(S_IRWXU >> 3)
#endif
#ifndef S_IRWXO
#define	S_IRWXO	(S_IRWXG >> 3)
#endif
#define _PC_LINK_MAX 2
/* MiNGW headers define FIONREAD, but lack ioctl() */
#undef FIONREAD

#undef stat
#undef fstat
#undef truncate
#undef ftruncate

#if defined(_FILE_OFFSET_BITS) && (_FILE_OFFSET_BITS == 64)
#ifdef _USE_32BIT_TIME_T
#define stat _stati64
#undef _stati64
#else
#define stat _stat64
#endif
#else
#ifdef __MINGW64_VERSION_MAJOR
#define stat _stat32
#else
#define stat _stat
#endif
#endif

int win32_stat(const char *file_name, struct stat *buf);
#ifndef __WIN32_SUPP_IMPLEMENTATION
ALWAYS_INLINE int stat(const char *file_name, struct stat *buf)
{
	return win32_stat(file_name, buf);
}
#endif

#define lstat(f, s) win32_lstat(f, s)
#define fstat(f, s) win32_fstat(f, s)
#define truncate(f, o) win32_truncate(f, o)
#define ftruncate(f, o) win32_ftruncate(f, o)

int readlink(const char *path, char *buf, size_t bufsiz);
int symlink(const char *oldpath, const char *newpath);
long pathconf(const char *file_name, int name);
int win32_truncate(const char *pathname, off_t len);
int win32_ftruncate(int fd, off_t length);
int win32_lstat(const char *file_name, struct stat *buf);
int win32_fstat(int fd, struct stat *buf);
int futimes(int fd, const struct timeval tv[2]);
int futimens(int fd, const struct timespec ts[2]);

struct statfs
{
  long f_type;                  /* type of filesystem (see below) */
  long f_bsize;                 /* optimal transfer block size */
  long f_blocks;                /* total data blocks in file system */
  long f_bfree;                 /* free blocks in fs */
  long f_bavail;                /* free blocks avail to non-superuser */
  long f_files;                 /* total file nodes in file system */
  long f_ffree;                 /* free file nodes in fs */
  long f_fsid;                  /* file system id */
  long f_namelen;               /* maximum length of filenames */
  long f_spare[6];              /* spare for later */
};

#ifndef _TIMESPEC_DEFINED
#define _TIMESPEC_DEFINED
struct timespec {
  time_t tv_sec;				/* seconds */
  long   tv_nsec;				/* and nanoseconds */
};

struct itimerspec {
  struct timespec  it_interval;	/* Timer period */
  struct timespec  it_value;	/* Timer expiration */
};
#endif	/* _TIMESPEC_DEFINED */

/* linux-compatible values for fs type */
#define MSDOS_SUPER_MAGIC     0x4d44
#define NTFS_SUPER_MAGIC      0x5346544E

int statfs(const char *path, struct statfs *buf);

#ifdef __cplusplus
}
#endif

#endif

#endif
