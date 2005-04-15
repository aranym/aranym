/*
 * $Header$
 *
 * 2001-2003 STanda
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
#include "hostfs_xfs.h"
#include "hostfs_nfapi.h"

/* note: must include after hostfs_xfs.h */
#include "mint/kerinfo.h"

#define ARADEBUG 0
#if ARADEBUG
#define DEBUG_DRV(x) x
#else
#define DEBUG_DRV(x)
#endif

/* Use the 0x4c2L for drive mapping (if == 0) */
#define USE_DCNTL 0


ulong    _cdecl fs_drive_bits();
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

ulong     _cdecl fs_drive_bits()
{
	return nfCall((HOSTFS(GET_DRIVE_BITS)));
}

long     _cdecl aranym_fs_native_init(int fs_devnum, char *mountpoint, char *hostroot, int halfsensitive,
									  void *fs, void *fs_dev)
{
	return nfCall((HOSTFS(XFS_INIT), (long)fs_devnum, mountpoint, hostroot, (long)halfsensitive, fs, fs_dev));
}

long     _cdecl ara_fs_root       (int drv, fcookie *fc)
{
	return nfCall((HOSTFS(XFS_ROOT), (long)drv, fc));
}

long     _cdecl ara_fs_lookup     (fcookie *dir, const char *name, fcookie *fc)
{
	return nfCall((HOSTFS(XFS_LOOKUP), dir, name, fc));
}

long     _cdecl ara_fs_creat      (fcookie *dir, const char *name,
								   unsigned int mode, int attrib, fcookie *fc)
{
	return nfCall((HOSTFS(XFS_CREATE), dir, name, (long)mode, (long)attrib, fc));
}

DEVDRV * _cdecl ara_fs_getdev     (fcookie *fc, long *devspecial)
{
	return (DEVDRV*) nfCall((HOSTFS(XFS_GETDEV), fc, devspecial));
}

long     _cdecl ara_fs_getxattr   (fcookie *file, XATTR *xattr)
{
	return nfCall((HOSTFS(XFS_GETXATTR), file, xattr));
}

long     _cdecl ara_fs_chattr     (fcookie *file, int attr)
{
	return nfCall((HOSTFS(XFS_CHATTR), file, (long)attr));
}

long     _cdecl ara_fs_chown      (fcookie *file, int uid, int gid)
{
	return nfCall((HOSTFS(XFS_CHOWN), file, (long)uid, (long)gid));
}

long     _cdecl ara_fs_chmode     (fcookie *file, unsigned int mode)
{
	return nfCall((HOSTFS(XFS_CHMOD), file, (long)mode));
}

long     _cdecl ara_fs_mkdir      (fcookie *dir, const char *name, unsigned int mode)
{
	return nfCall((HOSTFS(XFS_MKDIR), dir, name, (long)mode));
}

long     _cdecl ara_fs_rmdir      (fcookie *dir, const char *name)
{
	return nfCall((HOSTFS(XFS_RMDIR), dir, name));
}

long     _cdecl ara_fs_remove     (fcookie *dir, const char *name)
{
	return nfCall((HOSTFS(XFS_REMOVE), dir, name));
}

long     _cdecl ara_fs_getname    (fcookie *relto, fcookie *dir,
								   char *pathname, int size)
{
	return nfCall((HOSTFS(XFS_GETNAME), relto, dir, pathname, (long)size));
}

long     _cdecl ara_fs_rename     (fcookie *olddir, char *oldname,
								   fcookie *newdir, const char *newname)
{
	return nfCall((HOSTFS(XFS_RENAME), olddir, oldname, newdir, newname));
}

long     _cdecl ara_fs_opendir    (DIR *dirh, int tosflag)
{
	return nfCall((HOSTFS(XFS_OPENDIR), dirh, (long)tosflag));
}

long     _cdecl ara_fs_readdir    (DIR *dirh, char *name, int namelen,
								   fcookie *fc)
{
	return nfCall((HOSTFS(XFS_READDIR), dirh, name, (long)namelen, fc));
}

