/*
 * $Header$
 *
 * STanda 3.5.2001
 */

#ifndef _EXTFS_H
#define _EXTFS_H

#include "sysdeps.h"
#include "cpu_emulation.h"

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

#ifdef EXTFS_SUPPORT
class ExtFs {
  private:

	struct LogicalDev {
		int16	dummy;
	};			// Dummy structure... I don't know the meaning in COOK_FS

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

	struct ExtDta {			// used by Fsetdta, Fgetdta
		DIR	*ds_dirh;		// opendir resulting handle to perform dirscan
		uint16	ds_attrib;	// search attribs wanted
		uint8	ds_index;	// index in the fs_pathName array (seems like a hack, but I don't know better)
		int8	ds_name[14];

		// And now GEMDOS specified fields
		uint8	d_attrib;
		uint16	d_time;
		uint16	d_date;
		uint32	d_length;
		int8	d_fname[14];
	};				// See myDTA in Julian's COOK_FS

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

	typedef struct XfsDir ExtDir;


	struct ExtDrive	{
		bool halfSensitive;
		char *rootPath;
		char *currPath;		// Only Dsetpath uses this in the .DOS driver -> can be removed
	};

	ExtDrive drives[ 'Z'-'A'+1 ];

	bool isPathValid(const char *fileName);

  public:
	ExtFs()
	{
		// This is the default drv (shouldn't be used)
		// note: This is the secure drive when some error occures.
		install( 'A', ".", true );

		// initialize the drive array to NULLs
		for( char i='B'; i<='Z'; i++ )
			install( i, NULL, false );
	}
	~ExtFs() {
		// FIXME: here the ExtDrive.rootPaths should be freed
	}

	/**
	 * Installs the drive.
	 **/
	void init();
	void install( const char driveSign, const char* rootPath, bool halfSensitive );

	uint32 getDrvBits();

	/**
	 * MetaDos DOS driver dispatch functions.
	 **/
	void dispatch( uint32 fncode, M68kRegisters *r );

	void fetchDTA( ExtDta *dta, uint32 dtap );
	void flushDTA( ExtDta *dta, uint32 dtap );
	void fetchFILE( ExtFile *extFile, uint32 filep );
	void flushFILE( ExtFile *extFile, uint32 filep );
	void fetchEDIR( ExtDir *extDir, uint32 dirp );
	void flushEDIR( ExtDir *extDir, uint32 dirp );

	/**
	 * Some crossplatform mem & str functions to use in GEMDOS replacement.
	 **/
	void a2fmemcpy( uint8 *dest, uint8 *source, size_t count );
	void a2fstrcpy( char *dest, uint8 *source );
	void f2amemcpy( uint8 *dest, uint8 *source, size_t count );
	void f2astrcpy( uint8 *dest, uint8 *source );

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
	ExtDrive* getDrive( const char* pathName );
	void transformFileName( char* dest, const char* source );
	bool getHostFileName( char* result, ExtDrive* drv, char* pathName, const char* name );
	void convertPathA2F( char* fpathName, char* pathName, char* basePath = NULL );

	int16  getFreeDirIndex( char **pathNames );
	void   freeDirIndex( uint8 index, char **pathNames );
	bool   filterFiles( ExtDta *dta, char *fpathName, char *mask, struct dirent *dirEntry );
	int32  findFirst( ExtDta *dta, char *fpathName );

