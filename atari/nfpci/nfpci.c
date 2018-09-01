/*
	NatFeat host PCI driver

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

/*--- Include ---*/

#include <stdlib.h>
#include <string.h>

#include <mint/cookie.h>
#include <mint/osbind.h>

#include "../natfeat/nf_ops.h"
#include "nfpci_nfapi.h"
#include "nfpci.h"
#include "nfpci_bios.h"
#include "nfpci_cookie.h"

/*--- Defines ---*/

#ifndef C_XPCI
#define C_XPCI	0x58504349L
#endif

#ifndef C__PCI
#define C__PCI	0x5f504349L
#endif

#ifndef VEC_XBIOS
#define VEC_XBIOS	0x2e
#endif

#ifndef DEV_CONSOLE
#define DEV_CONSOLE	2
#endif

#define DRIVER_NAME	"ARAnyM host PCI driver"
#define VERSION	"v0.4"

/*--- Types ---*/

typedef void (*pcibios_routine_t)(void);

typedef struct {
	unsigned long *subcookie;
	unsigned long version;
	pcibios_routine_t routines[43];
} __attribute__((packed)) pcibios_cookie_t;

/*--- Local variables ---*/

static unsigned long nfPciId;

static pcibios_cookie_t pcibios_cookie={
	0, 0x00010000, {
	(pcibios_routine_t) pcibios_find_device,
	(pcibios_routine_t) pcibios_find_classcode,
	(pcibios_routine_t) pcibios_read_config_byte,
	(pcibios_routine_t) pcibios_read_config_word,
	(pcibios_routine_t) pcibios_read_config_long,
	(pcibios_routine_t) pcibios_read_config_byte_fast,
	(pcibios_routine_t) pcibios_read_config_word_fast,
	(pcibios_routine_t) pcibios_read_config_long_fast,
	(pcibios_routine_t) pcibios_write_config_byte,
	(pcibios_routine_t) pcibios_write_config_word,
	(pcibios_routine_t) pcibios_write_config_long,
	(pcibios_routine_t) pcibios_hook_interrupt,
	(pcibios_routine_t) pcibios_unhook_interrupt,
	(pcibios_routine_t) pcibios_special_cycle,
	(pcibios_routine_t) pcibios_get_routing,	/* unimplemented */
	(pcibios_routine_t) pcibios_set_interrupt,	/* unimplemented */
	(pcibios_routine_t) pcibios_get_resource,
	(pcibios_routine_t) pcibios_get_card_used,
	(pcibios_routine_t) pcibios_set_card_used,
	(pcibios_routine_t) pcibios_read_mem_byte,
	(pcibios_routine_t) pcibios_read_mem_word,
	(pcibios_routine_t) pcibios_read_mem_long,
	(pcibios_routine_t) pcibios_read_mem_byte_fast,
	(pcibios_routine_t) pcibios_read_mem_word_fast,
	(pcibios_routine_t) pcibios_read_mem_long_fast,
	(pcibios_routine_t) pcibios_write_mem_byte,
	(pcibios_routine_t) pcibios_write_mem_word,
	(pcibios_routine_t) pcibios_write_mem_long,
	(pcibios_routine_t) pcibios_read_io_byte,
	(pcibios_routine_t) pcibios_read_io_word,
	(pcibios_routine_t) pcibios_read_io_long,
	(pcibios_routine_t) pcibios_read_io_byte_fast,
	(pcibios_routine_t) pcibios_read_io_word_fast,
	(pcibios_routine_t) pcibios_read_io_long_fast,
	(pcibios_routine_t) pcibios_write_io_byte,
	(pcibios_routine_t) pcibios_write_io_word,
	(pcibios_routine_t) pcibios_write_io_long,
	(pcibios_routine_t) pcibios_get_machine_id,
	(pcibios_routine_t) pcibios_get_pagesize,
	(pcibios_routine_t) pcibios_virt_to_bus,
	(pcibios_routine_t) pcibios_bus_to_virt,
	(pcibios_routine_t) pcibios_virt_to_phys,
	(pcibios_routine_t) pcibios_phys_to_virt
	}
};

/*--- External variables ---*/

extern void pcixbios_newtrap(void);
extern void (*pcixbios_oldtrap)(void);

/*--- Functions prototypes ---*/

static const struct nf_ops *nfOps;
static void install_pci_bios(void);
static void install_pci_xbios(void);
static void press_any_key(void);

/*--- Functions ---*/