long     _cdecl ara_fs_rewinddir  (DIR *dirh)
{
	return nfCall((HOSTFS(XFS_REWINDDIR), dirh));
}

long     _cdecl ara_fs_closedir   (DIR *dirh)
{
	return nfCall((HOSTFS(XFS_CLOSEDIR), dirh));
}

long     _cdecl ara_fs_pathconf   (fcookie *dir, int which)
{
	return nfCall((HOSTFS(XFS_PATHCONF), dir, (long)which));
}

long     _cdecl ara_fs_dfree      (fcookie *dir, long *buf)
{
	return nfCall((HOSTFS(XFS_DFREE), dir, buf));
}

long     _cdecl ara_fs_writelabel (fcookie *dir, const char *name)
{
	return nfCall((HOSTFS(XFS_WRITELABEL), dir, name));
}

long     _cdecl ara_fs_readlabel  (fcookie *dir, char *name,
								   int namelen)
{
	return nfCall((HOSTFS(XFS_READLABEL), dir, name, (long)namelen));
}

long     _cdecl ara_fs_symlink    (fcookie *dir, const char *name,
								   const char *to)
{
	return nfCall((HOSTFS(XFS_SYMLINK), dir, name, to));
}

long     _cdecl ara_fs_readlink   (fcookie *dir, char *buf, int len)
{
	return nfCall((HOSTFS(XFS_READLINK), dir, buf, (long)len));
}

long     _cdecl ara_fs_hardlink   (fcookie *fromdir,
								   const char *fromname,
								   fcookie *todir, const char *toname)
{
	return nfCall((HOSTFS(XFS_HARDLINK), fromdir, fromname, todir, toname));
}

long     _cdecl ara_fs_fscntl     (fcookie *dir, const char *name,
								   int cmd, long arg)
{
	return nfCall((HOSTFS(XFS_FSCNTL), dir, name, (long)cmd, arg));
}

long     _cdecl ara_fs_dskchng    (int drv, int mode)
{
	return nfCall((HOSTFS(XFS_DSKCHNG), (long)drv, (long)mode));
}

long     _cdecl ara_fs_release    (fcookie *fc)
{
	return nfCall((HOSTFS(XFS_RELEASE), fc));
}

long     _cdecl ara_fs_dupcookie  (fcookie *new, fcookie *old)
{
	return nfCall((HOSTFS(XFS_DUPCOOKIE), new, old));
}

long     _cdecl ara_fs_sync       (void)
{
	return nfCall((HOSTFS(XFS_SYNC)));
}

long     _cdecl ara_fs_mknod      (fcookie *dir, const char *name, ulong mode)
{
	return nfCall((HOSTFS(XFS_MKNOD), dir, name, mode));
}

long     _cdecl ara_fs_unmount    (int drv)
{
	return nfCall((HOSTFS(XFS_UNMOUNT), (long)drv));
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
	FS_CASESENSITIVE |
	FS_LONGPATH      |
	FS_REENTRANT_L1  |
	FS_REENTRANT_L2  |
	FS_EXT_1         |
	FS_EXT_2         ,
        ara_fs_root, ara_fs_lookup, ara_fs_creat, ara_fs_getdev, ara_fs_getxattr,
        ara_fs_chattr, ara_fs_chown, ara_fs_chmode, ara_fs_mkdir, ara_fs_rmdir,
        ara_fs_remove, ara_fs_getname, ara_fs_rename, ara_fs_opendir,
        ara_fs_readdir, ara_fs_rewinddir, ara_fs_closedir, ara_fs_pathconf,
        ara_fs_dfree, ara_fs_writelabel, ara_fs_readlabel, ara_fs_symlink,
        ara_fs_readlink, ara_fs_hardlink, ara_fs_fscntl, ara_fs_dskchng,
        ara_fs_release, ara_fs_dupcookie, ara_fs_sync,
	/* FS_EXT_1 */
	ara_fs_mknod, ara_fs_unmount,
	/* FS_EXT_2 */
	/* FS_EXT_3 */
        0L,                 /* TODO: stat64() */
        0L, 0L, 0L,         /* res1-3 */
        0L, 0L,             /* lock, sleeperd */
        0L, 0L              /* block(), deblock() */
    };


