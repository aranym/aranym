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

#ifndef NFPCI_H
#define NFPCI_H

/* Functions called by each PCIBIOS backend (_PCI via cookie, XPCI via Xbios */

long find_device(unsigned long device_vendor_id, unsigned short index);
long find_classcode(unsigned long class_code, unsigned short index);
long read_config_byte(unsigned long device_handle, void *data, unsigned char num_register);
long read_config_word(unsigned long device_handle, void *data, unsigned char num_register);
long read_config_long(unsigned long device_handle, void *data, unsigned char num_register);
unsigned char read_config_byte_fast(unsigned long device_handle, unsigned char num_register);
unsigned short read_config_word_fast(unsigned long device_handle, unsigned char num_register);
unsigned long read_config_long_fast(unsigned long device_handle, unsigned char num_register);
long write_config_byte(unsigned long device_handle, unsigned char num_register, unsigned char value);
long write_config_word(unsigned long device_handle, unsigned char num_register, unsigned short value);
long write_config_long(unsigned long device_handle, unsigned char num_register, unsigned long value);
long hook_interrupt(unsigned long device_handle, void (*data)(), unsigned long parameter); 
long unhook_interrupt(unsigned long device_handle); 
long special_cycle(unsigned char num_bus, unsigned long data); 
/* get_routing */
/* set_interrupt */
long get_resource(unsigned long device_handle); 
long get_card_used(unsigned long device_handle, unsigned long *callback);
long set_card_used(unsigned long device_handle, unsigned long callback);
long read_mem_byte(unsigned long device_handle, unsigned long pci_address, unsigned char *data);
long read_mem_word(unsigned long device_handle, unsigned long pci_address, unsigned short *data);
long read_mem_long(unsigned long device_handle, unsigned long pci_address, unsigned long *data);
unsigned char read_mem_byte_fast(unsigned long device_handle, unsigned long pci_address);
unsigned short read_mem_word_fast(unsigned long device_handle, unsigned long pci_address);
unsigned long read_mem_long_fast(unsigned long device_handle, unsigned long pci_address);
long write_mem_byte(unsigned long device_handle, unsigned long pci_address, unsigned char value);
long write_mem_word(unsigned long device_handle, unsigned long pci_address, unsigned short value);
long write_mem_long(unsigned long device_handle, unsigned long pci_address, unsigned long value);
long read_io_byte(unsigned long device_handle, unsigned long pci_address, unsigned char *data);
long read_io_word(unsigned long device_handle, unsigned long pci_address, unsigned short *data);
long read_io_long(unsigned long device_handle, unsigned long pci_address, unsigned long *data);
unsigned char read_io_byte_fast(unsigned long device_handle, unsigned long pci_address);
unsigned short read_io_word_fast(unsigned long device_handle, unsigned long pci_address);
unsigned long read_io_long_fast(unsigned long device_handle, unsigned long pci_address);
long write_io_byte(unsigned long device_handle, unsigned long pci_address, unsigned char value);
long write_io_word(unsigned long device_handle, unsigned long pci_address, unsigned short value);
long write_io_long(unsigned long device_handle, unsigned long pci_address, unsigned long value);
long get_machine_id(void);
unsigned long get_pagesize(void);
long virt_to_bus(unsigned long device_handle, void *virt_cpu_address, void *data);
long bus_to_virt(unsigned long device_handle, unsigned long pci_address, void *data);
long virt_to_phys(void *virt_cpu_address, void *data);
long phys_to_virt(void *phys_cpu_address, void *data);

#endif /* NFPCI_H */
