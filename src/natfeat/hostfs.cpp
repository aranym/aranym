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

#ifdef HOSTFS_SUPPORT

#include "cpu_emulation.h"
#include "main.h"

#include "parameters.h"
#include "host_filesys.h"
#include "toserror.h"
#include "hostfs.h"
#include "tools.h"
#include "win32_supp.h"

#define DEBUG 0
#include "debug.h"

#if 0
# define DFNAME(x) x
#else
# define DFNAME(x)
#endif

#ifdef HAVE_SYS_VFS_H
# include <sys/vfs.h>
#endif
#ifdef HAVE_EXT2FS_EXT2_FS_H
# include <ext2fs/ext2_fs.h>
#else
#if defined(__linux__)
#define EXT2_IOC_GETFLAGS		_IOR('f', 1, long)
#define EXT2_IOC_SETFLAGS		_IOW('f', 2, long)
#define EXT2_IOC_GETVERSION		_IOR('v', 1, long)
#define EXT2_IOC_SETVERSION		_IOW('v', 2, long)
#define EXT2_IOC_GETVERSION_NEW		_IOR('f', 3, long)
#define EXT2_IOC_SETVERSION_NEW		_IOW('f', 4, long)
#define EXT2_IOC_GROUP_EXTEND		_IOW('f', 7, unsigned long)
#define EXT2_IOC_GROUP_ADD		_IOW('f', 8,struct ext2_new_group_input)
#define EXT4_IOC_GROUP_ADD		_IOW('f', 8,struct ext4_new_group_input)
#define EXT4_IOC_RESIZE_FS		_IOW('f', 16, __u64)
#endif
#endif

# include <cerrno>
# include <cstdlib>
# include <cstring>

#ifdef OS_mint
#include <mint/osbind.h>
#include <mint/mintbind.h>
#endif /* OS_mint */

#ifdef __CYGWIN__
#include <sys/socket.h> /* for FIONREAD */
#endif

#include "../../atari/hostfs/hostfs_nfapi.h"	/* XFS_xx and DEV_xx enum */

#ifndef __has_builtin
#define __has_builtin(x) 0
#define __builtin_available(...) 1
#else
#if !__has_builtin(__builtin_available)
#define __builtin_available(...) 1
#endif
#endif


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

// for the FS_EXT3 using host.xfs (recently changed)
#define FS_EXT_3 0x800


#define SHR(a, b)       \
  (-1 >> 1 == -1        \
   ? (a) >> (b)         \
   : (a) / (1 << (b)) - ((a) % (1 << (b)) < 0))

static long gmtoff(time_t t)
{
	struct tm *tp;

	if ((tp = localtime(&t)) == NULL)
		return 0;
#ifdef HAVE_STRUCT_TM_TM_GMTOFF
	return tp->tm_gmtoff;
#else
	{
		struct tm gtm;
		struct tm ltm;
		time_t lt;
		long a4, b4, a100, b100, a400, b400, intervening_leap_days, years, days;
		ltm = *tp;
		lt = mktime(&ltm);
	
		if (lt == (time_t) -1)
		{
			/* mktime returns -1 for errors, but -1 is also a
			   valid time_t value.  Check whether an error really
			   occurred.  */
			struct tm tm;
	
			if ((tp = localtime(&lt)) == NULL)
				return 0;
			tm = *tp;
			if ((ltm.tm_sec ^ tm.tm_sec) ||
				(ltm.tm_min ^ tm.tm_min) ||
				(ltm.tm_hour ^ tm.tm_hour) ||
				(ltm.tm_mday ^ tm.tm_mday) ||
				(ltm.tm_mon ^ tm.tm_mon) ||
				(ltm.tm_year ^ tm.tm_year))
				return 0;
		}
	
		if ((tp = gmtime(&lt)) == NULL)
			return 0;
		gtm = *tp;
		
		a4 = SHR(ltm.tm_year, 2) + SHR(1900, 2) - !(ltm.tm_year & 3);
		b4 = SHR(gtm.tm_year, 2) + SHR(1900, 2) - !(gtm.tm_year & 3);
		a100 = a4 / 25 - (a4 % 25 < 0);
		b100 = b4 / 25 - (b4 % 25 < 0);
		a400 = SHR(a100, 2);
		b400 = SHR(b100, 2);
		intervening_leap_days = (a4 - b4) - (a100 - b100) + (a400 - b400);
		years = ltm.tm_year - gtm.tm_year;
		days = (365 * years + intervening_leap_days + (ltm.tm_yday - gtm.tm_yday));
	
		return (60 * (60 * (24 * days + (ltm.tm_hour - gtm.tm_hour)) + (ltm.tm_min - gtm.tm_min)) + (ltm.tm_sec - gtm.tm_sec));
	}
#endif
}


static long mint_fake_gmtoff(time_t t)
{
	long offset;

	/*
	 * The mint kernel currently uses a fixed timezone offset
	 * for calculating UTC timestamps (the one for the current
	 * local time). So we must correct that here.
	 */
	offset = gmtoff(t);
	offset -= gmtoff(time(NULL));
	return offset;
}


#if defined HAVE_STRUCT_STAT_ST_ATIM_TV_NSEC
# ifdef TYPEOF_STRUCT_STAT_ST_ATIM_IS_STRUCT_TIMESPEC
#  define STAT_TIMESPEC(st, st_xtim) ((st)->st_xtim)
# else
#  define STAT_TIMESPEC_NS(st, st_xtim) ((st)->st_xtim.tv_nsec)
# endif
#elif defined HAVE_STRUCT_STAT_ST_ATIMESPEC_TV_NSEC
# define STAT_TIMESPEC(st, st_xtim) ((st)->st_xtim##espec)
#elif defined HAVE_STRUCT_STAT_ST_ATIMENSEC
# define STAT_TIMESPEC_NS(st, st_xtim) ((st)->st_xtim##ensec)
#elif defined HAVE_STRUCT_STAT_ST_ATIM_ST__TIM_TV_NSEC
# define STAT_TIMESPEC_NS(st, st_xtim) ((st)->st_xtim.st__tim.tv_nsec)
#endif

/* Return the nanosecond component of *ST's access time.  */
static long int
get_stat_atime_ns (struct stat const *st)
{
# if defined STAT_TIMESPEC
  return STAT_TIMESPEC (st, st_atim).tv_nsec;
# elif defined STAT_TIMESPEC_NS
  return STAT_TIMESPEC_NS (st, st_atim);
# else
  (void) st;
  return 0;
# endif
}

