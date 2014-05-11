/*
	NatFeat host CD-ROM access

	ARAnyM (C) 2003 Patrice Mandin

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
#include "cpu_emulation.h"
#include "parameters.h"
#include "nfcdrom.h"
#include "nfcdrom_atari.h"
#include "../../atari/nfcdrom/nfcdrom_nfapi.h"
#include "toserror.h"

#define DEBUG 0
#include "debug.h"

#include <cstdlib>
#include <errno.h>
#include <SDL_cdrom.h>
#include <SDL_endian.h>

/*--- Defines ---*/

#define NFCD_NAME	"nf:cdrom: "

#define CDROM_LEADOUT	0xaa

/*--- Constructor/desctructor ---*/

CdromDriver::CdromDriver(void)
{
	D(bug(NFCD_NAME "CdromDriver()"));
	drives_mask = 0xffffffffUL;
	for (int i=0; i<CD_MAX_DRIVES; i++) {
		drive_handles[i]=NULL;
	}
}

CdromDriver::~CdromDriver(void)
{
	D(bug(NFCD_NAME "~CdromDriver()"));
	for (int i=0; i<CD_MAX_DRIVES; i++) {
		CloseDrive(i);
	}
}

/*--- Public functions ---*/

int32 CdromDriver::dispatch(uint32 fncode)
{
	int32 ret;

	D(bug(NFCD_NAME "dispatch(%u)", fncode));

	switch(fncode) {
		case GET_VERSION:
    		ret = ARANFCDROM_NFAPI_VERSION;
			break;
		case NFCD_OPEN:
			ret = cd_open(getParameter(0),getParameter(1));
			break;
		case NFCD_CLOSE:
			ret = cd_close(getParameter(0));
			break;
		case NFCD_READ:
			ret = cd_read(getParameter(0),getParameter(1),getParameter(2),getParameter(3));
			break;
		case NFCD_WRITE:
			ret = cd_write(getParameter(0),getParameter(1),getParameter(2),getParameter(3));
			break;
		case NFCD_SEEK:
			ret = cd_seek(getParameter(0),getParameter(1));
			break;
		case NFCD_STATUS:
			ret = cd_status(getParameter(0),getParameter(1));
			break;
		case NFCD_IOCTL:
			ret = cd_ioctl(getParameter(0),getParameter(1),getParameter(2));
			break;
		case NFCD_STARTAUDIO:
			ret = cd_startaudio(getParameter(0),getParameter(1),getParameter(2));
			break;
		case NFCD_STOPAUDIO:
			ret = cd_stopaudio(getParameter(0));
			break;
		case NFCD_SETSONGTIME:
			ret = cd_setsongtime(getParameter(0),getParameter(1),getParameter(2),getParameter(3));
			break;
		case NFCD_GETTOC:
			ret = cd_gettoc(getParameter(0),getParameter(1),getParameter(2));
			break;
		case NFCD_DISCINFO:
			ret = cd_discinfo(getParameter(0),getParameter(1));
			break;
		case NFCD_DRIVESMASK:
			if (drives_mask == 0xffffffffUL) {
				ScanDrives();
			}
			ret = drives_mask;
			break;
		default:
			D(bug(NFCD_NAME " unimplemented function #%d", fncode));
			ret = TOS_ENOSYS;
			break;
	}
	D(bug(NFCD_NAME " function returning with 0x%08x", ret));
	return ret;
}

/*--- Protected functions ---*/

int CdromDriver::GetDrive(memptr device)
{
	metados_bos_header_t bos_device;

/*	D(bug(NFCD_NAME "GetDrive()"));*/
	Atari2Host_memcpy(&bos_device, device, sizeof(bos_device));
	return SDL_SwapBE16(bos_device.phys_letter);
}

unsigned char CdromDriver::BinaryToBcd(unsigned char value)
{
	int decimal;
	
/*	D(bug(NFCD_NAME "BinaryToBcd()"));*/
	decimal = (value/10)*16;
	return decimal | (value % 10);
}

