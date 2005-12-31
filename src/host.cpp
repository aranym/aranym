/*
 * $Header$
 *
 * STanda 2001
 */


#include "sysdeps.h"

#include "hostscreen.h"
#include "host.h"

// host OS dependent objects
HostScreen hostScreen;
Host *host;

Host::Host()
{
	audio = new HostAudio();
}

Host::~Host()
{
	delete audio;
}

HostAudio *Host::getAudio()
{
	return audio;
}

/*
 * $Log$
 * Revision 1.2  2001/10/25 19:56:01  standa
 * The Log and Header CVS tags in the Log removed. Was recursing.
 *
 * Revision 1.1  2001/10/08 21:46:05  standa
 * The Header and Log CVS tags added.
 *
 *
 *
 */
