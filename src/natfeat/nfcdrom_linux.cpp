/*
	NatFeat host CD-ROM access, Linux CD-ROM driver

	ARAnyM (C) 2003-2005 Patrice Mandin

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

#include <SDL_cdrom.h>
#include <SDL_endian.h>

#include <linux/cdrom.h>
#include <errno.h>
#include "toserror.h"

#include "sysdeps.h"
#include "cpu_emulation.h"
#include "parameters.h"
#include "nfcdrom.h"
#include "nfcdrom_atari.h"
#include "nfcdrom_linux.h"

#define DEBUG 0
#include "debug.h"

/*--- Defines ---*/

#define NFCD_NAME	"nf:cdrom:linux: "

/*--- Public functions ---*/

CdromDriverLinux::CdromDriverLinux()
{
	int i;

	D(bug(NFCD_NAME "CdromDriverLinux()"));
	drives_mask = 0xffffffffUL;
	for (i=0; i<CD_MAX_DRIVES; i++) {
		drive_handles[i]=-1;
	}
}

CdromDriverLinux::~CdromDriverLinux()
{
	int i;

	D(bug(NFCD_NAME "~CdromDriverLinux()"));
	for (i=0; i<CD_MAX_DRIVES; i++) {
		CloseDrive(i);
	}
}

/*--- Private functions ---*/

int CdromDriverLinux::OpenDrive(memptr device)
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
	if (drive_handles[drive]>=0) {
		return drive;
	}

	/* Open drive */
	drive_handles[drive]=open(SDL_CDName(bx_options.nfcdroms[drive].physdevtohostdev), O_RDONLY|O_EXCL|O_NONBLOCK, 0);
	if (drive_handles[drive]<0) {
		drive_handles[drive]=-1;
		drive = errnoHost2Mint(errno, TOS_ENOSYS);
		D(bug(NFCD_NAME " error opening drive %s: %s", SDL_CDName(bx_options.nfcdroms[drive].physdevtohostdev), strerror(errno)));
		return drive;
	}

	return drive;
}

void CdromDriverLinux::CloseDrive(int drive)
{
	/* Drive already closed ? */
	if (drive_handles[drive]<0) {
		return;
	}

	close(drive_handles[drive]);
	drive_handles[drive]=-1;
}

uint16 CdromDriverLinux::AtariToLinuxIoctl(uint16 opcode)
{
	uint16 new_opcode;

	switch(opcode) {
		case ATARI_CDROMREADOFFSET:
			new_opcode = CDROMMULTISESSION;
			break;
		case ATARI_CDROMPAUSE:
			new_opcode = CDROMPAUSE;
			break;
		case ATARI_CDROMRESUME:
			new_opcode = CDROMRESUME;
			break;
		case ATARI_CDROMPLAYMSF:
			new_opcode = CDROMPLAYMSF;
			break;
		case ATARI_CDROMPLAYTRKIND:
			new_opcode = CDROMPLAYTRKIND;
			break;
		case ATARI_CDROMREADTOCHDR:
			new_opcode = CDROMREADTOCHDR;
			break;
		case ATARI_CDROMREADTOCENTRY:
			new_opcode = CDROMREADTOCENTRY;
			break;
		case ATARI_CDROMSTOP:
			new_opcode = CDROMSTOP;
			break;
		case ATARI_CDROMSTART:
			new_opcode = CDROMSTART;
			break;
		case ATARI_CDROMEJECT:
			new_opcode = CDROMEJECT;
			break;
		case ATARI_CDROMVOLCTRL:
			new_opcode = CDROMVOLCTRL;
			break;
		case ATARI_CDROMSUBCHNL:
			new_opcode = CDROMSUBCHNL;
			break;
		case ATARI_CDROMREADMODE2:
			new_opcode = CDROMREADMODE2;
			break;
		case ATARI_CDROMREADMODE1:
			new_opcode = CDROMREADMODE1;
			break;
		case ATARI_CDROMPREVENTREMOVAL:
		case ATARI_CDROMALLOWREMOVAL:
			new_opcode = CDROM_LOCKDOOR;
			break;
		case ATARI_CDROMAUDIOCTRL:
			new_opcode = CDROMVOLCTRL;
			break;
		case ATARI_CDROMREADDA:
			new_opcode = CDROMREADRAW;
			break;
		case ATARI_CDROMRESET:
			new_opcode = CDROMRESET;
			break;
		case ATARI_CDROMGETMCN:
			new_opcode = CDROM_GET_MCN;
			break;
		case ATARI_CDROMGETTISRC:
			new_opcode = 0xffff;
			break;
		default:
			new_opcode = 0xffff;
			break;
	}

	D(bug(NFCD_NAME " opcode 0x%04x translated to 0x%04x", opcode, new_opcode));
	return new_opcode;
}

