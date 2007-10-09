/*
 * host.cpp
 *
 * Copyright (c) 2001-2007 Stanislav Opichal of ARAnyM dev team (see AUTHORS)
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
 * along with ARAnyM; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "sysdeps.h"

#include "hostscreen.h"
#include "host.h"
#ifdef ENABLE_OPENGL
#  include "dyngl.h"
#endif

#define DEBUG 0
#include "debug.h"

#define USE_SDL_CLOCK 1		// undefine this if your ARAnyM time goes slower

// host OS dependent objects
//HostScreen hostScreen;
Host *host;

Host::Host()
{
	D(bug("Host::Host()"));

#ifdef USE_SDL_CLOCK
	clock = new HostClock();
#else
	clock = new HostClockUnix();
#endif
}

Host::~Host()
{
	D(bug("Host::~Host()"));

	delete clock;
}

void Host::reset(void)
{
	/* TODO: should also reset hostscreen */
	audio.reset();
	clock->reset();
}

/*
vim:ts=4:sw=4:
*/
