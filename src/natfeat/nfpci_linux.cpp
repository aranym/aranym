/*
	NatFeat host PCI access, Linux PCI driver

	ARAnyM (C) 2004 Patrice Mandin

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

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#include <SDL_endian.h>

#include "sysdeps.h"
#include "cpu_emulation.h"
#include "parameters.h"
#include "nfpci.h"
#include "nfpci_linux.h"

#define DEBUG 0
#include "debug.h"

/*--- Defines ---*/

#define NFPCI_NAME	"nf:pci:linux: "

#define PCI_FILE_DEVICES	"/proc/bus/pci/devices"

/*--- Public functions ---*/

PciDriverLinux::PciDriverLinux()
{
	uint32 device[17], i;
	char buffer[512];
	FILE *f, *fc;

	D(bug(NFPCI_NAME "PciDriverLinux()"));
	num_pci_devices=0;
	pci_devices=NULL;

	f=fopen(PCI_FILE_DEVICES,"r");
	if (f==NULL) {
		return;
	}

	while (fgets(buffer, sizeof(buffer)-1, f)) {
		pci_device_t *new_device;

		sscanf(buffer, "%x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x",
			&device[0], &device[1], &device[2], &device[3], 
			&device[4], &device[5], &device[6], &device[7], 
			&device[8], &device[9], &device[10], &device[11], 
			&device[12], &device[13], &device[14], &device[15], 
			&device[16]
		);

		/* Add device to list */
		++num_pci_devices;
		pci_devices=(pci_device_t *)realloc(pci_devices, num_pci_devices * sizeof(pci_device_t));
		new_device = &pci_devices[num_pci_devices-1];
		for (i=0; i<17; i++) {
			new_device->info[i]=device[i];
		}
		snprintf(new_device->filename, sizeof(new_device->filename),
			"/proc/bus/pci/%02x/%02x.%x",
			(new_device->info[0] >> 8) & 0xff,
			(new_device->info[0] >> 3) & 0x1f,
			new_device->info[0] & 0x07
		);

		/* Read configuration bytes, 256 bytes max. */
		memset(new_device->config, 0, sizeof(new_device->config));

		fc=fopen(new_device->filename, "r");
		if (fc==NULL) {
			continue;
		}

		new_device->config_length = fread(&(new_device->config[0]), 1, 256, fc);
		fclose(fc);
	}

	fclose(f);

	/* List devices */
#if DEBUG
	D(bug(NFPCI_NAME " %d PCI devices found:", num_pci_devices));
	for (i=0; i<num_pci_devices; i++) {
		int k;

		D(bug(NFPCI_NAME "  Bus %d, Vendor 0x%04x, Device 0x%04x, %s",
			pci_devices[i].info[0], (pci_devices[i].info[1]>>16) & 0xffff,
			pci_devices[i].info[1] & 0xffff,
			pci_devices[i].filename
		));

		for (k=0; k<(pci_devices[i].config_length>>3); k++) {
			sprintf(buffer, "0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x",
				pci_devices[i].config[k*8+0],
				pci_devices[i].config[k*8+1],
				pci_devices[i].config[k*8+2],
				pci_devices[i].config[k*8+3],
				pci_devices[i].config[k*8+4],
				pci_devices[i].config[k*8+5],
				pci_devices[i].config[k*8+6],
				pci_devices[i].config[k*8+7]
			);
			D(bug(NFPCI_NAME "   %s", buffer));
		}		
	}
#endif	
}

PciDriverLinux::~PciDriverLinux()
{
	D(bug(NFPCI_NAME "~PciDriverLinux()"));

	if (pci_devices!=NULL) {
		free(pci_devices);
		pci_devices=NULL;
	}
	num_pci_devices=0;
}

/*--- Private functions ---*/

int32 PciDriverLinux::find_device(uint32 device_vendor_id, uint32 index)
{
	uint32 i, j;
	
	D(bug(NFPCI_NAME "find_device(0x%08x,%d)", device_vendor_id, index));

	j=0;
	for (i=0; i<num_pci_devices; i++) {
		if (device_vendor_id!=0xffffUL) {
			if (pci_devices[i].info[1]!=device_vendor_id) {
				continue;
			}
		}

		if (j==index) {
			return i;
		}
		j++;
	}

	return ATARI_PCI_DEVICE_NOT_FOUND;
}

