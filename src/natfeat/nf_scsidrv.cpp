/*
 * nf_scsidrv.c
 * 
 * Copyright (C) 2016 by Uwe Seimet
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

#ifdef NFSCSI_SUPPORT

#include "cpu_emulation.h"
#include "nf_scsidrv.h"
#include "host.h"

#define DEBUG 0
#include "debug.h"

#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#ifdef HAVE_LIBUDEV
#include <libudev.h>
#endif
#include <sys/ioctl.h>
#include <scsi/sg.h>
#include "toserror.h"

// The driver interface version, 1.02
#define INTERFACE_VERSION 0x0102
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
	SCSI_CLOSE,
	SCSI_INOUT,
	SCSI_ERROR,
	SCSI_CHECK_DEV
};

void SCSIDriver::set_error(Uint32 handle, Uint32 errbit)
{
	for(Uint32 i = 0; i < SCSI_MAX_HANDLES; i++)
	{
		if(handle != i && handle_meta_data[i].fd != 0 &&
		   handle_meta_data[i].id_lo == handle_meta_data[handle].id_lo)
		{
			handle_meta_data[i].error |= errbit;
		}
	}
}

// udev-based check for media change
bool SCSIDriver::check_mchg_udev()
{
	bool changed = false;

#ifdef HAVE_LIBUDEV
	if (udev_mon_fd <= 0)
		return false;

	fd_set udevFds;

	FD_ZERO(&udevFds);
	FD_SET(udev_mon_fd, &udevFds);

	int ret = select(udev_mon_fd + 1, &udevFds, 0, 0, &tv);
	if (ret > 0 && FD_ISSET(udev_mon_fd, &udevFds))
	{
		struct udev_device *dev = udev_monitor_receive_device(mon);
		while (dev)
		{
			if (!changed)
			{
				const char *dev_type = udev_device_get_devtype(dev);
				const char *action = udev_device_get_action(dev);
				if (!strcmp("disk", dev_type) && !strcmp("change", action))
				{
					D(bug(": %s has been changed", udev_device_get_devnode(dev)));
		
					// TODO Determine sg device name from block device name
					// and only report media change for the actually affected device
		
					changed = true;
				}
			}
		
			// Process all pending events
			dev = udev_monitor_receive_device(mon);
		}
	}
#endif

	return changed;
}

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

int32 SCSIDriver::Open(Uint32 handle, Uint32 id)
{
	D(bug("scsidrv_open: handle=%d, id=%d", handle, id));
	
	if (handle >= SCSI_MAX_HANDLES || handle_meta_data[handle].fd != 0 || check_device_file(id))
	{
		return -1;
	}
	
#ifdef HAVE_LIBUDEV
	if (!udev)
	{
		udev = udev_new();
		if (!udev)
			return -1;

		mon = udev_monitor_new_from_netlink(udev, "udev");
		udev_monitor_filter_add_match_subsystem_devtype(mon, "block", NULL);
		udev_monitor_enable_receiving(mon);
		udev_mon_fd = udev_monitor_get_fd(mon);

		tv.tv_sec = 0;
		tv.tv_usec = 0;
	}
#endif

	char device_file[16];
	sprintf(device_file, "/dev/sg%d", id);
	
	int fd = ::open(device_file, O_RDWR | O_NONBLOCK);
	if (fd < 0)
	{
		D(bug("              Cannot open device file %s", device_file));
		return fd;
	}
	
	handle_meta_data[handle].fd = fd;
	handle_meta_data[handle].id_lo = id;
	handle_meta_data[handle].error = 0;

	return 0;
}

int32 SCSIDriver::Close(Uint32 handle)
{
	D(bug("scsidrv_close: handle=%d", handle));
	
	if (handle >= SCSI_MAX_HANDLES || handle_meta_data[handle].fd == 0)
	{
 		return TOS_EIHNDL;
	}
	
	::close(handle_meta_data[handle].fd);
	
	handle_meta_data[handle].fd = 0;

	return 0;
}

int32 SCSIDriver::inout(Uint32 handle, Uint32 dir, unsigned char *cmd, Uint32 cmd_len, unsigned char *buffer, Uint32 transfer_len, unsigned char *sense_buffer, Uint32 timeout)
{
	if (sense_buffer)
		memset(sense_buffer, 0, 18);

	if (handle >= SCSI_MAX_HANDLES || handle_meta_data[handle].fd == 0)
 		return TOS_EIHNDL;

#if DEBUG
	{
		Uint32 i;
		char str[80];
		
		*str = '\0';
		if (cmd)
			for(i = 0; i < cmd_len && i < 10; i++)
			{
				sprintf(str + strlen(str), i ? ":$%02X" : "$%02X", cmd[i]);
			}
		D(bug("scsidrv_inout: handle=%d, dir=%d, cmd_len=%d, transfer_len=%d, timeout=%d, cmd=%s",
			handle, dir, cmd_len, transfer_len, timeout, str));
	}
#endif

	// No explicit LUN support, the SG driver maps LUNs to device files
	if (cmd && (cmd[1] & 0xe0))
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

	int status;
	if(check_mchg_udev())
	{
		// cErrMediach for all open handles
		Uint32 i;
		for(i = 0; i < SCSI_MAX_HANDLES; i++)
		{
			if(handle_meta_data[i].fd)
			{
				handle_meta_data[i].error |= 1;
			}
		}
	
		if(sense_buffer)
		{
			// Sense Key and ASC
			sense_buffer[2] = 0x06;
			sense_buffer[12] = 0x28;
		}
	
		status = 2;
	}
	else
	{
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
	
		status = ioctl(handle_meta_data[handle].fd, SG_IO, &io_hdr) < 0 ? -1 : io_hdr.status;
	}

	if (status > 0 && sense_buffer)
	{
		D(bug("             Sense Key=$%02X, ASC=$%02X, ASCQ=$%02X",
				  sense_buffer[2], sense_buffer[12], sense_buffer[13]));
		if(status == 2)
		{
			// Automatic media change and reset handling for
			// SCSI Driver version 1.0.2
			if((sense_buffer[2] & 0x0f) && !sense_buffer[13])
			{
				if(sense_buffer[12] == 0x28)
				{
					// cErrMediach
					set_error(handle, 1);
				}
				else if(sense_buffer[12] == 0x29)
				{
					// cErrReset
				    set_error(handle, 2);
				}
			}
		}
	}

	return status;
}

int32 SCSIDriver::Error(Uint32 handle, Uint32 rwflag, Uint32 errnum)
{
	D(bug("scsidrv_error: handle=%d, rwflag=%d, errno=%d",
	  handle, rwflag, errnum));

	if (handle >= SCSI_MAX_HANDLES || handle_meta_data[handle].fd == 0)
		return TOS_EIHNDL;

	int errbit = 1 << errnum;
	if (rwflag != 0)
	{
		set_error(handle, errbit);

		return 0;
	}
	else
	{
		int status = handle_meta_data[handle].error & errbit;
		handle_meta_data[handle].error &= ~errbit;

		return status;
	}
}

int32 SCSIDriver::check_dev(Uint32 id)
{
	D(bug("scsidrv_check_dev: id=%d", id));

	return check_device_file(id);
}

SCSIDriver::SCSIDriver()
{
	for (Uint32 handle = 0; handle < SCSI_MAX_HANDLES; handle++)
	{
		handle_meta_data[handle].fd = 0;
		handle_meta_data[handle].id_lo = 0;
		handle_meta_data[handle].error = 0;
	}
#ifdef HAVE_LIBUDEV
	udev_mon_fd = -1;
	udev = 0;
#endif
	reset();
}

SCSIDriver::~SCSIDriver()
{
	reset();
}

void SCSIDriver::reset()
{
	for (Uint32 handle = 0; handle < SCSI_MAX_HANDLES; handle++)
		Close(handle);
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
				ret = Open(getParameter(0), getParameter(1));
				break;
			
			case SCSI_CLOSE:
				ret = Close(getParameter(0));
				break;
			
			case SCSI_INOUT:
				{
					Uint32 handle = getParameter(0);
					Uint32 dir = getParameter(1);
					memptr cmd = getParameter(2);
					Uint32 cmd_len = getParameter(3);
					memptr buffer = getParameter(4);
					Uint32 transfer_len = getParameter(5);
					memptr sense_buffer = getParameter(6);
					Uint32 timeout = getParameter(7);
	
					ret = inout(
						handle,
						dir,
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
			
			case SCSI_ERROR:
				ret = Error(getParameter(0), getParameter(1), getParameter(2));
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

	D(bug(" -> %d", ret));
	
	return ret;
}

#endif /* NFSCSI_SUPPORT */
