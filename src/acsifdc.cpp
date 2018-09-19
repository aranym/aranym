/*
 * acsifdc.cpp - Atari floppy emulation code
 *
 * Copyright (c) 2001-2009 Petr Stehlik of ARAnyM dev team (see AUTHORS)
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

/*
 * The simple FDC emulation was derived from FAST's FDC code. FAST is
 * an Atari ST emulator written by Joachim Hoenig
 * (hoenig@informatik.uni-erlangen.de). Bugs are probably implemented by
 * me (nino@complang.tuwien.ac.at), so don't bother him with questions
 * regarding this code!
 */

#include "sysdeps.h"
#include <cassert>
#include <vector>

#include "hardware.h"
#include "cpu_emulation.h"
#include "memory-uae.h"
#include "acsifdc.h"
#include "ncr5380.h"
#include "parameters.h"

#define DEBUG 0
#include "debug.h"

/* Defines */

#define REMOUNT_FLOPPY_ON_BOOTSECTOR_READING	1

enum {
	WD1772_REG_COMMAND=0,
	WD1772_REG_STATUS=WD1772_REG_COMMAND,
	WD1772_REG_TRACK,
	WD1772_REG_SECTOR,
	WD1772_REG_DATA
};

/* Variables */

static NCR5380 ncr5380;
static int panic_floppy_error = 0;

ACSIFDC::ACSIFDC(memptr addr, uint32 size) : BASE_IO(addr, size) {
	drive_fd = -1;

	reset();
}

void ACSIFDC::reset()
{
	DMAaddr = 0;
	floppy_changed = false;
	head = sides = tracks = spt = secsize = 0;
	dma_mode = dma_scr = dma_car = dma_sr = 0;
	fdc_command = fdc_track = fdc_sector = fdc_data = fdc_status = 0;

	insert_floppy();
}

uint16 ACSIFDC::handleReadW(memptr addr) {
	int value = 0;
	switch(addr - getHWoffset()) {
		case 4:	value = getDMAData(); break;
		case 6: value = getDMAStatus(); break;
		default: value = BASE_IO::handleReadW(addr);
	}

	// D(bug("Reading ACSIFDC word data from %04lx = %d ($%02x) at %06x\n", addr, value, value, showPC()));
	return value;
}

uint8 ACSIFDC::handleRead(memptr addr) {
	addr -= getHWoffset();
	if (addr >= getHWsize())
		return 0;

	int value = 0;
	switch(addr) {
		case 4:	value = getDMAData() >> 8; break;
		case 5: value = getDMAData(); break;
		case 6: value = getDMAStatus() >> 8; break;
		case 7: value = getDMAStatus(); break;
		case 9: value = DMAaddr >> 16; D(bug("reading DMA addr")); break;
		case 0x0b: value = DMAaddr >> 8; break;
		case 0x0d: value = DMAaddr; break;
		case 0x0f: value = 0; break;	// missing floppy-control reg emulation
	}

	// D(bug("Reading ACSIFDC data from %04lx = %d ($%02x) at %06x\n", addr, value, value, showPC()));
	return value;
}

void ACSIFDC::handleWriteW(memptr addr, uint16 value) {
	switch(addr - getHWoffset()) {
		case 4: setDMASectorCount(value); break;
		case 6: setDMAMode(value); break;
		default: BASE_IO::handleWriteW(addr, value);
	}
}

void ACSIFDC::handleWrite(memptr addr, uint8 value) {
	addr -= getHWoffset();
	if (addr >= getHWsize())
		return;

	// D(bug("Writing ACSIFDC data to %04lx = %d ($%02x) at %06x\n", addr, value, value, showPC()));
	switch(addr) {
		case 4: setDMASectorCount(value << 8); break;
		case 5: setDMASectorCount(value); break;
		case 6: setDMAMode(value << 8); break;
		case 7: setDMAMode(value); break;
		case 9: DMAaddr = (DMAaddr & 0x00ffff) | (value << 16); break;
		case 0x0b: DMAaddr = (DMAaddr & 0xff00ff) | (value << 8); break;
		case 0x0d: DMAaddr = (DMAaddr & 0xffff00) | value; break;
		case 0x0f: break;	// missing floppy-control reg emulation
	}
}

