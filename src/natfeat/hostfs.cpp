/*
 * $Header$
 *
 * (c) STanda @ ARAnyM development team 2001-2003
 *
 * GPL
 */

#include "sysdeps.h"

#ifdef HOSTFS_SUPPORT

#include "cpu_emulation.h"
#include "main.h"

#include "parameters.h"
#include "toserror.h"
#include "hostfs.h"
#include "tools.h"
#include "araobjs.h"

#undef  DEBUG_FILENAMETRANSFORMATION
#define DEBUG 0
#include "debug.h"

#ifdef HAVE_SYS_VFS_H
# include <sys/vfs.h>
#endif

#ifdef HAVE_NEW_HEADERS
# include <cerrno>
# include <cstdlib>
# include <cstring>
#else
# include <errno.h>
# include <stdlib.h>
# include <stdio.h>
#endif

#ifdef OS_mint
#include <mint/osbind.h>
#include <mint/mintbind.h>
#endif /* OS_mint */

#include "../../atari/hostfs/hostfs_nfapi.h"	/* XFS_xx and DEV_xx enum */


// please remember to leave this define _after_ the reqired system headers!!!
// some systems does define this to some important value for them....
#ifndef O_BINARY
# define O_BINARY 0
#endif



#if 0
    // todo??? 1 - yes

    1 D(bug("%s", "fs_chattr"));
    1 D(bug("%s", "fs_chown"));
      D(bug("%s", "fs_chmod"));
      D(bug("%s", "fs_writelabel"));
      D(bug("%s", "fs_readlabel"));
    1 D(bug("%s", "fs_hardlink"));
    1 D(bug("%s", "fs_fscntl"));
      D(bug("%s", "fs_dskchng"));
      D(bug("%s", "fs_sync"));
      D(bug("%s", "fs_mknod"));
    1 D(bug("%s", "fs_unmount"));
    1 D(bug("%s", "fs_dev_ioctl"));
      D(bug("%s", "fs_dev_select"));
      D(bug("%s", "fs_dev_unselect"));
#endif

#if 0
  FS:
      ara_fs_root, ara_fs_lookup, ara_fs_creat, ara_fs_getdev, ara_fs_getxattr,
      ara_fs_chattr, ara_fs_chown, ara_fs_chmod, ara_fs_mkdir, ara_fs_rmdir,
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
  DEV:
      ara_fs_dev_open, ara_fs_dev_write, ara_fs_dev_read, ara_fs_dev_lseek,
      ara_fs_dev_ioctl, ara_fs_dev_datime, ara_fs_dev_close, ara_fs_dev_select,
      ara_fs_dev_unselect,
      NULL, NULL /* writeb, readb not needed */
#endif


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

		case XFS_CHATTR:
			D(bug("%s", "fs_chattr"));
			fetchXFSC( &fc, getParameter(0) );
			ret = xfs_chattr( &fc,
							  getParameter(1) /* mode */ );
			break;

		case XFS_CHOWN:
			D(bug("%s", "fs_chown"));
			D(bug("fs_chown - NOT IMPLEMENTED!"));
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
			ret = TOS_EINVFN;
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
			D(bug("fs_hardlink - NOT IMPLEMENTED!"));
			ret = TOS_E_OK;
			break;

		case XFS_FSCNTL:
			fetchXFSC( &fc, getParameter(0) );
			D(bug("fs_fscntl '%c'<<8|%d", (getParameter(2)>>8)&0xff ? (char)(getParameter(2)>>8)&0xff : 0x20, (char)(getParameter(2)&0xff)));
			D(bug("fs_fscntl - NOT IMPLEMENTED!"));
			ret = TOS_E_OK;
			break;

		case XFS_DSKCHNG:
			D(bug("%s", "fs_dskchng"));
			D(bug("fs_dskchng - NOT IMPLEMENTED!"));
			ret = TOS_E_OK;
			break;

		case XFS_RELEASE:
			D(bug("%s", "fs_release"));
			fetchXFSC( &fc, getParameter(0) );
			ret = xfs_release( &fc );
			flushXFSC( &fc, getParameter(0) );
			break;

		case XFS_DUPCOOKIE:
			D(bug("%s", "fs_dupcookie"));
			fetchXFSC( &resFc, getParameter(0) );
			fetchXFSC( &fc, getParameter(1) );
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
			//			flushXFSF( &extFile, getParameter(0) );
			break;

		case DEV_IOCTL:
			fetchXFSF( &extFile, getParameter(0) );
			D(bug("fs_dev_ioctl '%c'<<8|%d", (getParameter(1)>>8)&0xff ? (char)(getParameter(1)>>8)&0xff : 0x20, (char)(getParameter(1)&0xff)));
			D(bug("fs_dev_ioctl - NOT IMPLEMENTED!"));
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
			D(bug("fs_dev_select - NOT IMPLEMENTED!"));
			ret = TOS_E_OK;
			break;

		case DEV_UNSELECT:
			D(bug("%s", "fs_dev_unselect"));
			D(bug("fs_dev_unselect - NOT IMPLEMENTED!"));
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
	a2fmemcpy( (char*)&dirh->hostDir, dirp + 18, sizeof(dirh->hostDir) );
}
void HostFs::flushXFSD( XfsDir *dirh, memptr dirp )
{
	flushXFSC( (XfsCookie*)dirh, dirp ); // sizeof(12)
	WriteInt16( dirp + 12, dirh->index );
	WriteInt16( dirp + 14, dirh->flags );
	f2amemcpy( dirp + 18, (char*)&dirh->hostDir, sizeof(dirh->hostDir) );
}


