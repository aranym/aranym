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

#include <linux/cdrom.h>

#include <SDL_endian.h>

#include "sysdeps.h"
#include "cpu_emulation.h"
#include "parameters.h"
#include "nfcdrom.h"
#include "../../atari/nfcdrom/nfcdrom_nfapi.h"

#define DEBUG 1
#include "debug.h"

/*--- Defines ---*/

#ifndef EINVFN
#define EINVFN -32
#endif

#define CDROM_LEADOUT_CDAR	0xa2

/*--- Types ---*/

typedef struct {
	unsigned char count;
	unsigned char first;
} metados_bos_tracks_t __attribute__((packed));

typedef struct {
	void *next;
	unsigned long attrib;
	unsigned short phys_letter;
	unsigned short dma_channel;
	unsigned short sub_device;
	void *functions;
	unsigned short status;
	unsigned long reserved[2];
	char name[32];
} metados_bos_header_t __attribute__((packed));

struct atari_cdrom_tocentry 
{
	/* input parameters */
	unsigned char	cdte_track;		/* track number or CDROM_LEADOUT */
	unsigned char	cdte_format;	/* CDROM_LBA or CDROM_MSF */

	/* output parameters */
	unsigned char	cdte_info;
#if 0
	unsigned	cdte_adr:4;	/*	the SUBQ channel encodes 0: nothing,
								1: position data, 2: MCN, 3: ISRC,
								else: reserved */
	unsigned	cdte_ctrl:4;	/*	bit 0: audio with pre-emphasis,
									bit 1: digital copy permitted,
									bit 2: data track,
									bit 3: four channel */
#endif
	unsigned char	cdte_datamode;	/* currently not set */
	unsigned long	cdte_addr;	/* track start */
} __attribute__((packed));

struct atari_cdrom_subchnl 
{
	/* input parameters */
	unsigned char	cdsc_format;	/* CDROM_MSF or CDROM_LBA */
    
	/* output parameters */
	unsigned char	cdsc_audiostatus;	/* see below */
    unsigned char	cdsc_resvd;	/* reserved */
	unsigned char	cdsc_info;
#if 0
    unsigned	cdsc_adr:	4;	/* see above */
    unsigned	cdsc_ctrl:	4;	/* see above */
#endif
    unsigned char	cdsc_trk;	/* current track */
    unsigned char	cdsc_ind;	/* current index */
	unsigned long	cdsc_absaddr;	/* absolute address */
	unsigned long	cdsc_reladdr;	/* track relative address */
} __attribute__((packed));

struct atari_cdrom_audioctrl
{
	/* input parameters */
	short	set;    /* 0 == inquire only */

	/* input/output parameters */
	struct {
		unsigned char selection;
        unsigned char volume;
    } channel[4];
} __attribute__((packed));

/*
typedef struct {
	unsigned short length;
	unsigned char first;
	unsigned char last;
} atari_tocheader_t __attribute__((packed));
*/
typedef struct {
	unsigned char reserved1;
	unsigned char info;
/*
	unsigned adr:4;
	unsigned ctrl:4;
*/
	unsigned char track;
	unsigned char reserved2;
	unsigned long absaddr;
} atari_tocentry_t __attribute__((packed));

typedef struct {	/* TOC entry for MetaGetToc() function */
	unsigned char track;
	unsigned char minute;
	unsigned char second;
	unsigned char frame;
} gettoc_tocentry_t __attribute__((packed));

typedef struct {	/* Discinfo for MetaDiscInfo() function */
	unsigned char disctype, first, last, current;
	gettoc_tocentry_t	relative;
	gettoc_tocentry_t	absolute;
	gettoc_tocentry_t	end;
	unsigned char index, reserved1[3];
	unsigned long reserved2[123];
} atari_discinfo_t __attribute__((packed));

typedef struct {
    unsigned char	audiostatus;
    unsigned char	mcn[23];
} atari_mcn_t __attribute__((packed));

/*--- Public functions ---*/

char *CdromDriver::name()
{
	return "CDROM";
}

bool CdromDriver::isSuperOnly()
{
	return true;
}