	// GEMDOS functions
	int32 Dfree_(char *fpathName, uint32 diskinfop );
	int32 DfreeExtFs(LogicalDev *ldp, char *pathName, ExtFile *fp,
					 uint32 diskinfop, int16 drive );
	int32 DcreateExtFs(LogicalDev *ldp, char *pathName, ExtFile *fp,
					   const char *pn);
	int32 DdeleteExtFs(LogicalDev *ldp, char *pathName, ExtFile *fp,
					   const char *pn);
	int32 DsetpathExtFs(LogicalDev *ldp, char *pathName, ExtFile *fp,
						const char *pn);
	int32 FcreateExtFs(LogicalDev *ldp, char *pathName, ExtFile *fp,
					   const char *pn, int16 attr);
	int32 FopenExtFs(LogicalDev *ldp, char *pathName, ExtFile *fp,
					 const char *pn, int16 mode);
	int32 Fopen_( const char* fpathName, int flags, int mode, ExtFile *fp );
	int32 FcloseExtFs(LogicalDev *ldp, char *pathName, ExtFile *fp,
					  int16 handle);
	int32 FreadExtFs(LogicalDev *ldp, char *pathName, ExtFile *fp,
					 int16 handle, uint32 count, void *buffer);
	int32 FwriteExtFs(LogicalDev *ldp, char *pathName, ExtFile *fp,
					  int16 handle, uint32 count, void *buffer);
	int32 FdeleteExtFs(LogicalDev *ldp, char *pathName, ExtFile *fp,
					   const char *pn);
	int32 FseekExtFs(LogicalDev *ldp, char *pathName, ExtFile *fp,
					 int32 offset, int16 handle, int16 seekmode);

	int32 FattribExtFs(LogicalDev *ldp, char *pathName, ExtFile *fp,
					   const char* pn, int16 wflag, int16 attr);

	int32 FsfirstExtFs( LogicalDev *ldp, char *pathname, ExtDta *dta,
						const char *pn, int16 attribs );
	int32 FsnextExtFs ( LogicalDev *ldp, char *pathName, ExtDta *dta );
	int32 FrenameExtFs(LogicalDev *ldp, char *pathName, ExtFile *fp,
					   int16 reserved, char *oldpath, char *newPathName);
	int32 Fdatime_( char *fpathName, ExtFile *fp, uint32 *datetimep, int16 wflag);
	int32 FdatimeExtFs( LogicalDev *ldp, char *pathName, ExtFile *fp,
						uint32 *datetimep, int16 handle, int16 wflag);
	int32 FcntlExtFs( LogicalDev *ldp, char *pathName, ExtFile *fp,
					  int16 handle, void *arg, int16 cmd );
	int32 DpathconfExtFs( LogicalDev *ldp, char *pathName, ExtFile *fp,
						  const char* pn, int16 which );
	int32 Dpathconf_( char *fpathName, int16 which, ExtDrive *drv );

	int32 DopendirExtFs( LogicalDev *ldp, char *pathName, ExtDir *dirh, const char* pn, int16 flag);
	int32 Dopendir_( char *fpathName, ExtDir *dirh, int16 flag);
	int32 DclosedirExtFs( ExtDir *dirh );

	int32 DreaddirExtFs( LogicalDev *ldp, char *pathName, ExtDir *dirh,
						 int16 len, int32 dirhandle, char* buff );

	int32 DxreaddirExtFs( LogicalDev *ldp, char *pathName, ExtDir *dirh,
						  int16 len, int32 dirhandle, char* buff, uint32 xattrp, uint32 xretp );
	int32 Dxreaddir_( char *fpathName, ExtDir *dirh, int16 len, char* buff, uint32 xattrp, uint32 xretp );
	int32 DrewinddirExtFs( ExtDir *dirh );

	int32 FxattrExtFs( LogicalDev *ldp, char *pathName, ExtFile *fp,
					   int16 flag, const char* pn, uint32 xattrp );
	int32 Fxattr_( LogicalDev *ldp, char *fpathName, int16 flag, uint32 xattrp );   // Taking host pathName instead of Atari one.

	// these variables are another plain hack.
	// They should be in each mounted instance just like basePath and caseSensitive flag (which are not either now)
	uint32 mint_fs_drv;    /* FILESYS */
	uint32 mint_fs_devdrv; /* DEVDRV  */
	uint16 mint_fs_devnum; /* device number */

	void dispatchXFS( uint32 fncode, M68kRegisters *r );

	void fetchXFSC( XfsCookie *fc, uint32 filep );
	void flushXFSC( XfsCookie *fc, uint32 filep );
	void fetchXFSF( ExtFile *extFile, uint32 filep );
	void flushXFSF( ExtFile *extFile, uint32 filep );
	void fetchXFSD( XfsDir *dirh, uint32 dirp );
	void flushXFSD( XfsDir *dirh, uint32 dirp );

