/*
 * main.h - general definitions
 *
 * Copyright (c) 2001-2004 Petr Stehlik of ARAnyM dev team (see AUTHORS)
 * 
 * Inspired by Christian Bauer's Basilisk II
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

#ifndef MAIN_H
#define MAIN_H

#include "sysdeps.h"

// CPU type (0 = 68000, 1 = 68010, 2 = 68020, 3 = 68030, 4 = 68040/060)
extern int CPUType;

// FPU type (0 = no FPU, 1 = 68881, 2 = 68882)
extern int FPUType;

// 68k register structure (for Execute68k())
struct M68kRegisters {
	uint32 d[8];
	memptr a[8];
	uint16 sr;
	memptr usp, isp, msp;
	memptr pc;
};

#ifdef SDL_GUI
extern bool isGuiAvailable;
extern bool startupGUI;
extern char *startupAlert;
#endif

// General functions
extern bool InitAll(void);
extern void ExitAll(void);
extern void RestartAll(bool cold = false);

extern void invoke200HzInterrupt(void);

#ifdef OS_irix
extern void setactvdebug();
#else
extern void setactvdebug(int);
#endif

// Platform-specific functions
extern void QuitEmulator(int exitcode = EXIT_FAILURE);				// Quit emulator

#if FIXED_ADDRESSING
bool allocate_all_memory(uintptr fmemory, bool quiet);
#else
bool allocate_all_memory(bool quiet);
#endif
void release_all_memory(void);

#endif