void install_driver(unsigned long resident_length)
{
	Cconws(
		"\033p " DRIVER_NAME " " VERSION " \033q\r\n"
		"Copyright (c) ARAnyM Development Team, " __DATE__ "\r\n"
	);

	/* Check if a PCI driver is not already installed */
	if (cookie_present(C_XPCI, NULL)) {
		Cconws("A XPCI driver is already installed on this system\r\n");
		press_any_key();
		return;
	}	

	if (cookie_present(C__PCI, NULL)) {
		Cconws("A _PCI driver is already installed on this system\r\n");
		press_any_key();
		return;
	}	

	/* Check if NF is present for PCI */
	nfOps = nf_init();
	if (!nfOps) {
		Cconws("Native Features not present on this system\r\n");
		press_any_key();
		return;
	}	

	nfPciId = nfOps->get_id("PCI");
	if (nfPciId==0) {
		Cconws("NF PCI functions not present on this system\r\n");
		press_any_key();
		return;
	}

	/* Cookie with structure that holds function pointers */
	install_pci_bios();		
	
	/* XBIOS functions */
	install_pci_xbios();

	Ptermres(resident_length, 0);
	for(;;);	/* Never ending loop, should not go there */
}

static void press_any_key(void)
{
	Cconws("- Press any key to continue -\r\n");
	Crawcin();
}

static void install_pci_bios(void)
{
	/* Add our cookie */
	cookie_add(C__PCI, (unsigned long)&pcibios_cookie);
}

static void install_pci_xbios(void)
{
	/* Read old XBIOS trap handler, and install ours */
	pcixbios_oldtrap = Setexc(VEC_XBIOS, pcixbios_newtrap);

	cookie_add(C_XPCI, 0);
}

/* NF PCI functions */

long find_device(unsigned long device_vendor_id, unsigned long index)
{
	return nfOps->call(NFPCI(NFPCI_FIND_DEVICE), device_vendor_id, index);
}

long find_classcode(unsigned long class_code, unsigned long index)
{
	return nfOps->call(NFPCI(NFPCI_FIND_CLASSCODE), class_code, index);
}

long read_config_byte(unsigned long device_handle, void *data, unsigned long num_register)
{
	return nfOps->call(NFPCI(NFPCI_READ_CONFIG_BYTE), device_handle, data, num_register);
}

long read_config_word(unsigned long device_handle, void *data, unsigned long num_register)
{
	return nfOps->call(NFPCI(NFPCI_READ_CONFIG_WORD), device_handle, data, num_register);
}

long read_config_long(unsigned long device_handle, void *data, unsigned long num_register)
{
	return nfOps->call(NFPCI(NFPCI_READ_CONFIG_LONG), device_handle, data, num_register);
}

unsigned char read_config_byte_fast(unsigned long device_handle, unsigned long num_register)
{
	return nfOps->call(NFPCI(NFPCI_READ_CONFIG_BYTE_FAST), device_handle, num_register);
}

unsigned short read_config_word_fast(unsigned long device_handle, unsigned long num_register)
{
	return nfOps->call(NFPCI(NFPCI_READ_CONFIG_WORD_FAST), device_handle, num_register);
}

unsigned long read_config_long_fast(unsigned long device_handle, unsigned long num_register)
{
	return nfOps->call(NFPCI(NFPCI_READ_CONFIG_LONG_FAST), device_handle, num_register);
}

long write_config_byte(unsigned long device_handle, unsigned long num_register, unsigned long value)
{
	return nfOps->call(NFPCI(NFPCI_WRITE_CONFIG_BYTE), device_handle, num_register, value);
}

long write_config_word(unsigned long device_handle, unsigned long num_register, unsigned long value)
{
	return nfOps->call(NFPCI(NFPCI_WRITE_CONFIG_WORD), device_handle, num_register, value);
}

long write_config_long(unsigned long device_handle, unsigned long num_register, unsigned long value)
{
	return nfOps->call(NFPCI(NFPCI_WRITE_CONFIG_LONG), device_handle, num_register, value);
}

long hook_interrupt(unsigned long device_handle, void (*data)(), unsigned long parameter)
{
	return nfOps->call(NFPCI(NFPCI_HOOK_INTERRUPT), device_handle, data, parameter);
}
 
long unhook_interrupt(unsigned long device_handle)
{
	return nfOps->call(NFPCI(NFPCI_UNHOOK_INTERRUPT), device_handle);
}
 
long special_cycle(unsigned long num_bus, unsigned long data)
{
	return nfOps->call(NFPCI(NFPCI_SPECIAL_CYCLE), num_bus, data);
}
 
/* get_routing */

/* set_interrupt */

long get_resource(unsigned long device_handle)
{
	return nfOps->call(NFPCI(NFPCI_GET_RESOURCE), device_handle);
}
 
long get_card_used(unsigned long device_handle, unsigned long *callback)
{
	return nfOps->call(NFPCI(NFPCI_GET_CARD_USED), device_handle, callback);
}

long set_card_used(unsigned long device_handle, unsigned long callback)
{
	return nfOps->call(NFPCI(NFPCI_SET_CARD_USED), device_handle, callback);
}

long read_mem_byte(unsigned long device_handle, unsigned long pci_address, unsigned char *data)
{
	return nfOps->call(NFPCI(NFPCI_READ_MEM_BYTE), device_handle, pci_address, data);
}