int32 CdromDriver::dispatch(uint32 fncode)
{
	int32 ret;
/*
	int i;
*/
	D(bug("nf: cdrom: dispatch(%u)", fncode));
/*
	for (i=0; i<5; i++) {
		D(bug("nf: cdrom: parameter(%d)=0x%08x", i, getParameter(i)));
	}
*/
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
			D(bug("nf: cdrom:  unimplemented function #%d", fncode));
			ret = EINVFN;
			break;
	}
	D(bug("nf: cdrom:  function returning with 0x%08x", ret));
	return ret;
}

/*--- Private functions ---*/

CdromDriver::CdromDriver(void)
{
	int i;

	drives_mask = 0xffffffffUL;
	for (i=0; i<32; i++) {
		drive_handles[i]=-1;
	}

	D(bug("nf: cdrom: CdromDriver()"));
}

CdromDriver::~CdromDriver(void)
{
	int i;

	/* Close any open host device */
	for (i=0; i<32; i++) {
		if (drive_handles[i]>=0) {
			close(drive_handles[i]);
		}
	}

	D(bug("nf: cdrom: ~CdromDriver()"));
}

void CdromDriver::ScanDrives(void)
{
	int i;

	D(bug("nf: cdrom: ScanDrives()"));

	drives_mask = 0;
	for (i='A'; i<='Z'; i++) {
		/* Check if there is a valid filename */
		if (bx_options.nfcdroms[i-'A'].physdevtohostdev[0] == 0) {
			continue;
		}

		/* Check it is a device file, cdrom type */

		/* Add to MetaDOS CD-ROM drives */
		drives_mask |= 1<<(i-'A');
		D(bug("nf: cdrom: ScanDrives(): physical device %c added", i));
	}
}

int CdromDriver::GetDrive(memptr device)
{
	metados_bos_header_t *bos_device;

	bos_device=(metados_bos_header_t *) device;

	return ReadInt16((uint32) &(bos_device->phys_letter));
}

int CdromDriver::CheckDrive(memptr device)
{
	int drive;

	drive = GetDrive(device)-'A';
	D(bug("nf: cdrom:  physical device %c selected", drive+'A'));

	/* Drive exist ? */
	if ((drives_mask & (1<<drive))==0) {
		D(bug("nf: cdrom:  physical device %c do not exist", drive+'A'));
		return EINVFN;
	}

	/* Device not opened ? */
	if (drive_handles[drive]<0) {
		D(bug("nf: cdrom:  physical device %c not opened", drive+'A'));
		return EINVFN;
	}

	/* Return drive number, or negative error code */
	return drive;
}

uint16 CdromDriver::AtariToLinuxIoctl(uint16 opcode)
{
	uint16 new_opcode;

	new_opcode = opcode;
	switch(opcode) {
		case (('C'<<8)|0x00):
			new_opcode = CDROMMULTISESSION;
			break;
		case (('C'<<8)|0x0e):
		case (('C'<<8)|0x0f):
			new_opcode = CDROM_LOCKDOOR;
			break;
		case (('C'<<8)|0x10):
			new_opcode = CDROMVOLCTRL;
			break;
		case (('C'<<8)|0x14):
			new_opcode = 0xffff;
			break;
		case (('C'<<8)|0x11):
			new_opcode = CDROMREADRAW;
			break;
		case (('C'<<8)|0x13):
			new_opcode = CDROM_GET_MCN;
			break;
		default:
			new_opcode -= 'C'<<8;
			new_opcode += 'S'<<8;
			break;
	}

	D(bug("nf: cdrom:  opcode 0x%04x translated to 0x%04x", opcode, new_opcode));
	return new_opcode;
}

unsigned char CdromDriver::BinaryToBcd(unsigned char value)
{
	int decimal;
	
	decimal = (value/10)*16;

	return decimal | (value % 10);
}