int32 CdromDriverLinux::cd_read(memptr device, memptr buffer, uint32 first, uint32 length)
{
	int drive;

	drive = OpenDrive(device);
	if (drive<0) {
		return drive;
	}

	D(bug(NFCD_NAME "Read(%d,%d)", first, length));

	if (lseek(drive_handles[drive], first * 2048, SEEK_SET)<0) {
		D(bug(NFCD_NAME "Read(): can not seek to block %d", first));
		CloseDrive(drive);
		return TOS_ENOSYS;
	}

	if (read(drive_handles[drive], Atari2HostAddr(buffer), length * 2048)<0) {
		D(bug(NFCD_NAME "Read(): can not read %d blocks", length));
		CloseDrive(drive);
		return TOS_ENOSYS;
	}

	CloseDrive(drive);
	return TOS_E_OK;
}

int32 CdromDriverLinux::cd_status(memptr device, memptr ext_status)
{
	UNUSED(ext_status);
	int drive, errorcode, mediachanged;
	unsigned long status;

	drive = OpenDrive(device);
	if (drive<0) {
		return drive;
	}

	status = 0;
	errorcode=ioctl(drive_handles[drive], CDROM_MEDIA_CHANGED, &status);
	D(bug(NFCD_NAME "Status(CDROM_MEDIA_CHANGED): errorcode=0x%08x", errorcode));
	if (errorcode<0) {
		errorcode = errnoHost2Mint(errno, TOS_EDRVNR);
		CloseDrive(drive);
		return errorcode;
	}
	mediachanged = (errorcode==1);

	status = 0;
	errorcode=ioctl(drive_handles[drive], CDROM_DRIVE_STATUS, &status);
	D(bug(NFCD_NAME "Status(CDROM_DRIVE_STATUS): errorcode=0x%08x", errorcode));
	if (errorcode<0) {
		errorcode = errnoHost2Mint(errno, TOS_EDRVNR);
		CloseDrive(drive);
		return errorcode;
	}
	if (errorcode == CDS_DRIVE_NOT_READY) {
		CloseDrive(drive);
		return TOS_EDRVNR;
	}

	CloseDrive(drive);
	return (mediachanged<<3);
}

