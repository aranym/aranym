/*
	NatFeat host PCI driver, _PCI cookie with function pointers

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

	.globl	_pcibios_find_device, _pcibios_find_classcode
	.globl	_pcibios_read_config_byte, _pcibios_read_config_word
	.globl	_pcibios_read_config_long, _pcibios_read_config_byte_fast
	.globl	_pcibios_read_config_word_fast, _pcibios_read_config_long_fast
	.globl	_pcibios_write_config_byte, _pcibios_write_config_word
	.globl	_pcibios_write_config_long, _pcibios_hook_interrupt
	.globl	_pcibios_unhook_interrupt, _pcibios_special_cycle
	.globl	_pcibios_get_routing, _pcibios_set_interrupt
	.globl	_pcibios_get_resource, _pcibios_get_card_used
	.globl	_pcibios_set_card_used, _pcibios_read_mem_byte
	.globl	_pcibios_read_mem_word, _pcibios_read_mem_long
	.globl	_pcibios_read_mem_byte_fast, _pcibios_read_mem_word_fast
	.globl	_pcibios_read_mem_long_fast, _pcibios_write_mem_byte
	.globl	_pcibios_write_mem_word, _pcibios_write_mem_long
	.globl	_pcibios_read_io_byte, _pcibios_read_io_word
	.globl	_pcibios_read_io_long, _pcibios_read_io_byte_fast
	.globl	_pcibios_read_io_word_fast, _pcibios_read_io_long_fast
	.globl	_pcibios_write_io_byte, _pcibios_write_io_word
	.globl	_pcibios_write_io_long, _pcibios_get_machine_id
	.globl	_pcibios_get_pagesize, _pcibios_virt_to_bus
	.globl	_pcibios_bus_to_virt, _pcibios_virt_to_phys
	.globl	_pcibios_phys_to_virt

	.text

_pcibios_find_device:
	andl	#0xffff,d1
	movl	d1,sp@-
	movl	d0,sp@-
	bsr		_find_device
	addql	#8,sp
	rts

_pcibios_find_classcode:
	andl	#0xffff,d1
	movl	d1,sp@-
	movl	d0,sp@-
	bsr		_find_classcode
	addql	#8,sp
	rts

_pcibios_read_config_byte:
	andl	#0xff,d1
	movl	d1,sp@-
	movl	a0,sp@-
	movl	d0,sp@-
	bsr		_read_config_byte
	lea		sp@(12),sp
	rts

_pcibios_read_config_word:
	andl	#0xff,d1
	movl	d1,sp@-
	movl	a0,sp@-
	movl	d0,sp@-
	bsr		_read_config_word
	lea		sp@(12),sp
	rts

_pcibios_read_config_long:
	andl	#0xff,d1
	movl	d1,sp@-
	movl	a0,sp@-
	movl	d0,sp@-
	bsr		_read_config_long
	lea		sp@(12),sp
	rts

_pcibios_read_config_byte_fast:
	andl	#0xff,d1
	movl	d1,sp@-
	movl	d0,sp@-
	bsr		_read_config_byte_fast
	addql	#8,sp
	rts

_pcibios_read_config_word_fast:
	andl	#0xff,d1
	movl	d1,sp@-
	movl	d0,sp@-
	bsr		_read_config_word_fast
	addql	#8,sp
	rts

_pcibios_read_config_long_fast:
	andl	#0xff,d1
	movl	d1,sp@-
	movl	d0,sp@-
	bsr		_read_config_long_fast
	addql	#8,sp
	rts

_pcibios_write_config_byte:
	andl	#0xff,d2
	movl	d2,sp@-
	andl	#0xff,d1
	movl	d1,sp@-
	movl	d0,sp@-
	bsr		_write_config_byte
	lea		sp@(12),sp
	rts

_pcibios_write_config_word:
	andl	#0xffff,d2
	movl	d2,sp@-
	andl	#0xff,d1
	movl	d1,sp@-
	movl	d0,sp@-
	bsr		_write_config_word
	lea		sp@(12),sp
	rts

_pcibios_write_config_long:
	movl	d2,sp@-
	andl	#0xff,d1
	movl	d1,sp@-
	movl	d0,sp@-
	bsr		_write_config_long
	lea		sp@(12),sp
	rts

_pcibios_hook_interrupt:
	movl	a1,sp@-
	movl	a0,sp@-
	movl	d0,sp@-
	bsr		_hook_interrupt
	lea		sp@(12),sp
	rts

_pcibios_unhook_interrupt:
	movl	d0,sp@-
	bsr		_unhook_interrupt
	addql	#4,sp
	rts

_pcibios_special_cycle:
	movl	d1,sp@-
	andl	#0xff,d0
	movl	d0,sp@-
	bsr		_special_cycle
	addql	#8,sp
	rts

_pcibios_get_routing:
	movel	#0xfffffff8,d0	/* PCI_GENERAL_ERROR */
	rts

_pcibios_set_interrupt:
	movel	#0xfffffff8,d0	/* PCI_GENERAL_ERROR */
	rts

