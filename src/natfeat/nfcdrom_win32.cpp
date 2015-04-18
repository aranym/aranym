/*
	NatFeat host CD-ROM access, Win32 CD-ROM driver

	ARAnyM (C) 2014 ARAnyM developer team

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/



#include "sysdeps.h"
#include "win32_supp.h"
#include "toserror.h"
#include <winioctl.h>
#include "cpu_emulation.h"
#include "parameters.h"
#include "nfcdrom.h"
#include "nfcdrom_atari.h"
#include "nfcdrom_win32.h"

#define DEBUG 0
#include "debug.h"

/*--- Defines ---*/

#define NFCD_NAME	"nf:cdrom:win32: "

#ifndef MSF_TO_FRAMES
#define CD_FPS     75
#define MSF_TO_FRAMES(M, S, F)     ((M)*60*CD_FPS+(S)*CD_FPS+(F))
#endif
#ifndef FRAMES_TO_MSF
#define FRAMES_TO_MSF(f, M,S,F)	{					\
	int value = f;							\
	*(F) = value%CD_FPS;						\
	value /= CD_FPS;						\
	*(S) = value%60;						\
	value /= 60;							\
	*(M) = value;							\
}
#endif

/*--- CDROM definitions ---*/

#define _MS_STRUCT __attribute__ ((ms_struct))

#ifndef FILE_DEVICE_CD_ROM
#define FILE_DEVICE_CD_ROM                0x00000002
#endif

#define IOCTL_CDROM_BASE                  FILE_DEVICE_CD_ROM

#ifndef CTL_CODE
#define CTL_CODE(DeviceType, Function, Method, Access)( ((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method))
#endif

#ifndef ERROR_MEDIA_CHECK
# define ERROR_MEDIA_CHECK 679
#endif

#define IOCTL_CDROM_READ_TOC			CTL_CODE(IOCTL_CDROM_BASE, 0x0000, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CDROM_SEEK_AUDIO_MSF		CTL_CODE(IOCTL_CDROM_BASE, 0x0001, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CDROM_STOP_AUDIO			CTL_CODE(IOCTL_CDROM_BASE, 0x0002, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CDROM_PAUSE_AUDIO			CTL_CODE(IOCTL_CDROM_BASE, 0x0003, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CDROM_RESUME_AUDIO		CTL_CODE(IOCTL_CDROM_BASE, 0x0004, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CDROM_GET_VOLUME			CTL_CODE(IOCTL_CDROM_BASE, 0x0005, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CDROM_PLAY_AUDIO_MSF		CTL_CODE(IOCTL_CDROM_BASE, 0x0006, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CDROM_SET_VOLUME			CTL_CODE(IOCTL_CDROM_BASE, 0x000A, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CDROM_READ_Q_CHANNEL		CTL_CODE(IOCTL_CDROM_BASE, 0x000B, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CDROM_GET_CONTROL			CTL_CODE(IOCTL_CDROM_BASE, 0x000D, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CDROM_GET_LAST_SESSION	CTL_CODE(IOCTL_CDROM_BASE, 0x000E, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CDROM_RAW_READ			CTL_CODE(IOCTL_CDROM_BASE, 0x000F, METHOD_OUT_DIRECT,  FILE_READ_ACCESS)
#define IOCTL_CDROM_GET_DRIVE_GEOMETRY	CTL_CODE(IOCTL_CDROM_BASE, 0x0013, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CDROM_DISK_TYPE			CTL_CODE(IOCTL_CDROM_BASE, 0x0010, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_CDROM_GET_DRIVE_GEOMETRY	CTL_CODE(IOCTL_CDROM_BASE, 0x0013, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CDROM_GET_DRIVE_GEOMETRY_EX CTL_CODE(IOCTL_CDROM_BASE, 0x0014, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CDROM_READ_TOC_EX			CTL_CODE(IOCTL_CDROM_BASE, 0x0015, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CDROM_GET_CONFIGURATION	CTL_CODE(IOCTL_CDROM_BASE, 0x0016, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CDROM_EXCLUSIVE_ACCESS	CTL_CODE(IOCTL_CDROM_BASE, 0x0017, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_CDROM_SET_SPEED			CTL_CODE(IOCTL_CDROM_BASE, 0x0018, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CDROM_GET_INQUIRY_DATA	CTL_CODE(IOCTL_CDROM_BASE, 0x0019, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CDROM_ENABLE_STREAMING	CTL_CODE(IOCTL_CDROM_BASE, 0x001A, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CDROM_SEND_OPC_INFORMATION  CTL_CODE(IOCTL_CDROM_BASE, 0x001B, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_CDROM_GET_PERFORMANCE		CTL_CODE(IOCTL_CDROM_BASE, 0x001C, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CDROM_CHECK_VERIFY		CTL_CODE(IOCTL_CDROM_BASE, 0x0200, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CDROM_MEDIA_REMOVAL		CTL_CODE(IOCTL_CDROM_BASE, 0x0201, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CDROM_EJECT_MEDIA			CTL_CODE(IOCTL_CDROM_BASE, 0x0202, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CDROM_LOAD_MEDIA			CTL_CODE(IOCTL_CDROM_BASE, 0x0203, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CDROM_RESERVE				CTL_CODE(IOCTL_CDROM_BASE, 0x0204, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CDROM_RELEASE				CTL_CODE(IOCTL_CDROM_BASE, 0x0205, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CDROM_FIND_NEW_DEVICES	CTL_CODE(IOCTL_CDROM_BASE, 0x0206, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CDROM_SIMBAD				CTL_CODE(IOCTL_CDROM_BASE, 0x1003, METHOD_BUFFERED, FILE_READ_ACCESS)

/* CDROM_READ_TOC_EX.Format constants */
#define CDROM_READ_TOC_EX_FORMAT_TOC      0x00
#define CDROM_READ_TOC_EX_FORMAT_SESSION  0x01
#define CDROM_READ_TOC_EX_FORMAT_FULL_TOC 0x02
#define CDROM_READ_TOC_EX_FORMAT_PMA      0x03
#define CDROM_READ_TOC_EX_FORMAT_ATIP     0x04
#define CDROM_READ_TOC_EX_FORMAT_CDTEXT   0x05

