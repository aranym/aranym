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

#include "sysdeps.h"
#include "cpu_emulation.h"
#include "parameters.h"
#include "nfcdrom.h"
#include "nfcdrom_atari.h"
#include "../../atari/nfcdrom/nfcdrom_nfapi.h"
#include "toserror.h"

#define DEBUG 0
#include "debug.h"

#include <cstdlib>
#include <errno.h>
#include <SDL_endian.h>

/*--- Defines ---*/

#define NFCD_NAME	"nf:cdrom: "

/*--- Constructor/desctructor ---*/

CdromDriver::CdromDriver(void)
{
	D(bug(NFCD_NAME "CdromDriver()"));
	drives_mask = 0xffffffffUL;
}

CdromDriver::~CdromDriver(void)
{
	D(bug(NFCD_NAME "~CdromDriver()"));
	for (int i=0; i<CD_MAX_DRIVES; i++) {
		CloseDrive(i);
	}
}

/*--- Public functions ---*/

int32 CdromDriver::dispatch(uint32 fncode)
{
	int32 ret;

	D(bug(NFCD_NAME "dispatch(%u)", fncode));

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
			D(bug(NFCD_NAME " unimplemented function #%d", fncode));
			ret = TOS_ENOSYS;
			break;
	}
	D(bug(NFCD_NAME " function returning with 0x%08x", ret));
	return ret;
}

/*--- Protected functions ---*/

void CdromDriver::CloseDrive(int drive)
{
	UNUSED(drive);
}

int CdromDriver::GetDrive(memptr device)
{
	metados_bos_header_t bos_device;

/*	D(bug(NFCD_NAME "GetDrive()"));*/
	Atari2Host_memcpy(&bos_device, device, sizeof(bos_device));
	return SDL_SwapBE16(bos_device.phys_letter);
}

unsigned char CdromDriver::BinaryToBcd(unsigned char value)
{
	int decimal;
	
/*	D(bug(NFCD_NAME "BinaryToBcd()"));*/
	decimal = (value/10)*16;
	return decimal | (value % 10);
}

void CdromDriver::ScanDrives()
{
	int i;

	D(bug(NFCD_NAME "ScanDrives()"));

	drives_mask = 0;
	for (i=0; i<CD_MAX_DRIVES; i++) {
		/* Check if there is a valid filename */
		if (bx_options.nfcdroms[i].physdevtohostdev < 0) {
			continue;
		}

		/* Add to MetaDOS CD-ROM drives */
		drives_mask |= 1<<(i);
		bug(NFCD_NAME "ScanDrives(): physical device %c (%s) added", DriveToLetter(i), DeviceName(bx_options.nfcdroms[i].physdevtohostdev));
	}

	D(bug(NFCD_NAME "ScanDrives()=0x%08x", drives_mask));
}

/*--- MetaDOS functions ---*/

int32 CdromDriver::cd_open(memptr device, memptr buffer)
{
	UNUSED(device);
	UNUSED(buffer);
	D(bug(NFCD_NAME "cd_open()"));
	return TOS_E_OK;
}

int32 CdromDriver::cd_close(memptr device)
{
	UNUSED(device);
	D(bug(NFCD_NAME "cd_close()"));
	return TOS_E_OK;
}

int32 CdromDriver::cd_read(memptr device, memptr buffer, uint32 first, uint32 length)
{
	UNUSED(device);
	UNUSED(buffer);
	UNUSED(first);
	UNUSED(length);
	/* No CD-ROM support using SDL functions */
	return TOS_ENOSYS;
}

int32 CdromDriver::cd_write(memptr device, memptr buffer, uint32 first, uint32 length)
{
	UNUSED(device);
	UNUSED(buffer);
	UNUSED(first);
	UNUSED(length);
	D(bug(NFCD_NAME "cd_write()"));
	return TOS_ENOSYS;
}

int32 CdromDriver::cd_seek(memptr device, uint32 offset)
{
	UNUSED(device);
	UNUSED(offset);
	D(bug(NFCD_NAME "cd_seek()"));
	return TOS_ENOSYS;
}