int32 CdromDriver::cd_open(memptr device, memptr buffer)
{
	int drive;

	drive = GetDrive(device)-'A';
	D(bug("nf: cdrom: Open(): physical device %c selected", drive+'A'));

	/* Drive exist ? */
	if ((drives_mask & (1<<drive))==0) {
		D(bug("nf: cdrom: Open(): physical device %c do not exist", drive+'A'));
		return EINVFN;
	}

	/* Drive already opened ? */
	if (drive_handles[drive]>=0) {
		D(bug("nf: cdrom: Open(): physical device %c already opened", drive+'A'));
		return 0;
	}

	drive_handles[drive]=open(bx_options.nfcdroms[drive].physdevtohostdev, O_RDONLY|O_EXCL|O_NONBLOCK, 0);
	if (drive_handles[drive]<0) {
		D(bug("nf: cdrom: Open(): error opening %s", bx_options.nfcdroms[drive].physdevtohostdev));
		drive_handles[drive]=-1;
		return EINVFN;
	}

	D(bug("nf: cdrom: Open(): physical device %c opened", drive+'A'));
	return 0;
}

int32 CdromDriver::cd_close(memptr device)
{
	int drive;

	drive = GetDrive(device)-'A';
	D(bug("nf: cdrom: Close(): physical device %c selected", drive+'A'));

	/* Drive exist ? */
	if ((drives_mask & (1<<drive))==0) {
		D(bug("nf: cdrom: Close(): physical device %c do not exist", drive+'A'));
		return EINVFN;
	}

	/* Device not opened ? */
	if (drive_handles[drive]<0) {
		D(bug("nf: cdrom: Close(): physical device %c already closed", drive+'A'));
		return 0;
	}
/*
	close(drive_handles[drive]);
	drive_handles[drive]=-1;
*/
	D(bug("nf: cdrom: Close(): physical device %c closed", drive+'A'));
	return 0;
}

int32 CdromDriver::cd_read(memptr device, memptr buffer, uint32 first, uint32 length)
{
	int drive;

	drive = CheckDrive(device);
	if (drive<0) {
		return drive;
	}

	if (lseek(drive_handles[drive], first * 512, SEEK_SET)<0) {
		D(bug("nf: cdrom: Read(): can not seek to block %d", first));
		return EINVFN;
	}

	if (read(drive_handles[drive], Atari2HostAddr(buffer), length * 512)<0) {
		D(bug("nf: cdrom: Read(): can not read %d blocks", length));
		return EINVFN;
	}

	return 0;
}

int32 CdromDriver::cd_write(memptr device, memptr buffer, uint32 first, uint32 length)
{
	return EINVFN;
}

int32 CdromDriver::cd_seek(memptr device, uint32 offset)
{
	int drive;

	drive = CheckDrive(device);
	if (drive<0) {
		return drive;
	}

	return lseek(drive_handles[drive], offset * 512, 0);
}

int32 CdromDriver::cd_status(memptr device, memptr ext_status)
{
	int drive;

	drive = CheckDrive(device);
	if (drive<0) {
		return drive;
	}

	return 0;
}