_pcibios_get_resource:
	movl	d0,sp@-
	bsr		_get_resource
	addql	#4,sp
	rts

_pcibios_get_card_used:
	movl	a0,sp@-
	movl	d0,sp@-
	bsr		_get_card_used
	addql	#8,sp
	rts

_pcibios_set_card_used:
	movl	a0,sp@-
	movl	d0,sp@-
	bsr		_set_card_used
	addql	#8,sp
	rts

_pcibios_read_mem_byte:
	movl	a0,sp@-
	movl	d1,sp@-
	movl	d0,sp@-
	bsr		_read_mem_byte
	lea		sp@(12),sp
	rts

_pcibios_read_mem_word:
	movl	a0,sp@-
	movl	d1,sp@-
	movl	d0,sp@-
	bsr		_read_mem_word
	lea		sp@(12),sp
	rts

_pcibios_read_mem_long:
	movl	a0,sp@-
	movl	d1,sp@-
	movl	d0,sp@-
	bsr		_read_mem_long
	lea		sp@(12),sp
	rts

_pcibios_read_mem_byte_fast:
	movl	d1,sp@-
	movl	d0,sp@-
	bsr		_read_mem_byte_fast
	addql	#8,sp
	rts

_pcibios_read_mem_word_fast:
	movl	d1,sp@-
	movl	d0,sp@-
	bsr		_read_mem_word_fast
	addql	#8,sp
	rts

_pcibios_read_mem_long_fast:
	movl	d1,sp@-
	movl	d0,sp@-
	bsr		_read_mem_long_fast
	addql	#8,sp
	rts

_pcibios_write_mem_byte:
	andl	#0xff,d2
	movl	d2,sp@-
	movl	d1,sp@-
	movl	d0,sp@-
	bsr		_write_mem_byte
	lea		sp@(12),sp
	rts

_pcibios_write_mem_word:
	andl	#0xffff,d2
	movl	d2,sp@-
	movl	d1,sp@-
	movl	d0,sp@-
	bsr		_write_mem_word
	lea		sp@(12),sp
	rts

_pcibios_write_mem_long:
	movl	d2,sp@-
	movl	d1,sp@-
	movl	d0,sp@-
	bsr		_write_mem_long
	lea		sp@(12),sp
	rts

_pcibios_read_io_byte:
	movl	a0,sp@-
	movl	d1,sp@-
	movl	d0,sp@-
	bsr		_read_io_byte
	lea		sp@(12),sp
	rts

_pcibios_read_io_word:
	movl	a0,sp@-
	movl	d1,sp@-
	movl	d0,sp@-
	bsr		_read_io_word
	lea		sp@(12),sp
	rts

_pcibios_read_io_long:
	movl	a0,sp@-
	movl	d1,sp@-
	movl	d0,sp@-
	bsr		_read_io_long
	lea		sp@(12),sp
	rts

_pcibios_read_io_byte_fast:
	movl	d1,sp@-
	movl	d0,sp@-
	bsr		_read_io_byte_fast
	addql	#8,sp
	rts

_pcibios_read_io_word_fast:
	movl	d1,sp@-
	movl	d0,sp@-
	bsr		_read_io_word_fast
	addql	#8,sp
	rts

_pcibios_read_io_long_fast:
	movl	d1,sp@-
	movl	d0,sp@-
	bsr		_read_io_long_fast
	addql	#8,sp
	rts

_pcibios_write_io_byte:
	andl	#0xff,d2
	movl	d2,sp@-
	movl	d1,sp@-
	movl	d0,sp@-
	bsr		_write_io_byte
	lea		sp@(12),sp
	rts

_pcibios_write_io_word:
	andl	#0xffff,d2
	movl	d2,sp@-
	movl	d1,sp@-
	movl	d0,sp@-
	bsr		_write_io_word
	lea		sp@(12),sp
	rts

_pcibios_write_io_long:
	movl	d2,sp@-
	movl	d1,sp@-
	movl	d0,sp@-
	bsr		_write_io_long
	lea		sp@(12),sp
	rts

_pcibios_get_machine_id:
	bsr		_get_machine_id
	rts

_pcibios_get_pagesize:
	bsr		_get_pagesize
	rts

_pcibios_virt_to_bus:
	movl	a0,sp@-
	movl	d1,sp@-
	movl	d0,sp@-
	bsr		_virt_to_bus
	lea		sp@(12),sp
	rts

_pcibios_bus_to_virt:
	movl	a0,sp@-
	movl	d1,sp@-
	movl	d0,sp@-
	bsr		_bus_to_virt
	lea		sp@(12),sp
	rts

_pcibios_virt_to_phys:
	movl	a0,sp@-
	movl	d0,sp@-
	bsr		_bus_to_virt
	addql	#8,sp
	rts

_pcibios_phys_to_virt:
	movl	a0,sp@-
	movl	d0,sp@-
	bsr		_bus_to_virt
	addql	#8,sp
	rts
