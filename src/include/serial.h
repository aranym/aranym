/*
	Serial port emulation, base class

	ARAnyM (C) 2005 Patrice Mandin
		2010 Jean Conter

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

#ifndef SERIAL_H
#define SERIAL_H

#include "sysdeps.h"

class Serial {
	public:
		Serial(void);
		virtual ~Serial(void);
		virtual void reset(void);
		
		virtual uint8 getData();
		virtual void setData(uint8 value);
		virtual void setBaud(uint8 value);
		virtual uint16 getStatus();
		virtual void setRTSDTR(uint8 value);
		virtual uint16 getTBE();
};

#endif /* SERIAL_H */