#if USE_DCNTL

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

#endif /* USE_DCNTL */


extern DEVDRV aranym_fs_devdrv; /* from hostfs_dev.c */


#define MINT_FS_NAME    "hostfs"     /* mount point at u:\\ */
#define MINT_SER_NAME   "serial2"    /* device name in u:\\dev\\ */
#define MINT_COM_NAME   "aranym"     /* device name in u:\\dev\\ */

#if ARADEBUG

void c_conws_num( int x )
{
	const int R=10;

	char sss[50];
	char *ptr = &sss[49];
	int x=aranym_fs_descr.dev_no;
	*ptr--='\0';
	if( x<0 ) { *ptr--='-'; x=-x; };
	if( x==0 ) { *ptr--='0'; };
	while( x>0 ) { *ptr--="0123456789ABCDEF"[x%R]; x/=R; };
	ptr++;

	c_conws( (const char*)ptr );
}

#endif /* DEBUG_DRV */

FILESYS *aranym_fs_init(void)
{
#ifdef ARAnyM_MetaDOS
#	define c_conws(a) /* nothing */
#else
	long r;
	int succ;
	int keep = 0;
	int fail = 0;
#endif

	/* get the HostFs NatFeat ID */
	nfHostFsId = nfGetID(("HOSTFS"));
	if (nfHostFsId == 0) {
        c_conws(MSG_PFAILURE("u:\\"MINT_FS_NAME,
                             "\r\nThe HOSTFS NatFeat not found\r\n"));
		return NULL;
	}

	/* compare the version */
	if (nfCall((HOSTFS(GET_VERSION))) != HOSTFS_NFAPI_VERSION) {
		c_conws(MSG_PFAILURE("u:\\"MINT_FS_NAME,
							 "\r\nHOSTFS NFAPI version mismatch\n\r"));
		return NULL;
	}

#ifdef ARAnyM_MetaDOS
	return &aranym_fs;
#else

#if USE_DCNTL

	/* Try to install */
	r = d_cntl (FS_INSTALL, "u:\\", (long) &aranym_fs_descr);
	if (r != 0 && r != (long)kernel)
	{
		c_conws ("\r\nerror: Dcnt(FS_INSTALL,...) failed.\r\n");
		DEBUG(("Return value was %li", r));

		/* Nothing installed, so nothing to stay resident */
		return NULL;
	}
	else

#endif

	{
		ulong drvBits = fs_drive_bits();
		ushort drvNumber = 0;
		char mountPoint[] = "u:\\XX";

		succ |= 1;

		c_conws("\r\nMounts: ");

		while ( drvBits ) {
			/* search the 1st log 1 bit position -> drvNumber */
			while( ! (drvBits & 1) ) { drvNumber++; drvBits>>=1; }

			/* ready */
			succ = 0;

			mountPoint[3] = drvNumber+'a';
			mountPoint[4] = '\0';

			DEBUG_DRV({c_conws("\r\n  Drive: "); c_conws_num( drvNumber ); c_conws(" : ");})

#if !USE_DCNTL

			c_conws( (const char*)mountPoint );
			c_conws(" ");

			/* check whether the drive is already assigned or not */
			if ( ! ( *((long *) 0x4c2L) & (1UL << drvNumber) ) ) {
				r = aranym_fs_native_init( drvNumber, mountPoint, "/", 0 /*caseSensitive*/,
										   &aranym_fs, &aranym_fs_devdrv );
				if ( r < 0 ) {
					c_conws ("\r\nerror: Native init failed.");
					DEBUG(("Return value was %li", r));
				} else {
					/* put the drive letter into the GEMDOS drive bits */
					*((long *) 0x4c2L) |= 1UL << drvNumber;

					succ = 0; /* do not unmount */
					keep = 1; /* at least one is mounted */
				}
			} else {
				c_conws("\r\nerror: drive already present.");
			}
#else

#if 0
			/* two letter drive mount (via Dcntl()) */
			mountPoint[4] = mountPoint[3];
#endif
			c_conws( (const char*)mountPoint );
			c_conws(" ");

			/* mount */
			r = d_cntl(FS_MOUNT, mountPoint, (long) &aranym_fs_descr);
			DEBUG_DRV({c_conws("\r\nDevNo: "); c_conws_num( aranym_fs_descr.dev_no ); c_conws("\r\n");});
			if (r != aranym_fs_descr.dev_no )
			{
				c_conws ("\r\nerror: Dcnt(FS_MOUNT,...) failed.");
				DEBUG(("Return value was %li", r));
			} else {
				succ |= 2;
				/* init */

				r = aranym_fs_native_init( aranym_fs_descr.dev_no, mountPoint, "/", 0 /*caseSensitive*/,
										   &aranym_fs, &aranym_fs_devdrv );
				if ( r < 0 ) {
					c_conws ("\r\nerror: Native init failed.");
					DEBUG(("Return value was %li", r));
				} else {
					succ = 0; /* do not unmount */
					keep = 1; /* at least one is mounted */
				}
			}

			/* Try to uninstall, if necessary */
			if ( succ & 2 ) {
				/* unmount */
				r = d_cntl(FS_UNMOUNT, mountPoint,
						   (long) &aranym_fs_descr);
				c_conws("\r\nDcntl(FS_UNMOUNT,...)");
				if ( r < 0 ) {
					/* Can't uninstall,
					 * because unmount failed */
					fail |= 2;
				}
			}
#endif

			drvNumber++; drvBits>>=1;
		}

		c_conws("\r\n");
	}

	/* everything OK */
	if ( keep ) {
#if 0
		char buff[255];
		ksprintf_old( buff, "fs_drv = %08lx", (long)&aranym_fs );
		c_conws (buff);
#endif
		return &aranym_fs; /* We where successfull */
	}
#if USE_DCNTL
	else
	{
		/* uninstall */
		r = d_cntl(FS_UNINSTALL, "u:\\", (long) &aranym_fs_descr);
		c_conws("\r\nDcntl(FS_UNINSTALL,...)");
		if ( r < 0 ) {
			/* Can't say NULL,
			 * because uninstall failed */
			fail |= 1;
		}
	}
#endif

	c_conws("\r\n");

	if ( fail )
		return (FILESYS *) 1; /* Can't say NULL,
							   * because uninstall failed */

    return NULL; /* Nothing installed, so nothing to stay resident */
#endif
}