void CdromDriver::ScanDrives()
{
	int i;

	D(bug(NFCD_NAME "ScanDrives()"));

	drives_mask = 0;
	for (i=0; i<CD_MAX_DRIVES; i++) {
		/* Check if there is a valid filename */
		if (bx_options.nfcdroms[i].physdevtohostdev < 0) {
			continue;
		}

		/* Add to MetaDOS CD-ROM drives */
		drives_mask |= 1<<(i);
		bug(NFCD_NAME "ScanDrives(): physical device %c (%s) added", DriveToLetter(i), SDL_CDName(bx_options.nfcdroms[i].physdevtohostdev));
	}

	D(bug(NFCD_NAME "ScanDrives()=0x%08x", drives_mask));
}

int CdromDriver::OpenDrive(memptr device)
{
	int drive;

	drive = GetDrive(device);
	drive = DriveFromLetter(drive);

	/* Drive exist ? */
	if (drive < 0 || drive >= CD_MAX_DRIVES || (drives_mask & (1<<drive))==0) {
		D(bug(NFCD_NAME " physical device %c does not exist", GetDrive(device)));
		return TOS_ENOSYS;
	}

	/* Drive opened ? */
	if (drive_handles[drive]) {
		return drive;
	}

	/* Open drive */
	drive_handles[drive]=SDL_CDOpen(bx_options.nfcdroms[drive].physdevtohostdev);
	if (drive_handles[drive]==NULL) {
		D(bug(NFCD_NAME " error opening SDL CD drive %d (%s)", bx_options.nfcdroms[drive].physdevtohostdev, SDL_CDName(bx_options.nfcdroms[drive].physdevtohostdev)));
		return TOS_ENOSYS;
	}

	D(bug(NFCD_NAME "DriveOpened: %c", DriveToLetter(drive)));
	return drive;
}

void CdromDriver::CloseDrive(int drive)
{
/*	D(bug(NFCD_NAME "CloseDrive(%c)", DriveToLetter(drive)));*/

	/* Drive already closed ? */
	if (drive_handles[drive]==NULL) {
		return;
	}

	SDL_CDClose(drive_handles[drive]);
	drive_handles[drive]=NULL;
}

/*--- MetaDOS functions ---*/

int32 CdromDriver::cd_open(memptr device, memptr buffer)
{
	UNUSED(device);
	UNUSED(buffer);
	D(bug(NFCD_NAME "cd_open()"));
	return TOS_E_OK;
}

int32 CdromDriver::cd_close(memptr device)
{
	UNUSED(device);
	D(bug(NFCD_NAME "cd_close()"));
	return TOS_E_OK;
}

int32 CdromDriver::cd_read(memptr device, memptr buffer, uint32 first, uint32 length)
{
	UNUSED(device);
	UNUSED(buffer);
	UNUSED(first);
	UNUSED(length);
	/* No CD-ROM support using SDL functions */
	return TOS_ENOSYS;
}

int32 CdromDriver::cd_write(memptr device, memptr buffer, uint32 first, uint32 length)
{
	UNUSED(device);
	UNUSED(buffer);
	UNUSED(first);
	UNUSED(length);
	D(bug(NFCD_NAME "cd_write()"));
	return TOS_ENOSYS;
}

int32 CdromDriver::cd_seek(memptr device, uint32 offset)
{
	UNUSED(device);
	UNUSED(offset);
	D(bug(NFCD_NAME "cd_seek()"));
	return TOS_ENOSYS;
}

int32 CdromDriver::cd_status(memptr device, memptr ext_status)
{
	UNUSED(ext_status);
	int drive, mediachanged;
	CDstatus status;

	D(bug(NFCD_NAME "Status()"));
	drive = OpenDrive(device);
	if (drive<0) {
		return TOS_ENOSYS;
	}

	mediachanged = 1;	/* Can not check media changed: return always changed */

	status = SDL_CDStatus(drive_handles[drive]);
	CloseDrive(drive);

	if  ((status == CD_TRAYEMPTY) || (status == CD_ERROR)) {
		return TOS_EDRVNR;
	}

	return (mediachanged<<3);
}

