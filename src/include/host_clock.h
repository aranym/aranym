/*
	Host clock for timers

	ARAnyM (C) 2006 Patrice Mandin

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

#ifndef HOSTCLOCK_H
#define HOSTCLOCK_H

#include <stdint.h>

/*--- HostClock Class ---*/

class HostClock {
	protected:
		uint32_t startTime;

		virtual	uint32_t getCurTime(void);	/* Override this for your system */

	public:
		HostClock(void);
		virtual ~HostClock() {}

		void reset(void);
		uint32_t getClock(void);	/* integer part are milli seconds */
};

#endif /* HOSTCLOCK_H */
