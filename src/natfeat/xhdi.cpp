/*
 * xhdi.cpp - XHDI like disk driver interface
 *
 * Copyright (c) 2002-2008 Petr Stehlik of ARAnyM dev team (see AUTHORS)
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
#include <cassert>
#include "cpu_emulation.h"
#include "xhdi.h"
#include "atari_rootsec.h"
#include "tools.h"
#include <errno.h>
#if (defined(X86_ASSEMBLY) || defined(X86_64_ASSEMBLY)) && defined(__SSE2__)
#include <emmintrin.h>
/* #define USE_SSE_BYTESWAP 1 */
#endif

#define DEBUG 0
#include "debug.h"

#ifndef HAVE_FSEEKO
#  define fseeko(a,b,c)	fseek(a,b,c)
#endif
#ifdef __BEOS__
extern "C" int fseeko(FILE *stream, off_t offset, int whence);
#endif

#define XHDI_BLOCK_SIZE	512

/* XHDI error codes */
#include "toserror.h"


XHDIDriver::XHDIDriver()
{
	init_disks();
}

XHDIDriver::~XHDIDriver()
{
	close_disks();
}

void XHDIDriver::reset()
{
	close_disks();
	init_disks();
}

void XHDIDriver::init_disks()
{
	// init all disks
	for(unsigned i=0; i<sizeof(disks)/sizeof(disks[0]); i++) {
		disks[i].present = false;
		disks[i].file = NULL;
	}

	// setup disks array with copied disks and partitions values so that
	// user's changes in SETUP GUI don't affect the pending disk operations
	for(int i=0; i<DISKS; i++) {
		copy_scsidevice_settings(i, &bx_options.disks[i], disks+SCSI_START+i);
	}
	for(int i=0; i<2; i++) {
		copy_atadevice_settings(&bx_options.atadevice[0][i], disks+IDE_START+i);
	}
}

void XHDIDriver::close_disks()
{
	// close all open disks
	for(unsigned i=0; i<sizeof(disks)/sizeof(disks[0]); i++) {
		disk_t *disk = &disks[i];
		if (disk->file != NULL) {
#ifdef HAVE_FSYNC
			fsync(fileno(disk->file));
#endif
			fclose(disk->file);
			disk->file = NULL;
		}
	}
}

void XHDIDriver::copy_atadevice_settings(const bx_atadevice_options_t *src, disk_t *dest)
{
	safe_strncpy(dest->path, src->path, sizeof(dest->path));
	safe_strncpy(dest->name, src->model, sizeof(dest->name));
	dest->present = (src->isCDROM ? false : src->present);	// CD-ROMs not supported via XHDI
	dest->readonly = src->readonly;
	dest->byteswap = src->byteswap;
	dest->sim_root = false;		// ATA devices are real disks

	// check and remember disk size
	setDiskSizeInBlocks(dest);
}

