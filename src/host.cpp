/*
 * $Header$
 *
 * STanda 2001
 */


#include "sysdeps.h"

#include "hostscreen.h"
#include "host.h"

#define DEBUG 0
#include "debug.h"

// host OS dependent objects
HostScreen hostScreen;
Host *host;

Host::Host()
{
	D(bug("Host::Host()"));
}

Host::~Host()
{
	D(bug("Host::~Host()"));
}

void Host::reset(void)
{
	audio.reset();
}

/*
 * $Log$
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
