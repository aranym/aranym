/*
	Serial port emulation, base class

	ARAnyM (C) 2005 Patrice Mandin

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
#include "serial.h"

#define DEBUG 0
#include "debug.h"

/*--- Public functions ---*/

Serial::Serial(void)
{
	D(bug("serial: created"));
}

Serial::~Serial(void)
{
	D(bug("serial: destroyed"));
}

void Serial::reset(void)
{
	D(bug("serial: reset"));
}

uint8 Serial::getData()
{
	D(bug("serial: getData"));
	return 0;
}

void Serial::setData(uint8 value)
{
	DUNUSED(value);
	D(bug("serial: setData"));
}

void Serial::setBaud(uint32 value)
{
	DUNUSED(value);
	D(bug("serial: setBaud"));
}

uint16 Serial::getStatus()
{
	D(bug("serial: getStatus"));
	return 0;
}

void Serial::setRTS(bool value)
{
	DUNUSED(value);
	D(bug("serial: setRTS"));
}

void Serial::setDTR(bool value)
{
	DUNUSED(value);
	D(bug("serial: setDTR"));
}

uint16 Serial::getTBE()
{
	D(bug("serial: getTBE"));
	return 0;
}

/*
vim:ts=4:sw=4:
*/
