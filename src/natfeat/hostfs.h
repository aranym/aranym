/*
 * $Header$
 *
 * STanda 3.5.2001
 */

#ifndef _HOSTFS_H
#define _HOSTFS_H

#include "nf_base.h"
#include <map>


// FIXME: it is stupid to place these here,
//        but I didn't want to touch many files in CVS ;(
//	      Feel free to move it anywhere
#ifndef MIN
#define MIN(_a,_b) ((_a)<(_b)?(_a):(_b))
#endif
#ifndef MAX
#define MAX(_a,_b) ((_a)>(_b)?(_a):(_b))
#endif


#define MAXPATHNAMELEN 2048

#ifdef HOSTFS_SUPPORT
class HostFs : public NF_Base {
  private:

	struct XfsFsFile {
		XfsFsFile *parent;
		uint32	  refCount;
		uint32	  childCount;
		char	  *name;
	};

	struct XfsCookie {
		uint32    xfs;
		uint16    dev;
		uint16    aux;
		XfsFsFile *index;
	};

	struct ExtFile {
		XfsCookie fc;
		int16	  index;
		int16	  flags;
		int16	  links;
		int32	  hostfd;
		int32	  offset;
		int32	  devinfo;
		uint32	  next;
	} ;				// See MYFILE in Julian's COOK_FS

	struct XfsDir {
		XfsCookie fc;			// cookie for this directory
		uint16    index;		// index of the current entry
		uint16    flags;		// flags (e. g. tos or not)
		DIR       *dir;			// used DIR
		int16	  pathIndex;	// index of the pathName in the internal pool FIXME?
		XfsDir	  *next;		// linked together so we can close them to process term
	};

	struct ExtDrive	{
		int16     driveNumber;
		memptr    fsDrv;
		memptr    fsDevDrv;
		char      *hostRoot;
		char      *mountPoint;
		bool      halfSensitive;

	  public:
		~ExtDrive() {
			free( hostRoot );
			free( mountPoint );
		}
	};

	typedef std::map<int16,ExtDrive*> MountMap;
	MountMap mounts;

	bool isPathValid(const char *fileName);
	void xfs_debugCookie( XfsCookie *fc );

  public:
	/**
	 * Installs the drive.
	 **/
	void init();
	void install( const char driveSign, const char* rootPath, bool halfSensitive );

	uint32 getDrvBits();

	HostFs() {}
	virtual ~HostFs() {
	}

	/**
	 * MetaDos DOS driver dispatch functions.
	 **/
	char *name() { return "HOSTFS"; }
	bool isSuperOnly() { return false; }
	int32 dispatch( uint32 emulop );

	/**
	 * Some crossplatform mem & str functions to use in GEMDOS replacement.
	 **/
	void a2fmemcpy( char *dest, memptr source, size_t count );
	void a2fstrcpy( char *dest, memptr source );
	void f2amemcpy( memptr dest, char *source, size_t count );
	void f2astrcpy( memptr dest, char *source );

	/**
	 * Unix to ARAnyM structure conversion routines.
	 **/
	uint32 unix2toserrno( int unixerrno,int defaulttoserrno );
	uint16 statmode2xattrmode( mode_t m );
	uint16 statmode2attr( mode_t m );
	uint16 time2dos( time_t t );
	uint16 date2dos( time_t t );
	void   datetime2tm( uint32 dtm, struct tm* ttm );
	int    st2flags( uint16 flags );
	int16  flags2st( int flags );

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

	char *cookie2Pathname( XfsFsFile *fs, const char *name, char *buf );
	void xfs_freefs( XfsFsFile *fs );

	void xfs_native_init( int16 devnum, memptr mountpoint, memptr hostroot, bool halfSensitive,
						  memptr filesys, memptr filesys_devdrv );

	int32 xfs_root( uint16 dev, XfsCookie *fc );
	int32 xfs_dupcookie( XfsCookie *newCook, XfsCookie *oldCook );
	int32 xfs_release( XfsCookie *fc );
	int32 xfs_getxattr( XfsCookie *fc, memptr xattrp );
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

#endif /* HOSTFS_SUPPORT */

#endif // _HOSTFS_H


/*
 * $Log$
 * Revision 1.1  2002/12/10 20:47:21  standa
 * The HostFS (the host OS filesystem access via NatFeats) implementation.
 *
 *
 */