int32 CdromDriver::cd_ioctl(memptr device, uint16 opcode, memptr buffer)
{
	int drive, errorcode;
	CDstatus status;
	SDL_CD	*cur_cd;

	D(bug(NFCD_NAME "Ioctl(0x%04x)", opcode));
	drive = OpenDrive(device);
	if (drive<0) {
		return TOS_ENOSYS;
	}

	cur_cd = drive_handles[drive];
	/*
	 * brain-damaged SDL error system, which sometimes just calls
	 * SDL_SetError() and does not set errno, and only returns
	 * strings as error codes, requires us to clear errno
	 */
	errno = 0;
	
	status = SDL_CDStatus(cur_cd);

	errorcode = TOS_ENOSYS;
	switch(opcode) {
		case ATARI_CDROMREADOFFSET:	/* CDROMREADOFFSET */
			break;
		case ATARI_CDROMPAUSE:	/* CDROMPAUSE */
			{
				errorcode=SDL_CDPause(cur_cd);
			}
			break;
		case ATARI_CDROMRESUME:	/* CDROMRESUME */
			{
				errorcode=SDL_CDResume(cur_cd);
			}
			break;
		case ATARI_CDROMPLAYMSF:	/* CDROMPLAYMSF */
			{
				unsigned char *atari_msf;
				int start, length;

				atari_msf = (unsigned char *) Atari2HostAddr(buffer);
				D(bug(NFCD_NAME " Ioctl(CDROMPLAYMSF)"));

				start = MSF_TO_FRAMES(atari_msf[0],atari_msf[1],atari_msf[2]);
				length = MSF_TO_FRAMES(atari_msf[3],atari_msf[4],atari_msf[5])-start;
				
				errorcode=SDL_CDPlay(cur_cd, start, length);
			}
			break;
		case ATARI_CDROMPLAYTRKIND:	/* CDROMPLAYTRKIND */
			{
				unsigned char *atari_ti;
				int start, length, i;

				atari_ti = (unsigned char *) Atari2HostAddr(buffer);
				D(bug(NFCD_NAME " Ioctl(CDROMPLAYTRKIND)"));

				start = cur_cd->track[atari_ti[0]].offset;
				length = 0;
				for (i=atari_ti[0]; i<atari_ti[2]; i++) {
					length += cur_cd->track[i].length;
				}

				errorcode=SDL_CDPlay(cur_cd, start, length);
			}
			break;
		case ATARI_CDROMREADTOCHDR:	/* CDROMREADTOCHDR */
			{
				atari_cdromtochdr_t	*atari_tochdr;

				atari_tochdr = (atari_cdromtochdr_t *) Atari2HostAddr(buffer);
				D(bug(NFCD_NAME " Ioctl(READTOCHDR)"));

				if CD_INDRIVE(status) {
					atari_tochdr->cdth_trk0 = cur_cd->track[0].id;
					atari_tochdr->cdth_trk1 = cur_cd->track[cur_cd->numtracks-1].id;
					errorcode=0;
				} else {
					errorcode=TOS_EDRVNR;
				}
			}
			break;
		case ATARI_CDROMREADTOCENTRY:	/* CDROMREADTOCENTRY */
			{
				unsigned char	*atari_tocentry;
				int i, minute, second, frame;

				atari_tocentry = (unsigned char	*) Atari2HostAddr(buffer);
				D(bug(NFCD_NAME " Ioctl(READTOCENTRY)"));

				atari_tocentry[2] = 0;
				atari_tocentry[3] = 0;
				atari_tocentry[4] = atari_tocentry[5] = 0;
				atari_tocentry[6] = atari_tocentry[7] = atari_tocentry[8] = atari_tocentry[9] =0;
				errorcode=TOS_EDRVNR;
				if CD_INDRIVE(status) {
					/* Search the track */
					for (i=0; i<=cur_cd->numtracks; i++) {
						if (cur_cd->track[i].id == atari_tocentry[0]) {
							atari_tocentry[2] = cur_cd->track[i].type & 0x0f;
							if (atari_tocentry[1] == CDROM_LBA) {
								*((unsigned long *) &atari_tocentry[6]) = SDL_SwapBE32(cur_cd->track[i].offset);
							} else {
								FRAMES_TO_MSF(cur_cd->track[i].offset, &minute, &second, &frame);
								atari_tocentry[7] = minute;
								atari_tocentry[8] = second;
								atari_tocentry[9] = frame;
							}
							errorcode=0;
							break;
						}
					}
				}
			}
			break;
		case ('C'<<8)|0x07:	/* CDROMSTOP */
			{
				errorcode=SDL_CDStop(cur_cd);
			}
			break;
		case ATARI_CDROMSTART: 	/* CDROMSTART */
			break;
		case ATARI_CDROMEJECT:	/* CDROMEJECT */
			{
				errorcode=SDL_CDEject(cur_cd);
			}
			break;
		case ATARI_CDROMVOLCTRL:	/* CDROMVOLCTRL */
			break;
		case ATARI_CDROMSUBCHNL:	/* CDROMSUBCHNL */
			{
				unsigned char *atari_subchnl;
				int minute, second, frame, abs_addr, rel_addr;
				
				atari_subchnl = (unsigned char *) Atari2HostAddr(buffer);
				D(bug(NFCD_NAME " Ioctl(READSUBCHNL,0x%02x)", atari_subchnl[0]));

				atari_subchnl[1] = CDROM_AUDIO_INVALID;	/* audiostatus */
				atari_subchnl[5] = 1;	/* index */
				atari_subchnl[2] =		/* reserved */
					atari_subchnl[3] =	/* adr+ctrl */
					atari_subchnl[4] =	/* track */
					atari_subchnl[6] = 	/* abs_addr */
					atari_subchnl[7] =
					atari_subchnl[8] =
					atari_subchnl[9] =
					atari_subchnl[10] = 	/* rel_addr */
					atari_subchnl[11] =
					atari_subchnl[12] =
					atari_subchnl[13] = 0;

				switch(status) {
					case CD_TRAYEMPTY:
						atari_subchnl[1] = CDROM_AUDIO_NO_STATUS;
						break;
					case CD_STOPPED:
						atari_subchnl[1] = CDROM_AUDIO_COMPLETED;
						break;
					case CD_PLAYING:
						atari_subchnl[1] = CDROM_AUDIO_PLAY;
						break;
					case CD_PAUSED:
						atari_subchnl[1] = CDROM_AUDIO_PAUSED;
						break;
					case CD_ERROR:
						atari_subchnl[1] = CDROM_AUDIO_ERROR;
						break;
				}
				if CD_INDRIVE(status) {
					rel_addr = cur_cd->cur_frame;
					abs_addr = cur_cd->track[cur_cd->cur_track].offset + rel_addr;

					atari_subchnl[4] = cur_cd->track[cur_cd->cur_track].id;
					atari_subchnl[3] = cur_cd->track[cur_cd->cur_track].type & 0x0f;
					if (atari_subchnl[0] == CDROM_LBA) {
						*((unsigned long *) &atari_subchnl[6]) =
							SDL_SwapBE32(abs_addr);
						*((unsigned long *) &atari_subchnl[10]) = 
							SDL_SwapBE32(rel_addr);
					} else {
						FRAMES_TO_MSF(abs_addr, &minute, &second, &frame);
						*((unsigned long *) &atari_subchnl[6]) = 
							SDL_SwapBE32((minute<<16)|(second<<8)|frame);
						FRAMES_TO_MSF(rel_addr, &minute, &second, &frame);
						*((unsigned long *) &atari_subchnl[10]) = 
							SDL_SwapBE32((minute<<16)|(second<<8)|frame);
					}
					errorcode=0;
				} else {
					errorcode=TOS_EDRVNR;
				}
			}
			break;
		case ATARI_CDROMREADMODE2:	/* CDROMREADMODE2 */
		case ATARI_CDROMREADMODE1:	/* CDROMREADMODE1 */
			break;
		case ATARI_CDROMPREVENTREMOVAL:	/* CDROMPREVENTREMOVAL */
			break;
		case ATARI_CDROMALLOWREMOVAL:	/* CDROMALLOWREMOVAL */
			break;
		case ATARI_CDROMAUDIOCTRL:	/* CDROMAUDIOCTRL */
			break;
		case ATARI_CDROMREADDA:	/* CDROMREADDA */
			break;
		case ATARI_CDROMRESET:	/* unused */
			break;
		case ATARI_CDROMGETMCN:	/* CDROMGETMCN */
			break;
		case ATARI_CDROMGETTISRC:	/* CDROMGETTISRC */
			break;
	}
	if (errorcode == CD_ERROR)
	{
		if (errno != 0)
			errorcode = errnoHost2Mint(errno, TOS_EDRVNR);
		else
			errorcode = TOS_EDRVNR;
	}
	CloseDrive(drive);
	return errorcode;
}