int32 CdromDriverLinux::cd_ioctl(memptr device, uint16 opcode, memptr buffer)
{
	int drive, errorcode;
	uint16 new_opcode;

	drive = OpenDrive(device);
	if (drive<0) {
		return drive;
	}

	errorcode = TOS_ENOSYS;
	new_opcode=AtariToLinuxIoctl(opcode);
	if (new_opcode == 0xffff) {
		D(bug(NFCD_NAME "Ioctl(): ioctl 0x%04x unsupported", opcode));
		CloseDrive(drive);
		return errorcode;
	}

	switch(new_opcode) {
		case CDROMPLAYMSF:
			{
				struct cdrom_msf cd_msf;
				unsigned char *atari_msf;
				
				atari_msf = (unsigned char *) Atari2HostAddr(buffer);
				cd_msf.cdmsf_min0 = *atari_msf++;
				cd_msf.cdmsf_sec0 = *atari_msf++;
				cd_msf.cdmsf_frame0 = *atari_msf++;
				cd_msf.cdmsf_min1 = *atari_msf++;
				cd_msf.cdmsf_sec1 = *atari_msf++;
				cd_msf.cdmsf_frame1 = *atari_msf;

				D(bug(NFCD_NAME " Ioctl(CDROMPLAYMSF,%02d:%02d:%02d,%02d:%02d:%02d)",
					cd_msf.cdmsf_min0, cd_msf.cdmsf_sec0, cd_msf.cdmsf_frame0,
					cd_msf.cdmsf_min1, cd_msf.cdmsf_sec1, cd_msf.cdmsf_frame1
				));

				errorcode=ioctl(drive_handles[drive], new_opcode, &cd_msf);
			}
			break;
		case CDROMREADTOCENTRY:
			{
				struct cdrom_tocentry tocentry;
				unsigned char *atari_tocentry;
				
				atari_tocentry = (unsigned char *)  Atari2HostAddr(buffer);
				tocentry.cdte_track = atari_tocentry[0];
				tocentry.cdte_format = atari_tocentry[1];

				D(bug(NFCD_NAME " Ioctl(READTOCENTRY,0x%02x,0x%02x)", tocentry.cdte_track, tocentry.cdte_format));
				errorcode=ioctl(drive_handles[drive], new_opcode, &tocentry);

				if (errorcode>=0) {
					atari_tocentry[0] = tocentry.cdte_track;
					atari_tocentry[1] = tocentry.cdte_format;
					atari_tocentry[2] = (tocentry.cdte_adr & 0x0f)<<4;
					atari_tocentry[2] |= tocentry.cdte_ctrl & 0x0f;
					atari_tocentry[3] = tocentry.cdte_datamode;
					atari_tocentry[4] = atari_tocentry[5] = 0;

					if (tocentry.cdte_format == CDROM_LBA) {
						*((unsigned long *) &atari_tocentry[6]) = SDL_SwapBE32(tocentry.cdte_addr.lba);
					} else {
						atari_tocentry[6] = 0;
						atari_tocentry[7] = tocentry.cdte_addr.msf.minute;
						atari_tocentry[8] = tocentry.cdte_addr.msf.second;
						atari_tocentry[9] = tocentry.cdte_addr.msf.frame;
					}
				}
			}
			break;
		case CDROMVOLCTRL:
			{
				struct cdrom_volctrl volctrl;
				atari_cdrom_audioctrl_t *atari_audioctrl;

				if (opcode == ATARI_CDROMVOLCTRL) {
					/* CDROMVOLCTRL function emulation */
					/* CDROMAUDIOCTRL function emulation */
					struct atari_cdrom_volctrl *v = (struct atari_cdrom_volctrl *) Atari2HostAddr(buffer);
	
					D(bug(NFCD_NAME " Ioctl(CDROMVOLCTRL)"));
	
					/* Write volume settings */
					volctrl.channel0 = v->channel0;
					volctrl.channel1 = v->channel1;
					volctrl.channel2 = v->channel2;
					volctrl.channel3 = v->channel3;

					errorcode=ioctl(drive_handles[drive], CDROMVOLCTRL, &volctrl);
				} else
				{
					/* CDROMAUDIOCTRL function emulation */
					atari_audioctrl = (atari_cdrom_audioctrl_t *) Atari2HostAddr(buffer);
	
					D(bug(NFCD_NAME " Ioctl(CDROMAUDIOCTRL,0x%04x)", atari_audioctrl->set));
	
					if (atari_audioctrl->set == 0) {
						/* Read volume settings */
						errorcode=ioctl(drive_handles[drive], CDROMVOLREAD, &volctrl);
						if (errorcode>=0) {
							atari_audioctrl->channel[0].selection =
								atari_audioctrl->channel[1].selection =
								atari_audioctrl->channel[2].selection =
								atari_audioctrl->channel[3].selection = 0;
							atari_audioctrl->channel[0].volume = volctrl.channel0;
							atari_audioctrl->channel[1].volume = volctrl.channel1;
							atari_audioctrl->channel[2].volume = volctrl.channel2;
							atari_audioctrl->channel[3].volume = volctrl.channel3;
						}
					} else {
						/* Write volume settings */
						volctrl.channel0 = atari_audioctrl->channel[0].volume;
						volctrl.channel1 = atari_audioctrl->channel[1].volume;
						volctrl.channel2 = atari_audioctrl->channel[2].volume;
						volctrl.channel3 = atari_audioctrl->channel[3].volume;
	
						errorcode=ioctl(drive_handles[drive], CDROMVOLCTRL, &volctrl);
					}
				}
			}
			break;
		case CDROMSUBCHNL:
			{
				/* Structure is different between Atari and Linux */
				struct cdrom_subchnl subchnl;
				atari_cdromsubchnl_t *atari_subchnl;
				
				atari_subchnl = (atari_cdromsubchnl_t *) Atari2HostAddr(buffer);

				subchnl.cdsc_format = atari_subchnl->cdsc_format;

				D(bug(NFCD_NAME " Ioctl(READSUBCHNL,0x%02x)", atari_subchnl->cdsc_format));

				errorcode=ioctl(drive_handles[drive], new_opcode, &subchnl);

				if (errorcode>=0) {
					atari_subchnl->cdsc_format = subchnl.cdsc_format;
					atari_subchnl->cdsc_audiostatus = subchnl.cdsc_audiostatus;
					atari_subchnl->cdsc_resvd = 0;
					atari_subchnl->cdsc_info = (subchnl.cdsc_adr & 0x0f)<<4;
					atari_subchnl->cdsc_info |= subchnl.cdsc_ctrl & 0x0f;
					atari_subchnl->cdsc_trk = subchnl.cdsc_trk;
					atari_subchnl->cdsc_ind = subchnl.cdsc_ind;

					if (subchnl.cdsc_format == CDROM_LBA) {
						atari_subchnl->cdsc_absaddr = SDL_SwapBE32(subchnl.cdsc_absaddr.lba);
						atari_subchnl->cdsc_reladdr = SDL_SwapBE32(subchnl.cdsc_reladdr.lba);
					} else {
						atari_subchnl->cdsc_absaddr = SDL_SwapBE32(subchnl.cdsc_absaddr.msf.minute<<16);
						atari_subchnl->cdsc_absaddr |= SDL_SwapBE32(subchnl.cdsc_absaddr.msf.second<<8);
						atari_subchnl->cdsc_absaddr |= SDL_SwapBE32(subchnl.cdsc_absaddr.msf.frame);

						atari_subchnl->cdsc_reladdr = SDL_SwapBE32(subchnl.cdsc_reladdr.msf.minute<<16);
						atari_subchnl->cdsc_reladdr |= SDL_SwapBE32(subchnl.cdsc_reladdr.msf.second<<8);
						atari_subchnl->cdsc_reladdr |= SDL_SwapBE32(subchnl.cdsc_reladdr.msf.frame);
					}
				}
			}
			break;
		case CDROMREADMODE2:
		case CDROMREADMODE1:
			{
				struct cdrom_read cd_read, *atari_cdread;
				
				atari_cdread = (struct cdrom_read *) Atari2HostAddr(buffer);
				cd_read.cdread_lba = SDL_SwapBE32(atari_cdread->cdread_lba);
				cd_read.cdread_bufaddr = (char *)Atari2HostAddr(SDL_SwapBE32((unsigned long) (atari_cdread->cdread_bufaddr)));
				cd_read.cdread_buflen = SDL_SwapBE32(atari_cdread->cdread_buflen);

				errorcode=ioctl(drive_handles[drive], new_opcode, &cd_read);
			}
			break;
		case CDROMMULTISESSION:
			{
				struct cdrom_multisession cd_ms;
				Uint32 *atari_addr;

				atari_addr = (Uint32 *) Atari2HostAddr(buffer);
				cd_ms.xa_flag = 1;
				cd_ms.addr_format = CDROM_LBA;
				errorcode = ioctl(drive_handles[drive], new_opcode, &cd_ms);
				if (errorcode>=0) {
					*atari_addr=SDL_SwapBE32(cd_ms.addr.lba);
				}
			}
			break;
		case CDROM_GET_MCN:
			{
				atari_mcn_t	*atari_mcn;

				atari_mcn = (atari_mcn_t *) Atari2HostAddr(buffer);
				memset(atari_mcn, 0, sizeof(atari_mcn_t));

				errorcode=ioctl(drive_handles[drive], new_opcode, atari_mcn->mcn);
			}
			break;
		case CDROM_LOCKDOOR:
			errorcode=ioctl(drive_handles[drive], new_opcode, opcode == ATARI_CDROMPREVENTREMOVAL ? (void *)1 : (void *)0);
			break;
		case CDROMEJECT:
			if (buffer != 0)
				new_opcode = CDROMCLOSETRAY;
			errorcode=ioctl(drive_handles[drive], new_opcode, (void *)0);
			break;
		default:
			{
				D(bug(NFCD_NAME " Ioctl(0x%04x)", new_opcode));

				errorcode=ioctl(drive_handles[drive], new_opcode, Atari2HostAddr(buffer));
			}
			break;
	}
	if (errorcode < 0)
		errorcode = errnoHost2Mint(errno, TOS_EDRVNR);
	CloseDrive(drive);
	return errorcode;
}

