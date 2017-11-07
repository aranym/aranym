/*
 * The ARAnyM MetaDOS driver.
 *
 * 2002 STan
 *
 * Based on:
 * dosdir.h,v 1.2 2001/06/13 20:21:14 fna Exp
 *
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 */

# ifndef _dosdir_h
# define _dosdir_h

#include "mint/emu_tos.h"

/* table of processes holding locks on drives */
extern PROC *dlockproc [NUM_DRIVES];
extern long searchtime;

long _cdecl sys_d_free		(MetaDOSDir long *buf, int d);
long _cdecl sys_d_create	(MetaDOSDir const char *path);
long _cdecl sys_d_delete	(MetaDOSDir const char *path);
long _cdecl sys_f_sfirst	(MetaDOSDTA const char *path, int attrib);
long _cdecl sys_f_snext		(MetaDOSDTA0);
long _cdecl sys_f_attrib	(MetaDOSFile const char *name, int rwflag, int attr);
long _cdecl sys_f_delete	(MetaDOSDir const char *name);
long _cdecl sys_f_rename	(MetaDOSDir int junk, const char *old, const char *new);
long _cdecl sys_d_pathconf	(MetaDOSDir const char *name, int which);
long _cdecl sys_d_opendir	(MetaDOSDir const char *path, int flags);
long _cdecl sys_d_readdir	(MetaDOSDir int len, long handle, char *buf);
long _cdecl sys_d_xreaddir	(MetaDOSDir int len, long handle, char *buf, XATTR *xattr, long *xret);
long _cdecl sys_d_rewind	(MetaDOSDir long handle);
long _cdecl sys_d_closedir	(MetaDOSDir long handle);
long _cdecl sys_f_xattr		(MetaDOSFile int flag, const char *name, XATTR *xattr);
long _cdecl sys_f_link		(MetaDOSDir const char *old, const char *new);
long _cdecl sys_f_symlink	(MetaDOSDir const char *old, const char *new);
long _cdecl sys_f_readlink	(MetaDOSDir int buflen, char *buf, const char *linkfile);
long _cdecl sys_d_cntl		(MetaDOSDir int cmd, const char *name, long arg);
long _cdecl sys_f_chown		(MetaDOSDir const char *name, int uid, int gid);
long _cdecl sys_f_chown16	(MetaDOSDir const char *name, int uid, int gid, int follow_symlinks);
long _cdecl sys_f_chmod		(MetaDOSDir const char *name, unsigned mode);
long _cdecl sys_d_lock		(MetaDOSDir int mode, int drv);
long _cdecl sys_d_readlabel	(MetaDOSDir const char *path, char *label, int maxlen);
long _cdecl sys_d_writelabel 	(MetaDOSDir const char *path, const char *label);
long _cdecl sys_d_chroot	(MetaDOSDir const char *dir);
long _cdecl sys_f_stat64	(MetaDOSDir int flag, const char *name, STAT *stat);
long _cdecl sys_f_chdir		(MetaDOSFile short fd);
long _cdecl sys_f_opendir	(MetaDOSFile short fd);
long _cdecl sys_f_dirfd		(MetaDOSFile long handle);


# endif /* _dosdir_h */
