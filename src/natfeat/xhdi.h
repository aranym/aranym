/*
 * xhdi.h - XHDI like disk driver interface - declaration
 *
 * Copyright (c) 2002-2005 Petr Stehlik of ARAnyM dev team (see AUTHORS)
 * 
 * This file is part of the ARAnyM project which builds a new and powerful
 * TOS/FreeMiNT compatible virtual machine running on almost any hardware.
 *
 * ARAnyM is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * ARAnyM is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ARAnyM; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _XHDI_H
#define _XHDI_H
#include "nf_base.h"
#include "parameters.h"

#define ACSI_START	0
#define ACSI_END	7
#define SCSI_START	8
#define SCSI_END	15
#define IDE_START	16
#define IDE_END		17

typedef memptr wmemptr;
typedef memptr lmemptr;

struct disk_t {
	char path[512];
	char name[41];
	bool present;
	bool readonly;
	bool byteswap;
	bool sim_root; // true if the disk has to simulate root sector
	char partID[3];	// partition ID - not used for real harddisks
	int  size_blocks; // partition size in blocks - not used for real harddisks
};

class XHDIDriver : public NF_Base
{
private:
	disk_t disks[IDE_END+1];	// ACSI + SCSI + IDE

private:
	void copy_atadevice_settings(bx_atadevice_options_t *src, disk_t *dest);
	void copy_scsidevice_settings(bx_scsidevice_options_t *src, disk_t *dest);
	disk_t *dev2disk(uint16 major, uint16 minor);
	void byteSwapBuf(uint8 *buf, int size);
	bool setDiskSizeInBlocks(disk_t *disk);

protected:
	int32 XHDrvMap();
	int32 XHInqDriver(uint16 bios_device, memptr name, memptr version,
				memptr company, wmemptr ahdi_version, wmemptr maxIPL);
	int32 XHReadWrite(uint16 major, uint16 minor, uint16 rwflag,
				uint32 recno, uint16 count, memptr buf);
	int32 XHInqTarget2(uint16 major, uint16 minor, lmemptr blocksize,
				lmemptr device_flags, memptr product_name, uint16 stringlen);
	int32 XHInqDev2(uint16 bios_device, wmemptr major, wmemptr minor,
				lmemptr start_sector, memptr bpb, lmemptr blocks,
				memptr partid);
	int32 XHGetCapacity(uint16 major, uint16 minor,
				lmemptr blocks, lmemptr blocksize);

public:
	void reset();
	char *name() { return "XHDI"; }
	bool isSuperOnly() { return true; }
	int32 dispatch(uint32 fncode);
};

#endif /* _XHDI_H */

/*
vim:ts=4:sw=4:
*/
