/////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2002  MandrakeSoft S.A.
//
//    MandrakeSoft S.A.
//    43, rue d'Aboukir
//    75002 Paris - France
//    http://www.linux-mandrake.com/
//    http://www.mandrakesoft.com/
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2 of the License, or (at your option) any later version.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with this library; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA


// These are the low-level CDROM functions which are called
// from 'harddrv.cc'.  They effect the OS specific functionality
// needed by the CDROM emulation in 'harddrv.cc'.  Mostly, just
// ioctl() calls and such.  Should be fairly easy to add support
// for your OS if it is not supported yet.

#include "sysdeps.h"
#include "emu_bochs.h"
#include "cdrom.h"
#define DEBUG 0
#include "debug.h"

#ifdef SUPPORT_CDROM
# include <cstdlib>
# include <cstdio>
# include <cerrno>

#define LOG_THIS /* no SMF tricks here, not needed */

extern "C" {
#include <errno.h>
}

#ifdef OS_linux
extern "C" {
#include <sys/ioctl.h>
#include <linux/cdrom.h>
// I use the framesize in non OS specific code too
#define BX_CD_FRAMESIZE CD_FRAMESIZE
}

#elif defined(__GNU__) || (defined(OS_cygwin) && !defined(_WIN32))
extern "C" {
#include <sys/ioctl.h>
#define BX_CD_FRAMESIZE 2048
#define CD_FRAMESIZE 2048
}

#elif defined(OS_solaris)
extern "C" {
#include <sys/types.h>
#include "stat_.h"
#include <sys/ioctl.h>
#include <sys/cdio.h>
#define BX_CD_FRAMESIZE CDROM_BLK_2048
}

#elif defined(OS_beos)
#include "cdrom_beos.h"
#define BX_CD_FRAMESIZE 2048

#elif (defined (OS_netbsd) || defined(OS_openbsd) || defined(OS_freebsd))
// OpenBSD pre version 2.7 may require extern "C" { } structure around
// all the includes, because the i386 sys/disklabel.h contains code which 
// c++ considers invalid.
#include <sys/file.h>
#include <sys/cdio.h>
#include <sys/disklabel.h>
#include <netinet/in.h>
#ifdef OS_openbsd
#include <sys/dkio.h>
#endif

// XXX
#define BX_CD_FRAMESIZE 2048
#define CD_FRAMESIZE	2048

#elif defined(OS_irix)
#define BX_CD_FRAMESIZE 2048
#define CD_FRAMESIZE    2048

#elif defined(OS_mint)
#define BX_CD_FRAMESIZE 2048
#define CD_FRAMESIZE    2048

#elif defined(OS_darwin)
// #include <dev/disk.h>
// dev/disk.h is deprecated since OS 10.3. Use sys/disk.h instead.
#include <sys/disk.h>
#include <paths.h>

#include <IOKit/IOKitLib.h>
#include <IOKit/IOBSD.h>
#include <IOKit/storage/IOCDMedia.h>
#include <IOKit/storage/IOMedia.h>
#include <IOKit/storage/IOCDTypes.h>
#include <CoreFoundation/CoreFoundation.h>

// These definitions were taken from mount_cd9660.c
// There are some similar definitions in IOCDTypes.h
// however there seems to be some dissagreement in
// the definition of CDTOC.length
struct _CDMSF {
	u_char   minute;
	u_char   second;
	u_char   frame;
};

#define MSF_TO_LBA(msf)		\
	(((((msf).minute * 60UL) + (msf).second) * 75UL) + (msf).frame - 150)

struct _CDTOC_Desc {
	u_char        session;
	u_char        ctrl_adr;  /* typed to be machine and compiler independent */
	u_char        tno;
	u_char        point;
	struct _CDMSF  address;
	u_char        zero;
	struct _CDMSF  p;
};

struct _CDTOC {
	u_short            length;  /* in native cpu endian */
	u_char             first_session;
	u_char             last_session;
	struct _CDTOC_Desc  trackdesc[1];
};

static kern_return_t FindEjectableCDMedia( io_iterator_t *mediaIterator, mach_port_t *masterPort );
static kern_return_t GetDeviceFilePath( io_iterator_t mediaIterator, char *deviceFilePath, CFIndex maxPathSize );
static struct _CDTOC * ReadTOC( const char * devpath );

static char CDDevicePath[ MAXPATHLEN ];

#define BX_CD_FRAMESIZE 2048
#define CD_FRAMESIZE	2048

#elif defined(_WIN32)
#include <windows.h>
#include <winioctl.h>
#include "aspi-win32.h"
#include "scsidefs.h"

DWORD (*GetASPI32SupportInfo)(void);
DWORD (*SendASPI32Command)(LPSRB);
BOOL  (*GetASPI32Buffer)(PASPI32BUFF);
BOOL  (*FreeASPI32Buffer)(PASPI32BUFF);
BOOL  (*TranslateASPI32Address)(PDWORD,PDWORD);
DWORD (*GetASPI32DLLVersion)(void);


static OSVERSIONINFO osinfo;
static BOOL isWindowsXP;
static BOOL bHaveDev;
static UINT cdromCount = 0;
static HINSTANCE hASPI = NULL;

#define BX_CD_FRAMESIZE 2048
#define CD_FRAMESIZE	2048

// READ_TOC_EX structure(s) and #defines

#define CDROM_READ_TOC_EX_FORMAT_TOC      0x00
#define CDROM_READ_TOC_EX_FORMAT_SESSION  0x01
#define CDROM_READ_TOC_EX_FORMAT_FULL_TOC 0x02
#define CDROM_READ_TOC_EX_FORMAT_PMA      0x03
#define CDROM_READ_TOC_EX_FORMAT_ATIP     0x04
#define CDROM_READ_TOC_EX_FORMAT_CDTEXT   0x05

#define IOCTL_CDROM_BASE              FILE_DEVICE_CD_ROM
#define IOCTL_CDROM_READ_TOC_EX       CTL_CODE(IOCTL_CDROM_BASE, 0x0015, METHOD_BUFFERED, FILE_READ_ACCESS)

