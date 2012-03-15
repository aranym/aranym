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

#include "sysdeps.h"
#include "cpu_emulation.h"
#include "parameters.h"
#include "nfpci.h"
#include "../../atari/nfpci/nfpci_nfapi.h"

#define DEBUG 0
#include "debug.h"

/*--- Defines ---*/

#define NFPCI_NAME	"nf:pci: "

/*--- Types ---*/

/*--- Public functions ---*/

int32 PciDriver::dispatch(uint32 fncode)
{
	int32 ret;

	D(bug(NFPCI_NAME "dispatch(%u)", fncode));
	ret = ATARI_PCI_GENERAL_ERROR;

	switch(fncode) {
		case GET_VERSION:
    		ret = ARANFPCI_NFAPI_VERSION;
			break;
		case NFPCI_FIND_DEVICE:
			ret = find_device(getParameter(0),getParameter(1));
			break;
		case NFPCI_FIND_CLASSCODE:
			ret = find_classcode(getParameter(0),getParameter(1));
			break;
		case NFPCI_READ_CONFIG_BYTE:
			ret = read_config_byte(getParameter(0),getParameter(1),getParameter(2));
			break;
		case NFPCI_READ_CONFIG_WORD:
			ret = read_config_word(getParameter(0),getParameter(1),getParameter(2));
			break;
		case NFPCI_READ_CONFIG_LONG:
			ret = read_config_long(getParameter(0),getParameter(1),getParameter(2));
			break;
		case NFPCI_READ_CONFIG_BYTE_FAST:
			ret = read_config_byte_fast(getParameter(0),getParameter(1));
			break;
		case NFPCI_READ_CONFIG_WORD_FAST:
			ret = read_config_word_fast(getParameter(0),getParameter(1));
			break;
		case NFPCI_READ_CONFIG_LONG_FAST:
			ret = read_config_long_fast(getParameter(0),getParameter(1));
			break;
		case NFPCI_WRITE_CONFIG_BYTE:
			ret = write_config_byte(getParameter(0),getParameter(1),getParameter(2));
			break;
		case NFPCI_WRITE_CONFIG_WORD:
			ret = write_config_word(getParameter(0),getParameter(1),getParameter(2));
			break;
		case NFPCI_WRITE_CONFIG_LONG:
			ret = write_config_long(getParameter(0),getParameter(1),getParameter(2));
			break;
		case NFPCI_HOOK_INTERRUPT:
			ret = hook_interrupt(getParameter(0),getParameter(1),getParameter(2));
			break;
		case NFPCI_UNHOOK_INTERRUPT:
			ret = unhook_interrupt(getParameter(0));
			break;
		case NFPCI_SPECIAL_CYCLE:
			ret = special_cycle(getParameter(0),getParameter(1));
			break;
		case NFPCI_GET_RESOURCE:
			ret = get_resource(getParameter(0));
			break;
		case NFPCI_GET_CARD_USED:
			ret = get_card_used(getParameter(0),getParameter(1));
			break;
		case NFPCI_SET_CARD_USED:
			ret = set_card_used(getParameter(0),getParameter(1));
			break;
		case NFPCI_READ_MEM_BYTE:
			ret = read_mem_byte(getParameter(0),getParameter(1),getParameter(2));
			break;
		case NFPCI_READ_MEM_WORD:
			ret = read_mem_word(getParameter(0),getParameter(1),getParameter(2));
			break;
		case NFPCI_READ_MEM_LONG:
			ret = read_mem_long(getParameter(0),getParameter(1),getParameter(2));
			break;
		case NFPCI_READ_MEM_BYTE_FAST:
			ret = read_mem_byte_fast(getParameter(0),getParameter(1));
			break;
		case NFPCI_READ_MEM_WORD_FAST:
			ret = read_mem_word_fast(getParameter(0),getParameter(1));
			break;
		case NFPCI_READ_MEM_LONG_FAST:
			ret = read_mem_long_fast(getParameter(0),getParameter(1));
			break;
		case NFPCI_WRITE_MEM_BYTE:
			ret = write_mem_byte(getParameter(0),getParameter(1),getParameter(2));
			break;
		case NFPCI_WRITE_MEM_WORD:
			ret = write_mem_word(getParameter(0),getParameter(1),getParameter(2));
			break;
		case NFPCI_WRITE_MEM_LONG:
			ret = write_mem_long(getParameter(0),getParameter(1),getParameter(2));
			break;
		case NFPCI_READ_IO_BYTE:
			ret = read_io_byte(getParameter(0),getParameter(1),getParameter(2));
			break;
		case NFPCI_READ_IO_WORD:
			ret = read_io_word(getParameter(0),getParameter(1),getParameter(2));
			break;
		case NFPCI_READ_IO_LONG:
			ret = read_io_long(getParameter(0),getParameter(1),getParameter(2));
			break;
		case NFPCI_READ_IO_BYTE_FAST:
			ret = read_io_byte_fast(getParameter(0),getParameter(1));
			break;
		case NFPCI_READ_IO_WORD_FAST:
			ret = read_io_word_fast(getParameter(0),getParameter(1));
			break;
		case NFPCI_READ_IO_LONG_FAST:
			ret = read_io_long_fast(getParameter(0),getParameter(1));
			break;
		case NFPCI_WRITE_IO_BYTE:
			ret = write_io_byte(getParameter(0),getParameter(1),getParameter(2));
			break;
		case NFPCI_WRITE_IO_WORD:
			ret = write_io_word(getParameter(0),getParameter(1),getParameter(2));
			break;
		case NFPCI_WRITE_IO_LONG:
			ret = write_io_long(getParameter(0),getParameter(1),getParameter(2));
			break;
		case NFPCI_GET_MACHINE_ID:
			ret = get_machine_id();
			break;
		case NFPCI_GET_PAGESIZE:
			ret = get_pagesize();
			break;
		case NFPCI_VIRT_TO_BUS:
			ret = virt_to_bus(getParameter(0),getParameter(1),getParameter(2));
			break;
		case NFPCI_BUS_TO_VIRT:
			ret = bus_to_virt(getParameter(0),getParameter(1),getParameter(2));
			break;
		case NFPCI_VIRT_TO_PHYS:
			ret = virt_to_phys(getParameter(0),getParameter(1));
			break;
		case NFPCI_PHYS_TO_VIRT:
			ret = phys_to_virt(getParameter(0),getParameter(1));
			break;
		default:
			D(bug(NFPCI_NAME " unimplemented function #%d", fncode));
			break;
	}
	D(bug(NFPCI_NAME " function returning with 0x%08x", ret));
	return ret;
}

