/*
 * $Header$
 *
 * 2001/2002 STanda
 *
 * This is a part of the ARAnyM project sources.
 *
 * Originaly taken from the STonX CVS repository.
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

#include "natfeat.h"
#include "aranym_xfs.h"

/* note: must include after aranym_xfs.h */
#include "mint/kerinfo.h"


long     _cdecl aranym_fs_native_init(int fs_devnum, char *mountpoint, char *hostroot, int halfsensitive,
									  void *fs, void *fs_dev);

long     _cdecl ara_fs_root		 (int drv, fcookie *fc);
long     _cdecl ara_fs_lookup	 (fcookie *dir, const char *name, fcookie *fc);
long     _cdecl ara_fs_creat	 (fcookie *dir, const char *name,
								  unsigned mode, int attrib, fcookie *fc);
DEVDRV * _cdecl ara_fs_getdev	 (fcookie *fc, long *devspecial);
long     _cdecl ara_fs_getxattr	 (fcookie *file, XATTR *xattr);
long     _cdecl ara_fs_chattr	 (fcookie *file, int attr);
long     _cdecl ara_fs_chown		 (fcookie *file, int uid, int gid);
long     _cdecl ara_fs_chmode	 (fcookie *file, unsigned mode);
long     _cdecl ara_fs_mkdir		 (fcookie *dir, const char *name, unsigned mode);
long     _cdecl ara_fs_rmdir		 (fcookie *dir, const char *name);
long     _cdecl ara_fs_remove	 (fcookie *dir, const char *name);
long     _cdecl ara_fs_getname	 (fcookie *relto, fcookie *dir,
								  char *pathname, int size);
long     _cdecl ara_fs_rename	 (fcookie *olddir, char *oldname,
								  fcookie *newdir, const char *newname);
long     _cdecl ara_fs_opendir	 (DIR *dirh, int tosflag);
long     _cdecl ara_fs_readdir	 (DIR *dirh, char *name, int namelen,
								  fcookie *fc);
long     _cdecl ara_fs_rewinddir (DIR *dirh);
long     _cdecl ara_fs_closedir	 (DIR *dirh);
long     _cdecl ara_fs_pathconf	 (fcookie *dir, int which);
long     _cdecl ara_fs_dfree	 (fcookie *dir, long *buf);
long     _cdecl ara_fs_writelabel(fcookie *dir, const char *name);
long     _cdecl ara_fs_readlabel (fcookie *dir, char *name,
								  int namelen);
long     _cdecl ara_fs_symlink	 (fcookie *dir, const char *name,
								  const char *to);
long     _cdecl ara_fs_readlink	 (fcookie *dir, char *buf, int len);
long     _cdecl ara_fs_hardlink	 (fcookie *fromdir,
								  const char *fromname,
								  fcookie *todir, const char *toname);
long     _cdecl ara_fs_fscntl	 (fcookie *dir, const char *name,
								  int cmd, long arg);
long     _cdecl ara_fs_dskchng	 (int drv, int mode);
long     _cdecl ara_fs_release	 (fcookie *);
long     _cdecl ara_fs_dupcookie (fcookie *new, fcookie *old);
long     _cdecl ara_fs_sync		 (void);
long     _cdecl ara_fs_mknod     (fcookie *dir, const char *name, ulong mode);
long     _cdecl ara_fs_unmount	 (int drv);


unsigned long nfHostFsId;


long     _cdecl aranym_fs_native_init(int fs_devnum, char *mountpoint, char *hostroot, int halfsensitive,
									  void *fs, void *fs_dev)
{
	return nfCall((nfHostFsId + 0x00, (long)fs_devnum, mountpoint, hostroot, (long)halfsensitive, fs, fs_dev));
}

long     _cdecl ara_fs_root       (int drv, fcookie *fc)
{
	return nfCall((nfHostFsId + 0x01, (long)drv, fc));
}

