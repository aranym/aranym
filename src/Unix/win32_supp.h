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

wchar_t *win32_utf8_to_widechar(const char *name);
char *win32_widechar_to_utf8(const wchar_t *wname);

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
#define S_ISVTX 0001000 /* save swapped text even after use */
#endif
#ifndef S_ISGID
#define S_ISGID 0002000 /* set group id on execution */
#endif
#ifndef S_ISUID
#define S_ISUID 0004000 /* set user id on execution */
#endif
#ifndef S_IFLNK
#define S_IFLNK 0xa000	/* symbolic link */
#endif
#ifndef S_ISLNK
#define S_ISLNK(m)    (((m)&_S_IFMT) == S_IFLNK)
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

#ifndef O_BINARY
# ifdef _O_BINARY
#   define O_BINARY _O_BINARY
# else
#   define O_BINARY 0
# endif
#endif

#ifndef __MINGW_NOTHROW
#define __MINGW_NOTHROW
#endif

#ifndef CC_FOR_BUILD

#undef stat
#undef wstat
#undef fstat
#undef truncate
#undef ftruncate

#if defined(_FILE_OFFSET_BITS) && (_FILE_OFFSET_BITS == 64)
#ifdef _USE_32BIT_TIME_T
#define stat _stati64
#define wstat _wstati64
#undef _stati64
#else
#define stat _stat64
#define wstat _wstat64
#endif
#else
#ifdef __MINGW64_VERSION_MAJOR
#define stat _stat32
#define wstat _wstat32
#else
#define stat _stat
#define wstat _wstat
#endif
#endif

int win32_stat(const char *file_name, struct stat *buf);
#ifndef __WIN32_SUPP_IMPLEMENTATION
EXTERN_INLINE int stat(const char *file_name, struct stat *buf)
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

char *win32_realpath(const char *path, char *resolved);
#define realpath(path, resolved) win32_realpath(path, resolved)
#define HAVE_REALPATH 1

#define opendir win32_opendir
#define readdir win32_readdir
#define closedir win32_closedir
#define rewinddir win32_rewinddir
#define telldir win32_telldir
#define seekdir win32_seekdir

DIR *opendir(const char *szPath);
struct dirent *readdir(DIR *_dirp);
int closedir(DIR *_dirp);
void rewinddir(DIR *_dirp);
long telldir(DIR *_dirp);
void seekdir(DIR *_dirp, long lPos);

int win32_execv(const char *path, char *const *argv);

#define open win32_open
int __MINGW_NOTHROW open (const char*, int, ...);

#define unlink win32_unlink
#define os_remove win32_unlink
int __MINGW_NOTHROW unlink (const char*);

#define rmdir win32_rmdir
int __MINGW_NOTHROW rmdir (const char*);

#define mkdir win32_mkdir
int __MINGW_NOTHROW mkdir (const char*, ...);

#define chmod win32_chmod
int __MINGW_NOTHROW chmod (const char*, int);

#define rename win32_rename
int __MINGW_NOTHROW rename(const char*, const char*);

#define getcwd win32_getcwd
char * __MINGW_NOTHROW getcwd(char*, int);

#define statfs win32_statfs

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

int statfs(const char *path, struct statfs *buf);

/* linux-compatible values for fs type */
#define MSDOS_SUPER_MAGIC     0x4d44
#define NTFS_SUPER_MAGIC      0x5346544E

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

int win32_futimens(int fd, const struct timespec ts[2]);
#define futimens(fd, ts) win32_futimens(fd, ts)
#define HAVE_FUTIMENS 1

int win32_futimes(int fd, const struct timeval tv[2]);
#define futimes(fd, tv) win32_futimes(fd, tv)
#define HAVE_FUTIMES 1

#endif /* CC_FOR_BUILD */

#ifdef __cplusplus
}
#endif

#else

#define os_remove remove

#endif /* _WIN32 */

#endif
