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

    typedef struct
    {
		int16	dummy;
    } LogicalDev;            // Dummy structure... I don't know the meaning in COOK_FS

    typedef struct
    {
		int16   index;
		int16   mode;
		int16   flags;
		int32   hostfh;
		int32   offset;
		int16   device;
    } ExtFile;               // See MYFILE in Julian's COOK_FS

    typedef struct           /* used by Fsetdta, Fgetdta */
    {
		uint64  ds_dev;
		uint8 	ds_index;      // index in the fs_pathName array (seems like a hack, but I don't know better)
		DIR     *ds_dirh;      // opendir resulting handle to perform dirscan
		uint16 	ds_attrib;     // search attribs wanted
		int8    ds_name[14];

		// And now GEMDOS specified fields
		uint8   d_attrib;
		uint16  d_time;
		uint16  d_date;
		uint32  d_length;
		int8    d_fname[14];
    } ExtDta;                // See myDTA in Julian's COOK_FS

	typedef struct
	{
		bool halfSensitive;
		char *rootPath;
		char *currPath;      // Only Dsetpath uses this in the .DOS driver -> can be removed
    } ExtDrive;

    ExtDrive drives[ 'Z'-'A'+1 ];

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
	int    st2mode( uint16 mode );
	int16  mode2st( int flags );

	/**
	 * Path conversions.
	 *
	 * Note: This is the most sophisticated thing in this object.
	 **/
	ExtDrive* getDrive( const char* pathName );
	void transformFileName( char* dest, const char* source );
	bool getHostFileName( char* result, ExtDrive* drv, char* path, char* name );
	void convertPathA2F( char* fpathName, char* pathName, char* basePath = NULL );

	int16  getFreeDirIndex( char **pathNames );
	void   freeDirIndex( uint8 index, char **pathNames );
	bool   filterFiles( ExtDta *dta, char *fpathName, char *mask, struct dirent *dirEntry );
	int32  findFirst( ExtDta *dta, char *fpathName );
	uint32 fileOpen( const char* pathName, int flags, int mode, ExtFile *fp );

	// GEMDOS functions
	int32 Dfree(LogicalDev *ldp, char *pathName, ExtFile *fp,
				uint32 diskinfop, int16 drive );
	int32 Dcreate(LogicalDev *ldp, char *pathName, ExtFile *fp,
				  const char *pn);
	int32 Ddelete(LogicalDev *ldp, char *pathName, ExtFile *fp,
				  const char *pn);
	int32 Dsetpath(LogicalDev *ldp, char *pathName, ExtFile *fp,
				   const char *pn);
	int32 Fcreate(LogicalDev *ldp, char *pathName, ExtFile *fp,
				  const char *pn, int16 attr);
	int32 Fopen(LogicalDev *ldp, char *pathName, ExtFile *fp,
				const char *pn, int16 mode);
	int32 Fclose(LogicalDev *ldp, char *pathName, ExtFile *fp,
				 int16 handle);
	int32 Fread(LogicalDev *ldp, char *pathName, ExtFile *fp,
				int16 handle, uint32 count, void *buffer);
	int32 Fwrite(LogicalDev *ldp, char *pathName, ExtFile *fp,
				 int16 handle, uint32 count, void *buffer);
	int32 Fdelete(LogicalDev *ldp, char *pathName, ExtFile *fp,
				  const char *pn);
	int32 Fseek(LogicalDev *ldp, char *pathName, ExtFile *fp,
				int32 offset, int16 handle, int16 seekmode);

	int32 Fattrib(LogicalDev *ldp, char *pathName, ExtFile *fp,
				  const char* pn, int16 wflag, int16 attr);

	int32 Fsfirst( LogicalDev *ldp, char *pathname, ExtDta *dta,
				   const char *pn, int16 attribs );
	int32 Fsnext ( LogicalDev *ldp, char *pathName, ExtDta *dta );
	int32 Frename(LogicalDev *ldp, char *pathName, ExtFile *fp,
				  int16 reserved, char *oldpath, char *newPathName);
	int32 Fdatime( LogicalDev *ldp, char *pathName, ExtFile *fp,
				   uint32 *datetimep, int16 handle, int16 wflag);
	int32 Fcntl( LogicalDev *ldp, char *pathName, ExtFile *fp,
				 int16 handle, void *arg, int16 cmd );
	int32 Dpathconf( LogicalDev *ldp, char *pathName, ExtFile *fp,
					 const char* pn, int16 cmd);
	int32 Dopendir( LogicalDev *ldp, char *pathName, ExtFile *fp,
					const char* pn, int16 flag );
	int32 Dclosedir( LogicalDev *ldp, char *pathName, ExtFile *fp,
					 int32 dirhandle );
	int32 Dreaddir( LogicalDev *ldp, char *pathName, ExtFile *fp,
					int16 len, int32 dirhandle, char* buff );
	int32 Drewinddir( LogicalDev *ldp, char *pathName, ExtFile *fp,
					  int32 dirhandle );
	int32 Fxattr( LogicalDev *ldp, char *pathName, ExtDta *dta,
				  int16 flag, const char* pn, uint32 xattrp );
	int32 Fxattr_( LogicalDev *ldp, char *fpathName, ExtDta *dta,
				   int16 flag, const char* pn, uint32 xattrp );   // Taking host pathName instead of Atari one.
	int32 Dxreaddir( LogicalDev *ldp, char *pathName, ExtFile *fp,
					 int16 len, int32 dirhandle, char* buff, uint32 xattrp, uint32 xretp );
};

#endif /* EXTFS_SUPPORT */

#endif


/*
 * $Log$
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
