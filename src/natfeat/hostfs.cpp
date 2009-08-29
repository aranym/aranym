/*
 * hostfs.cpp - HostFS routines
 *
 * Copyright (c) 2001-2009 STanda of ARAnyM development team (see AUTHORS)
 *
 * This file is part of the ARAnyM project which builds a new and powerful
 * TOS/FreeMiNT compatible virtual machine running on almost any hardware.
 *
 * ARAnyM is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * ARAnyM is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ARAnyM; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "sysdeps.h"
#include <assert.h>

#ifdef HOSTFS_SUPPORT

#include "cpu_emulation.h"
#include "main.h"

#include "parameters.h"
#include "host_filesys.h"
#include "toserror.h"
#include "hostfs.h"
#include "tools.h"

#undef  DEBUG_FILENAMETRANSFORMATION
#undef DEBUG
#define DEBUG 0
#include "debug.h"

#if 0
# define DEBUG_FILENAMETRANSFORMATION
# define DFNAME(x) D2(x)
#else
# define DFNAME(x)
#endif

#ifdef HAVE_SYS_VFS_H
# include <sys/vfs.h>
#endif

# include <cerrno>
# include <cstdlib>
# include <cstring>

#ifdef OS_mint
#include <mint/osbind.h>
#include <mint/mintbind.h>
#endif /* OS_mint */

#include "../../atari/hostfs/hostfs_nfapi.h"	/* XFS_xx and DEV_xx enum */

// TODO FIXME the following POSIX emulation for MINGW32/WinAPI should be moved elsewhere...
#ifdef __MINGW32__
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
#define S_IFLNK 0
#define O_NONBLOCK 0
#define O_NOCTTY 0
#define S_IRWXG 0
#define S_IRWXO 0
#define _PC_LINK_MAX 2

static inline int readlink(const char *path, char *buf, size_t bufsiz)
	{ errno = ENOSYS; return -1; }
static inline int symlink(const char *oldpath, const char *newpath)
	{ errno = ENOSYS; return -1; }
static inline long pathconf(const char *file_name, int name)
	{ errno = ENOSYS; return -1; }
static inline int truncate(const char *file_name, off_t name)
	{ errno = ENOSYS; return -1; } // FIXME truncate has to be there somewhere
static inline int lstat(const char *file_name, struct stat *buf)
	{ return stat(file_name, buf); }

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

/* linux-compatible values for fs type */
#define MSDOS_SUPER_MAGIC     0x4d44
#define NTFS_SUPER_MAGIC      0x5346544E

/**
 * @author Prof. A Olowofoyeku (The African Chief)
 * @author Frank Heckenbach
 * @see http://gd.tuwien.ac.at/gnu/mingw/os-hacks.h
 */
int statfs(const char *path, struct statfs *buf)
{
  char tmp[MAX_PATH], resolved_path[MAX_PATH];
  int retval = 0;

  errno = 0;

  // FIXME add realpath (the strcpy is just a hack!)
  // realpath(path, resolved_path);
  strcpy(resolved_path, path);

  long sectors_per_cluster, bytes_per_sector;
  if(!GetDiskFreeSpaceA(resolved_path, (DWORD *)&sectors_per_cluster,
                        (DWORD *)&bytes_per_sector, (DWORD *)&buf->f_bavail,
                        (DWORD *)&buf->f_blocks))
  {
    errno = ENOENT;
    retval = -1;
  }
  else {
    buf->f_bsize = sectors_per_cluster * bytes_per_sector;
    buf->f_files = buf->f_blocks;
    buf->f_ffree = buf->f_bavail;
    buf->f_bfree = buf->f_bavail;
  }

  /* get the FS volume information */
  if(strspn(":", resolved_path) > 0)
    resolved_path[3] = '\0';    /* we want only the root */
  if(GetVolumeInformation
     (resolved_path, NULL, 0, (DWORD *)&buf->f_fsid, (DWORD *)&buf->f_namelen, NULL, tmp,
      MAX_PATH))
  {
    if(strcasecmp("NTFS", tmp) == 0)
    {
      buf->f_type = NTFS_SUPER_MAGIC;
    }
    else {
      buf->f_type = MSDOS_SUPER_MAGIC;
    }
  }
  else {
    errno = ENOENT;
    retval = -1;
  }
  return retval;
}
#endif // __MINGW32__

// please remember to leave this define _after_ the reqired system headers!!!
// some systems does define this to some important value for them....
#ifndef O_BINARY
# define O_BINARY 0
#endif

#ifndef S_ISSOCK
# define S_ISSOCK(m) 0
# undef  S_IFSOCK
# define S_IFSOCK 0
#endif

#ifdef HAVE_SYS_STATVFS_H
#  define STATVFS struct statvfs
#else
#    define STATVFS struct statfs
#endif

#if SIZEOF_DEV_T > 4
#    define WHEN_INT4B(x,y) y
#else
#    define WHEN_INT4B(x,y) x
#endif

// for the FS_EXT3 using host.xfs (recently changed)
#undef USE_FS_EXT3


int32 HostFs::dispatch(uint32 fncode)
{
    XfsCookie  fc;
    XfsCookie  resFc;
    XfsDir     dirh;
    ExtFile    extFile;

    D(bug("HOSTFS: calling %d", fncode));

    int32 ret = 0;
    switch (fncode) {
    	case GET_VERSION:
    		ret = HOSTFS_NFAPI_VERSION;
			D(bug("HOSTFS: version %ld", ret));
    		break;

    	case GET_DRIVE_BITS:
			ret = 0;
			for(int i=0; i<(int)(sizeof(bx_options.aranymfs)/sizeof(bx_options.aranymfs[0])); i++)
				if (bx_options.aranymfs[i].rootPath != NULL && bx_options.aranymfs[i].rootPath[0])
					ret |= (1 << i);
			D(bug("HOSTFS: drvBits %08lx", (uint32)ret));
    		break;

    	case XFS_INIT:
			ret = xfs_native_init( getParameter(0),
								   getParameter(1),
								   getParameter(2),
								   getParameter(3),
								   getParameter(4),
								   getParameter(5) );
			break;

		case XFS_ROOT:
			D(bug("%s", "fs_root"));
			fetchXFSC( &fc, getParameter(1) );
			ret = xfs_root( getParameter(0),   /* dev */
							&fc ); /* fcookie */
			flushXFSC( &fc, getParameter(1) );
			break;

		case XFS_LOOKUP:
			D(bug("%s", "fs_lookup"));
			fetchXFSC( &fc, getParameter(0) );
			ret = xfs_lookup( &fc,
							  (memptr)getParameter(1), /* name */
							  &resFc );
			flushXFSC( &fc, getParameter(0) );
			flushXFSC( &resFc, getParameter(2) );
			break;

		case XFS_CREATE:
			D(bug("%s", "fs_creat"));
			fetchXFSC( &fc, getParameter(0) );
			ret = xfs_creat( &fc,
							 (memptr)getParameter(1) /* name */,
							 getParameter(2) /* mode */,
							 getParameter(3) /* attrib */,
							 &resFc );
			flushXFSC( &fc, getParameter(0) );
			flushXFSC( &resFc, getParameter(4) );
			break;

		case XFS_GETDEV:
			D(bug("%s", "fs_getdev"));
			fetchXFSC( &fc, getParameter(0) );
			ret = xfs_getdev( &fc,
							  getParameter(1) /* *devspecial */ );
			break;

		case XFS_GETXATTR:
			D(bug("%s", "fs_getxattr"));
			fetchXFSC( &fc, getParameter(0) );
			ret = xfs_getxattr( &fc,
								getParameter(1) /* XATTR* */ );
			flushXFSC( &fc, getParameter(0) );
			break;

		case XFS_STAT64:
			D(bug("%s", "fs_stat64"));
			fetchXFSC( &fc, getParameter(0) );
			ret = xfs_stat64( &fc,
							  getParameter(1) /* STAT* */ );
			flushXFSC( &fc, getParameter(0) );
			break;

		case XFS_CHATTR:
			D(bug("%s", "fs_chattr"));
			fetchXFSC( &fc, getParameter(0) );
			ret = xfs_chattr( &fc,
							  getParameter(1) /* mode */ );
			break;

		case XFS_CHOWN:
			D(bug("%s", "fs_chown"));
			D(bug("fs_chown - TODO: NOT IMPLEMENTED!"));
			ret = TOS_E_OK;
			break;

		case XFS_CHMOD:
			D(bug("%s", "fs_chmod"));
			fetchXFSC( &fc, getParameter(0) );
			ret = xfs_chmod( &fc,
							 getParameter(1) /* mode */ );
			break;

		case XFS_MKDIR:
			D(bug("%s", "fs_mkdir"));
			fetchXFSC( &fc, getParameter(0) );
			ret = xfs_mkdir( &fc,
							 (memptr)getParameter(1) /* name */,
							 getParameter(2) /* mode */ );
			break;

		case XFS_RMDIR:
			D(bug("%s", "fs_rmdir"));
			fetchXFSC( &fc, getParameter(0) );
			ret = xfs_rmdir( &fc,
							 (memptr)getParameter(1) /* name */ );
			break;

		case XFS_REMOVE:
			D(bug("%s", "fs_remove"));
			fetchXFSC( &fc, getParameter(0) );
			ret = xfs_remove( &fc,
							  (memptr)getParameter(1) /* name */ );
			flushXFSC( &fc, getParameter(0) );
			break;

		case XFS_GETNAME:
			D(bug("%s", "fs_getname"));
			fetchXFSC( &fc, getParameter(0) );
			fetchXFSC( &resFc, getParameter(1) );
			ret = xfs_getname( &fc,
							   &resFc,
							   (memptr)getParameter(2), // pathName
							   getParameter(3) ); // size
			// not needed: flushXFSC( &fc, getParameter(0) );
			// not needed: flushXFSC( &resFc, getParameter(1) );
			break;

		case XFS_RENAME:
			D(bug("%s", "fs_rename"));
			fetchXFSC( &fc, getParameter(0) );
			fetchXFSC( &resFc, getParameter(2) );
			ret = xfs_rename( &fc,
							  (memptr)getParameter(1), /* oldName */
							  &resFc,
							  (memptr)getParameter(3) /* newName */ );
			// not needed: flushXFSC( &fc, getParameter(0) );
			// not needed: flushXFSC( &resFc, getParameter(1) );
			break;

		case XFS_OPENDIR:
			D(bug("%s", "fs_opendir"));
			fetchXFSD( &dirh, getParameter(0) );
			ret = xfs_opendir( &dirh,
							   getParameter(1) /* flags */ );
			flushXFSD( &dirh, getParameter(0) );
			break;

		case XFS_READDIR:
			D(bug("%s", "fs_readdir"));
			fetchXFSD( &dirh, getParameter(0) );
			ret = xfs_readdir( &dirh,
							   (memptr)getParameter(1) /* name */,
							   getParameter(2) /* namelen */,
							   &resFc );
			flushXFSD( &dirh, getParameter(0) );
			flushXFSC( &resFc, getParameter(3) );
			break;

		case XFS_REWINDDIR:
			D(bug("%s", "fs_rewinddir"));
			fetchXFSD( &dirh, getParameter(0) );
			ret = xfs_rewinddir( &dirh );
			flushXFSD( &dirh, getParameter(0) );
			break;

		case XFS_CLOSEDIR:
			D(bug("%s", "fs_closedir"));
			fetchXFSD( &dirh, getParameter(0) );
			ret = xfs_closedir( &dirh );
			flushXFSD( &dirh, getParameter(0) );
			break;

		case XFS_PATHCONF:
			D(bug("%s", "fs_pathconf"));
			fetchXFSC( &fc, getParameter(0) );
			ret = xfs_pathconf( &fc,
								getParameter(1) /* which */ );
			flushXFSC( &fc, getParameter(0) );
			break;

		case XFS_DFREE:
			D(bug("%s", "fs_dfree"));
			fetchXFSC( &fc, getParameter(0) );
			ret = xfs_dfree( &fc,
							 (memptr)getParameter(1) /* buff */ );
			break;

		case XFS_WRITELABEL:
			D(bug("%s", "fs_writelabel"));
			ret = TOS_EINVFN;
			break;

		case XFS_READLABEL:
			D(bug("%s", "fs_readlabel"));
			fetchXFSC( &fc, getParameter(0) );
			ret = xfs_readlabel( &fc, 
								 (memptr)getParameter(1) /* buff */,
								 getParameter(2) /* len */ );
			break;

		case XFS_SYMLINK:
			D(bug("%s", "fs_symlink"));
			fetchXFSC( &fc, getParameter(0) );
			ret = xfs_symlink( &fc,
							   (memptr)getParameter(1), /* fromName */
							   (memptr)getParameter(2)  /* toName */ );
			break;

		case XFS_READLINK:
			D(bug("%s", "fs_readlink"));
			fetchXFSC( &fc, getParameter(0) );
			ret = xfs_readlink( &fc,
								(memptr)getParameter(1) /* buff */,
								getParameter(2) /* len */ );
			break;

		case XFS_HARDLINK:
			D(bug("%s", "fs_hardlink"));
			D(bug("fs_hardlink - TODO: NOT IMPLEMENTED!"));
			ret = TOS_E_OK;
			break;

		case XFS_FSCNTL:
			fetchXFSC( &fc, getParameter(0) );
			D(bug("fs_fscntl '%c'<<8|%d", (getParameter(2)>>8)&0xff ? (char)(getParameter(2)>>8)&0xff : 0x20, (char)(getParameter(2)&0xff)));
			ret = xfs_fscntl( &fc,
							  (memptr)getParameter(1) /* name */,
							  getParameter(2) /* cmd */,
							  getParameter(3) /* arg */ );
			flushXFSC( &fc, getParameter(0) );
			break;

		case XFS_DSKCHNG:
			D(bug("%s", "fs_dskchng (dummy - not needed)"));
			ret = TOS_E_OK;
			break;

		case XFS_RELEASE:
			D(bug("%s", "fs_release"));
			fetchXFSC( &fc, getParameter(0) );
			ret = xfs_release( &fc );
			flushXFSC( &fc, getParameter(0) );
			break;

		case XFS_DUPCOOKIE:
			fetchXFSC( &resFc, getParameter(0) );
			fetchXFSC( &fc, getParameter(1) );
			D(bug("fs_dupcookie( %s )", cookie2Pathname( fc.drv, fc.index, NULL, NULL )));
			ret = xfs_dupcookie( &resFc, &fc );
			flushXFSC( &resFc, getParameter(0) );
			flushXFSC( &fc, getParameter(1) );
			break;

		case XFS_SYNC:
			D(bug("%s", "fs_sync"));
			ret = TOS_EINVFN;
			break;
		case XFS_MKNOD:
			D(bug("%s", "fs_mknod"));
			ret = TOS_EINVFN;
			break;
		case XFS_UNMOUNT:
			D(bug("%s", "fs_unmount"));
			ret = TOS_EINVFN;
			break;

		case DEV_OPEN:
			D(bug("%s", "fs_dev_open"));
			fetchXFSF( &extFile, getParameter(0) );
			ret = xfs_dev_open( &extFile );
			flushXFSF( &extFile, getParameter(0) );
			break;

		case DEV_WRITE:
			D(bug("%s", "fs_dev_write"));
			fetchXFSF( &extFile, getParameter(0) );
			ret = xfs_dev_write( &extFile,
								 (memptr)getParameter(1) /* buffer */,
								 getParameter(2) /* bytes */ );
			flushXFSF( &extFile, getParameter(0) );
			break;

		case DEV_READ:
			D(bug("%s", "fs_dev_read"));
			fetchXFSF( &extFile, getParameter(0) );
			ret = xfs_dev_read( &extFile,
								(memptr)getParameter(1) /* buffer */,
								getParameter(2) /* bytes */ );
			flushXFSF( &extFile, getParameter(0) );
			break;

		case DEV_LSEEK:
			D(bug("%s", "fs_dev_lseek"));
			fetchXFSF( &extFile, getParameter(0) );
			ret = xfs_dev_lseek( &extFile,
								 getParameter(1),		  // offset
								 getParameter(2) );		  // seekmode
			// not needed: flushXFSF( &extFile, getParameter(0) );
			break;

		case DEV_IOCTL:
			fetchXFSF( &extFile, getParameter(0) );
			D(bug("fs_dev_ioctl '%c'<<8|%d", (getParameter(1)>>8)&0xff ? (char)(getParameter(1)>>8)&0xff : 0x20, (char)(getParameter(1)&0xff)));
			D(bug("fs_dev_ioctl - TODO: NOT IMPLEMENTED!"));
			ret = TOS_EINVFN;
			break;

		case DEV_DATIME:
			D(bug("%s", "fs_dev_datime"));
			fetchXFSF( &extFile, getParameter(0) );
			ret = xfs_dev_datime( &extFile,
								  (memptr)getParameter(1), // datetimep
								  getParameter(2) );// wflag
			flushXFSF( &extFile, getParameter(0) );
			break;

		case DEV_CLOSE:
			D(bug("%s", "fs_dev_close"));
			fetchXFSF( &extFile, getParameter(0) );
			ret = xfs_dev_close( &extFile,
								 0 ); // pid
			flushXFSF( &extFile, getParameter(0) );
			break;

		case DEV_SELECT:
			D(bug("%s", "fs_dev_select"));
			D(bug("fs_dev_select - TODO: NOT IMPLEMENTED!"));
			ret = TOS_E_OK;
			break;

		case DEV_UNSELECT:
			D(bug("%s", "fs_dev_unselect"));
			D(bug("fs_dev_unselect - TODO: NOT IMPLEMENTED!"));
			ret = TOS_E_OK;
			break;

		default:
			panicbug("Unknown HOSTFS subID %d", fncode);
			ret = TOS_EINVFN;
	}
	return ret;
}

