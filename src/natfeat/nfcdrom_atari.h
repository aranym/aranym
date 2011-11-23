/*
	NatFeat host CD-ROM access
	Atari CD-ROM ioctls, structures and defines

	ARAnyM (C) 2011 Patrice Mandin

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

#ifndef NFCDROM_ATARI_H
#define NFCDROM_ATARI_H

/*--- Defines ---*/

#define ENOTREADY	-2
#define EINVFN -32

#define CDROM_LEADOUT_CDAR	0xa2

/* CD-ROM address types (cdrom_tocentry.cdte_format) */
#define	CDROM_LBA 0x01 /* "logical block": first frame is #0 */
#define	CDROM_MSF 0x02 /* "minute-second-frame": binary, not bcd here! */

/* audio states (from SCSI-2, but seen with other drives, too) */
#define	CDROM_AUDIO_INVALID	0x00	/* audio status not supported */
#define	CDROM_AUDIO_PLAY	0x11	/* audio play operation in progress */
#define	CDROM_AUDIO_PAUSED	0x12	/* audio play operation paused */
#define	CDROM_AUDIO_COMPLETED	0x13	/* audio play successfully completed */
#define	CDROM_AUDIO_ERROR	0x14	/* audio play stopped due to error */
#define	CDROM_AUDIO_NO_STATUS	0x15	/* no current audio status to return */

/*--- Types ---*/

typedef struct {
	unsigned char count;
	unsigned char first;
} metados_bos_tracks_t;

typedef struct {
	Uint32 next;	/* (void *) for Atari */
	Uint32 attrib;
	Uint16 phys_letter;
	Uint16 dma_channel;
	Uint16 sub_device;
	Uint32 functions;	/* (void *) for Atari */
	Uint16 status;
	Uint32 reserved[2];
	Uint8 name[32];
} metados_bos_header_t;

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

typedef struct {
	/* input parameters */
	Uint16	set;    /* 0 == inquire only */

	/* input/output parameters */
	struct {
		unsigned char selection;
		unsigned char volume;
	} channel[4];
} atari_cdrom_audioctrl_t;

typedef struct {
	unsigned char	audiostatus;
	unsigned char	mcn[23];
} atari_mcn_t;

/*--- Enums ---*/

/* Atari ioctl */
enum {
	ATARI_CDROMREADOFFSET = (('C'<<8)|0x00),
	ATARI_CDROMPAUSE,
	ATARI_CDROMRESUME,
	ATARI_CDROMPLAYMSF,
	ATARI_CDROMPLAYTRKIND,
	ATARI_CDROMREADTOCHDR,
	ATARI_CDROMREADTOCENTRY,
	ATARI_CDROMSTOP,
	ATARI_CDROMSTART,
	ATARI_CDROMEJECT,
	ATARI_CDROMVOLCTRL,
	ATARI_CDROMSUBCHNL,
	ATARI_CDROMREADMODE2,
	ATARI_CDROMREADMODE1,
	ATARI_CDROMPREVENTREMOVAL,
	ATARI_CDROMALLOWREMOVAL,
	ATARI_CDROMAUDIOCTRL,
	ATARI_CDROMREADDA,
	ATARI_CDROM12,	/* unused */
	ATARI_CDROMGETMCN,
	ATARI_CDROMGETTISRC
};

#endif /* NFCDROM_ATARI_H */
