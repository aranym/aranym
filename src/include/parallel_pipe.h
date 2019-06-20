/*
 * Parallel port emulation, output to program via pipe
 *
 * Copyright (c) 2019 ARAnyM developer team (see AUTHORS)
 *				 2019 Thorsten Otto
 *
 * This file is part of the ARAnyM project which builds a new and powerful
 * TOS/FreeMiNT compatible virtual machine running on almost any hardware.
 *
 * ARAnyM is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * ARAnyM is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ARAnyM; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _PARALLEL_PIPE_H
#define _PARALLEL_PIPE_H

class ParallelPipe: public Parallel
{
	private:
		const char *name;
		int inhandle;		/* our read end */
		int outhandle;	/* our write end */
		Uint32 last_output;
		Uint32 timeout;
		bool warned_already;
		bool can_restart;
		char **argv;
		const char *device_name;
		/*
		 * input buffer
		 */
		static const int BUFSIZE = 1024;
		static const int LO_WATER = 1;
		unsigned char buffer[BUFSIZE];
		int buffer_ptr;
		int buffer_size;

		void _close();
		int reopen();

	public:
		ParallelPipe(void);
		virtual ~ParallelPipe(void);
		virtual void reset(void);
		void check_pipe();

		virtual void setDirection(bool out);
		virtual uint8 getData();
		virtual void setData(uint8 value);
		virtual uint8 getBusy();
		virtual void setStrobe(bool high);
};

#endif /* _PARALLEL_PIPE_H */