int32 CdromDriverLinux::cd_startaudio(memptr device, uint32 dummy, memptr buffer)
{
	UNUSED(dummy);
	int drive, errorcode;
	struct cdrom_ti	track_index;
	metados_bos_tracks_t	*atari_track_index;

	drive = OpenDrive(device);
	if (drive<0) {
		return drive;
	}

	atari_track_index = (metados_bos_tracks_t *) Atari2HostAddr(buffer);
	track_index.cdti_trk0 = atari_track_index->first;
	track_index.cdti_ind0 = 1;
	track_index.cdti_trk1 = atari_track_index->first + atari_track_index->count -1;
	track_index.cdti_ind0 = 99;

	errorcode=ioctl(drive_handles[drive], CDROMPLAYTRKIND, &track_index);
	if (errorcode < 0)
		errorcode = errnoHost2Mint(errno, TOS_EDRVNR);
	CloseDrive(drive);
	return errorcode;
}

int32 CdromDriverLinux::cd_stopaudio(memptr device)
{
	int drive, errorcode;

	drive = OpenDrive(device);
	if (drive<0) {
		return drive;
	}

	errorcode=ioctl(drive_handles[drive], CDROMSTOP, NULL);
	if (errorcode < 0)
		errorcode = errnoHost2Mint(errno, TOS_EDRVNR);
	CloseDrive(drive);
	return errorcode;
}