typedef struct _CDROM_READ_TOC_EX {
    UCHAR Format    : 4;
    UCHAR Reserved1 : 3; // future expansion
    UCHAR Msf       : 1;
    UCHAR SessionTrack;
    UCHAR Reserved2;     // future expansion
    UCHAR Reserved3;     // future expansion
} CDROM_READ_TOC_EX, *PCDROM_READ_TOC_EX;

typedef struct _TRACK_DATA {
    UCHAR Reserved;
    UCHAR Control : 4;
    UCHAR Adr : 4;
    UCHAR TrackNumber;
    UCHAR Reserved1;
    UCHAR Address[4];
} TRACK_DATA, *PTRACK_DATA;

typedef struct _CDROM_TOC_SESSION_DATA {
    // Header
    UCHAR Length[2];  // add two bytes for this field
    UCHAR FirstCompleteSession;
    UCHAR LastCompleteSession;
    // One track, representing the first track
    // of the last finished session
    TRACK_DATA TrackData[1];
} CDROM_TOC_SESSION_DATA, *PCDROM_TOC_SESSION_DATA;

// End READ_TOC_EX structure(s) and #defines

#else // all others (Irix, Tru64)
#include <sys/types.h>
#include "stat_.h"
#include <sys/ioctl.h>
#define BX_CD_FRAMESIZE 2048
#define CD_FRAMESIZE 2048 
#endif

#ifdef OS_darwin
static kern_return_t FindEjectableCDMedia( io_iterator_t *mediaIterator,
					   mach_port_t *masterPort )
{
  kern_return_t     kernResult;
  CFMutableDictionaryRef     classesToMatch;
  kernResult = IOMasterPort( bootstrap_port, masterPort );
  if ( kernResult != KERN_SUCCESS )
    {
      fprintf ( stderr, "IOMasterPort returned %d\n", kernResult );
      return kernResult;
    }
  // CD media are instances of class kIOCDMediaClass.
  classesToMatch = IOServiceMatching( kIOCDMediaClass );
  if ( classesToMatch == NULL )
    fprintf ( stderr, "IOServiceMatching returned a NULL dictionary.\n" );
  else
    {
      // Each IOMedia object has a property with key kIOMediaEjectableKey
      // which is true if the media is indeed ejectable. So add property
      // to CFDictionary for matching.
      CFDictionarySetValue( classesToMatch,
                            CFSTR( kIOMediaEjectableKey ), kCFBooleanTrue );
    }
  kernResult = IOServiceGetMatchingServices( *masterPort,
                                             classesToMatch, mediaIterator );
  if ( (kernResult != KERN_SUCCESS) || (!*mediaIterator) ) 
    fprintf( stderr, "No ejectable CD media found.\n kernResult = %d\n", kernResult );

  return kernResult;
}


static kern_return_t GetDeviceFilePath( io_iterator_t mediaIterator,
					char *deviceFilePath, CFIndex maxPathSize )
{
  io_object_t nextMedia;
  kern_return_t kernResult = KERN_FAILURE;
  nextMedia = IOIteratorNext( mediaIterator );
  if ( !nextMedia )
    {
      *deviceFilePath = '\0';
    }
  else
    {
      CFTypeRef    deviceFilePathAsCFString;
      deviceFilePathAsCFString = IORegistryEntryCreateCFProperty(
                                                                 nextMedia, CFSTR( kIOBSDNameKey ),
                                                                 kCFAllocatorDefault, 0 );
      *deviceFilePath = '\0';
      if ( deviceFilePathAsCFString )
        {
          size_t devPathLength = strlen( _PATH_DEV );
          strcpy( deviceFilePath, _PATH_DEV );
          if ( CFStringGetCString( (const __CFString *) deviceFilePathAsCFString,
                                   deviceFilePath + devPathLength,
                                   maxPathSize - devPathLength,
                                   kCFStringEncodingASCII ) )
            {
              // fprintf( stderr, "BSD path: %s\n", deviceFilePath );
              kernResult = KERN_SUCCESS;
            }
          CFRelease( deviceFilePathAsCFString );
        }
    }
  IOObjectRelease( nextMedia );
  return kernResult;
}


static struct _CDTOC * ReadTOC( const char * devpath ) {

  struct _CDTOC * toc_p = NULL;
  io_iterator_t iterator = 0;
  io_registry_entry_t service = 0;
  CFDictionaryRef properties = 0;
  CFDataRef data = 0;
  mach_port_t port = 0;
  const char * devname;
  
  if (( devname = strrchr( devpath, '/' )) != NULL ) {
    ++devname;
  }
  else {
    devname = devpath;
  }

  if ( IOMasterPort(bootstrap_port, &port ) != KERN_SUCCESS ) {
    fprintf( stderr, "IOMasterPort failed\n" );
    goto Exit;
  }

  if ( IOServiceGetMatchingServices( port, IOBSDNameMatching( port, 0, devname ),
				     &iterator ) != KERN_SUCCESS ) {
    fprintf( stderr, "IOServiceGetMatchingServices failed\n" );
    goto Exit;
  }
  
  service = IOIteratorNext( iterator );

  IOObjectRelease( iterator );

  iterator = 0;

  while ( service && !IOObjectConformsTo( service, "IOCDMedia" )) {
    if ( IORegistryEntryGetParentIterator( service, kIOServicePlane,
					   &iterator ) != KERN_SUCCESS ) {
      fprintf( stderr, "IORegistryEntryGetParentIterator failed\n" );
      goto Exit;
    }

    IOObjectRelease( service );
    service = IOIteratorNext( iterator );
    IOObjectRelease( iterator );

  }

  if ( !service ) {
    fprintf( stderr, "CD media not found\n" );
    goto Exit;
  }

  if ( IORegistryEntryCreateCFProperties( service, (__CFDictionary **) &properties,
					  kCFAllocatorDefault,
					  kNilOptions ) != KERN_SUCCESS ) {
    fprintf( stderr, "IORegistryEntryGetParentIterator failed\n" );
    goto Exit;
  }

