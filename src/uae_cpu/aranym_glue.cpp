/* 2001 MJ */
/*
 *  aranym_glue.cpp - Glue UAE CPU to ARAnyM
 *
 *  based on
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

#include "sysdeps.h"

#include "cpu_emulation.h"
#include "newcpu.h"
#ifdef USE_JIT
# include "compiler/compemu.h"
#endif

// RAM and ROM pointers
memptr RAMBase = 0;	// RAM base (Atari address space) gb-- init is important
uint8 *RAMBaseHost;	// RAM base (host address space)
uint32 RAMSize = 0x00e00000;		// Size of RAM

memptr ROMBase = 0x00e00000;		// ROM base (Atari address space)
uint8 *ROMBaseHost;	// ROM base (host address space)
uint32 ROMSize = 0x00200000;		// Size of ROM

uint32 RealROMSize;	// Real size of ROM

const uint32 HWBase = 0x00f00000;	// HW base (Atari address space)

memptr FastRAMBase = 0x01000000;		// Fast-RAM base (Atari address space)
uint8 *FastRAMBaseHost;	// Fast-RAM base (host address space)

#ifdef USE_JIT
uint8 *FakeIOBaseHost;
#endif

#ifdef FIXED_VIDEORAM
memptr VideoRAMBase = ARANYMVRAMSTART;  // VideoRAM base (Atari address space)
#else
memptr VideoRAMBase;                    // VideoRAM base (Atari address space)
#endif
uint8 *VideoRAMBaseHost;// VideoRAM base (host address space)
//uint32 VideoRAMSize;	// Size of VideoRAM

#ifndef NOT_MALLOC
uintptr MEMBaseDiff;	// Global offset between a Atari address and its Host equivalent
#endif

uintptr VMEMBaseDiff;	// Global offset between a Atari VideoRAM address and /dev/fb0 mmap

// From newcpu.cpp
extern int quit_program;

/*
 *  Initialize 680x0 emulation
 */

bool InitMEM() {
#if REAL_ADDRESSING
	// Atari address space = host address space
	RAMBase = (uint32)RAMBaseHost;
	ROMBase = (uint32)ROMBaseHost;
#else
	InitMEMBaseDiff(RAMBaseHost, RAMBase);
#endif
	InitVMEMBaseDiff(VideoRAMBaseHost, VideoRAMBase);
	return true;
}

bool Init680x0(void)
{
	init_m68k();
#ifdef USE_JIT
	if (bx_options.jit.jit) compiler_init();
#endif
	return true;
}

/*
 * Instr. RESET
 */

void AtariReset(void)
{
	// reset Atari hardware here
}

/*
 * Reset CPU
 */

void Reset680x0(void)
{
	m68k_reset();
}

/*
 *  Deinitialize 680x0 emulation
 */

void Exit680x0(void)
{
#ifdef USE_JIT
	if (bx_options.jit.jit) compiler_exit();
#endif
	exit_m68k();
}


/*
 *  Reset and start 680x0 emulation
 */

void Start680x0(void)
{
	m68k_reset();
#ifdef USE_JIT
	if (bx_options.jit.jit)
		m68k_compile_execute();
	else
#endif
		m68k_execute();
}

/*
 * Restart running 680x0 emulation
 */
void Restart680x0(void)
{
	quit_program = 2;
	TriggerNMI();
}

/*
 * Quit 680x0 emulation
 */
void Quit680x0(void)
{
	quit_program = 1;
	TriggerNMI();
}


/*
 *  Trigger interrupts
 */
void TriggerVBL(void)
{
	SPCFLAGS_SET( SPCFLAG_VBL );
}

void TriggerInterrupt(void)
{
	SPCFLAGS_SET( SPCFLAG_INT5 );
}

void TriggerMFP(bool enable)
{
	if (enable)
		SPCFLAGS_SET( SPCFLAG_MFP );
	else
		SPCFLAGS_CLEAR( SPCFLAG_MFP );
}

void TriggerNMI(void)
{
	SPCFLAGS_SET( SPCFLAG_NMI );
}


// Two next functions ("executes") will be removed
/*
 *  Execute MacOS 68k trap
 *  r->a[7] and r->sr are unused!
 */
#if 0
void Execute68kTrap(uint16 trap, struct M68kRegisters *r)
{
	int i;

	// Save old PC
	uaecptr oldpc = m68k_getpc();

	// Set registers
	for (i=0; i<8; i++)
		m68k_dreg(regs, i) = r->d[i];
	for (i=0; i<7; i++)
		m68k_areg(regs, i) = r->a[i];

	// Push trap and EXEC_RETURN on stack
	m68k_areg(regs, 7) -= 2;
	put_word(m68k_areg(regs, 7), M68K_EXEC_RETURN);
	m68k_areg(regs, 7) -= 2;
	put_word(m68k_areg(regs, 7), trap);

	// Execute trap
	m68k_setpc(m68k_areg(regs, 7));
	quit_program = 0;
	m68k_go(true);

	// Clean up stack
	m68k_areg(regs, 7) += 4;

	// Restore old PC
	m68k_setpc(oldpc);

	// Get registers
	for (i=0; i<8; i++)
		r->d[i] = m68k_dreg(regs, i);
	for (i=0; i<7; i++)
		r->a[i] = m68k_areg(regs, i);
	quit_program = 0;
}


/*
 *  Execute 68k subroutine
 *  The executed routine must reside in UAE memory!
 *  r->a[7] and r->sr are unused!
 */

void Execute68k(uint32 addr, struct M68kRegisters *r)
{
	int i;

	// Save old PC
	uaecptr oldpc = m68k_getpc();

	// Set registers
	for (i=0; i<8; i++)
		m68k_dreg(regs, i) = r->d[i];
	for (i=0; i<7; i++)
		m68k_areg(regs, i) = r->a[i];

	// Push EXEC_RETURN and faked return address (points to EXEC_RETURN) on stack
	m68k_areg(regs, 7) -= 2;
	put_word(m68k_areg(regs, 7), M68K_EXEC_RETURN);
	m68k_areg(regs, 7) -= 4;
	put_long(m68k_areg(regs, 7), m68k_areg(regs, 7) + 4);

	// Execute routine
	m68k_setpc(addr);
	quit_program = 0;
	m68k_go(true);

	// Clean up stack
	m68k_areg(regs, 7) += 2;

	// Restore old PC
	m68k_setpc(oldpc);

	// Get registers
	for (i=0; i<8; i++)
		r->d[i] = m68k_dreg(regs, i);
	for (i=0; i<7; i++)
		r->a[i] = m68k_areg(regs, i);
	quit_program = 0;
}
#endif
