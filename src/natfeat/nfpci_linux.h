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

#ifndef NFPCI_LINUX_H
#define NFPCI_LINUX_H

#include "parameters.h"

/*--- Types ---*/

typedef struct {
	/* Infos read from PCI_FILE_DEVICES */
	/* 0: bus, device, function */
	/* 1: vendor id, device id */
	/* 2: irq */
	/* 3-8: base addresses */
	/* 9: ROM base address */
	/* 10-15: sizes */
	/* 16: ROM size */
	uint32 info[17];

	/* Configuration bytes */
	unsigned char config[256];
	uint32 config_length;

	/* Device file to access config bytes */
	char filename[256];
} pci_device_t;

/*--- Linux PCI driver class ---*/

class PciDriverLinux : public PciDriver
{
	private:
		int32 find_device(uint32 device_vendor_id, uint32 index);
		int32 find_classcode(uint32 class_code, uint32 index);
		int32 read_config_byte(uint32 device_handle, memptr data, uint32 num_register);
		int32 read_config_word(uint32 device_handle, memptr data, uint32 num_register);
		int32 read_config_long(uint32 device_handle, memptr data, uint32 num_register);
		int32 read_config_byte_fast(uint32 device_handle, uint32 num_register);
		int32 read_config_word_fast(uint32 device_handle, uint32 num_register);
		int32 read_config_long_fast(uint32 device_handle, uint32 num_register);

		pci_device_t *pci_devices;
		uint32 num_pci_devices;

	public:
		PciDriverLinux();
		virtual ~PciDriverLinux();
};

#endif /* NFPCI_LINUX_H */
