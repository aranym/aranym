/*
	NatFeat host PCI access

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

#ifndef NFPCI_H
#define NFPCI_H

/*--- Includes ---*/

#include "nf_base.h"
#include "parameters.h"

/*--- Defines ---*/

/* Error codes for PCI BIOS */
#define ATARI_PCI_SUCCESSFUL			0x00000000L
#define ATARI_PCI_FUNC_NOT_SUPPORTED	0xfffffffeL
#define ATARI_PCI_BAD_VENDOR_ID			0xfffffffdL
#define ATARI_PCI_DEVICE_NOT_FOUND		0xfffffffcL
#define ATARI_PCI_BAD_REGISTER_NUMBER	0xfffffffbL
#define ATARI_PCI_SET_FAILED			0xfffffffaL
#define ATARI_PCI_BUFFER_TOO_SMALL		0xfffffff9L
#define ATARI_PCI_GENERAL_ERROR			0xfffffff8L
#define ATARI_PCI_BAD_HANDLE			0xfffffff7L

#define ARANYM_PCI_ID	((3<<24)|1)

/*--- Types ---*/

/*--- Class ---*/

class PciDriver : public NF_Base
{
	private:
		int32 get_machine_id(void);

	protected:
		virtual int32 find_device(uint32 device_vendor_id, uint32 index);
		virtual int32 find_classcode(uint32 class_code, uint32 index);
		virtual	int32 read_config_byte(uint32 device_handle, memptr data, uint32 num_register);
		virtual	int32 read_config_word(uint32 device_handle, memptr data, uint32 num_register);
		virtual	int32 read_config_long(uint32 device_handle, memptr data, uint32 num_register);
		virtual	int32 read_config_byte_fast(uint32 device_handle, uint32 num_register);
		virtual	int32 read_config_word_fast(uint32 device_handle, uint32 num_register);
		virtual	int32 read_config_long_fast(uint32 device_handle, uint32 num_register);
		virtual	int32 write_config_byte(uint32 device_handle, uint32 num_register, uint32 value);
		virtual	int32 write_config_word(uint32 device_handle, uint32 num_register, uint32 value);
		virtual	int32 write_config_long(uint32 device_handle, uint32 num_register, uint32 value);
		virtual	int32 hook_interrupt(uint32 device_handle, memptr data, uint32 parameter);
		virtual	int32 unhook_interrupt(uint32 device_handle); 
		virtual	int32 special_cycle(uint32 num_bus, uint32 data); 
/* get_routing */
/* set_interrupt */	
		virtual	int32 get_resource(uint32 device_handle);
		virtual	int32 get_card_used(uint32 device_handle, memptr callback);
		virtual	int32 set_card_used(uint32 device_handle, memptr callback);
		virtual	int32 read_mem_byte(uint32 device_handle, uint32 pci_address, uint32 num_register);
		virtual	int32 read_mem_word(uint32 device_handle, uint32 pci_address, uint32 num_register);
		virtual	int32 read_mem_long(uint32 device_handle, uint32 pci_address, uint32 num_register);
		virtual	int32 read_mem_byte_fast(uint32 device_handle, uint32 pci_address);
		virtual	int32 read_mem_word_fast(uint32 device_handle, uint32 pci_address);
		virtual	int32 read_mem_long_fast(uint32 device_handle, uint32 pci_address);
		virtual	int32 write_mem_byte(uint32 device_handle, uint32 pci_address, uint32 value);
		virtual	int32 write_mem_word(uint32 device_handle, uint32 pci_address, uint32 value);
		virtual	int32 write_mem_long(uint32 device_handle, uint32 pci_address, uint32 value);
		virtual	int32 read_io_byte(uint32 device_handle, uint32 pci_address, memptr data);
		virtual	int32 read_io_word(uint32 device_handle, uint32 pci_address, memptr data);
		virtual	int32 read_io_long(uint32 device_handle, uint32 pci_address, memptr data);
		virtual	int32 read_io_byte_fast(uint32 device_handle, uint32 pci_address);
		virtual	int32 read_io_word_fast(uint32 device_handle, uint32 pci_address);
		virtual	int32 read_io_long_fast(uint32 device_handle, uint32 pci_address);
		virtual	int32 write_io_byte(uint32 device_handle, uint32 pci_address, uint32 value);
		virtual	int32 write_io_word(uint32 device_handle, uint32 pci_address, uint32 value);
		virtual	int32 write_io_long(uint32 device_handle, uint32 pci_address, uint32 value);
		virtual int32 get_pagesize(void);
		virtual int32 virt_to_bus(uint32 device_handle, memptr virt_cpu_address, memptr data);
		virtual int32 bus_to_virt(uint32 device_handle, uint32 pci_address, memptr data);
		virtual int32 virt_to_phys(memptr virt_cpu_address, memptr data);
		virtual int32 phys_to_virt(memptr phys_cpu_address, memptr data);

	public:
		const char *name() { return "PCI"; }
		bool isSuperOnly() { return false; }
		int32 dispatch(uint32 fncode);
		virtual ~PciDriver() { };
};

#endif /* NFPCI_H */