int32 CdromDriver::cd_startaudio(memptr device, uint32 dummy, memptr buffer)
{
	UNUSED(dummy);
	int drive, errorcode;
	metados_bos_tracks_t	*atari_track_index;

	D(bug(NFCD_NAME "StartAudio()"));
	drive = OpenDrive(device);
	if (drive<0) {
		return TOS_ENOSYS;
	}

	if (CD_INDRIVE(SDL_CDStatus(drive_handles[drive]))<=0) {
		return TOS_ENOSYS;
	}

	atari_track_index = (metados_bos_tracks_t *) Atari2HostAddr(buffer);

	errno = 0;
	errorcode = SDL_CDPlayTracks(drive_handles[drive],
		atari_track_index->first, 0,
		atari_track_index->count, 0);

	if (errorcode == CD_ERROR)
	{
		if (errno != 0)
			errorcode = errnoHost2Mint(errno, TOS_EDRVNR);
		else
			errorcode = TOS_EDRVNR;
	}
	CloseDrive(drive);
	return errorcode;
}

int32 CdromDriver::cd_stopaudio(memptr device)
{
	int drive, errorcode;

	D(bug(NFCD_NAME "StopAudio()"));
	drive = OpenDrive(device);
	if (drive<0) {
		return TOS_ENOSYS;
	}

	errno = 0;
	errorcode = SDL_CDStop(drive_handles[drive]);
	if (errorcode == CD_ERROR)
	{
		if (errno != 0)
			errorcode = errnoHost2Mint(errno, TOS_EDRVNR);
		else
			errorcode = TOS_EDRVNR;
	}

	CloseDrive(drive);
	return errorcode;
}

