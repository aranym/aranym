/*
 * xhdi.cpp - XHDI like disk driver interface
 *
 * Copyright (c) 2002-2004 Petr Stehlik of ARAnyM dev team (see AUTHORS)
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

#include "sysdeps.h"
#include "cpu_emulation.h"
#include "xhdi.h"

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
	// remember the values here so that SETUP changes don't affect
	// the pending disk operations
	ide0 = bx_options.atadevice[0][0];
	ide1 = bx_options.atadevice[0][1];
}

bx_atadevice_options_t *XHDIDriver::dev2disk(uint16 major, uint16 minor)
{
	if (minor != 0)
		return NULL;

	bx_atadevice_options_t *disk;
	switch(major) {
		case 16:	disk = &ide0; break;
		case 17:	disk = &ide1; break;
		default:	disk = NULL; break;
	}
	if (disk != NULL) {
		if (disk->present && !disk->isCDROM)
			return disk;
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

	bx_atadevice_options_t *disk = dev2disk(major, minor);
	if (disk == NULL)
		return EUNDEV;

	bool writing = (rwflag & 1);
	if (writing && disk->readonly)
		return EACCDN;

	FILE *f = fopen(disk->path, writing ? "r+b" : "rb");
	if (f != NULL) {
		int size = XHDI_BLOCK_SIZE*count;
		uint8 *hostbuf = Atari2HostAddr(buf);
		off_t offset = (off_t)recno * XHDI_BLOCK_SIZE;
		fseek(f, offset, SEEK_SET);
		if (writing) {
			if (! disk->byteswap)
				byteSwapBuf(hostbuf, size);
			fwrite(hostbuf, size, 1, f);
			if (! disk->byteswap)
				byteSwapBuf(hostbuf, size);
		}
		else {
			fread(hostbuf, size, 1, f);
			if (! disk->byteswap)
				byteSwapBuf(hostbuf, size);

		}
		fclose(f);
	}
	return E_OK;
}

int32 XHDIDriver::XHInqTarget2(uint16 major, uint16 minor, lmemptr blocksize,
					lmemptr device_flags, memptr product_name, uint16 stringlen)
{
	D(bug("ARAnyM XHInqTarget2(major=%u, minor=%u, product_name_len=%u)", major, minor, stringlen));

	bx_atadevice_options_t *disk = dev2disk(major, minor);
	if (disk == NULL)
		return EUNDEV;

	if (blocksize) {
		WriteInt32(blocksize, XHDI_BLOCK_SIZE);
	}

	if (device_flags) {
		WriteInt32(device_flags, 0);	// for CD-ROM could set REMOVABLE and EJECTABLE flags
	}

	if (product_name && stringlen) {
		f2amemcpy(product_name, disk->model, stringlen);	// strncpy would be better
		WriteInt8(product_name + stringlen-1, '\0');
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

	bx_atadevice_options_t *disk = dev2disk(major, minor);
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