int32 CdromDriverLinux::cd_setsongtime(memptr device, uint32 dummy, uint32 start_msf, uint32 end_msf)
{
	UNUSED(dummy);
	int drive, errorcode;
	struct cdrom_msf audio_bloc;

	drive = OpenDrive(device);
	if (drive<0) {
		return drive;
	}

	audio_bloc.cdmsf_min0 = (start_msf>>16) & 0xff;
	audio_bloc.cdmsf_sec0 = (start_msf>>8) & 0xff;
	audio_bloc.cdmsf_frame0 = (start_msf>>0) & 0xff;
	D(bug(NFCD_NAME " start: %02d:%02d:%02d", (start_msf>>16) & 0xff, (start_msf>>8) & 0xff, start_msf & 0xff));

	audio_bloc.cdmsf_min1 = (end_msf>>16) & 0xff;
	audio_bloc.cdmsf_sec1 = (end_msf>>8) & 0xff;
	audio_bloc.cdmsf_frame1 = (end_msf>>0) & 0xff;
	D(bug(NFCD_NAME " end:   %02d:%02d:%02d", (end_msf>>16) & 0xff, (end_msf>>8) & 0xff, end_msf & 0xff));

	errorcode=ioctl(drive_handles[drive], CDROMPLAYMSF, &audio_bloc);
	if (errorcode < 0)
		errorcode = errnoHost2Mint(errno, TOS_EDRVNR);
	CloseDrive(drive);
	return errorcode;
}