/*
 * $Log$
 * Revision 1.9  2003/10/02 18:13:41  standa
 * Large HOSTFS cleanup (see the ChangeLog for more)
 *
 * Revision 1.8  2003/03/24 08:58:53  joy
 * aranymfs.xfs renamed to hostfs.xfs
 *
 * Revision 1.7  2003/03/23 12:44:44  joy
 * updated
 *
 * Revision 1.6  2003/03/22 22:46:25  joy
 * cosmetical change in displayed info
 *
 * Revision 1.5  2003/03/20 21:27:22  standa
 * The .xfs mapping to the U:\G mountpouints (single letter) implemented.
 *
 * Revision 1.4  2003/03/20 01:08:17  standa
 * HOSTFS mapping update.
 *
 * Revision 1.3  2003/03/01 11:57:37  joy
 * major HOSTFS NF API cleanup
 *
 * Revision 1.2  2002/12/19 10:16:49  standa
 * Error message fixed.
 *
 * Revision 1.1  2002/12/10 20:47:21  standa
 * The HostFS (the host OS filesystem access via NatFeats) implementation.
 *
 * Revision 1.1  2002/05/22 07:53:22  standa
 * The PureC -> gcc conversion (see the CONFIGVARS).
 * MiNT .XFS sources added.
 *
 *
 */