void HostFs::fetchXFSC( XfsCookie *fc, memptr filep )
{
	fc->xfs	  = ReadInt32( filep );	 // fs
	fc->dev	  = ReadInt16( filep + 4 );	 // dev
	fc->aux	  = ReadInt16( filep + 6 );	 // aux
	fc->index = (XfsFsFile*)MAP32TOVOIDP( ReadInt32( filep + 8 ) ); // index

	MountMap::iterator it = mounts.find(fc->dev);
	fc->drv   = ( it != mounts.end() ? it->second : NULL );
}
void HostFs::flushXFSC( XfsCookie *fc, memptr filep )
{
	WriteInt32( filep	 , fc->xfs );
	WriteInt16( filep + 4, fc->dev );
	WriteInt16( filep + 6, fc->aux );
	WriteInt32( filep + 8, (uint32)MAPVOIDPTO32( fc->index ) );
}

void HostFs::fetchXFSF( ExtFile *extFile, memptr filep )
{
	extFile->links	= ReadInt16( filep );
	extFile->flags	= ReadInt16( filep + 2 );
#if SIZEOF_INT != 4 || DEBUG_NON32BIT
	extFile->hostFd = fdMapper.getNative( ReadInt32( filep + 4 ) ); // offset not needed (replaced by the host fd)
#else
	extFile->hostFd = (int)ReadInt32( filep + 4 ); // offset not needed (replaced by the host fd)
#endif
	fetchXFSC( &extFile->fc, filep + 12 ); // sizeof(12)
}
void HostFs::flushXFSF( ExtFile *extFile, memptr filep )
{
	WriteInt16( filep, extFile->links );
	WriteInt16( filep + 2, extFile->flags );
#if SIZEOF_INT != 4 || DEBUG_NON32BIT
	WriteInt32( filep + 4, fdMapper.get32bit( extFile->hostFd ) ); // instead of the offset
#else
	WriteInt32( filep + 4, (uint32)extFile->hostFd ); // instead of the offset
#endif
	flushXFSC( &extFile->fc, filep + 12 ); // sizeof(12)
}

void HostFs::fetchXFSD( XfsDir *dirh, memptr dirp )
{
	fetchXFSC( (XfsCookie*)dirh, dirp ); // sizeof(12)
	dirh->index = ReadInt16( dirp + 12 );
	dirh->flags = ReadInt16( dirp + 14 );
	Atari2Host_memcpy( (char*)&dirh->hostDir, dirp + 18, sizeof(dirh->hostDir) );
}
void HostFs::flushXFSD( XfsDir *dirh, memptr dirp )
{
	flushXFSC( (XfsCookie*)dirh, dirp ); // sizeof(12)
	WriteInt16( dirp + 12, dirh->index );
	WriteInt16( dirp + 14, dirh->flags );
	Host2Atari_memcpy( dirp + 18, (char*)&dirh->hostDir, sizeof(dirh->hostDir) );
}


uint32 HostFs::errnoHost2Mint(int unixerrno,int defaulttoserrno)
{
	int retval = defaulttoserrno;

	switch(unixerrno) {
		case EACCES:
		case EPERM:
		case ENFILE:  retval = TOS_EACCDN;break;
		case EBADF:	  retval = TOS_EIHNDL;break;
		case ENOTDIR: retval = TOS_EPTHNF;break;
		case EROFS:   retval = TOS_EROFS;break;
		case ENOENT: /* GEMDOS most time means it should be a file */
		case ECHILD:  retval = TOS_EFILNF;break;
		case ENXIO:	  retval = TOS_EDRIVE;break;
		case EIO:	  retval = TOS_EIO;break;	 /* -90 I/O error */
		case ENOEXEC: retval = TOS_EPLFMT;break;
		case ENOMEM:  retval = TOS_ENSMEM;break;
		case EFAULT:  retval = TOS_EIMBA;break;
		case EEXIST:  retval = TOS_EEXIST;break; /* -85 file exist, try again later */
		case EXDEV:	  retval = TOS_ENSAME;break;
		case ENODEV:  retval = TOS_EUNDEV;break;
		case EINVAL:  retval = TOS_EINVFN;break;
		case EMFILE:  retval = TOS_ENHNDL;break;
		case ENOSPC:  retval = TOS_ENOSPC;break; /* -91 disk full */
		case ENAMETOOLONG: retval = TOS_ERANGE; break;
	}

	D(bug("HOSTFS: errnoHost2Mint (%d,%d->%d)", unixerrno, defaulttoserrno, retval));

	return retval;
}


uint16 HostFs::time2dos(time_t t)
{
	struct tm *x;
	x = localtime (&t);
	D2(bug("HOSTFS: time2dos (%d:%d:%d)", x->tm_hour, x->tm_min, x->tm_sec));
	return ((x->tm_sec&0x3f)>>1)|((x->tm_min&0x3f)<<5)|((x->tm_hour&0x1f)<<11);
}


uint16 HostFs::date2dos(time_t t)
{
	struct tm *x;
	x = localtime (&t);
	D2(bug("HOSTFS: date2dos (%d,%d,%d)", x->tm_mday, x->tm_mon, x->tm_year-80+1980));
	return x->tm_mday|((x->tm_mon+1)<<5)|(MAX(x->tm_year-80,0)<<9);
}


void HostFs::datetime2tm(uint32 dtm, struct tm* ttm)
{
	ttm->tm_mday = dtm & 0x1f;
	ttm->tm_mon	 = ((dtm>>5) & 0x0f) - 1;
	ttm->tm_year = ((dtm>>9) & 0x7f) + 80;
	ttm->tm_sec	 = ((dtm>>16) & 0x1f) << 1;
	ttm->tm_min	 = (dtm>>21) & 0x3f;
	ttm->tm_hour = (dtm>>27) & 0x1f;
}


uint16 HostFs::modeHost2TOS(mode_t m)
{
	return ( S_ISDIR(m) ) ? 0x10 : 0;	/* FIXME */
	//	  if (!(da == 0 || ((da != 0) && (attribs != 8)) || ((attribs | 0x21) & da)))
}


