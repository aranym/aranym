/*
 * acia.cpp - ACIA 6850 emulation code
 *
 * Copyright (c) 2001-2006 Petr Stehlik of ARAnyM dev team (see AUTHORS)
 * 
 * This file is part of the ARAnyM project which builds a new and powerful
 * TOS/FreeMiNT compatible virtual machine running on almost any hardware.
 *
 * ARAnyM is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * ARAnyM is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ARAnyM; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
 
#include "sysdeps.h"
#include "hardware.h"
#include "cpu_emulation.h"
#include "memory-uae.h"
#include "acia.h"

#define DEBUG 0
#include "debug.h"

ACIA::ACIA(memptr addr, uint32 size) : BASE_IO(addr, size)
{
	D(bug("acia: interface created at 0x%06x", getHWoffset()));

	reset();
}

ACIA::~ACIA()
{
	D(bug("acia: interface destroyed at 0x%06x", getHWoffset()));
}

void ACIA::reset()
{
	cr = rxdr = txdr = 0;
	sr = 1<<ACIA_SR_TXEMPTY;

	D(bug("acia: reset"));
}

uae_u8 ACIA::handleRead(uaecptr addr)
{
	uae_u8 value;

	switch(addr-getHWoffset()) {
		case 0:
			value = ReadStatus();
			break;
		case 2:
			value = ReadData();
			break;
		default:
			value = 0;
	}
	
	D(bug("acia: Read 0x%02x from 0x%08x",value,addr));

	return value;
}

void ACIA::handleWrite(uaecptr addr, uae_u8 value)
{
	D(bug("acia: Write 0x%02x to 0x%08x",value,addr));

	switch(addr-getHWoffset()) {
		case 0:
			if ((value & ACIA_CR_PREDIV_MASK)==ACIA_CR_RESET) {
				ACIA::reset();
			} else {
				WriteControl(value);
			}
			break;
		case 2:
			WriteData(value);
			break;
	}
}

uae_u8 ACIA::ReadStatus()
{
	D(bug("acia: ReadStatus()=0x%02x",sr));
	return sr;
}

void ACIA::WriteControl(uae_u8 value)
{
	cr = value;
	D(bug("acia: WriteControl(0x%02x)",cr));

	PrintControlRegister("acia",value);
}

uae_u8 ACIA::ReadData()
{
	D(bug("acia: ReadData()=0x%02x",rxdr));
	return rxdr;
}

void ACIA::WriteData(uae_u8 value)
{
	txdr = value;
	D(bug("acia: WriteData(0x%02x)",txdr));
}

void ACIA::PrintControlRegister(const char *devicename, uae_u8 value)
{
	DUNUSED(devicename);
	/* Clock */
	switch(value & ACIA_CR_PREDIV_MASK) {
		case ACIA_CR_PREDIV1:
			D(bug("%s: clock = 500 kHz", devicename));
			break;
		case ACIA_CR_PREDIV16:
			D(bug("%s: clock = 31250 Hz", devicename));
			break;
		case ACIA_CR_PREDIV64:
			D(bug("%s: clock = 7812.5 Hz", devicename));
			break;
		case ACIA_CR_RESET:
		default:
			break;
	}

	/* Format */
	if (value & (1<<ACIA_CR_PARITY)) {
		D(bug("%s: parity off", devicename));
	} else {
		D(bug("%s: parity on", devicename));
	}
	if (value & (1<<ACIA_CR_STOPBITS)) {
		D(bug("%s: 1 stop bit", devicename));
	} else {
		D(bug("%s: 2 stop bits", devicename));
	}
	if (value & (1<<ACIA_CR_DATABITS)) {
		D(bug("%s: 8 data bits", devicename));
	} else {
		D(bug("%s: 7 data bits", devicename));
	}

	/* Emission control */
	if (value & (1<<ACIA_CR_EMIT_INTER)) {
		D(bug("%s: emit interrupt enabled", devicename));
	} else {
		D(bug("%s: emit interrupt disabled", devicename));
	}
	if (value & (1<<ACIA_CR_EMIT_RTS)) {
		D(bug("%s: RTS high", devicename));
	} else {
		D(bug("%s: RTS low", devicename));
	}
	if (((value>>ACIA_CR_EMIT_INTER) & 0x03)==0x03) {
		D(bug("%s: emit BREAK", devicename));
	}
	
	/* Reception control */
	if (value & (1<<ACIA_CR_REC_INTER)) {
		D(bug("%s: receive interrupt enabled", devicename));
	} else {
		D(bug("%s: receive interrupt disabled", devicename));
	}
}

/*
vim:ts=4:sw=4:
*/
