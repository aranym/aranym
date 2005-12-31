/*
 * $Header$
 *
 * The Host OS dependent objects.
 */

#ifndef HOST_H
#define HOST_H

#include "hostscreen.h"
#include "host_audio.h"

class Host {
	private:
		HostAudio *audio;
		
	public:
		Host();
		~Host();
		
		HostAudio *getAudio();
};

extern HostScreen hostScreen;
extern Host *host;

#endif /* HOST_H */

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
