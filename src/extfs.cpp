/*
 * $Header$
 *
 * STanda 2001
 */

#include "sysdeps.h"
#include "cpu_emulation.h"
#include "main.h"

#ifdef EXTFS_SUPPORT

#include "parameters.h"
#include "toserror.h"
#include "extfs.h"
#include "araobjs.h"

#undef  DEBUG_FILENAMETRANSFORMATION
#define DEBUG 0
#include "debug.h"

#ifdef HAVE_SYS_VFS_H
# include <sys/vfs.h>
#endif

#ifdef OS_mint
#include <mint/osbind.h>
#include <mint/mintbind.h>
#endif /* OS_mint */

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
	ExtDir	   extDir;
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
			r->d[0] = ExtFs::Dfree( &ldp, (char*)pathname, &extFile,
							 ReadInt32( r->a[7] + 6 ),			   // diskinfop
							 (int16) ReadInt16( r->a[7] + 10 ) ); // drive
			flushFILE( &extFile, r->a[5] );
			break;
		case 57:	// Dcreate:
			{
				char pn[MAXPATHNAMELEN];

				D(bug("%s", "Dcreate"));
				fetchFILE( &extFile, r->a[5] );
				a2fstrcpy( (char*)pathname, (uint8*)r->a[4] );
				a2fstrcpy( (char*)pn, (uint8*)ReadInt32( r->a[7] + 6 ) );
				r->d[0] = ExtFs::Dcreate( &ldp, (char*)pathname, &extFile,
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
				a2fstrcpy( (char*)pn, (uint8*)ReadInt32( r->a[7] + 6 ) );
				r->d[0] = ExtFs::Ddelete( &ldp, (char*)pathname, &extFile,
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
				a2fstrcpy( (char*)pn, (uint8*)ReadInt32( r->a[7] + 6 ) );
				r->d[0] = ExtFs::Dsetpath( &ldp, (char*)pathname, &extFile,
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
				a2fstrcpy( (char*)pn, (uint8*)ReadInt32( r->a[7] + 6 ) );
				r->d[0] = ExtFs::Fcreate( &ldp, (char*)pathname, &extFile,
								   (const char*)pn,					   // pathname
								   (int16) ReadInt16( r->a[7] + 10 ) ); // mode
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
				a2fstrcpy( (char*)pn, (uint8*)ReadInt32( r->a[7] + 6 ) );
				r->d[0] = ExtFs::Fopen( &ldp, (char*)pathname, &extFile,
								 (const char*)pn,					 // pathname
								 (int16) ReadInt16( r->a[7] + 10 ) ); // mode
				flushFILE( &extFile, r->a[5] );
				D(bug("MetaDOS: %s: %d", "/Fopen", (int32)r->d[0]));
			}
			break;
		case 62:	// Fclose:
			D(bug("%s", "Fclose"));
			fetchFILE( &extFile, r->a[5] );
			a2fstrcpy( (char*)pathname, (uint8*)r->a[4] );
			r->d[0] = ExtFs::Fclose( &ldp, (char*)pathname, &extFile,
							  (int16) ReadInt16( r->a[7] + 6 ) ); // handle
			flushFILE( &extFile, r->a[5] );
			break;
		case 63:	// Fread:
			D(bug("%s", "Fread"));
			fetchFILE( &extFile, r->a[5] );
			a2fstrcpy( (char*)pathname, (uint8*)r->a[4] );
			r->d[0] = ExtFs::Fread( &ldp, (char*)pathname, &extFile,
							 (int16) ReadInt16( r->a[7] + 6 ),	 // handle
							 ReadInt32( r->a[7] + 8 ),			 // count
							 (void*)ReadInt32( r->a[7] + 12) ); // buffer
			flushFILE( &extFile, r->a[5] );
			break;
		case 64:	// Fwrite:
			D(bug("%s", "Fwrite"));
			fetchFILE( &extFile, r->a[5] );
			a2fstrcpy( (char*)pathname, (uint8*)r->a[4] );
			r->d[0] = ExtFs::Fwrite( &ldp, (char*)pathname, &extFile,
							  (int16) ReadInt16( r->a[7] + 6 ),  // handle
							  ReadInt32( r->a[7] + 8 ),		  // count
							  (void*)ReadInt32( r->a[7] + 12 ) ); // buffer
			flushFILE( &extFile, r->a[5] );
			break;
		case 65:	// Fdelete:
			{
				char pn[MAXPATHNAMELEN];

				D(bug("%s", "Fdelete"));
				fetchFILE( &extFile, r->a[5] );
				a2fstrcpy( (char*)pathname, (uint8*)r->a[4] );
				a2fstrcpy( (char*)pn, (uint8*)ReadInt32( r->a[7] + 6 ) );
				r->d[0] = ExtFs::Fdelete( &ldp, (char*)pathname, &extFile,
								   (const char*)pn );				  // pathname
				flushFILE( &extFile, r->a[5] );
				D(bug("MetaDOS: %s: %d", "/Fdelete", (int32)r->d[0]));
			}
			break;
		case 66:	// Fseek:
			D(bug("%s", "Fseek"));
			fetchFILE( &extFile, r->a[5] );
			a2fstrcpy( (char*)pathname, (uint8*)r->a[4] );
			r->d[0] = ExtFs::Fseek( &ldp, (char*)pathname, &extFile,
							 ReadInt32( r->a[7] + 6 ),			 // offset
							 (int16) ReadInt16( r->a[7] + 10 ),  // handle
							 (int16) ReadInt16( r->a[7] + 12 ) );// seekmode
			flushFILE( &extFile, r->a[5] );
			break;
		case 67:	// Fattrib:
			{
				char pn[MAXPATHNAMELEN];

				D(bug("%s", "Fattrib"));
				fetchFILE( &extFile, r->a[5] );
				a2fstrcpy( (char*)pathname, (uint8*)r->a[4] );
				a2fstrcpy( (char*)pn, (uint8*)ReadInt32( r->a[7] + 6 ) );
				r->d[0] = ExtFs::Fattrib( &ldp, (char*)pathname, &extFile,
								   (const char*)pn,					 // pathname
								   (int16) ReadInt16( r->a[7] + 10 ),	// wflag
								   (int16) ReadInt16( r->a[7] + 12 ) );// attr
				flushFILE( &extFile, r->a[5] );
			}
			break;
		case 78:	// Fsfirst:
			{
				char pn[MAXPATHNAMELEN];

				D(bug("%s", "Fsfirst"));
				fetchDTA( &dta, r->a[5] );
				a2fstrcpy( (char*)pathname, (uint8*)r->a[4] );
				a2fstrcpy( (char*)pn, (uint8*)ReadInt32( r->a[7] + 6 ) );
				r->d[0] = ExtFs::Fsfirst( &ldp, (char*)pathname, &dta,
								   (const char*)pn,
								   (int16) ReadInt16( r->a[7] + 10 ) );
				flushDTA( &dta, r->a[5] );
				D(bug("MetaDOS: %s", "/Fsfirst"));
			}
			break;
		case 79:	// Fsnext:
			D(bug("%s", "Fsnext"));
			fetchDTA( &dta, r->a[5] );
			a2fstrcpy( (char*)pathname, (uint8*)r->a[4] );
			r->d[0] = ExtFs::Fsnext( &ldp, (char*)pathname, &dta );
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
				a2fstrcpy( (char*)pn, (uint8*)ReadInt32( r->a[7] + 8 ) );
				a2fstrcpy( (char*)npn, (uint8*)ReadInt32( r->a[7] + 12 ) );
				r->d[0] = ExtFs::Frename( &ldp, (char*)pathname, &extFile,
								   (int16) ReadInt16( r->a[7] + 6 ), // reserved
								   pn,				          // oldpathname
								   npn );		              // newpathname
				flushFILE( &extFile, r->a[5] );
			}
			break;
		case 87:	// Fdatime:
			D(bug("%s", "Fdatime"));
			fetchFILE( &extFile, r->a[5] );
			a2fstrcpy( (char*)pathname, (uint8*)r->a[4] );
			r->d[0] = ExtFs::Fdatime( &ldp, (char*)pathname, &extFile,
							   (uint32*)ReadInt32( r->a[7] + 6 ), // datetimep
							   (int16) ReadInt16( r->a[7] + 10 ),	// handle
							   (int16) ReadInt16( r->a[7] + 12 ) );// wflag
			flushFILE( &extFile, r->a[5] );
			break;
		case 260:	// Fcntl:
			D(bug("%s", "Fcntl"));
			fetchFILE( &extFile, r->a[5] );
			a2fstrcpy( (char*)pathname, (uint8*)r->a[4] );
			r->d[0] = ExtFs::Fcntl( &ldp, (char*)pathname, &extFile,
							 (int16)ReadInt16( r->a[7] + 6 ),	  // handle
							 (void*)ReadInt32( r->a[7] + 8 ),	  // arg
							 (int16)ReadInt16( r->a[7] + 12 ) ); // cmd
			flushFILE( &extFile, r->a[5] );
			break;
		case 292:	// Dpathconf:
			{
				char pn[MAXPATHNAMELEN];

				D(bug("%s", "Dpathconf"));
				fetchFILE( &extFile, r->a[5] );
				a2fstrcpy( (char*)pathname, (uint8*)r->a[4] );
				a2fstrcpy( (char*)pn, (uint8*)ReadInt32( r->a[7] + 6 ) );
				r->d[0] = ExtFs::Dpathconf( &ldp, (char*)pathname, &extFile,
									 (const char*)pn,					 // pathname
									 (int16)ReadInt16( r->a[7] + 10 ) ); // cmd
				flushFILE( &extFile, r->a[5] );
				D(bug("MetaDOS: /Dpathconf res = %#8x (%d)", r->d[0],(int32)r->d[0]));
			}
			break;
		case 296:	// Dopendir:
			{
				char pn[MAXPATHNAMELEN];

				D(bug("%s", "Dopendir"));
				fetchEDIR( &extDir, r->a[5] );
				a2fstrcpy( (char*)pathname, (uint8*)r->a[4] );
				a2fstrcpy( (char*)pn, (uint8*)ReadInt32( r->a[7] + 6 ) );
				r->d[0] = ExtFs::Dopendir( &ldp, pathname, &extDir,
									(const char*)pn,					// pathname
									(int16)ReadInt16( r->a[7] + 10 ) ); // flag
				flushEDIR( &extDir, r->a[5] );
				D(bug("MetaDOS: /Dopendir res = %#08x (%d)", r->d[0],(int32)r->d[0]));
			}
			break;
		case 297:	// Dreaddir:
			D(bug("%s", "Dreaddir"));
			fetchEDIR( &extDir, r->a[5] );
			a2fstrcpy( (char*)pathname, (uint8*)r->a[4] );
			r->d[0] = ExtFs::Dreaddir( &ldp, (char*)pathname, &extDir,
								(int16) ReadInt16( r->a[7] + 6 ),	 // len
								ReadInt32( r->a[7] + 8 ),			 // dirhandle
								(char*)ReadInt32( r->a[7] + 12 ) ); // bufferp
			flushEDIR( &extDir, r->a[5] );
			break;
		case 298:	// Drewinddir:
			D(bug("%s", "Drewinddir"));
			fetchEDIR( &extDir, r->a[5] );
			a2fstrcpy( (char*)pathname, (uint8*)r->a[4] );
			r->d[0] = ExtFs::Drewinddir( &extDir );
			//ReadInt32( r->a[7] + 6 ) );		 // dirhandle
			flushEDIR( &extDir, r->a[5] );
			break;
		case 299:	// Dclosedir:
			D(bug("%s", "Dclosedir"));
			fetchEDIR( &extDir, r->a[5] );
			a2fstrcpy( (char*)pathname, (uint8*)r->a[4] );
			r->d[0] = ExtFs::Dclosedir( &extDir );
			//ReadInt32( r->a[7] + 6 ) );		// dirhandle
			flushEDIR( &extDir, r->a[5] );
			break;
		case 300:	// Fxattr:
			{
				char pn[MAXPATHNAMELEN];

				D(bug("%s", "Fxattr"));
				fetchDTA( &dta, r->a[5] );
				a2fstrcpy( (char*)pathname, (uint8*)r->a[4] );
				a2fstrcpy( (char*)pn, (uint8*)ReadInt32( r->a[7] + 8 ) );
				r->d[0] = ExtFs::Fxattr( &ldp, (char*)pathname, &dta,
								  (int16) ReadInt16( r->a[7] + 6 ), // flag
								  (const char*)pn,				   // pathname
								  ReadInt32( r->a[7] + 12 ) );		 // XATTR*
				flushDTA( &dta, r->a[5] );
				D(bug("MetaDOS: /Fxattr res = %#08x (%d)", r->d[0],(int32)r->d[0]));
			}
			break;
		case 322:	// Dxreaddir:
			D(bug("%s", "Dxreaddir"));
			fetchEDIR( &extDir, r->a[5] );
			a2fstrcpy( (char*)pathname, (uint8*)r->a[4] );
			r->d[0] = ExtFs::Dxreaddir( &ldp, (char*)pathname, &extDir,
								 (int16) ReadInt16( r->a[7] + 6 ),	  // len
								 ReadInt32( r->a[7] + 8 ),			  // dirhandle
								 (char*)ReadInt32( r->a[7] + 12 ),	  // bufferp
								 ReadInt32( r->a[7] + 16 ),		  // XATTR*
								 ReadInt32( r->a[7] + 20 ) );		  // xret
			flushEDIR( &extDir, r->a[5] );
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

void ExtFs::dispatchXFS( uint32 fncode, M68kRegisters *r )
{
	XfsCookie  fc;
	XfsCookie  resFc;
	ExtFile	   extFile;
	ExtDir	   dirh;

#define    SBC_STX_FS     0x00010000
#define    SBC_STX_FS_DEV 0x00010100
#define    SBC_STX_SER    0x00010200
#define    SBC_STX_COM    0x00010300

	// fix the stack (the fncode was pushed onto the stack)
	r->a[7] += 4;

	D(fprintf(stderr, "    XFS: %4x ", fncode&0xffff));

#if 0 // toto??? 1 - yes
	1 D(bug("%s", "fs_chattr"));
	1 D(bug("%s", "fs_chown"));
	1 D(bug("%s", "fs_chmode"));
	  D(bug("%s", "fs_writelabel"));
	  D(bug("%s", "fs_readlabel"));
	1 D(bug("%s", "fs_symlink"));
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

	switch (fncode) {
		case SBC_STX_FS:
			D(bug("fs_init (%x)", r->a[7]));
			mint_fs_drv    = ReadInt32( r->a[7] );   // filesys
			mint_fs_devdrv = ReadInt32( r->a[7] + 4 );   // devdrv
			// +  8 // retaddr
			// + 12 // kerinfo
			mint_fs_devnum = ReadInt16( r->a[7] + 16 );   // devnum
			D(bug("MiNT fs:\n"
				  " fs_drv     = %#08x\n"
				  " fs_devdrv  = %#08x\n"
				  " fs_devnum  = %#04x\n"
				  ,mint_fs_drv
				  ,mint_fs_devdrv
				  ,(int)mint_fs_devnum));
			break;
		case SBC_STX_FS+0x01:
			D(bug("%s", "fs_root"));
			fetchXFSC( &fc, ReadInt32(r->a[7] + 6) );
			r->d[0] = xfs_root( ReadInt16(r->a[7] + 4),   /* dev */
								&fc ); /* fcookie */
			flushXFSC( &fc, ReadInt32(r->a[7] + 6) );
			break;
		case SBC_STX_FS+0x02:
			D(bug("%s", "fs_lookup"));
			fetchXFSC( &fc, ReadInt32(r->a[7] + 4) );
			r->d[0] = xfs_lookup( &fc,
								  (char*)ReadInt32( r->a[7] + 8 ) /* name */,
								  &resFc );
			flushXFSC( &fc, ReadInt32(r->a[7] + 4) );
			flushXFSC( &resFc, ReadInt32(r->a[7] + 12) );
			break;
		case SBC_STX_FS+0x03:
			D(bug("%s", "fs_creat"));
			fetchXFSC( &fc, ReadInt32(r->a[7] + 4) );
			r->d[0] = xfs_creat( &fc,
								 (char*)ReadInt32( r->a[7] + 8 ) /* name */,
								 ReadInt16( r->a[7] + 12 ) /* mode */,
								 ReadInt16( r->a[7] + 14 ) /* attrib */,
								  &resFc );
			flushXFSC( &fc, ReadInt32(r->a[7] + 4) );
			flushXFSC( &resFc, ReadInt32(r->a[7] + 16) );
			break;
		case SBC_STX_FS+0x04:
			D(bug("%s", "fs_getdev"));
			fetchXFSC( &fc, ReadInt32(r->a[7] + 4) );
			r->d[0] = xfs_getdev( &fc, (int32*)ReadInt32( r->a[7] + 8 ) );
			flushXFSC( &resFc, ReadInt32(r->a[7] + 4) );
			break;
		case SBC_STX_FS+0x05:
			D(bug("%s", "fs_getxattr"));
			fetchXFSC( &fc, ReadInt32(r->a[7] + 4) );
			r->d[0] = xfs_getxattr( &fc,
									ReadInt32( r->a[7] + 8 ) ); // XATTR*
			flushXFSC( &fc, ReadInt32(r->a[7] + 4) );
			break;
		case SBC_STX_FS+0x06:
			D(bug("%s", "fs_chattr"));
			r->d[0] = (uint32)TOS_EINVFN;
			break;
		case SBC_STX_FS+0x07:
			D(bug("%s", "fs_chown"));
			r->d[0] = (uint32)TOS_EINVFN;
			break;
		case SBC_STX_FS+0x08:
			D(bug("%s", "fs_chmode"));
			r->d[0] = (uint32)TOS_EINVFN;
			break;
		case SBC_STX_FS+0x09:
			D(bug("%s", "fs_mkdir"));
			fetchXFSC( &fc, ReadInt32(r->a[7] + 4) );
			r->d[0] = xfs_mkdir( &fc,
								 (char*)ReadInt32(r->a[7] + 8),  // name
								 ReadInt16(r->a[7] + 12) ); // mode
			break;
		case SBC_STX_FS+0x0a:
			D(bug("%s", "fs_rmdir"));
			fetchXFSC( &fc, ReadInt32(r->a[7] + 4) );
			r->d[0] = xfs_rmdir( &fc, (char*)ReadInt32(r->a[7] + 8) );
			break;
		case SBC_STX_FS+0x0b:
			D(bug("%s", "fs_remove"));
			fetchXFSC( &fc, ReadInt32(r->a[7] + 4) );
			r->d[0] = xfs_remove( &fc,
								  (char*)ReadInt32( r->a[7] + 8 ) ); // name
			flushXFSC( &fc, ReadInt32(r->a[7] + 4) );
			break;
		case SBC_STX_FS+0x0c:
			D(bug("%s", "fs_getname"));
			fetchXFSC( &fc, ReadInt32(r->a[7] + 4) );
			fetchXFSC( &resFc, ReadInt32(r->a[7] + 8) );
			r->d[0] = xfs_getname( &fc,
								   &resFc,
								   (char*)ReadInt32(r->a[7] + 12), // pathName
								   ReadInt16( r->a[7] + 16 ) ); // size
			// not needed: flushXFSC( &fc, ReadInt32(r->a[7] + 4) );
			// not needed: flushXFSC( &resFc, ReadInt32(r->a[7] + 12) );
			break;
		case SBC_STX_FS+0x0d:
			D(bug("%s", "fs_rename"));
			fetchXFSC( &fc, ReadInt32(r->a[7] + 4) );
			fetchXFSC( &resFc, ReadInt32(r->a[7] + 12) );
			r->d[0] = xfs_rename( &fc, (char*)ReadInt32(r->a[7] + 8),
								  &resFc, (char*)ReadInt32(r->a[7] + 16) );
			// not needed: flushXFSC( &fc, ReadInt32(r->a[7] + 4) );
			// not needed: flushXFSC( &resFc, ReadInt32(r->a[7] + 12) );
			break;
		case SBC_STX_FS+0x0e:
			D(bug("%s", "fs_opendir"));
			fetchXFSD( &dirh, ReadInt32(r->a[7] + 4) );
			r->d[0] = xfs_opendir( &dirh,
								   ReadInt16( r->a[7] + 8 ) ); // flags
			flushXFSD( &dirh, ReadInt32(r->a[7] + 4) );
			break;
		case SBC_STX_FS+0x0f:
			D(bug("%s", "fs_readdir"));
			fetchXFSD( &dirh, ReadInt32(r->a[7] + 4) );
			r->d[0] = xfs_readdir( &dirh,
								   (char*)ReadInt32( r->a[7] +  8 ), // buff
								   ReadInt16( r->a[7] + 12 ), // bufflen
								   &resFc );
			flushXFSD( &dirh,  ReadInt32(r->a[7] +  4) );
			flushXFSC( &resFc, ReadInt32(r->a[7] + 14) );
			break;
		case SBC_STX_FS+0x10:
			D(bug("%s", "fs_rewinddir"));
			fetchXFSD( &dirh, ReadInt32(r->a[7] + 4) );
			r->d[0] = ExtFs::Drewinddir( &dirh );
			flushXFSD( &dirh,  ReadInt32(r->a[7] +  4) );
			break;
		case SBC_STX_FS+0x11:
			D(bug("%s", "fs_closedir"));
			fetchXFSD( &dirh, ReadInt32(r->a[7] + 4) );
			r->d[0] = ExtFs::Dclosedir( &dirh );
			flushXFSD( &dirh, ReadInt32(r->a[7] + 4) );
			break;
		case SBC_STX_FS+0x12:
			D(bug("%s", "fs_pathconf"));
			fetchXFSC( &fc, ReadInt32(r->a[7] + 4) );
			r->d[0] = xfs_pathconf( &fc,
									ReadInt16(r->a[7] + 8) ); // which
			flushXFSC( &fc, ReadInt32(r->a[7] + 4) );
			break;
		case SBC_STX_FS+0x13:
			D(bug("%s", "fs_dfree"));
			fetchXFSC( &fc, ReadInt32(r->a[7] + 4) );
			r->d[0] = xfs_dfree( &fc,
								 ReadInt32(r->a[7] + 8) ); // buff
			break;
		case SBC_STX_FS+0x14:
			D(bug("%s", "fs_writelabel"));
			r->d[0] = (uint32)TOS_EINVFN;
			break;
		case SBC_STX_FS+0x15:
			D(bug("%s", "fs_readlabel"));
			r->d[0] = (uint32)TOS_EINVFN;
			break;
		case SBC_STX_FS+0x16:
			D(bug("%s", "fs_symlink"));
			r->d[0] = (uint32)TOS_EINVFN;
			break;
		case SBC_STX_FS+0x17:
			D(bug("%s", "fs_readlink"));
			fetchXFSC( &fc, ReadInt32(r->a[7] + 4) );
			r->d[0] = xfs_readlink( &fc,
									(char*)ReadInt32(r->a[7] + 8), // buff
									ReadInt16(r->a[7] + 12) ); // len
			break;
		case SBC_STX_FS+0x18:
			D(bug("%s", "fs_hardlink"));
			r->d[0] = (uint32)TOS_EINVFN;
			break;
		case SBC_STX_FS+0x19:
			D(bug("%s", "fs_fscntl"));
			r->d[0] = (uint32)TOS_EINVFN;
			break;
		case SBC_STX_FS+0x1a:
			D(bug("%s", "fs_dskchng"));
			r->d[0] = (uint32)TOS_EINVFN;
			break;
		case SBC_STX_FS+0x1b:
			D(bug("%s", "fs_release"));
			fetchXFSC( &fc, ReadInt32(r->a[7] + 4) );
			r->d[0] = xfs_release( &fc );
			flushXFSC( &fc, ReadInt32(r->a[7] + 4) );
			break;
		case SBC_STX_FS+0x1c:
			D(bug("%s", "fs_dupcookie"));
			fetchXFSC( &resFc, ReadInt32(r->a[7] + 4) );
			fetchXFSC( &fc, ReadInt32(r->a[7] + 8) );
			r->d[0] = xfs_dupcookie( &resFc, &fc );
			flushXFSC( &resFc, ReadInt32(r->a[7] + 4) );
			flushXFSC( &fc, ReadInt32(r->a[7] + 8) );
			break;
		case SBC_STX_FS+0x1d:
			D(bug("%s", "fs_sync"));
			r->d[0] = (uint32)TOS_EINVFN;
			break;
		case SBC_STX_FS+0x1e:
			D(bug("%s", "fs_mknod"));
			r->d[0] = (uint32)TOS_EINVFN;
			break;
		case SBC_STX_FS+0x1f:
			D(bug("%s", "fs_unmount"));
			r->d[0] = (uint32)TOS_EINVFN;
			break;

		case SBC_STX_FS_DEV+0x01:
			D(bug("%s", "fs_dev_open"));
			fetchXFSF( &extFile, ReadInt32(r->a[7] + 4) );
			r->d[0] = xfs_dev_open( &extFile );
			flushXFSF( &extFile, ReadInt32(r->a[7] + 4) );
			break;
		case SBC_STX_FS_DEV+0x02:
			D(bug("%s", "fs_dev_write"));
			fetchXFSF( &extFile, ReadInt32(r->a[7] + 4) );
			r->d[0] = ExtFs::Fwrite( NULL, NULL, &extFile,
							  0, // handle
							  ReadInt32(r->a[7] + 12), // bytes
							  (void*)ReadInt32(r->a[7] + 8) ); // buffer
			flushXFSF( &extFile, ReadInt32(r->a[7] + 4) );
			break;
		case SBC_STX_FS_DEV+0x03:
			D(bug("%s", "fs_dev_read"));
			fetchXFSF( &extFile, ReadInt32(r->a[7] + 4) );
			r->d[0] = ExtFs::Fread( NULL, NULL, &extFile,
							 0, // handle
							 ReadInt32(r->a[7] + 12), // bytes
							 (void*)ReadInt32(r->a[7] + 8) ); // buffer
			flushXFSF( &extFile, ReadInt32(r->a[7] + 4) );
			break;
		case SBC_STX_FS_DEV+0x04:
			D(bug("%s", "fs_dev_lseek"));
			fetchXFSF( &extFile, ReadInt32(r->a[7] + 4) );
			r->d[0] = ExtFs::Fseek( NULL, NULL, &extFile,
							 ReadInt32( r->a[7] + 8 ),			 // offset
							 0,  // handle
							 ReadInt16( r->a[7] + 12 ) );       // seekmode
			flushXFSF( &extFile, ReadInt32(r->a[7] + 4) );
			break;
		case SBC_STX_FS_DEV+0x05:
			D(bug("%s", "fs_dev_ioctl"));
			r->d[0] = (uint32)TOS_EINVFN;
			break;
		case SBC_STX_FS_DEV+0x06:
			D(bug("%s", "fs_dev_datime"));
			fetchXFSF( &extFile, ReadInt32(r->a[7] + 4) );
			r->d[0] = xfs_dev_datime( &extFile,
									  (uint32*)ReadInt32( r->a[7] + 8 ), // datetimep
									  (int16) ReadInt16( r->a[7] + 12 ) );// wflag
			flushXFSF( &extFile, ReadInt32(r->a[7] + 4) );
			break;
		case SBC_STX_FS_DEV+0x07:
			D(bug("%s", "fs_dev_close"));
			fetchXFSF( &extFile, ReadInt32(r->a[7] + 4) );
			r->d[0] = ExtFs::Fclose( NULL, NULL, &extFile,
							  0 ); // handle
			break;
		case SBC_STX_FS_DEV+0x08:
			D(bug("%s", "fs_dev_select"));
			r->d[0] = (uint32)TOS_E_OK;
			break;
		case SBC_STX_FS_DEV+0x09:
			D(bug("%s", "fs_dev_unselect"));
			r->d[0] = (uint32)TOS_E_OK;
			break;

		default:
			D(bug("Unknown"));
			r->d[0] = (uint32)TOS_EINVFN;
	}
}


void ExtFs::a2fmemcpy( uint8 *dest, uint8 *source, size_t count )
{
	while ( count-- )
		*dest++ = ReadInt8( (uint32)source++ );
}

void ExtFs::a2fstrcpy( char *dest, uint8 *source )
{
	while ( (*dest++ = ReadInt8( (uint32)source++ )) != 0 );
}


void ExtFs::f2amemcpy( uint8 *dest, uint8 *source, size_t count )
{
	while ( count-- )
		WriteInt8( (uint32)dest++, *source++ );
}

void ExtFs::f2astrcpy( uint8 *dest, uint8 *source )
{
	while ( *source )
		WriteInt8( (uint32)dest++, *source++ );
	WriteInt8( (uint32)dest++, 0 );
}


void ExtFs::fetchDTA( ExtDta *dta, uint32 dtap )
{
	// Normally reserved ... used in the CookFS
	dta->ds_dirh	 = (DIR*)ReadInt32( dtap );
	dta->ds_attrib	 = ReadInt16( dtap + 4 );
	dta->ds_index	 = ReadInt8( dtap + 6 );
	a2fmemcpy( (uint8*)dta->ds_name, (uint8*)(dtap + 7), 14 ); // search mask

	// Common FS
	dta->d_attrib = ReadInt8( dtap + 21 );
	dta->d_time	  = ReadInt16( dtap + 22 );
	dta->d_date	  = ReadInt16( dtap + 24 );
	dta->d_length = ReadInt32( dtap + 26 );
	a2fmemcpy( (uint8*)dta->d_fname, (uint8*)(dtap + 30), 14 );
}
void ExtFs::flushDTA( ExtDta *dta, uint32 dtap )
{
	// Normally reserved ... used in the CookFS
	WriteInt32( dtap	  , (uint32)dta->ds_dirh );
	WriteInt16( dtap + 4, dta->ds_attrib );
	WriteInt8( dtap + 6, dta->ds_index );
	f2amemcpy( (uint8*)(dtap + 7), (uint8*)dta->ds_name, 14 );

	// Common FS
	WriteInt8( dtap + 21, dta->d_attrib );
	WriteInt16( dtap + 22, dta->d_time );
	WriteInt16( dtap + 24, dta->d_date );
	WriteInt32( dtap + 26, dta->d_length );
	f2amemcpy( (uint8*)(dtap + 30), (uint8*)dta->d_fname, 14 );
}


void ExtFs::fetchFILE( ExtFile *extFile, uint32 filep )
{
	extFile->index	= ReadInt16( filep );
	//extFile->mode	= ReadInt16( filep + 2 );
	extFile->flags	= ReadInt16( filep + 4 ); // never used in the code
	extFile->hostfd = ReadInt32( filep + 6 );
	extFile->offset = ReadInt32( filep + 10 );  // probably not used
	//	extFile->device = ReadInt16( filep + 14 );  // never used

	extFile->links  = 0; // Fclose dosn't close the file when > 0
}
void ExtFs::flushFILE( ExtFile *extFile, uint32 filep )
{
	WriteInt16( filep	   , extFile->index );
	//WriteInt16( filep + 2, extFile->mode	);
	WriteInt16( filep + 4, extFile->flags );
	WriteInt32( filep + 6, extFile->hostfd );
	WriteInt32( filep +10, extFile->offset );
	//WriteInt16( filep +14, extFile->device );
}

void ExtFs::fetchEDIR( ExtDir *extDir, uint32 dirp )
{
	assert( 6 + sizeof(extDir->dir) < 8 * 4 ); // MetaDOS provided space

	extDir->flags      = ReadInt16( dirp     );
	extDir->fc.dev     = ReadInt16( dirp + 2 );
	extDir->pathIndex  = ReadInt16( dirp + 4 );
	a2fmemcpy( (uint8*)&extDir->dir, (uint8*)(dirp + 6), sizeof(extDir->dir) );
}
void ExtFs::flushEDIR( ExtDir *extDir, uint32 dirp )
{
	WriteInt16( dirp	  , extDir->flags );
	WriteInt16( dirp + 2, extDir->fc.dev );
	WriteInt16( dirp + 4, extDir->pathIndex );
	f2amemcpy( (uint8*)(dirp + 6), (uint8*)&extDir->dir, sizeof(extDir->dir) );
}


void ExtFs::fetchXFSC( XfsCookie *fc, uint32 filep )
{
	fc->xfs   = ReadInt32( filep );  // fs
	fc->dev   = ReadInt16( filep + 4 );  // dev
	fc->aux   = ReadInt16( filep + 6 );  // aux
	fc->index = (XfsFsFile*)ReadInt32( filep + 8 ); // index
}
void ExtFs::flushXFSC( XfsCookie *fc, uint32 filep )
{
	WriteInt32( filep    , fc->xfs );
	WriteInt16( filep + 4, fc->dev );
	WriteInt16( filep + 6, fc->aux );
	WriteInt32( filep + 8, (uint32)fc->index );
}

void ExtFs::fetchXFSF( ExtFile *extFile, uint32 filep )
{
	extFile->links  = ReadInt16( filep );
	extFile->flags  = ReadInt16( filep + 2 );
	extFile->hostfd = ReadInt32( filep + 4 ); // offset not needed (replaced by the host fd)
	extFile->devinfo= ReadInt32( filep + 8 );
	fetchXFSC( &extFile->fc, filep + 12 ); // sizeof(12)
	// 4bytes of the devdrvp
	extFile->next   = ReadInt32( filep + 28 );
}
void ExtFs::flushXFSF( ExtFile *extFile, uint32 filep )
{
	WriteInt16( filep, extFile->links );
	WriteInt16( filep + 2, extFile->flags );
	WriteInt32( filep + 4, extFile->hostfd ); // instead of the offset
	WriteInt32( filep + 8, extFile->devinfo );
	flushXFSC( &extFile->fc, filep + 12 ); // sizeof(12)
	WriteInt32( filep + 28, extFile->next );
}

void ExtFs::fetchXFSD( XfsDir *dirh, uint32 dirp )
{
	fetchXFSC( (XfsCookie*)dirh, dirp ); // sizeof(12)
	dirh->index = ReadInt16( dirp + 12 );
	dirh->flags = ReadInt16( dirp + 14 );
	dirh->pathIndex = ReadInt16( dirp + 16 );
	a2fmemcpy( (uint8*)&dirh->dir, (uint8*)(dirp + 18), sizeof(dirh->dir) );
	dirh->next  = (XfsDir*)ReadInt32( dirp + 76 );
}
void ExtFs::flushXFSD( XfsDir *dirh, uint32 dirp )
{
	flushXFSC( (XfsCookie*)dirh, dirp ); // sizeof(12)
	WriteInt16( dirp + 12, dirh->index );
	WriteInt16( dirp + 14, dirh->flags );
	WriteInt16( dirp + 16, dirh->pathIndex );
	f2amemcpy( (uint8*)(dirp + 18), (uint8*)&dirh->dir, sizeof(dirh->dir) );
	WriteInt32( dirp + 76, (uint32)dirh->next );
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
	return ((x->tm_sec&0x3f)>>1)|((x->tm_min&0x3f)<<5)|((x->tm_hour&0x1f)<<11);
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

	D2(bug("                    (statmode: %04x)", result));

	return result;
}


int ExtFs::st2flags(uint16 flags)
{
	switch(flags & 0x3)
	{
		case 0:
			return O_RDONLY;
		case 1:
			return O_WRONLY;	/* kludge to avoid files being created */
		case 2:
			return O_RDWR;
		default:
			return O_RDWR;      /* this should never happen (the O_WRONLY|O_RDWR simultaneously) */
	}
}


int16 ExtFs::flags2st(int flags)
{
	int16 res = 0;

	if (flags & O_WRONLY) res |= 1;
	if (flags & O_RDWR) res |= 2;

	return res;
}

/***
 * Long filename to 8+3 transformation.
 * The extensions, if exists in the original filename, are only shortened
 * and never appended with anything due to the filename extension driven
 * file type recognition posibility used by nearly all desktop programs.
 *
 * The translation rules are:
 *   The filename that has no extension and:
 *      - is max 11 chars long is splited to the filename and extension
 *        just by inserting a dot to the 8th position (example 1).
 *      - is longer than 11 chars is shortened to 8 chars and appended
 *        with the hashcode extension (~XX... example 2).
 *   The filename is over 8 chars:
 *      the filename is shortened to 5 and appended with the hashcode put
 *      into the name part (not the extention... example 3). The extension
 *      shortend to max 3 chars and appended too.
 *   The extension is over 3 chars long:
 *      The filename is appended with the hashcode and the extension is
 *      shortened to max 3 chars (example 4).
 *
 * Examples:
 *   1. longnamett        -> longname.tt
 *   2. longfilename      -> longfile.~XX
 *   3. longfilename.ext  -> longf~XX.ext
 *   4. file.html         -> file~XX.htm
 *
 * @param dest   The buffer to put the filename (max 12 char).
 * @param source The source filename string.
 *
 **/
void ExtFs::transformFileName( char* dest, const char* source )
{
#ifdef DEBUG_FILENAMETRANSFORMATION
	D(bug("MetaDOS: transformFileName(\"%s\")...", source));
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
	ssize_t extLen  = ( nameLen == len ) ? 0 : len - nameLen - 1;

	// copy the name... (max 12 chars due to the buffer length limitation)
	strncpy(dest, source, 12);
	dest[12] = '\0';

#ifdef DEBUG_FILENAMETRANSFORMATION
	D(bug("MetaDOS: transformFileName:... nameLen = %i, extLen = %i", nameLen, extLen));
#endif

	if ( nameLen > 8 || extLen > 3 )
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
			if ( nameLen < 12 ) {
				// filename is max 11 chars long and no extension
				// -> insert the . char into the name after the 8th char
				nameLen = 8;
				extLen = 3;
				dot = (char*)source + nameLen;
			} else {
				// filename is longer than 8+3 and has no extension
				// -> put the hash string as the file extension
				nameLen = 8;
				extLen = 3;
				dot = hashStr;
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
	D(bug("MetaDOS: /transformFileName(\"%s\") -> \"%s\"", source, dest));
#endif
}


bool ExtFs::getHostFileName( char* result, ExtDrive* drv, char* pathName, const char* name )
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
				D(bug("MetaDOS: getHostFileName dopendir(%s) failed.", pathName));
#endif
				goto lbl_final;  // should never happen
			}

			while ( true ) {
				if ((dirEntry = readdir( dh )) == NULL) {
#ifdef DEBUG_FILENAMETRANSFORMATION
					D(bug("MetaDOS: getHostFileName dreaddir: no more files."));
#endif
					nonexisting = true;
					goto lbl_final;
				}

				if ( !drv || drv->halfSensitive )
					if ( ! strcasecmp( name, dirEntry->d_name ) ) {
						finalName = dirEntry->d_name;
#ifdef DEBUG_FILENAMETRANSFORMATION
						D(bug("MetaDOS: getHostFileName found final file."));
#endif
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


ExtFs::ExtDrive* ExtFs::getDrive( const char* pathName )
{
	ExtDrive *drv = NULL;

	if ( pathName[0] != '\0' && pathName[1] == ':' ) {
		D2(bug("MetaDOS: getDrive '%c'", pathName[0]));
		drv = &drives[ toupper( pathName[0] ) - 'A' ];
	}

	if ( drv == NULL || drv->rootPath == NULL ) {
		D2(bug("MetaDOS: getDrive fail"));
		drv = &drives[0];
	}

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
	while ( (tp = strpbrk( n, "\\/" )) != NULL ) {
		char sep = *tp;
		*tp = '\0';

		getHostFileName( ffileName, drv, fpathName, n );
		ffileName += strlen( ffileName );
		*ffileName++ = '/';

		*tp = sep;
		n = tp+1;
	}

	getHostFileName( ffileName, drv, fpathName, n );
}


/*
 * build a complete linux filepath
 * --> fs    something like a handle to a known file
 *     name  a postfix to the path of fs or NULL, if no postfix
 *     buf   buffer for the complete filepath or NULL if static one
 *           should be used.
 * <-- complete filepath or NULL if error
 */
char *ExtFs::cookie2Pathname( ExtFs::XfsFsFile *fs, const char *name, char *buf )
{
    static char sbuf[MAXPATHNAMELEN]; /* FIXME: size should told by unix */

    if (!buf)
        buf = sbuf;
    if (!fs)
    {
        /* we are at root */
        if (!name)
            return NULL;
        else
            strcpy(buf, name);
    }
    else
    {
        char *h;
        if (!cookie2Pathname(fs->parent, fs->name, buf))
            return NULL;
        if (name && *name)
        {
            if ((h = strchr(buf, '\0'))[-1] != '/')
                *h++ = '/';

			*h = '\0';
			getHostFileName( h, NULL, buf, name );
        }
    }
    return buf;
}



int32 ExtFs::Dfree_(char *fpathName, uint32 diskinfop )
{

#ifdef HAVE_SYS_STATVFS_H
	struct statvfs buff;
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
	/* ULONG b_total   */  WriteInt32( diskinfop +  4, buff.f_blocks );
	/* ULONG b_secsize */  WriteInt32( diskinfop +  8, buff.f_bsize /* not 512 according to stonx_fs */ );
	/* ULONG b_clsize  */  WriteInt32( diskinfop + 12, 1 );

	return TOS_E_OK;
}

int32 ExtFs::Dfree(LogicalDev *ldp, char *pathName, ExtFile *fp, uint32 diskinfop, int16 drive )
{
	char fpathName[MAXPATHNAMELEN];
	convertPathA2F( fpathName, pathName );

	D(bug("MetaDOS: Dfree (drive = %d,%s,%s)", 'A' + drive, pathName, fpathName));

	return Dfree_( fpathName, diskinfop );
}

int32 ExtFs::xfs_dfree( XfsCookie *dir, uint32 buf )
{
	char fpathName[MAXPATHNAMELEN];
	cookie2Pathname( dir->index, NULL, fpathName );

	D(bug("    XFS: xfs_dfree (%s)", fpathName));

	return Dfree_( fpathName, buf );
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

int32 ExtFs::xfs_mkdir( XfsCookie *dir, char *name, uint16 mode )
{
	char fname[2048];
	a2fstrcpy( fname, (uint8*)name );

	char fpathName[MAXPATHNAMELEN];
	cookie2Pathname( dir->index, fname, fpathName );

	if ( mkdir( (char*)fpathName, mode ) )
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


int32 ExtFs::xfs_rmdir( XfsCookie *dir, char *name )
{
	char fname[2048];
	a2fstrcpy( fname, (uint8*)name );

	char pathName[MAXPATHNAMELEN];
	cookie2Pathname( dir->index, fname, pathName );

	char fpathName[MAXPATHNAMELEN];
	convertPathA2F( fpathName, pathName, "" ); // convert the fname into the hostfs form

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


/*
 * Check if the path is valid (if folders of the filename exist)
 * return true if stat(folders) was OK, else otherwise
 */
bool ExtFs::isPathValid(const char *fileName)
{
	char *path = strdup(fileName);
	char *end = strrchr(path, '/');
	if (end != NULL)
		*end = '\0';
	D(bug("Checking folder validity of path '%s'", path));
	if (*path) {
		struct stat statBuf;
		if (int staterr = stat(path, &statBuf) < 0) {
			D(bug("stat(%s) returns %d, errno=%d", path, staterr, errno));
			return false;	// path invalid
		}
	}
	return true;
}

int32 ExtFs::Fcreate(LogicalDev *ldp, char *pathName, ExtFile *fp, const char *pn, int16 attr)
{
	char fpathName[MAXPATHNAMELEN];
	convertPathA2F( fpathName, pathName );

	D(bug("MetaDOS: Fcreate (%s,%s,%d)", pathName, fpathName, attr));

#ifndef USE_CREAT_INSTEAD_OF_OPEN

	return Fopen_( (char*)fpathName,
				   O_CREAT|O_WRONLY|O_TRUNC,
				   S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH,
				   fp );

#else

	int fd = creat( pathName, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH );
	if (fd < 0) {
		if (! isPathValid( pathName ))
			return TOS_EPTHNF;
		else
			return unix2toserrno(errno,TOS_EFILNF);
	}

	fp->flags   = flags2st(O_CREAT|O_WRONLY|O_TRUNC);
	fp->offset = 0;
	fp->hostfd = fd;

	return TOS_E_OK;

#endif
}


int32 ExtFs::xfs_creat( XfsCookie *dir, char *name, uint16 mode, int16 flags, XfsCookie *fc )
{
	char fname[2048];
	a2fstrcpy( fname, (uint8*)name );

	char pathName[MAXPATHNAMELEN];
	cookie2Pathname(dir->index,fname,pathName); // get the cookie filename

	char fpathName[MAXPATHNAMELEN];
	convertPathA2F( fpathName, pathName, "" ); // convert the fname into the hostfs form (check the 8+3 file existence)

	int fd = open( fpathName, O_CREAT|O_WRONLY|O_TRUNC
#ifdef O_BINARY
					| O_BINARY
#endif
					, mode );
	if (fd < 0)
		return unix2toserrno(errno,TOS_EFILNF);
	close( fd );

	XfsFsFile *newFsFile = new XfsFsFile();
	newFsFile->name = strdup( fname );
    newFsFile->refCount = 1;
    newFsFile->childCount = 0;
    newFsFile->parent = dir->index;
    dir->index->childCount++;

    *fc = *dir;
    fc->index = newFsFile;

	return TOS_E_OK;
}


int32 ExtFs::Fopen_( const char* pathName, int flags, int mode_, ExtFile *fp )
{
	int fd = open( pathName, flags
#ifdef O_BINARY
					| O_BINARY
#endif
					, mode_ );
	if (fd < 0) {
		if (! isPathValid( pathName ))
			return TOS_EPTHNF;
		else
			return unix2toserrno(errno,TOS_EFILNF);
	}

	fp->flags   = flags2st(flags);
	fp->offset = 0;
	fp->hostfd = fd;

	D(bug("  Fopen (fd = %ld)", fp->hostfd));

	return TOS_E_OK;
}

int32 ExtFs::Fopen(LogicalDev *ldp, char *pathName, ExtFile *fp, const char *pn, int16 flags)
{
	char fpathName[MAXPATHNAMELEN];
	convertPathA2F( fpathName, pathName );

	D(bug("MetaDOS: Fopen (%s,%s,%d)", pathName, fpathName, flags));

	struct stat statBuf;
	if ( !stat(fpathName, &statBuf) )
		if ( S_ISDIR(statBuf.st_mode) )
			return TOS_EFILNF;

	return Fopen_( (char*)fpathName, st2flags(flags), 0, fp );
}


int32 ExtFs::xfs_dev_open(ExtFile *fp)
{
	char fpathName[MAXPATHNAMELEN];
	cookie2Pathname(fp->fc.index, NULL, fpathName);

	D(bug("    XFS: dev_open (%s,%d)", fpathName, fp->flags));

	return Fopen_( (char*)fpathName, st2flags(fp->flags), 0, fp );
}


int32 ExtFs::Fclose(LogicalDev *ldp, char *pathName, ExtFile *fp, int16 handle)
{
	D(bug("MetaDOS: Fclose (%ld, %d)", fp->hostfd, handle));

	if ( fp->links <= 0 )
		if ( close( fp->hostfd ) )
			return unix2toserrno(errno,TOS_EIO);

	return TOS_E_OK;
}


#define FRDWR_BUFFER_LENGTH	8192

int32 ExtFs::Fread(LogicalDev *ldp, char *pathName, ExtFile *fp, int16 handle, uint32 count, void *buffer)
{
	uint8 fBuff[ FRDWR_BUFFER_LENGTH ];
	uint8 *destBuff = (uint8*)buffer;
	ssize_t readCount = 0;
	ssize_t toRead = count;
	ssize_t toReadNow;

	D(bug("MetaDOS: Fread (%d,%d)", handle, count));

	while ( toRead > 0 ) {
		toReadNow = ( toRead > FRDWR_BUFFER_LENGTH ) ? FRDWR_BUFFER_LENGTH : toRead;
		readCount = read( fp->hostfd, fBuff, toReadNow );
		if ( readCount <= 0 )
			break;

		fp->offset += readCount;
		f2amemcpy( destBuff, fBuff, readCount );
		destBuff += readCount;
		toRead -= readCount;
	}

	D(bug("MetaDOS: Fread readCount (%d)", count - toRead));
	if ( readCount < 0 )
		return unix2toserrno(errno,TOS_EINTRN);

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
		writeCount = write( fp->hostfd, fBuff, toWriteNow );


		if ( writeCount <= 0 )
			break;

		fp->offset += writeCount;
		sourceBuff += writeCount;
		toWrite -= writeCount;
	}

	D(bug("MetaDOS: Fwrite writeCount (%d)", count - toWrite));
	if ( writeCount < 0 )
		return unix2toserrno(errno,TOS_EINTRN);

	return count - toWrite;
}


int32 ExtFs::Fdelete(LogicalDev *ldp, char *pathName, ExtFile *fp,
					 const char *pn)
{
	char fpathName[MAXPATHNAMELEN];
	convertPathA2F( fpathName, pathName );

	D(bug("MetaDOS: Fdelete (%s,%s)", pathName, fpathName));

	if ( unlink( fpathName ) )
		return unix2toserrno(errno,TOS_EFILNF);

	return TOS_E_OK;
}


int32 ExtFs::xfs_remove( XfsCookie *dir, char *name )
{
	char fname[2048];
	a2fstrcpy( fname, (uint8*)name );

	char pathName[MAXPATHNAMELEN];
	cookie2Pathname(dir->index,fname,pathName); // get the cookie filename

	char fpathName[MAXPATHNAMELEN];
	convertPathA2F( fpathName, pathName, "" ); // convert the fname into the hostfs form (check the 8+3 file existence)

	if ( unlink( fpathName ) )
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

	off_t newoff = lseek( fp->hostfd, offset, whence);

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
		D(bug("MetaDOS: Fsfirst no more directories!! (ARAnyM specific error)"));
		return TOS_ENHNDL; // FIXME? No more dir handles (ARAnyM specific)
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

	if ( ! fs_pathName[ dta->ds_index ] )
       		// if the mask didn't contain wildcads then the path is NULL
		return TOS_ENMFIL;
	
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

int32 ExtFs::xfs_rename( XfsCookie *olddir, char *oldname, XfsCookie *newdir, char *newname )
{
	char foldname[2048];
	char fnewname[2048];
	a2fstrcpy( foldname, (uint8*)oldname );
	a2fstrcpy( fnewname, (uint8*)newname );

	char fpathName[MAXPATHNAMELEN];
	char fnewPathName[MAXPATHNAMELEN];
	cookie2Pathname( olddir->index, foldname, fpathName );
	cookie2Pathname( newdir->index, fnewname, fnewPathName );

	if ( rename( fpathName, fnewPathName ) )
		return unix2toserrno(errno,TOS_EFILNF);

	return TOS_E_OK;
}


int32 ExtFs::Fdatime_( char *fpathName, ExtFile *fp, uint32 *datetimep, int16 wflag)
{
	D(bug("MetaDOS: Fdatime (%s)", fpathName));
	struct stat statBuf;

	if ( stat(fpathName, &statBuf) )
		return unix2toserrno(errno,TOS_EFILNF);

	uint32 datetime = ReadInt32( (uint32)datetimep );
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

	datetime =
		( time2dos(statBuf.st_mtime) << 16 ) | date2dos(statBuf.st_mtime);
	WriteInt32( (uint32)datetimep, datetime );

	return TOS_E_OK; //EBADRQ;
}

int32 ExtFs::Fdatime( LogicalDev *ldp, char *pathName, ExtFile *fp, uint32 *datetimep, int16 handle, int16 wflag)
{
	char fpathName[MAXPATHNAMELEN];
	convertPathA2F( fpathName, pathName );

	return Fdatime_( fpathName, fp, datetimep, wflag );
}

int32 ExtFs::xfs_dev_datime( ExtFile *fp, uint32 *datetimep, int16 wflag)
{
	char fpathName[MAXPATHNAMELEN];
	cookie2Pathname( fp->fc.index, NULL, fpathName );

	return Fdatime_( fpathName, fp, datetimep, wflag );
}


int32 ExtFs::Fcntl( LogicalDev *ldp, char *pathName, ExtFile *fp, int16 handle, void *arg, int16 cmd)
{
	D(bug("MetaDOS: Fcntl (NOT IMPLEMENTED!!!)"));
	return TOS_EINVFN;
}


int32 ExtFs::Dpathconf_( char *fpathName, int16 which, ExtDrive *drv )
{
	int	 oldErrno = errno;
#ifdef HAVE_SYS_STATVFS_H
	struct statvfs buf;
#else
	struct statfs buf;
#endif

	D(bug("MetaDOS: Dpathconf (%s,%d)", fpathName, which));

#ifdef HAVE_SYS_STATVFS_H
	if ( statvfs(fpathName, &buf) )
#else
	if ( statfs(fpathName, &buf) )
#endif
		return unix2toserrno(errno,TOS_EFILNF);

	switch (which) {
		case -1:
			return 7;  // maximal which value

		case 0:	  // DP_IOPEN
			return 0x7fffffffL; // unlimited

		case 1:	{ // DP_MAXLINKS
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
# if (defined(OS_openbsd) || defined(OS_freebsd) || defined(OS_netbsd))
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

		case 6:	{ // DP_CASE
			if ( drv )
				return drv->halfSensitive ? 2 : 0; // full case sensitive
			else
				return 0;
		}
		case 7:	  // D_XATTRMODE
			return 0x0ff0001fL;	 // only the archive bit is not recognised in the Fxattr

#if FIXME
// need to find MetaDOS working values for this (Thing refresh window)
		case 8:	  // DP_XATTR
			return 0x00000ffbL;	 // rdev is not used
#endif

		case 9:	  // DP_VOLNAMEMAX
			return 0;

		default:
			return TOS_EINVFN;
	}
	return TOS_EINVFN;
}


int32 ExtFs::Dpathconf( LogicalDev *ldp, char *pathName, ExtFile *fp, const char* pn, int16 which )
{
	D(bug("MetaDOS: Dpathconf (%s,%d)", pathName, which));

	char fpathName[MAXPATHNAMELEN];
	convertPathA2F( fpathName, pathName );

	ExtDrive *drv = getDrive( pathName );

	return Dpathconf_( fpathName, which, drv );
}


int32 ExtFs::xfs_pathconf( XfsCookie *fc, int16 which )
{
	char fpathName[MAXPATHNAMELEN];
	cookie2Pathname(fc->index, NULL, fpathName);

	return Dpathconf_( fpathName, which, NULL );
}


int32 ExtFs::Dopendir_( char *fpathName, ExtDir *dirh, int16 flags)
{
	dirh->dir = opendir( fpathName );
	if ( dirh->dir == NULL ) {
		freeDirIndex( dirh->pathIndex, fs_pathName );
		return unix2toserrno(errno,TOS_EPTHNF);
	}

	return TOS_E_OK;
}

int32 ExtFs::Dopendir( LogicalDev *ldp, char *pathName, ExtDir *dirh, const char* pn, int16 flags)
{
	if ( ( dirh->pathIndex = getFreeDirIndex( fs_pathName ) ) == -1 ) {
		D(bug("MetaDOS: Dopendir no more directories!! (ARAnyM specific error)"));
		return TOS_ENHNDL; // FIXME? No more dir handles (ARAnyM specific)
	}

	convertPathA2F( fs_pathName[ dirh->pathIndex ], pathName );

	D(bug("MetaDOS: Dopendir (%s,%s,%d)", pathName, fs_pathName[ dirh->pathIndex ], flags));

	dirh->flags = flags;
	dirh->fc.dev = toupper(pathName[0]) - 'A'; // FIXME
	return Dopendir_( fs_pathName[ dirh->pathIndex ], dirh, flags );
}

int32 ExtFs::xfs_opendir( XfsDir *dirh, uint16 flags )
{
	if ( ( dirh->pathIndex = getFreeDirIndex( fs_pathName ) ) == -1 ) {
		D(bug("MetaDOS: Dopendir no more directories!! (ARAnyM specific error)"));
		return TOS_ENHNDL; // FIXME? No more dir handles (ARAnyM specific)
	}

	cookie2Pathname(dirh->fc.index, NULL, fs_pathName[ dirh->pathIndex ]);

	D(bug("    XFS: opendir (%s,%d)", fs_pathName[ dirh->pathIndex ], flags));

	dirh->flags = flags;
	dirh->index = 0;
	return Dopendir_( fs_pathName[ dirh->pathIndex ], dirh, flags );
}



int32 ExtFs::Dclosedir( ExtDir *dirh )
{
	freeDirIndex( dirh->pathIndex, fs_pathName );

	if ( closedir( dirh->dir ) )
		return unix2toserrno(errno,TOS_EPTHNF);

	return TOS_E_OK;
}


int32 ExtFs::Dreaddir( LogicalDev *ldp, char *pathName, ExtDir *dirh,
					   int16 len, int32 dirhandle, char* buff )
{
	struct dirent *dirEntry;

	if ((void*)(dirEntry = readdir( dirh->dir )) == NULL)
		return unix2toserrno(errno,TOS_ENMFIL);

	if ( dirh->flags == 0 ) {
		if ( (uint16)len < strlen( dirEntry->d_name ) + 4 )
			return TOS_ERANGE;

		WriteInt32( (uint32)buff, dirEntry->d_ino );
		f2astrcpy( (uint8*)buff + 4, (uint8*)dirEntry->d_name );

		D(bug("MetaDOS: Dreaddir (%s,%s)", fs_pathName[ dirh->pathIndex ], (char*)dirEntry->d_name ));
	} else {
		char truncFileName[MAXPATHNAMELEN];
		transformFileName( truncFileName, (char*)dirEntry->d_name );

		if ( (uint16)len < strlen( truncFileName ) )
			return TOS_ERANGE;

		f2astrcpy( (uint8*)buff, (uint8*)truncFileName );
		D(bug("MetaDOS: Dreaddir (%s,%s)", fs_pathName[ dirh->pathIndex ], (char*)truncFileName ));
	}

	return TOS_E_OK;
}


int32 ExtFs::Dxreaddir_( char *fpathName, ExtDir *dirh,
						 int16 len, char* buff, uint32 xattrp, uint32 xretp )
{
	int32 result = ExtFs::Dreaddir( NULL, fpathName, dirh, len, 0, buff );
	if ( result != 0 )
		return result;

	ssize_t length = strlen( fpathName ) - 1;
	if ( fpathName[ length++ ] != '/' )
		fpathName[ length++ ] = '/';
	a2fstrcpy( &fpathName[ length ], (uint8*)&buff[ dirh->flags == 0 ? 4 : 0 ] );

	WriteInt32( xretp, Fxattr_( NULL, fpathName, 1, xattrp ) ); // FIXME: retp should be byref!
	fpathName[ length ] = '\0';

	return result;
}

int32 ExtFs::Dxreaddir( LogicalDev *ldp, char *pathName, ExtDir *dirh,
						 int16 len, int32 dirhandle, char* buff, uint32 xattrp, uint32 xretp )
{
	return Dxreaddir_( fs_pathName[ dirh->pathIndex ], dirh, len, buff, xattrp, xretp );
}


int32 ExtFs::xfs_readdir( ExtDir *dirh, char* buff, int16 len, XfsCookie *fc )
{
	struct dirent *dirEntry;


#if 0
	if ((void*)(dirEntry = readdir( dirh->dir )) == NULL)
		return unix2toserrno(errno,TOS_ENMFIL);
#else
    do {
		if ((void*)(dirEntry = readdir( dirh->dir )) == NULL)
			return unix2toserrno(errno,TOS_ENMFIL);
    } while ( !dirh->fc.index->parent &&
			  ( dirEntry->d_name[0] == '.' &&
				( !dirEntry->d_name[1] ||
				  ( dirEntry->d_name[1] == '.' && !dirEntry->d_name[2] ) ) ) );
#endif

	XfsFsFile *newFsFile = new XfsFsFile();
	newFsFile->name = strdup( dirEntry->d_name );

	if ( dirh->flags == 0 ) {
		if ( (uint16)len < strlen( dirEntry->d_name ) + 4 )
			return TOS_ERANGE;

		WriteInt32( (uint32)buff, dirEntry->d_ino );
		f2astrcpy( (uint8*)buff + 4, (uint8*)dirEntry->d_name );

		D(bug("    XFS: readdir (%s)", (char*)dirEntry->d_name ));
	} else {
		char truncFileName[MAXPATHNAMELEN];
		transformFileName( truncFileName, (char*)dirEntry->d_name );

		if ( (uint16)len < strlen( truncFileName ) )
			return TOS_ERANGE;

		f2astrcpy( (uint8*)buff, (uint8*)truncFileName );
		D(bug("    XFS: readdir (%s)", (char*)truncFileName ));
	}

	dirh->index++;
	dirh->fc.index->childCount++;
	newFsFile->parent = dirh->fc.index;
    newFsFile->refCount = 1;
    newFsFile->childCount = 0;

	fc->xfs = mint_fs_drv;
    fc->dev = dirh->fc.dev;
	fc->aux = 0;
    fc->index = newFsFile;

	return TOS_E_OK;
}


int32 ExtFs::Drewinddir( ExtDir *dirh )
{
	rewinddir( dirh->dir );
	dirh->index = 0;
	return TOS_E_OK;
}


int32 ExtFs::Fxattr_( LogicalDev *ldp, char *fpathName, int16 flag, uint32 xattrp )
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
	/* UWORD mode	   */  WriteInt16( xattrp     , statmode2xattrmode(statBuf.st_mode));
	/* LONG	 index	   */  WriteInt32( xattrp +  2, statBuf.st_ino );
	/* UWORD dev	   */  WriteInt16( xattrp +  6, statBuf.st_dev );	 // FIXME: this is Linux's one
	/* UWORD reserved1 */  WriteInt16( xattrp +  8, 0 );
	/* UWORD nlink	   */  WriteInt16( xattrp + 10, statBuf.st_nlink );
	/* UWORD uid	   */  WriteInt16( xattrp + 12, statBuf.st_uid );	 // FIXME: this is Linux's one
	/* UWORD gid	   */  WriteInt16( xattrp + 14, statBuf.st_gid );	 // FIXME: this is Linux's one
	/* LONG	 size	   */  WriteInt32( xattrp + 16, statBuf.st_size );
	/* LONG	 blksize   */  WriteInt32( xattrp + 20, statBuf.st_blksize );
	/* LONG	 nblocks   */  WriteInt32( xattrp + 24, statBuf.st_blocks );
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

	D(bug("MetaDOS: Fxattr mtime %#04x, mdate %#04x", ReadInt16(xattrp + 28), ReadInt16(xattrp + 30)));

	return TOS_E_OK;
}

int32 ExtFs::Fxattr( LogicalDev *ldp, char *pathName, ExtDta *dta,
					 int16 flag, const char* pn, uint32 xattrp )
{
	char fpathName[MAXPATHNAMELEN];
	convertPathA2F( fpathName, pathName );

	return Fxattr_( ldp, fpathName, flag, xattrp );
}

int32 ExtFs::xfs_getxattr( XfsCookie *fc, uint32 xattrp )
{
	char fpathName[MAXPATHNAMELEN];
	cookie2Pathname(fc->index, NULL, fpathName);

	return Fxattr_( NULL, fpathName, 1 /* lstat */, xattrp );
}


int32 ExtFs::xfs_root( uint16 dev, XfsCookie *fc )
{
	D2(bug( "root:\n"
		   "  dev    = %#04x\n"
		   "  devnum = %#04x\n",
		   dev, mint_fs_devnum));

	if ( dev != mint_fs_devnum )
		return TOS_EDRIVE;

	fc->xfs = mint_fs_drv;
	fc->dev = dev;
	fc->aux = 0;
	fc->index = new XfsFsFile();

	fc->index->parent = NULL;
	fc->index->name = "/";
	fc->index->refCount = 1;
	fc->index->childCount = 0;

	D2(bug( "root result:\n"
		   "  fs    = %08lx\n"
		   "  dev   = %04x\n"
		   "  aux   = %04x\n"
		   "  index = %08lx\n",
		   fc->xfs, fc->dev, fc->aux, fc->index));

	return TOS_E_OK;
}


int32 ExtFs::xfs_getdev( XfsCookie *fc, int32 *devspecial )
{
    WriteInt32((uint32)devspecial,0); /* reserved */
    return (int32)mint_fs_devdrv;
}


int32 ExtFs::xfs_readlink( XfsCookie *dir, char *buf, int16 len )
{
	char fpathName[MAXPATHNAMELEN];
	cookie2Pathname(dir->index,NULL,fpathName); // get the cookie filename

	char fbuf[MAXPATHNAMELEN];
	int rv;
    if ((rv=readlink(fpathName,fbuf,len-1)) < 0)
		return unix2toserrno( errno, TOS_EFILNF );

	// FIXME: the link destination should be scanned if pointing to some
	//	      of the native file cookies and then should be adjusted.

	f2astrcpy( (uint8*)buf, (uint8*)fbuf );
	return TOS_E_OK;
}


void ExtFs::xfs_freefs( XfsFsFile *fs )
{
    D2(bug( "freefs:\n"
		 "  fs = %08lx\n"
		 "    parent   = %08lx\n"
		 "    name     = \"%s\"\n"
		 "    usecnt   = %d\n"
		 "    childcnt = %d",
		 (long)fs,
		 (long)(fs->parent),
		 fs->name,
		 fs->refCount,
		 fs->childCount ));

    if ( !fs->refCount && !fs->childCount )	{
		D2(bug( "freefs: realfree" ));
		if ( fs->parent ) {
			fs->parent->childCount--;
			xfs_freefs( fs->parent );
			free( fs->name );
		}
		delete fs;
	} else {
		D2(bug( "freefs: notfree" ));
	}
}


int32 ExtFs::xfs_lookup( XfsCookie *dir, char *name, XfsCookie *fc )
{
	char fname[2048];
	a2fstrcpy( fname, (uint8*)name );

	D(bug( "    XFS: lookup: %s", fname ));

    XfsFsFile *newFsFile;

    if ( !fname || !*fname || (*fname == '.' && !fname[1]) ) {
		newFsFile = dir->index;
		newFsFile->refCount++;
	} else if ( *fname == '.' && fname[1] == '.' && !fname[2] ) {
		if ( !dir->index->parent ) {
			D(bug( "lookup to \"..\" at root" ));
			return TOS_EMOUNT;
		}
		newFsFile = dir->index->parent;
		newFsFile->refCount++;
	} else {
		if ( (newFsFile = new XfsFsFile()) == NULL ) {
			D(bug( "lookup: malloc() failed!" ));
			return TOS_ENSMEM;
		}

		newFsFile->parent = dir->index;

		char pathName[MAXPATHNAMELEN];
		cookie2Pathname(dir->index,fname,pathName); // get the cookie filename

		char fpathName[MAXPATHNAMELEN];
		convertPathA2F( fpathName, pathName, "" ); // convert the fname into the hostfs form

		struct stat statBuf;

		D(bug( "    XFS: lookup stat: %s", fpathName ));

		if ( lstat( fpathName, &statBuf ) )
			return unix2toserrno( errno, TOS_EFILNF );

		newFsFile->name = strdup(fname);
		newFsFile->refCount = 1;
		newFsFile->childCount = 0;
		dir->index->childCount++; /* same as: new->parent->childcnt++ */
	}

    *fc = *dir;
    fc->index = newFsFile;

	return TOS_E_OK;
}


int32 ExtFs::xfs_getname( XfsCookie *relto, XfsCookie *dir, char *pathName, int16 size )
{
	char base[MAXPATHNAMELEN];
	cookie2Pathname(relto->index,NULL,base); // get the cookie filename

	char dirBuff[MAXPATHNAMELEN];
	char *dirPath = dirBuff;
	cookie2Pathname(dir->index,NULL,dirBuff); // get the cookie filename

	char fpathName[MAXPATHNAMELEN];

    D2(bug( "    XFS:       getname: relto = \"%s\"", base ));
    size_t baselength = strlen(base);
    if ( baselength && base[baselength-1] == '/' ) {
		baselength--;
		base[baselength] = '\0';
		D2(bug( "    XFS:       getname: fixed relto = \"%s\"", base ));
    }

    D2(bug( "    XFS:       getname: dir = \"%s\"", dirPath ));
    size_t dirlength = strlen(dirPath);
    if ( dirlength < baselength ||
		 strncmp( dirPath, base, baselength ) ) {
		/* dir is not a sub...directory of relto, so use absolute */
		/* FIXME: try to use a relative path, if relativ is smaller */
		D2(bug( "    XFS:       getname: dir not relativ to relto!" ));
    } else {
		/* delete "same"-Part */
		dirPath += baselength;
    }
    D2(bug( "    XFS:       getname: relativ dir to relto = \"%s\"", dirPath ));

    /* copy and unix2dosname */
	char *pfpathName = fpathName;
    for ( ; *dirPath && size > 0; size--, dirPath++ )
		if ( *dirPath == '/' )
			*pfpathName++ = '\\';
		else
			*pfpathName++ = *dirPath;

    if ( !size ) {
		D2(bug( "    XFS:       getname: Relative dir is to long for buffer!" ));
		return TOS_ERANGE;
    } else {
		*pfpathName = '\0';
		D(bug( "    XFS:       getname result = \"%s\"", fpathName ));

		f2astrcpy( (uint8*)pathName, (uint8*)fpathName );
		return TOS_E_OK;
    }
}


int32 ExtFs::xfs_dupcookie( XfsCookie *newCook, XfsCookie *oldCook )
{
    XfsFsFile *fs = new XfsFsFile();

    if ( ( fs->parent = oldCook->index->parent ) != NULL ) {
		if ( (fs->name = strdup(oldCook->index->name)) == NULL ) {
			D(bug( "    XFS:       dupcookie: strdup() failed!" ));
			delete fs;
			return TOS_ENSMEM;
		}
		fs->parent->childCount++;
	} else
		fs->name = "/";
    fs->refCount = 1;
    fs->childCount = 0; /* don't heritate childs! */

    *newCook = *oldCook;
    newCook->index = fs;

	return TOS_E_OK;
}


int32 ExtFs::xfs_release( XfsCookie *fc )
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
			D(bug( "    XFS:       release: RELEASE OF UNUSED FILECOOKIE!" ));
			return TOS_EACCDN;
		}
    fc->index->refCount--;
    xfs_freefs( fc->index );

    return TOS_E_OK;
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

	// note: here it doesn't matter whether the stat() succeeds or not
	//       ... maybe some defaults when fails, but no real need.
	stat(fpathName, &statBuf);

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
 * Revision 1.44  2002/04/18 20:57:01  standa
 * Fsnext bug fix for LZHSHELL to be able to extract files to MetaDOS fs.
 *
 * Revision 1.43  2002/04/17 15:48:02  standa
 * The DP_* constant definitions for MiNTLib removed. These are really to be
 * placed into the mintlib header files. For ARAnyM there should exist some
 * copy of these defines and probably prefixed by e.g. TOS_ prefix?
 * This is planned to be done in the future.
 *
 * Revision 1.42  2002/04/17 15:44:17  standa
 * Patrice Mandin <pmandin@caramail.com> & STanda FreeMiNT compilation support
 * patch.
 *
 * Revision 1.41  2002/04/17 15:42:03  standa
 * JOYs fix modified a bit.
 *
 * Revision 1.40  2002/04/13 12:32:03  joy
 * Dpathconf(DP_XATTR) disabled - Thing is happy now
 *
 * Revision 1.39  2002/04/12 22:52:27  joy
 * AranymFS bug fixed - ST-Zip can unpack onto host fs now
 *
 * Revision 1.38  2002/04/04 19:13:51  standa
 * PureC patch for MetaDOS usage.
 * Upper case filenames to lower case conversion for new files on halfsensitive fs.
 *
 * Revision 1.37  2002/04/04 09:00:19  standa
 * The Fopen( dir ) fix.
 *
 * Revision 1.36  2002/03/27 16:14:39  standa
 * The JavaDoc documentation written into the extfs.cpp/transformFileName.
 *
 * Revision 1.35  2002/03/27 15:54:27  standa
 * Debug removed.
 *
 * Revision 1.34  2002/03/27 15:53:29  standa
 * The transformFileName() has been rewriten.
 *
 * Revision 1.33  2002/03/26 12:22:09  joy
 * filename conversion in transformFileName() partially rewritten. Now ST-Zip does not crash when browsing MetaDOS drive.
 *
 * Revision 1.32  2002/03/26 09:43:19  joy
 * the dest buf size must be at least 8+1+3+1=13 bytes long!
 *
 * Revision 1.31  2002/03/26 09:29:06  joy
 * transformFileName was writing long filenames to 8+3 TOS filename buffer. This is way wrong - you need to do all filename manipulation on a separate buffer and copy only the resulting 8+3 filename to the TOS fn buffer.
 *
 * Revision 1.30  2002/03/14 12:00:06  standa
 * ExtFs flags2st bugfix. Some Fcreate modifications (optional).
 *
 * Revision 1.29  2002/03/06 10:04:21  standa
 * xfs_getname implemented. cookie2PathName uses getHostFileName since now
 * -> case insensitive fs should work.
 *
 * Revision 1.28  2002/02/23 13:47:07  joy
 * open() needs O_BINARY to not mess with CR/LF conversion (OSes from Microsoft)
 *
 * Revision 1.27  2002/02/19 20:04:05  milan
 * src/ <-> CPU interaction cleaned
 * memory access cleaned
 *
 * Revision 1.26  2002/02/18 09:30:05  standa
 * Olivier Landemarre <Olivier.Landemarre@utbm.fr> && STanda extfs.cpp patch.
 * The statvfs stuff conditional compile fixed (no ifdef OS_linux, but OS_*bsd
 * used). The SDL_filesystem is damn needed here or ... ;(
 * XFS Debug logs made look nicer to me.
 *
 * Revision 1.25  2002/01/31 23:51:22  standa
 * The aranym.xfs for MiNT. Preliminary version.
 *
 * Revision 1.24  2002/01/31 19:37:47  milan
 * panicbug, cleaning
 *
 * Revision 1.23  2002/01/26 21:22:24  standa
 * Cleanup from no needed method arguments.
 *
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
