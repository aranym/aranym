/*
 * $Header$
 *
 * STanda 2001
 */

#include "sysdeps.h"
#include "cpu_emulation.h"
#include "main.h"

#include <sys/stat.h>
#include <sys/vfs.h>

#include <utime.h>

#ifdef EXTFS_SUPPORT

#include "parameters.h"
#include "toserror.h"
#include "extfs.h"
#include "araobjs.h"

#define DEBUG 0
#include "debug.h"


extern "C" {

	char* strapply( char* str, int (*functor)(int) )
	{
		char* pos = str;
		while ( (*pos = (char)functor( (int)*pos )) != 0 )
			pos++;

		return str;
	}

	char* strd2upath( char* dest, char* src )
	{
		char* result = dest;
		while( *src ) {
			*dest++ = (*src == '\\' ? '/' : *src);
			src++;
		}
		*dest=0;

		return result;
	}
}



void ExtFs::init()
{
	// go through the drive table and assign drives their rootPaths
	for( char i = 'B'-'A'; i < 'Z'-'A'+1; i++ ) {
		// D(bug("MetaDOS: init %c:%s", i + 'A', bx_options.aranymfs[i].rootPath ? bx_options.aranymfs[i].rootPath : "null"));

		if ( bx_options.aranymfs[i].rootPath[0] != '\0' )
			install( i+'A', bx_options.aranymfs[i].rootPath, bx_options.aranymfs[i].halfSensitive );
	}
}

uint32 ExtFs::getDrvBits() {
	uint32 drvBits = 0;
	for(int i='B'-'A'; i<'Z'-'A'+1; i++)
		if (drives[i].rootPath != NULL)
			drvBits |= (1 << i);

	return drvBits;
}


void ExtFs::install( const char driveSign, const char* rootPath, bool halfSensitive )
{
	int8 driveNo = toupper(driveSign) - 'A';

	if ( rootPath != NULL ) {
		drives[ driveNo ].rootPath = strdup( (char*)rootPath );
		drives[ driveNo ].currPath = NULL;
		drives[ driveNo ].halfSensitive = halfSensitive;

		D(bug("MetaDOS: installing %c:%s:%s",
				  driveNo + 'A',
				  rootPath ? rootPath : "null",
				  halfSensitive ? "halfSensitive" : "full"));
	}
}


