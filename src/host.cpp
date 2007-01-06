/*
 * $Header$
 *
 * STanda 2001
 */


#include "sysdeps.h"

#include "hostscreen.h"
#include "host.h"
#include "dyngl.h"

#define DEBUG 0
#include "debug.h"

#define USE_SDL_CLOCK 1		// undefine this if your ARAnyM time goes slower

// host OS dependent objects
HostScreen hostScreen;
Host *host;

Host::Host()
{
	D(bug("Host::Host()"));

#ifdef USE_SDL_CLOCK
	clock = new HostClock();
#else
	clock = new HostClockUnix();
#endif

#ifdef ENABLE_OPENGL
	if (dyngl_load(bx_options.opengl.library)==0) {
		bx_options.opengl.enabled = false;
	}
#else
	bx_options.opengl.enabled = false;
#endif
}

Host::~Host()
{
	D(bug("Host::~Host()"));

	delete clock;
}

void Host::reset(void)
{
	audio.reset();
	clock->reset();
}

/*
 * $Log$
 * Revision 1.6  2006/02/03 21:21:34  pmandin
 * Created host clock class to encapsulate OS-dependent time functions
 *
 * Revision 1.5  2006/01/26 18:53:53  pmandin
 * HostAudio object now statically created
 *
 * Revision 1.4  2006/01/23 18:27:19  pmandin
 * Add reset method for host stuff
 *
 * Revision 1.3  2005/12/31 09:07:43  pmandin
 * Created a Host class, put HostAudio in it
 *
 * Revision 1.2  2001/10/25 19:56:01  standa
 * The Log and Header CVS tags in the Log removed. Was recursing.
 *
 * Revision 1.1  2001/10/08 21:46:05  standa
 * The Header and Log CVS tags added.
 *
 *
 *
 */
