/*
	Joypads port emulation

	ARAnyM (C) 2008 Patrice Mandin

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

#ifndef JOYPADS_H
#define JOYPADS_H

#include "icio.h"

class JOYPADS : public BASE_IO
{
	private:
		static const int multiplexer0[8][4];	/* on 0xff9200 */
		static const int multiplexer1[8][4];	/* on 0xff9202 */

		Uint16	mask;
		Uint32	host_state[2];

	public:
		JOYPADS(memptr, uint32);
		virtual ~JOYPADS();
		virtual void reset(void);

		virtual uae_u8 handleRead(uaecptr addr);
		virtual void handleWrite(uaecptr addr, uae_u8 value);

		void sendJoystickAxis(int numjoy, int numaxis, int value);
		void sendJoystickHat(int numjoy, int value);
		void sendJoystickButton(int numjoy, int which, int pressed);
};

#endif /* JOYPADS_H */
