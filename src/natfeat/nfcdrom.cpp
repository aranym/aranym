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

#include <SDL_endian.h>

#include "sysdeps.h"
#include "cpu_emulation.h"
#include "parameters.h"
#include "nfcdrom.h"
#include "../../atari/nfcdrom/nfcdrom_nfapi.h"

#define DEBUG 0
#include "debug.h"

/*--- Defines ---*/

#define NFCD_NAME	"nf:cdrom: "

#define EINVFN -32

/*--- Types ---*/

typedef struct {
	unsigned long next;	/* (void *) for Atari */
	unsigned long attrib;
	unsigned short phys_letter;
	unsigned short dma_channel;
	unsigned short sub_device;
	unsigned long functions;	/* (void *) for Atari */
	unsigned short status;
	unsigned long reserved[2];
	char name[32];
} __attribute__((packed)) metados_bos_header_t;

/*--- Public functions ---*/

char *CdromDriver::name()
{
	return "CDROM";
}

bool CdromDriver::isSuperOnly()
{
	return true;
}

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
			ret = EINVFN;
			break;
	}
	D(bug(NFCD_NAME " function returning with 0x%08x", ret));
	return ret;
}

/*--- Protected functions ---*/

int CdromDriver::GetDrive(memptr device)
{
	metados_bos_header_t *bos_device;

/*	D(bug(NFCD_NAME "GetDrive()"));*/
	bos_device=(metados_bos_header_t *)Atari2HostAddr(device);
	return SDL_SwapBE16(bos_device->phys_letter);
}

unsigned char CdromDriver::BinaryToBcd(unsigned char value)
{
	int decimal;
	
/*	D(bug(NFCD_NAME "BinaryToBcd()"));*/
	decimal = (value/10)*16;
	return decimal | (value % 10);
}

int32 CdromDriver::cd_open(memptr device, memptr buffer)
{
	DUNUSED(device);
	DUNUSED(buffer);
	D(bug(NFCD_NAME "cd_open()"));
	return 0;
}

int32 CdromDriver::cd_close(memptr device)
{
	DUNUSED(device);
	D(bug(NFCD_NAME "cd_close()"));
	return 0;
}

int32 CdromDriver::cd_write(memptr device, memptr buffer, uint32 first, uint32 length)
{
	DUNUSED(device);
	DUNUSED(buffer);
	DUNUSED(first);
	DUNUSED(length);
	D(bug(NFCD_NAME "cd_write()"));
	return EINVFN;
}

int32 CdromDriver::cd_seek(memptr device, uint32 offset)
{
	DUNUSED(device);
	DUNUSED(offset);
	D(bug(NFCD_NAME "cd_seek()"));
	return EINVFN;
}
