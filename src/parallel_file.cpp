/*
	Parallel port emulation, base class

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

#include <stdio.h>

#include "sysdeps.h"
#include "cpu_emulation.h"
#include "parameters.h"
#include "parallel.h"
#include "parallel_file.h"

#define DEBUG 0
#include "debug.h"

/*--- Public functions ---*/

ParallelFile::ParallelFile(void)
{
	D(bug("parallel_file: created"));
	handle = NULL;
}

ParallelFile::~ParallelFile(void)
{
	D(bug("parallel_file: destroyed"));
	if (handle) {
		fclose(handle);
		handle=NULL;
	}
}

void ParallelFile::reset(void)
{
	D(bug("parallel_file: reset"));
}

void ParallelFile::setDirection(bool out)
{
	D(bug("parallel_file: setDirection"));
}

uint8 ParallelFile::getData()
{
	D(bug("parallel_file: getData"));
	return 0;
}

void ParallelFile::setData(uint8 value)
{
	D(bug("parallel_file: setData"));
	if (!handle) {
		handle = fopen("/tmp/aranym-parallel.txt", "w");
		if (!handle) {
			fprintf(stderr,"Can not open file for parallel port\n");
			return;
		}
	}
	fprintf(handle,"%c",value);	
	fflush(handle);	/* FIXME: really mandatory ? */
}

uint8 ParallelFile::getBusy()
{
	D(bug("parallel_file: getBusy"));
	return 0;
}

void ParallelFile::setStrobe(bool high)
{
	D(bug("parallel_file: setStrobe"));
}
