/*
 *  NatFeat host CD-ROM access
 *
 *  ARAnyM (C) 2003 Patrice Mandin
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef NFCDROM_H
#define NFCDROM_H

/*--- Includes ---*/

#include <SDL_cdrom.h>

#include "nf_base.h"
#include "parameters.h"

/*--- Defines ---*/

#define ENOTREADY	-2
#define EINVFN -32

#define CDROM_LEADOUT_CDAR	0xa2

/*--- Types ---*/

typedef struct {
	unsigned char count;
	unsigned char first;
} metados_bos_tracks_t;

typedef struct {
	unsigned char	cdth_trk0;      /* start track */
	unsigned char	cdth_trk1;      /* end track */
} atari_cdromtochdr_t;

typedef struct  {
	/* input parameters */
	unsigned char	cdte_track;     /* track number or CDROM_LEADOUT */
	unsigned char	cdte_format;    /* CDROM_LBA or CDROM_MSF */

	/* output parameters */
	unsigned char	cdte_info;
#if 0
    unsigned    cdte_adr:4;     /* the SUBQ channel encodes 0: nothing,
                                    1: position data, 2: MCN, 3: ISRC,
                                    else: reserved */
    unsigned    cdte_ctrl:4;    /* bit 0: audio with pre-emphasis,
                                    bit 1: digital copy permitted,
                                    bit 2: data track,
                                    bit 3: four channel */
#endif
	unsigned char	cdte_datamode;		/* currently not set */
	Uint16	dummy;	/* PM: what is this for ? */
	Uint32	cdte_addr;			/* track start */
} atari_cdromtocentry_t;

typedef struct {	/* TOC entry for MetaGetToc() function */
	unsigned char track;
	unsigned char minute;
	unsigned char second;
	unsigned char frame;
} atari_tocentry_t;

typedef struct {	/* Discinfo for MetaDiscInfo() function */
	unsigned char disctype, first, last, current;
	atari_tocentry_t	relative, absolute, end;
	unsigned char index, reserved1[3];
	Uint32	reserved2[123];
} atari_discinfo_t;

typedef struct {
	/* input parameters */
	unsigned char	cdsc_format;	/* CDROM_MSF or CDROM_LBA */
    
	/* output parameters */
	unsigned char	cdsc_audiostatus;	/* see below */
	unsigned char	cdsc_resvd;	/* reserved */
	unsigned char	cdsc_info;
#if 0
    unsigned	cdsc_adr:	4;	/* in info field */
    unsigned	cdsc_ctrl:	4;	/* in info field */
#endif
	unsigned char	cdsc_trk;	/* current track */
	unsigned char	cdsc_ind;	/* current index */
	Uint32	cdsc_absaddr;	/* absolute address */
	Uint32	cdsc_reladdr;	/* track relative address */
} atari_cdromsubchnl_t;

/*--- Class ---*/

class CdromDriver : public NF_Base
{
	private:
		SDL_CD *drive_handles[32];	/* Handle for each possible opened drive */

	protected:
		uint32	drives_mask;

		int GetDrive(memptr device);	/* Return drive letter of a metados device */
		unsigned char BinaryToBcd(unsigned char value);	/* Convert a value to BCD */

		virtual void ScanDrives();
		virtual int OpenDrive(memptr device);
		virtual void CloseDrive(int drive);

		int32 cd_open(memptr device, memptr buffer);
		int32 cd_close(memptr device);
		virtual int32 cd_read(memptr device, memptr buffer, uint32 first, uint32 length);
		int32 cd_write(memptr device, memptr buffer, uint32 first, uint32 length);
		int32 cd_seek(memptr device, uint32 offset);
		virtual int32 cd_status(memptr device, memptr ext_status);
		virtual int32 cd_ioctl(memptr device, uint16 opcode, memptr buffer);
	
		virtual int32 cd_startaudio(memptr device, uint32 dummy, memptr buffer);
		virtual int32 cd_stopaudio(memptr device);
		virtual int32 cd_setsongtime(memptr device, uint32 dummy, uint32 start_msf, uint32 end_msf);
		virtual int32 cd_gettoc(memptr device, uint32 dummy, memptr buffer);
		virtual int32 cd_discinfo(memptr device, memptr buffer);

	public:
		CdromDriver(void);
		virtual ~CdromDriver(void);

		const char *name();
		bool isSuperOnly();
		int32 dispatch(uint32 fncode);
};

#endif /* NFCDROM_H */
