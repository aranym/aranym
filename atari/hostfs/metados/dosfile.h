/*
 * The ARAnyM MetaDOS driver.
 *
 * 2002 STan
 *
 * Based on:
 * dosfile.h,v 1.7 2001/06/13 20:21:14 fna Exp
 *
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 */

# ifndef _dosfile_h
# define _dosfile_h

long _cdecl sys_f_open (MetaDOSFile const char *name, short mode);
long _cdecl sys_f_create (MetaDOSFile const char *name, short attrib);
long _cdecl sys_f_close (MetaDOSFile short fd);
long _cdecl sys_f_read (MetaDOSFile short fd, long count, char *buf);
long _cdecl sys_f_write (MetaDOSFile short fd, long count, const char *buf);
long _cdecl sys_f_seek (MetaDOSFile long place, short fd, short how);
long _cdecl sys_f_dup (MetaDOSFile short fd);
long _cdecl sys_f_force (MetaDOSFile short newh, short oldh);
long _cdecl sys_f_datime (MetaDOSFile ushort *timeptr, short fd, short wflag);
long _cdecl sys_f_lock (MetaDOSFile short fd, short mode, long start, long length);
long _cdecl sys_f_cntl (MetaDOSFile short fd, long arg, short cmd);
long _cdecl sys_f_select (MetaDOSFile unsigned short timeout, long *rfdp, long *wfdp, long *xfdp);
long _cdecl sys_f_midipipe (MetaDOSFile short pid, short in, short out);
long _cdecl sys_f_fchown (MetaDOSFile short fd, short uid, short gid);
long _cdecl sys_f_fchmod (MetaDOSFile short fd, ushort mode);
long _cdecl sys_f_seek64 (MetaDOSFile llong place, short fd, short how, llong *newpos);
#if 0
long _cdecl sys_f_poll (POLLFD *fds, ulong nfds, ulong timeout);
#endif

long _cdecl sys_ffstat (MetaDOSFile short fd, struct stat *st);
#if 0
long _cdecl sys_fwritev (MetaDOSFile short fd, const struct iovec *iov, long niov);
long _cdecl sys_freadv (MetaDOSFile short fd, const struct iovec *iov, long niov);
#endif


# endif /* _dosfile_h */