  data = (CFDataRef) CFDictionaryGetValue( properties, CFSTR(kIOCDMediaTOCKey) );
  if ( data == NULL ) {
    fprintf( stderr, "CFDictionaryGetValue failed\n" );
    goto Exit;
  }
  else {

    CFRange range;
    CFIndex buflen;

    buflen = CFDataGetLength( data ) + 1;
    range = CFRangeMake( 0, buflen );
    toc_p = (struct _CDTOC *) malloc( buflen );
    if ( toc_p == NULL ) {
      fprintf( stderr, "Out of memory\n" );
      goto Exit;
    }
    else {
      CFDataGetBytes( data, range, (unsigned char *) toc_p );
    }

    /*
    fprintf( stderr, "Table of contents\n length %d first %d last %d\n",
	    toc_p->length, toc_p->first_session, toc_p->last_session );
    */

    CFRelease( properties );

  }
  

 Exit:

  if ( service ) {
    IOObjectRelease( service );
  }

  return toc_p;

}
#endif

#ifdef _WIN32

bool ReadCDSector(unsigned int hid, unsigned int tid, unsigned int lun, unsigned long frame, unsigned char *buf, int bufsize)
{
	HANDLE hEventSRB;
	SRB_ExecSCSICmd srb;
	DWORD dwStatus;

	hEventSRB = CreateEvent(NULL, TRUE, FALSE, NULL);
	
	memset(&srb,0,sizeof(SRB_ExecSCSICmd));
	srb.SRB_Cmd        = SC_EXEC_SCSI_CMD;
	srb.SRB_HaId       = hid;
	srb.SRB_Target     = tid;
 	srb.SRB_Lun        = lun;
	srb.SRB_Flags      = SRB_DIR_IN | SRB_EVENT_NOTIFY;
	srb.SRB_SenseLen   = SENSE_LEN;
	srb.SRB_PostProc   = hEventSRB;
	srb.SRB_BufPointer = buf;
	srb.SRB_BufLen     = bufsize;
	srb.SRB_CDBLen     = 10;
	srb.CDBByte[0]     = SCSI_READ10;
	srb.CDBByte[2]     = (unsigned char) (frame>>24);
	srb.CDBByte[3]     = (unsigned char) (frame>>16);
	srb.CDBByte[4]     = (unsigned char) (frame>>8);
	srb.CDBByte[5]     = (unsigned char) (frame);
	srb.CDBByte[7]     = 0;
	srb.CDBByte[8]     = 1; /* read 1 frames */

	ResetEvent(hEventSRB);
	dwStatus = SendASPI32Command((SRB *)&srb);
	if(dwStatus == SS_PENDING) {
		WaitForSingleObject(hEventSRB, 100000);
	}
	CloseHandle(hEventSRB);
	return (srb.SRB_TargStat == STATUS_GOOD);
}

int GetCDCapacity(unsigned int hid, unsigned int tid, unsigned int lun)
{
	HANDLE hEventSRB;
	SRB_ExecSCSICmd srb;
	DWORD dwStatus;
	unsigned char buf[8];

	hEventSRB = CreateEvent(NULL, TRUE, FALSE, NULL);
	
	memset(&buf, 0, sizeof(buf));
	memset(&srb,0,sizeof(SRB_ExecSCSICmd));
	srb.SRB_Cmd        = SC_EXEC_SCSI_CMD;
	srb.SRB_HaId       = hid;
	srb.SRB_Target     = tid;
 	srb.SRB_Lun        = lun;
	srb.SRB_Flags      = SRB_DIR_IN | SRB_EVENT_NOTIFY;
	srb.SRB_SenseLen   = SENSE_LEN;
	srb.SRB_PostProc   = hEventSRB;
	srb.SRB_BufPointer = (unsigned char *)buf;
	srb.SRB_BufLen     = 8;
	srb.SRB_CDBLen     = 10;
	srb.CDBByte[0]     = SCSI_READCDCAP;
	srb.CDBByte[2]     = 0;
	srb.CDBByte[3]     = 0;
	srb.CDBByte[4]     = 0;
	srb.CDBByte[5]     = 0;
	srb.CDBByte[8]     = 0;

	ResetEvent(hEventSRB);
	dwStatus = SendASPI32Command((SRB *)&srb);
	if(dwStatus == SS_PENDING) {
		WaitForSingleObject(hEventSRB, 100000);
	}

	CloseHandle(hEventSRB);
	return ((buf[0] << 24) + (buf[1] << 16) + (buf[2] << 8) + buf[3]) * ((buf[4] << 24) + (buf[5] << 16) + (buf[6] << 8) + buf[7]);
}

#endif

cdrom_interface::cdrom_interface(char *dev)
{
  fd = -1; // File descriptor not yet allocated

  if ( dev == NULL )
    path = NULL;
  else {
    path = strdup(dev);
  }
  using_file=0;
#ifdef _WIN32
  bUseASPI = FALSE;
  osinfo.dwOSVersionInfoSize = sizeof(osinfo);
  GetVersionEx(&osinfo);
  isWindowsXP = (osinfo.dwMajorVersion >= 5) && (osinfo.dwMinorVersion >= 1);
#endif
}

void
cdrom_interface::init(void) {
}

cdrom_interface::~cdrom_interface(void)
{
#ifdef _WIN32
#else
	if (fd >= 0)
		close(fd);
#endif
	if (path)
		free(path);
}

  bool