uint32 HostFs::unix2toserrno(int unixerrno,int defaulttoserrno)
{
	int retval = defaulttoserrno;

	switch(unixerrno) {
		case EACCES:
		case EPERM:
		case ENFILE:  retval = TOS_EACCDN;break;
		case EBADF:	  retval = TOS_EIHNDL;break;
		case ENOTDIR: retval = TOS_EPTHNF;break;
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

	D(bug("HOSTFS: unix2toserrno (%d,%d->%d)", unixerrno, defaulttoserrno, retval));

	return retval;
}


uint16 HostFs::time2dos(time_t t)
{
	struct tm *x;
	x = localtime (&t);
	return ((x->tm_sec&0x3f)>>1)|((x->tm_min&0x3f)<<5)|((x->tm_hour&0x1f)<<11);
}


uint16 HostFs::date2dos(time_t t)
{
	struct tm *x;
	x = localtime (&t);
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


uint16 HostFs::statmode2attr(mode_t m)
{
	return ( S_ISDIR(m) ) ? 0x10 : 0;	/* FIXME */
	//	  if (!(da == 0 || ((da != 0) && (attribs != 8)) || ((attribs | 0x21) & da)))
}


uint16 HostFs::statmode2xattrmode(mode_t m)
{
	uint16 result = 0;

	if ( S_ISREG(m) ) result = 0x8000;
	if ( S_ISDIR(m) ) result = 0x4000;
	if ( S_ISCHR(m) ) result = 0x2000;
	if ( S_ISFIFO(m)) result = 0xa000;
	if ( S_ISBLK(m) ) result = 0xc000;	// S_IMEM in COMPEND.HYP ... FIXME!!
	if ( S_ISLNK(m) ) result = 0xe000;

	// permissions
	result |= (int16)(m & 0x0007); // other permissions
	if ( m & S_IXGRP ) result |= 0x0008;
	if ( m & S_IWGRP ) result |= 0x0010;
	if ( m & S_IRGRP ) result |= 0x0020;
	if ( m & S_IXUSR ) result |= 0x0040;
	if ( m & S_IWUSR ) result |= 0x0080;
	if ( m & S_IRUSR ) result |= 0x0100;
	if ( m & S_ISVTX ) result |= 0x0200;
	if ( m & S_ISGID ) result |= 0x0400;
	if ( m & S_ISUID ) result |= 0x0800;

	D2(bug("					(statmode: %04x)", result));

	return result;
}


int HostFs::st2flags(uint16 flags)
{
	int res = O_RDONLY;

	/* exclusive */
	if (!(flags & 0x3))
		res = O_RDONLY;
	if (flags & 0x1)
		res = O_WRONLY;
	if (flags & 0x2)
		res = O_RDWR;

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


int16 HostFs::flags2st(int flags)
{
	int16 res = 0; /* default read only */

	/* exclusive */
	if (!(flags & O_WRONLY|O_RDWR))
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
#ifdef DEBUG_FILENAMETRANSFORMATION
	D(bug("HOSTFS: transformFileName(\"%s\")...", source));
#endif

	// . and .. system folders
	if ( strcmp(source, ".") == 0 || strcmp(source, "..") == 0) {
		strcpy(dest, source);	// copy to final 8+3 buffer
		return;
	}

	// Get file name (strip off file extension)
	char *dot = strrchr( source, '.' );
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

#ifdef DEBUG_FILENAMETRANSFORMATION
	D(bug("HOSTFS: transformFileName:... nameLen = %i, extLen = %i", nameLen, extLen));
#endif

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

#ifdef DEBUG_FILENAMETRANSFORMATION
	D(bug("HOSTFS: /transformFileName(\"%s\") -> \"%s\"", source, dest));
#endif
}


bool HostFs::getHostFileName( char* result, ExtDrive* drv, char* pathName, const char* name )
{
	struct stat statBuf;

#ifdef DEBUG_FILENAMETRANSFORMATION
	D(bug("HOSTFS: getHostFileName (%s,%s)", pathName, name));
#endif

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

#ifdef DEBUG_FILENAMETRANSFORMATION
			D(bug(" (stat failed)"));
#endif

			// shorten the name from the pathName;
			*result = '\0';

			DIR *dh = opendir( pathName );
			if ( dh == NULL ) {
#ifdef DEBUG_FILENAMETRANSFORMATION
				D(bug("HOSTFS: getHostFileName dopendir(%s) failed.", pathName));
#endif
				goto lbl_final;	 // should never happen
			}

			while ( true ) {
				if ((dirEntry = readdir( dh )) == NULL) {
#ifdef DEBUG_FILENAMETRANSFORMATION
					D(bug("HOSTFS: getHostFileName dreaddir: no more files."));
#endif
					nonexisting = true;
					goto lbl_final;
				}

				if ( !drv || drv->halfSensitive )
					if ( ! strcasecmp( name, dirEntry->d_name ) ) {
						finalName = dirEntry->d_name;
#ifdef DEBUG_FILENAMETRANSFORMATION
						D(bug("HOSTFS: getHostFileName found final file."));
#endif
						goto lbl_final;
					}

				transformFileName( testName, dirEntry->d_name );

#ifdef DEBUG_FILENAMETRANSFORMATION
				D(bug("HOSTFS: getHostFileName (%s,%s,%s)", name, testName, dirEntry->d_name));
#endif

				if ( ! strcmp( testName, name ) ) {
					// FIXME isFile test (maybe?)
					// this follows one more argument to be passed (from the convertPathA2F)

					finalName = dirEntry->d_name;
					goto lbl_final;
				}
			}

		  lbl_final:
#ifdef DEBUG_FILENAMETRANSFORMATION
			D(bug("HOSTFS: getHostFileName final (%s,%s)", name, finalName));
#endif

			strcpy( result, finalName );

			// in case of halfsensitive filesystem,
			// an upper case filename should be lowecase?
			if ( nonexisting && !drv || drv->halfSensitive ) {
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
		D(bug(" (stat OK)"));
#endif

	return true;
}


// FIXME: remove the convertPathA2F() the check should be done some other way.
void HostFs::convertPathA2F( ExtDrive *drv, char* fpathName, char* pathName, const char* basePath )
{
	char *n, *tp, *ffileName;
	struct stat statBuf;

	// the default is the drive root dir
	if ( basePath == NULL )
		basePath = drv ? drv->hostRoot : "";

	strcpy( fpathName, basePath );
	ffileName = fpathName + strlen( fpathName );

	if (pathName[1]==':') {
		n = pathName+2;
	} else
		n = pathName;

	strd2upath( ffileName, n );
	if ( ! stat(fpathName, &statBuf) )
		return; // the path is ok except the backslashes

	// construct the pathName to get stat() from
	while ( (tp = strpbrk( n, "\\/" )) != NULL ) {
		char sep = *tp;
		*tp = '\0';

		getHostFileName( ffileName, drv, fpathName, n );
		ffileName += strlen( ffileName );
		*ffileName++ = DIRSEPARATOR[0];

		*tp = sep;
		n = tp+1;
	}

	getHostFileName( ffileName, drv, fpathName, n );
}


/*
 * build a complete linux filepath
 * --> fs	 something like a handle to a known file
 *	   name	 a postfix to the path of fs or NULL, if no postfix
 *	   buf	 buffer for the complete filepath or NULL if static one
 *			 should be used.
 * <-- complete filepath or NULL if error
 */
char *HostFs::cookie2Pathname( HostFs::XfsFsFile *fs, const char *name, char *buf )
{
	static char sbuf[MAXPATHNAMELEN]; /* FIXME: size should told by unix */

	if (!buf)
		buf = sbuf;
	if (!fs)
	{
		D2(bug("HOSTFS: cookie2pathname root? '%s'", name));
		/* we are at root */
		if (!name)
			return NULL;
		else
			strcpy(buf, name);
	}
	else
	{
		if (!cookie2Pathname(fs->parent, fs->name, buf))
			return NULL;
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
			getHostFileName( buf + strlen(buf), NULL, buf, name );
		}
	}
	D2(bug("HOSTFS: cookie2pathname '%s'", buf));
	return buf;
}



int32 HostFs::xfs_dfree( XfsCookie *dir, uint32 diskinfop )
{
	char fpathName[MAXPATHNAMELEN];
	cookie2Pathname( dir->index, NULL, fpathName );

	D(bug("HOSTFS: fs_dfree (%s)", fpathName));

#ifdef HAVE_SYS_STATVFS_H
# ifdef OS_solaris
	statvfs_t buff;
# else
	struct statvfs buff;
# endif
#else
	struct statfs buff;
#endif

#ifdef HAVE_SYS_STATVFS_H
	if ( statvfs(fpathName, &buff) )
#else
	if ( statfs(fpathName, &buff) )
#endif
		return unix2toserrno(errno,TOS_EFILNF);

	/* ULONG b_free	   */  WriteInt32( diskinfop	   , buff.f_bavail );
	/* ULONG b_total   */  WriteInt32( diskinfop +	4, buff.f_blocks );
	/* ULONG b_secsize */  WriteInt32( diskinfop +	8, buff.f_bsize /* not 512 according to stonx_fs */ );
	/* ULONG b_clsize  */  WriteInt32( diskinfop + 12, 1 );

	return TOS_E_OK;
}


int32 HostFs::xfs_mkdir( XfsCookie *dir, memptr name, uint16 mode )
{
	char fname[2048];
	a2fstrcpy( fname, name );

	char fpathName[MAXPATHNAMELEN];
	cookie2Pathname( dir->index, fname, fpathName );

	if ( mkdir( (char*)fpathName, mode ) )
		return unix2toserrno(errno,TOS_EFILNF);

	return TOS_E_OK;
}


int32 HostFs::xfs_rmdir( XfsCookie *dir, memptr name )
{
	char fname[2048];
	a2fstrcpy( fname, name );

	char pathName[MAXPATHNAMELEN];
	cookie2Pathname( dir->index, fname, pathName );

	// FIXME: remove the convertPathA2F() the check should be done some other way.
	char fpathName[MAXPATHNAMELEN];
	convertPathA2F( mounts.find(dir->dev)->second, fpathName, pathName, "" ); // convert the fname into the hostfs form

	if ( rmdir( fpathName ) )
		return unix2toserrno(errno,TOS_EFILNF);

	return TOS_E_OK;
}


int32 HostFs::xfs_creat( XfsCookie *dir, memptr name, uint16 mode, int16 flags, XfsCookie *fc )
{
	char fname[2048];
	a2fstrcpy( fname, name );

	char pathName[MAXPATHNAMELEN];
	cookie2Pathname(dir->index,fname,pathName); // get the cookie filename

	// FIXME: remove the convertPathA2F() the check should be done some other way.
	char fpathName[MAXPATHNAMELEN];
	convertPathA2F( mounts.find(dir->dev)->second, fpathName, pathName, "" ); // convert the fname into the hostfs form (check the 8+3 file existence)

	D(bug("HOSTFS:  dev_creat (%s,%x)", fpathName, flags));

	int fd = open( fpathName, O_CREAT|O_WRONLY|O_TRUNC|O_BINARY, mode );
	if (fd < 0)
		return unix2toserrno(errno,TOS_EFILNF);
	close( fd );

	XfsFsFile *newFsFile = new XfsFsFile();	MAPNEWVOIDP( newFsFile );
	newFsFile->name = strdup( fname );
	newFsFile->refCount = 1;
	newFsFile->childCount = 0;
	newFsFile->parent = dir->index;
	dir->index->childCount++;

	*fc = *dir;
	fc->index = newFsFile;

	D(bug("HOSTFS: /dev_creat (%s,%d)", fpathName, flags));
	return TOS_E_OK;
}


void HostFs::xfs_debugCookie( XfsCookie *fc )
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


int32 HostFs::xfs_dev_open(ExtFile *fp)
{
	char fpathName[MAXPATHNAMELEN];
	cookie2Pathname(fp->fc.index, NULL, fpathName);

	D(bug("HOSTFS:  dev_open (%s, %d)", fpathName, fp->flags));

	int fd = open( fpathName, st2flags(fp->flags)|O_BINARY, 0 );
	if (fd < 0)
		return unix2toserrno(errno,TOS_EFILNF);
	fp->hostFd = fd;

    #if SIZEOF_INT != 4 || DEBUG_NON32BIT
		fdMapper.putNative( fp->hostFd );
    #endif

	D(bug("HOSTFS: /dev_open (fd = %ld)", fp->hostFd));
	return TOS_E_OK;

}


int32 HostFs::xfs_dev_close(ExtFile *fp, int16 pid)
{
	D(bug("HOSTFS:  dev_close (fd = %d, links = %d, pid = %d)", fp->hostFd, fp->links, pid));

	if ( fp->links <= 0 ) {
		if ( close( fp->hostFd ) )
			return unix2toserrno(errno,TOS_EIO);

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

		f2amemcpy( destBuff, (char*)fBuff, readCount );
		destBuff += readCount;
		toRead -= readCount;
	}

	D(bug("HOSTFS: /dev_read readCount (%d)", count - toRead));
	if ( readCount < 0 )
		return unix2toserrno(errno,TOS_EINTRN);

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
		a2fmemcpy( (char*)fBuff, sourceBuff, toWriteNow );
		writeCount = write( fp->hostFd, fBuff, toWriteNow );
		if ( writeCount <= 0 )
			break;

		sourceBuff += writeCount;
		toWrite -= writeCount;
	}

	D(bug("HOSTFS: /dev_write writeCount (%d)", count - toWrite));
	if ( writeCount < 0 )
		return unix2toserrno(errno,TOS_EINTRN);

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
		return unix2toserrno(errno,TOS_EIO);

	return newoff;
}


int32 HostFs::xfs_remove( XfsCookie *dir, memptr name )
{
	char fname[2048];
	a2fstrcpy( fname, name );

	char pathName[MAXPATHNAMELEN];
	cookie2Pathname(dir->index,fname,pathName); // get the cookie filename

	// FIXME: remove the convertPathA2F() the check should be done some other way.
	char fpathName[MAXPATHNAMELEN];
	convertPathA2F( mounts.find(dir->dev)->second, fpathName, pathName, "" ); // convert the fname into the hostfs form (check the 8+3 file existence)

	if ( unlink( fpathName ) )
		return unix2toserrno(errno,TOS_EFILNF);

	return TOS_E_OK;
}


int32 HostFs::xfs_rename( XfsCookie *olddir, memptr oldname, XfsCookie *newdir, memptr newname )
{
	char foldname[2048];
	char fnewname[2048];
	a2fstrcpy( foldname, oldname );
	a2fstrcpy( fnewname, newname );

	char fpathName[MAXPATHNAMELEN];
	char fnewPathName[MAXPATHNAMELEN];
	cookie2Pathname( olddir->index, foldname, fpathName );
	cookie2Pathname( newdir->index, fnewname, fnewPathName );

	if ( rename( fpathName, fnewPathName ) )
		return unix2toserrno(errno,TOS_EFILNF);

	return TOS_E_OK;
}


int32 HostFs::xfs_symlink( XfsCookie *dir, memptr fromname, memptr toname )
{
	char ffromname[2048];
	char ftoname[2048];
	a2fstrcpy( ffromname, fromname );
	a2fstrcpy( ftoname, toname );

	char ffromName[MAXPATHNAMELEN];
	cookie2Pathname( dir->index, ffromname, ffromName );

	// search among the mount points to find suitable link...
	size_t toNmLen = strlen( ftoname );
	size_t mpLen = 0;

	// prefer the current device
	MountMap::iterator it = mounts.find(dir->dev);
	if ( ! ( ( mpLen = strlen( it->second->mountPoint ) ) < toNmLen &&
			 !strncmp( it->second->mountPoint, ftoname, mpLen ) ) )
	{
		// preference failed -> search all
		it = mounts.begin();
		while (it != mounts.end()) {
			mpLen = strlen( it->second->mountPoint );
			if ( mpLen < toNmLen &&
				 !strncmp( it->second->mountPoint, ftoname, mpLen ) )
				break;
			it++;
		}
	}

	char ftoName[MAXPATHNAMELEN];
	if ( it != mounts.end() ) {
		strcpy( ftoName, it->second->hostRoot );
		strcat( ftoName, &ftoname[ mpLen ] );
	} else {
		strcpy( ftoName, ftoname );
	}

	D(bug( "HOSTFS: fs_symlink: \"%s\" --> \"%s\"", ffromName, ftoName ));

	if ( symlink( ftoName, ffromName ) )
		return unix2toserrno(errno,TOS_EFILNF);

	return TOS_E_OK;
}


int32 HostFs::xfs_dev_datime( ExtFile *fp, memptr datetimep, int16 wflag)
{
	char fpathName[MAXPATHNAMELEN];
	cookie2Pathname( fp->fc.index, NULL, fpathName );

	D(bug("HOSTFS:  dev_datime (%s)", fpathName));
	struct stat statBuf;

	if ( stat(fpathName, &statBuf) )
		return unix2toserrno(errno,TOS_EFILNF);

	uint32 datetime = ReadInt32( datetimep );
	if (wflag != 0) {
		struct tm ttm;
		struct utimbuf tmb;

		datetime2tm( datetime, &ttm );
		tmb.actime = mktime( &ttm );  /* access time */
		tmb.modtime = tmb.actime; /* modification time */

		utime( fpathName, &tmb );

		D(bug("HOSTFS: /dev_datime: setting to: %d.%d.%d %d:%d.%d",
				  ttm.tm_mday,
				  ttm.tm_mon,
				  ttm.tm_year + 1900,
				  ttm.tm_sec,
				  ttm.tm_min,
				  ttm.tm_hour
				  ));
	}

	datetime =
		( time2dos(statBuf.st_mtime) << 16 ) | date2dos(statBuf.st_mtime);
	WriteInt32( datetimep, datetime );

	return TOS_E_OK; //EBADRQ;
}


int32 HostFs::xfs_pathconf( XfsCookie *fc, int16 which )
{
	char fpathName[MAXPATHNAMELEN];
	cookie2Pathname(fc->index, NULL, fpathName);

	int	 oldErrno = errno;
#ifdef HAVE_SYS_STATVFS_H
# ifdef OS_solaris
	statvfs_t buf;
# else
	struct statvfs buf;
# endif
#else
	struct statfs buf;
#endif

	// FIXME: Has to be different for .XFS and for HOSTFS.
	D(bug("HOSTFS: fs_pathconf (%s,%d)", fpathName, which));

#ifdef HAVE_SYS_STATVFS_H
	if ( statvfs(fpathName, &buf) )
#else
	if ( statfs(fpathName, &buf) )
#endif
		return unix2toserrno(errno,TOS_EFILNF);

	switch (which) {
		case -1:
			return 9;  // maximal which value

		case 0:	  // DP_IOPEN
			return 0x7fffffffL; // unlimited

		case 1: { // DP_MAXLINKS
			long result = pathconf(fpathName, _PC_LINK_MAX);
			if ( result == -1 && oldErrno != errno )
				return unix2toserrno(errno,TOS_EFILNF);

			return result;
		}
		case 2:	  // DP_PATHMAX
			return MAXPATHNAMELEN; // FIXME: This is the limitation of this implementation (ARAnyM specific)

		case 3:	  // DP_NAMEMAX
#ifdef HAVE_SYS_STATVFS_H
			return buf.f_namemax;
#else
# if (defined(OS_openbsd) || defined(OS_freebsd) || defined(OS_netbsd) || defined(OS_darwin))
			return MFSNAMELEN;
# else
#if defined(OS_mint)
			return Dpathconf(fpathName,3 /* DP_NAMEMAX */);
#else
			return buf.f_namelen;
#endif /* OS_mint */
#endif /* OS_*bsd */
#endif /* HAVE_SYS_STATVFS_H */

		case 4:	  // DP_ATOMIC
			return buf.f_bsize;	 // ST max vs Linux optimal

		case 5:	  // DP_TRUNC
			return 0;  // files are NOT truncated... (hope correct)

		case 6: { // DP_CASE
#if FIXME
			if ( drv )
				return drv->halfSensitive ? 2 : 0; // full case sensitive
			else
#endif
				return 0;
		}
		case 7:	  // D_XATTRMODE
			return 0x0fffffdfL;	 // only the archive bit is not recognised in the fs_getxattr

#if 1 // supported now ;)
		case 8:	  // DP_XATTR
			// FIXME: This argument should be set accordingly to the filesystem type mounted
			// to the particular path.
			return 0x00000ffbL;	 // rdev is not used

		case 9:	  // DP_VOLNAMEMAX
			return 0;
#endif
		default:
			return TOS_EINVFN;
	}
	return TOS_EINVFN;
}


int32 HostFs::xfs_opendir( XfsDir *dirh, uint16 flags )
{
	char fpathName[MAXPATHNAMELEN];
	cookie2Pathname(dirh->fc.index, NULL, fpathName);

	D(bug("HOSTFS: fs_opendir (%s,%d)", fpathName, flags));

	dirh->flags = flags;
	dirh->index = 0;

	dirh->hostDir = opendir( fpathName );
	if ( dirh->hostDir == NULL )
		return unix2toserrno(errno,TOS_EPTHNF);

	return TOS_E_OK;
}



int32 HostFs::xfs_closedir( XfsDir *dirh )
{
	if ( closedir( dirh->hostDir ) )
		return unix2toserrno(errno,TOS_EPTHNF);

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

		WriteInt32( (uint32)buff, dirEntry->d_ino );
		f2astrcpy( buff + 4, dirEntry->d_name );
	} else {
		char truncFileName[MAXPATHNAMELEN];
		transformFileName( truncFileName, (char*)dirEntry->d_name );

		D(bug("HOSTFS: fs_readdir (%s -> %s, %d)", (char*)dirEntry->d_name, (char*)truncFileName, len ));

		if ( (uint16)len < strlen( truncFileName ) )
			return TOS_ERANGE;

		f2astrcpy( buff, truncFileName );
	}

	dirh->index++;
	dirh->fc.index->childCount++;

	MAPNEWVOIDP( newFsFile );
	newFsFile->parent = dirh->fc.index;
	newFsFile->refCount = 1;
	newFsFile->childCount = 0;

	fc->xfs = mounts.find(dirh->fc.dev)->second->fsDrv;
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


int32 HostFs::xfs_getxattr( XfsCookie *fc, uint32 xattrp )
{
	char fpathName[MAXPATHNAMELEN];
	cookie2Pathname(fc->index, NULL, fpathName);

	D(bug("HOSTFS: fs_getxattr (%s)", fpathName));

	// perform the link stat itself
	struct stat statBuf;
	if ( lstat(fpathName, &statBuf) )
		return unix2toserrno(errno,TOS_EFILNF);

	// XATTR structure conversion (COMPEND.HYP)
	/* UWORD mode	   */  WriteInt16( xattrp	  , statmode2xattrmode(statBuf.st_mode));
	/* LONG	 index	   */  WriteInt32( xattrp +	 2, statBuf.st_ino );
	/* UWORD dev	   */  WriteInt16( xattrp +	 6, statBuf.st_dev );	 // FIXME: this is Linux's one
	/* UWORD reserved1 */  WriteInt16( xattrp +	 8, 0 );
	/* UWORD nlink	   */  WriteInt16( xattrp + 10, statBuf.st_nlink );
	/* UWORD uid	   */  WriteInt16( xattrp + 12, statBuf.st_uid );	 // FIXME: this is Linux's one
	/* UWORD gid	   */  WriteInt16( xattrp + 14, statBuf.st_gid );	 // FIXME: this is Linux's one
	/* LONG	 size	   */  WriteInt32( xattrp + 16, statBuf.st_size );
	/* LONG	 blksize   */  WriteInt32( xattrp + 20, statBuf.st_blksize );
#if defined(OS_beos)
	/* LONG	 nblocks   */  WriteInt32( xattrp + 24, 0 );
#else
	/* LONG	 nblocks   */  WriteInt32( xattrp + 24, statBuf.st_blocks );
#endif
	/* UWORD mtime	   */  WriteInt16( xattrp + 28, time2dos(statBuf.st_mtime) );
	/* UWORD mdate	   */  WriteInt16( xattrp + 30, date2dos(statBuf.st_mtime) );
	/* UWORD atime	   */  WriteInt16( xattrp + 32, time2dos(statBuf.st_atime) );
	/* UWORD adate	   */  WriteInt16( xattrp + 34, date2dos(statBuf.st_atime) );
	/* UWORD ctime	   */  WriteInt16( xattrp + 36, time2dos(statBuf.st_ctime) );
	/* UWORD cdate	   */  WriteInt16( xattrp + 38, date2dos(statBuf.st_ctime) );
	/* UWORD attr	   */  WriteInt16( xattrp + 40, statmode2attr(statBuf.st_mode) );
	/* UWORD reserved2 */  WriteInt16( xattrp + 42, 0 );
	/* LONG	 reserved3 */  WriteInt32( xattrp + 44, 0 );
	/* LONG	 reserved4 */  WriteInt32( xattrp + 48, 0 );

	D(bug("HOSTFS: fs_getxattr mtime %#04x, mdate %#04x", ReadInt16(xattrp + 28), ReadInt16(xattrp + 30)));

	return TOS_E_OK;
}


int32 HostFs::xfs_chattr( XfsCookie *fc, int16 attr )
{
	char fpathName[MAXPATHNAMELEN];
	cookie2Pathname(fc->index, NULL, fpathName);

	D(bug("HOSTFS: fs_chattr (%s)", fpathName));

	// perform the link stat itself
	struct stat statBuf;
    mode_t newmode;
    if ( lstat( fpathName, &statBuf ) )
		return unix2toserrno( errno, TOS_EACCDN );

    if ( attr & 0x01 ) /* FA_RDONLY */
		newmode = statBuf.st_mode & ~( S_IWUSR | S_IWGRP | S_IWOTH );
    else
		newmode = statBuf.st_mode | ( S_IWUSR | S_IWGRP | S_IWOTH );
    if ( newmode != statBuf.st_mode &&
		 chmod( fpathName, newmode ) )
		return unix2toserrno( errno, TOS_EACCDN );

	return TOS_E_OK;
}


int32 HostFs::xfs_chmod( XfsCookie *fc, uint16 mode )
{
	char fpathName[MAXPATHNAMELEN];
	cookie2Pathname(fc->index, NULL, fpathName);

    /* FIXME: ARAnyM has to run at root and uid and gid have to */
    /* FIXME: be same at unix and MiNT! */
    D(bug( "HOSTFS: fs_chmod (NOT TESTED)\n"
		   "  CANNOT WORK CORRECTLY UNTIL uid AND gid AT MiNT ARE SAME LIKE AT UNIX!)\n" ));

    if ( chmod( fpathName, mode ) )
		return unix2toserrno( errno, TOS_EACCDN );

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

	fc->xfs = it->second->fsDrv;
	fc->dev = dev;
	fc->aux = 0;
	fc->index = new XfsFsFile(); MAPNEWVOIDP( fc->index );

	fc->index->parent = NULL;
	fc->index->name = it->second->hostRoot;
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
	return (int32)mounts.find(fc->dev)->second->fsDevDrv;
}


int32 HostFs::xfs_readlink( XfsCookie *dir, memptr buf, int16 len )
{
	char fpathName[MAXPATHNAMELEN];
	cookie2Pathname(dir->index,NULL,fpathName); // get the cookie filename

	char fbuf[MAXPATHNAMELEN];
	int rv;
	if ((rv=readlink(fpathName,fbuf,len-1)) < 0)
		return unix2toserrno( errno, TOS_EFILNF );

	// put the trailing \0
	fbuf[rv] = '\0';

	// FIXME: what about relative host fs symlinks???
	//        this works for absolute ones

	// search among the mount points to find suitable link...
	size_t toNmLen = strlen( fbuf );
	size_t hrLen = 0;
	// prefer the current device
	MountMap::iterator it = mounts.find(dir->dev);
	if ( ! ( ( hrLen = strlen( it->second->hostRoot ) ) < toNmLen &&
			 !strncmp( it->second->hostRoot, fbuf, hrLen ) ) )
	{
		// preference failed -> search all
		it = mounts.begin();
		while (it != mounts.end()) {
			hrLen = strlen( it->second->hostRoot );
			if ( hrLen < toNmLen &&
				 !strncmp( it->second->hostRoot, fbuf, hrLen ) )
				break;
			it++;
		}
	}

	if ( it != mounts.end() ) {
		strcpy( fpathName, it->second->mountPoint );
		strcat( fpathName, &fbuf[ hrLen ] );
	} else {
		strcpy( fpathName, fbuf );
	}

	f2astrcpy( buf, fpathName );
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
	char fname[2048];
	a2fstrcpy( fname, name );

	D(bug( "HOSTFS: fs_lookup: %s", fname ));

	XfsFsFile *newFsFile;

	if ( !fname || !*fname || (*fname == '.' && !fname[1]) ) {
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

		char pathName[MAXPATHNAMELEN];
		cookie2Pathname(dir->index,fname,pathName); // get the cookie filename

		// FIXME: remove the convertPathA2F() the check should be done some other way.
		char fpathName[MAXPATHNAMELEN];
		convertPathA2F( mounts.find(dir->dev)->second, fpathName, pathName, "" ); // convert the fname into the hostfs form

		struct stat statBuf;

		D(bug( "HOSTFS: fs_lookup stat: %s", fpathName ));

		if ( lstat( fpathName, &statBuf ) )
			return unix2toserrno( errno, TOS_EFILNF );

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
    cookie2Pathname(relto->index,NULL,base); // get the cookie filename

    char dirBuff[MAXPATHNAMELEN];
    char *dirPath = dirBuff;
    cookie2Pathname(dir->index,NULL,dirBuff); // get the cookie filename

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

        f2astrcpy( pathName, fpathName );
        return TOS_E_OK;
    }
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
        fs->name = mounts.find(oldCook->dev)->second->hostRoot;

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


int32 HostFs::xfs_native_init( int16 devnum, memptr mountpoint, memptr hostroot, bool halfSensitive,
							   memptr filesys, memptr filesys_devdrv )
{
	char fmountpoint[2048];
	a2fstrcpy( fmountpoint, mountpoint );

	ExtDrive *drv = new ExtDrive();
	drv->fsDrv = filesys;
	drv->fsDevDrv = filesys_devdrv;

	// The mountPoint is of a "A:" format:
	//    MetaDOS mapping the devnum is <MAXDRIVES
	if ( strlen( fmountpoint ) == 2 ) {
		int dnum = fmountpoint[0]-'A';

		// -> use the [aranymfs] of config file here
		drv->mountPoint = strdup( fmountpoint );
		drv->hostRoot = strdup( bx_options.aranymfs[dnum].rootPath );
		drv->halfSensitive = bx_options.aranymfs[dnum].halfSensitive;
	} else {
		drv->mountPoint = strdup( fmountpoint );

		// the aranym.xfs tries to map drives to u:\\xx
		// in this case we use the [aranymfs] of config file here
		if ( !strncasecmp( fmountpoint, "u:\\", 3 ) &&
			 (unsigned int)(fmountpoint[3]-'a') < sizeof(bx_options.aranymfs)/sizeof(bx_options.aranymfs[0]) )
		{
			drv->hostRoot = strdup( bx_options.aranymfs[fmountpoint[3]-'a'].rootPath );
			drv->halfSensitive = bx_options.aranymfs[fmountpoint[3]-'a'].halfSensitive;
		} else {
			// no [aranymfs] match -> map to the passed mountpoint (future extension to map from m68k side)
			char fhostroot[2048];
			a2fstrcpy( fhostroot, hostroot );

			drv->hostRoot = strdup( fhostroot );
			drv->halfSensitive = halfSensitive;
		}
	}

	drv->driveNumber = devnum;
	mounts.insert(std::make_pair( devnum, drv ));

	D(bug("HOSTFS: fs_native_init:\n"
		  "\t\t fs_drv	   = %#08x\n"
		  "\t\t fs_devdrv  = %#08x\n"
		  "\t\t fs_devnum  = %#04x\n"
		  "\t\t fs_mountPoint = %s\n"
		  "\t\t fs_hostRoot   = %s\n"
		  ,drv->fsDrv
		  ,drv->fsDevDrv
		  ,(int)devnum
		  ,drv->mountPoint
		  ,drv->hostRoot
		  ));

	return TOS_E_OK;
}


#endif /* HOSTFS_SUPPORT */

/*
 * $Log$
 * Revision 1.11.2.6  2003/04/03 12:11:29  standa
 * 32bit <-> host mapping + general hostfs cleanup.
 *
 * Revision 1.11.2.5  2003/03/28 13:19:58  joy
 * little compile fixes
 *
 * Revision 1.11.2.4  2003/03/28 12:59:07  joy
 * little fix 2nd
 *
 * Revision 1.11.2.3  2003/03/28 12:57:38  joy
 * little fix
 *
 * Revision 1.11.2.2  2003/03/28 11:45:30  joy
 * DIRSEP in hostfs
 *
 * Revision 1.11.2.1  2003/03/26 18:18:15  milan
 * stolen from head
 *
 * Revision 1.12  2003/03/24 19:11:00  milan
 * Solaris support updated
 *
 * Revision 1.11  2003/03/20 21:27:22  standa
 * The .xfs mapping to the U:\G mountpouints (single letter) implemented.
 *
 * Revision 1.10  2003/03/20 01:08:17  standa
 * HOSTFS mapping update.
 *
 * Revision 1.9  2003/03/17 09:42:39  standa
 * The chattr,chmod implementation ported from stonx.
 *
 * Revision 1.8  2003/03/12 21:10:49  standa
 * Several methods changed from EINVFN -> E_OK (only fake /empty/ implementation)
 *
 * Revision 1.7  2003/03/11 18:53:29  standa
 * Ethernet initialization fixed to work also in nondebug mode.
 *
 * Revision 1.6  2003/03/08 08:51:51  joy
 * disable DEBUG
 *
 * Revision 1.5  2003/03/01 11:57:37  joy
 * major HOSTFS NF API cleanup
 *
 * Revision 1.4  2003/02/17 14:20:20  standa
 * #if defined(OS_beos) used.
 *
 * Revision 1.3  2003/02/17 14:16:16  standa
 * BeOS patch for aranymfs and hostfs
 *
 * Revision 1.2  2002/12/16 16:10:24  standa
 * just another std:: added.
 *
 * Revision 1.1  2002/12/10 20:47:21  standa
 * The HostFS (the host OS filesystem access via NatFeats) implementation.
 *
 *
 */