/* Return the nanosecond component of *ST's status change time.  */
static long int
get_stat_ctime_ns (struct stat const *st)
{
# if defined STAT_TIMESPEC
  return STAT_TIMESPEC (st, st_ctim).tv_nsec;
# elif defined STAT_TIMESPEC_NS
  return STAT_TIMESPEC_NS (st, st_ctim);
# else
  (void) st;
  return 0;
# endif
}

#ifndef HAVE_FUTIMES
int futimes(int fd, const struct timeval tv[2])
{
	UNUSED(fd);
	UNUSED(tv);
	errno = ENOSYS;
	return -1;
}
#endif

/* Return the nanosecond component of *ST's data modification time.  */
static long int
get_stat_mtime_ns (struct stat const *st)
{
# if defined STAT_TIMESPEC
  return STAT_TIMESPEC (st, st_mtim).tv_nsec;
# elif defined STAT_TIMESPEC_NS
  return STAT_TIMESPEC_NS (st, st_mtim);
# else
  (void) st;
  return 0;
# endif
}

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
			D(bug("HOSTFS: version %d", ret));
    		break;

    	case GET_DRIVE_BITS:
			ret = 0;
			for(int i=0; i<(int)(sizeof(bx_options.aranymfs.drive)/sizeof(bx_options.aranymfs.drive[0])); i++)
				if (bx_options.aranymfs.drive[i].rootPath[0])
					ret |= (1 << i);
			D(bug("HOSTFS: drvBits %08x", (uint32)ret));
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
								0,
								getParameter(1) /* XATTR* */ );
			flushXFSC( &fc, getParameter(0) );
			break;

		case XFS_STAT64:
			D(bug("%s", "fs_stat64"));
			fetchXFSC( &fc, getParameter(0) );
			ret = xfs_stat64( &fc,
							  0,
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
			// not needed: flushXFSC( &resFc, getParameter(2) );
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
    		XfsCookie  fromDir, toDir;
			fetchXFSC( &fromDir, getParameter(0) );
			fetchXFSC( &toDir, getParameter(2) );
			ret = xfs_hardlink( &fromDir,
							   (memptr)getParameter(1), /* fromName */
							   &toDir,
							   (memptr)getParameter(3)  /* toName */ );
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
			ret = xfs_dev_ioctl(&extFile, getParameter(1), (memptr)getParameter(2));
			flushXFSF( &extFile, getParameter(0) );
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
	/* devinfo unused */
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
	/* devinfo unused */
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
	ttm->tm_isdst = -1;
}


time_t HostFs::datetime2utc(uint32 dtm)
{
	struct tm ttm;
	datetime2tm(dtm, &ttm);
	return mktime(&ttm);
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
	if ( m & S_ISUID ) result |= 04000;

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
	if ( m & 04000 ) result |= S_ISUID;

	if ( (m & 0170000) == 0010000 ) result |= S_IFSOCK;
	if ( (m & 0170000) == 0020000 ) result |= S_IFCHR;
	if ( (m & 0170000) == 0040000 ) result |= S_IFDIR;
	if ( (m & 0170000) == 0060000 ) result |= S_IFBLK;
	if ( (m & 0170000) == 0100000 ) result |= S_IFREG;
	if ( (m & 0170000) == 0120000 ) result |= S_IFIFO;
	// Linux doesn't have this!	if ( m & 0140000 ) result |= S_IFMEM;
	if ( (m & 0170000) == 0160000 ) result |= S_IFLNK;

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


static int strapply_tolower(int c)
{
	return tolower(c);
}


static int strapply_toupper(int c)
{
	return toupper(c);
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
	strapply( dest, strapply_toupper );

	DFNAME(bug("HOSTFS: /transformFileName(\"%s\") -> \"%s\"", source, dest));
}


bool HostFs::getHostFileName( char* result, ExtDrive* drv, const char* pathName, const char* name )
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
				strapply( result, strapply_tolower );
			}
		}
		if ( dh != NULL )
			closedir( dh );
	}
	else {
		DFNAME(bug(" (stat OK)"));
	}

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
				Host2AtariUtf8Copy(buff, poschar, hostrootlen);
				return(TOS_E_OK);
			}
		}
	}
	if (len > 0) {
		//	there is no label name to extract
		//	fall back to a default label
		Host2AtariUtf8Copy(buff, "HOSTFS", len);
		return TOS_E_OK;
	} else {
		return(TOS_ENAMETOOLONG);
	}
}


int32 HostFs::xfs_mkdir( XfsCookie *dir, memptr name, uint16 mode )
{
	char fname[MAXPATHNAMELEN];
	Atari2HostUtf8Copy( fname, name, sizeof(fname) );

	char fpathName[MAXPATHNAMELEN];
	cookie2Pathname( dir, fname, fpathName );

	if ( HostFilesys::makeDir( (char*)fpathName, mode ) )
		return errnoHost2Mint(errno,TOS_EFILNF);

	return TOS_E_OK;
}


