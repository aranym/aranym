/*
	Parallel port emulation, output to file

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
#include "parameters.h"
#include "parallel.h"
#include "parallel_file.h"

#define DEBUG 0
#include "debug.h"

#include <cstdio>

/*--- Public functions ---*/

ParallelFile::ParallelFile(void)
{
	D(bug("parallel_file: created"));
	close_handle = 0;
	handle = NULL;
}

ParallelFile::~ParallelFile(void)
{
	D(bug("parallel_file: destroyed"));
	if (handle && close_handle) {
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
	DUNUSED(out);
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
		bx_parallel_options_t * p = &bx_options.parallel;
		if (strcmp("file", p->type)==0) {
			if (strcmp("stdout", p->file)==0) {
				handle = stdout;
			} else if (strcmp("stderr", p->file)==0) {
				handle = stderr;
			} else if (strlen(p->file) > 0) {
				handle = fopen(p->file, "w");
				if (!handle) {
					panicbug("Can not open file for parallel port\n");
					return;
				}
				close_handle=1;
			}
		}
	}
	fputc(value,handle);	
	if (!close_handle) {
		fflush(handle);	/* useful if you want to see the output before EOLN */
	}
}

uint8 ParallelFile::getBusy()
{
	D(bug("parallel_file: getBusy"));
	return 0;
}

void ParallelFile::setStrobe(bool high)
{
	DUNUSED(high);
	D(bug("parallel_file: setStrobe"));
}

/*
vim:ts=4:sw=4:
*/