int32 PciDriverLinux::find_classcode(uint32 class_code, uint32 index)
{
	uint32 j, i, class_device, class_mask;

	D(bug(NFPCI_NAME "find_classcode(0x%08x,%d)", class_code, index));

	class_mask = (class_code>>24) & 0x07;
	if ((class_mask & (1<<2))==0) {
		class_code &= 0x0000ffffUL;
	}
	if ((class_mask & (1<<1))==0) {
		class_code &= 0x00ff00ffUL;
	}
	if ((class_mask & (1<<0))==0) {
		class_code &= 0x00ffff00UL;
	}

	j=0;
	for (i=0; i<num_pci_devices; i++) {
		class_device = pci_devices[i].config[11]<<16;
		class_device |= pci_devices[i].config[10]<<8;
		class_device |= pci_devices[i].config[9];

		if ((class_mask & (1<<2))==0) {
			class_device &= 0x0000ffffUL;
		}
		if ((class_mask & (1<<1))==0) {
			class_device &= 0x00ff00ffUL;
		}
		if ((class_mask & (1<<0))==0) {
			class_device &= 0x00ffff00UL;
		}

		if (class_code != class_device) {
			continue;
		}

		if (j==index) {
			return i;
		}

		j++;
	}

	return ATARI_PCI_DEVICE_NOT_FOUND;
}

int32 PciDriverLinux::read_config_byte(uint32 device_handle, memptr data, uint32 num_register)
{
	unsigned char *buf;

	D(bug(NFPCI_NAME "read_config_byte(0x%08x,0x%08x,%d)", device_handle, data, num_register));

	if (device_handle>num_pci_devices)
		return ATARI_PCI_BAD_HANDLE;
	
	if (num_register>(uint32)(pci_devices[device_handle].config_length-1))
		return ATARI_PCI_BAD_REGISTER_NUMBER;

	buf = (unsigned char *) (Atari2HostAddr(data));
	*buf = pci_devices[device_handle].config[num_register];

	return ATARI_PCI_SUCCESSFUL;
}

int32 PciDriverLinux::read_config_word(uint32 device_handle, memptr data, uint32 num_register)
{
	unsigned short *buf;

	D(bug(NFPCI_NAME "read_config_word(0x%08x,0x%08x,%d)", device_handle, data, num_register));

	if (device_handle>num_pci_devices)
		return ATARI_PCI_BAD_HANDLE;
	
	if (num_register>(uint32)(pci_devices[device_handle].config_length-2))
		return ATARI_PCI_BAD_REGISTER_NUMBER;

	num_register &= -2;

	buf = (unsigned short *) (Atari2HostAddr(data));
	*buf = (pci_devices[device_handle].config[num_register]<<8) |
		pci_devices[device_handle].config[num_register+1];
	
	return ATARI_PCI_SUCCESSFUL;
}

int32 PciDriverLinux::read_config_long(uint32 device_handle, memptr data, uint32 num_register)
{
	unsigned long *buf;

	D(bug(NFPCI_NAME "read_config_long(0x%08x,0x%08x,%d)", device_handle, data, num_register));

	if (device_handle>num_pci_devices)
		return ATARI_PCI_BAD_HANDLE;
	
	if (num_register>pci_devices[device_handle].config_length-4)
		return ATARI_PCI_BAD_REGISTER_NUMBER;
	
	num_register &= -4;

	buf = (unsigned long *) (Atari2HostAddr(data));
	*buf = (pci_devices[device_handle].config[num_register]<<24) |
		(pci_devices[device_handle].config[num_register+1]<<16) |
		(pci_devices[device_handle].config[num_register+2]<<8) |
		pci_devices[device_handle].config[num_register+3];

	return ATARI_PCI_SUCCESSFUL;
}

int32 PciDriverLinux::read_config_byte_fast(uint32 device_handle, uint32 num_register)
{
	unsigned char reg_value;

	D(bug(NFPCI_NAME "read_config_byte_fast(0x%08x,%d)", device_handle, num_register));

	if ((device_handle>num_pci_devices) || (num_register>(pci_devices[device_handle].config_length-1))) {
		return 0;
	}

	reg_value = pci_devices[device_handle].config[num_register];
	
	return (int32) reg_value;
}

int32 PciDriverLinux::read_config_word_fast(uint32 device_handle, uint32 num_register)
{
	unsigned short reg_value;

	D(bug(NFPCI_NAME "read_config_word_fast(0x%08x,%d)", device_handle, num_register));

	if ((device_handle>num_pci_devices) || (num_register>(pci_devices[device_handle].config_length-2))) {
		return 0;
	}

	num_register &= -2;

	reg_value = (pci_devices[device_handle].config[num_register]<<8) |
		pci_devices[device_handle].config[num_register+1];

	return (int32) reg_value;
}

int32 PciDriverLinux::read_config_long_fast(uint32 device_handle, uint32 num_register)
{
	unsigned long reg_value;

	D(bug(NFPCI_NAME "read_config_long_fast(0x%08x,%d)", device_handle, num_register));

	if ((device_handle>num_pci_devices) || (num_register>(pci_devices[device_handle].config_length-4))) {
		return 0;
	}
	
	num_register &= -4;

	reg_value = (pci_devices[device_handle].config[num_register]<<24) |
		(pci_devices[device_handle].config[num_register+1]<<16) |
		(pci_devices[device_handle].config[num_register+2]<<8) |
		pci_devices[device_handle].config[num_register+3];

	return (int32) reg_value;
}