int32 CdromDriverLinux::cd_gettoc(memptr device, uint32 dummy, memptr buffer)
{
	UNUSED(dummy);
	int drive, i, numtracks, errorcode;
	struct cdrom_tochdr tochdr;
	struct cdrom_tocentry tocentry;
	atari_tocentry_t *atari_tocentry;

	drive = OpenDrive(device);
	if (drive<0) {
		return drive;
	}

	/* Read TOC header */
	errorcode=ioctl(drive_handles[drive], CDROMREADTOCHDR, &tochdr);
	if (errorcode<0) {
		errorcode = errnoHost2Mint(errno, TOS_EDRVNR);
		CloseDrive(drive);
		return errorcode;
	}

	D(bug(NFCD_NAME "GetToc():  TOC header read"));

	numtracks = tochdr.cdth_trk1-tochdr.cdth_trk0+1;

	/* Read TOC entries */
	atari_tocentry = (atari_tocentry_t *) Atari2HostAddr(buffer);
	for (i=0; i<=numtracks; i++) {
		D(bug(NFCD_NAME "GetToc():  reading TOC entry for track %d", i));

		tocentry.cdte_track = tochdr.cdth_trk0+i;
		if (i == numtracks) {
			tocentry.cdte_track = CDROM_LEADOUT;
		}
		tocentry.cdte_format = CDROM_MSF;

		errorcode = ioctl(drive_handles[drive], CDROMREADTOCENTRY, &tocentry);
		if (errorcode<0) {
			errorcode = errnoHost2Mint(errno, TOS_EDRVNR);
			CloseDrive(drive);
			return errorcode;
		}

		atari_tocentry[i].track = tocentry.cdte_track;
		atari_tocentry[i].minute = BinaryToBcd(tocentry.cdte_addr.msf.minute);
		atari_tocentry[i].second = BinaryToBcd(tocentry.cdte_addr.msf.second);
		atari_tocentry[i].frame = BinaryToBcd(tocentry.cdte_addr.msf.frame);
		if (tocentry.cdte_track == CDROM_LEADOUT) {
			atari_tocentry[i].track = CDROM_LEADOUT_CDAR;
		}

		D(bug(NFCD_NAME "GetToc():   track %d, id 0x%02x, %s",
			i, tocentry.cdte_track,
			tocentry.cdte_ctrl & CDROM_DATA_TRACK ? "data" : "audio"
		));

		if (tocentry.cdte_format == CDROM_MSF) {
			D(bug(NFCD_NAME "GetToc():    msf %02d:%02d:%02d, bcd: %02x:%02x:%02x",
				tocentry.cdte_addr.msf.minute,
				tocentry.cdte_addr.msf.second,
				tocentry.cdte_addr.msf.frame,
				atari_tocentry[i].minute,
				atari_tocentry[i].second,
				atari_tocentry[i].frame
			));
		} else {
			D(bug(NFCD_NAME "GetToc():    lba 0x%08x", tocentry.cdte_addr.lba));
		}
	}

	atari_tocentry[numtracks+1].track =
		atari_tocentry[numtracks+1].minute =
		atari_tocentry[numtracks+1].second = 
		atari_tocentry[numtracks+1].frame = 0;

	CloseDrive(drive);
	return TOS_E_OK;
}