void ExtFs::dispatch( uint32 fncode, M68kRegisters *r )
{
	LogicalDev ldp;
	ExtDta	   dta;
	ExtFile	   extFile;
	char	   pathname[ MAXPATHNAMELEN ];

	// fix the stack (the fncode was pushed onto the stack)
	r->a[7] += 4;

	D(bug("MetaDOS: %2d ", fncode));

	switch (fncode) {
		case 54:	// Dfree:
			D(bug("%s", "DFree"));
			fetchFILE( &extFile, r->a[5] );
			a2fstrcpy( (char*)pathname, (uint8*)r->a[4] );
			r->d[0] = Dfree( &ldp, (char*)pathname, &extFile,
							 get_long( r->a[7] + 6, true ),			   // diskinfop
							 (int16) get_word( r->a[7] + 10, true ) ); // drive
			flushFILE( &extFile, r->a[5] );
			break;
		case 57:	// Dcreate:
			{
				char pn[MAXPATHNAMELEN];

				D(bug("%s", "Dcreate"));
				fetchFILE( &extFile, r->a[5] );
				a2fstrcpy( (char*)pathname, (uint8*)r->a[4] );
				a2fstrcpy( (char*)pn, (uint8*)get_long( r->a[7] + 6, true ) );
				r->d[0] = Dcreate( &ldp, (char*)pathname, &extFile,
								   (const char*)pn );			   // pathname
				flushFILE( &extFile, r->a[5] );
			}
			break;
		case 58:	// Ddelete:
			{
				char pn[MAXPATHNAMELEN];

				D(bug("%s", "Ddelete"));
				fetchFILE( &extFile, r->a[5] );
				a2fstrcpy( (char*)pathname, (uint8*)r->a[4] );
				a2fstrcpy( (char*)pn, (uint8*)get_long( r->a[7] + 6, true ) );
				r->d[0] = Ddelete( &ldp, (char*)pathname, &extFile,
								   (const char*)pn );			   // pathname
				flushFILE( &extFile, r->a[5] );
			}
			break;
		case 59:
			{
				char pn[MAXPATHNAMELEN];

				D(bug("%s", "Dsetpath"));
				fetchFILE( &extFile, r->a[5] );
				a2fstrcpy( (char*)pathname, (uint8*)r->a[4] );
				a2fstrcpy( (char*)pn, (uint8*)get_long( r->a[7] + 6, true ) );
				r->d[0] = Dsetpath( &ldp, (char*)pathname, &extFile,
									(const char*)pn );				// pathname
				flushFILE( &extFile, r->a[5] );
				D(bug("MetaDOS: %s: %d", "/Dsetpath", (int32)r->d[0]));
			}
			break;
		case 60:	// Fcreate:
			{
				char pn[MAXPATHNAMELEN];

				D(bug("%s", "Fcreate"));
				fetchFILE( &extFile, r->a[5] );
				a2fstrcpy( (char*)pathname, (uint8*)r->a[4] );
				a2fstrcpy( (char*)pn, (uint8*)get_long( r->a[7] + 6, true ) );
				r->d[0] = Fcreate( &ldp, (char*)pathname, &extFile,
								   (const char*)pn,					   // pathname
								   (int16) get_word( r->a[7] + 10, true ) ); // mode
				flushFILE( &extFile, r->a[5] );
				D(bug("%s: %d", "/Fcreate", (int32)r->d[0]));
			}
			break;
		case 61:	// Fopen:
			{
				char pn[MAXPATHNAMELEN];

				D(bug("%s", "Fopen"));
				fetchFILE( &extFile, r->a[5] );
				a2fstrcpy( (char*)pathname, (uint8*)r->a[4] );
				a2fstrcpy( (char*)pn, (uint8*)get_long( r->a[7] + 6, true ) );
				r->d[0] = Fopen( &ldp, (char*)pathname, &extFile,
								 (const char*)pn,					 // pathname
								 (int16) get_word( r->a[7] + 10, true ) ); // mode
				flushFILE( &extFile, r->a[5] );
				D(bug("MetaDOS: %s: %d", "/Fopen", (int32)r->d[0]));
			}
			break;
		case 62:	// Fclose:
			D(bug("%s", "Fclose"));
			fetchFILE( &extFile, r->a[5] );
			a2fstrcpy( (char*)pathname, (uint8*)r->a[4] );
			r->d[0] = Fclose( &ldp, (char*)pathname, &extFile,
							  (int16) get_word( r->a[7] + 6, true ) ); // handle
			flushFILE( &extFile, r->a[5] );
			break;
		case 63:	// Fread:
			D(bug("%s", "Fread"));
			fetchFILE( &extFile, r->a[5] );
			a2fstrcpy( (char*)pathname, (uint8*)r->a[4] );
			r->d[0] = Fread( &ldp, (char*)pathname, &extFile,
							 (int16) get_word( r->a[7] + 6, true ),	 // handle
							 get_long( r->a[7] + 8, true ),			 // count
							 (void*)get_long( r->a[7] + 12, true) ); // buffer
			flushFILE( &extFile, r->a[5] );
			break;
		case 64:	// Fwrite:
			D(bug("%s", "Fwrite"));
			fetchFILE( &extFile, r->a[5] );
			a2fstrcpy( (char*)pathname, (uint8*)r->a[4] );
			r->d[0] = Fwrite( &ldp, (char*)pathname, &extFile,
							  (int16) get_word( r->a[7] + 6, true ),  // handle
							  get_long( r->a[7] + 8, true ),		  // count
							  (void*)get_long( r->a[7] + 12, true ) ); // buffer
			flushFILE( &extFile, r->a[5] );
			break;
		case 65:	// Fdelete:
			{
				char pn[MAXPATHNAMELEN];

				D(bug("%s", "Fdelete"));
				fetchFILE( &extFile, r->a[5] );
				a2fstrcpy( (char*)pathname, (uint8*)r->a[4] );
				a2fstrcpy( (char*)pn, (uint8*)get_long( r->a[7] + 6, true ) );
				r->d[0] = Fdelete( &ldp, (char*)pathname, &extFile,
								   (const char*)pn );				  // pathname
				flushFILE( &extFile, r->a[5] );
				D(bug("MetaDOS: %s: %d", "/Fdelete", (int32)r->d[0]));
			}
			break;
		case 66:	// Fseek:
			D(bug("%s", "Fseek"));
			fetchFILE( &extFile, r->a[5] );
			a2fstrcpy( (char*)pathname, (uint8*)r->a[4] );
			r->d[0] = Fseek( &ldp, (char*)pathname, &extFile,
							 get_long( r->a[7] + 6, true ),			 // offset
							 (int16) get_word( r->a[7] + 10, true ),  // handle
							 (int16) get_word( r->a[7] + 12, true ) );// seekmode
			flushFILE( &extFile, r->a[5] );
			break;
		case 67:	// Fattrib:
			{
				char pn[MAXPATHNAMELEN];

				D(bug("%s", "Fattrib"));
				fetchFILE( &extFile, r->a[5] );
				a2fstrcpy( (char*)pathname, (uint8*)r->a[4] );
				a2fstrcpy( (char*)pn, (uint8*)get_long( r->a[7] + 6, true ) );
				r->d[0] = Fattrib( &ldp, (char*)pathname, &extFile,
								   (const char*)pn,					 // pathname
								   (int16) get_word( r->a[7] + 10, true ),	// wflag
								   (int16) get_word( r->a[7] + 12, true ) );// attr
				flushFILE( &extFile, r->a[5] );
			}
			break;
		case 78:	// Fsfirst:
			{
				char pn[MAXPATHNAMELEN];

				D(bug("%s", "Fsfirst"));
				fetchDTA( &dta, r->a[5] );
				a2fstrcpy( (char*)pathname, (uint8*)r->a[4] );
				a2fstrcpy( (char*)pn, (uint8*)get_long( r->a[7] + 6, true ) );
				r->d[0] = Fsfirst( &ldp, (char*)pathname, &dta,
								   (const char*)pn,
								   (int16) get_word( r->a[7] + 10, true ) );
				flushDTA( &dta, r->a[5] );
				D(bug("MetaDOS: %s", "/Fsfirst"));
			}
			break;
		case 79:	// Fsnext:
			D(bug("%s", "Fsnext"));
			fetchDTA( &dta, r->a[5] );
			a2fstrcpy( (char*)pathname, (uint8*)r->a[4] );
			r->d[0] = Fsnext( &ldp, (char*)pathname, &dta );
			flushDTA( &dta, r->a[5] );
			D(bug("MetaDOS: %s", "/Fsnext"));
			break;
		case 86:	// Frename
			{
				char pn[MAXPATHNAMELEN];
				char npn[MAXPATHNAMELEN];

				D(bug("%s", "Frename"));
				fetchFILE( &extFile, r->a[5] );
				a2fstrcpy( (char*)pathname, (uint8*)r->a[4] );
				a2fstrcpy( (char*)pn, (uint8*)get_long( r->a[7] + 8, true ) );
				a2fstrcpy( (char*)npn, (uint8*)get_long( r->a[7] + 12, true ) );
				r->d[0] = Frename( &ldp, (char*)pathname, &extFile,
								   (int16) get_word( r->a[7] + 6, true ), // reserved
								   pn,				          // oldpathname
								   npn );		              // newpathname
				flushFILE( &extFile, r->a[5] );
			}
			break;
		case 87:	// Fdatime:
			D(bug("%s", "Fdatime"));
			fetchFILE( &extFile, r->a[5] );
			a2fstrcpy( (char*)pathname, (uint8*)r->a[4] );
			r->d[0] = Fdatime( &ldp, (char*)pathname, &extFile,
							   (uint32*)get_long( r->a[7] + 6, true ), // datetimep
							   (int16) get_word( r->a[7] + 10, true ),	// handle
							   (int16) get_word( r->a[7] + 12, true ) );// wflag
			flushFILE( &extFile, r->a[5] );
			break;
		case 260:	// Fcntl:
			D(bug("%s", "Fcntl"));
			fetchFILE( &extFile, r->a[5] );
			a2fstrcpy( (char*)pathname, (uint8*)r->a[4] );
			r->d[0] = Fcntl( &ldp, (char*)pathname, &extFile,
							 (int16) get_word( r->a[7] + 6, true ),	  // handle
							 (void*)get_long( r->a[7] + 8, true ),	  // arg
							 (int16) get_word( r->a[7] + 12, true ) ); // cmd
			flushFILE( &extFile, r->a[5] );
			break;
		case 292:	// Dpathconf:
			{
				char pn[MAXPATHNAMELEN];

				D(bug("%s", "Dpathconf"));
				fetchFILE( &extFile, r->a[5] );
				a2fstrcpy( (char*)pathname, (uint8*)r->a[4] );
				a2fstrcpy( (char*)pn, (uint8*)get_long( r->a[7] + 6, true ) );
				r->d[0] = Dpathconf( &ldp, (char*)pathname, &extFile,
									 (const char*)pn,					 // pathname
									 (int16) get_word( r->a[7] + 10, true ) ); // cmd
				flushFILE( &extFile, r->a[5] );
				D(bug("MetaDOS: /Dpathconf res = %#8x (%d)", r->d[0],(int32)r->d[0]));
			}
			break;
		case 296:	// Dopendir:
			{
				char pn[MAXPATHNAMELEN];

				D(bug("%s", "Dopendir"));
				fetchFILE( &extFile, r->a[5] );
				a2fstrcpy( (char*)pathname, (uint8*)r->a[4] );
				a2fstrcpy( (char*)pn, (uint8*)get_long( r->a[7] + 6, true ) );
				r->d[0] = Dopendir( &ldp, (char*)pathname, &extFile,
									(const char*)pn,					// pathname
									(int16) get_word( r->a[7] + 10, true ) ); // flag
				flushFILE( &extFile, r->a[5] );
				D(bug("MetaDOS: /Dopendir res = %#08x (%d)", r->d[0],(int32)r->d[0]));
			}
			break;
		case 297:	// Dreaddir:
			D(bug("%s", "Dreaddir"));
			fetchFILE( &extFile, r->a[5] );
			a2fstrcpy( (char*)pathname, (uint8*)r->a[4] );
			r->d[0] = Dreaddir( &ldp, (char*)pathname, &extFile,
								(int16) get_word( r->a[7] + 6, true ),	 // len
								get_long( r->a[7] + 8, true ),			 // dirhandle
								(char*)get_long( r->a[7] + 12, true ) ); // bufferp
			flushFILE( &extFile, r->a[5] );
			break;
		case 298:	// Drewinddir:
			D(bug("%s", "Drewinddir"));
			fetchFILE( &extFile, r->a[5] );
			a2fstrcpy( (char*)pathname, (uint8*)r->a[4] );
			r->d[0] = Drewinddir( &ldp, (char*)pathname, &extFile,
								  get_long( r->a[7] + 6, true ) );		 // dirhandle
			flushFILE( &extFile, r->a[5] );
			break;
		case 299:	// Dclosedir:
			D(bug("%s", "Dclosedir"));
			fetchFILE( &extFile, r->a[5] );
			a2fstrcpy( (char*)pathname, (uint8*)r->a[4] );
			r->d[0] = Dclosedir( &ldp, (char*)pathname, &extFile,
								 get_long( r->a[7] + 6, true ) );		// dirhandle
			flushFILE( &extFile, r->a[5] );
			break;
		case 300:	// Fxattr:
			{
				char pn[MAXPATHNAMELEN];

				D(bug("%s", "Fxattr"));
				fetchDTA( &dta, r->a[5] );
				a2fstrcpy( (char*)pathname, (uint8*)r->a[4] );
				a2fstrcpy( (char*)pn, (uint8*)get_long( r->a[7] + 8, true ) );
				r->d[0] = Fxattr( &ldp, (char*)pathname, &dta,
								  (int16) get_word( r->a[7] + 6, true ), // flag
								  (const char*)pn,				   // pathname
								  get_long( r->a[7] + 12, true ) );		 // XATTR*
				flushDTA( &dta, r->a[5] );
				D(bug("MetaDOS: /Fxattr res = %#08x (%d)", r->d[0],(int32)r->d[0]));
			}
			break;
		case 322:	// Dxreaddir:
			D(bug("%s", "Dxreaddir"));
			fetchFILE( &extFile, r->a[5] );
			a2fstrcpy( (char*)pathname, (uint8*)r->a[4] );
			r->d[0] = Dxreaddir( &ldp, (char*)pathname, &extFile,
								 (int16) get_word( r->a[7] + 6, true ),	  // len
								 get_long( r->a[7] + 8, true ),			  // dirhandle
								 (char*)get_long( r->a[7] + 12, true ),	  // bufferp
								 get_long( r->a[7] + 16, true ),		  // XATTR*
								 get_long( r->a[7] + 20, true ) );		  // xret
			break;
		case 338:	// Dreadlabel:
			D(bug("%s", "Dreadlabel"));
			r->d[0] = (uint32)TOS_EINVFN;
			break;
		default:
			D(bug("Unknown"));
			r->d[0] = (uint32)TOS_EINVFN;
	}
}


