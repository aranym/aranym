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

#ifndef _PARALLEL_X86_H
#define _PARALLEL_X86_H

class ParallelX86: public Parallel
{
	private:
		int port;
		int gcontrol;
		int old_strobe, old_busy;

		void set_ctrl(int);
		void clr_ctrl(int);

	public:
		ParallelX86(void);
		~ParallelX86(void);
		void reset(void);
		
		void setDirection(bool out);
		uint8 getData();
		void setData(uint8 value);
		uint8 getBusy();
		void setStrobe(bool high);
};

#endif /* _PARALLEL_X86_H */
