/*
	Host class
	for host dependent audio, video and clock

	(C) 2007 ARAnyM developer team

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

#include "hostscreen.h"
#include "host.h"

#define DEBUG 0
#include "debug.h"

#define USE_SDL_CLOCK 1		// undefine this if your ARAnyM time goes slower

Host *host;

Host::Host()
{
	D(bug("Host::Host()"));

#ifdef USE_SDL_CLOCK
	clock = new HostClock();
#else
	clock = new HostClockUnix();
#endif
	video = new HostScreen();
}

Host::~Host()
{
	D(bug("Host::~Host()"));

	delete video;
	delete clock;
}

void Host::reset(void)
{
	audio.reset();
	video->reset();
	clock->reset();
}

/*
vim:ts=4:sw=4:
*/
