/* 2001 MJ */

/*
 *  cpu_emulation.h - Definitions for Basilisk II CPU emulation module (UAE 0.8.10 version)
 *
 *  Basilisk II (C) 1997-2001 Christian Bauer
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

#ifndef CPU_EMULATION_H
#define CPU_EMULATION_H

#include <string.h>


/*
 *  Memory system
 */

// RAM and ROM pointers (allocated and set by main_*.cpp)
extern uint32 RAMBase;		// RAM base (Atari address space), does not include Low Mem when != 0
extern uint8 *RAMBaseHost;	// RAM base (host address space)
extern uint32 RAMSize;		// Size of RAM

extern uint32 ROMBase;		// ROM base (Atari address space)
extern uint8 *ROMBaseHost;	// ROM base (host address space)
extern uint32 ROMSize;		// Size of ROM

//extern uint32 TTRAMBase;	// TT-RAM base (Atari address space)
//extern uint8 *TTRAMBaseHost;	// TT-RAM base (host address space)
extern uint32 TTRAMSize;	// Size of TT-RAM

extern uint32 VideoRAMBase;	// VideoRAM base (Atari address space)
extern uint8 *VideoRAMBaseHost;	// VideoRAM base (host address space)
//extern uint32 VideoRAMSize;	// Size of VideoRAM

// Mac memory access functions
#include "memory.h"

/*
 *  680x0 emulation
 */

// Initialization
extern bool Init680x0(void);	// This routine may want to look at CPUType/FPUType to set up the apropriate emulation
extern void Exit680x0(void);

// 680x0 emulation functions
struct M68kRegisters;
extern void Start680x0(void);					// Reset and start 680x0
extern "C" void Execute68k(uint32 addr, M68kRegisters *r);	// Execute 68k code from EMUL_OP routine
extern "C" void Execute68kTrap(uint16 trap, M68kRegisters *r);	// Execute MacOS 68k trap from EMUL_OP routine

// Interrupt functions
// extern void TriggerInterrupt(void);	// Trigger interrupt level 1 (InterruptFlag must be set first)
extern void TriggerVBL(void);		// Trigger interrupt level 4
extern void TriggerMFP(int, int);		// Trigger interrupt level 6
extern void TriggerNMI(void);		// Trigger interrupt level 7

#endif