/**
 * Convert the unix file stat.st_mode value into a FreeMiNT's one.
 *
 * File permissions and mode bitmask according
 * to the FreeMiNT 1.16.x stat.h header file.
 *
 *  Name     Mask     Permission
 * S_IXOTH  0000001  Execute permission for all others.
 * S_IWOTH  0000002  Write permission for all others.
 * S_IROTH  0000004  Read permission for all others.
 * S_IXGRP  0000010  Execute permission for processes with same group ID.
 * S_IWGRP  0000020  Write permission for processes with same group ID.
 * S_IRGRP  0000040  Read permission for processes with same group ID.
 * S_IXUSR  0000100  Execute permission for processes with same user ID.
 * S_IWUSR  0000200  Write permission for processes with same user ID.
 * S_IRUSR  0000400  Read permission for processes with same user ID.

 * S_ISVTX  0001000  Sticky bit
 * S_ISGID  0002000  Alter effective group ID when executing this file.
 * S_ISUID  0004000  Alter effective user ID when executing this file.
 *
 * S_IFSOCK 0010000  File is a FreeMiNTNet socket file.
 * S_IFCHR  0020000  File is a BIOS (character) special file.
 * S_IFDIR  0040000  File is a directory.
 * S_IFBLK  0060000  File is a block special file.
 * S_IFREG  0100000  File is a regular file.
 * S_IFIFO  0120000  File is a FIFO (named pipe).
 * S_IMEM   0140000  File is a memory region.
 * S_IFLNK  0160000  File is a symbolic link.
 */
uint16 HostFs::modeHost2Mint(mode_t m)
{
	uint16 result = 0;

	// permissions
	if ( m & S_IXOTH ) result |= 00001;
	if ( m & S_IWOTH ) result |= 00002;
	if ( m & S_IROTH ) result |= 00004;
	if ( m & S_IXGRP ) result |= 00010;
	if ( m & S_IWGRP ) result |= 00020;
	if ( m & S_IRGRP ) result |= 00040;
	if ( m & S_IXUSR ) result |= 00100;
	if ( m & S_IWUSR ) result |= 00200;
	if ( m & S_IRUSR ) result |= 00400;
	if ( m & S_ISVTX ) result |= 01000;
	if ( m & S_ISGID ) result |= 02000;
	if ( m & S_ISUID ) result |= 03000;

	if ( S_ISSOCK(m) ) result |= 0010000;
	if ( S_ISCHR(m)  ) result |= 0020000;
	if ( S_ISDIR(m)  ) result |= 0040000;
	if ( S_ISBLK(m)  ) result |= 0060000;
	if ( S_ISREG(m)  ) result |= 0100000;
	if ( S_ISFIFO(m) ) result |= 0120000;
	// Linux doesn't have this! if ( S_ISMEM(m)  ) result |= 0140000;
	if ( S_ISLNK(m)  ) result |= 0160000;

	D2(bug("					(statmode: %#4o)", result));

	return result;
}


mode_t HostFs::modeMint2Host(uint16 m)
{
	mode_t result = 0;

	// permissions
	if ( m & 00001 ) result |= S_IXOTH;
	if ( m & 00002 ) result |= S_IWOTH;
	if ( m & 00004 ) result |= S_IROTH;
	if ( m & 00010 ) result |= S_IXGRP;
	if ( m & 00020 ) result |= S_IWGRP;
	if ( m & 00040 ) result |= S_IRGRP;
	if ( m & 00100 ) result |= S_IXUSR;
	if ( m & 00200 ) result |= S_IWUSR;
	if ( m & 00400 ) result |= S_IRUSR;
	if ( m & 01000 ) result |= S_ISVTX;
	if ( m & 02000 ) result |= S_ISGID;
	if ( m & 03000 ) result |= S_ISUID;

	if ( m & 0010000 ) result |= S_IFSOCK;
	if ( m & 0020000 ) result |= S_IFCHR;
	if ( m & 0040000 ) result |= S_IFDIR;
	if ( m & 0060000 ) result |= S_IFBLK;
	if ( m & 0100000 ) result |= S_IFREG;
	if ( m & 0120000 ) result |= S_IFIFO;
	// Linux doesn't have this!	if ( m & 0140000 ) result |= S_IFMEM;
	if ( m & 0160000 ) result |= S_IFLNK;

	D2(bug("					(statmode: %#04o)", result));

	return result;
}


int HostFs::flagsMint2Host(uint16 flags)
{
	int res = O_RDONLY;

	/* exclusive */
	if (flags & 0x1)
		res = O_WRONLY;
	if (flags & 0x2)
		res = O_RDWR;
	if ((flags & 0x3) == 0x3) /* FreeMiNT kernel O_EXEC */
		res = O_RDONLY;

	if (flags & 0x200)
		res |= O_CREAT;
	if (flags & 0x400)
		res |= O_TRUNC;
	if (flags & 0x800)
		res |= O_EXCL;
	if (flags & 0x1000)
		res |= O_APPEND;
	if (flags & 0x100)
		res |= O_NONBLOCK;
	if (flags & 0x4000)
		res |= O_NOCTTY;

	return res;
}


int16 HostFs::flagsHost2Mint(int flags)
{
	int16 res = 0; /* default read only */

	/* exclusive */
	if (!(flags & (O_WRONLY|O_RDWR)))
		res = 0;
	if (flags & O_WRONLY)
		res = 1; /* write only/ kludge to avoid files being created */
	if (flags & O_RDWR)
		res = 2;

	if (flags & O_CREAT)
		res |= 0x200;
	if (flags & O_TRUNC)
		res |= 0x400;
	if (flags & O_EXCL)
		res |= 0x800;
	if (flags & O_APPEND)
		res |= 0x1000;
	if (flags & O_NONBLOCK)
		res |= 0x100;
	if (flags & O_NOCTTY)
		res |= 0x4000;

	return res;
}

/***
 * Long filename to 8+3 transformation.
 * The extensions, if exists in the original filename, are only shortened
 * and never appended with anything due to the filename extension driven
 * file type recognition posibility used by nearly all desktop programs.
 *
 * The translation rules are:
 *	 The filename that has no extension and:
 *		- is max 11 chars long is splited to the filename and extension
 *		  just by inserting a dot to the 8th position (example 1).
 *		- is longer than 11 chars is shortened to 8 chars and appended
 *		  with the hashcode extension (~XX... example 2).
 *	 The filename is over 8 chars:
 *		the filename is shortened to 5 and appended with the hashcode put
 *		into the name part (not the extention... example 3). The extension
 *		shortend to max 3 chars and appended too.
 *	 The extension is over 3 chars long:
 *		The filename is appended with the hashcode and the extension is
 *		shortened to max 3 chars (example 4).
 *
 * Examples:
 *	 1. longnamett		  -> longname.tt
 *	 2. longfilename	  -> longfile.~XX
 *	 3. longfilename.ext  -> longf~XX.ext
 *	 4. file.html		  -> file~XX.htm
 *
 * @param dest	 The buffer to put the filename (max 12 char).
 * @param source The source filename string.
 *
 **/
void HostFs::transformFileName( char* dest, const char* source )
{
	DFNAME(bug("HOSTFS: transformFileName(\"%s\")...", source));

	// . and .. system folders
	if ( strcmp(source, ".") == 0 || strcmp(source, "..") == 0) {
		strcpy(dest, source);	// copy to final 8+3 buffer
		return;
	}

	// Get file name (strip off file extension)
	char *dot = (char *)strrchr( source, '.' );
	// ignore leading dot
	if (dot == source)
		dot = NULL;

	// find out dot position
	ssize_t len = strlen( source );
	ssize_t nameLen = ( dot == NULL ) ? len : dot - source;
	ssize_t extLen	= ( nameLen == len ) ? 0 : len - nameLen - 1;

	// copy the name... (max 12 chars due to the buffer length limitation)
	strncpy(dest, source, 12);
	dest[12] = '\0';

	DFNAME(bug("HOSTFS: transformFileName:... nameLen = %i, extLen = %i", nameLen, extLen));

	if (	nameLen > 8 || extLen > 3 ||  // the filename is longer than the 8+3 standard
		( nameLen <= 8 && extLen == 0 && dot != NULL ) )  // there is a dot at the end of the filename
	{
		// calculate a hash value from the long name
		uint32 hashValue = 0;
		for( int i=0; source[i]; i++ )
			hashValue += (hashValue << 3) + source[i];

		// hash value hex string as the unique shortenning
		char hashString[10];
		sprintf( hashString, "%08x", hashValue );
		hashString[5] = '~';
		char *hashStr = &hashString[5];

		if ( extLen == 0 ) {
			if ( nameLen >= 12 ) {
				// filename is longer than 8+3 and has no extension
				// -> put the hash string as the file extension
				nameLen = 8;
				extLen = 3;
				dot = hashStr;
			} else {
				// filename is max 11 chars long and no extension
				if ( dot == NULL ) {
					// no dot at the end -> insert the . char into the name after the 8th char
					nameLen = 8;
					dot = (char*)source + nameLen;
				} else {
					// there is a . at the end of the filename
					nameLen = MIN(8,nameLen);
					dot = hashStr;
				}
				extLen = 3;
			}
		} else {
			// shorten the name part to max 5
			nameLen = MIN(5,nameLen);
			// add the hash string
			memcpy( &dest[nameLen], hashStr, 4 ); // including the trailing \0!
			nameLen+=3;
			dot++;
		}

		if ( extLen > 0 ) {
			// and the extension
			extLen = MIN(3,extLen);
			dest[nameLen] = '.';
			strncpy(&dest[nameLen+1], dot, extLen);
		}

		dest[ nameLen+extLen+1 ] = '\0';
	}

	// replace spaces and dots in the filename with the _
	char *temp = dest;
	char *brkPos;
	while ( (brkPos = strpbrk( temp, " ." )) != NULL ) {
		*brkPos = '_';
		temp = brkPos + 1;
	}

	// set the extension separator
	if ( dot != NULL )
		dest[ nameLen ] = '.';

	// upper case conversion
	strapply( dest, toupper );

	DFNAME(bug("HOSTFS: /transformFileName(\"%s\") -> \"%s\"", source, dest));
}


bool HostFs::getHostFileName( char* result, ExtDrive* drv, char* pathName, const char* name )
{
	struct stat statBuf;

	DFNAME(bug("HOSTFS: getHostFileName (%s,%s)", pathName, name));

	// if the whole thing fails then take the requested name as is
	// it also completes the path
	strcpy( result, name );

	if ( ! strpbrk( name, "*?" ) && // if is it NOT a mask
		 stat(pathName, &statBuf) ) // and if such file NOT really exists
	{
		// the TOS filename was adjusted (lettercase, length, ..)
		char testName[MAXPATHNAMELEN];
		const char *finalName = name;
		struct dirent *dirEntry;
		bool nonexisting = false;

		DFNAME(bug(" (stat failed)"));

		// shorten the name from the pathName;
		*result = '\0';

		DIR *dh = host_opendir( pathName );
		if ( dh == NULL ) {
			DFNAME(bug("HOSTFS: getHostFileName dopendir(%s) failed.", pathName));
			goto lbl_final;	 // should never happen
		}

		while ( true ) {
			if ((dirEntry = readdir( dh )) == NULL) {
				DFNAME(bug("HOSTFS: getHostFileName dreaddir: no more files."));
				nonexisting = true;
				goto lbl_final;
			}

			if ( !drv || drv->halfSensitive )
				if ( ! strcasecmp( name, dirEntry->d_name ) ) {
					finalName = dirEntry->d_name;
					DFNAME(bug("HOSTFS: getHostFileName found final file."));
					goto lbl_final;
				}

			transformFileName( testName, dirEntry->d_name );

			DFNAME(bug("HOSTFS: getHostFileName (%s,%s,%s)", name, testName, dirEntry->d_name));

			if ( ! strcmp( testName, name ) ) {
				// FIXME isFile test (maybe?)
				// this follows one more argument to be passed

				finalName = dirEntry->d_name;
				goto lbl_final;
			}
		}

	lbl_final:
		DFNAME(bug("HOSTFS: getHostFileName final (%s,%s)", name, finalName));

		strcpy( result, finalName );

		// in case of halfsensitive filesystem,
		// an upper case filename should be lowecase?
		if ( nonexisting && (!drv || drv->halfSensitive) ) {
			bool isUpper = true;
			for( char *curr = result; *curr; curr++ ) {
				if ( *curr != toupper( *curr ) ) {
					isUpper = false;
					break;
				}
			}
			if ( isUpper ) {
				// lower case conversion
				strapply( result, tolower );
			}
		}
		if ( dh != NULL )
			closedir( dh );
	}
#ifdef DEBUG_FILENAMETRANSFORMATION
	else
		DFNAME(bug(" (stat OK)"));
#endif

	return true;
}