void ExtFs::a2fmemcpy( uint8 *dest, uint8 *source, size_t count )
{
	while ( count-- )
		*dest++ = get_byte( (uint32)source++, true );
}

void ExtFs::a2fstrcpy( char *dest, uint8 *source )
{
	while ( (*dest++ = get_byte( (uint32)source++, true )) != 0 );
}


void ExtFs::f2amemcpy( uint8 *dest, uint8 *source, size_t count )
{
	while ( count-- )
		put_byte( (uint32)dest++, *source++ );
}

void ExtFs::f2astrcpy( uint8 *dest, uint8 *source )
{
	while ( *source )
		put_byte( (uint32)dest++, *source++ );
	put_byte( (uint32)dest++, 0 );
}


void ExtFs::fetchDTA( ExtDta *dta, uint32 dtap )
{
	// Normally reserved ... used in the CookFS
	dta->ds_dirh	 = (DIR*)get_long( dtap, true );
	dta->ds_attrib	 = get_word( dtap + 4, true );
	dta->ds_index	 = get_byte( dtap + 6, true );
	//	dta->ds_ast		 = get_byte( dtap + 6, true );
	a2fmemcpy( (uint8*)dta->ds_name, (uint8*)(dtap + 7), 14 );

	// Common FS
	dta->d_attrib = get_byte( dtap + 21, true );
	dta->d_time	  = get_word( dtap + 22, true );
	dta->d_date	  = get_word( dtap + 24, true );
	dta->d_length = get_long( dtap + 26, true );
	a2fmemcpy( (uint8*)dta->d_fname, (uint8*)(dtap + 30), 14 );
}


void ExtFs::flushDTA( ExtDta *dta, uint32 dtap )
{
	// Normally reserved ... used in the CookFS
	put_long( dtap	  , (uint32)dta->ds_dirh );
	put_word( dtap + 4, dta->ds_attrib );
	put_byte( dtap + 6, dta->ds_index );
	//	put_byte( dtap + 6, dta->ds_ast );
	f2amemcpy( (uint8*)(dtap + 7), (uint8*)dta->ds_name, 14 );

	// Common FS
	put_byte( dtap + 21, dta->d_attrib );
	put_word( dtap + 22, dta->d_time );
	put_word( dtap + 24, dta->d_date );
	put_long( dtap + 26, dta->d_length );
	f2amemcpy( (uint8*)(dtap + 30), (uint8*)dta->d_fname, 14 );
}


void ExtFs::fetchFILE( ExtFile *extFile, uint32 filep )
{
	extFile->index	= get_word( filep,		 true );
	extFile->mode	= get_word( filep + 2,	 true );
	extFile->flags	= get_word( filep + 4,	 true ); // never used in the code
	extFile->hostfh = get_long( filep + 6,	 true );
	extFile->offset = get_long( filep + 10, true );  // probably not used
	extFile->device = get_word( filep + 14, true );  // never used
}

void ExtFs::flushFILE( ExtFile *extFile, uint32 filep )
{
	put_word( filep	   , extFile->index );
	put_word( filep + 2, extFile->mode	);
	put_word( filep + 4, extFile->flags );
	put_long( filep + 6, extFile->hostfh );
	put_long( filep +10, extFile->offset );
	put_word( filep +14, extFile->device );
}


uint32 ExtFs::unix2toserrno(int unixerrno,int defaulttoserrno)
{
	int retval = defaulttoserrno;

	D(bug("MetaDOS: unix2toserrno (%d,%d)", unixerrno, defaulttoserrno));

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
	return retval;
}


uint16 ExtFs::time2dos(time_t t)
{
	struct tm *x;
	x = localtime (&t);
	return (x->tm_sec>>1)|(x->tm_min<<5)|(x->tm_hour<<11);
}


uint16 ExtFs::date2dos(time_t t)
{
	struct tm *x;
	x = localtime (&t);
	return x->tm_mday|((x->tm_mon+1)<<5)|(MAX(x->tm_year-80,0)<<9);
}