long     _cdecl ara_fs_lookup     (fcookie *dir, const char *name, fcookie *fc)
{
	return nfCall((nfHostFsId + 0x02, dir, name, fc));
}

long     _cdecl ara_fs_creat      (fcookie *dir, const char *name,
								   unsigned int mode, int attrib, fcookie *fc)
{
	return nfCall((nfHostFsId + 0x03, dir, name, (long)mode, (long)attrib, fc));
}

DEVDRV * _cdecl ara_fs_getdev     (fcookie *fc, long *devspecial)
{
	return (DEVDRV*) nfCall((nfHostFsId + 0x04, fc, devspecial));
}

long     _cdecl ara_fs_getxattr   (fcookie *file, XATTR *xattr)
{
	return nfCall((nfHostFsId + 0x05, file, xattr));
}

long     _cdecl ara_fs_chattr     (fcookie *file, int attr)
{
	return nfCall((nfHostFsId + 0x06, file, (long)attr));
}

long     _cdecl ara_fs_chown      (fcookie *file, int uid, int gid)
{
	return nfCall((nfHostFsId + 0x07, file, (long)uid, (long)gid));
}

long     _cdecl ara_fs_chmode     (fcookie *file, unsigned int mode)
{
	return nfCall((nfHostFsId + 0x08, file, (long)mode));
}

long     _cdecl ara_fs_mkdir      (fcookie *dir, const char *name, unsigned int mode)
{
	return nfCall((nfHostFsId + 0x09, dir, name, (long)mode));
}

long     _cdecl ara_fs_rmdir      (fcookie *dir, const char *name)
{
	return nfCall((nfHostFsId + 0x0a, dir, name));
}

long     _cdecl ara_fs_remove     (fcookie *dir, const char *name)
{
	return nfCall((nfHostFsId + 0x0b, dir, name));
}

long     _cdecl ara_fs_getname    (fcookie *relto, fcookie *dir,
								   char *pathname, int size)
{
	return nfCall((nfHostFsId + 0x0c, relto, dir, pathname, (long)size));
}

long     _cdecl ara_fs_rename     (fcookie *olddir, char *oldname,
								   fcookie *newdir, const char *newname)
{
	return nfCall((nfHostFsId + 0x0d, olddir, oldname, newdir, newname));
}

long     _cdecl ara_fs_opendir    (DIR *dirh, int tosflag)
{
	return nfCall((nfHostFsId + 0x0e, dirh, (long)tosflag));
}

long     _cdecl ara_fs_readdir    (DIR *dirh, char *name, int namelen,
								   fcookie *fc)
{
	return nfCall((nfHostFsId + 0x0f, dirh, name, (long)namelen, fc));
}

long     _cdecl ara_fs_rewinddir  (DIR *dirh)
{
	return nfCall((nfHostFsId + 0x10, dirh));
}

long     _cdecl ara_fs_closedir   (DIR *dirh)
{
	return nfCall((nfHostFsId + 0x11, dirh));
}

long     _cdecl ara_fs_pathconf   (fcookie *dir, int which)
{
	return nfCall((nfHostFsId + 0x12, dir, (long)which));
}

long     _cdecl ara_fs_dfree      (fcookie *dir, long *buf)
{
	return nfCall((nfHostFsId + 0x13, dir, buf));
}

long     _cdecl ara_fs_writelabel (fcookie *dir, const char *name)
{
	return nfCall((nfHostFsId + 0x14, dir, name));
}

long     _cdecl ara_fs_readlabel  (fcookie *dir, char *name,
								   int namelen)
{
	return nfCall((nfHostFsId + 0x15, dir, name, (long)namelen));
}

long     _cdecl ara_fs_symlink    (fcookie *dir, const char *name,
								   const char *to)
{
	return nfCall((nfHostFsId + 0x16, dir, name, to));
}

