/*
	Parallel port emulation, Linux/X86 hardware access

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

#include <asm/io.h>
#include <sys/perm.h>

#include "parallel.h"
#include "parallel_x86.h"

#define DEBUG 0
#include "debug.h"

#define outportb(a,b)	outb(b,a)
#define inportb(a)	 inb(a)
#define permission(a,b,c)	ioperm(a,b,c)

ParallelX86::ParallelX86(void) 
{
	gcontrol = (1 << 2);	/* Ucc */
	port = 0x378;
	old_strobe = -1;
	old_busy = -1;
	// get the permission
	permission(port,4,1);
}

ParallelX86::~ParallelX86(void) 
{
}

void ParallelX86::reset(void) 
{
}

inline void ParallelX86::set_ctrl(int x)
{
	gcontrol |= (1 << x);
	outportb(port+2, gcontrol);
}

inline void ParallelX86::clr_ctrl(int x)
{
	gcontrol &=~(1 << x);
	outportb(port+2, gcontrol);
}

void ParallelX86::setDirection(bool out)
{
	if (out)
		clr_ctrl(5);
	else
		set_ctrl(5);
	D(bug("Parallel:direction: %s", out ? "OUT" : "IN"));
}

uint8 ParallelX86::getData()
{
	uint8 data = inportb(port);
	D(bug("Parallel:getData()=$%x", data));
	return data;
}

void ParallelX86::setData(uint8 value)
{
	outportb(port, value);
	D(bug("Parallel:setData($%x)", value));
}

uint8 ParallelX86::getBusy()
{
	uint8 busy = !(inportb(port+1) & 0x80);
	if (old_busy != busy)
		D(bug("Parallel:Busy = %s", busy == 1 ? "YES" : "NO"));
	old_busy = busy;
	return busy;
}

void ParallelX86::setStrobe(bool high)
{
	if (old_strobe != -1 && old_strobe == high)
		D(bug("Parallel:strobe(%s)", high ? "HIGH" : "LOW"));

	if (high)
		clr_ctrl(0);
	else
		set_ctrl(0);

	old_strobe = high;
}