int32 CdromDriverLinux::cd_discinfo(memptr device, memptr buffer)
{
	int drive, errorcode;
	struct cdrom_tochdr tochdr;
	struct cdrom_subchnl subchnl;
	struct cdrom_tocentry tocentry;
	atari_discinfo_t	*discinfo;

	drive = OpenDrive(device);
	if (drive<0) {
		return drive;
	}

	/* Read TOC header */
	errorcode=ioctl(drive_handles[drive], CDROMREADTOCHDR, &tochdr);
	if (errorcode<0) {
		errorcode = errnoHost2Mint(errno, TOS_EDRVNR);
		CloseDrive(drive);
		return errorcode;
	}
	D(bug(NFCD_NAME "DiscInfo():  TOC header read"));

	discinfo = (atari_discinfo_t *) Atari2HostAddr(buffer);
	memset(discinfo, 0, sizeof(atari_discinfo_t));

	discinfo->first = discinfo->current = tochdr.cdth_trk0;
	discinfo->last = tochdr.cdth_trk1;
	discinfo->index = 1;

	/* Read subchannel */
	subchnl.cdsc_format = CDROM_MSF;
	errorcode=ioctl(drive_handles[drive], CDROMSUBCHNL, &subchnl);
	if (errorcode<0) {
		errorcode = errnoHost2Mint(errno, TOS_EDRVNR);
		CloseDrive(drive);
		return errorcode;
	}
	D(bug(NFCD_NAME "DiscInfo():  Subchannel read"));

	discinfo->current = subchnl.cdsc_trk;	/* current track */
	discinfo->index = subchnl.cdsc_ind;	/* current index */

	discinfo->relative.track = 0;
	discinfo->relative.minute = BinaryToBcd(subchnl.cdsc_reladdr.msf.minute);
	discinfo->relative.second = BinaryToBcd(subchnl.cdsc_reladdr.msf.second);
	discinfo->relative.frame = BinaryToBcd(subchnl.cdsc_reladdr.msf.frame);

	discinfo->absolute.track = 0;
	discinfo->absolute.minute = BinaryToBcd(subchnl.cdsc_absaddr.msf.minute);
	discinfo->absolute.second = BinaryToBcd(subchnl.cdsc_absaddr.msf.second);
	discinfo->absolute.frame = BinaryToBcd(subchnl.cdsc_absaddr.msf.frame);

	/* Read toc entry for start of disc, to select disc type */
	tocentry.cdte_track = tochdr.cdth_trk0;
	tocentry.cdte_format = CDROM_MSF;

	errorcode = ioctl(drive_handles[drive], CDROMREADTOCENTRY, &tocentry);
	if (errorcode<0) {
		errorcode = errnoHost2Mint(errno, TOS_EDRVNR);
		CloseDrive(drive);
		return errorcode;
	}
	discinfo->disctype = ((tocentry.cdte_ctrl & CDROM_DATA_TRACK) == CDROM_DATA_TRACK);

	/* Read toc entry for end of disc */
	tocentry.cdte_track = CDROM_LEADOUT;
	tocentry.cdte_format = CDROM_MSF;

	errorcode = ioctl(drive_handles[drive], CDROMREADTOCENTRY, &tocentry);
	if (errorcode<0) {
		errorcode = errnoHost2Mint(errno, TOS_EDRVNR);
		CloseDrive(drive);
		return errorcode;
	}

	D(bug(NFCD_NAME "DiscInfo():  Toc entry read"));

	discinfo->end.track = 0;
	discinfo->end.minute = BinaryToBcd(tocentry.cdte_addr.msf.minute);
	discinfo->end.second = BinaryToBcd(tocentry.cdte_addr.msf.second);
	discinfo->end.frame = BinaryToBcd(tocentry.cdte_addr.msf.frame);

	CloseDrive(drive);
	return TOS_E_OK;
}

/*
vim:ts=4:sw=4:
*/