int32 HostFs::xfs_rmdir( XfsCookie *dir, memptr name )
{
	char fname[MAXPATHNAMELEN];
	Atari2HostUtf8Copy( fname, name, sizeof(fname) );

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
	Atari2HostUtf8Copy( fname, name, sizeof(fname) );

	// convert and mask out the file type bits for unix open()
	mode = modeMint2Host( mode ) & (S_ISUID|S_ISGID|S_ISVTX|S_IRWXU|S_IRWXG|S_IRWXO);

	D(bug("HOSTFS:  dev_creat (%s, flags: %#x, mode: %#o)", fname, flags, mode));

	char fpathName[MAXPATHNAMELEN];
	cookie2Pathname(dir,fname,fpathName); // get the cookie filename

	*fc = *dir;
	fc->index = 0;

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
	newFsFile->locks = 0;
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

		// clear the O_EXCL and O_CREAT flags
		flags &= ~(O_CREAT|O_EXCL);
	}

	int fd = open( fpathName, flags|O_BINARY, 0 );
	if (fd < 0)
		return errnoHost2Mint(errno,TOS_EFILNF);
	fp->hostFd = fd;
	
    #if SIZEOF_INT != 4 || DEBUG_NON32BIT
		fdMapper.putNative( fp->hostFd );
    #endif

	D(bug("HOSTFS: /dev_open (fd = %d)", fp->hostFd));
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

	D(bug("HOSTFS: /dev_read readCount (%d)", (int)(count - toRead)));
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

	D(bug("HOSTFS: /dev_write writeCount (%d)", (int)(count - toWrite)));
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
	Atari2HostUtf8Copy( fname, name, sizeof(fname) );

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
	Atari2HostUtf8Copy( foldname, oldname, sizeof(foldname) );
	Atari2HostUtf8Copy( fnewname, newname, sizeof(fnewname) );

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
	Atari2HostUtf8Copy( ffromname, fromname, sizeof(ffromname) );
	Atari2HostUtf8Copy( ftoname, toname, sizeof(ftoname) );

	char ffromName[MAXPATHNAMELEN];
	char ftoName[MAXPATHNAMELEN];
	cookie2Pathname( dir, ffromname, ffromName );
	
	strd2upath(ftoname, ftoname);
	strcpy( ftoName, ftoname );

	if (ftoName[0] == '\0' || ffromName[0] == '\0')
		return TOS_EFILNF;
	
	if (ftoName[0] != '/' && ftoName[1] != ':' )
	{
		// relative symlink. Use it as is
	} else {
		// search among the mount points to find suitable link...

		size_t nameLen = strlen(ftoname);
		/* convert U:/c/... to c:/... */
		if (nameLen >= 4 && strncasecmp(ftoname, "u:/", 3) == 0 &&
			DriveFromLetter(ftoname[3]) >= 0 &&
			(ftoname[4] == '\0' || ftoname[4] == '/'))
		{
			ftoname[0] = ftoname[3];
			memmove(ftoname + 2, ftoname + 4, nameLen - 3);
			nameLen -= 2;
		} else
		/* convert /c/... to c:/... */
		if (nameLen >= 2 && ftoname[0] == '/' &&
			DriveFromLetter(ftoname[1]) >= 0 &&
			(ftoname[2] == '\0' || ftoname[2] == '/'))
		{
			ftoname[0] = ftoname[1];
			ftoname[1] = ':';
		}
		if (nameLen == 2)
		{
			strcat(ftoname, "/");
			nameLen++;
		}
		
		if (strcmp(bx_options.aranymfs.symlinks, "conv") == 0)
		{
			bool found = false;
			for (MountMap::iterator it = mounts.begin(); it != mounts.end(); it++)
			{
				ExtDrive *drv = it->second;
				size_t mpLen = strlen( drv->mountPoint );
				if (mpLen == 0 || mpLen > nameLen)
					continue;
				if (strncasecmp(drv->mountPoint, ftoname, mpLen) == 0)
				{
					// target drive found; replace MiNTs mount point
					// with the hosts root directory
					int len = MAXPATHNAMELEN;
					safe_strncpy(ftoName, drv->hostRoot, len);
					int hrLen = strlen( drv->hostRoot );
					if (hrLen < len)
						safe_strncpy(ftoName + hrLen, ftoname + mpLen, len - hrLen);
					found = true;
					break;
				}
			}
			if (!found)
			{
				// undo a possible _unx2dos() conversion from MiNTlib
				if (toupper(ftoName[0]) == 'U' && ftoName[1] == ':')
					strcpy(ftoName, ftoname + 2);
			}
		} else
		{
			if (toupper(ftoName[0]) == 'U' && ftoName[1] == ':')
			{
				memmove(ftoName, ftoName + 2, strlen(ftoName + 2) + 1);
			} else
			{
				if (DriveFromLetter(ftoname[0]) >= 0 && ftoname[1] == ':')
				{
					ftoname[1] = ftoname[0];
					ftoname[0] = '/';
				}
				strcpy(ftoName, ftoname);
			}
		}
		if (ftoName[0] == '/' && DriveFromLetter(ftoname[1]) >= 0 &&
			(ftoName[2] == '\0' || ftoName[2] == '/'))
			ftoName[1] = tolower(ftoName[1]);
	}
	
	D(bug( "HOSTFS: fs_symlink: \"%s\" --> \"%s\"", ffromName, ftoName ));

	if ( symlink( ftoName, ffromName ) )
		return errnoHost2Mint(errno,TOS_EFILNF);

	return TOS_E_OK;
}


int32 HostFs::xfs_hardlink( XfsCookie *fromDir, memptr fromname, XfsCookie *toDir, memptr toname )
{
#ifdef HAVE_LINK
	char ffromname[MAXPATHNAMELEN];
	char ftoname[MAXPATHNAMELEN];
	Atari2HostUtf8Copy( ffromname, fromname, sizeof(ffromname) );
	Atari2HostUtf8Copy( ftoname, toname, sizeof(ftoname) );

	char ffromName[MAXPATHNAMELEN];
	cookie2Pathname( fromDir, ffromname, ffromName );

	char ftoName[MAXPATHNAMELEN];
	cookie2Pathname( toDir, ftoname, ftoName );

	D(bug( "HOSTFS: fs_hardlink: \"%s\" --> \"%s\"", ffromName, ftoName ));

	if ( link( ffromName, ftoName ) )
	{
		D(bug("link %s %s: %s", ffromName, ftoName, strerror(errno)));
		return errnoHost2Mint(errno,TOS_EFILNF);
	}
	
	return TOS_E_OK;
#else
	(void) fromDir;
	(void) fromname;
	(void) toDir;
	(void) toname;
	return TOS_EINVFN;
#endif
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

	    if (fp->fc.drv->fsFlags & FS_EXT_3)
	    {
		  tmb.actime = datetime;
		  tmb.actime -= mint_fake_gmtoff(tmb.actime);
		} else
		{
		  struct tm ttm;

		  datetime2tm( datetime, &ttm );
		  tmb.actime = mktime( &ttm );  /* access time */
		D(bug("HOSTFS: /dev_datime: setting to: %d.%d.%d %d:%d.%d",
				  ttm.tm_mday,
				  ttm.tm_mon,
				  ttm.tm_year + 1900,
				  ttm.tm_sec,
				  ttm.tm_min,
				  ttm.tm_hour
				  ));
		  tmb.actime -= gmtoff(tmb.actime);
		}
		tmb.modtime = tmb.actime; /* modification time */

		if (utime( fpathName, &tmb ) < 0)
			return errnoHost2Mint(errno, TOS_EACCES);
	}

	if (!wflag)
	{
		if (!(fp->fc.drv->fsFlags & FS_EXT_3))
		{
			datetime = statBuf.st_mtime + gmtoff(statBuf.st_mtime);
			datetime = (time2dos(datetime) << 16 ) | date2dos(datetime);
		} else
		{
			datetime = statBuf.st_mtime;
			datetime += mint_fake_gmtoff(statBuf.st_mtime);
		}
		WriteInt32( datetimep, datetime );
	}

	return TOS_E_OK; //EBADRQ;
}