int32 CdromDriver::cd_setsongtime(memptr device, uint32 dummy, uint32 start_msf, uint32 end_msf)
{
	UNUSED(dummy);
	int drive, errorcode, start, length;

	D(bug(NFCD_NAME "SetSongTime()"));
	drive = OpenDrive(device);
	if (drive<0) {
		return TOS_ENOSYS;
	}

	errno = 0;
	if (CD_INDRIVE(SDL_CDStatus(drive_handles[drive]))<=0) {
		return TOS_ENOSYS;
	}

	start = MSF_TO_FRAMES((start_msf>>16) & 0xff, (start_msf>>8) & 0xff, (start_msf>>0) & 0xff);
	length = start;
	length += MSF_TO_FRAMES((end_msf>>16) & 0xff, (end_msf>>8) & 0xff, (end_msf>>0) & 0xff);

	errorcode=SDL_CDPlay(drive_handles[drive], start, length);
	if (errorcode == CD_ERROR)
	{
		if (errno != 0)
			errorcode = errnoHost2Mint(errno, TOS_EDRVNR);
		else
			errorcode = TOS_EDRVNR;
	}
	CloseDrive(drive);
	return errorcode;
}

int32 CdromDriver::cd_gettoc(memptr device, uint32 dummy, memptr buffer)
{
	UNUSED(dummy);
	int drive, i;
	SDL_CD	*cur_cd;
	atari_tocentry_t *atari_tocentry;

	D(bug(NFCD_NAME "GetToc()"));
	drive = OpenDrive(device);
	if (drive<0) {
		return TOS_ENOSYS;
	}

	if (CD_INDRIVE(SDL_CDStatus(drive_handles[drive]))<=0) {
		return TOS_ENOSYS;
	}

	cur_cd = drive_handles[drive];

	atari_tocentry = (atari_tocentry_t *) Atari2HostAddr(buffer);
	for (i=0; i<=cur_cd->numtracks; i++) {
		int minute, second, frame;

		FRAMES_TO_MSF(cur_cd->track[i].offset, &minute, &second, &frame);
		atari_tocentry[i].track = cur_cd->track[i].id;
		if (cur_cd->track[i].id == CDROM_LEADOUT) {
			atari_tocentry[i].track = CDROM_LEADOUT_CDAR;
		}
		atari_tocentry[i].minute = BinaryToBcd(minute);
		atari_tocentry[i].second = BinaryToBcd(second);
		atari_tocentry[i].frame = BinaryToBcd(frame);
	}

	atari_tocentry[cur_cd->numtracks+1].track =
		atari_tocentry[cur_cd->numtracks+1].minute =
		atari_tocentry[cur_cd->numtracks+1].second = 
		atari_tocentry[cur_cd->numtracks+1].frame = 0;

	CloseDrive(drive);
	return TOS_E_OK;
}

