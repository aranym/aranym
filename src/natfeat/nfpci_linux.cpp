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

/*--- Types ---*/

/*--- Public functions ---*/

PciDriverLinux::PciDriverLinux()
{
}

PciDriverLinux::~PciDriverLinux()
{
}

/*--- Private functions ---*/

int32 PciDriverLinux::find_device(uint32 device_vendor_id, uint32 index)
{
	return ATARI_PCI_GENERAL_ERROR;
}

int32 PciDriverLinux::find_classcode(uint32 class_code, uint32 index)
{
	return ATARI_PCI_GENERAL_ERROR;
}

int32 PciDriverLinux::read_config_byte(uint32 device_handle, memptr data, uint32 num_register)
{
	return ATARI_PCI_GENERAL_ERROR;
}

int32 PciDriverLinux::read_config_word(uint32 device_handle, memptr data, uint32 num_register)
{
	return ATARI_PCI_GENERAL_ERROR;
}

int32 PciDriverLinux::read_config_long(uint32 device_handle, memptr data, uint32 num_register)
{
	return ATARI_PCI_GENERAL_ERROR;
}

int32 PciDriverLinux::read_config_byte_fast(uint32 device_handle, uint32 num_register)
{
	return ATARI_PCI_GENERAL_ERROR;
}

int32 PciDriverLinux::read_config_word_fast(uint32 device_handle, uint32 num_register)
{
	return ATARI_PCI_GENERAL_ERROR;
}

int32 PciDriverLinux::read_config_long_fast(uint32 device_handle, uint32 num_register)
{
	return ATARI_PCI_GENERAL_ERROR;
}

int32 PciDriverLinux::write_config_byte(uint32 device_handle, uint32 num_register, uint32 value)
{
	return ATARI_PCI_GENERAL_ERROR;
}

int32 PciDriverLinux::write_config_word(uint32 device_handle, uint32 num_register, uint32 value)
{
	return ATARI_PCI_GENERAL_ERROR;
}

int32 PciDriverLinux::write_config_long(uint32 device_handle, uint32 num_register, uint32 value)
{
	return ATARI_PCI_GENERAL_ERROR;
}

int32 PciDriverLinux::hook_interrupt(uint32 device_handle, memptr data, uint32 parameter)
{
	return ATARI_PCI_GENERAL_ERROR;
}

int32 PciDriverLinux::unhook_interrupt(uint32 device_handle)
{
	return ATARI_PCI_GENERAL_ERROR;
} 

int32 PciDriverLinux::special_cycle(uint32 num_bus, uint32 data)
{
	return ATARI_PCI_GENERAL_ERROR;
} 

/* get_routing */

/* set_interrupt */	

int32 PciDriverLinux::get_resource(uint32 device_handle)
{
	return ATARI_PCI_GENERAL_ERROR;
}

int32 PciDriverLinux::get_card_used(uint32 device_handle, memptr callback)
{
	return ATARI_PCI_GENERAL_ERROR;
}

int32 PciDriverLinux::set_card_used(uint32 device_handle, memptr callback)
{
	return ATARI_PCI_GENERAL_ERROR;
}

int32 PciDriverLinux::read_mem_byte(uint32 device_handle, uint32 pci_address, uint32 num_register)
{
	return ATARI_PCI_GENERAL_ERROR;
}

int32 PciDriverLinux::read_mem_word(uint32 device_handle, uint32 pci_address, uint32 num_register)
{
	return ATARI_PCI_GENERAL_ERROR;
}

int32 PciDriverLinux::read_mem_long(uint32 device_handle, uint32 pci_address, uint32 num_register)
{
	return ATARI_PCI_GENERAL_ERROR;
}

int32 PciDriverLinux::read_mem_byte_fast(uint32 device_handle, uint32 pci_address)
{
	return ATARI_PCI_GENERAL_ERROR;
}

int32 PciDriverLinux::read_mem_word_fast(uint32 device_handle, uint32 pci_address)
{
	return ATARI_PCI_GENERAL_ERROR;
}

int32 PciDriverLinux::read_mem_long_fast(uint32 device_handle, uint32 pci_address)
{
	return ATARI_PCI_GENERAL_ERROR;
}

int32 PciDriverLinux::write_mem_byte(uint32 device_handle, uint32 pci_address, uint32 value)
{
	return ATARI_PCI_GENERAL_ERROR;
}

int32 PciDriverLinux::write_mem_word(uint32 device_handle, uint32 pci_address, uint32 value)
{
	return ATARI_PCI_GENERAL_ERROR;
}

int32 PciDriverLinux::write_mem_long(uint32 device_handle, uint32 pci_address, uint32 value)
{
	return ATARI_PCI_GENERAL_ERROR;
}

int32 PciDriverLinux::read_io_byte(uint32 device_handle, uint32 pci_address, memptr data)
{
	return ATARI_PCI_GENERAL_ERROR;
}

int32 PciDriverLinux::read_io_word(uint32 device_handle, uint32 pci_address, memptr data)
{
	return ATARI_PCI_GENERAL_ERROR;
}

int32 PciDriverLinux::read_io_long(uint32 device_handle, uint32 pci_address, memptr data)
{
	return ATARI_PCI_GENERAL_ERROR;
}

int32 PciDriverLinux::read_io_byte_fast(uint32 device_handle, uint32 pci_address)
{
	return ATARI_PCI_GENERAL_ERROR;
}

int32 PciDriverLinux::read_io_word_fast(uint32 device_handle, uint32 pci_address)
{
	return ATARI_PCI_GENERAL_ERROR;
}

int32 PciDriverLinux::read_io_long_fast(uint32 device_handle, uint32 pci_address)
{
	return ATARI_PCI_GENERAL_ERROR;
}

int32 PciDriverLinux::write_io_byte(uint32 device_handle, uint32 pci_address, uint32 value)
{
	return ATARI_PCI_GENERAL_ERROR;
}

int32 PciDriverLinux::write_io_word(uint32 device_handle, uint32 pci_address, uint32 value)
{
	return ATARI_PCI_GENERAL_ERROR;
}

int32 PciDriverLinux::write_io_long(uint32 device_handle, uint32 pci_address, uint32 value)
{
	return ATARI_PCI_GENERAL_ERROR;
}

int32 PciDriverLinux::get_machine_id(void)
{
	return ATARI_PCI_GENERAL_ERROR;
}

int32 PciDriverLinux::get_pagesize(void)
{
	return ATARI_PCI_GENERAL_ERROR;
}

int32 PciDriverLinux::virt_to_bus(uint32 device_handle, memptr virt_cpu_address, memptr data)
{
	return ATARI_PCI_GENERAL_ERROR;
}

int32 PciDriverLinux::bus_to_virt(uint32 device_handle, uint32 pci_address, memptr data)
{
	return ATARI_PCI_GENERAL_ERROR;
}

int32 PciDriverLinux::virt_to_phys(memptr virt_cpu_address, memptr data)
{
	return ATARI_PCI_GENERAL_ERROR;
}

int32 PciDriverLinux::phys_to_virt(memptr phys_cpu_address, memptr data)
{
	return ATARI_PCI_GENERAL_ERROR;
}