/*
 * build a complete linux filepath
 * --> fs	 something like a handle to a known file
 *	   name	 a postfix to the path of fs or NULL, if no postfix
 *	   buf	 buffer for the complete filepath or NULL if static one
 *			 should be used.
 * <-- complete filepath or NULL if error
 */
char *HostFs::cookie2Pathname( ExtDrive *drv, HostFs::XfsFsFile *fs, const char *name, char *buf )
{
	static char sbuf[MAXPATHNAMELEN]; /* FIXME: size should told by unix */

	if (!buf)
		// use static buffer
		buf = sbuf;

	if (!fs) {
		// we are at root
		D2(bug("HOSTFS: cookie2pathname root? '%s'", name));
		if (!name)
			return NULL;

		// in the root cookie there is the host path that
		// needs no modification (drv->hostRoot)
		//  -> copy it to the buffer and return
		strcpy(buf, name);
		return buf;
	}

	// recurse to deep
	if (!cookie2Pathname(drv, fs->parent, fs->name, buf))
		return NULL;

	// returning from the recursion -> append the appropriate filename
	if (name && *name)
	{
		// make sure there's the right trailing dir separator
		int len = strlen(buf);
		if (len > 0) {
			char *last = buf + len-1;
			if (*last == '\\' || *last == '/') {
				*last = '\0';
			}
			strcat(buf, DIRSEPARATOR);
		}
		getHostFileName( buf + strlen(buf), drv, buf, name );
	}

	D2(bug("HOSTFS: cookie2pathname '%s'", buf));
	return buf;
}

char *HostFs::cookie2Pathname( HostFs::XfsCookie *fc, const char *name, char *buf )
{
	return cookie2Pathname( fc->drv, fc->index, name, buf );
}

#if DEBUG
void HostFs::debugCookie( HostFs::XfsCookie *fc )
{
	D(bug( "release():\n"
		 "	fc = %08lx\n"
		 "	  fs	= %08lx\n"
		 "	  dev	= %04x\n"
		 "	  aux	= %04x\n"
		 "	  index = %08lx\n"
		 "		parent	 = %08lx\n"
		 "		name	 = \"%s\"\n"
		 "		usecnt	 = %d\n"
		 "		childcnt = %d\n",
		 (long)fc,
		 (long)fc->xfs,
		 (int)fc->dev,
		 (int)fc->aux,
		 (long)fc->index,
		 (long)fc->index->parent,
		   fc->index->name,
		   fc->index->refCount,
		   fc->index->childCount ));
}
#endif

int32 HostFs::host_statvfs( const char *fpathName, void *buff )
{
	D(bug("HOSTFS: fs_dfree (%s)", fpathName));

#ifdef HAVE_SYS_STATVFS_H
	if ( statvfs(fpathName, (STATVFS *)buff) )
#else
	if ( statfs(fpathName, (STATVFS *)buff) )
#endif
		return errnoHost2Mint(errno,TOS_EFILNF);

	return TOS_E_OK;
}


int32 HostFs::xfs_dfree( XfsCookie *dir, uint32 diskinfop )
{
	char fpathName[MAXPATHNAMELEN];
	cookie2Pathname( dir, NULL, fpathName );

	STATVFS buff;
	int32 res = host_statvfs( fpathName, &buff);
	if ( res != TOS_E_OK )
		return res;

	/* ULONG b_free	   */  WriteInt32( diskinfop	   , buff.f_bavail );
	/* ULONG b_total   */  WriteInt32( diskinfop +	4, buff.f_blocks );
	/* ULONG b_secsize */  WriteInt32( diskinfop +	8, buff.f_bsize /* not 512 according to stonx_fs */ );
	/* ULONG b_clsize  */  WriteInt32( diskinfop + 12, 1 );

	return TOS_E_OK;
}


//	fake Dreadlabel, extract the last folder name of the host path being root out the mount on 68k side
int32 HostFs::xfs_readlabel(XfsCookie * dir, memptr buff, int16 len) {
	// dir->drv->hostRoot contains the full path of the folder mounted to a drive
	// to omitt very long label names, we stick to the last folder name
	size_t	hostrootlen;
	if (
			dir && dir->drv && dir->drv->hostRoot
		&&	(hostrootlen = strlen(dir->drv->hostRoot)) > 0
	) {
		//	it seems there is a host root path to be used
		char* startchar = dir->drv->hostRoot;	//	position on start of name
		char* poschar = &startchar[hostrootlen - 1];	//	position on last character
		if (poschar > startchar && *poschar == *DIRSEPARATOR) {
			//	ignore an ending slash
			--poschar;
		}
		if (poschar > startchar && *poschar == ':') {
			//	ignore an ending ":" from dos drive letters
			--poschar;
		}
		char* endchar = poschar;	//	remember this position as the end of hostRoot to copy
		//	search backwards for bounding slash
		while (poschar > startchar && *poschar != *DIRSEPARATOR)
			--poschar;
		if (*poschar == *DIRSEPARATOR)
			//	move to character behind that slash
			++poschar;
			
		if (poschar <= endchar) {
			//	there are some characters inbetween to copy
			hostrootlen = endchar - poschar + 2;
			if (len < 0 || hostrootlen > size_t(len)) {
				return(TOS_ENAMETOOLONG);
			} else {
				Host2AtariSafeStrncpy(buff, poschar, hostrootlen);
				return(TOS_E_OK);
			}
		}
	}
	if (len > 0) {
		//	there is no label name to extract
		//	fall back to a default label
		Host2AtariSafeStrncpy(buff, "HOSTFS", len);
		return TOS_E_OK;
	} else {
		return(TOS_ENAMETOOLONG);
	}
}


int32 HostFs::xfs_mkdir( XfsCookie *dir, memptr name, uint16 mode )
{
	char fname[MAXPATHNAMELEN];
	Atari2HostSafeStrncpy( fname, name, sizeof(fname) );

	char fpathName[MAXPATHNAMELEN];
	cookie2Pathname( dir, fname, fpathName );

	if ( HostFilesys::makeDir( (char*)fpathName, mode ) )
		return errnoHost2Mint(errno,TOS_EFILNF);

	return TOS_E_OK;
}


int32 HostFs::xfs_rmdir( XfsCookie *dir, memptr name )
{
	char fname[MAXPATHNAMELEN];
	Atari2HostSafeStrncpy( fname, name, sizeof(fname) );

	char fpathName[MAXPATHNAMELEN];
	cookie2Pathname( dir, fname, fpathName );

	if ( rmdir( fpathName ) )
		return errnoHost2Mint(errno,TOS_EFILNF);

	return TOS_E_OK;
}


int32 HostFs::xfs_creat( XfsCookie *dir, memptr name, uint16 mode, int16 flags, XfsCookie *fc )
{
	DUNUSED(flags);
	char fname[MAXPATHNAMELEN];
	Atari2HostSafeStrncpy( fname, name, sizeof(fname) );

	// convert and mask out the file type bits for unix open()
	mode = modeMint2Host( mode ) & (S_ISUID|S_ISGID|S_ISVTX|S_IRWXU|S_IRWXG|S_IRWXO);

	D(bug("HOSTFS:  dev_creat (%s, flags: %#x, mode: %#o)", fname, flags, mode));

	char fpathName[MAXPATHNAMELEN];
	cookie2Pathname(dir,fname,fpathName); // get the cookie filename

	int fd = open( fpathName, O_CREAT|O_EXCL|O_WRONLY|O_BINARY, mode );
	if (fd < 0)
		return errnoHost2Mint(errno,TOS_EFILNF);
	close( fd );

	XfsFsFile *newFsFile = new XfsFsFile();	MAPNEWVOIDP( newFsFile );
	newFsFile->name = strdup( fname );
	newFsFile->refCount = 1;
	newFsFile->childCount = 0;
	newFsFile->parent = dir->index;
	newFsFile->created = false;
	dir->index->childCount++;

	*fc = *dir;
	fc->index = newFsFile;

	return TOS_E_OK;
}


int32 HostFs::xfs_dev_open(ExtFile *fp)
{
	char fpathName[MAXPATHNAMELEN];
	cookie2Pathname(&fp->fc, NULL, fpathName);

	int flags = flagsMint2Host(fp->flags);

	D(bug("HOSTFS:  dev_open (%s, flags: %#x->%#x)", fpathName, fp->flags, flags));

	// the xfs_creat() does create the host fs file although
	// it should not according to the FreeMiNT .XFS design
	if ( !fp->fc.index->created && (flags & O_CREAT) ) {
		// if xfs_creat()'ed file is now opened with O_CREAT
		// then set the special index->created to true
		fp->fc.index->created = true;

		// crear the O_EXCL and O_CREAT flags
		flags &= ~(O_CREAT|O_EXCL);
	}

	int fd = open( fpathName, flags|O_BINARY, 0 );
	if (fd < 0)
		return errnoHost2Mint(errno,TOS_EFILNF);
	fp->hostFd = fd;

    #if SIZEOF_INT != 4 || DEBUG_NON32BIT
		fdMapper.putNative( fp->hostFd );
    #endif

	D(bug("HOSTFS: /dev_open (fd = %ld)", fp->hostFd));
	return TOS_E_OK;

}


int32 HostFs::xfs_dev_close(ExtFile *fp, int16 pid)
{
	DUNUSED(pid);
	D(bug("HOSTFS:  dev_close (fd = %d, links = %d, pid = %d)", fp->hostFd, fp->links, pid));

	if ( fp->links <= 0 ) {
		if ( close( fp->hostFd ) )
			return errnoHost2Mint(errno,TOS_EIO);

		#if SIZEOF_INT != 4 || DEBUG_NON32BIT
			fdMapper.removeNative( fp->hostFd );
		#endif

	    D(bug("HOSTFS: /dev_close (fd = %d, %d)", fp->hostFd, pid));
	}

	return TOS_E_OK;
}


#define FRDWR_BUFFER_LENGTH 8192

int32 HostFs::xfs_dev_read(ExtFile *fp, memptr buffer, uint32 count)
{
	uint8 fBuff[ FRDWR_BUFFER_LENGTH ];
	memptr	destBuff = buffer;
	ssize_t readCount = 0;
	ssize_t toRead = count;
	ssize_t toReadNow;

	D(bug("HOSTFS:  dev_read (fd = %d, %d)", fp->hostFd, count));

	while ( toRead > 0 ) {
		toReadNow = ( toRead > FRDWR_BUFFER_LENGTH ) ? FRDWR_BUFFER_LENGTH : toRead;
		readCount = read( fp->hostFd, fBuff, toReadNow );
		if ( readCount <= 0 )
			break;

		Host2Atari_memcpy( destBuff, fBuff, readCount );
		destBuff += readCount;
		toRead -= readCount;
	}

	D(bug("HOSTFS: /dev_read readCount (%d)", count - toRead));
	if ( readCount < 0 )
		return errnoHost2Mint(errno,TOS_EINTRN);

	return count - toRead;
}