int32 PciDriver::find_device(uint32 /*device_vendor_id*/, uint32 /*index*/)
{
	return ATARI_PCI_FUNC_NOT_SUPPORTED;
}

int32 PciDriver::find_classcode(uint32 /*class_code*/, uint32 /*index*/)
{
	return ATARI_PCI_FUNC_NOT_SUPPORTED;
}

int32 PciDriver::read_config_byte(uint32 /*device_handle*/, memptr /*data*/, uint32 /*num_register*/)
{
	return ATARI_PCI_FUNC_NOT_SUPPORTED;
}

int32 PciDriver::read_config_word(uint32 /*device_handle*/, memptr /*data*/, uint32 /*num_register*/)
{
	return ATARI_PCI_FUNC_NOT_SUPPORTED;
}

int32 PciDriver::read_config_long(uint32 /*device_handle*/, memptr /*data*/, uint32 /*num_register*/)
{
	return ATARI_PCI_FUNC_NOT_SUPPORTED;
}

int32 PciDriver::read_config_byte_fast(uint32 /*device_handle*/, uint32 /*num_register*/)
{
	return 0xffffffffUL;
}

int32 PciDriver::read_config_word_fast(uint32 /*device_handle*/, uint32 /*num_register*/)
{
	return 0xffffffffUL;
}

int32 PciDriver::read_config_long_fast(uint32 /*device_handle*/, uint32 /*num_register*/)
{
	return 0xffffffffUL;
}

int32 PciDriver::write_config_byte(uint32 /*device_handle*/, uint32 /*num_register*/, uint32 /*value*/)
{
	return ATARI_PCI_FUNC_NOT_SUPPORTED;
}

int32 PciDriver::write_config_word(uint32 /*device_handle*/, uint32 /*num_register*/, uint32 /*value*/)
{
	return ATARI_PCI_FUNC_NOT_SUPPORTED;
}

int32 PciDriver::write_config_long(uint32 /*device_handle*/, uint32 /*num_register*/, uint32 /*value*/)
{
	return ATARI_PCI_FUNC_NOT_SUPPORTED;
}

int32 PciDriver::hook_interrupt(uint32 /*device_handle*/, memptr /*data*/, uint32 /*parameter*/)
{
	return ATARI_PCI_FUNC_NOT_SUPPORTED;
}

int32 PciDriver::unhook_interrupt(uint32 /*device_handle*/)
{
	return ATARI_PCI_FUNC_NOT_SUPPORTED;
} 

int32 PciDriver::special_cycle(uint32 /*num_bus*/, uint32 /*data*/)
{
	return ATARI_PCI_FUNC_NOT_SUPPORTED;
} 

/* get_routing */

/* set_interrupt */	

int32 PciDriver::get_resource(uint32 /*device_handle*/)
{
	return ATARI_PCI_FUNC_NOT_SUPPORTED;
}

int32 PciDriver::get_card_used(uint32 /*device_handle*/, memptr /*callback*/)
{
	return ATARI_PCI_FUNC_NOT_SUPPORTED;
}

