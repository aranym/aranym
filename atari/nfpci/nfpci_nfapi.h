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

#ifndef NFPCI_NFAPI_H
#define NFPCI_NFAPI_H

/* if you change anything in the enum {} below you have to increase 
   this ARANFPCI_NFAPI_VERSION!
*/
#define ARANFPCI_NFAPI_VERSION	0x00000000

enum {
	GET_VERSION = 0,	/* no parameters, return NFAPI_VERSION in d0 */
	NFPCI_FIND_DEVICE,
	NFPCI_FIND_CLASSCODE,
	NFPCI_READ_CONFIG_BYTE,
	NFPCI_READ_CONFIG_WORD,
	NFPCI_READ_CONFIG_LONG,
	NFPCI_READ_CONFIG_BYTE_FAST,
	NFPCI_READ_CONFIG_WORD_FAST,
	NFPCI_READ_CONFIG_LONG_FAST,
	NFPCI_WRITE_CONFIG_BYTE,
	NFPCI_WRITE_CONFIG_WORD,
	NFPCI_WRITE_CONFIG_LONG,
	NFPCI_HOOK_INTERRUPT,
	NFPCI_UNHOOK_INTERRUPT,
	NFPCI_SPECIAL_CYCLE,
	NFPCI_GET_RESOURCE,
	NFPCI_GET_CARD_USED,
	NFPCI_SET_CARD_USED,
	NFPCI_READ_MEM_BYTE,
	NFPCI_READ_MEM_WORD,
	NFPCI_READ_MEM_LONG,
	NFPCI_READ_MEM_BYTE_FAST,
	NFPCI_READ_MEM_WORD_FAST,
	NFPCI_READ_MEM_LONG_FAST,
	NFPCI_WRITE_MEM_BYTE,
	NFPCI_WRITE_MEM_WORD,
	NFPCI_WRITE_MEM_LONG,
	NFPCI_READ_IO_BYTE,
	NFPCI_READ_IO_WORD,
	NFPCI_READ_IO_LONG,
	NFPCI_READ_IO_BYTE_FAST,
	NFPCI_READ_IO_WORD_FAST,
	NFPCI_READ_IO_LONG_FAST,
	NFPCI_WRITE_IO_BYTE,
	NFPCI_WRITE_IO_WORD,
	NFPCI_WRITE_IO_LONG,
	NFPCI_GET_MACHINE_ID,
	NFPCI_GET_PAGESIZE,
	NFPCI_VIRT_TO_BUS,
	NFPCI_BUS_TO_VIRT,
	NFPCI_VIRT_TO_PHYS,
	NFPCI_PHYS_TO_VIRT
};

#define NFPCI(a)	(nfPciId + a)

#endif /* NFCDROM_NFAPI_H */
