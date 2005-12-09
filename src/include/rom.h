/*
	ROM / OS loader, base class

	Copyright (c) 2005 Patrice Mandin

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

#ifndef ROM_H
#define ROM_H

class RomInitializationException {
};

class Rom {
	public:
		void init(void);
		void load(char *filename) throw (RomInitializationException);
		void reset(void);
};

class TosRom : public Rom {
	public:
		TosRom(void) throw (RomInitializationException);
};

class EmutosRom : public Rom {
	public:
		EmutosRom(void) throw (RomInitializationException);
};

class LinuxRom : public Rom {
	public:
		LinuxRom(void) throw (RomInitializationException);
		~LinuxRom(void);

		void reset(void);
};

extern Rom *rom;

#endif /* ROM_H */
