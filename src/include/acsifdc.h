/*
 * acsifdc.h - Atari floppy emulation code - class definition
 *
 * Copyright (c) 2001-2004 Petr Stehlik of ARAnyM dev team (see AUTHORS)
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

#ifndef _ACSIFDC_H
#define _ACSIFDC_H

#include "icio.h"

enum {
	DISKDMA_BIT0=0,	/* for scsi: bits 2-0 are ncr5380 register */
	DISKDMA_A0,		/* for floppy: bits A0,A1 are WD1772 register */
	DISKDMA_A1,
	DISKDMA_CS,		/* Select, 0=floppy, 1=acsi/scsi */
	DISKDMA_BIT4,
	DISKDMA_BIT5,
	DISKDMA_DMA,	/* DMA enabled, 0=on, 1=off */
	DISKDMA_DRQ,	/* Run command, 0=off, 1=on */
	DISKDMA_RW		/* DMA direction, 0=read, 1=write */
};

/* The DMA disk class */

class ACSIFDC : public BASE_IO {
private:
	memptr DMAaddr;
	bool floppy_changed;
	int drive_fd;
	int head, sides, tracks, spt, secsize;
	int dma_mode, dma_scr, dma_car, dma_sr;
	int fdc_command, fdc_track, fdc_sector, fdc_data, fdc_status;

public:
	ACSIFDC(memptr, uint32);
	virtual uint16 handleReadW(memptr);
	virtual void handleWriteW(memptr, uint16);
	virtual uint8 handleRead(memptr);
	virtual void handleWrite(memptr, uint8);
	memptr getDMAaddr() { return DMAaddr; }
	void setDMAaddr(memptr addr) { DMAaddr = addr; }
	void init();
	void remove_floppy();
	bool insert_floppy();
	bool is_floppy_inserted();

private:
	uint16 getDMAData();
	uint16 getDMAStatus();
	void setDMASectorCount(uint16);
	void setDMAMode(uint16);
	void set_floppy_geometry();
	void fdc_exec_command();
};

#endif /* _ACSIFDC_H */
