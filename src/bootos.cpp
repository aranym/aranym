/*
	ROM / OS loader, base class

	ARAnyM (C) 2005-2006 Patrice Mandin

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
#include "bootos.h"
#include "aranym_exception.h"

#define DEBUG 0
#include "debug.h"

BootOs *bootOs = NULL;

void BootOs::init(bool cold)
{
	if (cold)
		memset(RAMBaseHost, rand(), RAMSize);
	/* Setting "SP & PC" for CPU with ROM based OS (TOS, EmuTOS) */
	for (int i=0; i<8; i++) {
		RAMBaseHost[i] = ROMBaseHost[i];
	}
}

void BootOs::reset(bool cold)
{
	init(cold);
}

void BootOs::load(const char *filename) ARANYM_THROWS(AranymException)
{
	D(bug("Reading OS ROM image '%s'", filename));
	FILE *f = fopen(filename, "rb");

	if (f == NULL) {
		throw AranymException("OS ROM image '%s' not found.", filename);
	}

	/* Both TOS 4.04 and EmuTOS must be 512 KB */
	RealROMSize = 512<<10;	

	size_t sizeRead = fread(ROMBaseHost, 1, RealROMSize, f);
	fclose(f);

	if (sizeRead != (size_t)RealROMSize) {
		throw AranymException("OS ROM image '%s' reading error.\nMake sure the file is readable and its size is 524288 bytes (512 kB).", filename);
	}
}
/* vim:ts=4:sw=4
 */
