#ifndef WIN32_SUPP_H
#define WIN32_SUPP_H

#if defined OS_mingw || defined OS_cygwin
#define _WIN32_VERSION 0x501
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <errno.h>

int win32_errno_from_oserr(DWORD oserrno, int deferrno = EINVAL);
inline void win32_set_errno(int deferrno = EINVAL) { errno = win32_errno_from_oserr(GetLastError(), deferrno); }
const char *win32_errstring(DWORD err);

#endif

#if defined OS_mingw

#include <fcntl.h>
#include <sys/stat.h>

#define S_IXOTH	0
#define S_IWOTH	0
#define S_IROTH	0
#define S_IXGRP	0
#define S_IWGRP 0
#define S_IRGRP 0
#define S_ISVTX 0
#define S_ISGID 0
#define S_ISUID 0
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
#define S_IRWXG 0
#define S_IRWXO 0
#define _PC_LINK_MAX 2
/* MiNGW headers define FIONREAD, but lack ioctl() */
#undef FIONREAD

#define stat(f, s) win32_stat(f, s)
#define lstat(f, s) win32_lstat(f, s)

int readlink(const char *path, char *buf, size_t bufsiz);
int symlink(const char *oldpath, const char *newpath);
long pathconf(const char *file_name, int name);
int truncate(const char *file_name, off_t name);
int win32_lstat(const char *file_name, struct stat *buf);
int win32_stat(const char *file_name, struct stat *buf);
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

struct timespec {
  time_t tv_sec;				/* seconds */
  long   tv_nsec;				/* and nanoseconds */
};

/* linux-compatible values for fs type */
#define MSDOS_SUPER_MAGIC     0x4d44
#define NTFS_SUPER_MAGIC      0x5346544E

int statfs(const char *path, struct statfs *buf);

#endif

#endif