int32 HostFs::xfs_dev_write(ExtFile *fp, memptr buffer, uint32 count)
{
	uint8 fBuff[ FRDWR_BUFFER_LENGTH ];
	memptr sourceBuff = buffer;
	ssize_t toWrite = count;
	ssize_t toWriteNow;
	ssize_t writeCount = 0;

	D(bug("HOSTFS:  dev_write (fd = %d, %d)", fp->hostFd, count));

	while ( toWrite > 0 ) {
		toWriteNow = ( toWrite > FRDWR_BUFFER_LENGTH ) ? FRDWR_BUFFER_LENGTH : toWrite;
		Atari2Host_memcpy( fBuff, sourceBuff, toWriteNow );
		writeCount = write( fp->hostFd, fBuff, toWriteNow );
		if ( writeCount <= 0 )
			break;

		sourceBuff += writeCount;
		toWrite -= writeCount;
	}

	D(bug("HOSTFS: /dev_write writeCount (%d)", count - toWrite));
	if ( writeCount < 0 )
		return errnoHost2Mint(errno,TOS_EINTRN);

	return count - toWrite;
}


int32 HostFs::xfs_dev_lseek(ExtFile *fp, int32 offset, int16 seekmode)
{
	int whence;

	D(bug("HOSTFS:  dev_lseek (fd = %d,offset = %d,mode = %d)", fp->hostFd, offset, seekmode));

	switch (seekmode) {
		case 0:	 whence = SEEK_SET; break;
		case 1:	 whence = SEEK_CUR; break;
		case 2:	 whence = SEEK_END; break;
		default: return TOS_EINVFN;
	}

	off_t newoff = lseek( fp->hostFd, offset, whence);

	D(bug("HOSTFS: /dev_lseek (offset = %d,mode = %d,resoffset = %d)", offset, seekmode, (int32)newoff));

	if ( newoff == -1 )
		return errnoHost2Mint(errno,TOS_EIO);

	return newoff;
}


int32 HostFs::xfs_remove( XfsCookie *dir, memptr name )
{
	char fname[MAXPATHNAMELEN];
	Atari2HostSafeStrncpy( fname, name, sizeof(fname) );

	char fpathName[MAXPATHNAMELEN];
	cookie2Pathname(dir,fname,fpathName); // get the cookie filename

	if ( unlink( fpathName ) )
		return errnoHost2Mint(errno,TOS_EFILNF);

	return TOS_E_OK;
}


int32 HostFs::xfs_rename( XfsCookie *olddir, memptr oldname, XfsCookie *newdir, memptr newname )
{
	char foldname[MAXPATHNAMELEN];
	char fnewname[MAXPATHNAMELEN];
	Atari2HostSafeStrncpy( foldname, oldname, sizeof(foldname) );
	Atari2HostSafeStrncpy( fnewname, newname, sizeof(fnewname) );

	char fpathName[MAXPATHNAMELEN];
	char fnewPathName[MAXPATHNAMELEN];
	cookie2Pathname( olddir, foldname, fpathName );
	cookie2Pathname( newdir, fnewname, fnewPathName );

	if ( rename( fpathName, fnewPathName ) )
		return errnoHost2Mint(errno,TOS_EFILNF);

	return TOS_E_OK;
}

int32 HostFs::xfs_symlink( XfsCookie *dir, memptr fromname, memptr toname )
{
	char ffromname[MAXPATHNAMELEN];
	char ftoname[MAXPATHNAMELEN];
	Atari2HostSafeStrncpy( ffromname, fromname, sizeof(ffromname) );
	Atari2HostSafeStrncpy( ftoname, toname, sizeof(ftoname) );

	char ffromName[MAXPATHNAMELEN];
	cookie2Pathname( dir, ffromname, ffromName );

	// search among the mount points to find suitable link...
	size_t toNmLen = strlen( ftoname );

	// prefer the current device
	ExtDrive *drv = dir->drv;

	MountMap::iterator it = mounts.begin();

	// no dir->drv? should never happen!
	if ( !drv ) {
		if ( it == mounts.end() ) {
			panicbug( "HOSTFS: fs_symlink: \"%s\"-->\"%s\" dir->drv == NULL && mounts.size() == 0!", ffromname, ftoname );
			return TOS_EINTRN; // FIXME?
		}

		// get the first mount
		drv = it->second;
		it++;
		bug( "HOSTFS: ERROR: fs_symlink: \"%s\"-->\"%s\" dir->drv == NULL???", ffromname, ftoname );
	}

	// compare the beginning of the link path with the current
	// mountPoint path (if matches then use the current drive)
	size_t mpLen = 0;
	while ( drv ) {
		mpLen = strlen( drv->mountPoint );
		if (mpLen < toNmLen && !strncmp( drv->mountPoint, ftoname, mpLen ) )
			break;

		// no more mountpoints available
		if (it == mounts.end()) {
			drv = NULL;
			break;
		}

		// get the ExtDrive from the mountpoint
		drv = it->second;
		// move the iterator to the next one
		it++;
	}

	char ftoName[MAXPATHNAMELEN];
	if ( drv ) {
		strcpy( ftoName, drv->hostRoot );
		strcat( ftoName, &ftoname[ mpLen ] );
	} else {
		strcpy( ftoName, ftoname );
	}

	D(bug( "HOSTFS: fs_symlink: \"%s\" --> \"%s\"", ffromName, ftoName ));

	if ( symlink( ftoName, ffromName ) )
		return errnoHost2Mint(errno,TOS_EFILNF);

	return TOS_E_OK;
}


int32 HostFs::xfs_dev_datime( ExtFile *fp, memptr datetimep, int16 wflag)
{
	char fpathName[MAXPATHNAMELEN];
	cookie2Pathname( &fp->fc, NULL, fpathName );

	D(bug("HOSTFS:  dev_datime (%s)", fpathName));
	struct stat statBuf;

	if ( stat(fpathName, &statBuf) )
		return errnoHost2Mint(errno,TOS_EFILNF);

	uint32 datetime = ReadInt32( datetimep );
	if (wflag != 0) {
		struct utimbuf tmb;

#if defined(USE_FS_EXT3)
		tmb.actime = datetime;
#else
		struct tm ttm;

		datetime2tm( datetime, &ttm );
		tmb.actime = mktime( &ttm );  /* access time */
#endif
		tmb.modtime = tmb.actime; /* modification time */

		utime( fpathName, &tmb );

#if ! defined(USE_FS_EXT3)
		D(bug("HOSTFS: /dev_datime: setting to: %d.%d.%d %d:%d.%d",
				  ttm.tm_mday,
				  ttm.tm_mon,
				  ttm.tm_year + 1900,
				  ttm.tm_sec,
				  ttm.tm_min,
				  ttm.tm_hour
				  ));
#endif
	}

#if ! defined(USE_FS_EXT3)
	datetime =
		( time2dos(statBuf.st_mtime) << 16 ) | date2dos(statBuf.st_mtime);
#endif
	WriteInt32( datetimep, datetime );

	return TOS_E_OK; //EBADRQ;
}


int32 HostFs::xfs_pathconf( XfsCookie *fc, int16 which )
{
	char fpathName[MAXPATHNAMELEN];
	cookie2Pathname(fc, NULL, fpathName);

	int	 oldErrno = errno;

	// FIXME: Has to be different for .XFS and for HOSTFS.
	D(bug("HOSTFS: fs_pathconf (%s,%d)", fpathName, which));

	STATVFS buff;
	int32 res = host_statvfs( fpathName, &buff);
	if ( res != TOS_E_OK )
		return res;

	switch (which) {
		case -1:
			return 9;  // maximal which value

		case 0:	  // DP_IOPEN
			return 0x7fffffffL; // unlimited

		case 1: { // DP_MAXLINKS
			long result = pathconf(fpathName, _PC_LINK_MAX);
			if ( result == -1 && oldErrno != errno )
				return errnoHost2Mint(errno,TOS_EFILNF);

			return result;
		}
		case 2:	  // DP_PATHMAX
			return MAXPATHNAMELEN; // FIXME: This is the limitation of this implementation (ARAnyM specific)

		case 3:	  // DP_NAMEMAX
#ifdef HAVE_SYS_STATVFS_H
			return buff.f_namemax;
#else
# if (defined(OS_openbsd) || defined(OS_freebsd) || defined(OS_netbsd) || defined(OS_darwin))
			return MFSNAMELEN;
# else
#if defined(OS_mint)
			return Dpathconf(fpathName,3 /* DP_NAMEMAX */);
#else
			return buff.f_namelen;
#endif /* OS_mint */
#endif /* OS_*bsd */
#endif /* HAVE_SYS_STATVFS_H */

		case 4:	  // DP_ATOMIC
			return buff.f_bsize;	 // ST max vs Linux optimal

		case 5:	  // DP_TRUNC
			return 0;  // files are NOT truncated... (hope correct)

		case 6:   // DP_CASE
			return ( !fc->drv || fc->drv->halfSensitive ) ? 2 /*DP_CASEINSENS*/ : 0 /*DP_CASESENS*/;

		case 7:	  // D_XATTRMODE
			return 0x0fffffdfL;	 // only the archive bit is not recognised in the fs_getxattr

		case 8:	  // DP_XATTR
			// FIXME: This argument should be set accordingly to the filesystem type mounted
			// to the particular path.
			return 0x00000ffbL;	 // rdev is not used

		case 9:	  // DP_VOLNAMEMAX
			return 0;

		default:;
	}
	return TOS_EINVFN;
}


char *HostFs::host_readlink(const char *pathname, char *target, int len )
{
	int rv;
	target[0] = '\0';
	if ((rv=readlink(pathname,target,len))<0)
		return NULL;

	// put the trailing \0
	target[rv] = '\0';

	// relative host fs symlinks
	if ( target[0] != '/' && target[0] != '\\' && target[1] != ':' ) {
		// find the last dirseparator
		const char *slash = &pathname[strlen(pathname)];
		while ( --slash >= pathname &&
				*slash != '/' &&
				*slash != '\\' ) ;

		// get the path part (in front of the slash)
		// and prepend it to the link taget (if fits)
		int plen = slash - pathname + 1;
		if ( plen && plen + (int)strlen(target) <= len ) {
			memmove( target + plen, target, strlen(target)+1 );	
			memmove( target, pathname, plen );
		} else {
			panicbug( "HOSTFS: host_readlink: relative link doesn't fit the len '%s'\n", target );
			errno = ENOMEM;
			return NULL;
		}

		// find the last dirseparator
		slash = &target[strlen(target)];
		while ( --slash >= target &&
				*slash != '/' &&
				*slash != '\\' ) ;
		slash++;

		if ( slash > target ) {
			// if relative then we need an absolute path here
			char currdir[MAXPATHNAMELEN];
			assert(getcwd(currdir, sizeof(currdir)) != NULL);

			char abspath[MAXPATHNAMELEN];
			strncpy(abspath, target, slash-target);
			abspath[slash-target] = '\0';
			assert(chdir(abspath) == 0);
			assert(getcwd(abspath, sizeof(abspath)) != NULL);

			assert(chdir(currdir) == 0);

			strncat(abspath, slash-1, MAXPATHNAMELEN-strlen(abspath));
			strncpy(target, abspath, len);
		}
	}

	return target;
}

DIR *HostFs::host_opendir( const char *fpathName ) {
	DIR *result = opendir( fpathName );
	if ( result || errno != ENOTDIR )
		return result;

	// follow symlink when needed
	struct stat statBuf;
	if ( !lstat(fpathName, &statBuf) && S_ISLNK(statBuf.st_mode) ) {
		char temp[MAXPATHNAMELEN];
		if (host_readlink(fpathName,temp,sizeof(temp)-1))
			return host_opendir( temp );
	}

	return NULL;
}

