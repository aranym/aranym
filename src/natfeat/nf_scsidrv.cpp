/*
 * nf_scsidrv.c
 * 
 * Copyright (C) 2015 by Uwe Seimet
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
 * nf_scsidrv.c - Implementation of the host system part of a SCSI Driver
 * (Linux only), based on the Linux SG driver version 3. The corresponding
 * TOS binary and its source code can be downloaded from
 * http://hddriver.seimet.de/en/downloads.html, where you can also find
 * information on the open SCSI Driver standard.
 */

#include "sysdeps.h"

#if NFSCSI_SUPPORT

#include "cpu_emulation.h"
#include "nf_scsidrv.h"
#include "host.h"

#define DEBUG 0
#include "debug.h"

#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <scsi/sg.h>
#include "toserror.h"

// The driver interface version, 1.00
#define INTERFACE_VERSION 0x0100
// Maximum is 20 characters
#define BUS_NAME "Linux Generic SCSI"
// The SG driver supports cAllCmds
#define BUS_FEATURES 0x02
// The transfer length may depend on the device, 65536 should always be safe
#define BUS_TRANSFER_LEN 65536


enum SCSIDRV_OPERATIONS {
	SCSI_INTERFACE_VERSION = 0,
	SCSI_INTERFACE_FEATURES,
	SCSI_INQUIRE_BUS,
	SCSI_OPEN,
	SCSI_INOUT,
	SCSI_CHECK_DEV
};

int32 SCSIDriver::check_device_file(Uint32 id)
{
	char device_file[16];
	sprintf(device_file, "/dev/sg%d", id);

	if(!access(device_file, R_OK | W_OK))
	{
		D(bug("device file %s is accessible", device_file));
		return 0;
	}
	else
	{
		D(bug("device file %s is inaccessible", device_file));
		return -1;
	}
}

int32 SCSIDriver::interface_features(memptr busName, memptr features, memptr transferLen)
{
	Host2AtariSafeStrncpy(busName, BUS_NAME, 20);
	WriteNFInt16(features, BUS_FEATURES);
	WriteNFInt32(transferLen, BUS_TRANSFER_LEN);

	return 0;
}

int32 SCSIDriver::inquire_bus(Uint32 id)
{
	D(bug("scsidrv_inquire_bus: id=%d", id));

	char device_file[16];
	sprintf(device_file, "/dev/sg%d", id);

	while(!access(device_file, F_OK))
	{
		if(!check_device_file(id))
		{
			return id;
		}

		sprintf(device_file, "/dev/sg%d", ++id);
	}

	return -1;
}

int32 SCSIDriver::Open(Uint32 id)
{
	D(bug("scsidrv_open: id=%d", id));
	
	return check_device_file(id);
}

int32 SCSIDriver::inout(Uint32 dir, Uint32 id, unsigned char *cmd, Uint32 cmd_len, unsigned char *buffer, Uint32 transfer_len, unsigned char *sense_buffer, Uint32 timeout)
{
	if (sense_buffer)
		memset(sense_buffer, 0, 18);

#if DEBUG
	{
		Uint32 i;
		char str[80];
		
		*str = '\0';
		for(i = 0; i < cmd_len && i < 10; i++)
		{
			sprintf(str + strlen(str), i ? ":$%02X" : "$%02X", cmd[i]);
		}
		D(bug("scsidrv_inout: dir=%d, id=%d, cmd_len=%d, transfer_len=%d, timeout=%d, cmd=%s",
			dir, id, cmd_len, transfer_len, timeout, str));
	}
#endif

	// No explicit LUN support, the SG driver maps LUNs to device files
	if(cmd[1] & 0xe0)
	{
		if (sense_buffer)
		{
			// Sense Key and ASC
			sense_buffer[2] = 0x05;
			sense_buffer[12] = 0x25;
	
			D(bug("             Sense Key=$%02X, ASC=$%02X, ASCQ=$00",
					  sense_buffer[2], sense_buffer[12]));
		}
			
		return 2;
	}

	char device_file[16];
	sprintf(device_file, "/dev/sg%d", id);

	int fd = ::open(device_file, O_RDWR | O_NONBLOCK | O_EXCL);
	if(fd < 0) {
		D(bug("              Cannot open device file %s", device_file));

		return -1;
	}
	
	struct sg_io_hdr io_hdr;
	memset(&io_hdr, 0, sizeof(struct sg_io_hdr));

	io_hdr.interface_id = 'S';

	io_hdr.dxfer_direction = dir ? SG_DXFER_TO_DEV : SG_DXFER_FROM_DEV;
	if(!transfer_len) {
		io_hdr.dxfer_direction = SG_DXFER_NONE;
	}
	
	io_hdr.dxferp = buffer;
	io_hdr.dxfer_len = transfer_len;

	io_hdr.sbp = sense_buffer;
	io_hdr.mx_sb_len = sense_buffer ? 18 : 0;

	io_hdr.cmdp = cmd;
	io_hdr.cmd_len = cmd_len;

	io_hdr.timeout = timeout;

	int status = ioctl(fd, SG_IO, &io_hdr) < 0 ? -1 : io_hdr.status;

	::close(fd);

	if (status > 0 && sense_buffer)
	{
		D(bug("             Sense Key=$%02X, ASC=$%02X, ASCQ=$%02X",
				  sense_buffer[2], sense_buffer[12], sense_buffer[13]));
	}

	return status;
}

int32 SCSIDriver::check_dev(Uint32 id)
{
	D(bug("scsidrv_check_dev: id=%d", id));

	return check_device_file(id);
}

SCSIDriver::SCSIDriver()
{
	reset();
}

SCSIDriver::~SCSIDriver()
{
	reset();
}

void SCSIDriver::reset()
{
}

int32 SCSIDriver::dispatch(uint32 fncode)
{
	int32 ret;

	SAVE_EXCEPTION;
	{
		TRY(prb)
		{
			switch (fncode)
			{
			case  SCSI_INTERFACE_VERSION:
				D(bug("SCSI: Version"));
				ret = INTERFACE_VERSION;
				break;
			
			case SCSI_INTERFACE_FEATURES:
				ret = interface_features(getParameter(0), getParameter(1), getParameter(2));
				break;
			
			case SCSI_INQUIRE_BUS:
				ret = inquire_bus(getParameter(0));
				break;
				
			case SCSI_OPEN:
				ret = Open(getParameter(0));
				break;
			
			case SCSI_INOUT:
				{
					Uint32 dir = getParameter(0);
					Uint32 id = getParameter(1);
					memptr cmd = getParameter(2);
					Uint32 cmd_len = getParameter(3);
					memptr buffer = getParameter(4);
					Uint32 transfer_len = getParameter(5);
					memptr sense_buffer = getParameter(6);
					Uint32 timeout = getParameter(7);
	
					ret = inout(
						dir,
						id,
						(unsigned char *)(cmd ? Atari2HostAddr(cmd) : 0),
						cmd_len,
						(unsigned char *)(buffer ? Atari2HostAddr(buffer) : 0),
						transfer_len,
						(unsigned char *)(sense_buffer ? Atari2HostAddr(sense_buffer) : 0),
						timeout);
				}
				break;
				
			case SCSI_CHECK_DEV:
				ret = check_dev(getParameter(0));
				break;
			
			default:
				D(bug("SCSI: Invalid SCSI Driver operation %d requested", fncode));
				ret = -1;
				break;
			}
		} CATCH(prb)
		{
			ret = TOS_EIMBA; /* EFAULT */
		}
	}
	RESTORE_EXCEPTION;
	return ret;
}

#endif /* NFSCSI_SUPPORT */
