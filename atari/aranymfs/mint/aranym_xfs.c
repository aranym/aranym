/*
 * $Header$
 *
 * 2001/2002 STanda
 *
 * This is a part of the ARAnyM project sources. Originaly taken from the STonX
 * CVS repository and adjusted to our needs.
 *
 */

/*
 * Copyright 1998, 1999, 2001 by Markus Kohm <Markus.Kohm@gmx.de>.
 *
 * See COPYING for details of legal notes.
 *
 * Modified by Chris Felsch <C.Felsch@gmx.de>.
 *
 * Filesystem driver routines
 */

#include "aranym_xfs.h"

/*
 * interface to ARAnyM (see callnative.s)
 */
extern long     _cdecl ara_fs_native_init(struct kerinfo *k, short fs_devnum);

/*
 * assembler routines (see callnative.s)
 */
extern long		_cdecl ara_fs_root		 (int drv, fcookie *fc);
extern long		_cdecl ara_fs_lookup	 (fcookie *dir, const char *name, fcookie *fc);
extern long		_cdecl ara_fs_creat		 (fcookie *dir, const char *name,
										  unsigned mode, int attrib, fcookie *fc);
extern DEVDRV * _cdecl ara_fs_getdev	 (fcookie *fc, long *devspecial);
extern long		_cdecl ara_fs_getxattr	 (fcookie *file, XATTR *xattr);
extern long		_cdecl ara_fs_chattr	 (fcookie *file, int attr);
extern long		_cdecl ara_fs_chown		 (fcookie *file, int uid, int gid);
extern long		_cdecl ara_fs_chmode	 (fcookie *file, unsigned mode);
extern long		_cdecl ara_fs_mkdir		 (fcookie *dir, const char *name, unsigned mode);
extern long		_cdecl ara_fs_rmdir		 (fcookie *dir, const char *name);
extern long		_cdecl ara_fs_remove	 (fcookie *dir, const char *name);
extern long		_cdecl ara_fs_getname	 (fcookie *relto, fcookie *dir,
										  char *pathname, int size);
extern long		_cdecl ara_fs_rename	 (fcookie *olddir, char *oldname,
										  fcookie *newdir, const char *newname);
extern long		_cdecl ara_fs_opendir	 (DIR *dirh, int tosflag);
extern long		_cdecl ara_fs_readdir	 (DIR *dirh, char *name, int namelen,
										  fcookie *fc);
extern long		_cdecl ara_fs_rewinddir	 (DIR *dirh);
extern long		_cdecl ara_fs_closedir	 (DIR *dirh);
extern long		_cdecl ara_fs_pathconf	 (fcookie *dir, int which);
extern long		_cdecl ara_fs_dfree		 (fcookie *dir, long *buf);
extern long		_cdecl ara_fs_writelabel (fcookie *dir, const char *name);
extern long		_cdecl ara_fs_readlabel	 (fcookie *dir, char *name,
										  int namelen);
extern long		_cdecl ara_fs_symlink	 (fcookie *dir, const char *name,
										  const char *to);
extern long		_cdecl ara_fs_readlink	 (fcookie *dir, char *buf, int len);
extern long		_cdecl ara_fs_hardlink	 (fcookie *fromdir,
										  const char *fromname,
										  fcookie *todir, const char *toname);
extern long		_cdecl ara_fs_fscntl	 (fcookie *dir, const char *name,
										  int cmd, long arg);
extern long		_cdecl ara_fs_dskchng	 (int drv, int mode);
extern long		_cdecl ara_fs_release	 (fcookie *);
extern long		_cdecl ara_fs_dupcookie	 (fcookie *new, fcookie *old);
extern long		_cdecl ara_fs_sync		 (void);
extern long		_cdecl ara_fs_mknod		 (fcookie *dir, const char *name, ulong mode);
extern long		_cdecl ara_fs_unmount	 (int drv);

/*
 * filesystem driver map
 */