	char *cookie2Pathname( XfsFsFile *fs, const char *name, char *buf );
	void xfs_freefs( XfsFsFile *fs );

	int32 xfs_root( uint16 dev, XfsCookie *fc );
	int32 xfs_dupcookie( XfsCookie *newCook, XfsCookie *oldCook );
	int32 xfs_release( XfsCookie *fc );
	int32 xfs_getxattr( XfsCookie *fc, uint32 xattrp );
	int32 xfs_getdev( XfsCookie *fc, int32 *devspecial );
	int32 xfs_lookup( XfsCookie *dir, char *name, XfsCookie *fc );
	int32 xfs_getname( XfsCookie *relto, XfsCookie *dir, char *pathName, int16 size );
	int32 xfs_creat( XfsCookie *dir, char *name, uint16 mode, int16 flags, XfsCookie *fc );
	int32 xfs_rename( XfsCookie *olddir, char *oldname, XfsCookie *newdir, char *newname );
	int32 xfs_remove( XfsCookie *dir, char *name );
	int32 xfs_pathconf( XfsCookie *fc, int16 which );
	int32 xfs_opendir( XfsDir *dirh, uint16 flags );
	int32 xfs_readdir( XfsDir *dirh, char* buff, int16 len, XfsCookie *fc );
	int32 xfs_mkdir( XfsCookie *dir, char *name, uint16 mode );
	int32 xfs_rmdir( XfsCookie *dir, char *name );
	int32 xfs_readlink( XfsCookie *dir, char *buf, int16 len );
	int32 xfs_dfree( XfsCookie *dir, uint32 buf );

	int32 xfs_dev_open(ExtFile *fp);
	int32 xfs_dev_datime( ExtFile *fp, uint32 *datetimep, int16 wflag);

};

#endif /* EXTFS_SUPPORT */

#endif


/*
 * $Log$
 * Revision 1.18  2002/04/19 16:23:24  standa
 * The Fxattr bug fixed. QED works, Thing can refresh without the JOY's ugly
 * patch in Dpathconf.
 *
 * Revision 1.17  2002/04/19 14:21:04  standa
 * Patrice's FreeMiNT compilation patch adjusted by ExtFs suffixes.
 *
 * Revision 1.15  2002/04/17 15:44:17  standa
 * Patrice Mandin <pmandin@caramail.com> & STanda FreeMiNT compilation support
 * patch.
 *
 * Revision 1.14  2002/04/12 22:52:27  joy
 * AranymFS bug fixed - ST-Zip can unpack onto host fs now
 *
 * Revision 1.13  2002/03/07 08:06:08  standa
 * Addition to the last commit of extfs.cpp
 *
 * Revision 1.12  2002/01/31 23:51:22  standa
 * The aranym.xfs for MiNT. Preliminary version.
 *
 * Revision 1.11  2002/01/26 21:22:24  standa
 * Cleanup from no needed method arguments.
 *
 * Revision 1.10  2002/01/08 18:33:49  standa
 * The size of the bx_options.aranymfs[] and ExtFs::drives[] fixed.
 *
 * Revision 1.9  2001/12/04 09:32:18  standa
 * Olivier Landemarre <Olivier.Landemarre@utbm.fr>: Frename patch.
 *
 * Revision 1.8  2001/11/21 13:29:51  milan
 * cleanning & portability
 *
 * Revision 1.7  2001/11/20 23:29:26  milan
 * Extfs now not needed for ARAnyM's compilation
 *
 * Revision 1.6  2001/09/18 12:37:16  joy
 * getDrvBits() added
 *
 * Revision 1.5  2001/06/21 20:16:53  standa
 * Dgetdrv(), Dsetdrv(), Dgetpath(), Dsetpath() propagation added.
 * Only Dsetpath() ever noticed to be propagated by MetaDOS.
 * BetaDOS tested -> the same story.
 *
 * Revision 1.4  2001/06/18 13:21:55  standa
 * Several template.cpp like comments were added.
 * HostScreen SDL encapsulation class.
 *
 *
 */
