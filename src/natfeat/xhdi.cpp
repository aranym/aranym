/*
 * xhdi.cpp - XHDI like disk driver interface
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

#include <SDL_endian.h>

#include "sysdeps.h"
#include "cpu_emulation.h"
#include "xhdi.h"
#include "atari_rootsec.h"
#include "tools.h"

#define DEBUG 0
#include "debug.h"

#define XHDI_BLOCK_SIZE	512

/* XHDI error codes */
#define EDRVNR	-2	/* device not responding */
#define EUNDEV	-15 /* unknown device */
#define EINVFN	-32	/* invalid function number = unimplemented function */
#define EACCDN	-36	/* access denied, device is reserved */
#define E_OK	0

void XHDIDriver::reset()
{
	// setup disks array with copied disks and partitions values so that
	// user's changes in SETUP GUI don't affect the pending disk operations
	copy_scsidevice_settings(&bx_options.disk0, disks+SCSI_START+0);
	copy_scsidevice_settings(&bx_options.disk1, disks+SCSI_START+1);
	copy_scsidevice_settings(&bx_options.disk2, disks+SCSI_START+2);
	copy_scsidevice_settings(&bx_options.disk3, disks+SCSI_START+3);
	copy_scsidevice_settings(&bx_options.disk4, disks+SCSI_START+4);
	copy_scsidevice_settings(&bx_options.disk5, disks+SCSI_START+5);
	copy_scsidevice_settings(&bx_options.disk6, disks+SCSI_START+6);
	copy_scsidevice_settings(&bx_options.disk7, disks+SCSI_START+7);

	copy_atadevice_settings(&bx_options.atadevice[0][0], disks+IDE_START+0);
	copy_atadevice_settings(&bx_options.atadevice[0][1], disks+IDE_START+1);
}

void XHDIDriver::copy_atadevice_settings(bx_atadevice_options_t *src, disk_t *dest)
{
	safe_strncpy(dest->path, src->path, sizeof(dest->path));
	safe_strncpy(dest->name, src->model, sizeof(dest->name));
	dest->present = (src->isCDROM ? false : src->present);	// CD-ROMs not supported via XHDI
	dest->readonly = src->readonly;
	dest->byteswap = src->byteswap;
	dest->sim_root = false;		// ATA devices are real disks
	dest->size_blocks = 0;
}

void XHDIDriver::copy_scsidevice_settings(bx_scsidevice_options_t *src, disk_t *dest)
{
	safe_strncpy(dest->path, src->path, sizeof(dest->path));
	safe_strncpy(dest->name, src->name, sizeof(dest->name));
	dest->present = src->present;
	dest->readonly = src->readonly;
	dest->byteswap = src->byteswap;
	dest->sim_root = true;		// SCSI devices are simulated by prepending a virtual root sector to a single partition

	// check and remember disk size
	struct stat buf;
	if (dest->present && !stat(dest->path, &buf)) {
		dest->size_blocks = buf.st_size / XHDI_BLOCK_SIZE;
	}
	else {
		dest->size_blocks = 0;
	}

	dest->partID[0] = src->partID[0];
	dest->partID[1] = src->partID[1];
	dest->partID[2] = src->partID[2];
}

disk_t *XHDIDriver::dev2disk(uint16 major, uint16 minor)
{
	if (minor != 0)
		return NULL;

	disk_t *disk = NULL;
	if (major >= SCSI_START && major <= IDE_END) {
		disk = &disks[major];
		if (disk->present) {
			return disk;
		}
	}

	return NULL;
}

void XHDIDriver::byteSwapBuf(uint8 *buf, int size)
{
	for(int i=0; i<size; i++) {
		int tmp = buf[i];
		buf[i] = buf[i+1];
		buf[++i] = tmp;
	}
}

int32 XHDIDriver::XHDrvMap()
{
	D(bug("ARAnyM XHDrvMap"));

	return 0;	// drive map
}

int32 XHDIDriver::XHInqDriver(uint16 bios_device, memptr name, memptr version,
					memptr company, wmemptr ahdi_version, wmemptr maxIPL)
{
	DUNUSED(bios_device);
	DUNUSED(name);
	DUNUSED(version);
	DUNUSED(company);
	DUNUSED(ahdi_version);
	DUNUSED(maxIPL);
	D(bug("ARAnyM XHInqDriver(bios_device=%u)", bios_device));

	return EINVFN;
}