int32 HostFs::xfs_pathconf( XfsCookie *fc, int16 which )
{
	char fpathName[MAXPATHNAMELEN];
	cookie2Pathname(fc, NULL, fpathName);

	int	 oldErrno = errno;

	// FIXME: Has to be different for .XFS and for HOSTFS.
	D(bug("HOSTFS: fs_pathconf (%s,%d)", fpathName, which));

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
			{
				STATVFS buff;
				int32 res = host_statvfs( fpathName, &buff);
				if ( res != TOS_E_OK )
					return res;
				
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
			}
			
		case 4:	  // DP_ATOMIC
			{
				STATVFS buff;
				int32 res = host_statvfs( fpathName, &buff);
				if ( res != TOS_E_OK )
					return res;
				
				return buff.f_bsize;	 // ST max vs Linux optimal
			}
			
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

	// relative host fs symlinks are left alone. The MiNT kernel will parse them
	if ( target[0] != '/' && target[0] != '\\' && target[1] != ':' )
		return target;
	// convert to real path (example: "/tmp/../file" -> "/file")
	if (strcmp(bx_options.aranymfs.symlinks, "conv") == 0)
	{
		char *tmp = my_canonicalize_file_name(target, false);
		if (tmp == NULL)
			return target;
		size_t nameLen = strlen(tmp);
		for (MountMap::iterator it = mounts.begin(); it != mounts.end(); it++)
		{
			ExtDrive *drv = it->second;
			size_t hrLen = strlen( drv->hostRoot );
			if (hrLen == 0 || hrLen > nameLen)
				continue;
			if (strncmp(drv->hostRoot, tmp, hrLen) == 0)
			{
				// target drive found; replace the hosts root directory
				// with MiNTs mount point
				safe_strncpy(target, drv->mountPoint, len);
				int mLen = strlen( drv->mountPoint );
				if (mLen < len)
					safe_strncpy(target + mLen, tmp + hrLen, len - mLen);
				break;
			}
		}
		free(tmp);
	}
	
	D(bug("host_readlink(%s, %s)", pathname, target));

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


	fc->drv = dirh->fc.drv;
	fc->xfs = fc->drv->fsDrv;
	fc->dev = dirh->fc.dev;
	fc->aux = 0;
	fc->index = 0;

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

		if ( (uint16)len < strlen( dirEntry->d_name ) + 4 ) {
			delete newFsFile;
			return TOS_ERANGE;
		}

		char fpathName[MAXPATHNAMELEN];
		cookie2Pathname(&dirh->fc, dirEntry->d_name, fpathName);

/*
		struct stat statBuf;
		if ( lstat(fpathName, &statBuf) )
			return errnoHost2Mint(errno,TOS_EFILNF);
*/
		WriteInt32( (uint32)buff, dirEntry->d_ino /* statBuf.st_ino */ );
		Host2AtariUtf8Copy( buff + 4, dirEntry->d_name, len-4 );
	} else {
		char truncFileName[MAXPATHNAMELEN];
		transformFileName( truncFileName, (char*)dirEntry->d_name );

		D(bug("HOSTFS: fs_readdir (%s -> %s, %d)", (char*)dirEntry->d_name, (char*)truncFileName, len ));

		if ( (uint16)len < strlen( truncFileName ) ) {
			delete newFsFile;
			return TOS_ERANGE;
		}

		Host2AtariUtf8Copy( buff, truncFileName, len );
	}

	dirh->index++;
	dirh->fc.index->childCount++;

	MAPNEWVOIDP( newFsFile );
	newFsFile->parent = dirh->fc.index;
	newFsFile->refCount = 1;
	newFsFile->childCount = 0;
	newFsFile->created = false;
	newFsFile->locks = 0;

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

	(void) fc;
	if ( lstat(fpathName, statBuf) )
		return errnoHost2Mint(errno,TOS_EFILNF);

	return TOS_E_OK;
}

void HostFs::convert_to_xattr( ExtDrive *drv, const struct stat *statBuf, memptr xattrp )
{
	// XATTR structure conversion (COMPEND.HYP)
	/* UWORD mode	   */  WriteInt16( xattrp	  , modeHost2Mint(statBuf->st_mode) );
	/* LONG	 index	   */  WriteInt32( xattrp +	 2, statBuf->st_ino ); // FIXME: this is Linux's one

	/* UWORD dev	   */  WriteInt16( xattrp +	 6, statBuf->st_dev ); // FIXME: this is Linux's one

	/* UWORD reserved1 */  WriteInt16( xattrp +	 8, 0 );
	/* UWORD nlink	   */  WriteInt16( xattrp + 10, statBuf->st_nlink );
	/* UWORD uid	   */  WriteInt16( xattrp + 12, statBuf->st_uid );	 // FIXME: this is Linux's one
	/* UWORD gid	   */  WriteInt16( xattrp + 14, statBuf->st_gid );	 // FIXME: this is Linux's one
	/* LONG	 size	   */  WriteInt32( xattrp + 16, statBuf->st_size );
	unsigned long blksize, blocks;
#ifdef __MINGW32__
	blksize = 512 ; // FIXME: I just made up the number
#else
	blksize = statBuf->st_blksize;
#endif
	/* LONG	 blksize   */  WriteInt32( xattrp + 20, blksize );
	/*
	 * in struct xattr, "blocks" is the number blocks of size blksize
	 */
    if (blksize <= 512)
      blksize = 512;
#if defined(OS_beos) || defined (__MINGW32__)
	blocks = (statBuf->st_size + blksize - 1) / blksize;
#else
	blocks = (statBuf->st_blocks * 512 + blksize - 1) / blksize;
#endif
	/* LONG	 nblocks   */  WriteInt32( xattrp + 24, blocks );
    if (drv->fsFlags & FS_EXT_3)
    {
	/* UWORD mtime	   */  WriteInt32( xattrp + 28, statBuf->st_mtime + mint_fake_gmtoff(statBuf->st_mtime) );
	/* UWORD atime	   */  WriteInt32( xattrp + 32, statBuf->st_atime + mint_fake_gmtoff(statBuf->st_atime) );
	/* UWORD ctime	   */  WriteInt32( xattrp + 36, statBuf->st_ctime + mint_fake_gmtoff(statBuf->st_ctime) );
	} else
	{
	/* UWORD mtime	   */  WriteInt16( xattrp + 28, time2dos(statBuf->st_mtime) );
	/* UWORD mdate	   */  WriteInt16( xattrp + 30, date2dos(statBuf->st_mtime) );
	/* UWORD atime	   */  WriteInt16( xattrp + 32, time2dos(statBuf->st_atime) );
	/* UWORD adate	   */  WriteInt16( xattrp + 34, date2dos(statBuf->st_atime) );
	/* UWORD ctime	   */  WriteInt16( xattrp + 36, time2dos(statBuf->st_ctime) );
	/* UWORD cdate	   */  WriteInt16( xattrp + 38, date2dos(statBuf->st_ctime) );
	}
	/* UWORD attr	   */  WriteInt16( xattrp + 40, modeHost2TOS(statBuf->st_mode) );
	/* UWORD reserved2 */  WriteInt16( xattrp + 42, 0 );
	/* LONG	 reserved3 */  WriteInt32( xattrp + 44, 0 );
	/* LONG	 reserved4 */  WriteInt32( xattrp + 48, 0 );

	D(bug("HOSTFS: fs_getxattr mode %#02x, mtime %#04x, mdate %#04x", modeHost2Mint(statBuf->st_mode), ReadInt16(xattrp + 28), ReadInt16(xattrp + 30)));
}