FILESYS aranym_filesys =
    {
        (struct filesys *)0,	/* next */
        /*
         * FS_KNOPARSE         kernel shouldn't do parsing
         * FS_CASESENSITIVE    file names are case sensitive
         * FS_NOXBIT           if a file can be read, it can be executed
         * FS_LONGPATH         file system understands "size" argument to "getname"
         * FS_NO_C_CACHE       don't cache cookies for this filesystem
         * FS_DO_SYNC          file system has a sync function
         * FS_OWN_MEDIACHANGE  filesystem control self media change (dskchng)
         * FS_REENTRANT_L1     fs is level 1 reentrant
         * FS_REENTRANT_L2     fs is level 2 reentrant
         * FS_EXT_1            extensions level 1 - mknod & unmount
         * FS_EXT_2            extensions level 2 - additional place at the end
         * FS_EXT_3            extensions level 3 - stat & native UTC timestamps
         */
        (FS_NOXBIT|FS_CASESENSITIVE),
        ara_fs_root, ara_fs_lookup, ara_fs_creat, ara_fs_getdev, ara_fs_getxattr,
        ara_fs_chattr, ara_fs_chown, ara_fs_chmode, ara_fs_mkdir, ara_fs_rmdir,
        ara_fs_remove, ara_fs_getname, ara_fs_rename, ara_fs_opendir,
        ara_fs_readdir, ara_fs_rewinddir, ara_fs_closedir, ara_fs_pathconf,
        ara_fs_dfree, ara_fs_writelabel, ara_fs_readlabel, ara_fs_symlink,
        ara_fs_readlink, ara_fs_hardlink, ara_fs_fscntl, ara_fs_dskchng,
        ara_fs_release, ara_fs_dupcookie, ara_fs_sync, ara_fs_mknod,
        ara_fs_unmount,
        0L,				/* stat64() */
        0L, 0L, 0L,			        /* res1-3 */
        0L, 0L,				/* lock, sleeperd */
        0L, 0L				/* block(), deblock() */
    };

/*
 * filesystem basic description
 */
static struct fs_descr aranym_fs_descr =
    {
        &aranym_filesys,
        0, /* this is filled in by MiNT at FS_MOUNT */
        0, /* FIXME: what about flags? */
        {0,0,0,0}  /* reserved */
    };


FILESYS *aranym_fs_init(void) {
    //if ( aranym_cookie->flags & STNX_IS_XFS ) {
    if (1) {
        long r;
        int succ = 0;

        /* Try to install */
        r = d_cntl (FS_INSTALL, "u:\\", (long) &aranym_fs_descr);
        if (r != (long)kernel)
        {
            c_conws (MSG_PFAILURE("u:\\"MINT_FS_NAME,
                                  "Dcntl(FS_INSTALL,...) failed"));
            DEBUG(("Return value was %li", r));
            return NULL; /* Nothing installed, so nothing to stay resident */
        } else {
            succ |= 1;
            /* mount */
            r = d_cntl(FS_MOUNT, "u:\\"MINT_FS_NAME, (long) &aranym_fs_descr);
            if (r != aranym_fs_descr.dev_no )
            {
                c_conws (MSG_PFAILURE("u:\\"MINT_FS_NAME,
                                      "Dcnt(FS_MOUNT,...) failed"));
                DEBUG(("Return value was %li", r));
            } else {
                succ |= 2;
                /* init */
                r = ara_fs_native_init(KERNEL, aranym_fs_descr.dev_no);
                r = 0;
                if ( r < 0 ) {
                    c_conws (MSG_PFAILURE("u:\\"MINT_FS_NAME,
                                          "native init failed"));
                    DEBUG(("Return value was %li", r));
                } else {
                    char buff[255];
                    ksprintf_old( buff, "fs_drv = %08lx", (long)&aranym_filesys );
                    c_conws (buff);
                    return &aranym_filesys; /* We where successfull */
                }
            }
        }

        /* Try to uninstall, if necessary */
        if ( succ & 2 ) {
            /* unmount */
            r = d_cntl(FS_UNMOUNT, "u:\\"MINT_FS_NAME,
                       (long) &aranym_fs_descr);
            DEBUG(("Dcntl(FS_UNMOUNT,...) = %li", r ));
            if ( r < 0 ) {
                return (FILESYS *) 1; /* Can't uninstall,
                                       * because unmount failed */
            }
        }
        if ( succ & 1 ) {
            /* uninstall */
            r = d_cntl(FS_UNINSTALL, "u:\\"MINT_FS_NAME,
                       (long) &aranym_fs_descr);
            DEBUG(("Dcntl(FS_UNINSTALL,...) = %li", r ));
            if ( r < 0 ) {
                return (FILESYS *) 1; /* Can't say NULL,
                                       * because uninstall failed */
            }
        }
    } else {
        c_conws (MSG_PFAILURE("u:\\"MINT_FS_NAME,
                              "not activated at ARAnyM"));
    }

    return NULL; /* Nothing installed, so nothing to stay resident */
}


/*
 * $Log$
 *
 */