uint16 ACSIFDC::getDMAData()
{
	if (dma_mode & 0x10)
	{
		return dma_scr&0xff;
	}
	else
	{
		if (dma_mode & (1<<DISKDMA_CS))
		{
			dma_car = ncr5380.ReadData(dma_mode);

			getMFP()->setGPIPbit(0x20, 0x20);
			return dma_car&0xff;
		}
		else
		{
			int wd1772_reg;

			wd1772_reg = (dma_mode>>DISKDMA_A0) & 3;

			switch (wd1772_reg)
			{
				case WD1772_REG_STATUS:
					getMFP()->setGPIPbit(0x20, 0x20);
					if (floppy_changed) {
						floppy_changed = false;
						return fdc_status | 0x40;
					}
					return fdc_status&0xff;
				case WD1772_REG_TRACK:
					return fdc_track&0xff;
				case WD1772_REG_SECTOR:
					return fdc_sector&0xff;
				case WD1772_REG_DATA:
					return fdc_data&0xff;
				default:
					return 0;
			}
		}
	}
}

uint16 ACSIFDC::getDMAStatus()
{
	return dma_sr;
}

void ACSIFDC::setDMASectorCount(uint16 vv)
{
	// D(bug("DMA car/scr <- %x (mode=%x)", vv, dma_mode));
	if (dma_mode&0x10)
	{
		dma_scr = vv;
		D(bug("scr = %d", dma_scr));
	}
	else
	{
		if (dma_mode & (1<<DISKDMA_CS))
		{
			getMFP()->setGPIPbit(0x20, 0x20);
			dma_car = vv;

			ncr5380.WriteData(dma_mode, dma_car);
		}
		else
		{
			int wd1772_reg;

			wd1772_reg = (dma_mode>>DISKDMA_A0) & 3;

			switch (wd1772_reg)
			{
				case WD1772_REG_COMMAND:
					fdc_command = vv;
					fdc_exec_command();
					break;
				case WD1772_REG_TRACK:
					fdc_track = vv;
					break;
				case WD1772_REG_SECTOR:
					fdc_sector = vv;
					break;
				case WD1772_REG_DATA:
					fdc_data = vv;
					break;
			}
		}
	}
}

void ACSIFDC::setDMAMode(uint16 vv)
{
	dma_mode = vv;
	// D(bug("DMA mode <- %04x", dma_mode));
}

/*************************************************************************/

// parameters of default floppy (3.5" HD 1.44 MB)
#define SECSIZE		512
#define SIDES		2
#define SPT			18
#define TRACKS		80

void ACSIFDC::set_floppy_geometry()
{
	if (drive_fd > 0)
	{
		unsigned char buf[512];
		bool valid = true;	// suppose the bootsector data is valid
		int sectors = 0;

		// read bootsector
		lseek(drive_fd, 0, SEEK_SET);
		if (read(drive_fd, buf, sizeof(buf)) == sizeof(buf)) {
			// detect floppy geometry from bootsector data
			sectors=(buf[20]<<8)|buf[19];
			secsize=(buf[12]<<8)|buf[11];
			spt=buf[24];
			sides=buf[26];

			// check validity of data
			if (secsize <= 0 || sectors <= 0 || spt <=0 || sides <= 0) {
				// data is obviously invalid (probably unformatted disk)
				valid = false;
			}
			else {
				tracks = sectors / spt / sides;
				// check if all sectors are on the tracks
				if ((sides * spt * tracks) != sectors)
					valid = false;
			}
		}
		else {
			panicbug("FDC A: error reading boot sector");
			valid = false;
		}

		if (! valid) {
			// bootsector contains invalid data - use our default
			secsize = SECSIZE;
			sides = SIDES;

			// for a 80 track floppy compute the sector per track, otherwise assume HD floppy
			off_t disk_size = lseek(drive_fd, 0, SEEK_END);
			size_t bpt = (secsize * sides * TRACKS);
			spt = ((disk_size / bpt) && !(disk_size % bpt)) ? (disk_size / bpt) : SPT;

			sectors = spt * sides * TRACKS;
		}

		tracks = (sectors / spt / sides);

		D(bug("FDC A: %d/%d/%d %d bytes/sector", sides,tracks,spt,secsize));
	}
}

