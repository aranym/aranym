/*
 * hostfs.h - HostFS routines - declaration
 *
 * Copyright (c) 2001-2004 STanda of ARAnyM development team (see AUTHORS)
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

#ifndef _HOSTFS_H
#define _HOSTFS_H

#ifdef HOSTFS_SUPPORT

#include "nf_base.h"
#include "tools.h"
#include "win32_supp.h"

#include <map>

class HostFs : public NF_Base
{
	// the maximum pathname length allowed
	// note: this would be nice to be rewriten using
	//       the std::string to become unlimited
	static const int MAXPATHNAMELEN = 2048;

	struct ExtDrive	{
		int16     driveNumber;
		memptr    fsDrv;
		memptr    fsDevDrv;
		int32     fsFlags;
		char      *hostRoot;
		char      *mountPoint;
		bool      halfSensitive;

	  public:
		// copy constructor
		ExtDrive() {
			driveNumber = -1;
			fsDrv = 0;
			fsDevDrv = 0;
			fsFlags = 0;
			hostRoot = NULL;
			mountPoint = NULL;
			halfSensitive = true;
		}
		ExtDrive( ExtDrive *old );
		~ExtDrive() {
			free( hostRoot );
			free( mountPoint );
		}
	};

	struct XfsFsFile {
		XfsFsFile *parent;
		uint32	  refCount;
		uint32	  childCount;
		bool      created;      // only xfs_creat() was issued (no dev_open yet)

		memptr locks;
		char	  *name;
	};

	struct XfsCookie {
		memptr    xfs;          // m68k filesystem pointer
		uint16    dev;          // device number
		uint16    aux;          // used by FreeMiNT (custom filsystem field)
		XfsFsFile *index;       // the filesystem implementation specific structure (host one)

		ExtDrive  *drv;         // dev->drv mapping is found during the cookie fetch
	};

	struct ExtFile {
		XfsCookie fc;           // file cookie like in FreeMiNT
		int16	  flags;        // open flags
		int16	  links;        // number of MetaDOS's pointers that points to this

		int       hostFd;       // host filedescriptor of the file
	};

	struct XfsDir {
		XfsCookie fc;			// cookie for this directory
		uint16    index;		// index of the current entry
		uint16    flags;		// flags (e. g. tos or not)

		DIR       *hostDir;		// used DIR (host one)
	};


	// mountpoints map
	typedef std::map<int16,ExtDrive*> MountMap;
	MountMap mounts;

	#if SIZEOF_INT != 4 || DEBUG_NON32BIT
		// host filedescriptor mapper
		NativeTypeMapper<int> fdMapper;
	#endif

	void debugCookie( HostFs::XfsCookie *fc );

  private:
	void freeMounts();

  protected:
    void convert_to_xattr( ExtDrive *drv, const struct stat *statBuf, memptr xattrp );
    void convert_to_stat64( ExtDrive *drv, const struct stat *statBuf, memptr statp );

  public:
	HostFs();
	~HostFs();
	void reset();

	/**
	 * MetaDos DOS driver dispatch functions.
	 **/
	const char *name() { return "HOSTFS"; }
	bool isSuperOnly() { return true; }
	int32 dispatch( uint32 emulop );

	/**
	 * Host to ARAnyM structure conversion routines.
	 **/
	mode_t modeMint2Host( uint16 m );
	uint16 modeHost2Mint( mode_t m );
	uint16 modeHost2TOS( mode_t m );
	int    flagsMint2Host( uint16 flags );
	int16  flagsHost2Mint( int flags );
	uint16 time2dos( time_t t );
	uint16 date2dos( time_t t );
	void   datetime2tm( uint32 dtm, struct tm* ttm );
	time_t datetime2utc(uint32 dtm);

	/**
	 * Path conversions.
	 *
	 * Note: This is the most sophisticated thing in this object.
	 **/
	void transformFileName( char* dest, const char* source );
	bool getHostFileName( char* result, ExtDrive* drv, const char* pathName, const char* name );

	void fetchXFSC( XfsCookie *fc, memptr filep );
	void flushXFSC( XfsCookie *fc, memptr filep );
	void fetchXFSF( ExtFile *extFile, memptr filep );
	void flushXFSF( ExtFile *extFile, memptr filep );
	void fetchXFSD( XfsDir *dirh, memptr dirp );
	void flushXFSD( XfsDir *dirh, memptr dirp );

	char *cookie2Pathname( ExtDrive *drv, XfsFsFile *fs, const char *name, char *buf );
	char *cookie2Pathname( XfsCookie *fc, const char *name, char *buf );
	int32 host_stat64  ( XfsCookie *fc, const char *pathname, struct stat *statbuf );
	int32 host_statvfs ( const char *fpathName, void *buff );
	char *host_readlink( const char *pathname, char *target, int len );
	DIR  *host_opendir(  const char *name );

	void xfs_freefs( XfsFsFile *fs );

	int32 xfs_native_init( int16 devnum, memptr mountpoint, memptr hostroot, bool halfSensitive,
						   memptr filesys, memptr filesys_devdrv );

	int32 xfs_root( uint16 dev, XfsCookie *fc );
	int32 xfs_fscntl ( XfsCookie *dir, memptr name, int16 cmd, int32 arg );
	int32 xfs_dupcookie( XfsCookie *newCook, XfsCookie *oldCook );
	int32 xfs_release( XfsCookie *fc );
	int32 xfs_getxattr( XfsCookie *fc, memptr name, memptr xattrp );
	int32 xfs_stat64( XfsCookie *fc, memptr name, memptr statp );
	int32 xfs_chattr( XfsCookie *fc, int16 attr );
	int32 xfs_chmod( XfsCookie *fc, uint16 mode );
	int32 xfs_getdev( XfsCookie *fc, memptr devspecial );
	int32 xfs_lookup( XfsCookie *dir, memptr name, XfsCookie *fc );
	int32 xfs_getname( XfsCookie *relto, XfsCookie *dir, memptr pathName, int16 size );
	int32 xfs_creat( XfsCookie *dir, memptr name, uint16 mode, int16 flags, XfsCookie *fc );
	int32 xfs_rename( XfsCookie *olddir, memptr oldname, XfsCookie *newdir, memptr newname );
	int32 xfs_remove( XfsCookie *dir, memptr name );
	int32 xfs_pathconf( XfsCookie *fc, int16 which );
	int32 xfs_opendir( XfsDir *dirh, uint16 flags );
	int32 xfs_closedir( XfsDir *dirh );
	int32 xfs_readdir( XfsDir *dirh, memptr buff, int16 len, XfsCookie *fc );
	int32 xfs_rewinddir( XfsDir *dirh );
	int32 xfs_mkdir( XfsCookie *dir, memptr name, uint16 mode );
	int32 xfs_rmdir( XfsCookie *dir, memptr name );
	int32 xfs_dfree( XfsCookie *dir, memptr buff );
	int32 xfs_readlabel( XfsCookie *dir, memptr buff, int16 len );
	int32 xfs_symlink( XfsCookie *dir, memptr fromname, memptr toname );
	int32 xfs_readlink( XfsCookie *dir, memptr buff, int16 len );
	int32 xfs_hardlink( XfsCookie *fromDir, memptr fromname, XfsCookie *toDir, memptr toname );

	int32 xfs_dev_open(ExtFile *fp);
	int32 xfs_dev_close(ExtFile *fp, int16 pid);
	int32 xfs_dev_ioctl ( ExtFile *fp, int16 mode, memptr buff);
	int32 xfs_dev_datime( ExtFile *fp, memptr datetimep, int16 wflag);
	int32 xfs_dev_read( ExtFile *fp, memptr buffer, uint32 count);
	int32 xfs_dev_write(ExtFile *fp, memptr buffer, uint32 count);
	int32 xfs_dev_lseek(ExtFile *fp, int32 offset, int16 seekmode);

};

#endif // HOSTFS_SUPPORT

#endif // _HOSTFS_H
