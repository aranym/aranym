/*
	NatFeat host CD-ROM access

	ARAnyM (C) 2003-2011 Patrice Mandin

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

#ifndef NFCDROM_SDL_H
#define NFCDROM_SDL_H

/*--- Includes ---*/

#include "nfcdrom.h"
#include "SDL_compat.h"

#if !SDL_VERSION_ATLEAST(2, 0, 0)

#include <SDL_cdrom.h>

/*--- Class ---*/

class CdromDriverSdl : public CdromDriver
{
private:
	SDL_CD *drive_handles[CD_MAX_DRIVES];	/* Handle for each possible opened drive */

protected:
	virtual int OpenDrive(memptr device);
	virtual void CloseDrive(int drive);

	virtual int32 cd_status(memptr device, memptr ext_status);
	virtual int32 cd_ioctl(memptr device, uint16 opcode, memptr buffer);

	virtual int32 cd_startaudio(memptr device, uint32 dummy, memptr buffer);
	virtual int32 cd_stopaudio(memptr device);
	virtual int32 cd_setsongtime(memptr device, uint32 dummy, uint32 start_msf, uint32 end_msf);
	virtual int32 cd_gettoc(memptr device, uint32 dummy, memptr buffer);
	virtual int32 cd_discinfo(memptr device, memptr buffer);

public:
	CdromDriverSdl(void);
	virtual ~CdromDriverSdl(void);

	virtual int Count();
	virtual const char *DeviceName(int drive);
};

#endif

#endif /* NFCDROM_SDL_H */