void XHDIDriver::copy_scsidevice_settings(int index, const bx_scsidevice_options_t *src, disk_t *dest)
{
	safe_strncpy(dest->path, src->path, sizeof(dest->path));
	sprintf(dest->name, "PARTITION%d", index);
	dest->present = src->present;
	dest->readonly = src->readonly;
	dest->byteswap = src->byteswap;
	dest->sim_root = true;		// SCSI devices are simulated by prepending a virtual root sector to a single partition

	// check and remember disk size
	setDiskSizeInBlocks(dest);

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

void XHDIDriver::byteSwapBuf(uint8 *dst, int size)
{
#ifdef USE_SSE_BYTESWAP
	__m128i A;

	if ((((uintptr)dst) & 0xf) == 0)
	{
		while (size >= 16)
		{
			A = *((__m128i *)dst);
			*((__m128i *)dst) = _mm_or_si128(_mm_srli_epi16(A, 8), _mm_or_si128(_mm_slli_epi16(A, 8), _mm_setzero_si128()));
			dst += 16;
			size -= 16;
		}
	}
#endif
	while (size >= 2)
	{
		char tmp = *dst++;
		dst[-1] = *dst;
		*dst++ = tmp;
		size -= 2;
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

	return TOS_EINVFN;
}


int32 XHDIDriver::XHReadWrite(uint16 major, uint16 minor,
					uint16 rwflag, uint32 recno, uint16 count, memptr buf)
{
	D(bug("ARAnyM XH%s(%u.%u, recno=%lu, count=%u, buf=$%x)",
		(rwflag & 1) ? "Write" : "Read",
		major, minor, recno, count, buf));

	disk_t *disk = dev2disk(major, minor);
	if (disk == NULL) {
		return TOS_EUNDEV;
	}

	bool writing = (rwflag & 1);
	if (writing && disk->readonly) {
		return TOS_EACCDN;
	}

	FILE *f = disk->file;
	if (f == NULL) {
		// TODO FIXME - if opening the IDE disk drives here then block the ATA emu layer
		// until the disk->path is closed here again! Otherwise bad data loss might occur
		// if both the direct PARTITION access and the ATA emu layer start writing to the same disk
		f = fopen(disk->path, disk->readonly ? "rb" : "r+b");
		if (f != NULL) {
			disk->file = f;
		}
		else {
			return TOS_EDRVNR;
		}
	}

	if (disk->sim_root) {
		if (recno == 0 && count > 0) {
			if (!writing) {
				// simulate the root sector
				assert(sizeof(rootsector) == XHDI_BLOCK_SIZE);
				rootsector sector;
				memset(&sector, 0, sizeof(rootsector));

				sector.hd_siz = SDL_SwapBE32(disk->size_blocks);

				sector.part[0].flg = 1;
				if (disk->partID[0] == '$') {
					sector.part[0].id[0] = 0;
					sector.part[0].id[1] = 'D';
					char str[3] = {disk->partID[1], disk->partID[2], 0};
					sector.part[0].id[2] = strtol(str, NULL, 16);
				}
				else {
					sector.part[0].id[0] = disk->partID[0];
					sector.part[0].id[1] = disk->partID[1];
					sector.part[0].id[2] = disk->partID[2];
				}
				int start_sect = 1;
				sector.part[0].st = SDL_SwapBE32(start_sect);
				sector.part[0].siz = SDL_SwapBE32(disk->size_blocks - start_sect);

				// zero out the other three partitions in PTBL
				for(int i=1; i<4; i++) {
					sector.part[i].flg = 0;
					sector.part[i].id[0] = 0;
					sector.part[i].id[1] = 0;
					sector.part[i].id[2] = 0;
					sector.part[i].st = 0;
					sector.part[i].siz = 0;
				}

				Host2Atari_memcpy(buf, &sector, sizeof(sector));
			}
			// correct the count and buffer position
			count--;
			buf+=XHDI_BLOCK_SIZE;
			if (count == 0) {
				return writing ? TOS_EACCDN : TOS_E_OK;
			}
		}

		// correct the offset to the partition
		if (recno > 0)
			recno--;
	}

	off_t offset = (off_t)recno * XHDI_BLOCK_SIZE;
	if (fseeko(f, offset, SEEK_SET) != 0)
		return errnoHost2Mint(errno, TOS_EINVAL);
	if (count == 0)
		return writing ? TOS_EACCDN : TOS_E_OK;
	
	memptr bytes = count * XHDI_BLOCK_SIZE;
	if (writing)
	{
#ifdef USE_SSE_BYTESWAP
		__attribute__((__aligned__(16))
#endif
		uint8 tempbuf[bytes];

		memptr src_end = buf + bytes - 1;
		if (! ValidAtariAddr(buf, false, 1))
			BUS_ERROR(buf);
		if (! ValidAtariAddr(src_end, false, 1))
			BUS_ERROR(src_end);
		uint8 *src = Atari2HostAddr(buf);
		if (! disk->byteswap)
		{
			memcpy(tempbuf, src, bytes);
			byteSwapBuf(tempbuf, bytes);
			src = tempbuf;
		}
		if (fwrite(src, bytes, 1, f) != 1) {
			panicbug("nfXHDI: Error writing to device %u.%u (record=%d)", major, minor, recno);
			return TOS_EWRITF;
		}
	} else {
		memptr dst_end = buf + bytes - 1;
		if (! ValidAtariAddr(buf, true, 1))
			BUS_ERROR(buf);
		if (! ValidAtariAddr(dst_end, true, 1))
			BUS_ERROR(dst_end);
		uint8 *dst = Atari2HostAddr(buf);
		if (fread(dst, bytes, 1, f) != 1) {
			panicbug("nfXHDI: error reading device %u.%u (record=%d)", major, minor, recno);
			return TOS_EREADF;
		} else
		{
			if (! disk->byteswap)
				byteSwapBuf(dst, bytes);
		}
	}
	return TOS_E_OK;
}

int32 XHDIDriver::XHInqTarget2(uint16 major, uint16 minor, lmemptr blocksize,
					lmemptr device_flags, memptr product_name, uint16 stringlen)
{
	D(bug("ARAnyM XHInqTarget2(%u.%u, product_name_len=%u)", major, minor, stringlen));

	disk_t *disk = dev2disk(major, minor);
	if (disk == NULL)
		return TOS_EUNDEV;

	if (blocksize) {
		WriteInt32(blocksize, XHDI_BLOCK_SIZE);
	}

	if (device_flags) {
		WriteInt32(device_flags, 0);
	}

	if (product_name && stringlen) {
		Host2AtariSafeStrncpy(product_name, disk->name, stringlen);
	}

	return TOS_E_OK;
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

	return TOS_EINVFN;
}

int32 XHDIDriver::XHGetCapacity(uint16 major, uint16 minor,
					lmemptr blocks, lmemptr blocksize)
{
	D(bug("ARAnyM XHGetCapacity(%u.%u, blocks=%lu, blocksize=%lu)", major, minor, blocks, blocksize));

	disk_t *disk = dev2disk(major, minor);
	if (disk == NULL)
		return TOS_EUNDEV;

	if (! setDiskSizeInBlocks(disk))
		return TOS_EDRVNR;

	D(bug("XHGetCapacity in blocks = %ld\n", disk->size_blocks));
	if (blocks != 0)
		WriteAtariInt32(blocks, disk->size_blocks);
	if (blocksize != 0)
		WriteAtariInt32(blocksize, XHDI_BLOCK_SIZE);
	return TOS_E_OK;
}

int32 XHDIDriver::dispatch(uint32 fncode)
{
	D(bug("ARAnyM XHDI(%u)", fncode));
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
		
		case  2: /* XHReserve */
		case  3: /* XHLock */
		case  4: /* XHStop */
		case  5: /* XHEject */
		case  9: /* XHNewCookie */
		case 15: /* XHMediumChanged */
		case 16: /* XHMiNTInfo */
		case 17: /* XHDOSLimits */
		case 18: /* XHLastAccess */
		case 19: /* XHReaccess */
		default: ret = TOS_EINVFN;
				D(bug("Unimplemented ARAnyM XHDI function #%d", fncode));
				break;
	}
	D(bug("ARAnyM XHDI function returning with %d", ret));
	return ret;
}

bool XHDIDriver::setDiskSizeInBlocks(disk_t *disk)
{
	disk->size_blocks = 0;

	if (! disk->present)
		return false;

	struct stat buf;
	long blocks = 0;
	if (stat(disk->path, &buf))
		return false;

	if (S_ISBLK(buf.st_mode)) {
		int fd = open(disk->path, 0);
		if (fd < 0) {
			panicbug("open(%s) failed", disk->path);
			return false;
		}
		blocks = lseek(fd, 0, SEEK_END) / XHDI_BLOCK_SIZE;
		close(fd);
		D(bug("%ld blocks on %s", blocks, disk->path));
	}
	else {
		blocks = buf.st_size / XHDI_BLOCK_SIZE;
	}

	if (disk->sim_root)
		blocks++;	// add the virtual master boot record

	disk->size_blocks = blocks;

	return true;
}

/*
vim:ts=4:sw=4:
*/