int32 HostFs::xfs_getxattr( XfsCookie *fc, memptr name, memptr xattrp )
{
	char fpathName[MAXPATHNAMELEN];
	if (name)
	{
		char fname[MAXPATHNAMELEN];
		Atari2HostUtf8Copy( fname, name, sizeof(fname) );

		cookie2Pathname( fc, fname, fpathName );
	} else
	{
		cookie2Pathname( fc, NULL, fpathName );
	}

	D(bug("HOSTFS: fs_getxattr (%s)", fpathName));

	// perform the link stat itself
	struct stat statBuf;
	int32 res = host_stat64(fc, fpathName, &statBuf);
	if ( res != TOS_E_OK )
		return res;

	convert_to_xattr(fc->drv, &statBuf, xattrp);

	return TOS_E_OK;
}

void HostFs::convert_to_stat64( ExtDrive *drv, const struct stat *statBuf, memptr statp )
{
	(void) drv;
	/* LLONG    dev	   */  WriteInt64( statp +  0, statBuf->st_dev  ); // FIXME: this is Linux's one
	/* ULONG    ino	   */  WriteInt32( statp +  8, statBuf->st_ino ); // FIXME: this is Linux's one
	/* ULONG    mode   */  WriteInt32( statp + 12, modeHost2Mint(statBuf->st_mode) ); // FIXME: convert???
	/* ULONG    nlink  */  WriteInt32( statp + 16, statBuf->st_nlink );
	/* ULONG    uid	   */  WriteInt32( statp + 20, statBuf->st_uid ); // FIXME: this is Linux's one
	/* ULONG    gid	   */  WriteInt32( statp + 24, statBuf->st_gid ); // FIXME: this is Linux's one
	/* LLONG    rdev   */  WriteInt64( statp + 28, statBuf->st_rdev ); // FIXME: this is Linux's one

	/*    atime   */ WriteInt64( statp + 36,   statBuf->st_atime );
	/*    atime ns*/ WriteInt32( statp + 44, get_stat_atime_ns(statBuf) );
	/*    mtime   */ WriteInt64( statp + 48,   statBuf->st_mtime );
	/*    mtime ns*/ WriteInt32( statp + 56, get_stat_mtime_ns(statBuf) );
	/*    ctime   */ WriteInt64( statp + 60,   statBuf->st_ctime );
	/*    ctime ns*/ WriteInt32( statp + 68, get_stat_ctime_ns(statBuf) );

	/* LLONG    size   */  WriteInt64( statp + 72, statBuf->st_size );
	uint64 blksize, blocks;
#ifdef __MINGW32__
	blksize = 512 ; // FIXME: I just made up the number
#else
	blksize = statBuf->st_blksize;
#endif
	/*
	 * in struct stat, "blocks" is the number blocks of size 512
	 */
    if (blksize <= 512)
      blksize = 512;
#if defined(OS_beos) || defined (__MINGW32__)
	blocks = (statBuf->st_size + blksize - 1) / 512;
#else
	blocks = statBuf->st_blocks;
#endif
	/* LLONG    blocks */  WriteInt64( statp + 80,   blocks );
	/* ULONG    blksize*/  WriteInt32( statp + 88,   blksize );
	/* ULONG    flags  */  WriteInt32( statp + 92,   0 );
	/* ULONG    gen    */  WriteInt32( statp + 96,   0 );
	/* ULONG    reserverd[0]    */  WriteInt32( statp + 100,   0 );
	/* ULONG    reserverd[1]    */  WriteInt32( statp + 104,   0 );
	/* ULONG    reserverd[2]    */  WriteInt32( statp + 108,   0 );
	/* ULONG    reserverd[3]    */  WriteInt32( statp + 112,   0 );
	/* ULONG    reserverd[4]    */  WriteInt32( statp + 116,   0 );
	/* ULONG    reserverd[5]    */  WriteInt32( statp + 120,   0 );
	/* ULONG    reserverd[6]    */  WriteInt32( statp + 124,   0 );

	D(bug("HOSTFS: fs_stat64 mode %#02x, mtime %#08x", modeHost2Mint(statBuf->st_mode), ReadInt32(statp + 52)));
}