#define MAXIMUM_NUMBER_TRACKS             100
#define MAXIMUM_CDROM_SIZE                804
#define MINIMUM_CDROM_READ_TOC_EX_SIZE    2

typedef struct _CDROM_SEEK_AUDIO_MSF {
  UCHAR  M;
  UCHAR  S;
  UCHAR  F;
} _MS_STRUCT CDROM_SEEK_AUDIO_MSF, *PCDROM_SEEK_AUDIO_MSF;

typedef struct _CDROM_PLAY_AUDIO_MSF {
  UCHAR  StartingM;
  UCHAR  StartingS;
  UCHAR  StartingF;
  UCHAR  EndingM;
  UCHAR  EndingS;
  UCHAR  EndingF;
} _MS_STRUCT CDROM_PLAY_AUDIO_MSF, *PCDROM_PLAY_AUDIO_MSF;

typedef struct _CDROM_READ_TOC_EX {
  UCHAR  Format : 4;
  UCHAR  Reserved1 : 3; 
  UCHAR  Msf : 1;
  UCHAR  SessionTrack;
  UCHAR  Reserved2;
  UCHAR  Reserved3;
} _MS_STRUCT CDROM_READ_TOC_EX, *PCDROM_READ_TOC_EX;

typedef struct _TRACK_DATA {
  UCHAR  Reserved;
  UCHAR  Control : 4;
  UCHAR  Adr : 4;
  UCHAR  TrackNumber;
  UCHAR  Reserved1;
  UCHAR  Address[4];
} _MS_STRUCT TRACK_DATA, *PTRACK_DATA;

typedef struct _CDROM_TOC {
  UCHAR  Length[2];
  UCHAR  FirstTrack;
  UCHAR  LastTrack;
  TRACK_DATA  TrackData[MAXIMUM_NUMBER_TRACKS];
} _MS_STRUCT CDROM_TOC, *PCDROM_TOC;

#define CDROM_TOC_SIZE sizeof(CDROM_TOC)

/* SUB_Q_HEADER.AudioStatus constants */
#define AUDIO_STATUS_NOT_SUPPORTED  0x00
#define AUDIO_STATUS_IN_PROGRESS    0x11
#define AUDIO_STATUS_PAUSED         0x12
#define AUDIO_STATUS_PLAY_COMPLETE  0x13
#define AUDIO_STATUS_PLAY_ERROR     0x14
#define AUDIO_STATUS_NO_STATUS      0x15

/* CDROM_SUB_Q_DATA_FORMAT.Format constants */
#define IOCTL_CDROM_SUB_Q_CHANNEL         0x00
#define IOCTL_CDROM_CURRENT_POSITION      0x01
#define IOCTL_CDROM_MEDIA_CATALOG         0x02
#define IOCTL_CDROM_TRACK_ISRC            0x03

typedef struct _CDROM_SUB_Q_DATA_FORMAT {
  UCHAR Format;
  UCHAR Track;
} _MS_STRUCT CDROM_SUB_Q_DATA_FORMAT, *PCDROM_SUB_Q_DATA_FORMAT;

typedef struct _SUB_Q_HEADER {
  UCHAR  Reserved;
  UCHAR  AudioStatus;
  UCHAR  DataLength[2];
} _MS_STRUCT SUB_Q_HEADER, *PSUB_Q_HEADER;

typedef struct _SUB_Q_MEDIA_CATALOG_NUMBER {
  SUB_Q_HEADER  Header;
  UCHAR  FormatCode;
  UCHAR  Reserved[3];
  UCHAR  Reserved1 : 7;
  UCHAR  Mcval :1;
  UCHAR  MediaCatalog[15];
} _MS_STRUCT SUB_Q_MEDIA_CATALOG_NUMBER, *PSUB_Q_MEDIA_CATALOG_NUMBER;

typedef struct _SUB_Q_TRACK_ISRC {
  SUB_Q_HEADER  Header;
  UCHAR  FormatCode;
  UCHAR  Reserved0;
  UCHAR  Track;
  UCHAR  Reserved1;
  UCHAR  Reserved2 : 7;
  UCHAR  Tcval : 1;
  UCHAR  TrackIsrc[15];
} _MS_STRUCT SUB_Q_TRACK_ISRC, *PSUB_Q_TRACK_ISRC;

typedef struct _SUB_Q_CURRENT_POSITION {
  SUB_Q_HEADER  Header;
  UCHAR  FormatCode;
  UCHAR  Control : 4;
  UCHAR  ADR : 4;
  UCHAR  TrackNumber;
  UCHAR  IndexNumber;
  UCHAR  AbsoluteAddress[4];
  UCHAR  TrackRelativeAddress[4];
} _MS_STRUCT SUB_Q_CURRENT_POSITION, *PSUB_Q_CURRENT_POSITION;

typedef union _SUB_Q_CHANNEL_DATA {
  SUB_Q_CURRENT_POSITION  CurrentPosition;
  SUB_Q_MEDIA_CATALOG_NUMBER  MediaCatalog;
  SUB_Q_TRACK_ISRC  TrackIsrc;
} _MS_STRUCT SUB_Q_CHANNEL_DATA, *PSUB_Q_CHANNEL_DATA;

#define CDROM_EXCLUSIVE_CALLER_LENGTH 64

typedef enum _EXCLUSIVE_ACCESS_REQUEST_TYPE {
    ExclusiveAccessQueryState,
    ExclusiveAccessLockDevice,
    ExclusiveAccessUnlockDevice
} EXCLUSIVE_ACCESS_REQUEST_TYPE, *PEXCLUSIVE_ACCESS_REQUEST_TYPE;

typedef struct _CDROM_EXCLUSIVE_ACCESS {
  EXCLUSIVE_ACCESS_REQUEST_TYPE RequestType;
  ULONG                         Flags;
} _MS_STRUCT CDROM_EXCLUSIVE_ACCESS, *PCDROM_EXCLUSIVE_ACCESS;

typedef struct _CDROM_EXCLUSIVE_LOCK {
  CDROM_EXCLUSIVE_ACCESS Access;
  UCHAR                  CallerName[CDROM_EXCLUSIVE_CALLER_LENGTH];
} _MS_STRUCT CDROM_EXCLUSIVE_LOCK, *PCDROM_EXCLUSIVE_LOCK;

