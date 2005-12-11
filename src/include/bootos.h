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

#ifndef BOOTOS_H
#define BOOTOS_H

/* Exception class */

class BootOsException {
	private:
		char errMsg[256];
	public:
		BootOsException(void);
		BootOsException(char *errorMessage);

		char *getErrorMessage(void);
};

/* Base class */

class BootOs {
	public:
		void init(void);
		void load(char *filename) throw (BootOsException);
		void reset(void);
};

/* TOS ROM class */

class TosBootOs : public BootOs {
	public:
		TosBootOs(void) throw (BootOsException);
};

/* EmuTOS ROM class */

class EmutosBootOs : public BootOs {
	public:
		EmutosBootOs(void) throw (BootOsException);
};

/* Linux/m68k loader class */

class LinuxBootOs : public BootOs {
	public:
		LinuxBootOs(void) throw (BootOsException);
		~LinuxBootOs(void);

		void reset(void);
};

extern BootOs *bootOs;

#endif /* BOOTOS_H */