int32 XHDIDriver::XHReadWrite(uint16 major, uint16 minor,
					uint16 rwflag, uint32 recno, uint16 count, memptr buf)
{
	D(bug("ARAnyM XH%s(major=%u, minor=%u, recno=%lu, count=%u, buf=$%x)",
		(rwflag & 1) ? "Write" : "Read",
		major, minor, recno, count, buf));

	disk_t *disk = dev2disk(major, minor);
	if (disk == NULL) {
		return EUNDEV;
	}

	bool writing = (rwflag & 1);
	if (writing && disk->readonly) {
		return EACCDN;
	}

	FILE *f = fopen(disk->path, writing ? "r+b" : "rb");
	if (f == NULL) {
		return EDRVNR;
	}

	uint8 *hostbuf = Atari2HostAddr(buf);

	if (disk->sim_root) {
		assert(sizeof(rootsector) == XHDI_BLOCK_SIZE);
		if (recno == 0 && !writing) {
			// simulate the root sector
			rootsector sector;
			memset(&sector, 0, sizeof(rootsector));

			sector.hd_siz = SDL_SwapBE32(disk->size_blocks + 1);

			sector.part[0].flg = 1;
			sector.part[0].id[0] = disk->partID[0];
			sector.part[0].id[1] = disk->partID[1];
			sector.part[0].id[2] = disk->partID[2];
			sector.part[0].st = SDL_SwapBE32(1);
			sector.part[0].siz = SDL_SwapBE32(disk->size_blocks);

			sector.part[1].flg = 0;
			sector.part[1].id[0] = 0;
			sector.part[1].id[1] = 0;
			sector.part[1].id[2] = 0;
			sector.part[1].st = 0;
			sector.part[1].siz = 0;

			sector.part[2].flg = 0;
			sector.part[2].id[0] = 0;
			sector.part[2].id[1] = 0;
			sector.part[2].id[2] = 0;
			sector.part[2].st = 0;
			sector.part[2].siz = 0;

			sector.part[3].flg = 0;
			sector.part[3].id[0] = 0;
			sector.part[3].id[1] = 0;
			sector.part[3].id[2] = 0;
			sector.part[3].st = 0;
			sector.part[3].siz = 0;

			memcpy(hostbuf, &sector, sizeof(sector));
		}
		// correct the offset and count to the partition
		recno--;
		count--;
		hostbuf+=XHDI_BLOCK_SIZE;
		if (count == 0) {
			return E_OK;
		}
	}

	int size = XHDI_BLOCK_SIZE*count;
	off_t offset = (off_t)recno * XHDI_BLOCK_SIZE;
	fseek(f, offset, SEEK_SET);
	if (writing) {
		if (! disk->byteswap)
			byteSwapBuf(hostbuf, size);
		if (fwrite(hostbuf, size, 1, f) != 1) {
			panicbug("error writing");
		}
		if (! disk->byteswap)
			byteSwapBuf(hostbuf, size);
	}
	else {
		if (fread(hostbuf, size, 1, f) != 1) {
			panicbug("error reading");
		}
		if (! disk->byteswap)
			byteSwapBuf(hostbuf, size);

	}
	fclose(f);
	return E_OK;
}

int32 XHDIDriver::XHInqTarget2(uint16 major, uint16 minor, lmemptr blocksize,
					lmemptr device_flags, memptr product_name, uint16 stringlen)
{
	D(bug("ARAnyM XHInqTarget2(major=%u, minor=%u, product_name_len=%u)", major, minor, stringlen));

	disk_t *disk = dev2disk(major, minor);
	if (disk == NULL)
		return EUNDEV;

	if (blocksize) {
		WriteInt32(blocksize, XHDI_BLOCK_SIZE);
	}

	if (device_flags) {
		WriteInt32(device_flags, 0);
	}

	if (product_name && stringlen) {
		host2AtariSafeStrncpy(product_name, disk->name, stringlen);
	}

	return E_OK;
}

