/*
 * Parallel port emulation
 */
#define LINUXia32	0
#define CONSOLE		1

#include "parallel.h"
#if LINUXia32
#include <asm/io.h>
#include <sys/perm.h>
#endif

#define DEBUG 0
#include "debug.h"

#if LINUXia32
#define outportb(a,b)	outb(b,a)
#define inportb(a)	 inb(a)
#define permission(a,b,c)	ioperm(a,b,c)
#else
#define outportb(a,b)	
#define inportb(a)	 0
#define permission(a,b,c)
#endif

Parallel::Parallel() 
{
	gcontrol = (1 << 2);	/* Ucc */
	port = 0x378;
	old_strobe = -1;
	old_busy = -1;
	// get the permission
	permission(port,4,1);
}

inline void Parallel::set_ctrl(int x)
{
	gcontrol |= (1 << x);
	outportb(port+2, gcontrol);
}

inline void Parallel::clr_ctrl(int x)
{
	gcontrol &=~(1 << x);
	outportb(port+2, gcontrol);
}

void Parallel::setDirection(bool out)
{
	if (out)
		clr_ctrl(5);
	else
		set_ctrl(5);
	D(bug("Parallel:direction: %s", out ? "OUT" : "IN"));
}

uint8 Parallel::getData()
{
	uint8 data = inportb(port);
	D(bug("Parallel:getData()=$%x", data));
	return data;
}

void Parallel::setData(uint8 value)
{
#if CONSOLE
	putchar(value);
	fflush(stdout);
#else
	outportb(port, value);
#endif
	D(bug("Parallel:setData($%x)", value));
}

uint8 Parallel::getBusy()
{
#if CONSOLE
	return 0;
#else
	uint8 busy = !(inportb(port+1) & 0x80);
	if (old_busy != busy)
		D(bug("Parallel:Busy = %s", busy == 1 ? "YES" : "NO"));
	old_busy = busy;
	return busy;
#endif
}

void Parallel::setStrobe(bool high)
{
	if (old_strobe != -1 && old_strobe == high)
		D(bug("Parallel:strobe(%s)", high ? "HIGH" : "LOW"));

	if (high)
		clr_ctrl(0);
	else
		set_ctrl(0);

	old_strobe = high;
}
