/*
	NatFeat host CD-ROM access

	ARAnyM (C) 2003-2011 Patrice Mandin

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

#ifndef NFCDROM_H
#define NFCDROM_H

/*--- Includes ---*/

#include "nf_base.h"
#include "parameters.h"

/*--- Class ---*/

#define CD_FRAMESIZE 2048
#ifndef CDROM_LEADOUT
#define CDROM_LEADOUT	0xaa
#endif

class CdromDriver : public NF_Base
{
protected:
	uint32	drives_mask;

	int GetDrive(memptr device);	/* Return drive letter of a metados device */
	unsigned char BinaryToBcd(unsigned char value);	/* Convert a value to BCD */

	virtual void ScanDrives();
	virtual int OpenDrive(memptr device) = 0;
	virtual void CloseDrive(int drive);

	virtual int32 cd_open(memptr /* metados_bos_header_t * */ device, memptr /* meta_drvinfo * */ buffer);
	virtual int32 cd_close(memptr /* metados_bos_header_t * */ device);
	virtual int32 cd_read(memptr /* metados_bos_header_t * */ device, memptr buffer, uint32 /* LBA */ first, uint32 length);
	virtual int32 cd_write(memptr /* metados_bos_header_t * */ device, memptr buffer, uint32 /* LBA */ first, uint32 length);
	virtual int32 cd_seek(memptr /* metados_bos_header_t * */ device, uint32 /* LBA */ offset);
	virtual int32 cd_status(memptr /* metados_bos_header_t * */ device, memptr ext_status) = 0;
	virtual int32 cd_ioctl(memptr /* metados_bos_header_t * */ device, uint16 opcode, memptr buffer) = 0;

	virtual int32 cd_startaudio(memptr /* metados_bos_header_t * */ device, uint32 dummy, memptr /* metados_bos_tracks_t * */ buffer) = 0;
	virtual int32 cd_stopaudio(memptr /* metados_bos_header_t * */ device) = 0;
	virtual int32 cd_setsongtime(memptr /* metados_bos_header_t * */ device, uint32 dummy, uint32 start_msf, uint32 end_msf) = 0;
	virtual int32 cd_gettoc(memptr /* metados_bos_header_t * */ device, uint32 dummy, memptr /* atari_tocentry_t * */ buffer) = 0;
	virtual int32 cd_discinfo(memptr /* metados_bos_header_t * */ device, memptr /* atari_discinfo_t * */ buffer) = 0;

public:
	CdromDriver(void);
	virtual ~CdromDriver(void);

	virtual int Count() = 0;
	virtual const char *DeviceName(int drive) = 0;
	
	const char *name() { return "CDROM"; }
	bool isSuperOnly() { return true; }
	int32 dispatch(uint32 fncode);
};

#endif /* NFCDROM_H */