int32 CdromDriver::cd_ioctl(memptr device, uint16 opcode, memptr buffer)
{
	int drive;
	uint16 new_opcode;

	drive = CheckDrive(device);
	if (drive<0) {
		return drive;
	}
/*
	return EINVFN;
*/
	new_opcode=AtariToLinuxIoctl(opcode);
	if (new_opcode == 0xffff) {
		D(bug("nf: cdrom: Ioctl(): ioctl 0x%04x unsupported", opcode));
		return EINVFN;
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

				D(bug("nf: cdrom:  Ioctl(CDROMPLAYMSF,%02d:%02d:%02d,%02d:%02d:%02d)",
					cd_msf.cdmsf_min0, cd_msf.cdmsf_sec0, cd_msf.cdmsf_frame0,
					cd_msf.cdmsf_min1, cd_msf.cdmsf_sec1, cd_msf.cdmsf_frame1
				));

				return ioctl(drive_handles[drive], new_opcode, &cd_msf);
			}
			break;
		case CDROMREADTOCENTRY:
			{
				int errorcode;
				struct cdrom_tocentry tocentry;
				unsigned char *atari_tocentry;
				
				atari_tocentry = (unsigned char *)  Atari2HostAddr(buffer);
				tocentry.cdte_track = atari_tocentry[0];
				tocentry.cdte_format = atari_tocentry[1];

				D(bug("nf: cdrom:  Ioctl(READTOCENTRY,0x%02x,0x%02x)", tocentry.cdte_track, tocentry.cdte_format));

				errorcode=ioctl(drive_handles[drive], new_opcode, &tocentry);

				if (errorcode>=0) {
					atari_tocentry[0] = tocentry.cdte_track;
					atari_tocentry[1] = tocentry.cdte_format;
					atari_tocentry[2] = 0;
					atari_tocentry[3] = (tocentry.cdte_adr & 0x0f)<<4;
					atari_tocentry[3] |= tocentry.cdte_ctrl & 0x0f;
					atari_tocentry[4] = tocentry.cdte_datamode;

					if (tocentry.cdte_format == CDROM_LBA) {
						*((unsigned long *) &atari_tocentry[5]) = SDL_SwapBE32(tocentry.cdte_addr.lba);
					} else {
						atari_tocentry[5] = 0;
						atari_tocentry[6] = tocentry.cdte_addr.msf.minute;
						atari_tocentry[7] = tocentry.cdte_addr.msf.second;
						atari_tocentry[8] = tocentry.cdte_addr.msf.frame;
					}
				}
				return errorcode;
			}
			break;
		case CDROMVOLCTRL:
			{
				struct cdrom_volctrl volctrl;
				struct atari_cdrom_audioctrl *atari_audioctrl;
				int errorcode;

				if (opcode == (('C'<<8)|0x0a)) {
					break;
				}

				/* CDROMAUDIOCTRL function emulation */
				atari_audioctrl = (atari_cdrom_audioctrl *) Atari2HostAddr(buffer);
				errorcode=EINVFN;

				D(bug("nf: cdrom:  Ioctl(CDROMAUDIOCTRL,0x%04x)", atari_audioctrl->set));

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
				return errorcode;
			}
			break;
		case CDROMSUBCHNL:
			{
				int errorcode;

				/* Structure is different between Atari and Linux */
				struct cdrom_subchnl subchnl;
				struct atari_cdrom_subchnl *atari_subchnl;
				
				atari_subchnl = (atari_cdrom_subchnl *) Atari2HostAddr(buffer);

				subchnl.cdsc_format = atari_subchnl->cdsc_format;

				D(bug("nf: cdrom:  Ioctl(READSUBCHNL,0x%02x)", atari_subchnl->cdsc_format));

				errorcode=ioctl(drive_handles[drive], new_opcode, &subchnl);

				if (errorcode>=0) {
					atari_subchnl->cdsc_format = subchnl.cdsc_format;
					atari_subchnl->cdsc_audiostatus = subchnl.cdsc_audiostatus;
					atari_subchnl->cdsc_resvd = 0;
					atari_subchnl->cdsc_info = (subchnl.cdsc_adr & 0x0f)<<4;
					atari_subchnl->cdsc_info |= subchnl.cdsc_ctrl & 0x0f;
					atari_subchnl->cdsc_trk = subchnl.cdsc_trk;
					atari_subchnl->cdsc_ind = subchnl.cdsc_ind;

					atari_subchnl->cdsc_absaddr = ((subchnl.cdsc_absaddr.lba)>>24) & 0x000000ff;
					atari_subchnl->cdsc_absaddr |= ((subchnl.cdsc_absaddr.lba)<<8) & 0xffffff00;

					atari_subchnl->cdsc_reladdr = ((subchnl.cdsc_reladdr.lba)>>24) & 0x000000ff;
					atari_subchnl->cdsc_reladdr |= ((subchnl.cdsc_reladdr.lba)<<8) & 0xffffff00;
				}
				return errorcode;
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

				return ioctl(drive_handles[drive], new_opcode, &cd_read);
			}
			break;
		case CDROMMULTISESSION:
			{
				int errorcode;
				struct cdrom_multisession cd_ms;
				unsigned long *atari_addr;

				atari_addr = (unsigned long *) Atari2HostAddr(buffer);
				cd_ms.xa_flag = 1;
				cd_ms.addr_format = CDROM_LBA;
				errorcode = ioctl(drive_handles[drive], new_opcode, &cd_ms);
				if (errorcode>=0) {
					*atari_addr=SDL_SwapBE32(cd_ms.addr.lba);
				}
				return errorcode;
			}
			break;
		case CDROM_GET_MCN:
			{
				atari_mcn_t	*atari_mcn;

				atari_mcn = (atari_mcn_t *) Atari2HostAddr(buffer);
				memset(atari_mcn, 0, sizeof(atari_mcn_t));
				return ioctl(drive_handles[drive], new_opcode, atari_mcn->mcn);
			}
			break;
	}

	D(bug("nf: cdrom:  Ioctl(0x%04x)", new_opcode));

	return ioctl(drive_handles[drive], new_opcode, Atari2HostAddr(buffer));
}