int32 HostFs::xfs_stat64( XfsCookie *fc, memptr name, memptr statp )
{
	char fpathName[MAXPATHNAMELEN];
	if (name)
	{
		char fname[MAXPATHNAMELEN];
		Atari2HostUtf8Copy( fname, name, sizeof(fname) );

		cookie2Pathname( fc, fname, fpathName );
	} else
	{
		cookie2Pathname( fc, NULL, fpathName );
	}

	D(bug("HOSTFS: fs_stat64 (%s)", fpathName));

	// perform the link stat itself
	struct stat statBuf;
	int32 res = host_stat64(fc, fpathName, &statBuf);
	if ( res != TOS_E_OK )
		return res;

	convert_to_stat64(fc->drv, &statBuf, statp);
	
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
		   "  devdrv = %#08x\n",
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
	fc->index->created = false;
	fc->index->locks = 0;

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


int32 HostFs::xfs_readlink( XfsCookie *dir, memptr buf, int16 len )
{
	char fpathName[MAXPATHNAMELEN];
	cookie2Pathname(dir,NULL,fpathName); // get the cookie filename

	D(bug( "HOSTFS: fs_readlink: %s", fpathName ));

	char target[MAXPATHNAMELEN];
	if (!host_readlink(fpathName,target,sizeof(target)))
		return errnoHost2Mint( errno, TOS_EFILNF );

	D(bug( "HOSTFS: fs_readlink: -> %s", target ));

	Host2AtariUtf8Copy( buf, target, len );
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
	Atari2HostUtf8Copy( fname, name, sizeof(fname) );

	D(bug( "HOSTFS: fs_lookup: %s", fname ));

	XfsFsFile *newFsFile;

	*fc = *dir;
	fc->index = 0;

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
		if ( lstat( fpathName, &statBuf ) ) {
			delete newFsFile;
			return errnoHost2Mint( errno, TOS_EFILNF );
		}

		MAPNEWVOIDP( newFsFile );
		newFsFile->name = strdup(fname);
		newFsFile->refCount = 1;
		newFsFile->childCount = 0;
		newFsFile->created = false;
		newFsFile->locks = 0;
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

        Host2AtariUtf8Copy( pathName, fpathName, pfpathName - fpathName + 1 );
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

# define MINT_EXT2_IOC_GETFLAGS       (('f'<< 8) | 1)
# define MINT_EXT2_IOC_SETFLAGS       (('f'<< 8) | 2)
# define MINT_EXT2_IOC_GETVERSION_NEW (('f'<< 8) | 3)
# define MINT_EXT2_IOC_SETVERSION_NEW (('f'<< 8) | 4)

# define MINT_EXT2_IOC_GETVERSION     (('v'<< 8) | 1)
# define MINT_EXT2_IOC_SETVERSION     (('v'<< 8) | 2)

# define MINT_F_GETLK  5
# define MINT_F_SETLK  6
# define MINT_F_SETLKW 7


int32 HostFs::xfs_fscntl ( XfsCookie *dir, memptr name, int16 cmd, int32 arg)
{
	switch ((uint16)cmd)
	{
		case MX_KER_XFSNAME:
		{
			D(bug( "HOSTFS: fs_fscntl: MX_KER_XFSNAME: arg = %08x", arg ));
			Host2AtariUtf8Copy(arg, "hostfs", 8);
			return TOS_E_OK;
		}
		case MINT_FS_INFO:
		{
			D(bug( "HOSTFS: fs_fscntl: FS_INFO: arg = %08x", arg ));
			if (arg)
			{
				Host2AtariUtf8Copy(arg, "hostfs-xfs", 32);
				WriteInt32(arg+32, ((int32)HOSTFS_XFS_VERSION << 16) | HOSTFS_NFAPI_VERSION );
				WriteInt32(arg+36, MINT_FS_HOSTFS );
				Host2AtariUtf8Copy(arg+40, "host filesystem", 32);
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
				/* LLONG    blocks */  WriteInt64( arg +  4, buff.f_blocks );
				/* LLONG    freebs */  WriteInt64( arg + 12, buff.f_bavail );
				/* LLONG    inodes */  WriteInt64( arg + 20, 0xffffffffffffffffULL);
				/* LLONG    finodes*/  WriteInt64( arg + 28, 0xffffffffffffffffULL);
			}
			return TOS_E_OK;
		}

		case MINT_V_CNTR_WP:
			// FIXME: TODO!
			break;

		case MINT_FUTIME:
			// Mintlib calls the dcntl(FUTIME_ETC, filename) first (below),
			// but other libs might not know.
		{
			char fpathName[MAXPATHNAMELEN];
			struct utimbuf t_set;

			if (name)
			{
				char fname[MAXPATHNAMELEN];
				Atari2HostUtf8Copy( fname, name, sizeof(fname) );

				cookie2Pathname( dir, fname, fpathName );
			} else
			{
				cookie2Pathname( dir, NULL, fpathName );
			}
			
			if (arg)
			{
				t_set.actime  = ReadInt32( arg );
				t_set.modtime = ReadInt32( arg + 4 );
				t_set.actime = datetime2utc(t_set.actime) - gmtoff(t_set.actime);
				t_set.modtime = datetime2utc(t_set.modtime) - gmtoff(t_set.modtime);
			} else
			{
				t_set.actime = t_set.modtime = time(NULL);
			}
			if (utime(fpathName, &t_set))
				return errnoHost2Mint( errno, TOS_EFILNF );

			return TOS_E_OK;
		}

		case MINT_FUTIME_UTC:
		{
			char fpathName[MAXPATHNAMELEN];
			struct utimbuf t_set;

			if (name)
			{
				char fname[MAXPATHNAMELEN];
				Atari2HostUtf8Copy( fname, name, sizeof(fname) );

				cookie2Pathname( dir, fname, fpathName );
			} else
			{
				cookie2Pathname( dir, NULL, fpathName );
			}
			
			if (arg)
			{
				t_set.actime  = ReadInt32( arg );
				t_set.modtime = ReadInt32( arg + 4 );
				t_set.actime -= mint_fake_gmtoff(t_set.actime);
				t_set.modtime -= mint_fake_gmtoff(t_set.modtime);
			} else
			{
				t_set.actime = t_set.modtime = time(NULL);
			}
			if (utime(fpathName, &t_set))
				return errnoHost2Mint( errno, TOS_EFILNF );

			return TOS_E_OK;
		}

		case MINT_FTRUNCATE:
		{
			char fname[MAXPATHNAMELEN];
			Atari2HostUtf8Copy( fname, name, sizeof(fname) );
			char fpathName[MAXPATHNAMELEN];
			cookie2Pathname(dir,fname,fpathName); // get the cookie filename

			D(bug( "HOSTFS: fs_fscntl: FTRUNCATE: %s, %08x", fpathName, arg ));
			if(truncate(fpathName, arg))
				return errnoHost2Mint( errno, TOS_EFILNF );

			return TOS_E_OK;
		}

		case MINT_FSTAT:
			return xfs_getxattr(dir, name, arg);

		case MINT_FSTAT64:
			return xfs_stat64(dir, name, arg);
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
			int navail;
			
#ifdef FIONREAD
			if (ioctl(fp->hostFd, FIONREAD, &navail) < 0)
#endif
			{
				int32 pos = lseek( fp->hostFd, 0, SEEK_CUR ); // get position
				navail = lseek( fp->hostFd, 0, SEEK_END ) - pos;
				lseek( fp->hostFd, pos, SEEK_SET ); // set the position back
			}
			WriteInt32(buff, navail);
			return TOS_E_OK;
		}
		case MINT_FIOEXCEPT:
			WriteInt32(buff, 0);
			return TOS_E_OK;

		case MINT_FUTIME:
#ifdef HAVE_FUTIMENS
			// Mintlib calls the dcntl(FUTIME_UTC, filename) first (below).
			// but other libs might not know.
			if (__builtin_available(macOS 10.13, iOS 11, tvOS 11, watchOS 4, *))
			{
				struct timespec ts[2];
				if (buff)
				{
					ts[0].tv_sec = ReadInt32(buff);
					ts[1].tv_sec = ReadInt32(buff + 4);
					ts[0].tv_sec = datetime2utc(ts[0].tv_sec) - gmtoff(ts[0].tv_sec);
					ts[1].tv_sec = datetime2utc(ts[1].tv_sec) - gmtoff(ts[1].tv_sec);
				} else
				{
					ts[0].tv_sec = ts[1].tv_sec = time(NULL);
				}
				ts[0].tv_nsec = ts[1].tv_nsec = 0;
				if (futimens(fp->hostFd, ts))
				    return errnoHost2Mint( errno, TOS_EACCES );
			} else
#endif
			{
				struct timeval tv[2];
				if (buff)
				{
					tv[0].tv_sec = ReadInt32(buff);
					tv[1].tv_sec = ReadInt32(buff + 4);
					tv[0].tv_sec = datetime2utc(tv[0].tv_sec) - gmtoff(tv[0].tv_sec);
					tv[1].tv_sec = datetime2utc(tv[1].tv_sec) - gmtoff(tv[1].tv_sec);
				} else
				{
					tv[0].tv_sec = tv[1].tv_sec = time(NULL);
				}
				tv[0].tv_usec = tv[1].tv_usec = 0;
				if (futimes(fp->hostFd, tv))
				    return errnoHost2Mint( errno, TOS_EACCES );
			}
			return TOS_E_OK;

		case MINT_FUTIME_UTC:
#ifdef HAVE_FUTIMENS
			if (__builtin_available(macOS 10.13, iOS 11, tvOS 11, watchOS 4, *))
			{
				struct timespec ts[2];
				if (buff)
				{
					ts[0].tv_sec = ReadInt32(buff);
					ts[1].tv_sec = ReadInt32(buff + 4);
					ts[0].tv_sec -= mint_fake_gmtoff(ts[0].tv_sec);
					ts[1].tv_sec -= mint_fake_gmtoff(ts[1].tv_sec);
				} else
				{
					ts[0].tv_sec = ts[1].tv_sec = time(NULL);
				}
				ts[0].tv_nsec = ts[1].tv_nsec = 0;
				if (futimens(fp->hostFd, ts))
				    return errnoHost2Mint( errno, TOS_EACCES );
			} else
#endif
			{
				struct timeval tv[2];
				if (buff)
				{
					tv[0].tv_sec = ReadInt32(buff);
					tv[1].tv_sec = ReadInt32(buff + 4);
					tv[0].tv_sec -= mint_fake_gmtoff(tv[0].tv_sec);
					tv[1].tv_sec -= mint_fake_gmtoff(tv[1].tv_sec);
				} else
				{
					tv[0].tv_sec = tv[1].tv_sec = time(NULL);
				}
				tv[0].tv_usec = tv[1].tv_usec = 0;
				if (futimes(fp->hostFd, tv))
				    return errnoHost2Mint( errno, TOS_EACCES );
			}
			return TOS_E_OK;

		case MINT_F_SETLK:
		case MINT_F_SETLKW:
		case MINT_F_GETLK:
			/*
			 * locking can't be handled here.
			 * It has to be done in hostfs.xfs on the Atari side
			 * (and is done now, with newer kernels).
			 * We must maintain the root pointer of MiNT's file
			 * locks list, however.
			 * Note that the "buff" arguments to flock() is a pointer
			 * to a struct flock, and older kernels without implementing
			 * the call will pass that to us, while the new implementation
			 * just passes the address of the root of the locks list.
			 */
			if (!fp->fc.index)
				return TOS_EIHNDL;
			if (mode == MINT_F_GETLK)
				WriteInt32(buff, fp->fc.index->locks);
			else
				fp->fc.index->locks = ReadInt32(buff);
			return TOS_E_OK;			

		case MINT_FTRUNCATE:
			D(bug( "HOSTFS: fs_ioctl: FTRUNCATE( fd=%d, %08lx )", fp->hostFd, (unsigned long)ReadInt32(buff) ));
			if ((fp->flags & O_ACCMODE) == O_RDONLY)
				return TOS_EACCES;

			if (ftruncate( fp->hostFd, ReadInt32(buff)))
				return errnoHost2Mint( errno, TOS_EACCES );

			return TOS_E_OK;

		case MINT_FSTAT:
			{
				struct stat statBuf;
				D(bug( "HOSTFS: fs_ioctl: FSTAT: arg = %08lx", (unsigned long)buff ));
				if (fstat( fp->hostFd, &statBuf))
					return errnoHost2Mint( errno, TOS_EFILNF );
				if (buff)
				    convert_to_xattr(fp->fc.drv, &statBuf, buff);
			}
			return TOS_E_OK;
		
		case MINT_FSTAT64:
			{
				struct stat statBuf;
				D(bug( "HOSTFS: fs_ioctl: FSTAT64: arg = %08lx", (unsigned long)buff ));
				if (fstat( fp->hostFd, &statBuf))
					return errnoHost2Mint( errno, TOS_EFILNF );
				if (buff)
				    convert_to_stat64(fp->fc.drv, &statBuf, buff);
			}
			return TOS_E_OK;
		
		case MX_KER_XFSNAME:
			D(bug( "HOSTFS: fs_ioctl: MX_KER_XFSNAME: arg = %08lx", (unsigned long)buff ));
			if (buff)
			    Host2AtariUtf8Copy(buff, "hostfs", 8);
			return TOS_E_OK;
		
#ifdef EXT2_IOC_GETFLAGS
		case MINT_EXT2_IOC_GETFLAGS:
		    {
		    long flags;
			D(bug( "HOSTFS: fs_ioctl: EXT2_IOC_GETFLAGS( fd=%d, %08lx )", fp->hostFd, (unsigned long)buff ));
			if (ioctl( fp->hostFd, EXT2_IOC_GETFLAGS, &flags))
				return errnoHost2Mint( errno, TOS_EACCES );
		    WriteInt32(buff, flags);
		    }
			return TOS_E_OK;
#endif

#ifdef EXT2_IOC_SETFLAGS
		case MINT_EXT2_IOC_SETFLAGS:
		    {
		    long flags;
			D(bug( "HOSTFS: fs_ioctl: EXT2_IOC_SETFLAGS( fd=%d, %08lx )", fp->hostFd, (unsigned long)ReadInt32(buff) ));
			flags = ReadInt32(buff);
			if (ioctl( fp->hostFd, EXT2_IOC_SETFLAGS, &flags))
				return errnoHost2Mint( errno, TOS_EACCES );
		    }
			return TOS_E_OK;
#endif

#if defined EXT2_IOC_GETVERSION || defined EXT2_IOC_GETVERSION_NEW
		case MINT_EXT2_IOC_GETVERSION:
		case MINT_EXT2_IOC_GETVERSION_NEW:
		    {
		    long version;
			D(bug( "HOSTFS: fs_ioctl: EXT2_IOC_GETVERSION( fd=%d, %08lx )", fp->hostFd, (unsigned long)buff ));
#ifdef EXT2_IOC_GETVERSION_NEW
			if (ioctl( fp->hostFd, EXT2_IOC_GETVERSION_NEW, &version))
#endif
				if (ioctl( fp->hostFd, EXT2_IOC_GETVERSION, &version))
					return errnoHost2Mint( errno, TOS_EACCES );
		    WriteInt32(buff, version);
		    }
			return TOS_E_OK;
#endif

#if defined EXT2_IOC_SETVERSION || defined EXT2_IOC_SETVERSION_NEW
		case MINT_EXT2_IOC_SETVERSION:
		    {
		    long version;
			D(bug( "HOSTFS: fs_ioctl: EXT2_IOC_SETVERSION( fd=%d, %08lx )", fp->hostFd, (unsigned long)ReadInt32(buff) ));
			version  = ReadInt32(buff);
#ifdef EXT2_IOC_SETVERSION_NEW
			if (ioctl( fp->hostFd, EXT2_IOC_SETVERSION_NEW, &version))
#endif
				if (ioctl( fp->hostFd, EXT2_IOC_SETVERSION, &version))
					return errnoHost2Mint( errno, TOS_EACCES );
		    }
			return TOS_E_OK;
#endif

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
		fsFlags = old->fsFlags;
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
	Atari2HostUtf8Copy( fmountPoint, mountpoint, sizeof(fmountPoint) );

	ExtDrive *drv = new ExtDrive();
	drv->fsDrv = filesys;
	drv->fsFlags = ReadInt32(filesys + 4);
	drv->fsDevDrv = filesys_devdrv;

	/*
	 * for drivers that are not running under mint,
	 * report our current timezone (in the filesys.res1 field)
	 */
	WriteInt32(filesys + 136, timezone);

	int16 dnum = -1;
	size_t len = strlen( fmountPoint );
	if ( len == 2 && fmountPoint[1] == ':' ) {
		// The mountPoint is of a "X:" format: (BetaDOS mapping)
		dnum = DriveFromLetter(fmountPoint[0]);
	}
	else if (len >= 4 && strncasecmp(fmountPoint, "u:\\", 3) == 0)
	{
		// the hostfs.xfs tries to map drives to u:\\X
		// in this case we use the [HOSTFS] of config file here
		dnum = DriveFromLetter(fmountPoint[3]);
		/* convert U:/c/... to c:/... */
		fmountPoint[0] = toupper(fmountPoint[3]);
		memmove(fmountPoint + 2, fmountPoint + 4, len - 3);
		len -= 2;
	}
	strd2upath(fmountPoint, fmountPoint);
	if (len > 0 && fmountPoint[len - 1] != '/')
		strcat(fmountPoint, "/");
	drv->mountPoint = strdup( fmountPoint );
	
	int maxdnum = sizeof(bx_options.aranymfs.drive) / sizeof(bx_options.aranymfs.drive[0]);
	if (dnum >= 0 && dnum < maxdnum) {
		drv->hostRoot = my_canonicalize_file_name( bx_options.aranymfs.drive[dnum].rootPath, true );
		drv->halfSensitive = bx_options.aranymfs.drive[dnum].halfSensitive;
	}
	else {
		dnum = -1; // invalidate dnum

		// no [HOSTFS] match -> map to the passed mountpoint
		//  - future extension to map from m68k side
		char fhostroot[MAXPATHNAMELEN];
		Atari2HostUtf8Copy( fhostroot, hostroot, sizeof(fhostroot) );

		drv->hostRoot = my_canonicalize_file_name( fhostroot, true );
		drv->halfSensitive = halfSensitive;
	}

	// no rootPath -> do not map the drive
	if ( drv->hostRoot == NULL || !strlen(drv->hostRoot) ) {
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


	D(bug("HOSTFS: fs_native_init:\n"
	    "\t\t fs_drv	   = %#08x, flags %#08x\n"
		"\t\t fs_devdrv  = %#08x\n"
		  "\t\t fs_devnum  = %#04x (%c)\n"
		  "\t\t fs_mountPoint = %s\n"
		  "\t\t fs_hostRoot   = %s [%d]\n"
		  ,drv->fsDrv, drv->fsFlags
		  ,drv->fsDevDrv
		  ,(int)devnum
		  ,dnum < 0 ? '-' : DriveToLetter(dnum)
		  ,drv->mountPoint
		  ,drv->hostRoot
		  ,(int)strlen(drv->hostRoot)
		  ));

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
