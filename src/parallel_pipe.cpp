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

#include "sysdeps.h"
#include "cpu_emulation.h"
#include "parameters.h"
#include "parallel.h"
#include "parallel_pipe.h"
#include "shellparse.h"

#define DEBUG 0
#include "debug.h"

/*--- Public functions ---*/

ParallelPipe::ParallelPipe(void)
	: name("LPT")
	, inhandle(-1)
	, outhandle(-1)
	, last_output(0)
	, timeout(bx_options.parallel.timeout * 1000)
	, warned_already(false)
	, can_restart(false)
	, argv(NULL)
	, device_name(NULL)
	, buffer_ptr(0)
	, buffer_size(0)
{
	D(bug("parallel_pipe: created"));
	argv = shell_parse(bx_options.parallel.program, NULL);
	if (argv != NULL && argv[0] != NULL && argv[0][0] != '\0')
	{
		device_name = argv[0];
		can_restart = true;
	}
}

void ParallelPipe::_close(void)
{
	if (outhandle >= 0)
	{
		if (outhandle != inhandle)
			::close(outhandle);
		outhandle = -1;
	}
	if (inhandle >= 0)
	{
		::close(inhandle);
		inhandle = -1;
	}
	buffer_ptr = 0;
	buffer_size = 0;
}

ParallelPipe::~ParallelPipe(void)
{
	D(bug("parallel_pipe: destroyed"));
	_close();
	can_restart = false;
	free(argv);
	argv = NULL;
}


void ParallelPipe::check_pipe()
{
	if (inhandle >= 0 && outhandle >= 0 && inhandle != outhandle && SDL_GetTicks() >= (last_output + timeout))
	{
		D(bug("closing output pipe after inactivity"));
		_close();
	}
}


void ParallelPipe::reset(void)
{
	D(bug("parallel_pipe: reset"));
	_close();
}


void ParallelPipe::setDirection(bool out)
{
	direction = out;
	D(bug("parallel_pipe: setDirection"));
}


int ParallelPipe::reopen()
{
	int err = -1;
	int infds[2];
	int outfds[2];
	int pipefds[2];
	int count;
	pid_t pid;
	
	if (!can_restart)
		return err;
	if (pipe(infds))
	{
		return errno;
	}
	if (pipe(outfds))
	{
		err = errno;
		::close(infds[0]);
		::close(infds[1]);
		return err;
	}
	if (pipe(pipefds))
	{
		err = errno;
		::close(outfds[0]);
		::close(outfds[1]);
		::close(infds[0]);
		::close(infds[1]);
		return err;
	}
	if (fcntl(pipefds[1], F_SETFD, fcntl(pipefds[1], F_GETFD) | FD_CLOEXEC))
	{
		err = errno;
		::close(pipefds[0]);
		::close(pipefds[1]);
		::close(outfds[0]);
		::close(outfds[1]);
		::close(infds[0]);
		::close(infds[1]);
		return err;
	}

	pid = fork();
	if (pid == (pid_t)-1)
	{
		err = errno;
		::close(pipefds[0]);
		::close(pipefds[1]);
		::close(outfds[0]);
		::close(outfds[1]);
		::close(infds[0]);
		::close(infds[1]);
		panicbug("reopen: fork() failed");
		can_restart = false;
	} else if (pid == 0)
	{
		/* connect the output pipe of parent to childs stdin */
		::close(0);
		if (dup(outfds[0]) < 0) { D(panicbug("dup failed")); }
		::close(outfds[0]);

		/* connect the input pipe of parent to childs stdout */
		::close(1);
		if (dup(infds[1]) < 0) { D(panicbug("dup failed")); }
		::close(infds[1]);

		::close(outfds[1]);
		::close(infds[0]);
		::close(pipefds[0]);

		execvp(argv[0], argv);
		err = errno;
		if (write(pipefds[1], &err, sizeof(err)) < 0) { D(panicbug("write failed")); }
		_exit(127);
	} else
	{
		::close(infds[1]);
		::close(outfds[0]);
		::close(pipefds[1]);
		while ((count = read(pipefds[0], &err, sizeof(err))) == -1)
		{
			if (errno != EAGAIN && errno != EINTR)
				break;
		}
		::close(pipefds[0]);
		if (count)
		{
			::close(infds[0]);
			::close(outfds[1]);
			bug("%s(%s): exec failed: %s", name, argv[0], strerror(err));
			can_restart = false;
			warned_already = true;
		} else
		{
			/*
			 * we get here if we could not read the errno from the pipe,
			 * because the pipe was closed by a successful execv() in the child
			 */
			inhandle = infds[0];
			outhandle = outfds[1];
		}
		install_sigchild_handler();
		last_output = SDL_GetTicks();
	}
	
	return err;
}


uint8 ParallelPipe::getData()
{
	int l, i;
	unsigned char c;
	unsigned char tmpbuf[BUFSIZE];

	D(bug("parallel_pipe: getData"));
	if (inhandle < 0 && can_restart)
		reopen();
	if (inhandle >= 0 && (l = BUFSIZE - buffer_size) > BUFSIZE - LO_WATER)
	{
		if ((l = read(inhandle, tmpbuf, l)) < 0)
		{
			D(bug("read %s: %s", device_name, strerror(errno)));
			return 0;
		}
		for (i = 0; i < l; i++, buffer_ptr = (buffer_ptr + 1) & (BUFSIZE - 1))
			buffer[buffer_ptr] = tmpbuf[i];
		buffer_size += l;
	}
	if (buffer_size > 0)
	{
		c = buffer[(buffer_ptr - buffer_size) & (BUFSIZE - 1)];
		buffer_size--;
		return c;
	} else
	{
		D(bug("parallel buffer UNDERRUN!"));
		return -1;
	}
	return 0;
}


void ParallelPipe::setData(uint8 value)
{
	if (outhandle < 0 && can_restart)
		reopen();
	D(bug("parallel_pipe: setData"));
	if (outhandle < 0)
	{
		if (!warned_already)
		{
			if (device_name == NULL)
			{
				D(bug("someone trys to write to %s, but no device was configured", name));
			} else
			{
				D(bug("someone trys to write to %s(%s), but it could not be opened", name, device_name));
			}
			warned_already = true;
		}
	}
	if (outhandle >= 0)
	{
		last_output = SDL_GetTicks();
		if (write(outhandle, &value, 1) < 0)
		{
			D(bug("write %s: %s", device_name, strerror(errno)));
		}
	}
}


uint8 ParallelPipe::getBusy()
{
	if (!Enabled())
		return 0x01;
	D(bug("parallel_pipe: getBusy"));
	if (!direction)
	{
		fd_set x;
		int d;
		struct timeval t;

		if (inhandle < 0 && can_restart)
			reopen();
		if (inhandle < 0)
			return 1;
		if (buffer_size > 0)
			return 0;
		t.tv_sec = 0;
		t.tv_usec = 0;
		FD_ZERO(&x);
		FD_SET(inhandle, &x);
		d = select(inhandle + 1, &x, NULL, NULL, &t);
		return d != 1;
	}
			
	return outhandle < 0 && !can_restart;
}


void ParallelPipe::setStrobe(bool high)
{
	UNUSED(high);
	D(bug("parallel_pipe: setStrobe"));
}
