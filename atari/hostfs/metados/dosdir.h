/*
 * $Id$
 *
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

# include "mint/mint.h"
# include "mint/file.h"

# include "mintfake.h"


/* table of processes holding locks on drives */
extern PROC *dlockproc [NUM_DRIVES];
extern long searchtime;

long _cdecl d_free	    (MetaDOSDir long *buf, int d);
long _cdecl d_create	(MetaDOSDir const char *path);
long _cdecl d_delete	(MetaDOSDir const char *path);
long _cdecl f_sfirst	(MetaDOSDTA const char *path, int attrib);
long _cdecl f_snext	    (MetaDOSDTA0);
long _cdecl f_attrib	(MetaDOSFile const char *name, int rwflag, int attr);
long _cdecl f_delete	(MetaDOSFile const char *name);
long _cdecl f_rename	(MetaDOSFile int junk, const char *old, const char *new);
long _cdecl d_pathconf	(MetaDOSDir const char *name, int which);
long _cdecl d_opendir	(MetaDOSDir const char *path, int flags);
long _cdecl d_readdir	(MetaDOSDir int len, long handle, char *buf);
long _cdecl d_xreaddir	(MetaDOSDir int len, long handle, char *buf, XATTR *xattr, long *xret);
long _cdecl d_rewind	(MetaDOSDir long handle);
long _cdecl d_closedir	(MetaDOSDir long handle);
long _cdecl f_xattr	    (MetaDOSFile int flag, const char *name, XATTR *xattr);
long _cdecl d_readlabel	(MetaDOSDir const char *path, char *label, int maxlen);
long _cdecl d_writelabel (MetaDOSDir const char *path, const char *label);


# endif /* _dosdir_h */