typedef struct _VOLUME_CONTROL {
  UCHAR  PortVolume[4];
} _MS_STRUCT VOLUME_CONTROL, *PVOLUME_CONTROL;

/* CDROM_AUDIO_CONTROL.LbaFormat constants */
#define AUDIO_WITH_PREEMPHASIS            0x1
#define DIGITAL_COPY_PERMITTED            0x2
#define AUDIO_DATA_TRACK                  0x4
#define TWO_FOUR_CHANNEL_AUDIO            0x8

typedef struct _CDROM_AUDIO_CONTROL {
	UCHAR  LbaFormat;
	USHORT  LogicalBlocksPerSecond;
} _MS_STRUCT CDROM_AUDIO_CONTROL, *PCDROM_AUDIO_CONTROL;

typedef struct _CDROM_TOC_SESSION_DATA {
  UCHAR      Length[2];
  UCHAR      FirstCompleteSession;
  UCHAR      LastCompleteSession;
  TRACK_DATA TrackData[1];
} _MS_STRUCT CDROM_TOC_SESSION_DATA, *PCDROM_TOC_SESSION_DATA;

typedef enum _TRACK_MODE_TYPE {
	YellowMode2 = 0,
	XAForm2 = 1,
	CDDA = 2,
	RawWithC2AndSubCode  = 3,
	RawWithC2            = 4,
	RawWithSubCode       = 5
} TRACK_MODE_TYPE, *PTRACK_MODE_TYPE;

typedef struct __RAW_READ_INFO {
	LARGE_INTEGER  DiskOffset;
	ULONG  SectorCount;
	TRACK_MODE_TYPE  TrackMode;
} _MS_STRUCT RAW_READ_INFO, *PRAW_READ_INFO;

/* RedBook CD-DA 2352 */
#define CD_FRAMESIZE_RAW 2352
/* (Yellow Book) Mode1 Form1 */
#define M1RAW_SECTOR_SIZE 2048
/* (Yellow Book) Mode1 Form2 */
#define M2RAW_SECTOR_SIZE 2336
/* The maximum possible returned */
#define CD_FRAMESIZE_RAWER 2646

/*--- Public functions ---*/

CdromDriverWin32::CdromDriverWin32()
{
	int i;

	D(bug(NFCD_NAME "CdromDriverWin32()"));
	for (i = 0; i < CD_MAX_DRIVES; i++)
	{
		cddrives[i].handle = NULL;
		cddrives[i].device = NULL;
		cddrives[i].usecount = 0;
	}
	drives_scanned = false;
}


CdromDriverWin32::~CdromDriverWin32()
{
	int i;

	D(bug(NFCD_NAME "~CdromDriverWin32()"));
	for (i = 0; i < CD_MAX_DRIVES; i++)
	{
		while (cddrives[i].usecount > 0)
			CloseDrive(i);
		free(cddrives[i].device);
		cddrives[i].device = NULL;
	}
	drives_scanned = false;
	numcds = 0;
}


int CdromDriverWin32::Count()
{
	if (!drives_scanned)
	{
		(void) DeviceName(0);
	}
	return numcds;
}

const char *CdromDriverWin32::DeviceName(int drive)
{
	if (!drives_scanned)
	{
		char device[10];
		numcds = 0;

		for ( int i = 0 ; i < CD_MAX_DRIVES; i++ )
		{
			sprintf(device, "%c:\\", DriveToLetter(i));
			if ( GetDriveType(device) == DRIVE_CDROM )
			{
				sprintf(device, "\\\\.\\%c:", DriveToLetter(i));
				cddrives[numcds].device = strdup(device);
				numcds++;
			}
		}
		drives_scanned = true;
	}
	return cddrives[drive].device;
}

/*--- Private functions ---*/

int CdromDriverWin32::OpenDrive(memptr device)
{
	int drive;

	drive = GetDrive(device);
	drive = DriveFromLetter(drive);
	
	/* Drive exist ? */
	if (drive < 0 || drive >= CD_MAX_DRIVES || (drives_mask & (1<<drive)) == 0 ||
		DeviceName(drive = bx_options.nfcdroms[drive].physdevtohostdev) == NULL)
	{
		D(bug(NFCD_NAME " physical device %c does not exist", GetDrive(device)));
		return TOS_EUNDEV;
	}
	
	/* Drive opened ? */
	if (cddrives[drive].handle != NULL)
	{
		++cddrives[drive].usecount;
		return drive;
	}
	
	/* Open drive */
	if (cddrives[drive].usecount != 0)
	{
		D(panicbug(NFCD_NAME "::OpenDrive(): usecount != 0"));
	}
	cddrives[drive].handle = ::CreateFile(DeviceName(drive), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN|FILE_FLAG_OVERLAPPED, NULL);
	if (cddrives[drive].handle == INVALID_HANDLE_VALUE)
	{
		cddrives[drive].handle = NULL;
		win32_set_errno();
		int errorcode = errnoHost2Mint(errno, TOS_ENOSYS);
		D(bug(NFCD_NAME " error opening drive %s: %s", cddrives[drive].device, win32_errstring(GetLastError())));
		return errorcode;
	}

	++cddrives[drive].usecount;
	return drive;
}


void CdromDriverWin32::CloseDrive(int drive)
{
	/* Drive already closed ? */
	if (cddrives[drive].handle == NULL)
		return;
	--cddrives[drive].usecount;
	if (cddrives[drive].usecount <= 0)
	{
		if (cddrives[drive].usecount != 0)
		{
			D(panicbug(NFCD_NAME "::CloseDrive(): usecount != 0"));
		}
		::CloseHandle(cddrives[drive].handle);
		cddrives[drive].handle = NULL;
		cddrives[drive].usecount = 0;
	}
}


