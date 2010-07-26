/*
	Serial port emulation, Linux driver

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

#ifndef SERIAL_PORT_H
#define SERIAL_PORT_H

class Serialport: public Serial
{
	private:
		int handle;
		uint16 oldTBE;
		uint16 oldStatus;
		uint16 getTBE();

	public:
		Serialport(void);
		~Serialport(void);
		void reset(void);
		
		uint8 getData();
		void setData(uint8 value);
		void setBaud(uint32 value);
		uint16 getStatus();
		void setRTS(bool value);
		void setDTR(bool value);
};

#endif /* SERIAL_PORT_H */