int32 PciDriver::set_card_used(uint32 /*device_handle*/, memptr /*callback*/)
{
	return ATARI_PCI_FUNC_NOT_SUPPORTED;
}

int32 PciDriver::read_mem_byte(uint32 /*device_handle*/, uint32 /*pci_address*/, uint32 /*num_register*/)
{
	return ATARI_PCI_FUNC_NOT_SUPPORTED;
}

int32 PciDriver::read_mem_word(uint32 /*device_handle*/, uint32 /*pci_address*/, uint32 /*num_register*/)
{
	return ATARI_PCI_FUNC_NOT_SUPPORTED;
}

int32 PciDriver::read_mem_long(uint32 /*device_handle*/, uint32 /*pci_address*/, uint32 /*num_register*/)
{
	return ATARI_PCI_FUNC_NOT_SUPPORTED;
}

int32 PciDriver::read_mem_byte_fast(uint32 /*device_handle*/, uint32 /*pci_address*/)
{
	return 0xffffffffL;
}

int32 PciDriver::read_mem_word_fast(uint32 /*device_handle*/, uint32 /*pci_address*/)
{
	return 0xffffffffL;
}

int32 PciDriver::read_mem_long_fast(uint32 /*device_handle*/, uint32 /*pci_address*/)
{
	return 0xffffffffL;
}

int32 PciDriver::write_mem_byte(uint32 /*device_handle*/, uint32 /*pci_address*/, uint32 /*value*/)
{
	return ATARI_PCI_FUNC_NOT_SUPPORTED;
}

int32 PciDriver::write_mem_word(uint32 /*device_handle*/, uint32 /*pci_address*/, uint32 /*value*/)
{
	return ATARI_PCI_FUNC_NOT_SUPPORTED;
}

int32 PciDriver::write_mem_long(uint32 /*device_handle*/, uint32 /*pci_address*/, uint32 /*value*/)
{
	return ATARI_PCI_FUNC_NOT_SUPPORTED;
}

int32 PciDriver::read_io_byte(uint32 /*device_handle*/, uint32 /*pci_address*/, memptr /*data*/)
{
	return ATARI_PCI_FUNC_NOT_SUPPORTED;
}

int32 PciDriver::read_io_word(uint32 /*device_handle*/, uint32 /*pci_address*/, memptr /*data*/)
{
	return ATARI_PCI_FUNC_NOT_SUPPORTED;
}

int32 PciDriver::read_io_long(uint32 /*device_handle*/, uint32 /*pci_address*/, memptr /*data*/)
{
	return ATARI_PCI_FUNC_NOT_SUPPORTED;
}

int32 PciDriver::read_io_byte_fast(uint32 /*device_handle*/, uint32 /*pci_address*/)
{
	return 0xffffffffL;
}

int32 PciDriver::read_io_word_fast(uint32 /*device_handle*/, uint32 /*pci_address*/)
{
	return 0xffffffffL;
}

int32 PciDriver::read_io_long_fast(uint32 /*device_handle*/, uint32 /*pci_address*/)
{
	return 0xffffffffL;
}

int32 PciDriver::write_io_byte(uint32 /*device_handle*/, uint32 /*pci_address*/, uint32 /*value*/)
{
	return ATARI_PCI_FUNC_NOT_SUPPORTED;
}

int32 PciDriver::write_io_word(uint32 /*device_handle*/, uint32 /*pci_address*/, uint32 /*value*/)
{
	return ATARI_PCI_FUNC_NOT_SUPPORTED;
}

int32 PciDriver::write_io_long(uint32 /*device_handle*/, uint32 /*pci_address*/, uint32 /*value*/)
{
	return ATARI_PCI_FUNC_NOT_SUPPORTED;
}

int32 PciDriver::get_machine_id(void)
{
	return ARANYM_PCI_ID;
}

int32 PciDriver::get_pagesize(void)
{
	return ATARI_PCI_FUNC_NOT_SUPPORTED;
}

int32 PciDriver::virt_to_bus(uint32 /*device_handle*/, memptr /*virt_cpu_address*/, memptr /*data*/)
{
	return ATARI_PCI_FUNC_NOT_SUPPORTED;
}

int32 PciDriver::bus_to_virt(uint32 /*device_handle*/, uint32 /*pci_address*/, memptr /*data*/)
{
	return ATARI_PCI_FUNC_NOT_SUPPORTED;
}

int32 PciDriver::virt_to_phys(memptr /*virt_cpu_address*/, memptr /*data*/)
{
	return ATARI_PCI_FUNC_NOT_SUPPORTED;
}

int32 PciDriver::phys_to_virt(memptr /*phys_cpu_address*/, memptr /*data*/)
{
	return ATARI_PCI_FUNC_NOT_SUPPORTED;
}