void ExtFs::datetime2tm(uint32 dtm, struct tm* ttm)
{
	ttm->tm_mday = dtm & 0x1f;
	ttm->tm_mon	 = ((dtm>>5) & 0x0f) - 1;
	ttm->tm_year = ((dtm>>9) & 0x7f) + 80;
	ttm->tm_sec	 = ((dtm>>16) & 0x1f) << 1;
	ttm->tm_min	 = (dtm>>21) & 0x3f;
	ttm->tm_hour = (dtm>>27) & 0x1f;
}


uint16 ExtFs::statmode2attr(mode_t m)
{
	return ( S_ISDIR(m) ) ? 0x10 : 0;	/* FIXME */
	//	  if (!(da == 0 || ((da != 0) && (attribs != 8)) || ((attribs | 0x21) & da)))
}


uint16 ExtFs::statmode2xattrmode(mode_t m)
{
	uint16 result = 0;

	if ( S_ISREG(m) ) result = 0x8000;
	if ( S_ISDIR(m) ) result = 0x4000;
	if ( S_ISCHR(m) ) result = 0x2000;
	if ( S_ISFIFO(m)) result = 0xa000;
	if ( S_ISBLK(m) ) result = 0xc000;  // S_IMEM in COMPEND.HYP ... FIXME!!
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

	D(bug("statmode: %04x)", result));

	return result;
}


int ExtFs::st2mode(uint16 mode)
{
	switch(mode & 0x3)
		{
			case 0: return O_RDONLY;
			case 1: return O_WRONLY;	/* kludge to avoid files being created */
			case 2: return O_RDWR;
		}
	return 0;
}


int16 ExtFs::mode2st(int flags)
{
	int16 res = 0;

	if (flags | O_WRONLY) res |= 1;
	if (flags | O_RDWR) res |= 2;

	return res;
}

void ExtFs::transformFileName( char* dest, const char* source )
{
	ssize_t len = strlen( source );
	ssize_t dotPos = 0;
	bool	doConversion = true;
	char	*dot;

	// Get extension & convert other dots into underscores
	if ( ( dot = strrchr( source, '.' ) ) != NULL ) {
		dotPos = dot - source;

		if ( *source == '.' )
			if ( len == 1 ||
				 ( len == 2 && source[1] == '.' ) ) {
				doConversion = false;
				dot = NULL;
			} else if ( dotPos == 0 )
				dot = NULL;
	}

	if ( dot == NULL )
		dotPos = len;

	strncpy( dest, source, dotPos );
	dest[dotPos] = '\0';

	if ( !doConversion )	// . and .. system folders
		return;

	// if the filename is longer than 8+3 or if the extension is longer than 3
	if ( len > ( dot == NULL ? 11 : 12 ) || len - dotPos - 1 > 3 ) {
		// calculate a hash value from the long name
		uint32 hashValue = 0;
		for( int i=0; source[i] != '\0'; i++ )
			hashValue += (hashValue << 3) + source[i];

		// hash value hex string as the unique shortenning
		char   tmpStr[10];
		char   destIdx;
		sprintf( tmpStr, "%08x", hashValue );

		// put the value into the name or into extension
		if ( dot == NULL )
			destIdx = 8;
		else
			destIdx = MIN(5,dotPos);

		dest[destIdx] = '~';
		memcpy( &dest[destIdx+1], &tmpStr[6], 3 ); // including the trailing \0!

		dotPos = destIdx + 3;
	}

	// if the file have no . and is 12 char long than shorten it and add the .
	if ( dot != NULL ) {
		strncpy( dest+dotPos, dot, 4 );
		dest[dotPos+4] = '\0';
	} else if ( len > 8 ) {
		memmove( dest+9, dest+8, 4 );
		dotPos = 8;
		dot = dest+dotPos;
	}

	// replace spaces and dots in the filename with the _
	char *temp = dest;
	char *brkPos;
	while ( (brkPos = strpbrk( temp, " ." )) != NULL ) {
		*brkPos = '_';
		temp = brkPos + 1;
	}
	if ( dot != NULL )
		// set the extensi	on separator
		dest[ dotPos ] = '.';

#ifdef DEBUG_FILENAMETRANSFORMATION
	D(bug("MetaDOS: transformFileName (%s, len = %d)", dest, len));
#endif

	// upper case conversion
	strapply( dest, toupper );
}


