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

# include "mintfake.h"

/* local interpretation of the 'struct dirent' */
typedef struct {
	LIST	*list;
	FCOOKIE	*current;
	short	mode;
} DIR;

long __CDECL sys_dl_opendir (DIR *dirh, LIST *list, int flag);
long __CDECL sys_dl_readdir (DIR *dirh, char *buf, int len);
long __CDECL sys_dl_closedir (DIR *dirh);
long __CDECL sys_dl_readlabel (char *buf, int buflen);

long __CDECL sys_d_free	    	(MetaDOSDir long *buf, int d);
long __CDECL sys_d_create	(MetaDOSDir const char *path);
long __CDECL sys_d_delete	(MetaDOSDir const char *path);
long __CDECL sys_f_sfirst	(MetaDOSDTA const char *path, int attrib);
long __CDECL sys_f_snext	(MetaDOSDTA0);
long __CDECL sys_f_attrib	(MetaDOSFile const char *name, int rwflag, int attr);
long __CDECL sys_f_delete	(MetaDOSFile const char *name);
long __CDECL sys_f_rename	(MetaDOSFile int junk, const char *old, const char *new);
long __CDECL sys_d_pathconf	(MetaDOSDir const char *name, int which);
long __CDECL sys_d_opendir	(MetaDOSDir const char *path, int flags);
long __CDECL sys_d_readdir	(MetaDOSDir int len, long handle, char *buf);
long __CDECL sys_d_xreaddir	(MetaDOSDir int len, long handle, char *buf, struct xattr *xattr, long *xret);
long __CDECL sys_d_rewind	(MetaDOSDir long handle);
long __CDECL sys_d_closedir	(MetaDOSDir long handle);
long __CDECL sys_f_xattr	(MetaDOSFile int flag, const char *name, struct xattr *xattr);
long __CDECL sys_d_readlabel	(MetaDOSDir const char *path, char *label, int maxlen);
long __CDECL sys_d_writelabel	(MetaDOSDir const char *path, const char *label);


# endif /* _dosdir_h */