void ACSIFDC::remove_floppy()
{
	if (drive_fd >= 0) {
		close(drive_fd);
		drive_fd = -1;
		D(bug("Floppy removed"));
	}
}

bool ACSIFDC::insert_floppy()
{
	remove_floppy();

	char *path = bx_options.floppy.path;

	if (strlen(path) == 0)	// is path to floppy defined?
		return false;

	int status = open(path, O_RDWR
#ifdef HAVE_O_FSYNC
			| O_FSYNC
#else
# ifdef O_SYNC
			| O_SYNC
# endif
#endif
#ifdef O_BINARY
			| O_BINARY
#endif
			);
	bool rw = true;
	if (status < 0) {
		status = open(path, O_RDONLY
#ifdef O_BINARY
					| O_BINARY
#endif
					);
		rw = false;
	}
	if (status < 0) {
		D(bug("Inserting of floppy failed."));
		return false;
	}

	D(bug("Floppy inserted %s", rw ? "read-write" : "read-only"));
	DUNUSED(rw);
	drive_fd = status;

	set_floppy_geometry();

	floppy_changed = true;
	panic_floppy_error = 0;
	return true;
}

bool ACSIFDC::is_floppy_inserted()
{
	return (drive_fd >= 0);
}

bool ACSIFDC::read_file(int device, long offset, memptr address, int secsize, int count)
{
	if (lseek(device, offset, SEEK_SET) < 0) return false;
	std::vector<uint8> buffer(secsize);
	for(int i=0; i<count; i++) {
		if (::read(device, &buffer[0], secsize) != secsize) return false;
		memcpy(Atari2HostAddr(address), &buffer[0], secsize);
		address += secsize;
	}
	return true;
}

bool ACSIFDC::write_file(int device, long offset, memptr address, int secsize, int count)
{
	if (lseek(device, offset, SEEK_SET) < 0) return false;
	std::vector<uint8> buffer(secsize);
	for(int i=0; i<count; i++) {
		memcpy(&buffer[0], Atari2HostAddr(address), secsize);
		address += secsize;
		if (::write(device, &buffer[0], secsize) != secsize) return false;
	}
	return true;
}

