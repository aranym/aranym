/* 2001 MJ */

/*
 *  main.cpp - Startup/shutdown code
 *
 *  Basilisk II (C) 1997-2000 Christian Bauer
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "sysdeps.h"

#include "cpu_emulation.h"
#include "timer.h"
#include "main.h"
#include "hardware.h"

#define DEBUG 1
#include "debug.h"

/*
 *  Initialize everything, returns false on error
 */

bool InitAll(void)
{
	CPUType = 4;
	FPUType = 1;

	// Setting "SP & PC"
	put_long(0x00000000,get_long(ROMBase));
	put_long(0x00000004,get_long(ROMBase+4));

	// Init HW
	HWInit();

	// Init Time Manager
	TimerInit();

	// Init 680x0 emulation (this also activates the memory system which is needed for PatchROM())
	if (!Init680x0())
		return false;

	printf("Activate debugger...\n");
	activate_debugger();

	return true;
}


/*
 *  Deinitialize everything
 */

void ExitAll(void)
{
	// Exit Time Manager
	TimerExit();
}