int32 CdromDriverWin32::cd_read(memptr device, memptr buffer, uint32 first, uint32 length)
{
	int drive;
	LONG lo, hi;
	uint64_t pos;
	
	drive = OpenDrive(device);
	if (drive < 0)
		return drive;

	D(bug(NFCD_NAME "Read(%d,%d)", first, length));

	pos = ((uint64_t)first) * CD_FRAMESIZE;
	lo = pos & 0xffffffffl;
	hi = (pos >> 32) & 0xffffffffl;
	if (::SetFilePointer(cddrives[drive].handle, lo, &hi, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
	{
		win32_set_errno();
		int errorcode = errnoHost2Mint(errno, TOS_ENOSYS);
		D(bug(NFCD_NAME "Read(): can not seek to block %d", first));
		CloseDrive(drive);
		return errorcode;
	}

	if (::ReadFile(cddrives[drive].handle, Atari2HostAddr(buffer), length * CD_FRAMESIZE, NULL, NULL) == FALSE)
	{
		int errorcode = errnoHost2Mint(errno, TOS_ENOSYS);
		D(bug(NFCD_NAME "Read(): can not read %d blocks", length));
		CloseDrive(drive);
		return errorcode;
	}

	CloseDrive(drive);
	return TOS_E_OK;
}


int CdromDriverWin32::cd_winioctl(int drive, DWORD code, LPVOID in, DWORD insize, LPVOID out, DWORD outsize, LPDWORD returned)
{
	int errorcode = TOS_E_OK;
	DWORD lpBytesReturned;

	if (returned == NULL)
		returned = &lpBytesReturned;
	if (!::DeviceIoControl(cddrives[drive].handle, code, in, insize, out, outsize, returned, NULL))
	{
		win32_set_errno();
		errorcode = errnoHost2Mint(errno, TOS_EDRVNR);
	}
	return errorcode;
}


int32 CdromDriverWin32::cd_status(memptr device, memptr ext_status)
{
	int drive, errorcode;
	
	UNUSED(ext_status);
	drive = OpenDrive(device);
	if (drive < 0)
		return drive;

	errorcode = cd_winioctl(drive, IOCTL_CDROM_CHECK_VERIFY);
	if (errorcode < 0)
	{
		DWORD err = GetLastError();
		if (err == ERROR_MEDIA_CHECK || err == ERROR_MEDIA_CHANGED)
		{
			errorcode = 1<<3;
		}
		D(bug(NFCD_NAME "Status(CDROM_MEDIA_CHANGED): %s", win32_errstring(GetLastError())));
	}

	CloseDrive(drive);
	return errorcode;
}


int32 CdromDriverWin32::cd_ioctl(memptr device, uint16 opcode, memptr buffer)
{
	int drive, errorcode;

	drive = OpenDrive(device);
	if (drive < 0)
		return drive;

	errorcode = TOS_ENOSYS;

	switch(opcode)
	{
	case ATARI_CDROMREADOFFSET:
		{
			CDROM_TOC_SESSION_DATA data;
			Uint32 *atari_addr;

			atari_addr = (Uint32 *) Atari2HostAddr(buffer);
			errorcode = cd_winioctl(drive, IOCTL_CDROM_GET_LAST_SESSION, NULL, 0, &data, sizeof(data));
			if (errorcode >= 0)
			{
				Uint32 *p = (Uint32 *)data.TrackData[0].Address;
				*atari_addr = *p;
				D(bug(NFCD_NAME "CDROMREADOFFSET:    lba 0x%08x", SDL_SwapBE32(*atari_addr)));
			}
		}
		break;

	case ATARI_CDROMPAUSE:
		D(bug(NFCD_NAME " Ioctl(CDROMPAUSE)"));
		errorcode = cd_winioctl(drive, IOCTL_CDROM_PAUSE_AUDIO);
		break;
	
	case ATARI_CDROMRESUME:
		D(bug(NFCD_NAME " Ioctl(CDROMRESUME)"));
		errorcode = cd_winioctl(drive, IOCTL_CDROM_RESUME_AUDIO);
		break;
	
	case ATARI_CDROMPLAYMSF:
		{
			CDROM_PLAY_AUDIO_MSF cd_msf;
			atari_cdrom_msf_t *atari_msf;
			
			atari_msf = (atari_cdrom_msf_t *) Atari2HostAddr(buffer);
			cd_msf.StartingM = atari_msf->cdmsf_min0;
			cd_msf.StartingS = atari_msf->cdmsf_sec0;
			cd_msf.StartingF = atari_msf->cdmsf_frame0;
			cd_msf.EndingM = atari_msf->cdmsf_min1;
			cd_msf.EndingS = atari_msf->cdmsf_sec1;
			cd_msf.EndingF = atari_msf->cdmsf_frame1;

			D(bug(NFCD_NAME " Ioctl(CDROMPLAYMSF,%02d:%02d:%02d,%02d:%02d:%02d)",
				cd_msf.StartingM, cd_msf.StartingS, cd_msf.StartingF,
				cd_msf.EndingM, cd_msf.EndingS, cd_msf.EndingF
			));

			errorcode = cd_winioctl(drive, IOCTL_CDROM_PLAY_AUDIO_MSF, &cd_msf, sizeof(cd_msf));
		}
		break;

	case ATARI_CDROMPLAYTRKIND:
		{
			atari_cdrom_ti *ti = (atari_cdrom_ti *)Atari2HostAddr(buffer);
			D(bug(NFCD_NAME " Ioctl(CDROMPLAYTRKIND,%d.%d,%d.%d)",
				ti->cdti_trk0, ti->cdti_ind0,
				ti->cdti_trk1, ti->cdti_trk1
			));

			errorcode = cd_win_playtracks(drive, ti->cdti_trk0, ti->cdti_trk1);
		}
		break;
	
	case ATARI_CDROMREADTOCHDR:
		{
			CDROM_READ_TOC_EX hdr;
			CDROM_TOC toc;
			atari_cdromtochdr_t *atari_hdr;
			
			atari_hdr = (atari_cdromtochdr_t *)Atari2HostAddr(buffer);
			hdr.Format = CDROM_READ_TOC_EX_FORMAT_TOC;
			hdr.Reserved1 = 0;
			hdr.Msf = 1;
			hdr.SessionTrack = 0;
			hdr.Reserved2 = 0;
			hdr.Reserved3 = 0;
			errorcode = cd_winioctl(drive, IOCTL_CDROM_READ_TOC_EX, &hdr, sizeof(hdr), &toc, sizeof(toc));
			if (errorcode >= 0)
			{
				atari_hdr->cdth_trk0 = toc.FirstTrack;
				atari_hdr->cdth_trk1 = toc.LastTrack;
				D(bug(NFCD_NAME " Ioctl(CDROMREADTOCHDR) = {%d, %d}", atari_hdr->cdth_trk0, atari_hdr->cdth_trk1));
			}
		}
		break;
	
	case ATARI_CDROMREADTOCENTRY:
		{
			atari_cdromtocentry_t *atari_tocentry;
			CDROM_READ_TOC_EX hdr;
			CDROM_TOC toc;
			int i, numtracks;
			
			atari_tocentry = (atari_cdromtocentry_t *)  Atari2HostAddr(buffer);

			D(bug(NFCD_NAME " Ioctl(READTOCENTRY,0x%02x,0x%02x)", atari_tocentry->cdte_track, atari_tocentry->cdte_format));
			hdr.Format = CDROM_READ_TOC_EX_FORMAT_TOC;
			hdr.Reserved1 = 0;
			hdr.Msf = atari_tocentry->cdte_format == CDROM_LBA ? 0 : 1;
			hdr.SessionTrack = 0;
			hdr.Reserved2 = 0;
			hdr.Reserved3 = 0;
			errorcode = cd_winioctl(drive, IOCTL_CDROM_READ_TOC_EX, &hdr, sizeof(hdr), &toc, sizeof(toc));

			if (errorcode >= 0)
			{
				numtracks = toc.LastTrack - toc.FirstTrack + 1;
				for (i = 0; i < numtracks; i++)
				{
					if (toc.TrackData[i].TrackNumber == atari_tocentry->cdte_track)
					{
						Uint32 *p = (Uint32 *)toc.TrackData[i].Address;
						atari_tocentry->cdte_info = (toc.TrackData[i].Adr << 4) | toc.TrackData[i].Control;
						atari_tocentry->cdte_datamode = 0;
						atari_tocentry->dummy = 0;
						atari_tocentry->cdte_addr = *p;
						break;
					}
				}
				if (i >= numtracks)
					errorcode = TOS_EFILNF;
			}
		}
		break;

	case ATARI_CDROMSTOP:
		D(bug(NFCD_NAME " Ioctl(CDROMSTOP)"));
		errorcode = cd_winioctl(drive, IOCTL_CDROM_STOP_AUDIO);
		break;
	
	case ATARI_CDROMSTART:
		D(bug(NFCD_NAME " Ioctl(CDROMSTART)"));
		errorcode = cd_winioctl(drive, IOCTL_STORAGE_LOAD_MEDIA);
		break;
	
	case ATARI_CDROMEJECT:
		errorcode = cd_winioctl(drive, buffer != 0 ? IOCTL_STORAGE_LOAD_MEDIA : IOCTL_STORAGE_EJECT_MEDIA);
		break;

	case ATARI_CDROMVOLCTRL:
		{
			VOLUME_CONTROL volctrl;

			/* CDROMVOLCTRL function emulation */
			struct atari_cdrom_volctrl *v = (struct atari_cdrom_volctrl *) Atari2HostAddr(buffer);

			D(bug(NFCD_NAME " Ioctl(CDROMVOLCTRL)"));

			/* Write volume settings */
			volctrl.PortVolume[0] = v->channel0;
			volctrl.PortVolume[1] = v->channel1;
			volctrl.PortVolume[2] = v->channel2;
			volctrl.PortVolume[3] = v->channel3;

			errorcode = cd_winioctl(drive, IOCTL_CDROM_SET_VOLUME, &volctrl, sizeof(volctrl));
		}
		break;

	case ATARI_CDROMAUDIOCTRL:
		{
			VOLUME_CONTROL volctrl;
			atari_cdrom_audioctrl_t *atari_audioctrl;

			/* CDROMAUDIOCTRL function emulation */
			atari_audioctrl = (atari_cdrom_audioctrl_t *) Atari2HostAddr(buffer);

			D(bug(NFCD_NAME " Ioctl(CDROMAUDIOCTRL,0x%04x)", atari_audioctrl->set));

			if (atari_audioctrl->set == 0)
			{
				/* Read volume settings */
				errorcode = cd_winioctl(drive, IOCTL_CDROM_GET_VOLUME, NULL, 0, &volctrl, sizeof(volctrl));
				if (errorcode >= 0)
				{
					atari_audioctrl->channel[0].selection =
					atari_audioctrl->channel[1].selection =
					atari_audioctrl->channel[2].selection =
					atari_audioctrl->channel[3].selection = 0;
					atari_audioctrl->channel[0].volume = volctrl.PortVolume[0];
					atari_audioctrl->channel[1].volume = volctrl.PortVolume[1];
					atari_audioctrl->channel[2].volume = volctrl.PortVolume[2];
					atari_audioctrl->channel[3].volume = volctrl.PortVolume[3];
				}
			} else
			{
				/* Write volume settings */
				volctrl.PortVolume[0] = atari_audioctrl->channel[0].volume;
				volctrl.PortVolume[1] = atari_audioctrl->channel[1].volume;
				volctrl.PortVolume[2] = atari_audioctrl->channel[2].volume;
				volctrl.PortVolume[3] = atari_audioctrl->channel[3].volume;

				errorcode = cd_winioctl(drive, IOCTL_CDROM_SET_VOLUME, &volctrl, sizeof(volctrl));
			}
		}
		break;

	case ATARI_CDROMSUBCHNL:
		{
			CDROM_SUB_Q_DATA_FORMAT subchnl;
			SUB_Q_CHANNEL_DATA data;
			atari_cdromsubchnl_t *atari_subchnl;
			
			atari_subchnl = (atari_cdromsubchnl_t *) Atari2HostAddr(buffer);

			subchnl.Format = IOCTL_CDROM_CURRENT_POSITION;
			subchnl.Track = 0;
			
			D(bug(NFCD_NAME " Ioctl(READSUBCHNL,0x%02x)", atari_subchnl->cdsc_format));

			errorcode = cd_winioctl(drive, IOCTL_CDROM_READ_Q_CHANNEL, &subchnl, sizeof(subchnl), &data, sizeof(data));

			if (errorcode >= 0)
			{
				Uint32 *p;
				
				atari_subchnl->cdsc_audiostatus = data.CurrentPosition.Header.AudioStatus;
				atari_subchnl->cdsc_resvd = 0;
				atari_subchnl->cdsc_info = (data.CurrentPosition.ADR & 0x0f)<<4;
				atari_subchnl->cdsc_info |= data.CurrentPosition.Control & 0x0f;
				atari_subchnl->cdsc_trk = data.CurrentPosition.TrackNumber;
				atari_subchnl->cdsc_ind = data.CurrentPosition.IndexNumber;
				
				p = (Uint32 *)data.CurrentPosition.AbsoluteAddress;
				atari_subchnl->cdsc_absaddr = *p;
				p = (Uint32 *)data.CurrentPosition.TrackRelativeAddress;
				atari_subchnl->cdsc_reladdr = *p;
				if (atari_subchnl->cdsc_format != CDROM_LBA)
				{
					int minute, second, frame;
					
					FRAMES_TO_MSF(SDL_SwapBE32(atari_subchnl->cdsc_absaddr), &minute, &second, &frame);
					atari_subchnl->cdsc_absaddr = SDL_SwapBE32((minute<<16)|(second<<8)|frame);
					FRAMES_TO_MSF(SDL_SwapBE32(atari_subchnl->cdsc_reladdr), &minute, &second, &frame);
					atari_subchnl->cdsc_reladdr = SDL_SwapBE32((minute<<16)|(second<<8)|frame);
				}
				D(bug(NFCD_NAME "Ioctl(CDROMSUBCHNL): Format=%d, Status=0x%02x, Info=0x%02x, Trk=%d, Ind=%d, Abs=0x%08px, Rel=0x%08x",
					atari_subchnl->cdsc_format,
					atari_subchnl->cdsc_audiostatus,
					atari_subchnl->cdsc_info,
					atari_subchnl->cdsc_trk,
					atari_subchnl->cdsc_ind,
					SDL_SwapBE32(atari_subchnl->cdsc_absaddr),
					SDL_SwapBE32(atari_subchnl->cdsc_reladdr)));
			}
		}
		break;

	case ATARI_CDROMREADMODE2:
	case ATARI_CDROMREADMODE1:
		{
			atari_cdrom_read_t *atari_cdread;
			RAW_READ_INFO info;
			char *buf;
			
			/*
			 * TODO: verify this. linux/cdrom.h and mint/cdromio.h state that this ioctl()
			 * takes struct cdrom_read as input, but the drivers actually expect a struct cdrom_msf
			 */
			atari_cdread = (atari_cdrom_read_t *) Atari2HostAddr(buffer);
			info.DiskOffset.QuadPart = (uint64_t)SDL_SwapBE32(atari_cdread->cdread_lba) * CD_FRAMESIZE;
			info.SectorCount = SDL_SwapBE32(atari_cdread->cdread_buflen);
			buf = (char *)Atari2HostAddr(SDL_SwapBE32(atari_cdread->cdread_bufaddr));
			
			D(bug(NFCD_NAME " Ioctl(%s)", opcode == ATARI_CDROMREADMODE1 ? "CDROMREADMODE1" : "CDROMREADMODE2"));
			if (opcode == ATARI_CDROMREADMODE1)
			{
				info.TrackMode = YellowMode2;
				errorcode = cd_winioctl(drive, IOCTL_CDROM_RAW_READ, &info, sizeof(info), buf, info.SectorCount * CD_FRAMESIZE_RAWER);
			} else
			{
				info.TrackMode = XAForm2;
				errorcode = cd_winioctl(drive, IOCTL_CDROM_RAW_READ, &info, sizeof(info), buf, info.SectorCount * CD_FRAMESIZE_RAWER);
			}
		}
		break;

	case ATARI_CDROMPREVENTREMOVAL:
	case ATARI_CDROMALLOWREMOVAL:
		{
			PREVENT_MEDIA_REMOVAL allow;
			
			allow.PreventMediaRemoval = ATARI_CDROMPREVENTREMOVAL ? TRUE : FALSE;
			errorcode = cd_winioctl(drive, IOCTL_STORAGE_EJECTION_CONTROL, &allow, sizeof(allow));
		}
		break;

	case ATARI_CDROMREADDA:
		{
			atari_cdrom_msf_t *atari_msf;
			RAW_READ_INFO info;
			char *buf;
			ULONG lba;
			
			atari_msf = (atari_cdrom_msf_t *)Atari2HostAddr(buffer);
			lba = MSF_TO_FRAMES(atari_msf->cdmsf_min0, atari_msf->cdmsf_sec0, atari_msf->cdmsf_frame0);
			info.DiskOffset.QuadPart = (uint64_t)lba * CD_FRAMESIZE;
			info.TrackMode = CDDA;
			/* not sure about this, but on linux this command reads 1 sector into the same buffer that was passed as input */
			info.SectorCount = 1;
			buf = (char *)atari_msf;
			D(bug(NFCD_NAME " Ioctl(CDROMREADDA)"));
			errorcode = cd_winioctl(drive, IOCTL_CDROM_RAW_READ, &info, sizeof(info), buf, info.SectorCount * CD_FRAMESIZE_RAW);
		}
		break;
	
	case ATARI_CDROMRESET:
		D(bug(NFCD_NAME " Ioctl(CDROMRESET)"));
		break;

	case ATARI_CDROMGETMCN:
		{
			CDROM_SUB_Q_DATA_FORMAT sub;
			SUB_Q_CHANNEL_DATA data;
			atari_mcn_t	*atari_mcn;

			atari_mcn = (atari_mcn_t *) Atari2HostAddr(buffer);
			memset(atari_mcn, 0, sizeof(atari_mcn_t));
			sub.Format = IOCTL_CDROM_MEDIA_CATALOG;
			sub.Track = 0;
			D(bug(NFCD_NAME " Ioctl(CDROMGETMCN)"));
			errorcode = cd_winioctl(drive, IOCTL_CDROM_READ_Q_CHANNEL, &sub, sizeof(sub), &data, sizeof(data));
			if (errorcode >= 0)
			{
				atari_mcn->audiostatus = data.MediaCatalog.Header.AudioStatus;
				Host2AtariSafeStrncpy(buffer + offsetof(atari_mcn_t, mcn), (const char *)data.MediaCatalog.MediaCatalog, MIN(sizeof(atari_mcn->mcn), sizeof(data.MediaCatalog.MediaCatalog)));
			}
		}
		break;

	case ATARI_CDROMGETTISRC:
		{
			atari_tisrc_t *tisrc = (atari_tisrc_t *)Atari2HostAddr(buffer);
			CDROM_SUB_Q_DATA_FORMAT sub;
			SUB_Q_CHANNEL_DATA data;
			
			sub.Format = IOCTL_CDROM_TRACK_ISRC;
			sub.Track = tisrc->tisrc_track;
			errorcode = cd_winioctl(drive, IOCTL_CDROM_READ_Q_CHANNEL, &sub, sizeof(sub), &data, sizeof(data));
			if (errorcode >= 0)
			{
				tisrc->tisrc_audiostatus = data.TrackIsrc.Header.AudioStatus;
				Host2AtariSafeStrncpy(buffer + offsetof(atari_tisrc_t, tisrc_tisrc), (const char *)data.TrackIsrc.TrackIsrc, MIN(sizeof(tisrc->tisrc_tisrc), sizeof(data.TrackIsrc.TrackIsrc)));
			}
		}
		break;
	
	default:
		D(bug(NFCD_NAME "Ioctl(): ioctl 0x%04x unsupported", opcode));
		break;
	}
	CloseDrive(drive);
	return errorcode;
}


int CdromDriverWin32::cd_win_playtracks(int drive, unsigned char first, unsigned char last)
{
	CDROM_PLAY_AUDIO_MSF cd_msf;
	CDROM_READ_TOC_EX hdr;
	CDROM_TOC toc;
	int i, errorcode, numtracks;
	
	/* Read TOC header */
	hdr.Format = CDROM_READ_TOC_EX_FORMAT_TOC;
	hdr.Reserved1 = 0;
	hdr.Msf = 1;
	hdr.SessionTrack = 0;
	hdr.Reserved2 = 0;
	hdr.Reserved3 = 0;
	errorcode = cd_winioctl(drive, IOCTL_CDROM_READ_TOC_EX, &hdr, sizeof(hdr), &toc, sizeof(toc));
	if (errorcode < 0)
		return errorcode;
	numtracks = toc.LastTrack - toc.FirstTrack + 1;
	for (i = 0; i < numtracks; i++)
		if (toc.TrackData[i].TrackNumber == first)
		{
			cd_msf.StartingM = toc.TrackData[i].Address[1];
			cd_msf.StartingS = toc.TrackData[i].Address[2];
			cd_msf.StartingF = toc.TrackData[i].Address[3];
			break;
		}
	if (i >= numtracks)
		return TOS_EFILNF;
	for (i = 0; i < numtracks; i++)
		if (toc.TrackData[i].TrackNumber == last)
		{
			cd_msf.EndingM = toc.TrackData[i + 1].Address[1];
			cd_msf.EndingS = toc.TrackData[i + 1].Address[2];
			cd_msf.EndingF = toc.TrackData[i + 1].Address[3];
			break;
		}
	if (i >= numtracks)
		return TOS_EFILNF;

	errorcode = cd_winioctl(drive, IOCTL_CDROM_PLAY_AUDIO_MSF, &cd_msf, sizeof(cd_msf));
	return errorcode;
}


int32 CdromDriverWin32::cd_startaudio(memptr device, uint32 dummy, memptr buffer)
{
	UNUSED(dummy);
	int drive, errorcode;
	metados_bos_tracks_t	*atari_track_index;

	drive = OpenDrive(device);
	if (drive < 0)
		return drive;

	atari_track_index = (metados_bos_tracks_t *) Atari2HostAddr(buffer);

	errorcode = cd_win_playtracks(drive, atari_track_index->first, atari_track_index->first + atari_track_index->count - 1);
	CloseDrive(drive);
	return errorcode;
}


int32 CdromDriverWin32::cd_stopaudio(memptr device)
{
	int drive, errorcode;

	drive = OpenDrive(device);
	if (drive < 0)
		return drive;

	errorcode = cd_winioctl(drive, IOCTL_CDROM_STOP_AUDIO);
	CloseDrive(drive);
	return errorcode;
}


int32 CdromDriverWin32::cd_setsongtime(memptr device, uint32 dummy, uint32 start_msf, uint32 end_msf)
{
	UNUSED(dummy);
	int drive, errorcode;
	CDROM_PLAY_AUDIO_MSF cd_msf;

	drive = OpenDrive(device);
	if (drive < 0)
		return drive;

	cd_msf.StartingM = (start_msf>>16) & 0xff;
	cd_msf.StartingS = (start_msf>>8) & 0xff;
	cd_msf.StartingF = (start_msf>>0) & 0xff;
	D(bug(NFCD_NAME " start: %02d:%02d:%02d", (start_msf>>16) & 0xff, (start_msf>>8) & 0xff, start_msf & 0xff));

	cd_msf.EndingM = (end_msf>>16) & 0xff;
	cd_msf.EndingS = (end_msf>>8) & 0xff;
	cd_msf.EndingF = (end_msf>>0) & 0xff;
	D(bug(NFCD_NAME " end:   %02d:%02d:%02d", (end_msf>>16) & 0xff, (end_msf>>8) & 0xff, end_msf & 0xff));

	errorcode = cd_winioctl(drive, IOCTL_CDROM_PLAY_AUDIO_MSF, &cd_msf, sizeof(cd_msf));
	CloseDrive(drive);
	return errorcode;
}


int32 CdromDriverWin32::cd_gettoc(memptr device, uint32 dummy, memptr buffer)
{
	int drive, i, numtracks, errorcode;
	CDROM_READ_TOC_EX hdr;
	CDROM_TOC toc;
	atari_tocentry_t *atari_tocentry;

	UNUSED(dummy);
	drive = OpenDrive(device);
	if (drive < 0)
		return drive;

	/* Read TOC header */
	hdr.Format = CDROM_READ_TOC_EX_FORMAT_TOC;
	hdr.Reserved1 = 0;
	hdr.Msf = 1;
	hdr.SessionTrack = 0;
	hdr.Reserved2 = 0;
	hdr.Reserved3 = 0;
	errorcode = cd_winioctl(drive, IOCTL_CDROM_READ_TOC_EX, &hdr, sizeof(hdr), &toc, sizeof(toc));
	if (errorcode < 0)
	{
		CloseDrive(drive);
		return errorcode;
	}

	D(bug(NFCD_NAME "GetToc():  TOC header read"));

	numtracks = toc.LastTrack - toc.FirstTrack + 1;

	/* Read TOC entries */
	atari_tocentry = (atari_tocentry_t *) Atari2HostAddr(buffer);
	for (i = 0; i <= numtracks; i++)
	{
		atari_tocentry[i].track = toc.TrackData[i].TrackNumber;
		atari_tocentry[i].minute = BinaryToBcd(toc.TrackData[i].Address[1]);
		atari_tocentry[i].second = BinaryToBcd(toc.TrackData[i].Address[2]);
		atari_tocentry[i].frame = BinaryToBcd(toc.TrackData[i].Address[3]);
		if (i == numtracks)
			atari_tocentry[i].track = CDROM_LEADOUT_CDAR;

		if (hdr.Msf)
		{
			D(bug(NFCD_NAME "GetToc():    %02x: msf %02d:%02d:%02d, bcd: %02x:%02x:%02x",
				atari_tocentry[i].track,
				toc.TrackData[i].Address[1],
				toc.TrackData[i].Address[2],
				toc.TrackData[i].Address[3],
				atari_tocentry[i].minute,
				atari_tocentry[i].second,
				atari_tocentry[i].frame
			));
		} else
		{
			D(bug(NFCD_NAME "GetToc():    lba 0x%08x", *(Uint32 *)toc.TrackData[i].Address));
		}
	}

	atari_tocentry[numtracks+1].track =
	atari_tocentry[numtracks+1].minute =
	atari_tocentry[numtracks+1].second = 
	atari_tocentry[numtracks+1].frame = 0;

	CloseDrive(drive);
	return TOS_E_OK;
}


int32 CdromDriverWin32::cd_discinfo(memptr device, memptr buffer)
{
	int drive, errorcode;
	CDROM_READ_TOC_EX hdr;
	CDROM_TOC toc;
	CDROM_SUB_Q_DATA_FORMAT subchnl;
	SUB_Q_CHANNEL_DATA data;
	atari_discinfo_t *discinfo;
	int minute, second, frame, numtracks;

	drive = OpenDrive(device);
	if (drive < 0)
		return drive;

	/* Read TOC header */
	hdr.Format = CDROM_READ_TOC_EX_FORMAT_TOC;
	hdr.Reserved1 = 0;
	hdr.Msf = 1;
	hdr.SessionTrack = 0;
	hdr.Reserved2 = 0;
	hdr.Reserved3 = 0;
	errorcode = cd_winioctl(drive, IOCTL_CDROM_READ_TOC_EX, &hdr, sizeof(hdr), &toc, sizeof(toc));
	if (errorcode < 0)
	{
		CloseDrive(drive);
		return errorcode;
	}
	D(bug(NFCD_NAME "DiscInfo():  TOC header read"));

	discinfo = (atari_discinfo_t *) Atari2HostAddr(buffer);
	memset(discinfo, 0, sizeof(atari_discinfo_t));

	discinfo->first = discinfo->current = toc.FirstTrack;
	discinfo->last = toc.LastTrack;
	numtracks = toc.LastTrack - toc.FirstTrack + 1;
	discinfo->index = 1;

	/* Read subchannel */
	subchnl.Format = IOCTL_CDROM_CURRENT_POSITION;
	subchnl.Track = 0;
	errorcode = cd_winioctl(drive, IOCTL_CDROM_READ_Q_CHANNEL, &subchnl, sizeof(subchnl), &data, sizeof(data));
	if (errorcode < 0)
	{
		CloseDrive(drive);
		return errorcode;
	}
	D(bug(NFCD_NAME "DiscInfo():  Subchannel read"));

	discinfo->current = data.CurrentPosition.TrackNumber;	/* current track */
	discinfo->index = data.CurrentPosition.IndexNumber;	/* current index */

	Uint32 *p = (Uint32 *)data.CurrentPosition.TrackRelativeAddress;
	FRAMES_TO_MSF(SDL_SwapBE32(*p), &minute, &second, &frame);
	discinfo->relative.track = 0;
	discinfo->relative.minute = BinaryToBcd(minute);
	discinfo->relative.second = BinaryToBcd(second);
	discinfo->relative.frame = BinaryToBcd(frame);

	p = (Uint32 *)data.CurrentPosition.AbsoluteAddress;
	FRAMES_TO_MSF(SDL_SwapBE32(*p), &minute, &second, &frame);
	discinfo->absolute.track = 0;
	discinfo->absolute.minute = BinaryToBcd(minute);
	discinfo->absolute.second = BinaryToBcd(second);
	discinfo->absolute.frame = BinaryToBcd(frame);

	/* Read toc entry for start of disc, to select disc type */
	discinfo->disctype = ((toc.TrackData[0].Control & AUDIO_DATA_TRACK) == AUDIO_DATA_TRACK);

	/* Read toc entry for end of disc */
	discinfo->end.track = 0;
	discinfo->end.minute = BinaryToBcd(toc.TrackData[numtracks].Address[1]);
	discinfo->end.second = BinaryToBcd(toc.TrackData[numtracks].Address[2]);
	discinfo->end.frame = BinaryToBcd(toc.TrackData[numtracks].Address[3]);

	D(bug(NFCD_NAME "DiscInfo():  first=%d, last=%d, current=%d, ind=%d, rel=%02x:%02x:%02x, abs=%02x:%02x:%02x, end=%02x:%02x:%02x",
		discinfo->first, discinfo->last, discinfo->current, discinfo->index,
		discinfo->relative.minute, discinfo->relative.second, discinfo->relative.frame,
		discinfo->absolute.minute, discinfo->absolute.second, discinfo->absolute.frame,
		discinfo->end.minute, discinfo->end.second, discinfo->end.frame));

	CloseDrive(drive);
	return TOS_E_OK;
}

/*
vim:ts=4:sw=4:
*/
