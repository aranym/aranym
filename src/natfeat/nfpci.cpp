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

char *PciDriver::name()
{
	return "PCI";
}

bool PciDriver::isSuperOnly()
{
	return true;
}

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