int32 XHDIDriver::XHInqDev2(uint16 bios_device, wmemptr major, wmemptr minor,
					lmemptr start_sector, memptr bpb, lmemptr blocks,
					memptr partid)
{
	DUNUSED(bios_device);
	DUNUSED(major);
	DUNUSED(minor);
	DUNUSED(start_sector);
	DUNUSED(bpb);
	DUNUSED(blocks);
	DUNUSED(partid);
	D(bug("ARAnyM XHInqDev2(bios_device=%u)", bios_device));

	return EINVFN;
}

int32 XHDIDriver::XHGetCapacity(uint16 major, uint16 minor,
					lmemptr blocks, lmemptr blocksize)
{
	D(bug("ARAnyM XHGetCapacity(major=%u, minor=%u, blocks=%lu, blocksize=%lu)", major, minor, blocks, blocksize));

	disk_t *disk = dev2disk(major, minor);
	if (disk == NULL)
		return EUNDEV;

	struct stat buf;
	if (! stat(disk->path, &buf)) {
		long t_blocks = buf.st_size / XHDI_BLOCK_SIZE;
		D(bug("t_blocks = %ld\n", t_blocks));
		if (blocks != 0)
			WriteAtariInt32(blocks, t_blocks);
		if (blocksize != 0)
			WriteAtariInt32(blocksize, XHDI_BLOCK_SIZE);
		return E_OK;
	}
	else {
		return EDRVNR;
	}
}

int32 XHDIDriver::dispatch(uint32 fncode)
{
	D(bug("ARAnyM XHDI(%u)\n", fncode));
	int32 ret;
	switch(fncode) {
		case  0: ret = 0x0130;	/* XHDI version */
				break;

		case  1: ret = XHInqTarget2(
						getParameter(0), /* UWORD major */
						getParameter(1), /* UWORD minor */
						getParameter(2), /* ULONG *block_size */
						getParameter(3), /* ULONG *device_flags */
						getParameter(4), /* char  *product_name */
						             33  /* UWORD stringlen */
						);
				break;

		case  6: ret = XHDrvMap();
				break;

		case  7: ret = XHInqDev2(
						getParameter(0), /* UWORD bios_device */
						getParameter(1), /* UWORD *major */
						getParameter(2), /* UWORD *minor */
						getParameter(3), /* ULONG *start_sector */
						getParameter(4), /* BPB   *bpb */
						              0, /* ULONG *blocks */
						              0  /* char *partid */
						);
				break;

		case  8: ret = XHInqDriver(
						getParameter(0), /* UWORD bios_device */
						getParameter(1), /* char  *name */
						getParameter(2), /* char  *version */
						getParameter(3), /* char  *company */
						getParameter(4), /* UWORD *ahdi_version */
						getParameter(5)  /* UWORD *maxIPL */
						);
				break;

		case 10: ret = XHReadWrite(
						getParameter(0), /* UWORD major */
						getParameter(1), /* UWORD minor */
						getParameter(2), /* UWORD rwflag */
						getParameter(3), /* ULONG recno */
						getParameter(4), /* UWORD count */
						getParameter(5)  /* void *buf */
						);
				break;

		case 11: ret = XHInqTarget2(
						getParameter(0), /* UWORD major */
						getParameter(1), /* UWORD minor */
						getParameter(2), /* ULONG *block_size */
						getParameter(3), /* ULONG *device_flags */
						getParameter(4), /* char  *product_name */
						getParameter(5)  /* UWORD stringlen */
						);
				break;

		case 12: ret = XHInqDev2(
						getParameter(0), /* UWORD bios_device */
						getParameter(1), /* UWORD *major */
						getParameter(2), /* UWORD *minor */
						getParameter(3), /* ULONG *start_sector */
						getParameter(4), /* BPB   *bpb */
						getParameter(5), /* ULONG *blocks */
						getParameter(6)  /* char *partid */
						);
				break;

		case 14: ret = XHGetCapacity(
						getParameter(0), /* UWORD major */
						getParameter(1), /* UWORD minor */
						getParameter(2), /* ULONG *blocks */
						getParameter(3)  /* ULONG *blocksize */
						);
				break;
				
		default: ret = EINVFN;
				D(bug("Unimplemented ARAnyM XHDI function #%d", fncode));
				break;
	}
	D(bug("ARAnyM XHDI function returning with %d", ret));
	return ret;
}
