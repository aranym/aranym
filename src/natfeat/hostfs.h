/*
 * $Header$
 *
 * STanda 3.5.2001
 */

#ifndef _HOSTFS_H
#define _HOSTFS_H

#ifdef HOSTFS_SUPPORT


#include "nf_base.h"
#include "tools.h"
#ifdef HAVE_NEW_HEADERS
# include <map>
#else
# include <map.h>
#endif




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
		char      *hostRoot;
		char      *mountPoint;
		bool      halfSensitive;

	  public:
		// copy constructor
		ExtDrive() {
			driveNumber = -1;
			fsDrv = 0;
			fsDevDrv = 0;
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

	void xfs_debugCookie( XfsCookie *fc );

  public:
	/**
	 * Installs the drive.
	 **/
	void init();
	void install( const char driveSign, const char* rootPath, bool halfSensitive );

	uint32 getDrvBits();

	HostFs() {}
	virtual ~HostFs() {}

	/**
	 * MetaDos DOS driver dispatch functions.
	 **/
	char *name() { return "HOSTFS"; }
	bool isSuperOnly() { return true; }
	int32 dispatch( uint32 emulop );

	/**
	 * Host to ARAnyM structure conversion routines.
	 **/
	uint32 errnoHost2Mint( int unixerrno,int defaulttoserrno );
	mode_t modeMint2Host( uint16 m );
	uint16 modeHost2Mint( mode_t m );
	uint16 modeHost2TOS( mode_t m );
	int    flagsMint2Host( uint16 flags );
	int16  flagsHost2Mint( int flags );
	uint16 time2dos( time_t t );
	uint16 date2dos( time_t t );
	void   datetime2tm( uint32 dtm, struct tm* ttm );

	/**
	 * Path conversions.
	 *
	 * Note: This is the most sophisticated thing in this object.
	 **/
	void transformFileName( char* dest, const char* source );
	bool getHostFileName( char* result, ExtDrive* drv, char* pathName, const char* name );
	void convertPathA2F( ExtDrive *drv, char* fpathName, char* pathName, const char* basePath = NULL );

	void fetchXFSC( XfsCookie *fc, memptr filep );
	void flushXFSC( XfsCookie *fc, memptr filep );
	void fetchXFSF( ExtFile *extFile, memptr filep );
	void flushXFSF( ExtFile *extFile, memptr filep );
	void fetchXFSD( XfsDir *dirh, memptr dirp );
	void flushXFSD( XfsDir *dirh, memptr dirp );

	char *cookie2Pathname( ExtDrive *drv, XfsFsFile *fs, const char *name, char *buf );
	char *cookie2Pathname( XfsCookie *fc, const char *name, char *buf );
	void xfs_freefs( XfsFsFile *fs );

	int32 xfs_native_init( int16 devnum, memptr mountpoint, memptr hostroot, bool halfSensitive,
						   memptr filesys, memptr filesys_devdrv );

	int32 xfs_root( uint16 dev, XfsCookie *fc );
	int32 xfs_fscntl ( XfsCookie *dir, memptr name, int16 cmd, int32 arg );
	int32 xfs_dupcookie( XfsCookie *newCook, XfsCookie *oldCook );
	int32 xfs_release( XfsCookie *fc );
	int32 xfs_getxattr( XfsCookie *fc, memptr xattrp );
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
	int32 xfs_readlink( XfsCookie *dir, memptr buf, int16 len );
	int32 xfs_dfree( XfsCookie *dir, memptr buf );
	int32 xfs_symlink( XfsCookie *dir, memptr fromname, memptr toname );

	int32 xfs_dev_open(ExtFile *fp);
	int32 xfs_dev_close(ExtFile *fp, int16 pid);
	int32 xfs_dev_datime( ExtFile *fp, memptr datetimep, int16 wflag);
	int32 xfs_dev_read( ExtFile *fp, memptr buffer, uint32 count);
	int32 xfs_dev_write(ExtFile *fp, memptr buffer, uint32 count);
	int32 xfs_dev_lseek(ExtFile *fp, int32 offset, int16 seekmode);

};

#endif // HOSTFS_SUPPORT

#endif // _HOSTFS_H


/*
 * $Log$
 * Revision 1.10  2003/07/17 13:52:34  joy
 * hostfs fixes by Xavier
 *
 * Revision 1.9  2003/06/26 21:27:14  joy
 * xfs_creat()/xfs_dev_open() fixed (gunzip problem)
 * xfs_native_init() fixed to handle the FreeMiNT requirements correctly
 * general cleanup
 *
 * Revision 1.6.2.4  2003/04/15 19:09:26  standa
 * xfs_creat()/xfs_dev_open() fixed (gunzip problem).
 * xfs_native_init() fixed to handle the FreeMiNT requirements correctly.
 * general cleanup
 *
 * Revision 1.6.2.3  2003/04/10 21:49:34  joy
 * cygwin mapping fixed - compiler bug found
 *
 * Revision 1.6.2.2  2003/04/08 00:42:14  standa
 * The st2flags() and flags2st() methods fixed (a need for open()).
 * The isPathValid() method removed (was only useful for aranymfs.dos).
 * Dpathconf(8-9) added and 7 fixed (Thing has a bug in 1.27 here IIRC).
 * General debug messages cleanup.
 *
 * Revision 1.6.2.1  2003/04/03 12:11:29  standa
 * 32bit <-> host mapping + general hostfs cleanup.
 *
 * Revision 1.6  2003/03/20 01:08:17  standa
 * HOSTFS mapping update.
 *
 * Revision 1.5  2003/03/17 09:42:39  standa
 * The chattr,chmod implementation ported from stonx.
 *
 * Revision 1.4  2003/03/01 11:57:37  joy
 * major HOSTFS NF API cleanup
 *
 * Revision 1.3  2002/12/17 14:20:48  standa
 * Better STL suppor
 *
 * Revision 1.2  2002/12/16 15:39:18  standa
 * The map -> std::map
 *
 * Revision 1.1  2002/12/10 20:47:21  standa
 * The HostFS (the host OS filesystem access via NatFeats) implementation.
 *
 *
 */