long read_mem_word(unsigned long device_handle, unsigned long pci_address, unsigned short *data)
{
	return nfOps->call(NFPCI(NFPCI_READ_MEM_WORD), device_handle, pci_address, data);
}

long read_mem_long(unsigned long device_handle, unsigned long pci_address, unsigned long *data)
{
	return nfOps->call(NFPCI(NFPCI_READ_MEM_LONG), device_handle, pci_address, data);
}

unsigned char read_mem_byte_fast(unsigned long device_handle, unsigned long pci_address)
{
	return nfOps->call(NFPCI(NFPCI_READ_MEM_BYTE_FAST), device_handle, pci_address);
}

unsigned short read_mem_word_fast(unsigned long device_handle, unsigned long pci_address)
{
	return nfOps->call(NFPCI(NFPCI_READ_MEM_WORD_FAST), device_handle, pci_address);
}

unsigned long read_mem_long_fast(unsigned long device_handle, unsigned long pci_address)
{
	return nfOps->call(NFPCI(NFPCI_READ_MEM_LONG_FAST), device_handle, pci_address);
}

long write_mem_byte(unsigned long device_handle, unsigned long pci_address, unsigned long value)
{
	return nfOps->call(NFPCI(NFPCI_WRITE_MEM_BYTE), device_handle, pci_address, value);
}

long write_mem_word(unsigned long device_handle, unsigned long pci_address, unsigned long value)
{
	return nfOps->call(NFPCI(NFPCI_WRITE_MEM_WORD), device_handle, pci_address, value);
}

long write_mem_long(unsigned long device_handle, unsigned long pci_address, unsigned long value)
{
	return nfOps->call(NFPCI(NFPCI_WRITE_MEM_LONG), device_handle, pci_address, value);
}

long read_io_byte(unsigned long device_handle, unsigned long pci_address, unsigned char *data)
{
	return nfOps->call(NFPCI(NFPCI_READ_IO_BYTE), device_handle, pci_address, data);
}

long read_io_word(unsigned long device_handle, unsigned long pci_address, unsigned short *data)
{
	return nfOps->call(NFPCI(NFPCI_READ_IO_WORD), device_handle, pci_address, data);
}

long read_io_long(unsigned long device_handle, unsigned long pci_address, unsigned long *data)
{
	return nfOps->call(NFPCI(NFPCI_READ_IO_LONG), device_handle, pci_address, data);
}

unsigned char read_io_byte_fast(unsigned long device_handle, unsigned long pci_address)
{
	return nfOps->call(NFPCI(NFPCI_READ_IO_BYTE_FAST), device_handle, pci_address);
}

unsigned short read_io_word_fast(unsigned long device_handle, unsigned long pci_address)
{
	return nfOps->call(NFPCI(NFPCI_READ_IO_WORD_FAST), device_handle, pci_address);
}

unsigned long read_io_long_fast(unsigned long device_handle, unsigned long pci_address)
{
	return nfOps->call(NFPCI(NFPCI_READ_IO_LONG_FAST), device_handle, pci_address);
}

long write_io_byte(unsigned long device_handle, unsigned long pci_address, unsigned long value)
{
	return nfOps->call(NFPCI(NFPCI_WRITE_IO_BYTE), device_handle, pci_address, value);
}

long write_io_word(unsigned long device_handle, unsigned long pci_address, unsigned long value)
{
	return nfOps->call(NFPCI(NFPCI_WRITE_IO_WORD), device_handle, pci_address, value);
}

long write_io_long(unsigned long device_handle, unsigned long pci_address, unsigned long value)
{
	return nfOps->call(NFPCI(NFPCI_WRITE_IO_LONG), device_handle, pci_address, value);
}

long get_machine_id(void)
{
	return nfOps->call(NFPCI(NFPCI_GET_MACHINE_ID));
}

unsigned long get_pagesize(void)
{
	return nfOps->call(NFPCI(NFPCI_GET_PAGESIZE));
}

long virt_to_bus(unsigned long device_handle, void *virt_cpu_address, void *data)
{
	return nfOps->call(NFPCI(NFPCI_VIRT_TO_BUS), device_handle, virt_cpu_address, data);
}

long bus_to_virt(unsigned long device_handle, unsigned long pci_address, void *data)
{
	return nfOps->call(NFPCI(NFPCI_BUS_TO_VIRT), device_handle, pci_address, data);
}

long virt_to_phys(void *virt_cpu_address, void *data)
{
	return nfOps->call(NFPCI(NFPCI_VIRT_TO_PHYS), virt_cpu_address, data);
}

long phys_to_virt(void *phys_cpu_address, void *data)
{
	return nfOps->call(NFPCI(NFPCI_PHYS_TO_VIRT), phys_cpu_address, data);
}