bool ExtFs::getHostFileName( char* result, ExtDrive* drv, char* pathName, char* name )
{
	struct stat statBuf;

#ifdef DEBUG_FILENAMETRANSFORMATION
	D(bug("MetaDOS: getHostFileName (%s,%s)", pathName, name));
#endif

	// if the whole thing fails then take the requested name as is
	// it also completes the path
	strcpy( result, name );

	if ( ! strpbrk( name, "*?" ) &&	// if is it NOT a mask
		 stat(pathName, &statBuf) )	// or if such file NOT really exists
		{
			// the TOS filename was adjusted (lettercase, length, ..)
			char testName[MAXPATHNAMELEN];
			char *finalName = name;
			struct dirent *dirEntry;

#ifdef DEBUG_FILENAMETRANSFORMATION
			D(bug(" (stat failed)"));
#endif

			// shorten the name from the pathName;
			*result = '\0';

			DIR *dh = opendir( pathName );
			if ( dh == NULL )
				goto lbl_final;  // should never happen

			while ( true ) {
				if ((dirEntry = readdir( dh )) == NULL)
					goto lbl_final;

				if ( drv->halfSensitive )
					if ( ! strcasecmp( name, dirEntry->d_name ) ) {
						finalName = dirEntry->d_name;
						goto lbl_final;
					}

				transformFileName( testName, dirEntry->d_name );

#ifdef DEBUG_FILENAMETRANSFORMATION
				D(bug("MetaDOS: getHostFileName (%s,%s,%s)", name, testName, dirEntry->d_name));
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
			D(bug("MetaDOS: getHostFileName final (%s,%s)", name, finalName));
#endif

			strcpy( result, finalName );
			if ( dh != NULL )
				closedir( dh );
		}

#ifdef DEBUG_FILENAMETRANSFORMATION
	else
		D(bug(" (stat OK)"));
#endif

	return true;
}


ExtFs::ExtDrive* ExtFs::getDrive( const char* pathName )
{
	ExtDrive *drv = NULL;

	if ( pathName[0] != '\0' && pathName[1] == ':' )
		drv = &drives[ toupper( pathName[0] ) - 'A' ];

	if ( drv == NULL || drv->rootPath == NULL )
		drv = &drives[0];

	return drv;
}


void ExtFs::convertPathA2F( char* fpathName, char* pathName, char* basePath )
{
	char		  *n, *tp, *ffileName;
	struct stat statBuf;
	ExtDrive	  *drv = getDrive( pathName );

	// the default is the drive root dir
	if ( basePath == NULL )
		basePath = drv->rootPath;

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
	while ( (tp = strchr( n, '\\' )) != NULL ) {
		*tp = '\0';
		getHostFileName( ffileName, drv, fpathName, n );
		ffileName += strlen( ffileName );
		strcpy( ffileName, "/" ); ffileName++;
		*tp = '\\';
		n = tp+1;
	}
	getHostFileName( ffileName, drv, fpathName, n );
}


int32 ExtFs::Dfree(LogicalDev *ldp, char *pathName, ExtFile *fp, uint32 diskinfop, int16 drive )
{
	char fpathName[MAXPATHNAMELEN];
#ifdef HAVE_SYS_STATVFS_H
	struct statvfs buff;
#else
	struct statfs buff;
#endif

	convertPathA2F( fpathName, pathName );

	D(bug("MetaDOS: Dfree (drive = %d,%s,%s)", 'A' + drive, pathName, fpathName));

#ifdef HAVE_SYS_STATVFS_H
	if ( statvfs(fpathName, &buff) )
#else
	if ( statfs(fpathName, &buff) )
#endif
		return unix2toserrno(errno,TOS_EFILNF);

	/* ULONG b_free	   */  put_long( diskinfop	   , buff.f_bavail );
	/* ULONG b_total   */  put_long( diskinfop +  4, buff.f_blocks );
	/* ULONG b_secsize */  put_long( diskinfop +  8, buff.f_bsize /* not 512 according to stonx_fs */ );
	/* ULONG b_clsize  */  put_long( diskinfop + 12, 1 );

	return TOS_E_OK;
}


int32 ExtFs::Dcreate(LogicalDev *ldp, char *pathName, ExtFile *fp, const char *pn)
{
	char fpathName[MAXPATHNAMELEN];
	convertPathA2F( fpathName, pathName );

	D(bug("MetaDOS: Dcreate (%s,%s)", pathName, fpathName));

	if ( mkdir( (char*)fpathName,	S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH ) )
		return unix2toserrno(errno,TOS_EFILNF);

	return TOS_E_OK;
}


int32 ExtFs::Ddelete(LogicalDev *ldp, char *pathName, ExtFile *fp, const char *pn)
{
	char fpathName[MAXPATHNAMELEN];
	convertPathA2F( fpathName, pathName );

	D(bug("MetaDOS: Ddelete (%s,%s)", pathName, fpathName));

	if ( rmdir( fpathName ) )
		return unix2toserrno(errno,TOS_EFILNF);

	return TOS_E_OK;
}


int32 ExtFs::Dsetpath(LogicalDev *ldp, char *pathName, ExtFile *fp, const char *pn)
{
	char fpathName[MAXPATHNAMELEN];
	convertPathA2F( fpathName, pathName );

	D(bug("MetaDOS: Dsetpath (%s,%s)", pathName, fpathName));

	ExtDrive *drv = getDrive( pathName );
	if ( drv->currPath != NULL )
		free( drv->currPath );
	drv->currPath = strdup( pathName );

	struct stat statBuf;
	if ( stat(fpathName, &statBuf) )
		return unix2toserrno(errno,TOS_EPTHNF);

	return TOS_E_OK;
}


int32 ExtFs::Fcreate(LogicalDev *ldp, char *pathName, ExtFile *fp, const char *pn, int16 attr)
{
	char fpathName[MAXPATHNAMELEN];
	convertPathA2F( fpathName, pathName );

	D(bug("MetaDOS: Fcreate (%s,%s,%d)", pathName, fpathName, attr));

	return fileOpen( (char*)fpathName,
					 O_CREAT|O_WRONLY|O_TRUNC,
					 S_IRUSR|S_IWUSR | S_IRGRP | S_IROTH,
					 fp );
}


int32 ExtFs::Fopen(LogicalDev *ldp, char *pathName, ExtFile *fp, const char *pn, int16 mode)
{
	char fpathName[MAXPATHNAMELEN];
	convertPathA2F( fpathName, pathName );

	D(bug("MetaDOS: Fopen (%s,%s,%d)", pathName, fpathName, mode));

	fp->device	= toupper(pathName[0]) - 'A'; // FIXME

	return fileOpen( (char*)fpathName, st2mode(mode), 0, fp );
}


uint32 ExtFs::fileOpen( const char* pathName, int flags, int mode, ExtFile *fp )
{
	int fh = open( pathName, flags, mode );

	// D(bug("ret: %d", fh));

	if (fh < 0) {
		/*
		  DBG("open(): %s",strerror(errno));
		  SET_Z();
		*/
		return unix2toserrno(errno,TOS_EFILNF);
	}

	fp->mode   = mode2st( flags );
	fp->offset = 0;
	fp->hostfh = fh;

	return TOS_E_OK;
}


int32 ExtFs::Fclose(LogicalDev *ldp, char *pathName, ExtFile *fp, int16 handle)
{
	D(bug("MetaDOS: Fclose (%d)", handle));

	if ( close( fp->hostfh ) < 0 )
		return unix2toserrno(errno,TOS_EIO);

	return TOS_E_OK;
}


#define FRDWR_BUFFER_LENGTH	8192


int32 ExtFs::Fread(LogicalDev *ldp, char *pathName, ExtFile *fp, int16 handle, uint32 count, void *buffer)
{
	uint8 fBuff[ FRDWR_BUFFER_LENGTH ];
	uint8 *destBuff = (uint8*)buffer;
	ssize_t toRead = count;
	ssize_t toReadNow;
	ssize_t readCount = 0;

	D(bug("MetaDOS: Fread (%d,%d)", handle, count));

	while ( toRead > 0 ) {
		toReadNow = ( toRead > FRDWR_BUFFER_LENGTH ) ? FRDWR_BUFFER_LENGTH : toRead;
		readCount = read( fp->hostfh, fBuff, toReadNow );

		//		D(bug("MetaDOS: Fread readCount (%d)", readCount));

		if ( readCount <= 0 )
			break;

		f2amemcpy( destBuff, fBuff, readCount );
		destBuff += readCount;
		fp->offset += readCount;
		toRead -= readCount;
	}

	D(bug(" readCount (%d)", count - toRead));

	//	D(bug("MetaDOS: Fread error (%d)", errno));
	if ( readCount < 0 )
		return TOS_EINTRN;// FIXME

	return count - toRead;
}


int32 ExtFs::Fwrite(LogicalDev *ldp, char *pathName, ExtFile *fp, int16 handle, uint32 count, void *buffer)
{
	uint8 fBuff[ FRDWR_BUFFER_LENGTH ];
	uint8 *sourceBuff = (uint8*)buffer;
	ssize_t toWrite = count;
	ssize_t toWriteNow;
	ssize_t writeCount = 0;

	D(bug("MetaDOS: Fwrite (%d,%d)", handle, count));

	while ( toWrite > 0 ) {
		toWriteNow = ( toWrite > FRDWR_BUFFER_LENGTH ) ? FRDWR_BUFFER_LENGTH : toWrite;
		a2fmemcpy( fBuff, sourceBuff, toWriteNow );
		writeCount = write( fp->hostfh, fBuff, toWriteNow );

		D(bug("MetaDOS: Fwrite writeCount (%d)", writeCount));

		if ( writeCount <= 0 )
			break;

		sourceBuff += writeCount;
		fp->offset += writeCount;
		toWrite -= writeCount;
	}

	//	D(bug("MetaDOS: Fread error (%d)", errno));
	if ( writeCount < 0 )
		return TOS_EINTRN;// FIXME

	return count - toWrite;
}


int32 ExtFs::Fdelete(LogicalDev *ldp, char *pathName, ExtFile *fp,
					 const char *pn)
{
	char fpathName[MAXPATHNAMELEN];
	convertPathA2F( fpathName, pathName );

	D(bug("MetaDOS: Fdelete (%s,%s)", pathName, fpathName));

	if ( remove( fpathName ) )
		return unix2toserrno(errno,TOS_EFILNF);

	return TOS_E_OK;
}


int32 ExtFs::Fseek(LogicalDev *ldp, char *pathName, ExtFile *fp, int32 offset, int16 handle, int16 seekmode)
{
	int whence;

	switch (seekmode) {
		case 0:	 whence = SEEK_SET;	break;
		case 1:	 whence = SEEK_CUR;	break;
		case 2:	 whence = SEEK_END;	break;
		default: return TOS_EINVFN;
	}

	off_t newoff = lseek( fp->hostfh, offset, whence);

	D(bug("MetaDOS: Fseek (%d,%d,%d,%d)", offset, handle, seekmode, (int32)newoff));

	if ( newoff == -1 )
		return unix2toserrno(errno,TOS_EIO);

	fp->offset = (int32)newoff;
	return newoff;
}


int32 ExtFs::Fattrib(LogicalDev *ldp, char *pathName, ExtFile *fp, const char* pn, int16 wflag, int16 attr)
{
	char fpathName[MAXPATHNAMELEN];
	struct stat statBuf;

	convertPathA2F( fpathName, pathName );

	D(bug("MetaDOS: Fattrib (%s,%d,%d)", fpathName, wflag, attr));

	if ( stat(fpathName, &statBuf) )
		return unix2toserrno(errno,TOS_EFILNF);

	if (wflag != 0) {
		D(bug("MetaDOS: Fattrib set attribs (NOT IMPLEMENTED!!!)"));
	}

	return statmode2attr(statBuf.st_mode);
}



#define MAXDIRS 255
static char* fs_pathName[MAXDIRS+1];


int32 ExtFs::Fsfirst( LogicalDev *ldp, char *pathName, ExtDta *dta, const char *pn, int16 attribs )
{
	int16 idx;
	if ( ( idx = getFreeDirIndex( fs_pathName ) ) == -1 ) {
		D(bug("MetaDOS: Fsfirst no more directories!! (Fanda specific error)"));
		return TOS_ENHNDL; // FIXME? No more dir handles (Fanda specific)
	}
	dta->ds_index = idx;

	char *maskPos;
	if ( (maskPos = strrchr( pathName, '\\' )) != NULL ) {
		// Truncate the path
		*maskPos++ = '\0';
		// Copy the mask string
		strncpy( (char*)dta->ds_name, maskPos, 13 );
		dta->ds_name[13] = '\0';
	} else
		dta->ds_name[0] = '\0';

	dta->ds_attrib = attribs;

	convertPathA2F( fs_pathName[ dta->ds_index ], pathName );

	D(bug("MetaDOS: Fsfirst (%s, %s,%s,%#04x)", pathName,
			  fs_pathName[ dta->ds_index ],
			  dta->ds_name,
			  dta->ds_attrib ));
	D(bug("MetaDOS: Fsfirst (pn = %s)", pn));

	dta->ds_dirh = opendir( fs_pathName[ dta->ds_index ] );
	if ( dta->ds_dirh == NULL ) {
		freeDirIndex( dta->ds_index, fs_pathName );
		return TOS_EPTHNF;
	}

	int32 result = findFirst( dta, fs_pathName[ dta->ds_index ] );
	if ( result != TOS_E_OK || !strpbrk( (char*)dta->ds_name, "*?" ) ) {
		// if no file was found
		// or the mask doesn't contain wildcards
		// just a file exists test (no more Fsnext is called?)
		closedir( dta->ds_dirh );
		freeDirIndex( dta->ds_index, fs_pathName );
	}

	return result;
}


int32 ExtFs::Fsnext( LogicalDev *ldp, char *pathName, ExtDta *dta )
{
	D(bug("MetaDOS: Fsnext (%s,%s)", pathName, fs_pathName[ dta->ds_index ]));

	int32 result = findFirst( dta, fs_pathName[ dta->ds_index ] );
	if ( result != TOS_E_OK ) {
		closedir( dta->ds_dirh );
		freeDirIndex( dta->ds_index, fs_pathName );
	}

	return result;
}


int32 ExtFs::Frename(LogicalDev *ldp, char *pathName, ExtFile *fp,
					 int16 reserved, char *oldpath, char *newPathName )
{
	char fpathName[MAXPATHNAMELEN];
	char fnewPathName[MAXPATHNAMELEN];
	convertPathA2F( fpathName, pathName );
	//	convertPathA2F( fnewPathName, (char*)newPathName );

	if (   newPathName[0] == '\\' ||
		 ( newPathName[0] != '\0' && newPathName[1] == ':' ))
		convertPathA2F( fnewPathName, newPathName );
	else {
		strcpy(fnewPathName,fpathName);
		char *slashPos = strrchr( fnewPathName, '/' );
		if(slashPos != NULL)
			strcpy(slashPos+1,newPathName);
	}

	D(bug("MetaDOS: Frename (%s,%s)", fpathName, fnewPathName));

	if ( rename( fpathName, fnewPathName ) )
		return unix2toserrno(errno,TOS_EFILNF);

	return TOS_E_OK;
}


int32 ExtFs::Fdatime( LogicalDev *ldp, char *pathName, ExtFile *fp, uint32 *datetimep, int16 handle, int16 wflag)
{
	char fpathName[MAXPATHNAMELEN];
	struct stat statBuf;

	convertPathA2F( fpathName, pathName );
	if ( stat(fpathName, &statBuf) )
		return unix2toserrno(errno,TOS_EFILNF);

	D(bug("MetaDOS: Fdatime "));

	uint32 datetime = get_long( (uint32)datetimep, true );
	if (wflag != 0) {
		struct tm ttm;
		struct utimbuf tmb;

		datetime2tm( datetime, &ttm );
		tmb.actime = mktime( &ttm );  /* access time */
		tmb.modtime = tmb.actime; /* modification time */

		utime( fpathName, &tmb );

		D(bug("setting to: %d.%d.%d %d:%d.%d",
				  ttm.tm_mday,
				  ttm.tm_mon,
				  ttm.tm_year + 1900,
				  ttm.tm_sec,
				  ttm.tm_min,
				  ttm.tm_hour
				  ));
	}
	D(bug(""));

	datetime =
		( time2dos(statBuf.st_mtime) << 16 ) | date2dos(statBuf.st_mtime);
	put_long( (uint32)datetimep, datetime );

	return TOS_E_OK; //EBADRQ;
}


int32 ExtFs::Fcntl( LogicalDev *ldp, char *pathName, ExtFile *fp, int16 handle, void *arg, int16 cmd)
{
	D(bug("MetaDOS: Fcntl (NOT IMPLEMENTED!!!)"));
	return TOS_EINVFN;
}


int32 ExtFs::Dpathconf( LogicalDev *ldp, char *pathName, ExtFile *fp, const char* pn, int16 cmd)
{
	D(bug("MetaDOS: Dpathconf (%s,%d)", pathName, cmd));

	int	 oldErrno = errno;
	char fpathName[MAXPATHNAMELEN];
#ifdef HAVE_SYS_STATVFS_H 
	struct statvfs buf;
#else
	struct statfs buf;
#endif
	convertPathA2F( fpathName, pathName );

	D(bug("MetaDOS: Dpathconf (%s,%s,%d)", pathName, fpathName, cmd));

#ifdef HAVE_SYS_STATVFS_H 
	if ( statvfs(fpathName, &buf) )
#else
	if ( statfs(fpathName, &buf) )
#endif
		return unix2toserrno(errno,TOS_EFILNF);

	switch (cmd) {
		case -1:
			return 8;  // maximal cmd value

		case 0:	  // DP_IOPEN
			return 0x7fffffffL; // unlimited

		case 1:	{ // DP_MAXLINKS
			long result = pathconf(fpathName, _PC_LINK_MAX);
			if ( result == -1 && oldErrno != errno )
				return unix2toserrno(errno,TOS_EFILNF);

			return result;
		}
		case 2:	  // DP_PATHMAX
			return MAXPATHNAMELEN; // FIXME: This is the limitation of this implementation (Fanda specific)

		case 3:	  // DP_NAMEMAX
#ifdef HAVE_SYS_STATVFS_H 
                        return buf.f_namemax;
#else
			return buf.f_namelen;
#endif

		case 4:	  // DP_ATOMIC
			return buf.f_bsize;	 // ST max vs Linux optimal

		case 5:	  // DP_TRUNC
			return 0;  // files are NOT truncated... (hope correct)

		case 6:	{ // DP_CASE
			ExtDrive *drv = getDrive( pathName );
			return drv->halfSensitive ? 2 : 0; // full case sensitive
		}
		case 7:	  // D_XATTRMODE
			return 0x0ff0001fL;	 // only the archive bit is not recognised in the Fxattr

		case 8:	  // DP_XATTR
			return 0x00000ffbL;	 // rdev is not used

		default:
			return TOS_EINVFN;
	}
	return TOS_EINVFN;
}


int32 ExtFs::Dopendir( LogicalDev *ldp, char *pathName, ExtFile *fp, const char* pn, int16 flag)
{
	if ( ( fp->index = getFreeDirIndex( fs_pathName ) ) == -1 ) {
		D(bug("MetaDOS: Dopendir no more directories!! (Fanda specific error)"));
		return TOS_ENHNDL; // FIXME? No more dir handles (Fanda specific)
	}

	convertPathA2F( fs_pathName[ fp->index ], pathName );

	D(bug("MetaDOS: Dopendir (%s,%s,%d)", pathName, fs_pathName[ fp->index ], flag));

	fp->device = toupper(pathName[0]) - 'A'; // FIXME
	fp->hostfh = (uint32)opendir( fs_pathName[ fp->index ] );
	if ( ((DIR*)fp->hostfh) == NULL )
		return unix2toserrno(errno,TOS_EPTHNF);

	return TOS_E_OK;
}


int32 ExtFs::Dclosedir( LogicalDev *ldp, char *pathName, ExtFile *fp, int32 dirhandle)
{
	freeDirIndex( fp->index, fs_pathName );

	if ( closedir( (DIR*)fp->hostfh ) )
		return unix2toserrno(errno,TOS_EPTHNF);

	return TOS_E_OK;
}


int32 ExtFs::Dreaddir( LogicalDev *ldp, char *pathName, ExtFile *fp,
					   int16 len, int32 dirhandle, char* buff )
{
	struct dirent *dirEntry;

	if ((void*)(dirEntry = readdir( (DIR*)fp->hostfh )) == NULL)
		return unix2toserrno(errno,TOS_ENMFIL);

	if ( fp->mode == 0 ) {
		if ( (uint16)len < strlen( dirEntry->d_name ) )
			return TOS_ERANGE;

		put_long( (uint32)buff, dirEntry->d_ino );
		f2astrcpy( (uint8*)buff + 4, (uint8*)dirEntry->d_name );

		D(bug("MetaDOS: Dreaddir (%s,%s)", fs_pathName[ fp->index ], (char*)dirEntry->d_name ));
	} else {
		char truncFileName[MAXPATHNAMELEN];
		transformFileName( truncFileName, (char*)dirEntry->d_name );

		if ( (uint16)len < strlen( truncFileName ) )
			return TOS_ERANGE;

		f2astrcpy( (uint8*)buff, (uint8*)truncFileName );
		D(bug("MetaDOS: Dreaddir (%s,%s)", fs_pathName[ fp->index ], (char*)truncFileName ));
	}

	return TOS_E_OK;
}


int32 ExtFs::Drewinddir( LogicalDev *ldp, char *pathName, ExtFile *fp, int32 dirhandle)
{
	rewinddir( (DIR*)fp->hostfh );
	return TOS_E_OK;
}


int32 ExtFs::Fxattr( LogicalDev *ldp, char *pathName, ExtDta *dta,
					 int16 flag, const char* pn, uint32 xattrp )
{
	char fpathName[MAXPATHNAMELEN];
	convertPathA2F( fpathName, pathName );

	return Fxattr_( ldp, fpathName, dta, flag, pn, xattrp );
}


int32 ExtFs::Fxattr_( LogicalDev *ldp, char *fpathName, ExtDta *dta,
					  int16 flag, const char* pn, uint32 xattrp )
{
	D(bug("MetaDOS: Fxattr (%s,%d)", fpathName, flag));

	struct stat statBuf;

	if ( flag != 0 ) {
		// perform the link stat itself
		if ( lstat(fpathName, &statBuf) )
			return unix2toserrno(errno,TOS_EFILNF);
	} else {
		// perform the file stat
		if ( stat(fpathName, &statBuf) )
			return unix2toserrno(errno,TOS_EFILNF);
	}

	// XATTR structure conversion (COMPEND.HYP)
	/* UWORD mode	   */  put_word( xattrp		, statmode2xattrmode(statBuf.st_mode));
	/* LONG	 index	   */  put_long( xattrp +  2, statBuf.st_ino );
	/* UWORD dev	   */  put_word( xattrp +  6, statBuf.st_dev );	 // FIXME: this is Linux's one
	/* UWORD reserved1 */  put_word( xattrp +  8, 0 );
	/* UWORD nlink	   */  put_word( xattrp + 10, statBuf.st_nlink );
	/* UWORD uid	   */  put_word( xattrp + 12, statBuf.st_uid );	 // FIXME: this is Linux's one
	/* UWORD gid	   */  put_word( xattrp + 14, statBuf.st_gid );	 // FIXME: this is Linux's one
	/* LONG	 size	   */  put_long( xattrp + 16, statBuf.st_size );
	/* LONG	 blksize   */  put_long( xattrp + 20, statBuf.st_blksize );
	/* LONG	 nblocks   */  put_long( xattrp + 24, statBuf.st_blocks );
	/* UWORD mtime	   */  put_word( xattrp + 28, time2dos(statBuf.st_mtime) );
	/* UWORD mdate	   */  put_word( xattrp + 30, date2dos(statBuf.st_mtime) );
	/* UWORD atime	   */  put_word( xattrp + 32, time2dos(statBuf.st_atime) );
	/* UWORD adate	   */  put_word( xattrp + 34, date2dos(statBuf.st_atime) );
	/* UWORD ctime	   */  put_word( xattrp + 36, time2dos(statBuf.st_ctime) );
	/* UWORD cdate	   */  put_word( xattrp + 38, date2dos(statBuf.st_ctime) );
	/* UWORD attr	   */  put_word( xattrp + 40, statmode2attr(statBuf.st_mode) );
	/* UWORD reserved2 */  put_word( xattrp + 42, 0 );
	/* LONG	 reserved3 */  put_long( xattrp + 44, 0 );
	/* LONG	 reserved4 */  put_long( xattrp + 48, 0 );

	return TOS_E_OK;
}


int32 ExtFs::Dxreaddir( LogicalDev *ldp, char *pathName, ExtFile *fp,
						int16 len, int32 dirhandle, char* buff, uint32 xattrp, uint32 xretp )
{
	char*	fpathName = fs_pathName[ fp->index ];

	int32 result = Dreaddir( ldp, fpathName, fp, len, dirhandle, buff );
	if ( result != 0 )
		return result;

	ssize_t length = strlen( fpathName ) - 1;
	if ( fpathName[ length++ ] != '/' )
		fpathName[ length++ ] = '/';
	a2fstrcpy( &fpathName[ length ], (uint8*)&buff[ fp->mode == 0 ? 4 : 0 ] );

	put_long( xretp, Fxattr_( ldp, fpathName, NULL, 1, pathName, xattrp ) );
	fpathName[ length ] = '\0';

	return result;
}


int16 ExtFs::getFreeDirIndex( char **pathNames )
{
	int16 count = MAXDIRS;
	while ( count-- >= 0 )
		if ( pathNames[count] == NULL ) {
			pathNames[ count ] = new char[MAXPATHNAMELEN];
			return count;
		}

	return -1;
}

void ExtFs::freeDirIndex( uint8 index, char **pathNames )
{
	delete [] pathNames[ index ];
	pathNames[ index ] = NULL;
}


bool ExtFs::filterFiles( ExtDta *dta, char *fpathName, char *mask, struct dirent *dirEntry )
{
	strcat( fpathName, (char*)dirEntry->d_name );

	//FIXME	 if ( stat(fpathName, &statBuf) )
	//	return false;

	#if 0
	    // The . and .. dirs _should_ be returned (Julian Reschke said)
	    if ( dirEntry->d_name[0] == '.' &&
		   ( dirEntry->d_name[1] == '\0' ||
			 dirEntry->d_name[1] == '.' && dirEntry->d_name[2] == '\0' ) )
			return false;
	#endif

	// match from STonX
	if ( strcmp(mask,"*.*") == 0 )
		return true;
	else if ( strcasecmp( mask, (char*)dirEntry->d_name )==0)
		return true;
	else {
		char *p = mask,
			*n = (char*)dirEntry->d_name;

		for(;*n;)	{
			if (*p=='*') {while (*n && *n != '.') n++;p++;}
			else if (*p=='?' && *n) {n++;p++;}
			else if (*p++ != *n++) return false;
		}
		if (*p==0)
			return true;
	}
	return false;
}


int32 ExtFs::findFirst( ExtDta *dta, char *fpathName )
{
	struct stat statBuf;
	struct dirent *dirEntry;
	bool   suitableFile = false;
	char   tempPathMask[MAXPATHNAMELEN];
	char   *mask;

	strcat( fpathName, "/" );

	// if the mask is a filename then it must be expanded to the hostfs one
	convertPathA2F( tempPathMask, (char*)dta->ds_name, fpathName );
	if ( (mask = strrchr( tempPathMask, '/' )) != NULL )
		mask++;

	while ( ! suitableFile ) {
		if ((dirEntry = readdir( dta->ds_dirh )) == NULL)
			return unix2toserrno(errno,TOS_ENMFIL);

		// chop fileName (leave slash)
		*(strrchr( fpathName, '/' ) + 1) = '\0';

		// mask (&attrib) match?
		suitableFile = filterFiles( dta, fpathName, mask, dirEntry );
	}

	D(bug("MetaDOS: Fs..... (%s)", fpathName ));

	if ( stat(fpathName, &statBuf) )
		return unix2toserrno(errno,TOS_EFILNF);

	// TODO Here might be a section for Dreaddir function...

	// chop fileName to path only
	// for use in Fsnext
	*(strrchr( fpathName, '/' )) = '\0';

	transformFileName( (char*)dta->d_fname, (char*)dirEntry->d_name );

	dta->d_attrib = statmode2attr(statBuf.st_mode);
	dta->d_time = time2dos(statBuf.st_mtime);
	dta->d_date = date2dos(statBuf.st_mtime);
	dta->d_length = statBuf.st_size;

	return TOS_E_OK;
}

#endif /* EXTFS_SUPPORT */

/*
 * $Log$
 * Revision 1.22  2002/01/09 19:14:12  milan
 * Preliminary support for SGI/Irix
 *
 * Revision 1.21  2002/01/08 18:33:49  standa
 * The size of the bx_options.aranymfs[] and ExtFs::drives[] fixed.
 *
 * Revision 1.20  2002/01/08 17:51:07  standa
 * The aranymfs config file settings finished. Thanks to JOY.
 *
 * Revision 1.19  2001/12/11 21:04:32  standa
 * Debug set to 0
 *
 * Revision 1.18  2001/12/04 09:37:04  standa
 * One more Frename condition optimalization.
 *
 * Revision 1.17  2001/12/04 09:32:18  standa
 * Olivier Landemarre <Olivier.Landemarre@utbm.fr>: Frename patch.
 *
 * Revision 1.16  2001/11/21 13:29:51  milan
 * cleanning & portability
 *
 * Revision 1.15  2001/11/20 21:25:19  milan
 * Portability. And small correction in ATCs.
 *
 * Revision 1.14  2001/11/13 18:26:32  milan
 * portability
 *
 * Revision 1.13  2001/11/12 15:11:37  milan
 * Small upgrade to multiplatform compatibility
 *
 * Revision 1.12  2001/10/17 18:07:00  standa
 * the . and .. directories are returned by the Fsfirst and Fsnext (according to Julian Reschke)
 *
 * Revision 1.11  2001/10/16 19:06:01  standa
 * The debug changed to 0.
 *
 * Revision 1.10  2001/09/18 12:35:12  joy
 * getDrvBits() added
 *
 * Revision 1.9  2001/08/30 13:01:16  standa
 * The cast warnings removed.
 * missing return statements added.
 *
 * Revision 1.8  2001/08/30 12:42:25  standa
 * Indentation fixed.
 *
 * Revision 1.7	 2001/06/21 20:16:53  standa
 * Dgetdrv(), Dsetdrv(), Dgetpath(), Dsetpath() propagation added.
 * Only Dsetpath() ever noticed to be propagated by MetaDOS.
 * BetaDOS tested -> the same story.
 *
 * Revision 1.6	 2001/06/18 13:21:55  standa
 * Several template.cpp like comments were added.
 * HostScreen SDL encapsulation class.
 *
 *
 */
