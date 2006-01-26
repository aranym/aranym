/*
 * $Header$
 *
 * The Host OS dependent objects.
 */

#ifndef HOST_H
#define HOST_H

#include "hostscreen.h"
#include "host_audio.h"
#include "host_filesys.h"

class Host : public HostFilesys
{
	public:
		Host();
		~Host();
		void reset(void);

		HostAudio audio;
};

extern HostScreen hostScreen;
extern Host *host;

#endif /* HOST_H */

/*
 * $Log$
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