int32 HostFs::xfs_opendir( XfsDir *dirh, uint16 flags )
{
	char fpathName[MAXPATHNAMELEN];
	cookie2Pathname(&dirh->fc, NULL, fpathName);

	D(bug("HOSTFS: fs_opendir (%s,%d)", fpathName, flags));

	dirh->flags = flags;
	dirh->index = 0;

	dirh->hostDir = host_opendir( fpathName );
	if ( dirh->hostDir == NULL )
		return errnoHost2Mint(errno,TOS_EPTHNF);

	return TOS_E_OK;
}



int32 HostFs::xfs_closedir( XfsDir *dirh )
{
	if ( closedir( dirh->hostDir ) )
		return errnoHost2Mint(errno,TOS_EPTHNF);

	return TOS_E_OK;
}


int32 HostFs::xfs_readdir( XfsDir *dirh, memptr buff, int16 len, XfsCookie *fc )
{
	struct dirent *dirEntry;


	do {
		if ((void*)(dirEntry = readdir( dirh->hostDir )) == NULL)
			return TOS_ENMFIL;
	} while ( !dirh->fc.index->parent &&
			  ( dirEntry->d_name[0] == '.' &&
				( !dirEntry->d_name[1] ||
				  ( dirEntry->d_name[1] == '.' && !dirEntry->d_name[2] ) ) ) );

	XfsFsFile *newFsFile = new XfsFsFile();
	newFsFile->name = strdup( dirEntry->d_name );

	if ( dirh->flags == 0 ) {
		D(bug("HOSTFS: fs_readdir (%s, %d)", (char*)dirEntry->d_name, len ));

		if ( (uint16)len < strlen( dirEntry->d_name ) + 4 )
			return TOS_ERANGE;

		char fpathName[MAXPATHNAMELEN];
		cookie2Pathname(&dirh->fc, dirEntry->d_name, fpathName);

		struct stat statBuf;
		if ( lstat(fpathName, &statBuf) )
			return errnoHost2Mint(errno,TOS_EFILNF);

		/* Vincent identified that the above lstat is used just for finding out
		   the st_ino though the same inode number should already be in the
		   dirEntry struct. I don't know why lstat() (originally stat() but that
		   one was failing for invalid host symlinks) was used instead of
		   fetching the value from dirEntry directly so I am adding a check here
		   that will be watching whether the lstat's inode number ever differs
		   from the dirEntry one. When we are sure they are always identical
		   we can remove the whole lstat call */
		if (dirEntry->d_ino != statBuf.st_ino) {
			panicbug("HostFS: d_ino != st_ino! %d vs %d on '%s'", dirEntry->d_ino, statBuf.st_ino, fpathName);
		}
		WriteInt32( (uint32)buff, statBuf.st_ino );
		Host2AtariSafeStrncpy( buff + 4, dirEntry->d_name, len-4 );
	} else {
		char truncFileName[MAXPATHNAMELEN];
		transformFileName( truncFileName, (char*)dirEntry->d_name );

		D(bug("HOSTFS: fs_readdir (%s -> %s, %d)", (char*)dirEntry->d_name, (char*)truncFileName, len ));

		if ( (uint16)len < strlen( truncFileName ) )
			return TOS_ERANGE;

		Host2AtariSafeStrncpy( buff, truncFileName, len );
	}

	dirh->index++;
	dirh->fc.index->childCount++;

	MAPNEWVOIDP( newFsFile );
	newFsFile->parent = dirh->fc.index;
	newFsFile->refCount = 1;
	newFsFile->childCount = 0;

	fc->drv = dirh->fc.drv;
	fc->xfs = fc->drv->fsDrv;
	fc->dev = dirh->fc.dev;
	fc->aux = 0;
	fc->index = newFsFile;

	return TOS_E_OK;
}


int32 HostFs::xfs_rewinddir( XfsDir *dirh )
{
	rewinddir( dirh->hostDir );
	dirh->index = 0;
	return TOS_E_OK;
}

int32 HostFs::host_stat64( XfsCookie *fc, const char *fpathName, struct stat *statBuf ) {

	if ( lstat(fpathName, statBuf) )
		return errnoHost2Mint(errno,TOS_EFILNF);

	// symlink
	if ( S_ISLNK(statBuf->st_mode) ) {
		char target[MAXPATHNAMELEN];
		if (!host_readlink(fpathName,target,sizeof(target)-1))
			return errnoHost2Mint(errno,TOS_EFILNF);

		// doesn't point to a mapped drive
		if (!findDrive(fc, target)) {
			D(bug( "HOSTFS: host_stat64: follow symlink -> %s", target ));

			// get the information from the link target
			if ( stat(target, statBuf) ) {
				// on error just rollback to a symlink (broken one)
				lstat(fpathName, statBuf);
			}
		}
	}

	return TOS_E_OK;
}

int32 HostFs::xfs_getxattr( XfsCookie *fc, memptr xattrp )
{
	char fpathName[MAXPATHNAMELEN];
	cookie2Pathname(fc, NULL, fpathName);

	D(bug("HOSTFS: fs_getxattr (%s)", fpathName));

	// perform the link stat itself
	struct stat statBuf;
	int32 res = host_stat64(fc, fpathName, &statBuf);
	if ( res != TOS_E_OK )
		return res;

	// XATTR structure conversion (COMPEND.HYP)
	/* UWORD mode	   */  WriteInt16( xattrp	  , modeHost2Mint(statBuf.st_mode) );
	/* LONG	 index	   */  WriteInt32( xattrp +	 2, statBuf.st_ino ); // FIXME: this is Linux's one

	/* UWORD dev	   */  WriteInt16( xattrp +	 6, statBuf.st_dev ); // FIXME: this is Linux's one

	/* UWORD reserved1 */  WriteInt16( xattrp +	 8, 0 );
	/* UWORD nlink	   */  WriteInt16( xattrp + 10, statBuf.st_nlink );
	/* UWORD uid	   */  WriteInt16( xattrp + 12, statBuf.st_uid );	 // FIXME: this is Linux's one
	/* UWORD gid	   */  WriteInt16( xattrp + 14, statBuf.st_gid );	 // FIXME: this is Linux's one
	/* LONG	 size	   */  WriteInt32( xattrp + 16, statBuf.st_size );
#ifdef __MINGW32__
	/* LONG	 blksize   */  WriteInt32( xattrp + 20, 512 ); // FIXME: I just made up the number
#else
	/* LONG	 blksize   */  WriteInt32( xattrp + 20, statBuf.st_blksize );
#endif
#if defined(OS_beos) || defined (__MINGW32__)
	/* LONG	 nblocks   */  WriteInt32( xattrp + 24, 0 ); // FIXME: should be possible to find out for MINGW32
#else
	/* LONG	 nblocks   */  WriteInt32( xattrp + 24, statBuf.st_blocks );
#endif
#if defined(USE_FS_EXT3)
	/* UWORD mtime	   */  WriteInt32( xattrp + 28, statBuf.st_mtime );
	/* UWORD atime	   */  WriteInt32( xattrp + 32, statBuf.st_atime );
	/* UWORD atime	   */  WriteInt32( xattrp + 36, statBuf.st_ctime );
#else
	/* UWORD mtime	   */  WriteInt16( xattrp + 28, time2dos(statBuf.st_mtime) );
	/* UWORD mdate	   */  WriteInt16( xattrp + 30, date2dos(statBuf.st_mtime) );
	/* UWORD atime	   */  WriteInt16( xattrp + 32, time2dos(statBuf.st_atime) );
	/* UWORD adate	   */  WriteInt16( xattrp + 34, date2dos(statBuf.st_atime) );
	/* UWORD ctime	   */  WriteInt16( xattrp + 36, time2dos(statBuf.st_ctime) );
	/* UWORD cdate	   */  WriteInt16( xattrp + 38, date2dos(statBuf.st_ctime) );
#endif
	/* UWORD attr	   */  WriteInt16( xattrp + 40, modeHost2TOS(statBuf.st_mode) );
	/* UWORD reserved2 */  WriteInt16( xattrp + 42, 0 );
	/* LONG	 reserved3 */  WriteInt32( xattrp + 44, 0 );
	/* LONG	 reserved4 */  WriteInt32( xattrp + 48, 0 );

	D(bug("HOSTFS: fs_getxattr mode %#02x, mtime %#04x, mdate %#04x", modeHost2Mint(statBuf.st_mode), ReadInt16(xattrp + 28), ReadInt16(xattrp + 30)));

	return TOS_E_OK;
}

int32 HostFs::xfs_stat64( XfsCookie *fc, memptr statp )
{
	char fpathName[MAXPATHNAMELEN];
	cookie2Pathname(fc, NULL, fpathName);

	D(bug("HOSTFS: fs_stat64 (%s)", fpathName));

	// perform the link stat itself
	struct stat statBuf;
	int32 res = host_stat64(fc, fpathName, &statBuf);
	if ( res != TOS_E_OK )
		return res;

	/* LLONG hi dev	   */  WriteInt32( statp     , WHEN_INT4B( 0, ( statBuf.st_dev >> 32 ) & 0xffffffffUL ) ); // FIXME: this is Linux's one
	/* LLONG lo dev	   */  WriteInt32( statp +  4, statBuf.st_dev & 0xffffffffUL ); // FIXME: this is Linux's one
	/* ULONG    ino	   */  WriteInt32( statp +  8, statBuf.st_ino ); // FIXME: this is Linux's one
	/* ULONG    mode   */  WriteInt32( statp + 12, modeHost2Mint(statBuf.st_mode) ); // FIXME: convert???
	/* ULONG    nlink  */  WriteInt32( statp + 16, statBuf.st_nlink );
	/* ULONG    uid	   */  WriteInt32( statp + 20, statBuf.st_uid ); // FIXME: this is Linux's one
	/* ULONG    gid	   */  WriteInt32( statp + 24, statBuf.st_gid ); // FIXME: this is Linux's one
	/* LLONG hi rdev   */  WriteInt32( statp + 28, WHEN_INT4B( 0, ( statBuf.st_rdev >> 32 ) & 0xffffffffUL ) ); // FIXME: this is Linux's one
	/* LLONG lo rdev   */  WriteInt32( statp + 32,   statBuf.st_rdev & 0xffffffffUL ); // FIXME: this is Linux's one

	/* hi atime   */ WriteInt32( statp + 36, 0 );
	/* lo atime   */ WriteInt32( statp + 40,   statBuf.st_atime );
	/*    atime ns*/ WriteInt32( statp + 44, 0 );
	/* hi mtime   */ WriteInt32( statp + 48, 0 );
	/* lo mtime   */ WriteInt32( statp + 52,   statBuf.st_mtime );
	/*    mtime ns*/ WriteInt32( statp + 56, 0 );
	/* hi ctime   */ WriteInt32( statp + 60, 0 );
	/* lo ctime   */ WriteInt32( statp + 64,   statBuf.st_ctime );
	/*    ctime ns*/ WriteInt32( statp + 68, 0 );

	/* LLONG hi size   */  WriteInt32( statp + 72, ( statBuf.st_size >> 32 ) & 0xffffffffUL );
	/* LLONG lo size   */  WriteInt32( statp + 76,   statBuf.st_size & 0xffffffffUL );
#if defined(OS_beos) || defined(__MINGW32__)
	/* LLONG hi blocks */  WriteInt32( statp + 80,   0 );
	/* LLONG lo blocks */  WriteInt32( statp + 84,   0 );
	/* ULONG    blksize*/  WriteInt32( statp + 88,   0 );
#else
	/* LLONG hi blocks */  WriteInt32( statp + 80, ( statBuf.st_blocks >> 32 ) & 0xffffffffUL );
	/* LLONG lo blocks */  WriteInt32( statp + 84,   statBuf.st_blocks & 0xffffffffUL );
	/* ULONG    blksize*/  WriteInt32( statp + 88,   statBuf.st_blksize );
#endif
	/* ULONG    flags  */  WriteInt32( statp + 92,   0 );
	/* ULONG    gen    */  WriteInt32( statp + 96,   0 );

	D(bug("HOSTFS: fs_stat64 mode %#02x, mtime %#08lx", modeHost2Mint(statBuf.st_mode), ReadInt32(statp + 52)));

	return TOS_E_OK;
}


