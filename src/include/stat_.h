#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif

#ifdef HAVE_STAT_H
# include <stat.h>
#endif

#ifdef __LINUX_GLIBC_WRAP_H

#if __GLIBC_PREREQ(2, 33)

#ifdef __cplusplus
extern "C" {
#endif

/*
 * avoid references to stat/lstat/fstat, which is only available in glibc >= 2.33
 */

extern int __fxstat(int __ver, int __fildes, struct stat *__stat_buf)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (3)));
extern int __xstat(int __ver, const char *__filename,
      struct stat *__stat_buf) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2, 3)));
extern int __lxstat(int __ver, const char *__filename,
       struct stat *__stat_buf) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2, 3)));
extern int __fxstatat (int __ver, int __fildes, const char *__filename,
         struct stat *__stat_buf, int __flag)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (3, 4)));

extern __inline __attribute__((__gnu_inline__))
__attribute__((__nothrow__))
int stat(const char *__path, struct stat *__statbuf)
{
	return __xstat(1, __path, __statbuf);
}

extern __inline __attribute__((__gnu_inline__))
__attribute__((__nothrow__)) 
int lstat (const char *__path, struct stat *__statbuf)
{
	return __lxstat(1, __path, __statbuf);
}

extern __inline __attribute__((__gnu_inline__))
__attribute__((__nothrow__)) 
int fstat(int __fd, struct stat *__statbuf)
{
	return __fxstat(1, __fd, __statbuf);
}

extern __inline __attribute__((__gnu_inline__))
__attribute__((__nothrow__)) 
int fstatat(int __fd, const char *__filename, struct stat *__statbuf, int __flag)
{
	return __fxstatat(1, __fd, __filename, __statbuf, __flag);
}

#ifdef __cplusplus
}
#endif

#endif

#endif