void ACSIFDC::fdc_exec_command()
{
	static int dir=1,motor=1;
	int actual_side, d;
	long offset;

	int snd_porta = getYAMAHA()->getFloppyStat();
	D(bug("FDC DMA address = %06x, snd = %d", DMAaddr, snd_porta));
	actual_side=(~snd_porta)&1;
	d=(~snd_porta)&6;
	switch(d)
	{
		case 2:
		case 6:
			d=0;
			break;
		case 4:
			d=1;
			// we don't emulate second floppy drive
			d=-1;
			break;
		case 0:
			d=-1;
			break;
	}
	D(bug("FDC command 0x%04x drive=%d",fdc_command,d));
	fdc_status=0;
	if (fdc_command < 0x80)
	{
		if (d>=0)
		{
			switch(fdc_command&0xf0)
			{
				case 0x00:
					D(bug("\tFDC RESTORE"));
					head=0;
					fdc_track=0;
					break;
				case 0x10:
					D(bug("\tFDC SEEK to %d",fdc_data));
					head += fdc_data-fdc_track;
					fdc_track=fdc_data;
					if (head<0 || head>=tracks)
						head=0;
					break;
				case 0x30:
					fdc_track+=dir;
					/* fall through */
				case 0x20:
					head+=dir;
					break;
				case 0x50:
					fdc_track++;
					/* fall through */
				case 0x40:
					if (head<tracks)
						head++;
					dir=1;
					break;
				case 0x70:
					fdc_track--;
					/* fall through */
				case 0x60:
					if (head > 0)
						head--;
					dir=-1;
					break;
			}
			if (head==0)
				fdc_status |= 4;
			if (head != fdc_track && (fdc_command & 4))
				fdc_status |= 0x10;
			if (motor)
				fdc_status |= 0x20;
		}
		else fdc_status |= 0x10;
	}
	else if ((fdc_command & 0xf0) == 0xd0)
	{
		if (fdc_command == 0xd8)
			getMFP()->setGPIPbit(0x20, 0);
		else if (fdc_command == 0xd0)
			getMFP()->setGPIPbit(0x20, 0x20);
	}
	else
	{
		if (d>=0)
		{
			int record_not_found = 0;
			offset=secsize
				* (((spt*sides*head))
				+ (spt * actual_side) + (fdc_sector-1));
			// special hack for 'fixing' dma_scr in Linux where it's often = 20
			int newscr = spt - fdc_sector + 1;
			if (newscr < dma_scr && spt > 1) {
				D(bug("FDC: Fixed SCR from %d to %d", dma_scr, newscr));
				record_not_found = 1;
				dma_scr = newscr;
			}
			switch(fdc_command & 0xf0)
			{
				case 0x80:
					assert(dma_scr == 1); // otherwise the fallthrough will cause problems
					// fallthrough
				case 0x90:
					D(bug("\tFDC READ SECTOR  %d to 0x%06lx", dma_scr, DMAaddr));
#if REMOUNT_FLOPPY_ON_BOOTSECTOR_READING
					// special hack for remounting physical floppy on
					// bootsector access
					if (offset == 0 && dma_scr == 1) {
						D(bug("Remounting floppy - media change requested?"));
						// reading boot sector might indicate media change test
						insert_floppy();
					}
#endif
					if (read_file(drive_fd, offset, DMAaddr, secsize, dma_scr)) {
						DMAaddr += dma_scr*secsize;
						dma_scr=0;
						dma_sr=1;
						if (record_not_found)
							fdc_status |= 0x10;
						break;
					}
					else {
						if (! panic_floppy_error++)
							panicbug("Floppy read(%d, %06x, %d) failed.", drive_fd, DMAaddr, dma_scr);
					}
					fdc_status |= 0x10;
					dma_sr=1;
					break;
				case 0xa0:
					assert(dma_scr == 1); // otherwise the fallthrough will cause problems
					// fallthrough
				case 0xb0:
					if (write_file(drive_fd, offset, DMAaddr, secsize, dma_scr)) {
						DMAaddr += dma_scr*secsize;
						dma_scr=0;
						dma_sr=1;
						if (record_not_found)
							fdc_status |= 0x10;
						break;
					}
					else {
						if (! panic_floppy_error++)
							panicbug("Floppy write(%d, %06x, %d) failed.", drive_fd, DMAaddr, dma_scr);
					}
					fdc_status |= 0x10;
					dma_sr=1;
					break;
				case 0xc0:
					fdc_status |= 0x10;
					break;
				case 0xe0:
					fdc_status |= 0x10;
					break;
				case 0xf0:
					fdc_status |= 0x10;
					break;
			}
			if (head != fdc_track) fdc_status |= 0x10;
		}
		else fdc_status |= 0x10;
	}
	if (motor)
		fdc_status |= 0x80;
	if (!(fdc_status & 1))
		getMFP()->setGPIPbit(0x20, 0);
}

/*
vim:ts=4:sw=4:
*/