int32 HostFs::xfs_chattr( XfsCookie *fc, int16 attr )
{
	char fpathName[MAXPATHNAMELEN];
	cookie2Pathname(fc, NULL, fpathName);

	D(bug("HOSTFS: fs_chattr (%s)", fpathName));

	// perform the link stat itself
	struct stat statBuf;
    if ( lstat( fpathName, &statBuf ) )
		return errnoHost2Mint( errno, TOS_EACCDN );

    mode_t newmode;
    if ( attr & 0x01 ) /* FA_RDONLY */
		newmode = statBuf.st_mode & ~( S_IWUSR | S_IWGRP | S_IWOTH );
    else
		newmode = statBuf.st_mode | ( S_IWUSR | S_IWGRP | S_IWOTH );
    if ( newmode != statBuf.st_mode &&
		 chmod( fpathName, newmode ) )
		return errnoHost2Mint( errno, TOS_EACCDN );

	return TOS_E_OK;
}


int32 HostFs::xfs_chmod( XfsCookie *fc, uint16 mode )
{
	char fpathName[MAXPATHNAMELEN];
	cookie2Pathname(fc, NULL, fpathName);

    /* FIXME: ARAnyM has to run at root and uid and gid have to */
    /* FIXME: be same at unix and MiNT! */
    D(bug( "HOSTFS: fs_chmod (NOT TESTED)\n"
		   "  CANNOT WORK CORRECTLY UNTIL uid AND gid AT MiNT ARE SAME LIKE AT UNIX!)\n" ));

    if ( chmod( fpathName, mode ) )
		return errnoHost2Mint( errno, TOS_EACCDN );

	return TOS_E_OK;
}


int32 HostFs::xfs_root( uint16 dev, XfsCookie *fc )
{
	MountMap::iterator it = mounts.find(dev);
	if ( it == mounts.end() ) {
		D(bug( "root: dev = %#04x -> EDRIVE\n", dev ));
		return TOS_EDRIVE;
	}

	D(bug( "root:\n"
		   "  dev	 = %#04x\n"
		   "  devdrv = %#08lx\n",
		   dev, it->second->fsDrv));

	fc->drv = it->second;
	fc->xfs = fc->drv->fsDrv;
	fc->dev = dev;
	fc->aux = 0;
	fc->index = new XfsFsFile(); MAPNEWVOIDP( fc->index );

	fc->index->parent = NULL;
	fc->index->name = fc->drv->hostRoot;
	fc->index->refCount = 1;
	fc->index->childCount = 0;

	D2(bug( "root result:\n"
		   "  fs	= %08lx\n"
		   "  dev	= %04x\n"
		   "  aux	= %04x\n"
		   "  index = %08lx\n"
		   "	name = \"%s\"\n",
		   fc->xfs, fc->dev, fc->aux, fc->index, fc->index->name));

	return TOS_E_OK;
}


int32 HostFs::xfs_getdev( XfsCookie *fc, memptr devspecial )
{
	WriteInt32(devspecial,0); /* reserved */
	return (int32)fc->drv->fsDevDrv;
}


HostFs::ExtDrive *HostFs::findDrive( XfsCookie *dir, char *pathname )
{
	// absolute FreeMiNT's link stored in the HOSTFS
	if ( pathname[0] && pathname[1] == ':' )
		return NULL;

	// search among the mount points to find suitable link...
	size_t toNmLen = strlen( pathname );
	size_t hrLen = 0;

	// prefer the current device
	MountMap::iterator it = mounts.find(dir->dev);
	if ( ! ( ( hrLen = strlen( it->second->hostRoot ) ) <= toNmLen &&
			 !strncmp( it->second->hostRoot, pathname, hrLen ) ) )
	{
		// preference failed -> search all
		it = mounts.begin();
		while (it != mounts.end()) {
			hrLen = strlen( it->second->hostRoot );
			if ( hrLen <= toNmLen &&
				 !strncmp( it->second->hostRoot, pathname, hrLen ) )
				break;
			it++;
		}
	}

	if ( it != mounts.end() )
		return it->second;
	return NULL;
}


int32 HostFs::xfs_readlink( XfsCookie *dir, memptr buf, int16 len )
{
	char fpathName[MAXPATHNAMELEN];
	cookie2Pathname(dir,NULL,fpathName); // get the cookie filename

	D(bug( "HOSTFS: fs_readlink: %s", fpathName ));

	char target[MAXPATHNAMELEN];
	if (!host_readlink(fpathName,target,sizeof(target)-1))
		return errnoHost2Mint( errno, TOS_EFILNF );

	D(bug( "HOSTFS: fs_readlink: -> %s", target ));

	ExtDrive *drv = findDrive(dir, target);
	if ( drv ) {
		int len = strlen( drv->hostRoot );
		strcpy( fpathName, drv->mountPoint );
		if ( target[ len ] != '/' )
			strcat( fpathName, "/" );
		strcat( fpathName, &target[ len ] );
	} else {
		strcpy( fpathName, target );
	}

	D(bug( "HOSTFS: /fs_readlink: %s", fpathName ));

	Host2AtariSafeStrncpy( buf, fpathName, len );
	return TOS_E_OK;
}


void HostFs::xfs_freefs( XfsFsFile *fs )
{
	D2(bug( "freefs:\n"
		 "	fs = %08lx\n"
		 "	  parent   = %08lx\n"
		 "	  name	   = \"%s\"\n"
		 "	  usecnt   = %d\n"
		 "	  childcnt = %d",
		 (long)fs,
		 (long)(fs->parent),
		 fs->name,
		 fs->refCount,
		 fs->childCount ));

	if ( !fs->refCount && !fs->childCount ) {
		D2(bug( "freefs: realfree" ));
		if ( fs->parent ) {
			fs->parent->childCount--;
			xfs_freefs( fs->parent );
			free( fs->name );
		}
		MAPDELVOIDP( fs );
		delete fs;
	} else {
		D2(bug( "freefs: notfree" ));
	}
}


int32 HostFs::xfs_lookup( XfsCookie *dir, memptr name, XfsCookie *fc )
{
	char fname[MAXPATHNAMELEN];
	Atari2HostSafeStrncpy( fname, name, sizeof(fname) );

	D(bug( "HOSTFS: fs_lookup: %s", fname ));

	XfsFsFile *newFsFile;

	if ( !*fname || (*fname == '.' && !fname[1]) ) {
		newFsFile = dir->index;
		newFsFile->refCount++;
	} else if ( *fname == '.' && fname[1] == '.' && !fname[2] ) {
		if ( !dir->index->parent ) {
			D(bug( "HOSTFS: fs_lookup to \"..\" at root" ));
			return TOS_EMOUNT;
		}
		newFsFile = dir->index->parent;
		newFsFile->refCount++;
	} else {
		if ( (newFsFile = new XfsFsFile()) == NULL ) {
			D(bug( "HOSTFS: fs_lookup: malloc() failed!" ));
			return TOS_ENSMEM;
		}

		newFsFile->parent = dir->index;

		char fpathName[MAXPATHNAMELEN];
		cookie2Pathname(dir,fname,fpathName); // get the cookie filename

		D(bug( "HOSTFS: fs_lookup stat: %s", fpathName ));

		struct stat statBuf;
		if ( lstat( fpathName, &statBuf ) )
			return errnoHost2Mint( errno, TOS_EFILNF );

		MAPNEWVOIDP( newFsFile );
		newFsFile->name = strdup(fname);
        newFsFile->refCount = 1;
        newFsFile->childCount = 0;
        dir->index->childCount++; /* same as: new->parent->childcnt++ */
    }

    *fc = *dir;
    fc->index = newFsFile;

    return TOS_E_OK;
}


int32 HostFs::xfs_getname( XfsCookie *relto, XfsCookie *dir, memptr pathName, int16 size )
{
    char base[MAXPATHNAMELEN];
    cookie2Pathname(relto,NULL,base); // get the cookie filename

    char dirBuff[MAXPATHNAMELEN];
    char *dirPath = dirBuff;
    cookie2Pathname(dir,NULL,dirBuff); // get the cookie filename

    char fpathName[MAXPATHNAMELEN];

    D2(bug( "HOSTFS: fs_getname: relto = \"%s\"", base ));
    size_t baselength = strlen(base);
    if ( baselength && base[baselength-1] == DIRSEPARATOR[0] ) {
        baselength--;
        base[baselength] = '\0';
        D2(bug( "HOSTFS: fs_getname: fixed relto = \"%s\"", base ));
    }

    D2(bug( "HOSTFS: fs_getname: dir = \"%s\"", dirPath ));
    size_t dirlength = strlen(dirPath);
    if ( dirlength < baselength ||
         strncmp( dirPath, base, baselength ) ) {
        /* dir is not a sub...directory of relto, so use absolute */
        /* FIXME: try to use a relative path, if relativ is smaller */
        D2(bug( "HOSTFS: fs_getname: dir not relativ to relto!" ));
    } else {
        /* delete "same"-Part */
        dirPath += baselength;
    }
    D2(bug( "HOSTFS: fs_getname: relativ dir to relto = \"%s\"", dirPath ));

    /* copy and unix2dosname */
    char *pfpathName = fpathName;
    for ( ; *dirPath && size > 0; size--, dirPath++ )
        if ( *dirPath == '/' )
            *pfpathName++ = '\\';
        else
            *pfpathName++ = *dirPath;

    if ( !size ) {
        D2(bug( "HOSTFS: fs_getname: relative dir is too long!" ));
        return TOS_ERANGE;
    } else {
        *pfpathName = '\0';
        D(bug( "HOSTFS: fs_getname result = \"%s\"", fpathName ));

        Host2AtariSafeStrncpy( pathName, fpathName, pfpathName - fpathName + 1 );
        return TOS_E_OK;
    }
}


/*
 * from $FREEMINT_CVS/sys/mint/dcntl.h
 */

# define MINT_V_CNTR_WP	(('V'<< 8) | 100)		/* MiNT, write protect control */

/*
 * MagiC opcodes (all group 'm' opcodes are reserved for MagiC)
 */

# define MX_KER_GETINFO         (('m'<< 8) | 0)         /* mgx_dos.txt */
# define MX_KER_DOSLIMITS       (('m'<< 8) | 1)         /* mgx_dos.txt */
# define MX_KER_INSTXFS         (('m'<< 8) | 2)         /* mgx_dos.txt */
# define MX_KER_DRVSTAT         (('m'<< 8) | 4)         /* mgx_dos.txt */
# define MX_KER_XFSNAME         (('m'<< 8) | 5)         /* mgx_dos.txt */
# define MX_DEV_INSTALL         (('m'<< 8) | 0x20)      /* mgx_dos.txt */
# define MX_DFS_GETINFO         (('m'<< 8) | 0x40)      /* mgx_dos.txt */
# define MX_DFS_INSTDFS         (('m'<< 8) | 0x41)      /* mgx_dos.txt */

# define MINT_FS_INFO        0xf100
# define MINT_FS_USAGE       0xf101

# define MINT_FS_HOSTFS   (15L << 16)

/*
 * from $FREEMINT_CVS/sys/mint/ioctl.h
 */

# define MINT_FSTAT		(('F'<< 8) | 0)		/* handled by kernel */
# define MINT_FIONREAD		(('F'<< 8) | 1)
# define MINT_FIONWRITE		(('F'<< 8) | 2)
# define MINT_FUTIME		(('F'<< 8) | 3)

# define MINT_FTRUNCATE		(('F'<< 8) | 4)
# define MINT_FIOEXCEPT		(('F'<< 8) | 5)