int32 CdromDriver::cd_startaudio(memptr device, uint32 dummy, memptr buffer)
{
	int drive;
	struct cdrom_ti	track_index;
	metados_bos_tracks_t	*atari_track_index;

	drive = CheckDrive(device);
	if (drive<0) {
		return drive;
	}

	atari_track_index = (metados_bos_tracks_t *) Atari2HostAddr(buffer);
	track_index.cdti_trk0 = atari_track_index->first;
	track_index.cdti_ind0 = 1;
	track_index.cdti_trk1 = atari_track_index->first + atari_track_index->count -1;
	track_index.cdti_ind0 = 99;

	return ioctl(drive_handles[drive], CDROMPLAYTRKIND, &track_index);
}

int32 CdromDriver::cd_stopaudio(memptr device)
{
	int drive;

	drive = CheckDrive(device);
	if (drive<0) {
		return drive;
	}

	return ioctl(drive_handles[drive], CDROMSTOP, NULL);
}

int32 CdromDriver::cd_setsongtime(memptr device, uint32 dummy, uint32 start_msf, uint32 end_msf)
{
	int drive;
	struct cdrom_msf audio_bloc;

	drive = CheckDrive(device);
	if (drive<0) {
		return drive;
	}

	audio_bloc.cdmsf_min0 = (start_msf>>16) & 0xff;
	audio_bloc.cdmsf_sec0 = (start_msf>>8) & 0xff;
	audio_bloc.cdmsf_frame0 = (start_msf>>0) & 0xff;
	D(bug("nf: cdrom:  start: %02d:%02d:%02d", (start_msf>>16) & 0xff, (start_msf>>8) & 0xff, start_msf & 0xff));

	audio_bloc.cdmsf_min1 = (end_msf>>16) & 0xff;
	audio_bloc.cdmsf_sec1 = (end_msf>>8) & 0xff;
	audio_bloc.cdmsf_frame1 = (end_msf>>0) & 0xff;
	D(bug("nf: cdrom:  end:   %02d:%02d:%02d", (end_msf>>16) & 0xff, (end_msf>>8) & 0xff, end_msf & 0xff));

	return ioctl(drive_handles[drive], CDROMPLAYMSF, &audio_bloc);
}