cdrom_interface::insert_cdrom(char *dev)
{
  unsigned char buffer[BX_CD_FRAMESIZE];
  ssize_t ret;

  // Load CD-ROM. Returns false if CD is not ready.
  if (dev != NULL) path = strdup(dev);
#ifdef _WIN32
  char drive[256];
  if ( (path[1] == ':') && (strlen(path) == 2) )
  {
    if(osinfo.dwPlatformId == VER_PLATFORM_WIN32_NT) {
      // Use direct device access under windows NT/2k/XP

      // With all the backslashes it's hard to see, but to open D: drive 
      // the name would be: \\.\d:
      sprintf(drive, "\\\\.\\%s", path);
      D(bug("Using direct access for cdrom."));
      // This trick only works for Win2k and WinNT, so warn the user of that.
    } else {
      D(bug("Using ASPI for cdrom. Drive letters are unused yet."));
      bUseASPI = TRUE;
    }
  }
  else
  {
    strcpy(drive,path);
    using_file = 1;
    D(bug("Opening image file as a cd"));
  }
  if(bUseASPI) {
    DWORD d;
    UINT cdr, cnt, max;
    UINT i, j, k;
    SRB_HAInquiry sh;
    SRB_GDEVBlock sd;
    if (!hASPI) {
      hASPI = LoadLibrary("WNASPI32.DLL");
      if (hASPI) {
        SendASPI32Command    = (DWORD(*)(LPSRB))GetProcAddress( hASPI, "SendASPI32Command" );
        GetASPI32DLLVersion  = (DWORD(*)(void))GetProcAddress( hASPI, "GetASPI32DLLVersion" );
        GetASPI32SupportInfo = (DWORD(*)(void))GetProcAddress( hASPI, "GetASPI32SupportInfo" );
        d = GetASPI32DLLVersion();
      } else {
        panicbug("Could not load ASPI drivers, so cdrom access will fail");
        return false;
      }
    }
    cdr = 0;
    bHaveDev = FALSE;
    d = GetASPI32SupportInfo();
    cnt = LOBYTE(LOWORD(d));
    for(i = 0; i < cnt; i++) {
      memset(&sh, 0, sizeof(sh));
      sh.SRB_Cmd  = SC_HA_INQUIRY;
      sh.SRB_HaId = i;
      SendASPI32Command((LPSRB)&sh);
      if(sh.SRB_Status != SS_COMP)
        continue;

      max = (int)sh.HA_Unique[3];
      for(j = 0; j < max; j++) {
        for(k = 0; k < 8; k++) {
          memset(&sd, 0, sizeof(sd));
          sd.SRB_Cmd    = SC_GET_DEV_TYPE;
          sd.SRB_HaId   = i;
          sd.SRB_Target = j;
          sd.SRB_Lun    = k;
          SendASPI32Command((LPSRB)&sd);
          if(sd.SRB_Status == SS_COMP) {
            if(sd.SRB_DeviceType == DTYPE_CDROM) {
              cdr++;
              if(cdr > cdromCount) {
                hid = i;
                tid = j;
                lun = k;
                cdromCount++;
                bHaveDev = TRUE;
              }
            }
          }
          if(bHaveDev) break;
        }
        if(bHaveDev) break;
      }
    }
    fd=1;
  } else {
    hFile=CreateFile((char *)&drive, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_RANDOM_ACCESS, NULL); 
    if (hFile !=(void *)0xFFFFFFFF)
      fd=1;
    if (!using_file) {
      DWORD lpBytesReturned;
      DeviceIoControl(hFile, IOCTL_STORAGE_LOAD_MEDIA, NULL, 0, NULL, 0, &lpBytesReturned, NULL);
    }
  }
#elif defined(OS_darwin)
      if(strcmp(path, "drive") == 0)
      {
        mach_port_t masterPort = 0;
        io_iterator_t mediaIterator;
        kern_return_t kernResult;
        
        kernResult = FindEjectableCDMedia( &mediaIterator, &masterPort );
        if ( kernResult != KERN_SUCCESS ) {
          return false;
        }
        
        kernResult = GetDeviceFilePath( mediaIterator, CDDevicePath, sizeof( CDDevicePath ) );
        if ( kernResult != KERN_SUCCESS ) {
          return false;
        }
	
        // Here a cdrom was found so see if we can read from it.
        // At this point a failure will result in panic.
        if ( strlen( CDDevicePath ) ) {
          fd = open(CDDevicePath, O_RDONLY);
        }
      }
      else
      {
        fd = open(path, O_RDONLY);
      }
#else
      // all platforms except win32
      fd = open(path, O_RDONLY
#ifdef O_BINARY
                  | O_BINARY
#endif
           );

#endif
    if (fd < 0) {
       bug("open cd failed for %s: %s", path, strerror(errno));
       return(false);
    }

  // I just see if I can read a sector to verify that a
  // CD is in the drive and readable.
#ifdef _WIN32
    if(bUseASPI) {
      return ReadCDSector(hid, tid, lun, 0, buffer, BX_CD_FRAMESIZE);
    } else {
      if (!ReadFile(hFile, (void *) buffer, BX_CD_FRAMESIZE, (unsigned long *) &ret, NULL)) {
         CloseHandle(hFile);
         fd = -1;
         D(bug( "insert_cdrom: read returns error." ));
         return(false);
      }
    }
#else
    // do fstat to determine if it's a file or a device, then set using_file.
    struct stat stat_buf;
    ret = fstat (fd, &stat_buf);
    if (ret) {
      panicbug("fstat cdrom file returned error: %s", strerror (errno));
    }
    if (S_ISREG (stat_buf.st_mode)) {
      using_file = 1;
    } else {
      using_file = 0;
    }

    ret = read(fd, (char*) &buffer, BX_CD_FRAMESIZE);
    if (ret < 0) {
       close(fd);
       fd = -1;
       D(bug( "insert_cdrom: read returns error: %s", strerror (errno) ));
       return(false);
        }
#endif
    return(true);
}

  int
cdrom_interface::start_cdrom()
{
  // Spin up the cdrom drive.

  if (fd >= 0) {
#if defined(OS_netbsd)
    if (ioctl (fd, CDIOCSTART) < 0)
       D(bug( "start_cdrom: start returns error: %s", strerror (errno) ));
    return(true);
#else
    return(false); // OS not supported yet, return false always.
#endif
    }
  return(false);
}

  void
