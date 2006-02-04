/*
	ROM / OS loader, Linux/m68k

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
#ifdef ENABLE_LILO
#include "lilo.h"
#endif
#include "bootos_linux.h"
#include "aranym_exception.h"

#define DEBUG 0
#include "debug.h"

/*	Linux/m68k loader class */

LinuxBootOs::LinuxBootOs(void) throw (AranymException)
{
#ifdef ENABLE_LILO
	if (!LiloInit())
	{
		throw AranymException("Error loading Linux/m68k kernel");
	}
#else
	throw AranymException("Linux/m68k loader disabled");
#endif
}

LinuxBootOs::~LinuxBootOs(void)
{
#ifdef ENABLE_LILO
	LiloShutdown();
#endif
}

void LinuxBootOs::reset(void)
{
	/* Linux/m68k kernel is in RAM, and must be reloaded */

	/*
		FIXME: Well, if we get there, LiloInit() already returned true the first
		time. But maybe the kernel and/or ramdisk image has been deleted/corrupted
		since then, so exception should be also thrown there.
	*/
#ifdef ENABLE_LILO
	LiloInit();
#endif
}