int32 CdromDriver::cd_gettoc(memptr device, uint32 dummy, memptr buffer)
{
	int drive, i, numtracks, errorcode;
	struct cdrom_tochdr tochdr;
	struct cdrom_tocentry tocentry;
	gettoc_tocentry_t *atari_tocentry;

	drive = CheckDrive(device);
	if (drive<0) {
		return drive;
	}

	/* Read TOC header */
	errorcode=ioctl(drive_handles[drive], CDROMREADTOCHDR, &tochdr);
	if (errorcode<0) {
		return errorcode;
	}

	D(bug("nf: cdrom: GetToc():  TOC header read"));

	numtracks = tochdr.cdth_trk1-tochdr.cdth_trk0+1;

	/* Read TOC entries */
	atari_tocentry = (gettoc_tocentry_t *) Atari2HostAddr(buffer);
	for (i=0; i<=numtracks; i++) {
		D(bug("nf: cdrom: GetToc():  reading TOC entry for track %d", i));

		tocentry.cdte_track = tochdr.cdth_trk0+i;
		if (i == numtracks) {
			tocentry.cdte_track = CDROM_LEADOUT;
		}
		tocentry.cdte_format = CDROM_MSF;

		errorcode = ioctl(drive_handles[drive], CDROMREADTOCENTRY, &tocentry);
		if (errorcode<0) {
			return errorcode;
		}

		atari_tocentry[i].track = tocentry.cdte_track;
		atari_tocentry[i].minute = BinaryToBcd(tocentry.cdte_addr.msf.minute);
		atari_tocentry[i].second = BinaryToBcd(tocentry.cdte_addr.msf.second);
		atari_tocentry[i].frame = BinaryToBcd(tocentry.cdte_addr.msf.frame);
		if (tocentry.cdte_track == CDROM_LEADOUT) {
			atari_tocentry[i].track = CDROM_LEADOUT_CDAR;
		}

		D(bug("nf: cdrom: GetToc():   track %d, id 0x%02x, %s",
			i, tocentry.cdte_track,
			tocentry.cdte_ctrl & CDROM_DATA_TRACK ? "data" : "audio"
		));

		if (tocentry.cdte_format == CDROM_MSF) {
			D(bug("nf: cdrom: GetToc():    msf %02d:%02d:%02d, bcd: %02x:%02x:%02x",
				tocentry.cdte_addr.msf.minute,
				tocentry.cdte_addr.msf.second,
				tocentry.cdte_addr.msf.frame,
				atari_tocentry[i].minute,
				atari_tocentry[i].second,
				atari_tocentry[i].frame
			));
		} else {
			D(bug("nf: cdrom: GetToc():    lba 0x%08x", tocentry.cdte_addr.lba));
		}
	}

	atari_tocentry[numtracks+1].track =
		atari_tocentry[numtracks+1].minute =
		atari_tocentry[numtracks+1].second = 
		atari_tocentry[numtracks+1].frame = 0;

	return 0;
}

int32 CdromDriver::cd_discinfo(memptr device, memptr buffer)
{
	int drive, errorcode;
	struct cdrom_tochdr tochdr;
	struct cdrom_subchnl subchnl;
	struct cdrom_tocentry tocentry;
	atari_discinfo_t	*discinfo;

	drive = CheckDrive(device);
	if (drive<0) {
		return drive;
	}

	/* Read TOC header */
	errorcode=ioctl(drive_handles[drive], CDROMREADTOCHDR, &tochdr);
	if (errorcode<0) {
		return errorcode;
	}
	D(bug("nf: cdrom: DiscInfo():  TOC header read"));

	discinfo = (atari_discinfo_t *) Atari2HostAddr(buffer);
	memset(discinfo, 0, sizeof(atari_discinfo_t));

	discinfo->first = discinfo->current = tochdr.cdth_trk0;
	discinfo->last = tochdr.cdth_trk1;
	discinfo->index = 1;

	/* Read subchannel */
	subchnl.cdsc_format = CDROM_MSF;
	errorcode=ioctl(drive_handles[drive], CDROMSUBCHNL, &subchnl);
	if (errorcode<0) {
		return errorcode;
	}
	D(bug("nf: cdrom: DiscInfo():  Subchannel read"));

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

	/* Read toc entry for end of disc */
	tocentry.cdte_track = CDROM_LEADOUT;
	tocentry.cdte_format = CDROM_MSF;

	errorcode = ioctl(drive_handles[drive], CDROMREADTOCENTRY, &tocentry);
	if (errorcode<0) {
		return errorcode;
	}

	D(bug("nf: cdrom: DiscInfo():  Toc entry read"));

	discinfo->end.track = 0;
	discinfo->end.minute = BinaryToBcd(tocentry.cdte_addr.msf.minute);
	discinfo->end.second = BinaryToBcd(tocentry.cdte_addr.msf.second);
	discinfo->end.frame = BinaryToBcd(tocentry.cdte_addr.msf.frame);

	return 0;
}