int32 CdromDriver::cd_discinfo(memptr device, memptr buffer)
{
	int drive, minute, second, frame;
	SDL_CD	*cur_cd;
	atari_discinfo_t	*discinfo;

	D(bug(NFCD_NAME "DiscInfo()"));
	drive = OpenDrive(device);
	if (drive<0) {
		return TOS_ENOSYS;
	}

	if (CD_INDRIVE(SDL_CDStatus(drive_handles[drive]))<=0) {
		return TOS_ENOSYS;
	}

	cur_cd = drive_handles[drive];

	discinfo = (atari_discinfo_t *) Atari2HostAddr(buffer);
	memset(discinfo, 0, sizeof(atari_discinfo_t));

	discinfo->first = cur_cd->track[0].id;
	discinfo->last = cur_cd->track[cur_cd->numtracks-1].id;
	discinfo->current = cur_cd->track[cur_cd->cur_track].id;
	discinfo->index = 1;
	discinfo->disctype = (cur_cd->track[0].type == SDL_AUDIO_TRACK) ? 0 : 1;

	FRAMES_TO_MSF(cur_cd->cur_frame, &minute, &second, &frame);
	discinfo->relative.track = 0;
	discinfo->relative.minute = BinaryToBcd(minute);
	discinfo->relative.second = BinaryToBcd(second);
	discinfo->relative.frame = BinaryToBcd(frame);

	FRAMES_TO_MSF(cur_cd->track[cur_cd->cur_track].offset + cur_cd->cur_frame, &minute, &second, &frame);
	discinfo->absolute.track = 0;
	discinfo->absolute.minute = BinaryToBcd(minute);
	discinfo->absolute.second = BinaryToBcd(second);
	discinfo->absolute.frame = BinaryToBcd(frame);

	FRAMES_TO_MSF(cur_cd->track[cur_cd->numtracks].offset, &minute, &second, &frame);
	discinfo->end.track = 0;
	discinfo->end.minute = BinaryToBcd(minute);
	discinfo->end.second = BinaryToBcd(second);
	discinfo->end.frame = BinaryToBcd(frame);

	CloseDrive(drive);
	return TOS_E_OK;
}