# define MINT_FSTAT64		(('F'<< 8) | 6)		/* 1.15.4 extension, optional */
# define MINT_FUTIME_UTC	(('F'<< 8) | 7)		/* 1.15.4 extension, optional */
# define MINT_FIBMAP		(('F'<< 8) | 10)


int32 HostFs::xfs_fscntl ( XfsCookie *dir, memptr name, int16 cmd, int32 arg)
{
	switch ((uint16)cmd)
	{
		case MX_KER_XFSNAME:
		{
			D(bug( "HOSTFS: fs_fscntl: MX_KER_XFSNAME: arg = %08lx", arg ));
			Host2AtariSafeStrncpy(arg, "hostfs-xfs", 32);
			return TOS_E_OK;
		}
		case MINT_FS_INFO:
		{
			D(bug( "HOSTFS: fs_fscntl: FS_INFO: arg = %08lx", arg ));
			if (arg)
			{
				Host2AtariSafeStrncpy(arg, "hostfs-xfs", 32);
				WriteInt32(arg+32, ((int32)HOSTFS_XFS_VERSION << 16) | HOSTFS_NFAPI_VERSION );
				WriteInt32(arg+36, MINT_FS_HOSTFS );
				Host2AtariSafeStrncpy(arg+40, "host filesystem", 32);
			}
			return TOS_E_OK;
		}
		case MINT_FS_USAGE:
		{
			char fpathName[MAXPATHNAMELEN];
			cookie2Pathname( dir, NULL, fpathName );

			STATVFS buff;
			int32 res = host_statvfs( fpathName, &buff);
			if ( res != TOS_E_OK )
				return res;

			if (arg)
			{
				/* LONG  blocksize */  WriteInt32( arg     , buff.f_bsize );
				/* LLONG hi blocks */  WriteInt32( arg +  4, WHEN_INT4B( 0, ( buff.f_blocks >> 32 ) & 0xffffffffUL ) );
				/* LLONG lo blocks */  WriteInt32( arg +  8, buff.f_blocks & 0xffffffffUL );
				/* LLONG hi freebs */  WriteInt32( arg + 12, WHEN_INT4B( 0, ( buff.f_bavail >> 32 ) & 0xffffffffUL ) );
				/* LLONG lo freebs */  WriteInt32( arg + 16, buff.f_bavail & 0xffffffffUL );
				/* LLONG hi inodes */  WriteInt32( arg + 20, 0xffffffffUL);
				/* LLONG lo inodes */  WriteInt32( arg + 24, 0xffffffffUL);
				/* LLONG hi finodes*/  WriteInt32( arg + 28, 0xffffffffUL);
				/* LLONG lo finodes*/  WriteInt32( arg + 32, 0xffffffffUL);
			}
			return TOS_E_OK;
		}

		case MINT_V_CNTR_WP:
			// FIXME: TODO!
			break;

		case MINT_FUTIME:
			// We do not do any GEMDOS time setting.
			// Mintlib calls the dcntl(FUTIME_ETC, filename) first anyway.
			return TOS_ENOSYS;

		case MINT_FUTIME_UTC:
		{
			char fpathName[MAXPATHNAMELEN];
			cookie2Pathname( dir, NULL, fpathName );

			struct utimbuf t_set;
			t_set.actime  = ReadInt32( arg );
			t_set.modtime = ReadInt32( arg + 4 );
			if (utime(fpathName, &t_set))
				return errnoHost2Mint( errno, TOS_EFILNF );

			return TOS_E_OK;
		}

		case MINT_FTRUNCATE:
		{
			char fname[MAXPATHNAMELEN];
			Atari2HostSafeStrncpy( fname, name, sizeof(fname) );
			char fpathName[MAXPATHNAMELEN];
			cookie2Pathname(dir,fname,fpathName); // get the cookie filename

			D(bug( "HOSTFS: fs_fscntl: FTRUNCATE: %s, %08lx", fpathName, arg ));
			if(truncate(fpathName, arg))
				return errnoHost2Mint( errno, TOS_EFILNF );

			return TOS_E_OK;
		}
	}

	return TOS_ENOSYS;
}


int32 HostFs::xfs_dev_ioctl ( ExtFile *fp, int16 mode, memptr buff)
{
	switch ((uint16)mode)
	{
		case MINT_FIONWRITE:
			WriteInt32(buff, 1);
			return TOS_E_OK;
		case MINT_FIONREAD:
		{
			int32 pos = lseek( fp->hostFd, 0, SEEK_CUR ); // get position
			WriteInt32(buff, lseek( fp->hostFd, 0, SEEK_END ) - pos);
			lseek( fp->hostFd, pos, SEEK_SET ); // set the position back
			return TOS_E_OK;
		}
		case MINT_FIOEXCEPT:
			WriteInt32(buff, 0);
			return TOS_E_OK;

		case MINT_FUTIME:
		case MINT_FUTIME_UTC:
			// Do not provide this on filedescriptor level.
			// Mintlib calls the Dcntl(FUTIME_ETC, filename) first anyway.
			return TOS_ENOSYS;

#if FIXME
		case MINT_F_SETLK:
		case MINT_F_SETLKW:
		case MINT_F_GETLK:
			// FIXME: TODO! locking
			break;			
#endif

		case MINT_FTRUNCATE:
		{
			D(bug( "HOSTFS: fs_ioctl: FTRUNCATE( fd=%ld, %08lx )", fp->hostFd, ReadInt32(buff) ));
			if ((fp->flags & O_ACCMODE) == O_RDONLY)
				return TOS_EACCES;

			if (ftruncate( fp->hostFd, ReadInt32(buff)))
				return errnoHost2Mint( errno, TOS_EFILNF );

			return TOS_E_OK;
		}
	}

	return TOS_ENOSYS;
}


int32 HostFs::xfs_dupcookie( XfsCookie *newCook, XfsCookie *oldCook )
{
    XfsFsFile *fs = new XfsFsFile();

    if ( ( fs->parent = oldCook->index->parent ) != NULL ) {
        if ( (fs->name = strdup(oldCook->index->name)) == NULL ) {
            D(bug( "HOSTFS: fs_dupcookie: strdup() failed!" ));
            delete fs;
            return TOS_ENSMEM;
        }
        fs->parent->childCount++;
    } else
        fs->name = oldCook->drv->hostRoot;

	MAPNEWVOIDP( fs );
    fs->refCount = 1;
    fs->childCount = 0; /* don't heritate childs! */

    *newCook = *oldCook;
    newCook->index = fs;

    return TOS_E_OK;
}


int32 HostFs::xfs_release( XfsCookie *fc )
{
    D2(bug( "release():\n"
         "  fc = %08lx\n"
         "    fs    = %08lx\n"
         "    dev   = %04x\n"
         "    aux   = %04x\n"
         "    index = %08lx\n"
         "      parent   = %08lx\n"
         "      name     = \"%s\"\n"
         "      usecnt   = %d\n"
         "      childcnt = %d\n",
         (long)fc,
         (long)fc->xfs,
         (int)fc->dev,
         (int)fc->aux,
         (long)fc->index,
         (long)fc->index->parent,
           fc->index->name,
           fc->index->refCount,
           fc->index->childCount ));

    if ( !fc->index->refCount )
    {
        D(bug( "HOSTFS: fs_release: RELEASE OF UNUSED FILECOOKIE!" ));
        return TOS_EACCDN;
    }
    fc->index->refCount--;
    xfs_freefs( fc->index );

    return TOS_E_OK;
}


HostFs::ExtDrive::ExtDrive( HostFs::ExtDrive *old ) {
	if ( old ) {
		driveNumber = old->driveNumber;
		fsDrv = old->fsDrv;
		fsDevDrv = old->fsDevDrv;
		hostRoot = old->hostRoot ? strdup( old->hostRoot ) : NULL;
		mountPoint = old->hostRoot ? strdup( old->mountPoint ) : NULL;
		halfSensitive = old->halfSensitive;
	}
}


int32 HostFs::xfs_native_init( int16 devnum, memptr mountpoint, memptr hostroot,
				bool halfSensitive, memptr filesys, memptr filesys_devdrv )
{
	char fmountPoint[MAXPATHNAMELEN];
	Atari2HostSafeStrncpy( fmountPoint, mountpoint, sizeof(fmountPoint) );

	ExtDrive *drv = new ExtDrive();
	drv->fsDrv = filesys;
	drv->fsDevDrv = filesys_devdrv;
	drv->mountPoint = strdup( fmountPoint );

	int16 dnum = -1;
	if ( strlen( fmountPoint ) == 2 && fmountPoint[1] == ':' ) {
		// The mountPoint is of a "X:" format: (BetaDOS mapping)
		dnum = tolower(fmountPoint[0])-'a';
	}
	else if (strlen(fmountPoint) >= 4 && !strncasecmp(fmountPoint, "u:\\", 3)) {
		// the hostfs.xfs tries to map drives to u:\\X
		// in this case we use the [HOSTFS] of config file here
		dnum = tolower(fmountPoint[3])-'a';
	}

	int maxdnum = sizeof(bx_options.aranymfs) / sizeof(bx_options.aranymfs[0]);
	if (dnum >= 0 && dnum < maxdnum) {
		drv->hostRoot = strdup( bx_options.aranymfs[dnum].rootPath );
		drv->halfSensitive = bx_options.aranymfs[dnum].halfSensitive;
	} else {
		dnum = -1; // invalidate dnum

		// no [HOSTFS] match -> map to the passed mountpoint
		//  - future extension to map from m68k side
		char fhostroot[MAXPATHNAMELEN];
		Atari2HostSafeStrncpy( fhostroot, hostroot, sizeof(fhostroot) );

		drv->hostRoot = strdup( fhostroot );
		drv->halfSensitive = halfSensitive;
	}

	// if hostRoot is a symlink then follow it
	struct stat statBuf;
	while ( !lstat(drv->hostRoot, &statBuf) && S_ISLNK(statBuf.st_mode) ) {
		char target[MAXPATHNAMELEN];
		if (host_readlink(drv->hostRoot,target,sizeof(target)-1)) {
			free( drv->hostRoot );
			drv->hostRoot = strdup( target );
		}
	}

	// no rootPath -> do not map the drive
	if ( !strlen(drv->hostRoot) ) {
		delete drv;
		return TOS_EPTHNF;
	}

	drv->driveNumber = devnum;
	mounts.insert(std::make_pair( devnum, drv ));

	// if the drive mount was mounted to some FreeMiNT mountpoint
	// which devnum is higher that MAXDRIVES then serve also as the
	// GEMDOS drive equivalent. This is a need for the current
	// FreeMiNT kernel which requires the driver for u:\[a-z0-6] to react
	// to 0-31 devno's
	if (dnum != -1 && dnum != devnum) {
		drv = new ExtDrive( drv );
		drv->driveNumber = dnum;
		mounts.insert(std::make_pair( dnum, drv ));
	}


	bug("HOSTFS: fs_native_init:\n"
	    "\t\t fs_drv	   = %#08x\n"
		"\t\t fs_devdrv  = %#08x\n"
		  "\t\t fs_devnum  = %#04x\n"
		  "\t\t fs_mountPoint = %s\n"
		  "\t\t fs_hostRoot   = %s [%d]\n"
		  ,drv->fsDrv
		  ,drv->fsDevDrv
		  ,(int)devnum
		  ,drv->mountPoint
		  ,drv->hostRoot
		  ,strlen(drv->hostRoot)
		  );

	return TOS_E_OK;
}

void HostFs::freeMounts()
{
	for(MountMap::iterator it = mounts.begin(); it != mounts.end(); ) {
		delete it->second;
		mounts.erase(it++);
	}
}

HostFs::HostFs()
{
	mounts.clear();
}

void HostFs::reset()
{
	freeMounts();
}

HostFs::~HostFs()
{
	freeMounts();
}

#endif /* HOSTFS_SUPPORT */

/*
vim:ts=4:sw=4:
*/
