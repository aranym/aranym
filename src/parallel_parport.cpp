/*
	Parallel port emulation, Linux /dev/parport driver

	ARAnyM (C) 2005-2008 Patrice Mandin

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

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/ppdev.h>
#include <linux/parport.h>

#include "parallel.h"
#include "parallel_parport.h"

#define DEBUG 0
#include "debug.h"

ParallelParport::ParallelParport(void) 
{
	D(bug("ParallelParport: interface created"));
	handle = open(bx_options.parallel.parport,O_RDWR);
	if (handle<0) {
		panicbug("ParallelParport: Can not open device %s", bx_options.parallel.parport);
		return;
	}

	if (ioctl(handle, PPCLAIM)<0) {
		panicbug("ParallelParport: Can not claim access to parport device");
		close(handle);
		handle=-1;
		return;
	}	
}

ParallelParport::~ParallelParport(void) 
{
	D(bug("ParallelParport: interface destroyed"));
	if (handle>=0) {
		setDirection(1);
		if (ioctl(handle, PPRELEASE)<0) {
			panicbug("ParallelParport: Can not release parport device");
		}
		close(handle);
		handle=-1;
	}
}

void ParallelParport::reset(void) 
{
	D(bug("ParallelParport: reset"));
	setDirection(1);
	setStrobe(0);
}

void ParallelParport::setDirection(bool out)
{
	int datadir;

	D(bug("ParallelParport: setDirection"));
	if (handle<0) {
		return;
	}

	datadir = out ? 0 : 1;
	if (ioctl(handle, PPDATADIR, &datadir)<0) {
		panicbug("ParallelParport: Can not set direction");
	}
}

uint8 ParallelParport::getData()
{
	uint8 value=0;

	D(bug("ParallelParport: getData"));
	if (handle>=0) {
		if (ioctl(handle, PPRDATA, &value)<0) {
			panicbug("ParallelParport: Error reading data from device");
		}
	}
	return value;
}

void ParallelParport::setData(uint8 value)
{
	D(bug("ParallelParport: setData"));
	if (handle>=0) {
		if (ioctl(handle, PPWDATA, &value)<0) {
			panicbug("ParallelParport: Error writing data to device");
		}
	}
}

uint8 ParallelParport::getBusy()
{
	uint8 value;

	value=0;
	D(bug("ParallelParport: getBusy"));
	if (handle>=0) {
		if (ioctl(handle, PPRSTATUS, &value)<0) {
			panicbug("ParallelParport: Can not read BUSY line");
		}
	}
	return ((value & PARPORT_STATUS_BUSY)!=0);
}

void ParallelParport::setStrobe(bool high)
{
	struct ppdev_frob_struct frob;

	D(bug("ParallelParport: setStrobe"));
	if (handle<0) {
		return;
	}

	frob.mask = PARPORT_CONTROL_STROBE;
	frob.val = high ? PARPORT_CONTROL_STROBE : 0;
	if (ioctl(handle, PPFCONTROL, &frob)<0) {
		panicbug("ParallelParport: Can not set STROBE line");
	}
}