cdrom_interface::eject_cdrom()
{
  // Logically eject the CD.  I suppose we could stick in
  // some ioctl() calls to really eject the CD as well.

  if (fd >= 0) {
#if (defined(OS_openbsd) || defined(OS_freebsd) || defined(OS_netbsd))
    (void) ioctl (fd, CDIOCALLOW);
    if (ioctl (fd, CDIOCEJECT) < 0)
    {
	  D(bug( "eject_cdrom: eject returns error." ));
    }
#endif

#ifdef _WIN32
if (using_file == 0)
{
	if(bUseASPI) {
	} else {
		DWORD lpBytesReturned;
		DeviceIoControl(hFile, IOCTL_STORAGE_EJECT_MEDIA, NULL, 0, NULL, 0, &lpBytesReturned, NULL);
	}
}
#else /* _WIN32 */

#ifdef OS_linux
  if (!using_file)
    ioctl (fd, CDROMEJECT, NULL);
#endif

    close(fd);
#endif /* _WIN32 */
    fd = -1;
    }
}


  bool
cdrom_interface::read_toc(uint8* buf, int* length, bool msf, int start_track, int format)
{
  unsigned i;
  // Read CD TOC. Returns false if start track is out of bounds.

  if (fd < 0) {
    panicbug("cdrom: read_toc: file not open.");
    return false;
  }

#if defined(_WIN32)
  if (!isWindowsXP || using_file) { // This is a hack and works okay if there's one rom track only
#else
  if (using_file) {
#endif
    uint32 blocks;
    int len = 4;

    switch (format) {
      case 0:
        // From atapi specs : start track can be 0-63, AA
        if ((start_track > 1) && (start_track != 0xaa))
          return false;

        buf[2] = 1;
        buf[3] = 1;

        if (start_track <= 1) {
          buf[len++] = 0; // Reserved
          buf[len++] = 0x14; // ADR, control
          buf[len++] = 1; // Track number
          buf[len++] = 0; // Reserved

          // Start address
          if (msf) {
            buf[len++] = 0; // reserved
            buf[len++] = 0; // minute
            buf[len++] = 2; // second
            buf[len++] = 0; // frame
          } else {
            buf[len++] = 0;
            buf[len++] = 0;
            buf[len++] = 0;
            buf[len++] = 16; // logical sector 0
          }
        }

        // Lead out track
        buf[len++] = 0; // Reserved
        buf[len++] = 0x16; // ADR, control
        buf[len++] = 0xaa; // Track number
        buf[len++] = 0; // Reserved

        blocks = capacity();

        // Start address
        if (msf) {
          buf[len++] = 0; // reserved
          buf[len++] = (uint8)(((blocks + 150) / 75) / 60); // minute
          buf[len++] = (uint8)(((blocks + 150) / 75) % 60); // second
          buf[len++] = (uint8)((blocks + 150) % 75); // frame;
        } else {
          buf[len++] = (blocks >> 24) & 0xff;
          buf[len++] = (blocks >> 16) & 0xff;
          buf[len++] = (blocks >> 8) & 0xff;
          buf[len++] = (blocks >> 0) & 0xff;
        }

        buf[0] = ((len-2) >> 8) & 0xff;
        buf[1] = (len-2) & 0xff;

        break;

      case 1:
        // multi session stuff - emulate a single session only
        buf[0] = 0;
        buf[1] = 0x0a;
        buf[2] = 1;
        buf[3] = 1;
        for (i = 0; i < 8; i++)
          buf[4+i] = 0;
        len = 12;
        break;

      default:
        panicbug("cdrom: read_toc: unknown format");
        return false;
    }

    *length = len;

    return true;
  }
  // all these implementations below are the platform-dependent code required
  // to read the TOC from a physical cdrom.
#ifdef _WIN32
  if(isWindowsXP)
  {

    // This only works with WinXP
    CDROM_READ_TOC_EX input;
    memset(&input, 0, sizeof(input));
    input.Format = format;
    input.Msf = msf;
    input.SessionTrack = start_track;

    // We have to allocate a chunk of memory to make sure it is aligned on a sector base.
    UCHAR *data = (UCHAR *) VirtualAlloc(NULL, 2048*2, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
    unsigned long iBytesReturned;
    DeviceIoControl(hFile, IOCTL_CDROM_READ_TOC_EX, &input, sizeof(input), data, 804, &iBytesReturned, NULL);
    // now copy it to the users buffer and free our buffer
    memcpy(buf, data, iBytesReturned);
    VirtualFree(data, 0, MEM_RELEASE);
    *length = iBytesReturned;

    return true;
  }
  return false;
#elif (defined(OS_linux) || defined(OS_solaris))
  {
  struct cdrom_tochdr tochdr;
  if (ioctl(fd, CDROMREADTOCHDR, &tochdr))
    panicbug("cdrom: read_toc: READTOCHDR failed.");

  if ((start_track > tochdr.cdth_trk1) && (start_track != 0xaa))
    return false;

  buf[2] = tochdr.cdth_trk0;
  buf[3] = tochdr.cdth_trk1;

  if (start_track < tochdr.cdth_trk0)
    start_track = tochdr.cdth_trk0;

  int len = 4;
  for (int i = start_track; i <= tochdr.cdth_trk1; i++) {
    struct cdrom_tocentry tocentry;
    tocentry.cdte_format = (msf) ? CDROM_MSF : CDROM_LBA;
    tocentry.cdte_track = i;
    if (ioctl(fd, CDROMREADTOCENTRY, &tocentry))
      panicbug("cdrom: read_toc: READTOCENTRY failed.");
    buf[len++] = 0; // Reserved
    buf[len++] = (tocentry.cdte_adr << 4) | tocentry.cdte_ctrl ; // ADR, control
    buf[len++] = i; // Track number
    buf[len++] = 0; // Reserved

    // Start address
    if (msf) {
      buf[len++] = 0; // reserved
      buf[len++] = tocentry.cdte_addr.msf.minute;
      buf[len++] = tocentry.cdte_addr.msf.second;
      buf[len++] = tocentry.cdte_addr.msf.frame;
    } else {
      buf[len++] = (((unsigned)tocentry.cdte_addr.lba) >> 24) & 0xff;
      buf[len++] = (((unsigned)tocentry.cdte_addr.lba) >> 16) & 0xff;
      buf[len++] = (((unsigned)tocentry.cdte_addr.lba) >> 8) & 0xff;
      buf[len++] = (((unsigned)tocentry.cdte_addr.lba) >> 0) & 0xff;
    }
  }

  // Lead out track
  struct cdrom_tocentry tocentry;
  tocentry.cdte_format = (msf) ? CDROM_MSF : CDROM_LBA;
#ifdef CDROM_LEADOUT
  tocentry.cdte_track = CDROM_LEADOUT;
#else
  tocentry.cdte_track = 0xaa;
#endif
  if (ioctl(fd, CDROMREADTOCENTRY, &tocentry))
    panicbug("cdrom: read_toc: READTOCENTRY lead-out failed.");
  buf[len++] = 0; // Reserved
  buf[len++] = (tocentry.cdte_adr << 4) | tocentry.cdte_ctrl ; // ADR, control
  buf[len++] = 0xaa; // Track number
  buf[len++] = 0; // Reserved

  // Start address
  if (msf) {
    buf[len++] = 0; // reserved
    buf[len++] = tocentry.cdte_addr.msf.minute;
    buf[len++] = tocentry.cdte_addr.msf.second;
    buf[len++] = tocentry.cdte_addr.msf.frame;
  } else {
    buf[len++] = (((unsigned)tocentry.cdte_addr.lba) >> 24) & 0xff;
    buf[len++] = (((unsigned)tocentry.cdte_addr.lba) >> 16) & 0xff;
    buf[len++] = (((unsigned)tocentry.cdte_addr.lba) >> 8) & 0xff;
    buf[len++] = (((unsigned)tocentry.cdte_addr.lba) >> 0) & 0xff;
  }

  buf[0] = ((len-2) >> 8) & 0xff;
  buf[1] = (len-2) & 0xff;

  *length = len;

  return true;
  }
#elif (defined(OS_netbsd) || defined(OS_openbsd) || defined(OS_freebsd))
  {
  struct ioc_toc_header h;
  struct ioc_read_toc_entry t;

  if (ioctl (fd, CDIOREADTOCHEADER, &h) < 0)
    panicbug("cdrom: read_toc: READTOCHDR failed.");

  if ((start_track > h.ending_track) && (start_track != 0xaa))
    return false;

  buf[2] = h.starting_track;
  buf[3] = h.ending_track;

  if (start_track < h.starting_track)
    start_track = h.starting_track;

  int len = 4;
  for (int i = start_track; i <= h.ending_track; i++) {
    struct cd_toc_entry tocentry;
    t.address_format = (msf) ? CD_MSF_FORMAT : CD_LBA_FORMAT;
    t.starting_track = i;
    t.data_len = sizeof(tocentry);
    t.data = &tocentry;

    if (ioctl (fd, CDIOREADTOCENTRYS, &t) < 0)
      panicbug("cdrom: read_toc: READTOCENTRY failed.");

    buf[len++] = 0; // Reserved
    buf[len++] = (tocentry.addr_type << 4) | tocentry.control ; // ADR, control
    buf[len++] = i; // Track number
    buf[len++] = 0; // Reserved

    // Start address
    if (msf) {
      buf[len++] = 0; // reserved
      buf[len++] = tocentry.addr.msf.minute;
      buf[len++] = tocentry.addr.msf.second;
      buf[len++] = tocentry.addr.msf.frame;
    } else {
      buf[len++] = (((unsigned)tocentry.addr.lba) >> 24) & 0xff;
      buf[len++] = (((unsigned)tocentry.addr.lba) >> 16) & 0xff;
      buf[len++] = (((unsigned)tocentry.addr.lba) >> 8) & 0xff;
      buf[len++] = (((unsigned)tocentry.addr.lba) >> 0) & 0xff;
    }
  }

  // Lead out track
  struct cd_toc_entry tocentry;
  t.address_format = (msf) ? CD_MSF_FORMAT : CD_LBA_FORMAT;
  t.starting_track = 0xaa;
  t.data_len = sizeof(tocentry);
  t.data = &tocentry;

  if (ioctl (fd, CDIOREADTOCENTRYS, &t) < 0)
    panicbug("cdrom: read_toc: READTOCENTRY lead-out failed.");

  buf[len++] = 0; // Reserved
  buf[len++] = (tocentry.addr_type << 4) | tocentry.control ; // ADR, control
  buf[len++] = 0xaa; // Track number
  buf[len++] = 0; // Reserved

  // Start address
  if (msf) {
    buf[len++] = 0; // reserved
    buf[len++] = tocentry.addr.msf.minute;
    buf[len++] = tocentry.addr.msf.second;
    buf[len++] = tocentry.addr.msf.frame;
  } else {
    buf[len++] = (((unsigned)tocentry.addr.lba) >> 24) & 0xff;
    buf[len++] = (((unsigned)tocentry.addr.lba) >> 16) & 0xff;
    buf[len++] = (((unsigned)tocentry.addr.lba) >> 8) & 0xff;
    buf[len++] = (((unsigned)tocentry.addr.lba) >> 0) & 0xff;
  }

  buf[0] = ((len-2) >> 8) & 0xff;
  buf[1] = (len-2) & 0xff;

  *length = len;

  return true;
  }
#elif defined(OS_darwin)
  // Read CD TOC. Returns false if start track is out of bounds.

  {
  struct _CDTOC * toc = ReadTOC( CDDevicePath );
  
  if ((start_track > toc->last_session) && (start_track != 0xaa))
    return false;

  buf[2] = toc->first_session;
  buf[3] = toc->last_session;

  if (start_track < toc->first_session)
    start_track = toc->first_session;

  int len = 4;
  for (int i = start_track; i <= toc->last_session; i++) {
    buf[len++] = 0; // Reserved
    buf[len++] = toc->trackdesc[i].ctrl_adr ; // ADR, control
    buf[len++] = i; // Track number
    buf[len++] = 0; // Reserved

    // Start address
    if (msf) {
      buf[len++] = 0; // reserved
      buf[len++] = toc->trackdesc[i].address.minute;
      buf[len++] = toc->trackdesc[i].address.second;
      buf[len++] = toc->trackdesc[i].address.frame;
    } else {
      unsigned lba = (unsigned)(MSF_TO_LBA(toc->trackdesc[i].address));
      buf[len++] = (lba >> 24) & 0xff;
      buf[len++] = (lba >> 16) & 0xff;
      buf[len++] = (lba >> 8) & 0xff;
      buf[len++] = (lba >> 0) & 0xff;
    }
  }

  // Lead out track
  buf[len++] = 0; // Reserved
  buf[len++] = 0x16; // ADR, control
  buf[len++] = 0xaa; // Track number
  buf[len++] = 0; // Reserved

  uint32 blocks = capacity();

  // Start address
  if (msf) {
    buf[len++] = 0; // reserved
    buf[len++] = (uint8)(((blocks + 150) / 75) / 60); // minute
    buf[len++] = (uint8)(((blocks + 150) / 75) % 60); // second
    buf[len++] = (uint8)((blocks + 150) % 75); // frame;
  } else {
    buf[len++] = (blocks >> 24) & 0xff;
    buf[len++] = (blocks >> 16) & 0xff;
    buf[len++] = (blocks >> 8) & 0xff;
    buf[len++] = (blocks >> 0) & 0xff;
  }

  buf[0] = ((len-2) >> 8) & 0xff;
  buf[1] = (len-2) & 0xff;

  *length = len;

  return true;
  }
#else
  bug("read_toc: your OS is not supported yet.");
  return false; // OS not supported yet, return false always.
#endif
}


  uint32
cdrom_interface::capacity()
{
  // Return CD-ROM capacity.  I believe you want to return
  // the number of blocks of capacity the actual media has.

#if !defined(_WIN32)
  // win32 has its own way of doing this
  if (using_file) {
    // return length of the image file
    struct stat stat_buf;
    int ret = fstat (fd, &stat_buf);
    if (ret) {
       panicbug("fstat on cdrom image returned err: %s", strerror(errno));
    }
    if ((stat_buf.st_size % 2048) != 0)  {
      bug("expected cdrom image to be a multiple of 2048 bytes");
    }
    return stat_buf.st_size / 2048;
  }
#endif

#ifdef OS_beos
	return GetNumDeviceBlocks(fd, BX_CD_FRAMESIZE);
#elif defined(OS_solaris)
  {
    struct stat buf/* = {0}*/;

    if (fd < 0) {
      panicbug("cdrom: capacity: file not open.");
    } 
    
    if( fstat(fd, &buf) != 0 )
      panicbug("cdrom: capacity: stat() failed.");
  
    return(buf.st_size);
  }
#elif (defined(OS_netbsd) || defined(OS_openbsd))
  {
  // We just read the disklabel, imagine that...
  struct disklabel lp;

  if (fd < 0)
    panicbug("cdrom: capacity: file not open.");

  if (ioctl(fd, DIOCGDINFO, &lp) < 0)
    panicbug("cdrom: ioctl(DIOCGDINFO) failed");

  D(bug( "capacity: %u", lp.d_secperunit ));
  return(lp.d_secperunit);
  }
#elif defined(OS_linux)
  {
  // Read the TOC to get the data size, since BLKGETSIZE doesn't work on
  // non-ATAPI drives.  This is based on Keith Jones code below.
  // <splite@purdue.edu> 21 June 2001

  int i, dtrk_lba, num_sectors;
  struct cdrom_tochdr td;
  struct cdrom_tocentry te;

  if (fd < 0)
    panicbug("cdrom: capacity: file not open.");

  if (ioctl(fd, CDROMREADTOCHDR, &td) < 0)
    panicbug("cdrom: ioctl(CDROMREADTOCHDR) failed");

  num_sectors = -1;
  dtrk_lba = -1;

  for (i = td.cdth_trk0; i <= td.cdth_trk1; i++) {
    te.cdte_track = i;
    te.cdte_format = CDROM_LBA;
    if (ioctl(fd, CDROMREADTOCENTRY, &te) < 0)
      panicbug("cdrom: ioctl(CDROMREADTOCENTRY) failed");

    if (dtrk_lba != -1) {
      num_sectors = te.cdte_addr.lba - dtrk_lba;
      break;
    }
    if (te.cdte_ctrl & CDROM_DATA_TRACK) {
      dtrk_lba = te.cdte_addr.lba;
    }
  }

  if (num_sectors < 0) {
    if (dtrk_lba != -1) {
      te.cdte_track = CDROM_LEADOUT;
      te.cdte_format = CDROM_LBA;
      if (ioctl(fd, CDROMREADTOCENTRY, &te) < 0)
        panicbug("cdrom: ioctl(CDROMREADTOCENTRY) failed");
      num_sectors = te.cdte_addr.lba - dtrk_lba;
    } else
      panicbug("cdrom: no data track found");
  }

  return(num_sectors);

  }
#elif defined(OS_freebsd)
  {
  // Read the TOC to get the size of the data track.
  // Keith Jones <freebsd.dev@blueyonder.co.uk>, 16 January 2000

#define MAX_TRACKS 100

  int i, num_tracks, num_sectors;
  struct ioc_toc_header td;
  struct ioc_read_toc_entry rte;
  struct cd_toc_entry toc_buffer[MAX_TRACKS + 1];

  if (fd < 0)
    panicbug("cdrom: capacity: file not open.");

  if (ioctl(fd, CDIOREADTOCHEADER, &td) < 0)
    panicbug("cdrom: ioctl(CDIOREADTOCHEADER) failed");

  num_tracks = (td.ending_track - td.starting_track) + 1;
  if (num_tracks > MAX_TRACKS)
    panicbug("cdrom: TOC is too large");

  rte.address_format = CD_LBA_FORMAT;
  rte.starting_track = td.starting_track;
  rte.data_len = (num_tracks + 1) * sizeof(struct cd_toc_entry);
  rte.data = toc_buffer;
  if (ioctl(fd, CDIOREADTOCENTRYS, &rte) < 0)
    panicbug("cdrom: ioctl(CDIOREADTOCENTRYS) failed");

  num_sectors = -1;
  for (i = 0; i < num_tracks; i++) {
    if (rte.data[i].control & 4) {	/* data track */
      num_sectors = ntohl(rte.data[i + 1].addr.lba)
          - ntohl(rte.data[i].addr.lba);
      break;
      }
    }

  if (num_sectors < 0)
    panicbug("cdrom: no data track found");

  return(num_sectors);

  }
#elif defined(_WIN32)
  {
	  if(bUseASPI) {
		  return (GetCDCapacity(hid, tid, lun) / 2352);
	  } else if(using_file) {
	    ULARGE_INTEGER FileSize;
	    FileSize.LowPart = GetFileSize(hFile, &FileSize.HighPart);
		return (FileSize.QuadPart / 2048);
	  } else {  /* direct device access */
	    DWORD SectorsPerCluster;
	    DWORD TotalNumOfClusters;
	    GetDiskFreeSpace( path, &SectorsPerCluster, NULL, NULL, &TotalNumOfClusters);
		return (TotalNumOfClusters * SectorsPerCluster);
	  }
  }
#elif defined(OS_darwin)
// Find the size of the first data track on the cd.  This has produced
// the same results as the linux version on every cd I have tried, about
// 5.  The differences here seem to be that the entries in the TOC when
// retrieved from the IOKit interface appear in a reversed order when
// compared with the linux READTOCENTRY ioctl.
  {
  // Return CD-ROM capacity.  I believe you want to return
  // the number of bytes of capacity the actual media has.

  D(bug( "Capacity" ));

  struct _CDTOC * toc = ReadTOC( CDDevicePath );

  if ( toc == NULL ) {
    panicbug( "capacity: Failed to read toc" );
  }

  size_t toc_entries = ( toc->length - 2 ) / sizeof( struct _CDTOC_Desc );
  
  D(bug( "reading %d toc entries\n", toc_entries ));

  int start_sector = -1;
  int data_track = -1;

  // Iterate through the list backward. Pick the first data track and
  // get the address of the immediately previous (or following depending
  // on how you look at it).  The difference in the sector numbers
  // is returned as the sized of the data track.
  for ( int i=toc_entries - 1; i>=0; i-- ) {

    D(bug( "session %d ctl_adr %d tno %d point %d lba %d z %d p lba %d\n",
             (int)toc->trackdesc[i].session,
             (int)toc->trackdesc[i].ctrl_adr,
             (int)toc->trackdesc[i].tno,
             (int)toc->trackdesc[i].point,
             MSF_TO_LBA( toc->trackdesc[i].address ),
             (int)toc->trackdesc[i].zero,
             MSF_TO_LBA(toc->trackdesc[i].p )));

    if ( start_sector != -1 ) {
      start_sector = MSF_TO_LBA(toc->trackdesc[i].p) - start_sector;
      break;
    }

    if ((toc->trackdesc[i].ctrl_adr >> 4) != 1) continue;

    if ( toc->trackdesc[i].ctrl_adr & 0x04 ) {
      data_track = toc->trackdesc[i].point;
      start_sector = MSF_TO_LBA(toc->trackdesc[i].p);
    }
  }  

  free( toc );

  if ( start_sector == -1 ) {
    start_sector = 0;
  }

  D(bug("first data track %d data size is %d", data_track, start_sector));

  return start_sector;
  }
#else
  panicbug( "capacity: your OS is not supported yet." );
  return(0);
#endif
}

  void
cdrom_interface::read_block(uint8* buf, int lba)
{
  // Read a single block from the CD

#ifdef _WIN32
  LARGE_INTEGER pos;
#else
  off_t pos;
#endif
  ssize_t n;

#ifdef _WIN32
  if(bUseASPI) {
	  ReadCDSector(hid, tid, lun, lba, buf, BX_CD_FRAMESIZE);
	  n = BX_CD_FRAMESIZE;
  } else {
    pos.QuadPart = (LONGLONG)lba*BX_CD_FRAMESIZE;
    pos.LowPart = SetFilePointer(hFile, pos.LowPart, &pos.HighPart, SEEK_SET);
    if ((pos.LowPart == 0xffffffff) && (GetLastError() != NO_ERROR)) {
      panicbug("cdrom: read_block: SetFilePointer returned error.");
	}
	ReadFile(hFile, (void *) buf, BX_CD_FRAMESIZE, (unsigned long *) &n, NULL);
  }
#elif defined(OS_darwin)
#define CD_SEEK_DISTANCE kCDSectorSizeWhole
  if(using_file)
  {
    pos = lseek(fd, lba*BX_CD_FRAMESIZE, SEEK_SET);
    if (pos < 0) {
      panicbug("cdrom: read_block: lseek returned error.");
  }
  n = read(fd, buf, BX_CD_FRAMESIZE);
  }
  else
  {
    // This seek will leave us 16 bytes from the start of the data
    // hence the magic number.	
    pos = lseek(fd, lba*CD_SEEK_DISTANCE + 16, SEEK_SET);
    if (pos < 0) {
      panicbug("cdrom: read_block: lseek returned error.");
    }
    n = read(fd, buf, CD_FRAMESIZE);
  }
#else
  pos = lseek(fd, lba*BX_CD_FRAMESIZE, SEEK_SET);
  if (pos < 0) {
    panicbug("cdrom: read_block: lseek returned error.");
  }
  n = read(fd, (char*) buf, BX_CD_FRAMESIZE);
#endif

  if (n != BX_CD_FRAMESIZE) {
    panicbug("cdrom: read_block: read returned %d",
      (int) n);
    }
}

#endif /* if SUPPORT_CDROM */