long     _cdecl ara_fs_readlink   (fcookie *dir, char *buf, int len)
{
	return nfCall((nfHostFsId + 0x17, dir, buf, (long)len));
}

long     _cdecl ara_fs_hardlink   (fcookie *fromdir,
								   const char *fromname,
								   fcookie *todir, const char *toname)
{
	return nfCall((nfHostFsId + 0x18, fromdir, fromname, todir, toname));
}

long     _cdecl ara_fs_fscntl     (fcookie *dir, const char *name,
								   int cmd, long arg)
{
	return nfCall((nfHostFsId + 0x19, dir, name, (long)cmd, arg));
}

long     _cdecl ara_fs_dskchng    (int drv, int mode)
{
	return nfCall((nfHostFsId + 0x1a, (long)drv, (long)mode));
}

long     _cdecl ara_fs_release    (fcookie *fc)
{
	return nfCall((nfHostFsId + 0x1b, fc));
}

long     _cdecl ara_fs_dupcookie  (fcookie *new, fcookie *old)
{
	return nfCall((nfHostFsId + 0x1c, new, old));
}

long     _cdecl ara_fs_sync       (void)
{
	return nfCall((nfHostFsId + 0x1d));
}

long     _cdecl ara_fs_mknod      (fcookie *dir, const char *name, ulong mode)
{
	return nfCall((nfHostFsId + 0x1e, dir, name, mode));
}

long     _cdecl ara_fs_unmount    (int drv)
{
	return nfCall((nfHostFsId + 0x1f, (long)drv));
}


/*
 * filesystem driver map
 */
FILESYS aranym_fs =
    {
        (struct filesys *)0,    /* next */
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
        0L,             /* stat64() */
        0L, 0L, 0L,                 /* res1-3 */
        0L, 0L,             /* lock, sleeperd */
        0L, 0L              /* block(), deblock() */
    };


/*
 * filesystem basic description
 */
static struct fs_descr aranym_fs_descr =
    {
        &aranym_fs,
        0, /* this is filled in by MiNT at FS_MOUNT */
        0, /* FIXME: what about flags? */
        {0,0,0,0}  /* reserved */
    };


extern DEVDRV aranym_fs_devdrv; /* from aranym_dev.c */


#define MINT_FS_NAME    "nativefs"   /* mount point at u:\\ */
#define MINT_SER_NAME   "serial2"    /* device name in u:\\dev\\ */
#define MINT_COM_NAME   "aranym"     /* device name in u:\\dev\\ */


FILESYS *aranym_fs_init(void)
{
	// get the HostFs NatFeat ID
	nfHostFsId = nfGetID(("HOSTFS"));

#ifdef ARAnyM_MetaDOS
	return nfHostFsId ? &aranym_fs : NULL;
#else
    if ( nfHostFsId ) {
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
                r = aranym_fs_native_init(aranym_fs_descr.dev_no, "u:\\"MINT_FS_NAME, "/", 0 /*caseSensitive*/,
										  &aranym_fs, &aranym_fs_devdrv );
                r = 0;
                if ( r < 0 ) {
                    c_conws (MSG_PFAILURE("u:\\"MINT_FS_NAME,
                                          "native init failed"));
                    DEBUG(("Return value was %li", r));
                } else {
                    char buff[255];
                    ksprintf_old( buff, "fs_drv = %08lx", (long)&aranym_fs );
                    c_conws (buff);
                    return &aranym_fs; /* We where successfull */
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
                              "\r\nThe HOSTFS was not compiled in!"));
    }

    return NULL; /* Nothing installed, so nothing to stay resident */
#endif
}

/*
 * $Log$
 * Revision 1.1  2002/12/10 20:47:21  standa
 * The HostFS (the host OS filesystem access via NatFeats) implementation.
 *
 * Revision 1.1  2002/05/22 07:53:22  standa
 * The PureC -> gcc conversion (see the CONFIGVARS).
 * MiNT .XFS sources added.
 *
 *
 */
